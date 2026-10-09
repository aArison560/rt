#pragma once

// Cache BVH a invalidation ciblee (T062) — l'equivalent du *depsgraph* de
// Blender (voir `docs/INSPIRATION_BLENDER.md` §2) : la BVH n'est reconstruite
// que si `scene.objectVersion` a change depuis la derniere construction.
// Le compteur `bvhBuilds()` expose le nombre de reconstructions (DoD) et est
// affiche sur `stderr` quand `RT_DEBUG` est defini (chemin froid uniquement).
// Calque `accel/` : ne voit que `base/` + `geometry/` (regle d'or §2.1).
// R2 : `ensure()` renvoie `Status` (jamais de `throw`) ; R3 : le hot path
// (`Bvh::traverse`, T061) n'est pas touche — `ensure()` est le chemin froid
// (allocation `reserve` autorisee, comme `Bvh::build` en T060).
// Thread-safety (T066) : `ensure()` prend un verrou (`mutex_`) ; apres
// `ensure()`, `bvh()` est lisible en concurrence (traversal `const`,
// lecture seule, tuiles disjointes). `bvhBuilds()` verrouille brievement
// (jamais appele dans la boucle chaude).
// Limite documentee : le cache est **par scene** (un `BvhCache` par objet
// `Scene`, ou `clear()` en changeant de scene). `ensure()` reconstruit si
// la version differe **ou** si le nombre d'objets differe (garde-fou contre
// la reutilisation entre deux scenes differentes de meme version) ; deux
// scenes differentes de meme version **et** meme cardinalite partageraient
// a tort la BVH — d'ou la regle "un cache par scene".

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "rt/accel/Bvh.hpp"
#include "rt/base/Status.hpp"

namespace rt::geometry {
class AObject;
}

namespace rt::accel {

class BvhCache {
public:
	BvhCache() = default;
	BvhCache(const BvhCache&) = delete;
	BvhCache& operator=(const BvhCache&) = delete;

	// Reconstruit la BVH si `version` differe de la version construite, si
	// le nombre d'objets differe, ou si c'est le premier appel. Sinon ne
	// fait rien (0 allocation, 0 reconstruction). `version` =
	// `scene.objectVersion` (R5, `touchObjects()` l'incremente).
	// Erreur (`nullptr` dans `objs`) -> `Status` d'erreur, version
	// inchangee, compteur inchange (jamais de crash, R2).
	[[nodiscard]] Status ensure(
	    const std::vector<std::unique_ptr<geometry::AObject>>& objs, std::uint64_t version);

	// Reinitialise le cache (BVH videe, version oubliee). Le compteur
	// `bvhBuilds()` reste monotone (total depuis la construction).
	void clear();

	// BVH construite (valide apres un `ensure()` reussi ; lecture seule,
	// partageable entre threads pour `traverse()`).
	[[nodiscard]] const Bvh& bvh() const noexcept { return bvh_; }
	// Nombre de reconstructions effectuees (DoD T062, affiche en debug).
	[[nodiscard]] std::size_t bvhBuilds() const;
	// Version actuellement construite (`kNeverBuilt` = jamais construit).
	[[nodiscard]] std::uint64_t builtVersion() const;

	static constexpr std::uint64_t kNeverBuilt = 0xFFFFFFFFFFFFFFFFu;

private:
	Bvh bvh_;
	std::uint64_t builtVersion_ = kNeverBuilt;
	std::size_t bvhBuilds_ = 0;
	mutable std::mutex mutex_;
};

} // namespace rt::accel
