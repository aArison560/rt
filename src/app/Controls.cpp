// Controles clavier (T073, M5) — implementation sans SDL dans la logique.
// Voir `include/rt/app/Controls.hpp` pour le contrat et `README.md` pour
// la table des touches. Deplacements de 0.5 unite, FOV de 5 degres,
// lumiere de 0.2, bornes du schema (R1). Camera seule -> fanions R5 sans
// version++ (BVH conservee) ; lumiere -> `touchObjects` (version++, T062).

#include "rt/app/Controls.hpp"

#include <cmath>

#include "rt/scene/Scene.hpp"

namespace rt::app {

namespace {

// Pas de 0.5 (monde), FOV de 5 degres, lumiere de 0.2.
constexpr float kStep = 0.5F;
constexpr float kFovStep = 5.0F;
constexpr float kLightStep = 0.2F;
constexpr float kMinFov = 10.0F;
constexpr float kMaxFov = 120.0F;

// Codes SDL sans inclure <SDL.h> ici (stables SDL2, verifies a la compil).
// `Window` envoie le `SDL_Keycode` brut ; la correspondance est testee.
constexpr int kSdlUp = 1073741906;
constexpr int kSdlDown = 1073741905;
constexpr int kSdlLeft = 1073741904;
constexpr int kSdlRight = 1073741903;

void markCameraDirty(scene::Scene& scene) noexcept {
	scene.sceneDirty = true;
	scene.displayDirty = true;
}

void moveAlong(scene::Scene& scene, Vec3 direction) noexcept {
	const float len = length(direction);
	if (len <= 0.0F || !std::isfinite(len)) {
		return;
	}
	const Vec3 step = direction * (kStep / len);
	scene.camera.position = scene.camera.position + step;
	scene.camera.target = scene.camera.target + step;
	markCameraDirty(scene);
}

} // namespace

const char* toString(KeyAction action) noexcept {
	switch (action) {
	case KeyAction::None:
		return "none";
	case KeyAction::Forward:
		return "forward";
	case KeyAction::Back:
		return "back";
	case KeyAction::StrafeLeft:
		return "strafe-left";
	case KeyAction::StrafeRight:
		return "strafe-right";
	case KeyAction::MoveUp:
		return "move-up";
	case KeyAction::MoveDown:
		return "move-down";
	case KeyAction::FovIn:
		return "fov-in";
	case KeyAction::FovOut:
		return "fov-out";
	case KeyAction::LightUp:
		return "light-up";
	case KeyAction::LightDown:
		return "light-down";
	case KeyAction::Reset:
		return "reset";
	}
	return "unknown";
}

KeyAction keyFromSdl(int sdlKey) noexcept {
	switch (sdlKey) {
	case kSdlUp:
	case 'w':
	case 'W':
		return KeyAction::Forward;
	case kSdlDown:
	case 's':
	case 'S':
		// Note : `S` avance ici (mouvement) ; la capture d'ecran (T077)
		// utilise `P` pour eviter le conflit (documente dans `README.md`).
		return KeyAction::Back;
	case kSdlLeft:
	case 'a':
	case 'A':
		return KeyAction::StrafeLeft;
	case kSdlRight:
	case 'd':
	case 'D':
		return KeyAction::StrafeRight;
	case 'q':
	case 'Q':
		return KeyAction::MoveUp;
	case 'e':
	case 'E':
		return KeyAction::MoveDown;
	case '+':
	case '=':
		return KeyAction::FovIn;
	case '-':
	case '_':
		return KeyAction::FovOut;
	case '1':
		return KeyAction::LightUp;
	case '2':
		return KeyAction::LightDown;
	case 'r':
	case 'R':
		return KeyAction::Reset;
	default:
		break;
	}
	return KeyAction::None;
}

bool applyKeyAction(scene::Scene& scene, KeyAction action) noexcept {
	switch (action) {
	case KeyAction::None:
		return false;
	case KeyAction::Forward: {
		const Vec3 forward = scene.camera.target - scene.camera.position;
		moveAlong(scene, forward);
		return true;
	}
	case KeyAction::Back: {
		const Vec3 forward = scene.camera.target - scene.camera.position;
		moveAlong(scene, forward * -1.0F);
		return true;
	}
	case KeyAction::StrafeLeft: {
		const Vec3 forward = scene.camera.target - scene.camera.position;
		const Vec3 up = scene.camera.up;
		// Droite = forward x up ; gauche = -droite.
		const Vec3 right = Vec3(forward.y * up.z - forward.z * up.y,
		                        forward.z * up.x - forward.x * up.z,
		                        forward.x * up.y - forward.y * up.x);
		moveAlong(scene, right * -1.0F);
		return true;
	}
	case KeyAction::StrafeRight: {
		const Vec3 forward = scene.camera.target - scene.camera.position;
		const Vec3 up = scene.camera.up;
		const Vec3 right = Vec3(forward.y * up.z - forward.z * up.y,
		                        forward.z * up.x - forward.x * up.z,
		                        forward.x * up.y - forward.y * up.x);
		moveAlong(scene, right);
		return true;
	}
	case KeyAction::MoveUp: {
		scene.camera.position.y += kStep;
		scene.camera.target.y += kStep;
		markCameraDirty(scene);
		return true;
	}
	case KeyAction::MoveDown: {
		scene.camera.position.y -= kStep;
		scene.camera.target.y -= kStep;
		markCameraDirty(scene);
		return true;
	}
	case KeyAction::FovIn: {
		scene.camera.fov -= kFovStep;
		if (scene.camera.fov < kMinFov) {
			scene.camera.fov = kMinFov;
		}
		markCameraDirty(scene);
		return true;
	}
	case KeyAction::FovOut: {
		scene.camera.fov += kFovStep;
		if (scene.camera.fov > kMaxFov) {
			scene.camera.fov = kMaxFov;
		}
		markCameraDirty(scene);
		return true;
	}
	case KeyAction::LightUp:
	case KeyAction::LightDown: {
		if (scene.lights.empty()) {
			return false;
		}
		float& intensity = scene.lights[0].intensity;
		if (action == KeyAction::LightUp) {
			intensity += kLightStep;
			if (intensity > 10.0F) {
				intensity = 10.0F;
			}
		} else {
			intensity -= kLightStep;
			if (intensity < 0.0F) {
				intensity = 0.0F;
			}
		}
		scene.touchObjects();
		return true;
	}
	case KeyAction::Reset: {
		scene.camera.position = Vec3(0.0F, 1.0F, 4.0F);
		scene.camera.target = Vec3(0.0F, 0.0F, 0.0F);
		scene.camera.up = Vec3(0.0F, 1.0F, 0.0F);
		scene.camera.fov = 60.0F;
		markCameraDirty(scene);
		return true;
	}
	}
	return false;
}

} // namespace rt::app
