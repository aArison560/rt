/**
 * @file AreaLight.cpp
 * @author RT Team
 * @date 2026-07-05
 * @brief AreaLight implementation
 */

#include "lighting/AreaLight.hpp"

namespace {
thread_local std::mt19937 tlAreaRng(std::random_device{}());
thread_local std::uniform_real_distribution<double> tlAreaDist(-1.0, 1.0);
}

AreaLight::AreaLight()
    : ALight(Vec3(1, 1, 1), 1.0),
      position(0, 0, 0),
      uAxis(1, 0, 0),
      vAxis(0, 1, 0),
      uSize(0.5), vSize(0.5),
      samples(8)
{
    recalculateNormal();
}

AreaLight::AreaLight(const Vec3& position, const Vec3& uAxis, const Vec3& vAxis,
                     double uSize, double vSize, const Vec3& color, double intensity)
    : ALight(color, intensity),
      position(position),
      uAxis(uAxis),
      vAxis(vAxis),
      uSize(uSize), vSize(vSize),
      samples(8)
{
    this->uAxis.normalize();
    this->vAxis.normalize();
    recalculateNormal();
}

AreaLight::AreaLight(const AreaLight& other)
    : ALight(other),
      position(other.position),
      uAxis(other.uAxis),
      vAxis(other.vAxis),
      normal(other.normal),
      uSize(other.uSize), vSize(other.vSize),
      samples(other.samples) {}

AreaLight& AreaLight::operator=(const AreaLight& other)
{
    if (this == &other) {
        return *this;
    }
    ALight::operator=(other);
    position = other.position;
    uAxis = other.uAxis;
    vAxis = other.vAxis;
    normal = other.normal;
    uSize = other.uSize;
    vSize = other.vSize;
    samples = other.samples;
    return *this;
}

LightType AreaLight::getLightType() const
{
    return LightType::Area;
}

Vec3 AreaLight::samplePoint() const
{
    // Uniform random point on the rectangle surface
    double u = tlAreaDist(tlAreaRng) * uSize;
    double v = tlAreaDist(tlAreaRng) * vSize;
    return position + uAxis * u + vAxis * v;
}

void AreaLight::setPosition(const Vec3& position)
{
    this->position = position;
}

const Vec3& AreaLight::getPosition() const
{
    return position;
}

void AreaLight::setOrientation(const Vec3& uAxis, const Vec3& vAxis)
{
    this->uAxis = uAxis;
    this->uAxis.normalize();
    this->vAxis = vAxis;
    this->vAxis.normalize();
    recalculateNormal();
}

const Vec3& AreaLight::getUAxis() const { return uAxis; }
const Vec3& AreaLight::getVAxis() const { return vAxis; }

void AreaLight::setSize(double uSize, double vSize)
{
    this->uSize = uSize;
    this->vSize = vSize;
}

double AreaLight::getUSize() const { return uSize; }
double AreaLight::getVSize() const { return vSize; }

void AreaLight::setSamples(int samples) { this->samples = samples; }
int AreaLight::getSamples() const { return samples; }

void AreaLight::recalculateNormal()
{
    normal = uAxis.cross(vAxis).normalized();
}
