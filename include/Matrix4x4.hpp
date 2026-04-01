/**
 * @file Matrix4x4.hpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief 4x4 transformation matrix class
 *
 * Provides a 4x4 homogeneous transformation matrix with operations for
 * translation, rotation, scaling, and matrix composition.
 *
 * @relationships
 * - Composed of: 16 doubles (4x4 elements)
 * - Used by: Transform class, camera transformations
 * - Supports: rotation matrices, translation, scaling, inverse calculations
 */

#pragma once

#include "Vec3.hpp"
#include <array>

class Matrix4x4
{
public:
    /**
     * @brief Default constructor - creates identity matrix
     */
    Matrix4x4();

    /**
     * @brief Constructor from explicit 16 values (row-major order)
     * @param values Array of 16 doubles in row-major order
     */
    Matrix4x4(const std::array<double, 16>& values);

    /**
     * @brief Copy constructor
     * @param other The Matrix4x4 to copy
     */
    Matrix4x4(const Matrix4x4& other);

    /**
     * @brief Assignment operator
     * @param other The Matrix4x4 to assign
     * @return Reference to this Matrix4x4
     */
    Matrix4x4& operator=(const Matrix4x4& other);

    /**
     * @brief Matrix multiplication operator
     * @param other The Matrix4x4 to multiply with
     * @return New Matrix4x4 representing the product
     */
    Matrix4x4 operator*(const Matrix4x4& other) const;

    /**
     * @brief Transform a 3D point by this matrix
     * @param point The point to transform
     * @return Transformed point as Vec3
     */
    Vec3 transformPoint(const Vec3& point) const;

    /**
     * @brief Transform a 3D direction (vector) by this matrix
     * Ignores translation component (uses 3x3 portion only)
     * @param direction The direction to transform
     * @return Transformed direction as Vec3
     */
    Vec3 transformDirection(const Vec3& direction) const;

    /**
     * @brief Transform a homogeneous point (with w coordinate)
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param w W coordinate (typically 1.0 for points, 0.0 for vectors)
     * @param outX Output X coordinate
     * @param outY Output Y coordinate
     * @param outZ Output Z coordinate
     * @param outW Output W coordinate
     */
    void transformHomogeneous(double x, double y, double z, double w,
                            double& outX, double& outY, double& outZ, double& outW) const;

    /**
     * @brief Calculate the inverse matrix
     * @return New Matrix4x4 representing the inverse
     * @throw std::runtime_error if matrix is singular
     */
    Matrix4x4 inverse() const;

    /**
     * @brief Calculate the transpose matrix
     * @return New Matrix4x4 representing the transpose
     */
    Matrix4x4 transpose() const;

    /**
     * @brief Calculate the determinant
     * @return The determinant value
     */
    double determinant() const;

    /**
     * @brief Get element at position (row, col)
     * @param row Row index (0-3)
     * @param col Column index (0-3)
     * @return The element value
     */
    double get(int row, int col) const;

    /**
     * @brief Set element at position (row, col)
     * @param row Row index (0-3)
     * @param col Column index (0-3)
     * @param value The value to set
     */
    void set(int row, int col, double value);

    /**
     * @brief Create identity matrix
     * @return New identity Matrix4x4
     */
    static Matrix4x4 identity();

    /**
     * @brief Create translation matrix
     * @param x X translation
     * @param y Y translation
     * @param z Z translation
     * @return New translation Matrix4x4
     */
    static Matrix4x4 translate(double x, double y, double z);

    /**
     * @brief Create translation matrix from vector
     * @param offset The translation vector
     * @return New translation Matrix4x4
     */
    static Matrix4x4 translate(const Vec3& offset);

    /**
     * @brief Create rotation matrix around X axis
     * @param angleRadians Rotation angle in radians
     * @return New rotation Matrix4x4
     */
    static Matrix4x4 rotateX(double angleRadians);

    /**
     * @brief Create rotation matrix around Y axis
     * @param angleRadians Rotation angle in radians
     * @return New rotation Matrix4x4
     */
    static Matrix4x4 rotateY(double angleRadians);

    /**
     * @brief Create rotation matrix around Z axis
     * @param angleRadians Rotation angle in radians
     * @return New rotation Matrix4x4
     */
    static Matrix4x4 rotateZ(double angleRadians);

    /**
     * @brief Create rotation matrix around arbitrary axis
     * @param axis The rotation axis (should be normalized)
     * @param angleRadians Rotation angle in radians
     * @return New rotation Matrix4x4
     */
    static Matrix4x4 rotateAxis(const Vec3& axis, double angleRadians);

    /**
     * @brief Create scaling matrix
     * @param sx X scale factor
     * @param sy Y scale factor
     * @param sz Z scale factor
     * @return New scaling Matrix4x4
     */
    static Matrix4x4 scale(double sx, double sy, double sz);

    /**
     * @brief Create scaling matrix from vector
     * @param scale The scale factors vector
     * @return New scaling Matrix4x4
     */
    static Matrix4x4 scale(const Vec3& scale);

    /**
     * @brief Create perspective projection matrix
     * @param fov Field of view in radians
     * @param aspect Aspect ratio (width/height)
     * @param near Near clipping plane
     * @param far Far clipping plane
     * @return New perspective Matrix4x4
     */
    static Matrix4x4 perspective(double fov, double aspect, double near, double far);

    /**
     * @brief Create orthographic projection matrix
     * @param left Left plane
     * @param right Right plane
     * @param bottom Bottom plane
     * @param top Top plane
     * @param near Near plane
     * @param far Far plane
     * @return New orthographic Matrix4x4
     */
    static Matrix4x4 orthographic(double left, double right, double bottom, double top,
                                 double near, double far);

private:
    std::array<double, 16> data;  ///< 4x4 matrix stored in row-major order
};
