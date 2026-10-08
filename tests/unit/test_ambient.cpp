// Tests de l'ambiance globale pilotée par fichier (T054, *Ambiance light/++*), Catch2.
// DoD : luminosité minimale > 0 sur tous les pixels d'une scène éclairée et
// d'une scène sans lumière ; valeur modifiable par fichier, visible.

#include <catch2/catch_amalgamated.hpp>

#include <string>

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

// Luminosité d'affichage (post `present()`, gamma 2.2) en [0,1].
float displayLuminance(const rt::render::Framebuffer& fb, int x, int y, int width) {
	const std::size_t idx =
	    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
	const auto& px = fb.displayData()[idx];
	return (static_cast<float>(px.r) + static_cast<float>(px.g) + static_cast<float>(px.b)) /
	       (3.0F * 255.0F);
}

} // namespace

TEST_CASE("ambiance (T054) : scene eclairee, aucun pixel noir (DoD)", "[ambient][t054]") {
	rt::scene::Scene scene = parseOrDie(
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 material { "
	    "albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } } }",
	    "lit.rt");
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .seed = 0};
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	float minLum = 1.0F;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const float lum = displayLuminance(fb, x, y, params.width);
			if (lum < minLum) {
				minLum = lum;
			}
		}
	}
	INFO("min lum eclairee = " << minLum);
	// Aucun objet jamais totalement noir (SPECIFICATIONS §3.2, OPTIONS §2.3) :
	// meme le fond (0.02) encode > 0 apres gamma, et l'objet recoit
	// ambiant + diffus + speculaire (T053).
	REQUIRE(minLum > 0.0F);
}

TEST_CASE("ambiance (T054) : scene sans lumiere, aucun pixel noir (DoD)", "[ambient][t054]") {
	// 0 lumière = ambiant seul (FORMAT §5.4, valide). Ambiance franche
	// (0.2 × 1.0, materiau 0.5) pour un plancher ~0.08, bien au-dessus du
	// dithering T036 (±0.03) et de la quantification 8 bits.
	rt::scene::Scene scene = parseOrDie(
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.05 0.05 0.08) } "
	    "ambient { color (0.2 0.2 0.25) intensity 1.0 } "
	    "lights { } "
	    "objects { object { type sphere center (0 0 0) radius 1 material { "
	    "albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.5 } } } }",
	    "unlit.rt");
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .seed = 0};
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	float minLum = 1.0F;
	int blackCount = 0;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const float lum = displayLuminance(fb, x, y, params.width);
			if (lum < minLum) {
				minLum = lum;
			}
			if (lum <= 0.0F) {
				++blackCount;
			}
		}
	}
	INFO("min lum sans lumiere = " << minLum << " noirs = " << blackCount);
	REQUIRE(blackCount == 0);
	REQUIRE(minLum > 0.0F);
}

TEST_CASE("ambiance (T054) : pilotable depuis le fichier, visible (DoD)", "[ambient][t054]") {
	// Meme geometrie, seule `ambient.intensity` change (0.5 vs 2.0) :
	// l'image s'eclaircit (fichier -> pixels, sans recompiler).
	auto renderCentre = [](const char* intensity) {
		std::string content =
		    std::string("scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
		                "background { color (0.02 0.02 0.05) } "
		                "ambient { color (0.06 0.06 0.08) intensity ") +
		    intensity +
		    " } lights { } objects { object { type sphere center (0 0 0) radius 1 material { "
		    "albedo (0.8 0.8 0.8) diffuse 0.0 ambient 0.5 } } } }";
		rt::scene::Scene scene = parseOrDie(content, "amb.rt");
		rt::render::Framebuffer fb;
		REQUIRE(rt::render::render(scene, fb, {.width = 32, .height = 24, .spp = 4, .seed = 0})
		            .isOk());
		// Centre de la sphere (16,12) : que de l'ambiant (diffuse 0).
		return displayLuminance(fb, 16, 12, 32);
	};
	const float dim = renderCentre("0.5");
	const float bright = renderCentre("2.0");
	INFO("ambiant 0.5 -> " << dim << " 2.0 -> " << bright);
	REQUIRE(bright > dim + 0.05F);
	// Le schema borne l'intensite 0..10 (R1) : hors borne = erreur propre.
	rt::Result<rt::scene::Scene> bad = rt::scene::parseContent(
	    "scene { camera { position (0 1 4) target (0 0 0) } "
	    "ambient { color (0.06 0.06 0.08) intensity 99.0 } "
	    "objects { object { type sphere } } }",
	    "bad.rt");
	REQUIRE(bad.isError());
}
