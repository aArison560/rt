/**
 * @file DirectionalLight.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief DirectionalLight implementation
 */

#include "DirectionalLight.hpp"

DirectionalLight::DirectionalLight() : ALight(Vec3(1, 1, 1), 1.0), direction(0, 0, -1) {}

DirectionalLight::DirectionalLight(const Vec3& direction, const Vec3& color, double intensity)
    : ALight(color, intensity), direction(direction)
{
    // TODO: Normalize direction
}

DirectionalLight::DirectionalLight(const DirectionalLight& other) 
    : ALight(other), direction(other.direction) {}

DirectionalLight& DirectionalLight::operator=(const DirectionalLight& other)
{
    // TODO: Assignment
    return *this;
}

const char* DirectionalLight::getType() const
{
    return "DirectionalLight";
}

void DirectionalLight::setDirection(const Vec3& direction) 
{ 
    this->direction = direction;
    // TODO: Normalize
}

const Vec3& DirectionalLight::getDirection() const { return direction; }

Vec3 DirectionalLight::getDirectionToLight() const
{
    // TODO: Return negated direction (towards light source)
    return -direction;
}
