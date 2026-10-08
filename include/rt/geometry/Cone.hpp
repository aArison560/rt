#pragma once

// Cone (T044) — cone infini deux nappes autour de l'axe local Y, `noexcept`.
// Defini par le sommet `apex` et le demi-angle au sommet `angle` (radians,
// ]0, pi/2[ ; la scene l'exprime en degres, conversion `degreesToRadians`
// au chargement en T046). Equation `(x-ax)² + (z-az)² = k²·(y-ay)²` avec
// `k = tan(angle)`, soit `a·t² + b·t + c = 0` avec
// `a = dx²+dz²-k²·dy²`, `b = 2·(ox·dx+oz·dz-k²·oy·dy)`,
// `c = ox²+oz²-k²·oy²` (`o = O-apex`). Racines par ordre croissant, la
// premiere dans `[tMin,tMax]` a normale definie gagne (tangente =
// discriminant nul, interieur = petite racine hors bornes -> grande).
// `objectToWorld` (T045, M4, approche A) : rayon monde ramene en objet
// par `M⁻¹` (`t` conserve), point de retour par `M`, normale par
// `(M⁻¹)ᵀ` ; `localBounds()` reste en espace objet.
// `|a| <= eps` -> equation lineaire `b·t+c = 0` (rayon parallele a une
// generatrice : un seul `t` defini, jamais de division par zero).
// Nappe touchee : `Py >= ay` -> haute, sinon basse (signe de `y-ay`).
// Normale = gradient `G = (2·(Px-ax), -2·k²·(Py-ay), 2·(Pz-az))`
// normalise, orientee via `setFaceNormal`. Sommet (`|P-apex| <= eps` ou
// `G` quasi nul) -> candidat ignore (comportement defini sans `throw` :
// le sommet n'a pas de normale ; un rayon visant exactement l'apex est
// un miss sauf autre racine valide). `uv` coniques (`u` = azimut dans
// [0,1], `v` = hauteur objet). Degeneres : `angle` hors ]0,pi/2[,
// `apex` non fini, direction quasi nulle, `tMin > tMax`, `NaN`, `M`
// singuliere -> `false` defini, jamais d'exception (R2), aucune
// allocation (R3). La limitation (troncature + base) sera faite en T133 ;
// `localBounds()` = boite documentee `y = ±1e6`, `x/z` =
// `apex ± (k·1e6+1)` (le cone s'evase avec `|y|`), en espace objet.

#include "rt/base/Ray.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"

namespace rt::geometry {

class Cone final : public AObject {
public:
	Cone() noexcept;
	Cone(Vec3 apex, Real halfAngleRadians, std::uint32_t id = 0,
	     std::uint32_t materialIndex = 0) noexcept;

	[[nodiscard]] Vec3 apex() const noexcept { return apex_; }
	[[nodiscard]] Real halfAngle() const noexcept { return halfAngle_; }
	void setApex(Vec3 apex) noexcept { apex_ = apex; }
	void setHalfAngle(Real halfAngleRadians) noexcept { halfAngle_ = halfAngleRadians; }

	bool intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept override;
	AABB localBounds() const noexcept override;

private:
	Vec3 apex_ = Vec3(0, 0, 0);
	Real halfAngle_ = Real(0.34906585F); // 20 degres (defaut du schema FORMAT_SCENE).
};

} // namespace rt::geometry
