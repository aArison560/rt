/**
 * @file Texture.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Texture implementation
 */

#include "rendering/Texture.hpp"
#include <algorithm>
#include <utility>

Texture::Texture() : width(0), height(0), channels(0), data(nullptr), valid(false) {}

Texture::Texture(const std::string& filePath) : width(0), height(0), channels(0), data(nullptr), valid(false)
{
    load(filePath);
}

Texture::Texture(const Texture& other) : width(other.width), height(other.height), 
                                         channels(other.channels), valid(other.valid)
{
    if (other.data) {
        const size_t sz = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
        data = std::make_unique<unsigned char[]>(sz);
        std::copy(other.data.get(), other.data.get() + sz, data.get());
    }
}

Texture::Texture(Texture&& other) noexcept : width(other.width), height(other.height),
                                             channels(other.channels), data(std::move(other.data)),
                                             valid(other.valid) {}

Texture& Texture::operator=(const Texture& other)
{
    if (this == &other) {
        return *this;
    }
    width = other.width;
    height = other.height;
    channels = other.channels;
    valid = other.valid;
    if (other.data) {
        const size_t sz = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
        data = std::make_unique<unsigned char[]>(sz);
        std::copy(other.data.get(), other.data.get() + sz, data.get());
    } else {
        data.reset();
    }
    return *this;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    width = other.width;
    height = other.height;
    channels = other.channels;
    data = std::move(other.data);
    valid = other.valid;
    other.width = 0;
    other.height = 0;
    other.channels = 0;
    other.valid = false;
    return *this;
}

Texture::~Texture()
{
    // TODO: Clean up resources (data is unique_ptr, auto-cleanup)
}

bool Texture::load(const std::string& filePath)
{
    // TODO: Load PNG or JPEG using libpng/libjpeg
    // Set data, width, height, channels, valid
    (void)filePath;
    valid = false;
    return false;
}

bool Texture::isValid() const
{
    return valid;
}

Vec3 Texture::sample(double u, double v) const
{
    // TODO: Sample color at UV without filtering
    clampUV(u, v);
    int x = (int)(u * width);
    int y = (int)(v * height);
    return getPixel(x, y);
}

Vec3 Texture::sampleFiltered(double u, double v) const
{
    // TODO: Bilinear filtering
    return sample(u, v);
}

int Texture::getWidth() const { return width; }
int Texture::getHeight() const { return height; }
int Texture::getChannels() const { return channels; }

void Texture::clampUV(double& u, double& v) const
{
    // TODO: Clamp UV to [0, 1]
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
}

Vec3 Texture::getPixel(int x, int y) const
{
    // TODO: Get pixel and normalize to [0, 1]
    (void)x;
    (void)y;
    return Vec3();
}
