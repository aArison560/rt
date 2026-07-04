/**
 * @file Cylinder.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Cylinder implementation
 */

#include "geometry/Cylinder.hpp"
#include "core/HitRecord.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

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

std::optional<HitRecord> Cylinder::hit(const Ray& ray, double tMin, double tMax) const
{
    double tClosest = tMax;
    std::optional<HitRecord> best;
    bool found = false;

    double bodyTs[2];
    int bodyCount = intersectCylinderBody(ray, bodyTs);

    for (int i = 0; i < bodyCount; ++i) {
        double t = bodyTs[i];
        if (t < tMin || t > tMax) continue;

        Vec3 p = ray.pointAt(t);
        if (!isWithinHeightBounds(p)) continue;

        if (t < tClosest) {
            tClosest = t;
            found = true;
            best.emplace(t, p, Vec3(), material, this);
        }
    }

    double tLower = intersectLowerCap(ray);
    if (tLower >= tMin && tLower <= tMax && tLower < tClosest) {
        Vec3 p = ray.pointAt(tLower);
        if ((p - center).magnitudeSquared() <= radius * radius + Vec3::EPSILON) {
            tClosest = tLower;
            found = true;
            best.emplace(tLower, p, Vec3(), material, this);
        }
    }

    double tUpper = intersectUpperCap(ray);
    if (tUpper >= tMin && tUpper <= tMax && tUpper < tClosest) {
        Vec3 p = ray.pointAt(tUpper);
        Vec3 capCenter = center + axis * height;
        if ((p - capCenter).magnitudeSquared() <= radius * radius + Vec3::EPSILON) {
            tClosest = tUpper;
            found = true;
            best.emplace(tUpper, p, Vec3(), material, this);
        }
    }

    if (found && best) {
        Vec3 point = best->getPoint();
        Vec3 normal = getNormalAt(point);
        best->setNormal(normal);
        best->setMaterial(material);
        best->setObject(this);

        double u, v;
        getUVAt(point, u, v);
        best->setUV(u, v);

        best->setFrontFace(normal.dot(-ray.getDirection()) > 0.0);
    }

    return best;
}

void Cylinder::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    Vec3 halfH = axis * (height * 0.5);
    Vec3 r(radius, radius, radius);
    minCorner = center - r - halfH;
    maxCorner = center + r + halfH;
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
    Vec3 oc = point - center;
    double h = oc.dot(axis);

    if (h < Vec3::EPSILON)
        return -axis;
    if (h > height - Vec3::EPSILON)
        return axis;

    Vec3 radial = oc - axis * h;
    double rLen = radial.magnitude();
    if (rLen < Vec3::EPSILON)
        return axis;

    return radial / rLen;
}

void Cylinder::getUVAt(const Vec3& point, double& u, double& v) const
{
    Vec3 oc = point - center;
    double h = oc.dot(axis);
    Vec3 radial = oc - axis * h;
    double rLen = radial.magnitude();

    if (rLen < Vec3::EPSILON) {
        u = 0.5;
        v = 0.5;
        return;
    }

    Vec3 ref(1, 0, 0);
    if (std::abs(axis.dot(ref)) > 0.9)
        ref = Vec3(0, 0, 1);
    Vec3 uDir = axis.cross(ref).normalized();
    Vec3 vDir = uDir.cross(axis).normalized();

    if (h >= 0.0 - Vec3::EPSILON && h <= height + Vec3::EPSILON &&
        std::abs(rLen - radius) < Vec3::EPSILON) {
        double angle = std::atan2(radial.dot(vDir), radial.dot(uDir));
        u = angle / (2.0 * M_PI) + 0.5;
        v = height > Vec3::EPSILON ? h / height : 0.0;
    } else {
        double pu = radial.dot(uDir) / radius;
        double pv = radial.dot(vDir) / radius;
        u = (pu + 1.0) / 2.0;
        v = (pv + 1.0) / 2.0;
    }
}

int Cylinder::intersectCylinderBody(const Ray& ray, double tValues[2]) const
{
    Vec3 oc = ray.getOrigin() - center;
    double dDotA = ray.getDirection().dot(axis);
    double ocDotA = oc.dot(axis);

    Vec3 dPerp = ray.getDirection() - axis * dDotA;
    Vec3 ocPerp = oc - axis * ocDotA;

    double a = dPerp.dot(dPerp);
    if (a < Vec3::EPSILON)
        return 0;

    double b = 2.0 * ocPerp.dot(dPerp);
    double c = ocPerp.dot(ocPerp) - radius * radius;

    double disc = b * b - 4.0 * a * c;
    if (disc < 0.0)
        return 0;

    double sqrtDisc = std::sqrt(disc);
    double t0 = (-b - sqrtDisc) / (2.0 * a);
    double t1 = (-b + sqrtDisc) / (2.0 * a);

    if (t0 > t1) std::swap(t0, t1);

    tValues[0] = t0;
    tValues[1] = t1;
    return 2;
}

double Cylinder::intersectLowerCap(const Ray& ray) const
{
    double denom = ray.getDirection().dot(axis);
    if (std::abs(denom) < Vec3::EPSILON)
        return -1.0;

    return (center - ray.getOrigin()).dot(axis) / denom;
}

double Cylinder::intersectUpperCap(const Ray& ray) const
{
    double denom = ray.getDirection().dot(axis);
    if (std::abs(denom) < Vec3::EPSILON)
        return -1.0;

    Vec3 capCenter = center + axis * height;
    return (capCenter - ray.getOrigin()).dot(axis) / denom;
}

bool Cylinder::isWithinHeightBounds(const Vec3& point) const
{
    double h = (point - center).dot(axis);
    return h >= 0.0 && h <= height;
}
