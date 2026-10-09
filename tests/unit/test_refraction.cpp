// Tests de la transparence et de la refraction (T057), Catch2.
// DoD : `transparency = 0` identique au rendu opaque (octet par octet,
// insensible a `ior` et a `maxDepth`), `ior = 1` -> aucune deviation
// (Descartes `eta = 1`), sortie objet -> courbure exterieure coherente,
// reflexion totale interne repliee sur le miroir (jamais de NaN/trou noir),
// profondeur bornee par `max_depth`, scene `scenes/opt_glass.rt`.

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <string>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/shading/Material.hpp"

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

// Sphere de verre devant un plan de fond vert (`z = -3`, normale +Z) : le
// rayon transmis frappe le plan en un point qui depend de `ior` (deviation),
// donc deux `ior` donnent deux images differentes meme avec un fond uniforme
// (sans plan, tout rayon transmis manquerait vers le fond uni et `ior`
// serait invisible — piege evite ici).
std::string glassBackdropScene(const std::string& materialExtra) {
	std::string out = "scene { ";
	out += kCamera;
	out += kBackground;
	out += kAmbient;
	out += kLight;
	out += "objects { object { type sphere center (0 0 0) radius 1 material { ";
	out += "albedo (0.9 0.95 1.0) diffuse 0.3 ambient 0.05 specular 0.0 shininess 32 ";
	out += materialExtra;
	out += " } } ";
	out += "object { type plane point (0 0 -3) normal (0 0 1) material { ";
	out += "albedo (0.2 0.8 0.2) diffuse 0.8 ambient 0.1 } } } }";
	return out;
}

} // namespace

TEST_CASE("refraction (T057) : transparency=0 identique a l'opaque, ior ignore", "[refraction][t057]") {
	// Meme geometrie/lumiere : sans champ `transparency` (defaut 0), avec
	// `transparency 0.0` explicite, et avec `transparency 0.0` + `ior`
	// different (1.0 vs 1.5). Les trois doivent rendre l'octet identique
	// (le chemin `T = 0` = base seule, `ior` non lu).
	rt::scene::Scene without = parseOrDie(sphereScene(""), "opaque_default.rt");
	rt::scene::Scene explicitZero = parseOrDie(sphereScene("transparency 0.0 ior 1.0"), "opaque_zero.rt");
	rt::scene::Scene zeroOtherIor = parseOrDie(sphereScene("transparency 0.0 ior 1.5"), "opaque_ior.rt");
	const rt::render::RenderParams params{.width = 48, .height = 36, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbDefault;
	rt::render::Framebuffer fbZero;
	rt::render::Framebuffer fbIor;
	REQUIRE(rt::render::render(without, fbDefault, params).isOk());
	REQUIRE(rt::render::render(explicitZero, fbZero, params).isOk());
	REQUIRE(rt::render::render(zeroOtherIor, fbIor, params).isOk());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = fbDefault.accumAt(x, y);
			const rt::Vec3 b = fbZero.accumAt(x, y);
			const rt::Vec3 c = fbIor.accumAt(x, y);
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
			REQUIRE(a.x == c.x);
			REQUIRE(a.y == c.y);
			REQUIRE(a.z == c.z);
		}
	}
	// `T = 0` ignore `maxDepth` comme `R = 0` : profondeur 0 ou 8, meme image.
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

