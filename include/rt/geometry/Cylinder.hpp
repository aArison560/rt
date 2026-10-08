#pragma once

// Cylindre (T043) — cylindre infini autour de l'axe local Y, `noexcept`.
// Defini par un point `center` sur l'axe (seuls `x`/`z` fixent la ligne,
// `y` est un point de passage sans effet sur l'infini) et un rayon `r`.
// Equation `(x-cx)² + (z-cz)² = r²` soit `a·t² + b·t + c = 0` avec
// `a = dx²+dz²`, `b = 2·(ox·dx+oz·dz)`, `c = ox²+oz² - r²`
// (`ox = Ox-cx`, `oz = Oz-cz`). Racine la plus proche dans `[tMin,tMax]`
// (tangente = discriminant nul, interieur = petite racine hors bornes ->
// grande racine, dos = `frontFace` false via `setFaceNormal`).
// Normale radiale `outward = (Px-cx, 0, Pz-cz) / r` normalisee,
// `uv` cylindriques (`u = 1-(atan2(pz,px)+pi)/2pi`, `v = Py`).
// Cas parallele a l'axe (`a <= eps²`) -> miss defini (mur jamais touche,
// meme depuis l'interieur : le rayon longe le fut sans le couper).
// Degeneres : `r <= kEpsilon`, direction quasi nulle, `tMin > tMax`,
// `NaN/Inf` -> `false` defini, jamais d'exception (R2), aucune
// allocation (R3). Espace objet == monde en T043 (transform identite) ;
// T045 branchera `objectToWorld`. La limitation en hauteur sera faite
// en T133 (cylindre borne + caps) ; `localBounds()` = boite
// `x/z` serree (`cx±r`, `cz±r`), `y` = `±kCylinderExtent` documentee.

#include "rt/base/Ray.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"

namespace rt::geometry {

class Cylinder final : public AObject {
public:
	Cylinder() noexcept;
	Cylinder(Vec3 center, Real radius, std::uint32_t id = 0,
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
