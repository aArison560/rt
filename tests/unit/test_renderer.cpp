// Tests de la boucle de rendu mono-thread (T032) + progressif (T036), Catch2.
// DoD T032 : `./rt scenes/default.rt 64 64 --out /tmp/x.png` -> 0 (verifie
// a la main + via `main`), determinisme (2 rendus identiques octet par
// octet), aucune dependance SDL (R6 : `grep -R SDL src/render` vide).
// DoD T036 : memes spp+seed -> memes pixels (hash), callback par batch,
// spp eleve = moins bruite (convergence du dithering vers le fond).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <vector>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

rt::scene::Scene loadDefault() {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(result.isOk());
	return std::move(result.value());
}

// T046 : scene vide (meme camera que `default.rt`, aucun objet) pour tester
// le fond seul. Avant T046 le renderer ignorait les objets donc `default.rt`
// suffisait ; depuis que les objets sont rendus, le fond pur exige 0 objet
// (`objects { }` vide reste valide, la passe T024 l'accepte).
rt::scene::Scene loadEmptyForBackground() {
	rt::Result<rt::scene::Scene> result = rt::scene::parseContent(
	    "scene { camera { position (0 2 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } lights { } objects { } }",
	    "empty.rt");
	REQUIRE(result.isOk());
	return std::move(result.value());
}

struct ProgressLog {
	std::vector<int> doneValues;
	int totalSeen = 0;
};

void recordProgress(int done, int total, void* user) noexcept {
	auto* log = static_cast<ProgressLog*>(user);
	if (log == nullptr) {
		return;
	}
	// Capacite reservee par le test (aucune allocation dans `render/`,
	// l'allocation eventuelle du `vector` vit dans la callback applicative).
	log->doneValues.push_back(done);
	log->totalSeen = total;
}

double meanAbsDev(const rt::render::Framebuffer& fb, const rt::Vec3& ref) {
	double sum = 0.0;
	const int w = fb.width();
	const int h = fb.height();
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			const int n = fb.samplesAt(x, y);
			const double ax = static_cast<double>(accum.x) / static_cast<double>(n);
			const double ay = static_cast<double>(accum.y) / static_cast<double>(n);
			const double az = static_cast<double>(accum.z) / static_cast<double>(n);
			sum += std::fabs(ax - static_cast<double>(ref.x));
			sum += std::fabs(ay - static_cast<double>(ref.y));
			sum += std::fabs(az - static_cast<double>(ref.z));
		}
	}
	return sum / static_cast<double>(w * h * 3);
}

} // namespace

TEST_CASE("renderer : rend le fond sur une petite image", "[renderer]") {
	// T046 : fond pur = scene sans objet (avec objets, le centre montre la
	// sphere, voir le test multi-objets ci-dessous).
	const rt::scene::Scene scene = loadEmptyForBackground();
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1, .maxDepth = 4, .seed = 0};
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	REQUIRE(fb.width() == 16);
	REQUIRE(fb.height() == 12);
	// Chaque pixel a recu exactement `spp` echantillons (miss -> fond).
	REQUIRE(fb.samplesAt(0, 0) == 1);
	REQUIRE(fb.samplesAt(15, 11) == 1);
	// T036 : l'accumulation vaut le fond ± dithering deterministe
	// (±0.03 par echantillon, kDitherAmp = 0.06) ; la moyenne converge
	// vers le fond quand spp grandit (test de convergence ci-dessous).
	REQUIRE(fb.accumAt(0, 0).x == Catch::Approx(scene.background.color.x).margin(0.031));
	REQUIRE(fb.accumAt(8, 6).y == Catch::Approx(scene.background.color.y).margin(0.031));
	REQUIRE(fb.accumAt(15, 11).z == Catch::Approx(scene.background.color.z).margin(0.031));
	// Couleurs bornees, jamais de NaN (le `present()` sature deja, T030).
	for (int y = 0; y < 12; ++y) {
		for (int x = 0; x < 16; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			REQUIRE(std::isfinite(accum.x));
			REQUIRE(std::isfinite(accum.y));
			REQUIRE(std::isfinite(accum.z));
		}
	}
}

