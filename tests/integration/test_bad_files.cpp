// Tests de robustesse des fichiers (T025), Catch2, integration.
// Couvre le Prompt et le DoD : fichier inexistant, repertoire, vide,
// illisible (chmod 000), corrompu (fuzz maison suppression/duplication
// de tokens + tronque), imbrication folle (> 32), `include` cyclique
// (format sans `include`, donc impossible), binaire. Chaque cas :
// message utile `fichier:ligne:colonne`, code retour != 0, jamais
// de segfault (rejoue sous ASan via `make test-asan`).

#include <catch2/catch_amalgamated.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "rt/scene/Lexer.hpp"
#include "rt/scene/Parser.hpp"

namespace {

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

std::string parseFileFail(std::string_view path) {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile(path);
	REQUIRE(result.isError());
	REQUIRE_FALSE(result.status().message.empty());
	return result.status().message;
}

std::string parseContentFail(std::string_view content, std::string_view name = "f.rt") {
	rt::Result<rt::scene::Scene> result = rt::scene::parseContent(content, name);
	REQUIRE(result.isError());
	REQUIRE_FALSE(result.status().message.empty());
	return result.status().message;
}

std::string lexContentFail(std::string_view content, std::string_view name = "f.rt") {
	rt::Result<std::vector<rt::scene::Token>> result =
	    rt::scene::lexContent(content, name);
	REQUIRE(result.isError());
	REQUIRE_FALSE(result.status().message.empty());
	return result.status().message;
}

} // namespace

TEST_CASE("bad files : fixtures invalides rejetees avec localisation, sans crash",
          "[bad-files]") {
	const char* invalid[] = {
	    "tests/cases/invalid/missing_brace.rt",
	    "tests/cases/invalid/unclosed_string.rt",
	    "tests/cases/invalid/binary.rt",
	    "tests/cases/invalid/bad_float.rt",
	    "tests/cases/invalid/bad_number.rt",
	    "tests/cases/invalid/unknown_directive.rt",
	    "tests/cases/invalid/empty.rt",
	    "tests/cases/invalid/deep_nesting.rt",
	    "tests/cases/invalid/include.rt",
	    "tests/cases/invalid/truncated.rt",
	    "tests/cases/invalid/garbage.rt",
	};
	for (const char* path : invalid) {
		INFO("fichier : " << path);
		const std::string message = parseFileFail(path);
		INFO("message : " << message);
		REQUIRE(hasLocation(message, path));
	}
}

TEST_CASE("bad files : fixtures valides toujours acceptees", "[bad-files]") {
	for (const char* path :
	     {"tests/cases/valid/minimal.rt", "tests/cases/valid/group.rt"}) {
		INFO("fichier : " << path);
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile(path);
		INFO("message : " << (result.isError() ? result.status().message : std::string("ok")));
		REQUIRE(result.isOk());
	}
}

TEST_CASE("bad files : inexistant, repertoire et vide", "[bad-files]") {
	{
		rt::Result<rt::scene::Scene> result =
		    rt::scene::parseFile("tests/cases/invalid/does_not_exist_t025.rt");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
		REQUIRE_FALSE(result.status().message.empty());
	}
	{
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile("tests/cases");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
		REQUIRE(result.status().message.find("is a directory") != std::string::npos);
	}
	{
		rt::Result<rt::scene::Scene> result =
		    rt::scene::parseFile("tests/cases/invalid");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::IoError);
	}
	{
		const std::string message = parseFileFail("tests/cases/invalid/empty.rt");
		REQUIRE(message.find("empty file") != std::string::npos);
		REQUIRE(hasLocation(message, "tests/cases/invalid/empty.rt"));
	}
	{
		const std::string message = parseContentFail("", "empty.rt");
		REQUIRE(message.find("empty file") != std::string::npos);
	}
}

