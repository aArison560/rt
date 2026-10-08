// Tests de la boucle de rendu mono-thread (T032) + progressif (T036), Catch2.
// DoD T032 : `./rt scenes/default.rt 64 64 --out /tmp/x.png` -> 0 (verifie
// a la main + via `main`), determinisme (2 rendus identiques octet par
// octet), aucune dependance SDL (R6 : `grep -R SDL src/render` vide).
// DoD T036 : memes spp+seed -> memes pixels (hash), callback par batch,
// spp eleve = moins bruite (convergence du dithering vers le fond).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <vector>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

rt::scene::Scene loadDefault() {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(result.isOk());
	return std::move(result.value());
}

struct ProgressLog {
	std::vector<int> doneValues;
	int totalSeen = 0;
};

void recordProgress(int done, int total, void* user) noexcept {
	auto* log = static_cast<ProgressLog*>(user);
	if (log == nullptr) {
		return;
	}
	// Capacite reservee par le test (aucune allocation dans `render/`,
	// l'allocation eventuelle du `vector` vit dans la callback applicative).
	log->doneValues.push_back(done);
	log->totalSeen = total;
}

double meanAbsDev(const rt::render::Framebuffer& fb, const rt::Vec3& ref) {
	double sum = 0.0;
	const int w = fb.width();
	const int h = fb.height();
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			const int n = fb.samplesAt(x, y);
			const double ax = static_cast<double>(accum.x) / static_cast<double>(n);
			const double ay = static_cast<double>(accum.y) / static_cast<double>(n);
			const double az = static_cast<double>(accum.z) / static_cast<double>(n);
			sum += std::fabs(ax - static_cast<double>(ref.x));
			sum += std::fabs(ay - static_cast<double>(ref.y));
			sum += std::fabs(az - static_cast<double>(ref.z));
		}
	}
	return sum / static_cast<double>(w * h * 3);
}

} // namespace

TEST_CASE("renderer : rend le fond sur une petite image", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1, .maxDepth = 4, .seed = 0};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	REQUIRE(fb.width() == 16);
	REQUIRE(fb.height() == 12);
	// Chaque pixel a recu exactement `spp` echantillons (miss -> fond).
	REQUIRE(fb.samplesAt(0, 0) == 1);
	REQUIRE(fb.samplesAt(15, 11) == 1);
	// T036 : l'accumulation vaut le fond ± dithering deterministe
	// (±0.03 par echantillon, kDitherAmp = 0.06) ; la moyenne converge
	// vers le fond quand spp grandit (test de convergence ci-dessous).
	REQUIRE(fb.accumAt(0, 0).x == Catch::Approx(scene.background.color.x).margin(0.031));
	REQUIRE(fb.accumAt(8, 6).y == Catch::Approx(scene.background.color.y).margin(0.031));
	REQUIRE(fb.accumAt(15, 11).z == Catch::Approx(scene.background.color.z).margin(0.031));
	// Couleurs bornees, jamais de NaN (le `present()` sature deja, T030).
	for (int y = 0; y < 12; ++y) {
		for (int x = 0; x < 16; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			REQUIRE(std::isfinite(accum.x));
			REQUIRE(std::isfinite(accum.y));
			REQUIRE(std::isfinite(accum.z));
		}
	}
}

TEST_CASE("renderer : determinisme (2 rendus identiques)", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 42};
	rt::render::Framebuffer first;
	rt::render::Framebuffer second;
	REQUIRE(rt::render::render(scene, first, params).isOk());
	REQUIRE(rt::render::render(scene, second, params).isOk());
	REQUIRE(first.width() == second.width());
	REQUIRE(first.height() == second.height());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = first.accumAt(x, y);
			const rt::Vec3 b = second.accumAt(x, y);
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
		}
	}
	const std::size_t count = first.pixelCount();
	for (std::size_t i = 0; i < count; ++i) {
		REQUIRE(first.displayData()[i].r == second.displayData()[i].r);
		REQUIRE(first.displayData()[i].g == second.displayData()[i].g);
		REQUIRE(first.displayData()[i].b == second.displayData()[i].b);
	}
}

