#include "core/Scene.hpp"
#include "core/Renderer.hpp"
#include <SDL.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <vector>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Modes Blender-like : G = grab objet, R = rotate dir, C = caméra, L = lumière.
enum class Mode { Idle, Grab, Rotate, Cam, LightM };
enum class Axis { Free, X, Y, Z };

static bool	has_rt_extension(const char *path){
	size_t	len = strlen(path);
	if (len < 3) return false;
	return (strcmp(path + len - 3, ".rt") == 0);
}

// Base caméra anti-miroir (même convention que Renderer) : right = up×forward.
static void	camBasis(const Scene& s, Vec3& right, Vec3& up, Vec3& fwd){
	fwd = s.cam.dir.normalized();
	Vec3	wup{0,1,0};
	if (std::fabs(fwd.dot(wup)) > 0.999) wup = Vec3{0,0,1};
	right = wup.cross(fwd).normalized();
	up = fwd.cross(right).normalized();
}

// Rodrigues : tourne v autour de l'axe k (unitaire) d'un angle a (radians).
static Vec3	rotAxis(const Vec3& v, const Vec3& k, double a){
	double	c = std::cos(a), s = std::sin(a);
	return v * c + k.cross(v) * s + k * (k.dot(v) * (1 - c));
}

// Nom court du type d'objet pour l'aide / titre.
static const char*	objName(const Object* o){
	if (dynamic_cast<const Sphere*>(o)) return "sp";
	if (dynamic_cast<const Plane*>(o)) return "pl";
	if (dynamic_cast<const Cylinder*>(o)) return "cyl";
	if (dynamic_cast<const Cone*>(o)) return "cone";
	if (dynamic_cast<const Paraboloid*>(o)) return "parab";
	if (dynamic_cast<const Hyperboloid*>(o)) return "hyp";
	return "?";
}

static void	printHelp(size_t sel, size_t n){
	std::cout << "--- Blender-like ---\n"
		<< "Clic : select objet au curseur | 0-9/Tab : select (" << sel << "/" << n << ") | G : grab | R : rotate dir\n"
		<< "C : camera (souris=pan, molette=avant/arriere) | L : lumiere\n"
		<< "X/Y/Z : contrainte axe monde | U : libre (plan camera)\n"
		<< "Fleches : nudge 0.1 (Shift=0.01) | Molette en G/L : profondeur\n"
		<< "Entree/Space/Clic : valider | ESC : annuler | Q/ESC(idle) : quitter\n"
		<< "F : rendu net | H : aide\n";
}

static void	setTitle(SDL_Window* win, Mode m, Axis ax, size_t sel, size_t n, bool preview){
	const char*	ms = m == Mode::Idle ? "idle" : m == Mode::Grab ? "GRAB" : m == Mode::Rotate ? "ROTATE" : m == Mode::Cam ? "CAM" : "LIGHT";
	const char*	as = ax == Axis::Free ? "free" : ax == Axis::X ? "X" : ax == Axis::Y ? "Y" : "Z";
	char t[128];
	snprintf(t, sizeof(t), "RTv1 [%s|%s] obj %zu/%zu%s", ms, as, sel, n, preview ? " (preview)" : "");
	SDL_SetWindowTitle(win, t);
}

// Picking curseur : rayon à travers le pixel (x,y) -> index objet le plus proche, -1 si ciel.
// Même maths que Renderer (centre pixel, NDC -> caméra), O(n) objets, sans alloc.
static int	pickObject(const Scene& s, int x, int y){
	if (x < 0 || y < 0 || x >= s.width || y >= s.height) return -1;
	Vec3	right, up, fwd;
	camBasis(s, right, up, fwd);
	double	aspect = (double)s.width / s.height;
	double	scale = std::tan(s.cam.fov * M_PI / 180.0 * 0.5);
	double	nx = (2.0 * (x + 0.5) / s.width - 1) * aspect * scale;
	double	ny = (1 - 2.0 * (y + 0.5) / s.height) * scale;
	Ray		ray{s.cam.pos, (right * nx + up * ny + fwd).normalized()};
	int		best = -1;
	double	bestT = 1e30;
	Hit		tmp;
	for (size_t i = 0; i < s.objects.size(); i++){
		if (s.objects[i]->intersect(ray, tmp) && tmp.t < bestT){ bestT = tmp.t; best = (int)i; }
	}
	return best;
}

