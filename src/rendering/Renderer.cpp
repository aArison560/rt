/**
 * @file Renderer.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Renderer implementation
 */

#include "rendering/Renderer.hpp"
#include "core/HitRecord.hpp"
#include <algorithm>

Renderer::Renderer() : maxRecursionDepth(4), shadowSamples(1), shadowsEnabled(true),
                       reflectionsEnabled(true), refractionsEnabled(false), cancelled(false) {}

Renderer::~Renderer() {}

bool Renderer::render(const Scene& scene, int width, int height, unsigned char* pixelBuffer)
{
    cancelled = false;
    return renderRegion(scene, width, height, 0, 0, width, height, pixelBuffer);
}

bool Renderer::renderRegion(const Scene& scene, int width, int height,
                           int startX, int startY, int regionWidth, int regionHeight,
                           unsigned char* pixelBuffer)
{
    if (!pixelBuffer || width <= 0 || height <= 0 || regionWidth <= 0 || regionHeight <= 0)
        return false;

    const int x0 = std::max(0, startX);
    const int y0 = std::max(0, startY);
    const int x1 = std::min(width, startX + regionWidth);
    const int y1 = std::min(height, startY + regionHeight);

    if (x0 >= x1 || y0 >= y1)
        return false;

    const Camera& camera = scene.getCamera();

    for (int y = y0; y < y1; ++y) {
        if (cancelled)
            return false;

        for (int x = x0; x < x1; ++x) {
            const Ray ray = camera.generateRay(static_cast<double>(x), static_cast<double>(y),
                                               width, height);
            const Vec3 color = trace(ray, scene, 0, 1e-4, 1e30);

            unsigned char r, g, b, a;
            colorToRGBA(color, r, g, b, a);

            const size_t index = (static_cast<size_t>(y) * width + x) * 4;
            pixelBuffer[index + 0] = r;
            pixelBuffer[index + 1] = g;
            pixelBuffer[index + 2] = b;
            pixelBuffer[index + 3] = a;
        }
    }

    return true;
}

void Renderer::setMaxRecursionDepth(int depth) { maxRecursionDepth = depth; }
int Renderer::getMaxRecursionDepth() const { return maxRecursionDepth; }
void Renderer::setShadowSamples(int samples) { shadowSamples = samples; }
int Renderer::getShadowSamples() const { return shadowSamples; }
void Renderer::setShadowsEnabled(bool enabled) { shadowsEnabled = enabled; }
bool Renderer::getShadowsEnabled() const { return shadowsEnabled; }
void Renderer::setReflectionsEnabled(bool enabled) { reflectionsEnabled = enabled; }
bool Renderer::getReflectionsEnabled() const { return reflectionsEnabled; }
void Renderer::setRefractionsEnabled(bool enabled) { refractionsEnabled = enabled; }
bool Renderer::getRefractionsEnabled() const { return refractionsEnabled; }
void Renderer::cancel() { cancelled = true; }
bool Renderer::isCancelled() const { return cancelled; }

Vec3 Renderer::trace(const Ray& ray, const Scene& scene, int depth,
                    double tMin, double tMax) const
{
    // TODO: Main ray tracing function
    // 1. Find closest intersection
    // 2. Calculate lighting at intersection
    // 3. Handle reflections and refractions recursively
    HitRecord record;
    if (!castRay(ray, scene, tMin, tMax, record)) {
        return scene.getBackgroundColor();
    }
    return calculateLighting(record, -ray.getDirection(), scene, depth);
}

bool Renderer::castRay(const Ray& ray, const Scene& scene, double tMin, double tMax,
                      HitRecord& hitRecord) const
{
    // TODO: Find closest intersection with any object
    return false;
}

double Renderer::calculateShadow(const Vec3& hitPoint, const ALight& light,
                                const Scene& scene) const
{
    // TODO: Calculate shadow factor using shadow rays
    return 1.0; // No shadow
}

Vec3 Renderer::calculateLighting(const HitRecord& hitRecord, const Vec3& rayDir,
                                const Scene& scene, int depth) const
{
    // TODO: Implement Phong lighting model
    Vec3 color = calculateAmbient(hitRecord.getMaterial(), scene);
    // Add diffuse and specular from each light
    // Handle reflections and refractions if enabled and depth allows
    return color;
}

Vec3 Renderer::calculateAmbient(const Material* material,
                                const Scene& scene) const
{
    // TODO: Calculate ambient component
    return Vec3();
}

Vec3 Renderer::calculateDiffuse(const HitRecord& hitRecord, const ALight& light,
                               const Vec3& lightDir, double shadow) const
{
    // TODO: Calculate diffuse (Lambert) component
    return Vec3();
}

Vec3 Renderer::calculateSpecular(const HitRecord& hitRecord, const ALight& light,
                                const Vec3& lightDir, const Vec3& viewDir, double shadow) const
{
    // TODO: Calculate specular (Blinn-Phong) component
    return Vec3();
}

void Renderer::colorToRGBA(const Vec3& color, unsigned char& r, unsigned char& g,
                          unsigned char& b, unsigned char& a) const
{
    // TODO: Convert Vec3 [0,1] to RGBA [0,255]
    r = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255.0);
    g = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255.0);
    b = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255.0);
    a = 255;
}
