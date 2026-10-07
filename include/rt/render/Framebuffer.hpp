#pragma once

// Framebuffer prealloue et persistant (T030, regles R3/R4).
// Pixels RGBA8 (affichage) + tampon `float` d'accumulation, alloues une
// seule fois a la resolution : `display` = W*H*4 octets, `accum` = W*H*12
// (Vec3 par pixel), `counts` = W*H*4 (un compteur par pixel) soit W*H*20
// au total (cf. docs/MEMORY_STRATEGY.md §4.1). `clear()`, `addSample()` et
// `present()` ne reallouent jamais (R3) : `init()` seul alloue (chemin
// froid, `Status` en cas de dimensions invalides ; `bad_alloc` eventuel
// remonte au filet `main` comme pour `Scene`). `present()` applique le
// tonemapping de base (saturation 0..1, NaN/Inf -> 0) + gamma 2.2 vers
// l'affichage. Aucun `throw` ici (R2) hors allocation `std::vector` du
// chemin froid. Le framebuffer persiste entre les frames (R4, expose T071).

#include <cstddef>
#include <cstdint>
#include <vector>

#include "rt/base/Status.hpp"
#include "rt/base/Vec.hpp"

namespace rt::render {

struct Rgba8 {
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;
	std::uint8_t a = 255;
};

static_assert(sizeof(Rgba8) == 4, "Rgba8 doit tenir sur 4 octets (W*H*4)");

class Framebuffer {
  public:
	Framebuffer() = default;

	// Alloue (ou realloue si la resolution change) : chemin froid uniquement.
	// Dimensions 1..8192 (bornes du schema `limits.width/height`, R1).
	// Meme resolution -> reutilise les vecteurs (aucune realloc) + `clear()`.
	[[nodiscard]] Status init(int width, int height);

	// Remet a zero sans desallouer (capacite conservee, R3).
	void clear() noexcept;

	// Accumule un echantillon HDR dans `accum` (aucune allocation, R3).
	// Hors bornes -> ignore (documente, jamais de crash).
	void addSample(int x, int y, Vec3 color) noexcept;

	// Tonemapping (clamp 0..1) + gamma 2.2 : `accum`/`counts` -> `display`.
	// Aucune allocation (R3). NaN/Inf -> 0 (jamais de NaN dans l'affichage).
	void present() noexcept;

	[[nodiscard]] int width() const noexcept { return width_; }
	[[nodiscard]] int height() const noexcept { return height_; }
	[[nodiscard]] std::size_t pixelCount() const noexcept {
		return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
	}
	[[nodiscard]] std::size_t displaySizeBytes() const noexcept { return pixelCount() * 4U; }
	[[nodiscard]] std::size_t capacityBytes() const noexcept { return pixelCount() * 20U; }

	[[nodiscard]] const Rgba8* displayData() const noexcept { return display_.data(); }
	[[nodiscard]] Rgba8* displayData() noexcept { return display_.data(); }

	// Lecture pour les tests et le futur renderer (hors bornes -> 0).
	[[nodiscard]] Vec3 accumAt(int x, int y) const noexcept;
	[[nodiscard]] int samplesAt(int x, int y) const noexcept;

  private:
	[[nodiscard]] bool inBounds(int x, int y) const noexcept {
		return x >= 0 && y >= 0 && x < width_ && y < height_;
	}
	[[nodiscard]] std::size_t indexOf(int x, int y) const noexcept {
		return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
		       static_cast<std::size_t>(x);
	}

	int width_ = 0;
	int height_ = 0;
	std::vector<Rgba8> display_;
	std::vector<Vec3> accum_;
	std::vector<int> counts_;
};

} // namespace rt::render
