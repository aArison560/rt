// Tests de rt::Scalar (T010). Catch2 sera raccordé en T017 : en attendant,
// ce fichier est un programme autonome qui renvoie un code ≠ 0 si un
// contrôle échoue (mêmes invariants que les TEST_CASE Catch2).

#include <iostream>

#include "rt/base/Scalar.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
	if (!(cond)) {                                                                             \
	    ++g_failures;                                                                          \
	    std::cerr << "FAIL line " << __LINE__ << ": " #cond << '\n';                           \
	}                                                                                          \
    } while (false)

} // namespace

int main() {
    using namespace rt;

    CHECK(clamp(Real(5), Real(0), Real(1)) == Real(1));
    CHECK(clamp(Real(-5), Real(0), Real(1)) == Real(0));
    CHECK(clamp(Real(0.5f), Real(0), Real(1)) == Real(0.5f));

    CHECK(almostEqual(lerp(Real(0), Real(10), Real(0)), Real(0)));
    CHECK(almostEqual(lerp(Real(0), Real(10), Real(1)), Real(10)));
    CHECK(almostEqual(lerp(Real(0), Real(10), Real(0.5f)), Real(5)));
    CHECK(almostEqual(lerp(Real(-4), Real(4), Real(0.25f)), Real(-2)));

    CHECK(almostEqual(Real(1.0f), Real(1.0f + 1e-7f)));
    CHECK(!almostEqual(Real(1.0f), Real(1.1f)));
    CHECK(almostEqual(Real(0), Real(1e-7f))); // tolérance absolue près de 0
    CHECK(!almostEqual(Real(0), Real(0.5f)));

    CHECK(almostEqual(degreesToRadians(Real(0)), Real(0)));
    CHECK(almostEqual(degreesToRadians(Real(180)), kPi));
    CHECK(almostEqual(radiansToDegrees(kPi), Real(180)));
    CHECK(almostEqual(radiansToDegrees(degreesToRadians(Real(42))), Real(42)));

    CHECK(kInfinity > Real(0));
    CHECK(kInfinity + Real(1) == kInfinity);

    if (g_failures != 0) {
	std::cerr << g_failures << " failure(s)\n";
	return 1;
    }
    std::cout << "test_scalar: OK\n";
    return 0;
}