int	main(int argc, char **argv){
	bool	once = false; // --once : 1 rendu puis quitte (scripts, leak_test)
	const char*	rt = nullptr;
	for (int i = 1; i < argc; i++){
		if (strcmp(argv[i], "--once") == 0) once = true;
		else if (!rt) rt = argv[i];
	}
	if (!rt){ std::cerr << "Usage: " << argv[0] << " <scene.rt> [--once]" << std::endl; return 1; }
	if (!has_rt_extension(rt)){ std::cerr << "Error: extension .rt requise (" << rt << ")" << std::endl; return 1; }

	Scene	scene;
	if (parse_scene(rt, &scene) != 0){ std::cerr << "Error: parsing '" << rt << "'" << std::endl; return 1; }
	std::cout << "Scene: " << rt << " | Objets: " << scene.objects.size()
		<< " | Cam fov=" << scene.cam.fov << " | " << scene.width << "x" << scene.height << std::endl;
	for (size_t i = 0; i < scene.objects.size(); i++)
		std::cout << "  [" << i << "] " << objName(scene.objects[i].get())
			<< " pos " << scene.objects[i]->getPos().x << "," << scene.objects[i]->getPos().y << "," << scene.objects[i]->getPos().z << std::endl;

	if (SDL_Init(SDL_INIT_VIDEO) < 0){ std::cerr << "SDL_Init: " << SDL_GetError() << std::endl; return 1; }
	SDL_Window*	window = SDL_CreateWindow(rt, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, scene.width, scene.height, SDL_WINDOW_SHOWN);
	if (!window){ std::cerr << "SDL_CreateWindow: " << SDL_GetError() << std::endl; SDL_Quit(); return 1; }
	SDL_Renderer*	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE); // dummy/headless
	if (!renderer){ std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << std::endl; SDL_DestroyWindow(window); SDL_Quit(); return 1; }
	SDL_Texture*	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, scene.width, scene.height);
	if (!texture){ std::cerr << "SDL_CreateTexture: " << SDL_GetError() << std::endl; SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

	std::vector<uint32_t>	buffer(scene.width * scene.height);
	const int	PREVIEW_STEP = 3; // 9x plus vite : ~25 img/s pendant le drag
	const Uint32	FULL_DELAY = 400; // ms sans event -> rendu net auto

	auto	doRender = [&](bool preview){
		if (preview) Renderer::renderPreview(scene, buffer.data(), PREVIEW_STEP);
		else Renderer::render(scene, buffer.data());
		SDL_UpdateTexture(texture, NULL, buffer.data(), scene.width * sizeof(uint32_t));
	};

	printHelp(0, scene.objects.size());
	Mode	mode = Mode::Idle;
	Axis	axis = Axis::Free;
	size_t	sel = 0;
	Vec3	backupPos{0,0,0}, backupDir{0,0,1}, backupLight{0,0,0}, backupCam{0,0,0};
	bool	hasBackup = false;
	bool	dirty = true, preview = false;
	Uint32	lastChange = SDL_GetTicks();
	bool	running = true;
	bool	shiftDown = false;

	if (once){ // scripts : un seul rendu net, pas d'interactif
		doRender(false);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(500);
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		std::cout << "RTv1 --once OK '" << rt << "'" << std::endl;
		return 0;
	}

	// Backup avant chaque modal (ESC restaure).
	auto	beginModal = [&](Mode m){
		mode = m; axis = Axis::Free; hasBackup = true;
		if (m == Mode::Grab || m == Mode::Rotate){
			if (sel < scene.objects.size()){ backupPos = scene.objects[sel]->getPos(); backupDir = scene.objects[sel]->getDir(); }
		} else if (m == Mode::LightM) backupLight = scene.light.pos;
		else if (m == Mode::Cam) backupCam = scene.cam.pos;
	};
	auto	cancelModal = [&](){
		if (mode == Mode::Grab && sel < scene.objects.size()) scene.objects[sel]->setPos(backupPos);
		if (mode == Mode::Rotate && sel < scene.objects.size()) scene.objects[sel]->setDir(backupDir);
		if (mode == Mode::LightM) scene.light.pos = backupLight;
		if (mode == Mode::Cam) scene.cam.pos = backupCam;
		mode = Mode::Idle; axis = Axis::Free; dirty = true; preview = false;
	};
	auto	confirmModal = [&](){
		mode = Mode::Idle; axis = Axis::Free; dirty = true; preview = false; // net au relâchement
	};

	// Échelle souris -> monde : proportionnelle à la distance objet-caméra (stable près/loin).
	auto	grabScale = [&]()->double{
		double d = 5.0;
		if ((mode == Mode::Grab || mode == Mode::Rotate) && sel < scene.objects.size())
			d = (scene.objects[sel]->getPos() - scene.cam.pos).length();
		if (d < 0.5) d = 0.5;
		return d * 0.0016; // ~8px souris = 1% distance
	};

	// Applique un delta souris (dx,dy pixels) selon le mode + contrainte axe.
	auto	applyMouse = [&](int dx, int dy){
		if (dx == 0 && dy == 0) return;
		Vec3	right, up, fwd;
		camBasis(scene, right, up, fwd);
		double	k = grabScale();
		if (mode == Mode::Rotate){ // yaw autour Y monde + pitch autour right caméra
			if (sel >= scene.objects.size()) return;
			Vec3	d = scene.objects[sel]->getDir();
			double	yaw = -dx * 0.005, pitch = -dy * 0.005;
			if (axis == Axis::X) pitch = 0;
			if (axis == Axis::Y) yaw = 0; // contraint : yaw seul autour Y
			if (axis == Axis::Z) { yaw = -dx * 0.005; pitch = 0; }
			if (axis == Axis::Free || axis == Axis::Y) d = rotAxis(d, Vec3{0,1,0}, yaw);
			if (axis == Axis::Free || axis == Axis::X) d = rotAxis(d, right, pitch);
			if (axis == Axis::Z) d = rotAxis(d, fwd, yaw); // roll autour forward
			scene.objects[sel]->setDir(d); // 1 rebuild cache
			return;
		}
		Vec3	delta{0,0,0};
		if (axis == Axis::Free) delta = right * (-dx * k) + up * (dy * k);
		else if (axis == Axis::X) delta = Vec3{(double)-dx * k, 0, 0};
		else if (axis == Axis::Y) delta = Vec3{0, (double)-dy * k, 0};
		else delta = fwd * ((double)(-dy) * k * 2); // Z : profondeur
		if (mode == Mode::Grab && sel < scene.objects.size()) scene.objects[sel]->translate(delta);
		else if (mode == Mode::Cam) scene.cam.pos = scene.cam.pos + (axis == Axis::Free ? right * ((double)-dx * k) + up * ((double)dy * k) : delta);
		else if (mode == Mode::LightM) scene.light.pos = scene.light.pos + delta;
	};

	// Nudge clavier : 0.1 (Shift 0.01) dans le plan caméra ou axe contraint.
	auto	applyNudge = [&](int kx, int ky){
		Vec3	right, up, fwd;
		camBasis(scene, right, up, fwd);
		double	s = shiftDown ? 0.01 : 0.1;
		Vec3	d{0,0,0};
		if (mode == Mode::Rotate){ // R + flèches : pas de 2° (Shift 0.5°)
			if (sel >= scene.objects.size()) return;
			double	a = (shiftDown ? 0.5 : 2.0) * M_PI / 180.0;
			Vec3	dir = scene.objects[sel]->getDir();
			if (kx) dir = rotAxis(dir, Vec3{0,1,0}, -kx * a);
			if (ky) dir = rotAxis(dir, right, -ky * a);
			scene.objects[sel]->setDir(dir);
			return;
		}
		if (axis == Axis::X) d = Vec3{kx * s, 0, 0};
		else if (axis == Axis::Y) d = Vec3{0, ky * s, 0};
		else if (axis == Axis::Z) d = fwd * ((ky != 0 ? ky : kx) * s);
		else d = right * ((double)kx * s) + up * ((double)ky * s);
		if (mode == Mode::Idle){ // hors modal : nudge l'objet sélectionné
			if (sel < scene.objects.size()) scene.objects[sel]->translate(d);
		}
		else if (mode == Mode::Grab && sel < scene.objects.size()) scene.objects[sel]->translate(d);
		else if (mode == Mode::Cam) scene.cam.pos = scene.cam.pos + d;
		else if (mode == Mode::LightM) scene.light.pos = scene.light.pos + d;
	};

	SDL_SetRelativeMouseMode(SDL_FALSE);
	setTitle(window, mode, axis, sel, scene.objects.size(), false);

	while (running){
		bool	gotEvent = false;
		SDL_Event	e;
		while (SDL_PollEvent(&e)){
			gotEvent = true;
			lastChange = SDL_GetTicks();
			switch (e.type){
				case SDL_QUIT: running = false; break;
				case SDL_MOUSEWHEEL: { // molette : profondeur (G/L) ou dolly caméra
					Vec3	right, up, fwd;
					camBasis(scene, right, up, fwd);
					double	s = (shiftDown ? 0.05 : 0.3) * (e.wheel.y > 0 ? 1 : -1);
					if (mode == Mode::Cam) scene.cam.pos = scene.cam.pos + fwd * s;
					else if (mode == Mode::Grab && sel < scene.objects.size()) scene.objects[sel]->translate(fwd * s);
					else if (mode == Mode::LightM) scene.light.pos = scene.light.pos + fwd * s;
					else scene.cam.pos = scene.cam.pos + fwd * s; // idle : dolly rapide
					dirty = true; preview = true;
					break;
				}
				case SDL_MOUSEMOTION:
					if (mode != Mode::Idle){ applyMouse(e.motion.xrel, e.motion.yrel); dirty = true; preview = true; }
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (mode != Mode::Idle) confirmModal(); // clic = valider
					else { // idle : picking au curseur (clic droit Blender)
						int hit = pickObject(scene, e.button.x, e.button.y);
						if (hit >= 0){ sel = (size_t)hit; std::cout << "select [" << sel << "] " << objName(scene.objects[sel].get()) << " (clic)" << std::endl; setTitle(window, mode, axis, sel, scene.objects.size(), false); }
					}
					break;
				case SDL_KEYDOWN:
					shiftDown = (e.key.keysym.mod & KMOD_SHIFT) != 0;
					if (e.key.keysym.sym == SDLK_q){ running = false; break; }
					if (e.key.keysym.sym == SDLK_ESCAPE){
						if (mode != Mode::Idle) cancelModal();
						else running = false;
						break;
					}
					if (e.key.keysym.sym == SDLK_TAB && mode == Mode::Idle){
						if (!scene.objects.empty()) sel = (sel + 1) % scene.objects.size();
						std::cout << "select [" << sel << "] " << objName(scene.objects[sel].get()) << std::endl;
						break;
					}
					if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9 && mode == Mode::Idle){
						size_t i = (size_t)(e.key.keysym.sym - SDLK_0);
						if (i < scene.objects.size()){ sel = i; std::cout << "select [" << sel << "] " << objName(scene.objects[sel].get()) << std::endl; }
						break;
					}
					switch (e.key.keysym.sym){
						case SDLK_h: printHelp(sel, scene.objects.size()); break;
						case SDLK_f: dirty = true; preview = false; break; // force net
						case SDLK_g: beginModal(Mode::Grab); std::cout << "GRAB obj[" << sel << "] (U libre, X/Y/Z, ESC annule)" << std::endl; break;
						case SDLK_r: beginModal(Mode::Rotate); std::cout << "ROTATE obj[" << sel << "] dir (souris/fleches)" << std::endl; break;
						case SDLK_c: beginModal(Mode::Cam); std::cout << "CAM (souris=pan, molette=dolly)" << std::endl; break;
						case SDLK_l: beginModal(Mode::LightM); std::cout << "LIGHT (souris=plan camera, molette=profondeur)" << std::endl; break;
						case SDLK_x: if (mode != Mode::Idle) axis = Axis::X; break;
						case SDLK_y: if (mode != Mode::Idle) axis = Axis::Y; break;
						case SDLK_z: if (mode != Mode::Idle) axis = Axis::Z; break;
						case SDLK_u: if (mode != Mode::Idle) axis = Axis::Free; break;
						case SDLK_RETURN: case SDLK_SPACE: if (mode != Mode::Idle) confirmModal(); break;
						case SDLK_LEFT: applyNudge(-1, 0); dirty = true; preview = (mode != Mode::Idle); break;
						case SDLK_RIGHT: applyNudge(1, 0); dirty = true; preview = (mode != Mode::Idle); break;
						case SDLK_UP: applyNudge(0, 1); dirty = true; preview = (mode != Mode::Idle); break;
						case SDLK_DOWN: applyNudge(0, -1); dirty = true; preview = (mode != Mode::Idle); break;
						default: break;
					}
					break;
				case SDL_KEYUP:
					shiftDown = (e.key.keysym.mod & KMOD_SHIFT) != 0;
					break;
				default: break;
			}
		}
		if (dirty){
			doRender(preview);
			setTitle(window, mode, axis, sel, scene.objects.size(), preview);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
			dirty = false;
			// En modal : reste en preview tant que ça bouge ; le net vient à la validation.
			// Hors modal (nudge/molette idle) : enchaîne un rendu net direct.
			if (!preview) { /* net déjà affiché */ }
			else if (mode == Mode::Idle){ doRender(false); SDL_UpdateTexture(texture, NULL, buffer.data(), scene.width * sizeof(uint32_t)); SDL_RenderClear(renderer); SDL_RenderCopy(renderer, texture, NULL, NULL); SDL_RenderPresent(renderer); setTitle(window, mode, axis, sel, scene.objects.size(), false); }
		} else if (preview && mode != Mode::Idle && SDL_GetTicks() - lastChange > FULL_DELAY){
			(void)gotEvent; // garde la preview pendant le drag : pas de net auto qui freezerait
		}
		if (!dirty) SDL_Delay(8);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	std::cout << "RTv1 - interactif termine." << std::endl;
	return 0;
}
