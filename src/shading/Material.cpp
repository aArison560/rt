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

Vec3 specularTerm(Vec3 normal, Vec3 viewDir, Vec3 lightDir, const MaterialParams& material,
                   Vec3 lightColor, float lightIntensity) noexcept {
	// Coeffs sanitizes (bornes schema R1 : specular 0..1, shininess 1..1024).
	float specCoeff = 0.0F;
	if (std::isfinite(material.specular) && material.specular > 0.0F) {
		specCoeff = material.specular > 1.0F ? 1.0F : material.specular;
	}
	if (!(specCoeff > 0.0F)) {
		return Vec3{};
	}
	float shininess = 0.0F;
	if (std::isfinite(material.shininess) && material.shininess > 0.0F) {
		shininess = material.shininess;
		if (shininess < 1.0F) {
			shininess = 1.0F;
		} else if (shininess > 1024.0F) {
			shininess = 1024.0F;
		}
	} else {
		return Vec3{};
	}
	float intensity = 0.0F;
	if (std::isfinite(lightIntensity) && lightIntensity > 0.0F) {
		intensity = lightIntensity;
	} else {
		return Vec3{};
	}
	const Vec3 unitN = normalize(normal);
	const Vec3 unitV = normalize(viewDir);
	const Vec3 unitL = normalize(lightDir);
	if (nearZero(unitN) || nearZero(unitV) || nearZero(unitL)) {
		return Vec3{};
	}
	if (!std::isfinite(unitN.x) || !std::isfinite(unitV.x) || !std::isfinite(unitL.x)) {
		return Vec3{};
	}
	float nDotL = dot(unitN, unitL);
	if (!std::isfinite(nDotL) || nDotL <= 0.0F) {
		// Dos a la lumiere : pas de reflet (evite le halo arriere).
		return Vec3{};
	}
	const Vec3 halfVec = unitL + unitV;
	if (nearZero(halfVec)) {
		return Vec3{};
	}
	const Vec3 unitH = normalize(halfVec);
	if (nearZero(unitH)) {
		return Vec3{};
	}
	float nDotH = dot(unitN, unitH);
	if (!std::isfinite(nDotH) || nDotH <= 0.0F) {
		return Vec3{};
	}
	if (nDotH > 1.0F) {
		nDotH = 1.0F;
	}
	const double specPow = std::pow(static_cast<double>(nDotH), static_cast<double>(shininess));
	if (!std::isfinite(specPow) || specPow <= 0.0) {
		return Vec3{};
	}
	const float spec = static_cast<float>(specPow) * specCoeff * intensity;
	if (!std::isfinite(spec) || !(spec > 0.0F)) {
		return Vec3{};
	}
	const Vec3 cleanColor = Vec3(sanitizeChannel(lightColor.x), sanitizeChannel(lightColor.y),
	                             sanitizeChannel(lightColor.z));
	return Vec3(cleanColor.x * spec, cleanColor.y * spec, cleanColor.z * spec);
}

