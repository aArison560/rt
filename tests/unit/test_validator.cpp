// Tests de la passe de validation (T024), Catch2.
// Couvre le Prompt et le DoD : bornes du schema (min/max, enums, couleurs
// 0-1), `limits { max_objects max_lights max_texture_bytes }` -> erreur
// propre `scene too large: ...`, code retour != 0, pas d'allocation surprise,
// et validation croisee (ex. : `ior > 1` si transparence, camera degeneree,
// lumieres et plans incomplets, slice incoherente).

#include <catch2/catch_amalgamated.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "rt/scene/Parser.hpp"
#include "rt/scene/Validator.hpp"

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

std::string parseFail(std::string_view content, std::string_view name = "f.rt") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseContent(content, name);
	REQUIRE(result.isError());
	REQUIRE_FALSE(result.status().message.empty());
	return result.status().message;
}

rt::scene::Scene makeMinimal() {
	rt::scene::Scene scene;
	const char* content =
	    "scene {\n"
	    "  camera { position (0 1 4) target (0 0 0) fov 60 }\n"
	    "  objects { object { type sphere center (0 0 0) radius 1 } }\n"
	    "}\n";
	REQUIRE(parseOk(content, scene));
	return scene;
}

} // namespace

TEST_CASE("validator : limites max_objects et max_lights", "[validator]") {
	const std::string tooMany = parseFail(
	    "scene { limits { max_objects 2 } camera { position (0 1 4) target (0 0 0) } "
	    "objects { object { type sphere } object { type plane point (0 0 0) normal (0 1 0) } "
	    "object { type sphere } } }");
	REQUIRE(tooMany.find("scene too large") != std::string::npos);
	REQUIRE(tooMany.find("3 objects, limit 2") != std::string::npos);

	const std::string tooManyLights = parseFail(
	    "scene { limits { max_lights 1 } camera { position (0 1 4) target (0 0 0) } "
	    "lights { light { position (0 0 0) } light { position (1 1 1) } } "
	    "objects { object { type sphere } } }");
	REQUIRE(tooManyLights.find("scene too large") != std::string::npos);
	REQUIRE(tooManyLights.find("2 lights, limit 1") != std::string::npos);

	// Cas nominal : dans les limites, la validation passe.
	rt::scene::Scene scene = makeMinimal();
	REQUIRE(rt::scene::validate(scene).isOk());

	// Scenes d'exemple a la limite exacte : OK.
	rt::scene::Scene exact;
	REQUIRE(parseOk("scene { limits { max_objects 1 } camera { position "
	                "(0 1 4) target (0 0 0) } objects { object { type sphere } } }",
	                exact));
	REQUIRE(rt::scene::validate(exact).isOk());
}

TEST_CASE("validator : budget max_texture_bytes sans chargement", "[validator]") {
	const std::string dir = "/tmp/rt_t024_tex";
	std::filesystem::create_directories(dir);
	const std::string first = dir + "/a.bin";
	const std::string second = dir + "/b.bin";
	{
		std::ofstream out(first, std::ios::binary);
		REQUIRE(out.good());
		const std::string payload(2048, 'A');
		out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
	}
	{
		std::ofstream out(second, std::ios::binary);
		REQUIRE(out.good());
		const std::string payload(2048, 'B');
		out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
	}
	REQUIRE(std::filesystem::file_size(first) == 2048U);
	REQUIRE(std::filesystem::file_size(second) == 2048U);

	std::string content =
	    "scene { limits { max_texture_bytes 5000 } camera { position (0 1 4) "
	    "target (0 0 0) } objects { object { type sphere material { texture \"" +
	    first + "\" { scale 1 1 } } } } }";
	rt::scene::Scene fits;
	REQUIRE(parseOk(content, fits));
	REQUIRE(rt::scene::estimateTextureBytes(fits) == 2048U);
	REQUIRE(rt::scene::validate(fits).isOk());

	// Deux textures distinctes (2 x 2048 = 4096) sous la limite 5000 : OK,
	// mais le meme contenu avec une limite de 1000 depasse sans rien charger.
	std::string both =
	    "scene { limits { max_texture_bytes 5000 } camera { position (0 1 4) "
	    "target (0 0 0) } objects { object { type sphere material { texture \"" +
	    first +
	    "\" { scale 1 1 } } } object { type sphere material { texture \"" + second +
	    "\" { scale 1 1 } } } } }";
	rt::scene::Scene bothScene;
	REQUIRE(parseOk(both, bothScene));
	REQUIRE(rt::scene::estimateTextureBytes(bothScene) == 4096U);
	REQUIRE(rt::scene::validate(bothScene).isOk());

	std::string tight =
	    "scene { limits { max_texture_bytes 1000 } camera { position (0 1 4) "
	    "target (0 0 0) } objects { object { type sphere material { texture \"" +
	    first + "\" { scale 1 1 } } } } }";
	const std::string tooBig = parseFail(tight);
	REQUIRE(tooBig.find("scene too large") != std::string::npos);
	REQUIRE(tooBig.find("texture bytes") != std::string::npos);
	REQUIRE(tooBig.find("limit 1000") != std::string::npos);

	// Fichier absent : contribue 0 (l'existence sera controlee en T102),
	// donc pas d'erreur de budget ici.
	rt::scene::Scene missing;
	REQUIRE(parseOk("scene { limits { max_texture_bytes 1000 } camera { position "
	                "(0 1 4) target (0 0 0) } objects { object { type sphere "
	                "material { texture \"/tmp/rt_t024_tex/absent.png\" { scale 1 1 } } } } }",
	                missing));
	REQUIRE(rt::scene::estimateTextureBytes(missing) == 0U);
	REQUIRE(rt::scene::validate(missing).isOk());

	std::filesystem::remove(first);
	std::filesystem::remove(second);
}

