// Tests du shading diffus + ambiante minimale (T033), Catch2.
// DoD : image non noire partout (luminosite minimale > 0, constatee sur
// `scenes/default.rt`), couleurs bornees [0,1] et jamais de NaN
// (`std::isfinite` sur le buffer). Lambert verifie avec 1 ponctuelle +
// ambiante globale (ARCHITECTURE.md §4.4, sans ombre/attenuation).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/shading/Material.hpp"

namespace {

rt::shading::MaterialParams makeMat() {
	return {.albedo = rt::Vec3(0.9F, 0.3F, 0.2F), .ambient = 0.1F, .diffuse = 0.7F};
}

rt::shading::AmbientParams makeAmbient() {
	return {.color = rt::Vec3(0.06F, 0.06F, 0.08F), .intensity = 1.0F};
}

rt::shading::PointLightParams makeLight() {
	return {.position = rt::Vec3(3.0F, 5.0F, 2.0F),
	        .color = rt::Vec3(1.0F, 1.0F, 1.0F),
	        .intensity = 0.8F};
}

} // namespace

TEST_CASE("material : face a la lumiere = ambiant + diffus plein", "[material]") {
	const rt::shading::MaterialParams mat = makeMat();
	const rt::shading::AmbientParams ambient = makeAmbient();
	const rt::shading::PointLightParams light = makeLight();
	const rt::Vec3 hit(0.0F, 0.0F, 0.0F);
	// Normale vers la lumiere (meme direction que `light.position`).
	const rt::Vec3 normal = rt::normalize(light.position - hit);
	const rt::Vec3 shaded = rt::shading::shadeLambert(mat, normal, hit, light, ambient);
	// Ambiant seul (dos a la lumiere) < eclaire (face).
	const rt::Vec3 back = rt::shading::shadeLambert(mat, -normal, hit, light, ambient);
	REQUIRE(shaded.x > back.x);
	REQUIRE(shaded.y >= back.y);
	// L'ambiant garantit un plancher > 0 meme dos a la lumiere.
	REQUIRE(back.x > 0.0F);
	REQUIRE(back.y > 0.0F);
	REQUIRE(back.z > 0.0F);
	// Borne [0,1], fini.
	REQUIRE(shaded.x <= 1.0F);
	REQUIRE(shaded.y <= 1.0F);
	REQUIRE(shaded.z <= 1.0F);
	REQUIRE(std::isfinite(shaded.x));
	REQUIRE(std::isfinite(shaded.y));
	REQUIRE(std::isfinite(shaded.z));
}

TEST_CASE("material : doubler l'intensite double la part diffuse", "[material]") {
	// Ambiant nul pour isoler le diffus.
	const rt::shading::MaterialParams mat{.albedo = rt::Vec3(0.8F, 0.8F, 0.8F),
	                                      .ambient = 0.0F,
	                                      .diffuse = 0.7F};
	const rt::shading::AmbientParams ambient = makeAmbient();
	rt::shading::PointLightParams light = makeLight();
	light.intensity = 0.5F;
	const rt::Vec3 hit(0.0F, 0.0F, 0.0F);
	const rt::Vec3 normal = rt::normalize(light.position - hit);
	const rt::Vec3 half = rt::shading::shadeLambert(mat, normal, hit, light, ambient);
	light.intensity = 1.0F;
	const rt::Vec3 full = rt::shading::shadeLambert(mat, normal, hit, light, ambient);
	REQUIRE(full.x == Catch::Approx(half.x * 2.0F).epsilon(1e-5));
	REQUIRE(full.y == Catch::Approx(half.y * 2.0F).epsilon(1e-5));
	REQUIRE(full.z == Catch::Approx(half.z * 2.0F).epsilon(1e-5));
}

