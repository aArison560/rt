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
	// Attenuation (T051, FORMAT_SCENE.md §5.4) : `(c l q)`, `1/(c+l*d+q*d^2)`,
	// defaut `(1 0 0)` = pas d'attenuation. `range` : portee max, 0 = infinie.
	// Evalues par `lighting::attenuationFactor()` dans `render/` (le shading
	// recoit une intensite deja ponderee, pas de dependance `shading` ->
	// `lighting`, regle d'or §2.1). Valides par le schema (bornes R1, T024).
	Vec3 attenuation = Vec3(1.0F, 0.0F, 0.0F);
	float range = 0.0F;
};

struct DirectionalLightParams {
	// Direction de propagation soleil -> scene (FORMAT §5.4, `direction`) :
	// `L = -normalize(direction)` (constante, `lighting::toLightDir`, T055).
	// Pas d'attenuation par distance (OPTIONS_GUIDE §3.1, facteur 1).
	Vec3 direction = Vec3(0.0F, -1.0F, 0.0F);
	Vec3 color = Vec3(1.0F, 1.0F, 1.0F);
	float intensity = 1.0F;
};

static_assert(std::is_trivially_copyable_v<DirectionalLightParams>,
              "DirectionalLightParams doit rester POD (R3)");

struct SpotLightParams {
	// Spot orienté (T058, *Direct light*, FORMAT §5.4) : `position` (source),
	// `target` (point visé, requis schéma), `angle` (demi-ouverture 1..90°),
	// `color`/`intensity`/`attenuation`/`range` comme la ponctuelle (T051).
	// Le cône est évalué par `lighting::spotConeFactor()` dans `render/` ;
	// l'aveuglement face caméra par `lighting::spotBlindingFactor()` sur les
	// rayons manqués. POD (R3, copie par lumière sans alloc).
	Vec3 position = Vec3(0.0F, 0.0F, 0.0F);
	Vec3 color = Vec3(1.0F, 1.0F, 1.0F);
	float intensity = 1.0F;
	Vec3 target = Vec3(0.0F, 0.0F, 0.0F);
	float angle = 30.0F;
	Vec3 attenuation = Vec3(1.0F, 0.0F, 0.0F);
	float range = 0.0F;
};

static_assert(std::is_trivially_copyable_v<SpotLightParams>,
              "SpotLightParams doit rester POD (R3)");

// Sature chaque canal dans [0,1] (NaN/Inf -> 0). `noexcept`, sans allocation.
[[nodiscard]] Vec3 saturate(Vec3 color) noexcept;

// Lambert avec 1 ponctuelle + ambiante globale. `normal` devrait etre
// normalisee (on la renormalise par securite : nulle -> ambiant seul).
// `hitPoint` = point ombre en monde. Jamais de NaN en sortie.
[[nodiscard]] Vec3 shadeLambert(const MaterialParams& material, Vec3 normal, Vec3 hitPoint,
                                const PointLightParams& light,
                                const AmbientParams& ambient) noexcept;

// Speculaire Blinn-Phong (T053, M7 « petit point blanc ») : `H = norm(L+V)`,
// `spec = pow(max(dot(N,H),0), shininess) * specular * lightColor*intensity`.
// `normal`, `viewDir` (vers l'oeil, `-ray.dir`), `lightDir` (vers la source)
// devraient etre normalises (renormalises par securite : nul -> 0).
// Dos a la lumiere (`NdotL <= 0`), `specular <= 0`, `shininess` degeneree,
// `H` degenere, NaN/Inf -> 0 (defini, jamais de NaN en sortie).
// Le resultat N'EST PAS multiplie par l'albedo : il s'ajoute a la couleur
// de l'objet pour saturer en blanc (SPECIFICATIONS §3.2 d). `noexcept`,
// sans allocation (R2/R3). Choix Blinn-Phong (demi-vecteur) plutot que
// Phong (`reflect`) : plus stable aux incidences rasantes, 1 `pow` par
// lumiere (cout mesure en `docs/BENCH.md`, optimisable en T065).
[[nodiscard]] Vec3 specularTerm(Vec3 normal, Vec3 viewDir, Vec3 lightDir,
                                const MaterialParams& material, Vec3 lightColor,
                                float lightIntensity) noexcept;

// Commodite ponctuelle (T053) : `lightDir` derive de
// `light.position - hitPoint` (confondue/NaN -> 0). `viewDir` vers l'oeil.
// Appliquee par `render/` avec `intensity` deja ponderee par l'attenuation
// T051 (registres, R3). `noexcept`, sans allocation.
[[nodiscard]] Vec3 shadeSpecular(const MaterialParams& material, Vec3 normal, Vec3 viewDir,
                                 Vec3 hitPoint, const PointLightParams& light) noexcept;

// Lambert directionnelle (T055, *Parallel light*) : `L = -norm(direction)`
// constante (independante de la position, pas d'attenuation), memes ombres
// (traitees par `render/` avec `tMax` infini). `direction` nulle/NaN ->
// ambiant seul (defini). `noexcept`, sans allocation (R2/R3).
[[nodiscard]] Vec3 shadeLambertDirectional(const MaterialParams& material, Vec3 normal,
                                           const DirectionalLightParams& light,
                                           const AmbientParams& ambient) noexcept;

// Direction refractee de Descartes/Snell (T057, *Reflection & transparency*
// sous-criteres 3-5) : `n1*sin(theta1) = n2*sin(theta2)` (le correcteur
// cherchera cette formule — voir aussi `src/shading/Material.cpp`).
// `incident` = direction du rayon incident (vers la surface), `normal` = normale
// de shading (contre le rayon, `rec.normal`, unitaire), `frontFace` = vrai si
// le rayon arrive de l'exterieur (air -> objet, `eta = 1/ior`), faux s'il sort
// (objet -> air, `eta = ior`). `ior` sanitize (`!fini` -> 1, `<1` -> 1, `>3` -> 3,
// bornes du schema R1, T024) : `ior = 1` -> `eta = 1` -> aucune deviation
// (`T == I`, DoD). Decompose (base orthonormee, `cos1 = dot(-I, N)`) :
// `rPerp = eta*(I + cos1*N)`, `rPar = -sqrt(1-|rPerp|^2)*N`, `T = rPerp + rPar`
// (via `rt::refract`, qui renvoie le vecteur nul si `|rPerp|^2 > 1`, c'est la
// reflexion totale interne `sin(theta2) > 1`). Nul/NaN (incident/normale
// degeneree, `ior` degenere) -> vecteur nul (sentinelle, jamais de `throw`,
// l'appelant replie sur le miroir ou le direct, R2). `noexcept`, sans
// allocation (R2/R3).
[[nodiscard]] Vec3 refractDir(Vec3 incident, Vec3 normal, bool frontFace, float ior) noexcept;

} // namespace rt::shading
