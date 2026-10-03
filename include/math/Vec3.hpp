#pragma once
#include <cmath>

// Vecteur 3D double : base de tout (positions, directions, couleurs 0-255).
struct Vec3{
	double	x,
			y,
			z;
	Vec3(): x(0), y(0), z(0) {}
	Vec3(double x_, double y_, double z_): x(x_), y(y_), z(z_) {}

	// Arithmétique composante par composante.
	Vec3	operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
	Vec3	operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
	Vec3	operator*(double k) const { return Vec3(x * k, y * k, z * k); }
	Vec3	operator/(double k) const { return Vec3(x / k, y / k, z / k); }

	double	dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }  									// produit scalaire (angle, projection)
	Vec3	cross(const Vec3& o) const { return Vec3(y* o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); }	// produit vectoriel (base locale, normales)
	double	length() const { return std::sqrt(dot(*this)); }
	Vec3	normalized() const{
		double	l = length();
		if (l < 1e-9) return *this; // vecteur nul : inchangé (évite NaN)
		return *this * (1.0/l);
	}
};

// Compat C typedef
using t_vec3 = Vec3;
inline Vec3		vec3_add(Vec3 a, Vec3 b){ return a + b; }
inline Vec3		vec3_sub(Vec3 a, Vec3 b){ return a - b; }
inline Vec3		vec3_mul(Vec3 v, double k){ return v * k; }
inline double	vec3_dot(Vec3 a, Vec3 b){ return a.dot(b); }
inline Vec3		vec3_cross(Vec3 a, Vec3 b){ return a.cross(b); }
inline double	vec3_length(Vec3 v){ return v.length(); }
inline Vec3		vec3_normalize(Vec3 v){ return v.normalized(); }
