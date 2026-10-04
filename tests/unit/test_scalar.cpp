// Tests de rt::Scalar (T010), Catch2 (T017).

#include <catch2/catch_amalgamated.hpp>

#include "rt/base/Scalar.hpp"

TEST_CASE("clamp borne les valeurs", "[scalar]") {
    using namespace rt;

    REQUIRE(clamp(Real(5), Real(0), Real(1)) == Real(1));
    REQUIRE(clamp(Real(-5), Real(0), Real(1)) == Real(0));
    REQUIRE(clamp(Real(0.5f), Real(0), Real(1)) == Real(0.5f));
}

TEST_CASE("lerp interpole linéairement", "[scalar]") {
    using namespace rt;

    CHECK(almostEqual(lerp(Real(0), Real(10), Real(0)), Real(0)));
    CHECK(almostEqual(lerp(Real(0), Real(10), Real(1)), Real(10)));
    CHECK(almostEqual(lerp(Real(0), Real(10), Real(0.5f)), Real(5)));
    CHECK(almostEqual(lerp(Real(-4), Real(4), Real(0.25f)), Real(-2)));
}

TEST_CASE("almostEqual compare avec tolérance", "[scalar]") {
    using namespace rt;

    CHECK(almostEqual(Real(1.0f), Real(1.0f + 1e-7f)));
    CHECK_FALSE(almostEqual(Real(1.0f), Real(1.1f)));
    CHECK(almostEqual(Real(0), Real(1e-7f))); // tolérance absolue près de 0
    CHECK_FALSE(almostEqual(Real(0), Real(0.5f)));
}

TEST_CASE("conversions degrés/radians", "[scalar]") {
    using namespace rt;

    CHECK(almostEqual(degreesToRadians(Real(0)), Real(0)));
    CHECK(almostEqual(degreesToRadians(Real(180)), kPi));
    CHECK(almostEqual(radiansToDegrees(kPi), Real(180)));
    CHECK(almostEqual(radiansToDegrees(degreesToRadians(Real(42))), Real(42)));
}

TEST_CASE("kInfinity se comporte comme l'infini", "[scalar]") {
    using namespace rt;

    REQUIRE(kInfinity > Real(0));
    REQUIRE(kInfinity + Real(1) == kInfinity);
}
