#include "geometry/Cylinder.hpp"
#include "geometry/Cone.hpp"
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name) do { std::cout << "  " << name << "... "; } while(0)
#define CHECK(cond) do { \
    if (!(cond)) { \
        std::cout << "FAIL (" << #cond << ")\n"; \
        ++testsFailed; \
        return; \
    } \
} while(0)
#define PASS() do { std::cout << "OK\n"; ++testsPassed; } while(0)

static bool near(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) < eps;
}

static bool nearVec(const Vec3& a, const Vec3& b, double eps = 1e-6)
{
    return (a - b).magnitude() < eps;
}

// ============================================================
// Cylinder tests
// ============================================================

static void test_cylinder_miss_above()
{
    TEST("Cylinder: ray above misses");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Ray ray(Vec3(0, 5, 10), Vec3(0, 0, -1));
    HitRecord hr;
    bool hit = cyl.hit(ray, 0.0, 100.0, hr);
    CHECK(!hit);
    PASS();
}

static void test_cylinder_hit_body_side()
{
    TEST("Cylinder: ray from side hits body");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Ray ray(Vec3(-5, 1.0, 0), Vec3(1, 0, 0));
    HitRecord hr;
    bool hit = cyl.hit(ray, 0.0, 100.0, hr);
    CHECK(hit);
    CHECK(near(hr.getT(), 4.0));
    CHECK(nearVec(hr.getPoint(), Vec3(-1.0, 1.0, 0.0)));
    CHECK(hr.isFrontFace());
    PASS();
}

static void test_cylinder_miss_sideways()
{
    TEST("Cylinder: ray far sideways misses");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Ray ray(Vec3(10, 1.0, 0), Vec3(0, 0, -1));
    HitRecord hr;
    bool hit = cyl.hit(ray, 0.0, 100.0, hr);
    CHECK(!hit);
    PASS();
}

static void test_cylinder_hit_lower_cap()
{
    TEST("Cylinder: ray from below hits lower cap");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Ray ray(Vec3(0.5, -5, 0), Vec3(0, 1, 0));
    HitRecord hr;
    bool hit = cyl.hit(ray, 0.0, 100.0, hr);
    CHECK(hit);
    CHECK(near(hr.getPoint().y, 0.0));
    PASS();
}

static void test_cylinder_hit_upper_cap()
{
    TEST("Cylinder: ray from above hits upper cap");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Ray ray(Vec3(0.5, 5, 0), Vec3(0, -1, 0));
    HitRecord hr;
    bool hit = cyl.hit(ray, 0.0, 100.0, hr);
    CHECK(hit);
    CHECK(near(hr.getPoint().y, 2.0));
    PASS();
}

static void test_cylinder_normal_body()
{
    TEST("Cylinder: normal on body is radial");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Vec3 n = cyl.getNormalAt(Vec3(1.0, 1.0, 0.0));
    CHECK(nearVec(n, Vec3(1, 0, 0)));
    n = cyl.getNormalAt(Vec3(0, 1.0, 1.0));
    CHECK(nearVec(n, Vec3(0, 0, 1)));
    PASS();
}

static void test_cylinder_hit_body_angled()
{
    TEST("Cylinder: ray from diagonal hits body");
    Cylinder cyl(Vec3(0,0,0), Vec3(0,1,0), 1.0, 2.0);
    Ray ray(Vec3(-5, 0.5, 0.5), Vec3(1, 0, 0));
    HitRecord hr;
    bool hit = cyl.hit(ray, 0.0, 100.0, hr);
    CHECK(hit);
    CHECK(hr.getT() > 0);
    PASS();
}

// ============================================================
// Cone tests
// ============================================================

static void test_cone_miss_above()
{
    TEST("Cone: ray above misses");
    Cone cone(Vec3(0,0,0), Vec3(0,1,0), 30.0, 2.0);
    Ray ray(Vec3(0, 5, 10), Vec3(0, 0, -1));
    HitRecord hr;
    bool hit = cone.hit(ray, 0.0, 100.0, hr);
    CHECK(!hit);
    PASS();
}

static void test_cone_hit_side()
{
    TEST("Cone: ray from side hits cone surface");
    Cone cone(Vec3(0,0,0), Vec3(0,1,0), 30.0, 2.0);
    Ray ray(Vec3(-5, 1.0, 0), Vec3(1, 0, 0));
    HitRecord hr;
    bool hit = cone.hit(ray, 0.0, 100.0, hr);
    CHECK(hit);
    CHECK(hr.getT() > 0);
    double radAtH = 1.0 * std::tan(30.0 * M_PI / 180.0);
    CHECK(near(hr.getPoint().x, -radAtH));
    PASS();
}

static void test_cone_hit_base()
{
    TEST("Cone: ray from below hits base cap");
    Cone cone(Vec3(0,0,0), Vec3(0,1,0), 30.0, 2.0);
    Ray ray(Vec3(0.5, 5, 0), Vec3(0, -1, 0));
    HitRecord hr;
    bool hit = cone.hit(ray, 0.0, 100.0, hr);
    CHECK(hit);
    CHECK(near(hr.getPoint().y, 2.0));
    PASS();
}

static void test_cone_miss_sideways()
{
    TEST("Cone: ray far from apex misses");
    Cone cone(Vec3(0,0,0), Vec3(0,1,0), 10.0, 2.0);
    Ray ray(Vec3(10, 1.0, 0), Vec3(0, 0, -1));
    HitRecord hr;
    bool hit = cone.hit(ray, 0.0, 100.0, hr);
    CHECK(!hit);
    PASS();
}

static void test_cone_normal_side()
{
    TEST("Cone: normal on side points outward");
    Cone cone(Vec3(0,0,0), Vec3(0,1,0), 30.0, 2.0);
    double h = 1.0;
    double r = h * std::tan(30.0 * M_PI / 180.0);
    Vec3 n = cone.getNormalAt(Vec3(r, h, 0.0));
    CHECK(n.x > 0);
    CHECK(n.y < 0);
    CHECK(near(n.magnitude(), 1.0));
    PASS();
}

static void test_cone_normal_base()
{
    TEST("Cone: normal at base center faces up");
    Cone cone(Vec3(0,0,0), Vec3(0,1,0), 30.0, 2.0);
    Vec3 n = cone.getNormalAt(Vec3(0, 2.0, 0));
    std::cout << "got (" << n.x << "," << n.y << "," << n.z << ") ";
    CHECK(nearVec(n, Vec3(0, 1, 0)));
    PASS();
}

// ============================================================
// Main
// ============================================================

int main()
{
    std::cout << "=== Cylinder Tests ===\n";
    test_cylinder_miss_above();
    test_cylinder_hit_body_side();
    test_cylinder_miss_sideways();
    test_cylinder_hit_lower_cap();
    test_cylinder_hit_upper_cap();
    test_cylinder_normal_body();
    test_cylinder_hit_body_angled();

    std::cout << "\n=== Cone Tests ===\n";
    test_cone_miss_above();
    test_cone_hit_side();
    test_cone_hit_base();
    test_cone_miss_sideways();
    test_cone_normal_side();
    test_cone_normal_base();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << testsPassed << "\n";
    std::cout << "Failed: " << testsFailed << "\n";

    return testsFailed > 0 ? 1 : 0;
}
