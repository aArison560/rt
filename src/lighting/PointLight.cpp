// Lumiere ponctuelle et attenuation (T051) — implementation sans exception
// (R2), sans allocation (R3). Voir `include/rt/lighting/PointLight.hpp`
// pour le contrat et `docs/FORMAT_SCENE.md` §5.4 pour la formule
// (`1 / (c + l*d + q*d^2)`, `range` 0 = infinie). `double` pour la
// stabilite (grandes distances), `sanitize` des entrees non finies.

#include "rt/lighting/PointLight.hpp"

#include <cmath>

namespace rt::lighting {

float attenuationFactor(Vec3 attenuation, float distance, float range) noexcept {
	if (!std::isfinite(distance) || distance < 0.0F) {
		return 0.0F;
	}
	if (range > 0.0F && distance > range) {
		return 0.0F;
	}
	const float c =
	    (std::isfinite(attenuation.x) && attenuation.x > 0.0F) ? attenuation.x : 0.0F;
	const float l =
	    (std::isfinite(attenuation.y) && attenuation.y > 0.0F) ? attenuation.y : 0.0F;
	const float q =
	    (std::isfinite(attenuation.z) && attenuation.z > 0.0F) ? attenuation.z : 0.0F;
	const double d = static_cast<double>(distance);
	const double denom =
	    static_cast<double>(c) + static_cast<double>(l) * d + static_cast<double>(q) * d * d;
	if (!std::isfinite(denom) || denom <= 1e-6) {
		// `(0,0,0)` ou quasi nul : pas d'attenuation (evite la div0).
		return 1.0F;
	}
	const double att = 1.0 / denom;
	if (!std::isfinite(att) || att < 0.0) {
		return 0.0F;
	}
	return static_cast<float>(att);
}

} // namespace rt::lighting
