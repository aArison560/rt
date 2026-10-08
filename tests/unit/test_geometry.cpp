// Tests de l'interface des objets (T040) + sphere (T041) + plan (T042) + cylindre (T043), Catch2.
// DoD T040 : test de dispatch (appel via `AObject*` -> surcharge derivee) +
// `static_assert` sur `HitRecord` (verifie a la compilation dans
// `include/rt/geometry/Object.hpp`) + grep d'intersection sans macro
// (aucune macro generique, exige par la fiche M3).
// DoD T041 : 6 cas sphere (tangent, interieur, manquant, hors bornes,
// centre exact, tres loin) + degeneres, sans exception, ASan propre.
// DoD T042 : plan (parallele, dans le plan, avant/apres) + degeneres,
// sans division par zero (UBSan vert).

#include <catch2/catch_amalgamated.hpp>

#include <cmath>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "rt/base/Ray.hpp"
#include "rt/base/Vec.hpp"
#include "rt/geometry/Cylinder.hpp"
#include "rt/geometry/Object.hpp"
#include "rt/geometry/Plane.hpp"
#include "rt/geometry/Sphere.hpp"

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

namespace {

bool nearReal(rt::Real a, rt::Real b, float margin = 1e-4F) {
	return a == Catch::Approx(b).margin(static_cast<double>(margin));
}

} // namespace

TEST_CASE("geometry sphere : centre exact, face avant (T041)", "[geometry][sphere]") {
	const rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), rt::Real(1), 10, 4);
	const rt::Ray ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1));
	rt::HitRecord rec;
	REQUIRE(sphere.intersect(ray, rt::Real(0), rt::kInfinity, rec));
	REQUIRE(nearReal(rec.t, rt::Real(4)));
	REQUIRE(nearReal(rec.point.x, rt::Real(0)));
	REQUIRE(nearReal(rec.point.y, rt::Real(0)));
	REQUIRE(nearReal(rec.point.z, rt::Real(-1)));
	REQUIRE(rec.frontFace);
	// Normale sortante (0,0,-1) contre le rayon (0,0,1) : unitaire.
	REQUIRE(nearReal(rt::length(rec.normal), rt::Real(1)));
	REQUIRE(nearReal(rec.normal.z, rt::Real(-1)));
	REQUIRE(rec.materialIndex == 4);
	// `uv` dans [0,1], finis (texture T103).
	REQUIRE(std::isfinite(rec.uv.x));
	REQUIRE(std::isfinite(rec.uv.y));
	REQUIRE(rec.uv.x >= rt::Real(0));
	REQUIRE(rec.uv.x <= rt::Real(1));
	REQUIRE(rec.uv.y >= rt::Real(0));
	REQUIRE(rec.uv.y <= rt::Real(1));
	// Dispatch via la base : meme resultat par `AObject*`.
	const rt::geometry::AObject& base = sphere;
	rt::HitRecord viaBase;
	REQUIRE(base.intersect(ray, rt::Real(0), rt::kInfinity, viaBase));
	REQUIRE(nearReal(viaBase.t, rec.t));
}

TEST_CASE("geometry sphere : rayon tangent (discriminant nul, T041)", "[geometry][sphere]") {
	const rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), rt::Real(1));
	const rt::Ray ray(rt::Vec3(1, -5, 0), rt::normalize(rt::Vec3(0, 1, 0)));
	rt::HitRecord rec;
	REQUIRE(sphere.intersect(ray, rt::Real(0), rt::kInfinity, rec));
	REQUIRE(nearReal(rec.t, rt::Real(5), 1e-3F));
	REQUIRE(nearReal(rec.point.x, rt::Real(1), 1e-3F));
	REQUIRE(nearReal(rec.point.y, rt::Real(0), 1e-3F));
	REQUIRE(nearReal(rt::length(rec.normal), rt::Real(1)));
}

TEST_CASE("geometry sphere : rayon partant de l'interieur (T041)", "[geometry][sphere]") {
	const rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), rt::Real(2));
	const rt::Ray inside(rt::Vec3(0, 0, 0), rt::Vec3(0, 0, 1));
	rt::HitRecord rec;
	REQUIRE(sphere.intersect(inside, rt::Real(0), rt::kInfinity, rec));
	REQUIRE(nearReal(rec.t, rt::Real(2)));
	// Sortie par l'interieur : `frontFace` false, normale retournee
	// contre le rayon (0,0,-1) alors que la geometrique est (0,0,+1).
	REQUIRE_FALSE(rec.frontFace);
	REQUIRE(nearReal(rec.normal.z, rt::Real(-1)));
	REQUIRE(nearReal(rec.point.z, rt::Real(2)));
}

