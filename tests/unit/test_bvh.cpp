// Tests de la construction BVH (T060), Catch2.
// DoD : construire une BVH sur 1000 objets synthetiques < 50 ms ;
// noeuds bornes (`nbNodes <= 2N-1`). Sans `throw` (R2), buffer prealloue
// (R3, `sizeof(BvhNode) == 32` verifie a la compilation dans le header).
// Traversal (T061) : voir les cas `[t061]` en fin de fichier (equivalence
// brute-force sur 200 scenes aleatoires, pile fixe, Williams).

#include <catch2/catch_amalgamated.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include "rt/accel/Bvh.hpp"
#include "rt/base/Mat4.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"
#include "rt/geometry/Sphere.hpp"

namespace {

std::vector<std::unique_ptr<rt::geometry::AObject>> makeGridSpheres(std::size_t count) {
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.reserve(count);
	for (std::size_t i = 0; i < count; ++i) {
		const float x = static_cast<float>(i % 10) * 2.0F;
		const float y = static_cast<float>((i / 10) % 10) * 2.0F;
		const float z = static_cast<float>(i / 100) * 2.0F;
		objs.push_back(std::make_unique<rt::geometry::Sphere>(
		    rt::Vec3(x, y, z), 0.5F, static_cast<std::uint32_t>(i),
		    static_cast<std::uint32_t>(i)));
	}
	return objs;
}

// Profondeur max de l'arbre via les index (pile, sans allocation).
std::size_t treeDepth(const rt::accel::Bvh& bvh, std::uint32_t node) {
	if (node == rt::accel::Bvh::kInvalid || bvh.empty()) {
		return 0;
	}
	const rt::accel::BvhNode& current = bvh.nodes().at(node);
	if (current.isLeaf()) {
		return 1;
	}
	const std::size_t left = treeDepth(bvh, current.left());
	const std::size_t right = treeDepth(bvh, current.right());
	return 1 + (left > right ? left : right);
}

} // namespace

TEST_CASE("bvh : arbre vide et singleton (T060)", "[bvh][t060]") {
	rt::accel::Bvh bvh;
	std::vector<std::unique_ptr<rt::geometry::AObject>> empty;
	REQUIRE(bvh.build(empty).isOk());
	REQUIRE(bvh.empty());
	REQUIRE(bvh.nodeCount() == 0);
	REQUIRE(bvh.root() == rt::accel::Bvh::kInvalid);
	REQUIRE(bvh.primCount() == 0);

	auto single = makeGridSpheres(1);
	REQUIRE(bvh.build(single).isOk());
	REQUIRE(!bvh.empty());
	REQUIRE(bvh.nodeCount() == 1);
	REQUIRE(bvh.nodeCount() <= 2 * 1 - 1);
	REQUIRE(bvh.root() == 0);
	const rt::accel::BvhNode& leaf = bvh.nodes().at(0);
	REQUIRE(leaf.isLeaf());
	REQUIRE(leaf.leafCount() == 1);
	REQUIRE(treeDepth(bvh, bvh.root()) <= static_cast<std::size_t>(rt::accel::Bvh::kMaxDepth));
}

TEST_CASE("bvh : 1000 spheres < 50 ms et noeuds <= 2N-1 (T060 DoD)", "[bvh][t060]") {
	auto objs = makeGridSpheres(1000);
	rt::accel::Bvh bvh;
	const auto before = std::chrono::steady_clock::now();
	REQUIRE(bvh.build(objs).isOk());
	const auto after = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before);
	INFO("build 1000 spheres : " << elapsed.count() << " ms, " << bvh.nodeCount() << " noeuds");
	REQUIRE(elapsed.count() < 50);
	REQUIRE(bvh.primCount() == 1000);
	REQUIRE(bvh.nodeCount() >= 1);
	REQUIRE(bvh.nodeCount() <= 2 * 1000 - 1);
	REQUIRE(treeDepth(bvh, bvh.root()) <= static_cast<std::size_t>(rt::accel::Bvh::kMaxDepth));
	// Les feuilles partitionnent les 1000 primitives (somme == N).
	std::size_t leafTotal = 0;
	for (const rt::accel::BvhNode& node : bvh.nodes()) {
		if (node.isLeaf()) {
			leafTotal += node.leafCount();
			REQUIRE(node.leafStart() + node.leafCount() <= 1000);
		} else {
			REQUIRE(node.left() < bvh.nodeCount());
			REQUIRE(node.right() < bvh.nodeCount());
		}
	}
	REQUIRE(leafTotal == 1000);
}

