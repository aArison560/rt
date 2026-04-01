/**
 * @file Transform.hpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief Transformation class for object positioning and orientation
 *
 * Wraps transformation matrices and provides intuitive methods for
 * translation, rotation, and scaling. Stores transformation state
 * and allows composition of transformations.
 *
 * @relationships
 * - Composed of: Matrix4x4 (forward and inverse transforms)
 * - Used by: AObject and all subclasses (Sphere, Plane, Cylinder, Cone)
 * - Enables: object positioning, rotation, and scaling in world space
 */

#pragma once

#include "Matrix4x4.hpp"
#include "Vec3.hpp"

class Transform
{
public:
    /**
     * @brief Default constructor - creates identity transformation
     */
    Transform();

    /**
     * @brief Constructor from transformation matrix
     * @param matrix The transformation matrix
     */
    Transform(const Matrix4x4& matrix);

    /**
     * @brief Copy constructor
     * @param other The Transform to copy
     */
    Transform(const Transform& other);

    /**
     * @brief Assignment operator
     * @param other The Transform to assign
     * @return Reference to this Transform
     */
    Transform& operator=(const Transform& other);

    /**
     * @brief Compose this transformation with another
     * @param other The Transform to compose with
     * @return Reference to this Transform (chaining support)
     */
    Transform& compose(const Transform& other);

    /**
     * @brief Apply translation
     * @param x X offset
     * @param y Y offset
     * @param z Z offset
     * @return Reference to this Transform
     */
    Transform& translate(double x, double y, double z);

    /**
     * @brief Apply translation from vector
     * @param offset The translation vector
     * @return Reference to this Transform
     */
    Transform& translate(const Vec3& offset);

    /**
     * @brief Apply rotation around X axis
     * @param angleRadians Rotation angle in radians
     * @return Reference to this Transform
     */
    Transform& rotateX(double angleRadians);

    /**
     * @brief Apply rotation around Y axis
     * @param angleRadians Rotation angle in radians
     * @return Reference to this Transform
     */
    Transform& rotateY(double angleRadians);

    /**
     * @brief Apply rotation around Z axis
     * @param angleRadians Rotation angle in radians
     * @return Reference to this Transform
     */
    Transform& rotateZ(double angleRadians);

    /**
     * @brief Apply rotation around arbitrary axis
     * @param axis The rotation axis (should be normalized)
     * @param angleRadians Rotation angle in radians
     * @return Reference to this Transform
     */
    Transform& rotateAxis(const Vec3& axis, double angleRadians);

    /**
     * @brief Apply scaling
     * @param sx X scale factor
     * @param sy Y scale factor
     * @param sz Z scale factor
     * @return Reference to this Transform
     */
    Transform& scale(double sx, double sy, double sz);

    /**
     * @brief Apply scaling from vector
     * @param scale The scale factors vector
     * @return Reference to this Transform
     */
    Transform& scale(const Vec3& scale);

    /**
     * @brief Transform a 3D point
     * @param point The point to transform
     * @return Transformed point as Vec3
     */
    Vec3 transformPoint(const Vec3& point) const;

    /**
     * @brief Transform a 3D direction (ignores translation)
     * @param direction The direction to transform
     * @return Transformed direction as Vec3
     */
    Vec3 transformDirection(const Vec3& direction) const;

    /**
     * @brief Transform a point by inverse transformation
     * @param point The point to inverse transform
     * @return Inverse transformed point as Vec3
     */
    Vec3 inverseTransformPoint(const Vec3& point) const;

    /**
     * @brief Transform a direction by inverse transformation
     * @param direction The direction to inverse transform
     * @return Inverse transformed direction as Vec3
     */
    Vec3 inverseTransformDirection(const Vec3& direction) const;

    /**
     * @brief Get the forward (local to world) transformation matrix
     * @return Reference to the forward matrix
     */
    const Matrix4x4& getMatrix() const;

    /**
     * @brief Get the inverse (world to local) transformation matrix
     * @return Reference to the inverse matrix
     */
    const Matrix4x4& getInverseMatrix() const;

    /**
     * @brief Set the transformation matrix
     * @param matrix The new transformation matrix
     */
    void setMatrix(const Matrix4x4& matrix);

    /**
     * @brief Reset to identity transformation
     */
    void identity();

    /**
     * @brief Get the translation component
     * @return Translation as Vec3
     */
    Vec3 getTranslation() const;

    /**
     * @brief Get the scale factors (approximation)
     * @return Scale as Vec3
     */
    Vec3 getScale() const;

private:
    Matrix4x4 matrix;         ///< Forward transformation matrix
    Matrix4x4 inverseMatrix;  ///< Cached inverse matrix for efficiency

    /**
     * @brief Update the cached inverse matrix
     */
    void updateInverse();
};
