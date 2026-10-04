#pragma once

// Rayon, intervalle, AABB, enregistrement d'impact (T013) — POD compacts,
// header-only, noexcept, R2 (aucune exception). Tailles consignées dans
// docs/ARCHITECTURE.md §3.2.

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"

namespace rt {

struct Ray {
    Vec3 origin;
    Vec3 direction;          // supposée non nulle (normalisée par l'appelant)
    std::uint32_t depth = 0; // génération : rebonds depuis le rayon caméra

    constexpr Ray() = default;
    constexpr Ray(Vec3 origin_, Vec3 direction_, std::uint32_t depth_ = 0) noexcept
        : origin(origin_), direction(direction_), depth(depth_) {}

    [[nodiscard]] constexpr Vec3 at(Real t) const noexcept { return origin + direction * t; }
};

struct Interval {
    Real tMin = kInfinity;
    Real tMax = -kInfinity;

    constexpr Interval() = default;
    constexpr Interval(Real tMin_, Real tMax_) noexcept : tMin(tMin_), tMax(tMax_) {}

    [[nodiscard]] static constexpr Interval empty() noexcept { return {kInfinity, -kInfinity}; }
    [[nodiscard]] static constexpr Interval universe() noexcept { return {-kInfinity, kInfinity}; }

    [[nodiscard]] constexpr Real size() const noexcept { return tMax - tMin; }
    [[nodiscard]] constexpr bool contains(Real t) const noexcept { return tMin <= t && t <= tMax; }
    [[nodiscard]] constexpr bool surrounds(Real t) const noexcept { return tMin < t && t < tMax; }
    [[nodiscard]] constexpr Real clamp(Real t) const noexcept { return ::rt::clamp(t, tMin, tMax); }
    [[nodiscard]] constexpr Interval merged(Interval o) const noexcept {
	return {tMin < o.tMin ? tMin : o.tMin, tMax > o.tMax ? tMax : o.tMax};
    }
    // Élargit l'intervalle pour éviter les AABB dégénérées (épaisseur nulle).
    [[nodiscard]] constexpr Interval expanded(Real delta) const noexcept {
	return {tMin - delta, tMax + delta};
    }
};

struct AABB {
    Vec3 min;
    Vec3 max;

    constexpr AABB() = default;
    constexpr AABB(Vec3 min_, Vec3 max_) noexcept : min(min_), max(max_) {}

    [[nodiscard]] constexpr Vec3 center() const noexcept { return (min + max) * Real(0.5f); }

    [[nodiscard]] constexpr AABB merged(AABB o) const noexcept {
	return {min3(min, o.min), max3(max, o.max)};
    }

    // AABB gonflée d'au moins `delta` sur chaque axe (anti volume nul).
    [[nodiscard]] constexpr AABB padded(Real delta) const noexcept {
	const Vec3 pad = {delta, delta, delta};
	return {min - pad, max + pad};
    }

    // Test d'intersection rayon/AABB par la méthode des dalles (slabs).
    // Retourne true si le rayon traverse la boîte dans [tMin, tMax].
    [[nodiscard]] constexpr bool hit(const Ray& ray, Interval interval) const noexcept {
	for (std::size_t axis = 0; axis < 3; ++axis) {
	    const Real origin = ray.origin[axis];
	    const Real dir = ray.direction[axis];
	    if (dir > kEpsilon || dir < -kEpsilon) {
		const Real invD = Real(1) / dir;
		Real t0 = (min[axis] - origin) * invD;
		Real t1 = (max[axis] - origin) * invD;
		if (t0 > t1) {
		    const Real tmp = t0;
		    t0 = t1;
		    t1 = tmp;
		}
		if (t0 > interval.tMin) {
		    interval.tMin = t0;
		}
		if (t1 < interval.tMax) {
		    interval.tMax = t1;
		}
	    } else {
		// Rayon parallèle à l'axe : rejeté s'il est hors des dalles.
		if (origin < min[axis] || origin > max[axis]) {
		    return false;
		}
	    }
	    if (interval.tMax < interval.tMin) {
		return false;
	    }
	}
	return true;
    }

  private:
    [[nodiscard]] static constexpr Vec3 min3(Vec3 a, Vec3 b) noexcept {
	return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z};
    }
    [[nodiscard]] static constexpr Vec3 max3(Vec3 a, Vec3 b) noexcept {
	return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z};
    }
};

struct HitRecord {
    Vec3 point;
    Vec3 normal; // toujours orientée contre le rayon incident
    Real t = Real(0);
    bool frontFace = true; // le rayon arrive-t-il de l'extérieur ?
    std::uint32_t materialIndex = 0;
    Vec2 uv;

    constexpr HitRecord() = default;

    // `outwardNormal` est la normale géométrique sortante ; `normal` est son
    // orientation pour le shading (frontFace ? outward : -outward).
    constexpr void setFaceNormal(const Ray& ray, Vec3 outwardNormal) noexcept {
	frontFace = dot(ray.direction, outwardNormal) < Real(0);
	normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

static_assert(std::is_trivially_copyable_v<Ray>);
static_assert(std::is_trivially_copyable_v<Interval>);
static_assert(std::is_trivially_copyable_v<AABB>);
static_assert(std::is_trivially_copyable_v<HitRecord>);
static_assert(std::is_standard_layout_v<Ray>);
static_assert(std::is_standard_layout_v<Interval>);
static_assert(std::is_standard_layout_v<AABB>);
static_assert(std::is_standard_layout_v<HitRecord>);

} // namespace rt