TEST_CASE("geometry sphere : manquant et hors bornes (T041)", "[geometry][sphere]") {
	const rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), rt::Real(1));
	rt::HitRecord rec;
	// A cote : ligne x=0, z=-5, direction +y (distance 5 au centre).
	REQUIRE_FALSE(sphere.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 1, 0)), rt::Real(0),
	                               rt::kInfinity, rec));
	// Devant : meme rayon que le centre exact, mais fenetre trop courte.
	REQUIRE_FALSE(sphere.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1)), rt::Real(0),
	                               rt::Real(3), rec));
	// Derriere : fenetre apres les deux racines (4 et 6).
	REQUIRE_FALSE(sphere.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1)), rt::Real(7),
	                               rt::Real(10), rec));
	// Fenetre inversee : defini comme miss, sans crash.
	REQUIRE_FALSE(sphere.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1)), rt::Real(5),
	                               rt::Real(3), rec));
}

TEST_CASE("geometry sphere : tres loin, stable en double (T041)", "[geometry][sphere]") {
	const rt::geometry::Sphere sphere(rt::Vec3(0, 0, 1000), rt::Real(1));
	const rt::Ray ray(rt::Vec3(0, 0, 0), rt::Vec3(0, 0, 1));
	rt::HitRecord rec;
	REQUIRE(sphere.intersect(ray, rt::Real(0), rt::kInfinity, rec));
	REQUIRE(nearReal(rec.t, rt::Real(999), 1e-2F));
	REQUIRE(rec.frontFace);
	REQUIRE(nearReal(rt::length(rec.normal), rt::Real(1)));
	// Origine lointaine symetrique : meme stabilite.
	const rt::geometry::Sphere near(rt::Vec3(0, 0, 0), rt::Real(1));
	const rt::Ray far(rt::Vec3(0, 0, -1000000), rt::Vec3(0, 0, 1));
	rt::HitRecord recFar;
	REQUIRE(near.intersect(far, rt::Real(0), rt::kInfinity, recFar));
	REQUIRE(recFar.t > rt::Real(0));
	REQUIRE(std::isfinite(recFar.t));
}

TEST_CASE("geometry sphere : degeneres sans crash (T041)", "[geometry][sphere]") {
	rt::HitRecord rec;
	// Rayon nul (v1 : division par zero) -> miss defini.
	const rt::geometry::Sphere sphere(rt::Vec3(0, 0, 0), rt::Real(1));
	REQUIRE_FALSE(sphere.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 0)), rt::Real(0),
	                               rt::kInfinity, rec));
	// Rayon quasi nul -> miss defini.
	REQUIRE_FALSE(
	    sphere.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(rt::kEpsilon * 0.1F, 0, 0)),
	                     rt::Real(0), rt::kInfinity, rec));
	// Sphere degeneree (rayon nul ou negatif) -> miss defini.
	const rt::geometry::Sphere flat(rt::Vec3(0, 0, 0), rt::Real(0));
	REQUIRE_FALSE(flat.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1)), rt::Real(0),
	                             rt::kInfinity, rec));
	const rt::geometry::Sphere negative(rt::Vec3(0, 0, 0), rt::Real(-1));
	REQUIRE_FALSE(negative.intersect(rt::Ray(rt::Vec3(0, 0, -5), rt::Vec3(0, 0, 1)), rt::Real(0),
	                                 rt::kInfinity, rec));
	// `localBounds` exacte : centre +- rayon.
	const rt::geometry::Sphere placed(rt::Vec3(1, 2, 3), rt::Real(2));
	const rt::AABB box = placed.localBounds();
	REQUIRE(nearReal(box.min.x, rt::Real(-1)));
	REQUIRE(nearReal(box.min.y, rt::Real(0)));
	REQUIRE(nearReal(box.min.z, rt::Real(1)));
	REQUIRE(nearReal(box.max.x, rt::Real(3)));
	REQUIRE(nearReal(box.max.y, rt::Real(4)));
	REQUIRE(nearReal(box.max.z, rt::Real(5)));
}

