/**
 * @file Sphere.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Sphere geometric primitive
 *
 * Represents a sphere defined by center and radius. Implements ray-sphere
 * intersection using the quadratic formula.
 *
 * @relationships
 * - Inherits from: AObject
 * - Uses: Transform (applies to center), Material
 * - Composed of: center (Vec3), radius (double)
 * - Creates: HitRecord on intersection
 */

#pragma once

#include "AObject.hpp"

class Sphere : public AObject
{
public:
    /**
     * @brief Default constructor - creates unit sphere at origin
     */
    Sphere();

    /**
     * @brief Constructor with center and radius
     * @param center Center point of sphere
     * @param radius Radius of sphere
     */
    Sphere(const Vec3& center, double radius);

    /**
     * @brief Constructor with center, radius, and material
     * @param center Center point of sphere
     * @param radius Radius of sphere
     * @param material Shared pointer to material
     */
    Sphere(const Vec3& center, double radius, std::shared_ptr<Material> material);

    /**
     * @brief Copy constructor
     * @param other The Sphere to copy
     */
    Sphere(const Sphere& other);

    /**
     * @brief Assignment operator
     * @param other The Sphere to assign
     * @return Reference to this Sphere
     */
    Sphere& operator=(const Sphere& other);

    /**
     * @brief Virtual destructor
     */
    ~Sphere() override = default;

    /**
     * @brief Test ray intersection with sphere
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
     * @return "Sphere"
     */
    const char* getType() const override;

    /**
     * @brief Set sphere center
     * @param center The new center
     */
    void setCenter(const Vec3& center);

    /**
     * @brief Get sphere center
     * @return Center as Vec3
     */
    const Vec3& getCenter() const;

    /**
     * @brief Set sphere radius
     * @param radius The new radius
     */
    void setRadius(double radius);

    /**
     * @brief Get sphere radius
     * @return Radius value
     */
    double getRadius() const;

    /**
     * @brief Get surface normal at point (in object space)
     * @param point Point on sphere surface
     * @return Surface normal
     */
    Vec3 getNormalAt(const Vec3& point) const override;

    /**
     * @brief Get UV coordinates at point on sphere surface
     * @param point Point on sphere surface
     * @param u Output: U coordinate (longitude)
     * @param v Output: V coordinate (latitude)
     */
    void getUVAt(const Vec3& point, double& u, double& v) const override;

private:
    Vec3 center;    ///< Sphere center in object space
    double radius;  ///< Sphere radius

    /**
     * @brief Compute ray-sphere intersection using quadratic formula
     * @param ray Ray in object space
     * @param tMin Minimum t parameter
     * @param tMax Maximum t parameter
     * @param hitRecord Output: intersection data
     * @return true if intersection found
     */
    bool computeIntersection(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const;
};
