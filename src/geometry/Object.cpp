// Interface des objets (T040) — `toString` et documentation du dispatch.
// Vtable retenue (ADR-002) : chaque primitive surcharge `intersect` et
// `localBounds` ; aucun `switch` generique ni macro d'intersection.
// R2 : aucune exception ici ; R3 : aucune allocation.

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

} // namespace rt::geometry
