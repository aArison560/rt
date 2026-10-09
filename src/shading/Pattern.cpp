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

namespace {

// `splitmix64` (deterministe, sans etat global) pour seeder le melange.
[[nodiscard]] unsigned long long splitmixNext(unsigned long long& state) noexcept {
	unsigned long long z = (state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31U);
}

[[nodiscard]] float fade(float t) noexcept {
	return t * t * t * (t * (t * 6.0F - 15.0F) + 10.0F);
}

[[nodiscard]] float gradValue(unsigned char hash, float x, float y, float z) noexcept {
	// 12 gradients de Perlin ameliore (p. ex. `hh & 15`).
	const int h = static_cast<int>(hash & 15U);
	const float u = h < 8 ? x : y;
	const float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

} // namespace

void Perlin::init(unsigned long long seed) noexcept {
	unsigned char base[256];
	for (int i = 0; i < 256; ++i) {
		base[i] = static_cast<unsigned char>(i);
	}
	unsigned long long state = seed + 0x9E3779B97F4A7C15ULL;
	for (int i = 255; i > 0; --i) {
		const unsigned long long r = splitmixNext(state);
		const int j = static_cast<int>(r % static_cast<unsigned long long>(i + 1));
		const unsigned char tmp = base[i];
		base[i] = base[j];
		base[j] = tmp;
	}
	for (int i = 0; i < 512; ++i) {
		perm[i] = base[i & 255];
	}
}

float perlinValue(const Perlin& perlin, Vec3 p) noexcept {
	if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
		return 0.0F;
	}
	const int xi = static_cast<int>(std::floor(p.x)) & 255;
	const int yi = static_cast<int>(std::floor(p.y)) & 255;
	const int zi = static_cast<int>(std::floor(p.z)) & 255;
	const float xf = p.x - std::floor(p.x);
	const float yf = p.y - std::floor(p.y);
	const float zf = p.z - std::floor(p.z);
	const float u = fade(xf);
	const float v = fade(yf);
	const float w = fade(zf);
	const unsigned char aa = perlin.perm[perlin.perm[perlin.perm[xi] + yi] + zi];
	const unsigned char ab = perlin.perm[perlin.perm[perlin.perm[xi] + yi + 1] + zi];
	const unsigned char ba = perlin.perm[perlin.perm[perlin.perm[xi + 1] + yi] + zi];
	const unsigned char bb = perlin.perm[perlin.perm[perlin.perm[xi + 1] + yi + 1] + zi];
	const unsigned char aa1 = perlin.perm[perlin.perm[perlin.perm[xi] + yi] + zi + 1];
	const unsigned char ab1 = perlin.perm[perlin.perm[perlin.perm[xi] + yi + 1] + zi + 1];
	const unsigned char ba1 = perlin.perm[perlin.perm[perlin.perm[xi + 1] + yi] + zi + 1];
	const unsigned char bb1 =
	    perlin.perm[perlin.perm[perlin.perm[xi + 1] + yi + 1] + zi + 1];
	auto lerp = [](float a, float b, float t) noexcept { return a + t * (b - a); };
	const float x1 = lerp(gradValue(aa, xf, yf, zf), gradValue(ba, xf - 1.0F, yf, zf), u);
	const float x2 = lerp(gradValue(ab, xf, yf - 1.0F, zf),
	                      gradValue(bb, xf - 1.0F, yf - 1.0F, zf), u);
	const float y1 = lerp(x1, x2, v);
	const float x3 = lerp(gradValue(aa1, xf, yf, zf - 1.0F),
	                      gradValue(ba1, xf - 1.0F, yf, zf - 1.0F), u);
	const float x4 = lerp(gradValue(ab1, xf, yf - 1.0F, zf - 1.0F),
	                      gradValue(bb1, xf - 1.0F, yf - 1.0F, zf - 1.0F), u);
	const float y2 = lerp(x3, x4, v);
	const float result = lerp(y1, y2, w);
	if (!std::isfinite(result)) {
		return 0.0F;
	}
	if (result < -1.0F) {
		return -1.0F;
	}
	if (result > 1.0F) {
		return 1.0F;
	}
	return result;
}

float perlinFractal(const Perlin& perlin, Vec3 p, int octaves) noexcept {
	int oc = octaves;
	if (oc <= 0) {
		oc = 1;
	} else if (oc > 3) {
		oc = 3;
	}
	float total = 0.0F;
	float amplitude = 0.5F;
	float frequency = 1.0F;
	float norm = 0.0F;
	for (int i = 0; i < oc; ++i) {
		const Vec3 q(p.x * frequency, p.y * frequency, p.z * frequency);
		total += perlinValue(perlin, q) * amplitude;
		norm += amplitude;
		amplitude *= 0.5F;
		frequency *= 2.0F;
	}
	if (!(norm > 0.0F) || !std::isfinite(total)) {
		return 0.0F;
	}
	const float result = total / norm;
	if (!std::isfinite(result)) {
		return 0.0F;
	}
	if (result < -1.0F) {
		return -1.0F;
	}
	if (result > 1.0F) {
		return 1.0F;
	}
	return result;
}

Vec3 perlinAlbedo(Vec3 albedo, Vec3 point, const Perlin& perlin, float scale,
                  float frequency) noexcept {
	float freq = 1.0F;
	if (std::isfinite(scale) && scale > 0.0F && std::isfinite(frequency) && frequency > 0.0F) {
		freq = scale * frequency;
		if (!std::isfinite(freq) || freq <= 0.0F || freq > 64.0F) {
			freq = 1.0F;
		}
	}
	const Vec3 q(point.x * freq, point.y * freq, point.z * freq);
	const float n = perlinFractal(perlin, q, 3);
	const float k = 0.5F + 0.5F * n;
	const float factor = 0.35F + 0.65F * k;
	if (!std::isfinite(factor)) {
		return albedo;
	}
	return Vec3(albedo.x * factor, albedo.y * factor, albedo.z * factor);
}

Vec3 waveNormal(Vec3 normal, Vec3 point, float amplitude, float frequency) noexcept {
	if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
		return normal;
	}
	if (nearZero(normal)) {
		return normal;
	}
	float amp = 0.0F;
	if (std::isfinite(amplitude) && amplitude > 0.0F) {
		amp = amplitude > 1.0F ? 1.0F : amplitude;
	} else {
		return normal;
	}
	float freq = 1.0F;
	if (std::isfinite(frequency) && frequency > 0.0F) {
		freq = frequency > 64.0F ? 64.0F : frequency;
	}
	if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
		return normal;
	}
	const Vec3 wave(std::sin(freq * point.y), std::sin(freq * point.z),
	                std::sin(freq * point.x));
	const Vec3 perturbed(normal.x + amp * wave.x, normal.y + amp * wave.y,
	                     normal.z + amp * wave.z);
	if (!std::isfinite(perturbed.x) || nearZero(perturbed)) {
		return normal;
	}
	return normalize(perturbed);
}

} // namespace rt::shading
