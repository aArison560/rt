// Tests T058 : (a) ombres translucides + (b) *Direct light* (spot), Catch2.
// DoD : 2 scenes de preuve (`opt_shadow_transp.rt`, `opt_direct.rt`) + test de
// densite (ombre translucide > opaque en luminosite).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <string>

#include "rt/lighting/SpotLight.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

rt::scene::Scene parseOrDie(const std::string& content, const char* name) {
	rt::Result<rt::scene::Scene> result = rt::scene::parseContent(content, name);
	REQUIRE(result.isOk());
	return std::move(result.value());
}

rt::Vec3 meanAt(const rt::render::Framebuffer& fb, int x, int y) {
	const rt::Vec3 accum = fb.accumAt(x, y);
	const int n = fb.samplesAt(x, y);
	return rt::Vec3(accum.x / static_cast<float>(n), accum.y / static_cast<float>(n),
	                accum.z / static_cast<float>(n));
}

float luminance(rt::Vec3 c) {
	return (c.x + c.y + c.z) / 3.0F;
}

} // namespace

TEST_CASE("ombres translucides (T058a) : verre moins sombre qu'opaque", "[t058][shadow-transp]") {
	// Meme geometrie (sol + sphere occultrice + 1 ponctuelle), seule la
	// transparence de l'occulteur change : opaque `0.0` vs verre `0.8`.
	const char* opaqueText =
	    "scene { camera { position (0 4 7) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (4 5 3) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 0.8 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 transparency 0.0 ior 1.0 } } } }";
	const char* glassText =
	    "scene { camera { position (0 4 7) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (4 5 3) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 0.8 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 transparency 0.8 ior 1.5 } } } }";
	rt::scene::Scene opaque = parseOrDie(opaqueText, "opaque.rt");
	rt::scene::Scene glass = parseOrDie(glassText, "glass.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbOpaque;
	rt::render::Framebuffer fbGlass;
	REQUIRE(rt::render::render(opaque, fbOpaque, params).isOk());
	REQUIRE(rt::render::render(glass, fbGlass, params).isOk());
	double sumOpaqueShadow = 0.0;
	double sumGlassShadow = 0.0;
	int shadowCount = 0;
	int lighter = 0;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			const float lOpaque = luminance(meanAt(fbOpaque, x, y));
			const float lGlass = luminance(meanAt(fbGlass, x, y));
			REQUIRE(std::isfinite(lOpaque));
			REQUIRE(std::isfinite(lGlass));
			// Pixels d'ombre opaque (sombres) : la zone portee sur le sol.
			if (lOpaque < 0.06F) {
				++shadowCount;
				sumOpaqueShadow += lOpaque;
				sumGlassShadow += lGlass;
				if (lGlass > lOpaque + 0.03F) {
					++lighter;
				}
			}
		}
	}
	INFO("pixels d'ombre=" << shadowCount << " moyenne opaque=" << sumOpaqueShadow / shadowCount
	                       << " verre=" << sumGlassShadow / shadowCount << " eclaircis=" << lighter);
	// DoD : l'ombre translucide est moins sombre (densite, SPEC 5.2 F).
	REQUIRE(shadowCount > 0);
	REQUIRE(sumGlassShadow > sumOpaqueShadow);
	REQUIRE(lighter > 0);
}

