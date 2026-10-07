#pragma once

// Lexer du format `.rt` structure (T022) — decoupe sans exception.
// Tokens : `{ } ( )`, identifiants, chaines, nombres ; les commentaires `#`
// sont reconnus puis ignores. Chaque token garde sa ligne et sa colonne
// (1-based) pour des erreurs localisees `fichier:ligne:colonne`.
// Nombres : `[-+]? (chiffres [. chiffres]? | . chiffres) ([eE] [-+]? chiffres)?`
// (`1.5`, `-2e3`, `+0.25`, `.5`, `1.`). Chaines : `"..."` avec `\"` et `\\`
// uniquement ; guillemet non ferme, echappement invalide et retour ligne dans
// une chaine = erreur. Tout octet non UTF-8 valide = erreur ; tout caractere
// ASCII hors vocabulaire (`@`, `;`, ...) = erreur. Imbrication `{ }` bornee a
// `kMaxNesting` (32, cf. docs/FORMAT_SCENE.md §2) : depassement, `}` sans
// ouvrant et `{` non ferme en fin de fichier = erreur. Aucun `throw` (R2) ;
// les echecs partent en `rt::Status` via `rt::Result`.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "rt/base/Result.hpp"
#include "rt/base/Status.hpp"

namespace rt::scene {

enum class TokenKind : std::uint8_t {
	LBrace = 0,
	RBrace = 1,
	LParen = 2,
	RParen = 3,
	Ident = 4,
	String = 5,
	Number = 6,
	End = 7,
};

[[nodiscard]] std::string_view toString(TokenKind kind) noexcept;

struct Token {
	TokenKind kind = TokenKind::End;
	std::string text;
	double numberValue = 0.0;
	int line = 1;
	int col = 1;
};

// Garde-fou contre l'imbrication folle (T025) : 32 niveaux (FORMAT_SCENE §2).
inline constexpr int kMaxNesting = 32;

// Decoupe `content` en tokens. Succes : vecteur termine par un token `End`.
// Echec : `Status` d'erreur dont `message` vaut `fichier:ligne:colonne: detail`
// (`ParseError` pour la syntaxe, `LimitExceeded` pour la profondeur,
// `IoError` jamais ici). Ne lance jamais.
[[nodiscard]] Result<std::vector<Token>> lexContent(std::string_view content,
                                                    std::string_view filename);

// Lit `path` (binaire) puis decoupe comme `lexContent` avec `filename = path`.
// Fichier inexistant, repertoire, illisible : `IoError` avec le chemin.
[[nodiscard]] Result<std::vector<Token>> lexFile(std::string_view path);

} // namespace rt::scene
