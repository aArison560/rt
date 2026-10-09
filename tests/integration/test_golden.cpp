// Tests de non-regression visuelle (T059), Catch2.
// DoD : les 3 scenes (`fig_vi1_base.rt`, `fig_vi1.rt`, `fig_vi3.rt`) rendent ;
// le hash des images est enregistre dans `tests/golden/` (`.rgba` + `hashes.txt`)
// et compare a chaque `make test` avec tolerance sur quelques pixels.
// Generation : `sh scripts/gen_golden.sh` (ou `UPDATE_GOLDEN=1 ./rt_test "[golden]"`).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

// Resolution golden (T059) : 80x60 spp 4, seed 0 — assez petite pour que
// `make test` reste rapide (< 0.5 s pour les 3 scenes), assez grande pour
// capturer ombres + brillance. `maxDepth` vient de la scene (4).
constexpr int kGoldenWidth = 80;
constexpr int kGoldenHeight = 60;
constexpr int kGoldenSpp = 4;
constexpr long long kGoldenSeed = 0;
// Tolerance DoD ("quelques pixels") : ecart par canal <= 2 (arrondi gamma)
// ignore ; au-dela, au plus 5 pixels (0.1 % de 80x60) peuvent differer.
// Une vraie regression (couleur, lumiere, geometrie) touche des centaines
// de pixels et fait echouer.
constexpr int kChannelTolerance = 2;
constexpr int kMaxDifferingPixels = 5;

struct GoldenEntry {
	const char* scenePath = nullptr;
	const char* goldenPath = nullptr;
};

const GoldenEntry kEntries[] = {
    {"scenes/fig_vi1_base.rt", "tests/golden/fig_vi1_base.rgba"},
    {"scenes/fig_vi1.rt", "tests/golden/fig_vi1.rgba"},
    {"scenes/fig_vi3.rt", "tests/golden/fig_vi3.rgba"},
};

rt::scene::Scene parseFileOrDie(const char* path) {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile(path);
	INFO("parse " << path << (result.isOk() ? " ok" : result.status().message.c_str()));
	REQUIRE(result.isOk());
	return std::move(result.value());
}

void renderGolden(const rt::scene::Scene& scene, rt::render::Framebuffer& fb) {
	const rt::render::RenderParams params{.width = kGoldenWidth,
	                                       .height = kGoldenHeight,
	                                       .spp = kGoldenSpp,
	                                       .maxDepth = scene.limits.maxDepth,
	                                       .seed = kGoldenSeed};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	REQUIRE(fb.width() == kGoldenWidth);
	REQUIRE(fb.height() == kGoldenHeight);
}

// FNV-1a 64 bits sur le tampon d'affichage (log de debug, `hashes.txt`).
std::uint64_t fnv1a(const rt::render::Rgba8* data, std::size_t count) {
	std::uint64_t hash = 14695981039346656037ULL;
	const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
	for (std::size_t i = 0; i < count * 4U; ++i) {
		hash ^= static_cast<std::uint64_t>(bytes[i]);
		hash *= 1099511628211ULL;
	}
	return hash;
}

bool updateMode() {
	return std::getenv("UPDATE_GOLDEN") != nullptr;
}

void writeGolden(const char* goldenPath, const rt::render::Framebuffer& fb) {
	std::error_code ec;
	std::filesystem::create_directories(
	    std::filesystem::path(goldenPath).parent_path(), ec);
	std::ofstream out(goldenPath, std::ios::binary | std::ios::trunc);
	REQUIRE(out.good());
	const std::size_t count = fb.pixelCount();
	out.write(reinterpret_cast<const char*>(fb.displayData()),
	          static_cast<std::streamsize>(count * 4U));
	REQUIRE(out.good());
}

