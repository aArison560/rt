/**
 * @file SceneParser.hpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Scene configuration file parser
 *
 * Parses custom .rt scene configuration files and populates a Scene object
 * with objects, lights, and camera settings. Defines the file format grammar.
 *
 * FILE FORMAT GRAMMAR:
 *
 * scene.rt ::= (declaration)*
 *
 * declaration ::= ambient_decl
 *               | light_decl
 *               | object_decl
 *               | camera_decl
 *               | background_decl
 *
 * comment ::= '#' [^\n]*
 *
 * ambient_decl ::= 'A' WS number WS color
 * light_decl ::= 'L' WS pos WS color WS number
 * object_decl ::= sphere_decl | plane_decl | cylinder_decl | cone_decl
 * sphere_decl ::= 'sp' WS pos WS number WS color
 * plane_decl ::= 'pl' WS pos WS normal WS color
 * cylinder_decl ::= 'cy' WS pos WS normal WS number WS number WS color
 * cone_decl ::= 'co' WS pos WS normal WS number WS number WS color
 * camera_decl ::= 'c' WS pos WS direction WS number
 * background_decl ::= 'bg' WS color
 *
 * pos ::= number WS number WS number
 * normal ::= number WS number WS number
 * color ::= number WS number WS number
 * number ::= [-]?[0-9]+(.[0-9]+)?
 * WS ::= [ \t\r\n]+
 *
 * @relationships
 * - Uses: Scene, AObject subclasses, ALight subclasses, Material
 * - Used by: main (loads scenes from file)
 * - Parses: .rt files in scenes/ directory
 */

#pragma once

#include "Scene.hpp"
#include <filesystem>
#include <string>

class SceneParser
{
public:
    /**
     * @brief Default constructor
     */
    SceneParser();

    /**
     * @brief Destructor
     */
    ~SceneParser();

    /**
     * @brief Parse scene from file
     * @param filePath Path to .rt scene file
     * @param scene Output: scene populated with parsed content
     * @return true if parsing successful
     */
    [[nodiscard]] bool parseFile(const std::filesystem::path& filePath, Scene& scene);

    /**
     * @brief Parse scene from string
     * @param content Scene content as string
     * @param scene Output: scene populated from content
     * @return true if parsing successful
     */
    [[nodiscard]] bool parseString(const std::string& content, Scene& scene);

    /**
     * @brief Get last error message
     * @return Error description
     */
    [[nodiscard]] const std::string& getLastError() const;

    /**
     * @brief Get current line number during parsing
     * @return Line number (1-based)
     */
    [[nodiscard]] int getCurrentLine() const;

private:
    std::string lastError;    ///< Last error message
    int currentLine;          ///< Current line during parsing

    /**
     * @brief Main parsing loop
     * @param content Scene content as string
     * @param scene Scene to populate
     * @return true if successful
     */
    [[nodiscard]] bool parse(const std::string& content, Scene& scene);

    /**
     * @brief Report parsing error
     * @param message Error message
     */
    void reportError(const std::string& message);
};
