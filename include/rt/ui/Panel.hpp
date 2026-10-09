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

	// Session microui (logique immediate, sans SDL ici) : construit une
	// fenetre avec sliders/couleurs et boutons (chargement/lancement geres
	// par l'appelant via les setters ci-dessus). Sans scene -> sans effet.
	void frame() noexcept;

	// Demande de (re)lancement posee par l'UI (bouton), consommee par l'appelant.
	[[nodiscard]] bool takeLaunchRequest() noexcept;
	// Demande de sauvegarde posee par l'UI (bouton `Save PNG`, T077).
	[[nodiscard]] bool takeSaveRequest() noexcept;

  private:
	scene::Scene* scene_ = nullptr;
	void* ctx_ = nullptr;
	bool launchRequested_ = false;
	bool saveRequested_ = false;
};

} // namespace rt::ui
