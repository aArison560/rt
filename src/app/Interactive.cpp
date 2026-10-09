// Etat interactif (T076, R5) — implementation.
// Voir `include/rt/app/Interactive.hpp` pour le contrat. Aucune SDL,
// aucune scene ici (compteurs seuls, l'appelant applique aux fanions).

#include "rt/app/Interactive.hpp"

namespace rt::app {

void Interactive::onEdited() noexcept {
	needPreview_ = true;
	needFull_ = false;
	needBlit_ = false;
}

void Interactive::onPreviewDone() noexcept {
	needFull_ = true;
}

void Interactive::onFullDone() noexcept {
	needPreview_ = false;
	needFull_ = false;
	needBlit_ = true;
}

void Interactive::onDisplayOnly() noexcept {
	// Sans edition : reblit seul, jamais de re-trace complet (DoD T076).
	if (!needPreview_ && !needFull_) {
		needBlit_ = true;
	}
}

void Interactive::consumePreview() noexcept {
	needPreview_ = false;
	++counts_.previews;
	needBlit_ = true;
}

void Interactive::consumeFull() noexcept {
	needFull_ = false;
	++counts_.fulls;
	needBlit_ = true;
}

void Interactive::consumeBlit() noexcept {
	needBlit_ = false;
	++counts_.blits;
}

} // namespace rt::app
