// Capture d'ecran (T077, screenshot) — tests Catch2, sans ecran.
// DoD : touche produit un PNG valide ; chemin invalide -> erreur, pas de crash.

#include <catch2/catch_amalgamated.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "rt/io/Screenshot.hpp"
#include "rt/render/Framebuffer.hpp"

namespace {

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
	return magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G';
}

} // namespace

TEST_CASE("screenshot : nom horodate stable (T077)", "[screenshot]") {
	const std::string name = rt::io::screenshotName(0);
	REQUIRE(name.rfind("screenshot_", 0) == 0);
	REQUIRE(name.size() > 20U);
	REQUIRE(name.substr(name.size() - 4) == ".png");
}

TEST_CASE("screenshot : ecrit un PNG valide (T077)", "[screenshot]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(32, 24).isOk());
	fb.clear();
	fb.addSample(3, 4, rt::Vec3(0.9F, 0.1F, 0.2F));
	fb.present();
	const std::string dir = "/tmp/opencode";
	std::error_code code;
	std::filesystem::create_directories(dir, code);
	rt::Result<std::string> saved = rt::io::saveScreenshot(fb, dir);
	REQUIRE(saved.isOk());
	REQUIRE(std::filesystem::exists(saved.value()));
	REQUIRE(pngMagic(saved.value()));
	std::filesystem::remove(saved.value(), code);
}

TEST_CASE("screenshot : echec propre sans crash (T077)", "[screenshot]") {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(8, 8).isOk());
	fb.present();
	// Dossier inexistant -> IoError, pas de crash.
	rt::Result<std::string> bad = rt::io::saveScreenshot(fb, "/nonexistent_dir_xyz");
	REQUIRE(bad.isError());
	// Tampon vide -> InvalidArgument.
	rt::render::Framebuffer empty;
	REQUIRE(rt::io::saveScreenshot(empty, "/tmp/opencode").isError());
	// Dossier vide -> InvalidArgument.
	REQUIRE(rt::io::saveScreenshot(fb, "").isError());
}
