// Tests de l'ecriture d'image PNG/PPM (T034), Catch2.
// DoD : le PNG produit est valide (`file`/`identify` a la main + signature
// PNG verifiee ici), echec d'ecriture (repertoire inexistant) -> erreur
// propre et code != 0 dans `main`, jamais de crash.

#include <catch2/catch_amalgamated.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "rt/io/ImageWriter.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"

namespace {

const char* kPngTmp = "/tmp/opencode/rt_test_image.png";
const char* kPpmTmp = "/tmp/opencode/rt_test_image.ppm";

void ensureTmpDir() {
	std::error_code ec;
	std::filesystem::create_directories(
	    std::filesystem::path(kPngTmp).parent_path(), ec);
}

rt::render::Framebuffer makeSmall() {
	rt::render::Framebuffer fb;
	REQUIRE(fb.init(8, 6).isOk());
	fb.addSample(0, 0, rt::Vec3(1.0F, 0.0F, 0.0F));
	fb.addSample(7, 5, rt::Vec3(0.0F, 0.0F, 1.0F));
	fb.present();
	return fb;
}

std::string readPrefix(const char* path, std::size_t count) {
	std::ifstream file(path, std::ios::binary);
	REQUIRE(file.good());
	std::string prefix(count, '\0');
	file.read(prefix.data(), static_cast<std::streamsize>(count));
	REQUIRE(static_cast<std::size_t>(file.gcount()) == count);
	return prefix;
}

} // namespace

TEST_CASE("imagewriter : ecrit un PNG valide (signature PNG)", "[imagewriter]") {
	const rt::render::Framebuffer fb = makeSmall();
	ensureTmpDir();
	std::remove(kPngTmp);
	REQUIRE(rt::io::writeImage(fb, kPngTmp).isOk());
	// Signature PNG : 89 50 4E 47 0D 0A 1A 0A.
	const std::string prefix = readPrefix(kPngTmp, 8);
	REQUIRE(static_cast<unsigned char>(prefix[0]) == 0x89);
	REQUIRE(prefix[1] == 'P');
	REQUIRE(prefix[2] == 'N');
	REQUIRE(prefix[3] == 'G');
	REQUIRE(static_cast<unsigned char>(prefix[4]) == 0x0D);
	REQUIRE(static_cast<unsigned char>(prefix[5]) == 0x0A);
	REQUIRE(static_cast<unsigned char>(prefix[6]) == 0x1A);
	REQUIRE(static_cast<unsigned char>(prefix[7]) == 0x0A);
	std::ifstream file(kPngTmp, std::ios::binary | std::ios::ate);
	REQUIRE(file.good());
	REQUIRE(file.tellg() > 50);
	std::remove(kPngTmp);
}

TEST_CASE("imagewriter : ecrit un PPM P6 pour .ppm", "[imagewriter]") {
	const rt::render::Framebuffer fb = makeSmall();
	ensureTmpDir();
	std::remove(kPpmTmp);
	REQUIRE(rt::io::writeImage(fb, kPpmTmp).isOk());
	const std::string prefix = readPrefix(kPpmTmp, 2);
	REQUIRE(prefix[0] == 'P');
	REQUIRE(prefix[1] == '6');
	std::remove(kPpmTmp);
}

TEST_CASE("imagewriter : echecs propres sans crash", "[imagewriter]") {
	const rt::render::Framebuffer fb = makeSmall();
	REQUIRE(rt::io::writeImage(fb, "").isError());
	REQUIRE(rt::io::writeImage(fb, std::string(2000, 'a')).isError());
	// Repertoire inexistant -> IoError, jamais de segfault (DoD T034).
	const rt::Status missing =
	    rt::io::writeImage(fb, "/nonexistent_dir_xyz_12345/out.png");
	REQUIRE(missing.isError());
	REQUIRE(missing.code == rt::StatusCode::IoError);
	REQUIRE_FALSE(missing.message.empty());
	// Framebuffer vide -> InvalidArgument.
	rt::render::Framebuffer empty;
	REQUIRE(rt::io::writeImage(empty, kPngTmp).isError());
}

TEST_CASE("imagewriter : rendu reel de default.rt en PNG", "[imagewriter]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 1};
	REQUIRE(rt::render::render(parsed.value(), fb, params).isOk());
	ensureTmpDir();
	std::remove(kPngTmp);
	REQUIRE(rt::io::writeImage(fb, kPngTmp).isOk());
	const std::string prefix = readPrefix(kPngTmp, 8);
	REQUIRE(static_cast<unsigned char>(prefix[0]) == 0x89);
	REQUIRE(prefix[1] == 'P');
	std::remove(kPngTmp);
}
