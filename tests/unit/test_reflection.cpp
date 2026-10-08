// Tests de la reflexion miroir (T056), Catch2.
// DoD : `reflectivity=0` identique au rendu sans miroir, `reflectivity=1` =
// reflet net, pas de boucle infinie (profondeur bornee `maxDepth`).

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

} // namespace

TEST_CASE("reflexion (T056) : reflectivity=0 identique au rendu sans miroir", "[reflection][t056]") {
	// Meme scene, l'une avec `reflectivity 0.0` explicite, l'autre sans champ
	// (defaut 0.0) : les deux doivent rendre octet par octet identique.
	const char* explicitZero =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 reflectivity 0.0 } } } }";
	const char* implicitZero =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 } } } }";
	rt::scene::Scene sceneExplicit = parseOrDie(explicitZero, "explicit.rt");
	rt::scene::Scene sceneImplicit = parseOrDie(implicitZero, "implicit.rt");
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbExplicit;
	rt::render::Framebuffer fbImplicit;
	REQUIRE(rt::render::render(sceneExplicit, fbExplicit, params).isOk());
	REQUIRE(rt::render::render(sceneImplicit, fbImplicit, params).isOk());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = fbExplicit.accumAt(x, y);
			const rt::Vec3 b = fbImplicit.accumAt(x, y);
			INFO("pixel (" << x << "," << y << ")");
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
		}
	}
	// Alias `reflect` pilote le meme champ (FORMAT section 5.6).
	rt::scene::Scene sceneAlias = parseOrDie(
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 reflect 0.0 } } } }",
	    "alias.rt");
	rt::render::Framebuffer fbAlias;
	REQUIRE(rt::render::render(sceneAlias, fbAlias, params).isOk());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			REQUIRE(fbAlias.accumAt(x, y).x == fbImplicit.accumAt(x, y).x);
		}
	}
}

TEST_CASE("reflexion (T056) : reflectivity=1 = reflet net (miroir pur)", "[reflection][t056]") {
	// Sphere grise devant fond bleu : mate = diffus gris-rouge, miroir pur =
	// reflet du fond (le rayon reflechi repart vers la camera et manque tout).
	const char* matteText =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 reflectivity 0.0 } } } }";
	const char* mirrorText =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 reflectivity 1.0 } } } }";
	rt::scene::Scene matte = parseOrDie(matteText, "matte.rt");
	rt::scene::Scene mirror = parseOrDie(mirrorText, "mirror.rt");
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbMatte;
	rt::render::Framebuffer fbMirror;
	REQUIRE(rt::render::render(matte, fbMatte, params).isOk());
	REQUIRE(rt::render::render(mirror, fbMirror, params).isOk());
	const rt::Vec3 matteCentre = meanAt(fbMatte, 16, 12);
	const rt::Vec3 mirrorCentre = meanAt(fbMirror, 16, 12);
	INFO("mate r=" << matteCentre.x << " b=" << matteCentre.z << " miroir r=" << mirrorCentre.x
	               << " b=" << mirrorCentre.z);
	REQUIRE(std::isfinite(mirrorCentre.x));
	REQUIRE(std::isfinite(mirrorCentre.y));
	REQUIRE(std::isfinite(mirrorCentre.z));
	// Mate : diffus rouge dominant (albedo rouge eclaire).
	REQUIRE(matteCentre.x > matteCentre.z);
	// Miroir pur : reflet du fond bleu (pas de diffus rouge).
	REQUIRE(mirrorCentre.z > mirrorCentre.x);
	REQUIRE(mirrorCentre.z == Catch::Approx(0.9F).margin(0.06F));
	REQUIRE(mirrorCentre.x == Catch::Approx(0.1F).margin(0.06F));
	// Les deux images different nettement au centre (miroir visible).
	REQUIRE(std::fabs(mirrorCentre.x - matteCentre.x) > 0.1F);
	// Ponderation continue : `reflect 0.5` est entre les deux bornes.
	rt::scene::Scene half = parseOrDie(
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.1 0.1 0.9) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 reflectivity 0.5 } } } }",
	    "half.rt");
	rt::render::Framebuffer fbHalf;
	REQUIRE(rt::render::render(half, fbHalf, params).isOk());
	const rt::Vec3 halfCentre = meanAt(fbHalf, 16, 12);
	INFO("demi r=" << halfCentre.x << " b=" << halfCentre.z);
	REQUIRE(halfCentre.x > mirrorCentre.x);
	REQUIRE(halfCentre.x < matteCentre.x);
}

TEST_CASE("reflexion (T056) : profondeur bornee, pas de boucle infinie", "[reflection][t056]") {
	// Deux miroirs face a face (plans paralleles) : sans borne, la recursion
	// serait infinie. Avec `maxDepth`, le rendu termine (DoD).
	const char* twoMirrors =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type plane point (0 0 -1) normal (0 0 1) "
	    "material { albedo (0.8 0.8 0.8) reflectivity 1.0 } } "
	    "object { type plane point (0 0 6) normal (0 0 -1) "
	    "material { albedo (0.8 0.8 0.8) reflectivity 1.0 } } "
	    "} }";
	rt::scene::Scene scene = parseOrDie(twoMirrors, "mirrors.rt");
	// `maxDepth 4` termine et produit des couleurs finies bornees.
	rt::render::Framebuffer fbBounded;
	REQUIRE(rt::render::render(scene, fbBounded,
	                           {.width = 16, .height = 12, .spp = 1, .maxDepth = 4, .seed = 0})
	            .isOk());
	for (int y = 0; y < 12; ++y) {
		for (int x = 0; x < 16; ++x) {
			const rt::Vec3 c = meanAt(fbBounded, x, y);
			REQUIRE(std::isfinite(c.x));
			REQUIRE(std::isfinite(c.y));
			REQUIRE(std::isfinite(c.z));
			REQUIRE(c.x >= 0.0F);
			REQUIRE(c.x <= 1.0F + 0.031F);
		}
	}
	// `maxDepth 0` desactive la reflexion : miroir `R=1` == mat (direct seul).
	const char* mirrorText =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 reflectivity 1.0 } } } }";
	const char* matteText =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 reflectivity 0.0 } } } }";
	rt::scene::Scene mirrorScene = parseOrDie(mirrorText, "m1.rt");
	rt::scene::Scene matteScene = parseOrDie(matteText, "m0.rt");
	rt::render::Framebuffer fbDepth0;
	rt::render::Framebuffer fbMatte;
	REQUIRE(rt::render::render(mirrorScene, fbDepth0,
	                           {.width = 32, .height = 24, .spp = 4, .maxDepth = 0, .seed = 0})
	            .isOk());
	REQUIRE(rt::render::render(matteScene, fbMatte,
	                           {.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0})
	            .isOk());
	for (int y = 0; y < 24; ++y) {
		for (int x = 0; x < 32; ++x) {
			REQUIRE(fbDepth0.accumAt(x, y).x == fbMatte.accumAt(x, y).x);
			REQUIRE(fbDepth0.accumAt(x, y).y == fbMatte.accumAt(x, y).y);
			REQUIRE(fbDepth0.accumAt(x, y).z == fbMatte.accumAt(x, y).z);
		}
	}
}
