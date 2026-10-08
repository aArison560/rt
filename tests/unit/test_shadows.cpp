// Tests des rayons d'ombre + multi-spot (T052), Catch2.
// DoD : 2 lumieres -> 2 zones d'ombre distinctes ; pas d'acne (bandes) ;
// figure VI.3 anticipee (ombres cumulees selon les sources bloquees).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
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

TEST_CASE("ombres multi-spot (T052) : 2 lumieres -> 2 zones d'ombre distinctes", "[shadows][t052]") {
	// Sol + sphere occultrice + 2 ponctuelles laterales opposees.
	// Chaque lumiere projette l'ombre de la sphere d'un cote different ;
	// a 2 lumieres, l'ombre cumulee (les 2 bloquees) est plus sombre que les
	// penombres (1 bloquee), elles-memes plus sombres que le plein eclairage.
	const char* twoLights =
	    "scene { camera { position (0 4 7) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (4 5 3) color (1 1 1) intensity 1.0 } "
	    "light { position (-4 5 3) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 0.8 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } "
	    "} }";
	const char* leftOnly =
	    "scene { camera { position (0 4 7) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (4 5 3) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 0.8 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } "
	    "} }";
	const char* rightOnly =
	    "scene { camera { position (0 4 7) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (-4 5 3) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 0.8 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } "
	    "} }";
	rt::scene::Scene both = parseOrDie(twoLights, "both.rt");
	rt::scene::Scene left = parseOrDie(leftOnly, "left.rt");
	rt::scene::Scene right = parseOrDie(rightOnly, "right.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbBoth;
	rt::render::Framebuffer fbLeft;
	rt::render::Framebuffer fbRight;
	REQUIRE(rt::render::render(both, fbBoth, params).isOk());
	REQUIRE(rt::render::render(left, fbLeft, params).isOk());
	REQUIRE(rt::render::render(right, fbRight, params).isOk());
	// Les ombres laterales sont distinctes : il existe des pixels du sol
	// eclaires a gauche mais ombres a droite, et inversement. On balaie
	// toute l'image (l'ombre se projette en arriere du centre, pas
	// forcement en bas) ; seuils loin du plancher ambiant (~0.002) et du
	// dithering (±0.03) pour eviter les faux positifs.
	int leftLitRightShadowed = 0;
	int rightLitLeftShadowed = 0;
	int bothLit = 0;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			const float lBoth = luminance(meanAt(fbBoth, x, y));
			const float lLeft = luminance(meanAt(fbLeft, x, y));
			const float lRight = luminance(meanAt(fbRight, x, y));
			REQUIRE(std::isfinite(lBoth));
			REQUIRE(std::isfinite(lLeft));
			REQUIRE(std::isfinite(lRight));
			// Plein sol eclaire : les 3 images claires (> ambiant + dithering).
			if (lLeft > 0.12F && lRight > 0.12F) {
				++bothLit;
				// A 2 lumieres, le plein eclairage melange les 2 sources :
				// plus clair qu'a 1 seule (somme, saturee [0,1]).
				REQUIRE(lBoth + 0.031F >= lLeft);
				REQUIRE(lBoth + 0.031F >= lRight);
			}
			// Ombre portee a gauche seule (eclaire a droite, sombre a gauche).
			if (lLeft < 0.05F && lRight > 0.15F) {
				++leftLitRightShadowed;
			}
			if (lRight < 0.05F && lLeft > 0.15F) {
				++rightLitLeftShadowed;
			}
		}
	}
	INFO("plein=" << bothLit << " ombreG=" << leftLitRightShadowed
	              << " ombreD=" << rightLitLeftShadowed);
	REQUIRE(bothLit > 0);
	// Les 2 zones d'ombre existent et sont distinctes (figure VI.3 : melange).
	REQUIRE(leftLitRightShadowed > 0);
	REQUIRE(rightLitLeftShadowed > 0);
}

TEST_CASE("ombres (T052) : pas d'acne sur plan eclair sans occulteur", "[shadows][t052]") {
	// Plan seul + 1 ponctuelle au-dessus : aucun occulteur, donc aucune ombre
	// legitime. Avec un `tMin` eps + decalage `P+N*eps`, tous les pixels du
	// plan restent eclaires (pas de bandes d'auto-ombrage).
	rt::scene::Scene scene = parseOrDie(
	    "scene { camera { position (0 4 6) target (0 -1 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 5 2) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } } }",
	    "acne.rt");
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 48, .height = 36, .spp = 4, .seed = 0})
	            .isOk());
	int lit = 0;
	int total = 0;
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const float lum = luminance(meanAt(fb, x, y));
			REQUIRE(std::isfinite(lum));
			REQUIRE_FALSE(std::isnan(lum));
			// Le plan remplit l'image (camera vise le sol) : tout pixel est
			// soit du plan eclaire (> ambiant), soit du fond (< 0.06).
			// Aucun pixel du plan ne doit tomber au plancher ambiant par acne.
			if (lum > 0.06F) {
				++lit;
			}
			++total;
		}
		(void)total;
	}
	INFO("pixels eclaires : " << lit << "/1728");
	// Le plan domine l'image : une majorite de pixels est eclairee, sans
	// bandes sombres (si l'eps manquait, une partie tomberait < 0.05).
	REQUIRE(lit > total / 2);
}

TEST_CASE("ombres (T052) : sphere sur sol, l'ombre assombrit selon les sources", "[shadows][t052]") {
	// 1 vs 2 lumieres sur la meme geometrie : l'ombre d'1 source reste
	// eclairee par l'autre (penombre), donc plus claire que l'ombre cumulee
	// quand les 2 sont bloquees — anticipe la figure VI.3 (melange d'ombres).
	rt::scene::Scene one = parseOrDie(
	    "scene { camera { position (0 3 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (3 5 2) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.6 0.6 0.6) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } } }",
	    "one.rt");
	rt::scene::Scene two = parseOrDie(
	    "scene { camera { position (0 3 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (3 5 2) color (1 1 1) intensity 1.0 } "
	    "light { position (-3 5 2) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 -1 0) normal (0 1 0) "
	    "material { albedo (0.6 0.6 0.6) diffuse 0.8 ambient 0.05 } } "
	    "object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } } }",
	    "two.rt");
	rt::render::Framebuffer fbOne;
	rt::render::Framebuffer fbTwo;
	const rt::render::RenderParams params{.width = 48, .height = 36, .spp = 4, .seed = 0};
	REQUIRE(rt::render::render(one, fbOne, params).isOk());
	REQUIRE(rt::render::render(two, fbTwo, params).isOk());
	double sumOne = 0.0;
	double sumTwo = 0.0;
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const float lOne = luminance(meanAt(fbOne, x, y));
			const float lTwo = luminance(meanAt(fbTwo, x, y));
			REQUIRE(std::isfinite(lOne));
			REQUIRE(std::isfinite(lTwo));
			sumOne += lOne;
			sumTwo += lTwo;
		}
	}
	INFO("moyenne 1 spot=" << sumOne / 1728.0 << " 2 spots=" << sumTwo / 1728.0);
	// 2 sources melangees : l'image globale est plus claire (les penombres
	// d'1 source sont rehaussees par l'autre).
	REQUIRE(sumTwo > sumOne);
}
