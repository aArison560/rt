// Tests de l'interface des objets (T040) — dispatch vtable, Catch2.
// DoD : test de dispatch (appel via `AObject*` -> surcharge derivee) +
// `static_assert` sur `HitRecord` (verifie a la compilation dans
// `include/rt/geometry/Object.hpp`) + grep d'intersection sans macro
// (aucune macro generique, exige par la fiche M3).

#include <catch2/catch_amalgamated.hpp>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "rt/base/Ray.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Object.hpp"

namespace {

// Doubles minimales prouvant que le dispatch est virtuel (pas de switch,
// pas de macro) : chacune remplit `HitRecord` a sa facon.
class StubSphere final : public rt::geometry::AObject {
public:
	StubSphere() noexcept : AObject(rt::geometry::ObjectKind::Sphere, 1, 0) {}
	bool intersect(const rt::Ray& ray, rt::Real tMin, rt::Real tMax,
	               rt::HitRecord& rec) const noexcept override {
		(void)ray;
		(void)tMin;
		(void)tMax;
		rec.t = rt::Real(1);
		rec.materialIndex = materialIndex();
		rec.frontFace = true;
		return true;
	}
	rt::AABB localBounds() const noexcept override {
		return rt::AABB(rt::Vec3(-1, -1, -1), rt::Vec3(1, 1, 1));
	}
};

class StubPlane final : public rt::geometry::AObject {
public:
	StubPlane() noexcept : AObject(rt::geometry::ObjectKind::Plane, 2, 7) {}
	bool intersect(const rt::Ray& ray, rt::Real tMin, rt::Real tMax,
	               rt::HitRecord& rec) const noexcept override {
		(void)ray;
		(void)tMin;
		(void)tMax;
		rec.t = rt::Real(2);
		rec.materialIndex = materialIndex();
		rec.frontFace = false;
		return true;
	}
	rt::AABB localBounds() const noexcept override {
		return rt::AABB(rt::Vec3(-2, -2, -2), rt::Vec3(2, 2, 2));
	}
};

} // namespace

TEST_CASE("geometry : dispatch virtuel par type (T040)", "[geometry]") {
	std::vector<std::unique_ptr<rt::geometry::AObject>> objects;
	objects.push_back(std::make_unique<StubSphere>());
	objects.push_back(std::make_unique<StubPlane>());

	REQUIRE(objects[0]->kind() == rt::geometry::ObjectKind::Sphere);
	REQUIRE(objects[1]->kind() == rt::geometry::ObjectKind::Plane);
	REQUIRE(objects[0]->id() == 1);
	REQUIRE(objects[1]->id() == 2);
	REQUIRE(objects[1]->materialIndex() == 7);

	const rt::Ray ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1));
	rt::HitRecord rec;
	REQUIRE(objects[0]->intersect(ray, rt::Real(0), rt::kInfinity, rec));
	REQUIRE(rec.t == rt::Real(1));
	REQUIRE(rec.frontFace);
	REQUIRE(objects[1]->intersect(ray, rt::Real(0), rt::kInfinity, rec));
	REQUIRE(rec.t == rt::Real(2));
	REQUIRE_FALSE(rec.frontFace);

	// `localBounds` est aussi specifique a chaque type.
	const rt::AABB sphereBox = objects[0]->localBounds();
	REQUIRE(sphereBox.min.x == rt::Real(-1));
	REQUIRE(sphereBox.max.x == rt::Real(1));
	const rt::AABB planeBox = objects[1]->localBounds();
	REQUIRE(planeBox.min.x == rt::Real(-2));

	// Champs communs : transform identite par defaut (T045 l'exploitera),
	// `objectToWorld` accessible, modifiable sans changer le dispatch.
	const rt::Transform identity = rt::Transform();
	const rt::Vec3 moved = objects[0]->objectToWorld().applyPoint(rt::Vec3(1, 2, 3));
	REQUIRE(moved.x == Catch::Approx(identity.applyPoint(rt::Vec3(1, 2, 3)).x));
	objects[0]->setTransform(rt::Transform::translate(rt::Vec3(1, 0, 0)));
	REQUIRE(objects[0]->objectToWorld().applyPoint(rt::Vec3(0, 0, 0)).x == Catch::Approx(1.0F));
	objects[0]->setMaterialIndex(3);
	REQUIRE(objects[0]->materialIndex() == 3);
	REQUIRE(std::string(rt::geometry::toString(rt::geometry::ObjectKind::Sphere)) == "sphere");
	REQUIRE(std::string(rt::geometry::toString(rt::geometry::ObjectKind::Plane)) == "plane");
	REQUIRE(std::string(rt::geometry::toString(rt::geometry::ObjectKind::Cylinder)) == "cylinder");
	REQUIRE(std::string(rt::geometry::toString(rt::geometry::ObjectKind::Cone)) == "cone");
}

TEST_CASE("geometry : HitRecord reste un POD compact (T040)", "[geometry]") {
	REQUIRE(std::is_trivially_copyable_v<rt::HitRecord>);
	REQUIRE(std::is_standard_layout_v<rt::HitRecord>);
}
