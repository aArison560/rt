#pragma once

// Boucle de rendu mono-thread (T032) + multi-objets (T046) + reflexion (T056)
// + refraction (T057) — sans SDL (R6).
// `render()` parcourt les pixels, genere le rayon via `Camera` (T031),
// cherche l'intersection la plus proche parmi tous les objets (T046 : tri
// par `t`, `tMax` resserre ; objets directs + groupes aplatis, 4 types en
// coexistence, doublons du meme type autorises ; `tMin` = 0.001), ombre
// via Lambert (T033 : materiau de l'objet touche + ambiance + **toutes** les
// ponctuelles avec position, T052 : multi-spot melange, shadow ray `tMin`
// eps anti-acne + attenuation T051, miss -> fond de scene) + speculaire
// Blinn-Phong (T053) + reflexion bornee (T056 : `reflectivity` 0 = mat /
// 1 = miroir pur, `out = direct*(1-R) + reflechi*R`, `maxDepth` 0..32)
// + refraction bornee (T057 : Descartes `n1*sin(t1) = n2*sin(t2)`,
// `eta = frontFace ? 1/ior : ior`, `out = base*(1-T) + transmis*T`,
// `T = transparency` 0 = opaque / 1 = transmis pur, `ior = 1` = sans
// deviation, repli miroir en reflexion totale interne),
// ecrit dans le `Framebuffer` persistant (T030) puis `present()` (tonemapping + gamma 2.2).
// persistant (T030) puis `present()` (tonemapping + gamma 2.2). Profondeur max bornee
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
