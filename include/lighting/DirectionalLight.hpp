/**
 * @file DirectionalLight.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Directional light source
 *
 * Represents parallel light rays from a specific direction (like sunlight).
 * Rays are always parallel regardless of position being illuminated.
 *
 * @relationships
 * - Inherits from: ALight
 * - Composed of: direction (Vec3)
 * - Used by: Renderer (calculates diffuse/specular lighting, shadows)
 * - Contained by: Scene
 */

#pragma once

#include "ALight.hpp"

class DirectionalLight : public ALight
{
public:
    /**
     * @brief Default constructor - creates light in negative Z direction
     */
    DirectionalLight();

    /**
     * @brief Constructor with direction and color
     * @param direction Direction light travels (will be normalized)
     * @param color Light color (RGB)
     * @param intensity Light intensity
     */
    DirectionalLight(const Vec3& direction, const Vec3& color, double intensity);

    /**
     * @brief Copy constructor
     * @param other The DirectionalLight to copy
     */
    DirectionalLight(const DirectionalLight& other);

    /**
     * @brief Assignment operator
     * @param other The DirectionalLight to assign
     * @return Reference to this DirectionalLight
     */
    DirectionalLight& operator=(const DirectionalLight& other);

    /**
     * @brief Virtual destructor
     */
    ~DirectionalLight() override = default;

    /**
     * @brief Get light type identifier
     * @return "DirectionalLight"
     */
    const char* getType() const override;

    /**
     * @brief Set light direction
     * @param direction Direction light travels (will be normalized)
     */
    void setDirection(const Vec3& direction);

    /**
     * @brief Get light direction
     * @return The direction as normalized Vec3
     */
    const Vec3& getDirection() const;

    /**
     * @brief Get light direction (negated for towards light)
     * @return Direction from surface towards light
     */
    Vec3 getDirectionToLight() const;

private:
    Vec3 direction; ///< Direction light travels (normalized)
};
