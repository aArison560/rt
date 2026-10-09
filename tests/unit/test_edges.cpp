// Bords fenetres (T078) — tests Catch2 + integration.
// DoD : resize/min/quit/focus/DISPLAY absent sans plantage (ASan).

#include <catch2/catch_amalgamated.hpp>

#include <cstdlib>
#include <filesystem>
#include <sys/wait.h>

#include "rt/platform/Window.hpp"
#include "rt/render/Framebuffer.hpp"

namespace {

bool haveDisplay() {
	const char* display = std::getenv("DISPLAY");
	return display != nullptr && display[0] != '\0';
}

} // namespace

TEST_CASE("edges : taille minimale 64x64 imposee (T078)", "[edges]") {
	rt::platform::Window window;
	// Avant SDL (pas besoin d'ecran) : dimensions invalides refusees.
	REQUIRE(window.init(0, 64, "t").isError());
	REQUIRE(window.init(64, 0, "t").isError());
	REQUIRE(window.init(63, 64, "t").isError());
	REQUIRE(window.init(64, 63, "t").isError());
	REQUIRE(window.init(8193, 64, "t").isError());
	REQUIRE_FALSE(window.isOpen());
}

TEST_CASE("edges : reopen sequentiel et double ouverture (T078)", "[edges]") {
	if (!haveDisplay()) {
		SUCCEED("no DISPLAY: bords SDL non testables ici (headless vert)");
		return;
	}
	rt::platform::Window window;
	if (window.init(64, 64, "t078-reopen").isError()) {
		SUCCEED("SDL indisponible ici, pas de crash");
		return;
	}
	REQUIRE(window.isOpen());
	// Double ouverture simultanee refusee (pas de crash, pas de fuite).
	REQUIRE(window.init(64, 64, "t").isError());
	window.shutdown();
	REQUIRE_FALSE(window.isOpen());
	// Reouverture sequentielle OK.
	REQUIRE(window.init(64, 64, "t078-reopen2").isOk());
	REQUIRE(window.isOpen());
	window.shutdown();
	REQUIRE_FALSE(window.isOpen());
}

TEST_CASE("edges : blit multi-resolutions sans crash (T078)", "[edges]") {
	if (!haveDisplay()) {
		SUCCEED("no DISPLAY: resize non testable ici");
		return;
	}
	rt::platform::Window window;
	if (window.init(64, 64, "t078-resize").isError()) {
		SUCCEED("SDL indisponible ici, pas de crash");
		return;
	}
	rt::render::Framebuffer small;
	REQUIRE(small.init(64, 64).isOk());
	small.present();
	window.blit(small);
	rt::render::Framebuffer wide;
	REQUIRE(wide.init(96, 64).isOk());
	wide.present();
	// Texture recreee (chemin froid), pas de crash, compteur avance.
	window.blit(wide);
	REQUIRE(window.stats().blitCount == 2);
	window.shutdown();
}

TEST_CASE("edges : sans ecran le mode fenetre echoue proprement (T078)", "[edges]") {
	// `runWindowed` refuse sans DISPLAY/WAYLAND (code 1, message, pas de bloc).
	// Commande exacte, sans `--out` (fenetre demandee), avec `--quiet`.
	const int rc = std::system(
	    "env -u DISPLAY -u WAYLAND_DISPLAY ./rt scenes/default.rt 64 64 --window --quiet "
	    ">/dev/null 2>&1");
	// 139/134 = crash (interdit) ; 0 = inattendu (fenetre sans ecran) ; 1 = DoD.
	REQUIRE(rc != 139 * 256);
	REQUIRE(rc != 134 * 256);
	REQUIRE(WEXITSTATUS(rc) == 1);
}