TEST_CASE("geometry plane : avant (dessus) et arriere (T042)", "[geometry][plane]") {
	const rt::geometry::Plane plane(rt::Vec3(0, 0, 0), rt::Vec3(0, 1, 0), 20, 5);
	// Dessus : origine a y=5, direction -y -> t=5, face avant.
	rt::HitRecord rec;
	REQUIRE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                        rt::kInfinity, rec));
	REQUIRE(rec.t == Catch::Approx(5.0).margin(1e-4));
	REQUIRE(nearReal(rec.point.y, rt::Real(0)));
	REQUIRE(rec.frontFace);
	REQUIRE(nearReal(rt::length(rec.normal), rt::Real(1)));
	REQUIRE(nearReal(rec.normal.y, rt::Real(1)));
	REQUIRE(rec.materialIndex == 5);
	REQUIRE(std::isfinite(rec.uv.x));
	REQUIRE(std::isfinite(rec.uv.y));
	// Derriere : meme origine, direction +y (s'eloigne) -> miss.
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, 1, 0)), rt::Real(0),
	                              rt::kInfinity, rec));
	// Dessous : origine a y=-5, direction +y -> t=5, face arriere.
	rt::HitRecord below;
	REQUIRE(plane.intersect(rt::Ray(rt::Vec3(0, -5, 0), rt::Vec3(0, 1, 0)), rt::Real(0),
	                        rt::kInfinity, below));
	REQUIRE(below.t == Catch::Approx(5.0).margin(1e-4));
	REQUIRE_FALSE(below.frontFace);
	// Normale retournee contre le rayon : (0,-1,0).
	REQUIRE(nearReal(below.normal.y, rt::Real(-1)));
	// Dispatch via la base.
	const rt::geometry::AObject& base = plane;
	rt::HitRecord viaBase;
	REQUIRE(base.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                       rt::kInfinity, viaBase));
	REQUIRE(viaBase.t == Catch::Approx(5.0).margin(1e-4));
}

TEST_CASE("geometry plane : parallele et quasi parallele, sans division par zero (T042)",
          "[geometry][plane]") {
	const rt::geometry::Plane plane(rt::Vec3(0, 0, 0), rt::Vec3(0, 1, 0));
	rt::HitRecord rec;
	// Strictement parallele : direction dans le plan.
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(1, 0, 0)), rt::Real(0),
	                              rt::kInfinity, rec));
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, 0, 1)), rt::Real(0),
	                              rt::kInfinity, rec));
	// Quasi parallele : |denom| = 1e-7 < kEpsilon (1e-6) -> miss defini.
	const rt::Vec3 almost = rt::normalize(rt::Vec3(1, rt::kEpsilon * 0.1F, 0));
	REQUIRE_FALSE(
	    plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), almost), rt::Real(0), rt::kInfinity, rec));
	// Direction nulle (v1 : division) -> miss defini.
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, 0, 0)), rt::Real(0),
	                              rt::kInfinity, rec));
	// Fenetre trop courte : hit a t=5 hors [0,3].
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                              rt::Real(3), rec));
	// Fenetre inversee -> miss defini.
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(6),
	                              rt::Real(3), rec));
}

TEST_CASE("geometry plane : dans le plan et uv coherents (T042)", "[geometry][plane]") {
	const rt::geometry::Plane plane(rt::Vec3(0, 0, 0), rt::Vec3(0, 1, 0));
	rt::HitRecord rec;
	// Origine dans le plan, direction dans le plan -> miss (pas de `t` isole).
	REQUIRE_FALSE(plane.intersect(rt::Ray(rt::Vec3(1, 0, 2), rt::Vec3(1, 0, 0)), rt::Real(0),
	                              rt::kInfinity, rec));
	// Deux impacts a des positions differentes -> `uv` differents
	// (au moins une coordonnee change selon l'axe tangent).
	rt::HitRecord first;
	rt::HitRecord second;
	REQUIRE(plane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                        rt::kInfinity, first));
	REQUIRE(plane.intersect(rt::Ray(rt::Vec3(3, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                        rt::kInfinity, second));
	REQUIRE((first.uv.x != second.uv.x || first.uv.y != second.uv.y));
	REQUIRE(std::isfinite(first.uv.x));
	REQUIRE(std::isfinite(first.uv.y));
	// `localBounds` documentee : ±1e6.
	const rt::AABB box = plane.localBounds();
	REQUIRE(box.min.x == Catch::Approx(-1e6).margin(1.0));
	REQUIRE(box.max.x == Catch::Approx(1e6).margin(1.0));
	REQUIRE(box.min.y == Catch::Approx(-1e6).margin(1.0));
	REQUIRE(box.max.y == Catch::Approx(1e6).margin(1.0));
}

TEST_CASE("geometry plane : normale degeneree et plan vertical (T042)", "[geometry][plane]") {
	rt::HitRecord rec;
	// Constructeur a normale nulle -> repli (0,1,0), plan defini.
	const rt::geometry::Plane fallback(rt::Vec3(0, 0, 0), rt::Vec3(0, 0, 0));
	REQUIRE(fallback.normal().y == Catch::Approx(1.0).margin(1e-5));
	REQUIRE(fallback.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                           rt::kInfinity, rec));
	// `setNormal` nulle : garde l'ancienne, toujours defini.
	rt::geometry::Plane mutablePlane(rt::Vec3(0, 0, 0), rt::Vec3(0, 1, 0));
	mutablePlane.setNormal(rt::Vec3(0, 0, 0));
	REQUIRE(mutablePlane.intersect(rt::Ray(rt::Vec3(0, 5, 0), rt::Vec3(0, -1, 0)), rt::Real(0),
	                               rt::kInfinity, rec));
	// Plan vertical (normale +x) : helper bascule sur (1,0,0) -> `uv` finis.
	const rt::geometry::Plane vertical(rt::Vec3(0, 0, 0), rt::Vec3(1, 0, 0));
	rt::HitRecord side;
	REQUIRE(vertical.intersect(rt::Ray(rt::Vec3(-5, 1, 2), rt::Vec3(1, 0, 0)), rt::Real(0),
	                           rt::kInfinity, side));
	REQUIRE(side.t == Catch::Approx(5.0).margin(1e-4));
	REQUIRE(std::isfinite(side.uv.x));
	REQUIRE(std::isfinite(side.uv.y));
}