std::vector<std::uint8_t> readGolden(const char* goldenPath, std::size_t expectedBytes) {
	std::ifstream in(goldenPath, std::ios::binary);
	if (!in.good()) {
		FAIL("golden manquant : " << goldenPath
		                          << " (regenere via sh scripts/gen_golden.sh)");
	}
	std::vector<std::uint8_t> data(expectedBytes, 0);
	in.read(reinterpret_cast<char*>(data.data()),
	        static_cast<std::streamsize>(expectedBytes));
	if (static_cast<std::size_t>(in.gcount()) != expectedBytes) {
		FAIL("golden tronque : " << goldenPath << " (" << in.gcount() << " o, attendu "
		                         << expectedBytes << ")");
	}
	return data;
}

int countDiffering(const rt::render::Framebuffer& fb,
                   const std::vector<std::uint8_t>& expected) {
	const auto* actual = reinterpret_cast<const std::uint8_t*>(fb.displayData());
	int differing = 0;
	const std::size_t count = fb.pixelCount();
	for (std::size_t p = 0; p < count; ++p) {
		bool pixelDiffers = false;
		for (int c = 0; c < 4; ++c) {
			const int a = static_cast<int>(actual[p * 4U + static_cast<std::size_t>(c)]);
			const int e = static_cast<int>(expected[p * 4U + static_cast<std::size_t>(c)]);
			if (std::abs(a - e) > kChannelTolerance) {
				pixelDiffers = true;
				break;
			}
		}
		if (pixelDiffers) {
			++differing;
		}
	}
	return differing;
}

float luminanceAt(const rt::render::Framebuffer& fb, int x, int y) {
	const rt::Vec3 accum = fb.accumAt(x, y);
	const int n = fb.samplesAt(x, y);
	REQUIRE(n > 0);
	const float r = accum.x / static_cast<float>(n);
	const float g = accum.y / static_cast<float>(n);
	const float b = accum.z / static_cast<float>(n);
	REQUIRE(std::isfinite(r));
	REQUIRE(std::isfinite(g));
	REQUIRE(std::isfinite(b));
	return (r + g + b) / 3.0F;
}

} // namespace

TEST_CASE("golden T059 : les 3 scenes rendent sans NaN", "[golden][t059]") {
	for (const GoldenEntry& entry : kEntries) {
		DYNAMIC_SECTION(entry.scenePath) {
			rt::scene::Scene scene = parseFileOrDie(entry.scenePath);
			rt::render::Framebuffer fb;
			renderGolden(scene, fb);
			bool hasBright = false;
			bool hasDark = false;
			for (int y = 0; y < kGoldenHeight; ++y) {
				for (int x = 0; x < kGoldenWidth; ++x) {
					const float lum = luminanceAt(fb, x, y);
					REQUIRE_FALSE(std::isnan(lum));
					if (lum > 0.3F) {
						hasBright = true;
					}
					if (lum < 0.08F) {
						hasDark = true;
					}
				}
			}
			INFO("hash fnv1a=" << std::hex << fnv1a(fb.displayData(), fb.pixelCount()));
			REQUIRE(hasBright);
			REQUIRE(hasDark);
		}
	}
}

TEST_CASE("golden T059 : fig_vi1 = 4 types, 2 spots, brillance", "[golden][t059]") {
	rt::scene::Scene scene = parseFileOrDie("scenes/fig_vi1.rt");
	REQUIRE(scene.lights.size() == 2);
	for (const rt::scene::Light& light : scene.lights) {
		INFO("light " << light.name);
		REQUIRE(light.type == rt::scene::LightType::Spot);
		REQUIRE(light.hasTarget);
	}
	bool hasPlane = false;
	bool hasSphere = false;
	bool hasCylinder = false;
	bool hasCone = false;
	bool hasSpecular = false;
	for (const rt::scene::Object& object : scene.objects) {
		if (object.type == rt::scene::ObjectType::Plane) {
			hasPlane = true;
		}
		if (object.type == rt::scene::ObjectType::Sphere) {
			hasSphere = true;
		}
		if (object.type == rt::scene::ObjectType::Cylinder) {
			hasCylinder = true;
		}
		if (object.type == rt::scene::ObjectType::Cone) {
			hasCone = true;
		}
		if (object.material.specular > 0.0F) {
			hasSpecular = true;
		}
	}
	REQUIRE(hasPlane);
	REQUIRE(hasSphere);
	REQUIRE(hasCylinder);
	REQUIRE(hasCone);
	REQUIRE(hasSpecular);
	rt::render::Framebuffer fb;
	renderGolden(scene, fb);
	int whites = 0;
	for (int y = 0; y < kGoldenHeight; ++y) {
		for (int x = 0; x < kGoldenWidth; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			const int n = fb.samplesAt(x, y);
			const float r = accum.x / static_cast<float>(n);
			const float g = accum.y / static_cast<float>(n);
			const float b = accum.z / static_cast<float>(n);
			if (r > 0.9F && g > 0.9F && b > 0.9F) {
				++whites;
			}
		}
	}
	INFO("pixels blancs (speculaire) : " << whites);
	REQUIRE(whites > 0);
}

