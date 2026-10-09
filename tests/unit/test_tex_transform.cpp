// Echelle et decalage de texture par objet (T104, *Textures* 3-4).
// `u' = u*sx + ox`, `v' = v*sy + oy` avant `sampleTexture`. Test : UV
// transformees + scene 4 cas (etire/decale, separement).

#include <catch2/catch_amalgamated.hpp>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"

namespace {

int countDifferent(const rt::render::Framebuffer& a, const rt::render::Framebuffer& b,
                   double threshold) {
	int different = 0;
	for (int y = 0; y < a.height(); ++y) {
		for (int x = 0; x < a.width(); ++x) {
			const rt::Vec3 ca = a.accumAt(x, y);
			const rt::Vec3 cb = b.accumAt(x, y);
			const int na = a.samplesAt(x, y);
			const int nb = b.samplesAt(x, y);
			const double dr = static_cast<double>(ca.x) / static_cast<double>(na) -
			                  static_cast<double>(cb.x) / static_cast<double>(nb);
			const double dg = static_cast<double>(ca.y) / static_cast<double>(na) -
			                  static_cast<double>(cb.y) / static_cast<double>(nb);
			const double db = static_cast<double>(ca.z) / static_cast<double>(na) -
			                  static_cast<double>(cb.z) / static_cast<double>(nb);
			if (std::fabs(dr) + std::fabs(dg) + std::fabs(db) > threshold) {
				++different;
			}
		}
	}
	return different;
}

} // namespace

TEST_CASE("texture : scale et offset lus par objet (T104)", "[texture][t104]") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/opt_tex_transform.rt");
	INFO((result.isError() ? result.status().message : std::string("ok")));
	REQUIRE(result.isOk());
	const rt::scene::Scene& scene = result.value();
	REQUIRE(scene.objects.size() == 4);
	REQUIRE(scene.objects[0].material.texture.present);
	REQUIRE(scene.objects[0].material.texture.scale.x == Catch::Approx(1.0F));
	REQUIRE(scene.objects[1].material.texture.scale.x == Catch::Approx(4.0F));
	REQUIRE(scene.objects[1].material.texture.scale.y == Catch::Approx(4.0F));
	REQUIRE(scene.objects[2].material.texture.offset.x == Catch::Approx(0.5F));
	REQUIRE(scene.objects[2].material.texture.offset.y == Catch::Approx(0.0F));
	REQUIRE(scene.objects[3].material.texture.offset.x == Catch::Approx(0.0F));
	REQUIRE(scene.objects[3].material.texture.offset.y == Catch::Approx(0.5F));
}

TEST_CASE("texture : etire et decale changent l'image (T104)", "[texture][t104]") {
	rt::Result<rt::scene::Scene> base = rt::scene::parseFile("scenes/opt_tex_transform.rt");
	REQUIRE(base.isOk());
	rt::render::Framebuffer fbBase;
	rt::render::RenderParams params{.width = 160, .height = 120, .spp = 2, .maxDepth = 2, .seed = 12};
	REQUIRE(rt::render::render(base.value(), fbBase, params, nullptr).isOk());
	// Meme scene avec scale 1 partout et offset 0 : l'etire (scale 4) et
	// les decales doivent en differer visiblement.
	rt::scene::Scene flat = base.value();
	for (auto& obj : flat.objects) {
		obj.material.texture.scale = rt::Vec3(1.0F, 1.0F, 0.0F);
		obj.material.texture.offset = rt::Vec3(0.0F, 0.0F, 0.0F);
	}
	rt::render::Framebuffer fbFlat;
	REQUIRE(rt::render::render(flat, fbFlat, params, nullptr).isOk());
	const int total = fbBase.width() * fbBase.height();
	REQUIRE(countDifferent(fbBase, fbFlat, 0.05) > total / 100);
	// Offset seul (0.5,0) sur damier 2-periodique : decale d'une demi-tuile,
	// l'image change aussi (verifie separement de l'echelle).
	rt::scene::Scene shifted = base.value();
	for (auto& obj : shifted.objects) {
		obj.material.texture.scale = rt::Vec3(1.0F, 1.0F, 0.0F);
		obj.material.texture.offset = rt::Vec3(0.5F, 0.0F, 0.0F);
	}
	rt::render::Framebuffer fbShifted;
	REQUIRE(rt::render::render(shifted, fbShifted, params, nullptr).isOk());
	REQUIRE(countDifferent(fbFlat, fbShifted, 0.05) > total / 100);
}
