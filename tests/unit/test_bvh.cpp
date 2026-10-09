// Tests de la construction BVH (T060), Catch2.
// DoD : construire une BVH sur 1000 objets synthetiques < 50 ms ;
// noeuds bornes (`nbNodes <= 2N-1`). Sans `throw` (R2), buffer prealloue
// (R3, `sizeof(BvhNode) == 32` verifie a la compilation dans le header).

#include <catch2/catch_amalgamated.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "rt/accel/Bvh.hpp"
#include "rt/base/Mat4.hpp"
#include "rt/geometry/Object.hpp"
#include "rt/geometry/Sphere.hpp"

namespace {

std::vector<std::unique_ptr<rt::geometry::AObject>> makeGridSpheres(std::size_t count) {
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.reserve(count);
	for (std::size_t i = 0; i < count; ++i) {
		const float x = static_cast<float>(i % 10) * 2.0F;
		const float y = static_cast<float>((i / 10) % 10) * 2.0F;
		const float z = static_cast<float>(i / 100) * 2.0F;
		objs.push_back(std::make_unique<rt::geometry::Sphere>(
		    rt::Vec3(x, y, z), 0.5F, static_cast<std::uint32_t>(i),
		    static_cast<std::uint32_t>(i)));
	}
	return objs;
}

// Profondeur max de l'arbre via les index (pile, sans allocation).
std::size_t treeDepth(const rt::accel::Bvh& bvh, std::uint32_t node) {
	if (node == rt::accel::Bvh::kInvalid || bvh.empty()) {
		return 0;
	}
	const rt::accel::BvhNode& current = bvh.nodes().at(node);
	if (current.isLeaf()) {
		return 1;
	}
	const std::size_t left = treeDepth(bvh, current.left());
	const std::size_t right = treeDepth(bvh, current.right());
	return 1 + (left > right ? left : right);
}

} // namespace

TEST_CASE("bvh : arbre vide et singleton (T060)", "[bvh][t060]") {
	rt::accel::Bvh bvh;
	std::vector<std::unique_ptr<rt::geometry::AObject>> empty;
	REQUIRE(bvh.build(empty).isOk());
	REQUIRE(bvh.empty());
	REQUIRE(bvh.nodeCount() == 0);
	REQUIRE(bvh.root() == rt::accel::Bvh::kInvalid);
	REQUIRE(bvh.primCount() == 0);

	auto single = makeGridSpheres(1);
	REQUIRE(bvh.build(single).isOk());
	REQUIRE(!bvh.empty());
	REQUIRE(bvh.nodeCount() == 1);
	REQUIRE(bvh.nodeCount() <= 2 * 1 - 1);
	REQUIRE(bvh.root() == 0);
	const rt::accel::BvhNode& leaf = bvh.nodes().at(0);
	REQUIRE(leaf.isLeaf());
	REQUIRE(leaf.leafCount() == 1);
	REQUIRE(treeDepth(bvh, bvh.root()) <= static_cast<std::size_t>(rt::accel::Bvh::kMaxDepth));
}

TEST_CASE("bvh : 1000 spheres < 50 ms et noeuds <= 2N-1 (T060 DoD)", "[bvh][t060]") {
	auto objs = makeGridSpheres(1000);
	rt::accel::Bvh bvh;
	const auto before = std::chrono::steady_clock::now();
	REQUIRE(bvh.build(objs).isOk());
	const auto after = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before);
	INFO("build 1000 spheres : " << elapsed.count() << " ms, " << bvh.nodeCount() << " noeuds");
	REQUIRE(elapsed.count() < 50);
	REQUIRE(bvh.primCount() == 1000);
	REQUIRE(bvh.nodeCount() >= 1);
	REQUIRE(bvh.nodeCount() <= 2 * 1000 - 1);
	REQUIRE(treeDepth(bvh, bvh.root()) <= static_cast<std::size_t>(rt::accel::Bvh::kMaxDepth));
	// Les feuilles partitionnent les 1000 primitives (somme == N).
	std::size_t leafTotal = 0;
	for (const rt::accel::BvhNode& node : bvh.nodes()) {
		if (node.isLeaf()) {
			leafTotal += node.leafCount();
			REQUIRE(node.leafStart() + node.leafCount() <= 1000);
		} else {
			REQUIRE(node.left() < bvh.nodeCount());
			REQUIRE(node.right() < bvh.nodeCount());
		}
	}
	REQUIRE(leafTotal == 1000);
}

TEST_CASE("bvh : worldBounds suit la transformation (T060)", "[bvh][t060]") {
	rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), 1.0F);
	const rt::AABB local = rt::accel::Bvh::worldBounds(sphere);
	REQUIRE(local.min.x == Catch::Approx(-1.0F));
	REQUIRE(local.max.x == Catch::Approx(1.0F));

	rt::Transform moved = rt::Transform::translate(rt::Vec3(42.0F, 0.0F, 0.0F));
	sphere.setTransform(moved);
	const rt::AABB world = rt::accel::Bvh::worldBounds(sphere);
	REQUIRE(world.min.x == Catch::Approx(41.0F));
	REQUIRE(world.max.x == Catch::Approx(43.0F));
	// La boite racine d'une BVH a 2 spheres contient les deux.
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.push_back(std::make_unique<rt::geometry::Sphere>(rt::Vec3(-5, 0, 0), 1.0F));
	objs.push_back(std::make_unique<rt::geometry::Sphere>(rt::Vec3(5, 0, 0), 1.0F));
	rt::accel::Bvh bvh;
	REQUIRE(bvh.build(objs).isOk());
	const rt::accel::BvhNode& root = bvh.nodes().at(bvh.root());
	REQUIRE(root.bounds.min.x <= -6.0F);
	REQUIRE(root.bounds.max.x >= 6.0F);
}

TEST_CASE("bvh : pointeur nul refuse sans crash (T060)", "[bvh][t060]") {
	std::vector<std::unique_ptr<rt::geometry::AObject>> objs;
	objs.push_back(nullptr);
	rt::accel::Bvh bvh;
	REQUIRE(bvh.build(objs).isError());
}
