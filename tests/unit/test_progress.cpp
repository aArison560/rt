// Barre de progression (T108, *Environment 1*), Catch2.
// Le panneau recoit `done/total` par batch spp (callback `onProgress`,
// 1x par batch, hors hot path fin) et l'affiche ; en headless la meme
// callback imprime sur stderr (T036, verifie sur scene lente via le
// binaire, voir DoD).

#include <catch2/catch_amalgamated.hpp>

#include "rt/scene/Parser.hpp"
#include "rt/ui/Panel.hpp"

TEST_CASE("progress : panneau 0 puis 1..N puis complet (T108)", "[progress][t108]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::ui::Panel panel;
	panel.attach(&parsed.value());
	REQUIRE(panel.progressDone() == 0);
	REQUIRE(panel.progressTotal() == 0);
	REQUIRE(panel.progressFraction() == Catch::Approx(0.0F));
	panel.setProgress(1, 4);
	REQUIRE(panel.progressDone() == 1);
	REQUIRE(panel.progressTotal() == 4);
	REQUIRE(panel.progressFraction() == Catch::Approx(0.25F));
	panel.setProgress(4, 4);
	REQUIRE(panel.progressFraction() == Catch::Approx(1.0F));
	// `frame()` avec et sans progression : sans crash (logique immediate).
	panel.frame();
	panel.setProgress(-1, 0);
	REQUIRE(panel.progressFraction() == Catch::Approx(0.0F));
	panel.frame();
	// Borne : done > total sature.
	panel.setProgress(9, 4);
	REQUIRE(panel.progressDone() == 4);
	REQUIRE(panel.progressFraction() == Catch::Approx(1.0F));
	panel.frame();
}
