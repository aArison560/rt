/**
 * @file Transform.cpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief Transform implementation
 */

#include "scene/Transform.hpp"
#include <cmath>
#include <stdexcept>

Transform::Transform() : matrix(Matrix4x4::identity()), inverseMatrix(Matrix4x4::identity()) {}

Transform::Transform(const Matrix4x4& matrix) : matrix(matrix)
{
    updateInverse();
}

Transform::Transform(const Transform& other) 
    : matrix(other.matrix), inverseMatrix(other.inverseMatrix) {}

Transform& Transform::operator=(const Transform& other)
{
    if (this == &other) {
        return *this;
    }
    matrix = other.matrix;
    inverseMatrix = other.inverseMatrix;
    return *this;
}

Transform& Transform::compose(const Transform& other)
{
    matrix = matrix * other.matrix;
    updateInverse();
    return *this;
}

Transform& Transform::translate(double x, double y, double z)
{
    matrix = matrix * Matrix4x4::translate(x, y, z);
    updateInverse();
    return *this;
}

Transform& Transform::translate(const Vec3& offset)
{
    return translate(offset.x, offset.y, offset.z);
}

Transform& Transform::rotateX(double angleRadians)
{
    matrix = matrix * Matrix4x4::rotateX(angleRadians);
    updateInverse();
    return *this;
}

Transform& Transform::rotateY(double angleRadians)
{
    matrix = matrix * Matrix4x4::rotateY(angleRadians);
    updateInverse();
    return *this;
}

Transform& Transform::rotateZ(double angleRadians)
{
    matrix = matrix * Matrix4x4::rotateZ(angleRadians);
    updateInverse();
    return *this;
}

Transform& Transform::rotateAxis(const Vec3& axis, double angleRadians)
{
    matrix = matrix * Matrix4x4::rotateAxis(axis, angleRadians);
    updateInverse();
    return *this;
}

Transform& Transform::scale(double sx, double sy, double sz)
{
    matrix = matrix * Matrix4x4::scale(sx, sy, sz);
    updateInverse();
    return *this;
}

Transform& Transform::scale(const Vec3& scale)
{
    return Transform::scale(scale.x, scale.y, scale.z);
}

Vec3 Transform::transformPoint(const Vec3& point) const
{
    return matrix.transformPoint(point);
}

Vec3 Transform::transformDirection(const Vec3& direction) const
{
    return matrix.transformDirection(direction);
}

Vec3 Transform::inverseTransformPoint(const Vec3& point) const
{
    return inverseMatrix.transformPoint(point);
}

Vec3 Transform::inverseTransformDirection(const Vec3& direction) const
{
    return inverseMatrix.transformDirection(direction);
}

const Matrix4x4& Transform::getMatrix() const
{
    return matrix;
}

const Matrix4x4& Transform::getInverseMatrix() const
{
    return inverseMatrix;
}

void Transform::setMatrix(const Matrix4x4& matrix)
{
    this->matrix = matrix;
    updateInverse();
}

void Transform::identity()
{
    matrix = Matrix4x4::identity();
    inverseMatrix = Matrix4x4::identity();
}

Vec3 Transform::getTranslation() const
{
    return Vec3(matrix.get(0, 3), matrix.get(1, 3), matrix.get(2, 3));
}

Vec3 Transform::getScale() const
{
    Vec3 c0(matrix.get(0, 0), matrix.get(1, 0), matrix.get(2, 0));
    Vec3 c1(matrix.get(0, 1), matrix.get(1, 1), matrix.get(2, 1));
    Vec3 c2(matrix.get(0, 2), matrix.get(1, 2), matrix.get(2, 2));
    return Vec3(c0.magnitude(), c1.magnitude(), c2.magnitude());
}

void Transform::updateInverse()
{
    inverseMatrix = matrix.inverse();
}
