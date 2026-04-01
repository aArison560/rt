/**
 * @file Ray.cpp
 * @author RT Team - Dev A
 * @date 2026-04-01
 * @brief Ray implementation
 */

#include "core/Ray.hpp"

Ray::Ray() : origin(Vec3()), direction(Vec3(0, 0, 1))
{
    // Default ray at origin pointing positive Z
}

Ray::Ray(const Vec3& origin, const Vec3& direction) 
    : origin(origin), direction(direction)
{
    // TODO: Normalize direction if needed
}

Ray::Ray(const Ray& other) : origin(other.origin), direction(other.direction) {}

Ray& Ray::operator=(const Ray& other)
{
    // TODO: Assignment
    return *this;
}

Vec3 Ray::pointAt(double t) const
{
    // TODO: Return origin + t * direction
    return origin;
}

const Vec3& Ray::getOrigin() const
{
    return origin;
}

const Vec3& Ray::getDirection() const
{
    return direction;
}

void Ray::setOrigin(const Vec3& origin)
{
    // TODO: Set origin
}

void Ray::setDirection(const Vec3& direction)
{
    // TODO: Set and normalize direction
}
