/**
 * @file ImageBuffer.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief ImageBuffer implementation
 */

#include "rendering/ImageBuffer.hpp"
#include <algorithm>
#include <utility>
#include <png.h>

ImageBuffer::ImageBuffer() : width(0), height(0), data(nullptr) {}

ImageBuffer::ImageBuffer(int width, int height) : width(0), height(0), data(nullptr)
{
    allocate(width, height);
}

ImageBuffer::~ImageBuffer()
{
}

ImageBuffer::ImageBuffer(const ImageBuffer& other) : width(other.width), height(other.height)
{
    if (other.data) {
        data = std::make_unique<unsigned char[]>(other.getDataSize());
        std::copy(other.data.get(), other.data.get() + other.getDataSize(), data.get());
    }
}

ImageBuffer::ImageBuffer(ImageBuffer&& other) noexcept 
    : width(other.width), height(other.height), data(std::move(other.data)) {}

ImageBuffer& ImageBuffer::operator=(const ImageBuffer& other)
{
    if (this == &other) {
        return *this;
    }
    width = other.width;
    height = other.height;
    if (other.data) {
        data = std::make_unique<unsigned char[]>(other.getDataSize());
        std::copy(other.data.get(), other.data.get() + other.getDataSize(), data.get());
    } else {
        data.reset();
    }
    return *this;
}

ImageBuffer& ImageBuffer::operator=(ImageBuffer&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    width = other.width;
    height = other.height;
    data = std::move(other.data);
    other.width = 0;
    other.height = 0;
    return *this;
}

void ImageBuffer::allocate(int width, int height)
{
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
    if (!inBounds(x,y) || !data) return;
    size_t off = pixelOffset(x,y);
    data[off+0] = r;
    data[off+1] = g;
    data[off+2] = b;
    data[off+3] = a;
}

void ImageBuffer::setPixel(int x, int y, const Vec3& color)
{
    unsigned char r = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255.0);
    unsigned char g = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255.0);
    unsigned char b = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255.0);
    setPixel(x, y, r, g, b, 255);
}

void ImageBuffer::getPixel(int x, int y, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
    if (!inBounds(x,y) || !data) { r = g = b = a = 0; return; }
    size_t off = pixelOffset(x,y);
    r = data[off+0];
    g = data[off+1];
    b = data[off+2];
    a = data[off+3];
}

void ImageBuffer::fill(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    if (!data) return;
    size_t total = getPixelCount();
    for (size_t i = 0; i < total; ++i) {
        data[i*4 + 0] = r;
        data[i*4 + 1] = g;
        data[i*4 + 2] = b;
        data[i*4 + 3] = a;
    }
}

void ImageBuffer::fill(const ImageBuffer& other)
{
    if (!data || !other.data) return;
    if (width != other.width || height != other.height) return;
    std::copy(other.data.get(), other.data.get() + getDataSize(), data.get());
}

void ImageBuffer::clear()
{
    if (!data) return;
    std::fill_n(data.get(), getDataSize(), 0);
}

size_t ImageBuffer::getPixelCount() const { return static_cast<size_t>(width) * height; }
size_t ImageBuffer::getDataSize() const { return getPixelCount() * 4; }

bool ImageBuffer::savePNG(const std::filesystem::path& filename) const
{
    if (!data || width <= 0 || height <= 0) return false;

    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) return false;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { fclose(fp); return false; }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, nullptr); fclose(fp); return false; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    for (int y = 0; y < height; ++y) {
        png_write_row(png, data.get() + static_cast<size_t>(y) * width * 4);
    }

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return true;
}

size_t ImageBuffer::pixelOffset(int x, int y) const
{
    return (static_cast<size_t>(y) * width + x) * 4;
}

bool ImageBuffer::inBounds(int x, int y) const
{
    return x >= 0 && x < width && y >= 0 && y < height;
}
