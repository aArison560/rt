// Tests de rt::Mat4 / rt::Transform (T012). Programme autonome (Catch2 en T017) :
// renvoie un code ≠ 0 si un contrôle échoue.

#include <iostream>
#include <optional>
#include <type_traits>

#include "rt/base/Mat4.hpp"
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

#define CHECK_NEAR(a, b) CHECK(rt::almostEqual((a), (b), rt::Real(1e-5f)))

} // namespace

int main() {
    using namespace rt;

    // Identité : transformPoint/Vector/Normal inchangés.
    const Mat4 id = Mat4::identity();
    const Vec3 p = transformPoint(id, Vec3(Real(1), Real(2), Real(3)));
    CHECK(p[0] == Real(1) && p[1] == Real(2) && p[2] == Real(3));
    const Vec3 v = transformVector(id, Vec3(Real(0), Real(1), Real(0)));
    CHECK(v[1] == Real(1));
    const Vec3 n0 = transformNormal(id, Vec3(Real(0), Real(1), Real(0)));
    CHECK_NEAR(n0[1], Real(1));

    // Inverse × matrice ≈ identité (tolérance 1e-5).
    const Transform t = Transform::translate(Vec3(Real(2), Real(-3), Real(7)))
                            .compose(Transform::rotateY(degreesToRadians(Real(35))))
                            .compose(Transform::scale(Vec3(Real(2), Real(0.5f), Real(3))));
    const Mat4 a = t.matrix;
    const std::optional<Mat4> inv = a.inverse();
    CHECK(inv.has_value());
    const Mat4 prod = inv.has_value() ? a * inv.value() : Mat4();
    for (int i = 0; i < 4; ++i) {
	for (int j = 0; j < 4; ++j) {
	    CHECK_NEAR(prod(i, j), i == j ? Real(1) : Real(0));
	}
    }
    const Mat4 prod2 = inv.has_value() ? inv.value() * a : Mat4();
    for (int i = 0; i < 4; ++i) {
	for (int j = 0; j < 4; ++j) {
	    CHECK_NEAR(prod2(i, j), i == j ? Real(1) : Real(0));
	}
    }

    // Matrice singulière → nullopt, jamais de throw.
    const Mat4 sing; // matrice nulle : déterminant 0
    const std::optional<Mat4> invSing = sing.inverse();
    CHECK(!invSing.has_value());

    // Translation : le point est déplacé, le vecteur ne l'est pas.
    const Transform tr = Transform::translate(Vec3(Real(42), Real(42), Real(42)));
    const Vec3 pt = tr.applyPoint(Vec3(Real(0), Real(0), Real(0)));
    CHECK_NEAR(pt[0], Real(42));
    CHECK_NEAR(pt[1], Real(42));
    CHECK_NEAR(pt[2], Real(42));
    const Vec3 tv = tr.applyVector(Vec3(Real(1), Real(0), Real(0)));
    CHECK_NEAR(tv[0], Real(1));
    CHECK_NEAR(tv[1], Real(0));

    // Composition : scale puis translate ≠ translate puis scale.
    const Transform sc = Transform::scale(Vec3(Real(2), Real(2), Real(2)));
    const Vec3 tp = Transform::translate(Vec3(Real(10), Real(0), Real(0)))
                        .compose(sc)
                        .applyPoint(Vec3(Real(1), Real(0), Real(0)));
    CHECK_NEAR(tp[0], Real(12)); // translation après scale : 10 + 2*1
    const Vec3 ps = sc.compose(Transform::translate(Vec3(Real(10), Real(0), Real(0))))
                        .applyPoint(Vec3(Real(1), Real(0), Real(0)));
    CHECK_NEAR(ps[0], Real(22)); // scale après translation : 2*(1+10)

    // Rotation Z de 90° : +X → +Y.
    const Transform rz = Transform::rotateZ(kPi / Real(2));
    const Vec3 ru = rz.applyVector(Vec3(Real(1), Real(0), Real(0)));
    CHECK_NEAR(ru[0], Real(0));
    CHECK_NEAR(ru[1], Real(1));

    // Normale après rotation non uniforme + scale : reste unitaire et
    // perpendiculaire à la tangente transformée (inverse-transposée).
    const Transform nonUniform = Transform::scale(Vec3(Real(4), Real(1), Real(1)))
                                     .compose(Transform::rotateY(degreesToRadians(Real(30))));
    const Vec3 tangent = normalize(Vec3(Real(1), Real(1), Real(0))); // direction quelconque
    const Vec3 normal = normalize(Vec3(Real(1), Real(-1), Real(0))); // ⊥ tangente avant
    const Vec3 tT = normalize(nonUniform.applyVector(tangent));
    const Vec3 tN = nonUniform.applyNormal(normal);
    CHECK_NEAR(length(tN), Real(1));
    CHECK_NEAR(dot(tT, tN), Real(0)); // orthogonalité préservée

    // Transposée : transpose() == transpose() et (A·B)ᵀ == Bᵀ·Aᵀ.
    const Mat4 trA = a * a.transpose();
    CHECK_NEAR(trA(0, 1), trA(1, 0));
    CHECK_NEAR(trA(0, 2), trA(2, 0));
    CHECK_NEAR(trA(1, 3), trA(3, 1));

    // Rotation pure : longueur des vecteurs préservée.
    const Transform rot = Transform::rotateX(Real(0.7f))
                              .compose(Transform::rotateY(Real(-0.3f)))
                              .compose(Transform::rotateZ(Real(1.2f)));
    const Vec3 rv = rot.applyVector(Vec3(Real(1), Real(2), Real(3)));
    CHECK_NEAR(length(rv), length(Vec3(Real(1), Real(2), Real(3))));

    // Statique : trivialement copiable (POD) — vérifié par le header.
    CHECK(std::is_trivially_copyable_v<Mat4>);
    CHECK(std::is_trivially_copyable_v<Transform>);

    if (g_failures != 0) {
	std::cerr << g_failures << " failure(s)\n";
	return 1;
    }
    std::cout << "test_mat4: OK\n";
    return 0;
}
