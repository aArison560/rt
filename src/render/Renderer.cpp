// Boucle de rendu mono-thread (T032) + multi-objets (T046) — implementation
// sans exception, sans SDL (R6) et sans allocation dans la boucle (R3).
// Voir `include/rt/render/Renderer.hpp` pour le contrat et
// `docs/ARCHITECTURE.md` §4 pour le pipeline (camera -> intersection ->
// framebuffer). T046 branche la recherche d'intersection : tous les objets
// de la scene (objets directs + groupes aplatis, 4 types coexistant,
// doublons autorises) sont convertis une fois en `geometry::AObject`
// (chemin froid, avant la boucle), puis chaque rayon cherche le plus proche
// (`tri par t`, `tMax` resserre). Hit -> Lambert (T033) avec le materiau de
// l'objet touche + 1ere lumiere ponctuelle + ambiance ; miss -> fond.
// Progressif T036 : batches externes + `seedFor` absolu + jitter sous-pixel
// (AA en T120) + dithering deterministe minimal (moyenne -> 0 quand spp
// grandit, conservé pour hit et miss afin que `spp 64 < spp 1` reste vrai).

#include "rt/render/Renderer.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "rt/base/Rng.hpp"
#include "rt/geometry/Cone.hpp"
#include "rt/geometry/Cylinder.hpp"
#include "rt/geometry/Plane.hpp"
#include "rt/geometry/Sphere.hpp"
#include "rt/lighting/PointLight.hpp"
#include "rt/render/Camera.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/shading/Material.hpp"

