// Mode headless obligatoire (T035), Catch2, integration.
// DoD : `env -u DISPLAY ./rt scenes/default.rt 64 64 --out /tmp/a.png`
// -> code 0, teste automatiquement via `make test` (ce fichier + la porte
// headless de `scripts/run_cases.sh`). Garantie : aucune initialisation SDL
// dans ce chemin (R6) — `grep -R "SDL_Init|SDL_Create|#include.*SDL"`
// `src/ include/` est vide, et le rendu reussit sans `DISPLAY`.

#include <catch2/catch_amalgamated.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "rt/app/Options.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"

namespace {

rt::Result<rt::app::Options> parse(std::initializer_list<const char*> items) {
	std::vector<std::string> args;
	args.reserve(items.size());
	for (const char* item : items) {
		args.emplace_back(item);
	}
	return rt::app::parseOptionsVec(args);
}

bool pngMagic(const std::string& path) {
	std::ifstream in(path, std::ios::binary);
	if (!in.good()) {
		return false;
	}
	unsigned char magic[8] = {};
	in.read(reinterpret_cast<char*>(magic), 8);
	if (in.gcount() < 8) {
		return false;
	}
	// PNG : 89 50 4E 47 0D 0A 1A 0A, ou PPM P6 (fallback sans libpng, T034).
	if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
		return true;
	}
	return magic[0] == 'P' && magic[1] == '6';
}

} // namespace

TEST_CASE("headless : --headless accepte et reste sans fenetre", "[headless]") {
	{
		rt::Result<rt::app::Options> result = parse({"rt", "s.rt", "--headless"});
		REQUIRE(result.isOk());
		REQUIRE(result.value().headless);
	}
	{
		rt::Result<rt::app::Options> result = parse({"rt", "s.rt"});
		REQUIRE(result.isOk());
		REQUIRE_FALSE(result.value().headless);
	}
}

TEST_CASE("headless : rendu lib sans DISPLAY", "[headless]") {
	// Sauve/restaure DISPLAY : le rendu ne doit jamais en dependre (R6).
	const char* saved = std::getenv("DISPLAY");
	std::string savedValue;
	const bool hadDisplay = (saved != nullptr);
	if (hadDisplay) {
		savedValue = saved;
	}
	unsetenv("DISPLAY");
	REQUIRE(std::getenv("DISPLAY") == nullptr);

	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1, .maxDepth = 4, .seed = 0};
	REQUIRE(rt::render::render(parsed.value(), fb, params).isOk());
	REQUIRE(fb.width() == 16);
	REQUIRE(fb.height() == 12);

	if (hadDisplay) {
		setenv("DISPLAY", savedValue.c_str(), 1);
	}
}

TEST_CASE("headless : binaire sans DISPLAY (DoD T035)", "[headless]") {
	const std::string out = "/tmp/rt_headless_t035.png";
	std::error_code code;
	std::filesystem::remove(out, code);
	// Commande exacte du DoD (dimensions 64x64, sortie PNG).
	const int rc = std::system("env -u DISPLAY ./rt scenes/default.rt 64 64 --out /tmp/rt_headless_t035.png >/dev/null 2>&1");
	REQUIRE(rc == 0);
	REQUIRE(std::filesystem::exists(out));
	REQUIRE(std::filesystem::file_size(out) > 0U);
	REQUIRE(pngMagic(out));
	std::filesystem::remove(out, code);
	// Variante explicite `--headless` : meme garantie, meme code 0.
	const std::string out2 = "/tmp/rt_headless_t035_flag.png";
	std::filesystem::remove(out2, code);
	const int rc2 = std::system("env -u DISPLAY ./rt scenes/default.rt 16 12 --out /tmp/rt_headless_t035_flag.png --headless >/dev/null 2>&1");
	REQUIRE(rc2 == 0);
	REQUIRE(std::filesystem::exists(out2));
	REQUIRE(pngMagic(out2));
	std::filesystem::remove(out2, code);
}
