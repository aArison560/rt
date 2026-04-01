/**
 * @file Plane.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Plane geometric primitive
 *
 * Represents an infinite plane defined by a point and surface normal.
 * Implements ray-plane intersection using dot product.
 *
 * @relationships
 * - Inherits from: AObject
 * - Uses: Transform (applies to point and normal), Material
 * - Composed of: point (Vec3), normal (Vec3)
 * - Creates: HitRecord on intersection
 */

#pragma once

#include "AObject.hpp"

class Plane : public AObject
{
public:
    /**
     * @brief Default constructor - creates XZ plane at origin (Y=0)
     */
    Plane();

    /**
     * @brief Constructor with point and normal
     * @param point A point on the plane
     * @param normal The surface normal (will be normalized)
     */
    Plane(const Vec3& point, const Vec3& normal);

    /**
     * @brief Constructor with point, normal, and material
     * @param point A point on the plane
     * @param normal The surface normal (will be normalized)
     * @param material Shared pointer to material
     */
    Plane(const Vec3& point, const Vec3& normal, std::shared_ptr<Material> material);

    /**
     * @brief Copy constructor
     * @param other The Plane to copy
     */
    Plane(const Plane& other);

    /**
     * @brief Assignment operator
     * @param other The Plane to assign
     * @return Reference to this Plane
     */
    Plane& operator=(const Plane& other);

    /**
     * @brief Virtual destructor
     */
    ~Plane() override = default;

    /**
     * @brief Test ray intersection with plane
     * @param ray The ray to intersect
     * @param tMin Minimum ray parameter
     * @param tMax Maximum ray parameter
     * @param hitRecord Output: intersection data if hit
     * @return true if intersection found
     */
    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const override;

    /**
     * @brief Get bounding box in object space
     * Note: Plane is infinite, so bounding box is symbolic
     * @param minCorner Output: minimum corner
     * @param maxCorner Output: maximum corner
     */
    void getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const override;

    /**
     * @brief Get object type identifier
     * @return "Plane"
     */
    const char* getType() const override;

    /**
     * @brief Set plane point
     * @param point A point on the plane
     */
    void setPoint(const Vec3& point);

    /**
     * @brief Get plane point
     * @return Point as Vec3
     */
    const Vec3& getPoint() const;

    /**
     * @brief Set plane normal
     * @param normal The surface normal (will be normalized)
     */
    void setNormal(const Vec3& normal);

    /**
     * @brief Get plane normal
     * @return Normal as Vec3 (normalized)
     */
    const Vec3& getNormal() const;

    /**
     * @brief Get surface normal at point (constant for plane)
     * @param point Point on plane (unused)
     * @return Surface normal
     */
    Vec3 getNormalAt(const Vec3& point) const override;

    /**
     * @brief Calculate signed distance from point to plane
     * @param point Point in space
     * @return Signed distance (positive = normal side, negative = opposite)
     */
    double distanceTo(const Vec3& point) const;

    /**
     * @brief Check if point is on the plane (within epsilon)
     * @param point Point to test
     * @return true if point is on plane
     */
    bool contains(const Vec3& point) const;

private:
    Vec3 point;   ///< A point on the plane
    Vec3 normal;  ///< Plane normal (normalized)
};
