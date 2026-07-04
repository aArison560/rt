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
#include "lighting/DirectionalLight.hpp"
#include <fstream>
#include <filesystem>
#include <format>
#include <sstream>
#include <vector>
#include <cctype>
#include <memory>

namespace {
static std::string trim(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

static bool toDoubles(const std::vector<std::string>& tokens, size_t startIndex, std::vector<double>& out)
{
    out.clear();
    for (size_t i = startIndex; i < tokens.size(); ++i) {
        try {
            out.push_back(std::stod(tokens[i]));
        } catch (...) {
            return false;
        }
    }
    return true;
}
}

SceneParser::SceneParser() : currentLine(1) {}

SceneParser::~SceneParser() {}

bool SceneParser::parseFile(const std::filesystem::path& filePath, Scene& scene)
{
    std::ifstream file(filePath);
    if (!file) {
        reportError("Cannot open file: " + filePath.string());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str(), scene);
}

bool SceneParser::parseString(const std::string& content, Scene& scene)
{
    lastError.clear();
    currentLine = 1;
    scene.clear();
    return parse(content, scene);
}

const std::string& SceneParser::getLastError() const { return lastError; }
int SceneParser::getCurrentLine() const { return currentLine; }

bool SceneParser::parse(const std::string& content, Scene& scene)
{
    std::istringstream input(content);
    std::string line;
    std::shared_ptr<AObject> lastObject;
    currentLine = 0;

    while (std::getline(input, line)) {
        ++currentLine;

        const size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        std::istringstream ls(line);
        std::vector<std::string> tokens;
        std::string tok;
        while (ls >> tok) {
            tokens.push_back(tok);
        }
        if (tokens.empty()) {
            continue;
        }

        const std::string& id = tokens[0];
        std::vector<double> v;
        if (!toDoubles(tokens, 1, v)) {
            reportError("Invalid numeric value in line: " + line);
            return false;
        }

        if (id == "bg") {
            if (v.size() != 3) {
                reportError("bg expects 3 values");
                return false;
            }
            scene.setBackgroundColor(Vec3(v[0], v[1], v[2]));
            continue;
        }

        if (id == "A") {
            if (v.size() == 1) {
                scene.addLight(std::make_shared<AmbientLight>(Vec3(1.0, 1.0, 1.0), v[0]));
            } else if (v.size() == 3) {
                scene.addLight(std::make_shared<AmbientLight>(Vec3(v[0], v[1], v[2]), 1.0));
            } else {
                reportError("A expects 1 or 3 values");
                return false;
            }
            continue;
        }

        if (id == "L") {
            if (v.size() != 7) {
                reportError("L expects 7 values: px py pz r g b intensity");
                return false;
            }
            scene.addLight(std::make_shared<PointLight>(
                Vec3(v[0], v[1], v[2]),
                Vec3(v[3], v[4], v[5]),
                v[6]
            ));
            continue;
        }

        if (id == "directional") {
            if (v.size() != 7) {
                reportError("directional expects 7 values: dx dy dz r g b intensity");
                return false;
            }
            scene.addLight(std::make_shared<DirectionalLight>(
                Vec3(v[0], v[1], v[2]),
                Vec3(v[3], v[4], v[5]),
                v[6]
            ));
            continue;
        }

        if (id == "c") {
            if (v.size() == 7) {
                Camera cam(
                    Vec3(v[0], v[1], v[2]),
                    Vec3(v[3], v[4], v[5]),
                    Vec3(0.0, 1.0, 0.0),
                    v[6]
                );
                scene.setCamera(cam);
            } else if (v.size() == 10) {
                const Vec3 pos(v[0], v[1], v[2]);
                const Vec3 lookAt(v[3], v[4], v[5]);
                Vec3 dir = lookAt - pos;
                try {
                    dir = dir.normalized();
                } catch (...) {
                    dir = Vec3(0.0, 0.0, -1.0);
                }
                Camera cam(
                    pos,
                    dir,
                    Vec3(v[6], v[7], v[8]),
                    v[9]
                );
                scene.setCamera(cam);
            } else {
                reportError("c expects 7 or 10 values");
                return false;
            }
            continue;
        }

        if (id == "sp") {
            if (v.size() != 4) {
                reportError("sp expects 4 values: cx cy cz radius");
                return false;
            }
            lastObject = std::make_shared<Sphere>(Vec3(v[0], v[1], v[2]), v[3]);
            scene.addObject(lastObject);
            continue;
        }

        if (id == "pl") {
            if (v.size() != 6) {
                reportError("pl expects 6 values: px py pz nx ny nz");
                return false;
            }
            lastObject = std::make_shared<Plane>(Vec3(v[0], v[1], v[2]), Vec3(v[3], v[4], v[5]));
            scene.addObject(lastObject);
            continue;
        }

        if (id == "cy") {
            if (v.size() != 8) {
                reportError("cy expects 8 values: cx cy cz ax ay az radius height");
                return false;
            }
            lastObject = std::make_shared<Cylinder>(
                Vec3(v[0], v[1], v[2]),
                Vec3(v[3], v[4], v[5]),
                v[6],
                v[7]
            );
            scene.addObject(lastObject);
            continue;
        }

        if (id == "co") {
            if (v.size() != 8) {
                reportError("co expects 8 values: ax ay az dx dy dz halfAngle height");
                return false;
            }
            lastObject = std::make_shared<Cone>(
                Vec3(v[0], v[1], v[2]),
                Vec3(v[3], v[4], v[5]),
                v[6],
                v[7]
            );
            scene.addObject(lastObject);
            continue;
        }

        if (id == "material") {
            if (!lastObject) {
                reportError("material found before any object");
                return false;
            }
            if (v.size() != 8 && v.size() != 9) {
                reportError("material expects 8 or 9 values: r g b amb diff spec shininess reflect [roughness]");
                return false;
            }
            std::shared_ptr<Material> mat = std::make_shared<Material>(
                Vec3(v[0], v[1], v[2]),
                v[3],
                v[4],
                v[5],
                v[6]
            );
            mat->setReflectivity(v[7]);
            if (v.size() == 9) {
                mat->setRoughness(v[8]);
            }
            lastObject->setMaterial(mat);
            continue;
        }

        reportError("Unknown directive: " + id);
        return false;
    }

    return true;
}

void SceneParser::reportError(const std::string& message)
{
    lastError = std::format("Line {}: {}", currentLine, message);
}