TEST_CASE("renderer : determinisme (2 rendus identiques)", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	const rt::render::RenderParams params{.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 42};
	rt::render::Framebuffer first;
	rt::render::Framebuffer second;
	REQUIRE(rt::render::render(scene, first, params).isOk());
	REQUIRE(rt::render::render(scene, second, params).isOk());
	REQUIRE(first.width() == second.width());
	REQUIRE(first.height() == second.height());
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			const rt::Vec3 a = first.accumAt(x, y);
			const rt::Vec3 b = second.accumAt(x, y);
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
		}
	}
	const std::size_t count = first.pixelCount();
	for (std::size_t i = 0; i < count; ++i) {
		REQUIRE(first.displayData()[i].r == second.displayData()[i].r);
		REQUIRE(first.displayData()[i].g == second.displayData()[i].g);
		REQUIRE(first.displayData()[i].b == second.displayData()[i].b);
	}
}

TEST_CASE("renderer : parametres invalides -> erreur propre, sans crash", "[renderer]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 0, .height = 12, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 0, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 9000, .height = 12, .spp = 1}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 0}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 2048}).isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .maxDepth = 99})
	            .isError());
	REQUIRE(rt::render::render(scene, fb, {.width = 16, .height = 12, .spp = 1, .seed = -1})
	            .isError());
}

TEST_CASE("renderer : camera degeneree -> erreur propagee", "[renderer]") {
	rt::scene::Scene scene = loadDefault();
	scene.camera.position = scene.camera.target;
	rt::render::Framebuffer fb;
	const rt::render::RenderParams params{.width = 16, .height = 12, .spp = 1};
	REQUIRE(rt::render::render(scene, fb, params).isError());
}

TEST_CASE("renderer progressif : callback par batch (T036)", "[renderer][progress]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer fb;
	ProgressLog log;
	log.doneValues.reserve(8);
	rt::render::RenderParams params{.width = 16, .height = 12, .spp = 8, .maxDepth = 4, .seed = 7};
	params.onProgress = &recordProgress;
	params.progressUser = &log;
	REQUIRE(rt::render::render(scene, fb, params).isOk());
	// 1 appel par batch, done = 1..spp, total == spp.
	REQUIRE(log.doneValues.size() == 8U);
	REQUIRE(log.totalSeen == 8);
	for (std::size_t i = 0; i < log.doneValues.size(); ++i) {
		REQUIRE(log.doneValues[i] == static_cast<int>(i + 1U));
	}
	// Sans callback : meme image, sans appel.
	rt::render::Framebuffer silent;
	const rt::render::RenderParams quiet{.width = 16, .height = 12, .spp = 8, .maxDepth = 4, .seed = 7};
	REQUIRE(rt::render::render(scene, silent, quiet).isOk());
	for (int y = 0; y < 12; ++y) {
		for (int x = 0; x < 16; ++x) {
			REQUIRE(fb.accumAt(x, y).x == silent.accumAt(x, y).x);
			REQUIRE(fb.accumAt(x, y).y == silent.accumAt(x, y).y);
			REQUIRE(fb.accumAt(x, y).z == silent.accumAt(x, y).z);
		}
	}
}

