/**
 * @file AObject.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Abstract object base class implementation
 */

#include "geometry/AObject.hpp"

AObject::AObject() : transform(), material(nullptr), castsShadows(true) {}

AObject::AObject(const AObject& other) 
    : transform(other.transform), material(other.material), castsShadows(other.castsShadows) {}

AObject& AObject::operator=(const AObject& other)
{
    // TODO: Assignment
    return *this;
}

void AObject::setTransform(const Transform& transform)
{
    // TODO: Set transform
}

const Transform& AObject::getTransform() const
{
    return transform;
}

void AObject::setMaterial(std::shared_ptr<Material> material)
{
    // TODO: Set material
}

std::shared_ptr<Material> AObject::getMaterial() const
{
    return material;
}

void AObject::setCastsShadows(bool castsShadows)
{
    // TODO: Set shadow casting flag
}

bool AObject::getCastsShadows() const
{
    return castsShadows;
}

void AObject::getWorldBoundingBox(Vec3& minCorner, Vec3& maxCorner) const
{
    // TODO: Transform bounding box to world space
    getBoundingBox(minCorner, maxCorner);
}

Vec3 AObject::getNormalAt(const Vec3& point) const
{
    // TODO: Default implementation (should be overridden)
    return Vec3(0, 1, 0);
}

void AObject::getUVAt(const Vec3& point, double& u, double& v) const
{
    // TODO: Default implementation (should be overridden)
    u = 0.5;
    v = 0.5;
}
