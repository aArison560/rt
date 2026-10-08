// Boucle de rendu mono-thread (T032) — implementation sans exception,
// sans SDL (R6) et sans allocation dans la boucle (R3).
// Voir `include/rt/render/Renderer.hpp` pour le contrat et
// `docs/ARCHITECTURE.md` §4 pour le pipeline (camera -> intersection ->
// framebuffer). La recherche d'intersection sera branchee en P4 (T040+) :
// en attendant, tout rayon manque (aucune primitive) et retourne le fond
// de scene (`scene.background.color`), ce qui rend deja une image lisible
// et deterministe.

#include "rt/render/Renderer.hpp"

#include "rt/render/Camera.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/scene/Scene.hpp"

namespace rt::render {

namespace {

constexpr int kMinDim = 1;
constexpr int kMaxDim = 8192;
constexpr int kMinSpp = 1;
constexpr int kMaxSpp = 1024;
constexpr int kMinDepth = 0;
constexpr int kMaxDepth = 32;
constexpr long long kMinSeed = 0;
constexpr long long kMaxSeed = 4294967295LL;

// Miss -> fond de scene (T032). Aucune allocation, `noexcept`.
// La passe d'ombrage (Lambert, T033) et les intersections (P4)
// enrichiront ce point sans changer la boucle.
[[nodiscard]] Vec3 shadeMiss(const scene::Scene& scene) noexcept {
	return scene.background.color;
}

} // namespace

Status render(const scene::Scene& scene, Framebuffer& fb, const RenderParams& params) {
	if (params.width < kMinDim || params.width > kMaxDim || params.height < kMinDim ||
	    params.height > kMaxDim) {
		return Status::error(StatusCode::InvalidArgument, "bad render size: expected 1..8192",
		                     __LINE__);
	}
	if (params.spp < kMinSpp || params.spp > kMaxSpp) {
		return Status::error(StatusCode::InvalidArgument, "bad render spp: expected 1..1024",
		                     __LINE__);
	}
	if (params.maxDepth < kMinDepth || params.maxDepth > kMaxDepth) {
		return Status::error(StatusCode::InvalidArgument, "bad render depth: expected 0..32",
		                     __LINE__);
	}
	if (params.seed < kMinSeed || params.seed > kMaxSeed) {
		return Status::error(StatusCode::InvalidArgument,
		                     "bad render seed: expected 0..4294967295", __LINE__);
	}
	Camera camera;
	if (Status status = camera.init(scene.camera, params.width, params.height);
	    status.isError()) {
		return status;
	}
	if (Status status = fb.init(params.width, params.height); status.isError()) {
		return status;
	}
	// Boucle chaude : registres/arena uniquement (R3). `spp` echantillons
	// identiques pour l'instant (jitter AA en T120, RNG par pixel en T036) :
	// l'accumulation reste deterministe (moyenne == fond).
	for (int y = 0; y < params.height; ++y) {
		for (int x = 0; x < params.width; ++x) {
			for (int s = 0; s < params.spp; ++s) {
				(void)s;
				// Le rayon est genere pour chaque echantillon (P4 branchera
				// l'intersection dessus sans changer la boucle).
				const Ray ray = camera.rayForPixel(x, y);
				(void)ray;
				const Vec3 color = shadeMiss(scene);
				fb.addSample(x, y, color);
			}
		}
	}
	fb.present();
	return Status::ok();
}

} // namespace rt::render
