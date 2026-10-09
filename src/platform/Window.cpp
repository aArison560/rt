// Couche plateforme SDL (T070) — implementation RAII sans lien vers le moteur.
// Voir `include/rt/platform/Window.hpp` pour le contrat.
// Seule `Framebuffer` est lue (affichage), jamais la scene ni le moteur.
// Le chemin sans fenetre n'appelle jamais ce fichier (garantie verifiee par
// l'absence de SDL dans `runHeadless`, cf. `src/app/main.cpp`).

#include "rt/platform/Window.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <SDL.h>

#include "rt/render/Framebuffer.hpp"

namespace rt::platform {

namespace {

// Titre par defaut quand `title` est nul ou vide.
constexpr const char* kDefaultTitle = "rt";
// Format de texture correspondant a `Rgba8 { r, g, b, a }` en memoire
// (octets r,g,b,a -> mot 0xAABBGGRR en petit-boutiste, cf. SDL docs).
constexpr std::uint32_t kTextureFormat = SDL_PIXELFORMAT_ABGR8888;

[[nodiscard]] Status failIo(const std::string& detail, int line) {
	return Status::error(StatusCode::IoError, detail, line);
}

} // namespace

Window::~Window() { shutdown(); }

Status Window::init(int width, int height, const char* title) {
	if (open_) {
		return Status::error(StatusCode::InvalidArgument, "window already open", __LINE__);
	}
	if (width < 1 || height < 1 || width > 8192 || height > 8192) {
		return Status::error(StatusCode::InvalidArgument, "bad window size: expected 1..8192",
		                     __LINE__);
	}
	// T078 : taille minimale d'affichage 64x64.
	if (width < 64 || height < 64) {
		return Status::error(StatusCode::InvalidArgument, "window too small: min 64x64", __LINE__);
	}
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::string msg("SDL_Init failed: ");
		msg += SDL_GetError();
		if (std::getenv("DISPLAY") == nullptr) {
			msg += " (no DISPLAY: use --headless or --out for headless mode)";
		}
		return failIo(msg, __LINE__);
	}
	const char* shown = (title != nullptr && title[0] != '\0') ? title : kDefaultTitle;
	SDL_Window* win = SDL_CreateWindow(shown, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
	                                   height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (win == nullptr) {
		std::string msg("SDL_CreateWindow failed: ");
		msg += SDL_GetError();
		SDL_Quit();
		return failIo(msg, __LINE__);
	}
	SDL_SetWindowMinimumSize(win, 64, 64);
	SDL_Renderer* ren =
	    SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr) {
		ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	}
	if (ren == nullptr) {
		std::string msg("SDL_CreateRenderer failed: ");
		msg += SDL_GetError();
		SDL_DestroyWindow(win);
		SDL_Quit();
		return failIo(msg, __LINE__);
	}
	SDL_Texture* tex =
	    SDL_CreateTexture(ren, kTextureFormat, SDL_TEXTUREACCESS_STATIC, width, height);
	if (tex == nullptr) {
		std::string msg("SDL_CreateTexture failed: ");
		msg += SDL_GetError();
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		SDL_Quit();
		return failIo(msg, __LINE__);
	}
	window_ = win;
	renderer_ = ren;
	texture_ = tex;
	width_ = width;
	height_ = height;
	texWidth_ = width;
	texHeight_ = height;
	open_ = true;
	quitSeen_ = false;
	keyCount_ = 0;
	dragging_ = false;
	accumDx_ = 0;
	accumDy_ = 0;
	accumWheel_ = 0;
	mouseX_ = 0;
	mouseY_ = 0;
	leftDown_ = false;
	uiHead_ = 0;
	uiCount_ = 0;
	stats_ = WindowStats{};
	return Status::ok();
}

void Window::shutdown() noexcept {
	if (!open_) {
		return;
	}
	if (texture_ != nullptr) {
		SDL_DestroyTexture(static_cast<SDL_Texture*>(texture_));
		texture_ = nullptr;
	}
	if (renderer_ != nullptr) {
		SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
		renderer_ = nullptr;
	}
	if (window_ != nullptr) {
		SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
		window_ = nullptr;
	}
	SDL_Quit();
	open_ = false;
	quitSeen_ = false;
	keyCount_ = 0;
	dragging_ = false;
	accumDx_ = 0;
	accumDy_ = 0;
	accumWheel_ = 0;
	mouseX_ = 0;
	mouseY_ = 0;
	leftDown_ = false;
	uiHead_ = 0;
	uiCount_ = 0;
	width_ = 0;
	height_ = 0;
	texWidth_ = 0;
	texHeight_ = 0;
}

