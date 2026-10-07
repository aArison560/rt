// Tests du parser `.rt` -> `Scene` (T023), Catch2.
// Couvre le Prompt et le DoD : imbrication, defauts, alias, tetes
// `object sphere "nom"` / `light point "cle"`, tableaux (`size`,
// `attenuation`), groupes recursifs, erreurs localisees
// `fichier:ligne:colonne`, garde-fous `limits` (`scene too large`),
// et les fixtures `tests/cases/` sans crash.

#include <catch2/catch_amalgamated.hpp>

#include <string>
#include <string_view>

#include "rt/scene/Parser.hpp"

namespace {

using rt::scene::ObjectType;

bool parseOk(std::string_view content, rt::scene::Scene& out, std::string_view name = "t.rt") {
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

bool hasLocation(const std::string& message, std::string_view filename) {
	if (message.rfind(std::string(filename) + ":", 0) != 0) {
		return false;
	}
	const std::size_t first = message.find(':');
	if (first == std::string::npos) {
		return false;
	}
	const std::size_t second = message.find(':', first + 1);
	if (second == std::string::npos) {
		return false;
	}
	return message.find(':', second + 1) != std::string::npos;
}

const char* kMinimal =
    "scene {\n"
    "  camera { position (0 1 4) target (0 0 0) fov 60 }\n"
    "  objects { object { type sphere center (0 0 0) radius 1 } }\n"
    "}\n";

} // namespace

TEST_CASE("parser : scene minimale et defauts", "[parser]") {
	rt::scene::Scene scene;
	REQUIRE(parseOk(kMinimal, scene));
	REQUIRE(scene.limits.width == 640);
	REQUIRE(scene.limits.height == 480);
	REQUIRE(scene.limits.samples == 4);
	REQUIRE(scene.camera.position.x == Catch::Approx(0.0F));
	REQUIRE(scene.camera.position.y == Catch::Approx(1.0F));
	REQUIRE(scene.camera.position.z == Catch::Approx(4.0F));
	REQUIRE(scene.camera.fov == Catch::Approx(60.0F));
	REQUIRE(scene.objects.size() == 1);
	REQUIRE(scene.objects[0].type == ObjectType::Sphere);
	REQUIRE(scene.objects[0].hasType);
	REQUIRE(scene.objects[0].radius == Catch::Approx(1.0F));
	// Defauts du materiau (table schema) : gris mat.
	REQUIRE(scene.objects[0].material.albedo.x == Catch::Approx(0.8F));
	REQUIRE(scene.objects[0].material.diffuse == Catch::Approx(0.7F));
	// Fond / ambiance par defaut.
	REQUIRE(scene.background.color.x == Catch::Approx(0.0F));
	REQUIRE(scene.ambient.intensity == Catch::Approx(1.0F));
}

TEST_CASE("parser : fixtures valides", "[parser]") {
	for (const char* path : {"tests/cases/valid/minimal.rt", "tests/cases/valid/group.rt"}) {
		INFO("fichier : " << path);
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile(path);
		INFO("message : " << (result.isError() ? result.status().message : std::string("ok")));
		REQUIRE(result.isOk());
	}
	{
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile("tests/cases/valid/minimal.rt");
		REQUIRE(result.isOk());
		const rt::scene::Scene& scene = result.value();
		REQUIRE(scene.objects.size() == 1);
		REQUIRE(scene.lights.size() == 1);
		REQUIRE(scene.objects[0].material.albedo.x == Catch::Approx(0.8F));
	}
	{
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile("tests/cases/valid/group.rt");
		REQUIRE(result.isOk());
		const rt::scene::Scene& scene = result.value();
		REQUIRE(scene.lights.size() == 2);
		REQUIRE(scene.groups.size() == 2);
		REQUIRE(scene.totalObjectCount() == 3);
		REQUIRE(scene.groups[0].objects.size() == 1);
		REQUIRE(scene.groups[1].objects.size() == 2);
	}
}

TEST_CASE("parser : tetes equivalentes aux proprietes", "[parser]") {
	rt::scene::Scene viaHead;
	rt::scene::Scene viaProps;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } objects { "
	                "object sphere \"boule\" { center (1 2 3) radius 2 } } }",
	                viaHead));
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } objects { "
	                "object { type sphere name \"boule\" center (1 2 3) radius 2 } } }",
	                viaProps));
	REQUIRE(viaHead.objects.size() == 1);
	REQUIRE(viaHead.objects[0].type == ObjectType::Sphere);
	REQUIRE(viaHead.objects[0].name == "boule");
	REQUIRE(viaHead.objects[0].center.x == Catch::Approx(1.0F));
	REQUIRE(viaProps.objects[0].name == viaHead.objects[0].name);

	// Lumiere : `light point "key"` == `light { type point name "key" }`.
	rt::scene::Scene lightHead;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } lights { "
	                "light point \"key\" { position (2 4 3) intensity 1.0 } } "
	                "objects { object { type sphere } } }",
	                lightHead));
	REQUIRE(lightHead.lights.size() == 1);
	REQUIRE(lightHead.lights[0].name == "key");
	REQUIRE(lightHead.lights[0].type == rt::scene::LightType::Point);

	// Le contenu gagne sur la tete (FORMAT §4).
	rt::scene::Scene conflict;
	REQUIRE(parseOk("scene { camera { position (0 1 4) target (0 0 0) } objects { "
	                "object sphere { type plane point (0 0 0) normal (0 1 0) } } }",
	                conflict));
	REQUIRE(conflict.objects[0].type == ObjectType::Plane);
}

