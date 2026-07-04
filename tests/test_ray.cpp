#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Ray.hpp"

TEST_CASE("Ray default constructor", "[ray]")
{
    Ray r;
    CHECK(r.getOrigin() == Vec3(0, 0, 0));
    CHECK(r.getDirection() == Vec3(0, 0, 1));
}

TEST_CASE("Ray parameterized constructor", "[ray]")
{
    Ray r(Vec3(1, 2, 3), Vec3(4, 0, 0));
    CHECK(r.getOrigin() == Vec3(1, 2, 3));
    CHECK(r.getDirection() == Vec3(1, 0, 0));
}

TEST_CASE("Ray auto-normalizes direction", "[ray]")
{
    Ray r(Vec3(0, 0, 0), Vec3(3, 4, 0));
    CHECK(r.getDirection().magnitude() == Approx(1.0));
    CHECK(r.getDirection() == Vec3(0.6, 0.8, 0));
}

TEST_CASE("Ray zero direction uses fallback", "[ray]")
{
    Ray r(Vec3(0, 0, 0), Vec3(0, 0, 0));
    CHECK(r.getDirection().magnitude() == Approx(1.0));
}

TEST_CASE("Ray pointAt", "[ray]")
{
    Ray r(Vec3(1, 2, 3), Vec3(1, 0, 0));
    CHECK(r.pointAt(0) == Vec3(1, 2, 3));
    CHECK(r.pointAt(5) == Vec3(6, 2, 3));
    CHECK(r.pointAt(-2) == Vec3(-1, 2, 3));
}

TEST_CASE("Ray copy and assignment", "[ray]")
{
    Ray a(Vec3(1, 2, 3), Vec3(0, 1, 0));
    Ray b(a);
    CHECK(b.getOrigin() == Vec3(1, 2, 3));
    CHECK(b.getDirection() == Vec3(0, 1, 0));

    Ray c;
    c = a;
    CHECK(c.getOrigin() == Vec3(1, 2, 3));
    CHECK(c.getDirection() == Vec3(0, 1, 0));
}

TEST_CASE("Ray setters", "[ray]")
{
    Ray r;
    r.setOrigin(Vec3(5, 6, 7));
    CHECK(r.getOrigin() == Vec3(5, 6, 7));
    r.setDirection(Vec3(0, -5, 0));
    CHECK(r.getDirection() == Vec3(0, -1, 0));
}

TEST_CASE("Ray setter zero direction keeps previous", "[ray]")
{
    Ray r(Vec3(0, 0, 0), Vec3(1, 0, 0));
    r.setDirection(Vec3(0, 0, 0));
    CHECK(r.getDirection().magnitude() == Approx(1.0));
}