TEST_CASE("bvh : worldBounds suit la transformation (T060)", "[bvh][t060]") {
	rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), 1.0F);
	const rt::AABB local = rt::accel::Bvh::worldBounds(sphere);
	REQUIRE(local.min.x == Catch::Approx(-1.0F));
	REQUIRE(local.max.x == Catch::Approx(1.0F));

	rt::Transform moved = rt::Transform::translate(rt::Vec3(42.0F, 0.0F, 0.0F));
	sphere.setTransform(moved);
	const rt::AABB world = rt::accel::Bvh::worldBounds(sphere);
	REQUIRE(world.min.x == Catch::Approx(41.0F));
	REQUIRE(world.max.x == Catch::Approx(43.0F));
	// La boite racine d'une BVH a 2 spheres contient les deux.
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.push_back(std::make_unique<rt::geometry::Sphere>(rt::Vec3(-5, 0, 0), 1.0F));
	objs.push_back(std::make_unique<rt::geometry::Sphere>(rt::Vec3(5, 0, 0), 1.0F));
	rt::accel::Bvh bvh;
	REQUIRE(bvh.build(objs).isOk());
	const rt::accel::BvhNode& root = bvh.nodes().at(bvh.root());
	REQUIRE(root.bounds.min.x <= -6.0F);
	REQUIRE(root.bounds.max.x >= 6.0F);
}

TEST_CASE("bvh : pointeur nul refuse sans crash (T060)", "[bvh][t060]") {
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.push_back(nullptr);
	rt::accel::Bvh bvh;
	REQUIRE(bvh.build(objs).isError());
}

