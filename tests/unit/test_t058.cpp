// Tests T058 : ombres transparentes + direct light (spot aveuglant), Catch2.
// DoD : 2 scènes de preuve (`scenes/opt_transparent_shadow.rt`,
// `scenes/opt_direct.rt`) + test de densité d'ombre (ombre translucide >
// ombre opaque en luminosité).

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

// Sol + sphère occultrice à l'origine + ponctuelle latérale. Seul le
// matériau de l'occultrice change (opaque vs verre) : la géométrie d'ombre
// est donc identique, seule la densité change (DoD).
std::string shadowScene(const std::string& occluderMaterial) {
	std::string out = "scene { camera { position (0 3 6) target (0 0 0) fov 50 } ";
	out += "background { color (0.02 0.02 0.05) } ";
	out += "ambient { color (0.06 0.06 0.08) intensity 1.0 } ";
	out += "lights { light { position (3 5 2) color (1 1 1) intensity 1.2 } } ";
	out += "objects { ";
	out += "object { type plane point (0 -1 0) normal (0 1 0) material { ";
	out += "albedo (0.6 0.6 0.6) diffuse 0.8 ambient 0.05 } } ";
	out += "object { type sphere center (0 0 0) radius 1 material { ";
	out += occluderMaterial;
	out += " } } } }";
	return out;
}

} // namespace

TEST_CASE("ombres transparentes (T058) : translucide moins sombre qu'opaque",
          "[t058][shadows]") {
	// Même géométrie, même lumière : occultrice opaque (`transparency 0`,
	// défaut) vs verre (`transparency 0.8, ior 1.5`, validation T024 : `ior >
	// 1` exigé). L'ombre portée sur le sol doit être éclaircie par la
	// transmission (`trans_eff = 0.8 * 1.5/1.5 = 0.8`, voir
	// `render::shadowTransmittance`).
	rt::scene::Scene opaque = parseOrDie(
	    shadowScene("albedo (0.9 0.1 0.1) diffuse 0.7 ambient 0.1"), "opaque.rt");
	rt::scene::Scene glass = parseOrDie(
	    shadowScene("albedo (0.9 0.95 1.0) diffuse 0.3 ambient 0.05 transparency 0.8 ior 1.5"),
	    "glass.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbOpaque;
	rt::render::Framebuffer fbGlass;
	REQUIRE(rt::render::render(opaque, fbOpaque, params).isOk());
	REQUIRE(rt::render::render(glass, fbGlass, params).isOk());
	// Pixels d'ombre : sombres en opaque (< 0.08, au-dessus du plancher
	// ambiant ~0.002 mais sous le plein éclairage > 0.15, loin du dithering
	// ±0.03). Parmi eux, le verre doit être nettement plus clair (DoD).
	int shadowPixels = 0;
	double sumOpaqueShadow = 0.0;
	double sumGlassShadow = 0.0;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			const float lOpaque = luminance(meanAt(fbOpaque, x, y));
			const float lGlass = luminance(meanAt(fbGlass, x, y));
			REQUIRE(std::isfinite(lOpaque));
			REQUIRE(std::isfinite(lGlass));
			REQUIRE_FALSE(std::isnan(lOpaque));
			if (lOpaque < 0.08F) {
				++shadowPixels;
				sumOpaqueShadow += lOpaque;
				sumGlassShadow += lGlass;
			}
		}
	}
	INFO("pixels d'ombre (opaque<0.08) : " << shadowPixels);
	REQUIRE(shadowPixels > 0);
	const double meanOpaque = sumOpaqueShadow / static_cast<double>(shadowPixels);
	const double meanGlass = sumGlassShadow / static_cast<double>(shadowPixels);
	INFO("moyenne ombre opaque=" << meanOpaque << " verre=" << meanGlass);
	// L'ombre translucide est plus claire (marge 0.03 > dithering).
	REQUIRE(meanGlass > meanOpaque + 0.03);
	// Le verre ne supprime pas l'ombre (transmission 0.8, pas 1) : elle reste
	// sous le plein éclairage. Et l'opaque reste sombre (binaire conservé).
	REQUIRE(meanOpaque < 0.08);
	REQUIRE(meanGlass < 0.30);
}

