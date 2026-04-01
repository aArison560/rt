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
#include <string>
#include <fstream>
#include <sstream>

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
    bool parseFile(const std::string& filePath, Scene& scene);

    /**
     * @brief Parse scene from string
     * @param content Scene content as string
     * @param scene Output: scene populated from content
     * @return true if parsing successful
     */
    bool parseString(const std::string& content, Scene& scene);

    /**
     * @brief Get last error message
     * @return Error description
     */
    const std::string& getLastError() const;

    /**
     * @brief Get current line number during parsing
     * @return Line number (1-based)
     */
    int getCurrentLine() const;

private:
    std::string lastError;    ///< Last error message
    int currentLine;          ///< Current line during parsing
    size_t currentPos;        ///< Current position in parsed content
    std::string content;      ///< Current content being parsed

    /**
     * @brief Main parsing loop
     * @param scene Scene to populate
     * @return true if successful
     */
    bool parse(Scene& scene);

    /**
     * @brief Skip whitespace and comments
     */
    void skipWhitespace();

    /**
     * @brief Check if at end of input
     * @return true if no more input
     */
    bool isAtEnd() const;

    /**
     * @brief Peek next character
     * @return Next character, or '\0' if at end
     */
    char peekChar() const;

    /**
     * @brief Consume next character
     * @return Next character
     */
    char consumeChar();

    /**
     * @brief Parse single number
     * @param value Output: parsed number
     * @return true if successful
     */
    bool parseNumber(double& value);

    /**
     * @brief Parse 3D point
     * @param point Output: parsed point
     * @return true if successful
     */
    bool parseVec3(Vec3& point);

    /**
     * @brief Parse ambient light
     * @param scene Scene to add to
     * @return true if successful
     */
    bool parseAmbient(Scene& scene);

    /**
     * @brief Parse point light
     * @param scene Scene to add to
     * @return true if successful
     */
    bool parseLight(Scene& scene);

    /**
     * @brief Parse sphere
     * @param scene Scene to add to
     * @return true if successful
     */
    bool parseSphere(Scene& scene);

    /**
     * @brief Parse plane
     * @param scene Scene to add to
     * @return true if successful
     */
    bool parsePlane(Scene& scene);

    /**
     * @brief Parse cylinder
     * @param scene Scene to add to
     * @return true if successful
     */
    bool parseCylinder(Scene& scene);

    /**
     * @brief Parse cone
     * @param scene Scene to add to
     * @return true if successful
     */
    bool parseCone(Scene& scene);

    /**
     * @brief Parse camera
     * @param scene Scene to update
     * @return true if successful
     */
    bool parseCamera(Scene& scene);

    /**
     * @brief Parse background color
     * @param scene Scene to update
     * @return true if successful
     */
    bool parseBackground(Scene& scene);

    /**
     * @brief Report parsing error
     * @param message Error message
     */
    void reportError(const std::string& message);

    /**
     * @brief Check if character matches identifier
     * @param identifier Expected identifier (1-2 chars)
     * @return true if matches and consumed
     */
    bool matchIdentifier(const std::string& identifier);
};
