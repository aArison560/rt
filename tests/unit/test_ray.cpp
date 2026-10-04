// Tests de rt::Ray / Interval / AABB / HitRecord (T013). Programme autonome
// (Catch2 raccordé en T017) : renvoie ≠ 0 si un contrôle échoue.

#include <array>
#include <cstddef>
#include <iostream>
#include <type_traits>

#include "rt/base/Ray.hpp"
#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
	if (!(cond)) {                                                                             \
	    ++g_failures;                                                                          \
	    std::cerr << "FAIL line " << __LINE__ << ": " #cond << '\n';                           \
	}                                                                                          \
    } while (false)

#define CHECK_NEAR(a, b) CHECK((a) > (b) ? (a) - (b) < 1e-4f : (b) - (a) < 1e-4f)

} // namespace

int main() {
    using namespace rt;

    // --- Ray ---
    const Ray r(Vec3(0, 0, 0), Vec3(0, 0, 1));
    const Vec3 p = r.at(Real(2.5f));
    CHECK_NEAR(p.x, Real(0));
    CHECK_NEAR(p.y, Real(0));
    CHECK_NEAR(p.z, Real(2.5f));
    CHECK(Ray(Vec3(), Vec3(1, 0, 0), 3).depth == 3);

    // --- Interval ---
    const Interval iv(Real(1), Real(5));
    CHECK(iv.contains(Real(1)));
    CHECK(iv.contains(Real(5)));
    CHECK(!iv.contains(Real(0.9f)));
    CHECK(iv.surrounds(Real(3)));
    CHECK(!iv.surrounds(Real(1)));
    CHECK_NEAR(iv.clamp(Real(10)), Real(5));
    CHECK_NEAR(iv.clamp(Real(-2)), Real(1));
    CHECK(Interval::empty().size() < Real(0)); // min > max : intervalle vide
    CHECK(Interval::universe().contains(Real(0)));
    const Interval merged = Interval(Real(0), Real(2)).merged(Interval(Real(1), Real(9)));
    CHECK_NEAR(merged.tMin, Real(0));
    CHECK_NEAR(merged.tMax, Real(9));

    // --- AABB : hit depuis les 6 côtés d'une boîte unité centrée à l'origine ---
    const AABB box(Vec3(-1, -1, -1), Vec3(1, 1, 1));
    const Interval full = {Real(0), kInfinity};
    const std::array<Vec3, 6> dirs = {
        {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
    const std::array<Vec3, 6> origs = {
        {{-5, 0, 0}, {5, 0, 0}, {0, -5, 0}, {0, 5, 0}, {0, 0, -5}, {0, 0, 5}}};
    for (std::size_t i = 0; i < dirs.size(); ++i) {
	const Ray ray(origs[i], normalize(dirs[i]));
	CHECK(box.hit(ray, full));
    }
    // Manqué de chaque côté.
    for (std::size_t i = 0; i < dirs.size(); ++i) {
	Vec3 o = origs[i];
	o.x += Real(5); // décalé en x ET en y : le rayon passe à côté
	o.y += Real(5);
	const Ray ray(o, normalize(dirs[i]));
	CHECK(!box.hit(ray, full));
    }
    // Origine à l'intérieur, rayon vers dehors.
    CHECK(box.hit(Ray(Vec3(0, 0, 0), Vec3(0, 0, 1)), full));
    // Rayon quasi parallèle à un axe, hors des dalles.
    CHECK(!box.hit(Ray(Vec3(0, 3, 0), Vec3(kEpsilon * 0.1f, Real(0), Real(1))), full));
    // AABB tMax < tMin après slabs → miss.
    CHECK(!box.hit(Ray(Vec3(-5, 0, 0), Vec3(-1, 0, 0)), full)); // s'éloigne

    // Union et padding.
    const AABB grown = box.merged(AABB(Vec3(3, 0, 0), Vec3(4, 1, 1)));
    CHECK_NEAR(grown.min.x, Real(-1));
    CHECK_NEAR(grown.max.x, Real(4));
    const AABB fat = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0)).padded(Real(0.5f));
    CHECK_NEAR(fat.max.x - fat.min.x, Real(1)); // boîte dégénérée épaissie

    // --- HitRecord / frontFace ---
    HitRecord rec;
    const Ray outside(Vec3(0, 0, -5), Vec3(0, 0, 1));
    rec.setFaceNormal(outside, Vec3(0, 0, -1)); // normale sortante de la sphère
    CHECK(rec.frontFace);
    CHECK_NEAR(rec.normal.z, Real(-1));
    const Ray inside(Vec3(0, 0, 0), Vec3(0, 0, 1));
    HitRecord rec2;
    rec2.setFaceNormal(inside, Vec3(0, 0, 1));
    CHECK(!rec2.frontFace);
    CHECK_NEAR(rec2.normal.z, Real(-1)); // inversée : contre le rayon

    // POD compacts.
    CHECK(std::is_trivially_copyable_v<Ray>);
    CHECK(std::is_trivially_copyable_v<AABB>);
    CHECK(std::is_trivially_copyable_v<HitRecord>);

    if (g_failures != 0) {
	std::cerr << g_failures << " failure(s)\n";
	return 1;
    }
    std::cout << "test_ray: OK\n";
    return 0;
}
