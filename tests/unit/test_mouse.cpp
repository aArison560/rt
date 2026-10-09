// Souris et molette (T074, M5) — tests Catch2, sans ecran.
// DoD : deplacement camera visible immediatement ; scene inchangee sauf
// camera (rappel image1/image2 : seul l'oeil bouge).

#include <catch2/catch_amalgamated.hpp>

#include "rt/app/Controls.hpp"
#include "rt/scene/Scene.hpp"

namespace {

rt::scene::Scene makeScene() {
	rt::scene::Scene scene;
	scene.init();
	scene.markClean();
	return scene;
}

} // namespace

TEST_CASE("mouse : orbite change la camera, pas le reste (T074)", "[mouse]") {
	rt::scene::Scene scene = makeScene();
	const rt::Vec3 pos0 = scene.camera.position;
	const std::size_t lights0 = scene.lights.size();
	const std::size_t objs0 = scene.objects.size();
	const float fov0 = scene.camera.fov;

	REQUIRE_FALSE(rt::app::orbitCamera(scene, 0, 0));
	REQUIRE_FALSE(scene.sceneDirty);
	REQUIRE(rt::app::orbitCamera(scene, 40, 0));
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.displayDirty);
	REQUIRE((scene.camera.position.x != pos0.x || scene.camera.position.z != pos0.z));
	// Cible fixe (orbite autour), FOV/lumieres/objets intacts.
	REQUIRE(scene.lights.size() == lights0);
	REQUIRE(scene.objects.size() == objs0);
	REQUIRE(scene.camera.fov == Catch::Approx(fov0).margin(1e-6));

	scene.markClean();
	REQUIRE(rt::app::orbitCamera(scene, 0, 30));
	REQUIRE(scene.sceneDirty);
	// Pitch borne (pas de retournement) : y reste fini.
	REQUIRE(std::isfinite(scene.camera.position.y));
}

TEST_CASE("mouse : molette FOV en direct et bornes (T074)", "[mouse]") {
	rt::scene::Scene scene = makeScene();
	const float fov0 = scene.camera.fov;

	REQUIRE_FALSE(rt::app::adjustFov(scene, 0));
	REQUIRE_FALSE(scene.sceneDirty);
	REQUIRE(rt::app::adjustFov(scene, 1));
	REQUIRE(scene.camera.fov < fov0);
	REQUIRE(scene.sceneDirty);
	scene.markClean();
	REQUIRE(rt::app::adjustFov(scene, -1));
	REQUIRE(scene.camera.fov == Catch::Approx(fov0).margin(1e-5));

	// Bornes 10..120 (schema R1) : 100 crans ne depassent pas.
	for (int i = 0; i < 100; ++i) {
		rt::app::adjustFov(scene, 1);
	}
	REQUIRE(scene.camera.fov >= 10.0F);
	for (int i = 0; i < 100; ++i) {
		rt::app::adjustFov(scene, -1);
	}
	REQUIRE(scene.camera.fov <= 120.0F);
}
