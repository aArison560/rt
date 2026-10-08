#pragma once

// Interface des objets (T040) — dispatch par table virtuelle (vtable).
// Chaque primitive possede SA fonction d'intersection (exigence M3 de la
// fiche : « pas une macro generique ») ; le choix vtable vs std::variant
// est tranche dans `docs/ADR/002-dispatch.md` : vtable (heritage + virtuel
// pur). Aucune macro d'intersection dans `src/geometry/` (DoD), aucun
// `throw` (R2), aucune allocation dans `intersect` (R3 : registres + pile).
// Calque `geometry/` : ne voit que `base/` (regle d'or §2.1) — jamais
// `scene/` : la conversion `scene::Object -> geometry::*` se fera en T046.
// Approche A retenue (T012, ARCHITECTURE §4.3) et branchee en T045 (M4) :
// le rayon monde est ramene en espace objet par `M⁻¹` (sans renormaliser
// la direction, donc `t` est conserve : `rayonObjet(t) = M⁻¹·rayonMonde(t)`),
// l'intersection s'y fait avec les parametres objet (centre, rayon, ...),
// puis le point revient par `M` et la normale par `(M⁻¹)ᵀ` normalisee
// (`applyNormal`). `frontFace` est recalcule en monde (meme signe qu'en
// objet : `dot(M·D, (M⁻¹)ᵀ·N) = dot(D, N)`). `M` singuliere -> miss
// defini (`nullopt`, jamais de `throw`). `localBounds()` reste en espace
// objet (la BVH T060+ la transformera).

#include <cstdint>
#include <optional>
#include <type_traits>

#include "rt/base/Mat4.hpp"
#include "rt/base/Ray.hpp"

namespace rt::geometry {

enum class ObjectKind : std::uint8_t {
	Sphere = 0,
	Plane = 1,
	Cylinder = 2,
	Cone = 3,
};

[[nodiscard]] const char* toString(ObjectKind kind) noexcept;

class AObject {
public:
	AObject() noexcept = default;
	AObject(ObjectKind kind, std::uint32_t id, std::uint32_t materialIndex) noexcept
	    : kind_(kind), id_(id), materialIndex_(materialIndex) {}
	virtual ~AObject() = default;

	AObject(const AObject&) = default;
	AObject& operator=(const AObject&) = default;
	AObject(AObject&&) = default;
	AObject& operator=(AObject&&) = default;

	// Intersection specifique a chaque type : la plus proche dans
	// [tMin, tMax], `rec` rempli (point, normale contre le rayon via
	// `setFaceNormal`, `t`, `frontFace`, `uv`, `materialIndex`).
	// Retourne false si aucun impact valide (miss, hors bornes, degenere).
	// `noexcept`, sans allocation, sans `throw` (R2/R3).
	virtual bool intersect(const Ray& ray, Real tMin, Real tMax, HitRecord& rec) const noexcept = 0;

	// Boite englobante en espace objet (exacte pour les bornes ;
	// infinie -> grande boite documentee par la primitive, cf. Plane).
	virtual AABB localBounds() const noexcept = 0;

	[[nodiscard]] ObjectKind kind() const noexcept { return kind_; }
	[[nodiscard]] std::uint32_t id() const noexcept { return id_; }
	[[nodiscard]] std::uint32_t materialIndex() const noexcept { return materialIndex_; }
	[[nodiscard]] const Transform& objectToWorld() const noexcept { return objectToWorld_; }

	void setId(std::uint32_t id) noexcept { id_ = id; }
	void setMaterialIndex(std::uint32_t index) noexcept { materialIndex_ = index; }
	// Translation/rotation depuis le schema (T045 l'exploite) : remplace
	// la matrice monde (identite par defaut, donc monde == objet sans
	// `setTransform`). Voir les helpers ci-dessous pour le passage
	// monde <-> objet (approche A, M4).
	void setTransform(const Transform& transform) noexcept { objectToWorld_ = transform; }

	// Ramene un rayon monde en espace objet : `O' = M⁻¹·O` (point),
	// `D' = M⁻¹·D` (vecteur, volontairement non renormalise pour que
	// `t` soit conserve). `nullopt` si `M` non inversible -> l'appelant
	// renvoie `false` (miss defini). `noexcept`, sans allocation (R2/R3).
	[[nodiscard]] std::optional<Ray> worldToObjectRay(const Ray& worldRay) const noexcept;
	// Point objet -> monde (`M`, w=1). `noexcept`, sans allocation.
	[[nodiscard]] Vec3 objectToWorldPoint(Vec3 p) const noexcept;
	// Normale objet -> monde (`(M⁻¹)ᵀ` puis normalisee ; vecteur nul si
	// singuliere). `noexcept`, sans allocation.
	[[nodiscard]] Vec3 objectToWorldNormal(Vec3 n) const noexcept;

protected:
	ObjectKind kind_ = ObjectKind::Sphere;
	std::uint32_t id_ = 0;
	std::uint32_t materialIndex_ = 0;
	Transform objectToWorld_;
};

// DoD T040 : `static_assert` sur `HitRecord` (POD compact de T013,
// rempli par chaque `intersect`). La taille est consignee dans
// `docs/ARCHITECTURE.md` §3.2 (44 o en `Real = float`).
static_assert(std::is_trivially_copyable_v<HitRecord>);
static_assert(std::is_standard_layout_v<HitRecord>);

} // namespace rt::geometry
