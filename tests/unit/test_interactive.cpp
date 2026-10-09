// Etat interactif (T076, R5) — tests Catch2, sans ecran.
// DoD : UI fluide (preview 1 spp puis affinage), aucun re-trace complet
// a chaque frame d'affichage (compteurs).

#include <catch2/catch_amalgamated.hpp>

#include "rt/app/Interactive.hpp"

TEST_CASE("interactive : edition -> preview puis full puis blit (T076)", "[interactive]") {
	rt::app::Interactive ctl;
	REQUIRE_FALSE(ctl.needsPreview());
	REQUIRE_FALSE(ctl.needsFull());
	REQUIRE_FALSE(ctl.needsBlit());

	ctl.onEdited();
	REQUIRE(ctl.needsPreview());
	REQUIRE_FALSE(ctl.needsFull());
	ctl.consumePreview();
	REQUIRE_FALSE(ctl.needsPreview());
	REQUIRE(ctl.counts().previews == 1);
	REQUIRE(ctl.needsBlit());
	ctl.consumeBlit();
	REQUIRE_FALSE(ctl.needsBlit());
	REQUIRE(ctl.counts().blits == 1);

	ctl.onPreviewDone();
	REQUIRE(ctl.needsFull());
	ctl.consumeFull();
	REQUIRE(ctl.counts().fulls == 1);
	REQUIRE(ctl.needsBlit());
	ctl.consumeBlit();
	REQUIRE(ctl.counts().blits == 2);
	ctl.onFullDone();
	REQUIRE_FALSE(ctl.needsPreview());
	REQUIRE_FALSE(ctl.needsFull());
}

TEST_CASE("interactive : affichage seul = blit sans re-trace (T076)", "[interactive]") {
	rt::app::Interactive ctl;
	ctl.onDisplayOnly();
	REQUIRE(ctl.needsBlit());
	REQUIRE_FALSE(ctl.needsPreview());
	REQUIRE_FALSE(ctl.needsFull());
	ctl.consumeBlit();
	REQUIRE(ctl.counts().blits == 1);
	REQUIRE(ctl.counts().previews == 0);
	REQUIRE(ctl.counts().fulls == 0);
	// 100 frames d'affichage : 0 preview, 0 full (DoD, pas de re-trace gratuit).
	for (int i = 0; i < 100; ++i) {
		ctl.onDisplayOnly();
		ctl.consumeBlit();
	}
	REQUIRE(ctl.counts().previews == 0);
	REQUIRE(ctl.counts().fulls == 0);
	REQUIRE(ctl.counts().blits == 101);
}
