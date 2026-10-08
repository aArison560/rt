// Tests de la boucle de rendu mono-thread (T032), Catch2.
// DoD : `./rt scenes/default.rt 64 64 --out /tmp/x.png` -> 0 (verifie
// a la main + via `main`), determinisme (2 rendus identiques octet par
// octet), aucune dependance SDL (R6 : `grep -R SDL src/render` vide).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

rt::scene::Scene loadDefault() {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(result.isOk());
	return std::move(result.value());
}

} // namespace

TEST_CASE("renderer : rend le fond sur une petite image", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1, .maxDepth = 4, .seed = 0};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	REQUIRE(fb.width() == 16);
	REQUIRE(fb.height() == 12);
	// Chaque pixel a recu exactement `spp` echantillons (miss -> fond).
	REQUIRE(fb.samplesAt(0, 0) == 1);
	REQUIRE(fb.samplesAt(15, 11) == 1);
	// L'accumulation vaut le fond de scene (aucune primitive en P3).
	REQUIRE(fb.accumAt(0, 0).x == Catch::Approx(scene.background.color.x));
	REQUIRE(fb.accumAt(8, 6).y == Catch::Approx(scene.background.color.y));
	REQUIRE(fb.accumAt(15, 11).z == Catch::Approx(scene.background.color.z));
	// Couleurs bornees, jamais de NaN (le `present()` sature deja, T030).
	for (int y = 0; y < 12; ++y) {
		for (int x = 0; x < 16; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			REQUIRE(std::isfinite(accum.x));
			REQUIRE(std::isfinite(accum.y));
			REQUIRE(std::isfinite(accum.z));
		}
	}
}

TEST_CASE("renderer : determinisme (2 rendus identiques)", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 42};
	rt::render::Framebuffer first;
	rt::render::Framebuffer second;
	REQUIRE(rt::render::render(scene, first, params).isOk());
	REQUIRE(rt::render::render(scene, second, params).isOk());
	REQUIRE(first.width() == second.width());
	REQUIRE(first.height() == second.height());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = first.accumAt(x, y);
			const rt::Vec3 b = second.accumAt(x, y);
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
		}
	}
	const std::size_t count = first.pixelCount();
	for (std::size_t i = 0; i < count; ++i) {
		REQUIRE(first.displayData()[i].r == second.displayData()[i].r);
		REQUIRE(first.displayData()[i].g == second.displayData()[i].g);
		REQUIRE(first.displayData()[i].b == second.displayData()[i].b);
	}
}

TEST_CASE("renderer : parametres invalides -> erreur propre, sans crash", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 0, .height = 12, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 0, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 9000, .height = 12, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 0}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 2048}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .maxDepth = 99})
	            .isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .seed = -1})
	            .isError());
}

TEST_CASE("renderer : camera degeneree -> erreur propagee", "[renderer]") {
	rt::scene::Scene scene = loadDefault();
	scene.camera.position = scene.camera.target;
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1};
	REQUIRE(rt::render::render(scene, fb, params).isError());
}
