#pragma once

// Parser du format `.rt` structure (T023) — tokens -> `Scene`, sans exception.
// Consomme les tokens du lexer (T022) en s'appuyant sur la table unique
// `schema/` (T021, regle R1) : toute cle est resolue par `find()`, toute
// valeur est controlee par `check*()` **avant** stockage (aucune allocation
// avant validation des bornes). Imbrication, defauts, alias (`lookAt`,
// `color`, `position`, `reflect`), tetes `object sphere "nom"` /
// `light point "cle"` (equivalentes aux proprietes, le contenu gagne),
// tableaux (`size` 2 nombres, `attenuation` 3 nombres), groupes recursifs.
// Chaque echec renvoie `Status` localise `fichier:ligne:colonne` ; le
// `Result` detruit proprement les blocs ouverts (pas de fuite, R2).

#include <string_view>
#include <vector>

#include "rt/base/Result.hpp"
#include "rt/scene/Lexer.hpp"
#include "rt/scene/Scene.hpp"

namespace rt::scene {

// Decoupe puis construit depuis un contenu en memoire.
[[nodiscard]] Result<Scene> parseContent(std::string_view content, std::string_view filename);

// Lit `path` (via le lexer) puis construit comme `parseContent`.
// Fichier inexistant / repertoire : `IoError` avec le chemin.
[[nodiscard]] Result<Scene> parseFile(std::string_view path);

// Construit depuis des tokens deja lexes (le nom sert aux erreurs).
[[nodiscard]] Result<Scene> parseTokens(const std::vector<Token>& tokens,
                                       std::string_view filename);

} // namespace rt::scene
