// Spot orienté + aveuglement (T058, *Direct light*) — implémentation sans
// exception (R2), sans allocation (R3). Voir
// `include/rt/lighting/SpotLight.hpp` pour le contrat et
// `docs/OPTIONS_GUIDE.md` §3.2 pour la formule (cône + pénombre).
// Formule cône : `toFrag = normalize(P - pos)`, `axis = normalize(target -
// pos)`, `cosAngle = dot(toFrag, axis)`, `cutoff = cos(radians(angle))`.
// Aveuglement : observateur dans le cône ET rayon vers la source
// (double `smoothstep`, centre saturé, bord fondu).

#include "rt/lighting/SpotLight.hpp"

#include <cmath>

#include "rt/base/Scalar.hpp"

namespace rt::lighting {

namespace {

// `smoothstep(e0, e1, x)` borné 0..1, sans allocation. `e0 == e1` -> marche.
[[nodiscard]] float smoothstep(float e0, float e1, float x) noexcept {
	if (!std::isfinite(x) || !std::isfinite(e0) || !std::isfinite(e1)) {
		return 0.0F;
	}
	if (!(e1 > e0)) {
		return x >= e1 ? 1.0F : 0.0F;
	}
	float t = (x - e0) / (e1 - e0);
	if (t <= 0.0F) {
		return 0.0F;
	}
	if (t >= 1.0F) {
		return 1.0F;
	}
	return t * t * (3.0F - 2.0F * t);
}

} // namespace

Vec3 spotAxis(Vec3 position, Vec3 target) noexcept {
	const Vec3 delta = target - position;
	if (!std::isfinite(delta.x) || !std::isfinite(delta.y) || !std::isfinite(delta.z)) {
		return Vec3{};
	}
	const Vec3 unit = normalize(delta);
	if (nearZero(unit)) {
		return Vec3{};
	}
	if (!std::isfinite(unit.x) || !std::isfinite(unit.y) || !std::isfinite(unit.z)) {
		return Vec3{};
	}
	return unit;
}

float spotConeFactor(Vec3 fragPoint, Vec3 position, Vec3 target, float angleDeg) noexcept {
	if (!std::isfinite(angleDeg) || !(angleDeg > 0.0F) || !(angleDeg <= 90.0F)) {
		return 0.0F;
	}
	const Vec3 axis = spotAxis(position, target);
	if (nearZero(axis)) {
		return 0.0F;
	}
	const Vec3 toFrag = fragPoint - position;
	if (!std::isfinite(toFrag.x) || !std::isfinite(toFrag.y) || !std::isfinite(toFrag.z)) {
		return 0.0F;
	}
	const float distSq = dot(toFrag, toFrag);
	if (!std::isfinite(distSq) || distSq <= kEpsilon * kEpsilon) {
		return 0.0F;
	}
	const float dist = std::sqrt(distSq);
	const Vec3 unitFrag = toFrag / dist;
	if (nearZero(unitFrag) || !std::isfinite(unitFrag.x)) {
		return 0.0F;
	}
	float cosAngle = dot(unitFrag, axis);
	if (!std::isfinite(cosAngle)) {
		return 0.0F;
	}
	if (cosAngle > 1.0F) {
		cosAngle = 1.0F;
	} else if (cosAngle < -1.0F) {
		cosAngle = -1.0F;
	}
	const float cutoff = std::cos(static_cast<double>(degreesToRadians(angleDeg)));
	if (!std::isfinite(cutoff)) {
		return 0.0F;
	}
	const auto cutoffF = static_cast<float>(cutoff);
	if (cosAngle < cutoffF) {
		return 0.0F;
	}
	// Pénombre douce (0.02 en cos) : centre = 1, bord = fondu.
	return smoothstep(cutoffF, cutoffF + 0.02F, cosAngle);
}

float spotBlindingFactor(Vec3 rayOrigin, Vec3 rayDir, Vec3 position, Vec3 target, float angleDeg,
                         float blindRadiusDeg) noexcept {
	if (!std::isfinite(angleDeg) || !(angleDeg > 0.0F) || !(angleDeg <= 90.0F)) {
		return 0.0F;
	}
	if (!std::isfinite(blindRadiusDeg) || !(blindRadiusDeg > 0.0F) ||
	    !(blindRadiusDeg <= 45.0F)) {
		return 0.0F;
	}
	const Vec3 axis = spotAxis(position, target);
	if (nearZero(axis)) {
		return 0.0F;
	}
	if (!std::isfinite(rayOrigin.x) || !std::isfinite(rayDir.x)) {
		return 0.0F;
	}
	const Vec3 toOrigin = rayOrigin - position;
	const float distOriginSq = dot(toOrigin, toOrigin);
	if (!std::isfinite(distOriginSq) || distOriginSq <= kEpsilon * kEpsilon) {
		return 0.0F;
	}
	const float distOrigin = std::sqrt(distOriginSq);
	const Vec3 dirToOrigin = toOrigin / distOrigin;
	if (nearZero(dirToOrigin) || !std::isfinite(dirToOrigin.x)) {
		return 0.0F;
	}
	// L'observateur doit être éclairé par le spot (dans son cône).
	float cosToObserver = dot(dirToOrigin, axis);
	if (!std::isfinite(cosToObserver)) {
		return 0.0F;
	}
	const float cutoff =
	    static_cast<float>(std::cos(static_cast<double>(degreesToRadians(angleDeg))));
	if (!std::isfinite(cutoff) || cosToObserver < cutoff) {
		return 0.0F;
	}
	// Le rayon doit viser la source (proche de `pos - origin`).
	const Vec3 toLight = position - rayOrigin;
	const float distLightSq = dot(toLight, toLight);
	if (!std::isfinite(distLightSq) || distLightSq <= kEpsilon * kEpsilon) {
		return 0.0F;
	}
	const float distLight = std::sqrt(distLightSq);
	const Vec3 dirToLight = toLight / distLight;
	const Vec3 unitRay = normalize(rayDir);
	if (nearZero(unitRay) || !std::isfinite(unitRay.x)) {
		return 0.0F;
	}
	float cosView = dot(unitRay, dirToLight);
	if (!std::isfinite(cosView)) {
		return 0.0F;
	}
	if (cosView > 1.0F) {
		cosView = 1.0F;
	}
	const float blindCos = static_cast<float>(
	    std::cos(static_cast<double>(degreesToRadians(blindRadiusDeg))));
	if (!std::isfinite(blindCos) || cosView < blindCos) {
		return 0.0F;
	}
	const float coneW = smoothstep(cutoff, 1.0F, cosToObserver);
	const float viewW = smoothstep(blindCos, 1.0F, cosView);
	const float out = coneW * viewW;
	if (!std::isfinite(out) || !(out > 0.0F)) {
		return 0.0F;
	}
	return out > 1.0F ? 1.0F : out;
}

} // namespace rt::lighting
