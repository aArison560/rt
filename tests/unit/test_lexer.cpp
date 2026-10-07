// Tests du lexer `.rt` (T022), Catch2.
// Couvre le Prompt et le DoD : tokens `{ } ( ) ident string number`,
// ligne/colonne, nombres (`1.5`, `-2e3`), guillemet non ferme, caracteres
// invalides (UTF-8 compris), profondeur bornee a 32, `Status` localise
// `fichier:ligne:colonne`, fixtures `tests/cases/` sans crash.

#include <catch2/catch_amalgamated.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "rt/scene/Lexer.hpp"

namespace {

using rt::scene::TokenKind;

// `lexContent` renvoie `Result<vector<Token>>` : succes -> tokens termines
// par `End`, echec -> `Status` avec `message = fichier:ligne:colonne: detail`.
bool lexOk(std::string_view content, std::vector<rt::scene::Token> &out) {
	rt::Result<std::vector<rt::scene::Token>> result =
	    rt::scene::lexContent(content, "test.rt");
	if (result.isError()) {
		INFO("erreur inattendue : " << result.status().message);
		return false;
	}
	out = result.value();
	return true;
}

std::string lexFailMessage(std::string_view content, std::string_view filename = "f.rt") {
	rt::Result<std::vector<rt::scene::Token>> result = rt::scene::lexContent(content, filename);
	REQUIRE(result.isError());
	REQUIRE_FALSE(result.status().message.empty());
	return result.status().message;
}

bool messageHasLocation(const std::string &message, std::string_view filename) {
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
	const std::size_t third = message.find(':', second + 1);
	return third != std::string::npos;
}

} // namespace

TEST_CASE("lexer : symboles et positions ligne/colonne", "[lexer]") {
	std::vector<rt::scene::Token> tokens;
	REQUIRE(lexOk("scene {\n  width 640\n}", tokens));
	REQUIRE(tokens.size() == 6); // scene { width 640 } + End
	REQUIRE(tokens[0].kind == TokenKind::Ident);
	REQUIRE(tokens[0].text == "scene");
	REQUIRE(tokens[0].line == 1);
	REQUIRE(tokens[0].col == 1);
	REQUIRE(tokens[1].kind == TokenKind::LBrace);
	REQUIRE(tokens[1].line == 1);
	REQUIRE(tokens[1].col == 7);
	REQUIRE(tokens[2].kind == TokenKind::Ident);
	REQUIRE(tokens[2].text == "width");
	REQUIRE(tokens[2].line == 2);
	REQUIRE(tokens[2].col == 3);
	REQUIRE(tokens[3].kind == TokenKind::Number);
	REQUIRE(tokens[3].text == "640");
	REQUIRE(tokens[3].line == 2);
	REQUIRE(tokens[3].col == 9);
	REQUIRE(tokens[4].kind == TokenKind::RBrace);
	REQUIRE(tokens[4].line == 3);
	REQUIRE(tokens[4].col == 1);
	REQUIRE(tokens[5].kind == TokenKind::End);
}

TEST_CASE("lexer : parentheses, vecteurs et chaines", "[lexer]") {
	std::vector<rt::scene::Token> tokens;
	REQUIRE(lexOk("position (0 1.5 -2) name \"sol\"", tokens));
	REQUIRE(tokens[0].kind == TokenKind::Ident);
	REQUIRE(tokens[1].kind == TokenKind::LParen);
	REQUIRE(tokens[2].kind == TokenKind::Number);
	REQUIRE(tokens[2].text == "0");
	REQUIRE(tokens[3].kind == TokenKind::Number);
	REQUIRE(tokens[3].text == "1.5");
	REQUIRE(tokens[4].kind == TokenKind::Number);
	REQUIRE(tokens[4].text == "-2");
	REQUIRE(tokens[5].kind == TokenKind::RParen);
	REQUIRE(tokens[6].kind == TokenKind::Ident);
	REQUIRE(tokens[6].text == "name");
	REQUIRE(tokens[7].kind == TokenKind::String);
	REQUIRE(tokens[7].text == "sol");

	// Echappements `\"` et `\\` uniquement, avec la valeur attendue.
	std::vector<rt::scene::Token> escaped;
	REQUIRE(lexOk("\"a\\\"b\\\\c\"", escaped));
	REQUIRE(escaped[0].kind == TokenKind::String);
	REQUIRE(escaped[0].text == "a\"b\\c");

	// `#` dans une chaine n'ouvre pas un commentaire.
	std::vector<rt::scene::Token> hash;
	REQUIRE(lexOk("\"a#b\" # commentaire", hash));
	REQUIRE(hash[0].kind == TokenKind::String);
	REQUIRE(hash[0].text == "a#b");
	REQUIRE(hash[1].kind == TokenKind::End);
}

