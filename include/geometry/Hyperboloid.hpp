/**
 * @file Hyperboloid.hpp
 * @author RT Team
 * @date 2026-07-10
 * @brief Hyperboloid geometric primitive
 *
 * Represents a hyperboloid surface defined by a general quadratic equation.
 * Implements ray-hyperboloid intersection using analytical geometry.
 *
 * Hyperboloid equation: Ax² + Bxy + Cxz + Dy² + Exz + Fy² + Gxz + Hx + Iy + Jz = 0
 *
 * @relationships
 * - Inherits from: AObject
 * - Uses: Transform (applies to center and coefficients), Material
 * - Composed of: center (Vec3), axis (Vec3), coeffs[10] (double)
 * - Creates: HitRecord on intersection
 *
 * @note Each hyperboloid is defined by 10 coefficients that fully specify its shape,
 *       position, and orientation in 3D space.
 */

#pragma once

#include "AObject.hpp"
#include "core/Ray.hpp"
#include <optional>

class HitRecord;

class Hyperboloid : public AObject
{
public:
    /**
     * @brief Default constructor - creates a standard hyperboloid along Z-axis
     */
    Hyperboloid();

    /**
     * @brief Constructor with center, axis, and coefficients
     * @param center Center point of hyperboloid
     * @param axis Rotation axis direction (will be normalized)
     * @param coeffs Array of 10 coefficients defining the hyperboloid equation
     */
    Hyperboloid(const Vec3& center, const Vec3& axis, const double coeffs[10]);

    /**
     * @brief Constructor with center, axis, coefficients, and material
     * @param center Center point of hyperboloid
     * @param axis Rotation axis direction (will be normalized)
     * @param coeffs Array of 10 coefficients defining the hyperboloid equation
     * @param material Shared pointer to material
     */
    Hyperboloid(const Vec3& center, const Vec3& axis, const double coeffs[10],
                std::shared_ptr<Material> material);

    /**
     * @brief Copy constructor
     * @param other The Hyperboloid to copy
     */
    Hyperboloid(const Hyperboloid& other);

    /**
     * @brief Assignment operator
     * @param other The Hyperboloid to assign
     * @return Reference to this Hyperboloid
     */
    Hyperboloid& operator=(const Hyperboloid& other);

    /**
     * @brief Virtual destructor
     */
    ~Hyperboloid() override = default;

    /**
     * @brief Test ray intersection with hyperboloid
     * @param ray The ray to intersect
     * @param tMin Minimum ray parameter
     * @param tMax Maximum ray parameter
     * @return Optional hit record containing intersection data
     */
    [[nodiscard]] std::optional<HitRecord> hit(const Ray& ray, double tMin, double tMax) const override;

    /**
     * @brief Get bounding box in object space
     * @param minCorner Output: minimum corner of bounding box
     * @param maxCorner Output: maximum corner of bounding box
     */
    void getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const override;

    /**
     * @brief Get object type identifier
     * @return String identifier of object type
     */
    [[nodiscard]] const char* getType() const override;

    /**
     * @brief Set hyperboloid center
     * @param center The new center
     */
    void setCenter(const Vec3& center);

    /**
     * @brief Get hyperboloid center
     * @return Center as Vec3
     */
    [[nodiscard]] const Vec3& getCenter() const;

    /**
     * @brief Set hyperboloid axis
     * @param axis Direction vector (will be normalized)
     */
    void setAxis(const Vec3& axis);

    /**
     * @brief Get hyperboloid axis
     * @return Axis as normalized Vec3
     */
    [[nodiscard]] const Vec3& getAxis() const;

    /**
     * @brief Set hyperboloid coefficients
     * @param coeffs Array of 10 coefficients
     */
    void setCoefficients(const double coeffs[10]);

    /**
     * @brief Get hyperboloid coefficients
     * @return Array of 10 coefficients
     */
    [[nodiscard]] const double* getCoefficients() const;

    /**
     * @brief Get surface normal at point (in object space)
     * @param point Point on hyperboloid surface
     * @return Surface normal
     */
    [[nodiscard]] Vec3 getNormalAt(const Vec3& point) const override;

    /**
     * @brief Get UV coordinates at point on hyperboloid surface
     * @param point Point on hyperboloid surface
     * @param u Output: U coordinate (angular around axis)
     * @param v Output: V coordinate (height parameter)
     */
    void getUVAt(const Vec3& point, double& u, double& v) const override;

private:
    /**
     * @brief Test intersection with hyperboloid surface
     * @param ray Ray in object space
     * @param tValues Array to store up to 2 t values
     * @return Number of intersections (0, 1, or 2)
     */
    [[nodiscard]] int intersectHyperboloid(const Ray& ray, double tValues[2]) const;

    Vec3 center;          ///< Hyperboloid center in object space
    Vec3 axis;            ///< Axis direction (normalized)
    double coeffs[10];    ///< Coefficients for hyperboloid equation:
                         ///< Ax² + Bxy + Cxz + Dy² + Exz + Fy² + Gxz + Hx + Iy + Jz = 0
};
