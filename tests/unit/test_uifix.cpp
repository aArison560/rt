// Fix microui (affichage) — tests Catch2, sans ecran.
// DoD : entrees souris transferees (`handleMouseMove/Down/Up`) -> `frame()`
// survole ; file `Window::pollUiEvent` bornee sans alloc ; overlay nul
// sans crash (froid, hors hot path).

#include <catch2/catch_amalgamated.hpp>

extern "C" {
#include "microui/microui.h"
}

#include "rt/app/UiOverlay.hpp"
#include "rt/platform/Window.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/ui/Panel.hpp"

TEST_CASE("uifix : entrees sans scene sans crash", "[uifix]") {
	rt::ui::Panel panel;
	REQUIRE(panel.nativeContext() == nullptr);
	REQUIRE_FALSE(panel.isHovering());
	panel.handleMouseMove(20, 20);
	panel.handleMouseDown(20, 20, 1);
	panel.handleMouseUp(20, 20, 1);
	panel.handleScroll(0, 1);
	panel.handleMouseDown(0, 0, 0);
	panel.handleMouseDown(0, 0, 99);
	panel.frame();
	REQUIRE_FALSE(panel.isHovering());
	REQUIRE_FALSE(panel.takeLaunchRequest());
}

TEST_CASE("uifix : survol dedans/dehors (10,10,240,340)", "[uifix]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::ui::Panel panel;
	panel.attach(&parsed.value());
	REQUIRE(panel.nativeContext() != nullptr);
	panel.handleMouseMove(20, 20);
	panel.frame();
	panel.frame();
	REQUIRE(panel.isHovering());
	panel.handleMouseMove(500, 500);
	panel.frame();
	panel.frame();
	REQUIRE_FALSE(panel.isHovering());
	// Clic hors UI : pas de requete parasite.
	REQUIRE_FALSE(panel.takeLaunchRequest());
	REQUIRE_FALSE(panel.takeSaveRequest());
}

TEST_CASE("uifix : window file UI sans ecran sans crash", "[uifix]") {
	rt::platform::Window window;
	REQUIRE(window.nativeRenderer() == nullptr);
	rt::platform::UiEvent ev;
	REQUIRE_FALSE(window.pollUiEvent(ev));
	int x = -1;
	int y = -1;
	bool down = true;
	window.uiMouseState(x, y, down);
	REQUIRE(x == 0);
	REQUIRE(y == 0);
	REQUIRE_FALSE(down);
	REQUIRE_FALSE(window.beginPresent());
	window.endPresent();
}

TEST_CASE("uifix : overlay nul sans crash", "[uifix]") {
	rt::app::UiOverlay overlay;
	REQUIRE_FALSE(overlay.ready());
	REQUIRE_FALSE(overlay.init(nullptr));
	overlay.draw(nullptr);
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::ui::Panel panel;
	panel.attach(&parsed.value());
	panel.frame();
	// Sans renderer : sans effet, jamais de crash.
	overlay.draw(panel.nativeContext());
	overlay.shutdown();
	REQUIRE_FALSE(overlay.ready());
}

TEST_CASE("uifix : frame() produit des commandes dessinables", "[uifix]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::ui::Panel panel;
	panel.attach(&parsed.value());
	panel.handleMouseMove(20, 20);
	panel.frame();
	auto* ctx = static_cast<mu_Context*>(panel.nativeContext());
	REQUIRE(ctx != nullptr);
	mu_Command* cmd = nullptr;
	int rects = 0;
	int texts = 0;
	while (mu_next_command(ctx, &cmd) != 0) {
		if (cmd->type == MU_COMMAND_RECT) {
			++rects;
		} else if (cmd->type == MU_COMMAND_TEXT) {
			++texts;
		}
	}
	// Fenetre + boutons + sliders : des rects a dessiner, du texte a poser.
	REQUIRE(rects > 5);
	REQUIRE(texts > 5);
}
