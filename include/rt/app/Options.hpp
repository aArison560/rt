#pragma once

// Ligne de commande complete (T026) — lecture des arguments separee du reste.
// `parseOptions()` ne fait que lire `argv` (testable unitairement) : aucun
// acces fichier, aucune initialisation SDL, aucun `throw` (R2). Les bornes
// reprennent la table unique `schema/` (R1) : width/height 1..8192,
// samples 1..1024, seed 0..4294967295. `threads` 1..256 et `tile` n 1..64
// (decoupage vertical sans recouvrement, 0 pixel, cf.
// `docs/DISTRIBUTED_RENDERING.md` §2.1). Toute valeur invalide -> `Status`
// `InvalidArgument` avec message explicite ; `main` affiche l'erreur puis
// l'usage et sort 2. `./rt` sans argument -> erreur + usage (code 2).

#include <string>
#include <string_view>
#include <vector>

#include "rt/base/Result.hpp"

namespace rt::app {

struct Options {
	std::string scenePath;
	bool hasScene = false;
	int width = 0;
	bool hasWidth = false;
	int height = 0;
	bool hasHeight = false;
	std::string outPath;
	bool hasOut = false;
	int spp = 0;
	bool hasSpp = false;
	long long seed = 0;
	bool hasSeed = false;
	int threads = 0;
	bool hasThreads = false;
	int tileIndex = 0;
	int tileCount = 1;
	bool hasTile = false;
	bool headless = false;
	bool quiet = false;
	bool showHelp = false;
	bool showVersion = false;
};

// Point d'entree reel (`main`) : `argv[0]` = nom du programme.
[[nodiscard]] Result<Options> parseOptions(int argc, char const* const* argv);

// Point d'entree des tests : `args[0]` = nom du programme (comme `argv`).
[[nodiscard]] Result<Options> parseOptionsVec(const std::vector<std::string>& args);

// Texte complet de `--help` (affiche par `main`, teste unitairement).
[[nodiscard]] std::string usageText();

} // namespace rt::app
