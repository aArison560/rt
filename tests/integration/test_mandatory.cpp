// Test "seul l'oeil deplace" (T081), Catch2.
// DoD : compare `scenes/fig_vi1.rt` et `scenes/fig_vi2.rt` ligne a ligne et
// echoue si autre chose que la directive `camera` differe ; les 2 images
// montrent la meme scene sous deux angles.

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

std::vector<std::string> readLines(const char* path) {
	std::ifstream in(path);
	REQUIRE(in.good());
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(in, line)) {
		lines.push_back(line);
	}
	return lines;
}

std::string trimmedLeft(const std::string& line) {
	const std::size_t first = line.find_first_not_of(" \t\r");
	if (first == std::string::npos) {
		return "";
	}
	return line.substr(first);
}

bool isCommentOrBlank(const std::string& line) {
	const std::string t = trimmedLeft(line);
	return t.empty() || t[0] == '#';
}

// Decoupe en lignes "camera" vs "reste" (hors commentaires/vides).
// La ligne `scene "nom" {` est normalisee (l'identite du fichier n'est pas
// du contenu de scene).
void splitCamera(const std::vector<std::string>& lines,
                 std::vector<std::string>& rest,
                 std::vector<std::string>& camera) {
	bool inCamera = false;
	int depth = 0;
	for (std::string line : lines) {
		if (isCommentOrBlank(line)) {
			continue;
		}
		std::string t = trimmedLeft(line);
		// Normalise le nom de scene : `scene "fig_vi1" {` == `scene "fig_vi2" {`.
		if (t.rfind("scene \"", 0) == 0) {
			t = "scene \"S\" {";
			line = t;
		}
		if (!inCamera && t == "camera {") {
			inCamera = true;
			depth = 1;
			camera.push_back(t);
			continue;
		}
		if (inCamera) {
			camera.push_back(t);
			for (char c : t) {
				if (c == '{') {
					++depth;
				}
				if (c == '}') {
					--depth;
				}
			}
			if (depth <= 0) {
				inCamera = false;
			}
			continue;
		}
		rest.push_back(t);
	}
	REQUIRE(!inCamera);
	REQUIRE(!camera.empty());
}

int countDiffering(const rt::render::Framebuffer& a,
                   const rt::render::Framebuffer& b) {
	REQUIRE(a.width() == b.width());
	REQUIRE(a.height() == b.height());
	const auto* pa = reinterpret_cast<const std::uint8_t*>(a.displayData());
	const auto* pb = reinterpret_cast<const std::uint8_t*>(b.displayData());
	int differing = 0;
	const std::size_t count = a.pixelCount();
	for (std::size_t p = 0; p < count; ++p) {
		bool diff = false;
		for (int c = 0; c < 4; ++c) {
			const int x = static_cast<int>(pa[p * 4U + static_cast<std::size_t>(c)]);
			const int y = static_cast<int>(pb[p * 4U + static_cast<std::size_t>(c)]);
			if (std::abs(x - y) > 2) {
				diff = true;
				break;
			}
		}
		if (diff) {
			++differing;
		}
	}
	return differing;
}

} // namespace

TEST_CASE("mandatory T081 : fig_vi2 ne differe de fig_vi1 que par camera",
          "[mandatory][t081]") {
	const std::vector<std::string> lines1 = readLines("scenes/fig_vi1.rt");
	const std::vector<std::string> lines2 = readLines("scenes/fig_vi2.rt");
	std::vector<std::string> rest1;
	std::vector<std::string> rest2;
	std::vector<std::string> cam1;
	std::vector<std::string> cam2;
	splitCamera(lines1, rest1, cam1);
	splitCamera(lines2, rest2, cam2);
	INFO("lignes hors camera : " << rest1.size() << " vs " << rest2.size());
	REQUIRE(rest1.size() == rest2.size());
	for (std::size_t i = 0; i < rest1.size(); ++i) {
		INFO("ligne " << i << " : [" << rest1[i] << "] vs [" << rest2[i] << "]");
		REQUIRE(rest1[i] == rest2[i]);
	}
	// Les blocs camera doivent differer (l'oeil est deplace).
	bool cameraDiffers = (cam1.size() != cam2.size());
	if (!cameraDiffers) {
		for (std::size_t i = 0; i < cam1.size(); ++i) {
			if (cam1[i] != cam2[i]) {
				cameraDiffers = true;
				break;
			}
		}
	}
	INFO("camera fig_vi1 :");
	for (const std::string& l : cam1) {
		INFO("  " << l);
	}
	INFO("camera fig_vi2 :");
	for (const std::string& l : cam2) {
		INFO("  " << l);
	}
	REQUIRE(cameraDiffers);
}

TEST_CASE("mandatory T081 : meme scene sous deux angles (parse + rendu)",
          "[mandatory][t081]") {
	rt::Result<rt::scene::Scene> r1 = rt::scene::parseFile("scenes/fig_vi1.rt");
	REQUIRE(r1.isOk());
	rt::Result<rt::scene::Scene> r2 = rt::scene::parseFile("scenes/fig_vi2.rt");
	REQUIRE(r2.isOk());
	const rt::scene::Scene& s1 = r1.value();
	const rt::scene::Scene& s2 = r2.value();
	// Meme contenu : autant de lumieres et d'objets.
	REQUIRE(s1.lights.size() == s2.lights.size());
	REQUIRE(s1.objects.size() == s2.objects.size());
	// Seule la camera change (position differente, meme cible).
	const bool samePos = (s1.camera.position.x == s2.camera.position.x &&
	                      s1.camera.position.y == s2.camera.position.y &&
	                      s1.camera.position.z == s2.camera.position.z);
	REQUIRE(!samePos);
	REQUIRE(s1.camera.target.x == s2.camera.target.x);
	REQUIRE(s1.camera.target.y == s2.camera.target.y);
	REQUIRE(s1.camera.target.z == s2.camera.target.z);
	// Les 2 images rendent et sont differentes (deux angles) mais pas
	// totalement etrangeres (meme fond / memes objets).
	rt::render::Framebuffer fb1;
	rt::render::Framebuffer fb2;
	const rt::render::RenderParams params{
	    .width = 80, .height = 60, .spp = 4, .maxDepth = 4, .seed = 0};
	REQUIRE(rt::render::render(s1, fb1, params).isOk());
	REQUIRE(rt::render::render(s2, fb2, params).isOk());
	const int differing = countDiffering(fb1, fb2);
	INFO("pixels differents entre les 2 angles : " << differing << "/"
	                                               << fb1.pixelCount());
	REQUIRE(differing > 100);
	REQUIRE(differing < static_cast<int>(fb1.pixelCount()));
}
