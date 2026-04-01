/**
 * @file PointLight.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief PointLight implementation
 */

#include "PointLight.hpp"

PointLight::PointLight() : ALight(Vec3(1, 1, 1), 1.0), position(0, 0, 0),
                           constantAtt(1.0), linearAtt(0.0), quadraticAtt(0.0) {}

PointLight::PointLight(const Vec3& position, const Vec3& color, double intensity)
    : ALight(color, intensity), position(position),
      constantAtt(1.0), linearAtt(0.0), quadraticAtt(0.0) {}

PointLight::PointLight(const Vec3& position, const Vec3& color, double intensity,
                       double constant, double linear, double quadratic)
    : ALight(color, intensity), position(position),
      constantAtt(constant), linearAtt(linear), quadraticAtt(quadratic) {}

PointLight::PointLight(const PointLight& other) : ALight(other), position(other.position),
                                                   constantAtt(other.constantAtt),
                                                   linearAtt(other.linearAtt),
                                                   quadraticAtt(other.quadraticAtt) {}

PointLight& PointLight::operator=(const PointLight& other)
{
    // TODO: Assignment
    return *this;
}

const char* PointLight::getType() const
{
    return "PointLight";
}

void PointLight::setPosition(const Vec3& position) { this->position = position; }
const Vec3& PointLight::getPosition() const { return position; }

void PointLight::setAttenuation(double constant, double linear, double quadratic)
{
    constantAtt = constant;
    linearAtt = linear;
    quadraticAtt = quadratic;
}

double PointLight::getConstantAttenuation() const { return constantAtt; }
double PointLight::getLinearAttenuation() const { return linearAtt; }
double PointLight::getQuadraticAttenuation() const { return quadraticAtt; }

double PointLight::getAttenuationFactor(double distance) const
{
    // TODO: Calculate attenuation: 1 / (c + l*d + q*d²)
    return 1.0 / (constantAtt + linearAtt * distance + quadraticAtt * distance * distance);
}

Vec3 PointLight::getDirectionTo(const Vec3& point) const
{
    // TODO: Return normalized direction from light to point
    return Vec3();
}

double PointLight::getDistance(const Vec3& point) const
{
    // TODO: Calculate distance from light to point
    return 0.0;
}
