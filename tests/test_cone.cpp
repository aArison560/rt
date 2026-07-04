#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "geometry/Cone.hpp"

TEST_CASE("Cone ray above misses", "[cone]")
{
    Cone cone(Vec3(0, 0, 0), Vec3(0, 1, 0), 30.0, 2.0);
    Ray ray(Vec3(0, 5, 10), Vec3(0, 0, -1));
    auto hr = cone.hit(ray, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Cone ray from side hits cone surface", "[cone]")
{
    Cone cone(Vec3(0, 0, 0), Vec3(0, 1, 0), 30.0, 2.0);
    Ray ray(Vec3(-5, 1.0, 0), Vec3(1, 0, 0));
    auto hr = cone.hit(ray, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getT() > 0);
    double radAtH = 1.0 * std::tan(30.0 * M_PI / 180.0);
    CHECK(hr->getPoint().x == Approx(-radAtH));
}

TEST_CASE("Cone ray from below hits base cap", "[cone]")
{
    Cone cone(Vec3(0, 0, 0), Vec3(0, 1, 0), 30.0, 2.0);
    Ray ray(Vec3(0.5, 5, 0), Vec3(0, -1, 0));
    auto hr = cone.hit(ray, 0.0, 100.0);
    REQUIRE(hr.has_value());
    CHECK(hr->getPoint().y == Approx(2.0));
}

TEST_CASE("Cone ray far from apex misses", "[cone]")
{
    Cone cone(Vec3(0, 0, 0), Vec3(0, 1, 0), 10.0, 2.0);
    Ray ray(Vec3(10, 1.0, 0), Vec3(0, 0, -1));
    auto hr = cone.hit(ray, 0.0, 100.0);
    CHECK_FALSE(hr.has_value());
}

TEST_CASE("Cone normal on side points outward", "[cone]")
{
    Cone cone(Vec3(0, 0, 0), Vec3(0, 1, 0), 30.0, 2.0);
    double h = 1.0;
    double r = h * std::tan(30.0 * M_PI / 180.0);
    Vec3 n = cone.getNormalAt(Vec3(r, h, 0.0));
    CHECK(n.x > 0);
    CHECK(n.y < 0);
    CHECK(n.magnitude() == Approx(1.0));
}

TEST_CASE("Cone normal at base center faces up", "[cone]")
{
    Cone cone(Vec3(0, 0, 0), Vec3(0, 1, 0), 30.0, 2.0);
    Vec3 n = cone.getNormalAt(Vec3(0, 2.0, 0));
    CHECK(n == Vec3(0, 1, 0));
}
