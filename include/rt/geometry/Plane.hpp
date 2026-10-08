#pragma once

// Plan (T042) — plan infini en espace objet, `noexcept`.
// Defini par un point `P0` et une normale `n` normalisee.
// Intersection `t = dot(P0 - O, n) / dot(D, n)` ; `|dot(D,n)| <= kEpsilon`
// -> miss defini (parallele, jamais de division par zero, UBSan propre).
// Normale via `setFaceNormal` (coherente des deux cotes), `uv` derives des
// axes tangents (`u = dot(P-P0, uAxis)`, `v = dot(P-P0, vAxis)`).
// Degeneres : normale quasi nulle, `tMin > tMax`, `NaN`, `t` hors bornes
// -> `false` defini, jamais d'exception (R2), aucune allocation (R3).
// `localBounds()` = grande boite `±kPlaneExtent` (documentee) car le plan
// est infini ; la BVH (T060+) traitera ce cas a part.
// Espace objet == monde en T042 (transform identite) ; T045 branchera
// `objectToWorld`.

#include "rt/base/Ray.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"

namespace rt::geometry {

class Plane final : public AObject {
public:
	Plane() noexcept;
	Plane(Vec3 point, Vec3 normal, std::uint32_t id = 0,
	      std::uint32_t materialIndex = 0) noexcept;

	[[nodiscard]] Vec3 point() const noexcept { return point_; }
	[[nodiscard]] Vec3 normal() const noexcept { return normal_; }
	void setPoint(Vec3 point) noexcept { point_ = point; }
	void setNormal(Vec3 normal) noexcept;

	bool intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept override;
	AABB localBounds() const noexcept override;

private:
	Vec3 point_ = Vec3(0, 0, 0);
	Vec3 normal_ = Vec3(0, 1, 0);
};

} // namespace rt::geometry
