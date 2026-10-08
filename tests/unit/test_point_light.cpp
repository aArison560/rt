// Tests de la lumiere ponctuelle + attenuation (T051), Catch2.
// DoD : doubler l'intensite double la contribution (a epsilon, sans NaN) ;
// attenuation `1/(c+l*d+q*d^2)` + `range` (0 = infinie), degenere defini.

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <limits>

#include "rt/lighting/PointLight.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/shading/Material.hpp"

TEST_CASE("pointlight : attenuation par defaut = 1 (pas d'attenuation)", "[pointlight][t051]") {
	const rt::Vec3 def(1.0F, 0.0F, 0.0F);
	REQUIRE(rt::lighting::attenuationFactor(def, 0.0F, 0.0F) == Catch::Approx(1.0F));
	REQUIRE(rt::lighting::attenuationFactor(def, 5.0F, 0.0F) == Catch::Approx(1.0F));
	REQUIRE(rt::lighting::attenuationFactor(def, 1000000.0F, 0.0F) == Catch::Approx(1.0F));
	// `(0,0,0)` : pas d'attenuation non plus (evite la div0, defini).
	REQUIRE(rt::lighting::attenuationFactor(rt::Vec3(0.0F, 0.0F, 0.0F), 5.0F, 0.0F) ==
	        Catch::Approx(1.0F));
}

TEST_CASE("pointlight : attenuation constante/lineaire/quadratique bornee", "[pointlight][t051]") {
	// Quadrique pure : 1/d^2.
	REQUIRE(rt::lighting::attenuationFactor(rt::Vec3(0.0F, 0.0F, 1.0F), 2.0F, 0.0F) ==
	        Catch::Approx(0.25F).epsilon(1e-5));
	// Lineaire pure : 1/d.
	REQUIRE(rt::lighting::attenuationFactor(rt::Vec3(0.0F, 1.0F, 0.0F), 4.0F, 0.0F) ==
	        Catch::Approx(0.25F).epsilon(1e-5));
	// Mixte : 1/(1+0+1) = 0.5 a d=1.
	REQUIRE(rt::lighting::attenuationFactor(rt::Vec3(1.0F, 0.0F, 1.0F), 1.0F, 0.0F) ==
	        Catch::Approx(0.5F).epsilon(1e-5));
	// Decroissance avec la distance (a epsilon) : plus loin = plus sombre.
	const float near =
	    rt::lighting::attenuationFactor(rt::Vec3(0.0F, 0.0F, 1.0F), 2.0F, 0.0F);
	const float far =
	    rt::lighting::attenuationFactor(rt::Vec3(0.0F, 0.0F, 1.0F), 4.0F, 0.0F);
	REQUIRE(far < near);
	REQUIRE(far == Catch::Approx(near * 0.25F).epsilon(1e-5));
}

TEST_CASE("pointlight : portee max (range, 0 = infinie)", "[pointlight][t051]") {
	const rt::Vec3 def(1.0F, 0.0F, 0.0F);
	// Dedans : 1 ; au-dela : 0.
	REQUIRE(rt::lighting::attenuationFactor(def, 9.0F, 10.0F) == Catch::Approx(1.0F));
	REQUIRE(rt::lighting::attenuationFactor(def, 11.0F, 10.0F) == 0.0F);
	// 0 = infinie : jamais coupee.
	REQUIRE(rt::lighting::attenuationFactor(def, 100000.0F, 0.0F) == Catch::Approx(1.0F));
}

TEST_CASE("pointlight : degeneres sans NaN, jamais d'Inf", "[pointlight][t051]") {
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	const rt::Vec3 def(1.0F, 0.0F, 0.0F);
	REQUIRE(rt::lighting::attenuationFactor(def, nan, 0.0F) == 0.0F);
	REQUIRE(rt::lighting::attenuationFactor(def, inf, 0.0F) == 0.0F);
	REQUIRE(rt::lighting::attenuationFactor(def, -1.0F, 0.0F) == 0.0F);
	REQUIRE(rt::lighting::attenuationFactor(rt::Vec3(nan, nan, nan), 2.0F, 0.0F) ==
	        Catch::Approx(1.0F));
	for (float d : {0.0F, 0.001F, 1.0F, 10.0F, 1000.0F}) {
		const float att = rt::lighting::attenuationFactor(def, d, 0.0F);
		REQUIRE(std::isfinite(att));
		REQUIRE_FALSE(std::isnan(att));
		REQUIRE(att >= 0.0F);
	}
}

