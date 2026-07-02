/**
 * @file Cylinder.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Cylinder implementation
 */

#include "geometry/Cylinder.hpp"
#include "core/HitRecord.hpp"

Cylinder::Cylinder() : center(0, 0, 0), axis(0, 1, 0), radius(1.0), height(2.0) {}

Cylinder::Cylinder(const Vec3& center, const Vec3& axis, double radius, double height)
    : center(center), axis(axis), radius(radius), height(height)
{
    this->axis.normalize();
}

Cylinder::Cylinder(const Vec3& center, const Vec3& axis, double radius, double height,
                   std::shared_ptr<Material> material)
    : center(center), axis(axis), radius(radius), height(height)
{
    this->axis.normalize();
    this->material = material;
}

Cylinder::Cylinder(const Cylinder& other) 
    : AObject(other), center(other.center), axis(other.axis), 
      radius(other.radius), height(other.height) {}

Cylinder& Cylinder::operator=(const Cylinder& other)
{
    if (this == &other) {
        return *this;
    }
    AObject::operator=(other);
    center = other.center;
    axis = other.axis;
    radius = other.radius;
    height = other.height;
    return *this;
}

bool Cylinder::hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const
{
    // TODO: Ray-cylinder intersection
    // 1. Intersect with infinite cylinder surface
    // 2. Check if within height bounds
    // 3. Test end caps
    (void)ray;
    (void)tMin;
    (void)tMax;
    (void)hitRecord;
    return false;
}

void Cylinder::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    minCorner = center - Vec3(radius, height * 0.5, radius);
    maxCorner = center + Vec3(radius, height * 0.5, radius);
}

const char* Cylinder::getType() const
{
    return "Cylinder";
}

void Cylinder::setCenter(const Vec3& center) { this->center = center; }
const Vec3& Cylinder::getCenter() const { return center; }
void Cylinder::setAxis(const Vec3& axis) { this->axis = axis; }
const Vec3& Cylinder::getAxis() const { return axis; }
void Cylinder::setRadius(double radius) { this->radius = radius; }
double Cylinder::getRadius() const { return radius; }
void Cylinder::setHeight(double height) { this->height = height; }
double Cylinder::getHeight() const { return height; }

Vec3 Cylinder::getNormalAt(const Vec3& point) const
{
    // TODO: Calculate surface normal at point
    (void)point;
    return Vec3();
}

void Cylinder::getUVAt(const Vec3& point, double& u, double& v) const
{
    // TODO: Calculate UV coordinates 
    (void)point;
    u = 0.0;
    v = 0.0;
}

int Cylinder::intersectCylinderBody(const Ray& ray, double tValues[2]) const
{
    // TODO: Intersect with cylinder body
    (void)ray;
    (void)tValues;
    return 0;
}

double Cylinder::intersectLowerCap(const Ray& ray) const
{
    // TODO: Intersect with bottom cap
    (void)ray;
    return -1.0;
}

double Cylinder::intersectUpperCap(const Ray& ray) const
{
    // TODO: Intersect with top cap
    (void)ray;
    return -1.0;
}

bool Cylinder::isWithinHeightBounds(const Vec3& point) const
{
    // TODO: Check if point is within height bounds
    (void)point;
    return false;
}