TEST_CASE("lexer : nombres valides du format", "[lexer]") {
	const char *valid[] = {"1.5",  "-2e3", "+0.25", "1e-3",  "-2E3",
	                       ".5",   "1.",   "0",     "640",   "+1",
	                       "-0.5", "1E+3", "2.25",  "-0.25", ".25"};
	for (const char *lexeme : valid) {
		INFO("nombre : " << lexeme);
		std::vector<rt::scene::Token> tokens;
		REQUIRE(lexOk(lexeme, tokens));
		REQUIRE(tokens[0].kind == TokenKind::Number);
		REQUIRE(tokens[0].text == lexeme);
	}
}

TEST_CASE("lexer : nombres absurdes rejetes", "[lexer]") {
	const char *invalid[] = {"1.2.3", "1e",  "1e+",  "--2", "12abc",
	                         ".",     "+",  "-",     "+.",  "1.2e",
	                         "1ee3",  "1..2", "3.4.5"};
	for (const char *lexeme : invalid) {
		INFO("nombre absurde : " << lexeme);
		const std::string message = lexFailMessage(lexeme);
		REQUIRE(messageHasLocation(message, "f.rt"));
	}

	// Valeurs du Prompt toujours acceptees avec la bonne valeur numerique.
	std::vector<rt::scene::Token> tokens;
	REQUIRE(lexOk("1.5 -2e3", tokens));
	REQUIRE(tokens[0].numberValue == Catch::Approx(1.5));
	REQUIRE(tokens[1].numberValue == Catch::Approx(-2000.0));
}

TEST_CASE("lexer : guillemet non ferme et echappements", "[lexer]") {
	REQUIRE(messageHasLocation(lexFailMessage("\"oops"), "f.rt"));
	REQUIRE(messageHasLocation(lexFailMessage("name \"sol"), "f.rt"));
	REQUIRE(messageHasLocation(lexFailMessage("\"a\\qb\""), "f.rt"));
	REQUIRE(messageHasLocation(lexFailMessage("\"a\nb\""), "f.rt"));

	const std::string unterminated = lexFailMessage("scene { name \"oops\n}", "scene.rt");
	REQUIRE(unterminated.rfind("scene.rt:", 0) == 0);
}

TEST_CASE("lexer : caracteres invalides et UTF-8", "[lexer]") {
	REQUIRE(messageHasLocation(lexFailMessage("width @"), "f.rt"));
	REQUIRE(messageHasLocation(lexFailMessage("width;"), "f.rt"));
	REQUIRE(messageHasLocation(lexFailMessage("a,b"), "f.rt"));

	// Octet binaire isole (0xFF) : jamais du UTF-8 valide.
	std::string binary("width \xFF");
	REQUIRE(messageHasLocation(lexFailMessage(binary), "f.rt"));

	// Sequence tronquee et surlong (0xC0 0xAF) : invalides.
	std::string truncated("x \"a\xE2\x82");
	REQUIRE(messageHasLocation(lexFailMessage(truncated), "f.rt"));
	std::string overlong("x \xC0\xAF");
	REQUIRE(messageHasLocation(lexFailMessage(overlong), "f.rt"));

	// UTF-8 valide mais hors vocabulaire hors chaine/commentaire : erreur.
	std::string outside("largeur \xC3\xA9");
	REQUIRE(messageHasLocation(lexFailMessage(outside), "f.rt"));

	// Le meme caractere DANS une chaine ou un commentaire passe.
	std::vector<rt::scene::Token> tokens;
	REQUIRE(lexOk(std::string("\"caf\xC3\xA9\""), tokens));
	REQUIRE(tokens[0].kind == TokenKind::String);
	std::vector<rt::scene::Token> commented;
	REQUIRE(lexOk(std::string("# caf\xC3\xA9\nscene"), commented));
	REQUIRE(commented[0].kind == TokenKind::Ident);
}