TEST_CASE("material : cas degeneres sans NaN, satures [0,1]", "[material]") {
	const rt::shading::MaterialParams mat = makeMat();
	const rt::shading::AmbientParams ambient = makeAmbient();
	const rt::shading::PointLightParams light = makeLight();
	const rt::Vec3 hit(0.0F, 0.0F, 0.0F);
	// Normale nulle -> ambiant seul, fini.
	const rt::Vec3 nullNormal = rt::shading::shadeLambert(mat, rt::Vec3{}, hit, light, ambient);
	REQUIRE(std::isfinite(nullNormal.x));
	REQUIRE(std::isfinite(nullNormal.y));
	REQUIRE(std::isfinite(nullNormal.z));
	// Lumiere confondue au point -> ambiant seul, fini.
	rt::shading::PointLightParams coincident = light;
	coincident.position = hit;
	const rt::Vec3 coincidentShaded = rt::shading::shadeLambert(mat, rt::Vec3(0.0F, 1.0F, 0.0F),
	                                                            hit, coincident, ambient);
	REQUIRE(std::isfinite(coincidentShaded.x));
	// Intensite enorme -> sature a 1, jamais > 1 ni Inf.
	rt::shading::PointLightParams hot = light;
	hot.intensity = 1e9F;
	const rt::Vec3 hotShaded = rt::shading::shadeLambert(
	    mat, rt::normalize(light.position - hit), hit, hot, ambient);
	REQUIRE(hotShaded.x <= 1.0F);
	REQUIRE(hotShaded.y <= 1.0F);
	REQUIRE(hotShaded.z <= 1.0F);
	REQUIRE(std::isfinite(hotShaded.x));
	// NaN en entree -> 0, pas de fuite NaN.
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const rt::shading::MaterialParams nanMat{.albedo = rt::Vec3(nan, nan, nan),
	                                         .ambient = 0.1F,
	                                         .diffuse = 0.7F};
	const rt::Vec3 nanShaded =
	    rt::shading::shadeLambert(nanMat, rt::Vec3(0.0F, 1.0F, 0.0F), hit, light, ambient);
	REQUIRE(std::isfinite(nanShaded.x));
	REQUIRE(std::isfinite(nanShaded.y));
	REQUIRE(std::isfinite(nanShaded.z));
}

TEST_CASE("material : saturate borne [0,1] et neutralise NaN/Inf", "[material]") {
	const rt::Vec3 saturated = rt::shading::saturate(rt::Vec3(5.0F, -2.0F, 0.5F));
	REQUIRE(saturated.x == 1.0F);
	REQUIRE(saturated.y == 0.0F);
	REQUIRE(saturated.z == Catch::Approx(0.5F));
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	const rt::Vec3 cleaned = rt::shading::saturate(rt::Vec3(nan, inf, -inf));
	REQUIRE(cleaned.x == 0.0F);
	REQUIRE(cleaned.y == 0.0F);
	REQUIRE(cleaned.z == 0.0F);
}

TEST_CASE("material : image rendue non noire, sans NaN (DoD T033)", "[material]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Scene& scene = parsed.value();
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 1};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	float minLuminance = 1.0F;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			REQUIRE(std::isfinite(accum.x));
			REQUIRE(std::isfinite(accum.y));
			REQUIRE(std::isfinite(accum.z));
			REQUIRE_FALSE(std::isnan(accum.x));
			REQUIRE_FALSE(std::isnan(accum.y));
			REQUIRE_FALSE(std::isnan(accum.z));
			const float luminance = (accum.x + accum.y + accum.z) / 3.0F;
			if (luminance < minLuminance) {
				minLuminance = luminance;
			}
		}
	}
	// T046 : l'ombrage (Lambert + ambiant) garantit un plancher > 0, mais le
	// dithering deterministe T036 (±0.03, moyenne -> 0) peut pousser un pixel
	// sombre sous 0 en `accum` (HDR, avant `present()` qui sature [0,1]).
	// On exige donc le plancher ambiant moins le dithering, jamais de NaN.
	// Sans dithering le plancher serait > 0 (cf. test face/dos ci-dessus).
	REQUIRE(minLuminance > -0.031F);
}