TEST_CASE("refraction (T057) : ior=1 aucune deviation (Descartes eta=1)", "[refraction][t057]") {
	// Niveau unitaire (`shading::refractDir`, formule `n1*sin(t1)=n2*sin(t2)`)
	// : `ior = 1` -> `eta = 1` dans les deux sens -> `T == I`.
	const rt::Vec3 incident = rt::normalize(rt::Vec3(0.3F, -0.8F, 0.5F));
	const rt::Vec3 normal = rt::normalize(rt::Vec3(0.0F, 1.0F, 0.0F));
	const rt::Vec3 entry = rt::shading::refractDir(incident, normal, true, 1.0F);
	const rt::Vec3 exit = rt::shading::refractDir(incident, normal, false, 1.0F);
	REQUIRE(std::isfinite(entry.x));
	REQUIRE(std::isfinite(exit.x));
	REQUIRE(entry.x == Catch::Approx(incident.x).margin(1e-5));
	REQUIRE(entry.y == Catch::Approx(incident.y).margin(1e-5));
	REQUIRE(entry.z == Catch::Approx(incident.z).margin(1e-5));
	REQUIRE(exit.x == Catch::Approx(incident.x).margin(1e-5));
	REQUIRE(exit.y == Catch::Approx(incident.y).margin(1e-5));
	REQUIRE(exit.z == Catch::Approx(incident.z).margin(1e-5));
	// Incidence normale : meme avec `ior = 1.5`, le rayon traverse droit
	// (composante tangentielle nulle, `rPerp = 0`).
	const rt::Vec3 straight = rt::normalize(rt::Vec3(0.0F, -1.0F, 0.0F));
	const rt::Vec3 glassStraight = rt::shading::refractDir(straight, normal, true, 1.5F);
	REQUIRE(glassStraight.x == Catch::Approx(straight.x).margin(1e-5));
	REQUIRE(glassStraight.y == Catch::Approx(straight.y).margin(1e-5));
	REQUIRE(glassStraight.z == Catch::Approx(straight.z).margin(1e-5));
	// Niveau image : sphere de verre `T = 1, ior = 1.5` au centre (incidence
	// quasi normale, donc sans deviation quel que soit `ior`) montre le fond
	// (transmis pur, aucune part diffuse), comme le miroir pur de T056 mais
	// par transmission. (`ior = 1` + `transparency > 0` est refuse par la
	// validation T024 — `ior > 1` exige — donc le cas `ior = 1` reste
	// unitaire ci-dessus, jamais par scene.)
	rt::scene::Scene opaque = parseOrDie(sphereScene("transparency 0.0"), "ior_opaque.rt");
	rt::scene::Scene glass = parseOrDie(sphereScene("transparency 1.0 ior 1.5"), "ior_glass.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fbOpaque;
	rt::render::Framebuffer fbGlass;
	REQUIRE(rt::render::render(opaque, fbOpaque, params).isOk());
	REQUIRE(rt::render::render(glass, fbGlass, params).isOk());
	const rt::Vec3 opaqueMean = meanAt(fbOpaque, 32, 24);
	const rt::Vec3 glassMean = meanAt(fbGlass, 32, 24);
	INFO("opaque centre r=" << opaqueMean.x << " verre ior=1.5 centre r=" << glassMean.x);
	REQUIRE(opaqueMean.x > 0.15F);
	REQUIRE(opaqueMean.x > opaqueMean.z);
	REQUIRE(glassMean.x == Catch::Approx(0.02F).margin(0.06));
	REQUIRE(glassMean.z == Catch::Approx(0.05F).margin(0.06));
	REQUIRE(luminance(glassMean) < 0.10F);
	REQUIRE((opaqueMean.x - glassMean.x) > 0.15F);
}

TEST_CASE("refraction (T057) : sortie objet = courbure exterieure coherente", "[refraction][t057]") {
	// Sortie verre -> air (`frontFace = false`, `eta = ior = 1.5`) a 30 degres
	// de la normale : `sin(t2) = 1.5*sin(30) = 0.75` -> `t2 ~ 48.6 degres`,
	// le rayon s'ecarte de la normale (courbure vers l'exterieur).
	// `N` = normale de shading (contre le rayon, donc vers l'interieur ici).
	const rt::Vec3 outward(0.0F, 1.0F, 0.0F);
	const rt::Vec3 inward(0.0F, -1.0F, 0.0F);
	const rt::Vec3 incident = rt::normalize(rt::Vec3(0.5F, 0.8660254F, 0.0F));
	const rt::Vec3 refracted = rt::shading::refractDir(incident, inward, false, 1.5F);
	REQUIRE(std::isfinite(refracted.x));
	REQUIRE_FALSE(rt::nearZero(refracted));
	REQUIRE(refracted.x == Catch::Approx(0.75F).margin(1e-4));
	REQUIRE(refracted.y == Catch::Approx(0.6614378F).margin(1e-4));
	// L'angle a la normale exterieure (`-N`) grandit : `cos` diminue.
	const float cosIn = rt::dot(rt::normalize(incident), outward);
	const float cosOut = rt::dot(rt::normalize(refracted), outward);
	REQUIRE(cosIn == Catch::Approx(0.8660254F).margin(1e-5));
	REQUIRE(cosOut < cosIn);
	REQUIRE(cosOut == Catch::Approx(0.6614378F).margin(1e-4));
	// Entree air -> verre (`frontFace`, `eta = 1/1.5`) : au contraire le rayon
	// se rapproche de la normale (`cos` grandit).
	const rt::Vec3 entry = rt::shading::refractDir(incident, inward, true, 1.5F);
	REQUIRE_FALSE(rt::nearZero(entry));
	const float cosEntry = rt::dot(rt::normalize(entry), outward);
	REQUIRE(cosEntry > cosIn);
	// Niveau image : meme `transparency = 1`, `ior = 1.33` (eau) vs `1.5`
	// (verre) divergent hors du centre (incidence oblique, `eta` differente),
	// identiques au centre (incidence normale, aucune deviation dans les deux
	// cas). (`ior = 1` + `transparency > 0` est refuse en T024, donc on
	// compare deux `ior > 1` valides.) Scene avec plan de fond (sinon fond
	// uni = `ior` invisible, piege evite).
	rt::scene::Scene glass1 = parseOrDie(glassBackdropScene("transparency 1.0 ior 1.33"), "curve133.rt");
	rt::scene::Scene glass15 = parseOrDie(glassBackdropScene("transparency 1.0 ior 1.5"), "curve15.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fb1;
	rt::render::Framebuffer fb15;
	REQUIRE(rt::render::render(glass1, fb1, params).isOk());
	REQUIRE(rt::render::render(glass15, fb15, params).isOk());
	const rt::Vec3 centre1 = meanAt(fb1, 32, 24);
	const rt::Vec3 centre15 = meanAt(fb15, 32, 24);
	INFO("centre ior=1.33 r=" << centre1.x << " centre ior=1.5 r=" << centre15.x);
	REQUIRE(centre1.x == Catch::Approx(centre15.x).margin(0.05));
	int different = 0;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			const rt::Vec3 a = meanAt(fb1, x, y);
			const rt::Vec3 b = meanAt(fb15, x, y);
			const float dx = a.x - b.x;
			const float dy = a.y - b.y;
			const float dz = a.z - b.z;
			if ((dx * dx + dy * dy + dz * dz) > 0.002F * 0.002F) {
				++different;
			}
		}
	}
	INFO("pixels differents ior=1.33 vs ior=1.5 : " << different);
	REQUIRE(different > 0);
}