TEST_CASE("lexer : accolades et profondeur bornee", "[lexer]") {
	// `{` non ferme en fin de fichier.
	REQUIRE(messageHasLocation(lexFailMessage("scene { limits { width 640"), "f.rt"));
	// `}` sans ouvrant.
	REQUIRE(messageHasLocation(lexFailMessage("scene }"), "f.rt"));
	REQUIRE(messageHasLocation(lexFailMessage("}"), "f.rt"));

	// 32 niveaux passent, le 33e casse (FORMAT_SCENE §2).
	std::string ok;
	for (int i = 0; i < 32; ++i) {
		ok += "b { ";
	}
	for (int i = 0; i < 32; ++i) {
		ok += "} ";
	}
	std::vector<rt::scene::Token> tokens;
	REQUIRE(lexOk(ok, tokens));

	std::string deep;
	for (int i = 0; i < 33; ++i) {
		deep += "b { ";
	}
	const std::string deepMessage = lexFailMessage(deep, "deep.rt");
	REQUIRE(deepMessage.rfind("deep.rt:", 0) == 0);
	rt::Result<std::vector<rt::scene::Token>> deepResult =
	    rt::scene::lexContent(deep, "deep.rt");
	REQUIRE(deepResult.status().code == rt::StatusCode::LimitExceeded);
}

TEST_CASE("lexer : commentaires et espaces libres", "[lexer]") {
	std::vector<rt::scene::Token> tokens;
	REQUIRE(lexOk("# ligne entiere\nscene # fin de ligne\n{ # encore\n}", tokens));
	REQUIRE(tokens[0].kind == TokenKind::Ident);
	REQUIRE(tokens[0].text == "scene");
	REQUIRE(tokens[1].kind == TokenKind::LBrace);
	REQUIRE(tokens[2].kind == TokenKind::RBrace);
	REQUIRE(tokens[3].kind == TokenKind::End);
	// La colonne suit le commentaire : `}` est en colonne 1 de la ligne 4.
	REQUIRE(tokens[2].line == 4);
}

TEST_CASE("lexer : fixtures tests/cases sans crash", "[lexer]") {
	// Valides : acceptes.
	for (const char *path : {"tests/cases/valid/minimal.rt", "tests/cases/valid/group.rt"}) {
		INFO("fichier : " << path);
		rt::Result<std::vector<rt::scene::Token>> result = rt::scene::lexFile(path);
		INFO("message : " << (result.isError() ? result.status().message : "ok"));
		REQUIRE(result.isOk());
		REQUIRE_FALSE(result.value().empty());
		REQUIRE(result.value().back().kind == TokenKind::End);
	}

	// Invalides lexicaux : rejetes avec `fichier:ligne:colonne`.
	for (const char *path : {"tests/cases/invalid/missing_brace.rt",
	                        "tests/cases/invalid/unclosed_string.rt",
	                        "tests/cases/invalid/binary.rt",
	                        "tests/cases/invalid/bad_float.rt"}) {
		INFO("fichier : " << path);
		rt::Result<std::vector<rt::scene::Token>> result = rt::scene::lexFile(path);
		REQUIRE(result.isError());
		REQUIRE(messageHasLocation(result.status().message, path));
	}

	// `abc` est un identifiant valide : le lexer l'accepte (c'est le parser
	// T023 qui le refusera comme nombre attendu). Pas de crash.
	{
		rt::Result<std::vector<rt::scene::Token>> result =
		    rt::scene::lexFile("tests/cases/invalid/bad_number.rt");
		REQUIRE(result.isOk());
	}

	// Inexistant et repertoire : `IoError` propre, jamais de crash.
	{
		rt::Result<std::vector<rt::scene::Token>> result =
		    rt::scene::lexFile("tests/cases/invalid/does_not_exist.rt");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
	}
	{
		rt::Result<std::vector<rt::scene::Token>> result = rt::scene::lexFile("tests/cases");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
	}
}
