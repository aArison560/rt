// Tests du modele `Scene` (T028), Catch2.
// Couvre le Prompt et le DoD : `init`/`reset`/`clear` (reutilisation sans
// realloc), `reserve` a la capacite annoncee des la scene lue, drapeaux
// `sceneDirty`/`displayDirty` + compteur `objectVersion` (future BVH T062),
// lecture seule via `const Scene&`, memoire bornee par `limits`.

#include <catch2/catch_amalgamated.hpp>

#include <cstddef>
#include <string_view>

#include "rt/scene/Parser.hpp"
#include "rt/scene/Scene.hpp"

namespace {

bool parseOk(std::string_view content, rt::scene::Scene& out,
             std::string_view name = "t.rt") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseContent(content, name);
	if (result.isError()) {
		INFO("erreur inattendue : " << result.status().message);
		return false;
	}
	out = std::move(result.value());
	return true;
}

// Lecture seule pour les calques superieurs : ne prend que `const Scene&`.
std::size_t countReadOnly(const rt::scene::Scene& scene) {
	return scene.totalObjectCount() + scene.lights.size();
}

} // namespace

TEST_CASE("scene : init restaure les defauts et reserve", "[scene]") {
	rt::scene::Scene scene;
	scene.init();
	REQUIRE(scene.limits.width == 640);
	REQUIRE(scene.limits.height == 480);
	REQUIRE(scene.limits.maxObjects == 256);
	REQUIRE(scene.limits.maxLights == 16);
	REQUIRE(scene.lights.empty());
	REQUIRE(scene.objects.empty());
	REQUIRE(scene.groups.empty());
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.displayDirty);
	REQUIRE(scene.objectVersion == 0);
	// Capacite annoncee des la scene lue (bornee, sans realloc courante).
	REQUIRE(scene.lights.capacity() >= 16);
	REQUIRE(scene.objects.capacity() >= 256);
	REQUIRE(scene.groups.capacity() >= 16);
	REQUIRE(countReadOnly(scene) == 0);
}

TEST_CASE("scene : reset reutilise la capacite sans realloc", "[scene]") {
	rt::scene::Scene scene;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } lights { "
	                "light { position (0 0 0) } } "
	                "objects { object { type sphere } object { type sphere } } }",
	                scene));
	REQUIRE(scene.objects.size() == 2);
	REQUIRE(scene.lights.size() == 1);
	const std::size_t lightCap = scene.lights.capacity();
	const std::size_t objectCap = scene.objects.capacity();
	const std::size_t groupCap = scene.groups.capacity();
	scene.markClean();
	REQUIRE_FALSE(scene.sceneDirty);
	scene.reset();
	// Apres reset : vide, sale (a revalider), capacite conservee.
	REQUIRE(scene.lights.empty());
	REQUIRE(scene.objects.empty());
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.displayDirty);
	REQUIRE(scene.lights.capacity() == lightCap);
	REQUIRE(scene.objects.capacity() == objectCap);
	REQUIRE(scene.groups.capacity() == groupCap);
	// Les defauts sont restaures.
	REQUIRE(scene.limits.width == 640);
	REQUIRE(scene.camera.fov == Catch::Approx(60.0F));
}

TEST_CASE("scene : clear preserve la capacite", "[scene]") {
	rt::scene::Scene scene;
	scene.init();
	scene.objects.push_back(rt::scene::Object());
	scene.lights.push_back(rt::scene::Light());
	const std::size_t lightCap = scene.lights.capacity();
	const std::size_t objectCap = scene.objects.capacity();
	scene.clear();
	REQUIRE(scene.lights.empty());
	REQUIRE(scene.objects.empty());
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.displayDirty);
	REQUIRE(scene.lights.capacity() == lightCap);
	REQUIRE(scene.objects.capacity() == objectCap);
}

TEST_CASE("scene : objectVersion incremente a chaque mutation", "[scene]") {
	rt::scene::Scene scene;
	scene.init();
	REQUIRE(scene.objectVersion == 0);
	scene.touchObjects();
	REQUIRE(scene.objectVersion == 1);
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.displayDirty);
	scene.markClean();
	REQUIRE_FALSE(scene.sceneDirty);
	REQUIRE_FALSE(scene.displayDirty);
	REQUIRE(scene.objectVersion == 1);
	scene.touchObjects();
	REQUIRE(scene.objectVersion == 2);
	scene.touchObjects();
	REQUIRE(scene.objectVersion == 3);
	// Le parser touche une fois puis nettoie (version 1, propre).
	rt::scene::Scene parsed;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } "
	                "objects { object { type sphere } } }",
	                parsed));
	REQUIRE(parsed.objectVersion == 1);
	REQUIRE_FALSE(parsed.sceneDirty);
	REQUIRE_FALSE(parsed.displayDirty);
	REQUIRE(countReadOnly(parsed) == 1);
}

TEST_CASE("scene : totalObjectCount et memoire bornee par limits", "[scene]") {
	rt::scene::Scene scene;
	REQUIRE(parseOk("scene { limits { max_objects 16 max_lights 4 } "
	                "camera { position (0 2 6) target (0 0 0) } "
	                "lights { light { position (0 0 0) } light { position (1 1 1) } } "
	                "objects { group \"g\" { object { type sphere } } "
	                "object { type sphere } } }",
	                scene));
	REQUIRE(scene.totalObjectCount() == 2);
	REQUIRE(scene.totalObjectCount() <= static_cast<std::size_t>(scene.limits.maxObjects));
	REQUIRE(scene.lights.size() <= static_cast<std::size_t>(scene.limits.maxLights));
	// Borne memoire : objets + lumieres contenus dans `limits`.
	const std::size_t memObjects =
	    static_cast<std::size_t>(scene.limits.maxObjects) * sizeof(rt::scene::Object);
	const std::size_t memLights =
	    static_cast<std::size_t>(scene.limits.maxLights) * sizeof(rt::scene::Light);
	REQUIRE(memObjects == 16 * sizeof(rt::scene::Object));
	REQUIRE(memLights == 4 * sizeof(rt::scene::Light));
	REQUIRE(scene.objects.size() * sizeof(rt::scene::Object) <= memObjects);
}
