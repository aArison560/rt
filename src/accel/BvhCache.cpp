// Cache BVH a invalidation ciblee (T062) — voir `include/rt/accel/BvhCache.hpp`.
// Chemin froid uniquement (verrou + `Bvh::build` + `getenv`, jamais dans la
// boucle chaude R3) ; aucun `throw` explicite (R2).

#include "rt/accel/BvhCache.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <vector>

namespace rt::accel {

Status BvhCache::ensure(const std::vector<std::unique_ptr<geometry::AObject>>& objs,
                        std::uint64_t version) {
	const std::lock_guard<std::mutex> lock(mutex_);
	// Meme version et meme cardinalite : rien a faire (0 reconstruction).
	// `primCount()` vaut le nombre d'objets servi au dernier `build()`.
	if (builtVersion_ != kNeverBuilt && builtVersion_ == version &&
	    bvh_.primCount() == objs.size()) {
		return Status::ok();
	}
	if (Status status = bvh_.build(objs); status.isError()) {
		return status;
	}
	builtVersion_ = version;
	++bvhBuilds_;
	// Compteur affiche en debug (DoD T062) : chemin froid, `stderr`.
	// `RT_DEBUG` defini (meme vide) suffit — meme convention que `RT_LOG`.
	if (std::getenv("RT_DEBUG") != nullptr) { // NOLINT(concurrency-mt-unsafe)
		std::fprintf(stderr, "[debug] bvhBuilds=%zu version=%llu objs=%zu nodes=%zu\n",
		             bvhBuilds_, static_cast<unsigned long long>(builtVersion_), objs.size(),
		             bvh_.nodeCount());
	}
	return Status::ok();
}

void BvhCache::clear() {
	const std::lock_guard<std::mutex> lock(mutex_);
	bvh_.clear();
	builtVersion_ = kNeverBuilt;
}

std::size_t BvhCache::bvhBuilds() const {
	const std::lock_guard<std::mutex> lock(mutex_);
	return bvhBuilds_;
}

std::uint64_t BvhCache::builtVersion() const {
	const std::lock_guard<std::mutex> lock(mutex_);
	return builtVersion_;
}

} // namespace rt::accel
