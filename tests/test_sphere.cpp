#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "geometry/Sphere.hpp"

TEST_CASE("Sphere default constructor", "[sphere]")
{
    Sphere s;
    CHECK(s.getCenter() == Vec3(0, 0, 0));
    CHECK(s.getRadius() == 1.0);
    CHECK(s.getType() == std::string("Sphere"));
}

TEST_CASE("Sphere parameterized constructor", "[sphere]")
{
    Sphere s(Vec3(1, 2, 3), 5.0);
    CHECK(s.getCenter() == Vec3(1, 2, 3));
    CHECK(s.getRadius() == 5.0);
}

TEST_CASE("Sphere hit from outside", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    Ray r(Vec3(-5, 0, 0), Vec3(1, 0, 0));
    auto hr = s.hit(r, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() == Approx(4.0));
    CHECK(hr->getPoint() == Vec3(-1, 0, 0));
    CHECK(hr->getNormal() == Vec3(-1, 0, 0));
    CHECK(hr->isFrontFace());
}

TEST_CASE("Sphere hit from inside", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 5.0);
    Ray r(Vec3(0, 0, 0), Vec3(0, 1, 0));
    auto hr = s.hit(r, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() == Approx(5.0));
    CHECK_FALSE(hr->isFrontFace());
}

TEST_CASE("Sphere miss", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    Ray r(Vec3(0, 10, 0), Vec3(0, 0, 1));
    auto hr = s.hit(r, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Sphere tangent ray barely hits", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    Ray r(Vec3(0, 1, -5), Vec3(0, 0, 1));
    auto hr = s.hit(r, 0.0, 100.0);
    CHECK(hr.has_value());
}

TEST_CASE("Sphere hit behind ray origin", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    Ray r(Vec3(5, 0, 0), Vec3(1, 0, 0));
    auto hr = s.hit(r, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Sphere tMin/tMax filtering", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    Ray r(Vec3(-5, 0, 0), Vec3(1, 0, 0));
    auto hr = s.hit(r, 0.0, 3.0);
    CHECK_FALSE(hr.has_value());
    hr = s.hit(r, 0.0, 5.0);
    CHECK(hr.has_value());
}

TEST_CASE("Sphere normal at point", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    CHECK(s.getNormalAt(Vec3(1, 0, 0)) == Vec3(1, 0, 0));
    CHECK(s.getNormalAt(Vec3(0, 1, 0)) == Vec3(0, 1, 0));
    CHECK(s.getNormalAt(Vec3(0, 0, 1)) == Vec3(0, 0, 1));
    Vec3 diag = Vec3(1, 1, 0).normalized();
    CHECK(s.getNormalAt(diag) == diag);
}

TEST_CASE("Sphere bounding box", "[sphere]")
{
    Sphere s(Vec3(1, 2, 3), 5.0);
    Vec3 minC, maxC;
    s.getBoundingBox(minC, maxC);
    CHECK(minC == Vec3(-4, -3, -2));
    CHECK(maxC == Vec3(6, 7, 8));
}

TEST_CASE("Sphere copy and assignment", "[sphere]")
{
    Sphere a(Vec3(1, 2, 3), 4.5);
    Sphere b(a);
    CHECK(b.getCenter() == Vec3(1, 2, 3));
    CHECK(b.getRadius() == 4.5);

    Sphere c;
    c = a;
    CHECK(c.getCenter() == Vec3(1, 2, 3));
    CHECK(c.getRadius() == 4.5);
}

TEST_CASE("Sphere UV coordinates", "[sphere]")
{
    Sphere s(Vec3(0, 0, 0), 1.0);
    double u, v;
    s.getUVAt(Vec3(1, 0, 0), u, v);
    CHECK(u == Approx(0.5));
    CHECK(v == Approx(0.5));
}
