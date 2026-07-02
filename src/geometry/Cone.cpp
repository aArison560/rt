/**
 * @file Cone.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Cone implementation
 */

#include "geometry/Cone.hpp"
#include "core/HitRecord.hpp"
#include <algorithm>
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
    double tClosest = tMax;
    HitRecord best;
    bool found = false;

    double bodyTs[2];
    int bodyCount = intersectConeSurface(ray, bodyTs);

    for (int i = 0; i < bodyCount; ++i) {
        double t = bodyTs[i];
        if (t < tMin || t > tMax) continue;

        Vec3 p = ray.pointAt(t);
        Vec3 oc = p - apex;
        double h = oc.dot(axis);

        if (h < Vec3::EPSILON || h > height + Vec3::EPSILON) continue;
        if (!isWithinHeightBounds(p)) continue;

        if (t < tClosest) {
            tClosest = t;
            found = true;
            best.setT(t);
            best.setPoint(p);
        }
    }

    double tCap = intersectBaseCap(ray);
    if (tCap >= tMin && tCap <= tMax && tCap < tClosest) {
        Vec3 p = ray.pointAt(tCap);
        Vec3 oc = p - apex;
        double h = oc.dot(axis);
        Vec3 radial = oc - axis * h;
        double baseRadius = height * sinHalfAngle / cosHalfAngle;

        if (h >= height - Vec3::EPSILON && h <= height + Vec3::EPSILON &&
            radial.magnitudeSquared() <= baseRadius * baseRadius + Vec3::EPSILON) {
            tClosest = tCap;
            found = true;
            best.setT(tCap);
            best.setPoint(p);
        }
    }

    if (found) {
        Vec3 oc = best.getPoint() - apex;
        double h = oc.dot(axis);
        Vec3 normal;

        if (std::abs(h - height) < Vec3::EPSILON) {
            normal = -axis;
        } else {
            Vec3 radial = oc - axis * h;
            double tanSq = (sinHalfAngle * sinHalfAngle) / (cosHalfAngle * cosHalfAngle);
            normal = (radial - axis * (tanSq * h)).normalized();
        }

        best.setNormal(normal);
        best.setMaterial(material.get());
        best.setObject(const_cast<Cone*>(this));

        double u, v;
        getUVAt(best.getPoint(), u, v);
        best.setUV(u, v);

        best.setFrontFace(normal.dot(-ray.getDirection()) > 0.0);
        hitRecord = best;
    }

    return found;
}

void Cone::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    Vec3 baseCenter = apex + axis * height;
    double baseRadius = height * sinHalfAngle / cosHalfAngle;

    double minX = std::min(apex.x, baseCenter.x) - baseRadius;
    double minY = std::min(apex.y, baseCenter.y) - baseRadius;
    double minZ = std::min(apex.z, baseCenter.z) - baseRadius;
    double maxX = std::max(apex.x, baseCenter.x) + baseRadius;
    double maxY = std::max(apex.y, baseCenter.y) + baseRadius;
    double maxZ = std::max(apex.z, baseCenter.z) + baseRadius;

    minCorner = Vec3(minX, minY, minZ);
    maxCorner = Vec3(maxX, maxY, maxZ);
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
    Vec3 oc = point - apex;
    double h = oc.dot(axis);
    Vec3 radial = oc - axis * h;
    double rLen = radial.magnitude();

    if (std::abs(h - height) < Vec3::EPSILON)
        return -axis;

    if (rLen < Vec3::EPSILON)
        return axis;

    double tanSq = (sinHalfAngle * sinHalfAngle) / (cosHalfAngle * cosHalfAngle);
    return (radial - axis * (tanSq * h)).normalized();
}

void Cone::getUVAt(const Vec3& point, double& u, double& v) const
{
    Vec3 oc = point - apex;
    double h = oc.dot(axis);
    Vec3 radial = oc - axis * h;
    double rLen = radial.magnitude();

    Vec3 ref(1, 0, 0);
    if (std::abs(axis.dot(ref)) > 0.9)
        ref = Vec3(0, 0, 1);
    Vec3 uDir = axis.cross(ref).normalized();
    Vec3 vDir = uDir.cross(axis).normalized();

    if (rLen < Vec3::EPSILON) {
        u = 0.5;
        v = 0.0;
        return;
    }

    double angle = std::atan2(radial.dot(vDir), radial.dot(uDir));
    u = angle / (2.0 * M_PI) + 0.5;

    if (std::abs(h - height) < Vec3::EPSILON) {
        double baseRadius = height * sinHalfAngle / cosHalfAngle;
        double pu = radial.dot(uDir) / baseRadius;
        double pv = radial.dot(vDir) / baseRadius;
        u = (pu + 1.0) / 2.0;
        v = (pv + 1.0) / 2.0;
    } else {
        v = height > Vec3::EPSILON ? h / height : 0.0;
    }
}

void Cone::updateAngleCache()
{
    halfAngleRadians = halfAngleDegrees * M_PI / 180.0;
    sinHalfAngle = std::sin(halfAngleRadians);
    cosHalfAngle = std::cos(halfAngleRadians);
}

int Cone::intersectConeSurface(const Ray& ray, double tValues[2]) const
{
    Vec3 oc = ray.getOrigin() - apex;
    double dDotA = ray.getDirection().dot(axis);
    double ocDotA = oc.dot(axis);

    double dMagSq = ray.getDirection().dot(ray.getDirection());
    double ocMagSq = oc.dot(oc);
    double od = oc.dot(ray.getDirection());

    double invCos2 = 1.0 / (cosHalfAngle * cosHalfAngle);

    double a = dMagSq - dDotA * dDotA * invCos2;
    if (std::abs(a) < Vec3::EPSILON)
        return 0;

    double b = 2.0 * (od - ocDotA * dDotA * invCos2);
    double c = ocMagSq - ocDotA * ocDotA * invCos2;

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

double Cone::intersectBaseCap(const Ray& ray) const
{
    double denom = ray.getDirection().dot(axis);
    if (std::abs(denom) < Vec3::EPSILON)
        return -1.0;

    Vec3 baseCenter = apex + axis * height;
    return (baseCenter - ray.getOrigin()).dot(axis) / denom;
}

bool Cone::isWithinHeightBounds(const Vec3& point) const
{
    double h = (point - apex).dot(axis);
    return h >= 0.0 && h <= height;
}