void Window::updateTexture(const render::Framebuffer& fb) noexcept {
	if (!open_ || renderer_ == nullptr || texture_ == nullptr) {
		return;
	}
	const int fw = fb.width();
	const int fh = fb.height();
	if (fw <= 0 || fh <= 0 || fb.displayData() == nullptr) {
		return;
	}
	// Resolution differente -> recree la texture (chemin froid, hors boucle).
	if (fw != texWidth_ || fh != texHeight_) {
		SDL_DestroyTexture(static_cast<SDL_Texture*>(texture_));
		SDL_Texture* fresh = SDL_CreateTexture(static_cast<SDL_Renderer*>(renderer_),
		                                       kTextureFormat, SDL_TEXTUREACCESS_STATIC, fw, fh);
		if (fresh == nullptr) {
			return;
		}
		texture_ = fresh;
		texWidth_ = fw;
		texHeight_ = fh;
	}
	const auto* pixels = fb.displayData();
	SDL_UpdateTexture(static_cast<SDL_Texture*>(texture_), nullptr, pixels, fw * 4);
}

bool Window::beginPresent() noexcept {
	if (!open_ || renderer_ == nullptr || texture_ == nullptr) {
		return false;
	}
	SDL_RenderClear(static_cast<SDL_Renderer*>(renderer_));
	SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(texture_),
	               nullptr, nullptr);
	return true;
}

void Window::endPresent() noexcept {
	if (!open_ || renderer_ == nullptr) {
		return;
	}
	SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

void Window::blit(const render::Framebuffer& fb) noexcept {
	if (!open_ || renderer_ == nullptr || texture_ == nullptr) {
		return;
	}
	updateTexture(fb);
	if (!beginPresent()) {
		return;
	}
	endPresent();
	++stats_.blitCount;
}

void Window::presentCached() noexcept {
	if (!open_ || renderer_ == nullptr || texture_ == nullptr) {
		return;
	}
	const auto start = std::chrono::steady_clock::now();
	SDL_RenderClear(static_cast<SDL_Renderer*>(renderer_));
	SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(texture_),
	               nullptr, nullptr);
	SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
	const auto stop = std::chrono::steady_clock::now();
	const long long us =
	    std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
	stats_.lastBlitUs = us;
	++stats_.exposeCount;
	std::fprintf(stderr, "[expose] blit in %lld us\n", us);
	std::fflush(stderr);
}

bool Window::pollQuit() noexcept {
	if (!open_) {
		return false;
	}
	pumpEvents();
	return quitSeen_;
}

bool Window::pollExpose() noexcept {
	if (!open_) {
		return false;
	}
	bool saw = false;
	SDL_Event ev;
	while (SDL_PollEvent(&ev) != 0) {
		if (ev.type == SDL_QUIT) {
			// Laisse `pollQuit` le voir au prochain appel : re-empile.
			SDL_PushEvent(&ev);
			break;
		}
		if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
			saw = true;
		}
	}
	if (saw) {
		presentCached();
	}
	return saw;
}

bool Window::pollKey(int& outSdlKey) noexcept {
	outSdlKey = 0;
	if (!open_) {
		return false;
	}
	pumpEvents();
	if (keyCount_ <= 0) {
		return false;
	}
	outSdlKey = keyQueue_[0];
	for (int i = 1; i < keyCount_; ++i) {
		keyQueue_[i - 1] = keyQueue_[i];
	}
	--keyCount_;
	return true;
}

