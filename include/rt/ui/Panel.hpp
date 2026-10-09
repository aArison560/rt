#pragma once

// Panneau microui (T075) — champs editables alimentes par la table unique.
// Chaque champ visible vient de `schema/Directives` (R1) : ajouter une
// directive l'affiche (test). Modifier une couleur depuis l'UI change
// l'image (re-trace par l'appelant, fanions R5). microui est vendored dans
// `thirdparty/microui/` (v2.02, MIT, rxi) et pilote en `Panel.cpp`.
// Aucune levee (R2) ; allocations hors boucle chaude uniquement.

#include <string>
#include <vector>

#include "rt/base/Vec.hpp"

namespace rt::scene {
struct Scene;
}

namespace rt::ui {

class Panel {
  public:
	Panel() = default;
	~Panel();

	Panel(const Panel&) = delete;
	Panel& operator=(const Panel&) = delete;
	Panel(Panel&&) = delete;
	Panel& operator=(Panel&&) = delete;

	// Attache une scene mutable (non possedee, doit survivre au panneau).
	// Nul -> detache (les setters deviennent sans effet, jamais de crash).
	void attach(scene::Scene* scene) noexcept;
	[[nodiscard]] bool attached() const noexcept { return scene_ != nullptr; }

	// Noms des champs issus de la table `schema/` (R1, test T075).
	[[nodiscard]] std::vector<std::string> fieldNames() const;

	// Editions (retournent vrai si appliquees, fanions R5 leves).
	bool setBackground(Vec3 color) noexcept;
	bool setAmbientIntensity(float intensity) noexcept;
	bool setCameraFov(float fov) noexcept;
	bool setFirstAlbedo(Vec3 albedo) noexcept;
	// Deplacement live du 1er objet (T109, *Environment 3*) : `x` absolu
	// applique a `center.x` + `point.x` (couvre sphere/cylindre/cone et
	// plan), `touchObjects()` (version++ -> BVH invalidee, R5). Sans
	// objet -> faux.
	bool setFirstObjectX(float x) noexcept;
	// Echelle de texture live du 1er objet (T109) : `s` -> `scale (s s)`,
	// fanions R5 seuls (pas de version++ : la BVH est inchangee).
	// Sans objet ou sans texture -> faux (jamais de crash).
	bool setFirstTextureScale(float scale) noexcept;

	// Session microui (logique immediate, sans SDL ici) : construit une
	// fenetre avec sliders/couleurs et boutons (chargement/lancement geres
	// par l'appelant via les setters ci-dessus). Sans scene -> sans effet.
	void frame() noexcept;

	// Demande de (re)lancement posee par l'UI (bouton), consommee par l'appelant.
	[[nodiscard]] bool takeLaunchRequest() noexcept;
	// Demande de sauvegarde posee par l'UI (bouton `Save PNG`, T077).
	[[nodiscard]] bool takeSaveRequest() noexcept;

	// Progression du rendu (T108, *Environment 1*) : `setProgress(done,
	// total)` est appele par la callback `onProgress` du renderer (1x par
	// batch spp, hors hot path fin) ; `frame()` affiche `done/total`,
	// le pourcentage et une barre textuelle. `total <= 0` ou `done < 0`
	// reinitialise (0/0). Sans appel : 0/0 (jamais de fausse barre).
	void setProgress(int done, int total) noexcept;
	[[nodiscard]] int progressDone() const noexcept { return progressDone_; }
	[[nodiscard]] int progressTotal() const noexcept { return progressTotal_; }
	[[nodiscard]] float progressFraction() const noexcept;

  private:
	scene::Scene* scene_ = nullptr;
	void* ctx_ = nullptr;
	bool launchRequested_ = false;
	bool saveRequested_ = false;
	int progressDone_ = 0;
	int progressTotal_ = 0;
};

} // namespace rt::ui
