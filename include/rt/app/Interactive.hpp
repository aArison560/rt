#pragma once

// Etat interactif (T076, R5) — machine basse-qualite puis affinage.
// `sceneDirty` -> appercu 1 spp immediat (BVH invalidee via `touchObjects`
// quand objets, sinon fanions seuls pour la camera) puis affinage au `spp`
// cible ; `displayDirty` seul -> reblit sans re-trace (compteur `blits`).
// Le calcul reste sur `ThreadPool` (tuiles, T063) : l'UI ne bloque jamais.
// Testable sans ecran (compteurs, pas de SDL ici).

#include <cstddef>

namespace rt::app {

struct InteractiveCounts {
	long long previews = 0;
	long long fulls = 0;
	long long blits = 0;
};

class Interactive {
  public:
	Interactive() = default;

	// Une edition vient d'avoir lieu (clavier/souris/UI) : demande appercu.
	void onEdited() noexcept;
	// Appercu 1 spp termine -> demande affinage (si `targetSpp > 1`).
	void onPreviewDone() noexcept;
	// Affinage termine -> plus rien a faire (sauf blit).
	void onFullDone() noexcept;
	// Affichage seul (expose/resize) -> reblit demande.
	void onDisplayOnly() noexcept;

	[[nodiscard]] bool needsPreview() const noexcept { return needPreview_; }
	[[nodiscard]] bool needsFull() const noexcept { return needFull_; }
	[[nodiscard]] bool needsBlit() const noexcept { return needBlit_; }
	[[nodiscard]] InteractiveCounts counts() const noexcept { return counts_; }

	// Consomme les demandes (appele apres chaque etape effectuee).
	void consumePreview() noexcept;
	void consumeFull() noexcept;
	void consumeBlit() noexcept;

  private:
	bool needPreview_ = false;
	bool needFull_ = false;
	bool needBlit_ = false;
	InteractiveCounts counts_;
};

} // namespace rt::app
