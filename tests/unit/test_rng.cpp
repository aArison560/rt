// Tests de `rt::Rng` / `rt::seedFor` (T016). Programme autonome (Catch2 en T017) :
// renvoie ≠ 0 si un contrôle échoue. Couvre le Prompt et le DoD T016 :
// même entrée → même suite, voisins indépendants, tuile pleine == 2 bandes
// (octet par octet), déterminisme sur 3 exécutions.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "rt/base/Rng.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
	if (!(cond)) {                                                                             \
	    ++g_failures;                                                                          \
	    std::cerr << "FAIL line " << __LINE__ << ": " #cond << '\n';                           \
	}                                                                                          \
    } while (false)

constexpr int kWidth = 32;
constexpr int kHeight = 16;
constexpr int kSpp = 4;
constexpr std::uint32_t kSceneSeed = 42U;

// Une « image » synthétique : pour chaque pixel (x, y) et chaque échantillon s,
// un `uint32_t` tiré du RNG seedé en coordonnées absolues. C'est le modèle exact
// du futur `renderRegion(x0, y0, w, h)` : seule la graine compte, pas l'ordre.
std::vector<std::uint32_t> renderFull() {
    const auto count = static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) *
                       static_cast<std::size_t>(kSpp);
    std::vector<std::uint32_t> pixels(count, 0U);
    for (int y = 0; y < kHeight; ++y) {
	for (int x = 0; x < kWidth; ++x) {
	    for (int s = 0; s < kSpp; ++s) {
		rt::Rng rng = rt::rngFor(x, y, s, kSceneSeed);
		const auto idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
		                  static_cast<std::size_t>(x)) *
		                     static_cast<std::size_t>(kSpp) +
		                 static_cast<std::size_t>(s);
		pixels[idx] = rng.nextUint32();
	    }
	}
    }
    return pixels;
}

// Même image calculée en 2 bandes horizontales [0, H/2) et [H/2, H),
// avec les coordonnées GLOBALES (le piège serait d'utiliser le y local).
std::vector<std::uint32_t> renderTwoBands() {
    const auto count = static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) *
                       static_cast<std::size_t>(kSpp);
    std::vector<std::uint32_t> pixels(count, 0U);
    const int split = kHeight / 2;
    for (int band = 0; band < 2; ++band) {
	const int y0 = (band == 0) ? 0 : split;
	const int y1 = (band == 0) ? split : kHeight;
	for (int y = y0; y < y1; ++y) { // y GLOBAL, jamais y - y0
	    for (int x = 0; x < kWidth; ++x) {
		for (int s = 0; s < kSpp; ++s) {
		    rt::Rng rng = rt::rngFor(x, y, s, kSceneSeed);
		    const auto idx =
		        (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) +
		         static_cast<std::size_t>(x)) *
		            static_cast<std::size_t>(kSpp) +
		        static_cast<std::size_t>(s);
		    pixels[idx] = rng.nextUint32();
		}
	    }
	}
    }
    return pixels;
}

} // namespace

int main() {
    using namespace rt;

    // --- Même entrée → même suite (8 tirages) -------------------------------------
    {
	Rng a = rngFor(7, 3, 2, kSceneSeed);
	Rng b(seedFor(7, 3, 2, kSceneSeed));
	for (int i = 0; i < 8; ++i) {
	    CHECK(a.nextUint32() == b.nextUint32());
	}
    }
    {
	// Deux RNG construits séparément sur la même graine restent synchrones,
	// y compris en flottants.
	Rng a = rngFor(0, 0, 0, kSceneSeed);
	Rng b = rngFor(0, 0, 0, kSceneSeed);
	for (int i = 0; i < 8; ++i) {
	    CHECK(a.nextFloat() == b.nextFloat());
	}
    }

    // --- Pixels voisins indépendants ----------------------------------------------
    {
	const std::uint64_t s00 = seedFor(0, 0, 0, kSceneSeed);
	CHECK(s00 != seedFor(1, 0, 0, kSceneSeed));
	CHECK(s00 != seedFor(0, 1, 0, kSceneSeed));
	CHECK(s00 != seedFor(0, 0, 1, kSceneSeed));
	CHECK(s00 != seedFor(0, 0, 0, 43U));
	CHECK(seedFor(-1, 0, 0, kSceneSeed) != seedFor(0, 0, 0, kSceneSeed));

	Rng a = rngFor(5, 5, 0, kSceneSeed);
	Rng b = rngFor(6, 5, 0, kSceneSeed);
	int diffs = 0;
	for (int i = 0; i < 8; ++i) {
	    if (a.nextUint32() != b.nextUint32()) {
		++diffs;
	    }
	}
	CHECK(diffs == 8); // suites entièrement distinctes, pas un simple décalage
    }

    // --- nextFloat : bornes [0, 1), jamais de NaN ---------------------------------
    {
	Rng rng = rngFor(11, 4, 1, kSceneSeed);
	for (int i = 0; i < 1024; ++i) {
	    const float f = rng.nextFloat();
	    CHECK(f >= 0.0F && f < 1.0F);
	    CHECK(!std::isnan(f));
	}
	// nextRange reste dans l'intervalle demandé.
	for (int i = 0; i < 256; ++i) {
	    const float f = rng.nextRange(-2.0F, 5.0F);
	    CHECK(f >= -2.0F && f < 5.0F);
	}
    }

    // --- discard : rejouer un échantillon donne la même suite ----------------------
    {
	Rng a = rngFor(2, 9, 0, kSceneSeed);
	Rng b = rngFor(2, 9, 0, kSceneSeed);
	a.discard(3);
	(void)b.nextUint32();
	(void)b.nextUint32();
	(void)b.nextUint32();
	CHECK(a.nextUint32() == b.nextUint32());
    }

    // --- DoD : tuile pleine == 2 bandes, octet par octet ----------------------------
    {
	const std::vector<std::uint32_t> full = renderFull();
	const std::vector<std::uint32_t> bands = renderTwoBands();
	CHECK(full.size() == bands.size());
	CHECK(!full.empty());
	for (std::size_t i = 0; i < full.size(); ++i) {
	    if (full[i] != bands[i]) {
		CHECK(full[i] == bands[i]);
		break; // un seul rapport, pas de spam
	    }
	}
	// Sensibilité du test : une graine LOCALE (y - y0) donnerait un résultat
	// différent — la couture serait détectée. Preuve : le pixel global
	// (0, H/2) ne partage pas sa graine avec le pixel local (0, 0).
	CHECK(seedFor(0, kHeight / 2, 0, kSceneSeed) != seedFor(0, 0, 0, kSceneSeed));
    }

    // --- DoD : déterminisme sur 3 exécutions ---------------------------------------
    {
	const std::vector<std::uint32_t> run1 = renderFull();
	const std::vector<std::uint32_t> run2 = renderFull();
	const std::vector<std::uint32_t> run3 = renderFull();
	CHECK(run1 == run2);
	CHECK(run2 == run3);
    }

    if (g_failures == 0) {
	std::cout << "test_rng: OK\n";
    }
    return g_failures == 0 ? 0 : 1;
}
