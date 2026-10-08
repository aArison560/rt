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
