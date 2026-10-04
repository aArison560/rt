#pragma once

// Scalaires du noyau `base` (T010) — header-only, tout constexpr/noexcept.
// Décision de l'ADR-001 (§1, point 7) : `Real = float` pour le hot path,
// tolérances de test calibrées en conséquence.

#include <limits>
#include <numbers>

namespace rt {

using Real = float;

inline constexpr Real kEpsilon = 1e-6f;
inline constexpr Real kPi = std::numbers::pi_v<Real>;
inline constexpr Real kInfinity = std::numeric_limits<Real>::infinity();

[[nodiscard]] constexpr Real clamp(Real v, Real lo, Real hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

[[nodiscard]] constexpr Real lerp(Real a, Real b, Real t) noexcept { return a + (b - a) * t; }

// Égalité à epsilon près, absolue et relative : robuste près de zéro comme
// pour les grandes valeurs.
[[nodiscard]] constexpr bool almostEqual(Real a, Real b, Real eps = kEpsilon) noexcept {
    const Real diff = a > b ? a - b : b - a;
    const Real absA = a < Real(0) ? -a : a;
    const Real absB = b < Real(0) ? -b : b;
    const Real m0 = absA > absB ? absA : absB;
    const Real m = m0 < Real(1) ? Real(1) : m0;
    return diff <= eps * m;
}

[[nodiscard]] constexpr Real degreesToRadians(Real degrees) noexcept {
    return degrees * (kPi / Real(180));
}

[[nodiscard]] constexpr Real radiansToDegrees(Real radians) noexcept {
    return radians * (Real(180) / kPi);
}

// Cas constexpr vérifiés à la compilation (DoD T010).
static_assert(clamp(Real(5), Real(0), Real(1)) == Real(1));
static_assert(lerp(Real(0), Real(10), Real(0.5f)) == Real(5));
static_assert(almostEqual(Real(1.0f), Real(1.0f + 1e-7f)));
static_assert(!almostEqual(Real(1.0f), Real(1.1f)));
static_assert(almostEqual(degreesToRadians(Real(180)), kPi, Real(1e-6f)));

} // namespace rt
