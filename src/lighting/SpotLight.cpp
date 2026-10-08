// Spotlight cone (T058, *Direct light*) — implementation sans exception (R2),
// sans allocation (R3). Voir `include/rt/lighting/SpotLight.hpp` pour le contrat.

#include "rt/lighting/SpotLight.hpp"

#include <cmath>

namespace rt::lighting {

bool isInSpotCone(const SpotLightParams& spot, Vec3 hitPoint) noexcept {
	if (nearZero(spot.spotDir)) {
		return false;
	}
	if (!std::isfinite(spot.cosCutoff) || spot.cosCutoff > 1.0F || spot.cosCutoff < -1.0F) {
		return false;
	}
	const Vec3 toFrag = hitPoint - spot.position;
	if (!std::isfinite(toFrag.x) || !std::isfinite(toFrag.y) || !std::isfinite(toFrag.z)) {
		return false;
	}
	const Real distSq = dot(toFrag, toFrag);
	if (!std::isfinite(distSq) || distSq <= kEpsilon * kEpsilon) {
		return false;
	}
	const float dist = static_cast<float>(std::sqrt(static_cast<double>(distSq)));
	if (!std::isfinite(dist) || !(dist > 0.0F)) {
		return false;
	}
	const Vec3 toFragDir = toFrag / dist;
	const float cosAngle = static_cast<float>(dot(toFragDir, spot.spotDir));
	if (!std::isfinite(cosAngle)) {
		return false;
	}
	return cosAngle >= spot.cosCutoff;
}

SpotLightParams makeSpotParams(Vec3 position, Vec3 color, float intensity, Vec3 attenuation,
                               float range, Vec3 target, float angleDeg) noexcept {
	SpotLightParams out;
	out.position = position;
	out.color = color;
	out.intensity = intensity;
	out.attenuation = attenuation;
	out.range = range;
	const Vec3 axis = target - position;
	if (!std::isfinite(axis.x) || !std::isfinite(axis.y) || !std::isfinite(axis.z)) {
		out.spotDir = Vec3{};
		out.cosCutoff = 2.0F;
		return out;
	}
	const Vec3 unitAxis = normalize(axis);
	if (nearZero(unitAxis)) {
		out.spotDir = Vec3{};
		out.cosCutoff = 2.0F;
		return out;
	}
	out.spotDir = unitAxis;
	if (!std::isfinite(angleDeg) || angleDeg <= 0.0F || angleDeg > 90.0F) {
		out.cosCutoff = 2.0F;
		return out;
	}
	const double rad = static_cast<double>(angleDeg) * 3.141592653589793 / 180.0;
	const double cosCut = std::cos(rad);
	if (!std::isfinite(cosCut)) {
		out.cosCutoff = 2.0F;
		return out;
	}
	out.cosCutoff = static_cast<float>(cosCut);
	return out;
}

} // namespace rt::lighting
