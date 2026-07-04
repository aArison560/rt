/**
 * @file Scene.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Scene implementation
 */

#include "scene/Scene.hpp"
#include <algorithm>

Scene::Scene() : camera(), backgroundColor(0.1, 0.1, 0.1), name("Unnamed Scene"), ambientMultiplier(1.0), objectVersion(0) {}

Scene::~Scene()
{
}

Scene::Scene(const Scene& other) : camera(other.camera), objects(other.objects),
                                   lights(other.lights), backgroundColor(other.backgroundColor),
                                   name(other.name), ambientMultiplier(other.ambientMultiplier),
                                   objectVersion(other.objectVersion) {}

Scene& Scene::operator=(const Scene& other)
{
    if (this == &other) {
        return *this;
    }
    camera = other.camera;
    objects = other.objects;
    lights = other.lights;
    backgroundColor = other.backgroundColor;
    name = other.name;
    ambientMultiplier = other.ambientMultiplier;
    objectVersion = other.objectVersion;
    return *this;
}

void Scene::setCamera(const Camera& camera) { this->camera = camera; }
Camera& Scene::getCamera() { return camera; }
const Camera& Scene::getCamera() const { return camera; }

void Scene::addObject(std::shared_ptr<AObject> object)
{
    if (object) {
        objects.push_back(object);
        ++objectVersion;
    }
}

bool Scene::removeObject(std::shared_ptr<AObject> object)
{
    auto it = std::find(objects.begin(), objects.end(), object);
    if (it == objects.end()) {
        return false;
    }
    objects.erase(it);
    ++objectVersion;
    return true;
}

size_t Scene::getObjectCount() const { return objects.size(); }

std::shared_ptr<AObject> Scene::getObject(size_t index) const
{
    return objects.at(index);
}

const std::vector<std::shared_ptr<AObject>>& Scene::getObjects() const
{
    return objects;
}

void Scene::addLight(std::shared_ptr<ALight> light)
{
    if (light) lights.push_back(light);
}

bool Scene::removeLight(std::shared_ptr<ALight> light)
{
    auto it = std::find(lights.begin(), lights.end(), light);
    if (it == lights.end()) {
        return false;
    }
    lights.erase(it);
    return true;
}

size_t Scene::getLightCount() const { return lights.size(); }

std::shared_ptr<ALight> Scene::getLight(size_t index) const
{
    return lights.at(index);
}

const std::vector<std::shared_ptr<ALight>>& Scene::getLights() const
{
    return lights;
}

void Scene::setBackgroundColor(const Vec3& color) { backgroundColor = color; }
const Vec3& Scene::getBackgroundColor() const { return backgroundColor; }

void Scene::clearObjects() { objects.clear(); ++objectVersion; }
void Scene::clearLights() { lights.clear(); }
void Scene::clear() { clearObjects(); clearLights(); }

const std::string& Scene::getName() const { return name; }
void Scene::setName(const std::string& name) { this->name = name; }

double Scene::getAmbientMultiplier() const { return ambientMultiplier; }
void Scene::setAmbientMultiplier(double multiplier) { ambientMultiplier = multiplier; }

uint64_t Scene::getObjectVersion() const { return objectVersion; }
void Scene::resetObjectVersion() { objectVersion = 0; }
