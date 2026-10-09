// Interface microui (T075) — tests Catch2, sans ecran.
// DoD : couleur modifiee depuis l'UI -> image change ; table schema
// alimente l'UI (ajouter une directive l'affiche : `fieldNames` == `all()`).

#include <catch2/catch_amalgamated.hpp>

#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/schema/Directives.hpp"
#include "rt/ui/Panel.hpp"

TEST_CASE("panel : champs issus de la table schema R1 (T075)", "[panel]") {
	rt::ui::Panel panel;
	const auto names = panel.fieldNames();
	REQUIRE(names.size() == rt::schema::count());
	REQUIRE(names.size() > 50U);
	// Quelques chemins attendus (table reelle, pas de doublon).
	bool hasCamera = false;
	bool hasBackground = false;
	for (const auto& name : names) {
		if (name == "scene.camera.position") {
			hasCamera = true;
		}
		if (name == "scene.background.color") {
			hasBackground = true;
		}
	}
	REQUIRE(hasCamera);
	REQUIRE(hasBackground);
	// `all()` direct == `fieldNames()` : une ligne ajoutee au schema
	// apparait ici (pas de definition dupliquee).
	REQUIRE(names.size() == rt::schema::all().size());
}

TEST_CASE("panel : setters sans scene attaches sans crash (T075)", "[panel]") {
	rt::ui::Panel panel;
	REQUIRE_FALSE(panel.attached());
	REQUIRE_FALSE(panel.setBackground(rt::Vec3(1.0F, 0.0F, 0.0F)));
	REQUIRE_FALSE(panel.setFirstAlbedo(rt::Vec3(1.0F, 0.0F, 0.0F)));
	panel.frame();
	REQUIRE_FALSE(panel.takeLaunchRequest());
	REQUIRE_FALSE(panel.takeSaveRequest());
}

TEST_CASE("panel : couleur depuis l'UI change l'image (T075)", "[panel]") {
	rt::Result<rt::scene::Scene> parsed = rt::scene::parseFile("scenes/default.rt");
	REQUIRE(parsed.isOk());
	rt::scene::Scene& scene = parsed.value();
	rt::ui::Panel panel;
	panel.attach(&scene);
	REQUIRE(panel.attached());

	rt::render::Framebuffer before;
	rt::render::RenderParams params{.width = 64, .height = 48, .spp = 2, .maxDepth = 2, .seed = 7};
	REQUIRE(rt::render::render(scene, before, params).isOk());

	// Modifie l'albedo du 1er objet depuis l'UI (comme un slider/couleur).
	REQUIRE(panel.setFirstAlbedo(rt::Vec3(0.1F, 0.9F, 0.2F)));
	REQUIRE(scene.sceneDirty);
	rt::render::Framebuffer after;
	REQUIRE(rt::render::render(scene, after, params).isOk());

	// Au moins un pixel differe (couleur visible).
	bool differs = false;
	for (int y = 0; y < 48 && !differs; ++y) {
		for (int x = 0; x < 64 && !differs; ++x) {
			const rt::Vec3 a = before.accumAt(x, y);
			const rt::Vec3 b = after.accumAt(x, y);
			if (a.x != b.x || a.y != b.y || a.z != b.z) {
				differs = true;
			}
		}
	}
	REQUIRE(differs);
	panel.frame();
}
