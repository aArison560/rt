// Tests du speculaire Blinn-Phong (T053, M7), Catch2.
// DoD : saturation en blanc (pixels proches de (1,1,1) sur sphere lisse) ;
// degrade visible (luminosite decroit de la lumiere vers l'ombre).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <limits>
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

} // namespace

TEST_CASE("speculaire (T053) : aligne N=H = pic maximal", "[specular][t053]") {
	const rt::shading::MaterialParams mat{.albedo = rt::Vec3(0.8F, 0.2F, 0.2F),
	                                      .ambient = 0.1F,
	                                      .diffuse = 0.7F,
	                                      .specular = 0.5F,
	                                      .shininess = 32.0F};
	const rt::Vec3 normal(0.0F, 0.0F, 1.0F);
	const rt::Vec3 view(0.0F, 0.0F, 1.0F);
	const rt::Vec3 lightDir(0.0F, 0.0F, 1.0F);
	const rt::Vec3 lightColor(1.0F, 1.0F, 1.0F);
	const rt::Vec3 spec =
	    rt::shading::specularTerm(normal, view, lightDir, mat, lightColor, 1.0F);
	// NdotH = 1 -> pow = 1 -> spec = 0.5 * 1.0 = 0.5 par canal.
	REQUIRE(spec.x == Catch::Approx(0.5F).epsilon(1e-5));
	REQUIRE(spec.y == Catch::Approx(0.5F).epsilon(1e-5));
	REQUIRE(spec.z == Catch::Approx(0.5F).epsilon(1e-5));
	REQUIRE(std::isfinite(spec.x));
}

TEST_CASE("speculaire (T053) : dos a la lumiere et degeneres -> 0, sans NaN", "[specular][t053]") {
	const rt::shading::MaterialParams mat{.albedo = rt::Vec3(0.8F, 0.8F, 0.8F),
	                                      .ambient = 0.1F,
	                                      .diffuse = 0.7F,
	                                      .specular = 0.8F,
	                                      .shininess = 64.0F};
	const float nan = std::numeric_limits<float>::quiet_NaN();
	// Dos a la lumiere : N opposee a L -> 0.
	const rt::Vec3 back =
	    rt::shading::specularTerm(rt::Vec3(0.0F, 0.0F, 1.0F), rt::Vec3(0.0F, 0.0F, 1.0F),
	                              rt::Vec3(0.0F, 0.0F, -1.0F), mat, rt::Vec3(1.0F, 1.0F, 1.0F), 1.0F);
	REQUIRE(back.x == 0.0F);
	REQUIRE(back.y == 0.0F);
	REQUIRE(back.z == 0.0F);
	// Normale nulle -> 0.
	const rt::Vec3 nullN =
	    rt::shading::specularTerm(rt::Vec3{}, rt::Vec3(0.0F, 0.0F, 1.0F),
	                              rt::Vec3(0.0F, 0.0F, 1.0F), mat, rt::Vec3(1.0F, 1.0F, 1.0F), 1.0F);
	REQUIRE(nullN.x == 0.0F);
	// Vue nulle -> 0.
	const rt::Vec3 nullV =
	    rt::shading::specularTerm(rt::Vec3(0.0F, 0.0F, 1.0F), rt::Vec3{},
	                              rt::Vec3(0.0F, 0.0F, 1.0F), mat, rt::Vec3(1.0F, 1.0F, 1.0F), 1.0F);
	REQUIRE(nullV.x == 0.0F);
	// Specular 0 -> 0.
	rt::shading::MaterialParams flat = mat;
	flat.specular = 0.0F;
	const rt::Vec3 none =
	    rt::shading::specularTerm(rt::Vec3(0.0F, 0.0F, 1.0F), rt::Vec3(0.0F, 0.0F, 1.0F),
	                              rt::Vec3(0.0F, 0.0F, 1.0F), flat, rt::Vec3(1.0F, 1.0F, 1.0F), 1.0F);
	REQUIRE(none.x == 0.0F);
	// Intensite 0 ou NaN -> 0, jamais de NaN.
	const rt::Vec3 zeroI =
	    rt::shading::specularTerm(rt::Vec3(0.0F, 0.0F, 1.0F), rt::Vec3(0.0F, 0.0F, 1.0F),
	                              rt::Vec3(0.0F, 0.0F, 1.0F), mat, rt::Vec3(1.0F, 1.0F, 1.0F), 0.0F);
	REQUIRE(zeroI.x == 0.0F);
	const rt::Vec3 nanI =
	    rt::shading::specularTerm(rt::Vec3(0.0F, 0.0F, 1.0F), rt::Vec3(0.0F, 0.0F, 1.0F),
	                              rt::Vec3(0.0F, 0.0F, 1.0F), mat, rt::Vec3(1.0F, 1.0F, 1.0F), nan);
	REQUIRE(nanI.x == 0.0F);
	REQUIRE(std::isfinite(nanI.x));
	// `shadeSpecular` ponctuelle : lumiere confondue au point -> 0.
	rt::shading::PointLightParams coincident;
	coincident.position = rt::Vec3(0.0F, 0.0F, 0.0F);
	coincident.color = rt::Vec3(1.0F, 1.0F, 1.0F);
	coincident.intensity = 2.0F;
	const rt::Vec3 coinc = rt::shading::shadeSpecular(mat, rt::Vec3(0.0F, 0.0F, 1.0F),
	                                                  rt::Vec3(0.0F, 0.0F, 1.0F),
	                                                  rt::Vec3(0.0F, 0.0F, 0.0F), coincident);
	REQUIRE(coinc.x == 0.0F);
	REQUIRE(std::isfinite(coinc.x));
}

