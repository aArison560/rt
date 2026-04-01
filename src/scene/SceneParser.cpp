/**
 * @file SceneParser.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Scene configuration parser implementation
 */

#include "scene/SceneParser.hpp"
#include "geometry/Sphere.hpp"
#include "geometry/Plane.hpp"
#include "geometry/Cylinder.hpp"
#include "geometry/Cone.hpp"
#include "lighting/PointLight.hpp"
#include "lighting/AmbientLight.hpp"
#include <fstream>
#include <sstream>

SceneParser::SceneParser() : currentLine(1), currentPos(0) {}

SceneParser::~SceneParser() {}

bool SceneParser::parseFile(const std::string& filePath, Scene& scene)
{
    // TODO: Load file content and parse
    std::ifstream file(filePath);
    if (!file) {
        reportError("Cannot open file: " + filePath);
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str(), scene);
}

bool SceneParser::parseString(const std::string& content, Scene& scene)
{
    // TODO: Parse scene from string content
    this->content = content;
    currentPos = 0;
    currentLine = 1;
    return parse(scene);
}

const std::string& SceneParser::getLastError() const { return lastError; }
int SceneParser::getCurrentLine() const { return currentLine; }

bool SceneParser::parse(Scene& scene)
{
    // TODO: Main parse loop calling specific parsers
    return false;
}

void SceneParser::skipWhitespace()
{
    // TODO: Skip spaces, tabs, newlines and comments
}

bool SceneParser::isAtEnd() const
{
    return currentPos >= content.length();
}

char SceneParser::peekChar() const
{
    if (isAtEnd()) return '\0';
    return content[currentPos];
}

char SceneParser::consumeChar()
{
    if (isAtEnd()) return '\0';
    char c = content[currentPos++];
    if (c == '\n') currentLine++;
    return c;
}

bool SceneParser::parseNumber(double& value)
{
    // TODO: Parse floating point number
    return false;
}

bool SceneParser::parseVec3(Vec3& point)
{
    // TODO: Parse three numbers as Vec3
    double x, y, z;
    if (!parseNumber(x) || !parseNumber(y) || !parseNumber(z)) {
        return false;
    }
    point = Vec3(x, y, z);
    return true;
}

bool SceneParser::parseAmbient(Scene& scene)
{
    // TODO: Parse ambient light directive
    // Format: A intensity color
    return true;
}

bool SceneParser::parseLight(Scene& scene)
{
    // TODO: Parse point light directive
    // Format: L position color intensity
    return true;
}

bool SceneParser::parseSphere(Scene& scene)
{
    // TODO: Parse sphere directive
    // Format: sp center radius color
    return true;
}

bool SceneParser::parsePlane(Scene& scene)
{
    // TODO: Parse plane directive
    // Format: pl point normal color
    return true;
}

bool SceneParser::parseCylinder(Scene& scene)
{
    // TODO: Parse cylinder directive
    // Format: cy center normal radius height color
    return true;
}

bool SceneParser::parseCone(Scene& scene)
{
    // TODO: Parse cone directive
    // Format: co apex normal half-angle height color
    return true;
}

bool SceneParser::parseCamera(Scene& scene)
{
    // TODO: Parse camera directive
    // Format: c position direction fov
    return true;
}

bool SceneParser::parseBackground(Scene& scene)
{
    // TODO: Parse background color directive
    // Format: bg color
    return true;
}

void SceneParser::reportError(const std::string& message)
{
    lastError = "Line " + std::to_string(currentLine) + ": " + message;
}

bool SceneParser::matchIdentifier(const std::string& identifier)
{
    // TODO: Check if current position matches identifier
    return false;
}
