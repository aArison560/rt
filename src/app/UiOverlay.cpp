// Overlay microui -> SDL (fix affichage) — implementation `app/`.
// Pont autorise : `app` voit `platform` (SDL) et `ui` (microui).
// Froid uniquement (1x/frame) : allocations TTF/Surface bornees au frame
// et liberees aussitot (pas de fuite, hors hot path `render/`).

#include "rt/app/UiOverlay.hpp"

#include <array>
#include <cstdlib>

#include <SDL.h>

extern "C" {
#include "microui/microui.h"
}

#if __has_include(<SDL_ttf.h>)
#include <SDL_ttf.h>
#define RT_HAVE_TTF 1
#else
#define RT_HAVE_TTF 0
#endif

namespace rt::app {

namespace {

#if RT_HAVE_TTF
// Polices systeme courantes (Debian/Ubuntu) : la premiere lisible gagne.
// `RT_FONT` permet de forcer un chemin (tests, postes exotiques).
const char* fontCandidates() noexcept {
	const char* env = std::getenv("RT_FONT");
	if (env != nullptr && env[0] != '\0') {
		return env;
	}
	return nullptr;
}

void* openSystemFont() noexcept {
	const char* forced = fontCandidates();
	if (forced != nullptr) {
		TTF_Font* f = TTF_OpenFont(forced, 13);
		if (f != nullptr) {
			return f;
		}
	}
	static constexpr std::array<const char*, 4> kPaths = {
	    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
	    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
	    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
	    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
	};
	for (const char* path : kPaths) {
		TTF_Font* f = TTF_OpenFont(path, 13);
		if (f != nullptr) {
			return f;
		}
	}
	return nullptr;
}
#endif

void drawRectCmd(SDL_Renderer* ren, const mu_RectCommand& cmd) noexcept {
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
	const SDL_Rect rc{cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h};
	if (rc.w > 0 && rc.h > 0) {
		SDL_RenderFillRect(ren, &rc);
	}
}

void drawIconCmd(SDL_Renderer* ren, const mu_IconCommand& cmd) noexcept {
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(ren, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
	const int x = cmd.rect.x;
	const int y = cmd.rect.y;
	const int w = cmd.rect.w;
	const int h = cmd.rect.h;
	if (w <= 0 || h <= 0) {
		return;
	}
	if (cmd.id == MU_ICON_CLOSE) {
		SDL_RenderDrawLine(ren, x, y, x + w, y + h);
		SDL_RenderDrawLine(ren, x + w, y, x, y + h);
	} else if (cmd.id == MU_ICON_CHECK) {
		SDL_RenderDrawLine(ren, x, y + h / 2, x + w / 3, y + h - 2);
		SDL_RenderDrawLine(ren, x + w / 3, y + h - 2, x + w, y);
	} else {
		// COLLAPSED / EXPANDED : petit triangle.
		const int cx = x + w / 2;
		const int cy = y + h / 2;
		if (cmd.id == MU_ICON_COLLAPSED) {
			SDL_RenderDrawLine(ren, cx - 3, cy - 4, cx - 3, cy + 4);
			SDL_RenderDrawLine(ren, cx - 3, cy - 4, cx + 3, cy);
			SDL_RenderDrawLine(ren, cx - 3, cy + 4, cx + 3, cy);
		} else {
			SDL_RenderDrawLine(ren, cx - 4, cy - 2, cx + 4, cy - 2);
			SDL_RenderDrawLine(ren, cx - 4, cy - 2, cx, cy + 3);
			SDL_RenderDrawLine(ren, cx + 4, cy - 2, cx, cy + 3);
		}
	}
}

void drawTextFallback(SDL_Renderer* ren, const mu_TextCommand& cmd) noexcept {
	// Sans police : pastille de fond pour garder le layout visible.
	// Le panneau reste cliquable (sliders/boutons par rectangles).
	(void)ren;
	(void)cmd;
}

} // namespace

UiOverlay::~UiOverlay() { shutdown(); }

bool UiOverlay::init(void* renderer) {
	if (renderer == nullptr) {
		return false;
	}
	renderer_ = renderer;
#if RT_HAVE_TTF
	if (TTF_WasInit() == 0) {
		if (TTF_Init() != 0) {
			renderer_ = renderer;
			font_ = nullptr;
			return false;
		}
	}
	font_ = openSystemFont();
	return font_ != nullptr;
#else
	font_ = nullptr;
	return false;
#endif
}

void UiOverlay::shutdown() noexcept {
#if RT_HAVE_TTF
	if (font_ != nullptr) {
		TTF_CloseFont(static_cast<TTF_Font*>(font_));
		font_ = nullptr;
	}
	// Pas de `TTF_Quit()` ici : `WasInit` est global au process et le
	// chemin headless/valgrind (`--version`) ne l'initialise jamais.
	// Laisser le runtime liberer a la sortie evite un double-quit.
#endif
	renderer_ = nullptr;
}

void UiOverlay::draw(void* muCtx) noexcept {
	if (renderer_ == nullptr || muCtx == nullptr) {
		return;
	}
	auto* ren = static_cast<SDL_Renderer*>(renderer_);
	auto* ctx = static_cast<mu_Context*>(muCtx);
	mu_Command* cmd = nullptr;
	while (mu_next_command(ctx, &cmd) != 0) {
		if (cmd->type == MU_COMMAND_CLIP) {
			const mu_Rect rc = cmd->clip.rect;
			if (rc.w >= 0x100000 || rc.h >= 0x100000 || (rc.w <= 0 || rc.h <= 0)) {
				SDL_RenderSetClipRect(ren, nullptr);
			} else {
				const SDL_Rect sdlRc{rc.x, rc.y, rc.w, rc.h};
				SDL_RenderSetClipRect(ren, &sdlRc);
			}
		} else if (cmd->type == MU_COMMAND_RECT) {
			drawRectCmd(ren, cmd->rect);
		} else if (cmd->type == MU_COMMAND_ICON) {
			drawIconCmd(ren, cmd->icon);
		} else if (cmd->type == MU_COMMAND_TEXT) {
			const char* str = cmd->text.str;
			if (str == nullptr || str[0] == '\0') {
				continue;
			}
#if RT_HAVE_TTF
			if (font_ == nullptr) {
				drawTextFallback(ren, cmd->text);
				continue;
			}
			auto* font = static_cast<TTF_Font*>(font_);
			const SDL_Color col{cmd->text.color.r, cmd->text.color.g, cmd->text.color.b,
			                    cmd->text.color.a};
			SDL_Surface* surf = TTF_RenderUTF8_Blended(font, str, col);
			if (surf == nullptr) {
				continue;
			}
			SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
			if (tex == nullptr) {
				SDL_FreeSurface(surf);
				continue;
			}
			const SDL_Rect dst{cmd->text.pos.x, cmd->text.pos.y, surf->w, surf->h};
			SDL_RenderCopy(ren, tex, nullptr, &dst);
			SDL_DestroyTexture(tex);
			SDL_FreeSurface(surf);
#else
			drawTextFallback(ren, cmd->text);
#endif
		}
	}
	SDL_RenderSetClipRect(ren, nullptr);
}

} // namespace rt::app
