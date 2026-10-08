// Cone (T044) — intersection analytique sans exception (R2), sans
// allocation (R3). Reecrit pour le calque `geometry/` (aucune copie de la
// v1 : la v1 est un cone fini avec base ; ici cone infini deux nappes
// autour de Y, contrat `bool + HitRecord&`, `Real = float`,
// `setFaceNormal`, `uv` coniques). Intermediaires en `double` pour la
// stabilite (meme discipline que sphere T041 / cylindre T043). Le cas
// limite de l'apex (qui a casse la v1) est defini : candidat ignore,
// jamais de `throw`, jamais de division par zero (UBSan propre).

#include "rt/geometry/Cone.hpp"

#include <cmath>

#include "rt/base/Scalar.hpp"

namespace rt::geometry {

namespace {

// Demi-etendue en Y de la boite d'un cone infini (meme ordre de grandeur
// que plan/cylindre : ±1e6 ; `x/z` suivent l'evasement `k·|y|`).
constexpr Real kConeExtent = Real(1000000);

} // namespace

Cone::Cone() noexcept : AObject(ObjectKind::Cone, 0, 0), apex_(Vec3(0, 0, 0)), halfAngle_(Real(0.34906585F)) {
}

Cone::Cone(Vec3 apex, Real halfAngleRadians, std::uint32_t id, std::uint32_t materialIndex) noexcept
    : AObject(ObjectKind::Cone, id, materialIndex), apex_(apex), halfAngle_(halfAngleRadians) {
}

bool Cone::intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept {
	if (!(tMin <= tMax)) {
		return false;
	}
	if (std::isnan(tMin) || std::isnan(tMax)) {
		return false;
	}
	// Demi-angle strictement dans ]0, pi/2[ : sinon `tan` degenere.
	if (!(halfAngle_ > kEpsilon) || !(halfAngle_ < kPi / Real(2) - kEpsilon)) {
		return false;
	}
	if (std::isnan(halfAngle_) || !std::isfinite(halfAngle_)) {
		return false;
	}
	if (!std::isfinite(apex_.x) || !std::isfinite(apex_.y) || !std::isfinite(apex_.z)) {
		return false;
	}
	if (!std::isfinite(ray.origin.x) || !std::isfinite(ray.origin.y) ||
	    !std::isfinite(ray.origin.z) || !std::isfinite(ray.direction.x) ||
	    !std::isfinite(ray.direction.y) || !std::isfinite(ray.direction.z)) {
		return false;
	}
	const double k = std::tan(static_cast<double>(halfAngle_));
	if (!(k > 0.0) || !std::isfinite(k)) {
		return false;
	}
	const double k2 = k * k;
	const double dx = static_cast<double>(ray.direction.x);
	const double dy = static_cast<double>(ray.direction.y);
	const double dz = static_cast<double>(ray.direction.z);
	// Direction quasi nulle : aucun `t` defini.
	const double dir2 = dx * dx + dy * dy + dz * dz;
	const double eps = static_cast<double>(kEpsilon);
	if (!(dir2 > eps * eps)) {
		return false;
	}
	const double ox = static_cast<double>(ray.origin.x) - static_cast<double>(apex_.x);
	const double oy = static_cast<double>(ray.origin.y) - static_cast<double>(apex_.y);
	const double oz = static_cast<double>(ray.origin.z) - static_cast<double>(apex_.z);
	const double a = dx * dx + dz * dz - k2 * dy * dy;
	const double b = 2.0 * (ox * dx + oz * dz - k2 * oy * dy);
	const double c = ox * ox + oz * oz - k2 * oy * oy;
	const double lo = static_cast<double>(tMin);
	const double hi = static_cast<double>(tMax);
	// Candidats par ordre croissant : jusqu'a 2 racines (quadratique) ou
	// 1 (lineaire si `|a| <= eps`, rayon parallele a une generatrice).
	double first = 0.0;
	double second = 0.0;
	int count = 0;
	if (a > eps || a < -eps) {
		const double discriminant = b * b - 4.0 * a * c;
		if (!(discriminant >= 0.0) || !std::isfinite(discriminant)) {
			return false;
		}
		const double sqrtd = std::sqrt(discriminant);
		const double inv2a = 1.0 / (2.0 * a);
		const double rootSmall = (-b - sqrtd) * inv2a;
		const double rootLarge = (-b + sqrtd) * inv2a;
		if (rootSmall <= rootLarge) {
			first = rootSmall;
			second = rootLarge;
		} else {
			first = rootLarge;
			second = rootSmall;
		}
		count = 2;
	} else {
		// Lineaire `b·t + c = 0` : `|b| <= eps` -> aucun `t` isole.
		if (!(b > eps || b < -eps)) {
			return false;
		}
		const double tSingle = -c / b;
		if (!std::isfinite(tSingle)) {
			return false;
		}
		first = tSingle;
		count = 1;
	}
	for (int i = 0; i < count; ++i) {
		const double tHit = (i == 0) ? first : second;
		if (!(tHit >= lo && tHit <= hi) || !std::isfinite(tHit)) {
			continue;
		}
		const Real t = static_cast<Real>(tHit);
		const Vec3 hitPoint = ray.at(t);
		if (!std::isfinite(hitPoint.x) || !std::isfinite(hitPoint.y) ||
		    !std::isfinite(hitPoint.z)) {
			continue;
		}
		// Sommet : aucune normale definie -> candidat ignore (defini,
		// sans `throw`). C'est le cas limite qui a casse la v1.
		const double px = static_cast<double>(hitPoint.x) - static_cast<double>(apex_.x);
		const double py = static_cast<double>(hitPoint.y) - static_cast<double>(apex_.y);
		const double pz = static_cast<double>(hitPoint.z) - static_cast<double>(apex_.z);
		if (px * px + py * py + pz * pz <= eps * eps) {
			continue;
		}
		// Gradient `G = (2·px, -2·k²·py, 2·pz)`, normalise par securite.
		Vec3 outward = Vec3(static_cast<Real>(px), static_cast<Real>(-k2 * py),
		                    static_cast<Real>(pz));
		if (!std::isfinite(outward.x) || !std::isfinite(outward.y) ||
		    !std::isfinite(outward.z)) {
			continue;
		}
		outward = normalize(outward);
		if (nearZero(outward)) {
			continue;
		}
		// `uv` coniques : `u` = azimut dans [0,1], `v` = hauteur monde.
		const double theta = std::atan2(static_cast<double>(outward.z),
		                                static_cast<double>(outward.x));
		const Real u = static_cast<Real>(1.0 - (theta + static_cast<double>(kPi)) /
		                                           (2.0 * static_cast<double>(kPi)));
		const Real v = hitPoint.y;
		if (!std::isfinite(u) || !std::isfinite(v)) {
			continue;
		}
		rec.t = t;
		rec.point = hitPoint;
		rec.setFaceNormal(ray, outward);
		rec.materialIndex = materialIndex();
		rec.uv = Vec2(u, v);
		return true;
	}
	return false;
}

AABB Cone::localBounds() const noexcept {
	if (!(halfAngle_ > kEpsilon) || !(halfAngle_ < kPi / Real(2) - kEpsilon) ||
	    !std::isfinite(halfAngle_)) {
		return AABB(apex_, apex_).padded(kEpsilon);
	}
	const double k = std::tan(static_cast<double>(halfAngle_));
	if (!(k > 0.0) || !std::isfinite(k)) {
		return AABB(apex_, apex_).padded(kEpsilon);
	}
	// Le cone s'evase avec `|y|` : `x/z` = `apex ± (k·1e6+1)`.
	const double half = k * static_cast<double>(kConeExtent) + 1.0;
	const Real halfReal = static_cast<Real>(half);
	if (!std::isfinite(halfReal)) {
		return AABB(Vec3(-kConeExtent, -kConeExtent, -kConeExtent),
		            Vec3(kConeExtent, kConeExtent, kConeExtent));
	}
	const Vec3 min(apex_.x - halfReal, -kConeExtent, apex_.z - halfReal);
	const Vec3 max(apex_.x + halfReal, kConeExtent, apex_.z + halfReal);
	return AABB(min, max);
}

} // namespace rt::geometry
