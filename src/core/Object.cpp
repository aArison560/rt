#include "core/Object.hpp"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double	fract(double x){ return x - std::floor(x); } // partie fractionnaire -> [0,1)

bool	Sphere::intersect(const Ray& ray, Hit& hit) const{
	Vec3	oc = ray.origin - center;
	double	a = ray.dir.dot(ray.dir);
	double	b = 2.0 * oc.dot(ray.dir);
	double	c = oc.dot(oc) - radius*radius;
	double	disc = b*b - 4*a*c; // discriminant du trinôme
	if (disc < 0) return false;
	double	sq = std::sqrt(disc);
	double	t1 = (-b - sq) / (2*a);
	double	t2 = (-b + sq) / (2*a);
	double	t = (t1 > 1e-4) ? t1 : t2; // plus proche devant la caméra
	if (t < 1e-4) return false;
	hit.t = t;
	hit.point = ray.origin + ray.dir * t;
	hit.normal = (hit.point - center).normalized();
	hit.color = color;
	hit.material = material; // copie thread-safe
	getUV(hit.point, hit.u, hit.v); // remplit u/v sphériques
	return true;
}

void	Sphere::getUV(const Vec3& point, double& u, double& v) const{
	// Sphérique équirectangulaire : u = longitude, v = latitude (pôles pincés, couture à ±π).
	Vec3	l = (point - center) / radius; // sphère unité
	double	theta = std::atan2(l.z, l.x);          // -pi..pi, 4 quadrants
	double	phi = std::asin(std::fmax(-1.0, std::fmin(1.0, l.y))); // -pi/2..pi/2, clamp anti-arrondi
	u = fract(theta / (2*M_PI) + 0.5);
	v = 0.5 - phi / M_PI;
}

bool	Plane::intersect(const Ray& ray, Hit& hit) const{
	double	denom = normal.dot(ray.dir);
	if (std::fabs(denom) < 1e-6) return false; // rayon parallèle : pas d'impact
	double	t = (pos - ray.origin).dot(normal) / denom;
	if (t < 1e-4) return false;
	hit.t = t;
	hit.point = ray.origin + ray.dir * t;
	hit.normal = normal;
	if (hit.normal.dot(ray.dir) > 0) hit.normal = hit.normal * -1; // face au rayon
	hit.color = color;
	hit.material = material;
	getUV(hit.point, hit.u, hit.v);
	return true;
}

void	Plane::getUV(const Vec3& point, double& u, double& v) const{
	// Planaire : projette sur 2 axes du plan, fract = carrelage infini (1 case = 1 unité monde).
	Vec3	arb{0,1,0};
	if (std::fabs(normal.dot(arb)) > 0.999) arb = Vec3{0,0,1}; // évite n×n = 0
	Vec3	axisU = normal.cross(arb).normalized();
	Vec3	axisV = normal.cross(axisU).normalized();
	Vec3	rel = point - pos;
	u = fract(rel.dot(axisU));
	v = fract(rel.dot(axisV));
}

// Cylinder/Cone déplacés vers Quadric.cpp (Quadric générique AGENTS.md Phase 1)
