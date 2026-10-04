#pragma once

// Matrices 4×4 et transformations rigides/affines (T012) — header-only, noexcept.
// Règle R2 : inverse() ne throw jamais — déterminant quasi nul (|det| <= kEpsilon)
// → std::nullopt. Mêmes gardes sentinelles que Vec.hpp pour les cas dégénérés.
//
// Convention : Mat4 stocke m[row][col], vecteur colonne : v' = M · v.
// Les rotations sont en main droite, angles en radians (Real).

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <type_traits>

#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"

namespace rt {

struct Mat4 {
    std::array<std::array<Real, 4>, 4> m = {};

    constexpr Mat4() noexcept = default;

    [[nodiscard]] constexpr Real& operator()(std::size_t r, std::size_t c) noexcept {
	return m[r][c];
    }
    [[nodiscard]] constexpr Real operator()(std::size_t r, std::size_t c) const noexcept {
	return m[r][c];
    }

    // Matrice identité.
    [[nodiscard]] static constexpr Mat4 identity() noexcept {
	Mat4 r;
	r(0, 0) = Real(1);
	r(1, 1) = Real(1);
	r(2, 2) = Real(1);
	r(3, 3) = Real(1);
	return r;
    }

    [[nodiscard]] constexpr Mat4 transpose() const noexcept {
	Mat4 r;
	for (std::size_t i = 0; i < 4; ++i) {
	    for (std::size_t j = 0; j < 4; ++j) {
		r(i, j) = m[j][i];
	    }
	}
	return r;
    }

    [[nodiscard]] constexpr Mat4 operator*(const Mat4& o) const noexcept {
	Mat4 r;
	for (std::size_t i = 0; i < 4; ++i) {
	    for (std::size_t j = 0; j < 4; ++j) {
		Real acc = Real(0);
		for (std::size_t k = 0; k < 4; ++k) {
		    acc += m[i][k] * o.m[k][j];
		}
		r(i, j) = acc;
	    }
	}
	return r;
    }

    // Déterminant par développement de Laplace sur la 1re ligne (cofacteurs 3×3).
    [[nodiscard]] constexpr Real determinant() const noexcept {
	auto det3 = [](Real a, Real b, Real c, Real d, Real e, Real f, Real g, Real h,
	               Real i) constexpr {
	    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
	};
	return m[0][0] * det3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1],
	                      m[3][2], m[3][3]) -
	       m[0][1] * det3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0],
	                      m[3][2], m[3][3]) +
	       m[0][2] * det3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0],
	                      m[3][1], m[3][3]) -
	       m[0][3] * det3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0],
	                      m[3][1], m[3][2]);
    }

    // Inverse par Gauss–Jordan avec pivot partiel. |det| <= kEpsilon → nullopt.
    [[nodiscard]] constexpr std::optional<Mat4> inverse() const noexcept {
	Mat4 a = *this;
	Mat4 r = identity();
	for (std::size_t col = 0; col < 4; ++col) {
	    // Pivot partiel : ligne du plus grand |a| sous la diagonale.
	    std::size_t piv = col;
	    for (std::size_t row = col + 1; row < 4; ++row) {
		const Real av = a(row, col);
		const Real pv = a(piv, col);
		if ((av < Real(0) ? -av : av) > (pv < Real(0) ? -pv : pv)) {
		    piv = row;
		}
	    }
	    const Real pv = a(piv, col);
	    if (pv < kEpsilon && pv > -kEpsilon) {
		return std::nullopt; // singulière
	    }
	    if (piv != col) {
		for (std::size_t j = 0; j < 4; ++j) {
		    Real tmp = a(col, j);
		    a(col, j) = a(piv, j);
		    a(piv, j) = tmp;
		    tmp = r(col, j);
		    r(col, j) = r(piv, j);
		    r(piv, j) = tmp;
		}
	    }
	    const Real d = a(col, col);
	    for (std::size_t j = 0; j < 4; ++j) {
		a(col, j) /= d;
		r(col, j) /= d;
	    }
	    for (std::size_t row = 0; row < 4; ++row) {
		if (row == col) {
		    continue;
		}
		const Real f = a(row, col);
		for (std::size_t j = 0; j < 4; ++j) {
		    a(row, j) -= f * a(col, j);
		    r(row, j) -= f * r(col, j);
		}
	    }
	}
	return r;
    }
};

