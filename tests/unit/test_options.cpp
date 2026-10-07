// Tests de la ligne de commande (T026), Catch2.
// Couvre le Prompt et le DoD : `./rt <scene.rt> [width height]`
// `[--out --spp --seed --threads --tile --headless --quiet --version --help]`,
// valeurs invalides -> erreur + usage, `--help` complet, lecture separee
// (testable unitairement via `parseOptionsVec`).

#include <catch2/catch_amalgamated.hpp>

#include <string>
#include <vector>

#include "rt/app/Options.hpp"

namespace {

rt::Result<rt::app::Options> parse(std::initializer_list<const char*> items) {
	std::vector<std::string> args;
	args.reserve(items.size());
	for (const char* item : items) {
		args.emplace_back(item);
	}
	return rt::app::parseOptionsVec(args);
}

bool ok(std::initializer_list<const char*> items, rt::app::Options& out) {
	rt::Result<rt::app::Options> result = parse(items);
	if (result.isError()) {
		INFO("erreur inattendue : " << result.status().message);
		return false;
	}
	out = std::move(result.value());
	return true;
}

std::string fail(std::initializer_list<const char*> items) {
	rt::Result<rt::app::Options> result = parse(items);
	REQUIRE(result.isError());
	REQUIRE_FALSE(result.status().message.empty());
	return result.status().message;
}

} // namespace

TEST_CASE("options : scene seule et defauts", "[options]") {
	rt::app::Options opts;
	REQUIRE(ok({"rt", "scenes/m.rt"}, opts));
	REQUIRE(opts.hasScene);
	REQUIRE(opts.scenePath == "scenes/m.rt");
	REQUIRE_FALSE(opts.hasWidth);
	REQUIRE_FALSE(opts.hasHeight);
	REQUIRE_FALSE(opts.hasOut);
	REQUIRE_FALSE(opts.hasSpp);
	REQUIRE_FALSE(opts.hasSeed);
	REQUIRE_FALSE(opts.hasThreads);
	REQUIRE_FALSE(opts.hasTile);
	REQUIRE_FALSE(opts.headless);
	REQUIRE_FALSE(opts.quiet);
	REQUIRE_FALSE(opts.showHelp);
	REQUIRE_FALSE(opts.showVersion);
	REQUIRE(opts.tileCount == 1);
}

TEST_CASE("options : width height positionnels", "[options]") {
	rt::app::Options opts;
	REQUIRE(ok({"rt", "s.rt", "640", "480"}, opts));
	REQUIRE(opts.hasWidth);
	REQUIRE(opts.hasHeight);
	REQUIRE(opts.width == 640);
	REQUIRE(opts.height == 480);
	// Un seul des deux -> erreur (doivent aller ensemble).
	REQUIRE(fail({"rt", "s.rt", "640"}).find("together") != std::string::npos);
	// Hors bornes 1..8192.
	REQUIRE(fail({"rt", "s.rt", "0", "480"}).find("bad width") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "640", "99999"}).find("bad height") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "abc", "480"}).find("bad width") != std::string::npos);
	// Trop de positionnels.
	REQUIRE(fail({"rt", "s.rt", "640", "480", "extra"}).find("too many") != std::string::npos);
}

TEST_CASE("options : toutes les flags longues", "[options]") {
	rt::app::Options opts;
	REQUIRE(ok({"rt", "s.rt", "--out", "a.png", "--spp", "16", "--seed", "42",
	            "--threads", "4", "--tile", "1/4", "--headless", "--quiet"},
	           opts));
	REQUIRE(opts.hasOut);
	REQUIRE(opts.outPath == "a.png");
	REQUIRE(opts.hasSpp);
	REQUIRE(opts.spp == 16);
	REQUIRE(opts.hasSeed);
	REQUIRE(opts.seed == 42);
	REQUIRE(opts.hasThreads);
	REQUIRE(opts.threads == 4);
	REQUIRE(opts.hasTile);
	REQUIRE(opts.tileIndex == 1);
	REQUIRE(opts.tileCount == 4);
	REQUIRE(opts.headless);
	REQUIRE(opts.quiet);
	// Formes `--opt=value`.
	REQUIRE(ok({"rt", "s.rt", "--out=a.png", "--spp=8", "--seed=7"}, opts));
	REQUIRE(opts.outPath == "a.png");
	REQUIRE(opts.spp == 8);
	REQUIRE(opts.seed == 7);
	// Alias --width/--height (compat cluster, DISTRIBUTED §2.1).
	REQUIRE(ok({"rt", "s.rt", "--width", "800", "--height", "600"}, opts));
	REQUIRE(opts.width == 800);
	REQUIRE(opts.height == 600);
	REQUIRE(ok({"rt", "s.rt", "--width=320", "--height=200"}, opts));
	REQUIRE(opts.width == 320);
}

