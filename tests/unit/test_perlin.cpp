// Bruit de Perlin (T106, *Disruptions* 3-4), Catch2.
// Determinisme (meme graine -> meme suite), independance (deux pixels
// voisins different), scene `opt_perlin.rt`.

#include <catch2/catch_amalgamated.hpp>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/shading/Pattern.hpp"

TEST_CASE("perlin : determinisme par graine (T106)", "[pattern][t106]") {
	rt::shading::Perlin first;
	first.init(42ULL);
	rt::shading::Perlin second;
	second.init(42ULL);
	// Meme graine -> meme table -> memes valeurs (couture tuiles OK).
	for (const rt::Vec3 probe :
	     {rt::Vec3(0.0F, 0.0F, 0.0F), rt::Vec3(1.3F, -2.1F, 0.7F), rt::Vec3(5.0F, 5.0F, 5.0F)}) {
		REQUIRE(rt::shading::perlinValue(first, probe) ==
		        Catch::Approx(rt::shading::perlinValue(second, probe)));
		REQUIRE(rt::shading::perlinFractal(first, probe, 3) ==
		        Catch::Approx(rt::shading::perlinFractal(second, probe, 3)));
	}
	// Deux pixels voisins : valeurs (tres probablement) differentes.
	const float nearA = rt::shading::perlinValue(first, rt::Vec3(0.3F, 0.7F, 0.2F));
	const float nearB = rt::shading::perlinValue(first, rt::Vec3(0.8F, 0.7F, 0.2F));
	REQUIRE(nearA != nearB);
	// Meme echantillon dans une tuile pleine ou une bande : la valeur ne
	// depend que des coordonnees absolues (pas de l'ordre d'evaluation).
	const rt::Vec3 p(3.25F, -1.5F, 2.0F);
	REQUIRE(rt::shading::perlinFractal(first, p, 2) ==
	        Catch::Approx(rt::shading::perlinFractal(second, p, 2)));
	// Bornes : fractal en [-1,1], jamais de NaN.
	for (int i = 0; i < 50; ++i) {
		const rt::Vec3 q(static_cast<float>(i) * 0.37F, static_cast<float>(i) * -0.11F,
		                 static_cast<float>(i) * 0.23F);
		const float v = rt::shading::perlinFractal(first, q, 3);
		REQUIRE(std::isfinite(v));
		REQUIRE(v >= -1.0F);
		REQUIRE(v <= 1.0F);
	}
}

TEST_CASE("perlin : scene marbree rend sans NaN (T106)", "[pattern][t106]") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/opt_perlin.rt");
	INFO((result.isError() ? result.status().message : std::string("ok")));
	REQUIRE(result.isOk());
	rt::render::Framebuffer first;
	rt::render::RenderParams params{.width = 80, .height = 60, .spp = 1, .maxDepth = 2, .seed = 14};
	REQUIRE(rt::render::render(result.value(), first, params, nullptr).isOk());
	// Determinisme de bout en bout : deux rendus memes seed = octet par octet.
	rt::render::Framebuffer second;
	REQUIRE(rt::render::render(result.value(), second, params, nullptr).isOk());
	for (int y = 0; y < first.height(); ++y) {
		for (int x = 0; x < first.width(); ++x) {
			const rt::Vec3 a = first.accumAt(x, y);
			const rt::Vec3 b = second.accumAt(x, y);
			REQUIRE(a.x == Catch::Approx(b.x));
			REQUIRE(a.y == Catch::Approx(b.y));
			REQUIRE(a.z == Catch::Approx(b.z));
		}
	}
}
