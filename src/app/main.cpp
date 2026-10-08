#include <exception>
#include <iostream>

#include "rt/app/Options.hpp"
#include "rt/base/Log.hpp"
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

// T026 : ligne de commande complete via `rt::app::parseOptions` (testable).
// Succes -> parse la scene, rend en memoire (T032) et sort 0 (l'ecriture
// `--out` arrive en T034 : le flag est accepte mais n'ecrit pas encore).
// T029 : affiche un resume `ok: <scene>: N objects, M lights, WxH, spp S`
// (supprime par `--quiet`), pour que `./rt scenes/default.rt` "produise
// quelque chose" avant le rendu (DoD T029). `--help`/`--version` -> 0
// sans scene. Erreur CLI -> usage + 2. Erreur de scene/rendu -> message
// `fichier:ligne:colonne` + 1. `./rt` sans argument -> usage + 2
// (documente dans `README.md`).
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
	const rt::render::RenderParams params{
	    .width = width, .height = height, .spp = spp, .maxDepth = scene.limits.maxDepth, .seed = seed};
	rt::render::Framebuffer framebuffer;
	if (rt::Status status = rt::render::render(scene, framebuffer, params); status.isError()) {
		rt::log::error(status.message);
		return 1;
	}
	if (!opts.quiet) {
		std::cout << "ok: " << opts.scenePath << ": " << scene.totalObjectCount()
		          << " objects, " << scene.lights.size() << " lights, " << width << "x"
		          << height << ", spp " << spp << '\n';
	}
	return 0;
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
