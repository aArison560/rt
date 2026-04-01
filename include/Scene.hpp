/**
 * @file Scene.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Scene container for objects, lights, and camera
 *
 * Holds all the elements of a scene: geometric objects, lights, and camera.
 * Provides methods to add/remove objects and lights, retrieve them, and
 * manage the scene state.
 *
 * @relationships
 * - Contains: Camera, list of AObject*, list of ALight*
 * - Composed of: Vec3 (background color)
 * - Used by: Renderer (renders scene)
 * - Used by: SceneParser (populates scene from file)
 */

#pragma once

#include "Camera.hpp"
#include "AObject.hpp"
#include "ALight.hpp"
#include <vector>
#include <memory>

class Scene
{
public:
    /**
     * @brief Default constructor - creates empty scene with default camera
     */
    Scene();

    /**
     * @brief Destructor
     */
    ~Scene();

    /**
     * @brief Copy constructor
     * @param other The Scene to copy
     */
    Scene(const Scene& other);

    /**
     * @brief Assignment operator
     * @param other The Scene to assign
     * @return Reference to this Scene
     */
    Scene& operator=(const Scene& other);

    /**
     * @brief Set camera for the scene
     * @param camera The camera object
     */
    void setCamera(const Camera& camera);

    /**
     * @brief Get camera
     * @return Reference to Camera
     */
    Camera& getCamera();

    /**
     * @brief Get camera (const)
     * @return Const reference to Camera
     */
    const Camera& getCamera() const;

    /**
     * @brief Add object to scene
     * @param object Shared pointer to object to add
     */
    void addObject(std::shared_ptr<AObject> object);

    /**
     * @brief Remove object from scene
     * @param object Shared pointer to object to remove
     * @return true if object was removed, false if not found
     */
    bool removeObject(std::shared_ptr<AObject> object);

    /**
     * @brief Get number of objects in scene
     * @return Object count
     */
    size_t getObjectCount() const;

    /**
     * @brief Get object at index
     * @param index Object index
     * @return Shared pointer to object
     * @throw std::out_of_range if index is invalid
     */
    std::shared_ptr<AObject> getObject(size_t index) const;

    /**
     * @brief Get all objects
     * @return Vector of all objects
     */
    const std::vector<std::shared_ptr<AObject>>& getObjects() const;

    /**
     * @brief Add light to scene
     * @param light Shared pointer to light to add
     */
    void addLight(std::shared_ptr<ALight> light);

    /**
     * @brief Remove light from scene
     * @param light Shared pointer to light to remove
     * @return true if light was removed, false if not found
     */
    bool removeLight(std::shared_ptr<ALight> light);

    /**
     * @brief Get number of lights in scene
     * @return Light count
     */
    size_t getLightCount() const;

    /**
     * @brief Get light at index
     * @param index Light index
     * @return Shared pointer to light
     * @throw std::out_of_range if index is invalid
     */
    std::shared_ptr<ALight> getLight(size_t index) const;

    /**
     * @brief Get all lights
     * @return Vector of all lights
     */
    const std::vector<std::shared_ptr<ALight>>& getLights() const;

    /**
     * @brief Set background color
     * @param color The background color (RGB)
     */
    void setBackgroundColor(const Vec3& color);

    /**
     * @brief Get background color
     * @return The background color as Vec3
     */
    const Vec3& getBackgroundColor() const;

    /**
     * @brief Clear all objects from scene
     */
    void clearObjects();

    /**
     * @brief Clear all lights from scene
     */
    void clearLights();

    /**
     * @brief Clear entire scene (objects, lights, but keep camera)
     */
    void clear();

    /**
     * @brief Get scene name/identifier
     * @return Scene name
     */
    const std::string& getName() const;

    /**
     * @brief Set scene name/identifier
     * @param name The scene name
     */
    void setName(const std::string& name);

    /**
     * @brief Get scene ambient multiplier
     * @return Ambient multiplier (0.0-1.0+)
     */
    double getAmbientMultiplier() const;

    /**
     * @brief Set scene ambient multiplier
     * @param multiplier Ambient multiplier
     */
    void setAmbientMultiplier(double multiplier);

private:
    Camera camera;                              ///< Scene camera
    std::vector<std::shared_ptr<AObject>> objects; ///< Scene objects
    std::vector<std::shared_ptr<ALight>> lights;   ///< Scene lights
    Vec3 backgroundColor;                       ///< Background color
    std::string name;                           ///< Scene name
    double ambientMultiplier;                   ///< Global ambient multiplier
};
