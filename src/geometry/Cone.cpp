/**
 * @file Cone.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Cone implementation
 */

#include "geometry/Cone.hpp"
#include "core/HitRecord.hpp"
#include <cmath>

Cone::Cone() : apex(0, 0, 0), axis(0, 1, 0), halfAngleDegrees(45.0), height(2.0)
{
    updateAngleCache();
}

Cone::Cone(const Vec3& apex, const Vec3& axis, double halfAngleDegrees, double height)
    : apex(apex), axis(axis), halfAngleDegrees(halfAngleDegrees), height(height)
{
    this->axis.normalize();
    updateAngleCache();
}

Cone::Cone(const Vec3& apex, const Vec3& axis, double halfAngleDegrees, double height,
           std::shared_ptr<Material> material)
    : apex(apex), axis(axis), halfAngleDegrees(halfAngleDegrees), height(height)
{
    this->axis.normalize();
    this->material = material;
    updateAngleCache();
}

Cone::Cone(const Cone& other)
    : AObject(other), apex(other.apex), axis(other.axis), 
      halfAngleDegrees(other.halfAngleDegrees), halfAngleRadians(other.halfAngleRadians),
      cosHalfAngle(other.cosHalfAngle), sinHalfAngle(other.sinHalfAngle), height(other.height) {}

Cone& Cone::operator=(const Cone& other)
{
    if (this == &other) {
        return *this;
    }
    AObject::operator=(other);
    apex = other.apex;
    axis = other.axis;
    halfAngleDegrees = other.halfAngleDegrees;
    halfAngleRadians = other.halfAngleRadians;
    cosHalfAngle = other.cosHalfAngle;
    sinHalfAngle = other.sinHalfAngle;
    height = other.height;
    return *this;
}

bool Cone::hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const
{
    // TODO: Ray-cone intersection
    // 1. Intersect with cone surface
    // 2. Check if within height bounds
    // 3. Test base cap
    (void)ray;
    (void)tMin;
    (void)tMax;
    (void)hitRecord;
    return false;
}

void Cone::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    // TODO: Calculate bounding box
    minCorner = apex - Vec3(height, 0.0, height);
    maxCorner = apex + Vec3(height, height, height);
}

const char* Cone::getType() const
{
    return "Cone";
}

void Cone::setApex(const Vec3& apex) { this->apex = apex; }
const Vec3& Cone::getApex() const { return apex; }
void Cone::setAxis(const Vec3& axis) { this->axis = axis; }
const Vec3& Cone::getAxis() const { return axis; }
void Cone::setHalfAngle(double halfAngleDegrees) { 
    this->halfAngleDegrees = halfAngleDegrees; 
    updateAngleCache();
}
double Cone::getHalfAngle() const { return halfAngleDegrees; }
double Cone::getHalfAngleRadians() const { return halfAngleRadians; }
void Cone::setHeight(double height) { this->height = height; }
double Cone::getHeight() const { return height; }

Vec3 Cone::getNormalAt(const Vec3& point) const
{
    // TODO: Calculate surface normal at point
    (void)point;
    return Vec3();
}

void Cone::getUVAt(const Vec3& point, double& u, double& v) const
{
    // TODO: Calculate UV coordinates
    (void)point;
    u = 0.0;
    v = 0.0;
}

void Cone::updateAngleCache()
{
    // TODO: Convert degrees to radians and cache sin/cos
    halfAngleRadians = halfAngleDegrees * M_PI / 180.0;
    sinHalfAngle = std::sin(halfAngleRadians);
    cosHalfAngle = std::cos(halfAngleRadians);
}

int Cone::intersectConeSurface(const Ray& ray, double tValues[2]) const
{
    // TODO: Intersect with cone surface
    (void)ray;
    (void)tValues;
    return 0;
}

double Cone::intersectBaseCap(const Ray& ray) const
{
    // TODO: Intersect with base cap
    (void)ray;
    return -1.0;
}

bool Cone::isWithinHeightBounds(const Vec3& point) const
{
    // TODO: Check if point is within height bounds
    (void)point;
    return false;
}