TEST_CASE("renderer progressif : graine change l'image, meme graine la fige (T036)",
          "[renderer][progress]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer seedA;
	rt::render::Framebuffer seedB;
	rt::render::Framebuffer seedA2;
	REQUIRE(rt::render::render(scene, seedA, {.width = 24, .height = 16, .spp = 4, .maxDepth = 4, .seed = 1}).isOk());
	REQUIRE(rt::render::render(scene, seedB, {.width = 24, .height = 16, .spp = 4, .maxDepth = 4, .seed = 2}).isOk());
	REQUIRE(rt::render::render(scene, seedA2, {.width = 24, .height = 16, .spp = 4, .maxDepth = 4, .seed = 1}).isOk());
	// Meme graine -> octet par octet identique (reproductibilite DoD).
	bool identical = true;
	bool different = false;
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 24; ++x) {
			const rt::Vec3 a = seedA.accumAt(x, y);
			const rt::Vec3 a2 = seedA2.accumAt(x, y);
			const rt::Vec3 b = seedB.accumAt(x, y);
			if (a.x != a2.x || a.y != a2.y || a.z != a2.z) {
				identical = false;
			}
			if (a.x != b.x || a.y != b.y || a.z != b.z) {
				different = true;
			}
		}
	}
	REQUIRE(identical);
	// Graines differentes -> au moins un pixel differe (RNG branche).
	REQUIRE(different);
}

TEST_CASE("renderer progressif : spp eleve moins bruite (T036)", "[renderer][progress]") {
	const rt::scene::Scene scene = loadDefault();
	rt::render::Framebuffer low;
	rt::render::Framebuffer high;
	REQUIRE(rt::render::render(scene, low, {.width = 32, .height = 24, .spp = 1, .maxDepth = 4, .seed = 42}).isOk());
	REQUIRE(rt::render::render(scene, high, {.width = 32, .height = 24, .spp = 64, .maxDepth = 4, .seed = 42}).isOk());
	// Ecart moyen au fond : la moyenne de 64 echantillons est plus proche
	// du fond qu'un echantillon unique (convergence 1/sqrt(spp)).
	const double devLow = meanAbsDev(low, scene.background.color);
	const double devHigh = meanAbsDev(high, scene.background.color);
	INFO("devLow=" << devLow << " devHigh=" << devHigh);
	REQUIRE(devHigh < devLow);
}

