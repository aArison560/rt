#pragma once

// Lumiere parallele (T055, *Parallel light*) — couche `lighting/`
// (metier du rendu). `toLightDir()` convertit la `direction` du schema
// (FORMAT_SCENE.md §5.4, sens de propagation soleil -> scene) en `L`
// (vers le soleil, constante pour tous les points) : `L = -normalize(dir)`.
// Pas d'attenuation par distance (OPTIONS_GUIDE.md §3.1) : le `render/`
// utilise `intensity` telle quelle (facteur 1). Tout est `noexcept`, sans
// allocation (R3), sans `throw` (R2) : direction nulle/NaN -> vecteur nul
// (defini, l'appelant ignore la lumiere).

#include "rt/base/Vec.hpp"

namespace rt::lighting {

// Direction vers la source (`L`) depuis la `direction` du fichier :
// - `direction` non finie ou quasi nulle -> `(0,0,0)` (lumiere ignoree) ;
// - sinon `-normalize(direction)` (constante, independante de la position).
// `noexcept`, sans allocation.
[[nodiscard]] Vec3 toLightDir(Vec3 direction) noexcept;

} // namespace rt::lighting
