// Tests de rt::Vec2/Vec3/Vec4 (T011), Catch2 (T017).

#include <catch2/catch_amalgamated.hpp>

#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"

namespace {

// Comparaison flottante à 1e-5 près (même tolérance que l'ancien harnais).
bool near(rt::Real a, rt::Real b) { return a == Catch::Approx(b).margin(1e-5); }

} // namespace

TEST_CASE("arithmétique de base", "[vec]") {
    using namespace rt;

    const Vec3 a(Real(1), Real(2), Real(3));
    const Vec3 b(Real(4), Real(-5), Real(6));
    const Vec3 s = a + b;
    REQUIRE(s[0] == Real(5));
    REQUIRE(s[1] == Real(-3));
    REQUIRE(s[2] == Real(9));
    const Vec3 d = a - b;
    REQUIRE(d[0] == Real(-3));
    REQUIRE(d[1] == Real(7));
    REQUIRE(d[2] == Real(-3));
    const Vec3 m = a * Real(2);
    REQUIRE(m[0] == Real(2));
    REQUIRE(m[1] == Real(4));
    REQUIRE(m[2] == Real(6));
}

TEST_CASE("division quasi nulle : vecteur inchangé, jamais de throw", "[vec]") {
    using namespace rt;

    const Vec3 v(Real(1), Real(2), Real(3));
    const Vec3 w = v / Real(0);
    REQUIRE(w[0] == v[0]);
    REQUIRE(w[1] == v[1]);
    REQUIRE(w[2] == v[2]);
    const Vec3 w2 = v / Real(1e-7f); // |s| <= kEpsilon = 1e-6
    REQUIRE(w2[0] == v[0]);
    REQUIRE(w2[2] == v[2]);
    const Vec3 w3 = v / Real(-1e-8f);
    REQUIRE(w3[1] == v[1]);
    const Vec3 w4 = v / Real(2);
    REQUIRE(w4[0] == Real(0.5f));
    REQUIRE(w4[1] == Real(1));
    REQUIRE(w4[2] == Real(1.5f));
}

TEST_CASE("vecteur nul : normalize → (0,0,0)", "[vec]") {
    using namespace rt;

    const Vec3 zero(Real(0), Real(0), Real(0));
    const Vec3 nz = normalize(zero);
    REQUIRE(nz[0] == Real(0));
    REQUIRE(nz[1] == Real(0));
    REQUIRE(nz[2] == Real(0));
    REQUIRE(nearZero(zero));
    REQUIRE_FALSE(nearZero(Vec3(Real(0), Real(0), Real(0.5f))));
}

TEST_CASE("normalize : longueur 1, même direction", "[vec]") {
    using namespace rt;

    const Vec3 n = normalize(Vec3(Real(0), Real(3), Real(4)));
    REQUIRE(near(length(n), Real(1)));
    REQUIRE(near(n[1], Real(0.6f)));
    REQUIRE(near(n[2], Real(0.8f)));
}

TEST_CASE("produit vectoriel orthogonal aux entrées", "[vec]") {
    using namespace rt;

    const Vec3 x(Real(1), Real(0), Real(0));
    const Vec3 y(Real(0), Real(1), Real(0));
    const Vec3 z = cross(x, y);
    REQUIRE(near(dot(z, x), Real(0)));
    REQUIRE(near(dot(z, y), Real(0)));
    REQUIRE(near(z[2], Real(1)));
    // Produit nul si colinéaires.
    const Vec3 zeroCross = cross(x, x * Real(3));
    REQUIRE(nearZero(zeroCross));
}

TEST_CASE("reflect : rebond symétrique à 45°", "[vec]") {
    using namespace rt;

    const Vec3 in(Real(1), Real(-1), Real(0));
    const Vec3 up(Real(0), Real(1), Real(0));
    const Vec3 r = reflect(in, up);
    REQUIRE(near(r[0], Real(1)));
    REQUIRE(near(r[1], Real(1)));
    REQUIRE(near(r[2], Real(0)));
}

TEST_CASE("refract : déviation et réflexion totale interne", "[vec]") {
    using namespace rt;

    // Passage air → verre (eta = 1/1.5), direction déviée vers la normale,
    // énergie conservée (longueur unitaire).
    const Vec3 vi = normalize(Vec3(Real(0.5f), Real(-1), Real(0)));
    const Vec3 nSurf = normalize(Vec3(Real(0), Real(1), Real(0)));
    const Vec3 rt2 = refract(vi, nSurf, Real(1.0f / 1.5f));
    REQUIRE(near(length(rt2), Real(1)));
    REQUIRE(rt2[1] < Real(0)); // continue vers le bas
    // Réflexion totale interne : verre → air avec grand angle → sentinel nul.
    const Vec3 grazing = normalize(Vec3(Real(0.99f), Real(-0.14f), Real(0)));
    const Vec3 nUp = Vec3(Real(0), Real(1), Real(0));
    const Vec3 t = refract(grazing, nUp, Real(1.5f));
    REQUIRE(t[0] == Real(0));
    REQUIRE(t[1] == Real(0));
    REQUIRE(t[2] == Real(0));
}

TEST_CASE("min/max composante à composante", "[vec]") {
    using namespace rt;

    const Vec3 p(Real(1), Real(-3), Real(5));
    const Vec3 q(Real(-2), Real(4), Real(0));
    const Vec3 lo = min(p, q);
    const Vec3 hi = max(p, q);
    REQUIRE(lo[0] == Real(-2));
    REQUIRE(lo[1] == Real(-3));
    REQUIRE(lo[2] == Real(0));
    REQUIRE(hi[0] == Real(1));
    REQUIRE(hi[1] == Real(4));
    REQUIRE(hi[2] == Real(5));
}

TEST_CASE("Vec2 / Vec4 : comportements cohérents", "[vec]") {
    using namespace rt;

    const Vec2 v2(Real(3), Real(4));
    REQUIRE(near(length(v2), Real(5)));
    REQUIRE(near(length(normalize(v2)), Real(1)));
    const Vec4 v4(Real(1), Real(2), Real(3), Real(4));
    REQUIRE(near(lengthSquared(v4), Real(30)));
    REQUIRE(nearZero(Vec4()));
}
