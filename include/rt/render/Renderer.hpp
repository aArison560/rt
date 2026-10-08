#pragma once

// Boucle de rendu mono-thread (T032) — sans SDL (regle R6).
// `render()` parcourt les pixels, genere le rayon via `Camera` (T031),
// cherche l'intersection la plus proche (P4, T040+ : pour l'instant
// aucune primitive n'existe donc tout rayon manque et retourne le fond
// de scene), ecrit dans le `Framebuffer` persistant (T030) puis
// `present()` (tonemapping + gamma 2.2). Profondeur max bornee
// (`maxDepth`, utilisee par la reflexion en T056), aucune allocation
// dans la boucle (registres uniquement, regle R3), aucun `throw`
// (regle R2 : `Status` en cas de parametres/camera/framebuffer invalides).
// Deterministe : memes `spp` + meme `seed` -> memes pixels (T036
// etendra au jitter par `seedFor`, la graine est deja validee ici).

#include "rt/base/Status.hpp"

namespace rt::scene {
struct Scene;
}

namespace rt::render {

class Framebuffer;

struct RenderParams {
	int width = 640;
	int height = 480;
	int spp = 4;
	int maxDepth = 4;
	long long seed = 0;
};

// Rend `scene` dans `fb` (reallouee une seule fois si la resolution
// change, chemin froid). `fb` contient l'image presente en sortie.
// Erreur -> `Status` (parametres hors bornes, camera degeneree,
// framebuffer invalide), jamais de `throw`, jamais de SDL.
[[nodiscard]] Status render(const scene::Scene& scene, Framebuffer& fb,
                            const RenderParams& params);

} // namespace rt::render
