#pragma once

// Spot orienté + aveuglement face caméra (T058, *Direct light*).
// Couche `lighting/` (métier du rendu) : aucune dépendance vers le haut
// (règle d'or §2.1). Tout est `noexcept`, sans allocation (R3), sans
// `throw` (R2) : les dégénérés (position confondue à la cible, angle hors
// bornes, vecteurs NaN/nuls) ont un comportement défini (axe nul, facteur 0).

#include "rt/base/Vec.hpp"

namespace rt::lighting {

// Axe du spot : `normalize(target - position)` (direction d'émission).
// Confondus / NaN / quasi nul -> `(0,0,0)` (spot ignoré, défini).
[[nodiscard]] Vec3 spotAxis(Vec3 position, Vec3 target) noexcept;

// Facteur de cône pour un fragment `fragPoint` (0..1) :
// `toFrag = normalize(P - pos)`, `cosAngle = dot(toFrag, axis)`,
// `cutoff = cos(radians(angle))` (demi-ouverture, bornes schéma 1..90).
// `cosAngle < cutoff` -> 0 (hors cône) ; sinon `smoothstep` sur une
// pénombre de 0.02 en cos (bord doux, anti-crénelage) -> 1 au centre.
// `angle` non fini / hors ]0,90] -> 0. `noexcept`, sans allocation.
[[nodiscard]] float spotConeFactor(Vec3 fragPoint, Vec3 position, Vec3 target,
                                   float angleDeg) noexcept;

// Facteur d'aveuglement pour un rayon manqué (0..1) : l'observateur doit
// être dans le cône (`dot(normalize(origin - pos), axis) >= cutoff`, le
// spot éclaire vers l'observateur) ET le rayon doit viser la source
// (`dot(rayDir, normalize(pos - origin)) >= cos(blindRadius)`).
// `blindRadiusDeg` = rayon apparent de la source (défaut 8°, large pour
// une preuve visible en 64×48). Retourne 0 hors conditions, sinon produit
// des deux `smoothstep` (centre = 1, bord = 0). `noexcept`, sans allocation.
[[nodiscard]] float spotBlindingFactor(Vec3 rayOrigin, Vec3 rayDir, Vec3 position, Vec3 target,
                                       float angleDeg, float blindRadiusDeg = 8.0F) noexcept;

} // namespace rt::lighting
