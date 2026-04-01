/**
 * @file AObject.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Abstract base class for all scene objects
 *
 * Defines the interface that all geometric primitives (Sphere, Plane, Cylinder, Cone)
 * must implement. Every object has a transformation, material, and can be intersected
 * with rays.
 *
 * @relationships
 * - Base class for: Sphere, Plane, Cylinder, Cone
 * - Composed of: Transform, Material
 * - Uses: Ray, HitRecord
 * - Contained by: Scene
 */

#pragma once

#include "scene/Transform.hpp"
#include "core/Material.hpp"
#include "core/Ray.hpp"
#include <memory>

class HitRecord;

class AObject
{
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~AObject() = default;

    /**
     * @brief Test ray intersection with this object
     * @param ray The ray to intersect with
     * @param tMin Minimum ray parameter (typically 0.0 or small epsilon)
     * @param tMax Maximum ray parameter (for bounded searches)
     * @param hitRecord Output: contains intersection data if hit
     * @return true if intersection found within [tMin, tMax], false otherwise
     */
    virtual bool hit(const Ray& ray, double tMin, double tMax, HitRecord& hitRecord) const = 0;

    /**
     * @brief Set object transformation
     * @param transform The transformation to apply
     */
    virtual void setTransform(const Transform& transform);

    /**
     * @brief Get object transformation
     * @return Reference to the Transform
     */
    virtual const Transform& getTransform() const;

    /**
     * @brief Set object material
     * @param material Shared pointer to material
     */
    virtual void setMaterial(std::shared_ptr<Material> material);

    /**
     * @brief Get object material
     * @return Shared pointer to Material
     */
    virtual std::shared_ptr<Material> getMaterial() const;

    /**
     * @brief Set whether object casts shadows
     * @param castsShadows Whether object should cast shadows
     */
    virtual void setCastsShadows(bool castsShadows);

    /**
     * @brief Check if object casts shadows
     * @return true if object casts shadows
     */
    virtual bool getCastsShadows() const;

    /**
     * @brief Get bounding box of object in object space
     * @param minCorner Output: minimum corner of bounding box
     * @param maxCorner Output: maximum corner of bounding box
     */
    virtual void getBoundingBox(Vec3& minCorner, Vec3& maxCorner) const = 0;

    /**
     * @brief Get bounding box in world space
     * @param minCorner Output: minimum corner in world space
     * @param maxCorner Output: maximum corner in world space
     */
    virtual void getWorldBoundingBox(Vec3& minCorner, Vec3& maxCorner) const;

    /**
     * @brief Get object type identifier (for runtime type checking)
     * @return String identifier of object type
     */
    virtual const char* getType() const = 0;

    /**
     * @brief Get normal at a specific point on object (in world space)
     * @param point Point on object surface (in world space)
     * @return Surface normal at point
     * @note Default implementation should be overridden for efficiency
     */
    virtual Vec3 getNormalAt(const Vec3& point) const;

    /**
     * @brief Get UV coordinates at a point on surface
     * @param point Point on object surface
     * @param u Output: U texture coordinate
     * @param v Output: V texture coordinate
     */
    virtual void getUVAt(const Vec3& point, double& u, double& v) const;

protected:
    /**
     * @brief Protected constructor (only subclasses can instantiate)
     */
    AObject();

    /**
     * @brief Copy constructor (protected for derived classes)
     * @param other The object to copy
     */
    AObject(const AObject& other);

    /**
     * @brief Assignment operator (protected for derived classes)
     * @param other The object to assign
     * @return Reference to this object
     */
    AObject& operator=(const AObject& other);

    Transform transform;                  ///< Object transformation
    std::shared_ptr<Material> material;   ///< Object material
    bool castsShadows;                    ///< Whether object casts shadows
};
