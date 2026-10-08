// Cylindre (T043) — intersection analytique sans exception (R2), sans
// allocation (R3). Reecrit pour le calque `geometry/` (aucune copie de la
// v1 : la v1 est un cylindre fini avec caps ; ici cylindre infini autour
// de Y, contrat `bool + HitRecord&`, `Real = float`, `setFaceNormal`,
// `uv` cylindriques). Intermediaires en `double` pour la stabilite.

#include "rt/geometry/Cylinder.hpp"

#include <cmath>

#include "rt/base/Scalar.hpp"

namespace rt::geometry {

namespace {

// Demi-etendue en Y de la boite d'un cylindre infini (meme ordre de
// grandeur que le plan T042 : ±1e6 ; la BVH affinera en T060, la
// limitation en hauteur arrivera en T133).
constexpr Real kCylinderExtent = Real(1000000);

} // namespace

Cylinder::Cylinder() noexcept : AObject(ObjectKind::Cylinder, 0, 0), center_(Vec3(0, 0, 0)), radius_(Real(1)) {
}

Cylinder::Cylinder(Vec3 center, Real radius, std::uint32_t id, std::uint32_t materialIndex) noexcept
    : AObject(ObjectKind::Cylinder, id, materialIndex), center_(center), radius_(radius) {
}

bool Cylinder::intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept {
	if (!(tMin <= tMax)) {
		return false;
	}
	if (std::isnan(tMin) || std::isnan(tMax)) {
		return false;
	}
	if (!(radius_ > kEpsilon) || !std::isfinite(radius_)) {
		return false;
	}
	if (!std::isfinite(center_.x) || !std::isfinite(center_.y) || !std::isfinite(center_.z)) {
		return false;
	}
	if (!std::isfinite(ray.origin.x) || !std::isfinite(ray.origin.y) ||
	    !std::isfinite(ray.origin.z) || !std::isfinite(ray.direction.x) ||
	    !std::isfinite(ray.direction.y) || !std::isfinite(ray.direction.z)) {
		return false;
	}
	// Intermediaires en double : stables pour les cylindres loin de
	// l'origine (meme discipline que la sphere T041).
	const double dx = static_cast<double>(ray.direction.x);
	const double dz = static_cast<double>(ray.direction.z);
	const double ox = static_cast<double>(ray.origin.x) - static_cast<double>(center_.x);
	const double oz = static_cast<double>(ray.origin.z) - static_cast<double>(center_.z);
	const double radius = static_cast<double>(radius_);
	// Composante perpendiculaire a l'axe Y : si quasi nulle, le rayon est
	// parallele a l'axe (ou nul) -> aucun `t` de mur defini.
	const double a = dx * dx + dz * dz;
	const double eps = static_cast<double>(kEpsilon);
	if (!(a > eps * eps)) {
		return false;
	}
	const double b = 2.0 * (ox * dx + oz * dz);
	const double c = ox * ox + oz * oz - radius * radius;
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
	// Normale radiale `(Px-cx, 0, Pz-cz) / r`, renormalisee par securite.
	Vec3 outward = Vec3(hitPoint.x - center_.x, Real(0), hitPoint.z - center_.z) / radius_;
	if (!std::isfinite(outward.x) || !std::isfinite(outward.y) || !std::isfinite(outward.z)) {
		return false;
	}
	outward = normalize(outward);
	if (nearZero(outward)) {
		return false;
	}
	// `uv` cylindriques pour la texture (T103 les exploitera) :
	// `u` = azimut normalise dans [0,1], `v` = hauteur monde.
	const double px = static_cast<double>(outward.x);
	const double pz = static_cast<double>(outward.z);
	const double theta = std::atan2(pz, px);
	const Real u = static_cast<Real>(1.0 - (theta + static_cast<double>(kPi)) /
	                                           (2.0 * static_cast<double>(kPi)));
	const Real v = hitPoint.y;
	if (!std::isfinite(u) || !std::isfinite(v)) {
		return false;
	}
	rec.t = t;
	rec.point = hitPoint;
	rec.setFaceNormal(ray, outward);
	rec.materialIndex = materialIndex();
	rec.uv = Vec2(u, v);
	return true;
}

AABB Cylinder::localBounds() const noexcept {
	if (!(radius_ > kEpsilon) || !std::isfinite(radius_)) {
		return AABB(center_, center_).padded(kEpsilon);
	}
	const Vec3 min(center_.x - radius_, -kCylinderExtent, center_.z - radius_);
	const Vec3 max(center_.x + radius_, kCylinderExtent, center_.z + radius_);
	return AABB(min, max);
}

} // namespace rt::geometry
