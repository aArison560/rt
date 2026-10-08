// Interface des objets (T040) — `toString` et documentation du dispatch.
// Vtable retenue (ADR-002) : chaque primitive surcharge `intersect` et
// `localBounds` ; aucun `switch` generique ni macro d'intersection.
// R2 : aucune exception ici ; R3 : aucune allocation.
// T045 (M4, approche A) : helpers monde <-> objet — le rayon monde est
// ramene en objet par `M⁻¹` (`t` conserve car la direction n'est pas
// renormalisee), le point revient par `M`, la normale par `(M⁻¹)ᵀ`.

#include "rt/geometry/Object.hpp"

namespace rt::geometry {

const char* toString(ObjectKind kind) noexcept {
	switch (kind) {
	case ObjectKind::Sphere:
		return "sphere";
	case ObjectKind::Plane:
		return "plane";
	case ObjectKind::Cylinder:
		return "cylinder";
	case ObjectKind::Cone:
		return "cone";
	}
	return "unknown";
}

std::optional<Ray> AObject::worldToObjectRay(const Ray& worldRay) const noexcept {
	const std::optional<Mat4> inv = objectToWorld_.matrix.inverse();
	if (!inv) {
		return std::nullopt;
	}
	const Vec3 origin = transformPoint(*inv, worldRay.origin);
	const Vec3 direction = transformVector(*inv, worldRay.direction);
	return Ray(origin, direction, worldRay.depth);
}

Vec3 AObject::objectToWorldPoint(Vec3 p) const noexcept {
	return objectToWorld_.applyPoint(p);
}

Vec3 AObject::objectToWorldNormal(Vec3 n) const noexcept {
	return objectToWorld_.applyNormal(n);
}

} // namespace rt::geometry
