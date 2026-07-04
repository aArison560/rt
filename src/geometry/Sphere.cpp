/**
 * @file Sphere.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Sphere implementation
 */

#include "geometry/Sphere.hpp"
#include "core/HitRecord.hpp"
#include <cmath>
#include <optional>

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
    if (this == &other) return *this;
    AObject::operator=(other);
    center = other.center;
    radius = other.radius;
    return *this;
}

std::optional<HitRecord> Sphere::hit(const Ray& ray, double tMin, double tMax) const
{
    return computeIntersection(ray, tMin, tMax);
}

void Sphere::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    minCorner = center - Vec3(radius, radius, radius);
    maxCorner = center + Vec3(radius, radius, radius);
}

const char* Sphere::getType() const
{
    return "Sphere";
}

void Sphere::setCenter(const Vec3& c)
{
    center = c;
}

const Vec3& Sphere::getCenter() const
{
    return center;
}

void Sphere::setRadius(double r)
{
    radius = r;
}

double Sphere::getRadius() const
{
    return radius;
}

Vec3 Sphere::getNormalAt(const Vec3& point) const
{
    return (point - center).normalized();
}

void Sphere::getUVAt(const Vec3& point, double& u, double& v) const
{
    Vec3 p = (point - center).normalized();
    double theta = std::atan2(p.z, p.x);
    double phi = std::acos(p.y);
    u = 1.0 - (theta + M_PI) / (2.0 * M_PI);
    v = 1.0 - (phi) / M_PI;
}

std::optional<HitRecord> Sphere::computeIntersection(const Ray& ray, double tMin, double tMax) const
{
    Vec3 oc = ray.getOrigin() - center;
    Vec3 d = ray.getDirection();
    double a = d.dot(d);
    double b = 2.0 * oc.dot(d);
    double c = oc.dot(oc) - radius * radius;
    double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) return std::nullopt;
    double sqrtd = std::sqrt(discriminant);

    double root = (-b - sqrtd) / (2.0 * a);
    if (root < tMin || root > tMax) {
        root = (-b + sqrtd) / (2.0 * a);
        if (root < tMin || root > tMax) return std::nullopt;
    }

    Vec3 point = ray.pointAt(root);
    Vec3 outwardNormal = (point - center) / radius;
    outwardNormal.normalize();

    HitRecord hr(root, point, outwardNormal, material, this);

    // Determine front face
    bool front = ray.getDirection().dot(outwardNormal) < 0.0;
    hr.setFrontFace(front);

    double u, v;
    getUVAt(point, u, v);
    hr.setUV(u, v);

    return hr;
}
