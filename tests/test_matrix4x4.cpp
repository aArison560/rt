#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "core/Matrix4x4.hpp"

TEST_CASE("Matrix4x4 default constructor is identity", "[matrix]")
{
    Matrix4x4 m;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            CHECK(m.get(r, c) == (r == c ? 1.0 : 0.0));
}

TEST_CASE("Matrix4x4 identity static", "[matrix]")
{
    Matrix4x4 m = Matrix4x4::identity();
    CHECK(m.get(0, 0) == 1.0);
    CHECK(m.get(1, 1) == 1.0);
    CHECK(m.get(2, 2) == 1.0);
    CHECK(m.get(3, 3) == 1.0);
    CHECK(m.get(0, 1) == 0.0);
}

TEST_CASE("Matrix4x4 copy and assignment", "[matrix]")
{
    std::array<double, 16> vals{};
    for (int i = 0; i < 16; ++i) vals[i] = static_cast<double>(i);
    Matrix4x4 a(vals);
    Matrix4x4 b(a);
    for (int i = 0; i < 16; ++i)
        CHECK(b.get(i / 4, i % 4) == static_cast<double>(i));

    Matrix4x4 c;
    c = a;
    for (int i = 0; i < 16; ++i)
        CHECK(c.get(i / 4, i % 4) == static_cast<double>(i));
}

TEST_CASE("Matrix4x4 multiplication", "[matrix]")
{
    Matrix4x4 id = Matrix4x4::identity();
    Matrix4x4 t = Matrix4x4::translate(3, 4, 5);
    Matrix4x4 r = id * t;
    CHECK(r.get(0, 3) == 3.0);
    CHECK(r.get(1, 3) == 4.0);
    CHECK(r.get(2, 3) == 5.0);
}

TEST_CASE("Matrix4x4 transformPoint", "[matrix]")
{
    Matrix4x4 t = Matrix4x4::translate(10, 20, 30);
    Vec3 p = t.transformPoint(Vec3(1, 2, 3));
    CHECK(p == Vec3(11, 22, 33));
}

TEST_CASE("Matrix4x4 transformDirection ignores translation", "[matrix]")
{
    Matrix4x4 t = Matrix4x4::translate(10, 20, 30);
    Vec3 d = t.transformDirection(Vec3(1, 0, 0));
    CHECK(d == Vec3(1, 0, 0));
}

TEST_CASE("Matrix4x4 inverse identity", "[matrix]")
{
    Matrix4x4 id;
    Matrix4x4 inv = id.inverse();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            CHECK(inv.get(r, c) == (r == c ? 1.0 : 0.0));
}

TEST_CASE("Matrix4x4 inverse translation", "[matrix]")
{
    Matrix4x4 t = Matrix4x4::translate(3, 4, 5);
    Matrix4x4 inv = t.inverse();
    Vec3 p = inv.transformPoint(Vec3(3, 4, 5));
    CHECK(p == Vec3(0, 0, 0));
}

TEST_CASE("Matrix4x4 inverse rotation", "[matrix]")
{
    Matrix4x4 r = Matrix4x4::rotateY(M_PI / 2);
    Matrix4x4 inv = r.inverse();
    Vec3 p = inv.transformPoint(Vec3(0, 0, 1));
    CHECK(p == Vec3(-1, 0, 0));
}

TEST_CASE("Matrix4x4 singular inverse throws", "[matrix]")
{
    std::array<double, 16> zero{};
    Matrix4x4 m(zero);
    CHECK_THROWS(m.inverse());
}

TEST_CASE("Matrix4x4 determinant", "[matrix]")
{
    Matrix4x4 id;
    CHECK(id.determinant() == Approx(1.0));
    Matrix4x4 t = Matrix4x4::translate(1, 2, 3);
    CHECK(t.determinant() == Approx(1.0));
    Matrix4x4 s = Matrix4x4::scale(2, 3, 4);
    CHECK(s.determinant() == Approx(24.0));
}

TEST_CASE("Matrix4x4 transpose", "[matrix]")
{
    Matrix4x4 t = Matrix4x4::translate(3, 4, 5);
    Matrix4x4 tr = t.transpose();
    CHECK(tr.get(3, 0) == 3.0);
    CHECK(tr.get(3, 1) == 4.0);
    CHECK(tr.get(3, 2) == 5.0);
    CHECK(tr.get(0, 3) == 0.0);
}

TEST_CASE("Matrix4x4 translate static", "[matrix]")
{
    Matrix4x4 t = Matrix4x4::translate(2, 3, 4);
    CHECK(t.get(0, 3) == 2.0);
    CHECK(t.get(1, 3) == 3.0);
    CHECK(t.get(2, 3) == 4.0);

    Matrix4x4 tv = Matrix4x4::translate(Vec3(5, 6, 7));
    CHECK(tv.get(0, 3) == 5.0);
}

TEST_CASE("Matrix4x4 rotation", "[matrix]")
{
    Matrix4x4 rx = Matrix4x4::rotateX(M_PI / 2);
    Vec3 p = rx.transformPoint(Vec3(0, 1, 0));
    CHECK(p == Vec3(0, 0, 1));

    Matrix4x4 ry = Matrix4x4::rotateY(M_PI / 2);
    p = ry.transformPoint(Vec3(1, 0, 0));
    CHECK(p == Vec3(0, 0, -1));

    Matrix4x4 rz = Matrix4x4::rotateZ(M_PI / 2);
    p = rz.transformPoint(Vec3(1, 0, 0));
    CHECK(p == Vec3(0, 1, 0));
}

TEST_CASE("Matrix4x4 rotateAxis", "[matrix]")
{
    Matrix4x4 r = Matrix4x4::rotateAxis(Vec3(0, 1, 0), M_PI / 2);
    Vec3 p = r.transformPoint(Vec3(1, 0, 0));
    CHECK(p == Vec3(0, 0, -1));
}

TEST_CASE("Matrix4x4 scale", "[matrix]")
{
    Matrix4x4 s = Matrix4x4::scale(2, 3, 4);
    Vec3 p = s.transformPoint(Vec3(1, 1, 1));
    CHECK(p == Vec3(2, 3, 4));

    Matrix4x4 sv = Matrix4x4::scale(Vec3(5, 6, 7));
    p = sv.transformPoint(Vec3(1, 1, 1));
    CHECK(p == Vec3(5, 6, 7));
}

TEST_CASE("Matrix4x4 perspective", "[matrix]")
{
    Matrix4x4 p = Matrix4x4::perspective(M_PI / 2, 16.0 / 9.0, 0.1, 100.0);
    double outX, outY, outZ, outW;
    p.transformHomogeneous(0, 0, -10, 1.0, outX, outY, outZ, outW);
    CHECK(outW == Approx(10.0));
}

TEST_CASE("Matrix4x4 orthographic", "[matrix]")
{
    Matrix4x4 o = Matrix4x4::orthographic(-1, 1, -1, 1, 0.1, 100.0);
    Vec3 p = o.transformPoint(Vec3(0, 0, -50));
    double expected = -2.0 / (100.0 - 0.1) * (-50) - (100.0 + 0.1) / (100.0 - 0.1);
    CHECK(p.z == Approx(expected));
}