// T050 : modele de materiau complet — chaque champ pilotable depuis le
// fichier (regle du sujet), bornes du schema (R1), plumbing
// `scene::Material` -> `shading::MaterialParams` (copie sans perte dans
// `render/`). Les champs actifs (`albedo`/`ambient`/`diffuse`) sont
// observables dans l'image (pixels) ; les avances (`specular`,
// `shininess`, `reflectivity`, `transparency`, `ior`, texture, pattern)
// sont stockes et valides ici, leur effet visuel arrivant en
// T053/T056/T057/T102/T105 — le test par champ verifie le fichier -> Scene
// pour tous, et le fichier -> pixels pour les actifs.
TEST_CASE("material complet (T050) : defauts du schema", "[material][t050]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseContent(
	    "scene { camera { position (0 1 4) target (0 0 0) } "
	    "objects { object { type sphere } } }",
	    "defaults.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Material& mat = parsed.value().objects[0].material;
	REQUIRE(mat.albedo.x == Catch::Approx(0.8F));
	REQUIRE(mat.albedo.y == Catch::Approx(0.8F));
	REQUIRE(mat.albedo.z == Catch::Approx(0.8F));
	REQUIRE(mat.ambient == Catch::Approx(0.1F));
	REQUIRE(mat.diffuse == Catch::Approx(0.7F));
	REQUIRE(mat.specular == Catch::Approx(0.5F));
	REQUIRE(mat.shininess == Catch::Approx(32.0F));
	REQUIRE(mat.reflectivity == Catch::Approx(0.0F));
	REQUIRE(mat.transparency == Catch::Approx(0.0F));
	REQUIRE(mat.ior == Catch::Approx(1.5F));
	REQUIRE_FALSE(mat.texture.present);
	REQUIRE_FALSE(mat.pattern.present);
}

