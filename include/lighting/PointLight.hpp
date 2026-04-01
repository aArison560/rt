/**
 * @file PointLight.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Point light source
 *
 * Represents a light that radiates uniformly in all directions from a point.
 * Used for spotlight effects and local light sources.
 *
 * @relationships
 * - Inherits from: ALight
 * - Composed of: position (Vec3), optional attenuation parameters
 * - Used by: Renderer (calculates diffuse/specular lighting, shadows)
 * - Contained by: Scene
 */

#pragma once

#include "ALight.hpp"

class PointLight : public ALight
{
public:
    /**
     * @brief Default constructor - creates light at origin
     */
    PointLight();

    /**
     * @brief Constructor with position and color
     * @param position Position of the light in world space
     * @param color Light color (RGB)
     * @param intensity Light intensity
     */
    PointLight(const Vec3& position, const Vec3& color, double intensity);

    /**
     * @brief Constructor with position, color, intensity, and attenuation
     * @param position Position of the light
     * @param color Light color
     * @param intensity Light intensity
     * @param constant Constant attenuation factor
     * @param linear Linear attenuation factor
     * @param quadratic Quadratic attenuation factor
     */
    PointLight(const Vec3& position, const Vec3& color, double intensity,
               double constant, double linear, double quadratic);

    /**
     * @brief Copy constructor
     * @param other The PointLight to copy
     */
    PointLight(const PointLight& other);

    /**
     * @brief Assignment operator
     * @param other The PointLight to assign
     * @return Reference to this PointLight
     */
    PointLight& operator=(const PointLight& other);

    /**
     * @brief Virtual destructor
     */
    ~PointLight() override = default;

    /**
     * @brief Get light type identifier
     * @return "PointLight"
     */
    const char* getType() const override;

    /**
     * @brief Set light position
     * @param position The new position
     */
    void setPosition(const Vec3& position);

    /**
     * @brief Get light position
     * @return The position as Vec3
     */
    const Vec3& getPosition() const;

    /**
     * @brief Set attenuation factors
     * @param constant Constant attenuation factor
     * @param linear Linear attenuation factor (per unit distance)
     * @param quadratic Quadratic attenuation factor (per distance squared)
     */
    void setAttenuation(double constant, double linear, double quadratic);

    /**
     * @brief Get constant attenuation factor
     * @return Constant attenuation
     */
    double getConstantAttenuation() const;

    /**
     * @brief Get linear attenuation factor
     * @return Linear attenuation
     */
    double getLinearAttenuation() const;

    /**
     * @brief Get quadratic attenuation factor
     * @return Quadratic attenuation
     */
    double getQuadraticAttenuation() const;

    /**
     * @brief Calculate light intensity at a distance
     * @param distance Distance from light
     * @return Attenuated intensity factor (0.0-1.0)
     */
    double getAttenuationFactor(double distance) const;

    /**
     * @brief Get direction from light to a point
     * @param point The target point
     * @return Direction vector (normalized)
     */
    Vec3 getDirectionTo(const Vec3& point) const;

    /**
     * @brief Get distance from light to a point
     * @param point The target point
     * @return Distance value
     */
    double getDistance(const Vec3& point) const;

private:
    Vec3 position;      ///< Light position in world space
    double constantAtt;  ///< Constant attenuation factor
    double linearAtt;    ///< Linear attenuation factor
    double quadraticAtt; ///< Quadratic attenuation factor
};
