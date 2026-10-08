#pragma once

// Sphere (T041) — primitive analytique en espace objet, `noexcept`.
// Equation `|O + t·D - C|² = r²` soit `a·t² + b·t + c = 0` avec
// `a = D·D`, `b = 2·OC·D`, `c = OC·OC - r²` (`OC = O - C`).
// Racine la plus proche dans `[tMin, tMax]` (tangente = discriminant nul,
// interieur = petite racine hors bornes -> grande racine, dos = `frontFace`
// false via `setFaceNormal`). Normale `outward = (P - C) / r` normalisee,
// `uv` spheriques (`theta = atan2(p.z, p.x)`, `phi = acos(p.y)`).
// Degeneres : `r <= kEpsilon`, direction quasi nulle (`a <= eps²`),
// `tMin > tMax`, `NaN/Inf`, `M` singuliere -> `false` defini, jamais
// d'exception (R2), aucune allocation (R3). `objectToWorld` (T045, M4,
// approche A) : rayon monde ramene en objet par `M⁻¹` (`t` conserve),
// point de retour par `M`, normale par `(M⁻¹)ᵀ`.

#include "rt/base/Ray.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"

namespace rt::geometry {

class Sphere final : public AObject {
public:
	Sphere() noexcept;
	Sphere(Vec3 center, Real radius, std::uint32_t id = 0,
	       std::uint32_t materialIndex = 0) noexcept;

	[[nodiscard]] Vec3 center() const noexcept { return center_; }
	[[nodiscard]] Real radius() const noexcept { return radius_; }
	void setCenter(Vec3 center) noexcept { center_ = center; }
	void setRadius(Real radius) noexcept { radius_ = radius; }

	bool intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept override;
	AABB localBounds() const noexcept override;

private:
	Vec3 center_ = Vec3(0, 0, 0);
	Real radius_ = Real(1);
};

} // namespace rt::geometry
