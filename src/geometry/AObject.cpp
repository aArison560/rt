/**
 * @file AObject.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Abstract object base class implementation
 */

#include "geometry/AObject.hpp"
#include <algorithm>

AObject::AObject() : transform(), material(nullptr), castsShadows(true) {}

AObject::AObject(const AObject& other) 
    : transform(other.transform), material(other.material), castsShadows(other.castsShadows) {}

AObject& AObject::operator=(const AObject& other)
{
    if (this == &other) {
        return *this;
    }
    transform = other.transform;
    material = other.material;
    castsShadows = other.castsShadows;
    return *this;
}

void AObject::setTransform(const Transform& transform)
{
    this->transform = transform;
}

const Transform& AObject::getTransform() const
{
    return transform;
}

void AObject::setMaterial(std::shared_ptr<Material> material)
{
    this->material = material;
}

std::shared_ptr<Material> AObject::getMaterial() const
{
    return material;
}

void AObject::setCastsShadows(bool castsShadows)
{
    this->castsShadows = castsShadows;
}

bool AObject::getCastsShadows() const
{
    return castsShadows;
}

void AObject::getWorldBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    Vec3 objMin, objMax;
    getBoundingBox(objMin, objMax);

    // Transform the 8 corners of the object-space bounding box to world space
    Vec3 corners[8] = {
        transform.transformPoint(Vec3(objMin.x, objMin.y, objMin.z)),
        transform.transformPoint(Vec3(objMin.x, objMin.y, objMax.z)),
        transform.transformPoint(Vec3(objMin.x, objMax.y, objMin.z)),
        transform.transformPoint(Vec3(objMin.x, objMax.y, objMax.z)),
        transform.transformPoint(Vec3(objMax.x, objMin.y, objMin.z)),
        transform.transformPoint(Vec3(objMax.x, objMin.y, objMax.z)),
        transform.transformPoint(Vec3(objMax.x, objMax.y, objMin.z)),
        transform.transformPoint(Vec3(objMax.x, objMax.y, objMax.z))
    };

    // Compute the axis-aligned bounding box that encloses all transformed corners
    minCorner = corners[0];
    maxCorner = corners[0];
    for (int i = 1; i < 8; ++i) {
        minCorner.x = std::min(minCorner.x, corners[i].x);
        minCorner.y = std::min(minCorner.y, corners[i].y);
        minCorner.z = std::min(minCorner.z, corners[i].z);
        maxCorner.x = std::max(maxCorner.x, corners[i].x);
        maxCorner.y = std::max(maxCorner.y, corners[i].y);
        maxCorner.z = std::max(maxCorner.z, corners[i].z);
    }
}

Vec3 AObject::getNormalAt(const Vec3& point) const
{
    (void)point;
    return Vec3(0, 1, 0);
}

void AObject::getUVAt(const Vec3& point, double& u, double& v) const
{
    (void)point;
    u = 0.5;
    v = 0.5;
}