TEST_CASE("ombres transparentes (T058) : ior dense absorbe plus (1.5 vs 3.0)",
          "[t058][shadows]") {
	// Même `transparency 0.8`, `ior` 1.5 (verre) vs 3.0 (dense) : `trans_eff =
	// 0.8 * 1.5/ior` -> 0.8 vs 0.4, l'ombre dense est plus sombre que le verre
	// mais reste plus claire que l'opaque (proportionnel à `transparency` et
	// à `ior`, Prompt T058).
	rt::scene::Scene glass15 = parseOrDie(
	    shadowScene("albedo (0.9 0.95 1.0) diffuse 0.3 ambient 0.05 transparency 0.8 ior 1.5"),
	    "ior15.rt");
	rt::scene::Scene glass30 = parseOrDie(
	    shadowScene("albedo (0.9 0.95 1.0) diffuse 0.3 ambient 0.05 transparency 0.8 ior 3.0"),
	    "ior30.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fb15;
	rt::render::Framebuffer fb30;
	REQUIRE(rt::render::render(glass15, fb15, params).isOk());
	REQUIRE(rt::render::render(glass30, fb30, params).isOk());
	double sum15 = 0.0;
	double sum30 = 0.0;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			sum15 += luminance(meanAt(fb15, x, y));
			sum30 += luminance(meanAt(fb30, x, y));
		}
	}
	const double mean15 = sum15 / (64.0 * 48.0);
	const double mean30 = sum30 / (64.0 * 48.0);
	INFO("moyenne ior=1.5 : " << mean15 << " ior=3.0 : " << mean30);
	REQUIRE(std::isfinite(mean15));
	REQUIRE(std::isfinite(mean30));
	// Le verre léger transmet plus que le dense (marge au-delà du dithering).
	REQUIRE(mean15 > mean30 + 0.005);
}

TEST_CASE("spot (T058) : cone dedans/dehors + degeneres definis", "[t058][spot]") {
	// Niveau unitaire (`lighting::spotConeFactor`, formule OPTIONS_GUIDE §3.2 :
	// `cosAngle = dot(toFrag, axis)`, `cutoff = cos(angle)`).
	const rt::Vec3 pos(0.0F, 5.0F, 0.0F);
	const rt::Vec3 tgt(0.0F, 0.0F, 0.0F);
	// Centre du cône (sur l'axe) -> 1 (plein).
	REQUIRE(rt::lighting::spotConeFactor(rt::Vec3(0.0F, 0.0F, 0.0F), pos, tgt, 30.0F) ==
	        Catch::Approx(1.0F).margin(1e-5));
	// Hors cône (90° de l'axe) -> 0.
	REQUIRE(rt::lighting::spotConeFactor(rt::Vec3(5.0F, 5.0F, 0.0F), pos, tgt, 30.0F) ==
	        0.0F);
	// Dégénérés -> 0 (défini, jamais de NaN) : angle 0, cible confondue.
	REQUIRE(rt::lighting::spotConeFactor(rt::Vec3(0.0F, 0.0F, 0.0F), pos, tgt, 0.0F) == 0.0F);
	REQUIRE(rt::lighting::spotConeFactor(rt::Vec3(0.0F, 0.0F, 0.0F), pos, pos, 30.0F) == 0.0F);
	const float nan = std::numeric_limits<float>::quiet_NaN();
	REQUIRE(rt::lighting::spotConeFactor(rt::Vec3(nan, nan, nan), pos, tgt, 30.0F) == 0.0F);
	// Axe : cible - position normalisée, confondus -> nul.
	const rt::Vec3 axis = rt::lighting::spotAxis(pos, tgt);
	REQUIRE(axis.y == Catch::Approx(-1.0F).margin(1e-5));
	const rt::Vec3 nullAxis = rt::lighting::spotAxis(pos, pos);
	REQUIRE(nullAxis.x == 0.0F);
	REQUIRE(nullAxis.y == 0.0F);
}