void Window::pumpEvents() noexcept {
	if (!open_) {
		return;
	}
	SDL_Event ev;
	while (SDL_PollEvent(&ev) != 0) {
		if (ev.type == SDL_QUIT) {
			quitSeen_ = true;
		} else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
			presentCached();
		} else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED) {
			// T071/T078 : recopie proportionnelle — la texture existante est
			// etiree par `SDL_RenderCopy` (aucun nouveau calcul, marque ici).
			presentCached();
		} else if (ev.type == SDL_KEYDOWN) {
			if (keyCount_ < kKeyQueue) {
				keyQueue_[keyCount_++] = static_cast<int>(ev.key.keysym.sym);
			}
		} else if (ev.type == SDL_MOUSEMOTION) {
			mouseX_ = static_cast<int>(ev.motion.x);
			mouseY_ = static_cast<int>(ev.motion.y);
			if (dragging_) {
				accumDx_ += static_cast<int>(ev.motion.xrel);
				accumDy_ += static_cast<int>(ev.motion.yrel);
			}
			UiEvent out;
			out.type = UiEvent::Move;
			out.x = mouseX_;
			out.y = mouseY_;
			pushUiEvent(out);
		} else if (ev.type == SDL_MOUSEBUTTONDOWN) {
			mouseX_ = static_cast<int>(ev.button.x);
			mouseY_ = static_cast<int>(ev.button.y);
			int btn = 0;
			if (ev.button.button == SDL_BUTTON_LEFT) {
				dragging_ = true;
				leftDown_ = true;
				btn = 1;
			} else if (ev.button.button == SDL_BUTTON_RIGHT) {
				btn = 2;
			} else if (ev.button.button == SDL_BUTTON_MIDDLE) {
				btn = 3;
			}
			if (btn != 0) {
				UiEvent out;
				out.type = UiEvent::Down;
				out.x = mouseX_;
				out.y = mouseY_;
				out.button = btn;
				pushUiEvent(out);
			}
		} else if (ev.type == SDL_MOUSEBUTTONUP) {
			mouseX_ = static_cast<int>(ev.button.x);
			mouseY_ = static_cast<int>(ev.button.y);
			int btn = 0;
			if (ev.button.button == SDL_BUTTON_LEFT) {
				dragging_ = false;
				leftDown_ = false;
				btn = 1;
			} else if (ev.button.button == SDL_BUTTON_RIGHT) {
				btn = 2;
			} else if (ev.button.button == SDL_BUTTON_MIDDLE) {
				btn = 3;
			}
			if (btn != 0) {
				UiEvent out;
				out.type = UiEvent::Up;
				out.x = mouseX_;
				out.y = mouseY_;
				out.button = btn;
				pushUiEvent(out);
			}
		} else if (ev.type == SDL_MOUSEWHEEL) {
			accumWheel_ += static_cast<int>(ev.wheel.y);
			UiEvent out;
			out.type = UiEvent::Wheel;
			out.x = mouseX_;
			out.y = mouseY_;
			out.wheelX = static_cast<int>(ev.wheel.x);
			out.wheelY = static_cast<int>(ev.wheel.y);
			pushUiEvent(out);
		}
	}
}

void Window::pushUiEvent(const UiEvent& ev) noexcept {
	if (uiCount_ >= kUiQueue) {
		return;
	}
	const int idx = (uiHead_ + uiCount_) % kUiQueue;
	uiQueue_[idx] = ev;
	++uiCount_;
}

bool Window::pollUiEvent(UiEvent& out) noexcept {
	out = UiEvent{};
	if (!open_) {
		return false;
	}
	pumpEvents();
	if (uiCount_ <= 0) {
		return false;
	}
	out = uiQueue_[uiHead_];
	uiHead_ = (uiHead_ + 1) % kUiQueue;
	--uiCount_;
	return out.type != UiEvent::None;
}

void Window::uiMouseState(int& x, int& y, bool& leftDown) const noexcept {
	x = mouseX_;
	y = mouseY_;
	leftDown = leftDown_;
}

bool Window::pollMouse(int& dx, int& dy, int& wheel) noexcept {
	dx = 0;
	dy = 0;
	wheel = 0;
	if (!open_) {
		return false;
	}
	pumpEvents();
	dx = accumDx_;
	dy = accumDy_;
	wheel = accumWheel_;
	accumDx_ = 0;
	accumDy_ = 0;
	accumWheel_ = 0;
	return (dx != 0 || dy != 0 || wheel != 0);
}

} // namespace rt::platform
