#pragma once

// Camera de rendu (T031) — base orthonormee + generation de rayons.
// Construite depuis `scene::Camera` (donnees `camera{}` du schema, R1) ou
// depuis position/cible/up/fov explicites. `init()` valide les cas
// degeneres (cible == position, up nul, up colineaire a la visee —
// typiquement camera verticale —, fov hors 1..179, dimensions invalides)
// et renvoie `Status`, jamais `throw` (R2). `rayForPixel()` est le chemin
// chaud : `noexcept`, sans allocation (R3), suppose `init()` reussi.
// Formules : cf. docs/ARCHITECTURE.md §4.1 (Gram-Schmidt, aspect, demi
// hauteurs, u/v en [-1,1], direction normalisee).

#include "rt/base/Ray.hpp"
#include "rt/base/Status.hpp"
#include "rt/base/Vec.hpp"

namespace rt::scene {
struct Camera;
}

namespace rt::render {

class Camera {
  public:
	Camera() = default;

	// Depuis les donnees du schema (`scene.camera`, R1). `width`/`height`
	// 1..8192 (bornes `limits`, R1). Echec -> `Status` (jamais `throw`).
	[[nodiscard]] Status init(const scene::Camera& desc, int width, int height);
	[[nodiscard]] Status init(Vec3 position, Vec3 target, Vec3 up, float fovDeg, int width,
	                         int height);

	// Rayon pour le pixel (x, y) avec decalage sous-pixel `jitter`
	// (0,0) = centre du pixel, jitter en pixels (ex. ±0.5 pour l'AA).
	// Origine = position, direction normalisee, profondeur 0.
	// Precondition : `init()` a reussi (`isValid()`), sinon rayon degenere.
	[[nodiscard]] Ray rayForPixel(int x, int y, Vec2 jitter) const noexcept;
	[[nodiscard]] Ray rayForPixel(int x, int y) const noexcept;

	[[nodiscard]] bool isValid() const noexcept { return valid_; }
	[[nodiscard]] Vec3 position() const noexcept { return position_; }
	[[nodiscard]] Vec3 forward() const noexcept { return forward_; }
	[[nodiscard]] Vec3 right() const noexcept { return right_; }
	[[nodiscard]] Vec3 up() const noexcept { return trueUp_; }
	[[nodiscard]] float fovDeg() const noexcept { return fovDeg_; }
	[[nodiscard]] float aspect() const noexcept { return aspect_; }
	[[nodiscard]] int width() const noexcept { return width_; }
	[[nodiscard]] int height() const noexcept { return height_; }

  private:
	Vec3 position_{};
	Vec3 forward_ = Vec3(0.0F, 0.0F, -1.0F);
	Vec3 right_ = Vec3(1.0F, 0.0F, 0.0F);
	Vec3 trueUp_ = Vec3(0.0F, 1.0F, 0.0F);
	float halfWidth_ = 1.0F;
	float halfHeight_ = 1.0F;
	float fovDeg_ = 60.0F;
	float aspect_ = 1.0F;
	int width_ = 0;
	int height_ = 0;
	bool valid_ = false;
};

} // namespace rt::render