namespace {

// Brute-force de reference (miroir de `findClosestHit`, T046) : le plus
// proche dans `[tMin, tMax]`, `tMax` resserre. Sert l'equivalence T061.
bool linearHit(const std::vector<std::unique_ptr<rt::geometry::AObject>>& objs,
               const rt::Ray& ray, rt::Real tMin, rt::Real tMax, rt::HitRecord& out) noexcept {
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

// Direction uniforme sur la sphere (rejet, graine fixee par l'appelant).
rt::Vec3 uniformDirection(std::mt19937& rng) {
	std::uniform_real_distribution<float> dist(-1.0F, 1.0F);
	for (int attempt = 0; attempt < 100; ++attempt) {
		const rt::Vec3 candidate(dist(rng), dist(rng), dist(rng));
		const float lenSq = rt::dot(candidate, candidate);
		if (lenSq > 1e-6F && lenSq <= 1.0F) {
			return rt::normalize(candidate);
		}
	}
	return rt::Vec3(0.0F, 0.0F, 1.0F);
}

} // namespace

TEST_CASE("bvh : traverse identique au brute-force sur 200 scenes aleatoires (T061 DoD)",
          "[bvh][t061]") {
	std::mt19937 rng(12345);
	std::uniform_real_distribution<float> centerDist(-5.0F, 5.0F);
	std::uniform_real_distribution<float> radiusDist(0.3F, 1.5F);
	std::uniform_real_distribution<float> originDist(-8.0F, 8.0F);
	constexpr rt::Real tMin = 0.001F;
	for (int scene = 0; scene < 200; ++scene) {
		const std::size_t count = static_cast<std::size_t>(1 + (rng() % 16));
		std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
		objs.reserve(count);
		for (std::size_t i = 0; i < count; ++i) {
			objs.push_back(std::make_unique<rt::geometry::Sphere>(
			    rt::Vec3(centerDist(rng), centerDist(rng), centerDist(rng)), radiusDist(rng),
			    static_cast<std::uint32_t>(i), static_cast<std::uint32_t>(i)));
		}
		rt::accel::Bvh bvh;
		REQUIRE(bvh.build(objs).isOk());
		for (int shot = 0; shot < 20; ++shot) {
			const rt::Ray ray(rt::Vec3(originDist(rng), originDist(rng), originDist(rng)),
			                  uniformDirection(rng));
			rt::HitRecord expected;
			rt::HitRecord actual;
			const bool hitLinear = linearHit(objs, ray, tMin, rt::kInfinity, expected);
			const bool hitBvh = bvh.traverse(ray, tMin, rt::kInfinity, actual, objs);
			REQUIRE(hitBvh == hitLinear);
			if (hitLinear) {
				REQUIRE(actual.t == Catch::Approx(expected.t).epsilon(1e-4));
				REQUIRE(actual.materialIndex == expected.materialIndex);
			}
		}
	}
}

TEST_CASE("bvh : traverse sur 1000 spheres et rayons axes (T061)", "[bvh][t061]") {
	auto objs = makeGridSpheres(1000);
	rt::accel::Bvh bvh;
	REQUIRE(bvh.build(objs).isOk());
	constexpr rt::Real tMin = 0.001F;
	// Rayons alignes aux axes (cas Williams : directions a composantes nulles).
	const rt::Ray axisRays[] = {
	    rt::Ray(rt::Vec3(9.0F, 0.0F, 0.0F), rt::Vec3(-1.0F, 0.0F, 0.0F)),
	    rt::Ray(rt::Vec3(0.0F, 9.0F, 0.0F), rt::Vec3(0.0F, -1.0F, 0.0F)),
	    rt::Ray(rt::Vec3(0.0F, 0.0F, 9.0F), rt::Vec3(0.0F, 0.0F, -1.0F)),
	    rt::Ray(rt::Vec3(0.0F, 0.0F, -9.0F), rt::Vec3(0.0F, 0.0F, 1.0F)),
	    rt::Ray(rt::Vec3(50.0F, 50.0F, 50.0F), rt::Vec3(0.0F, 0.0F, 1.0F)), // manque tout
	};
	for (const rt::Ray& ray : axisRays) {
		rt::HitRecord expected;
		rt::HitRecord actual;
		const bool hitLinear = linearHit(objs, ray, tMin, rt::kInfinity, expected);
		const bool hitBvh = bvh.traverse(ray, tMin, rt::kInfinity, actual, objs);
		REQUIRE(hitBvh == hitLinear);
		if (hitLinear) {
			REQUIRE(actual.t == Catch::Approx(expected.t).epsilon(1e-4));
		}
	}
	// 100 rayons aleatoires sur le grand arbre (profondeur reelle).
	std::mt19937 rng(777);
	std::uniform_real_distribution<float> originDist(-4.0F, 22.0F);
	for (int shot = 0; shot < 100; ++shot) {
		const rt::Ray ray(rt::Vec3(originDist(rng), originDist(rng), originDist(rng)),
		                  uniformDirection(rng));
		rt::HitRecord expected;
		rt::HitRecord actual;
		const bool hitLinear = linearHit(objs, ray, tMin, rt::kInfinity, expected);
		const bool hitBvh = bvh.traverse(ray, tMin, rt::kInfinity, actual, objs);
		REQUIRE(hitBvh == hitLinear);
		if (hitLinear) {
			REQUIRE(actual.t == Catch::Approx(expected.t).epsilon(1e-4));
		}
	}
}

TEST_CASE("bvh : traverse degeneree definie (T061)", "[bvh][t061]") {
	rt::accel::Bvh empty;
	std::vector<std::unique_ptr<rt::geometry::AObject>> noObjs;
	rt::HitRecord rec;
	// Arbre vide : miss defini (rec intouche, comme `findClosestHit`).
	const rt::Ray ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1));
	REQUIRE(!empty.traverse(ray, 0.001F, rt::kInfinity, rec, noObjs));
	// Direction nulle : meme reponse que le brute-force (miss defini).
	auto objs = makeGridSpheres(8);
	rt::accel::Bvh bvh;
	REQUIRE(bvh.build(objs).isOk());
	const rt::Ray nullDir(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 0));
	rt::HitRecord expected;
	rt::HitRecord actual;
	REQUIRE(linearHit(objs, nullDir, 0.001F, rt::kInfinity, expected) ==
	        bvh.traverse(nullDir, 0.001F, rt::kInfinity, actual, objs));
	// Vecteur de taille differente : miss defini, jamais de crash.
	std::vector<std::unique_ptr<rt::geometry::AObject>> other = makeGridSpheres(3);
	REQUIRE(!bvh.traverse(ray, 0.001F, rt::kInfinity, rec, other));
	// Fenetre degeneree : miss defini des deux cotes.
	REQUIRE(!linearHit(objs, ray, 5.0F, 1.0F, expected));
	REQUIRE(!bvh.traverse(ray, 5.0F, 1.0F, actual, objs));
}
