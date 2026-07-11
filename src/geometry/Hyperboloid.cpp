/**
 * @file Hyperboloid.cpp
 * @author RT Team
 * @date 2026-07-10
 * @brief Hyperboloid implementation
 *
 * Ray-hyperboloid intersection and surface calculations using analytical geometry.
 */

#include "geometry/Hyperboloid.hpp"
#include "core/HitRecord.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

Hyperboloid::Hyperboloid() : center(0, 0, 0), axis(0, 0, 1)
{
    coeffs[0] = 1.0;    // x^2 coefficient
    coeffs[1] = 0.0;    // xy coefficient
    coeffs[2] = 0.0;    // xz coefficient
    coeffs[3] = 1.0;    // y^2 coefficient
    coeffs[4] = 0.0;    // xz coefficient
    coeffs[5] = -1.0;   // z^2 coefficient
    coeffs[6] = 0.0;    // xz coefficient
    coeffs[7] = 0.0;    // x coefficient
    coeffs[8] = 0.0;    // y coefficient
    coeffs[9] = 0.0;    // z coefficient
    axis.normalize();
}

Hyperboloid::Hyperboloid(const Vec3& center, const Vec3& axis, const double coeffs[10])
    : center(center), axis(axis)
{
    for (int i = 0; i < 10; ++i) {
        this->coeffs[i] = coeffs[i];
    }
    this->axis.normalize();
}

Hyperboloid::Hyperboloid(const Vec3& center, const Vec3& axis, const double coeffs[10],
                         std::shared_ptr<Material> material)
    : center(center), axis(axis)
{
    for (int i = 0; i < 10; ++i) {
        this->coeffs[i] = coeffs[i];
    }
    this->axis.normalize();
    this->material = material;
}

Hyperboloid::Hyperboloid(const Hyperboloid& other)
    : AObject(other), center(other.center), axis(other.axis)
{
    for (int i = 0; i < 10; ++i) {
        coeffs[i] = other.coeffs[i];
    }
}

Hyperboloid& Hyperboloid::operator=(const Hyperboloid& other)
{
    if (this == &other) {
        return *this;
    }
    AObject::operator=(other);
    center = other.center;
    axis = other.axis;
    for (int i = 0; i < 10; ++i) {
        coeffs[i] = other.coeffs[i];
    }
    return *this;
}

std::optional<HitRecord> Hyperboloid::hit(const Ray& ray, double tMin, double tMax) const
{
    double tClosest = tMax;
    std::optional<HitRecord> best;
    bool found = false;

    double tValues[2];
    int hitCount = intersectHyperboloid(ray, tValues);

    for (int i = 0; i < hitCount; ++i) {
        double t = tValues[i];
        if (t < tMin || t > tMax) continue;

        Vec3 p = ray.pointAt(t);

        if (t < tClosest) {
            tClosest = t;
            found = true;
            best.emplace(t, p, Vec3(), material, this);
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

void Hyperboloid::getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    double scale = 3.0;
    minCorner = center - Vec3(scale, scale, scale);
    maxCorner = center + Vec3(scale, scale, scale);

    for (int i = 0; i < 10; ++i) {
        if (std::abs(coeffs[i]) > 0.001) {
            scale *= 2.0;
            minCorner = center - Vec3(scale, scale, scale);
            maxCorner = center + Vec3(scale, scale, scale);
            break;
        }
    }
}

const char* Hyperboloid::getType() const
{
    return "Hyperboloid";
}

void Hyperboloid::setCenter(const Vec3& center)
{
    this->center = center;
}

const Vec3& Hyperboloid::getCenter() const
{
    return center;
}

void Hyperboloid::setAxis(const Vec3& axis)
{
    this->axis = axis;
    this->axis.normalize();
}

const Vec3& Hyperboloid::getAxis() const
{
    return axis;
}

void Hyperboloid::setCoefficients(const double coeffs[10])
{
    for (int i = 0; i < 10; ++i) {
        this->coeffs[i] = coeffs[i];
    }
}

const double* Hyperboloid::getCoefficients() const
{
    return coeffs;
}

Vec3 Hyperboloid::getNormalAt(const Vec3& point) const
{
    Vec3 oc = point - center;

    double normalX = 2.0 * coeffs[0] * oc.x + coeffs[1] * oc.y + coeffs[2] * oc.z + coeffs[6];
    double normalY = coeffs[1] * oc.x + 2.0 * coeffs[3] * oc.y + coeffs[5] * oc.z + coeffs[8];
    double normalZ = coeffs[2] * oc.x + coeffs[5] * oc.y + 2.0 * coeffs[4] * oc.z + coeffs[9];

    return Vec3(normalX, normalY, normalZ).normalized();
}

void Hyperboloid::getUVAt(const Vec3& point, double& u, double& v) const
{
    Vec3 oc = point - center;
    double angle = atan2(oc.dot(axis.cross(Vec3(1, 0, 0))), oc.dot(axis.cross(Vec3(0, 1, 0))));
    u = (angle + M_PI) / (2.0 * M_PI);

    double dotProduct = oc.dot(axis);
    v = (dotProduct + 3.0) / 6.0;
}

int Hyperboloid::intersectHyperboloid(const Ray& ray, double tValues[2]) const
{
    Vec3 oc = ray.getOrigin() - center;

    double a = coeffs[0] * ray.getDirection().x * ray.getDirection().x +
               coeffs[1] * ray.getDirection().x * ray.getDirection().y +
               coeffs[2] * ray.getDirection().x * ray.getDirection().z +
               coeffs[3] * ray.getDirection().y * ray.getDirection().y +
               coeffs[4] * ray.getDirection().y * ray.getDirection().z +
               coeffs[5] * ray.getDirection().z * ray.getDirection().z +
               coeffs[6] * ray.getDirection().x +
               coeffs[7] * ray.getDirection().y +
               coeffs[8] * ray.getDirection().z;

    double b = coeffs[0] * oc.x * ray.getDirection().x +
               coeffs[1] * oc.x * ray.getDirection().y +
               coeffs[2] * oc.x * ray.getDirection().z +
               coeffs[3] * oc.y * ray.getDirection().x +
               coeffs[4] * oc.y * ray.getDirection().z +
               coeffs[5] * oc.z * ray.getDirection().x +
               coeffs[6] * oc.x +
               coeffs[7] * oc.y +
               coeffs[8] * oc.z +
               coeffs[9] * ray.getDirection().x;

    double c = coeffs[0] * oc.x * oc.x +
               coeffs[1] * oc.x * oc.y +
               coeffs[2] * oc.x * oc.z +
               coeffs[3] * oc.y * oc.y +
               coeffs[4] * oc.y * oc.z +
               coeffs[5] * oc.z * oc.z +
               coeffs[6] * oc.x +
               coeffs[7] * oc.y +
               coeffs[8] * oc.z;

    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0.0) {
        return 0;
    }

    double sqrtDisc = sqrt(discriminant);
    double t0 = (-b - sqrtDisc) / (2 * a);
    double t1 = (-b + sqrtDisc) / (2 * a);

    if (t0 > t1) {
        std::swap(t0, t1);
    }

    tValues[0] = t0;
    tValues[1] = t1;

    int count = 0;
    if (t0 >= -Vec3::EPSILON) count++;
    if (t1 >= -Vec3::EPSILON && t1 != t0) count++;

    return count;
}
