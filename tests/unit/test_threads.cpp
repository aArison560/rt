// Tests du ThreadPool et du rendu par tuiles (T063), Catch2.
// DoD : `--threads 1/2/4/8` = images **identiques** (graine absolue T016) ;
// TSan vert (tuiles disjointes, contexte lecture seule). Sans `throw` (R2),
// file + conditions (pas de spin), erreur par tache sans `terminate`.

#include <catch2/catch_amalgamated.hpp>

#include <atomic>
#include <stdexcept>
#include <string>
#include <vector>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/sched/ThreadPool.hpp"

namespace {

rt::scene::Scene loadTwoSpheres() {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseContent(
	    "scene { camera { position (0 1 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 6) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (-1.2 0 0) radius 1 material { albedo (0.9 0.1 0.1) } } "
	    "object { type sphere center (1.2 0 0) radius 1 material { albedo (0.1 0.1 0.9) } } "
	    "} }",
	    "two.rt");
	REQUIRE(parsed.isOk());
	return std::move(parsed.value());
}

void renderWithThreads(const rt::scene::Scene& scene, int threads, rt::render::Framebuffer& fb) {
	const rt::render::RenderParams params{
	    .width = 48, .height = 36, .spp = 2, .maxDepth = 4, .seed = 11, .threads = threads};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
}

bool framebuffersEqual(const rt::render::Framebuffer& a, const rt::render::Framebuffer& b) {
	if (a.width() != b.width() || a.height() != b.height()) {
		return false;
	}
	for (int y = 0; y < a.height(); ++y) {
		for (int x = 0; x < a.width(); ++x) {
			const rt::Vec3 va = a.accumAt(x, y);
			const rt::Vec3 vb = b.accumAt(x, y);
			if (va.x != vb.x || va.y != vb.y || va.z != vb.z) {
				return false;
			}
		}
	}
	const std::size_t count = a.pixelCount();
	for (std::size_t i = 0; i < count; ++i) {
		if (a.displayData()[i].r != b.displayData()[i].r ||
		    a.displayData()[i].g != b.displayData()[i].g ||
		    a.displayData()[i].b != b.displayData()[i].b) {
			return false;
		}
	}
	return true;
}

} // namespace

TEST_CASE("threadpool : taches paralleles sans perte (T063)", "[threads][t063]") {
	rt::sched::ThreadPool pool(4);
	REQUIRE(pool.size() == 4);
	std::atomic<int> counter{0};
	for (int i = 0; i < 100; ++i) {
		pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
	}
	pool.waitIdle();
	REQUIRE(counter.load() == 100);
	REQUIRE(!pool.hasError());
	// Reutilisable entre batches (2e vague sur le meme pool).
	for (int i = 0; i < 50; ++i) {
		pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
	}
	pool.waitIdle();
	REQUIRE(counter.load() == 150);
	REQUIRE(!pool.hasError());
}

TEST_CASE("threadpool : exception par tache sans terminate (T063)", "[threads][t063]") {
	rt::sched::ThreadPool pool(2);
	std::atomic<int> done{0};
	pool.submit([&done] { done.fetch_add(1, std::memory_order_relaxed); });
	pool.submit([] {
		throw std::runtime_error("boom-task");
	});
	pool.submit([&done] { done.fetch_add(1, std::memory_order_relaxed); });
	pool.waitIdle();
	// Les 2 taches saines ont tourne malgre la levee (pas de `terminate`).
	REQUIRE(done.load() == 2);
	REQUIRE(pool.hasError());
	REQUIRE(!pool.firstError().empty());
}

TEST_CASE("renderer threads : 1/2/4/8 identiques octet par octet (T063 DoD)",
          "[threads][t063]") {
	const rt::scene::Scene scene = loadTwoSpheres();
	rt::render::Framebuffer ref;
	rt::render::Framebuffer two;
	rt::render::Framebuffer four;
	rt::render::Framebuffer eight;
	renderWithThreads(scene, 1, ref);
	renderWithThreads(scene, 2, two);
	renderWithThreads(scene, 4, four);
	renderWithThreads(scene, 8, eight);
	INFO("48x36 spp2 seed11 : 1 vs 2/4/8 threads");
	REQUIRE(framebuffersEqual(ref, two));
	REQUIRE(framebuffersEqual(ref, four));
	REQUIRE(framebuffersEqual(ref, eight));
}

TEST_CASE("renderer threads : bornes 1..256 (T063)", "[threads][t063]") {
	const rt::scene::Scene scene = loadTwoSpheres();
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .threads = 0})
	            .isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .threads = 257})
	            .isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .threads = 256})
	            .isOk());
}

TEST_CASE("renderer stats : compteurs coherents (T064)", "[threads][t064]") {
	const rt::scene::Scene scene = loadTwoSpheres();
	rt::render::Framebuffer fb;
	rt::render::RenderStats stats;
	const rt::render::RenderParams params{
	    .width = 48, .height = 36, .spp = 2, .maxDepth = 4, .seed = 11, .threads = 2};
	REQUIRE(rt::render::render(scene, fb, params, &stats).isOk());
	// `primaryRays` deterministe, objets/lumieres lus de la scene, threads
	// repris des params, temps positifs, rays/s coherent.
	REQUIRE(stats.primaryRays == 48 * 36 * 2);
	REQUIRE(stats.objects == static_cast<int>(scene.totalObjectCount()));
	REQUIRE(stats.lights == static_cast<int>(scene.lights.size()));
	REQUIRE(stats.threadsUsed == 2);
	REQUIRE(stats.buildMs >= 0.0);
	REQUIRE(stats.renderMs > 0.0);
	REQUIRE(stats.totalMs >= stats.renderMs);
	REQUIRE(stats.raysPerSec > 0.0);
	// Sans `stats` : meme image (le compteur ne change rien aux pixels).
	rt::render::Framebuffer plain;
	REQUIRE(rt::render::render(scene, plain, params).isOk());
	REQUIRE(framebuffersEqual(fb, plain));
}
