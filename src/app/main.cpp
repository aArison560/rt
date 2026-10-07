#include <exception>
#include <iostream>

#include "rt/app/Options.hpp"
#include "rt/base/Log.hpp"
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
// Succes -> parse la scene et sort 0 (le rendu viendra en T032).
// T029 : affiche un resume `ok: <scene>: N objects, M lights, WxH, spp S`
// (supprime par `--quiet`), pour que `./rt scenes/default.rt` "produise
// quelque chose" avant le rendu (DoD T029). `--help`/`--version` -> 0
// sans scene. Erreur CLI -> usage + 2. Erreur de scene -> message
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
	if (!opts.quiet) {
		const int width = opts.hasWidth ? opts.width : scene.limits.width;
		const int height = opts.hasHeight ? opts.height : scene.limits.height;
		const int spp = opts.hasSpp ? opts.spp : scene.limits.samples;
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
