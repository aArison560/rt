#pragma once
#include "math/Ray.hpp"
#include "core/Material.hpp"
#include <cstdint>

// Impact rayon/objet : distance + point + normale + couleur + UV + matériau copié.
struct Hit{
	double	t = 1e30; // distance ; 1e30 = aucun impact
	Vec3	point;
	Vec3	normal; // toujours retournée face au rayon
	Vec3	color;  // fallback uni si ni image ni checker
	double	u = 0;   // Phase 2 : coords texture [0,1)
	double	v = 0;
	Material material; // copie (thread-safe, sans verrou)
};
using t_hit = Hit;

class Object{
	public:
		virtual			~Object() = default;
		virtual bool	intersect(const Ray& ray, Hit& hit) const = 0; // remplit Hit si t le plus proche
		virtual void	getUV(const Vec3& point, double& u, double& v) const { (void)point; u = 0; v = 0; } // défaut : pas de placage
		// Déplacement Blender G/R : O(1), sans réallocation.
		virtual void	translate(const Vec3& delta) = 0; // G X/Y/Z
		virtual Vec3	getPos() const = 0;
		virtual void	setPos(const Vec3& p) = 0;
		virtual Vec3	getDir() const { return Vec3{0,0,1}; } // défaut pour Sphere/Plane
		virtual void	setDir(const Vec3& d) { (void)d; } // no-op si non orientable

		Vec3			color;
		Material		material; // Phase 2 : albedo = color par défaut
};

class Sphere : public Object{
	public:
		Vec3	center;
		double	radius;

		Sphere(const Vec3& c, double r, const Vec3& col){ center = c; radius = r; color = col; material = Material(col); }

		bool	intersect(const Ray& ray, Hit& hit) const override; // discriminant b²-4ac
		void	getUV(const Vec3& point, double& u, double& v) const override; // sphérique atan2/asin
		void	translate(const Vec3& d) override { center = center + d; }
		Vec3	getPos() const override { return center; }
		void	setPos(const Vec3& p) override { center = p; }
};

class Plane : public Object{
	public:
		Vec3	pos;
		Vec3	normal;

		Plane(const Vec3& p, const Vec3& n, const Vec3& col){ pos = p; normal = n.normalized(); color = col; material = Material(col); }

		bool	intersect(const Ray& ray, Hit& hit) const override; // t = (pos-o)·n / (d·n)
		void	getUV(const Vec3& point, double& u, double& v) const override; // planaire : projection + fract
		void	translate(const Vec3& d) override { pos = pos + d; }
		Vec3	getPos() const override { return pos; }
		void	setPos(const Vec3& p) override { pos = p; }
		Vec3	getDir() const override { return normal; }
		void	setDir(const Vec3& d) override { normal = d.normalized(); }
};

// Quadric générique et primitives associées (Cylinder/Cone héritent Quadric)
#include "core/Quadric.hpp"

// Compat C types
enum	e_obj_type {
	OBJ_SPHERE,
	OBJ_PLANE,
	OBJ_CYLINDER,
	OBJ_CONE,
	OBJ_PARABOLOID,
	OBJ_HYPERBOLOID
};

struct	s_obj {
	e_obj_type	type;
	Vec3 		pos;
	Vec3		dir;
	double		radius;
	double		height;
	Vec3		color;
	s_obj		*next; 
};

using	t_obj = s_obj;
using	t_obj_type = e_obj_type;
