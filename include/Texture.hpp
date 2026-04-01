/**
 * @file Texture.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Texture loading and sampling for materials
 *
 * Loads PNG and JPEG image files and provides UV-based color sampling
 * for texture mapping and bump mapping effects.
 *
 * @relationships
 * - Composed of: pixel data buffer
 * - Used by: Material (optional texture pointer)
 * - Used by: Renderer (sample colors at UV coordinates)
 * - Supports: PNG and JPEG formats via libpng and libjpeg
 */

#pragma once

#include "Vec3.hpp"
#include <string>
#include <memory>

class Texture
{
public:
    /**
     * @brief Default constructor - creates invalid texture
     */
    Texture();

    /**
     * @brief Constructor that loads texture from file
     * @param filePath Path to PNG or JPEG image file
     * @throw std::runtime_error if file cannot be loaded
     */
    Texture(const std::string& filePath);

    /**
     * @brief Copy constructor
     * @param other The Texture to copy
     */
    Texture(const Texture& other);

    /**
     * @brief Move constructor
     * @param other The Texture to move
     */
    Texture(Texture&& other) noexcept;

    /**
     * @brief Assignment operator
     * @param other The Texture to assign
     * @return Reference to this Texture
     */
    Texture& operator=(const Texture& other);

    /**
     * @brief Move assignment operator
     * @param other The Texture to move assign
     * @return Reference to this Texture
     */
    Texture& operator=(Texture&& other) noexcept;

    /**
     * @brief Destructor - releases image data
     */
    ~Texture();

    /**
     * @brief Load texture from file
     * @param filePath Path to PNG or JPEG image file
     * @return true if load successful, false otherwise
     */
    bool load(const std::string& filePath);

    /**
     * @brief Check if texture is valid and loaded
     * @return true if texture data is available
     */
    bool isValid() const;

    /**
     * @brief Sample color from texture at UV coordinates
     * @param u U coordinate (0.0-1.0)
     * @param v V coordinate (0.0-1.0)
     * @return Color at UV as Vec3 (RGB normalized to 0-1)
     */
    Vec3 sample(double u, double v) const;

    /**
     * @brief Sample color with bilinear filtering
     * @param u U coordinate (0.0-1.0)
     * @param v V coordinate (0.0-1.0)
     * @return Filtered color at UV as Vec3
     */
    Vec3 sampleFiltered(double u, double v) const;

    /**
     * @brief Get texture width in pixels
     * @return Width in pixels
     */
    int getWidth() const;

    /**
     * @brief Get texture height in pixels
     * @return Height in pixels
     */
    int getHeight() const;

    /**
     * @brief Get number of channels (3 for RGB, 4 for RGBA)
     * @return Channel count
     */
    int getChannels() const;

private:
    int width;           ///< Texture width in pixels
    int height;          ///< Texture height in pixels
    int channels;        ///< Number of color channels (3 or 4)
    std::unique_ptr<unsigned char[]> data; ///< Pixel data (RGBA format)
    bool valid;          ///< Whether texture is loaded and valid

    /**
     * @brief Clamp UV coordinates to [0, 1] range
     * @param u Reference to U coordinate
     * @param v Reference to V coordinate
     */
    void clampUV(double& u, double& v) const;

    /**
     * @brief Get pixel data at image coordinates
     * @param x X pixel coordinate
     * @param y Y pixel coordinate
     * @return Color as Vec3 (RGB normalized to 0-1)
     */
    Vec3 getPixel(int x, int y) const;
};