TEST_CASE("pointlight : doubler l'intensite double la contribution (DoD T051)", "[pointlight][t051]") {
	// Meme garantie qu'en T033, avec attenuation par defaut (facteur 1) :
	// ambiant nul pour isoler le diffus, a epsilon.
	const rt::shading::MaterialParams mat{.albedo = rt::Vec3(0.8F, 0.8F, 0.8F),
	                                      .ambient = 0.0F,
	                                      .diffuse = 0.7F};
	const rt::shading::AmbientParams ambient{.color = rt::Vec3(0.06F, 0.06F, 0.08F),
	                                         .intensity = 1.0F};
	rt::shading::PointLightParams light;
	light.position = rt::Vec3(3.0F, 5.0F, 2.0F);
	light.color = rt::Vec3(1.0F, 1.0F, 1.0F);
	light.intensity = 0.5F;
	light.attenuation = rt::Vec3(1.0F, 0.0F, 0.0F);
	light.range = 0.0F;
	const rt::Vec3 hit(0.0F, 0.0F, 0.0F);
	const rt::Vec3 normal = rt::normalize(light.position - hit);
	const float att = rt::lighting::attenuationFactor(light.attenuation,
	                                                  rt::length(light.position - hit), light.range);
	REQUIRE(att == Catch::Approx(1.0F));
	const rt::Vec3 half = rt::shading::shadeLambert(mat, normal, hit, light, ambient);
	light.intensity = 1.0F;
	const rt::Vec3 full = rt::shading::shadeLambert(mat, normal, hit, light, ambient);
	REQUIRE(full.x == Catch::Approx(half.x * 2.0F).epsilon(1e-5));
	REQUIRE(full.y == Catch::Approx(half.y * 2.0F).epsilon(1e-5));
	REQUIRE(full.z == Catch::Approx(half.z * 2.0F).epsilon(1e-5));
	REQUIRE(std::isfinite(full.x));
	REQUIRE(std::isfinite(full.y));
	REQUIRE(std::isfinite(full.z));
}

TEST_CASE("pointlight : attenuation visible dans l'image, sans NaN (T051)", "[pointlight][t051]") {
	auto renderWith = [](const char* atten, const char* range) {
		std::string content =
		    std::string("scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
		                "background { color (0.02 0.02 0.05) } "
		                "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 attenuation ") +
		    atten + " range " + range + " } } "
		    "objects { object { type sphere center (0 0 0) radius 1 "
		    "material { albedo (0.8 0.8 0.8) diffuse 0.7 ambient 0.1 } } } }";
		rt::Result<rt::scene::Scene> parsed = rt::scene::parseContent(content, "atten.rt");
		REQUIRE(parsed.isOk());
		rt::render::Framebuffer fb;
		REQUIRE(rt::render::render(parsed.value(), fb, {.width = 32, .height = 24, .spp = 4})
		            .isOk());
		const rt::Vec3 accum = fb.accumAt(16, 12);
		const int n = fb.samplesAt(16, 12);
		const rt::Vec3 mean(accum.x / static_cast<float>(n), accum.y / static_cast<float>(n),
		                     accum.z / static_cast<float>(n));
		REQUIRE(std::isfinite(mean.x));
		REQUIRE(std::isfinite(mean.y));
		REQUIRE(std::isfinite(mean.z));
		REQUIRE_FALSE(std::isnan(mean.x));
		return mean;
	};
	// Sans attenuation : le plus clair.
	const rt::Vec3 plain = renderWith("1 0 0", "0");
	// Quadrique : plus sombre a ~6 unites (1/36 ≈ 0.028).
	const rt::Vec3 quad = renderWith("0 0 1", "0");
	INFO("plain r=" << plain.x << " quad r=" << quad.x);
	REQUIRE(quad.x < plain.x);
	// Portee coupee (range 1 << distance ~6) : ambiant seul, plus sombre.
	const rt::Vec3 cut = renderWith("1 0 0", "1");
	INFO("cut r=" << cut.x);
	REQUIRE(cut.x < quad.x);
	// HDR `accum` avant `present()` : le dithering T036 (±0.03) peut pousser
	// un pixel sombre legerement sous 0 ; le `present()` sature [0,1].
	REQUIRE(cut.x > -0.031F);
}
