// Tests de rt::Mat4 / rt::Transform (T012), Catch2 (T017).

#include <catch2/catch_amalgamated.hpp>

#include <optional>
#include <type_traits>

#include "rt/base/Mat4.hpp"
#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"

namespace {

// Comparaison flottante à 1e-5 près (même tolérance que l'ancien harnais).
bool near(rt::Real a, rt::Real b) { return a == Catch::Approx(b).margin(1e-5); }

} // namespace

TEST_CASE("identité : transformPoint/Vector/Normal inchangés", "[mat4]") {
    using namespace rt;

    const Mat4 id = Mat4::identity();
    const Vec3 p = transformPoint(id, Vec3(Real(1), Real(2), Real(3)));
    REQUIRE(p[0] == Real(1));
    REQUIRE(p[1] == Real(2));
    REQUIRE(p[2] == Real(3));
    const Vec3 v = transformVector(id, Vec3(Real(0), Real(1), Real(0)));
    REQUIRE(v[1] == Real(1));
    const Vec3 n0 = transformNormal(id, Vec3(Real(0), Real(1), Real(0)));
    REQUIRE(near(n0[1], Real(1)));
}

TEST_CASE("inverse × matrice ≈ identité", "[mat4]") {
    using namespace rt;

    const Transform t = Transform::translate(Vec3(Real(2), Real(-3), Real(7)))
                            .compose(Transform::rotateY(degreesToRadians(Real(35))))
                            .compose(Transform::scale(Vec3(Real(2), Real(0.5f), Real(3))));
    const Mat4 a = t.matrix;
    const std::optional<Mat4> inv = a.inverse();
    REQUIRE(inv.has_value());
    const Mat4 prod = inv.has_value() ? a * inv.value() : Mat4();
    for (int i = 0; i < 4; ++i) {
	for (int j = 0; j < 4; ++j) {
	    REQUIRE(near(prod(i, j), i == j ? Real(1) : Real(0)));
	}
    }
    const Mat4 prod2 = inv.has_value() ? inv.value() * a : Mat4();
    for (int i = 0; i < 4; ++i) {
	for (int j = 0; j < 4; ++j) {
	    REQUIRE(near(prod2(i, j), i == j ? Real(1) : Real(0)));
	}
    }
}

TEST_CASE("matrice singulière → nullopt, jamais de throw", "[mat4]") {
    using namespace rt;

    const Mat4 sing; // matrice nulle : déterminant 0
    const std::optional<Mat4> invSing = sing.inverse();
    REQUIRE_FALSE(invSing.has_value());
}

TEST_CASE("translation : le point est déplacé, le vecteur ne l'est pas", "[mat4]") {
    using namespace rt;

    const Transform tr = Transform::translate(Vec3(Real(42), Real(42), Real(42)));
    const Vec3 pt = tr.applyPoint(Vec3(Real(0), Real(0), Real(0)));
    REQUIRE(near(pt[0], Real(42)));
    REQUIRE(near(pt[1], Real(42)));
    REQUIRE(near(pt[2], Real(42)));
    const Vec3 tv = tr.applyVector(Vec3(Real(1), Real(0), Real(0)));
    REQUIRE(near(tv[0], Real(1)));
    REQUIRE(near(tv[1], Real(0)));
}

TEST_CASE("composition : scale puis translate ≠ translate puis scale", "[mat4]") {
    using namespace rt;

    const Transform sc = Transform::scale(Vec3(Real(2), Real(2), Real(2)));
    const Vec3 tp = Transform::translate(Vec3(Real(10), Real(0), Real(0)))
                        .compose(sc)
                        .applyPoint(Vec3(Real(1), Real(0), Real(0)));
    REQUIRE(near(tp[0], Real(12))); // translation après scale : 10 + 2*1
    const Vec3 ps = sc.compose(Transform::translate(Vec3(Real(10), Real(0), Real(0))))
                        .applyPoint(Vec3(Real(1), Real(0), Real(0)));
    REQUIRE(near(ps[0], Real(22))); // scale après translation : 2*(1+10)
}

TEST_CASE("rotation Z de 90° : +X → +Y", "[mat4]") {
    using namespace rt;

    const Transform rz = Transform::rotateZ(kPi / Real(2));
    const Vec3 ru = rz.applyVector(Vec3(Real(1), Real(0), Real(0)));
    REQUIRE(near(ru[0], Real(0)));
    REQUIRE(near(ru[1], Real(1)));
}

TEST_CASE("normale après transformation non unitaire : unitaire et orthogonale", "[mat4]") {
    using namespace rt;

    // Normale après rotation non uniforme + scale : reste unitaire et
    // perpendiculaire à la tangente transformée (inverse-transposée).
    const Transform nonUniform = Transform::scale(Vec3(Real(4), Real(1), Real(1)))
                                     .compose(Transform::rotateY(degreesToRadians(Real(30))));
    const Vec3 tangent = normalize(Vec3(Real(1), Real(1), Real(0))); // direction quelconque
    const Vec3 normal = normalize(Vec3(Real(1), Real(-1), Real(0))); // ⊥ tangente avant
    const Vec3 tT = normalize(nonUniform.applyVector(tangent));
    const Vec3 tN = nonUniform.applyNormal(normal);
    REQUIRE(near(length(tN), Real(1)));
    REQUIRE(near(dot(tT, tN), Real(0))); // orthogonalité préservée
}

TEST_CASE("transposée : (A·Aᵀ) est symétrique", "[mat4]") {
    using namespace rt;

    const Transform t = Transform::translate(Vec3(Real(2), Real(-3), Real(7)))
                            .compose(Transform::scale(Vec3(Real(2), Real(0.5f), Real(3))));
    const Mat4 a = t.matrix;
    const Mat4 trA = a * a.transpose();
    REQUIRE(near(trA(0, 1), trA(1, 0)));
    REQUIRE(near(trA(0, 2), trA(2, 0)));
    REQUIRE(near(trA(1, 3), trA(3, 1)));
}

TEST_CASE("rotation pure : longueur des vecteurs préservée", "[mat4]") {
    using namespace rt;

    const Transform rot = Transform::rotateX(Real(0.7f))
                              .compose(Transform::rotateY(Real(-0.3f)))
                              .compose(Transform::rotateZ(Real(1.2f)));
    const Vec3 rv = rot.applyVector(Vec3(Real(1), Real(2), Real(3)));
    REQUIRE(near(length(rv), length(Vec3(Real(1), Real(2), Real(3)))));
}

TEST_CASE("Mat4 et Transform sont trivialement copiables (POD)", "[mat4]") {
    REQUIRE(std::is_trivially_copyable_v<rt::Mat4>);
    REQUIRE(std::is_trivially_copyable_v<rt::Transform>);
}
