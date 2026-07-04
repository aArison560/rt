/**
 * @file HitRecord.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief HitRecord implementation
 */

#include "core/HitRecord.hpp"
#include "core/Material.hpp"
#include "geometry/AObject.hpp"

HitRecord::HitRecord() : t(0), point(), normal(), u(0), v(0), 
                         material(nullptr), object(nullptr), valid(false), frontFace(true) {}

HitRecord::~HitRecord() = default;

HitRecord::HitRecord(double t, const Vec3& point, const Vec3& normal,
                     std::shared_ptr<Material> material, const AObject* object)
    : t(t), point(point), normal(normal), u(0), v(0), material(std::move(material)), 
      object(object), valid(true), frontFace(true) {}

HitRecord::HitRecord(const HitRecord& other)
    : t(other.t), point(other.point), normal(other.normal), u(other.u), v(other.v),
      material(other.material), object(other.object), valid(other.valid), 
      frontFace(other.frontFace) {}

HitRecord& HitRecord::operator=(const HitRecord& other)
{
    if (this == &other) {
        return *this;
    }
    t = other.t;
    point = other.point;
    normal = other.normal;
    u = other.u;
    v = other.v;
    material = other.material;
    object = other.object;
    valid = other.valid;
    frontFace = other.frontFace;
    return *this;
}

bool HitRecord::isValid() const { return valid; }
double HitRecord::getT() const { return t; }
void HitRecord::setT(double t) { this->t = t; }
const Vec3& HitRecord::getPoint() const { return point; }
void HitRecord::setPoint(const Vec3& point) { this->point = point; }
const Vec3& HitRecord::getNormal() const { return normal; }
void HitRecord::setNormal(const Vec3& normal) { this->normal = normal; }
std::shared_ptr<Material> HitRecord::getMaterial() const { return material; }
void HitRecord::setMaterial(std::shared_ptr<Material> material) { this->material = std::move(material); }
const AObject* HitRecord::getObject() const { return object; }
void HitRecord::setObject(const AObject* object) { this->object = object; }

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
    return frontFace ? normal : -normal;
}

void HitRecord::invalidate()
{
    valid = false;
}
