/**
 * @file AreaLight.hpp
 * @author RT Team
 * @date 2026-07-05
 * @brief Rectangular area light source
 *
 * Represents a light that emits from a planar rectangular surface.
 * Provides random point sampling on the surface for soft shadow
 * and soft diffuse lighting computation.
 *
 * @relationships
 * - Inherits from: ALight
 * - Composed of: position (center), uAxis, vAxis (orientation), sizes
 * - Used by: Renderer (soft shadow rays, area lighting)
 * - Contained by: Scene
 */

#pragma once

#include "ALight.hpp"
#include <random>

class AreaLight : public ALight
{
public:
    /**
     * @brief Default constructor - creates 1x1 area light at origin
     */
    AreaLight();

    /**
     * @brief Constructor with full parameters
     * @param position Center of the rectangular light in world space
     * @param uAxis Vector direction of the light's local X axis (will be normalized)
     * @param vAxis Vector direction of the light's local Y axis (will be normalized)
     * @param uSize Half-size along uAxis
     * @param vSize Half-size along vAxis
     * @param color Light color (RGB)
     * @param intensity Light intensity
     */
    AreaLight(const Vec3& position, const Vec3& uAxis, const Vec3& vAxis,
              double uSize, double vSize, const Vec3& color, double intensity);

    /**
     * @brief Copy constructor
     * @param other The AreaLight to copy
     */
    AreaLight(const AreaLight& other);

    /**
     * @brief Assignment operator
     * @param other The AreaLight to assign
     * @return Reference to this AreaLight
     */
    AreaLight& operator=(const AreaLight& other);

    /**
     * @brief Virtual destructor
     */
    ~AreaLight() override = default;

    /**
     * @brief Get light type
     * @return LightType::Area
     */
    [[nodiscard]] LightType getLightType() const override;

    /**
     * @brief Get a random sample point on the area light surface
     * @return Position on the light surface in world space
     *
     * Returns a uniformly distributed random point on the rectangular
     * surface of the area light.
     */
    [[nodiscard]] Vec3 samplePoint() const;

    /**
     * @brief Set area light center position
     * @param position The new center
     */
    void setPosition(const Vec3& position);

    /**
     * @brief Get area light center position
     * @return Center as Vec3
     */
    [[nodiscard]] const Vec3& getPosition() const;

    /**
     * @brief Set area light orientation and size
     * @param uAxis Local X axis direction (will be normalized)
     * @param vAxis Local Y axis direction (will be normalized)
     */
    void setOrientation(const Vec3& uAxis, const Vec3& vAxis);

    /**
     * @brief Get local U axis
     * @return U axis (normalized)
     */
    [[nodiscard]] const Vec3& getUAxis() const;

    /**
     * @brief Get local V axis
     * @return V axis (normalized)
     */
    [[nodiscard]] const Vec3& getVAxis() const;

    /**
     * @brief Set area light dimensions
     * @param uSize Half-size along U axis
     * @param vSize Half-size along V axis
     */
    void setSize(double uSize, double vSize);

    /**
     * @brief Get U half-size
     * @return Half-size along U axis
     */
    [[nodiscard]] double getUSize() const;

    /**
     * @brief Get V half-size
     * @return Half-size along V axis
     */
    [[nodiscard]] double getVSize() const;

    /**
     * @brief Set sample count for area light integration
     * @param samples Number of sample points per shading calculation
     */
    void setSamples(int samples);

    /**
     * @brief Get sample count
     * @return Number of sample points
     */
    [[nodiscard]] int getSamples() const;

private:
    Vec3 position;       ///< Center of the rectangular light
    Vec3 uAxis;          ///< Local X axis direction (normalized)
    Vec3 vAxis;          ///< Local Y axis direction (normalized)
    Vec3 normal;         ///< Surface normal (uAxis × vAxis, normalized)
    double uSize;        ///< Half-size along U axis
    double vSize;        ///< Half-size along V axis
    int samples;         ///< Sample count for area light integration

    /**
     * @brief Recompute cached normal from uAxis and vAxis
     */
    void recalculateNormal();
};
