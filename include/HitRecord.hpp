/**
 * @file HitRecord.hpp
 * @author RT Team - Dev B
 * @date 2026-04-01
 * @brief Record of ray-object intersection information
 *
 * Stores all relevant data from a ray-object intersection including
 * the intersection point, surface normal, parameter t, and pointer
 * to the hit object and material.
 *
 * @relationships
 * - Contains: Vec3 (point, normal), pointers to Material and AObject
 * - Created by: AObject::hit() methods
 * - Used by: Renderer (lighting calculations, reflection/refraction)
 * - Used by: Shadow ray calculations, reflection tracing
 */

#pragma once

#include "Vec3.hpp"

class Material;
class AObject;

class HitRecord
{
public:
    /**
     * @brief Default constructor - creates invalid hit record
     */
    HitRecord();

    /**
     * @brief Constructor with intersection data
     * @param t Ray parameter at intersection
     * @param point 3D point of intersection
     * @param normal Surface normal at intersection (should point away from object)
     * @param material Pointer to material at intersection
     * @param object Pointer to intersected object
     */
    HitRecord(double t, const Vec3& point, const Vec3& normal,
              Material* material, AObject* object);

    /**
     * @brief Copy constructor
     * @param other The HitRecord to copy
     */
    HitRecord(const HitRecord& other);

    /**
     * @brief Assignment operator
     * @param other The HitRecord to assign
     * @return Reference to this HitRecord
     */
    HitRecord& operator=(const HitRecord& other);

    /**
     * @brief Check if this represents a valid hit
     * @return true if hit is valid
     */
    bool isValid() const;

    /**
     * @brief Get ray parameter t at intersection
     * @return The t value
     */
    double getT() const;

    /**
     * @brief Set ray parameter t
     * @param t The new t value
     */
    void setT(double t);

    /**
     * @brief Get intersection point
     * @return The 3D point as Vec3
     */
    const Vec3& getPoint() const;

    /**
     * @brief Set intersection point
     * @param point The new point
     */
    void setPoint(const Vec3& point);

    /**
     * @brief Get surface normal at intersection
     * @return The normal as Vec3 (should be normalized)
     */
    const Vec3& getNormal() const;

    /**
     * @brief Set surface normal
     * @param normal The new normal
     */
    void setNormal(const Vec3& normal);

    /**
     * @brief Get material at intersection
     * @return Pointer to Material
     */
    Material* getMaterial() const;

    /**
     * @brief Set material
     * @param material Pointer to new Material
     */
    void setMaterial(Material* material);

    /**
     * @brief Get intersected object
     * @return Pointer to AObject
     */
    AObject* getObject() const;

    /**
     * @brief Set intersected object
     * @param object Pointer to new AObject
     */
    void setObject(AObject* object);

    /**
     * @brief Get UV coordinates at intersection (if applicable)
     * @param u Reference to store U coordinate
     * @param v Reference to store V coordinate
     */
    void getUV(double& u, double& v) const;

    /**
     * @brief Set UV coordinates
     * @param u U coordinate (0.0-1.0)
     * @param v V coordinate (0.0-1.0)
     */
    void setUV(double u, double v);

    /**
     * @brief Check if hit is on front face (ray entering object)
     * @return true if front face hit
     */
    bool isFrontFace() const;

    /**
     * @brief Set front face flag
     * @param frontFace Whether this is a front face hit
     */
    void setFrontFace(bool frontFace);

    /**
     * @brief Get facing normal (points toward ray origin if front, opposite if back)
     * @return The facing normal as Vec3
     */
    Vec3 getFacingNormal() const;

    /**
     * @brief Reset hit record to invalid state
     */
    void invalidate();

private:
    double t;           ///< Ray parameter at intersection
    Vec3 point;         ///< 3D point of intersection
    Vec3 normal;        ///< Surface normal (pointing outward)
    double u, v;        ///< UV coordinates at intersection
    Material* material; ///< Pointer to material at intersection
    AObject* object;    ///< Pointer to intersected object
    bool valid;         ///< Whether this hit is valid
    bool frontFace;     ///< Whether ray hit front face of object
};
