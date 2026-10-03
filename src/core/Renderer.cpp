#include "core/Renderer.hpp"
#include "core/Object.hpp"
#include "core/Texture.hpp"
#include <cmath>
#include <thread>
#include <vector>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Clamp 0-255 puis pack ARGB8888 (alpha opaque).
static uint32_t	rgbToUint32(const Vec3& c){
	uint8_t	r = (uint8_t)std::fmax(0, std::fmin(255, c.x));
	uint8_t	g = (uint8_t)std::fmax(0, std::fmin(255, c.y));
	uint8_t	b = (uint8_t)std::fmax(0, std::fmin(255, c.z));
	return (0xFFu<<24)|(r<<16)|(g<<8)|b;
}

Vec3	Renderer::computeLight(const Scene& scene, const Hit& hit){
	Vec3	lightDir = (scene.light.pos - hit.point).normalized(); // Phase 5.5 : L.x++ => ombre à gauche
	// Couleur de base : image > checker > uni (mêmes UV, zéro changement d'éclairage).
	Vec3	base = hit.color;
	const Material& m = hit.material;
	if (m.useImage && m.map && !m.map->empty()) {
		base = m.map->sample(hit.u, hit.v);
	} else if (m.useChecker) { // damier : parité (⌊u·s⌋+⌊v·s⌋) mod 2
		double s = m.uvScale > 0 ? m.uvScale : 8.0;
		long iu = (long)std::floor(hit.u * s);
		long iv = (long)std::floor(hit.v * s);
		bool alt = ((iu + iv) & 1) != 0;
		base = alt ? m.albedo2 : m.albedo;
	}
	// Diffuse : base·(ambient + max(0,N·L)·I·0.9).
	double	diff = std::fmax(0.0, hit.normal.dot(lightDir)) * scene.light.intensity;
	Vec3	col = Vec3(base.x * (scene.ambient.x + diff*0.9),
				base.y * (scene.ambient.y + diff*0.9),
				base.z * (scene.ambient.z + diff*0.9));
	col.x = std::fmin(255,col.x); col.y= std::fmin(255,col.y); col.z= std::fmin(255,col.z);

	// Ombre dure : rayon vers la lumière, si un objet bloque avant la lampe -> ambient seul.
	Ray		shadow{hit.point + hit.normal*1e-4, lightDir}; // epsilon anti auto-intersection
	double	lightDist = (scene.light.pos - hit.point).length();

	for (auto &obj: scene.objects){
		Hit	tmp;
		if (obj->intersect(shadow, tmp) && tmp.t > 1e-4 && tmp.t < lightDist){
			col= Vec3(base.x * scene.ambient.x,
					base.y * scene.ambient.y,
					base.z * scene.ambient.z);
			break;
		}
	}
	return col;
}

int	Renderer::render(Scene& scene, uint32_t* buffer){
	return renderPreview(scene, buffer, 1);
}

// Preview Blender-like : calcule 1 rayon par bloc step×step puis duplique
// la couleur sur le bloc. step=1 = identique à render. step=2 -> ~4x plus
// vite, step=3 -> ~9x (720p : ~0.3s -> ~0.04s, ~25 img/s pendant le drag).
int	Renderer::renderPreview(Scene& scene, uint32_t* buffer, int step){
	if (!buffer) return 1;
	if (step < 1) step = 1;
	int		w = scene.width,
			h = scene.height;
	double	aspect = (double) w / h;
	double	scale = std::tan(scene.cam.fov * M_PI / 180.0 *0.5); // demi-fov radians
	Vec3	forward = scene.cam.dir.normalized();
	Vec3	worldUp{0,1,0}; // Y-up RTv1 (Blender : Z-up converti)

	if (std::fabs(forward.dot(worldUp)) > 0.999) worldUp = Vec3{0, 0, 1}; // forward vertical : autre référence

	// Repère caméra anti-miroir : right = up×forward (cam.x++ => objet à gauche, pan Blender).
	Vec3			right = worldUp.cross(forward).normalized();
	Vec3			up = forward.cross(right).normalized();
	unsigned int	nthreads = std::thread::hardware_concurrency(); // 1 thread par bande de lignes

	if (nthreads == 0) nthreads = 4;
	if (nthreads > (unsigned)h) nthreads = h;

	std::vector<std::thread>	threads;
	int							rows = h / nthreads;

	auto	worker = [&](int y0,int y1){
		// Aligné sur les blocs step : chaque thread ne touche que ses blocs (sans overlap).
		int ys = (y0 / step) * step;
		if (ys < y0) ys += step;
		for (int y = ys; y < y1; y += step) {
			for (int x = 0; x < w; x += step) {
				// Pixel -> rayon primaire (centre du pixel, NDC -> caméra).
				double	nx = (2.0 * (x + 0.5) / w - 1) * aspect * scale;
				double	ny = (1 - 2.0 * (y + 0.5) / h) * scale;
				Vec3	dir = (right * nx + up * ny + forward).normalized();
				Ray		ray{scene.cam.pos, dir};
				Hit		closest;

				closest.t = 1e30;

				// Plus proche objet intersecté.
				bool	has = false;
				Hit		tmp;

				for (auto &obj : scene.objects) {
					if (obj->intersect(ray, tmp) && tmp.t < closest.t) {
						closest = tmp;
						has = true;
					}
				}
				uint32_t px;
				if (has)
					px = rgbToUint32(computeLight(scene, closest));
				else { // fond : dégradé ciel (haut bleu nuit, bas bleu clair assombri).
					double	t = 0.5 * (dir.y + 1);
					Vec3	bg = Vec3(135, 206, 235) * (1 - t) + Vec3(25, 25, 112) * t;

					bg = bg * 0.6;
					px = rgbToUint32(bg);
				}
				// Duplique sur le bloc step×step (bords clampés).
				for (int dy = 0; dy < step && y + dy < h; dy++)
					for (int dx = 0; dx < step && x + dx < w; dx++)
						buffer[(y + dy) * w + x + dx] = px;
			}
		}
	};
	for (unsigned i = 0; i < nthreads; i++){
		int	y0 = i * rows,
			y1 = (i == nthreads - 1) ? h : y0 + rows; // dernier thread prend le reste
		threads.emplace_back(worker, y0, y1);
	}
	for (auto &t : threads) t.join();
	return 0;
}