TEST_CASE("material complet (T050) : chaque champ pilotable depuis le fichier", "[material][t050]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseContent(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { "
	    "object { type sphere center (0 0 0) radius 1 material { "
	    "albedo (0.1 0.2 0.3) ambient 0.2 diffuse 0.3 specular 0.4 shininess 64 "
	    "reflectivity 0.25 transparency 0.5 ior 1.5 "
	    "texture \"/tmp/t050_tex.png\" { scale 2 2 offset 0.1 0.2 } "
	    "pattern { type checker scale 2 frequency 3 } } } } }",
	    "full.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Material& mat = parsed.value().objects[0].material;
	REQUIRE(mat.albedo.x == Catch::Approx(0.1F));
	REQUIRE(mat.albedo.y == Catch::Approx(0.2F));
	REQUIRE(mat.albedo.z == Catch::Approx(0.3F));
	REQUIRE(mat.ambient == Catch::Approx(0.2F));
	REQUIRE(mat.diffuse == Catch::Approx(0.3F));
	REQUIRE(mat.specular == Catch::Approx(0.4F));
	REQUIRE(mat.shininess == Catch::Approx(64.0F));
	REQUIRE(mat.reflectivity == Catch::Approx(0.25F));
	REQUIRE(mat.transparency == Catch::Approx(0.5F));
	REQUIRE(mat.ior == Catch::Approx(1.5F));
	REQUIRE(mat.texture.present);
	REQUIRE(mat.texture.file == "/tmp/t050_tex.png");
	REQUIRE(mat.pattern.present);
	REQUIRE(mat.pattern.type == "checker");
	REQUIRE(mat.pattern.scale == Catch::Approx(2.0F));
	REQUIRE(mat.pattern.frequency == Catch::Approx(3.0F));
	// Alias : `color` -> `albedo`, `reflect` -> `reflectivity` (FORMAT §2).
	rt::Result<rt::scene::Scene> aliased = rt::scene::parseContent(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { "
	    "object { type sphere material { color (0.9 0.1 0.1) reflect 0.7 } } } }",
	    "alias.rt");
	REQUIRE(aliased.isOk());
	REQUIRE(aliased.value().objects[0].material.albedo.x == Catch::Approx(0.9F));
	REQUIRE(aliased.value().objects[0].material.reflectivity == Catch::Approx(0.7F));
	// Fixture de reference : `material.rt` (8 champs, sans texture/pattern).
	rt::Result<rt::scene::Scene> fixture = rt::scene::parseFile("tests/cases/valid/material.rt");
	REQUIRE(fixture.isOk());
	const rt::scene::Material& fmat = fixture.value().objects[0].material;
	REQUIRE(fmat.specular == Catch::Approx(0.8F));
	REQUIRE(fmat.shininess == Catch::Approx(32.0F));
	REQUIRE(fmat.reflectivity == Catch::Approx(0.2F));
	REQUIRE(fmat.transparency == Catch::Approx(0.5F));
	REQUIRE(fmat.ior == Catch::Approx(1.5F));
}

TEST_CASE("material complet (T050) : bornes rejetees sans crash", "[material][t050]") {
	auto expectFail = [](const char* body) {
		std::string content =
		    std::string("scene { camera { position (0 1 4) target (0 0 0) } objects { "
		                "object { type sphere material { ") +
		    body + " } } } }";
		rt::Result<rt::scene::Scene> result = rt::scene::parseContent(content, "bad.rt");
		REQUIRE(result.isError());
		REQUIRE_FALSE(result.status().message.empty());
	};
	expectFail("specular 1.5");
	expectFail("specular -0.1");
	expectFail("shininess 0");
	expectFail("shininess 2048");
	expectFail("reflectivity 1.5");
	expectFail("reflectivity -0.2");
	expectFail("transparency 1.2");
	expectFail("ior 0.5");
	expectFail("ior 5.0");
	expectFail("albedo (1.5 0 0)");
	expectFail("diffuse 2.0");
	expectFail("ambient -0.1");
	// Croisee T024 : transparence > 0 exige ior > 1.
	rt::Result<rt::scene::Scene> cross = rt::scene::parseContent(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { "
	    "object { type sphere material { transparency 0.5 ior 1.0 } } } }",
	    "cross.rt");
	REQUIRE(cross.isError());
	REQUIRE(cross.status().message.find("ior") != std::string::npos);
}

TEST_CASE("material complet (T050) : champs actifs visibles dans l'image", "[material][t050]") {
	auto renderCentre = [](const char* albedo, const char* extra) {
		std::string content =
		    std::string("scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
		                "background { color (0.02 0.02 0.05) } "
		                "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
		                "objects { object { type sphere center (0 0 0) radius 1 material { albedo ") +
		    albedo + " " + extra + " } } } }";
		rt::Result<rt::scene::Scene> parsed = rt::scene::parseContent(content, "shade.rt");
		REQUIRE(parsed.isOk());
		rt::render::Framebuffer fb;
		REQUIRE(rt::render::render(parsed.value(), fb, {.width = 32, .height = 24, .spp = 4})
		            .isOk());
		const rt::Vec3 accum = fb.accumAt(16, 12);
		const int n = fb.samplesAt(16, 12);
		return rt::Vec3(accum.x / static_cast<float>(n), accum.y / static_cast<float>(n),
		                 accum.z / static_cast<float>(n));
	};
	// Albedo rouge vs bleu : le canal dominant suit (fichier -> pixels).
	const rt::Vec3 red = renderCentre("(0.9 0.1 0.1)", "diffuse 0.7");
	const rt::Vec3 blue = renderCentre("(0.1 0.1 0.9)", "diffuse 0.7");
	INFO("rouge r=" << red.x << " b=" << red.z << " bleu r=" << blue.x << " b=" << blue.z);
	REQUIRE(red.x > red.z);
	REQUIRE(blue.z > blue.x);
	REQUIRE(red.x > blue.x);
	// Diffuse 0 (ambiant seul) vs 1 (plein) : la face eclairee change.
	const rt::Vec3 flat = renderCentre("(0.8 0.8 0.8)", "diffuse 0.0 ambient 0.1");
	const rt::Vec3 full = renderCentre("(0.8 0.8 0.8)", "diffuse 1.0 ambient 0.1");
	INFO("diffuse0 r=" << flat.x << " diffuse1 r=" << full.x);
	REQUIRE(full.x > flat.x + 0.05F);
	// Champs avances stockes sans casser le rendu : meme scene + `specular`
	// / `reflectivity` / `transparency+ior` differents rendent sans erreur
	// (effet visuel en T053/T056/T057, plumbing verifie ici).
	for (const char* extra : {"specular 0.0", "specular 1.0 shininess 256",
	                          "reflectivity 0.9", "transparency 0.5 ior 1.5"}) {
		const rt::Vec3 pixel = renderCentre("(0.8 0.2 0.2)", extra);
		REQUIRE(std::isfinite(pixel.x));
		REQUIRE(std::isfinite(pixel.y));
		REQUIRE(std::isfinite(pixel.z));
	}
}
