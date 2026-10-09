#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <thread>

#include "rt/app/Controls.hpp"
#include "rt/app/Interactive.hpp"
#include "rt/app/Options.hpp"
#include "rt/app/UiOverlay.hpp"
#include "rt/base/Log.hpp"
#include "rt/io/ImageWriter.hpp"
#include "rt/io/Screenshot.hpp"
#include "rt/platform/Window.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/render/Renderer.hpp"
#include "rt/scene/Parser.hpp"
#include "rt/ui/Panel.hpp"

namespace {

constexpr const char* kVersion = "0.1.0";

int printVersion() {
	std::cout << "rt " << kVersion << '\n';
	return 0;
}

int printHelp() {
	std::cout << rt::app::usageText();
	return 0;
}

int printUsageError(const std::string& message) {
	rt::log::error(message);
	std::cerr << rt::app::usageText();
	return 2;
}

// T036 : progression par batches + ETA (stderr, sauf `--quiet`).
// Appele 1x par batch (spp fois max, hors hot path par pixel), `noexcept`
// (R2) et sans allocation dans `render/` (R3, pointeur brut + `void*`).
// L'UI future (T075/T108) branchera le meme callback sur une barre.
struct ProgressClock {
	std::chrono::steady_clock::time_point start{};
};

void onProgressPrint(int done, int total, void* user) noexcept {
	if (user == nullptr || total <= 0 || done <= 0) {
		return;
	}
	const auto* clock = static_cast<const ProgressClock*>(user);
	const auto now = std::chrono::steady_clock::now();
	const double elapsed = std::chrono::duration<double>(now - clock->start).count();
	double eta = 0.0;
	if (done < total) {
		eta = elapsed * static_cast<double>(total - done) / static_cast<double>(done);
	}
	std::fprintf(stderr, "\rprogress: %d/%d (%.0f%%, eta %.1fs)", done, total,
	             100.0 * static_cast<double>(done) / static_cast<double>(total), eta);
	std::fflush(stderr);
	if (done == total) {
		std::fprintf(stderr, "\n");
		std::fflush(stderr);
	}
}

// T026 : ligne de commande complete via `rt::app::parseOptions` (testable).
// T035 : composition root headless — parse -> load -> trace -> write -> exit.
// Aucune init video dans le chemin sans `--window` (R6) : `--out` ecrit et
// sort, `--headless` explicite le mode sans fenetre. L'absence de `DISPLAY`
// n'est donc jamais un echec sans `--window` : ce chemin ne lit meme pas
// cette variable. Succes -> 0, scene/rendu/ecriture -> 1 avec message
// `fichier:ligne:colonne` pour la scene.
// CLI -> usage + 2. `./rt` sans argument -> usage + 2 (README.md).
// T029 : affiche un resume `ok: ...` + `wrote <fichier>` sauf `--quiet`.
// `--help`/`--version` -> 0 sans scene.
// T070 : `--window` sans `--headless` ouvre une fenetre (voir `runWindowed`
// ci-dessous, SDL isole dans `platform/`).
int runWindowed(const rt::app::Options& opts, rt::scene::Scene& scene,
                rt::render::Framebuffer& framebuffer, int width, int height);
int runHeadless(const rt::app::Options& opts) {
	rt::Result<rt::scene::Scene> result = rt::scene::parseFile(opts.scenePath);
	if (result.isError()) {
		rt::log::error(result.status().message);
		return 1;
	}
	rt::scene::Scene& scene = result.value();
	// T032 : boucle de rendu mono-thread, independante de SDL (R6).
	// Resolution/spp/seed : CLI > `limits` (R1). `maxDepth` vient de la
	// scene (aucun `--max-depth` en T026, profondeur bornee pour T056).
	const int width = opts.hasWidth ? opts.width : scene.limits.width;
	const int height = opts.hasHeight ? opts.height : scene.limits.height;
	const int spp = opts.hasSpp ? opts.spp : scene.limits.samples;
	const long long seed = opts.hasSeed ? opts.seed : scene.limits.seed;
	// T063 : `--threads` (1..256 valides en T026, defaut 1 = mono historique).
	const int threads = opts.hasThreads ? opts.threads : 1;
	// T036 : `--spp`/`--seed` -> batches progressifs, reproductibles
	// (meme spp + meme seed = memes pixels). Callback + ETA sur stderr
	// sauf `--quiet` (futur affichage T075/T108).
	ProgressClock clock{std::chrono::steady_clock::now()};
	rt::render::RenderParams params{.width = width,
	                                .height = height,
	                                .spp = spp,
	                                .maxDepth = scene.limits.maxDepth,
	                                .seed = seed,
	                                .threads = threads};
	if (!opts.quiet) {
		params.onProgress = &onProgressPrint;
		params.progressUser = &clock;
	}
	rt::render::Framebuffer framebuffer;
	// T064 : compteurs affiches en fin de rendu (`stderr`, sauf `--quiet`) et
	// prets pour l'UI (T075 : `RenderStats` alimente la barre + rays/s).
	rt::render::RenderStats stats;
	if (rt::Status status = rt::render::render(scene, framebuffer, params, &stats);
	    status.isError()) {
		rt::log::error(status.message);
		return 1;
	}
	if (!opts.quiet) {
		std::fprintf(stderr,
		             "[stats] rays=%lld objects=%d lights=%d threads=%d build=%.1fms "
		             "render=%.1fms total=%.1fms rays/s=%.0f bvhBuilds=%zu\n",
		             stats.primaryRays, stats.objects, stats.lights, stats.threadsUsed,
		             stats.buildMs, stats.renderMs, stats.totalMs, stats.raysPerSec,
		             stats.bvhBuilds);
	}
	// T034 : `--out` -> PNG (libpng) ou PPM (`.ppm` / fallback sans lib).
	// Echec (repertoire inexistant, chemin vide) -> message + 1, sans crash.
	if (opts.hasOut) {
		if (rt::Status status = rt::io::writeImage(framebuffer, opts.outPath); status.isError()) {
			rt::log::error(status.message);
			return 1;
		}
	}
	if (!opts.quiet) {
		std::cout << "ok: " << opts.scenePath << ": " << scene.totalObjectCount() << " objects, "
		          << scene.lights.size() << " lights, " << width << "x" << height << ", spp " << spp
		          << ", threads " << threads;
		if (opts.hasOut) {
			std::cout << ", wrote " << opts.outPath;
		}
		std::cout << '\n';
	}
	// T070 : `--window` sans `--headless` -> montre le tampon persistant.
	// Sans ce fanion, aucun appel video n'a eu lieu ci-dessus (R6).
	if (opts.showWindow && !opts.headless) {
		return runWindowed(opts, scene, framebuffer, width, height);
	}
	return 0;
}

// Parse CLI (T026) puis composition root (T035 + T070).
// T035 : chemin sans fenetre — parse -> load -> trace -> write -> exit.
// Aucune init video dans ce chemin (R6) : `--out` ecrit et sort,
// `--headless` explicite le mode sans fenetre. L'absence de `DISPLAY` n'est
// donc jamais un echec ici : ce chemin ne lit meme pas cette variable.
// T070 : `--window` (sans `--headless`) ouvre une fenetre SDL (RAII),
// y copie le tampon persistant puis boucle d'evenements jusqu'a fermeture.
// T073 : touches clavier (voir `Controls.hpp` + `README.md`) -> fanions R5
// puis re-trace + reblit uniquement si la scene a change (jamais gratuit).
// `--headless` l'emporte sur `--window` (repli sans reseau ni ecran, T078).
int runWindowed(const rt::app::Options& opts, rt::scene::Scene& scene,
                rt::render::Framebuffer& framebuffer, int width, int height) {
	// T078 : sans ecran, echec propre immediat (pas de blocage, code 1).
	// `SDL_VIDEODRIVER=dummy` reste accepte pour les tests (pilote factice).
	if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr &&
	    std::getenv("SDL_VIDEODRIVER") == nullptr) {
		rt::log::error("no DISPLAY: cannot open window (use --headless or --out)");
		return 1;
	}
	rt::platform::Window window;
	if (rt::Status status = window.init(width, height, opts.scenePath.c_str()); status.isError()) {
		rt::log::error(status.message);
		return 1;
	}
	// Fix microui : overlay SDL (rects + texte TTF) compose par-dessus le
	// framebuffer en un seul `present` (froid, 1x/frame, hors hot path).
	// Echec police -> mode degrade (rects cliquables, texte ignore).
	rt::app::UiOverlay overlay;
	(void)overlay.init(window.nativeRenderer());
	// T075 : panneau microui attache (champs issus de `schema/`, sliders
	// FOV/ambiance + boutons Render/Save ; dessin SDL minimal, logique testee).
	rt::ui::Panel panel;
	panel.attach(&scene);
	// Premier frame : remplit la liste `mu_Command` avant le 1er present.
	panel.frame();
	window.updateTexture(framebuffer);
	if (window.beginPresent()) {
		overlay.draw(panel.nativeContext());
		window.endPresent();
	}
	if (!opts.quiet) {
		std::cout << "window: " << width << "x" << height << " (close to quit)\n";
	}
	const int spp = opts.hasSpp ? opts.spp : scene.limits.samples;
	const long long seed = opts.hasSeed ? opts.seed : scene.limits.seed;
	const int threads = opts.hasThreads ? opts.threads : 1;
	// T108 (*Environment 1*) : la callback de progression alimente a la fois
	// le panneau (barre `done/total`, 1x par batch spp) et le terminal
	// (meme format qu'en headless, sauf `--quiet`). `noexcept` (R2).
	struct WindowProgress {
		rt::ui::Panel* panel = nullptr;
		bool quiet = false;
	};
	WindowProgress windowProgress{&panel, opts.quiet};
	auto onWindowProgress = [](int done, int total, void* user) noexcept {
		auto* progress = static_cast<WindowProgress*>(user);
		if (progress == nullptr || progress->panel == nullptr) {
			return;
		}
		progress->panel->setProgress(done, total);
		if (!progress->quiet && total > 0 && done > 0) {
			std::fprintf(stderr, "\r[window] progress: %d/%d (%d%%)", done, total,
			             (done * 100) / total);
			std::fflush(stderr);
			if (done == total) {
				std::fprintf(stderr, "\n");
				std::fflush(stderr);
			}
		}
	};
	// T076 : machine preview (1 spp) puis affinage (spp cible), blit seul
	// sinon. Le calcul reste en tuiles `ThreadPool` (pas de blocage UI).
	rt::app::Interactive ctl;
	scene.markClean();
	long long rerenders = 0;
	while (!window.pollQuit()) {
		// Survol UI du frame precedent : le drag va au slider, pas a l'orbite.
		const bool uiHover = panel.isHovering();
		int sdlKey = 0;
		bool changed = false;
		bool wantShot = false;
		while (window.pollKey(sdlKey)) {
			// T077 : `P` = capture (pas de re-trace, tampon courant).
			if (sdlKey == 'p' || sdlKey == 'P') {
				wantShot = true;
				continue;
			}
			const rt::app::KeyAction action = rt::app::keyFromSdl(sdlKey);
			if (action == rt::app::KeyAction::None) {
				continue;
			}
			if (rt::app::applyKeyAction(scene, action)) {
				changed = true;
			}
		}
		// T074 : souris (orbite + molette FOV), en direct, meme fanions R5.
		// Si l'UI est survolee, la molette/drag vont a microui (scroll /
		// slider), pas a la camera.
		int mdx = 0;
		int mdy = 0;
		int mwheel = 0;
		bool mouseMoved = window.pollMouse(mdx, mdy, mwheel);
		int uiWheelX = 0;
		int uiWheelY = 0;
		if (mouseMoved) {
			if (uiHover) {
				uiWheelX = 0;
				uiWheelY = mwheel;
			} else {
				if ((mdx != 0 || mdy != 0) && rt::app::orbitCamera(scene, mdx, mdy)) {
					changed = true;
				}
				if (mwheel != 0 && rt::app::adjustFov(scene, mwheel)) {
					changed = true;
				}
			}
		}
		// Fix microui : transfere la file SDL -> `mu_input_*` avant `frame()`.
		// `pollUiEvent` pompe deja ; les residus molette (hors hover) sont
		// aussi transmis comme scroll (inoffensif hors fenetre UI).
		rt::platform::UiEvent uiEv;
		while (window.pollUiEvent(uiEv)) {
			if (uiEv.type == rt::platform::UiEvent::Move) {
				panel.handleMouseMove(uiEv.x, uiEv.y);
			} else if (uiEv.type == rt::platform::UiEvent::Down) {
				panel.handleMouseMove(uiEv.x, uiEv.y);
				panel.handleMouseDown(uiEv.x, uiEv.y, uiEv.button);
			} else if (uiEv.type == rt::platform::UiEvent::Up) {
				panel.handleMouseMove(uiEv.x, uiEv.y);
				panel.handleMouseUp(uiEv.x, uiEv.y, uiEv.button);
			} else if (uiEv.type == rt::platform::UiEvent::Wheel) {
				panel.handleScroll(uiEv.wheelX, uiEv.wheelY);
			}
		}
		if (mouseMoved && uiHover && (uiWheelX != 0 || uiWheelY != 0)) {
			panel.handleScroll(uiWheelX, uiWheelY);
		}
		// T075/T076 : l'UI fait avancer ses sliders chaque frame ; si elle a
		// leve R5, c'est une edition comme les autres (appercu puis affinage).
		panel.frame();
		if (panel.takeLaunchRequest()) {
			changed = true;
		}
		if (panel.takeSaveRequest()) {
			wantShot = true;
		}
		// T077 : capture (tampon courant, horodaté, sans recalcul).
		if (wantShot) {
			const char* envDir = std::getenv("RT_SCREENSHOT_DIR");
			const std::string shotDir =
			    (envDir != nullptr && envDir[0] != '\0') ? envDir : "docs/preuves";
			rt::Result<std::string> saved = rt::io::saveScreenshot(framebuffer, shotDir);
			if (saved.isOk()) {
				std::cout << "saved " << saved.value() << '\n';
			} else {
				rt::log::error(saved.status().message);
			}
		}
		if (scene.sceneDirty && !changed) {
			changed = true;
		}
		if (changed) {
			ctl.onEdited();
		}
		if (ctl.needsPreview()) {
			rt::render::RenderParams preview{.width = width,
			                                 .height = height,
			                                 .spp = 1,
			                                 .maxDepth = scene.limits.maxDepth,
			                                 .seed = seed,
			                                 .threads = threads,
			                                 .onProgress = onWindowProgress,
			                                 .progressUser = &windowProgress};
			rt::render::RenderStats stats;
			if (rt::render::render(scene, framebuffer, preview, &stats).isOk()) {
				window.updateTexture(framebuffer);
				ctl.consumePreview();
				ctl.onPreviewDone();
				if (!opts.quiet) {
					std::fprintf(stderr, "[preview] 1 spp blit\n");
				}
			}
			scene.markClean();
		}
		if (ctl.needsFull()) {
			rt::render::RenderParams params{.width = width,
			                                .height = height,
			                                .spp = spp,
			                                .maxDepth = scene.limits.maxDepth,
			                                .seed = seed,
			                                .threads = threads,
			                                .onProgress = onWindowProgress,
			                                .progressUser = &windowProgress};
			rt::render::RenderStats stats;
			if (rt::render::render(scene, framebuffer, params, &stats).isOk()) {
				window.updateTexture(framebuffer);
				++rerenders;
				ctl.consumeFull();
				ctl.onFullDone();
				if (!opts.quiet) {
					std::fprintf(stderr, "[keys] rerender #%lld (dirty)\n", rerenders);
				}
			}
			scene.markClean();
		}
		// Fix microui : compose `framebuffer + UI` en un seul `present`
		// chaque frame (hover/slider fluide meme sans re-trace).
		if (window.beginPresent()) {
			overlay.draw(panel.nativeContext());
			window.endPresent();
		}
		// T075 : fait avancer l'UI (sliders/boutons) chaque frame, sans bloquer.
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
	return 0;
}

// Parse CLI (T026) puis composition root headless (T035). Par defaut tout
// appel avec scene passe par `runHeadless` : `--headless` ou non, avec ou
// sans `--out`, avec ou sans `DISPLAY` — aucun chemin n'ouvre de fenetre
// sauf `--window` sans `--headless` (T070, SDL isole dans `platform/`).
int run(int argc, char** argv) {
	rt::Result<rt::app::Options> parsed = rt::app::parseOptions(argc, argv);
	if (parsed.isError()) {
		return printUsageError(parsed.status().message);
	}
	const rt::app::Options& opts = parsed.value();
	if (opts.showHelp) {
		return printHelp();
	}
	if (opts.showVersion) {
		return printVersion();
	}
	return runHeadless(opts);
}

} // namespace

// Filet unique (règle R2, T015) : seul `try/catch` autorisé du dépôt.
// Tout le hot path rapporte par `rt::Status`/`bool`, jamais par exception.
int main(int argc, char** argv) {
	try {
		return run(argc, argv);
	} catch (const std::exception& e) {
		rt::log::error(e.what());
		return 1;
	} catch (...) {
		rt::log::error("unknown exception");
		return 1;
	}
}
