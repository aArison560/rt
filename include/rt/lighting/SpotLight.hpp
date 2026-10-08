#pragma once

// Spotlight cone (*Direct light*, T058, SPEC section 5.2 E) — couche `lighting/`
// (metier du rendu). Le spot est une ponctuelle avec cone : `position`,
// `target`, `angle` (demi-ouverture en degres, schema 1..90). Eclaire `P` ssi
// `P` est dans le cone :
//   spotDir  = normalize(target - position)   (axe, constant par lumiere)
//   cosCut   = cos(radians(angle))             (seuil, constant)
//   toFrag   = normalize(P - position)         (rayon lumineux)
//   visible  = dot(toFrag, spotDir) >= cosCut  (dans le cone)
// Hors cone -> contribution 0 (pas d'eclairage, pas d'ombre portee utile).
// Tout est `noexcept`, sans allocation (R3), sans `throw` (R2) : degenere
// (spotDir nul, `P` confondu, NaN) -> 0 (defini, lumiere ignoree).
// Le `render/` (calque du dessus) precalcule `spotDir`/`cosCut` une fois
// (chemin froid) puis appelle `isInSpotCone()` par pixel (registres, R3).
// Attenuation T051 et ombres T052/T058 s'appliquent comme pour les ponctuelles.

#include "rt/base/Vec.hpp"

namespace rt::lighting {

struct SpotLightParams {
	Vec3 position = Vec3(0.0F, 0.0F, 0.0F);
	Vec3 color = Vec3(1.0F, 1.0F, 1.0F);
	float intensity = 1.0F;
	Vec3 attenuation = Vec3(1.0F, 0.0F, 0.0F);
	float range = 0.0F;
	// Axe normalise vers `target` (`(0,0,0)` si degenere -> ignoree).
	Vec3 spotDir = Vec3(0.0F, 0.0F, 0.0F);
	// Seuil `cos(angle)` (`angle` en degres 1..90 -> radians).
	float cosCutoff = -1.0F;
};

static_assert(sizeof(SpotLightParams) <= 64,
              "SpotLightParams reste compact (R3, copie par lumiere sans alloc)");

// Dans le cone ? `hitPoint` = point ombre en monde. `false` si degenere
// (axe nul, `P` confondu, NaN), hors cone, ou seuil incoherent.
[[nodiscard]] bool isInSpotCone(const SpotLightParams& spot, Vec3 hitPoint) noexcept;

// Construit l'axe + seuil depuis `position`/`target`/`angleDeg` (degres) :
// - `target == position` ou NaN -> axe nul (ignoree) ;
// - `angleDeg` hors 0..90 ou NaN -> seuil degénéré (ignoree, `cosCut > 1`).
// `noexcept`, sans allocation. Appelee une fois par lumiere (chemin froid).
[[nodiscard]] SpotLightParams makeSpotParams(Vec3 position, Vec3 color, float intensity,
                                             Vec3 attenuation, float range, Vec3 target,
                                             float angleDeg) noexcept;

} // namespace rt::lighting
