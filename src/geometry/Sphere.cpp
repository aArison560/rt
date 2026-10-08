// Sphere (T041) — intersection analytique sans exception (R2),
// sans allocation (R3). Reecrit pour le calque `geometry/` (aucune copie
// de la v1 : meme equation quadratique, contrat `bool + HitRecord&`,
// `Real = float`, `setFaceNormal`, `uv` spheriques).

#include "rt/geometry/Sphere.hpp"

#include <cmath>

#include "rt/base/Scalar.hpp"

namespace rt::geometry {

Sphere::Sphere() noexcept : AObject(ObjectKind::Sphere, 0, 0), center_(Vec3(0, 0, 0)), radius_(Real(1)) {}

Sphere::Sphere(Vec3 center, Real radius, std::uint32_t id, std::uint32_t materialIndex) noexcept
    : AObject(ObjectKind::Sphere, id, materialIndex), center_(center), radius_(radius) {}

bool Sphere::intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept {
	if (!(tMin <= tMax)) {
		return false;
	}
	if (std::isnan(tMin) || std::isnan(tMax)) {
		return false;
	}
	if (radius_ <= kEpsilon) {
		return false;
	}
	if (!std::isfinite(center_.x) || !std::isfinite(center_.y) || !std::isfinite(center_.z) ||
	    !std::isfinite(radius_)) {
		return false;
	}
	if (!std::isfinite(ray.origin.x) || !std::isfinite(ray.origin.y) ||
	    !std::isfinite(ray.origin.z) || !std::isfinite(ray.direction.x) ||
	    !std::isfinite(ray.direction.y) || !std::isfinite(ray.direction.z)) {
		return false;
	}
	// Intermediaires en double : stables pour les spheres tres loin
	// (t ~ 1e6, `a·t²` ~ 1e12 tient en float mais perd en precision).
	const double dx = static_cast<double>(ray.direction.x);
	const double dy = static_cast<double>(ray.direction.y);
	const double dz = static_cast<double>(ray.direction.z);
	const double ox = static_cast<double>(ray.origin.x) - static_cast<double>(center_.x);
	const double oy = static_cast<double>(ray.origin.y) - static_cast<double>(center_.y);
	const double oz = static_cast<double>(ray.origin.z) - static_cast<double>(center_.z);
	const double radius = static_cast<double>(radius_);
	const double a = dx * dx + dy * dy + dz * dz;
	if (!(a > static_cast<double>(kEpsilon) * static_cast<double>(kEpsilon))) {
		return false; // direction quasi nulle : aucun `t` defini
	}
	const double b = 2.0 * (ox * dx + oy * dy + oz * dz);
	const double c = ox * ox + oy * oy + oz * oz - radius * radius;
	const double discriminant = b * b - 4.0 * a * c;
	if (!(discriminant >= 0.0) || !std::isfinite(discriminant)) {
		return false;
	}
	const double sqrtd = std::sqrt(discriminant);
	const double inv2a = 1.0 / (2.0 * a);
	const double rootSmall = (-b - sqrtd) * inv2a;
	const double rootLarge = (-b + sqrtd) * inv2a;
	const double lo = static_cast<double>(tMin);
	const double hi = static_cast<double>(tMax);
	double tHit = rootSmall;
	if (!(tHit >= lo && tHit <= hi)) {
		tHit = rootLarge;
		if (!(tHit >= lo && tHit <= hi)) {
			return false;
		}
	}
	if (!std::isfinite(tHit)) {
		return false;
	}
	const Real t = static_cast<Real>(tHit);
	const Vec3 hitPoint = ray.at(t);
	if (!std::isfinite(hitPoint.x) || !std::isfinite(hitPoint.y) || !std::isfinite(hitPoint.z)) {
		return false;
	}
	// Normale sortante `(P - C) / r`, renormalisee par securite.
	Vec3 outward = (hitPoint - center_) / radius_;
	if (!std::isfinite(outward.x) || !std::isfinite(outward.y) || !std::isfinite(outward.z)) {
		return false;
	}
	outward = normalize(outward);
	if (nearZero(outward)) {
		return false;
	}
	// `uv` spheriques pour la texture (T103 les exploitera) : `p` unitaire.
	const double px = static_cast<double>(outward.x);
	const double py = static_cast<double>(outward.y);
	const double pz = static_cast<double>(outward.z);
	const double clampedY = py < -1.0 ? -1.0 : (py > 1.0 ? 1.0 : py);
	const double theta = std::atan2(pz, px);
	const double phi = std::acos(clampedY);
	const Real u = static_cast<Real>(1.0 - (theta + static_cast<double>(kPi)) /
	                                           (2.0 * static_cast<double>(kPi)));
	const Real v = static_cast<Real>(1.0 - phi / static_cast<double>(kPi));
	rec.t = t;
	rec.point = hitPoint;
	rec.setFaceNormal(ray, outward);
	rec.materialIndex = materialIndex();
	rec.uv = Vec2(u, v);
	return true;
}

AABB Sphere::localBounds() const noexcept {
	if (!(radius_ > kEpsilon) || !std::isfinite(radius_)) {
		return AABB(center_, center_).padded(kEpsilon);
	}
	const Vec3 extent(radius_, radius_, radius_);
	return AABB(center_ - extent, center_ + extent);
}

} // namespace rt::geometry
