/**
 * @file Plane.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Plane implementation
 */

#include "geometry/Plane.hpp"
#include "core/HitRecord.hpp"
#include <cmath>

Plane::Plane() : point(0, 0, 0), normal(0, 1, 0) {}

Plane::Plane(const Vec3& point, const Vec3& normal) : point(point), normal(normal)
{
    // TODO: Normalize normal
}

Plane::Plane(const Vec3& point, const Vec3& normal, std::shared_ptr<Material> material)
    : point(point), normal(normal)
{
    this->material = material;
}

Plane::Plane(const Plane& other) : AObject(other), point(other.point), normal(other.normal) {}

Plane& Plane::operator=(const Plane& other)
{
    // TODO: Assignment
    return *this;
}

bool Plane::hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const
{
    // TODO: Ray-plane intersection using dot product
    // t = (p - o) · n / (d · n)
    return false;
}

void Plane::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    // TODO: Plane is infinite, return symbolic bounding box
    minCorner = Vec3(-1e6, -1e6, -1e6);
    maxCorner = Vec3(1e6, 1e6, 1e6);
}

const char* Plane::getType() const
{
    return "Plane";
}

void Plane::setPoint(const Vec3& point)
{
    // TODO: Set plane point
}

const Vec3& Plane::getPoint() const
{
    return point;
}

void Plane::setNormal(const Vec3& normal)
{
    // TODO: Set and normalize normal
}

const Vec3& Plane::getNormal() const
{
    return normal;
}

Vec3 Plane::getNormalAt(const Vec3& point) const
{
    return normal;
}

double Plane::distanceTo(const Vec3& point) const
{
    // TODO: Calculate signed distance from point to plane
    return 0.0;
}

bool Plane::contains(const Vec3& point) const
{
    // TODO: Check if point is on plane within epsilon
    return false;
}
