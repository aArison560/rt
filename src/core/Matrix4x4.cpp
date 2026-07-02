/**
 * @file Matrix4x4.cpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief 4x4 transformation matrix implementation
 */

#include "core/Matrix4x4.hpp"
#include <cmath>
#include <stdexcept>
#include <algorithm>

Matrix4x4::Matrix4x4()
{
    data = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
}

Matrix4x4::Matrix4x4(const std::array<double, 16>& values) : data(values) {}

Matrix4x4::Matrix4x4(const Matrix4x4& other) : data(other.data) {}

Matrix4x4& Matrix4x4::operator=(const Matrix4x4& other)
{
    if (this == &other) {
        return *this;
    }
    data = other.data;
    return *this;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const
{
    Matrix4x4 out;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            double sum = 0.0;
            for (int k = 0; k < 4; ++k) {
                sum += get(r, k) * other.get(k, c);
            }
            out.set(r, c, sum);
        }
    }
    return out;
}

Vec3 Matrix4x4::transformPoint(const Vec3& point) const
{
    double outX, outY, outZ, outW;
    transformHomogeneous(point.x, point.y, point.z, 1.0, outX, outY, outZ, outW);
    if (std::abs(outW) > Vec3::EPSILON) {
        return Vec3(outX / outW, outY / outW, outZ / outW);
    }
    return Vec3(outX, outY, outZ);
}

Vec3 Matrix4x4::transformDirection(const Vec3& direction) const
{
    double x = get(0, 0) * direction.x + get(0, 1) * direction.y + get(0, 2) * direction.z;
    double y = get(1, 0) * direction.x + get(1, 1) * direction.y + get(1, 2) * direction.z;
    double z = get(2, 0) * direction.x + get(2, 1) * direction.y + get(2, 2) * direction.z;
    return Vec3(x, y, z);
}

void Matrix4x4::transformHomogeneous(double x, double y, double z, double w,
                                     double& outX, double& outY, double& outZ, double& outW) const
{
    outX = get(0, 0) * x + get(0, 1) * y + get(0, 2) * z + get(0, 3) * w;
    outY = get(1, 0) * x + get(1, 1) * y + get(1, 2) * z + get(1, 3) * w;
    outZ = get(2, 0) * x + get(2, 1) * y + get(2, 2) * z + get(2, 3) * w;
    outW = get(3, 0) * x + get(3, 1) * y + get(3, 2) * z + get(3, 3) * w;
}

Matrix4x4 Matrix4x4::inverse() const
{
    // Gauss-Jordan elimination on augmented matrix [A | I]
    double a[4][8] = {};
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            a[r][c] = get(r, c);
            a[r][c + 4] = (r == c) ? 1.0 : 0.0;
        }
    }

    for (int col = 0; col < 4; ++col) {
        int pivotRow = col;
        double maxAbs = std::abs(a[col][col]);
        for (int r = col + 1; r < 4; ++r) {
            double v = std::abs(a[r][col]);
            if (v > maxAbs) {
                maxAbs = v;
                pivotRow = r;
            }
        }
        if (maxAbs <= Vec3::EPSILON) {
            throw std::runtime_error("Matrix4x4::inverse singular matrix");
        }

        if (pivotRow != col) {
            for (int c = 0; c < 8; ++c) {
                std::swap(a[col][c], a[pivotRow][c]);
            }
        }

        double pivot = a[col][col];
        for (int c = 0; c < 8; ++c) {
            a[col][c] /= pivot;
        }

        for (int r = 0; r < 4; ++r) {
            if (r == col) {
                continue;
            }
            double factor = a[r][col];
            if (std::abs(factor) <= Vec3::EPSILON) {
                continue;
            }
            for (int c = 0; c < 8; ++c) {
                a[r][c] -= factor * a[col][c];
            }
        }
    }

    Matrix4x4 out;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out.set(r, c, a[r][c + 4]);
        }
    }
    return out;
}

Matrix4x4 Matrix4x4::transpose() const
{
    Matrix4x4 out;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out.set(r, c, get(c, r));
        }
    }
    return out;
}

double Matrix4x4::determinant() const
{
    // Laplace expansion by first row
    auto det3 = [](double a00, double a01, double a02,
                   double a10, double a11, double a12,
                   double a20, double a21, double a22) -> double {
        return a00 * (a11 * a22 - a12 * a21)
             - a01 * (a10 * a22 - a12 * a20)
             + a02 * (a10 * a21 - a11 * a20);
    };

    double m00 = get(0, 0), m01 = get(0, 1), m02 = get(0, 2), m03 = get(0, 3);
    double d0 = det3(get(1, 1), get(1, 2), get(1, 3), get(2, 1), get(2, 2), get(2, 3), get(3, 1), get(3, 2), get(3, 3));
    double d1 = det3(get(1, 0), get(1, 2), get(1, 3), get(2, 0), get(2, 2), get(2, 3), get(3, 0), get(3, 2), get(3, 3));
    double d2 = det3(get(1, 0), get(1, 1), get(1, 3), get(2, 0), get(2, 1), get(2, 3), get(3, 0), get(3, 1), get(3, 3));
    double d3 = det3(get(1, 0), get(1, 1), get(1, 2), get(2, 0), get(2, 1), get(2, 2), get(3, 0), get(3, 1), get(3, 2));
    return m00 * d0 - m01 * d1 + m02 * d2 - m03 * d3;
}

