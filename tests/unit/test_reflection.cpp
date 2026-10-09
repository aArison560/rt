// Tests de la reflexion miroir (T056), Catch2.
// DoD : `reflectivity = 0` identique au rendu sans miroir (octet par octet),
// `reflectivity = 1` = reflet net (miroir pur, aucune part diffuse),
// profondeur bornee par `max_depth` (pas de boucle infinie entre 2 miroirs
// face a face, `max_depth` pilote depuis le fichier).

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

const char* kCamera = "camera { position (0 0 5) target (0 0 0) fov 60 } ";
const char* kBackground = "background { color (0.02 0.02 0.05) } ";
const char* kAmbient = "ambient { color (0.06 0.06 0.08) intensity 1.0 } ";
const char* kLight = "lights { light { position (0 3 5) color (1 1 1) intensity 1.5 } } ";

std::string sphereScene(const std::string& materialExtra) {
	std::string out = "scene { ";
	out += kCamera;
	out += kBackground;
	out += kAmbient;
	out += kLight;
	out += "objects { object { type sphere center (0 0 0) radius 1 material { ";
	out += "albedo (0.9 0.1 0.1) diffuse 0.8 ambient 0.1 specular 0.0 shininess 32 ";
	out += materialExtra;
	out += " } } } }";
	return out;
}

} // namespace

TEST_CASE("reflexion (T056) : reflectivity=0 identique au rendu sans miroir", "[reflection][t056]") {
	// Meme geometrie, meme lumiere : l'une sans champ `reflectivity`
	// (defaut 0), l'autre avec `reflectivity 0.0` explicite. Les deux
	// doivent rendre l'octet identique (le chemin `R = 0` = direct seul).
	rt::scene::Scene without = parseOrDie(sphereScene(""), "matte_default.rt");
	rt::scene::Scene explicitZero = parseOrDie(sphereScene("reflectivity 0.0"), "matte_zero.rt");
	REQUIRE(without.limits.maxDepth == 4);
	const rt::render::RenderParams params{.width = 48, .height = 36, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbDefault;
	rt::render::Framebuffer fbZero;
	REQUIRE(rt::render::render(without, fbDefault, params).isOk());
	REQUIRE(rt::render::render(explicitZero, fbZero, params).isOk());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = fbDefault.accumAt(x, y);
			const rt::Vec3 b = fbZero.accumAt(x, y);
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
		}
	}
	// `R = 0` ignore `maxDepth` : profondeur 0 ou 8, meme image.
	const rt::render::RenderParams depth0{.width = 48, .height = 36, .spp = 4, .maxDepth = 0, .seed = 0};
	const rt::render::RenderParams depth8{.width = 48, .height = 36, .spp = 4, .maxDepth = 8, .seed = 0};
	rt::render::Framebuffer fbD0;
	rt::render::Framebuffer fbD8;
	REQUIRE(rt::render::render(explicitZero, fbD0, depth0).isOk());
	REQUIRE(rt::render::render(explicitZero, fbD8, depth8).isOk());
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			REQUIRE(fbD0.accumAt(x, y).x == fbD8.accumAt(x, y).x);
			REQUIRE(fbD0.accumAt(x, y).y == fbD8.accumAt(x, y).y);
			REQUIRE(fbD0.accumAt(x, y).z == fbD8.accumAt(x, y).z);
		}
	}
}

TEST_CASE("reflexion (T056) : reflectivity=1 = reflet net (miroir pur)", "[reflection][t056]") {
	// Sphere rouge mate vs sphere miroir pur, meme geometrie/lumiere.
	// Le rayon primaire au centre frappe la face avant (normale +Z) et le
	// rayon reflechi repart vers la camera (miss -> fond sombre) : le miroir
	// montre le fond, pas sa propre couleur (aucune part diffuse).
	rt::scene::Scene matte = parseOrDie(sphereScene("reflectivity 0.0"), "matte.rt");
	rt::scene::Scene mirror = parseOrDie(sphereScene("reflectivity 1.0"), "mirror.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbMatte;
	rt::render::Framebuffer fbMirror;
	REQUIRE(rt::render::render(matte, fbMatte, params).isOk());
	REQUIRE(rt::render::render(mirror, fbMirror, params).isOk());
	const rt::Vec3 matteMean = meanAt(fbMatte, 32, 24);
	const rt::Vec3 mirrorMean = meanAt(fbMirror, 32, 24);
	INFO("matte centre r=" << matteMean.x << " g=" << matteMean.y << " b=" << matteMean.z);
	INFO("miroir centre r=" << mirrorMean.x << " g=" << mirrorMean.y << " b=" << mirrorMean.z);
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			REQUIRE(std::isfinite(fbMirror.accumAt(x, y).x));
			REQUIRE(std::isfinite(fbMirror.accumAt(x, y).y));
			REQUIRE(std::isfinite(fbMirror.accumAt(x, y).z));
		}
	}
	// Mate : rouge dominant au centre (diffus eclaire, `specular 0`).
	REQUIRE(matteMean.x > matteMean.z);
	REQUIRE(matteMean.x > 0.15F);
	// Miroir pur : proche du fond sombre (0.02, 0.02, 0.05) ± dithering
	// (±0.03 par echantillon, moyenne sur 4 -> marge 0.05 large).
	REQUIRE(mirrorMean.x == Catch::Approx(0.02F).margin(0.06));
	REQUIRE(mirrorMean.z == Catch::Approx(0.05F).margin(0.06));
	REQUIRE(luminance(mirrorMean) < 0.10F);
	// Les deux centres different nettement : le reflet net n'a rien de mat.
	REQUIRE((matteMean.x - mirrorMean.x) > 0.15F);
}

