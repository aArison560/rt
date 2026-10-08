// Lumiere parallele (T055) — implementation sans exception (R2),
// sans allocation (R3). Voir `include/rt/lighting/DirectionalLight.hpp`
// pour le contrat (`L = -normalize(direction)`, pas d'attenuation).

#include "rt/lighting/DirectionalLight.hpp"

#include <cmath>

namespace rt::lighting {

Vec3 toLightDir(Vec3 direction) noexcept {
	if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
	    !std::isfinite(direction.z)) {
		return Vec3{};
	}
	const Vec3 unit = normalize(direction);
	if (nearZero(unit)) {
		return Vec3{};
	}
	return unit * -1.0F;
}

} // namespace rt::lighting
