// Ecriture d'image (T034) — PNG via libpng + fallback PPM.
// Voir `include/rt/io/ImageWriter.hpp` pour le contrat et
// `docs/OUTILS.md` §5 pour les outils (`file`/`identify`).
// Choix (OUTILS.md §1.1) : PNG via **libpng systeme** (1.6.48, presente,
// CI l'installe, `pkg-config --libs libpng` -> `-lpng16`) plutot que
// `stb_image_write` vendored — `stb` ne compilerait pas en
// `-Wall -Wextra -Werror` sans flags dedies, et libpng est deja la voie
// normale du depot. Sans `<png.h>` a la compilation : fallback PPM
// (documente, jamais de crash). libpng signale par `setjmp` (C, pas
// d'exception, R2) ; les allocations `std::vector` du chemin froid peuvent
// lever `bad_alloc` vers le filet `main` (comme `Scene`, T028).

#include "rt/io/ImageWriter.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

#include "rt/render/Framebuffer.hpp"

#if __has_include(<png.h>)
#include <png.h>
#define RT_HAS_LIBPNG 1
#else
#define RT_HAS_LIBPNG 0
#endif

namespace rt::io {

namespace {

[[nodiscard]] bool hasPpmExtension(const std::string& path) noexcept {
	if (path.size() < 4) {
		return false;
	}
	const char* ext = path.c_str() + path.size() - 4;
	return (ext[0] == '.' || ext[0] == '.') && (ext[1] == 'p' || ext[1] == 'P') &&
	       (ext[2] == 'p' || ext[2] == 'P') && (ext[3] == 'm' || ext[3] == 'M');
}

Status writePpm(const render::Framebuffer& fb, const std::string& path) {
	if (fb.width() <= 0 || fb.height() <= 0 || fb.displayData() == nullptr) {
		return Status::error(StatusCode::InvalidArgument, "bad framebuffer: nothing to write",
		                     __LINE__);
	}
	std::FILE* file = std::fopen(path.c_str(), "wb");
	if (file == nullptr) {
		std::string message("cannot open output file '");
		message.append(path);
		message.append("'");
		return Status::error(StatusCode::IoError, message, __LINE__);
	}
	if (std::fprintf(file, "P6\n%d %d\n255\n", fb.width(), fb.height()) < 0) {
		std::fclose(file);
		return Status::error(StatusCode::IoError, "cannot write ppm header", __LINE__);
	}
	const std::size_t count = fb.pixelCount();
	for (std::size_t i = 0; i < count; ++i) {
		const render::Rgba8 pixel = fb.displayData()[i];
		const unsigned char rgb[3] = {pixel.r, pixel.g, pixel.b};
		if (std::fwrite(rgb, 1, 3, file) != 3) {
			std::fclose(file);
			return Status::error(StatusCode::IoError, "cannot write ppm pixels", __LINE__);
		}
	}
	if (std::fclose(file) != 0) {
		return Status::error(StatusCode::IoError, "cannot close output file", __LINE__);
	}
	return Status::ok();
}

#if RT_HAS_LIBPNG

Status writePng(const render::Framebuffer& fb, const std::string& path) {
	if (fb.width() <= 0 || fb.height() <= 0 || fb.displayData() == nullptr) {
		return Status::error(StatusCode::InvalidArgument, "bad framebuffer: nothing to write",
		                     __LINE__);
	}
	std::FILE* file = std::fopen(path.c_str(), "wb");
	if (file == nullptr) {
		std::string message("cannot open output file '");
		message.append(path);
		message.append("'");
		return Status::error(StatusCode::IoError, message, __LINE__);
	}
	png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pngPtr == nullptr) {
		std::fclose(file);
		return Status::error(StatusCode::IoError, "cannot create png writer", __LINE__);
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (infoPtr == nullptr) {
		png_destroy_write_struct(&pngPtr, nullptr);
		std::fclose(file);
		return Status::error(StatusCode::IoError, "cannot create png info", __LINE__);
	}
	// libpng signale par `longjmp` (C) : aucune exception (R2).
	if (setjmp(png_jmpbuf(pngPtr)) != 0) {
		png_destroy_write_struct(&pngPtr, &infoPtr);
		std::fclose(file);
		return Status::error(StatusCode::IoError, "cannot encode png", __LINE__);
	}
	png_init_io(pngPtr, file);
	png_set_IHDR(pngPtr, infoPtr, static_cast<png_uint_32>(fb.width()),
	             static_cast<png_uint_32>(fb.height()), 8, PNG_COLOR_TYPE_RGB_ALPHA,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(pngPtr, infoPtr);
	// Lignes pointant directement dans `displayData()` (RGBA, aucune copie,
	// allocation froide du tableau de pointeurs uniquement).
	std::vector<png_bytep> rows(static_cast<std::size_t>(fb.height()));
	const render::Rgba8* base = fb.displayData();
	for (int y = 0; y < fb.height(); ++y) {
		rows[static_cast<std::size_t>(y)] =
		    reinterpret_cast<png_bytep>(const_cast<render::Rgba8*>(base) +
		                                static_cast<std::size_t>(y) *
		                                    static_cast<std::size_t>(fb.width()));
	}
	png_write_image(pngPtr, rows.data());
	png_write_end(pngPtr, infoPtr);
	png_destroy_write_struct(&pngPtr, &infoPtr);
	if (std::fclose(file) != 0) {
		return Status::error(StatusCode::IoError, "cannot close output file", __LINE__);
	}
	return Status::ok();
}

#endif

} // namespace

Status writeImage(const render::Framebuffer& fb, const std::string& path) {
	if (path.empty()) {
		return Status::error(StatusCode::InvalidArgument, "bad --out: empty path", __LINE__);
	}
	if (path.size() > 1024) {
		return Status::error(StatusCode::InvalidArgument, "bad --out: path too long", __LINE__);
	}
	if (hasPpmExtension(path)) {
		return writePpm(fb, path);
	}
#if RT_HAS_LIBPNG
	return writePng(fb, path);
#else
	// Fallback documente : sans libpng, meme un `.png` contient du PPM.
	return writePpm(fb, path);
#endif
}

} // namespace rt::io
