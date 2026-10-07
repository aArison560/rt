// Tests du framebuffer prealloue et persistant (T030), Catch2.
// DoD : test de taille/resolution ; ASan propre ; `sizeof` consigne en
// `docs/MEMORY_STRATEGY.md` §4.1. Regles R2 (aucun throw) et R3 (aucune
// allocation par frame : capacites stables apres clear/addSample/present).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <cstddef>

#include "rt/render/Framebuffer.hpp"

TEST_CASE("framebuffer : init valide les dimensions et alloue une fois", "[framebuffer]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.width() == 0);
	REQUIRE(fb.height() == 0);
	REQUIRE(fb.pixelCount() == 0U);

	REQUIRE(fb.init(0, 240).isError());
	REQUIRE(fb.init(320, 0).isError());
	REQUIRE(fb.init(-1, 240).isError());
	REQUIRE(fb.init(8193, 240).isError());
	REQUIRE(fb.init(320, 8193).isError());
	REQUIRE(fb.width() == 0);

	REQUIRE(fb.init(320, 240).isOk());
	REQUIRE(fb.width() == 320);
	REQUIRE(fb.height() == 240);
	REQUIRE(fb.pixelCount() == 320U * 240U);
	REQUIRE(fb.displaySizeBytes() == 320U * 240U * 4U);
	REQUIRE(fb.capacityBytes() == 320U * 240U * 20U);
	REQUIRE(fb.displayData() != nullptr);

	// Echantillon nul au depart.
	REQUIRE(fb.samplesAt(0, 0) == 0);
	REQUIRE(fb.samplesAt(319, 239) == 0);
	REQUIRE(fb.samplesAt(320, 240) == 0);
	REQUIRE(fb.samplesAt(-1, 0) == 0);
}

TEST_CASE("framebuffer : addSample accumule sans allouer", "[framebuffer]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(64, 64).isOk());
	const rt::render::Rgba8* before = fb.displayData();

	fb.addSample(10, 20, rt::Vec3(1.0F, 0.0F, 0.0F));
	REQUIRE(fb.samplesAt(10, 20) == 1);
	REQUIRE(fb.accumAt(10, 20).x == 1.0F);
	REQUIRE(fb.samplesAt(11, 20) == 0);

	fb.addSample(10, 20, rt::Vec3(0.0F, 1.0F, 0.0F));
	REQUIRE(fb.samplesAt(10, 20) == 2);
	REQUIRE(fb.accumAt(10, 20).x == 1.0F);
	REQUIRE(fb.accumAt(10, 20).y == 1.0F);

	// Hors bornes : ignore, jamais de crash.
	fb.addSample(-1, 0, rt::Vec3(1.0F, 1.0F, 1.0F));
	fb.addSample(64, 0, rt::Vec3(1.0F, 1.0F, 1.0F));
	fb.addSample(0, 64, rt::Vec3(1.0F, 1.0F, 1.0F));
	REQUIRE(fb.samplesAt(-1, 0) == 0);

	// Aucune realloc : le pointeur d'affichage est stable (R3).
	REQUIRE(fb.displayData() == before);
}

TEST_CASE("framebuffer : present applique gamma 2.2 et sature", "[framebuffer]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(8, 8).isOk());

	// Blanc pur -> 255 (gamma(1) = 1).
	fb.addSample(0, 0, rt::Vec3(1.0F, 1.0F, 1.0F));
	fb.present();
	REQUIRE(fb.displayData()[0].r == 255);
	REQUIRE(fb.displayData()[0].g == 255);
	REQUIRE(fb.displayData()[0].b == 255);
	REQUIRE(fb.displayData()[0].a == 255);

	// Noir (aucun echantillon) -> 0.
	REQUIRE(fb.displayData()[1].r == 0);
	REQUIRE(fb.displayData()[1].g == 0);

	// Moyenne 0.5 -> gamma(0.5) = 0.5^(1/2.2) ~= 0.73 -> ~186.
	fb.addSample(2, 0, rt::Vec3(1.0F, 0.0F, 0.0F));
	fb.addSample(2, 0, rt::Vec3(0.0F, 1.0F, 0.0F));
	fb.present();
	const float expected = std::pow(0.5F, 1.0F / 2.2F) * 255.0F;
	REQUIRE(std::abs(static_cast<int>(fb.displayData()[2].r) - static_cast<int>(expected)) <= 2);
	REQUIRE(std::abs(static_cast<int>(fb.displayData()[2].g) - static_cast<int>(expected)) <= 2);
	REQUIRE(fb.displayData()[2].b == 0);

	// Saturation : > 1 -> 255, < 0 -> 0.
	fb.addSample(3, 0, rt::Vec3(5.0F, -2.0F, 0.5F));
	fb.present();
	REQUIRE(fb.displayData()[3].r == 255);
	REQUIRE(fb.displayData()[3].g == 0);
}

TEST_CASE("framebuffer : NaN et Inf ne fuient jamais dans l'affichage", "[framebuffer]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(4, 4).isOk());
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	fb.addSample(0, 0, rt::Vec3(nan, inf, -inf));
	fb.addSample(1, 0, rt::Vec3(1.0F, 1.0F, 1.0F));
	fb.present();
	REQUIRE(fb.displayData()[0].r == 0);
	REQUIRE(fb.displayData()[0].g == 0);
	REQUIRE(fb.displayData()[0].b == 0);
	REQUIRE(fb.displayData()[1].r == 255);
}

TEST_CASE("framebuffer : clear reutilise sans realloc, init meme taille aussi", "[framebuffer]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(32, 32).isOk());
	fb.addSample(0, 0, rt::Vec3(1.0F, 1.0F, 1.0F));
	fb.present();
	REQUIRE(fb.displayData()[0].r == 255);
	const rt::render::Rgba8* ptr = fb.displayData();

	fb.clear();
	REQUIRE(fb.displayData() == ptr);
	REQUIRE(fb.samplesAt(0, 0) == 0);
	REQUIRE(fb.accumAt(0, 0).x == 0.0F);
	fb.present();
	REQUIRE(fb.displayData()[0].r == 0);
	REQUIRE(fb.displayData() == ptr);

	// Meme resolution : pas de realloc, juste un clear.
	REQUIRE(fb.init(32, 32).isOk());
	REQUIRE(fb.displayData() == ptr);

	// Nouvelle resolution : reallocation attendue (chemin froid).
	REQUIRE(fb.init(16, 16).isOk());
	REQUIRE(fb.width() == 16);
	REQUIRE(fb.pixelCount() == 256U);
}

TEST_CASE("framebuffer : tailles documentees", "[framebuffer]") {
	REQUIRE(sizeof(rt::render::Rgba8) == 4U);
	// 2 ints + 3 vectors (24 o chacun sur 64 bits) = 80 o.
	// Consigne dans docs/MEMORY_STRATEGY.md §4.1 (DoD T030).
	INFO("sizeof(Framebuffer) = " << sizeof(rt::render::Framebuffer));
	REQUIRE(sizeof(rt::render::Framebuffer) == 80U);

	rt::render::Framebuffer fb;
	REQUIRE(fb.init(320, 240).isOk());
	REQUIRE(fb.displaySizeBytes() == 320U * 240U * 4U);
	REQUIRE(fb.capacityBytes() == 320U * 240U * 20U);
	// 1920x1080 -> display 8 294 400 o (~8 Mo), total ~41 Mo.
	REQUIRE(1920U * 1080U * 4U == 8294400U);
}
