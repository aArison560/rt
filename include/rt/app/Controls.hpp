#pragma once

// Controles clavier (T073, M5) — logique pure, sans SDL dans l'en-tete.
// `Window` (platform) traduit les evenements en `SdlKey` brut (int) ;
// ce module traduit en `KeyAction` puis applique a la `Scene` (camera,
// lumiere, FOV). Chaque action leve `sceneDirty`/`displayDirty` (R5) sans
// reconstruire inutilement (pas de version++ pour la camera seule).
// Touches documentees dans `README.md` et affichees dans l'UI (T075).
// Aucune levee (R2), aucune allocation (R3).

#include <cstdint>

namespace rt::scene {
struct Scene;
}

namespace rt::app {

// Actions clavier (stables, testables sans ecran).
enum class KeyAction : std::uint8_t {
	None = 0,
	Forward = 1,
	Back = 2,
	StrafeLeft = 3,
	StrafeRight = 4,
	MoveUp = 5,
	MoveDown = 6,
	FovIn = 7,
	FovOut = 8,
	LightUp = 9,
	LightDown = 10,
	Reset = 11,
};

[[nodiscard]] const char* toString(KeyAction action) noexcept;

// Traduit un code touche SDL (`SDL_Keycode`, int) en action.
// Inconnu -> `None` (jamais de re-affichage gratuit, DoD T073).
[[nodiscard]] KeyAction keyFromSdl(int sdlKey) noexcept;

// Applique l'action a `scene` (camera/lumiere/FOV), leve les fanions R5.
// Retourne vrai si l'action a change la scene (donc re-affichage requis),
// faux si `None` ou sans effet (aucun re-affichage, DoD T073).
// `scene` nulle -> faux (documente, jamais de crash).
bool applyKeyAction(scene::Scene& scene, KeyAction action) noexcept;

} // namespace rt::app
