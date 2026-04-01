/**
 * @file ImageBuffer.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief CPU-side pixel buffer management
 *
 * Manages a pixel buffer in main memory and provides utilities for
 * pixel manipulation and texture creation from buffer data.
 *
 * @relationships
 * - Composed of: dynamic pixel array (RGBA format)
 * - Used by: Window (display/update), Renderer (output)
 * - Managed by: Window class typically
 */

#pragma once

#include "Vec3.hpp"
#include <memory>

class ImageBuffer
{
public:
    /**
     * @brief Default constructor - creates empty buffer
     */
    ImageBuffer();

    /**
     * @brief Constructor with explicit dimensions
     * @param width Buffer width in pixels
     * @param height Buffer height in pixels
     */
    ImageBuffer(int width, int height);

    /**
     * @brief Destructor
     */
    ~ImageBuffer();

    /**
     * @brief Copy constructor
     * @param other The ImageBuffer to copy
     */
    ImageBuffer(const ImageBuffer& other);

    /**
     * @brief Move constructor
     * @param other The ImageBuffer to move
     */
    ImageBuffer(ImageBuffer&& other) noexcept;

    /**
     * @brief Assignment operator
     * @param other The ImageBuffer to assign
     * @return Reference to this ImageBuffer
     */
    ImageBuffer& operator=(const ImageBuffer& other);

    /**
     * @brief Move assignment operator
     * @param other The ImageBuffer to move assign
     * @return Reference to this ImageBuffer
     */
    ImageBuffer& operator=(ImageBuffer&& other) noexcept;

    /**
     * @brief Allocate/reallocate buffer
     * @param width Buffer width in pixels
     * @param height Buffer height in pixels
     */
    void allocate(int width, int height);

    /**
     * @brief Check if buffer is allocated
     * @return true if buffer has data
     */
    bool isAllocated() const;

    /**
     * @brief Get buffer width
     * @return Width in pixels
     */
    int getWidth() const;

    /**
     * @brief Get buffer height
     * @return Height in pixels
     */
    int getHeight() const;

    /**
     * @brief Get pointer to pixel data
     * @return Pointer to RGBA data (width*height*4 bytes)
     */
    unsigned char* getData() const;

    /**
     * @brief Get pointer to pixel data (const)
     * @return Const pointer to RGBA data
     */
    const unsigned char* getDataConst() const;

    /**
     * @brief Set pixel color
     * @param x X coordinate
     * @param y Y coordinate
     * @param r Red channel (0-255)
     * @param g Green channel (0-255)
     * @param b Blue channel (0-255)
     * @param a Alpha channel (0-255, default 255)
     */
    void setPixel(int x, int y, unsigned char r, unsigned char g,
                 unsigned char b, unsigned char a = 255);

    /**
     * @brief Set pixel from Vec3 color
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Color as Vec3 (RGB in 0-1 range)
     */
    void setPixel(int x, int y, const Vec3& color);

    /**
     * @brief Get pixel color
     * @param x X coordinate
     * @param y Y coordinate
     * @param r Output: red channel
     * @param g Output: green channel
     * @param b Output: blue channel
     * @param a Output: alpha channel
     */
    void getPixel(int x, int y, unsigned char& r, unsigned char& g,
                 unsigned char& b, unsigned char& a) const;

    /**
     * @brief Fill entire buffer with constant color
     * @param r Red channel (0-255)
     * @param g Green channel (0-255)
     * @param b Blue channel (0-255)
     * @param a Alpha channel (0-255)
     */
    void fill(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

    /**
     * @brief Fill buffer from another buffer
     * @param other Source buffer (must have same dimensions)
     */
    void fill(const ImageBuffer& other);

    /**
     * @brief Clear buffer (fill with black transparent)
     */
    void clear();

    /**
     * @brief Get total pixel count
     * @return Width * Height
     */
    size_t getPixelCount() const;

    /**
     * @brief Get total data size in bytes
     * @return Pixel count * 4 (RGBA)
     */
    size_t getDataSize() const;

private:
    int width;                  ///< Buffer width
    int height;                 ///< Buffer height
    std::unique_ptr<unsigned char[]> data; ///< Pixel data (RGBA format)

    /**
     * @brief Calculate pixel offset
     * @param x X coordinate
     * @param y Y coordinate
     * @return Byte offset in data array
     */
    size_t pixelOffset(int x, int y) const;

    /**
     * @brief Check if coordinates are in bounds
     * @param x X coordinate
     * @param y Y coordinate
     * @return true if coordinates are valid
     */
    bool inBounds(int x, int y) const;
};
