#!/bin/sh
# scripts/bench_bvh.sh — micro-bench BVH traverse vs brute-force (T061).
#
# Usage : sh scripts/bench_bvh.sh <linear|traverse> [spheres] [rays]
#   linear    : recherche lineaire (miroir de `findClosestHit`, T046)
#   traverse  : `Bvh::traverse()` (pile fixe + Williams, T061)
#   spheres   : nombre de spheres en grille (defaut 1000, DoD T060)
#   rays      : nombre de rayons deterministes (defaut 2000)
#
# La source C++ est incluse ci-dessous (heredoc) : le binaire est construit
# dans /tmp (chemin froid, hors chronometrage) puis execute une passe.
# Graine fixee (LCG 0x12345678) : les deux modes tirent les MEMES rayons.
# Sert `scripts/bench.sh --cmd` pour l'avant/apres dans docs/BENCH.md :
#   sh scripts/bench.sh --cmd "sh scripts/bench_bvh.sh linear" \
#       --label t061-bvh-linear --runs 5 \
#       --note "T061 avant : brute-force, 1000 spheres x 2000 rayons"
#   sh scripts/bench.sh --cmd "sh scripts/bench_bvh.sh traverse" \
#       --label t061-bvh-traverse --runs 5 \
#       --note "T061 apres : BVH pile fixe + Williams, memes rayons"

set -u

MODE=${1:?usage: sh scripts/bench_bvh.sh '<linear|traverse>' [spheres] [rays]}
SPHERES=${2:-1000}
RAYS=${3:-2000}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

SRC=/tmp/bvh_cmp.cpp
BIN=/tmp/bvh_cmp

case "$MODE" in
linear | traverse) ;;
*) echo "bench_bvh : mode '$MODE' inconnu (linear|traverse)" >&2; exit 2 ;;
esac

case "$SPHERES $RAYS" in
'' | *[!0-9\ ]* | *'  '*) echo "bench_bvh : spheres/rays doivent etre des entiers" >&2; exit 2 ;;
esac

# Reconstruit le binaire s'il est plus vieux que ce script ou les sources.
REBUILD=0
if [ ! -x "$BIN" ]; then
	REBUILD=1
elif [ "$0" -nt "$BIN" ]; then
	REBUILD=1
else
	for f in "$ROOT_DIR/src/accel/Bvh.cpp" "$ROOT_DIR/src/geometry/Object.cpp" \
	    "$ROOT_DIR/src/geometry/Sphere.cpp" "$ROOT_DIR/src/geometry/Plane.cpp" \
	    "$ROOT_DIR/src/geometry/Cylinder.cpp" "$ROOT_DIR/src/geometry/Cone.cpp" \
	    "$ROOT_DIR/include/rt/accel/Bvh.hpp"; do
		if [ "$f" -nt "$BIN" ]; then
			REBUILD=1
			break
		fi
	done
fi

if [ "$REBUILD" -eq 1 ]; then
	cat >"$SRC" <<'EOF'
// Micro-bench T061 : brute-force vs Bvh::traverse (memes rayons, graine fixe).
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "rt/accel/Bvh.hpp"
#include "rt/geometry/Sphere.hpp"

namespace {

std::uint32_t g_state = 0x12345678u;

std::uint32_t nextUint() {
	g_state = g_state * 1664525u + 1013904223u;
	return g_state;
}

float nextFloat(float lo, float hi) {
	const float unit = static_cast<float>(nextUint() >> 8) * (1.0F / 16777216.0F);
	return lo + (hi - lo) * unit;
}

bool linearHit(const std::vector<std::unique_ptr<rt::geometry::AObject>>& objs,
               const rt::Ray& ray, rt::Real tMin, rt::Real tMax, rt::HitRecord& out) {
	bool hit = false;
	rt::Real closest = tMax;
	rt::HitRecord tmp;
	for (const std::unique_ptr<rt::geometry::AObject>& obj : objs) {
		if (obj && obj->intersect(ray, tMin, closest, tmp)) {
			out = tmp;
			closest = tmp.t;
			hit = true;
		}
	}
	return hit;
}

} // namespace

int main(int argc, char** argv) {
	if (argc != 4) {
		std::printf("usage: bvh_cmp <linear|traverse> <spheres> <rays>\n");
		return 2;
	}
	const bool useBvh = argv[1][0] == 't';
	const int sphereCount = std::atoi(argv[2]);
	const int rayCount = std::atoi(argv[3]);
	if (sphereCount < 1 || rayCount < 1) {
		return 2;
	}
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.reserve(static_cast<std::size_t>(sphereCount));
	for (int i = 0; i < sphereCount; ++i) {
		const float x = static_cast<float>(i % 10) * 2.0F;
		const float y = static_cast<float>((i / 10) % 10) * 2.0F;
		const float z = static_cast<float>(i / 100) * 2.0F;
		objs.push_back(std::make_unique<rt::geometry::Sphere>(
		    rt::Vec3(x, y, z), 0.5F, static_cast<std::uint32_t>(i),
		    static_cast<std::uint32_t>(i)));
	}
	rt::accel::Bvh bvh;
	if (useBvh && bvh.build(objs).isError()) {
		return 1;
	}
	std::vector<rt::Ray> rays;
	rays.reserve(static_cast<std::size_t>(rayCount));
	for (int i = 0; i < rayCount; ++i) {
		const rt::Vec3 origin(nextFloat(-4.0F, 22.0F), nextFloat(-4.0F, 22.0F),
		                      nextFloat(-4.0F, 22.0F));
		rt::Vec3 dir(0.0F, 0.0F, 1.0F);
		for (int attempt = 0; attempt < 100; ++attempt) {
			const rt::Vec3 candidate(nextFloat(-1.0F, 1.0F), nextFloat(-1.0F, 1.0F),
			                         nextFloat(-1.0F, 1.0F));
			const float lenSq = rt::dot(candidate, candidate);
			if (lenSq > 1e-6F && lenSq <= 1.0F) {
				dir = rt::normalize(candidate);
				break;
			}
		}
		rays.emplace_back(origin, dir);
	}
	double checksum = 0.0;
	long hits = 0;
	for (const rt::Ray& ray : rays) {
		rt::HitRecord rec;
		const bool hit = useBvh ? bvh.traverse(ray, 0.001F, rt::kInfinity, rec, objs)
		                        : linearHit(objs, ray, 0.001F, rt::kInfinity, rec);
		if (hit) {
			checksum += static_cast<double>(rec.t);
			++hits;
		}
	}
	std::printf("%s spheres=%d rays=%d hits=%ld checksum=%.6f\n", useBvh ? "traverse" : "linear",
	            sphereCount, rayCount, hits, checksum);
	return 0;
}
EOF
	c++ -Wall -Wextra -Werror -O2 -std=c++2c -I"$ROOT_DIR/include" "$SRC" \
	    "$ROOT_DIR/src/accel/Bvh.cpp" "$ROOT_DIR/src/geometry/Object.cpp" \
	    "$ROOT_DIR/src/geometry/Sphere.cpp" "$ROOT_DIR/src/geometry/Plane.cpp" \
	    "$ROOT_DIR/src/geometry/Cylinder.cpp" "$ROOT_DIR/src/geometry/Cone.cpp" \
	    -o "$BIN" || exit 1
fi

exec "$BIN" "$MODE" "$SPHERES" "$RAYS"