TEST_CASE("validator : ior > 1 si transparence", "[validator]") {
	// Transparence sans indice : ior par defaut 1.5 > 1, donc OK.
	rt::scene::Scene ok;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } objects { "
	                "object { type sphere material { transparency 0.5 ior 1.5 } } } }",
	                ok));
	REQUIRE(rt::scene::validate(ok).isOk());

	// ior = 1 avec transparence > 0 : incoherent (pas de deviation mais
	// objet translucide) -> erreur croisee explicite.
	const std::string bad = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { "
	    "object { type sphere material { transparency 0.5 ior 1.0 } } } }");
	REQUIRE(bad.find("ior") != std::string::npos);
	REQUIRE(bad.find("transparency") != std::string::npos);

	// Opaque avec ior = 1 : OK (pas de refraction attendue).
	rt::scene::Scene opaque;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } objects { "
	                "object { type sphere material { transparency 0.0 ior 1.0 } } } }",
	                opaque));
	REQUIRE(rt::scene::validate(opaque).isOk());

	// Meme regle via l'API directe sur une scene programmee.
	rt::scene::Scene direct = makeMinimal();
	direct.objects[0].material.transparency = 0.4F;
	direct.objects[0].material.ior = 1.0F;
	REQUIRE(rt::scene::validate(direct).isError());
}

TEST_CASE("validator : camera degeneree", "[validator]") {
	// Cible confondue avec l'oeil : direction nulle.
	const std::string same = parseFail(
	    "scene { camera { position (1 2 3) target (1 2 3) } objects { "
	    "object { type sphere } } }");
	REQUIRE(same.find("target equals position") != std::string::npos);

	// Verticale nulle.
	const std::string upZero = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) up (0 0 0) } objects { "
	    "object { type sphere } } }");
	REQUIRE(upZero.find("up is zero") != std::string::npos);

	// Verticale colineaire a la visee.
	const std::string collinear = parseFail(
	    "scene { camera { position (0 4 0) target (0 0 0) up (0 1 0) } objects { "
	    "object { type sphere } } }");
	REQUIRE(collinear.find("collinear") != std::string::npos);
}

TEST_CASE("validator : lumieres et objets incomplets", "[validator]") {
	// Ponctuelle sans position.
	const std::string noPos = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } lights { light { type "
	    "point color (1 1 1) } } objects { object { type sphere } } }");
	REQUIRE(noPos.find("requires position") != std::string::npos);

	// Directionnelle sans direction.
	const std::string noDir = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } lights { light { type "
	    "directional color (1 1 1) } } objects { object { type sphere } } }");
	REQUIRE(noDir.find("requires direction") != std::string::npos);

	// Spot sans cible.
	const std::string noTarget = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } lights { light { type "
	    "spot position (1 2 3) } } objects { object { type sphere } } }");
	REQUIRE(noTarget.find("requires target") != std::string::npos);

	// Plan sans point ni normale.
	const std::string noPoint = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { object { type "
	    "plane } } }");
	REQUIRE(noPoint.find("requires point") != std::string::npos);

	const std::string noNormal = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { object { type "
	    "plane point (0 0 0) } } }");
	REQUIRE(noNormal.find("requires normal") != std::string::npos);

	// Cylindre d'axe nul.
	const std::string axisZero = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { object { type "
	    "cylinder center (0 0 0) radius 1 axis (0 0 0) } } }");
	REQUIRE(axisZero.find("axis is zero") != std::string::npos);

	// Tranche incoherente : min > max.
	const std::string sliceBad = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { object { type "
	    "sphere slice { axis y min 2 max -2 } } } }");
	REQUIRE(sliceBad.find("slice min > max") != std::string::npos);
}

TEST_CASE("validator : bornes du schema rejouees", "[validator]") {
	// Couleur hors 0-1.
	const std::string color = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } objects { object { type "
	    "sphere material { albedo (1.5 0 0) } } } }");
	REQUIRE(color.find("out of range") != std::string::npos);

	// FOV hors 1-179.
	const std::string fov = parseFail(
	    "scene { camera { position (0 1 4) target (0 0 0) fov 200 } objects { "
	    "object { type sphere } } }");
	REQUIRE(fov.find("out of range") != std::string::npos);

	// Programmatique : une couleur modifiee apres parse est rejete par validate.
	rt::scene::Scene direct = makeMinimal();
	REQUIRE(rt::scene::validate(direct).isOk());
	direct.background.color.x = 2.0F;
	REQUIRE(rt::scene::validate(direct).isError());
}
