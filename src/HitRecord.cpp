/**
 * @file HitRecord.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief HitRecord implementation
 */

#include "HitRecord.hpp"

HitRecord::HitRecord() : t(0), point(), normal(), u(0), v(0), 
                         material(nullptr), object(nullptr), valid(false), frontFace(true) {}

HitRecord::HitRecord(double t, const Vec3& point, const Vec3& normal,
                     Material* material, AObject* object)
    : t(t), point(point), normal(normal), u(0), v(0), material(material), 
      object(object), valid(true), frontFace(true) {}

HitRecord::HitRecord(const HitRecord& other)
    : t(other.t), point(other.point), normal(other.normal), u(other.u), v(other.v),
      material(other.material), object(other.object), valid(other.valid), 
      frontFace(other.frontFace) {}

HitRecord& HitRecord::operator=(const HitRecord& other)
{
    // TODO: Assignment
    return *this;
}

bool HitRecord::isValid() const { return valid; }
double HitRecord::getT() const { return t; }
void HitRecord::setT(double t) { this->t = t; }
const Vec3& HitRecord::getPoint() const { return point; }
void HitRecord::setPoint(const Vec3& point) { this->point = point; }
const Vec3& HitRecord::getNormal() const { return normal; }
void HitRecord::setNormal(const Vec3& normal) { this->normal = normal; }
Material* HitRecord::getMaterial() const { return material; }
void HitRecord::setMaterial(Material* material) { this->material = material; }
AObject* HitRecord::getObject() const { return object; }
void HitRecord::setObject(AObject* object) { this->object = object; }

void HitRecord::getUV(double& u, double& v) const
{
    u = this->u;
    v = this->v;
}

void HitRecord::setUV(double u, double v)
{
    this->u = u;
    this->v = v;
}

bool HitRecord::isFrontFace() const { return frontFace; }
void HitRecord::setFrontFace(bool frontFace) { this->frontFace = frontFace; }

Vec3 HitRecord::getFacingNormal() const
{
    // TODO: Return normal pointing toward ray origin
    return frontFace ? normal : -normal;
}

void HitRecord::invalidate()
{
    valid = false;
}
