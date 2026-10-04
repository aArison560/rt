// Tests de rt::Ray / Interval / AABB / HitRecord (T013), Catch2 (T017).

#include <catch2/catch_amalgamated.hpp>

#include <array>
#include <cstddef>
#include <type_traits>

#include "rt/base/Ray.hpp"
#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"

namespace {

// Comparaison flottante à 1e-4 près (même tolérance que l'ancien harnais).
bool near(rt::Real a, rt::Real b) { return a == Catch::Approx(b).margin(1e-4); }

} // namespace

TEST_CASE("Ray::at évalue l'origine + t * direction", "[ray]") {
    using namespace rt;

    const Ray r(Vec3(0, 0, 0), Vec3(0, 0, 1));
    const Vec3 p = r.at(Real(2.5f));
    REQUIRE(near(p.x, Real(0)));
    REQUIRE(near(p.y, Real(0)));
    REQUIRE(near(p.z, Real(2.5f)));
    REQUIRE(Ray(Vec3(), Vec3(1, 0, 0), 3).depth == 3);
}

TEST_CASE("Interval : contains, surrounds, clamp", "[ray]") {
    using namespace rt;

    const Interval iv(Real(1), Real(5));
    REQUIRE(iv.contains(Real(1)));
    REQUIRE(iv.contains(Real(5)));
    REQUIRE_FALSE(iv.contains(Real(0.9f)));
    REQUIRE(iv.surrounds(Real(3)));
    REQUIRE_FALSE(iv.surrounds(Real(1)));
    REQUIRE(near(iv.clamp(Real(10)), Real(5)));
    REQUIRE(near(iv.clamp(Real(-2)), Real(1)));
    REQUIRE(Interval::empty().size() < Real(0)); // min > max : intervalle vide
    REQUIRE(Interval::universe().contains(Real(0)));
    const Interval merged = Interval(Real(0), Real(2)).merged(Interval(Real(1), Real(9)));
    REQUIRE(near(merged.tMin, Real(0)));
    REQUIRE(near(merged.tMax, Real(9)));
}

TEST_CASE("AABB : hit depuis les 6 côtés d'une boîte unité", "[ray]") {
    using namespace rt;

    const AABB box(Vec3(-1, -1, -1), Vec3(1, 1, 1));
    const Interval full = {Real(0), kInfinity};
    const std::array<Vec3, 6> dirs = {
        {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
    const std::array<Vec3, 6> origs = {
        {{-5, 0, 0}, {5, 0, 0}, {0, -5, 0}, {0, 5, 0}, {0, 0, -5}, {0, 0, 5}}};
    for (std::size_t i = 0; i < dirs.size(); ++i) {
	const Ray ray(origs[i], normalize(dirs[i]));
	REQUIRE(box.hit(ray, full));
    }
    // Manqué de chaque côté.
    for (std::size_t i = 0; i < dirs.size(); ++i) {
	Vec3 o = origs[i];
	o.x += Real(5); // décalé en x ET en y : le rayon passe à côté
	o.y += Real(5);
	const Ray ray(o, normalize(dirs[i]));
	REQUIRE_FALSE(box.hit(ray, full));
    }
    // Origine à l'intérieur, rayon vers dehors.
    REQUIRE(box.hit(Ray(Vec3(0, 0, 0), Vec3(0, 0, 1)), full));
    // Rayon quasi parallèle à un axe, hors des dalles.
    REQUIRE_FALSE(box.hit(Ray(Vec3(0, 3, 0), Vec3(kEpsilon * 0.1f, Real(0), Real(1))), full));
    // AABB tMax < tMin après slabs → miss.
    REQUIRE_FALSE(box.hit(Ray(Vec3(-5, 0, 0), Vec3(-1, 0, 0)), full)); // s'éloigne
}

TEST_CASE("AABB : union et padding", "[ray]") {
    using namespace rt;

    const AABB box(Vec3(-1, -1, -1), Vec3(1, 1, 1));
    const AABB grown = box.merged(AABB(Vec3(3, 0, 0), Vec3(4, 1, 1)));
    REQUIRE(near(grown.min.x, Real(-1)));
    REQUIRE(near(grown.max.x, Real(4)));
    const AABB fat = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0)).padded(Real(0.5f));
    REQUIRE(near(fat.max.x - fat.min.x, Real(1))); // boîte dégénérée épaissie
}

TEST_CASE("HitRecord : frontFace oriente la normale contre le rayon", "[ray]") {
    using namespace rt;

    HitRecord rec;
    const Ray outside(Vec3(0, 0, -5), Vec3(0, 0, 1));
    rec.setFaceNormal(outside, Vec3(0, 0, -1)); // normale sortante de la sphère
    REQUIRE(rec.frontFace);
    REQUIRE(near(rec.normal.z, Real(-1)));
    const Ray inside(Vec3(0, 0, 0), Vec3(0, 0, 1));
    HitRecord rec2;
    rec2.setFaceNormal(inside, Vec3(0, 0, 1));
    REQUIRE_FALSE(rec2.frontFace);
    REQUIRE(near(rec2.normal.z, Real(-1))); // inversée : contre le rayon
}

TEST_CASE("Ray, AABB et HitRecord sont des POD compacts", "[ray]") {
    using namespace rt;

    REQUIRE(std::is_trivially_copyable_v<Ray>);
    REQUIRE(std::is_trivially_copyable_v<AABB>);
    REQUIRE(std::is_trivially_copyable_v<HitRecord>);
}
