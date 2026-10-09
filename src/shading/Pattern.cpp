// Motifs proceduraux (T105) — implementation sans exception ni allocation.
// Voir `include/rt/shading/Pattern.hpp` pour le contrat.
// Damier en UV objet : `floor` gere les negatifs (cases alternees aussi
// sous l'origine), parite sur la somme (soustraction harsh -> `fmod` evite,
// `floor` suffit : `cell` peut etre negative, `% 2` C tronque vers 0 donc
// on utilise `floor` + comparaison de parite via `fmod(cell, 2)` ? Non :
// plus simple et robuste aux negatifs : `((xi + yi) & 1)` sur des entiers
// apres `floor` converti en `long long` (negatifs : `-1 & 1 == 1` en
// complement a 2, alterne correctement). NaN/Inf -> albedo inchange.

#include "rt/shading/Pattern.hpp"

#include <cmath>
#include <cstring>

namespace rt::shading {

PatternKind patternKindFrom(const char* type, bool present) noexcept {
	if (!present) {
		return PatternKind::None;
	}
	if (type == nullptr) {
		return PatternKind::None;
	}
	if (std::strcmp(type, "checker") == 0) {
		return PatternKind::Checker;
	}
	if (std::strcmp(type, "sine") == 0) {
		return PatternKind::Sine;
	}
	if (std::strcmp(type, "perlin") == 0) {
		return PatternKind::Perlin;
	}
	return PatternKind::None;
}

Vec3 checkerAlbedo(Vec3 albedo, Vec2 uv, float scale, float frequency) noexcept {
	if (!std::isfinite(uv.x) || !std::isfinite(uv.y)) {
		return albedo;
	}
	float freq = 1.0F;
	if (std::isfinite(scale) && scale > 0.0F && std::isfinite(frequency) && frequency > 0.0F) {
		freq = scale * frequency;
		if (!std::isfinite(freq) || freq <= 0.0F || freq > 1024.0F) {
			freq = 1.0F;
		}
	}
	const float fu = uv.x * freq;
	const float fv = uv.y * freq;
	if (!std::isfinite(fu) || !std::isfinite(fv)) {
		return albedo;
	}
	const long long xi = static_cast<long long>(std::floor(fu));
	const long long yi = static_cast<long long>(std::floor(fv));
	const bool light = ((xi + yi) & 1LL) == 0LL;
	if (light) {
		return albedo;
	}
	return Vec3(albedo.x * 0.15F, albedo.y * 0.15F, albedo.z * 0.15F);
}

} // namespace rt::shading
