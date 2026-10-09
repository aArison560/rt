// BVH : construction par mediane (T060) — voir `include/rt/accel/Bvh.hpp`.
// Chemin froid uniquement (allocation `reserve`/`resize` autorisee, comme
// `collectSceneObjects` en T046) ; aucun `throw` explicite (R2), aucune
// allocation par noeud apres la reservation initiale (R3, buffer prealloue
// `2N`, borne `nbNodes <= 2N-1` d'un arbre binaire strict).

#include "rt/accel/Bvh.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"

namespace rt::accel {

namespace {

// Etendue d'une boite sur un axe (0/1/2), toujours >= 0 si finie.
[[nodiscard]] Real extentOf(const AABB& box, std::size_t axis) noexcept {
	const Real ext = box.max[axis] - box.min[axis];
	if (!std::isfinite(ext) || ext < Real(0)) {
		return Real(0);
	}
	return ext;
}
// Centroide d'une boite (moyenne, sans allocation).
[[nodiscard]] Vec3 centroidOf(const AABB& box) noexcept {
	return (box.min + box.max) * Real(0.5F);
}

// Test rayon/AABB optimise (T061, methode de Williams) : `invDir` est
// precalcule une fois par `traverse()` (aucune division par noeud),
// `parallel[axis]` couvre les directions quasi nulles (`|d| <= kEpsilon`,
// meme garde que `AABB::hit` en T013 : jamais de division par zero).
// `enter` recoit le `t` d'entree (pour visiter le fils proche d'abord).
// `noexcept`, sans allocation (R2/R3).
[[nodiscard]] bool boxHit(const Ray& ray, const Real invDir[3], const bool parallel[3],
                          const AABB& box, Real tMin, Real tMax, Real& enter) noexcept {
	Real currentMin = tMin;
	Real currentMax = tMax;
	for (std::size_t axis = 0; axis < 3; ++axis) {
		if (!parallel[axis]) {
			Real t0 = (box.min[axis] - ray.origin[axis]) * invDir[axis];
			Real t1 = (box.max[axis] - ray.origin[axis]) * invDir[axis];
			if (t0 > t1) {
				const Real tmp = t0;
				t0 = t1;
				t1 = tmp;
			}
			if (t0 > currentMin) {
				currentMin = t0;
			}
			if (t1 < currentMax) {
				currentMax = t1;
			}
		} else {
			// Rayon parallele a l'axe : rejete s'il est hors des dalles.
			if (ray.origin[axis] < box.min[axis] || ray.origin[axis] > box.max[axis]) {
				return false;
			}
		}
		if (currentMax < currentMin) {
			return false;
		}
	}
	enter = currentMin;
	return true;
}

} // namespace

AABB Bvh::worldBounds(const geometry::AObject& obj) noexcept {
	const AABB local = obj.localBounds();
	const Transform& world = obj.objectToWorld();
	Vec3 lo(kInfinity, kInfinity, kInfinity);
	Vec3 hi(-kInfinity, -kInfinity, -kInfinity);
	for (int i = 0; i < 8; ++i) {
		const Vec3 corner((i & 1) != 0 ? local.max.x : local.min.x,
		                  (i & 2) != 0 ? local.max.y : local.min.y,
		                  (i & 4) != 0 ? local.max.z : local.min.z);
		const Vec3 worldCorner = world.applyPoint(corner);
		if (!std::isfinite(worldCorner.x) || !std::isfinite(worldCorner.y) ||
		    !std::isfinite(worldCorner.z)) {
			continue;
		}
		if (worldCorner.x < lo.x) {
			lo.x = worldCorner.x;
		}
		if (worldCorner.y < lo.y) {
			lo.y = worldCorner.y;
		}
		if (worldCorner.z < lo.z) {
			lo.z = worldCorner.z;
		}
		if (worldCorner.x > hi.x) {
			hi.x = worldCorner.x;
		}
		if (worldCorner.y > hi.y) {
			hi.y = worldCorner.y;
		}
		if (worldCorner.z > hi.z) {
			hi.z = worldCorner.z;
		}
	}
	if (!std::isfinite(lo.x) || !std::isfinite(lo.y) || !std::isfinite(lo.z) ||
	    !std::isfinite(hi.x) || !std::isfinite(hi.y) || !std::isfinite(hi.z) || !(lo.x <= hi.x) ||
	    !(lo.y <= hi.y) || !(lo.z <= hi.z)) {
		// Repli conservatif (jamais de boite NaN : la traversal visiterait
		// ou rejeterait au hasard). ±1e6 = troncature des infinis (T042).
		constexpr Real extent = Real(1000000);
		return AABB(Vec3(-extent, -extent, -extent), Vec3(extent, extent, extent));
	}
	return AABB(lo, hi);
}

void Bvh::clear() noexcept {
	nodes_.clear();
	primIndices_.clear();
	primBoxes_.clear();
	root_ = kInvalid;
}

Status Bvh::build(const std::vector<std::unique_ptr<geometry::AObject>>& objs) {
	clear();
	const std::size_t count = objs.size();
	if (count == 0) {
		return Status::ok();
	}
	// Garde-fou : un pointeur nul serait un crash en `worldBounds`.
	for (std::size_t i = 0; i < count; ++i) {
		if (!objs[i]) {
			clear();
			return RT_ERROR(StatusCode::InvalidArgument, "bvh build: null object");
		}
	}
	// Buffer prealloue (R3) : un arbre binaire strict a N feuilles a au
	// plus 2N-1 noeuds (DoD). `reserve` unique, chemin froid.
	nodes_.reserve(2 * count);
	primBoxes_.resize(count);
	primIndices_.resize(count);
	for (std::size_t i = 0; i < count; ++i) {
		primBoxes_[i] = worldBounds(*objs[i]);
		primIndices_[i] = static_cast<std::uint32_t>(i);
	}
	root_ = buildRecursive(0, static_cast<std::uint32_t>(count), 0);
	return Status::ok();
}

std::uint32_t Bvh::buildRecursive(std::uint32_t start, std::uint32_t count, int depth) {
	// Union des boites du segment (boite du noeud).
	AABB bounds = primBoxes_[primIndices_[start]];
	for (std::uint32_t i = 1; i < count; ++i) {
		bounds = bounds.merged(primBoxes_[primIndices_[start + i]]);
	}
	// Feuille : peu de primitives ou profondeur max atteinte (borne).
	if (count <= static_cast<std::uint32_t>(kMaxLeaf) || depth >= kMaxDepth) {
		const std::uint32_t index = static_cast<std::uint32_t>(nodes_.size());
		nodes_.push_back(BvhNode::makeLeaf(bounds, start, count));
		return index;
	}
	// Axe le plus long (partition par mediane, pas de SAH : suffisant pour
	// le DoD < 50 ms sur 1000 objets, SAH en option si le temps le permet).
	std::size_t axis = 0;
	{
		const Real ex = extentOf(bounds, 0);
		const Real ey = extentOf(bounds, 1);
		const Real ez = extentOf(bounds, 2);
		if (ey > ex && ey >= ez) {
			axis = 1;
		} else if (ez > ex && ez > ey) {
			axis = 2;
		}
	}
	// Si l'etendue est nulle sur les 3 axes (objets concentriques), toute
	// partition serait arbitraire : feuille (evite une recursion inutile).
	if (!(extentOf(bounds, axis) > Real(0))) {
		const std::uint32_t index = static_cast<std::uint32_t>(nodes_.size());
		nodes_.push_back(BvhNode::makeLeaf(bounds, start, count));
		return index;
	}
	// Mediane des centroides (`nth_element`, sans allocation, chemin froid).
	const std::uint32_t mid = start + count / 2;
	std::nth_element(primIndices_.begin() + start, primIndices_.begin() + mid,
	                 primIndices_.begin() + start + count,
	                 [this, axis](std::uint32_t lhs, std::uint32_t rhs) noexcept {
		                 const Vec3 left = centroidOf(primBoxes_[lhs]);
		                 const Vec3 right = centroidOf(primBoxes_[rhs]);
		                 const Real leftValue = left[axis];
		                 const Real rightValue = right[axis];
		                 if (!std::isfinite(leftValue) || !std::isfinite(rightValue)) {
			                 return lhs < rhs;
		                 }
		                 return leftValue < rightValue;
	                 });
	// `nth_element` avec `count >= 2` donne deux segments non vides
	// (`mid` strictement interieur), donc la recursion termine.
	const std::uint32_t index = static_cast<std::uint32_t>(nodes_.size());
	nodes_.push_back(BvhNode::makeInterior(bounds, kInvalid, kInvalid));
	const std::uint32_t left = buildRecursive(start, mid - start, depth + 1);
	const std::uint32_t right = buildRecursive(mid, start + count - mid, depth + 1);
	nodes_[index] = BvhNode::makeInterior(bounds, left, right);
	return index;
}

bool Bvh::traverse(const Ray& ray, Real tMin, Real tMax, HitRecord& rec,
                   const std::vector<std::unique_ptr<geometry::AObject>>& objs) const noexcept {
	if (nodes_.empty() || root_ == kInvalid || root_ >= nodes_.size()) {
		return false;
	}
	// Le vecteur doit correspondre a celui servi a `build()` : meme taille
	// (les index restent valides), sinon miss defini, jamais de crash.
	if (objs.size() != primBoxes_.size() || primIndices_.size() != primBoxes_.size()) {
		return false;
	}
	if (!(tMin <= tMax) || std::isnan(tMin) || std::isnan(tMax)) {
		return false;
	}
	// Williams : une seule division par axe pour tout le parcours.
	Real invDir[3] = {Real(0), Real(0), Real(0)};
	bool parallel[3] = {true, true, true};
	for (std::size_t axis = 0; axis < 3; ++axis) {
		const Real dir = ray.direction[axis];
		if ((dir > kEpsilon || dir < -kEpsilon) && std::isfinite(dir)) {
			invDir[axis] = Real(1) / dir;
			parallel[axis] = false;
		}
	}
	// Pile fixe (tableau local, R3 : aucune allocation, pas de recursion).
	std::uint32_t stack[kStackSize];
	std::size_t top = 0;
	stack[top++] = root_;
	bool hit = false;
	Real closest = tMax;
	HitRecord tmp;
	while (top > 0) {
		const std::uint32_t index = stack[--top];
		if (index >= nodes_.size()) {
			continue;
		}
		const BvhNode& node = nodes_[index];
		Real enter = tMin;
		if (!boxHit(ray, invDir, parallel, node.bounds, tMin, closest, enter)) {
			continue;
		}
		if (node.isLeaf()) {
			const std::uint32_t start = node.leafStart();
			const std::uint32_t count = node.leafCount();
			if (start >= primIndices_.size() ||
			    count > primIndices_.size() - start) {
				continue;
			}
			for (std::uint32_t i = 0; i < count; ++i) {
				const std::uint32_t prim = primIndices_[start + i];
				if (prim >= objs.size()) {
					continue;
				}
				const std::unique_ptr<geometry::AObject>& obj = objs[prim];
				if (!obj) {
					continue;
				}
				if (obj->intersect(ray, tMin, closest, tmp)) {
					rec = tmp;
					closest = tmp.t;
					hit = true;
				}
			}
		} else {
			const std::uint32_t left = node.left();
			const std::uint32_t right = node.right();
			if (left >= nodes_.size() || right >= nodes_.size()) {
				continue;
			}
			// Fils proche d'abord (ressert `closest` plus tot, elague plus).
			Real enterLeft = tMin;
			Real enterRight = tMin;
			const bool hitLeft =
			    boxHit(ray, invDir, parallel, nodes_[left].bounds, tMin, closest, enterLeft);
			const bool hitRight =
			    boxHit(ray, invDir, parallel, nodes_[right].bounds, tMin, closest, enterRight);
			if (hitLeft && hitRight) {
				if (top + 2 > kStackSize) {
					continue;
				}
				if (enterLeft < enterRight) {
					stack[top++] = right;
					stack[top++] = left;
				} else {
					stack[top++] = left;
					stack[top++] = right;
				}
			} else if (hitLeft) {
				if (top + 1 > kStackSize) {
					continue;
				}
				stack[top++] = left;
			} else if (hitRight) {
				if (top + 1 > kStackSize) {
					continue;
				}
				stack[top++] = right;
			}
		}
	}
	return hit;
}

} // namespace rt::accel
