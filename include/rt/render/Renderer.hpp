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
// Progressif (T036) : `spp` echantillons accumules en batches (1 batch =
// 1 echantillon par pixel, externe), `onProgress(done, total)` appele
// apres chaque batch (futur affichage, ETA calcule par l'appelant).
// Deterministe : memes `spp` + meme `seed` -> memes pixels, la graine
// venant de `seedFor(x, y, s, seed)` (coordonnees absolues, T016) —
// le meme echantillon calcule dans une tuile ou en plein donne le meme
// jitter (couture cluster impossible).

#include "rt/base/Status.hpp"

namespace rt::scene {
struct Scene;
}

namespace rt::render {

class Framebuffer;

// Callback progressif (T036) : (done 1..total, total, user), `noexcept`.
using ProgressCallback = void (*)(int doneSamples, int totalSamples, void* user) noexcept;

struct RenderParams {
	int width = 640;
	int height = 480;
	int spp = 4;
	int maxDepth = 4;
	long long seed = 0;
	// Callback progressif (T036) : appele apres chaque echantillon-batch
	// avec (done in 1..spp, total == spp). Pointeur brut + `void*`
	// (pas de `std::function`, aucune allocation, R3). `nullptr` = muet.
	// Ne doit ni allouer dans le hot path par pixel (appele 1x par batch)
	// ni lever (marque `noexcept`, une levee = `terminate`).
	ProgressCallback onProgress = nullptr;
	void* progressUser = nullptr;
};

// Rend `scene` dans `fb` (reallouee une seule fois si la resolution
// change, chemin froid). `fb` contient l'image presente en sortie.
// Erreur -> `Status` (parametres hors bornes, camera degeneree,
// framebuffer invalide), jamais de `throw`, jamais de SDL.
[[nodiscard]] Status render(const scene::Scene& scene, Framebuffer& fb,
                            const RenderParams& params);

} // namespace rt::render