TEST_CASE("bad files : illisible (chmod 000) sans crash", "[bad-files]") {
	const std::string path = "/tmp/rt_t025_unreadable.rt";
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		REQUIRE(out.good());
		out << "scene { camera { position (0 1 4) target (0 0 0) } "
		       "objects { object { type sphere } } }\n";
	}
	std::error_code code;
	std::filesystem::permissions(path,
	                             std::filesystem::perms::none,
	                             std::filesystem::perm_options::replace,
	                             code);
	REQUIRE_FALSE(code);
	{
		// Sous root le fichier reste lisible : on exige seulement l'absence
		// de crash (erreur IoError ou succes selon les droits effectifs).
		rt::Result<rt::scene::Scene> result = rt::scene::parseFile(path);
		if (result.isError()) {
			REQUIRE(result.status().code == rt::StatusCode::IoError);
			REQUIRE_FALSE(result.status().message.empty());
		} else {
			REQUIRE(result.value().objects.size() == 1);
		}
	}
	std::filesystem::permissions(path,
	                             std::filesystem::perms::owner_read
	                                 | std::filesystem::perms::owner_write,
	                             std::filesystem::perm_options::replace,
	                             code);
	std::filesystem::remove(path, code);
}

TEST_CASE("bad files : imbrication folle bornee a 32", "[bad-files]") {
	// 32 niveaux passent au lexer, le 33e casse (FORMAT_SCENE §2).
	{
		std::string ok;
		for (int i = 0; i < 32; ++i) {
			ok += "b { ";
		}
		for (int i = 0; i < 32; ++i) {
			ok += "} ";
		}
		rt::Result<std::vector<rt::scene::Token>> result =
		    rt::scene::lexContent(ok, "ok.rt");
		REQUIRE(result.isOk());
	}
	{
		std::string deep;
		for (int i = 0; i < 33; ++i) {
			deep += "b { ";
		}
		const std::string message = lexContentFail(deep, "deep.rt");
		REQUIRE(message.find("nesting too deep") != std::string::npos);
		rt::Result<std::vector<rt::scene::Token>> result =
		    rt::scene::lexContent(deep, "deep.rt");
		REQUIRE(result.status().code == rt::StatusCode::LimitExceeded);
	}
	// Fixture : 40 groupes imbriques, rejetee avec la limite.
	{
		rt::Result<rt::scene::Scene> result =
		    rt::scene::parseFile("tests/cases/invalid/deep_nesting.rt");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::LimitExceeded);
		REQUIRE(hasLocation(result.status().message,
		                    "tests/cases/invalid/deep_nesting.rt"));
	}
	// Contenu genere : meme limite par le parser, sans crash.
	{
		std::string content = "scene { camera { position (0 1 4) target (0 0 0) } objects { ";
		for (int i = 0; i < 40; ++i) {
			content += "group \"d\" { ";
		}
		content += "object { type sphere } ";
		for (int i = 0; i < 40; ++i) {
			content += "} ";
		}
		content += "} }";
		const std::string message = parseContentFail(content, "deep.rt");
		INFO("message : " << message);
		REQUIRE(hasLocation(message, "deep.rt"));
	}
}

TEST_CASE("bad files : include inconnu, aucun cycle possible", "[bad-files]") {
	// Le format n'a PAS de directive `include` (FORMAT_SCENE §2) : toute
	// tentative est une erreur fatale, donc aucun `include` cyclique
	// n'est constructible.
	const std::string message = parseContentFail(
	    "scene { camera { position (0 1 4) target (0 0 0) } include \"other.rt\" "
	    "objects { object { type sphere } } }",
	    "inc.rt");
	REQUIRE(message.find("include") != std::string::npos);
	REQUIRE(hasLocation(message, "inc.rt"));
	{
		rt::Result<rt::scene::Scene> result =
		    rt::scene::parseFile("tests/cases/invalid/include.rt");
		REQUIRE(result.isError());
		REQUIRE(result.status().code == rt::StatusCode::NotFound);
		REQUIRE(result.status().message.find("include") != std::string::npos);
	}
	{
		const std::string nested = parseContentFail(
		    "scene { camera { position (0 1 4) target (0 0 0) } objects { "
		    "group \"g\" { include \"x.rt\" } } }",
		    "inc2.rt");
		REQUIRE(nested.find("include") != std::string::npos);
	}
}

