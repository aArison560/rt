/**
 * @file ALight.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Abstract base class for all light sources
 *
 * Defines the interface that all light types (Point, Directional, Ambient)
 * must implement for the Phong lighting model.
 *
 * @relationships
 * - Base class for: PointLight, DirectionalLight, AmbientLight
 * - Composed of: Vec3 (color), double (intensity)
 * - Used by: Renderer (lighting calculations)
 * - Contained by: Scene
 */

#pragma once

#include "core/Vec3.hpp"

enum class LightType
{
    Ambient,
    Point,
    Directional,
    Area
};

class ALight
{
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ALight() = default;

    /**
     * @brief Set light color
     * @param color The light color (RGB)
     */
    virtual void setColor(const Vec3& color);

    /**
     * @brief Get light color
     * @return The light color as Vec3
     */
    [[nodiscard]] virtual const Vec3& getColor() const;

    /**
     * @brief Set light intensity
     * @param intensity The intensity multiplier (typically 0.0-1.0)
     */
    virtual void setIntensity(double intensity);

    /**
     * @brief Get light intensity
     * @return The intensity value
     */
    [[nodiscard]] virtual double getIntensity() const;

    /**
     * @brief Get light type
     * @return LightType enum value
     */
    [[nodiscard]] virtual LightType getLightType() const = 0;

protected:
    /**
     * @brief Protected constructor
     * @param color Light color
     * @param intensity Light intensity
     */
    ALight(const Vec3& color, double intensity);

    /**
     * @brief Protected copy constructor
     * @param other The light to copy
     */
    ALight(const ALight& other);

    /**
     * @brief Protected assignment operator
     * @param other The light to assign
     * @return Reference to this light
     */
    ALight& operator=(const ALight& other);

    Vec3 color;      ///< Light color (RGB)
    double intensity; ///< Light intensity multiplier
};