// T046 : scene multi-objets — tri par `t`, doublons, coexistence.
// 6 objets dont 2 spheres sur le meme axe : la plus proche gagne.
// Camera (0,0,5) -> (0,0,0), centre de l'image vise (0,0,0.5..2).
// Proche rouge en (0,0,1) r=1, lointaine bleue en (0,0,-2) r=1.
// Lumiere frontale (0,3,5) : les normales face camera recoivent du diffus
// (avec un spot lateral, `NdotL` vaudrait 0 et le centre resterait au
// plancher ambiant ~0.005, indistinguable du dithering ±0.03).
TEST_CASE("renderer multi-objets : le plus proche gagne (6 objets, 2 spheres)", "[renderer][multiobject]") {
	const char* both =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (0 0 1) radius 1 material { albedo (0.9 0.1 0.1) } } "
	    "object { type sphere center (0 0 -2) radius 1 material { albedo (0.1 0.1 0.9) } } "
	    "object { type plane point (0 -1 0) normal (0 1 0) material { albedo (0.5 0.5 0.5) } } "
	    "object { type cylinder center (2 0 0) radius 0.4 material { albedo (0.2 0.8 0.2) } } "
	    "object { type cone center (-2 0 0) angle 20 material { albedo (0.8 0.8 0.2) } } "
	    "object { type sphere center (0 2 0) radius 0.3 material { albedo (0.1 0.9 0.9) } } "
	    "} }";
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseContent(both, "six.rt");
	REQUIRE(parsed.isOk());
	REQUIRE(parsed.value().totalObjectCount() == 6);
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(parsed.value(), fb, {.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	// Centre (16,12) : vise la proche rouge -> R dominant, pas bleu.
	const rt::Vec3 centre = fb.accumAt(16, 12);
	const int n = fb.samplesAt(16, 12);
	REQUIRE(n == 4);
	const float r = centre.x / static_cast<float>(n);
	const float b = centre.z / static_cast<float>(n);
	INFO("centre r=" << r << " b=" << b);
	REQUIRE(r > b);
	REQUIRE(r > 0.05F);
	REQUIRE(std::isfinite(r));
	REQUIRE(std::isfinite(b));
	// Meme scene sans la proche : le centre devient bleu (la lointaine gagne).
	const char* farOnly =
	    "scene { camera { position (0 0 5) target (0 0 0) fov 60 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 5) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (0 0 -2) radius 1 material { albedo (0.1 0.1 0.9) } } "
	    "} }";
	rt::Result<rt::scene::Scene> parsedFar = rt::scene::parseContent(farOnly, "far.rt");
	REQUIRE(parsedFar.isOk());
	rt::render::Framebuffer fbFar;
	REQUIRE(rt::render::render(parsedFar.value(), fbFar, {.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	const rt::Vec3 centreFar = fbFar.accumAt(16, 12);
	const int nFar = fbFar.samplesAt(16, 12);
	const float rFar = centreFar.x / static_cast<float>(nFar);
	const float bFar = centreFar.z / static_cast<float>(nFar);
	INFO("far r=" << rFar << " b=" << bFar);
	REQUIRE(bFar > rFar);
}

// T046 : coexistence des 4 types + doublons sans doublon visuel.
// `primitives.rt` (sphere+plan+cylindre+cone) differe du fond pur ;
// 2 spheres cote a cote (meme type, lumiere frontale) montrent rouge a
// gauche, bleu a droite — les deux instances rendent, pas une seule.
TEST_CASE("renderer multi-objets : coexistence des 4 types et doublons", "[renderer][multiobject]") {
	// 4 types visibles : au moins 5% des pixels s'ecartent du fond (>0.05).
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("tests/cases/valid/primitives.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Scene& scene = parsed.value();
	rt::render::Framebuffer fb;
	REQUIRE(rt::render::render(scene, fb, {.width = 48, .height = 36, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	int different = 0;
	int total = 48 * 36;
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 accum = fb.accumAt(x, y);
			const int n = fb.samplesAt(x, y);
			const float ax = accum.x / static_cast<float>(n);
			const float ay = accum.y / static_cast<float>(n);
			const float az = accum.z / static_cast<float>(n);
			REQUIRE(std::isfinite(ax));
			REQUIRE(std::isfinite(ay));
			REQUIRE(std::isfinite(az));
			const float dr = ax - scene.background.color.x;
			const float dg = ay - scene.background.color.y;
			const float db = az - scene.background.color.z;
			if ((dr * dr + dg * dg + db * db) > 0.05F * 0.05F) {
				++different;
			}
		}
	}
	INFO("pixels non-fond : " << different << "/" << total);
	REQUIRE(different > total / 20);
	// Doublons : 2 spheres du meme type sous lumiere frontale (0,3,6) — les
	// deux recoivent du diffus (`NdotL` ~0.7), les couleurs restent
	// distinguables malgre le dithering ±0.03. `group.rt` n'est pas utilise
	// ici car sa 1ere lumiere laterale (3,5,2) laisse la sphere gauche au
	// plancher ambiant (~0.005, noye dans le dithering).
	const char* twoSpheres =
	    "scene { camera { position (0 1 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 6) color (1 1 1) intensity 1.0 } } "
	    "objects { "
	    "object { type sphere center (-1.2 0 0) radius 1 material { albedo (0.9 0.1 0.1) } } "
	    "object { type sphere center (1.2 0 0) radius 1 material { albedo (0.1 0.1 0.9) } } "
	    "} }";
	rt::Result<rt::scene::Scene> grouped = rt::scene::parseContent(twoSpheres, "two.rt");
	REQUIRE(grouped.isOk());
	rt::render::Framebuffer fbGroup;
	REQUIRE(rt::render::render(grouped.value(), fbGroup, {.width = 64, .height = 48, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	const rt::Vec3 left = fbGroup.accumAt(16, 24);
	const rt::Vec3 right = fbGroup.accumAt(48, 24);
	const float leftR = left.x / static_cast<float>(fbGroup.samplesAt(16, 24));
	const float leftB = left.z / static_cast<float>(fbGroup.samplesAt(16, 24));
	const float rightR = right.x / static_cast<float>(fbGroup.samplesAt(48, 24));
	const float rightB = right.z / static_cast<float>(fbGroup.samplesAt(48, 24));
	INFO("gauche r=" << leftR << " b=" << leftB << " droite r=" << rightR << " b=" << rightB);
	// Gauche = sphere rouge (-1.2) : R > B ; droite = bleue (+1.2) : B > R.
	// Pas de doublon visuel (les deux instances rendent, pas une seule).
	REQUIRE(leftR > leftB);
	REQUIRE(rightB > rightR);
	// Groupes aplatis : `group.rt` (2 groupes, 3 objets) rend non-fond.
	rt::Result<rt::scene::Scene> groupFile = rt::scene::parseFile("tests/cases/valid/group.rt");
	REQUIRE(groupFile.isOk());
	REQUIRE(groupFile.value().totalObjectCount() == 3);
	rt::render::Framebuffer fbGrp;
	REQUIRE(rt::render::render(groupFile.value(), fbGrp, {.width = 48, .height = 36, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	int groupDiff = 0;
	for (int y = 0; y < 36; ++y) {
		for (int x = 0; x < 48; ++x) {
			const rt::Vec3 accum = fbGrp.accumAt(x, y);
			const int n = fbGrp.samplesAt(x, y);
			const float ax = accum.x / static_cast<float>(n);
			const float ay = accum.y / static_cast<float>(n);
			const float az = accum.z / static_cast<float>(n);
			const float dr = ax - groupFile.value().background.color.x;
			const float dg = ay - groupFile.value().background.color.y;
			const float db = az - groupFile.value().background.color.z;
			if ((dr * dr + dg * dg + db * db) > 0.05F * 0.05F) {
				++groupDiff;
			}
		}
	}
	INFO("group.rt non-fond : " << groupDiff << "/" << 48 * 36);
	REQUIRE(groupDiff > (48 * 36) / 20);
	// Transform de scene : sphere a l'origine + `translate (2 0 0)` ≡ sphere
	// placee en (2 0 0) (coherence des images, M4 au niveau pixels).
	const char* placedText =
	    "scene { camera { position (0 1 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 6) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (2 0 0) radius 0.8 } } }";
	const char* movedText =
	    "scene { camera { position (0 1 6) target (0 0 0) fov 50 } "
	    "background { color (0.02 0.02 0.05) } "
	    "lights { light { position (0 3 6) color (1 1 1) intensity 1.0 } } "
	    "objects { object { type sphere center (0 0 0) radius 0.8 transform { translate (2 0 0) } } } }";
	rt::Result<rt::scene::Scene> placedRes = rt::scene::parseContent(placedText, "placed.rt");
	rt::Result<rt::scene::Scene> movedRes = rt::scene::parseContent(movedText, "moved.rt");
	REQUIRE(placedRes.isOk());
	REQUIRE(movedRes.isOk());
	rt::render::Framebuffer fbPlaced;
	rt::render::Framebuffer fbMoved;
	REQUIRE(rt::render::render(placedRes.value(), fbPlaced, {.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	REQUIRE(rt::render::render(movedRes.value(), fbMoved, {.width = 32, .height = 24, .spp = 4, .maxDepth = 4, .seed = 0}).isOk());
	for (int y = 0; y < 24; ++y) {
		for (int x = 0; x < 32; ++x) {
			const rt::Vec3 a = fbPlaced.accumAt(x, y);
			const rt::Vec3 b = fbMoved.accumAt(x, y);
			REQUIRE(a.x == b.x);
			REQUIRE(a.y == b.y);
			REQUIRE(a.z == b.z);
		}
	}
}
