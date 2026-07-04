#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "geometry/Plane.hpp"

TEST_CASE("Plane default constructor", "[plane]")
{
    Plane p;
    CHECK(p.getPoint() == Vec3(0, 0, 0));
    CHECK(p.getNormal() == Vec3(0, 1, 0));
    CHECK(p.getType() == std::string("Plane"));
}

TEST_CASE("Plane parameterized constructor normalizes", "[plane]")
{
    Plane p(Vec3(1, 2, 3), Vec3(0, 5, 0));
    CHECK(p.getPoint() == Vec3(1, 2, 3));
    CHECK(p.getNormal() == Vec3(0, 1, 0));
}

TEST_CASE("Plane hit from front", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    Ray r(Vec3(0, 5, 0), Vec3(0, -1, 0));
    auto hr = p.hit(r, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() == Approx(5.0));
    CHECK(hr->getPoint() == Vec3(0, 0, 0));
    CHECK(hr->getNormal() == Vec3(0, 1, 0));
    CHECK(hr->isFrontFace());
}

TEST_CASE("Plane hit from behind", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    Ray r(Vec3(0, -5, 0), Vec3(0, 1, 0));
    auto hr = p.hit(r, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() == Approx(5.0));
    CHECK(hr->getPoint() == Vec3(0, 0, 0));
    CHECK(hr->getNormal() == Vec3(0, -1, 0));
    CHECK_FALSE(hr->isFrontFace());
}

TEST_CASE("Plane parallel ray misses", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    Ray r(Vec3(0, 1, 0), Vec3(1, 0, 0));
    auto hr = p.hit(r, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Plane hit at angle", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    Vec3 dir = Vec3(1, -1, 0).normalized();
    Ray r(Vec3(0, 5, 0), dir);
    auto hr = p.hit(r, 0.0, 100.0);
    REQUIRE(hr.has_value());
    double tExpected = (p.getPoint() - r.getOrigin()).dot(p.getNormal()) / dir.dot(p.getNormal());
    CHECK(hr->getT() == Approx(tExpected));
    CHECK(hr->getPoint().y == Approx(0.0));
}

TEST_CASE("Plane hit tMin/tMax filtering", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    Ray r(Vec3(0, 5, 0), Vec3(0, -1, 0));
    auto hr = p.hit(r, 0.0, 3.0);
    CHECK_FALSE(hr.has_value());
    hr = p.hit(r, 0.0, 10.0);
    CHECK(hr.has_value());
}

TEST_CASE("Plane normal at point is constant", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    CHECK(p.getNormalAt(Vec3(100, 0, 0)) == Vec3(0, 1, 0));
}

TEST_CASE("Plane distanceTo", "[plane]")
{
    Plane p(Vec3(0, 2, 0), Vec3(0, 1, 0));
    CHECK(p.distanceTo(Vec3(0, 5, 0)) == Approx(3.0));
    CHECK(p.distanceTo(Vec3(0, 0, 0)) == Approx(-2.0));
}

TEST_CASE("Plane contains", "[plane]")
{
    Plane p(Vec3(0, 0, 0), Vec3(0, 1, 0));
    CHECK(p.contains(Vec3(5, 0, -3)));
    CHECK_FALSE(p.contains(Vec3(0, 1, 0)));
}

TEST_CASE("Plane copy and assignment", "[plane]")
{
    Plane a(Vec3(1, 2, 3), Vec3(0, 1, 0));
    Plane b(a);
    CHECK(b.getPoint() == Vec3(1, 2, 3));
    CHECK(b.getNormal() == Vec3(0, 1, 0));

    Plane c;
    c = a;
    CHECK(c.getPoint() == Vec3(1, 2, 3));
    CHECK(c.getNormal() == Vec3(0, 1, 0));
}
