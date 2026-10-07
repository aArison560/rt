// Modele `Scene` (T028, pose par T023) — cycle de vie sans exception (R2).
// `init`/`reset` restaurent les defauts de `docs/FORMAT_SCENE.md` §5
// (memes valeurs que la table `src/schema/`) en conservant la capacite
// des vecteurs (`clear` + affectation, pas de `shrink`) ; `touchObjects`
// incremente `objectVersion` et leve les drapeaux R5 (future BVH T062).

#include "rt/scene/Scene.hpp"

namespace rt::scene {

const char* toString(LightType type) noexcept {
	switch (type) {
	case LightType::Point:
		return "point";
	case LightType::Spot:
		return "spot";
	case LightType::Directional:
		return "directional";
	case LightType::Area:
		return "area";
	}
	return "unknown";
}

const char* toString(ObjectType type) noexcept {
	switch (type) {
	case ObjectType::Sphere:
		return "sphere";
	case ObjectType::Plane:
		return "plane";
	case ObjectType::Cylinder:
		return "cylinder";
	case ObjectType::Cone:
		return "cone";
	}
	return "unknown";
}

namespace {

void assignDefaults(Scene& scene) {
	scene.name.clear();
	scene.limits = Limits();
	scene.camera = Camera();
	scene.background = Background();
	scene.ambient = Ambient();
	scene.sceneDirty = true;
	scene.displayDirty = true;
}

} // namespace

void Scene::init() {
	lights.clear();
	objects.clear();
	groups.clear();
	assignDefaults(*this);
	objectVersion = 0;
}

void Scene::reset() {
	// Conserve la capacite : `clear` ne desalloue pas, les affectations
	// de defauts n'allouent que `name` vide (petite chaine, SSO).
	lights.clear();
	objects.clear();
	groups.clear();
	assignDefaults(*this);
}

void Scene::clear() {
	name.clear();
	lights.clear();
	objects.clear();
	groups.clear();
	sceneDirty = true;
	displayDirty = true;
}

void Scene::markClean() {
	sceneDirty = false;
	displayDirty = false;
}

void Scene::touchObjects() {
	++objectVersion;
	sceneDirty = true;
	displayDirty = true;
}

namespace {

std::size_t countGroup(const Group& group) {
	std::size_t total = group.objects.size();
	for (const std::shared_ptr<Group>& child : group.children) {
		if (child) {
			total += countGroup(*child);
		}
	}
	return total;
}

} // namespace

std::size_t Scene::totalObjectCount() const {
	std::size_t total = objects.size();
	for (const Group& group : groups) {
		total += countGroup(group);
	}
	return total;
}

} // namespace rt::scene
