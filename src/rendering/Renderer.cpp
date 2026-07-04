/**
 * @file Renderer.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Renderer implementation
 */

#include "rendering/Renderer.hpp"
#include "core/HitRecord.hpp"
#include "lighting/PointLight.hpp"
#include "lighting/DirectionalLight.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <thread>

namespace {
static thread_local std::mt19937 tlRng(std::random_device{}());
static thread_local std::uniform_real_distribution<double> tlDist(0.0, 1.0);
}

Renderer::Renderer() : maxRecursionDepth(4), shadowSamples(8), shadowsEnabled(true),
                       reflectionsEnabled(true), refractionsEnabled(false),
                       samplesPerPixel(4), cancelled(false) {}

Renderer::~Renderer() {}

bool Renderer::render(const Scene& scene, int width, int height, unsigned char* pixelBuffer)
{
    cancelled = false;

    // Rebuild BVH
    auto objects = scene.getObjects();
    bvhRoot = BVHNode::build(objects, 0, objects.size());

    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    int stripHeight = height / numThreads;

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i) {
        int startY = static_cast<int>(i) * stripHeight;
        int regionH = (i == numThreads - 1) ? height - startY : stripHeight;
        threads.emplace_back([this, &scene, width, height, startY, regionH, pixelBuffer]() {
            (void)renderRegion(scene, width, height, 0, startY, width, regionH, pixelBuffer);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return !cancelled;
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
            Vec3 accumulatedColor(0, 0, 0);

            for (int s = 0; s < samplesPerPixel; ++s) {
                double offsetX = tlDist(tlRng) - 0.5;
                double offsetY = tlDist(tlRng) - 0.5;
                const Ray ray = camera.generateRay(
                    static_cast<double>(x) + offsetX,
                    static_cast<double>(y) + offsetY,
                    width, height);
                accumulatedColor += trace(ray, scene, 0, 1e-4, 1e30);
            }

            Vec3 color = accumulatedColor / static_cast<double>(samplesPerPixel);

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
void Renderer::setSamplesPerPixel(int samples) { samplesPerPixel = std::max(1, samples); }
int Renderer::getSamplesPerPixel() const { return samplesPerPixel; }
void Renderer::cancel() { cancelled = true; }
bool Renderer::isCancelled() const { return cancelled; }

static double schlickFresnel(double cosTheta, double n1, double n2)
{
    double r0 = (n1 - n2) / (n1 + n2);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cosTheta, 5.0);
}

Vec3 Renderer::trace(const Ray& ray, const Scene& scene, int depth,
                    double tMin, double tMax) const
{
    auto record = castRay(ray, scene, tMin, tMax);
    if (!record) {
        return scene.getBackgroundColor();
    }
    Vec3 viewDir = -ray.getDirection();
    Vec3 color = calculateLighting(*record, viewDir, scene, depth);
    const Material* mat = record->getMaterial().get();

if (mat) {
            Vec3 point = record->getPoint();
            Vec3 normal = record->getNormal();

            double reflect = mat->getReflectivity();
            double refract = mat->getTransparency();

            // Fresnel for dielectrics (transparent materials)
            if (refract > 0.0 && (reflectionsEnabled || refractionsEnabled) && depth < maxRecursionDepth) {
                double cosTheta = std::abs(viewDir.dot(normal));
                double n1 = 1.0;
                double n2 = record->isFrontFace() ? mat->getRefractiveIndex() : 1.0;
                double fresnel = schlickFresnel(cosTheta, n1, n2);
                reflect = fresnel;
            }

            // Reflection
            if (reflectionsEnabled && reflect > 0.0 && depth < maxRecursionDepth) {
                double rrReflect = reflect;
                if (depth >= 3 && rrReflect < 0.25) {
                    if (tlDist(tlRng) >= rrReflect) {
                        rrReflect = 0.0;
                    } else {
                        rrReflect = 1.0;
                    }
                }
                if (rrReflect > 0.0) {
                    Vec3 reflectedDir = ray.getDirection().reflect(normal);
                    Vec3 reflectedOrigin = point + normal * Vec3::EPSILON;
                    Ray reflectedRay(reflectedOrigin, reflectedDir);
                    Vec3 reflectedColor = trace(reflectedRay, scene, depth + 1, 1e-4, 1e30);
                    color += reflectedColor * rrReflect;
                }
            }

            // Refraction (only for transparent materials, weighted by (1 - Fresnel))
            if (refractionsEnabled && refract > 0.0 && depth < maxRecursionDepth) {
                double rrTrans = refract * (1.0 - reflect);
                if (rrTrans <= 0.0) {
                    // skip
                } else if (depth >= 3 && rrTrans < 0.25) {
                    if (tlDist(tlRng) >= rrTrans) {
                        rrTrans = 0.0;
                    } else {
                        rrTrans = 1.0;
                    }
                }
                if (rrTrans > 0.0) {
                    Vec3 refractedDir;
                    double ratio;
                    Vec3 refractionNormal;
                    if (record->isFrontFace()) {
                        ratio = 1.0 / mat->getRefractiveIndex();
                        refractionNormal = normal;
                    } else {
                        ratio = mat->getRefractiveIndex();
                        refractionNormal = -normal;
                    }
                    if (ray.getDirection().refract(refractionNormal, ratio, refractedDir)) {
                        Vec3 refractedOrigin = point - normal * Vec3::EPSILON;
                        Ray refractedRay(refractedOrigin, refractedDir);
                        Vec3 refractedColor = trace(refractedRay, scene, depth + 1, 1e-4, 1e30);
                        color += refractedColor * rrTrans;
                    }
                }
            }
        }

    color.x = std::clamp(color.x, 0.0, 1.0);
    color.y = std::clamp(color.y, 0.0, 1.0);
    color.z = std::clamp(color.z, 0.0, 1.0);
    return color;
}

std::optional<HitRecord> Renderer::castRay(const Ray& ray, const Scene& scene, double tMin, double tMax) const
{
    if (bvhRoot) {
        return bvhRoot->hit(ray, tMin, tMax);
    }

    double closest = tMax;
    std::optional<HitRecord> bestHit;

    const auto& objects = scene.getObjects();
    for (const auto& obj : objects) {
        auto hr = obj->hit(ray, tMin, closest);
        if (hr && (!bestHit || hr->getT() < bestHit->getT())) {
            closest = hr->getT();
            bestHit = hr;
        }
    }
    return bestHit;
}

double Renderer::calculateShadow(const Vec3& hitPoint, const ALight& light,
                                const Scene& scene) const
{
    if (!shadowsEnabled) return 1.0;

    if (light.getLightType() == LightType::Point) {
        const PointLight& pl = static_cast<const PointLight&>(light);
        Vec3 toLight = pl.getPosition() - hitPoint;
        double dist = toLight.magnitude();
        Vec3 dir = toLight / dist;

        if (shadowSamples <= 1) {
            Ray shadowRay(hitPoint + dir * 1e-4, dir);
            if (castRay(shadowRay, scene, 1e-4, dist - 1e-4)) {
                return 0.0;
            }
            return 1.0;
        }

        Vec3 uVec, vVec;
        if (std::abs(dir.x) > 0.9) {
            uVec = Vec3(0, 1, 0).cross(dir).normalized();
        } else {
            uVec = Vec3(1, 0, 0).cross(dir).normalized();
        }
        vVec = dir.cross(uVec).normalized();

        double lightRadius = 0.5;
        int visible = 0;
        for (int s = 0; s < shadowSamples; ++s) {
            double uOff = (tlDist(tlRng) * 2.0 - 1.0) * lightRadius;
            double vOff = (tlDist(tlRng) * 2.0 - 1.0) * lightRadius;
            Vec3 samplePos = pl.getPosition() + uVec * uOff + vVec * vOff;
            Vec3 toSample = samplePos - hitPoint;
            double sampleDist = toSample.magnitude();
            Vec3 sampleDir = toSample / sampleDist;

            Ray shadowRay(hitPoint + sampleDir * 1e-4, sampleDir);
            if (!castRay(shadowRay, scene, 1e-4, sampleDist - 1e-4)) {
                ++visible;
            }
        }
        return static_cast<double>(visible) / static_cast<double>(shadowSamples);

    } else if (light.getLightType() == LightType::Directional) {
        const DirectionalLight& dl = static_cast<const DirectionalLight&>(light);
        Vec3 dir = dl.getDirectionToLight();

        if (shadowSamples <= 1) {
            Ray shadowRay(hitPoint + dir * 1e-4, dir);
            if (castRay(shadowRay, scene, 1e-4, 1e30)) {
                return 0.0;
            }
            return 1.0;
        }

        Vec3 uVec, vVec;
        if (std::abs(dir.x) > 0.9) {
            uVec = Vec3(0, 1, 0).cross(dir).normalized();
        } else {
            uVec = Vec3(1, 0, 0).cross(dir).normalized();
        }
        vVec = dir.cross(uVec).normalized();

        double coneAngle = 0.02;
        int visible = 0;
        for (int s = 0; s < shadowSamples; ++s) {
            double theta = tlDist(tlRng) * 2.0 * M_PI;
            double phi = tlDist(tlRng) * coneAngle;
            double sinPhi = std::sin(phi);
            Vec3 jittered = (uVec * std::cos(theta) + vVec * std::sin(theta)) * sinPhi
                          + dir * std::cos(phi);
            jittered.normalize();

            Ray shadowRay(hitPoint + jittered * 1e-4, jittered);
            if (!castRay(shadowRay, scene, 1e-4, 1e30)) {
                ++visible;
            }
        }
        return static_cast<double>(visible) / static_cast<double>(shadowSamples);
    }
    return 1.0;
}

Vec3 Renderer::calculateLighting(const HitRecord& hitRecord, const Vec3& rayDir,
                                const Scene& scene, int depth) const
{
    (void)depth;
    const Material* material = hitRecord.getMaterial().get();

    // Ambient
    Vec3 color = calculateAmbient(material, scene);

    // For each light
    for (const auto& lightPtr : scene.getLights()) {
        const ALight& light = *lightPtr;

        if (light.getLightType() == LightType::Ambient) {
            continue;
        }

        Vec3 hitPoint = hitRecord.getPoint();
        double shadow = calculateShadow(hitPoint, light, scene);

        if (light.getLightType() == LightType::Point) {
            const PointLight& pl = static_cast<const PointLight&>(light);
            Vec3 lightDir = (pl.getPosition() - hitPoint).normalized();
            double distance = (pl.getPosition() - hitPoint).magnitude();
            double attenuation = pl.getAttenuationFactor(distance);

            Vec3 diff = calculateDiffuse(hitRecord, light, lightDir, shadow);
            Vec3 spec = calculateSpecular(hitRecord, light, lightDir, rayDir, shadow);

            color += (diff + spec) * attenuation;
        }

        if (light.getLightType() == LightType::Directional) {
            const DirectionalLight& dl = static_cast<const DirectionalLight&>(light);
            Vec3 lightDir = dl.getDirectionToLight();

            Vec3 diff = calculateDiffuse(hitRecord, light, lightDir, shadow);
            Vec3 spec = calculateSpecular(hitRecord, light, lightDir, rayDir, shadow);

            color += diff + spec;
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
        if (lp->getLightType() == LightType::Ambient) {
            ambientColor = ambientColor + lp->getColor() * lp->getIntensity();
        }
    }
    return ambientColor;
}

Vec3 Renderer::calculateDiffuse(const HitRecord& hitRecord, const ALight& light,
                               const Vec3& lightDir, double shadow) const
{
    const Material* material = hitRecord.getMaterial().get();
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
    const Material* material = hitRecord.getMaterial().get();
    if (!material) return Vec3(0,0,0);
    Vec3 normal = hitRecord.getFacingNormal();
    Vec3 half = (lightDir + viewDir).normalized();
    double nh = std::max(0.0, normal.dot(half));
    double specFactor = std::pow(nh, material->getShininess());
    Vec3 spec = light.getColor() * (material->getSpecular() * specFactor * light.getIntensity() * shadow);
    return spec;
}

static double reinhardToneMap(double c)
{
    return c / (1.0 + c);
}

static double srgbGammaEncode(double c)
{
    if (c <= 0.0031308) return 12.92 * c;
    return 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
}

void Renderer::colorToRGBA(const Vec3& color, unsigned char& r, unsigned char& g,
                          unsigned char& b, unsigned char& a) const
{
    double rx = reinhardToneMap(std::max(0.0, color.x));
    double gy = reinhardToneMap(std::max(0.0, color.y));
    double bz = reinhardToneMap(std::max(0.0, color.z));

    r = static_cast<unsigned char>(std::clamp(srgbGammaEncode(rx), 0.0, 1.0) * 255.0);
    g = static_cast<unsigned char>(std::clamp(srgbGammaEncode(gy), 0.0, 1.0) * 255.0);
    b = static_cast<unsigned char>(std::clamp(srgbGammaEncode(bz), 0.0, 1.0) * 255.0);
    a = 255;
}
