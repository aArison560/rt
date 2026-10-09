// Couche plateforme SDL (T070) — tests Catch2.
// DoD : fenetre ouverte, image affichee, fermeture propre, sans fuite SDL.
// Sans `DISPLAY` (CI), pilote factice `dummy` (seul pilote propre sous LSan,
// le repli par defaut sonde KMSDRM/libdrm et fuit dans SDL2 malgre `SDL_Quit`) ;
// avec `DISPLAY`, ouverture reelle breve + blit + expose + fermeture.

#include <catch2/catch_amalgamated.hpp>

#include <cstdlib>

#include "rt/platform/Window.hpp"
#include "rt/app/Options.hpp"
#include "rt/render/Framebuffer.hpp"

namespace {

bool haveDisplay() {
	const char* display = std::getenv("DISPLAY");
	return display != nullptr && display[0] != '\0';
}

} // namespace

TEST_CASE("window : dimensions invalides refusees sans crash", "[window]") {
	rt::platform::Window window;
	REQUIRE(window.init(0, 64, "t").isError());
	REQUIRE(window.init(64, 0, "t").isError());
	REQUIRE(window.init(64, 63, "t").isError());
	REQUIRE(window.init(63, 64, "t").isError());
	REQUIRE(window.init(9000, 64, "t").isError());
	REQUIRE_FALSE(window.isOpen());
	window.shutdown();
	REQUIRE_FALSE(window.isOpen());
	// Double fermeture sans echec.
	window.shutdown();
}

TEST_CASE("window : double init refusee", "[window]") {
	if (!haveDisplay()) {
		SUCCEED("no DISPLAY: skip double-open (init would fail cleanly)");
		return;
	}
	rt::platform::Window window;
	rt::Status first = window.init(64, 64, "t070-double");
	if (first.isError()) {
		SUCCEED("SDL indisponible ici, pas de crash");
		return;
	}
	REQUIRE(window.isOpen());
	REQUIRE(window.init(64, 64, "t").isError());
	window.shutdown();
	REQUIRE_FALSE(window.isOpen());
}

TEST_CASE("window : blit puis expose sans recalcul (T070/T071)", "[window]") {
	if (!haveDisplay()) {
		// CI headless : force `SDL_VIDEODRIVER=dummy` (cf. `runWindowed`,
		// pilote reserve aux tests). Restaure ensuite : `test_edges` exige
		// l'echec sans pilote et herite de cet environnement via `system`.
		const bool hadDriver = (std::getenv("SDL_VIDEODRIVER") != nullptr);
		if (!hadDriver) {
			setenv("SDL_VIDEODRIVER", "dummy", 1);
		}
		rt::platform::Window window;
		// Sans ecran, l'echec est propre (message + code), jamais de crash.
		// Le resultat depend du pilote factice : erreur OU succes les deux
		// valides, l'important est l'absence de crash et la RAII.
		rt::Status status = window.init(64, 64, "t070-nodisplay");
		if (status.isError()) {
			REQUIRE_FALSE(window.isOpen());
		} else {
			window.shutdown();
			REQUIRE_FALSE(window.isOpen());
		}
		if (!hadDriver) {
			unsetenv("SDL_VIDEODRIVER");
		}
		SUCCEED("no DISPLAY: init propre (erreur ou succes factice)");
		return;
	}
	rt::platform::Window window;
	rt::Status status = window.init(128, 96, "t070");
	REQUIRE(status.isOk());
	REQUIRE(window.isOpen());
	REQUIRE(window.width() == 128);
	REQUIRE(window.height() == 96);

	rt::render::Framebuffer fb;
	REQUIRE(fb.init(128, 96).isOk());
	fb.clear();
	fb.addSample(10, 10, rt::Vec3(1.0F, 0.0F, 0.0F));
	fb.present();
	window.blit(fb);
	REQUIRE(window.stats().blitCount == 1);
	// Expose : re-presente sans toucher au tampon (compteur dedie).
	window.presentCached();
	REQUIRE(window.stats().exposeCount == 1);
	REQUIRE(window.stats().lastBlitUs >= 0);
	// Tampon nul / ferme : ignore sans crash.
	window.shutdown();
	REQUIRE_FALSE(window.isOpen());
	window.blit(fb);
	window.presentCached();
	SUCCEED("open/blit/expose/close propres");
}

TEST_CASE("options : --window reconnu (T070)", "[window]") {
	// Le fanion est teste ici pour garder `test_options.cpp` intact (T026).
	// `parseOptionsVec` prend `args[0]` = nom du programme.
	std::vector<std::string> args = {"rt", "scenes/default.rt", "--window"};
	rt::Result<rt::app::Options> parsed = rt::app::parseOptionsVec(args);
	// `parseOptionsVec` est declare dans `rt/app/Options.hpp` ; si ce test
	// compile, le fanion existe. On verifie sa valeur.
	REQUIRE(parsed.isOk());
	REQUIRE(parsed.value().showWindow);
}