TEST_CASE("spot cone (T058b) : dedans eclaire, dehors ambiant seul", "[t058][spot]") {
	// Spot etroit (15 degres) de (0,3,3) vers l'origine : la sphere centrale est
	// dans le cone, la laterale (3,0,0) est dehors (ambiant seul).
	const char* sceneText =
	    "scene { camera { position (0 1 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light spot { position (0 3 3) target (0 0 0) angle 15 "
	    "color (1 1 1) intensity 1.5 } } "
	    "objects { "
	    "object { type sphere center (0 0 0) radius 0.8 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } "
	    "object { type sphere center (3 0 0) radius 0.8 "
	    "material { albedo (0.2 0.2 0.8) diffuse 0.7 ambient 0.1 } } } }";
	rt::scene::Scene scene = parseOrDie(sceneText, "spot.rt");
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 64, .height = 48, .spp = 4, .seed = 0})
	            .isOk());
	// Centre (sphere axiale) : eclaire ; droite (hors cone) : sombre.
	// Balaye une petite fenetre autour des projections (jitter ±0.5 px).
	float centreMax = 0.0F;
	float sideMax = 0.0F;
	for (int y = 20; y < 28; ++y) {
		for (int x = 28; x < 36; ++x) {
			centreMax = std::max(centreMax, luminance(meanAt(fb, x, y)));
		}
	}
	for (int y = 20; y < 28; ++y) {
		for (int x = 44; x < 56; ++x) {
			sideMax = std::max(sideMax, luminance(meanAt(fb, x, y)));
		}
	}
	INFO("centre max=" << centreMax << " cote max=" << sideMax);
	REQUIRE(std::isfinite(centreMax));
	REQUIRE(std::isfinite(sideMax));
	REQUIRE(centreMax > 0.15F);
	REQUIRE(sideMax < 0.08F);
	REQUIRE(centreMax > sideMax + 0.08F);
	// Unitaire : `isInSpotCone` (dedans / dehors / degenere).
	const rt::lighting::SpotLightParams spot = rt::lighting::makeSpotParams(
	    rt::Vec3(0.0F, 3.0F, 3.0F), rt::Vec3(1.0F, 1.0F, 1.0F), 1.5F,
	    rt::Vec3(1.0F, 0.0F, 0.0F), 0.0F, rt::Vec3(0.0F, 0.0F, 0.0F), 15.0F);
	REQUIRE(rt::lighting::isInSpotCone(spot, rt::Vec3(0.0F, 0.0F, 0.0F)));
	REQUIRE_FALSE(rt::lighting::isInSpotCone(spot, rt::Vec3(3.0F, 0.0F, 0.0F)));
	REQUIRE_FALSE(rt::lighting::isInSpotCone(spot, rt::Vec3(0.0F, 3.0F, 3.0F)));
}

TEST_CASE("direct light (T058b) : spot face camera aveugle (saturation)", "[t058][direct]") {
	// `opt_direct.rt` : headlight sur la camera, intensite 3 -> blanc central.
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/opt_direct.rt");
	REQUIRE(parsed.isOk());
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(parsed.value(), fb, {.width = 48, .height = 36, .spp = 4, .seed = 0})
	            .isOk());
	const rt::Vec3 centre = meanAt(fb, 24, 18);
	INFO("centre r=" << centre.x << " g=" << centre.y << " b=" << centre.z);
	REQUIRE(std::isfinite(centre.x));
	// Aveuglement : saturation quasi blanche au centre (diffus frontal + spec).
	REQUIRE(centre.x > 0.85F);
	REQUIRE(centre.y > 0.85F);
	REQUIRE(centre.z > 0.85F);
	// Meme scene en ponctuelle faible (1.0) : pas de saturation (temoin).
	rt::scene::Scene dim = parseOrDie(
	    "scene { camera { position (0 1 4) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 1 4) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.5 0.5 0.5) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 specular 0.8 shininess 64 } } } }",
	    "dim.rt");
	rt::render::Framebuffer fbDim;
	REQUIRE(rt::render::render(dim, fbDim, {.width = 48, .height = 36, .spp = 4, .seed = 0})
	            .isOk());
	const rt::Vec3 dimCentre = meanAt(fbDim, 24, 18);
	INFO("faible r=" << dimCentre.x);
	REQUIRE(dimCentre.x < centre.x);
}

TEST_CASE("ombres translucides (T058a) : scene opt_shadow_transp.rt rend", "[t058][shadow-transp]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/opt_shadow_transp.rt");
	REQUIRE(parsed.isOk());
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(parsed.value(), fb, {.width = 48, .height = 36, .spp = 4, .seed = 0})
	            .isOk());
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 c = meanAt(fb, x, y);
			REQUIRE(std::isfinite(c.x));
			REQUIRE(std::isfinite(c.y));
			REQUIRE(std::isfinite(c.z));
		}
	}
}
