#pragma once

// RNG déterministe par pixel global (T016) — header-only, noexcept, sans allocation.
// Choix : PCG32 (XSH-RR, O'Neill) + mélange SplitMix64 pour `seedFor`.
// Règles R2/R3 : aucune exception, aucune allocation dynamique dans le générateur ;
// l'état tient dans 16 octets (trivialement copiable, `static_assert` ci-dessous).
//
// Contrat cluster (docs/DISTRIBUTED_RENDERING.md §2.2) : la graine ne dépend QUE
// de coordonnées absolues — `seedFor(globalX, globalY, sample, sceneSeed)`.
// Deux tuiles/bandes qui couvrent des pixels disjoints produisent donc les mêmes
// suites que le rendu plein, sans couture. Ne JAMAIS nourrir ce générateur avec
// des coordonnées locales de tuile.

#include <cstdint>
#include <type_traits>

namespace rt {

inline constexpr std::uint64_t kRngDefaultStream = 0x853c49e6748fea9bULL; // impaire (PCG)
inline constexpr std::uint64_t kRngMultiplier = 6364136223846793005ULL;   // PCG
inline constexpr std::uint64_t kRngGolden = 0x9e3779b97f4a7c15ULL;        // golden ratio 64 bits

// Finaliseur SplitMix64 (Steele et al.) : avalanche complète, constexpr.
[[nodiscard]] constexpr std::uint64_t splitMix64(std::uint64_t z) noexcept {
    z += kRngGolden;
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
}

// Graine d'un échantillon : uniquement (x absolu, y absolu, index d'échantillon,
// graine de scène). Les `int` sont repliés en `uint32_t` (bits conservés) pour
// un comportement défini même si un appelant passe une valeur négative.
[[nodiscard]] constexpr std::uint64_t seedFor(int globalX, int globalY, int sample,
                                              std::uint32_t sceneSeed) noexcept {
    std::uint64_t h = splitMix64(static_cast<std::uint64_t>(sceneSeed) + kRngGolden);
    h = splitMix64(h + static_cast<std::uint64_t>(static_cast<std::uint32_t>(globalX)));
    h = splitMix64(h + static_cast<std::uint64_t>(static_cast<std::uint32_t>(globalY)) *
                           0xbf58476d1ce4e5b9ULL);
    h = splitMix64(h + static_cast<std::uint64_t>(static_cast<std::uint32_t>(sample)) *
                           0x94d049bb133111ebULL);
    // Évite l'état nul (première sortie PCG nulle) sans perdre le déterminisme.
    return h == 0U ? kRngGolden : h;
}

// Générateur PCG32 XSH-RR : 64 bits d'état, 32 bits de sortie, période 2^64.
// Initialisation standard (état nul + 2 transitions) pour diffuser les graines
// proches (pixels voisins) dès la première sortie.
class Rng {
  public:
    constexpr explicit Rng(std::uint64_t seed, std::uint64_t seq = 1U) noexcept
        : inc_((seq << 1U) | 1U) {
	(void)nextUint32();
	state_ += seed;
	(void)nextUint32();
    }

    constexpr std::uint32_t nextUint32() noexcept {
	const std::uint64_t old = state_;
	state_ = old * kRngMultiplier + inc_;
	const auto xorshifted = static_cast<std::uint32_t>(((old >> 18U) ^ old) >> 27U);
	const auto rot = static_cast<std::uint32_t>(old >> 59U);
	return (xorshifted >> rot) | (xorshifted << ((~rot + 1U) & 31U));
    }

    // Flottant dans [0, 1) sur 24 bits (mantisse exacte d'un float).
    constexpr float nextFloat() noexcept {
	return static_cast<float>(nextUint32() >> 8U) * (1.0F / 16777216.0F);
    }

    // Flottant dans [lo, hi).
    constexpr float nextRange(float lo, float hi) noexcept { return lo + (hi - lo) * nextFloat(); }

    // Avance le flux de `n` tirages (rejeu déterministe d'un échantillon).
    constexpr void discard(std::uint64_t n) noexcept {
	for (std::uint64_t i = 0; i < n; ++i) {
	    (void)nextUint32();
	}
    }

    [[nodiscard]] constexpr std::uint64_t state() const noexcept { return state_; }
    [[nodiscard]] constexpr std::uint64_t stream() const noexcept { return inc_; }

  private:
    std::uint64_t state_ = 0U;
    std::uint64_t inc_ = kRngDefaultStream;
};

// Raccourci : RNG prêt pour le pixel absolu (x, y) et l'échantillon `sample`.
[[nodiscard]] constexpr Rng rngFor(int globalX, int globalY, int sample,
                                   std::uint32_t sceneSeed) noexcept {
    return Rng(seedFor(globalX, globalY, sample, sceneSeed));
}

// Vérifications de contrat à la compilation : déterminisme + sensibilité.
namespace detail {
inline constexpr std::uint64_t kRngCheckA = seedFor(1, 2, 3, 4U);
inline constexpr std::uint64_t kRngCheckB = seedFor(1, 2, 3, 4U);
inline constexpr std::uint64_t kRngCheckNx = seedFor(2, 2, 3, 4U);
inline constexpr std::uint64_t kRngCheckNy = seedFor(1, 3, 3, 4U);
inline constexpr std::uint64_t kRngCheckNs = seedFor(1, 2, 4, 4U);
inline constexpr std::uint64_t kRngCheckSeed = seedFor(1, 2, 3, 5U);
} // namespace detail
static_assert(detail::kRngCheckA == detail::kRngCheckB);
static_assert(detail::kRngCheckA != detail::kRngCheckNx);
static_assert(detail::kRngCheckA != detail::kRngCheckNy);
static_assert(detail::kRngCheckA != detail::kRngCheckNs);
static_assert(detail::kRngCheckA != detail::kRngCheckSeed);
static_assert(std::is_trivially_copyable_v<Rng>);
static_assert(sizeof(Rng) == 16);

} // namespace rt
