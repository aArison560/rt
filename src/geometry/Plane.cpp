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
    this->normal.normalize();
}

Plane::Plane(const Vec3& point, const Vec3& normal, std::shared_ptr<Material> material)
    : point(point), normal(normal)
{
    this->normal.normalize();
    this->material = material;
}

Plane::Plane(const Plane& other) : AObject(other), point(other.point), normal(other.normal) {}

Plane& Plane::operator=(const Plane& other)
{
    if (this == &other) {
        return *this;
    }
    AObject::operator=(other);
    point = other.point;
    normal = other.normal;
    return *this;
}

bool Plane::hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const
{
    const double denom = ray.getDirection().dot(normal);
    if (std::abs(denom) <= Vec3::EPSILON) {
        return false;
    }

    const double t = (point - ray.getOrigin()).dot(normal) / denom;
    if (t < tMin || t > tMax) {
        return false;
    }

    const Vec3 hitPoint = ray.pointAt(t);
    const bool frontFace = ray.getDirection().dot(normal) < 0.0;
    const Vec3 outward = normal;
    const Vec3 finalNormal = frontFace ? outward : -outward;

    hitRecord = HitRecord(t, hitPoint, finalNormal, material.get(), const_cast<Plane*>(this));
    hitRecord.setFrontFace(frontFace);
    hitRecord.setUV(0.0, 0.0);
    return true;
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
    this->point = point;
}

const Vec3& Plane::getPoint() const
{
    return point;
}

void Plane::setNormal(const Vec3& normal)
{
    this->normal = normal;
    this->normal.normalize();
}

const Vec3& Plane::getNormal() const
{
    return normal;
}

Vec3 Plane::getNormalAt(const Vec3& point) const
{
    (void)point;
    return normal;
}

double Plane::distanceTo(const Vec3& point) const
{
    return (point - this->point).dot(normal);
}

bool Plane::contains(const Vec3& point) const
{
    return std::abs(distanceTo(point)) <= Vec3::EPSILON;
}
