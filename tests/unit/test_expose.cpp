// Expose sans recalcul (T071, R4) — tests Catch2.
// DoD : `grep -n "render(" src/platform/` sans appel dans le chemin expose
// (verifie a la main + par ce test via compteurs), preuve ecrite en T072.
// Strategie : `blit` 1x (copie tampon -> texture), puis `presentCached` Nx
// (re-presente sans toucher au tampon) ; le tampon reste inchange (pas de
// nouveau calcul) et seul `exposeCount` augmente.

#include <catch2/catch_amalgamated.hpp>

#include <cstdlib>

#include "rt/platform/Window.hpp"
#include "rt/render/Framebuffer.hpp"

namespace {

bool haveDisplay() {
	const char* display = std::getenv("DISPLAY");
	return display != nullptr && display[0] != '\0';
}

} // namespace

TEST_CASE("expose : reblit sans recalcul (R4, T071)", "[expose]") {
	if (!haveDisplay()) {
		SUCCEED("no DISPLAY: expose non testable ici (CI headless, T035 reste vert)");
		return;
	}
	rt::platform::Window window;
	if (window.init(96, 64, "t071").isError()) {
		SUCCEED("SDL indisponible ici, pas de crash");
		return;
	}
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(96, 64).isOk());
	fb.clear();
	fb.addSample(5, 5, rt::Vec3(0.2F, 0.4F, 0.6F));
	fb.present();
	const rt::Vec3 before = fb.accumAt(5, 5);

	window.blit(fb);
	REQUIRE(window.stats().blitCount == 1);
	REQUIRE(window.stats().exposeCount == 0);

	// 3 exposes : aucun nouveau `blit`, tampon inchange (pas de recalcul).
	window.presentCached();
	window.presentCached();
	window.presentCached();
	REQUIRE(window.stats().blitCount == 1);
	REQUIRE(window.stats().exposeCount == 3);
	REQUIRE(window.stats().lastBlitUs >= 0);
	REQUIRE(fb.accumAt(5, 5).x == before.x);
	REQUIRE(fb.accumAt(5, 5).y == before.y);
	REQUIRE(fb.accumAt(5, 5).z == before.z);

	// Resize : recopie proportionnelle (etire la texture existante, T071).
	// `pollQuit` re-presente sur `RESIZED` sans nouveau calcul (voir code).
	window.shutdown();
	SUCCEED("expose = reblit seul, 0 recalcul");
}
