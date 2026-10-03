#pragma once
#include "math/Vec3.hpp"

// Rayon : origine + direction (direction supposée normalisée à l'usage).
struct Ray{
	Vec3	origin,
			dir;
	Ray() {}
	Ray(const Vec3& o, const Vec3& d): origin(o), dir(d) {}
};
using t_ray = Ray;
