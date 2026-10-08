// Boucle de rendu mono-thread (T032) — implementation sans exception,
// sans SDL (R6) et sans allocation dans la boucle (R3).
// Voir `include/rt/render/Renderer.hpp` pour le contrat et
// `docs/ARCHITECTURE.md` §4 pour le pipeline (camera -> intersection ->
// framebuffer). La recherche d'intersection sera branchee en P4 (T040+) :
// en attendant, tout rayon manque (aucune primitive) et retourne le fond
// de scene (`scene.background.color`), ce qui rend deja une image lisible
// et deterministe. Progressif T036 : batches externes + `seedFor` absolu
// + jitter sous-pixel (AA en T120) + dithering deterministe minimal pour
// rendre la convergence observable avant P4/P5 (moyenne -> fond).

#include "rt/render/Renderer.hpp"

#include "rt/base/Rng.hpp"
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
// T036 : dithering deterministe minimal (±kDitherAmp/2 par echantillon,
// moyenne -> 0 quand spp grandit). Rend le progressif observable
// (spp 4 plus bruite que spp 64) avant que P4/P5 n'apporte le vrai bruit
// Monte-Carlo (intersections, ombres). Sera absorbe par l'ombrage reel.
constexpr float kDitherAmp = 0.06F;

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
	// Boucle chaude : registres uniquement (R3), batches externes (T036).
	// Chaque echantillon `s` utilise `rngFor(x, y, s, seed)` (coordonnees
	// absolues, T016) : jitter sous-pixel pour le rayon + dithering pour
	// la couleur. Memes `spp` + meme `seed` -> memes pixels, octet par
	// octet, que ce soit en plein ou par tuile/bande (couture impossible).
	// `onProgress(s+1, spp)` apres chaque batch (1 appel par batch, pas
	// par pixel : hors hot path fin, futur affichage + ETA par l'appelant).
	const auto sceneSeed = static_cast<std::uint32_t>(params.seed);
	for (int s = 0; s < params.spp; ++s) {
		for (int y = 0; y < params.height; ++y) {
			for (int x = 0; x < params.width; ++x) {
				Rng rng = rngFor(x, y, s, sceneSeed);
				const Vec2 jitter(rng.nextFloat() - 0.5F, rng.nextFloat() - 0.5F);
				// Le rayon jittere servira a l'intersection en P4 ; en
				// attendant il manque toujours (fond), mais le tirage est
				// deja consomme pour figer le contrat RNG.
				const Ray ray = camera.rayForPixel(x, y, jitter);
				(void)ray;
				Vec3 color = shadeMiss(scene);
				color.x += (rng.nextFloat() - 0.5F) * kDitherAmp;
				color.y += (rng.nextFloat() - 0.5F) * kDitherAmp;
				color.z += (rng.nextFloat() - 0.5F) * kDitherAmp;
				fb.addSample(x, y, color);
			}
		}
		if (params.onProgress != nullptr) {
			params.onProgress(s + 1, params.spp, params.progressUser);
		}
	}
	fb.present();
	return Status::ok();
}

} // namespace rt::render
