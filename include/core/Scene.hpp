#pragma once
#include "math/Vec3.hpp"
#include "core/Object.hpp"
#include <vector>
#include <memory>
#include <string>

struct Camera{
	Vec3	pos{0, 0, -5};
	Vec3	dir{0, 0, 1}; // forward normalisée au parsing
	double	fov = 60; // degrés
};
using t_cam = Camera;

struct Light{
	Vec3	pos{5, 5, -5}; // ponctuelle unique (Phase 3 : multi-lights)
	double	intensity = 0.8;
};
using t_light = Light;

class Scene{
	public:
		Camera									cam;
		Light									light;
		Vec3									ambient{0.1, 0.1, 0.1}; // éclairage minimal anti-noir
		std::vector<std::unique_ptr<Object>>	objects; // propriété unique : clear() suffit, pas de new nu
		int										width = 800,
												height = 600;
		std::string								filepath;

		// Compat C : liste chaînée miroir pour ancien code
		s_obj*									objs = nullptr;

		Scene() = default;
		~Scene();

		void	clear(); // vide objects + libère la liste C (obligatoire, anti-fuite)
		// Déplacements Blender Phase 5.5 : O(1), sans réallocation.
		inline void moveCamera(const Vec3& delta) noexcept { cam.pos = cam.pos + delta; } // Shift+MMB pan : cam.x++ => objet à gauche
		inline void moveLight(const Vec3& delta) noexcept { light.pos = light.pos + delta; } // G : L.x++ => ombre à gauche
		inline void translateObject(size_t idx, const Vec3& delta) { if (idx < objects.size()) objects[idx]->translate(delta); } // G X/Y/Z
		inline void setObjectPos(size_t idx, const Vec3& p) { if (idx < objects.size()) objects[idx]->setPos(p); }
		inline void rotateObject(size_t idx, const Vec3& newDir) { if (idx < objects.size()) objects[idx]->setDir(newDir); } // R : 1 rebuild cache
};

// Compat C
using	t_scene = Scene;
void	scene_free(Scene *scene);
int		parse_scene(const char *path, Scene *scene);
