#pragma once
#include "core/Scene.hpp"
#include <cstdint>

class Renderer{
	public:
		static int	render(Scene& scene, uint32_t* buffer); // multi-thread lignes : remplit ARGB8888
		static int	renderPreview(Scene& scene, uint32_t* buffer, int step); // preview fluide : 1 rayon par bloc step×step
		static Vec3	computeLight(const Scene& scene, const Hit& hit); // base image>checker>uni + diffuse + ombre dure
};

// Compat C
inline int	render(t_scene *s, uint32_t *b){ return Renderer::render(*s, b); }
inline Vec3 compute_light(t_scene *s, Hit *h){ return Renderer::computeLight(*s, *h); }
inline int	intersect_sphere(Ray r, s_obj *o, Hit *h){ Sphere s(o->pos, o->radius, o->color); return s.intersect(r, *h) ? 1 : 0; }
inline int	intersect_plane(Ray r, s_obj *o, Hit *h){ Plane p(o->pos, o->dir, o->color); return p.intersect(r, *h) ? 1 : 0; }
inline int	intersect_cylinder(Ray r, s_obj *o, Hit *h){ (void)r; (void)o; (void)h; return 0; } // legacy : voir Cylinder (Quadric)
