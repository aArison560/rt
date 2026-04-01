/**
 * @file ALight.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Abstract light base class implementation
 */

#include "lighting/ALight.hpp"

ALight::ALight(const Vec3& color, double intensity) : color(color), intensity(intensity) {}

ALight::ALight(const ALight& other) : color(other.color), intensity(other.intensity) {}

ALight& ALight::operator=(const ALight& other)
{
    // TODO: Assignment
    return *this;
}

void ALight::setColor(const Vec3& color) { this->color = color; }
const Vec3& ALight::getColor() const { return color; }
void ALight::setIntensity(double intensity) { this->intensity = intensity; }
double ALight::getIntensity() const { return intensity; }
