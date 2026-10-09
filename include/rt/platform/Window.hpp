#pragma once

// Couche plateforme SDL (T070) — fenetre RAII sans lien vers le moteur.
// `Window` ouvre une fenetre SDL2, y copie le `Framebuffer` persistant puis
// presente. Aucun acces a la scene ni au moteur de trace : seule
// `rt::render::Framebuffer` (tampon d'affichage, calque du dessous) est lue,
// par pointeur brut (pas d'allocation, pas de levee, R2/R3).
// Le chemin sans fenetre n'appelle jamais ce fichier (R6) : `main` branche
// `--headless`/`--out` vers le chemin froid sans SDL.
// Taille minimale 64x64 imposee a la creation (T078).

#include "rt/base/Status.hpp"

namespace rt::render {
class Framebuffer;
}

namespace rt::platform {

struct WindowStats {
	long long blitCount = 0;
	long long exposeCount = 0;
	long long lastBlitUs = 0;
};

class Window {
  public:
	Window() = default;
	// RAII : libere texture, moteur de presentation, fenetre, puis SDL.
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	// Ouvre `w`x`h` (1..8192, 64 minimum effectif pour l'affichage).
	// `title` peut etre nul (titre par defaut). Erreur -> `Status`
	// (dimensions, SDL indisponible, absence de `DISPLAY` en mode fenetre).
	[[nodiscard]] Status init(int width, int height, const char* title);
	// Ferme et libere (idempotent, sans echec).
	void shutdown() noexcept;

	[[nodiscard]] bool isOpen() const noexcept { return open_; }
	[[nodiscard]] int width() const noexcept { return width_; }
	[[nodiscard]] int height() const noexcept { return height_; }
	[[nodiscard]] WindowStats stats() const noexcept { return stats_; }

	// Copie l'affichage du tampon vers la texture puis presente.
	// Tampon nul ou ferme -> ignore (documente, jamais de crash).
	void blit(const render::Framebuffer& fb) noexcept;
	// Chemin dedie a l'exposition (T071) : re-presente la texture
	// conservee sans toucher au tampon ni au moteur de trace.
	// Mesure la duree et journalise `[expose] blit in <us> us` sur stderr.
	void presentCached() noexcept;
	// Vrai si un evenement de fermeture a ete recu (a vider par l'appelant).
	[[nodiscard]] bool pollQuit() noexcept;
	// Vrai si un evenement d'exposition a ete recu et deja re-presente.
	// Le re-affichage est fait ici meme (reblit), l'appelant n'a rien a faire.
	[[nodiscard]] bool pollExpose() noexcept;
	// File de touches (T073) : `pollQuit` pompe les evenements et stocke les
	// touches ; `pollKey` en rend une (code `SDL_Keycode` brut, 0 si vide).
	// Aucun acces scene/moteur ici, la traduction vit dans `app/Controls`.
	[[nodiscard]] bool pollKey(int& outSdlKey) noexcept;

  private:
	void pumpEvents() noexcept;
	void* window_ = nullptr;
	void* renderer_ = nullptr;
	void* texture_ = nullptr;
	int width_ = 0;
	int height_ = 0;
	int texWidth_ = 0;
	int texHeight_ = 0;
	bool open_ = false;
	bool quitSeen_ = false;
	static constexpr int kKeyQueue = 32;
	int keyQueue_[kKeyQueue] = {};
	int keyCount_ = 0;
	WindowStats stats_;
};

} // namespace rt::platform