TEST_CASE("geometry cylinder : face avant, normale radiale (T043)", "[geometry][cylinder]") {
	const rt::geometry::Cylinder cylinder(rt::Vec3(0, 0, 0), rt::Real(1), 30, 6);
	// Depuis +x vers l'axe : t=1, point (1,0,0), face avant.
	rt::HitRecord rec;
	REQUIRE(cylinder.intersect(rt::Ray(rt::Vec3(2, 0, 0), rt::Vec3(-1, 0, 0)), rt::Real(0),
	                           rt::kInfinity, rec));
	REQUIRE(rec.t == Catch::Approx(1.0).margin(1e-4));
	REQUIRE(nearReal(rec.point.x, rt::Real(1)));
	REQUIRE(nearReal(rec.point.y, rt::Real(0)));
	REQUIRE(rec.frontFace);
	REQUIRE(nearReal(rt::length(rec.normal), rt::Real(1)));
	REQUIRE(nearReal(rec.normal.x, rt::Real(1)));
	REQUIRE(nearReal(rec.normal.y, rt::Real(0)));
	REQUIRE(rec.materialIndex == 6);
	// `uv` cylindriques : `u` dans [0,1], `v` = hauteur, finis.
	REQUIRE(std::isfinite(rec.uv.x));
	REQUIRE(std::isfinite(rec.uv.y));
	REQUIRE(rec.uv.x >= rt::Real(0));
	REQUIRE(rec.uv.x <= rt::Real(1));
	// Dispatch via la base : meme resultat par `AObject*`.
	const rt::geometry::AObject& base = cylinder;
	rt::HitRecord viaBase;
	REQUIRE(base.intersect(rt::Ray(rt::Vec3(2, 0, 0), rt::Vec3(-1, 0, 0)), rt::Real(0),
	                       rt::kInfinity, viaBase));
	REQUIRE(viaBase.t == Catch::Approx(1.0).margin(1e-4));
}

TEST_CASE("geometry cylinder : tangent (discriminant nul, T043)", "[geometry][cylinder]") {
	const rt::geometry::Cylinder cylinder(rt::Vec3(0, 0, 0), rt::Real(1));
	// Droite x=1, z=-5 -> +z : tangente au fut en (1,0,0), t=5.
	rt::HitRecord rec;
	REQUIRE(cylinder.intersect(rt::Ray(rt::Vec3(1, 0, -5), rt::Vec3(0, 0, 1)), rt::Real(0),
	                           rt::kInfinity, rec));
	REQUIRE(rec.t == Catch::Approx(5.0).margin(1e-3));
	REQUIRE(nearReal(rec.point.x, rt::Real(1), 1e-3F));
	REQUIRE(nearReal(rec.point.z, rt::Real(0), 1e-3F));
	REQUIRE(nearReal(rt::length(rec.normal), rt::Real(1)));
}