TEST_CASE("speculaire (T053) : saturation en blanc sur sphere lisse (DoD)", "[specular][t053]") {
	// Sphere lisse face camera, spot frontal fort : le reflet sature en blanc.
	const char* shiny =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 2.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 material { "
	    "albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 specular 1.0 shininess 32 } } } }";
	const char* matte =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 2.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 material { "
	    "albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 specular 0.0 shininess 32 } } } }";
	rt::scene::Scene sceneShiny = parseOrDie(shiny, "shiny.rt");
	rt::scene::Scene sceneMatte = parseOrDie(matte, "matte.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fbShiny;
	rt::render::Framebuffer fbMatte;
	REQUIRE(rt::render::render(sceneShiny, fbShiny, params).isOk());
	REQUIRE(rt::render::render(sceneMatte, fbMatte, params).isOk());
	// Comptage sur l'affichage (post `present()`, gamma 2.2) : blanc = 255.
	int whiteShiny = 0;
	int whiteMatte = 0;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const std::size_t idx =
			    static_cast<std::size_t>(y) * static_cast<std::size_t>(params.width) +
			    static_cast<std::size_t>(x);
			const auto& px = fbShiny.displayData()[idx];
			if (px.r >= 250 && px.g >= 250 && px.b >= 250) {
				++whiteShiny;
			}
			const auto& pm = fbMatte.displayData()[idx];
			if (pm.r >= 250 && pm.g >= 250 && pm.b >= 250) {
				++whiteMatte;
			}
			REQUIRE(std::isfinite(fbShiny.accumAt(x, y).x));
		}
	}
	INFO("blancs spec=1 : " << whiteShiny << " spec=0 : " << whiteMatte);
	// DoD : presence de pixels proches de (1,1,1) avec speculaire...
	REQUIRE(whiteShiny > 0);
	// ...et aucun sans speculaire (meme geometrie, meme lumiere).
	REQUIRE(whiteMatte == 0);
}

TEST_CASE("speculaire (T053) : degrade visible de la lumiere vers l'ombre (DoD)",
          "[specular][t053]") {
	rt::scene::Scene scene = parseOrDie(
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "ambient { color (0.06 0.06 0.08) intensity 1.0 } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 2.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 1 material { "
	    "albedo (0.8 0.2 0.2) diffuse 0.7 ambient 0.1 specular 1.0 shininess 32 } } } }",
	    "grad.rt");
	const rt::render::RenderParams params{.width = 64, .height = 48, .spp = 4, .seed = 0};
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	float minLum = 10.0F;
	float maxLum = -10.0F;
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const std::size_t idx =
			    static_cast<std::size_t>(y) * static_cast<std::size_t>(params.width) +
			    static_cast<std::size_t>(x);
			const auto& px = fb.displayData()[idx];
			const float lum =
			    (static_cast<float>(px.r) + static_cast<float>(px.g) + static_cast<float>(px.b)) /
			    (3.0F * 255.0F);
			if (lum < minLum) {
				minLum = lum;
			}
			if (lum > maxLum) {
				maxLum = lum;
			}
		}
	}
	INFO("min=" << minLum << " max=" << maxLum);
	// Le point blanc sature pendant que le fond reste sombre : degrade ample.
	REQUIRE(maxLum > 0.9F);
	REQUIRE(minLum < 0.3F);
	REQUIRE((maxLum - minLum) > 0.5F);
}
