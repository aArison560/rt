/**
 * @file Material.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Material properties for lighting calculations
 *
 * Defines material properties used in Phong lighting model including
 * color, ambient/diffuse/specular coefficients, shininess, and optional texture.
 *
 * @relationships
 * - Composed of: Vec3 (color), optional Texture pointer
 * - Used by: AObject and all subclasses (sphere, plane, etc.)
 * - Used by: Renderer (lighting calculations)
 * - Embeds: reflectivity and transparency for advanced effects
 */

#pragma once

#include "Vec3.hpp"

class Texture;  // Forward declaration

class Material
{
public:
    /**
     * @brief Default constructor - creates white matte material
     */
    Material();

    /**
     * @brief Constructor with basic properties
     * @param color The material color
     * @param ambient Ambient coefficient (0.0-1.0)
     * @param diffuse Diffuse coefficient (0.0-1.0)
     * @param specular Specular coefficient (0.0-1.0)
     * @param shininess Shininess coefficient (typically 8-256)
     */
    Material(const Vec3& color, double ambient, double diffuse, double specular, double shininess);

    /**
     * @brief Copy constructor
     * @param other The Material to copy
     */
    Material(const Material& other);

    /**
     * @brief Assignment operator
     * @param other The Material to assign
     * @return Reference to this Material
     */
    Material& operator=(const Material& other);

    /**
     * @brief Set material color
     * @param color The new color
     */
    void setColor(const Vec3& color);

    /**
     * @brief Get material color
     * @return The color as Vec3 (RGB in range 0-1)
     */
    const Vec3& getColor() const;

    /**
     * @brief Set ambient coefficient
     * @param ambient Ambient coefficient (0.0-1.0)
     */
    void setAmbient(double ambient);

    /**
     * @brief Get ambient coefficient
     * @return The ambient coefficient
     */
    double getAmbient() const;

    /**
     * @brief Set diffuse coefficient
     * @param diffuse Diffuse coefficient (0.0-1.0)
     */
    void setDiffuse(double diffuse);

    /**
     * @brief Get diffuse coefficient
     * @return The diffuse coefficient
     */
    double getDiffuse() const;

    /**
     * @brief Set specular coefficient
     * @param specular Specular coefficient (0.0-1.0)
     */
    void setSpecular(double specular);

    /**
     * @brief Get specular coefficient
     * @return The specular coefficient
     */
    double getSpecular() const;

    /**
     * @brief Set shininess coefficient
     * @param shininess Shininess value (typically 8-256)
     */
    void setShininess(double shininess);

    /**
     * @brief Get shininess coefficient
     * @return The shininess value
     */
    double getShininess() const;

    /**
     * @brief Set reflectivity
     * @param reflectivity Reflection coefficient (0.0-1.0)
     */
    void setReflectivity(double reflectivity);

    /**
     * @brief Get reflectivity
     * @return The reflectivity coefficient
     */
    double getReflectivity() const;

    /**
     * @brief Set transparency
     * @param transparency Transparency coefficient (0.0-1.0, 0=opaque)
     */
    void setTransparency(double transparency);

    /**
     * @brief Get transparency
     * @return The transparency coefficient
     */
    double getTransparency() const;

    /**
     * @brief Set refractive index
     * @param refractiveIndex Refractive index (1.0 for vacuum/air)
     */
    void setRefractiveIndex(double refractiveIndex);

    /**
     * @brief Get refractive index
     * @return The refractive index
     */
    double getRefractiveIndex() const;

    /**
     * @brief Set texture
     * @param texture Pointer to texture object (can be nullptr)
     */
    void setTexture(Texture* texture);

    /**
     * @brief Get texture
     * @return Pointer to texture or nullptr if no texture
     */
    Texture* getTexture() const;

    /**
     * @brief Check if material has a texture
     * @return true if material has a texture
     */
    bool hasTexture() const;

private:
    Vec3 color;              ///< Material color (RGB)
    double ambient;          ///< Ambient lighting coefficient
    double diffuse;          ///< Diffuse lighting coefficient
    double specular;         ///< Specular lighting coefficient
    double shininess;        ///< Shininess/glossiness coefficient
    double reflectivity;     ///< Reflection strength (0.0-1.0)
    double transparency;     ///< Transparency (0.0 = opaque, 1.0 = transparent)
    double refractiveIndex;  ///< Refractive index for transparency
    Texture* texture;        ///< Optional texture pointer
};