TEST_CASE("direct light (T058) : spot face camera aveugle (sature)", "[t058][direct]") {
	// Spot derrière la scène visant la caméra (observateur dans le cône) :
	// les rayons manqués qui visent la source saturent en blanc. Même scène
	// avec le spot visant ailleurs (hors caméra) : pas d'aveuglement.
	const char* head =
	    "scene { camera { position (0 1 5) target (0 1.5 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } ";
	const char* tail =
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) material { "
	    "albedo (0.6 0.6 0.6) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (1.5 0 0) radius 0.8 material { "
	    "albedo (0.9 0.1 0.1) diffuse 0.7 ambient 0.1 } } } }";
	rt::scene::Scene facing = parseOrDie(
	    std::string(head) +
	        "lights { light spot \"face\" { position (0 1 -2) target (0 1 5) angle 30 "
	        "color (1 1 1) intensity 5.0 } } " + tail,
	    "facing.rt");
	rt::scene::Scene away = parseOrDie(
	    std::string(head) +
	        "lights { light spot \"side\" { position (0 1 -2) target (0 -1 -2) angle 30 "
	        "color (1 1 1) intensity 5.0 } } " + tail,
	    "away.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbFacing;
	rt::render::Framebuffer fbAway;
	REQUIRE(rt::render::render(facing, fbFacing, params).isOk());
	REQUIRE(rt::render::render(away, fbAway, params).isOk());
	const rt::Vec3 centerFacing = meanAt(fbFacing, 32, 24);
	const rt::Vec3 centerAway = meanAt(fbAway, 32, 24);
	INFO("centre face=" << luminance(centerFacing) << " à l'opposé=" << luminance(centerAway));
	REQUIRE(std::isfinite(centerFacing.x));
	REQUIRE(std::isfinite(centerAway.x));
	// Aveuglement : le centre face au spot sature (proche de 1, blanc).
	REQUIRE(luminance(centerFacing) > 0.7F);
	REQUIRE(centerFacing.x > 0.9F);
	REQUIRE(centerFacing.y > 0.9F);
	// À l'opposé : pas d'aveuglement, le centre reste sombre (fond/ambiant).
	REQUIRE(luminance(centerAway) < 0.3F);
	REQUIRE((luminance(centerFacing) - luminance(centerAway)) > 0.4F);
}

TEST_CASE("direct light (T058) : scenes de preuve rendent (DoD)", "[t058][direct]") {
	// Les 2 scènes du DoD existent, parsent et rendent sans erreur.
	rt::Result<rt::scene::Scene> trans = rt::scene::parseFile("scenes/opt_transparent_shadow.rt");
	REQUIRE(trans.isOk());
	rt::Result<rt::scene::Scene> direct = rt::scene::parseFile("scenes/opt_direct.rt");
	REQUIRE(direct.isOk());
	REQUIRE(trans.value().totalObjectCount() >= 3);
	REQUIRE(direct.value().lights.size() >= 1);
	rt::render::Framebuffer fbTrans;
	rt::render::Framebuffer fbDirect;
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 2, .seed = 0};
	REQUIRE(rt::render::render(trans.value(), fbTrans, params).isOk());
	REQUIRE(rt::render::render(direct.value(), fbDirect, params).isOk());
	bool brightTrans = false;
	bool brightDirect = false;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			REQUIRE(std::isfinite(fbTrans.accumAt(x, y).x));
			REQUIRE(std::isfinite(fbDirect.accumAt(x, y).x));
			REQUIRE_FALSE(std::isnan(fbTrans.accumAt(x, y).x));
			REQUIRE_FALSE(std::isnan(fbDirect.accumAt(x, y).x));
			if (luminance(meanAt(fbTrans, x, y)) > 0.05F) {
				brightTrans = true;
			}
			if (luminance(meanAt(fbDirect, x, y)) > 0.7F) {
				brightDirect = true;
			}
		}
	}
	// Transparente : image lisible (sol + sphères éclairés).
	REQUIRE(brightTrans);
	// Directe : au moins un pixel aveuglé (saturé, centre).
	REQUIRE(brightDirect);
}
