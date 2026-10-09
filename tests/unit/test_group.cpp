// Elements composes reutilisables (T101, *Composed elements*), Catch2.
// `scenes/opt_group.rt` definit la composition "verre" (cone + cylindre +
// sphere) une fois comme patron et l'instancie 2 fois (`verre_gauche`,
// `verre_droite`) a des positions differentes via le `transform` parent.
// Le test affirme : meme definition (memes types dans le meme ordre),
// 2 instances a 2 endroits distincts (translates opposes), et partage
// memoire (un `shared_ptr<Group>` enfant pointe par 2 parents = 1 seule
// definition physique, `totalObjectCount` compte les 2 instances).

#include <catch2/catch_amalgamated.hpp>

#include <memory>

#include "rt/scene/Parser.hpp"

TEST_CASE("group : meme definition instanciee 2 fois a 2 endroits (T101)", "[group][t101]") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile("scenes/opt_group.rt");
	INFO((result.isError() ? result.status().message : std::string("ok")));
	REQUIRE(result.isOk());
	const rt::scene::Scene& scene = result.value();
	REQUIRE(scene.groups.size() == 2);
	REQUIRE(scene.groups[0].name == "verre_gauche");
	REQUIRE(scene.groups[1].name == "verre_droite");
	// Meme definition : 3 objets, memes types dans le meme ordre.
	for (const rt::scene::Group& group : scene.groups) {
		REQUIRE(group.objects.size() == 3);
		REQUIRE(group.objects[0].type == rt::scene::ObjectType::Cone);
		REQUIRE(group.objects[1].type == rt::scene::ObjectType::Cylinder);
		REQUIRE(group.objects[2].type == rt::scene::ObjectType::Sphere);
	}
	// 2 endroits distincts : translates opposes sur X.
	REQUIRE(scene.groups[0].transform.ops.size() == 1);
	REQUIRE(scene.groups[1].transform.ops.size() == 1);
	const float leftX = scene.groups[0].transform.ops[0].translate.x;
	const float rightX = scene.groups[1].transform.ops[0].translate.x;
	REQUIRE(leftX == Catch::Approx(-2.2F));
	REQUIRE(rightX == Catch::Approx(2.2F));
	REQUIRE(leftX != rightX);
	// 1 sol + 2x3 objets de groupes = 7.
	REQUIRE(scene.totalObjectCount() == 7);
}

TEST_CASE("group : partage memoire via shared_ptr (T101)", "[group][t101]") {
	// Une definition allouee une fois, pointee par 2 parents : pas de
	// duplication en memoire (meme adresse), mais 2 instances comptees.
	auto definition = std::make_shared<rt::scene::Group>();
	definition->name = "patron";
	rt::scene::Object sphere;
	sphere.type = rt::scene::ObjectType::Sphere;
	sphere.hasType = true;
	definition->objects.push_back(sphere);

	rt::scene::Scene scene;
	scene.init();
	rt::scene::Group left;
	left.name = "gauche";
	left.children.push_back(definition);
	rt::scene::Group right;
	right.name = "droite";
	right.children.push_back(definition);
	scene.groups.push_back(std::move(left));
	scene.groups.push_back(std::move(right));

	REQUIRE(scene.groups[0].children.size() == 1);
	REQUIRE(scene.groups[1].children.size() == 1);
	// Partage physique : les 2 parents pointent la meme definition.
	REQUIRE(scene.groups[0].children[0].get() == scene.groups[1].children[0].get());
	REQUIRE(scene.groups[0].children[0].get() == definition.get());
	// Mais 2 instances logiques comptees.
	REQUIRE(scene.totalObjectCount() == 2);
}
