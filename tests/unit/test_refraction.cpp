// Tests de la transparence / refraction (T057, Snell/Descartes), Catch2.
// DoD : `ior=1` -> pas de deviation, sortie verre->air coherente (dont TIR),
// formule commentee dans `Renderer.cpp`, scene `scenes/opt_glass.rt`.

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <string>

#include "rt/base/Vec.hpp"
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

} // namespace

TEST_CASE("refraction (T057) : ior=1 pas de deviation (Descartes n1=n2)", "[refraction][t057]") {
	// Incidence normale : `refract(I, N, 1)` == I (aucune deviation).
	const rt::Vec3 normal(0.0F, 1.0F, 0.0F);
	const rt::Vec3 incident(0.0F, -1.0F, 0.0F);
	const rt::Vec3 straight = rt::refract(incident, normal, 1.0F);
	REQUIRE_FALSE(rt::nearZero(straight));
	REQUIRE(straight.x == Catch::Approx(incident.x).margin(1e-5F));
	REQUIRE(straight.y == Catch::Approx(incident.y).margin(1e-5F));
	REQUIRE(straight.z == Catch::Approx(incident.z).margin(1e-5F));
	// Incidence oblique 45 degres : `ratio = 1` ne devie pas non plus.
	const rt::Vec3 oblique = rt::normalize(rt::Vec3(0.0F, -1.0F, -1.0F));
	const rt::Vec3 obliqueOut = rt::refract(oblique, normal, 1.0F);
	REQUIRE_FALSE(rt::nearZero(obliqueOut));
	REQUIRE(obliqueOut.x == Catch::Approx(oblique.x).margin(1e-5F));
	REQUIRE(obliqueOut.y == Catch::Approx(oblique.y).margin(1e-5F));
	REQUIRE(obliqueOut.z == Catch::Approx(oblique.z).margin(1e-5F));
}

TEST_CASE("refraction (T057) : entree/sortie coherentes + reflexion totale interne",
          "[refraction][t057]") {
	// Surface `N = +z`, rayon incident a 60 degres de la normale (`cos = 0.5`).
	const rt::Vec3 normal(0.0F, 0.0F, 1.0F);
	const rt::Vec3 incident = rt::normalize(rt::Vec3(0.8660254F, 0.0F, -0.5F));
	REQUIRE_FALSE(rt::nearZero(incident));
	// Entree air -> verre (`ratio = 1/1.5`) : se rapproche de la normale
	// (`|z|` transmis > `|z|` incident, penche vers `-N`).
	const rt::Vec3 entering = rt::refract(incident, normal, 1.0F / 1.5F);
	REQUIRE_FALSE(rt::nearZero(entering));
	REQUIRE(std::isfinite(entering.x));
	// `z` transmis ≈ -0.816 (calcule : (0.577, 0, -0.816)), plus normal que `-0.5`.
	REQUIRE(entering.z < incident.z);
	REQUIRE(entering.z == Catch::Approx(-0.816F).margin(0.01F));
	REQUIRE(entering.x == Catch::Approx(0.577F).margin(0.01F));
	// Sortie verre -> air au meme angle (`ratio = 1.5`) : au-dela de l'angle
	// critique (41.8 degres) -> reflexion totale interne (sentinelle nulle).
	const rt::Vec3 tir = rt::refract(incident, normal, 1.5F);
	REQUIRE(rt::nearZero(tir));
	// Sortie a incidence douce (cos = 0.9, ~25 degres) : s'ecarte de la normale,
	// fini et normalise.
	const rt::Vec3 gentle = rt::normalize(rt::Vec3(0.4358899F, 0.0F, -0.9F));
	const rt::Vec3 exiting = rt::refract(gentle, normal, 1.5F);
	REQUIRE_FALSE(rt::nearZero(exiting));
	REQUIRE(std::isfinite(exiting.x));
	REQUIRE(std::fabs(exiting.x) > std::fabs(gentle.x));
}

