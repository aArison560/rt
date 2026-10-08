// Shading diffus + ambiante minimale (T033) — implementation sans
// exception (R2), sans allocation (R3). Voir
// `include/rt/shading/Material.hpp` pour le contrat et
// `docs/ARCHITECTURE.md` §4.4 pour la formule (Phong sans speculaire :
// ambiant + Lambert, 1 ponctuelle, sans ombre/attenuation — T051/T052).

#include "rt/shading/Material.hpp"

#include <cmath>

namespace rt::shading {

namespace {

[[nodiscard]] float sanitizeChannel(float c) noexcept {
	if (!std::isfinite(c)) {
		return 0.0F;
	}
	if (c <= 0.0F) {
		return 0.0F;
	}
	if (c >= 1.0F) {
		return 1.0F;
	}
	return c;
}

} // namespace

Vec3 saturate(Vec3 color) noexcept {
	return Vec3(sanitizeChannel(color.x), sanitizeChannel(color.y), sanitizeChannel(color.z));
}

Vec3 shadeLambert(const MaterialParams& material, Vec3 normal, Vec3 hitPoint,
                  const PointLightParams& light, const AmbientParams& ambient) noexcept {
	// Ambiant global : albedo * (couleur * intensite * coeff), sature.
	// C'est lui qui garantit "aucun objet n'est jamais totalement noir"
	// (item *Ambiance light*, OPTIONS_GUIDE.md §2.3) meme dos a la lumiere.
	const Vec3 ambientLight =
	    Vec3(sanitizeChannel(ambient.color.x * ambient.intensity),
	         sanitizeChannel(ambient.color.y * ambient.intensity),
	         sanitizeChannel(ambient.color.z * ambient.intensity));
	const float ambientCoeff = sanitizeChannel(material.ambient);
	Vec3 result = Vec3(sanitizeChannel(material.albedo.x) * ambientLight.x * ambientCoeff,
	                   sanitizeChannel(material.albedo.y) * ambientLight.y * ambientCoeff,
	                   sanitizeChannel(material.albedo.z) * ambientLight.z * ambientCoeff);

	// Direction vers la lumiere : degeneree (confondue, NaN) -> ambiant seul.
	const Vec3 toLight = light.position - hitPoint;
	if (!std::isfinite(toLight.x) || !std::isfinite(toLight.y) || !std::isfinite(toLight.z)) {
		return saturate(result);
	}
	const float distSquared = dot(toLight, toLight);
	if (!std::isfinite(distSquared) || distSquared <= kEpsilon * kEpsilon) {
		return saturate(result);
	}
	const Vec3 lightDir = toLight / std::sqrt(distSquared);
	const Vec3 unitNormal = normalize(normal);
	if (nearZero(unitNormal)) {
		return saturate(result);
	}
	float nDotL = dot(unitNormal, lightDir);
	if (!std::isfinite(nDotL) || nDotL <= 0.0F) {
		return saturate(result);
	}
	if (nDotL > 1.0F) {
		nDotL = 1.0F;
	}
	const float diffuseCoeff = sanitizeChannel(material.diffuse);
	const float lightIntensity =
	    std::isfinite(light.intensity) && light.intensity > 0.0F ? light.intensity : 0.0F;
	const Vec3 lightColor = Vec3(sanitizeChannel(light.color.x), sanitizeChannel(light.color.y),
	                             sanitizeChannel(light.color.z));
	const Vec3 diffuse = Vec3(sanitizeChannel(material.albedo.x) * diffuseCoeff * nDotL *
	                              lightColor.x * lightIntensity,
	                          sanitizeChannel(material.albedo.y) * diffuseCoeff * nDotL *
	                              lightColor.y * lightIntensity,
	                          sanitizeChannel(material.albedo.z) * diffuseCoeff * nDotL *
	                              lightColor.z * lightIntensity);
	result += diffuse;
	return saturate(result);
}

} // namespace rt::shading