TEST_CASE("refraction (T057) : pourcentage continu 0.5 = moyenne opaque/transmis", "[refraction][t057]") {
	// `out = base*(1-T) + transmis*T` (avec `R = 0`, `base = direct`) : a 0.5,
	// le centre vaut la moyenne des deux bornes (ponderation correcte).
	rt::scene::Scene opaque = parseOrDie(sphereScene("transparency 0.0"), "mix0.rt");
	rt::scene::Scene half = parseOrDie(sphereScene("transparency 0.5 ior 1.5"), "mix05.rt");
	rt::scene::Scene full = parseOrDie(sphereScene("transparency 1.0 ior 1.5"), "mix1.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .maxDepth = 4, .seed = 0};
	rt::render::Framebuffer fb0;
	rt::render::Framebuffer fbHalf;
	rt::render::Framebuffer fb1;
	REQUIRE(rt::render::render(opaque, fb0, params).isOk());
	REQUIRE(rt::render::render(half, fbHalf, params).isOk());
	REQUIRE(rt::render::render(full, fb1, params).isOk());
	const rt::Vec3 c0 = meanAt(fb0, 32, 24);
	const rt::Vec3 cHalf = meanAt(fbHalf, 32, 24);
	const rt::Vec3 c1 = meanAt(fb1, 32, 24);
	INFO("T=0 r=" << c0.x << " T=0.5 r=" << cHalf.x << " T=1 r=" << c1.x);
	REQUIRE(c0.x > cHalf.x);
	REQUIRE(cHalf.x > c1.x);
	const float expected = (c0.x + c1.x) * 0.5F;
	REQUIRE(cHalf.x == Catch::Approx(expected).margin(0.05));
	const float expectedG = (c0.y + c1.y) * 0.5F;
	REQUIRE(cHalf.y == Catch::Approx(expectedG).margin(0.05));
}

TEST_CASE("refraction (T057) : reflexion totale interne repliee, profondeur bornee", "[refraction][t057]") {
	// Unitaire : verre -> air rasant (`eta = 1.5`, `|rPerp|^2 > 1`) ->
	// sentinelle nulle (pas de transmis, le renderer replie sur le miroir).
	const rt::Vec3 grazing = rt::normalize(rt::Vec3(0.99F, -0.14F, 0.0F));
	const rt::Vec3 nUp(0.0F, 1.0F, 0.0F);
	const rt::Vec3 tir = rt::shading::refractDir(grazing, nUp, false, 1.5F);
	REQUIRE(tir.x == 0.0F);
	REQUIRE(tir.y == 0.0F);
	REQUIRE(tir.z == 0.0F);
	// Degeneres definis : normale nulle, incident nul, `ior` NaN -> nul.
	const float nan = std::numeric_limits<float>::quiet_NaN();
	REQUIRE(rt::nearZero(rt::shading::refractDir(grazing, rt::Vec3{}, false, 1.5F)));
	REQUIRE(rt::nearZero(rt::shading::refractDir(rt::Vec3{}, nUp, false, 1.5F)));
	REQUIRE(std::isfinite(rt::shading::refractDir(grazing, nUp, false, nan).x));
	// Image : sphere de verre rendue sans NaN/Inf, y compris aux bords
	// (incidences rasantes ou la TIR se declenche), et la profondeur borne
	// le rebond (`maxDepth = 0` = direct seul, `8` = transmis).
	rt::scene::Scene glass = parseOrDie(sphereScene("transparency 0.9 ior 1.5"), "tir.rt");
	const rt::render::RenderParams deep{.width = 48, .height = 36, .spp = 4, .maxDepth = 8, .seed = 0};
	rt::render::Framebuffer fbDeep;
	REQUIRE(rt::render::render(glass, fbDeep, deep).isOk());
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 a = fbDeep.accumAt(x, y);
			REQUIRE(std::isfinite(a.x));
			REQUIRE(std::isfinite(a.y));
			REQUIRE(std::isfinite(a.z));
			REQUIRE_FALSE(std::isnan(a.x));
		}
	}
	const rt::render::RenderParams flat{.width = 48, .height = 36, .spp = 4, .maxDepth = 0, .seed = 0};
	rt::render::Framebuffer fbFlat;
	REQUIRE(rt::render::render(glass, fbFlat, flat).isOk());
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
	INFO("pixels differents profondeur 0 vs 8 (verre) : " << different);
	REQUIRE(different > 0);
}

TEST_CASE("refraction (T057) : scene opt_glass.rt rend sans erreur", "[refraction][t057]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/opt_glass.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Scene& scene = parsed.value();
	REQUIRE(scene.totalObjectCount() >= 2);
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 2, .maxDepth = 4, .seed = 0};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	bool hasBright = false;
	for (int y = 0; y < 48; ++y) {
		for (int x = 0; x < 64; ++x) {
			const rt::Vec3 a = fb.accumAt(x, y);
			REQUIRE(std::isfinite(a.x));
			REQUIRE_FALSE(std::isnan(a.x));
			if ((a.x + a.y + a.z) / 3.0F > 0.05F) {
				hasBright = true;
			}
		}
	}
	REQUIRE(hasBright);
}