Vec3 shadeSpecular(const MaterialParams& material, Vec3 normal, Vec3 viewDir, Vec3 hitPoint,
                   const PointLightParams& light) noexcept {
	const Vec3 toLight = light.position - hitPoint;
	if (!std::isfinite(toLight.x) || !std::isfinite(toLight.y) || !std::isfinite(toLight.z)) {
		return Vec3{};
	}
	const float distSq = dot(toLight, toLight);
	if (!std::isfinite(distSq) || distSq <= kEpsilon * kEpsilon) {
		return Vec3{};
	}
	float intensity = 0.0F;
	if (std::isfinite(light.intensity) && light.intensity > 0.0F) {
		intensity = light.intensity;
	} else {
		return Vec3{};
	}
	const Vec3 lightColor = Vec3(light.color.x, light.color.y, light.color.z);
	// `normalize(toLight)` : `toLight` non nul ici (garde ci-dessus).
	const float dist = std::sqrt(distSq);
	const Vec3 lightDir = toLight / dist;
	return specularTerm(normal, viewDir, lightDir, material, lightColor, intensity);
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

Vec3 shadeLambertDirectional(const MaterialParams& material, Vec3 normal,
                              const DirectionalLightParams& light,
                              const AmbientParams& ambient) noexcept {
	// Meme ambiant que `shadeLambert()` (plancher > 0, T054).
	const Vec3 ambientLight =
	    Vec3(sanitizeChannel(ambient.color.x * ambient.intensity),
	         sanitizeChannel(ambient.color.y * ambient.intensity),
	         sanitizeChannel(ambient.color.z * ambient.intensity));
	const float ambientCoeff = sanitizeChannel(material.ambient);
	Vec3 result = Vec3(sanitizeChannel(material.albedo.x) * ambientLight.x * ambientCoeff,
	                   sanitizeChannel(material.albedo.y) * ambientLight.y * ambientCoeff,
	                   sanitizeChannel(material.albedo.z) * ambientLight.z * ambientCoeff);
	// `L` constante (T055) : `-normalize(direction)`, nulle/NaN -> ambiant seul.
	if (!std::isfinite(light.direction.x) || !std::isfinite(light.direction.y) ||
	    !std::isfinite(light.direction.z)) {
		return saturate(result);
	}
	const Vec3 negDir = light.direction * -1.0F;
	const Vec3 lightDir = normalize(negDir);
	if (nearZero(lightDir)) {
		return saturate(result);
	}
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
	if (!(lightIntensity > 0.0F)) {
		return saturate(result);
	}
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

Vec3 refractDir(Vec3 incident, Vec3 normal, bool frontFace, float ior) noexcept {
	// Loi de Snell-Descartes (T057, SPECIFICATIONS §5.2 F — formule cherchee
	// par le correcteur) : `n1 * sin(theta1) = n2 * sin(theta2)` avec
	// `eta = n1 / n2`. Air (n=1) dehors, verre (n=ior) dedans :
	// entree (`frontFace`, air -> objet) : `eta = 1 / ior` (devie vers la
	// normale) ; sortie (`!frontFace`, objet -> air) : `eta = ior / 1`
	// (devie loin de la normale, courbure exterieure). `ior = 1` -> `eta = 1`
	// dans les deux sens -> aucune deviation (`T == I`, DoD).
	// Decomposition (I, N unitaires, `cos1 = dot(-I, N)`) :
	// `rPerp = eta * (I + cos1 * N)` (composante tangentielle),
	// `rPar = -sqrt(1 - |rPerp|^2) * N` (composante normale),
	// `T = rPerp + rPar`. Reflexion totale interne quand `|rPerp|^2 > 1`
	// (`sin(theta2) > 1`, sortie rasante) -> `rt::refract` renvoie le vecteur
	// nul (sentinelle, R2) et l'appelant (`render::traceRay`) replie sur le
	// miroir (100 % reflechi, Fresnel = 1). Degeneres -> nul (defini).
	float cleanIor = 1.0F;
	if (std::isfinite(ior)) {
		if (ior < 1.0F) {
			cleanIor = 1.0F;
		} else if (ior > 3.0F) {
			cleanIor = 3.0F;
		} else {
			cleanIor = ior;
		}
	}
	if (!std::isfinite(incident.x) || !std::isfinite(incident.y) || !std::isfinite(incident.z) ||
	    !std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
		return Vec3{};
	}
	const Vec3 unitI = normalize(incident);
	const Vec3 unitN = normalize(normal);
	if (nearZero(unitI) || nearZero(unitN)) {
		return Vec3{};
	}
	if (!std::isfinite(unitI.x) || !std::isfinite(unitN.x)) {
		return Vec3{};
	}
	// `eta = n1/n2` selon le sens (entree/sortie, cf. `HitRecord::frontFace`).
	const float eta = frontFace ? (1.0F / cleanIor) : cleanIor;
	if (!std::isfinite(eta) || !(eta > 0.0F)) {
		return Vec3{};
	}
	// `rt::refract` applique exactement `rPerp`/`rPar` ci-dessus (Vec.hpp) et
	// renvoie `(0,0,0)` en reflexion totale interne (jamais de `throw`, R2).
	return refract(unitI, unitN, eta);
}

} // namespace rt::shading
