/**
 * @file Scene.cpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Scene implementation
 */

#include "Scene.hpp"

Scene::Scene() : camera(), backgroundColor(0.1, 0.1, 0.1), name("Unnamed Scene"), ambientMultiplier(1.0) {}

Scene::~Scene()
{
    // TODO: Cleanup (smart pointers handle this automatically)
}

Scene::Scene(const Scene& other) : camera(other.camera), objects(other.objects),
                                   lights(other.lights), backgroundColor(other.backgroundColor),
                                   name(other.name), ambientMultiplier(other.ambientMultiplier) {}

Scene& Scene::operator=(const Scene& other)
{
    // TODO: Assignment
    return *this;
}

void Scene::setCamera(const Camera& camera) { this->camera = camera; }
Camera& Scene::getCamera() { return camera; }
const Camera& Scene::getCamera() const { return camera; }

void Scene::addObject(std::shared_ptr<AObject> object)
{
    // TODO: Add object to scene
    if (object) objects.push_back(object);
}

bool Scene::removeObject(std::shared_ptr<AObject> object)
{
    // TODO: Remove object from scene
    return false;
}

size_t Scene::getObjectCount() const { return objects.size(); }

std::shared_ptr<AObject> Scene::getObject(size_t index) const
{
    // TODO: Get object with bounds checking
    return objects.at(index);
}

const std::vector<std::shared_ptr<AObject>>& Scene::getObjects() const
{
    return objects;
}

void Scene::addLight(std::shared_ptr<ALight> light)
{
    // TODO: Add light to scene
    if (light) lights.push_back(light);
}

bool Scene::removeLight(std::shared_ptr<ALight> light)
{
    // TODO: Remove light from scene
    return false;
}

size_t Scene::getLightCount() const { return lights.size(); }

std::shared_ptr<ALight> Scene::getLight(size_t index) const
{
    // TODO: Get light with bounds checking
    return lights.at(index);
}

const std::vector<std::shared_ptr<ALight>>& Scene::getLights() const
{
    return lights;
}

void Scene::setBackgroundColor(const Vec3& color) { backgroundColor = color; }
const Vec3& Scene::getBackgroundColor() const { return backgroundColor; }

void Scene::clearObjects() { objects.clear(); }
void Scene::clearLights() { lights.clear(); }
void Scene::clear() { clearObjects(); clearLights(); }

const std::string& Scene::getName() const { return name; }
void Scene::setName(const std::string& name) { this->name = name; }

double Scene::getAmbientMultiplier() const { return ambientMultiplier; }
void Scene::setAmbientMultiplier(double multiplier) { ambientMultiplier = multiplier; }
