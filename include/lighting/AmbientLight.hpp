/**
 * @file AmbientLight.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Ambient light source
 *
 * Represents global ambient lighting that illuminates all surfaces
 * uniformly from all directions.
 *
 * @relationships
 * - Inherits from: ALight
 * - Composed of: no position or direction (global)
 * - Used by: Renderer (contributes ambient component of Phong model)
 * - Contained by: Scene (typically only one ambient light)
 */

#pragma once

#include "ALight.hpp"

class AmbientLight : public ALight
{
public:
    /**
     * @brief Default constructor - creates neutral ambient light
     */
    AmbientLight();

    /**
     * @brief Constructor with color and intensity
     * @param color Light color (RGB)
     * @param intensity Light intensity
     */
    AmbientLight(const Vec3& color, double intensity);

    /**
     * @brief Copy constructor
     * @param other The AmbientLight to copy
     */
    AmbientLight(const AmbientLight& other);

    /**
     * @brief Assignment operator
     * @param other The AmbientLight to assign
     * @return Reference to this AmbientLight
     */
    AmbientLight& operator=(const AmbientLight& other);

    /**
     * @brief Virtual destructor
     */
    ~AmbientLight() override = default;

    /**
     * @brief Get light type identifier
     * @return "AmbientLight"
     */
    const char* getType() const override;
};
