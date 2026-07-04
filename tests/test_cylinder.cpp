#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "geometry/Cylinder.hpp"
#include "core/Material.hpp"

TEST_CASE("Cylinder ray above misses", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Ray ray(Vec3(0, 5, 10), Vec3(0, 0, -1));
    auto hr = cyl.hit(ray, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Cylinder ray from side hits body", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Ray ray(Vec3(-5, 1.0, 0), Vec3(1, 0, 0));
    auto hr = cyl.hit(ray, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() == Approx(4.0));
    CHECK(hr->getPoint() == Vec3(-1.0, 1.0, 0.0));
    CHECK(hr->isFrontFace());
}

TEST_CASE("Cylinder ray far sideways misses", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Ray ray(Vec3(10, 1.0, 0), Vec3(0, 0, -1));
    auto hr = cyl.hit(ray, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Cylinder ray from below hits lower cap", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Ray ray(Vec3(0.5, -5, 0), Vec3(0, 1, 0));
    auto hr = cyl.hit(ray, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getPoint().y == Approx(0.0));
}

TEST_CASE("Cylinder ray from above hits upper cap", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Ray ray(Vec3(0.5, 5, 0), Vec3(0, -1, 0));
    auto hr = cyl.hit(ray, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getPoint().y == Approx(2.0));
}

TEST_CASE("Cylinder normal on body is radial", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Vec3 n = cyl.getNormalAt(Vec3(1.0, 1.0, 0.0));
    CHECK(n == Vec3(1, 0, 0));
    n = cyl.getNormalAt(Vec3(0, 1.0, 1.0));
    CHECK(n == Vec3(0, 0, 1));
}

TEST_CASE("Cylinder ray from diagonal hits body", "[cylinder]")
{
    Cylinder cyl(Vec3(0, 0, 0), Vec3(0, 1, 0), 1.0, 2.0);
    Ray ray(Vec3(-5, 0.5, 0.5), Vec3(1, 0, 0));
    auto hr = cyl.hit(ray, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() > 0);
}
