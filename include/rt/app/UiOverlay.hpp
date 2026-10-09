#pragma once

// Overlay microui -> SDL (fix affichage, `app/` composition root).
// `Window` (platform) ne connait pas microui (R-sous-couche) et `Panel`
// (ui) ne connait pas SDL : c'est ici, au-dessus des deux, que les
// commandes `mu_Command` deviennent des `SDL_Rect/FillRect/Copy`.
// Chemin froid uniquement (1x/frame fenetre) : jamais dans le hot path
// `render/` (R2/R3 intactes). Sans police TTF -> rectangles seuls
// (panneau visible, texte degrade mais cliquable).

namespace rt::app {

class UiOverlay {
  public:
	UiOverlay() = default;
	~UiOverlay();
	UiOverlay(const UiOverlay&) = delete;
	UiOverlay& operator=(const UiOverlay&) = delete;
	UiOverlay(UiOverlay&&) = delete;
	UiOverlay& operator=(UiOverlay&&) = delete;

	// Ouvre la police (DejaVu/Noto via `RT_FONT` sinon defauts systeme).
	// `renderer` non possede (doit survivre). Faux -> mode degrade
	// (rectangles + icones, sans texte) mais jamais de crash.
	[[nodiscard]] bool init(void* renderer);
	void shutdown() noexcept;
	[[nodiscard]] bool ready() const noexcept { return font_ != nullptr; }

	// Dessine le contexte microui (`Panel::nativeContext()`) sur le
	// renderer courant (entre `Window::beginPresent()` et `endPresent()`).
	// Nul -> sans effet. Sans police -> texte ignore (rects gardes).
	void draw(void* muCtx) noexcept;

  private:
	void* renderer_ = nullptr;
	void* font_ = nullptr;
};

} // namespace rt::app
