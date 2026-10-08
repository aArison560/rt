// Tests de la lumiere parallele (T055, *Parallel light*), Catch2.
// DoD : test comparatif point vs directionnel (divergentes vs paralleles) ;
// `scenes/opt_parallel.rt` versionnee.

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <limits>
#include <string>

#include "rt/lighting/DirectionalLight.hpp"
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

float displayMean(const rt::render::Framebuffer& fb, int x, int y, int width) {
	const std::size_t idx =
	    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
	const auto& px = fb.displayData()[idx];
	return (static_cast<float>(px.r) + static_cast<float>(px.g) + static_cast<float>(px.b)) / 3.0F;
}

} // namespace

TEST_CASE("directionnelle (T055) : L = -normalize(direction), sans attenuation",
          "[directional][t055]") {
	// `L` constante, independante de la position (soleil).
	const rt::Vec3 down = rt::lighting::toLightDir(rt::Vec3(0.3F, -1.0F, -0.2F));
	REQUIRE(std::isfinite(down.x));
	REQUIRE(rt::lengthSquared(down) == Catch::Approx(1.0F).epsilon(1e-5));
	// Vers le soleil (opposee a la propagation) : `direction.y < 0` -> `L.y > 0`.
	REQUIRE(down.y > 0.0F);
	// Meme direction normalisee deux fois : meme `L` (pas d'attenuation).
	const rt::Vec3 twice = rt::lighting::toLightDir(rt::Vec3(0.6F, -2.0F, -0.4F));
	REQUIRE(twice.x == Catch::Approx(down.x).epsilon(1e-5));
	REQUIRE(twice.y == Catch::Approx(down.y).epsilon(1e-5));
	REQUIRE(twice.z == Catch::Approx(down.z).epsilon(1e-5));
	// Degeneres -> nul (lumiere ignoree, definie).
	const rt::Vec3 nullDir = rt::lighting::toLightDir(rt::Vec3{});
	REQUIRE(nullDir.x == 0.0F);
	REQUIRE(nullDir.y == 0.0F);
	REQUIRE(nullDir.z == 0.0F);
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const rt::Vec3 nanDir = rt::lighting::toLightDir(rt::Vec3(nan, nan, nan));
	REQUIRE(nanDir.x == 0.0F);
}

TEST_CASE("directionnelle (T055) : Lambert parallele, dos et degeneres -> ambiant",
          "[directional][t055]") {
	const rt::shading::MaterialParams mat{.albedo = rt::Vec3(0.7F, 0.7F, 0.7F),
	                                      .ambient = 0.1F,
	                                      .diffuse = 0.8F};
	const rt::shading::AmbientParams ambient{.color = rt::Vec3(0.06F, 0.06F, 0.08F),
	                                         .intensity = 1.0F};
	// Face au soleil : plus clair que l'ambiant seul.
	rt::shading::DirectionalLightParams sun;
	sun.direction = rt::Vec3(0.0F, -1.0F, 0.0F);
	sun.color = rt::Vec3(1.0F, 1.0F, 1.0F);
	sun.intensity = 1.0F;
	const rt::Vec3 lit =
	    rt::shading::shadeLambertDirectional(mat, rt::Vec3(0.0F, 1.0F, 0.0F), sun, ambient);
	rt::shading::DirectionalLightParams off = sun;
	off.intensity = 0.0F;
	const rt::Vec3 ambOnly =
	    rt::shading::shadeLambertDirectional(mat, rt::Vec3(0.0F, 1.0F, 0.0F), off, ambient);
	REQUIRE(lit.x > ambOnly.x + 0.05F);
	REQUIRE(std::isfinite(lit.x));
	// Dos au soleil : ambiant seul.
	const rt::Vec3 back =
	    rt::shading::shadeLambertDirectional(mat, rt::Vec3(0.0F, -1.0F, 0.0F), sun, ambient);
	REQUIRE(back.x == Catch::Approx(ambOnly.x).epsilon(1e-5));
	// Direction nulle : ambiant seul, fini.
	rt::shading::DirectionalLightParams nullSun = sun;
	nullSun.direction = rt::Vec3{};
	const rt::Vec3 nullShaded =
	    rt::shading::shadeLambertDirectional(mat, rt::Vec3(0.0F, 1.0F, 0.0F), nullSun, ambient);
	REQUIRE(nullShaded.x == Catch::Approx(ambOnly.x).epsilon(1e-5));
	REQUIRE(std::isfinite(nullShaded.x));
}