TEST_CASE("options : help et version sans scene", "[options]") {
	rt::app::Options help;
	REQUIRE(ok({"rt", "--help"}, help));
	REQUIRE(help.showHelp);
	rt::app::Options helpShort;
	REQUIRE(ok({"rt", "-h"}, helpShort));
	REQUIRE(helpShort.showHelp);
	rt::app::Options version;
	REQUIRE(ok({"rt", "--version"}, version));
	REQUIRE(version.showVersion);
	rt::app::Options versionShort;
	REQUIRE(ok({"rt", "-v"}, versionShort));
	REQUIRE(versionShort.showVersion);
	// Usage complet : chaque option documentee.
	const std::string text = rt::app::usageText();
	REQUIRE(text.find("--out") != std::string::npos);
	REQUIRE(text.find("--spp") != std::string::npos);
	REQUIRE(text.find("--seed") != std::string::npos);
	REQUIRE(text.find("--threads") != std::string::npos);
	REQUIRE(text.find("--tile") != std::string::npos);
	REQUIRE(text.find("--headless") != std::string::npos);
	REQUIRE(text.find("--quiet") != std::string::npos);
	REQUIRE(text.find("--version") != std::string::npos);
	REQUIRE(text.find("--help") != std::string::npos);
	REQUIRE(text.find("usage:") != std::string::npos);
}

TEST_CASE("options : erreurs d'usage", "[options]") {
	// Sans argument -> erreur (DoD : usage + code != 0).
	REQUIRE(fail({"rt"}).find("no scene") != std::string::npos);
	// Option inconnue.
	REQUIRE(fail({"rt", "s.rt", "--nope"}).find("unknown option") != std::string::npos);
	// Valeurs manquantes.
	REQUIRE(fail({"rt", "s.rt", "--out"}).find("missing value") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--spp"}).find("missing value") != std::string::npos);
	// Bornes : spp 1..1024, seed 0..4294967295, threads 1..256.
	REQUIRE(fail({"rt", "s.rt", "--spp", "0"}).find("bad --spp") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--spp", "99999"}).find("bad --spp") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--spp", "abc"}).find("bad --spp") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--seed", "-1"}).find("bad --seed") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--seed", "99999999999"}).find("bad --seed") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--threads", "0"}).find("bad --threads") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--threads", "999"}).find("bad --threads") != std::string::npos);
	// Doublons refuses.
	REQUIRE(fail({"rt", "s.rt", "--spp", "4", "--spp", "8"}).find("duplicate") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--out", "a", "--out", "b"}).find("duplicate") != std::string::npos);
	// Positionnel + --width -> doublon.
	REQUIRE(fail({"rt", "s.rt", "640", "480", "--width", "800"}).find("duplicate") != std::string::npos);
	// --out vide.
	REQUIRE(fail({"rt", "s.rt", "--out="}).find("bad --out") != std::string::npos);
}

TEST_CASE("options : tuile k/n", "[options]") {
	rt::app::Options opts;
	REQUIRE(ok({"rt", "s.rt", "--tile", "0/1"}, opts));
	REQUIRE(opts.tileIndex == 0);
	REQUIRE(opts.tileCount == 1);
	REQUIRE(ok({"rt", "s.rt", "--tile=3/4"}, opts));
	REQUIRE(opts.tileIndex == 3);
	REQUIRE(opts.tileCount == 4);
	// k doit etre dans 0..n-1, n dans 1..64, format strict k/n.
	REQUIRE(fail({"rt", "s.rt", "--tile", "4/4"}).find("bad --tile") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--tile", "0/0"}).find("bad --tile") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--tile", "0/99"}).find("bad --tile") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--tile", "abc"}).find("bad --tile") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--tile", "1"}).find("bad --tile") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--tile"}).find("missing value") != std::string::npos);
	REQUIRE(fail({"rt", "s.rt", "--tile", "0/2", "--tile", "1/2"}).find("duplicate") != std::string::npos);
}

TEST_CASE("options : separateur -- et quiet court", "[options]") {
	rt::app::Options opts;
	REQUIRE(ok({"rt", "-q", "s.rt"}, opts));
	REQUIRE(opts.quiet);
	// Apres `--`, `--help` est un nom de fichier, pas une option.
	REQUIRE(ok({"rt", "--", "--help"}, opts));
	REQUIRE(opts.hasScene);
	REQUIRE(opts.scenePath == "--help");
	REQUIRE_FALSE(opts.showHelp);
}
