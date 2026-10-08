// Plan (T042) — intersection stable sans exception (R2), sans
// allocation (R3). Reecrit pour le calque `geometry/` (meme equation que
// la v1, contrat `bool + HitRecord&`, `Real = float`, garde `kEpsilon`
// avant toute division pour UBSan).

#include "rt/geometry/Plane.hpp"

#include <cmath>

#include "rt/base/Scalar.hpp"

namespace rt::geometry {

namespace {

// Demi-etendue de la boite d'un plan infini (v1 : ±1e6, repris comme ordre
// de grandeur documente ; la BVH affinera en T060).
constexpr Real kPlaneExtent = Real(1000000);

} // namespace

Plane::Plane() noexcept : AObject(ObjectKind::Plane, 0, 0), point_(Vec3(0, 0, 0)), normal_(Vec3(0, 1, 0)) {}

Plane::Plane(Vec3 point, Vec3 normal, std::uint32_t id, std::uint32_t materialIndex) noexcept
    : AObject(ObjectKind::Plane, id, materialIndex), point_(point), normal_(normalize(normal)) {
	if (nearZero(normal_)) {
		normal_ = Vec3(0, 1, 0);
	}
}

void Plane::setNormal(Vec3 normal) noexcept {
	const Vec3 unit = normalize(normal);
	if (nearZero(unit)) {
		return; // garde l'ancienne normale : plan toujours defini
	}
	normal_ = unit;
}

bool Plane::intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept {
	if (!(tMin <= tMax)) {
		return false;
	}
	if (std::isnan(tMin) || std::isnan(tMax)) {
		return false;
	}
	if (nearZero(normal_)) {
		return false;
	}
	if (std::isnan(ray.origin.x) || std::isnan(ray.origin.y) || std::isnan(ray.origin.z) ||
	    std::isnan(ray.direction.x) || std::isnan(ray.direction.y) || std::isnan(ray.direction.z)) {
		return false;
	}
	if (std::isnan(point_.x) || std::isnan(point_.y) || std::isnan(point_.z)) {
		return false;
	}
	// Denominateur avant division : quasi parallele -> miss defini,
	// aucune division par zero (UBSan propre).
	const Real denom = dot(ray.direction, normal_);
	if (!(denom > kEpsilon || denom < -kEpsilon)) {
		return false;
	}
	const Real numer = dot(point_ - ray.origin, normal_);
	const Real t = numer / denom;
	if (!(t >= tMin && t <= tMax) || std::isnan(t)) {
		return false;
	}
	const Vec3 hitPoint = ray.at(t);
	if (!std::isfinite(hitPoint.x) || !std::isfinite(hitPoint.y) || !std::isfinite(hitPoint.z)) {
		return false;
	}
	// Axes tangents pour `uv` : `helper` non colineaire a `n`,
	// `u` dans le plan, `v = n × u` (orthonorme).
	Vec3 helper = Vec3(0, 1, 0);
	const Real ny = normal_.y < Real(0) ? -normal_.y : normal_.y;
	if (!(ny < Real(0.99))) {
		helper = Vec3(1, 0, 0);
	}
	Vec3 uAxis = cross(normal_, helper);
	if (nearZero(uAxis)) {
		uAxis = Vec3(1, 0, 0);
	} else {
		uAxis = normalize(uAxis);
	}
	const Vec3 vAxis = normalize(cross(normal_, uAxis));
	const Vec3 offset = hitPoint - point_;
	const Real u = dot(offset, uAxis);
	const Real v = dot(offset, vAxis);
	if (!std::isfinite(u) || !std::isfinite(v)) {
		return false;
	}
	rec.t = t;
	rec.point = hitPoint;
	rec.setFaceNormal(ray, normal_);
	rec.materialIndex = materialIndex();
	rec.uv = Vec2(u, v);
	return true;
}

AABB Plane::localBounds() const noexcept {
	return AABB(Vec3(-kPlaneExtent, -kPlaneExtent, -kPlaneExtent),
	            Vec3(kPlaneExtent, kPlaneExtent, kPlaneExtent));
}

} // namespace rt::geometry