TEST_CASE("renderer : parametres invalides -> erreur propre, sans crash", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 0, .height = 12, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 0, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 9000, .height = 12, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 0}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 2048}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .maxDepth = 99})
	            .isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .seed = -1})
	            .isError());
}

TEST_CASE("renderer : camera degeneree -> erreur propagee", "[renderer]") {
	rt::scene::Scene scene = loadDefault();
	scene.camera.position = scene.camera.target;
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1};
	REQUIRE(rt::render::render(scene, fb, params).isError());
}

TEST_CASE("renderer progressif : callback par batch (T036)", "[renderer][progress]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	ProgressLog log;
	log.doneValues.reserve(8);
	rt::render::RenderParams params{.width = 16, .height = 12, .spp = 8, .maxDepth = 4, .seed = 7};
	params.onProgress = &recordProgress;
	params.progressUser = &log;
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	// 1 appel par batch, done = 1..spp, total == spp.
	REQUIRE(log.doneValues.size() == 8U);
	REQUIRE(log.totalSeen == 8);
	for (std::size_t i = 0; i < log.doneValues.size(); ++i) {
		REQUIRE(log.doneValues[i] == static_cast<int>(i + 1U));
	}
	// Sans callback : meme image, sans appel.
	rt::render::Framebuffer silent;
	const rt::render::RenderParams quiet{.width = 16, .height = 12, .spp = 8, .maxDepth = 4, .seed = 7};
	REQUIRE(rt::render::render(scene, silent, quiet).isOk());
	for (int y = 0; y < 12; ++y) {
		for (int x = 0; x < 16; ++x) {
			REQUIRE(fb.accumAt(x, y).x == silent.accumAt(x, y).x);
			REQUIRE(fb.accumAt(x, y).y == silent.accumAt(x, y).y);
			REQUIRE(fb.accumAt(x, y).z == silent.accumAt(x, y).z);
		}
	}
}

TEST_CASE("renderer progressif : graine change l'image, meme graine la fige (T036)",
          "[renderer][progress]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer seedA;
	rt::render::Framebuffer seedB;
	rt::render::Framebuffer seedA2;
	REQUIRE(rt::render::render(scene, seedA, {.width = 24, .height = 16, .spp = 4, .maxDepth = 4, .seed = 1}).isOk());
	REQUIRE(rt::render::render(scene, seedB, {.width = 24, .height = 16, .spp = 4, .maxDepth = 4, .seed = 2}).isOk());
	REQUIRE(rt::render::render(scene, seedA2, {.width = 24, .height = 16, .spp = 4, .maxDepth = 4, .seed = 1}).isOk());
	// Meme graine -> octet par octet identique (reproductibilite DoD).
	bool identical = true;
	bool different = false;
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 24; ++x) {
			const rt::Vec3 a = seedA.accumAt(x, y);
			const rt::Vec3 a2 = seedA2.accumAt(x, y);
			const rt::Vec3 b = seedB.accumAt(x, y);
			if (a.x != a2.x || a.y != a2.y || a.z != a2.z) {
				identical = false;
			}
			if (a.x != b.x || a.y != b.y || a.z != b.z) {
				different = true;
			}
		}
	}
	REQUIRE(identical);
	// Graines differentes -> au moins un pixel differe (RNG branche).
	REQUIRE(different);
}

TEST_CASE("renderer progressif : spp eleve moins bruite (T036)", "[renderer][progress]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer low;
	rt::render::Framebuffer high;
	REQUIRE(rt::render::render(scene, low, {.width = 32, .height = 24, .spp = 1, .maxDepth = 4, .seed = 42}).isOk());
	REQUIRE(rt::render::render(scene, high, {.width = 32, .height = 24, .spp = 64, .maxDepth = 4, .seed = 42}).isOk());
	// Ecart moyen au fond : la moyenne de 64 echantillons est plus proche
	// du fond qu'un echantillon unique (convergence 1/sqrt(spp)).
	const double devLow = meanAbsDev(low, scene.background.color);
	const double devHigh = meanAbsDev(high, scene.background.color);
	INFO("devLow=" << devLow << " devHigh=" << devHigh);
	REQUIRE(devHigh < devLow);
}