TEST_CASE("directionnelle (T055) : ponctuelle diverge, parallele reste egale (DoD comparatif)",
          "[directional][t055]") {
	// 2 spheres grises symetriques (-1.2 / +1.2), meme materiau mat
	// (speculaire 0 pour isoler le diffus). Lumiere laterale droite :
	// - ponctuelle en (4 5 3) : la droite est plus proche -> plus claire ;
	// - directionnelle (-4 -5 -3) (meme provenance) : `L` constante, pas
	//   d'attenuation -> les 2 cotes egaux (ombres paralleles).
	const char* head =
	    "scene { camera { position (0 1 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } ";
	const char* tail =
	    "objects { "
	    "object { type sphere center (-1.2 0 0) radius 1 material { "
	    "albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 specular 0.0 } } "
	    "object { type sphere center (1.2 0 0) radius 1 material { "
	    "albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 specular 0.0 } } "
	    "} }";
	rt::scene::Scene pointScene = parseOrDie(
	    std::string(head) +
	        "lights { light { position (4 5 3) color (1 1 1) intensity 1.0 } } " + tail,
	    "point.rt");
	rt::scene::Scene dirScene = parseOrDie(
	    std::string(head) +
	        "lights { light directional \"sun\" { direction (-4 -5 -3) color (1 1 1) "
	        "intensity 1.0 } } " +
	        tail,
	    "dir.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbPoint;
	rt::render::Framebuffer fbDir;
	REQUIRE(rt::render::render(pointScene, fbPoint, params).isOk());
	REQUIRE(rt::render::render(dirScene, fbDir, params).isOk());
	// Centres approximatifs des spheres (gauche x=20, droite x=44, y=24).
	const float leftPoint = displayMean(fbPoint, 20, 24, params.width);
	const float rightPoint = displayMean(fbPoint, 44, 24, params.width);
	const float leftDir = displayMean(fbDir, 20, 24, params.width);
	const float rightDir = displayMean(fbDir, 44, 24, params.width);
	INFO("ponctuelle gauche=" << leftPoint << " droite=" << rightPoint);
	INFO("directionnelle gauche=" << leftDir << " droite=" << rightDir);
	REQUIRE(std::isfinite(leftPoint));
	REQUIRE(std::isfinite(rightPoint));
	REQUIRE(std::isfinite(leftDir));
	REQUIRE(std::isfinite(rightDir));
	// Ponctuelle : la droite (cote lumiere) est nettement plus claire.
	REQUIRE((rightPoint - leftPoint) > 5.0F);
	// Directionnelle : les 2 cotes egaux (meme `L`, pas de distance).
	REQUIRE(std::fabs(rightDir - leftDir) < 4.0F);
	// Les modeles different vraiment (images non identiques).
	REQUIRE(std::fabs(rightPoint - rightDir) > 1.0F);
}

TEST_CASE("directionnelle (T055) : memes ombres paralleles, pas d'acne", "[directional][t055]") {
	// Sol + sphere occultrice, soleil lateral (1 -1 -0.2) : l'ombre portee
	// assombrit le sol (parallele, `tMax` infini). Sol seul : tout eclaire.
	const char* head =
	    "scene { camera { position (0 4 7) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } "
	    "lights { light directional \"sun\" { direction (1 -1 -0.2) color (1 1 1) "
	    "intensity 1.0 } } ";
	rt::scene::Scene withOcc = parseOrDie(
	    std::string(head) +
	        "objects { "
	        "object { type plane point (0 -1 0) normal (0 1 0) material { "
	        "albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } "
	        "object { type sphere center (0 0 0) radius 0.8 material { "
	        "albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 } } } }",
	    "occ.rt");
	rt::scene::Scene noOcc = parseOrDie(
	    std::string(head) +
	        "objects { "
	        "object { type plane point (0 -1 0) normal (0 1 0) material { "
	        "albedo (0.7 0.7 0.7) diffuse 0.8 ambient 0.05 } } } }",
	    "noocc.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbOcc;
	rt::render::Framebuffer fbNoOcc;
	REQUIRE(rt::render::render(withOcc, fbOcc, params).isOk());
	REQUIRE(rt::render::render(noOcc, fbNoOcc, params).isOk());
	float minOcc = 255.0F;
	float minNoOcc = 255.0F;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const float lumOcc = displayMean(fbOcc, x, y, params.width);
			const float lumNoOcc = displayMean(fbNoOcc, x, y, params.width);
			REQUIRE(std::isfinite(lumOcc));
			REQUIRE(std::isfinite(lumNoOcc));
			if (lumOcc < minOcc) {
				minOcc = lumOcc;
			}
			if (lumNoOcc < minNoOcc) {
				minNoOcc = lumNoOcc;
			}
		}
	}
	INFO("min avec occultrice=" << minOcc << " sans=" << minNoOcc);
	// Ombre portee : avec la sphere, des pixels tombent dans l'ombre.
	REQUIRE(minOcc < 30.0F);
	// Sans occultrice : aucun auto-ombrage (pas d'acne, `tMin` eps).
	REQUIRE(minNoOcc > 100.0F);
}

TEST_CASE("directionnelle (T055) : scenes/opt_parallel.rt rend (DoD)", "[directional][t055]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/opt_parallel.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Scene& scene = parsed.value();
	REQUIRE(scene.lights.size() == 1);
	REQUIRE(scene.lights[0].type == rt::scene::LightType::Directional);
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	// Image non triviale : des pixels s'ecartent du fond, sans NaN.
	int different = 0;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const std::size_t idx =
			    static_cast<std::size_t>(y) * static_cast<std::size_t>(params.width) +
			    static_cast<std::size_t>(x);
			const auto& px = fb.displayData()[idx];
			REQUIRE(std::isfinite(fb.accumAt(x, y).x));
			const float dr = static_cast<float>(px.r) / 255.0F - scene.background.color.x;
			const float dg = static_cast<float>(px.g) / 255.0F - scene.background.color.y;
			const float db = static_cast<float>(px.b) / 255.0F - scene.background.color.z;
			if ((dr * dr + dg * dg + db * db) > 0.05F * 0.05F) {
				++different;
			}
		}
	}
	INFO("pixels non-fond : " << different << "/" << 64 * 48);
	REQUIRE(different > (64 * 48) / 20);
}
