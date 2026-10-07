#pragma once

// Modele de donnees `Scene` (T028, utilise par le parser T023).
// Contient les limites, la camera, le fond, l'ambiance, les lumieres,
// les objets et les groupes hierarchiques (cf. docs/FORMAT_SCENE.md §3).
// Les vecteurs grandissent par `push_back` avec controle contre `limits`
// (jamais de `reserve(max)` sur entree non validee : regle T024).
// Cycle de vie (T028) : `init()` (defauts + reserve bornee), `reset()`
// (defauts, capacite conservee), `clear()` (vide, capacite conservee).
// Drapeaux R5 : `sceneDirty` / `displayDirty`, compteur `objectVersion`
// (invalidera la BVH en T062). Lecture seule pour les calques superieurs.
// Aucun `throw` ici (R2) hors allocation `std::vector` du chemin froid.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rt/base/Vec.hpp"

namespace rt::scene {

enum class LightType : std::uint8_t {
	Point = 0,
	Spot = 1,
	Directional = 2,
	Area = 3,
};

[[nodiscard]] const char* toString(LightType type) noexcept;

enum class ObjectType : std::uint8_t {
	Sphere = 0,
	Plane = 1,
	Cylinder = 2,
	Cone = 3,
};

[[nodiscard]] const char* toString(ObjectType type) noexcept;

struct Limits {
	int width = 640;
	int height = 480;
	int samples = 4;
	int maxDepth = 4;
	long long seed = 0;
	int maxObjects = 256;
	int maxLights = 16;
	long long maxTextureBytes = 67108864LL;
};

struct Camera {
	Vec3 position = Vec3(0.0F, 1.0F, 4.0F);
	Vec3 target = Vec3(0.0F, 0.0F, 0.0F);
	Vec3 up = Vec3(0.0F, 1.0F, 0.0F);
	float fov = 60.0F;
};

struct Background {
	Vec3 color = Vec3(0.0F, 0.0F, 0.0F);
};

struct Ambient {
	Vec3 color = Vec3(0.06F, 0.06F, 0.08F);
	float intensity = 1.0F;
};

struct Light {
	LightType type = LightType::Point;
	std::string name;
	Vec3 position = Vec3(0.0F, 0.0F, 0.0F);
	bool hasPosition = false;
	Vec3 color = Vec3(1.0F, 1.0F, 1.0F);
	float intensity = 1.0F;
	Vec3 direction = Vec3(0.0F, 0.0F, 0.0F);
	bool hasDirection = false;
	Vec3 target = Vec3(0.0F, 0.0F, 0.0F);
	bool hasTarget = false;
	float angle = 30.0F;
	Vec3 attenuation = Vec3(1.0F, 0.0F, 0.0F);
	float range = 0.0F;
	bool hasSize = false;
	Vec3 size = Vec3(1.0F, 1.0F, 0.0F);
};

struct TextureRef {
	std::string file;
	Vec3 scale = Vec3(1.0F, 1.0F, 0.0F);
	Vec3 offset = Vec3(0.0F, 0.0F, 0.0F);
	bool present = false;
};

struct PatternRef {
	std::string type = "checker";
	float scale = 1.0F;
	float frequency = 1.0F;
	bool present = false;
};

struct Material {
	Vec3 albedo = Vec3(0.8F, 0.8F, 0.8F);
	float ambient = 0.1F;
	float diffuse = 0.7F;
	float specular = 0.5F;
	float shininess = 32.0F;
	float reflectivity = 0.0F;
	float transparency = 0.0F;
	float ior = 1.5F;
	float bump = 0.0F;
	TextureRef texture;
	PatternRef pattern;
	// Reference par nom (`material "verre"`, resolue en T024) : vide = inline.
	std::string materialRef;
};

struct TransformOp {
	enum class Kind : std::uint8_t {
		Translate = 0,
		Scale = 1,
		Rotate = 2,
	};
	Kind kind = Kind::Translate;
	Vec3 translate = Vec3(0.0F, 0.0F, 0.0F);
	Vec3 scale = Vec3(1.0F, 1.0F, 1.0F);
	char rotateAxis = 'y';
	float rotateAngle = 0.0F;
};

struct Transform {
	std::vector<TransformOp> ops;
};

struct Slice {
	std::string axis = "y";
	bool hasMin = false;
	float minValue = 0.0F;
	bool hasMax = false;
	float maxValue = 0.0F;
	std::string frame = "object";
	std::string shape = "slab";
	bool present = false;
};

struct Object {
	ObjectType type = ObjectType::Sphere;
	bool hasType = false;
	std::string name;
	Vec3 center = Vec3(0.0F, 0.0F, 0.0F);
	float radius = 1.0F;
	Vec3 point = Vec3(0.0F, 0.0F, 0.0F);
	bool hasPoint = false;
	Vec3 normal = Vec3(0.0F, 1.0F, 0.0F);
	bool hasNormal = false;
	Vec3 axis = Vec3(0.0F, 1.0F, 0.0F);
	float angle = 20.0F;
	bool hasHeight = false;
	float height = 0.0F;
	Material material;
	Transform transform;
	Slice slice;
};

struct Group {
	std::string name;
	Transform transform;
	std::vector<Object> objects;
	std::vector<std::shared_ptr<Group>> children;
};

struct Scene {
	std::string name;
	Limits limits;
	Camera camera;
	Background background;
	Ambient ambient;
	std::vector<Light> lights;
	std::vector<Object> objects;
	std::vector<Group> groups;
	bool sceneDirty = true;
	bool displayDirty = true;
	std::uint64_t objectVersion = 0;

	void init();
	void reset();
	void clear();
	void markClean();
	void touchObjects();
	[[nodiscard]] std::size_t totalObjectCount() const;
};

} // namespace rt::scene
