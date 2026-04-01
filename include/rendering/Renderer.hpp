/**
 * @file Renderer.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Ray tracing renderer
 *
 * Core rendering engine that performs ray tracing to generate an image
 * of a scene. Implements ray casting, intersection testing, lighting
 * calculations (Phong model), shadows, reflections, and transparency.
 *
 * @relationships
 * - Uses: Scene, Ray, HitRecord, Material, ALight
 * - Produces: Color pixel data
 * - Used by: main, Window (display system)
 * - Implements: Phong lighting, shadow rays, recursion for reflections
 */

#pragma once

#include "scene/Scene.hpp"
#include "core/Vec3.hpp"
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "lighting/ALight.hpp"
#include "core/Material.hpp"
#include <vector>

class Renderer
{
public:
    /**
     * @brief Default constructor
     */
    Renderer();

    /**
     * @brief Destructor
     */
    ~Renderer();

    /**
     * @brief Render a scene to a pixel buffer
     * @param scene The scene to render
     * @param width Output image width in pixels
     * @param height Output image height in pixels
     * @param pixelBuffer Output: array of pixels (width*height*4, RGBA format)
     * @return true if render succeeded
     */
    bool render(const Scene& scene, int width, int height, unsigned char* pixelBuffer);

    /**
     * @brief Render a region of the scene (for progressive rendering)
     * @param scene The scene
     * @param width Full image width
     * @param height Full image height
     * @param startX Starting X coordinate
     * @param startY Starting Y coordinate
     * @param regionWidth Width of region to render
     * @param regionHeight Height of region to render
     * @param pixelBuffer Output: pixel buffer (must be width*height*4)
     * @return true if render succeeded
     */
    bool renderRegion(const Scene& scene, int width, int height,
                     int startX, int startY, int regionWidth, int regionHeight,
                     unsigned char* pixelBuffer);

    /**
     * @brief Set maximum recursion depth for reflections/refractions
     * @param depth Maximum depth (typically 4-8)
     */
    void setMaxRecursionDepth(int depth);

    /**
     * @brief Get maximum recursion depth
     * @return Maximum depth
     */
    int getMaxRecursionDepth() const;

    /**
     * @brief Set number of shadow ray samples per light
     * @param samples Number of samples (1 = hard shadows, >1 = soft shadows)
     */
    void setShadowSamples(int samples);

    /**
     * @brief Get number of shadow ray samples
     * @return Sample count
     */
    int getShadowSamples() const;

    /**
     * @brief Enable/disable shadow tracing
     * @param enabled Whether shadows should be computed
     */
    void setShadowsEnabled(bool enabled);

    /**
     * @brief Check if shadows are enabled
     * @return true if shadows are enabled
     */
    bool getShadowsEnabled() const;

    /**
     * @brief Enable/disable reflection tracing
     * @param enabled Whether reflections should be computed
     */
    void setReflectionsEnabled(bool enabled);

    /**
     * @brief Check if reflections are enabled
     * @return true if reflections are enabled
     */
    bool getReflectionsEnabled() const;

    /**
     * @brief Enable/disable refraction tracing
     * @param enabled Whether refractions should be computed
     */
    void setRefractionsEnabled(bool enabled);

    /**
     * @brief Check if refractions are enabled
     * @return true if refractions are enabled
     */
    bool getRefractionsEnabled() const;

    /**
     * @brief Cancel ongoing render (for multithreading)
     */
    void cancel();

    /**
     * @brief Check if render was cancelled
     * @return true if render was cancelled
     */
    bool isCancelled() const;

private:
    int maxRecursionDepth;  ///< Maximum recursion depth
    int shadowSamples;      ///< Number of shadow ray samples per light
    bool shadowsEnabled;    ///< Whether to compute shadows
    bool reflectionsEnabled; ///< Whether to compute reflections
    bool refractionsEnabled; ///< Whether to compute refractions
    bool cancelled;         ///< Whether render was cancelled

    /**
     * @brief Trace a ray through the scene
     * @param ray The ray to trace
     * @param scene The scene
     * @param depth Current recursion depth
     * @param tMin Minimum ray parameter
     * @param tMax Maximum ray parameter
     * @return Color at ray endpoint as Vec3 (RGB in range 0-1)
     */
    Vec3 trace(const Ray& ray, const Scene& scene, int depth,
              double tMin, double tMax) const;

    /**
     * @brief Find closest intersection of ray with scene
     * @param ray The ray to test
     * @param scene The scene
     * @param tMin Minimum ray parameter
     * @param tMax Maximum ray parameter
     * @param hitRecord Output: intersection information
     * @return true if intersection found
     */
    bool castRay(const Ray& ray, const Scene& scene, double tMin, double tMax,
                HitRecord& hitRecord) const;

    /**
     * @brief Calculate shadow factor from a point to a light
     * @param hitPoint Point to test
     * @param light The light source
     * @param scene The scene
     * @return Shadow factor (0.0 = fully shadowed, 1.0 = fully lit)
     */
    double calculateShadow(const Vec3& hitPoint, const ALight& light,
                          const Scene& scene) const;

    /**
     * @brief Calculate Phong lighting at a point
     * @param hitRecord The intersection information
     * @param rayDir The incoming ray direction
     * @param scene The scene
     * @param depth Recursion depth
     * @return Color from lighting as Vec3
     */
    Vec3 calculateLighting(const HitRecord& hitRecord, const Vec3& rayDir,
                          const Scene& scene, int depth) const;

    /**
     * @brief Calculate ambient light contribution
     * @param material The material
     * @param scene The scene
     * @return Ambient color
     */
    Vec3 calculateAmbient(const Material* material,
                          const Scene& scene) const;

    /**
     * @brief Calculate diffuse light contribution
     * @param hitRecord The intersection
     * @param light The light source
     * @param lightDir Direction towards light
     * @param shadow Shadow factor (0-1)
     * @return Diffuse color
     */
    Vec3 calculateDiffuse(const HitRecord& hitRecord, const ALight& light,
                         const Vec3& lightDir, double shadow) const;

    /**
     * @brief Calculate specular light contribution
     * @param hitRecord The intersection
     * @param light The light source
     * @param lightDir Direction towards light
     * @param viewDir Direction towards viewer
     * @param shadow Shadow factor
     * @return Specular color
     */
    Vec3 calculateSpecular(const HitRecord& hitRecord, const ALight& light,
                          const Vec3& lightDir, const Vec3& viewDir, double shadow) const;

    /**
     * @brief Convert color to 8-bit RGBA format
     * @param color Color as Vec3 (0-1 range)
     * @param r Output: red channel (0-255)
     * @param g Output: green channel (0-255)
     * @param b Output: blue channel (0-255)
     * @param a Output: alpha channel (0-255)
     */
    void colorToRGBA(const Vec3& color, unsigned char& r, unsigned char& g,
                    unsigned char& b, unsigned char& a) const;
};
