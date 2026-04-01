/**
 * @file Cone.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Cone geometric primitive
 *
 * Represents a finite cone defined by apex, axis, half-angle, and height.
 * Implements ray-cone intersection with end cap testing.
 *
 * @relationships
 * - Inherits from: AObject
 * - Uses: Transform (applies to apex and axis), Material
 * - Composed of: apex (Vec3), axis (Vec3), halfAngle (double), height (double)
 * - Creates: HitRecord on intersection
 */

#pragma once

#include "AObject.hpp"

class Cone : public AObject
{
public:
    /**
     * @brief Default constructor - creates unit cone along positive Y axis
     */
    Cone();

    /**
     * @brief Constructor with full parameters
     * @param apex Tip of the cone
     * @param axis Direction of cone axis (will be normalized)
     * @param halfAngleDegrees Half-angle of cone in degrees
     * @param height Height of cone along axis
     */
    Cone(const Vec3& apex, const Vec3& axis, double halfAngleDegrees, double height);

    /**
     * @brief Constructor with apex, axis, half-angle, height, and material
     * @param apex Tip of the cone
     * @param axis Direction of cone axis (will be normalized)
     * @param halfAngleDegrees Half-angle of cone in degrees
     * @param height Height of cone along axis
     * @param material Shared pointer to material
     */
    Cone(const Vec3& apex, const Vec3& axis, double halfAngleDegrees, double height,
         std::shared_ptr<Material> material);

    /**
     * @brief Copy constructor
     * @param other The Cone to copy
     */
    Cone(const Cone& other);

    /**
     * @brief Assignment operator
     * @param other The Cone to assign
     * @return Reference to this Cone
     */
    Cone& operator=(const Cone& other);

    /**
     * @brief Virtual destructor
     */
    ~Cone() override = default;

    /**
     * @brief Test ray intersection with cone
     * @param ray The ray to intersect
     * @param tMin Minimum ray parameter
     * @param tMax Maximum ray parameter
     * @param hitRecord Output: intersection data if hit
     * @return true if intersection found
     */
    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const override;

    /**
     * @brief Get bounding box in object space
     * @param minCorner Output: minimum corner
     * @param maxCorner Output: maximum corner
     */
    void getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const override;

    /**
     * @brief Get object type identifier
     * @return "Cone"
     */
    const char* getType() const override;

    /**
     * @brief Set cone apex
     * @param apex The tip position
     */
    void setApex(const Vec3& apex);

    /**
     * @brief Get cone apex
     * @return Apex as Vec3
     */
    const Vec3& getApex() const;

    /**
     * @brief Set cone axis
     * @param axis Direction vector (will be normalized)
     */
    void setAxis(const Vec3& axis);

    /**
     * @brief Get cone axis
     * @return Axis as normalized Vec3
     */
    const Vec3& getAxis() const;

    /**
     * @brief Set cone half-angle
     * @param halfAngleDegrees Half-angle in degrees
     */
    void setHalfAngle(double halfAngleDegrees);

    /**
     * @brief Get cone half-angle
     * @return Half-angle in degrees
     */
    double getHalfAngle() const;

    /**
     * @brief Get cone half-angle in radians
     * @return Half-angle in radians
     */
    double getHalfAngleRadians() const;

    /**
     * @brief Set cone height
     * @param height The new height
     */
    void setHeight(double height);

    /**
     * @brief Get cone height
     * @return Height value
     */
    double getHeight() const;

    /**
     * @brief Get surface normal at point on cone
     * @param point Point on cone surface
     * @return Surface normal
     */
    Vec3 getNormalAt(const Vec3& point) const override;

    /**
     * @brief Get UV coordinates at point on cone surface
     * @param point Point on cone surface
     * @param u Output: U coordinate (angular)
     * @param v Output: V coordinate (along axis)
     */
    void getUVAt(const Vec3& point, double& u, double& v) const override;

private:
    Vec3 apex;              ///< Cone apex (tip)
    Vec3 axis;              ///< Cone axis direction (normalized)
    double halfAngleDegrees; ///< Half-angle in degrees
    double halfAngleRadians; ///< Half-angle in radians (cached)
    double cosHalfAngle;    ///< cos(halfAngle) (cached)
    double sinHalfAngle;    ///< sin(halfAngle) (cached)
    double height;          ///< Cone height

    /**
     * @brief Update cached trigonometric values
     */
    void updateAngleCache();

    /**
     * @brief Test intersection with cone surface
     * @param ray Ray in object space
     * @param tValues Array to store up to 2 t values
     * @return Number of intersections (0, 1, or 2)
     */
    int intersectConeSurface(const Ray& ray, double tValues[2]) const;

    /**
     * @brief Test intersection with base end cap
     * @param ray Ray in object space
     * @return Parameter t of intersection, or -1 if no intersection
     */
    double intersectBaseCap(const Ray& ray) const;

    /**
     * @brief Check if point is within height bounds
     * @param point Point to check
     * @return true if point's height is within [0, height]
     */
    bool isWithinHeightBounds(const Vec3& point) const;
};