// Point homogène w=1 (translation appliquée).
[[nodiscard]] constexpr Vec3 transformPoint(const Mat4& m, Vec3 p) noexcept {
    const Real x = m(0, 0) * p.x + m(0, 1) * p.y + m(0, 2) * p.z + m(0, 3);
    const Real y = m(1, 0) * p.x + m(1, 1) * p.y + m(1, 2) * p.z + m(1, 3);
    const Real z = m(2, 0) * p.x + m(2, 1) * p.y + m(2, 2) * p.z + m(2, 3);
    return {x, y, z};
}

// Vecteur directionnel w=0 (pas de translation).
[[nodiscard]] constexpr Vec3 transformVector(const Mat4& m, Vec3 v) noexcept {
    const Real x = m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z;
    const Real y = m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z;
    const Real z = m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z;
    return {x, y, z};
}

// Normale : inverse-transposée de la partie 3×3, puis renormalisée.
// Matrice non inversible → vecteur nul (sentinelle, jamais de throw).
[[nodiscard]] inline Vec3 transformNormal(const Mat4& m, Vec3 n) noexcept {
    Mat4 upper;
    for (std::size_t i = 0; i < 3; ++i) {
	for (std::size_t j = 0; j < 3; ++j) {
	    upper(i, j) = m(i, j);
	}
    }
    upper(3, 3) = Real(1);
    const std::optional<Mat4> inv = upper.inverse();
    if (!inv) {
	return {Real(0), Real(0), Real(0)};
    }
    const Vec3 t = transformVector(inv->transpose(), n);
    return normalize(t);
}

struct Transform {
    Mat4 matrix = Mat4::identity();

    [[nodiscard]] static constexpr Transform translate(Vec3 t) noexcept {
	Transform r;
	r.matrix(0, 3) = t.x;
	r.matrix(1, 3) = t.y;
	r.matrix(2, 3) = t.z;
	return r;
    }

    [[nodiscard]] static constexpr Transform scale(Vec3 s) noexcept {
	Transform r;
	r.matrix(0, 0) = s.x;
	r.matrix(1, 1) = s.y;
	r.matrix(2, 2) = s.z;
	return r;
    }

    [[nodiscard]] static constexpr Transform rotateX(Real a) noexcept {
	const Real c = std::cos(a);
	const Real s = std::sin(a);
	Transform r;
	r.matrix(1, 1) = c;
	r.matrix(1, 2) = -s;
	r.matrix(2, 1) = s;
	r.matrix(2, 2) = c;
	return r;
    }

    [[nodiscard]] static constexpr Transform rotateY(Real a) noexcept {
	const Real c = std::cos(a);
	const Real s = std::sin(a);
	Transform r;
	r.matrix(0, 0) = c;
	r.matrix(0, 2) = s;
	r.matrix(2, 0) = -s;
	r.matrix(2, 2) = c;
	return r;
    }

    [[nodiscard]] static constexpr Transform rotateZ(Real a) noexcept {
	const Real c = std::cos(a);
	const Real s = std::sin(a);
	Transform r;
	r.matrix(0, 0) = c;
	r.matrix(0, 1) = -s;
	r.matrix(1, 0) = s;
	r.matrix(1, 1) = c;
	return r;
    }

    // Composition : applique d'abord `rhs`, puis `*this` (M = M_this · M_rhs).
    [[nodiscard]] constexpr Transform compose(const Transform& rhs) const noexcept {
	return {matrix * rhs.matrix};
    }

    [[nodiscard]] constexpr Vec3 applyPoint(Vec3 p) const noexcept {
	return transformPoint(matrix, p);
    }
    [[nodiscard]] constexpr Vec3 applyVector(Vec3 v) const noexcept {
	return transformVector(matrix, v);
    }
    [[nodiscard]] constexpr Vec3 applyNormal(Vec3 n) const noexcept {
	return transformNormal(matrix, n);
    }
};

// Structures POD triviales : vérifié à la compilation.
static_assert(std::is_trivially_copyable_v<Mat4>);
static_assert(std::is_trivially_copyable_v<Transform>);

} // namespace rt
