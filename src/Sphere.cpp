/**
 * @file Sphere.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Sphere implementation
 */

#include "Sphere.hpp"
#include "HitRecord.hpp"
#include <cmath>

Sphere::Sphere() : center(0, 0, 0), radius(1.0) {}

Sphere::Sphere(const Vec3& center, double radius) : center(center), radius(radius) {}

Sphere::Sphere(const Vec3& center, double radius, std::shared_ptr<Material> material)
    : center(center), radius(radius)
{
    this->material = material;
}

Sphere::Sphere(const Sphere& other) : AObject(other), center(other.center), radius(other.radius) {}

Sphere& Sphere::operator=(const Sphere& other)
{
    // TODO: Assignment
    return *this;
}

bool Sphere::hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const
{
    // TODO: Implement ray-sphere intersection
    // Transform ray to object space using inverse transform
    // Solve quadratic equation: (p-c)·(p-c) - r² = 0
    // where p = o + td
    return computeIntersection(ray, tMin, tMax, hitRecord);
}

void Sphere::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    // TODO: Calculate bounding box
    minCorner = center - Vec3(radius, radius, radius);
    maxCorner = center + Vec3(radius, radius, radius);
}

const char* Sphere::getType() const
{
    return "Sphere";
}

void Sphere::setCenter(const Vec3& center)
{
    // TODO: Set center
}

const Vec3& Sphere::getCenter() const
{
    return center;
}

void Sphere::setRadius(double radius)
{
    // TODO: Set radius
}

double Sphere::getRadius() const
{
    return radius;
}

Vec3 Sphere::getNormalAt(const Vec3& point) const
{
    // TODO: Normal is (point - center) normalized
    return Vec3();
}

void Sphere::getUVAt(const Vec3& point, double& u, double& v) const
{
    // TODO: Calculate UV using spherical coordinates
    // u = atan2(z, x) / (2π) + 0.5
    // v = acos(y/r) / π
}

bool Sphere::computeIntersection(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const
{
    // TODO: Implement quadratic ray-sphere intersection
    return false;
}
