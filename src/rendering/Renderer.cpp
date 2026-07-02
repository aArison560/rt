/**
 * @file Renderer.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Renderer implementation
 */

#include "rendering/Renderer.hpp"
#include "core/HitRecord.hpp"
#include "lighting/PointLight.hpp"
#include <algorithm>
#include <cmath>

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
    HitRecord record;
    if (!castRay(ray, scene, tMin, tMax, record)) {
        return scene.getBackgroundColor();
    }
    return calculateLighting(record, -ray.getDirection(), scene, depth);
}

bool Renderer::castRay(const Ray& ray, const Scene& scene, double tMin, double tMax,
                      HitRecord& hitRecord) const
{
    double closest = tMax;
    bool found = false;

    const auto& objects = scene.getObjects();
    for (const auto& obj : objects) {
        HitRecord hr;
        if (obj->hit(ray, tMin, closest, hr)) {
            // hr.t may be set by object
            if (hr.isValid() || hr.getT() > 0.0) {
                closest = hr.getT();
                hitRecord = hr;
                found = true;
            }
        }
    }
    return found;
}

double Renderer::calculateShadow(const Vec3& hitPoint, const ALight& light,
                                const Scene& scene) const
{
    // Minimal implementation: no soft shadows, simple hard shadow check
    if (!shadowsEnabled) return 1.0;
    // If light is point light, cast shadow ray toward it and check occlusion
    const char* type = light.getType();
    if (std::string(type) == "PointLight") {
        const PointLight* pl = dynamic_cast<const PointLight*>(&light);
        if (!pl) return 1.0;
        Vec3 toLight = pl->getPosition() - hitPoint;
        double dist = toLight.magnitude();
        Vec3 dir = toLight / dist;
        Ray shadowRay(hitPoint + dir * 1e-4, dir);
        HitRecord hr;
        if (castRay(shadowRay, scene, 1e-4, dist - 1e-4, hr)) {
            return 0.0; // in shadow
        }
    }
    return 1.0;
}

Vec3 Renderer::calculateLighting(const HitRecord& hitRecord, const Vec3& rayDir,
                                const Scene& scene, int depth) const
{
    (void)depth;
    const Material* material = hitRecord.getMaterial();

    // Ambient
    Vec3 color = calculateAmbient(material, scene);

    // For each light
    for (const auto& lightPtr : scene.getLights()) {
        const ALight& light = *lightPtr;
        const std::string type = light.getType();

        if (type == std::string("AmbientLight")) {
            // ambient handled globally
            continue;
        }

        if (type == std::string("PointLight")) {
            const PointLight* pl = dynamic_cast<const PointLight*>(&light);
            if (!pl) continue;
            Vec3 lightPos = pl->getPosition();
            Vec3 hitPoint = hitRecord.getPoint();

            Vec3 lightDir = (lightPos - hitPoint).normalized();
            double distance = (lightPos - hitPoint).magnitude();
            double attenuation = pl->getAttenuationFactor(distance);

            double shadow = calculateShadow(hitPoint, light, scene);

            // Diffuse
            Vec3 diff = calculateDiffuse(hitRecord, light, lightDir, shadow);

            // Specular
            Vec3 spec = calculateSpecular(hitRecord, light, lightDir, rayDir, shadow);

            color += (diff + spec) * attenuation;
        }
    }

    // Clamp to [0,1]
    color.x = std::clamp(color.x, 0.0, 1.0);
    color.y = std::clamp(color.y, 0.0, 1.0);
    color.z = std::clamp(color.z, 0.0, 1.0);

    return color;
}

Vec3 Renderer::calculateAmbient(const Material* material,
                                const Scene& scene) const
{
    Vec3 ambientColor(0.0, 0.0, 0.0);
    if (material) {
        ambientColor = material->getColor() * material->getAmbient() * scene.getAmbientMultiplier();
    }
    // Also include ambient lights present in scene
    for (const auto& lp : scene.getLights()) {
        if (std::string(lp->getType()) == std::string("AmbientLight")) {
            ambientColor = ambientColor + lp->getColor() * lp->getIntensity();
        }
    }
    return ambientColor;
}

Vec3 Renderer::calculateDiffuse(const HitRecord& hitRecord, const ALight& light,
                               const Vec3& lightDir, double shadow) const
{
    const Material* material = hitRecord.getMaterial();
    if (!material) return Vec3(0,0,0);
    Vec3 normal = hitRecord.getFacingNormal();
    double ndotl = std::max(0.0, normal.dot(lightDir));
    Vec3 diffuse = material->getColor() * material->getDiffuse() * ndotl * light.getIntensity() * shadow;
    // Modulate by light color
    diffuse = diffuse.componentMult(light.getColor());
    return diffuse;
}

Vec3 Renderer::calculateSpecular(const HitRecord& hitRecord, const ALight& light,
                                const Vec3& lightDir, const Vec3& viewDir, double shadow) const
{
    const Material* material = hitRecord.getMaterial();
    if (!material) return Vec3(0,0,0);
    Vec3 normal = hitRecord.getFacingNormal();
    Vec3 half = (lightDir + viewDir).normalized();
    double nh = std::max(0.0, normal.dot(half));
    double specFactor = std::pow(nh, material->getShininess());
    Vec3 spec = light.getColor() * (material->getSpecular() * specFactor * light.getIntensity() * shadow);
    return spec;
}

void Renderer::colorToRGBA(const Vec3& color, unsigned char& r, unsigned char& g,
                          unsigned char& b, unsigned char& a) const
{
    r = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255.0);
    g = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255.0);
    b = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255.0);
    a = 255;
}
