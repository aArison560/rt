// Interaction live (T109, *Environment 3*), Catch2.
// Camera/objet/couleurs/textures sans relancer : chaque edition leve R5
// (apercu 1 spp puis affinage via `Interactive`, T076). `scenes/live.rt`
// + protocole `docs/preuves/live.md` (5 gestes).

#include <catch2/catch_amalgamated.hpp>

#include "rt/app/Interactive.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/ui/Panel.hpp"

TEST_CASE("live : editions sans relancer levent R5 (T109)", "[live][t109]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/live.rt");
	INFO((parsed.isError() ? parsed.status().message : std::string("ok")));
	REQUIRE(parsed.isOk());
	rt::scene::Scene& scene = parsed.value();
	rt::ui::Panel panel;
	panel.attach(&scene);
	scene.markClean();
	const std::uint64_t versionBefore = scene.objectVersion;

	// 1. Camera (FOV) : fanions seuls, pas de version++ (BVH intacte).
	REQUIRE(panel.setCameraFov(70.0F));
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.objectVersion == versionBefore);

	// 2. Couleur (albedo) : touche les objets (re-shading).
	scene.markClean();
	REQUIRE(panel.setFirstAlbedo(rt::Vec3(0.2F, 0.7F, 0.3F)));
	REQUIRE(scene.sceneDirty);

	// 3. Position d'objet : deplace + invalide la BVH (version++).
	scene.markClean();
	const std::uint64_t versionColor = scene.objectVersion;
	REQUIRE(panel.setFirstObjectX(2.0F));
	REQUIRE(scene.objects[0].center.x == Catch::Approx(2.0F));
	REQUIRE(scene.objectVersion == versionColor + 1);

	// 4. Texture (scale) : visible, BVH intacte (pas de version++).
	scene.markClean();
	const std::uint64_t versionMove = scene.objectVersion;
	REQUIRE(panel.setFirstTextureScale(4.0F));
	REQUIRE(scene.objects[0].material.texture.scale.x == Catch::Approx(4.0F));
	REQUIRE(scene.sceneDirty);
	REQUIRE(scene.objectVersion == versionMove);

	// 5. La machine preview->full absorbe chaque edition sans redemarrage.
	rt::app::Interactive ctl;
	ctl.onEdited();
	REQUIRE(ctl.needsPreview());
	ctl.consumePreview();
	ctl.onPreviewDone();
	REQUIRE(ctl.needsFull());
	ctl.consumeFull();
	REQUIRE(ctl.counts().fulls == 1);
	ctl.onFullDone();
	REQUIRE_FALSE(ctl.needsPreview());
	REQUIRE_FALSE(ctl.needsFull());
}