TEST_CASE("parser : alias canoniques", "[parser]") {
	rt::scene::Scene scene;
	REQUIRE(parseOk("scene { camera { position (0 1 4) lookAt (0 0 0) } objects { "
	                "object { type sphere position (1 1 1) material { color (0.1 0.2 0.3) "
	                "reflect 0.5 } } } }",
	                scene));
	REQUIRE(scene.camera.target.x == Catch::Approx(0.0F));
	REQUIRE(scene.objects[0].center.x == Catch::Approx(1.0F));
	REQUIRE(scene.objects[0].material.albedo.z == Catch::Approx(0.3F));
	REQUIRE(scene.objects[0].material.reflectivity == Catch::Approx(0.5F));
}

TEST_CASE("parser : transform, material complet et tableaux", "[parser]") {
	rt::scene::Scene scene;
	REQUIRE(parseOk(
	    "scene { camera { position (0 2 6) target (0 0 0) fov 50 } lights { "
	    "light { type point position (3 5 2) color (1 1 1) intensity 0.8 "
	    "attenuation 1 0 0 range 10 } } objects { "
	    "object { type sphere center (0 0 0) radius 1 transform { translate (0 0.2 0) "
	    "scale 2 rotate axis y angle 30 } material { albedo (0.9 0.3 0.2) diffuse 0.7 "
	    "specular 0.8 shininess 32 reflectivity 0.2 transparency 0.5 ior 1.5 } } } }",
	    scene));
	REQUIRE(scene.lights[0].attenuation.x == Catch::Approx(1.0F));
	REQUIRE(scene.lights[0].range == Catch::Approx(10.0F));
	REQUIRE(scene.objects[0].transform.ops.size() == 3);
	REQUIRE(scene.objects[0].material.transparency == Catch::Approx(0.5F));
	REQUIRE(scene.objects[0].material.ior == Catch::Approx(1.5F));
}

TEST_CASE("parser : erreurs localisees fichier:ligne:colonne", "[parser]") {
	REQUIRE(hasLocation(parseFail("scene { camera {} objects { object { type sphere } } "
	                              "witdh 1 }"),
	                    "f.rt"));
	const std::string unknown = parseFail("scene { limits { witdh 640 } camera { position "
	                                      "(0 1 4) target (0 0 0) } objects { object { type "
	                                      "sphere } } }");
	REQUIRE(hasLocation(unknown, "f.rt"));
	REQUIRE(unknown.find("witdh") != std::string::npos);

	const std::string badNumber = parseFail("scene { limits { width abc } camera { position "
	                                        "(0 1 4) target (0 0 0) } objects { object { "
	                                        "type sphere } } }");
	REQUIRE(hasLocation(badNumber, "f.rt"));

	// Vecteur a 2 composantes : erreur (FORMAT §9).
	const std::string badVec = parseFail("scene { camera { position (0 1) target (0 0 0) } "
	                                     "objects { object { type sphere } } }");
	REQUIRE(hasLocation(badVec, "f.rt"));

	// Bloc requis manquant.
	REQUIRE(hasLocation(parseFail("scene { camera { position (0 1 4) target (0 0 0) } }"),
	                    "f.rt"));
	REQUIRE(hasLocation(parseFail("scene { objects { object { type sphere } } }"), "f.rt"));

	// Doublon d'un bloc 0/1.
	REQUIRE(hasLocation(
	    parseFail("scene { camera { position (0 1 4) target (0 0 0) } camera { position "
	              "(0 1 4) target (0 0 0) } objects { object { type sphere } } }"),
	    "f.rt"));

	// Legacy sans `scene` : message de conversion, pas de crash.
	const std::string legacy = parseFail("bg 0 0 0\n");
	REQUIRE(hasLocation(legacy, "f.rt"));
	REQUIRE(legacy.find("legacy") != std::string::npos);
}

TEST_CASE("parser : fixtures invalides du parser", "[parser]") {
	for (const char* path :
	     {"tests/cases/invalid/bad_number.rt", "tests/cases/invalid/unknown_directive.rt"}) {
		INFO("fichier : " << path);
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile(path);
		REQUIRE(result.isError());
		REQUIRE(result.status().message.rfind(path, 0) == 0);
		REQUIRE(hasLocation(result.status().message, path));
	}
	// Inexistant / repertoire : IoError propre.
	{
		rt::Result<rt::scene::Scene> result =
		    rt::scene::parseFile("tests/cases/invalid/does_not_exist.rt");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
	}
	{
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile("tests/cases");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
	}
}

TEST_CASE("parser : garde-fous limits (scene too large)", "[parser]") {
	// 3 objets pour une limite de 2 -> erreur propre avec le format T024.
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
}

TEST_CASE("parser : scene imbriquee 3 niveaux (File ++)", "[parser]") {
	rt::scene::Scene scene;
	REQUIRE(parseOk("scene \"vitrine\" { camera { position (0 2 6) target (0 0 0) } "
	                "objects { group \"sol\" { object plane \"sol\" { point (0 -1 0) normal "
	                "(0 1 0) material { albedo (0.5 0.5 0.5) } } } group \"vitrine\" { "
	                "group \"sous\" { object sphere \"b\" { center (0 0 0) radius 1 } } } } }",
	                scene));
	REQUIRE(scene.name == "vitrine");
	REQUIRE(scene.groups.size() == 2);
	REQUIRE(scene.totalObjectCount() == 2);
	REQUIRE(scene.groups[0].objects[0].type == ObjectType::Plane);
	REQUIRE(scene.groups[1].children.size() == 1);
	REQUIRE(scene.groups[1].children[0]->objects[0].name == "b");
}
