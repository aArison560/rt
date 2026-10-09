// Chargement de textures PNG/JPEG (T102), Catch2.
// 1 PNG + 1 JPEG dans `textures/`, cache partage, absent -> `Status`
// avec chemin complet, 0 crash, borne `maxBytes`, liberation `clear()`.

#include <catch2/catch_amalgamated.hpp>

#include <string>

#include "rt/io/Texture.hpp"

TEST_CASE("texture : charge 1 PNG et 1 JPEG (T102)", "[texture][t102]") {
	rt::io::TextureCache cache;
	rt::Result<std::shared_ptr<rt::io::TextureImage>> png =
	    cache.load("textures/checker.png", 67108864LL);
	INFO((png.isError() ? png.status().message : std::string("png ok")));
	REQUIRE(png.isOk());
	REQUIRE(png.value()->width == 64);
	REQUIRE(png.value()->height == 64);
	REQUIRE(png.value()->rgba.size() == 64U * 64U * 4U);
	rt::Result<std::shared_ptr<rt::io::TextureImage>> jpg =
	    cache.load("textures/gradient.jpg", 67108864LL);
	INFO((jpg.isError() ? jpg.status().message : std::string("jpg ok")));
	REQUIRE(jpg.isOk());
	REQUIRE(jpg.value()->width == 64);
	REQUIRE(jpg.value()->height == 64);
	REQUIRE(jpg.value()->rgba.size() == 64U * 64U * 4U);
	REQUIRE(cache.size() == 2);
	REQUIRE(cache.bytes() == 2U * 64U * 64U * 4U);
}

TEST_CASE("texture : cache partage le meme pointeur (T102)", "[texture][t102]") {
	rt::io::TextureCache cache;
	rt::Result<std::shared_ptr<rt::io::TextureImage>> first =
	    cache.load("textures/checker.png", 0);
	rt::Result<std::shared_ptr<rt::io::TextureImage>> second =
	    cache.load("textures/checker.png", 0);
	REQUIRE(first.isOk());
	REQUIRE(second.isOk());
	REQUIRE(first.value().get() == second.value().get());
	REQUIRE(cache.size() == 1);
	cache.clear();
	REQUIRE(cache.size() == 0);
	REQUIRE(cache.bytes() == 0);
	// Apres `clear`, recharge une image neuve (meme contenu, autre objet).
	rt::Result<std::shared_ptr<rt::io::TextureImage>> third =
	    cache.load("textures/checker.png", 0);
	REQUIRE(third.isOk());
	REQUIRE(third.value()->rgba == first.value()->rgba);
}

TEST_CASE("texture : absent et bornes sans crash (T102)", "[texture][t102]") {
	rt::io::TextureCache cache;
	rt::Result<std::shared_ptr<rt::io::TextureImage>> missing =
	    cache.load("textures/nonexistent_xyz.png", 67108864LL);
	REQUIRE(missing.isError());
	// Le message porte le chemin complet (DoD).
	REQUIRE(missing.status().message.find("textures/nonexistent_xyz.png") != std::string::npos);
	rt::Result<std::shared_ptr<rt::io::TextureImage>> empty =
	    cache.load("", 67108864LL);
	REQUIRE(empty.isError());
	// Borne depassee -> erreur propre, pas d'allocation surprise.
	rt::Result<std::shared_ptr<rt::io::TextureImage>> tooBig =
	    cache.load("textures/checker.png", 10);
	REQUIRE(tooBig.isError());
	REQUIRE(tooBig.status().code == rt::StatusCode::LimitExceeded);
	// Format inconnu -> erreur propre.
	rt::Result<std::shared_ptr<rt::io::TextureImage>> bad =
	    cache.load("author", 67108864LL);
	REQUIRE(bad.isError());
}
