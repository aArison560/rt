#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "rt/base/Log.hpp"
#include "rt/scene/Lexer.hpp"

namespace {

constexpr const char* kVersion = "0.1.0";

int printVersion() {
	std::cout << "rt " << kVersion << '\n';
	return 0;
}

int printUsage() {
	std::cout << "usage: rt [--version] [--help] [scene.rt ...]\n";
	return 0;
}

// T022 : cablage minimal du lexer (le parser complet arrive en T023,
// la CLI complete en T026). `./rt scene.rt` ne fait que lexer : succes -> 0,
// erreur lexicale -> message `fichier:ligne:colonne` sur stderr et 1.
// `./rt` seul et `./rt --version` gardent le comportement historique (0).
int run(int argc, char** argv) {
	if (argc <= 1) {
	return printVersion();
	}
	const std::string_view first(argv[1]);
	if (argc == 2 && (first == "--version" || first == "-v")) {
	return printVersion();
	}
	if (argc == 2 && (first == "--help" || first == "-h")) {
	return printUsage();
	}
	bool sawFile = false;
	for (int i = 1; i < argc; ++i) {
	const std::string_view arg(argv[i]);
	if (!sawFile && arg == "--") {
	    continue;
	}
	if (!sawFile && arg.size() > 0 && arg[0] == '-' && arg != "--") {
	    // Option inconnue a ce stade (T026) : erreur propre, pas de crash.
	    std::string message("unknown option '");
	    message.append(arg);
	    message.append("' (try --help)");
	    rt::log::error(message);
	    return 2;
	}
	sawFile = true;
	rt::Result<std::vector<rt::scene::Token>> result = rt::scene::lexFile(arg);
	if (result.isError()) {
	    rt::log::error(result.status().message);
	    return 1;
	}
	}
	if (!sawFile) {
	return printVersion();
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
