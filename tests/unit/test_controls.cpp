// Controles clavier (T073, M5) — tests Catch2, sans ecran.
// DoD : chaque touche a un effet visible ; documentee ; pas de re-affichage
// gratuit (compteurs : `None` -> faux, mappée -> vrai + fanions R5).

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

TEST_CASE("controls : touches inconnues sans effet (T073)", "[controls]") {
	rt::scene::Scene scene = makeScene();
	REQUIRE(rt::app::keyFromSdl(0) == rt::app::KeyAction::None);
	REQUIRE(rt::app::keyFromSdl(999999) == rt::app::KeyAction::None);
	REQUIRE_FALSE(rt::app::applyKeyAction(scene, rt::app::KeyAction::None));
	REQUIRE_FALSE(scene.sceneDirty);
	REQUIRE_FALSE(scene.displayDirty);
}

TEST_CASE("controls : camera avance/recule et fanions R5 (T073)", "[controls]") {
	rt::scene::Scene scene = makeScene();
	const rt::Vec3 pos0 = scene.camera.position;
	const rt::Vec3 tgt0 = scene.camera.target;
	const std::uint64_t version0 = scene.objectVersion;

	REQUIRE(rt::app::keyFromSdl('w') == rt::app::KeyAction::Forward);
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::Forward));
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.displayDirty);
	// Camera seule : BVH conservee (pas de version++).
	REQUIRE(scene.objectVersion == version0);
	REQUIRE((scene.camera.position.x != pos0.x || scene.camera.position.y != pos0.y ||
	         scene.camera.position.z != pos0.z));
	REQUIRE((scene.camera.target.x != tgt0.x || scene.camera.target.y != tgt0.y ||
	         scene.camera.target.z != tgt0.z));

	scene.markClean();
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::Back));
	REQUIRE(scene.sceneDirty);
	// Aller + retour ~= point de depart (pas de 0.5, direction normalisee).
	REQUIRE(scene.camera.position.x == Catch::Approx(pos0.x).margin(1e-4));
	REQUIRE(scene.camera.position.y == Catch::Approx(pos0.y).margin(1e-4));
	REQUIRE(scene.camera.position.z == Catch::Approx(pos0.z).margin(1e-4));
}

TEST_CASE("controls : lateraux, vertical, FOV et reset (T073)", "[controls]") {
	rt::scene::Scene scene = makeScene();
	const float fov0 = scene.camera.fov;

	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::StrafeLeft));
	REQUIRE(scene.sceneDirty);
	scene.markClean();
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::StrafeRight));
	REQUIRE(scene.sceneDirty);
	scene.markClean();
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::MoveUp));
	REQUIRE(scene.sceneDirty);
	scene.markClean();

	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::FovIn));
	REQUIRE(scene.camera.fov < fov0);
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::FovOut));
	REQUIRE(scene.camera.fov == Catch::Approx(fov0).margin(1e-5));

	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::Reset));
	REQUIRE(scene.camera.fov == Catch::Approx(60.0F).margin(1e-5));
}

TEST_CASE("controls : lumieres 1/2 et bornes (T073)", "[controls]") {
	rt::scene::Scene scene = makeScene();
	// Sans lumiere : pas d'effet, pas de fanion (pas de re-affichage gratuit).
	scene.lights.clear();
	scene.markClean();
	REQUIRE_FALSE(rt::app::applyKeyAction(scene, rt::app::KeyAction::LightUp));
	REQUIRE_FALSE(scene.sceneDirty);

	rt::scene::Light light;
	light.intensity = 1.0F;
	scene.lights.push_back(light);
	scene.markClean();
	const std::uint64_t version0 = scene.objectVersion;
	REQUIRE(rt::app::keyFromSdl('1') == rt::app::KeyAction::LightUp);
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::LightUp));
	REQUIRE(scene.lights[0].intensity > 1.0F);
	// Lumiere -> version++ (BVH a revalider, T062).
	REQUIRE(scene.objectVersion == version0 + 1);
	REQUIRE(rt::app::applyKeyAction(scene, rt::app::KeyAction::LightDown));
}
