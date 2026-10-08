#pragma once

// Lumiere ponctuelle et attenuation (T051) — couche `lighting/` (metier du
// rendu). `attenuationFactor()` applique la formule du schema
// (FORMAT_SCENE.md §5.4) : `att = 1 / (c + l*d + q*d^2)`, `range > 0` et
// `d > range` -> 0 (portee max, 0 = infinie). Tout est `noexcept`, sans
// allocation (R3), sans `throw` (R2) : les degeneres (distance NaN/Inf/
// negative, denominateur <= eps, composantes non finies) ont un comportement
// defini (0 ou 1, jamais de NaN/Inf en sortie). Le `render/` (calque du
// dessus) appelle cette fonction puis pondere `intensity` avant
// `shadeLambert()` — `shading/` ne voit pas `lighting/` (regle d'or §2.1).

#include "rt/base/Vec.hpp"

namespace rt::lighting {

// Facteur d'attenuation pour une distance `d` (>= 0, monde) :
// - `d` non finie ou negative -> 0 ;
// - `range > 0` et `d > range` -> 0 ;
// - composantes `attenuation` non finies ou < 0 -> 0 (valeurs validees en
//   T024, mais l'appel programmatique reste defini) ;
// - `denominateur <= 1e-6` (dont `(0,0,0)`) -> 1 (pas d'attenuation, evite
//   la division par zero) ;
// - sinon `1 / denominateur` (peut depasser 1 si `c < 1` ; le `saturate`
//   final du shading borne la couleur, jamais de NaN/Inf).
// `noexcept`, sans allocation.
[[nodiscard]] float attenuationFactor(Vec3 attenuation, float distance, float range) noexcept;

} // namespace rt::lighting
