#pragma once

// BVH : construction (T060) — arbre binaire sur les AABB monde des objets.
// Partition par mediane sur l'axe le plus long, profondeur bornee,
// feuilles de <= `kMaxLeaf` primitives. Les noeuds sont compacts et POD
// (32 octets, `static_assert`), stockes dans un buffer prealloue
// (`reserve(2N)`, **zero `new` par noeud** apres la reservation, R3).
// Calque `accel/` : ne voit que `base/` + `geometry/` (regle d'or §2.1).
// R2 : `build()` renvoie `Status` (jamais de `throw` explicite) ; une
// eventuelle `bad_alloc` du chemin froid remonte au filet de `main`
// (comme `collectSceneObjects` en T046). Le hot path (`traverse`, T061)
// n'allouera ni ne lancera.
// Note : les primitives infinies (plan, cylindre, cone) ont des
// `localBounds()` tronquees a ±1e6 (documente en T042-T044) ; la BVH est
// donc conservative dans ce volume (suffisant pour les scenes a l'echelle
// ~10, dont les 1000 spheres synthetiques du DoD).

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "rt/base/Ray.hpp"
#include "rt/base/Status.hpp"

namespace rt::geometry {
class AObject;
}

namespace rt::accel {

// Noeud compact : `bounds` (24 o) + 2 × 32 bits = 32 o.
// Encodage sans champ supplementaire (reste a 32 o) :
//   - interieur : `first` = index du fils gauche, `second` = fils droit
//     (tous deux < 2^31, le bit haut est a 0) ;
//   - feuille : `first` = offset dans `primIndices_`, `second` = nombre
//     de primitives avec le bit haut `kLeafBit` positionne.
// Le test feuille/branchu est donc un seul masque, sans allocation (R3).
struct BvhNode {
	AABB bounds;
	std::uint32_t first = kInvalid;
	std::uint32_t second = 0;

	static constexpr std::uint32_t kInvalid = 0xFFFFFFFFu;
	static constexpr std::uint32_t kLeafBit = 0x80000000u;

	[[nodiscard]] constexpr bool isLeaf() const noexcept { return (second & kLeafBit) != 0u; }
	[[nodiscard]] constexpr std::uint32_t left() const noexcept { return first; }
	[[nodiscard]] constexpr std::uint32_t right() const noexcept { return second; }
	[[nodiscard]] constexpr std::uint32_t leafStart() const noexcept { return first; }
	[[nodiscard]] constexpr std::uint32_t leafCount() const noexcept {
		return second & ~kLeafBit;
	}

	[[nodiscard]] static constexpr BvhNode makeLeaf(AABB box, std::uint32_t start,
	                                                std::uint32_t count) noexcept {
		BvhNode node;
		node.bounds = box;
		node.first = start;
		node.second = count | kLeafBit;
		return node;
	}

	[[nodiscard]] static constexpr BvhNode makeInterior(AABB box, std::uint32_t left,
	                                                    std::uint32_t right) noexcept {
		BvhNode node;
		node.bounds = box;
		node.first = left;
		node.second = right;
		return node;
	}
};

static_assert(sizeof(BvhNode) == 32u);
static_assert(std::is_trivially_copyable_v<BvhNode>);
static_assert(std::is_standard_layout_v<BvhNode>);

class Bvh {
public:
	static constexpr std::uint32_t kInvalid = BvhNode::kInvalid;
	// Profondeur max de recursion (borne, evite la pile profonde, R3).
	static constexpr int kMaxDepth = 32;
	// Taille max d'une feuille (compromis traversal/build, documente).
	static constexpr std::size_t kMaxLeaf = 4;
	// Pile fixe de traversal (T061) : >= `kMaxDepth` + marge (un arbre
	// binaire strict ne depile jamais plus que sa profondeur + 1).
	static constexpr std::size_t kStackSize = 64;

	Bvh() = default;

	// Construit l'arbre sur les AABB monde (`worldBounds`) des objets.
	// `objs` reste proprietaire des primitives (aucune copie, aucun `new`
	// par noeud : `nodes_.reserve(2N)` une fois, chemin froid).
	// Vide (`N == 0`) -> arbre vide valide (`empty()`, `root() == kInvalid`).
	// Pointeur nul dans `objs` -> `InvalidArgument` (jamais de crash).
	Status build(const std::vector<std::unique_ptr<geometry::AObject>>& objs);

	void clear() noexcept;

	[[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }
	[[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }
	[[nodiscard]] std::size_t primCount() const noexcept { return primIndices_.size(); }
	[[nodiscard]] std::uint32_t root() const noexcept { return root_; }
	[[nodiscard]] const std::vector<BvhNode>& nodes() const noexcept { return nodes_; }
	[[nodiscard]] const std::vector<std::uint32_t>& primIndices() const noexcept {
		return primIndices_;
	}
	[[nodiscard]] const std::vector<AABB>& primBoxes() const noexcept { return primBoxes_; }

	// Traversal d'un rayon (T061) : plus proche dans `[tMin, tMax]`, `rec`
	// rempli comme `intersect` (point, normale contre le rayon via
	// `setFaceNormal`, `t`, `frontFace`, `uv`, `materialIndex`).
	// Pile fixe `kStackSize` (tableau local, pas de recursion profonde),
	// test AABB optimise (methode de Williams : `invDir` precalcule une
	// fois, dalles sans division par noeud), `tMax` resserre au plus
	// proche (comme `findClosestHit` en T046). Resultats identiques a la
	// recherche lineaire (test d'equivalence T061, tolerance flottante).
	// `objs` doit etre le vecteur servi a `build()` (meme taille, sinon
	// miss defini, jamais de crash) ; `objs` reste proprietaire.
	// `noexcept`, sans allocation (R2/R3 : pile + registres uniquement).
	[[nodiscard]] bool traverse(const Ray& ray, Real tMin, Real tMax, HitRecord& rec,
	                            const std::vector<std::unique_ptr<geometry::AObject>>& objs)
	    const noexcept;

	// AABB monde d'un objet : les 8 coins de `localBounds()` (espace objet)
	// passes par `objectToWorld` (approche A, T045). `noexcept`, pile
	// uniquement (R2/R3). Non finie en sortie -> grosse boite conservative.
	[[nodiscard]] static AABB worldBounds(const geometry::AObject& obj) noexcept;

private:
	[[nodiscard]] std::uint32_t buildRecursive(std::uint32_t start, std::uint32_t count,
	                                           int depth);

	std::vector<BvhNode> nodes_;
	std::vector<std::uint32_t> primIndices_;
	std::vector<AABB> primBoxes_;
	std::uint32_t root_ = kInvalid;
};

} // namespace rt::accel
