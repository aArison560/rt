// UV sur les 4 primitives + application texture (T103, *Textures* 1-2).
// Chaque primitive genere des UV (T041-T044) ; `sampleTexture` pave et
// echantillonne ; le renderer remplace l'albedo par le texel. Test :
// damier visible sur les 4 objets (variance), sans distorsion aberrante
// (pas de NaN, couleurs bornees).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <cstdint>

#include "rt/io/Texture.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"

namespace {

rt::io::TextureImage makeTwoByTwo() {
	rt::io::TextureImage image;
	image.width = 2;
	image.height = 2;
	// Ligne 0 = haut : rouge, vert ; ligne 1 = bas : bleu, blanc.
	image.rgba = {
	    255, 0, 0, 255, 0, 255, 0, 255,
	    0, 0, 255, 255, 255, 255, 255, 255,
	};
	return image;
}

} // namespace

TEST_CASE("texture : echantillonnage avec pavage (T103)", "[texture][t103]") {
	const rt::io::TextureImage image = makeTwoByTwo();
	// v=0 = bas (convention GL, retourne) : (0,0)=bleu, (0.5,0)=blanc,
	// (0,0.5)=rouge, (0.5,0.5)=vert.
	const rt::Vec3 blue = rt::io::sampleTexture(image, 0.0F, 0.0F);
	REQUIRE(blue.x == Catch::Approx(0.0F));
	REQUIRE(blue.z == Catch::Approx(1.0F));
	const rt::Vec3 white = rt::io::sampleTexture(image, 0.5F, 0.0F);
	REQUIRE(white.x == Catch::Approx(1.0F));
	REQUIRE(white.y == Catch::Approx(1.0F));
	REQUIRE(white.z == Catch::Approx(1.0F));
	const rt::Vec3 red = rt::io::sampleTexture(image, 0.0F, 0.5F);
	REQUIRE(red.x == Catch::Approx(1.0F));
	REQUIRE(red.y == Catch::Approx(0.0F));
	// Pavage : u=1.25 == u=0.25 (meme colonne 0 : bleu a v=0).
	const rt::Vec3 tiled = rt::io::sampleTexture(image, 1.25F, 0.0F);
	REQUIRE(tiled.x == Catch::Approx(blue.x));
	REQUIRE(tiled.z == Catch::Approx(blue.z));
	// Negatifs : u=-0.75 == u=0.25.
	const rt::Vec3 neg = rt::io::sampleTexture(image, -0.75F, 0.0F);
	REQUIRE(neg.x == Catch::Approx(blue.x));
	// Degeneres : image vide et NaN -> noir defini, jamais de NaN.
	const rt::io::TextureImage empty;
	const rt::Vec3 black = rt::io::sampleTexture(empty, 0.5F, 0.5F);
	REQUIRE(black.x == Catch::Approx(0.0F));
	const rt::Vec3 nan = rt::io::sampleTexture(image, std::nanf(""), 0.5F);
	REQUIRE(std::isfinite(nan.x));
}

TEST_CASE("texture : damier sur les 4 objets (T103)", "[texture][t103]") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/opt_textures4.rt");
	INFO((result.isError() ? result.status().message : std::string("ok")));
	REQUIRE(result.isOk());
	rt::scene::Scene& scene = result.value();
	rt::render::Framebuffer fb;
	rt::render::RenderParams params{.width = 80, .height = 60, .spp = 2, .maxDepth = 2, .seed = 11};
	REQUIRE(rt::render::render(scene, fb, params, nullptr).isOk());
	// Pas de NaN, couleurs bornees (present() sature).
	int nonBackground = 0;
	double sumR = 0.0;
	double sumG = 0.0;
	double sumB = 0.0;
	const int total = fb.width() * fb.height();
	for (int y = 0; y < fb.height(); ++y) {
		for (int x = 0; x < fb.width(); ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			const int n = fb.samplesAt(x, y);
			const double r = static_cast<double>(accum.x) / static_cast<double>(n);
			const double g = static_cast<double>(accum.y) / static_cast<double>(n);
			const double b = static_cast<double>(accum.z) / static_cast<double>(n);
			REQUIRE(std::isfinite(r));
			REQUIRE(std::isfinite(g));
			REQUIRE(std::isfinite(b));
			sumR += r;
			sumG += g;
			sumB += b;
			// Fond = (0.02,0.02,0.05) : tout pixel nettement au-dessus
			// est un objet (eclaire).
			if (r > 0.1 || g > 0.1 || b > 0.12) {
				++nonBackground;
			}
		}
	}
	// Les 4 objets couvrent une part significative de l'image.
	REQUIRE(nonBackground > total / 20);
	// L'image moyenne n'est ni noire ni saturee (textures visibles).
	const double meanR = sumR / static_cast<double>(total);
	REQUIRE(meanR > 0.03);
	REQUIRE(meanR < 0.95);
	(void)sumG;
	(void)sumB;
}

TEST_CASE("texture : avec vs sans texture les pixels different (T103)", "[texture][t103]") {
	rt::Result<rt::scene::Scene> textured = rt::scene::parseFile("scenes/opt_textures4.rt");
	REQUIRE(textured.isOk());
	rt::render::Framebuffer fbTex;
	rt::render::RenderParams params{.width = 80, .height = 60, .spp = 2, .maxDepth = 2, .seed = 11};
	REQUIRE(rt::render::render(textured.value(), fbTex, params, nullptr).isOk());
	// Meme scene sans textures : albedo gris unis.
	rt::scene::Scene plain = textured.value();
	for (auto& obj : plain.objects) {
		obj.material.texture.present = false;
		obj.material.texture.file.clear();
		obj.material.albedo = rt::Vec3(0.6F, 0.6F, 0.6F);
	}
	rt::render::Framebuffer fbPlain;
	REQUIRE(rt::render::render(plain, fbPlain, params, nullptr).isOk());
	// Les deux images different sur plus de 5% des pixels (la texture a
	// un effet visible, pas un simple re-étiquetage).
	int different = 0;
	const int total = fbTex.width() * fbTex.height();
	for (int y = 0; y < fbTex.height(); ++y) {
		for (int x = 0; x < fbTex.width(); ++x) {
			const rt::Vec3 a = fbTex.accumAt(x, y);
			const rt::Vec3 b = fbPlain.accumAt(x, y);
			const int na = fbTex.samplesAt(x, y);
			const int nb = fbPlain.samplesAt(x, y);
			const double dr = static_cast<double>(a.x) / static_cast<double>(na) -
			                  static_cast<double>(b.x) / static_cast<double>(nb);
			const double dg = static_cast<double>(a.y) / static_cast<double>(na) -
			                  static_cast<double>(b.y) / static_cast<double>(nb);
			const double db = static_cast<double>(a.z) / static_cast<double>(na) -
			                  static_cast<double>(b.z) / static_cast<double>(nb);
			if (std::fabs(dr) + std::fabs(dg) + std::fabs(db) > 0.05) {
				++different;
			}
		}
	}
	REQUIRE(different > total / 20);
}
