// Damier procedural (T105, *Disruptions* 2), Catch2.
// `checkerAlbedo` alterne sur plan et sphere ; `scenes/opt_checker.rt`
// montre les deux.

#include <catch2/catch_amalgamated.hpp>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/shading/Pattern.hpp"

TEST_CASE("pattern : damier alterne en UV (T105)", "[pattern][t105]") {
	const rt::Vec3 albedo(0.8F, 0.8F, 0.8F);
	// Case (0,0) claire, (1,0) sombre, (0,1) sombre, (1,1) claire.
	const rt::Vec3 light = rt::shading::checkerAlbedo(albedo, rt::Vec2(0.25F, 0.25F), 1.0F, 1.0F);
	REQUIRE(light.x == Catch::Approx(0.8F));
	const rt::Vec3 dark = rt::shading::checkerAlbedo(albedo, rt::Vec2(1.25F, 0.25F), 1.0F, 1.0F);
	REQUIRE(dark.x == Catch::Approx(0.12F).margin(0.001));
	// Taille reglable : scale 2 double la frequence ((0.6,0.1) clair en
	// x1, sombre en x2).
	const rt::Vec3 scaled =
	    rt::shading::checkerAlbedo(albedo, rt::Vec2(0.6F, 0.1F), 2.0F, 1.0F);
	const rt::Vec3 unscaled =
	    rt::shading::checkerAlbedo(albedo, rt::Vec2(0.6F, 0.1F), 1.0F, 1.0F);
	REQUIRE((scaled.x != unscaled.x));
	// Degeneres : NaN et scale <= 0 -> albedo inchange, jamais de NaN.
	const rt::Vec3 nan =
	    rt::shading::checkerAlbedo(albedo, rt::Vec2(std::nanf(""), 0.0F), 1.0F, 1.0F);
	REQUIRE(nan.x == Catch::Approx(0.8F));
	const rt::Vec3 bad = rt::shading::checkerAlbedo(albedo, rt::Vec2(1.25F, 0.25F), 0.0F, 1.0F);
	REQUIRE(bad.x == Catch::Approx(0.12F).margin(0.001));
	// Noms du schema -> kind.
	REQUIRE(rt::shading::patternKindFrom("checker", true) == rt::shading::PatternKind::Checker);
	REQUIRE(rt::shading::patternKindFrom("sine", true) == rt::shading::PatternKind::Sine);
	REQUIRE(rt::shading::patternKindFrom("perlin", true) == rt::shading::PatternKind::Perlin);
	REQUIRE(rt::shading::patternKindFrom("checker", false) == rt::shading::PatternKind::None);
}

TEST_CASE("pattern : damier sur plan et sphere (T105)", "[pattern][t105]") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/opt_checker.rt");
	INFO((result.isError() ? result.status().message : std::string("ok")));
	REQUIRE(result.isOk());
	rt::render::Framebuffer fb;
	rt::render::RenderParams params{.width = 160, .height = 120, .spp = 2, .maxDepth = 2, .seed = 13};
	REQUIRE(rt::render::render(result.value(), fb, params, nullptr).isOk());
	// Les deux objets sont damiers : l'image contient a la fois des pixels
	// clairs et des pixels sombres (alternance, pas d'aplats seuls).
	int bright = 0;
	int dark = 0;
	for (int y = 0; y < fb.height(); ++y) {
		for (int x = 0; x < fb.width(); ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			const int n = fb.samplesAt(x, y);
			const double lum = (static_cast<double>(accum.x) + static_cast<double>(accum.y) +
			                    static_cast<double>(accum.z)) /
			                   (3.0 * static_cast<double>(n));
			if (lum > 0.35) {
				++bright;
			} else if (lum > 0.05 && lum < 0.2) {
				++dark;
			}
		}
	}
	REQUIRE(bright > 500);
	REQUIRE(dark > 200);
}
