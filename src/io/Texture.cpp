// Chargement de textures PNG/JPEG (T102) — implementation sans exception.
// Voir `include/rt/io/Texture.hpp` pour le contrat.
// PNG : libpng (lecture RGBA8, `setjmp` -> `Status`, comme `ImageWriter`).
// JPEG : libjpeg (decompression RGB -> RGBA8, gestionnaire d'erreur
// `setjmp` local pour ne jamais `exit()` sur fichier corrompu, R2).
// Fichier absent -> `IoError` avec chemin complet ; magie inconnue ->
// `IoError` ; `stat` > `maxBytes` -> `LimitExceeded` (T024).

#include "rt/io/Texture.hpp"

#include <csetjmp>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <utility>
#include <vector>

#if __has_include(<png.h>)
#include <png.h>
#define RT_HAS_LIBPNG 1
#else
#define RT_HAS_LIBPNG 0
#endif

#if __has_include(<jpeglib.h>)
#include <jpeglib.h>
#define RT_HAS_LIBJPEG 1
#else
#define RT_HAS_LIBJPEG 0
#endif

namespace rt::io {

namespace {

[[nodiscard]] Result<std::shared_ptr<TextureImage>> failIo(const std::string& detail, int line) {
	return Result<std::shared_ptr<TextureImage>>::fail(
	    Status::error(StatusCode::IoError, detail, line));
}

[[nodiscard]] long long fileSize(const std::string& path, bool& exists) {
	struct stat info {};
	if (::stat(path.c_str(), &info) != 0) {
		exists = false;
		return -1;
	}
	exists = true;
	return static_cast<long long>(info.st_size);
}

#if RT_HAS_LIBPNG
Result<std::shared_ptr<TextureImage>> loadPng(const std::string& path) {
	std::FILE* file = std::fopen(path.c_str(), "rb");
	if (file == nullptr) {
		std::string message("texture not found: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	std::uint8_t sig[8] = {};
	if (std::fread(sig, 1, 8, file) != 8 || png_sig_cmp(sig, 0, 8) != 0) {
		std::fclose(file);
		std::string message("texture not PNG: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png == nullptr) {
		std::fclose(file);
		return failIo("texture: libpng init failed", __LINE__);
	}
	png_infop info = png_create_info_struct(png);
	if (info == nullptr) {
		png_destroy_read_struct(&png, nullptr, nullptr);
		std::fclose(file);
		return failIo("texture: libpng info failed", __LINE__);
	}
	if (setjmp(png_jmpbuf(png)) != 0) {
		png_destroy_read_struct(&png, &info, nullptr);
		std::fclose(file);
		std::string message("texture corrupt PNG: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	png_init_io(png, file);
	png_set_sig_bytes(png, 8);
	png_read_info(png, info);
	const png_uint_32 width = png_get_image_width(png, info);
	const png_uint_32 height = png_get_image_height(png, info);
	const int color = png_get_color_type(png, info);
	const int depth = png_get_bit_depth(png, info);
	if (width == 0 || height == 0 || width > 16384 || height > 16384) {
		png_destroy_read_struct(&png, &info, nullptr);
		std::fclose(file);
		std::string message("texture bad PNG size: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	if (depth == 16) {
		png_set_strip_16(png);
	}
	if (color == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
	}
	if (color == PNG_COLOR_TYPE_GRAY && depth < 8) {
		png_set_expand_gray_1_2_4_to_8(png);
	}
	if (png_get_valid(png, info, PNG_INFO_tRNS) != 0) {
		png_set_tRNS_to_alpha(png);
	}
	if (color == PNG_COLOR_TYPE_RGB || color == PNG_COLOR_TYPE_GRAY ||
	    color == PNG_COLOR_TYPE_PALETTE) {
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}
	if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png);
	}
	png_read_update_info(png, info);
	auto image = std::make_shared<TextureImage>();
	image->width = static_cast<int>(width);
	image->height = static_cast<int>(height);
	image->rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
	std::vector<png_bytep> rows(static_cast<std::size_t>(height));
	for (png_uint_32 y = 0; y < height; ++y) {
		rows[static_cast<std::size_t>(y)] =
		    image->rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4U;
	}
	png_read_image(png, rows.data());
	png_read_end(png, info);
	png_destroy_read_struct(&png, &info, nullptr);
	std::fclose(file);
	return Result<std::shared_ptr<TextureImage>>::ok(std::move(image));
}
#endif

#if RT_HAS_LIBJPEG
struct JpegError {
	struct jpeg_error_mgr base {};
	std::jmp_buf jump {};
};

extern "C" void jpegErrorExit(j_common_ptr info) {
	auto* err = reinterpret_cast<JpegError*>(info->err);
	(*info->err->output_message)(info);
	longjmp(err->jump, 1);
}

Result<std::shared_ptr<TextureImage>> loadJpeg(const std::string& path) {
	std::FILE* file = std::fopen(path.c_str(), "rb");
	if (file == nullptr) {
		std::string message("texture not found: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	struct jpeg_decompress_struct cinfo {};
	JpegError jerr {};
	cinfo.err = jpeg_std_error(&jerr.base);
	jerr.base.error_exit = &jpegErrorExit;
	if (setjmp(jerr.jump) != 0) {
		jpeg_destroy_decompress(&cinfo);
		std::fclose(file);
		std::string message("texture corrupt JPEG: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress(&cinfo);
	const JDIMENSION width = cinfo.output_width;
	const JDIMENSION height = cinfo.output_height;
	if (width == 0 || height == 0 || width > 16384 || height > 16384 ||
	    cinfo.output_components != 3) {
		jpeg_destroy_decompress(&cinfo);
		std::fclose(file);
		std::string message("texture bad JPEG size: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	auto image = std::make_shared<TextureImage>();
	image->width = static_cast<int>(width);
	image->height = static_cast<int>(height);
	image->rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
	std::vector<std::uint8_t> row(static_cast<std::size_t>(width) * 3U);
	while (cinfo.output_scanline < height) {
		JSAMPROW rowPtr = row.data();
		jpeg_read_scanlines(&cinfo, &rowPtr, 1);
		const std::size_t y = static_cast<std::size_t>(cinfo.output_scanline) - 1U;
		std::uint8_t* dst =
		    image->rgba.data() + y * static_cast<std::size_t>(width) * 4U;
		for (JDIMENSION x = 0; x < width; ++x) {
			dst[static_cast<std::size_t>(x) * 4U + 0U] =
			    row[static_cast<std::size_t>(x) * 3U + 0U];
			dst[static_cast<std::size_t>(x) * 4U + 1U] =
			    row[static_cast<std::size_t>(x) * 3U + 1U];
			dst[static_cast<std::size_t>(x) * 4U + 2U] =
			    row[static_cast<std::size_t>(x) * 3U + 2U];
			dst[static_cast<std::size_t>(x) * 4U + 3U] = 0xFF;
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	std::fclose(file);
	return Result<std::shared_ptr<TextureImage>>::ok(std::move(image));
}
#endif

} // namespace

Result<std::shared_ptr<TextureImage>> TextureCache::load(const std::string& path,
                                                         long long maxBytes) {
	if (path.empty()) {
		return Result<std::shared_ptr<TextureImage>>::fail(Status::error(
		    StatusCode::InvalidArgument, "texture: empty file path", __LINE__));
	}
	const auto cached = cache_.find(path);
	if (cached != cache_.end() && cached->second) {
		return Result<std::shared_ptr<TextureImage>>::ok(cached->second);
	}
	bool exists = false;
	const long long size = fileSize(path, exists);
	if (!exists) {
		std::string message("texture not found: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	if (maxBytes > 0 && size > maxBytes) {
		std::string message("texture too large: '");
		message.append(path);
		message.append("'");
		return Result<std::shared_ptr<TextureImage>>::fail(
		    Status::error(StatusCode::LimitExceeded, message, __LINE__));
	}
	std::FILE* probe = std::fopen(path.c_str(), "rb");
	if (probe == nullptr) {
		std::string message("texture not found: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	std::uint8_t magic[8] = {};
	const std::size_t got = std::fread(magic, 1, 8, probe);
	std::fclose(probe);
	if (got < 2) {
		std::string message("texture unreadable: '");
		message.append(path);
		message.append("'");
		return failIo(message, __LINE__);
	}
	Result<std::shared_ptr<TextureImage>> result =
	    Result<std::shared_ptr<TextureImage>>::fail(
	        Status::error(StatusCode::IoError, "texture: unknown format", __LINE__));
	if (got >= 8 && magic[0] == 0x89 && magic[1] == 0x50 && magic[2] == 0x4E &&
	    magic[3] == 0x47) {
#if RT_HAS_LIBPNG
		result = loadPng(path);
#else
		std::string message("texture PNG unsupported (no libpng): '");
		message.append(path);
		message.append("'");
		result = failIo(message, __LINE__);
#endif
	} else if (magic[0] == 0xFF && magic[1] == 0xD8) {
#if RT_HAS_LIBJPEG
		result = loadJpeg(path);
#else
		std::string message("texture JPEG unsupported (no libjpeg): '");
		message.append(path);
		message.append("'");
		result = failIo(message, __LINE__);
#endif
	} else {
		std::string message("texture unknown format (need PNG/JPEG): '");
		message.append(path);
		message.append("'");
		result = failIo(message, __LINE__);
	}
	if (result.isOk()) {
		cache_[path] = result.value();
	}
	return result;
}

void TextureCache::clear() {
	cache_.clear();
}

std::size_t TextureCache::bytes() const noexcept {
	std::size_t total = 0;
	for (const auto& entry : cache_) {
		if (entry.second) {
			total += static_cast<std::size_t>(entry.second->width) *
			         static_cast<std::size_t>(entry.second->height) * 4U;
		}
	}
	return total;
}

Vec3 sampleTexture(const TextureImage& image, float u, float v) noexcept {
	if (image.width <= 0 || image.height <= 0 || image.rgba.empty()) {
		return Vec3{};
	}
	if (!std::isfinite(u) || !std::isfinite(v)) {
		return Vec3{};
	}
	// Pavage : `fract` (les UV du plan sont en unites monde, negatives
	// admises). `u - floor(u)` vaut toujours dans [0,1[.
	float fu = u - std::floor(u);
	float fv = v - std::floor(v);
	if (!std::isfinite(fu) || !std::isfinite(fv)) {
		return Vec3{};
	}
	if (fu < 0.0F) {
		fu = 0.0F;
	} else if (fu >= 1.0F) {
		fu = 0.999999F;
	}
	if (fv < 0.0F) {
		fv = 0.0F;
	} else if (fv >= 1.0F) {
		fv = 0.999999F;
	}
	const auto w = static_cast<std::size_t>(image.width);
	const auto h = static_cast<std::size_t>(image.height);
	std::size_t xi = static_cast<std::size_t>(fu * static_cast<float>(w));
	std::size_t yi = static_cast<std::size_t>(fv * static_cast<float>(h));
	if (xi >= w) {
		xi = w - 1U;
	}
	if (yi >= h) {
		yi = h - 1U;
	}
	// Ligne 0 = haut de l'image (PNG/JPEG) ; `v = 0` = bas des UV
	// (convention FIXME/GL) : on retourne verticalement pour que le haut
	// de la texture apparaisse en haut de la sphere.
	yi = h - 1U - yi;
	const std::size_t idx = (yi * w + xi) * 4U;
	if (idx + 3U >= image.rgba.size()) {
		return Vec3{};
	}
	constexpr float kInv255 = 1.0F / 255.0F;
	return Vec3(static_cast<float>(image.rgba[idx]) * kInv255,
	            static_cast<float>(image.rgba[idx + 1U]) * kInv255,
	            static_cast<float>(image.rgba[idx + 2U]) * kInv255);
}

} // namespace rt::io
