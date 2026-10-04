// Tests de rt::Vec2/Vec3/Vec4 (T011). Programme autonome (Catch2 en T017) :
// renvoie un code ≠ 0 si un contrôle échoue.

#include <iostream>

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

    // Arithmétique de base.
    Vec3 const a(Real(1), Real(2), Real(3));
    Vec3 const b(Real(4), Real(-5), Real(6));
    Vec3 s = a + b;
    CHECK(s[0] == Real(5) && s[1] == Real(-3) && s[2] == Real(9));
    Vec3 d = a - b;
    CHECK(d[0] == Real(-3) && d[1] == Real(7) && d[2] == Real(-3));
    Vec3 m = a * Real(2);
    CHECK(m[0] == Real(2) && m[1] == Real(4) && m[2] == Real(6));

    // Division quasi nulle → vecteur inchangé (documenté, jamais de throw).
    Vec3 v(Real(1), Real(2), Real(3));
    Vec3 w = v / Real(0);
    CHECK(w[0] == v[0] && w[1] == v[1] && w[2] == v[2]);
    Vec3 w2 = v / Real(1e-7f); // |s| <= kEpsilon = 1e-6
    CHECK(w2[0] == v[0] && w2[2] == v[2]);
    Vec3 w3 = v / Real(-1e-8f);
    CHECK(w3[1] == v[1]);
    Vec3 w4 = v / Real(2);
    CHECK(w4[0] == Real(0.5f) && w4[1] == Real(1) && w4[2] == Real(1.5f));

    // Vecteur nul : normalize → (0,0,0) ; division par ~0 inchangée.
    Vec3 const zero(Real(0), Real(0), Real(0));
    Vec3 nz = normalize(zero);
    CHECK(nz[0] == Real(0) && nz[1] == Real(0) && nz[2] == Real(0));
    CHECK(nearZero(zero));
    CHECK(!nearZero(Vec3(Real(0), Real(0), Real(0.5f))));

    // normalize : longueur 1, même direction.
    Vec3 n = normalize(Vec3(Real(0), Real(3), Real(4)));
    CHECK_NEAR(length(n), Real(1));
    CHECK_NEAR(n[1], Real(0.6f));
    CHECK_NEAR(n[2], Real(0.8f));

    // Orthogonalité : cross(a,b) · a == 0 et · b == 0.
    Vec3 const x(Real(1), Real(0), Real(0));
    Vec3 const y(Real(0), Real(1), Real(0));
    Vec3 z = cross(x, y);
    CHECK_NEAR(dot(z, x), Real(0));
    CHECK_NEAR(dot(z, y), Real(0));
    CHECK_NEAR(z[2], Real(1));
    // L2 vers L3 : cross produit nul si colinéaires.
    Vec3 const zeroCross = cross(x, x * Real(3));
    CHECK(nearZero(zeroCross));

    // reflect : incidence à 45° sur le sol → rebond symétrique.
    Vec3 const in(Real(1), Real(-1), Real(0));
    Vec3 const up(Real(0), Real(1), Real(0));
    Vec3 r = reflect(in, up);
    CHECK_NEAR(r[0], Real(1));
    CHECK_NEAR(r[1], Real(1));
    CHECK_NEAR(r[2], Real(0));

    // refract : passage air → verre (eta = 1/1.5), direction déviée vers la
    // normale, énergie conservée (longueur unitaire).
    Vec3 const vi = normalize(Vec3(Real(0.5f), Real(-1), Real(0)));
    Vec3 const nSurf = normalize(Vec3(Real(0), Real(1), Real(0)));
    Vec3 rt2 = refract(vi, nSurf, Real(1.0f / 1.5f));
    CHECK_NEAR(length(rt2), Real(1));
    CHECK(rt2[1] < Real(0)); // continue vers le bas
    // Réflexion totale interne : verre → air avec grand angle → sentinel nul.
    Vec3 const grazing = normalize(Vec3(Real(0.99f), Real(-0.14f), Real(0)));
    Vec3 const nUp = Vec3(Real(0), Real(1), Real(0));
    Vec3 t = refract(grazing, nUp, Real(1.5f));
    CHECK(t[0] == Real(0) && t[1] == Real(0) && t[2] == Real(0));

    // min/max composante à composante.
    Vec3 const p(Real(1), Real(-3), Real(5));
    Vec3 const q(Real(-2), Real(4), Real(0));
    Vec3 lo = min(p, q);
    Vec3 hi = max(p, q);
    CHECK(lo[0] == Real(-2) && lo[1] == Real(-3) && lo[2] == Real(0));
    CHECK(hi[0] == Real(1) && hi[1] == Real(4) && hi[2] == Real(5));

    // Vec2 / Vec4 : comportements cohérents.
    Vec2 const v2(Real(3), Real(4));
    CHECK_NEAR(length(v2), Real(5));
    CHECK_NEAR(length(normalize(v2)), Real(1));
    Vec4 const v4(Real(1), Real(2), Real(3), Real(4));
    CHECK_NEAR(lengthSquared(v4), Real(30));
    CHECK(nearZero(Vec4()));

    if (g_failures != 0) {
	std::cerr << g_failures << " failure(s)\n";
	return 1;
    }
    std::cout << "test_vec: OK\n";
    return 0;
}