TEST_CASE("refraction (T057) : transparence montre le fond derriere", "[refraction][t057]") {
	// Pas d'objet derriere (ombres opaques en T057, T058 les affinera) : devant
	// opaque gris vs devant verre (`Tr 0.9`) face a un fond bleu. Le verre
	// transmet le fond (miss -> fond), l'opaque montre son diffus gris.
	const char* opaqueText =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 transparency 0.0 ior 1.0 } } } }";
	const char* glassText =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.9 0.9 0.9) diffuse 0.2 transparency 0.9 ior 1.5 } } } }";
	rt::scene::Scene opaque = parseOrDie(opaqueText, "opaque.rt");
	rt::scene::Scene glass = parseOrDie(glassText, "glass.rt");
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbOpaque;
	rt::render::Framebuffer fbGlass;
	REQUIRE(rt::render::render(opaque, fbOpaque, params).isOk());
	REQUIRE(rt::render::render(glass, fbGlass, params).isOk());
	const rt::Vec3 opaqueCentre = meanAt(fbOpaque, 16, 12);
	const rt::Vec3 glassCentre = meanAt(fbGlass, 16, 12);
	INFO("opaque r=" << opaqueCentre.x << " b=" << opaqueCentre.z << " verre r=" << glassCentre.x
	                 << " b=" << glassCentre.z);
	REQUIRE(std::isfinite(glassCentre.x));
	REQUIRE(std::isfinite(glassCentre.y));
	REQUIRE(std::isfinite(glassCentre.z));
	// Le verre transmet le fond bleu : plus bleu et moins rouge que l'opaque.
	REQUIRE(glassCentre.z > opaqueCentre.z);
	REQUIRE(glassCentre.x < opaqueCentre.x);
	// `ior` pilote la deviation : `1.1` vs `1.5` (meme `Tr`) donnent 2 images
	// differentes (au moins un pixel differe au-dela du dithering).
	// `ior` pilote la deviation : meme scene avec bille rouge derriere (le decalage
	// par `ior` echantillonne un point different de la bille, meme ombree en T057),
	// `1.1` vs `1.5` donnent 2 images differentes au-dela du dithering.
	const char* behind15 =
	    "scene { camera { position (0 1 4) target (0 0 -1) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (1.2 0 -2) radius 0.5 "
	    "material { albedo (0.9 0.1 0.1) diffuse 0.7 } } "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.9 0.9 0.9) diffuse 0.2 transparency 0.9 ior 1.5 } } } }";
	const char* behind11 =
	    "scene { camera { position (0 1 4) target (0 0 -1) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (1.2 0 -2) radius 0.5 "
	    "material { albedo (0.9 0.1 0.1) diffuse 0.7 } } "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.9 0.9 0.9) diffuse 0.2 transparency 0.9 ior 1.1 } } } }";
	rt::scene::Scene scene15 = parseOrDie(behind15, "behind15.rt");
	rt::scene::Scene glass11 = parseOrDie(behind11, "glass11.rt");
	rt::render::Framebuffer fb15;
	rt::render::Framebuffer fbGlass11;
	REQUIRE(rt::render::render(scene15, fb15, params).isOk());
	REQUIRE(rt::render::render(glass11, fbGlass11, params).isOk());
	bool different = false;
	for (int y = 0; y < params.height && !different; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = meanAt(fb15, x, y);
			const rt::Vec3 b = meanAt(fbGlass11, x, y);
			if (std::fabs(a.x - b.x) > 0.031F || std::fabs(a.y - b.y) > 0.031F ||
			    std::fabs(a.z - b.z) > 0.031F) {
				different = true;
				break;
			}
		}
	}
	INFO("ior 1.5 vs 1.1 differents : " << different);
	REQUIRE(different);
	// `maxDepth 0` coupe la transmission : verre `Tr 0.9` == direct (opaque-like).
	rt::render::Framebuffer fbDepth0;
	REQUIRE(rt::render::render(glass, fbDepth0,
	                           {.width = 32, .height = 24, .spp = 4, .maxDepth = 0, .seed = 0})
	            .isOk());
	const rt::Vec3 depth0Centre = meanAt(fbDepth0, 16, 12);
	INFO("profondeur0 r=" << depth0Centre.x << " verre r=" << glassCentre.x);
	REQUIRE(std::fabs(depth0Centre.x - glassCentre.x) > 0.03F);
}

TEST_CASE("refraction (T057) : scene opt_glass.rt rend sans erreur", "[refraction][t057]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/opt_glass.rt");
	REQUIRE(parsed.isOk());
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(parsed.value(), fb, {.width = 48, .height = 36, .spp = 4, .seed = 0})
	            .isOk());
	int different = 0;
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 c = meanAt(fb, x, y);
			REQUIRE(std::isfinite(c.x));
			REQUIRE(std::isfinite(c.y));
			REQUIRE(std::isfinite(c.z));
			const float dr = c.x - parsed.value().background.color.x;
			const float dg = c.y - parsed.value().background.color.y;
			const float db = c.z - parsed.value().background.color.z;
			if ((dr * dr + dg * dg + db * db) > 0.05F * 0.05F) {
				++different;
			}
		}
	}
	INFO("non-fond : " << different << "/1728");
	REQUIRE(different > (48 * 36) / 20);
}
