#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Vec3.hpp"

TEST_CASE("Vec3 default constructor", "[vec3]")
{
    Vec3 v;
    REQUIRE(v.x == 0.0);
    REQUIRE(v.y == 0.0);
    REQUIRE(v.z == 0.0);
}

TEST_CASE("Vec3 parameterized constructor", "[vec3]")
{
    Vec3 v(1.5, -2.5, 3.0);
    REQUIRE(v.x == 1.5);
    REQUIRE(v.y == -2.5);
    REQUIRE(v.z == 3.0);
}

TEST_CASE("Vec3 copy constructor and assignment", "[vec3]")
{
    Vec3 a(1, 2, 3);
    Vec3 b(a);
    REQUIRE(b == Vec3(1, 2, 3));
    Vec3 c(9, 9, 9);
    c = a;
    REQUIRE(c == Vec3(1, 2, 3));
}

TEST_CASE("Vec3 arithmetic", "[vec3]")
{
    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);
    CHECK(a + b == Vec3(5, 7, 9));
    CHECK(a - b == Vec3(-3, -3, -3));
    CHECK(-a == Vec3(-1, -2, -3));
    CHECK(a * 2.0 == Vec3(2, 4, 6));
    CHECK(2.0 * a == Vec3(2, 4, 6));
    CHECK(a / 2.0 == Vec3(0.5, 1.0, 1.5));

    Vec3 c = a;
    c += b;
    CHECK(c == Vec3(5, 7, 9));
    c = a;
    c -= b;
    CHECK(c == Vec3(-3, -3, -3));
    c = a;
    c *= 3.0;
    CHECK(c == Vec3(3, 6, 9));
}

TEST_CASE("Vec3 division by zero throws", "[vec3]")
{
    Vec3 v(1, 2, 3);
    CHECK_THROWS(v / 0.0);
}

TEST_CASE("Vec3 comparison", "[vec3]")
{
    Vec3 a(1, 2, 3);
    Vec3 b(1, 2, 3);
    Vec3 c(1, 2, 4);
    CHECK(a == b);
    CHECK(a != c);
    CHECK(a != Vec3(1, 2, 3 + 1e-4));
}

TEST_CASE("Vec3 componentMult", "[vec3]")
{
    Vec3 a(2, 3, 4);
    Vec3 b(5, 6, 7);
    Vec3 r = a.componentMult(b);
    CHECK(r == Vec3(10, 18, 28));
}

TEST_CASE("Vec3 dot product", "[vec3]")
{
    CHECK(Vec3(1, 0, 0).dot(Vec3(0, 1, 0)) == 0.0);
    CHECK(Vec3(1, 0, 0).dot(Vec3(1, 0, 0)) == 1.0);
    CHECK(Vec3(1, 2, 3).dot(Vec3(4, 5, 6)) == 32.0);
    CHECK(Vec3(-1, 0, 0).dot(Vec3(1, 0, 0)) == -1.0);
}

TEST_CASE("Vec3 cross product", "[vec3]")
{
    CHECK(Vec3(1, 0, 0).cross(Vec3(0, 1, 0)) == Vec3(0, 0, 1));
    CHECK(Vec3(0, 1, 0).cross(Vec3(1, 0, 0)) == Vec3(0, 0, -1));
    CHECK(Vec3(1, 0, 0).cross(Vec3(1, 0, 0)) == Vec3(0, 0, 0));
}

TEST_CASE("Vec3 magnitude", "[vec3]")
{
    CHECK(Vec3(3, 4, 0).magnitude() == 5.0);
    CHECK(Vec3(3, 4, 0).magnitudeSquared() == 25.0);
    CHECK(Vec3(0, 0, 0).magnitude() == 0.0);
}

TEST_CASE("Vec3 normalization", "[vec3]")
{
    Vec3 v(3, 0, 0);
    v.normalize();
    CHECK(v == Vec3(1, 0, 0));
    CHECK(Vec3(0, 5, 0).normalized() == Vec3(0, 1, 0));
    CHECK_THROWS(Vec3(0, 0, 0).normalize());
    CHECK_THROWS(Vec3(0, 0, 0).normalized());
}

TEST_CASE("Vec3 distance", "[vec3]")
{
    CHECK(Vec3(0, 0, 0).distance(Vec3(3, 4, 0)) == 5.0);
    CHECK(Vec3(0, 0, 0).distanceSquared(Vec3(3, 4, 0)) == 25.0);
}

TEST_CASE("Vec3 lerp", "[vec3]")
{
    Vec3 a(0, 0, 0);
    Vec3 b(10, 10, 10);
    CHECK(a.lerp(b, 0.0) == Vec3(0, 0, 0));
    CHECK(a.lerp(b, 1.0) == Vec3(10, 10, 10));
    CHECK(a.lerp(b, 0.5) == Vec3(5, 5, 5));
}

TEST_CASE("Vec3 reflect", "[vec3]")
{
    Vec3 incident(1, -1, 0);
    Vec3 normal(0, 1, 0);
    Vec3 reflected = incident.reflect(normal);
    CHECK(reflected == Vec3(1, 1, 0));

    CHECK(Vec3(0, -1, 0).reflect(Vec3(0, 1, 0)) == Vec3(0, 1, 0));
}

TEST_CASE("Vec3 refract", "[vec3]")
{
    Vec3 normal(0, 1, 0);
    Vec3 v = Vec3(1, -1, 0).normalized();
    Vec3 refracted;
    bool ok = v.refract(normal, 1.0 / 1.5, refracted);
    REQUIRE(ok);
    CHECK(refracted.magnitude() == Approx(1.0));

    Vec3 tir = Vec3(0.707, -0.707, 0).normalized();
    Vec3 tirResult;
    bool tirOk = tir.refract(Vec3(0, -1, 0), 1.5, tirResult);
    CHECK_FALSE(tirOk);
}

TEST_CASE("Vec3 stream output", "[vec3]")
{
    Vec3 v(1, 2, 3);
    std::ostringstream os;
    os << v;
    CHECK(os.str() == "(1, 2, 3)");
}

TEST_CASE("Vec3 operator== uses epsilon", "[vec3]")
{
    Vec3 a(1.0, 2.0, 3.0);
    Vec3 b(1.0 + 1e-7, 2.0, 3.0);
    CHECK(a == b);
    Vec3 c(1.0 + 1e-4, 2.0, 3.0);
    CHECK(a != c);
}
