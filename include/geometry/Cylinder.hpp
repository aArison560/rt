/**
 * @file Cylinder.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Cylinder geometric primitive
 *
 * Represents a finite cylinder defined by center, axis, radius, and height.
 * Implements ray-cylinder intersection with end cap testing.
 *
 * @relationships
 * - Inherits from: AObject
 * - Uses: Transform (applies to center and axis), Material
 * - Composed of: center (Vec3), axis (Vec3), radius (double), height (double)
 * - Creates: HitRecord on intersection
 */

#pragma once

#include "AObject.hpp"

class Cylinder : public AObject
{
public:
    /**
     * @brief Default constructor - creates unit cylinder along Y axis
     */
    Cylinder();

    /**
     * @brief Constructor with full parameters
     * @param center Base center of cylinder
     * @param axis Direction of cylinder axis (will be normalized)
     * @param radius Cylinder radius
     * @param height Cylinder height
     */
    Cylinder(const Vec3& center, const Vec3& axis, double radius, double height);

    /**
     * @brief Constructor with center, axis, radius, height, and material
     * @param center Base center of cylinder
     * @param axis Direction of cylinder axis (will be normalized)
     * @param radius Cylinder radius
     * @param height Cylinder height
     * @param material Shared pointer to material
     */
    Cylinder(const Vec3& center, const Vec3& axis, double radius, double height,
             std::shared_ptr<Material> material);

    /**
     * @brief Copy constructor
     * @param other The Cylinder to copy
     */
    Cylinder(const Cylinder& other);

    /**
     * @brief Assignment operator
     * @param other The Cylinder to assign
     * @return Reference to this Cylinder
     */
    Cylinder& operator=(const Cylinder& other);

    /**
     * @brief Virtual destructor
     */
    ~Cylinder() override = default;

    /**
     * @brief Test ray intersection with cylinder
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
     * @return "Cylinder"
     */
    const char* getType() const override;

    /**
     * @brief Set cylinder center
     * @param center The base center
     */
    void setCenter(const Vec3& center);

    /**
     * @brief Get cylinder center
     * @return Center as Vec3
     */
    const Vec3& getCenter() const;

    /**
     * @brief Set cylinder axis
     * @param axis Direction vector (will be normalized)
     */
    void setAxis(const Vec3& axis);

    /**
     * @brief Get cylinder axis
     * @return Axis as normalized Vec3
     */
    const Vec3& getAxis() const;

    /**
     * @brief Set cylinder radius
     * @param radius The new radius
     */
    void setRadius(double radius);

    /**
     * @brief Get cylinder radius
     * @return Radius value
     */
    double getRadius() const;

    /**
     * @brief Set cylinder height
     * @param height The new height
     */
    void setHeight(double height);

    /**
     * @brief Get cylinder height
     * @return Height value
     */
    double getHeight() const;

    /**
     * @brief Get surface normal at point on cylinder
     * @param point Point on cylinder surface
     * @return Surface normal
     */
    Vec3 getNormalAt(const Vec3& point) const override;

    /**
     * @brief Get UV coordinates at point on cylinder surface
     * @param point Point on cylinder surface
     * @param u Output: U coordinate (angular)
     * @param v Output: V coordinate (along axis)
     */
    void getUVAt(const Vec3& point, double& u, double& v) const override;

private:
    Vec3 center;      ///< Base center of cylinder
    Vec3 axis;        ///< Cylinder axis direction (normalized)
    double radius;    ///< Cylinder radius
    double height;    ///< Cylinder height

    /**
     * @brief Test intersection with cylinder body (infinite surface)
     * @param ray Ray in object space
     * @param tValues Array to store up to 2 t values of intersection
     * @return Number of intersections found (0, 1, or 2)
     */
    int intersectCylinderBody(const Ray& ray, double tValues[2]) const;

    /**
     * @brief Test intersection with lower end cap
     * @param ray Ray in object space
     * @return Parameter t of intersection, or -1 if no intersection
     */
    double intersectLowerCap(const Ray& ray) const;

    /**
     * @brief Test intersection with upper end cap
     * @param ray Ray in object space
     * @return Parameter t of intersection, or -1 if no intersection
     */
    double intersectUpperCap(const Ray& ray) const;

    /**
     * @brief Check if point is within height bounds
     * @param point Point to check
     * @return true if point's height is within [0, height]
     */
    bool isWithinHeightBounds(const Vec3& point) const;
};
