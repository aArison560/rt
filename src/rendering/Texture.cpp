/**
 * @file Texture.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Texture implementation
 */

#include "rendering/Texture.hpp"
#include <algorithm>
#include <filesystem>
#include <utility>
#include <png.h>
#include <jpeglib.h>
#include <cstdio>
#include <csetjmp>
#include <vector>

Texture::Texture() : width(0), height(0), channels(0), data(nullptr), valid(false) {}

Texture::Texture(const std::filesystem::path& filePath) : width(0), height(0), channels(0), data(nullptr), valid(false)
{
    if (!load(filePath)) {
        valid = false;
    }
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
}

bool Texture::load(const std::filesystem::path& filePath)
{
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) return false;

    unsigned char sig[8];
    size_t bytesRead = fread(sig, 1, 8, fp);

    // --- PNG path ---
    if (bytesRead == 8 && png_sig_cmp(sig, 0, 8) == 0) {
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) { fclose(fp); return false; }

        png_infop info = png_create_info_struct(png);
        if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return false; }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return false;
        }

        png_init_io(png, fp);
        png_set_sig_bytes(png, 8);
        png_read_info(png, info);

        width = png_get_image_width(png, info);
        height = png_get_image_height(png, info);
        int bitDepth = png_get_bit_depth(png, info);
        int colorType = png_get_color_type(png, info);

        if (bitDepth == 16) png_set_strip_16(png);
        if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
        if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(png);
        if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
        if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_PALETTE) {
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        }

        png_read_update_info(png, info);
        channels = 4;

        data = std::make_unique<unsigned char[]>(static_cast<size_t>(width) * height * 4);
        std::vector<png_bytep> rowPointers(height);
        for (int y = 0; y < height; ++y) {
            rowPointers[y] = data.get() + static_cast<size_t>(y) * width * 4;
        }
        png_read_image(png, rowPointers.data());
        png_read_end(png, nullptr);
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);

        valid = true;
        return true;
    }

    // --- JPEG path ---
    // Rewind to start of file (first 8 bytes already consumed)
    rewind(fp);

    struct jpegErrorManager {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    struct jpeg_decompress_struct cinfo;
    jpegErrorManager jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = [](j_common_ptr cinfoPtr) {
        jpegErrorManager* myerr = reinterpret_cast<jpegErrorManager*>(cinfoPtr->err);
        (*cinfoPtr->err->output_message)(cinfoPtr);
        longjmp(myerr->setjmp_buffer, 1);
    };

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

    // Request decompression to RGB
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    width = static_cast<int>(cinfo.output_width);
    height = static_cast<int>(cinfo.output_height);
    channels = 4;

    const int rowStride = width * static_cast<int>(cinfo.output_components);
    data = std::make_unique<unsigned char[]>(static_cast<size_t>(width) * height * 4);

    // Temporary buffer for one RGB row
    std::vector<unsigned char> rgbRow(static_cast<size_t>(rowStride));

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char* rowPtrs[1] = { rgbRow.data() };
        jpeg_read_scanlines(&cinfo, rowPtrs, 1);

        size_t dstOff = static_cast<size_t>(cinfo.output_scanline - 1) * static_cast<size_t>(width) * 4;
        for (int x = 0; x < width; ++x) {
            data[dstOff + x * 4 + 0] = rgbRow[static_cast<size_t>(x) * 3 + 0];
            data[dstOff + x * 4 + 1] = rgbRow[static_cast<size_t>(x) * 3 + 1];
            data[dstOff + x * 4 + 2] = rgbRow[static_cast<size_t>(x) * 3 + 2];
            data[dstOff + x * 4 + 3] = 0xFF;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    valid = true;
    return true;
}

bool Texture::isValid() const
{
    return valid;
}

Vec3 Texture::sample(double u, double v) const
{
    clampUV(u, v);
    int x = (int)(u * width);
    int y = (int)(v * height);
    return getPixel(x, y);
}

Vec3 Texture::sampleFiltered(double u, double v) const
{
    clampUV(u, v);

    if (!valid || width == 0 || height == 0)
        return Vec3();

    // Convert UV to floating-point pixel coordinates
    double fx = u * static_cast<double>(width);
    double fy = v * static_cast<double>(height);

    // Clamp to the last pixel boundary to avoid out-of-bounds
    fx = std::clamp(fx, 0.0, static_cast<double>(width - 1));
    fy = std::clamp(fy, 0.0, static_cast<double>(height - 1));

    int ix = static_cast<int>(fx);
    int iy = static_cast<int>(fy);
    double tx = fx - static_cast<double>(ix);
    double ty = fy - static_cast<double>(iy);

    // If exactly on a pixel boundary, fall back to nearest-neighbor
    if (tx < Vec3::EPSILON && ty < Vec3::EPSILON)
        return getPixel(ix, iy);

    int ix1 = std::min(ix + 1, width - 1);
    int iy1 = std::min(iy + 1, height - 1);

    // Sample the four neighboring pixels
    Vec3 c00 = getPixel(ix, iy);
    Vec3 c10 = getPixel(ix1, iy);
    Vec3 c01 = getPixel(ix, iy1);
    Vec3 c11 = getPixel(ix1, iy1);

    // Bilinear interpolation: lerp along X then Y
    Vec3 top = c00.lerp(c10, tx);
    Vec3 bot = c01.lerp(c11, tx);
    return top.lerp(bot, ty);
}

int Texture::getWidth() const { return width; }
int Texture::getHeight() const { return height; }
int Texture::getChannels() const { return channels; }

void Texture::clampUV(double& u, double& v) const
{
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
}

Vec3 Texture::getPixel(int x, int y) const
{
    if (!data || x < 0 || x >= width || y < 0 || y >= height) return Vec3();
    size_t off = (static_cast<size_t>(y) * width + x) * 4;
    return Vec3(data[off + 0] / 255.0, data[off + 1] / 255.0, data[off + 2] / 255.0);
}
