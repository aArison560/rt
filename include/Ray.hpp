/**
 * @file Ray.hpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief Ray class for ray casting calculations
 *
 * Represents a ray in 3D space defined by an origin point and a direction vector.
 * Provides method to compute a point along the ray at parameter t.
 *
 * @relationships
 * - Composed of: Vec3 (origin and direction)
 * - Used by: Camera (ray generation), Renderer (ray tracing), AObject (intersection)
 * - Fundamental to ray casting algorithm
 */

#pragma once

#include "Vec3.hpp"

class Ray
{
public:
    /**
     * @brief Default constructor - creates a ray at origin pointing positive Z
     */
    Ray();

    /**
     * @brief Constructor with explicit origin and direction
     * @param origin The starting point of the ray
     * @param direction The direction vector of the ray (should be normalized)
     */
    Ray(const Vec3& origin, const Vec3& direction);

    /**
     * @brief Copy constructor
     * @param other The Ray to copy
     */
    Ray(const Ray& other);

    /**
     * @brief Assignment operator
     * @param other The Ray to assign
     * @return Reference to this Ray
     */
    Ray& operator=(const Ray& other);

    /**
     * @brief Get a point along the ray at parameter t
     * @param t The ray parameter (distance along direction from origin)
     * @return The point P = origin + t * direction
     */
    Vec3 pointAt(double t) const;

    /**
     * @brief Get the ray origin
     * @return Reference to the origin Vec3
     */
    const Vec3& getOrigin() const;

    /**
     * @brief Get the ray direction
     * @return Reference to the direction Vec3
     */
    const Vec3& getDirection() const;

    /**
     * @brief Set the ray origin
     * @param origin The new origin point
     */
    void setOrigin(const Vec3& origin);

    /**
     * @brief Set the ray direction
     * @param direction The new direction vector (should be normalized)
     */
    void setDirection(const Vec3& direction);

private:
    Vec3 origin;      ///< The starting point of the ray
    Vec3 direction;   ///< The normalized direction vector
};
