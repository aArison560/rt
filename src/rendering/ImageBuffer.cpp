/**
 * @file ImageBuffer.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief ImageBuffer implementation
 */

#include "rendering/ImageBuffer.hpp"

ImageBuffer::ImageBuffer() : width(0), height(0), data(nullptr) {}

ImageBuffer::ImageBuffer(int width, int height) : width(0), height(0), data(nullptr)
{
    allocate(width, height);
}

ImageBuffer::~ImageBuffer()
{
    // TODO: Cleanup (unique_ptr handles this)
}

ImageBuffer::ImageBuffer(const ImageBuffer& other) : width(other.width), height(other.height)
{
    // TODO: Deep copy data
}

ImageBuffer::ImageBuffer(ImageBuffer&& other) noexcept 
    : width(other.width), height(other.height), data(std::move(other.data)) {}

ImageBuffer& ImageBuffer::operator=(const ImageBuffer& other)
{
    // TODO: Assignment with deep copy
    return *this;
}

ImageBuffer& ImageBuffer::operator=(ImageBuffer&& other) noexcept
{
    // TODO: Move assignment
    return *this;
}

void ImageBuffer::allocate(int width, int height)
{
    // TODO: Allocate width*height*4 bytes for RGBA data
    this->width = width;
    this->height = height;
    data = std::make_unique<unsigned char[]>(static_cast<size_t>(width) * height * 4);
}

bool ImageBuffer::isAllocated() const { return data != nullptr; }
int ImageBuffer::getWidth() const { return width; }
int ImageBuffer::getHeight() const { return height; }
unsigned char* ImageBuffer::getData() const { return data.get(); }
const unsigned char* ImageBuffer::getDataConst() const { return data.get(); }

void ImageBuffer::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    // TODO: Set pixel with bounds checking
}

void ImageBuffer::setPixel(int x, int y, const Vec3& color)
{
    // TODO: Convert Vec3 [0,1] to RGBA [0,255]
}

void ImageBuffer::getPixel(int x, int y, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
    // TODO: Get pixel with bounds checking
}

void ImageBuffer::fill(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    // TODO: Fill entire buffer with color
}

void ImageBuffer::fill(const ImageBuffer& other)
{
    // TODO: Fill with another buffer's data
}

void ImageBuffer::clear()
{
    // TODO: Clear to black transparent (0,0,0,0)
}

size_t ImageBuffer::getPixelCount() const { return static_cast<size_t>(width) * height; }
size_t ImageBuffer::getDataSize() const { return getPixelCount() * 4; }

size_t ImageBuffer::pixelOffset(int x, int y) const
{
    // TODO: Calculate byte offset for pixel
    return (static_cast<size_t>(y) * width + x) * 4;
}

bool ImageBuffer::inBounds(int x, int y) const
{
    // TODO: Check if coordinates are valid
    return x >= 0 && x < width && y >= 0 && y < height;
}
