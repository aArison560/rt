#include <chrono>
#include <cstdio>
#include <exception>
#include <iostream>

#include "rt/app/Options.hpp"
#include "rt/base/Log.hpp"
#include "rt/io/ImageWriter.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"

namespace {

constexpr const char* kVersion = "0.1.0";

int printVersion() {
	std::cout << "rt " << kVersion << '\n';
	return 0;
}

int printHelp() {
	std::cout << rt::app::usageText();
	return 0;
}

int printUsageError(const std::string& message) {
	rt::log::error(message);
	std::cerr << rt::app::usageText();
	return 2;
}

// T036 : progression par batches + ETA (stderr, sauf `--quiet`).
// Appele 1x par batch (spp fois max, hors hot path par pixel), `noexcept`
// (R2) et sans allocation dans `render/` (R3, pointeur brut + `void*`).
// L'UI future (T075/T108) branchera le meme callback sur une barre.
struct ProgressClock {
	std::chrono::steady_clock::time_point start{};
};

void onProgressPrint(int done, int total, void* user) noexcept {
	if (user == nullptr || total <= 0 || done <= 0) {
		return;
	}
	const auto* clock = static_cast<const ProgressClock*>(user);
	const auto now = std::chrono::steady_clock::now();
	const double elapsed =
	    std::chrono::duration<double>(now - clock->start).count();
	double eta = 0.0;
	if (done < total) {
		eta = elapsed * static_cast<double>(total - done) / static_cast<double>(done);
	}
	std::fprintf(stderr, "\rprogress: %d/%d (%.0f%%, eta %.1fs)", done, total,
	             100.0 * static_cast<double>(done) / static_cast<double>(total), eta);
	std::fflush(stderr);
	if (done == total) {
		std::fprintf(stderr, "\n");
		std::fflush(stderr);
	}
}

// T026 : ligne de commande complete via `rt::app::parseOptions` (testable).
// T035 : composition root headless — parse -> load -> render -> write -> exit.
// Aucune initialisation SDL dans ce chemin (R6) : `--out` ecrit et sort,
// `--headless` explicite le mode sans fenetre (defaut avec `--out`, et seul
// mode jusqu'a T070). L'absence de `DISPLAY` n'est donc jamais un echec :
// ce binaire ne lit meme pas cette variable. Succes -> 0, scene/rendu/
// ecriture -> 1 avec message `fichier:ligne:colonne` pour la scene.
// CLI -> usage + 2. `./rt` sans argument -> usage + 2 (README.md).
// T029 : affiche un resume `ok: ...` + `wrote <fichier>` sauf `--quiet`.
// `--help`/`--version` -> 0 sans scene.
int runHeadless(const rt::app::Options& opts) {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile(opts.scenePath);
	if (result.isError()) {
		rt::log::error(result.status().message);
		return 1;
	}
	const rt::scene::Scene& scene = result.value();
	// T032 : boucle de rendu mono-thread, independante de SDL (R6).
	// Resolution/spp/seed : CLI > `limits` (R1). `maxDepth` vient de la
	// scene (aucun `--max-depth` en T026, profondeur bornee pour T056).
	const int width = opts.hasWidth ? opts.width : scene.limits.width;
	const int height = opts.hasHeight ? opts.height : scene.limits.height;
	const int spp = opts.hasSpp ? opts.spp : scene.limits.samples;
	const long long seed = opts.hasSeed ? opts.seed : scene.limits.seed;
	// T036 : `--spp`/`--seed` -> batches progressifs, reproductibles
	// (meme spp + meme seed = memes pixels). Callback + ETA sur stderr
	// sauf `--quiet` (futur affichage T075/T108).
	ProgressClock clock{std::chrono::steady_clock::now()};
	rt::render::RenderParams params{
	    .width = width, .height = height, .spp = spp, .maxDepth = scene.limits.maxDepth, .seed = seed};
	if (!opts.quiet) {
		params.onProgress = &onProgressPrint;
		params.progressUser = &clock;
	}
	rt::render::Framebuffer framebuffer;
	if (rt::Status status = rt::render::render(scene, framebuffer, params); status.isError()) {
		rt::log::error(status.message);
		return 1;
	}
	// T034 : `--out` -> PNG (libpng) ou PPM (`.ppm` / fallback sans lib).
	// Echec (repertoire inexistant, chemin vide) -> message + 1, sans crash.
	if (opts.hasOut) {
		if (rt::Status status = rt::io::writeImage(framebuffer, opts.outPath); status.isError()) {
			rt::log::error(status.message);
			return 1;
		}
	}
	if (!opts.quiet) {
		std::cout << "ok: " << opts.scenePath << ": " << scene.totalObjectCount()
		          << " objects, " << scene.lights.size() << " lights, " << width << "x"
		          << height << ", spp " << spp;
		if (opts.hasOut) {
			std::cout << ", wrote " << opts.outPath;
		}
		std::cout << '\n';
	}
	return 0;
}

// Parse CLI (T026) puis composition root headless (T035). Jusqu'a T070,
// tout appel avec scene passe par `runHeadless` : `--headless` ou non,
// avec ou sans `--out`, avec ou sans `DISPLAY` — aucun chemin n'ouvre
// de fenetre. T070 branchera ici le mode fenetre quand `--out` est absent
// et `--headless` n'est pas demande.
int run(int argc, char** argv) {
	rt::Result<rt::app::Options> parsed = rt::app::parseOptions(argc, argv);
	if (parsed.isError()) {
		return printUsageError(parsed.status().message);
	}
	const rt::app::Options& opts = parsed.value();
	if (opts.showHelp) {
		return printHelp();
	}
	if (opts.showVersion) {
		return printVersion();
	}
	return runHeadless(opts);
}

} // namespace

// Filet unique (règle R2, T015) : seul `try/catch` autorisé du dépôt.
// Tout le hot path rapporte par `rt::Status`/`bool`, jamais par exception.
int main(int argc, char** argv) {
	try {
		return run(argc, argv);
	} catch (const std::exception& e) {
		rt::log::error(e.what());
		return 1;
	} catch (...) {
		rt::log::error("unknown exception");
		return 1;
	}
}