TEST_CASE("reflexion (T056) : profondeur bornee, pas de boucle infinie", "[reflection][t056]") {
	// Deux plans miroirs face a face, camera entre les deux : sans borne,
	// le rayon rebondirait sans fin (A -> B -> A -> ...). `maxDepth`
	// coupe la recursion (pile bornee 0..32, R2/R3).
	const char* facing =
	    "scene { camera { position (0 0 1) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } "
	    "lights { light { position (0 5 1) color (1 1 1) intensity 1.0 } } "
	    "limits { max_depth 8 } "
	    "objects { "
	    "object { type plane point (0 0 0) normal (0 0 1) "
	    "material { albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 specular 0.0 reflectivity 1.0 } } "
	    "object { type plane point (0 0 2) normal (0 0 -1) "
	    "material { albedo (0.2 0.2 0.8) diffuse 0.7 ambient 0.1 specular 0.0 reflectivity 1.0 } } "
	    "} }";
	rt::scene::Scene scene = parseOrDie(facing, "facing.rt");
	// Le parametre `max_depth` du fichier est bien lu (profondeur pilotee
	// depuis la scene, comme `main` le propage dans `RenderParams`).
	REQUIRE(scene.limits.maxDepth == 8);
	const rt::render::RenderParams deep{.width = 48,
	                                    .height = 36,
	                                    .spp = 4,
	                                    .maxDepth = scene.limits.maxDepth,
	                                    .seed = 0};
	rt::render::Framebuffer fbDeep;
	// Termine (pas de boucle infinie), sans NaN.
	REQUIRE(rt::render::render(scene, fbDeep, deep).isOk());
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 a = fbDeep.accumAt(x, y);
			REQUIRE(std::isfinite(a.x));
			REQUIRE(std::isfinite(a.y));
			REQUIRE(std::isfinite(a.z));
			REQUIRE_FALSE(std::isnan(a.x));
		}
	}
	// `maxDepth = 0` (direct seul) vs `maxDepth = 8` (miroirs) : la
	// reflexion change l'image (la borne a un effet observable).
	const rt::render::RenderParams flat{.width = 48, .height = 36, .spp = 4, .maxDepth = 0, .seed = 0};
	rt::render::Framebuffer fbFlat;
	REQUIRE(rt::render::render(scene, fbFlat, flat).isOk());
	int different = 0;
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 a = fbFlat.accumAt(x, y);
			const rt::Vec3 b = fbDeep.accumAt(x, y);
			const float dx = a.x - b.x;
			const float dy = a.y - b.y;
			const float dz = a.z - b.z;
			if ((dx * dx + dy * dy + dz * dz) > 0.001F * 0.001F) {
				++different;
			}
		}
	}
	INFO("pixels differents profondeur 0 vs 8 : " << different);
	REQUIRE(different > 0);
	// Profondeur max du moteur (16, au-dela du schema 0..16) termine aussi.
	const rt::render::RenderParams deepest{.width = 32, .height = 24, .spp = 2, .maxDepth = 16, .seed = 0};
	rt::render::Framebuffer fbDeepest;
	REQUIRE(rt::render::render(scene, fbDeepest, deepest).isOk());
}

TEST_CASE("reflexion (T056) : pourcentage continu 0.5 = melange diffus/miroir", "[reflection][t056]") {
	// `out = direct * (1 - R) + reflechi * R` : a 0.5, le centre vaut la
	// moyenne des deux bornes (ponderation correcte avec le diffus).
	rt::scene::Scene matte = parseOrDie(sphereScene("reflectivity 0.0"), "half0.rt");
	rt::scene::Scene half = parseOrDie(sphereScene("reflectivity 0.5"), "half05.rt");
	rt::scene::Scene mirror = parseOrDie(sphereScene("reflectivity 1.0"), "half1.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fb0;
	rt::render::Framebuffer fbHalf;
	rt::render::Framebuffer fb1;
	REQUIRE(rt::render::render(matte, fb0, params).isOk());
	REQUIRE(rt::render::render(half, fbHalf, params).isOk());
	REQUIRE(rt::render::render(mirror, fb1, params).isOk());
	const rt::Vec3 c0 = meanAt(fb0, 32, 24);
	const rt::Vec3 cHalf = meanAt(fbHalf, 32, 24);
	const rt::Vec3 c1 = meanAt(fb1, 32, 24);
	INFO("R=0 r=" << c0.x << " R=0.5 r=" << cHalf.x << " R=1 r=" << c1.x);
	// Ordre : mat (clair, rouge) > demi > miroir (sombre, fond).
	REQUIRE(c0.x > cHalf.x);
	REQUIRE(cHalf.x > c1.x);
	// Moyenne a 0.05 pres (meme jitter + meme dithering : deterministe).
	const float expected = (c0.x + c1.x) * 0.5F;
	REQUIRE(cHalf.x == Catch::Approx(expected).margin(0.05));
	const float expectedG = (c0.y + c1.y) * 0.5F;
	REQUIRE(cHalf.y == Catch::Approx(expectedG).margin(0.05));
}