namespace rt::render {

namespace {

// Bornes de validation (T032, inchangees).
constexpr int kMinDim = 1;
constexpr int kMaxDim = 8192;
constexpr int kMinSpp = 1;
constexpr int kMaxSpp = 1024;
constexpr int kMinDepth = 0;
constexpr int kMaxDepth = 32;
constexpr long long kMinSeed = 0;
constexpr long long kMaxSeed = 4294967295LL;
// T036 : dithering deterministe minimal (±kDitherAmp/2 par echantillon,
// moyenne -> 0 quand spp grandit). Conservé en T046 pour hit et miss :
// le progressif reste observable (spp 1 plus bruite que spp 64).
constexpr float kDitherAmp = 0.06F;
// T046 : `tMin` des rayons primaires (anti-acne, au-dessus de `kEpsilon`).
constexpr Real kPrimaryTMin = 0.001F;

// Miss -> fond de scene (T032). Aucune allocation, `noexcept`.
[[nodiscard]] Vec3 shadeMiss(const scene::Scene& scene) noexcept {
	return scene.background.color;
}

// T046 : `scene::Transform` (ops dans l'ordre d'ecriture, repetables) ->
// `rt::Transform` monde. Ordre : le 1er op s'applique en 1er, donc
// `M = Op_n-1 * ... * Op_0` (prepend via `compose` : `t.compose(acc)` =
// `t * acc`). Angles `rotate` en degres (schema) -> radians (moteur).
// Axe inconnu -> op ignore (identite), jamais de `throw` (R2).
[[nodiscard]] Transform buildLocalTransform(const scene::Transform& src) noexcept {
	Transform acc;
	for (const scene::TransformOp& op : src.ops) {
		switch (op.kind) {
		case scene::TransformOp::Kind::Translate: {
			const Transform t = Transform::translate(op.translate);
			acc = t.compose(acc);
			break;
		}
		case scene::TransformOp::Kind::Scale: {
			const Transform t = Transform::scale(op.scale);
			acc = t.compose(acc);
			break;
		}
		case scene::TransformOp::Kind::Rotate: {
			const Real rad = degreesToRadians(op.rotateAngle);
			Transform t;
			if (op.rotateAxis == 'x' || op.rotateAxis == 'X') {
				t = Transform::rotateX(rad);
			} else if (op.rotateAxis == 'z' || op.rotateAxis == 'Z') {
				t = Transform::rotateZ(rad);
			} else {
				// Defaut `y` (schema) : tout autre caractere est rejete a
				// la validation, ici on applique `y` pour rester defini.
				t = Transform::rotateY(rad);
			}
			acc = t.compose(acc);
			break;
		}
		}
	}
	return acc;
}

[[nodiscard]] shading::MaterialParams toMaterialParams(const scene::Material& src) noexcept {
	shading::MaterialParams out;
	out.albedo = src.albedo;
	out.ambient = src.ambient;
	out.diffuse = src.diffuse;
	out.specular = src.specular;
	out.shininess = src.shininess;
	out.reflectivity = src.reflectivity;
	out.transparency = src.transparency;
	out.ior = src.ior;
	out.hasTexture = src.texture.present;
	out.hasPattern = src.pattern.present;
	return out;
}

[[nodiscard]] shading::AmbientParams toAmbientParams(const scene::Scene& scene) noexcept {
	shading::AmbientParams out;
	out.color = scene.ambient.color;
	out.intensity = scene.ambient.intensity;
	return out;
}

// T046 : 1ere lumiere avec position (les directionnelles sans position sont
// ignorees jusqu'en T055). Aucune -> intensite 0 = ambiant seul (defini).
// T051 : recopie aussi `attenuation`/`range` (FORMAT_SCENE.md §5.4) ; le
// facteur est evalue dans la boucle chaude via
// `lighting::attenuationFactor()` (aucune allocation, R3).
[[nodiscard]] shading::PointLightParams toPointLight(const scene::Scene& scene) noexcept {
	shading::PointLightParams out;
	out.intensity = 0.0F;
	for (const scene::Light& light : scene.lights) {
		if (!light.hasPosition) {
			continue;
		}
		out.position = light.position;
		out.color = light.color;
		out.intensity = light.intensity;
		out.attenuation = light.attenuation;
		out.range = light.range;
		return out;
	}
	return out;
}

// T046 : `scene::Object` + monde `M` -> primitive concrete. `center` vaut
// le centre (sphere/cylindre) ou l'apex (cone), `angle` est en degres
// (schema) -> radians (moteur). `axis`/`height`/`slice` ignores (T130/T133,
// documente). `matIndex` = position dans le tableau de materiaux monde.
// Retourne `nullptr` sur type inconnu (ne doit pas arriver, scene validee).
[[nodiscard]] std::unique_ptr<geometry::AObject> makeWorldObject(
    const scene::Object& src, const Transform& world, std::uint32_t id,
    std::uint32_t matIndex) {
	switch (src.type) {
	case scene::ObjectType::Sphere: {
		auto obj =
		    std::make_unique<geometry::Sphere>(src.center, src.radius, id, matIndex);
		obj->setTransform(world);
		return obj;
	}
	case scene::ObjectType::Plane: {
		auto obj = std::make_unique<geometry::Plane>(src.point, src.normal, id, matIndex);
		obj->setTransform(world);
		return obj;
	}
	case scene::ObjectType::Cylinder: {
		auto obj =
		    std::make_unique<geometry::Cylinder>(src.center, src.radius, id, matIndex);
		obj->setTransform(world);
		return obj;
	}
	case scene::ObjectType::Cone: {
		const Real halfAngle = degreesToRadians(src.angle);
		auto obj =
		    std::make_unique<geometry::Cone>(src.center, halfAngle, id, matIndex);
		obj->setTransform(world);
		return obj;
	}
	}
	return nullptr;
}

void appendOneObject(const scene::Object& src, const Transform& world,
                     std::vector<std::unique_ptr<geometry::AObject>>& outObjs,
                     std::vector<shading::MaterialParams>& outMats, std::uint32_t& nextId) {
	const std::uint32_t matIndex = static_cast<std::uint32_t>(outMats.size());
	std::unique_ptr<geometry::AObject> obj = makeWorldObject(src, world, nextId, matIndex);
	if (!obj) {
		return;
	}
	outMats.push_back(toMaterialParams(src.material));
	outObjs.push_back(std::move(obj));
	++nextId;
}

// T046 : aplatit un groupe (objets + enfants) sous `parentWorld`.
// `groupWorld = parentWorld * local`, chaque objet vaut
// `groupWorld * objLocal`. `nextId` fournit `id` unique et croissant.
void appendGroupObjects(const scene::Group& group, const Transform& parentWorld,
                        std::vector<std::unique_ptr<geometry::AObject>>& outObjs,
                        std::vector<shading::MaterialParams>& outMats,
                        std::uint32_t& nextId) {
	const Transform local = buildLocalTransform(group.transform);
	const Transform groupWorld = parentWorld.compose(local);
	for (const scene::Object& obj : group.objects) {
		const Transform objLocal = buildLocalTransform(obj.transform);
		const Transform objWorld = groupWorld.compose(objLocal);
		appendOneObject(obj, objWorld, outObjs, outMats, nextId);
	}
	for (const std::shared_ptr<scene::Group>& child : group.children) {
		if (child) {
			appendGroupObjects(*child, groupWorld, outObjs, outMats, nextId);
		}
	}
}

// T046 : scene -> objets monde (chemin froid, avant la boucle chaude).
// Reserve `totalObjectCount()` (aucune realloc pour les scenes courantes,
// memoire bornee par `limits` via la validation T024).
void collectSceneObjects(const scene::Scene& scene,
                         std::vector<std::unique_ptr<geometry::AObject>>& outObjs,
                         std::vector<shading::MaterialParams>& outMats) {
	outObjs.clear();
	outMats.clear();
	const std::size_t total = scene.totalObjectCount();
	outObjs.reserve(total);
	outMats.reserve(total);
	std::uint32_t nextId = 0;
	const Transform identity;
	for (const scene::Object& obj : scene.objects) {
		const Transform objWorld = buildLocalTransform(obj.transform);
		appendOneObject(obj, objWorld, outObjs, outMats, nextId);
	}
	for (const scene::Group& group : scene.groups) {
		appendGroupObjects(group, identity, outObjs, outMats, nextId);
	}
}

// T046 : plus proche parmi tous les objets (tri par `t`). `tMax` se
// resserre au plus proche trouve : le gagnant est le `t` minimal dans
// `[tMin, tMax]`. Plusieurs objets du meme type + coexistence des 4 types.
// `noexcept`, sans allocation (R2/R3) : `HitRecord` sur pile uniquement.
[[nodiscard]] bool findClosestHit(const std::vector<std::unique_ptr<geometry::AObject>>& objs,
                                  const Ray& ray, Real tMin, Real tMax,
                                  HitRecord& outRec) noexcept {
	bool hit = false;
	Real closest = tMax;
	HitRecord tmp;
	for (const std::unique_ptr<geometry::AObject>& obj : objs) {
		if (obj && obj->intersect(ray, tMin, closest, tmp)) {
			outRec = tmp;
			closest = tmp.t;
			hit = true;
		}
	}
	return hit;
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
	// Chemin froid (T046) : conversion scene -> objets monde (geometrie +
	// materiaux), lumiere et ambiance. Allocation unique avant la boucle ;
	// la boucle chaude n'alloue plus (R3).
	std::vector<std::unique_ptr<geometry::AObject>> worldObjs;
	std::vector<shading::MaterialParams> worldMats;
	collectSceneObjects(scene, worldObjs, worldMats);
	const shading::PointLightParams pointLight = toPointLight(scene);
	const shading::AmbientParams ambient = toAmbientParams(scene);
	// Boucle chaude : registres + pile uniquement (R3), batches externes (T036).
	// Chaque echantillon `s` utilise `rngFor(x, y, s, seed)` (coordonnees
	// absolues, T016) : jitter sous-pixel pour le rayon + dithering pour
	// la couleur (hit comme miss). Memes `spp` + meme `seed` -> memes
	// pixels, octet par octet, que ce soit en plein ou par tuile/bande
	// (couture impossible). `onProgress(s+1, spp)` apres chaque batch
	// (1 appel par batch, pas par pixel : hors hot path fin).
	const auto sceneSeed = static_cast<std::uint32_t>(params.seed);
	for (int s = 0; s < params.spp; ++s) {
		for (int y = 0; y < params.height; ++y) {
			for (int x = 0; x < params.width; ++x) {
				Rng rng = rngFor(x, y, s, sceneSeed);
				const Vec2 jitter(rng.nextFloat() - 0.5F, rng.nextFloat() - 0.5F);
				const Ray ray = camera.rayForPixel(x, y, jitter);
				HitRecord rec;
				Vec3 color;
				if (findClosestHit(worldObjs, ray, kPrimaryTMin, kInfinity, rec)) {
					shading::MaterialParams mat;
					if (rec.materialIndex < worldMats.size()) {
						mat = worldMats[rec.materialIndex];
					}
					// T051 : attenuation ponctuelle (FORMAT_SCENE.md §5.4).
					// `att` ne depend que de la distance monde (registres, R3) ;
					// `(1,0,0)` + `range 0` -> 1 (scenes existantes inchangees).
					// Degeneres (NaN/Inf) -> 0 = ambiant seul, jamais de NaN.
					shading::PointLightParams effLight = pointLight;
					const Vec3 toLight = pointLight.position - rec.point;
					const float dist =
					    std::isfinite(toLight.x) && std::isfinite(toLight.y) &&
					            std::isfinite(toLight.z)
					        ? length(toLight)
					        : std::numeric_limits<float>::quiet_NaN();
					const float att = lighting::attenuationFactor(
					    pointLight.attenuation, dist, pointLight.range);
					effLight.intensity = pointLight.intensity * att;
					color = shading::shadeLambert(mat, rec.normal, rec.point, effLight,
					                              ambient);
				} else {
					color = shadeMiss(scene);
				}
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