TEST_CASE("geometry cylinder : rayon partant de l'interieur (T043)", "[geometry][cylinder]") {
	const rt::geometry::Cylinder cylinder(rt::Vec3(0, 0, 0), rt::Real(2));
	// Depuis l'axe vers +x : sortie a t=2, `frontFace` false.
	rt::HitRecord rec;
	REQUIRE(cylinder.intersect(rt::Ray(rt::Vec3(0, 0, 0), rt::Vec3(1, 0, 0)), rt::Real(0),
	                           rt::kInfinity, rec));
	REQUIRE(nearReal(rec.t, rt::Real(2)));
	REQUIRE_FALSE(rec.frontFace);
	// Normale retournee contre le rayon : (-1,0,0).
	REQUIRE(nearReal(rec.normal.x, rt::Real(-1)));
	REQUIRE(nearReal(rec.point.x, rt::Real(2)));
	// Fenetre trop courte : hit a t=2 hors [0,1].
	REQUIRE_FALSE(cylinder.intersect(rt::Ray(rt::Vec3(0, 0, 0), rt::Vec3(1, 0, 0)), rt::Real(0),
	                                 rt::Real(1), rec));
}

TEST_CASE("geometry cylinder : axial et parallele a l'axe, sans division (T043)",
          "[geometry][cylinder]") {
	const rt::geometry::Cylinder cylinder(rt::Vec3(0, 0, 0), rt::Real(1));
	rt::HitRecord rec;
	// Sur l'axe, le long de Y : longe le fut sans le couper -> miss defini.
	REQUIRE_FALSE(cylinder.intersect(rt::Ray(rt::Vec3(0, -5, 0), rt::Vec3(0, 1, 0)), rt::Real(0),
	                                 rt::kInfinity, rec));
	// Parallele a l'axe depuis l'exterieur : miss defini.
	REQUIRE_FALSE(cylinder.intersect(rt::Ray(rt::Vec3(2, -5, 0), rt::Vec3(0, 1, 0)), rt::Real(0),
	                                 rt::kInfinity, rec));
	// Direction nulle (v1 : division) -> miss defini.
	REQUIRE_FALSE(cylinder.intersect(rt::Ray(rt::Vec3(2, 0, 0), rt::Vec3(0, 0, 0)), rt::Real(0),
	                                 rt::kInfinity, rec));
	// A cote : ligne z=-5, direction +x (distance 5 a l'axe) -> miss.
	REQUIRE_FALSE(cylinder.intersect(rt::Ray(rt::Vec3(0, 3, -5), rt::Vec3(1, 0, 0)), rt::Real(0),
	                                 rt::kInfinity, rec));
	// Fenetre inversee -> miss defini.
	REQUIRE_FALSE(cylinder.intersect(rt::Ray(rt::Vec3(2, 0, 0), rt::Vec3(-1, 0, 0)),
	                                 rt::Real(5), rt::Real(3), rec));
}

TEST_CASE("geometry cylinder : degenere et localBounds (T043)", "[geometry][cylinder]") {
	rt::HitRecord rec;
	// Rayon nul ou negatif -> miss defini.
	const rt::geometry::Cylinder flat(rt::Vec3(0, 0, 0), rt::Real(0));
	REQUIRE_FALSE(flat.intersect(rt::Ray(rt::Vec3(2, 0, 0), rt::Vec3(-1, 0, 0)), rt::Real(0),
	                             rt::kInfinity, rec));
	const rt::geometry::Cylinder negative(rt::Vec3(0, 0, 0), rt::Real(-1));
	REQUIRE_FALSE(negative.intersect(rt::Ray(rt::Vec3(2, 0, 0), rt::Vec3(-1, 0, 0)),
	                                   rt::Real(0), rt::kInfinity, rec));
	// Centre deporte : l'axe passe par (3,*,0), meme intersection decalee.
	const rt::geometry::Cylinder moved(rt::Vec3(3, 0, 0), rt::Real(1));
	rt::HitRecord off;
	REQUIRE(moved.intersect(rt::Ray(rt::Vec3(5, 0, 0), rt::Vec3(-1, 0, 0)), rt::Real(0),
	                        rt::kInfinity, off));
	REQUIRE(off.t == Catch::Approx(1.0).margin(1e-4));
	REQUIRE(nearReal(off.point.x, rt::Real(4)));
	// `localBounds` : x/z serres (cx±r), y = ±1e6 (infini documente).
	const rt::AABB box = moved.localBounds();
	REQUIRE(box.min.x == Catch::Approx(2.0).margin(1e-4));
	REQUIRE(box.max.x == Catch::Approx(4.0).margin(1e-4));
	REQUIRE(box.min.z == Catch::Approx(-1.0).margin(1e-4));
	REQUIRE(box.max.z == Catch::Approx(1.0).margin(1e-4));
	REQUIRE(box.min.y == Catch::Approx(-1e6).margin(1.0));
	REQUIRE(box.max.y == Catch::Approx(1e6).margin(1.0));
}
