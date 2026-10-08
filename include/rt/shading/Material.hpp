#pragma once

// Shading diffus + ambiante minimale (T033) — Lambert + fond global.
// Modele de materiau complet (T050) : `MaterialParams` porte desormais
// tout le bloc `material` du schema (FORMAT_SCENE.md §5.6) — `albedo`,
// `ambient`, `diffuse`, `specular`, `shininess`, `reflectivity`,
// `transparency`, `ior`, `hasTexture`, `hasPattern` — afin que chaque champ
// soit pilotable depuis le fichier (regle du sujet : rien par recompilation)
// et observable (plumbing `scene::Material` -> `shading::MaterialParams`
// dans `render/`, teste par champ en `tests/unit/test_material.cpp`).
// Les effets visuels des champs avances arrivent avec leur tache :
// speculaire en T053, reflexion en T056, refraction en T057, textures en
// T102, patterns en T105. En attendant, `shadeLambert()` n'utilise que
// `albedo`/`ambient`/`diffuse` (T033) ; les autres champs sont stockes,
// valides par le schema (bornes R1, `ior > 1` si transparence en T024) et
// recopies sans perte — un changement de fichier change les params.
// Couche `shading/` (metier du rendu) : ne voit PAS `scene/` (regle d'or
// §2.1, calques) — elle recoit des params POD copies depuis
// `scene::Material`/`scene::Ambient`/`scene::Light` par `render/`.
// Tout est `noexcept`, sans allocation (R3), sans `throw` (R2) : les cas
// degeneres (normale nulle, lumiere confondue au point, NaN/Inf) ont un
// comportement defini (retour ambiant, saturation [0,1], NaN -> 0).
// La gamma 2.2 est appliquee en sortie par `Framebuffer::present()`
// (T030) ; ce module garantit seulement des couleurs bornees [0,1].
// Formule (ARCHITECTURE.md §4.4, cas T033 : 1 ponctuelle, sans ombre
// ni attenuation — T051 — ni speculaire — T053) :
//   L = normalize(lightPos - P), NdotL = max(dot(N, L), 0)
//   amb = albedo * (ambient.color * ambient.intensity * matAmbient)
//   dif = albedo * matDiffuse * NdotL * (light.color * light.intensity)
//   out = saturate(amb + dif)  (NaN/Inf -> 0, puis clamp 0..1).

#include "rt/base/Vec.hpp"

#include <type_traits>

namespace rt::shading {

struct MaterialParams {
	Vec3 albedo = Vec3(0.8F, 0.8F, 0.8F);
	float ambient = 0.1F;
	float diffuse = 0.7F;
	float specular = 0.5F;
	float shininess = 32.0F;
	float reflectivity = 0.0F;
	float transparency = 0.0F;
	float ior = 1.5F;
	// Presence texture/pattern (T050) : le chemin du fichier vit dans
	// `scene::Material` (chemin froid, `std::string`) ; le shading ne garde
	// que des drapeaux POD pour rester trivialement copiable (R3, pas
	// d'allocation dans le hot path). Echantillonnage en T102/T105.
	bool hasTexture = false;
	bool hasPattern = false;
};

static_assert(std::is_trivially_copyable_v<MaterialParams>,
              "MaterialParams doit rester POD (R3, copie par pixel sans alloc)");

struct AmbientParams {
	Vec3 color = Vec3(0.06F, 0.06F, 0.08F);
	float intensity = 1.0F;
};

struct PointLightParams {
	Vec3 position = Vec3(0.0F, 0.0F, 0.0F);
	Vec3 color = Vec3(1.0F, 1.0F, 1.0F);
	float intensity = 1.0F;
};

// Sature chaque canal dans [0,1] (NaN/Inf -> 0). `noexcept`, sans allocation.
[[nodiscard]] Vec3 saturate(Vec3 color) noexcept;

// Lambert avec 1 ponctuelle + ambiante globale. `normal` devrait etre
// normalisee (on la renormalise par securite : nulle -> ambiant seul).
// `hitPoint` = point ombre en monde. Jamais de NaN en sortie.
[[nodiscard]] Vec3 shadeLambert(const MaterialParams& material, Vec3 normal, Vec3 hitPoint,
                                const PointLightParams& light,
                                const AmbientParams& ambient) noexcept;

} // namespace rt::shading
