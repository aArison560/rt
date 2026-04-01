/**
 * @file AmbientLight.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief AmbientLight implementation
 */

#include "lighting/AmbientLight.hpp"

AmbientLight::AmbientLight() : ALight(Vec3(1, 1, 1), 0.2) {}

AmbientLight::AmbientLight(const Vec3& color, double intensity) : ALight(color, intensity) {}

AmbientLight::AmbientLight(const AmbientLight& other) : ALight(other) {}

AmbientLight& AmbientLight::operator=(const AmbientLight& other)
{
    // TODO: Assignment
    return *this;
}

const char* AmbientLight::getType() const
{
    return "AmbientLight";
}