TEST_CASE("bad files : binaire, NUL, UTF-8 et garbage sans crash", "[bad-files]") {
	{
		const std::string message = parseFileFail("tests/cases/invalid/binary.rt");
		REQUIRE(hasLocation(message, "tests/cases/invalid/binary.rt"));
	}
	{
		const std::string message = parseFileFail("tests/cases/invalid/garbage.rt");
		REQUIRE(hasLocation(message, "tests/cases/invalid/garbage.rt"));
	}
	{
		std::string binary("width \xFF");
		const std::string message = lexContentFail(binary, "bin.rt");
		REQUIRE(hasLocation(message, "bin.rt"));
	}
	{
		std::string nul("scene {\x00 camera }", 17);
		const std::string message = lexContentFail(nul, "nul.rt");
		REQUIRE(hasLocation(message, "nul.rt"));
	}
	{
		std::string truncated("x \"a\xE2\x82");
		const std::string message = lexContentFail(truncated, "trunc.rt");
		REQUIRE(hasLocation(message, "trunc.rt"));
	}
	{
		std::string control("width \x01");
		const std::string message = lexContentFail(control, "ctl.rt");
		REQUIRE(hasLocation(message, "ctl.rt"));
	}
	{
		const std::string message = lexContentFail("width @", "at.rt");
		REQUIRE(hasLocation(message, "at.rt"));
	}
	// Nombre absurde et fichier tronque : erreur propre, pas de crash.
	{
		const std::string message = parseFileFail("tests/cases/invalid/bad_float.rt");
		REQUIRE(message.find("invalid number") != std::string::npos);
	}
	{
		const std::string message = parseFileFail("tests/cases/invalid/truncated.rt");
		REQUIRE(hasLocation(message, "tests/cases/invalid/truncated.rt"));
	}
	{
		const std::string message =
		    parseContentFail("scene { camera { position (0 1 4) target (0 0 0) } "
		                     "objects { object { type sphere } } } garbage_trailing",
		                     "trail.rt");
		REQUIRE(hasLocation(message, "trail.rt"));
	}
}

TEST_CASE("bad files : fuzz suppression/duplication de tokens sans crash", "[bad-files]") {
	rt::Result<std::vector<rt::scene::Token>> lexed =
	    rt::scene::lexFile("tests/cases/valid/minimal.rt");
	REQUIRE(lexed.isOk());
	const std::vector<rt::scene::Token>& tokens = lexed.value();
	REQUIRE_FALSE(tokens.empty());
	REQUIRE(tokens.back().kind == rt::scene::TokenKind::End);
	// Reconstruction sans mutation : le contenu de reference parse.
	{
		rt::Result<rt::scene::Scene> intact =
		    rt::scene::parseTokens(tokens, "minimal.rt");
		REQUIRE(intact.isOk());
	}
	const std::size_t count = tokens.size() - 1U;
	REQUIRE(count > 10U);
	std::size_t deleteFails = 0;
	std::size_t duplicateFails = 0;
	for (std::size_t i = 0; i < count; ++i) {
		// Suppression du token i.
		{
			std::vector<rt::scene::Token> mutant = tokens;
			mutant.erase(mutant.begin() + static_cast<std::ptrdiff_t>(i));
			rt::Result<rt::scene::Scene> result =
			    rt::scene::parseTokens(mutant, "mut.rt");
			// Pas de crash : soit ok (mutation neutre), soit erreur
			// avec un message non vide.
			if (result.isError()) {
				REQUIRE_FALSE(result.status().message.empty());
				++deleteFails;
			}
		}
		// Duplication du token i.
		{
			std::vector<rt::scene::Token> mutant = tokens;
			mutant.insert(mutant.begin() + static_cast<std::ptrdiff_t>(i), tokens[i]);
			rt::Result<rt::scene::Scene> result =
			    rt::scene::parseTokens(mutant, "mut.rt");
			if (result.isError()) {
				REQUIRE_FALSE(result.status().message.empty());
				++duplicateFails;
			}
		}
	}
	// Le fuzz est effectif : la plupart des mutations cassent le fichier.
	INFO("deleteFails=" << deleteFails << " duplicateFails=" << duplicateFails
	                    << " count=" << count);
	REQUIRE(deleteFails > 0U);
	REQUIRE(duplicateFails > 0U);
	// Troncation texte : moitie et quart manquants, sans crash.
	{
		std::ifstream input("tests/cases/valid/minimal.rt", std::ios::binary);
		REQUIRE(input.good());
		const std::string content((std::istreambuf_iterator<char>(input)),
		                          std::istreambuf_iterator<char>());
		REQUIRE_FALSE(content.empty());
		const std::string half = content.substr(0, content.size() / 2U);
		const std::string quarter = content.substr(0, content.size() / 4U);
		REQUIRE(parseContentFail(half, "half.rt").find("half.rt:") == 0U);
		REQUIRE(parseContentFail(quarter, "quarter.rt").find("quarter.rt:") == 0U);
	}
}
