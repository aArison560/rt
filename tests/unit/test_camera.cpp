// Tests de la camera de rendu (T031), Catch2.
// DoD : tests verts ; deplacement de la camera change l'image (constate
// par un test de pixels/directions). Cas degeneres -> `Status`, jamais
// `throw` (R2). Hot path `rayForPixel` : `noexcept`, sans allocation (R3).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>

#include "rt/render/Camera.hpp"
#include "rt/scene/Scene.hpp"

#include "rt/base/Result.hpp"
#include "rt/scene/Parser.hpp"

namespace {

rt::render::Camera makeDefault(int width = 320, int height = 240) {
	rt::render::Camera cam;
	rt::Status status =
	    cam.init(rt::Vec3(0.0F, 2.0F, 6.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	             rt::Vec3(0.0F, 1.0F, 0.0F), 50.0F, width, height);
	REQUIRE(status.isOk());
	REQUIRE(cam.isValid());
	return cam;
}

float directionLength(rt::Vec3 v) {
	return std::sqrt(rt::dot(v, v));
}

} // namespace

TEST_CASE("camera : init valide les cas degeneres sans throw", "[camera]") {
	rt::render::Camera cam;
	// Cible == position.
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 0.0F, 0.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 60.0F, 320, 240)
	            .isError());
	REQUIRE_FALSE(cam.isValid());
	// Up nul.
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 0.0F, 0.0F), 60.0F, 320, 240)
	            .isError());
	// Up colineaire (camera verticale : visee (0,-1,0) contre up (0,1,0)).
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 0.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 60.0F, 320, 240)
	            .isError());
	// Fov hors bornes 1..179 (bornes du schema, R1).
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 0.5F, 320, 240)
	            .isError());
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 180.0F, 320, 240)
	            .isError());
	// Dimensions invalides.
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 60.0F, 0, 240)
	            .isError());
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 60.0F, 320, 9000)
	            .isError());
	// Valide apres echecs : l'etat repart proprement.
	REQUIRE(cam
	            .init(rt::Vec3(0.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 60.0F, 320, 240)
	            .isOk());
	REQUIRE(cam.isValid());
}

TEST_CASE("camera : herite des donnees camera{} du schema", "[camera]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	const rt::scene::Scene& scene = parsed.value();

	rt::render::Camera fromScene;
	REQUIRE(fromScene.init(scene.camera, scene.limits.width, scene.limits.height).isOk());
	REQUIRE(fromScene.isValid());

	rt::render::Camera direct;
	REQUIRE(direct
	            .init(scene.camera.position, scene.camera.target, scene.camera.up, scene.camera.fov,
	                  scene.limits.width, scene.limits.height)
	            .isOk());
	// Meme description -> meme base orthonormee.
	REQUIRE(rt::dot(fromScene.forward(), direct.forward()) > 0.99999F);
	REQUIRE(rt::dot(fromScene.right(), direct.right()) > 0.99999F);
	REQUIRE(rt::dot(fromScene.up(), direct.up()) > 0.99999F);
	REQUIRE(fromScene.aspect() == direct.aspect());
}

TEST_CASE("camera : centre de l'image, coins et normalisation", "[camera]") {
	const rt::render::Camera cam = makeDefault(320, 240);

	// Base orthonormee : chaque axe est unitaire et perpendiculaire.
	REQUIRE(std::abs(directionLength(cam.forward()) - 1.0F) < 1e-5F);
	REQUIRE(std::abs(directionLength(cam.right()) - 1.0F) < 1e-5F);
	REQUIRE(std::abs(directionLength(cam.up()) - 1.0F) < 1e-5F);
	REQUIRE(std::abs(rt::dot(cam.forward(), cam.right())) < 1e-5F);
	REQUIRE(std::abs(rt::dot(cam.forward(), cam.up())) < 1e-5F);
	REQUIRE(std::abs(rt::dot(cam.right(), cam.up())) < 1e-5F);

	// Centre : proche de la visee (a 0.003 pres sur u pour 320 de large).
	const rt::Ray center = cam.rayForPixel(160, 120);
	REQUIRE(std::abs(directionLength(center.direction) - 1.0F) < 1e-5F);
	REQUIRE(rt::dot(center.direction, cam.forward()) > 0.999F);
	REQUIRE(center.origin.x == 0.0F);
	REQUIRE(center.origin.y == 2.0F);
	REQUIRE(center.origin.z == 6.0F);

	// Coins : 4 directions distinctes, toutes normalisees et devant.
	const rt::Ray topLeft = cam.rayForPixel(0, 0);
	const rt::Ray topRight = cam.rayForPixel(319, 0);
	const rt::Ray bottomLeft = cam.rayForPixel(0, 239);
	const rt::Ray bottomRight = cam.rayForPixel(319, 239);
	for (const rt::Ray* ray : {&topLeft, &topRight, &bottomLeft, &bottomRight}) {
		REQUIRE(std::abs(directionLength(ray->direction) - 1.0F) < 1e-4F);
		REQUIRE(rt::dot(ray->direction, cam.forward()) > 0.0F);
	}
	// Symetrie gauche/droite autour de la visee.
	REQUIRE(std::abs(rt::dot(topLeft.direction, cam.right()) +
	                 rt::dot(topRight.direction, cam.right())) < 0.01F);
	// Jitter : meme pixel, echantillon different -> rayon different (AA).
	const rt::Ray jittered = cam.rayForPixel(160, 120, rt::Vec2(0.25F, -0.25F));
	REQUIRE(rt::dot(jittered.direction, center.direction) < 1.0F);
	REQUIRE(rt::dot(jittered.direction, center.direction) > 0.999F);
}

TEST_CASE("camera : deplacer la camera change l'image (DoD T031)", "[camera]") {
	const rt::render::Camera camA = makeDefault();
	rt::render::Camera camB;
	REQUIRE(camB
	            .init(rt::Vec3(2.0F, 1.0F, 4.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 50.0F, 320, 240)
	            .isOk());

	// Meme pixel, oeil deplace : la direction change nettement.
	const rt::Ray rayA = camA.rayForPixel(160, 120);
	const rt::Ray rayB = camB.rayForPixel(160, 120);
	REQUIRE(rt::dot(rayA.direction, rayB.direction) < 0.999F);

	// Meme oeil, cible changee : la direction change aussi.
	rt::render::Camera camC;
	REQUIRE(camC
	            .init(rt::Vec3(0.0F, 2.0F, 6.0F), rt::Vec3(1.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 50.0F, 320, 240)
	            .isOk());
	const rt::Ray rayC = camC.rayForPixel(160, 120);
	REQUIRE(rt::dot(rayA.direction, rayC.direction) < 0.999F);

	// Fov change : les coins s'ecartent (grand angle = rayons plus obliques).
	rt::render::Camera wide;
	REQUIRE(wide
	            .init(rt::Vec3(0.0F, 2.0F, 6.0F), rt::Vec3(0.0F, 0.0F, 0.0F),
	                  rt::Vec3(0.0F, 1.0F, 0.0F), 90.0F, 320, 240)
	            .isOk());
	const rt::Ray cornerNarrow = camA.rayForPixel(0, 0);
	const rt::Ray cornerWide = wide.rayForPixel(0, 0);
	REQUIRE(rt::dot(cornerWide.direction, wide.forward()) <
	        rt::dot(cornerNarrow.direction, camA.forward()));
}
