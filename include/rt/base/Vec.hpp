#pragma once

// Vecteurs du noyau `base` (T011) — header-only, tout noexcept.
// Règle R2 (docs/MEMORY_STRATEGY.md §3.3) : aucune exception, jamais.
// Les cas dégénérés ont un comportement défini et documenté :
//   - v / s avec |s| <= kEpsilon → renvoie v **inchangé** ;
//   - normalize() d'un vecteur nul → renvoie le vecteur nul ;
//   - refract() en réflexion totale interne → renvoie le vecteur nul.
// C'est le correctif du bug v1 (throw dans Vec3::operator/ et normalize).

#include <cmath>
#include <cstddef>
#include <type_traits>

#include "rt/base/Scalar.hpp"

namespace rt {

struct Vec2 {
    Real x = Real(0);
    Real y = Real(0);

    constexpr Vec2() = default;
    constexpr Vec2(Real x_, Real y_) noexcept : x(x_), y(y_) {}

    [[nodiscard]] constexpr Real& operator[](std::size_t i) noexcept { return i == 0 ? x : y; }
    [[nodiscard]] constexpr Real operator[](std::size_t i) const noexcept { return i == 0 ? x : y; }

    [[nodiscard]] constexpr Vec2 operator-() const noexcept { return {-x, -y}; }
    constexpr Vec2& operator+=(Vec2 o) noexcept {
	x += o.x;
	y += o.y;
	return *this;
    }
    constexpr Vec2& operator-=(Vec2 o) noexcept {
	x -= o.x;
	y -= o.y;
	return *this;
    }
    constexpr Vec2& operator*=(Real s) noexcept {
	x *= s;
	y *= s;
	return *this;
    }
    // Division par un scalaire quasi nul → vecteur inchangé (documenté).
    constexpr Vec2& operator/=(Real s) noexcept {
	if (s > kEpsilon || s < -kEpsilon) {
	    x /= s;
	    y /= s;
	}
	return *this;
    }
};

struct Vec3 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);

    constexpr Vec3() = default;
    constexpr Vec3(Real x_, Real y_, Real z_) noexcept : x(x_), y(y_), z(z_) {}

    [[nodiscard]] constexpr Real& operator[](std::size_t i) noexcept {
	return i == 0 ? x : (i == 1 ? y : z);
    }
    [[nodiscard]] constexpr Real operator[](std::size_t i) const noexcept {
	return i == 0 ? x : (i == 1 ? y : z);
    }

    [[nodiscard]] constexpr Vec3 operator-() const noexcept { return {-x, -y, -z}; }
    constexpr Vec3& operator+=(Vec3 o) noexcept {
	x += o.x;
	y += o.y;
	z += o.z;
	return *this;
    }
    constexpr Vec3& operator-=(Vec3 o) noexcept {
	x -= o.x;
	y -= o.y;
	z -= o.z;
	return *this;
    }
    constexpr Vec3& operator*=(Real s) noexcept {
	x *= s;
	y *= s;
	z *= s;
	return *this;
    }
    constexpr Vec3& operator/=(Real s) noexcept {
	if (s > kEpsilon || s < -kEpsilon) {
	    x /= s;
	    y /= s;
	    z /= s;
	}
	return *this;
    }
};

struct Vec4 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);
    Real w = Real(0);

    constexpr Vec4() = default;
    constexpr Vec4(Real x_, Real y_, Real z_, Real w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]] constexpr Real& operator[](std::size_t i) noexcept {
	return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
    }
    [[nodiscard]] constexpr Real operator[](std::size_t i) const noexcept {
	return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
    }

    [[nodiscard]] constexpr Vec4 operator-() const noexcept { return {-x, -y, -z, -w}; }
    constexpr Vec4& operator+=(Vec4 o) noexcept {
	x += o.x;
	y += o.y;
	z += o.z;
	w += o.w;
	return *this;
    }
    constexpr Vec4& operator-=(Vec4 o) noexcept {
	x -= o.x;
	y -= o.y;
	z -= o.z;
	w -= o.w;
	return *this;
    }
    constexpr Vec4& operator*=(Real s) noexcept {
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return *this;
    }
    constexpr Vec4& operator/=(Real s) noexcept {
	if (s > kEpsilon || s < -kEpsilon) {
	    x /= s;
	    y /= s;
	    z /= s;
	    w /= s;
	}
	return *this;
    }
};

