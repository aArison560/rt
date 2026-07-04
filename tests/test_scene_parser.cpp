#include "catch_amalgamated.hpp"
using Catch::Approx;
#include "scene/SceneParser.hpp"
#include "scene/Scene.hpp"
#include "core/Vec3.hpp"

#include <filesystem>
#include <fstream>

TEST_CASE("SceneParser parses background", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "bg 0.2 0.3 0.4\n";
    REQUIRE(parser.parseString(content, scene));
    CHECK(scene.getBackgroundColor() == Vec3(0.2, 0.3, 0.4));
}

TEST_CASE("SceneParser parses ambient light", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "A 0.5 0.6 0.7\n";
    REQUIRE(parser.parseString(content, scene));
    CHECK(scene.getLightCount() >= 1);
    CHECK(scene.getAmbientMultiplier() > 0);
}

TEST_CASE("SceneParser parses camera with 7 values (no up)", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "c 0.0 2.5 10.0 0.0 1.0 -2.0 40.0\n";
    REQUIRE(parser.parseString(content, scene));
    const Camera& cam = scene.getCamera();
    CHECK(cam.getPosition() == Vec3(0.0, 2.5, 10.0));
}

TEST_CASE("SceneParser parses camera with 10 values (explicit up)", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "c 0.0 2.5 10.0 0.0 1.0 -1.2 0.0 1.0 0.0 45.0\n";
    REQUIRE(parser.parseString(content, scene));
    const Camera& cam = scene.getCamera();
    CHECK(cam.getPosition() == Vec3(0.0, 2.5, 10.0));
}

TEST_CASE("SceneParser parses sphere", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "sp 1.0 2.0 3.0 4.0\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getObjectCount() == 1);
    CHECK(scene.getObject(0)->getType() == std::string("Sphere"));
}

TEST_CASE("SceneParser parses plane", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "pl 0.0 -2.0 0.0 0.0 1.0 0.0\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getObjectCount() == 1);
    CHECK(scene.getObject(0)->getType() == std::string("Plane"));
}

TEST_CASE("SceneParser parses cylinder", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "cy 0.0 -1.5 -4.0 0.0 1.0 0.0 0.5 1.5\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getObjectCount() == 1);
    CHECK(scene.getObject(0)->getType() == std::string("Cylinder"));
}

TEST_CASE("SceneParser parses cone", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "co 2.5 -1.8 -6.0 0.0 1.0 0.0 25.0 1.8\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getObjectCount() == 1);
    CHECK(scene.getObject(0)->getType() == std::string("Cone"));
}

TEST_CASE("SceneParser parses postfix material", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "sp 0.0 0.5 0.0 1.0\nmaterial 0.9 0.2 0.1 0.8 0.6 0.1 32.0 0.1\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getObjectCount() == 1);
    auto mat = scene.getObject(0)->getMaterial();
    REQUIRE(mat != nullptr);
    CHECK(mat->getColor() == Vec3(0.9, 0.2, 0.1));
    CHECK(mat->getAmbient() == Approx(0.8));
    CHECK(mat->getDiffuse() == Approx(0.6));
    CHECK(mat->getShininess() == Approx(32.0));
    CHECK(mat->getReflectivity() == Approx(0.1));
}

TEST_CASE("SceneParser parses point light", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "L 8.0 8.0 8.0 1.0 1.0 0.9 1.0\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getLightCount() == 1);
}

TEST_CASE("SceneParser parses directional light", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "directional 1.0 1.0 0.5 1.0 1.0 0.8 0.95\n";
    REQUIRE(parser.parseString(content, scene));
    REQUIRE(scene.getLightCount() == 1);
}

TEST_CASE("SceneParser parses complete scene", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = R"(
bg 0.3 0.3 0.35
A 0.4 0.4 0.4
L 8.0 8.0 8.0 1.0 1.0 0.9 1.0
c 0.0 2.5 10.0 0.0 1.0 -1.2 0.0 1.0 0.0 45.0
sp 0.0 0.5 0.0 1.0
pl 0.0 -2.0 0.0 0.0 1.0 0.0
material 0.9 0.2 0.1 0.8 0.6 0.1 32.0 0.1
)";
    REQUIRE(parser.parseString(content, scene));
    CHECK(scene.getLightCount() >= 1);
    CHECK(scene.getObjectCount() >= 1);
}

TEST_CASE("SceneParser handles comments", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "# this is a comment\nbg 0.1 0.2 0.3\n# another comment\n";
    REQUIRE(parser.parseString(content, scene));
    CHECK(scene.getBackgroundColor() == Vec3(0.1, 0.2, 0.3));
}

TEST_CASE("SceneParser returns error for unknown directive", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "unknown 1 2 3\n";
    CHECK_FALSE(parser.parseString(content, scene));
    CHECK_FALSE(parser.getLastError().empty());
}

TEST_CASE("SceneParser returns error for invalid syntax", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "sp a b c\n";
    CHECK_FALSE(parser.parseString(content, scene));
    CHECK_FALSE(parser.getLastError().empty());
}

TEST_CASE("SceneParser parses from actual scene file", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    bool ok = parser.parseFile("scenes/simple_spheres.rt", scene);
    REQUIRE(ok);
    CHECK(scene.getObjectCount() > 0);
    CHECK(scene.getLightCount() > 0);
}

TEST_CASE("SceneParser handles empty content", "[sceneparser]")
{
    SceneParser parser;
    Scene scene;
    std::string content = "";
    CHECK(parser.parseString(content, scene));
    CHECK(scene.getObjectCount() == 0);
    CHECK(scene.getLightCount() == 0);
}
