#pragma once

// Chargement de textures PNG/JPEG (T102, *Textures* sous-critere 5).
// `TextureImage` = image RGBA8 (w*h*4 octets, `vector`, chemin froid).
// `TextureCache` = cache `nom -> partage` (`shared_ptr`, RAII) : le meme
// chemin charge deux fois rend le meme pointeur, `clear()` libere tout
// (valgrind propre), fichier absent -> `Status` avec le chemin complet,
// memoire bornee par `maxBytes` (`limits.max_texture_bytes`, T024).
// PNG via libpng systeme, JPEG via libjpeg systeme (**bibliotheque autre
// que MiniLibX/XPM**, exigence du sous-critere 5). Detection par magie
// (PNG `89 50 4E 47`, JPEG `FF D8`), pas par extension. Erreurs libpng/
// libjpeg (`setjmp`, C) converties en `Status`, jamais d'exception (R2)
// hors `bad_alloc` du chemin froid (filet `main`, comme `Scene`).
// Calque `io/` : ne voit que `base/` (regle d'or §2.1).

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "rt/base/Result.hpp"
#include "rt/base/Status.hpp"

namespace rt::io {

struct TextureImage {
	int width = 0;
	int height = 0;
	std::vector<std::uint8_t> rgba;
};

class TextureCache {
  public:
	TextureCache() = default;
	~TextureCache() = default;

	TextureCache(const TextureCache&) = delete;
	TextureCache& operator=(const TextureCache&) = delete;
	TextureCache(TextureCache&&) = default;
	TextureCache& operator=(TextureCache&&) = default;

	// Charge (ou rend le cache) `path`, borne par `maxBytes` (octets du
	// fichier sur disque ; `LimitExceeded` si depasse). `maxBytes <= 0` =
	// pas de borne. Fichier absent/illisible -> `IoError` avec le chemin
	// complet dans le message. Contenu ni PNG ni JPEG -> `IoError`.
	[[nodiscard]] Result<std::shared_ptr<TextureImage>> load(const std::string& path,
	                                                         long long maxBytes);
	// Libere tout le cache (RAII : les `shared_ptr` externes gardent leur
	// image vivante, le cache ne retient plus rien).
	void clear();
	[[nodiscard]] std::size_t size() const noexcept { return cache_.size(); }
	// Octets pixels caches (somme w*h*4).
	[[nodiscard]] std::size_t bytes() const noexcept;

  private:
	std::map<std::string, std::shared_ptr<TextureImage>> cache_;
};

} // namespace rt::io