// Opérateurs binaires — mêmes règles de garde que les op= ci-dessus.
[[nodiscard]] constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
[[nodiscard]] constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
[[nodiscard]] constexpr Vec2 operator*(Vec2 v, Real s) noexcept { return {v.x * s, v.y * s}; }
[[nodiscard]] constexpr Vec2 operator*(Real s, Vec2 v) noexcept { return v * s; }
[[nodiscard]] constexpr Vec2 operator*(Vec2 a, Vec2 b) noexcept { return {a.x * b.x, a.y * b.y}; }
[[nodiscard]] constexpr Vec2 operator/(Vec2 v, Real s) noexcept {
    return (s > kEpsilon || s < -kEpsilon) ? Vec2(v.x / s, v.y / s) : v;
}

[[nodiscard]] constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
[[nodiscard]] constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
[[nodiscard]] constexpr Vec3 operator*(Vec3 v, Real s) noexcept {
    return {v.x * s, v.y * s, v.z * s};
}
[[nodiscard]] constexpr Vec3 operator*(Real s, Vec3 v) noexcept { return v * s; }
[[nodiscard]] constexpr Vec3 operator*(Vec3 a, Vec3 b) noexcept {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
[[nodiscard]] constexpr Vec3 operator/(Vec3 v, Real s) noexcept {
    return (s > kEpsilon || s < -kEpsilon) ? Vec3(v.x / s, v.y / s, v.z / s) : v;
}

[[nodiscard]] constexpr Vec4 operator+(Vec4 a, Vec4 b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
[[nodiscard]] constexpr Vec4 operator-(Vec4 a, Vec4 b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
[[nodiscard]] constexpr Vec4 operator*(Vec4 v, Real s) noexcept {
    return {v.x * s, v.y * s, v.z * s, v.w * s};
}
[[nodiscard]] constexpr Vec4 operator*(Real s, Vec4 v) noexcept { return v * s; }
[[nodiscard]] constexpr Vec4 operator*(Vec4 a, Vec4 b) noexcept {
    return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}
[[nodiscard]] constexpr Vec4 operator/(Vec4 v, Real s) noexcept {
    return (s > kEpsilon || s < -kEpsilon) ? Vec4(v.x / s, v.y / s, v.z / s, v.w / s) : v;
}

// Produits scalaire / vectoriel.
[[nodiscard]] constexpr Real dot(Vec2 a, Vec2 b) noexcept { return a.x * b.x + a.y * b.y; }
[[nodiscard]] constexpr Real dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
[[nodiscard]] constexpr Real dot(Vec4 a, Vec4 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
[[nodiscard]] constexpr Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

[[nodiscard]] inline Real lengthSquared(Vec2 v) noexcept { return dot(v, v); }
[[nodiscard]] inline Real lengthSquared(Vec3 v) noexcept { return dot(v, v); }
[[nodiscard]] inline Real lengthSquared(Vec4 v) noexcept { return dot(v, v); }
[[nodiscard]] inline Real length(Vec2 v) noexcept { return std::sqrt(lengthSquared(v)); }
[[nodiscard]] inline Real length(Vec3 v) noexcept { return std::sqrt(lengthSquared(v)); }
[[nodiscard]] inline Real length(Vec4 v) noexcept { return std::sqrt(lengthSquared(v)); }

// normalize() d'un vecteur nul (ou quasi nul, |v|² <= kEpsilon²) → vecteur nul.
[[nodiscard]] inline Vec2 normalize(Vec2 v) noexcept {
    const Real ls = lengthSquared(v);
    return ls > kEpsilon * kEpsilon ? v / std::sqrt(ls) : Vec2(Real(0), Real(0));
}
[[nodiscard]] inline Vec3 normalize(Vec3 v) noexcept {
    const Real ls = lengthSquared(v);
    return ls > kEpsilon * kEpsilon ? v / std::sqrt(ls) : Vec3(Real(0), Real(0), Real(0));
}
[[nodiscard]] inline Vec4 normalize(Vec4 v) noexcept {
    const Real ls = lengthSquared(v);
    return ls > kEpsilon * kEpsilon ? v / std::sqrt(ls) : Vec4(Real(0), Real(0), Real(0), Real(0));
}

// reflect(v, n) avec n normalisée : composante miroir de v autour de n.
[[nodiscard]] constexpr Vec3 reflect(Vec3 v, Vec3 n) noexcept {
    return v - n * (Real(2) * dot(v, n));
}
[[nodiscard]] constexpr Vec2 reflect(Vec2 v, Vec2 n) noexcept {
    return v - n * (Real(2) * dot(v, n));
}

// refract(v, n, eta) : n normalisée, opposée à v (face avant).
// Réflexion totale interne → vecteur nul (sentinel, jamais de throw).
[[nodiscard]] inline Vec3 refract(Vec3 v, Vec3 n, Real eta) noexcept {
    const Real d = dot(-v, n);
    const Real cosTheta = d < Real(1) ? d : Real(1);
    const Vec3 rOutPerp = (v + n * cosTheta) * eta;
    const Real perp2 = lengthSquared(rOutPerp);
    if (perp2 > Real(1)) {
	return {Real(0), Real(0), Real(0)};
    }
    const Vec3 rOutParallel = n * (-std::sqrt(Real(1) - perp2));
    return rOutPerp + rOutParallel;
}
[[nodiscard]] inline Vec2 refract(Vec2 v, Vec2 n, Real eta) noexcept {
    const Real d = dot(-v, n);
    const Real cosTheta = d < Real(1) ? d : Real(1);
    const Vec2 rOutPerp = (v + n * cosTheta) * eta;
    const Real perp2 = lengthSquared(rOutPerp);
    if (perp2 > Real(1)) {
	return {Real(0), Real(0)};
    }
    const Vec2 rOutParallel = n * (-std::sqrt(Real(1) - perp2));
    return rOutPerp + rOutParallel;
}

// min/max composante à composante.
[[nodiscard]] constexpr Vec2 min(Vec2 a, Vec2 b) noexcept {
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
}
[[nodiscard]] constexpr Vec3 min(Vec3 a, Vec3 b) noexcept {
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z};
}
[[nodiscard]] constexpr Vec4 min(Vec4 a, Vec4 b) noexcept {
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z,
            a.w < b.w ? a.w : b.w};
}
[[nodiscard]] constexpr Vec2 max(Vec2 a, Vec2 b) noexcept {
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
}
[[nodiscard]] constexpr Vec3 max(Vec3 a, Vec3 b) noexcept {
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z};
}
[[nodiscard]] constexpr Vec4 max(Vec4 a, Vec4 b) noexcept {
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z,
            a.w > b.w ? a.w : b.w};
}

// nearZero : toutes les composantes strictement sous kEpsilon en valeur absolue.
[[nodiscard]] constexpr bool nearZero(Vec2 v) noexcept {
    return (v.x < kEpsilon && v.x > -kEpsilon) && (v.y < kEpsilon && v.y > -kEpsilon);
}
[[nodiscard]] constexpr bool nearZero(Vec3 v) noexcept {
    return (v.x < kEpsilon && v.x > -kEpsilon) && (v.y < kEpsilon && v.y > -kEpsilon) &&
           (v.z < kEpsilon && v.z > -kEpsilon);
}
[[nodiscard]] constexpr bool nearZero(Vec4 v) noexcept {
    return (v.x < kEpsilon && v.x > -kEpsilon) && (v.y < kEpsilon && v.y > -kEpsilon) &&
           (v.z < kEpsilon && v.z > -kEpsilon) && (v.w < kEpsilon && v.w > -kEpsilon);
}

// Structures POD triviales : vérifié à la compilation.
static_assert(std::is_trivially_copyable_v<Vec2>);
static_assert(std::is_trivially_copyable_v<Vec3>);
static_assert(std::is_trivially_copyable_v<Vec4>);

} // namespace rt