TEST_CASE("golden T059 : fig_vi3 = melange d'ombres (2 zones distinctes)",
          "[golden][t059]") {
	rt::scene::Scene both = parseFileOrDie("scenes/fig_vi3.rt");
	REQUIRE(both.lights.size() == 2);
	REQUIRE(both.lights[0].type == rt::scene::LightType::Spot);
	REQUIRE(both.lights[1].type == rt::scene::LightType::Spot);
	rt::scene::Scene left = both;
	rt::scene::Scene right = both;
	left.lights.erase(left.lights.begin() + 1);
	right.lights.erase(right.lights.begin());
	rt::render::Framebuffer fbBoth;
	rt::render::Framebuffer fbLeft;
	rt::render::Framebuffer fbRight;
	renderGolden(both, fbBoth);
	renderGolden(left, fbLeft);
	renderGolden(right, fbRight);
	int leftLitRightShadowed = 0;
	int rightLitLeftShadowed = 0;
	int bothLit = 0;
	for (int y = 0; y < kGoldenHeight; ++y) {
		for (int x = 0; x < kGoldenWidth; ++x) {
			const float lBoth = luminanceAt(fbBoth, x, y);
			const float lLeft = luminanceAt(fbLeft, x, y);
			const float lRight = luminanceAt(fbRight, x, y);
			if (lLeft > 0.12F && lRight > 0.12F) {
				++bothLit;
				REQUIRE(lBoth + 0.031F >= lLeft);
				REQUIRE(lBoth + 0.031F >= lRight);
			}
			if (lLeft < 0.05F && lRight > 0.15F) {
				++leftLitRightShadowed;
			}
			if (lRight < 0.05F && lLeft > 0.15F) {
				++rightLitLeftShadowed;
			}
		}
	}
	INFO("plein=" << bothLit << " ombreG=" << leftLitRightShadowed
	              << " ombreD=" << rightLitLeftShadowed);
	REQUIRE(bothLit > 0);
	REQUIRE(leftLitRightShadowed > 0);
	REQUIRE(rightLitLeftShadowed > 0);
}

TEST_CASE("golden T059 : non-regression octet-stable (tolerance quelques pixels)",
          "[golden][t059]") {
	for (const GoldenEntry& entry : kEntries) {
		DYNAMIC_SECTION(entry.scenePath) {
			rt::scene::Scene scene = parseFileOrDie(entry.scenePath);
			rt::render::Framebuffer fb;
			renderGolden(scene, fb);
			const std::uint64_t hash = fnv1a(fb.displayData(), fb.pixelCount());
			INFO("fnv1a=" << std::hex << hash << " " << entry.scenePath);
			if (updateMode()) {
				writeGolden(entry.goldenPath, fb);
				SUCCEED("golden regenere : " << entry.goldenPath);
				continue;
			}
			const std::size_t expectedBytes = fb.pixelCount() * 4U;
			const std::vector<std::uint8_t> expected =
			    readGolden(entry.goldenPath, expectedBytes);
			const int differing = countDiffering(fb, expected);
			INFO("pixels differents (>2/canal) : " << differing << "/"
			                                       << fb.pixelCount());
			REQUIRE(differing <= kMaxDifferingPixels);
		}
	}
}