double Matrix4x4::get(int row, int col) const
{
    // TODO: Return element at (row, col)
    return data[row * 4 + col];
}

void Matrix4x4::set(int row, int col, double value)
{
    // TODO: Set element
    data[row * 4 + col] = value;
}

Matrix4x4 Matrix4x4::identity()
{
    return Matrix4x4();
}

Matrix4x4 Matrix4x4::translate(double x, double y, double z)
{
    Matrix4x4 out = identity();
    out.set(0, 3, x);
    out.set(1, 3, y);
    out.set(2, 3, z);
    return out;
}

Matrix4x4 Matrix4x4::translate(const Vec3& offset)
{
    // TODO: Create translation from vector
    return translate(offset.x, offset.y, offset.z);
}

Matrix4x4 Matrix4x4::rotateX(double angleRadians)
{
    Matrix4x4 out = identity();
    const double c = std::cos(angleRadians);
    const double s = std::sin(angleRadians);
    out.set(1, 1, c);
    out.set(1, 2, -s);
    out.set(2, 1, s);
    out.set(2, 2, c);
    return out;
}

Matrix4x4 Matrix4x4::rotateY(double angleRadians)
{
    Matrix4x4 out = identity();
    const double c = std::cos(angleRadians);
    const double s = std::sin(angleRadians);
    out.set(0, 0, c);
    out.set(0, 2, s);
    out.set(2, 0, -s);
    out.set(2, 2, c);
    return out;
}

Matrix4x4 Matrix4x4::rotateZ(double angleRadians)
{
    Matrix4x4 out = identity();
    const double c = std::cos(angleRadians);
    const double s = std::sin(angleRadians);
    out.set(0, 0, c);
    out.set(0, 1, -s);
    out.set(1, 0, s);
    out.set(1, 1, c);
    return out;
}

Matrix4x4 Matrix4x4::rotateAxis(const Vec3& axis, double angleRadians)
{
    Vec3 a = axis.normalized();
    const double c = std::cos(angleRadians);
    const double s = std::sin(angleRadians);
    const double t = 1.0 - c;

    Matrix4x4 out = identity();
    out.set(0, 0, t * a.x * a.x + c);
    out.set(0, 1, t * a.x * a.y - s * a.z);
    out.set(0, 2, t * a.x * a.z + s * a.y);

    out.set(1, 0, t * a.x * a.y + s * a.z);
    out.set(1, 1, t * a.y * a.y + c);
    out.set(1, 2, t * a.y * a.z - s * a.x);

    out.set(2, 0, t * a.x * a.z - s * a.y);
    out.set(2, 1, t * a.y * a.z + s * a.x);
    out.set(2, 2, t * a.z * a.z + c);
    return out;
}

Matrix4x4 Matrix4x4::scale(double sx, double sy, double sz)
{
    Matrix4x4 out = identity();
    out.set(0, 0, sx);
    out.set(1, 1, sy);
    out.set(2, 2, sz);
    return out;
}

Matrix4x4 Matrix4x4::scale(const Vec3& scale)
{
    // TODO: Scale from vector
    return Matrix4x4::scale(scale.x, scale.y, scale.z);
}

Matrix4x4 Matrix4x4::perspective(double fov, double aspect, double near, double far)
{
    Matrix4x4 out;
    const double f = 1.0 / std::tan(fov * 0.5);
    out.set(0, 0, f / aspect);
    out.set(0, 1, 0.0);
    out.set(0, 2, 0.0);
    out.set(0, 3, 0.0);

    out.set(1, 0, 0.0);
    out.set(1, 1, f);
    out.set(1, 2, 0.0);
    out.set(1, 3, 0.0);

    out.set(2, 0, 0.0);
    out.set(2, 1, 0.0);
    out.set(2, 2, (far + near) / (near - far));
    out.set(2, 3, (2.0 * far * near) / (near - far));

    out.set(3, 0, 0.0);
    out.set(3, 1, 0.0);
    out.set(3, 2, -1.0);
    out.set(3, 3, 0.0);
    return out;
}

Matrix4x4 Matrix4x4::orthographic(double left, double right, double bottom, double top,
                                  double near, double far)
{
    Matrix4x4 out = identity();
    out.set(0, 0, 2.0 / (right - left));
    out.set(1, 1, 2.0 / (top - bottom));
    out.set(2, 2, -2.0 / (far - near));
    out.set(0, 3, -(right + left) / (right - left));
    out.set(1, 3, -(top + bottom) / (top - bottom));
    out.set(2, 3, -(far + near) / (far - near));
    return out;
}
