// Boucle de rendu mono-thread (T032) + multi-objets (T046) + reflexion (T056)
// + refraction (T057) + ombres transparentes et spot aveuglant (T058) —
// implementation sans exception, sans SDL (R6) et sans allocation dans la
// boucle (R3). Voir `include/rt/render/Renderer.hpp` pour le contrat et
// `docs/ARCHITECTURE.md` §4 pour le pipeline (camera -> intersection ->
// framebuffer). T046 branche la recherche d'intersection : tous les objets de
// la scene (objets directs + groupes aplatis, 4 types coexistant, doublons
// autorises) sont convertis une fois en `geometry::AObject` (chemin froid,
// avant la boucle), puis chaque rayon cherche le plus proche (`tri par t`,
// `tMax` resserre). Hit -> Lambert (T033) avec le materiau de l'objet touche
// + ambiance + **toutes** les ponctuelles (T052 : multi-spot, shadow ray
// `tMin` eps anti-acne, attenuation T051) + directionnelles (T055) + spots
// orientés (T058 : cône `spotConeFactor`, ombre continue, attenuation) +
// ombres continues (T058 : `shadowTransmittance`, translucide moins sombre
// qu'opaque, `trans_eff = transparency * 1.5/ior`) + speculaire Blinn-Phong
// (T053 : `specular`, `shininess`, `V = -ray.dir`, sature en blanc) +
// reflexion bornee (T056 : `reflectivity` 0 = mat / 1 = miroir pur,
// `max_depth` 0..32) + refraction bornee (T057 : Descartes `n1*sin(t1) =
// n2*sin(t2)`, `eta = frontFace ? 1/ior : ior`, `transparency` 0 = opaque /
// 1 = transmis pur, repli miroir en reflexion totale interne) ; miss -> fond
// ou aveuglement spot (T058 : `spotBlindingFactor`, rayon vers la source dont
// le cône éclaire l'observateur -> blanc saturé).
// Progressif T036 : batches externes + `seedFor` absolu + jitter sous-pixel
// (AA en T120) + dithering deterministe minimal (moyenne -> 0 quand spp
// grandit, conservé pour hit et miss afin que `spp 64 < spp 1` reste vrai).

#include "rt/render/Renderer.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "rt/base/Rng.hpp"
#include "rt/accel/Bvh.hpp"
#include "rt/geometry/Cone.hpp"
#include "rt/geometry/Cylinder.hpp"
#include "rt/geometry/Plane.hpp"
#include "rt/geometry/Sphere.hpp"
#include "rt/lighting/DirectionalLight.hpp"
#include "rt/lighting/PointLight.hpp"
#include "rt/lighting/SpotLight.hpp"
#include "rt/render/Camera.hpp"
#include "rt/render/Framebuffer.hpp"
#include "rt/io/Texture.hpp"
#include "rt/scene/Scene.hpp"
#include "rt/sched/ThreadPool.hpp"
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
constexpr int kMinThreads = 1;
constexpr int kMaxThreads = 256;
// T063 : tuiles 32×32 (localite cache, cf. `ARCHITECTURE.md` §8.2).
constexpr int kTileSize = 32;
constexpr long long kMinSeed = 0;
constexpr long long kMaxSeed = 4294967295LL;
// T036 : dithering deterministe minimal (±kDitherAmp/2 par echantillon,
// moyenne -> 0 quand spp grandit). Conservé en T046 pour hit et miss :
// le progressif reste observable (spp 1 plus bruite que spp 64).
constexpr float kDitherAmp = 0.06F;
// T046 : `tMin` des rayons primaires (anti-acne, au-dessus de `kEpsilon`).
constexpr Real kPrimaryTMin = 0.001F;
// T052 : `tMin` des rayons d'ombre (meme epsilon, anti-acne) et decalage
// `P + N*eps` (evite l'auto-intersection, pas de bandes).
// (T056 : le miss -> fond est retourne directement par `traceRay()`
// via `ctx.background`, sans fonction dediee.)

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
	out.texScale = src.texture.scale;
	out.texOffset = src.texture.offset;
	return out;
}

[[nodiscard]] shading::AmbientParams toAmbientParams(const scene::Scene& scene) noexcept {
	shading::AmbientParams out;
	out.color = scene.ambient.color;
	out.intensity = scene.ambient.intensity;
	return out;
}

// T052/T058 : ponctuelles pures (`type point` et `area` traitée comme
// ponctuelle, ordre du fichier conservé). Les spots (`type spot`) sont
// collectés à part en T058 (`collectSpotLights`, cône + aveuglement) et les
// directionnelles en T055. Chemin froid : `reserve(lights.size())`.
void collectPointLights(const scene::Scene& scene,
                        std::vector<shading::PointLightParams>& out) {
	out.clear();
	out.reserve(scene.lights.size());
	for (const scene::Light& light : scene.lights) {
		if (light.type == scene::LightType::Spot ||
		    light.type == scene::LightType::Directional) {
			continue;
		}
		if (!light.hasPosition) {
			continue;
		}
		shading::PointLightParams params;
		params.position = light.position;
		params.color = light.color;
		params.intensity = light.intensity;
		params.attenuation = light.attenuation;
		params.range = light.range;
		out.push_back(params);
	}
}

// T058 : spots (`type spot`, `position` + `target` + `angle` validés en T024 :
// `position` requise, `target` requise). Cône évalué par
// `lighting::spotConeFactor()` dans `shadeDirect`, aveuglement par
// `lighting::spotBlindingFactor()` sur les rayons manqués. Ordre du fichier
// conservé (mélange déterministe). Chemin froid.
void collectSpotLights(const scene::Scene& scene,
                       std::vector<shading::SpotLightParams>& out) {
	out.clear();
	out.reserve(scene.lights.size());
	for (const scene::Light& light : scene.lights) {
		if (light.type != scene::LightType::Spot) {
			continue;
		}
		if (!light.hasPosition || !light.hasTarget) {
			continue;
		}
		shading::SpotLightParams params;
		params.position = light.position;
		params.color = light.color;
		params.intensity = light.intensity;
		params.target = light.target;
		params.angle = light.angle;
		params.attenuation = light.attenuation;
		params.range = light.range;
		out.push_back(params);
	}
}

// T055 : toutes les directionnelles (`type directional`, `hasDirection`,
// validees en T024 : direction non nulle). `L` constante par lumiere
// (`lighting::toLightDir`, independante de la position, pas d'attenuation,
// `tMax` infini pour les ombres). Ordre du fichier conserve.
void collectDirectionalLights(const scene::Scene& scene,
                              std::vector<shading::DirectionalLightParams>& out) {
	out.clear();
	out.reserve(scene.lights.size());
	for (const scene::Light& light : scene.lights) {
		if (light.type != scene::LightType::Directional || !light.hasDirection) {
			continue;
		}
		shading::DirectionalLightParams params;
		params.direction = light.direction;
		params.color = light.color;
		params.intensity = light.intensity;
		out.push_back(params);
	}
}

// T046/T051 : remplacement historique par `collectPointLights()` (T052) —
// l'ancienne `toPointLight()` (1ere lumiere) est supprimee pour eviter
// `-Wunused-function` : le cas mono-lumiere est `collectPointLights()` a 1
// element (pixels identiques, verifie par les tests existants).

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
                     std::vector<shading::MaterialParams>& outMats,
                     std::vector<const io::TextureImage*>& outTex,
                     io::TextureCache& texCache, long long maxTexBytes, std::uint32_t& nextId) {
	const std::uint32_t matIndex = static_cast<std::uint32_t>(outMats.size());
	std::unique_ptr<geometry::AObject> obj = makeWorldObject(src, world, nextId, matIndex);
	if (!obj) {
		return;
	}
	outMats.push_back(toMaterialParams(src.material));
	// T103 : texture image alignee sur `outMats` (meme index). Chargement
	// froid via le cache partage ; echec (absent/corrompu) -> `nullptr`
	// defini (repli albedo, jamais de crash). Borne `limits` (T024/T102).
	const io::TextureImage* texPtr = nullptr;
	if (src.material.texture.present && !src.material.texture.file.empty()) {
		rt::Result<std::shared_ptr<io::TextureImage>> loaded =
		    texCache.load(src.material.texture.file, maxTexBytes);
		if (loaded.isOk() && loaded.value()) {
			texPtr = loaded.value().get();
		}
	}
	outTex.push_back(texPtr);
	outObjs.push_back(std::move(obj));
	++nextId;
}

// T046 : aplatit un groupe (objets + enfants) sous `parentWorld`.
// `groupWorld = parentWorld * local`, chaque objet vaut
// `groupWorld * objLocal`. `nextId` fournit `id` unique et croissant.
void appendGroupObjects(const scene::Group& group, const Transform& parentWorld,
                        std::vector<std::unique_ptr<geometry::AObject>>& outObjs,
                        std::vector<shading::MaterialParams>& outMats,
                        std::vector<const io::TextureImage*>& outTex,
                        io::TextureCache& texCache, long long maxTexBytes,
                        std::uint32_t& nextId) {
	const Transform local = buildLocalTransform(group.transform);
	const Transform groupWorld = parentWorld.compose(local);
	for (const scene::Object& obj : group.objects) {
		const Transform objLocal = buildLocalTransform(obj.transform);
		const Transform objWorld = groupWorld.compose(objLocal);
		appendOneObject(obj, objWorld, outObjs, outMats, outTex, texCache, maxTexBytes, nextId);
	}
	for (const std::shared_ptr<scene::Group>& child : group.children) {
		if (child) {
			appendGroupObjects(*child, groupWorld, outObjs, outMats, outTex, texCache,
			                   maxTexBytes, nextId);
		}
	}
}

// T046 : scene -> objets monde (chemin froid, avant la boucle chaude).
// Reserve `totalObjectCount()` (aucune realloc pour les scenes courantes,
// memoire bornee par `limits` via la validation T024).
void collectSceneObjects(const scene::Scene& scene,
                         std::vector<std::unique_ptr<geometry::AObject>>& outObjs,
                         std::vector<shading::MaterialParams>& outMats,
                         std::vector<const io::TextureImage*>& outTex,
                         io::TextureCache& texCache) {
	outObjs.clear();
	outMats.clear();
	outTex.clear();
	const std::size_t total = scene.totalObjectCount();
	outObjs.reserve(total);
	outMats.reserve(total);
	outTex.reserve(total);
	const long long maxTexBytes = scene.limits.maxTextureBytes;
	std::uint32_t nextId = 0;
	const Transform identity;
	for (const scene::Object& obj : scene.objects) {
		const Transform objWorld = buildLocalTransform(obj.transform);
		appendOneObject(obj, objWorld, outObjs, outMats, outTex, texCache, maxTexBytes, nextId);
	}
	for (const scene::Group& group : scene.groups) {
		appendGroupObjects(group, identity, outObjs, outMats, outTex, texCache, maxTexBytes,
		                   nextId);
	}
}

// T046 : plus proche parmi tous les objets (tri par `t`). `tMax` se
// resserre au plus proche trouve : le gagnant est le `t` minimal dans
// `[tMin, tMax]`. Plusieurs objets du meme type + coexistence des 4 types.
// T065 : si `bvh` est fournie (construite une fois par rendu, T060-T062),
// la traversal remplace la boucle lineaire (memes `t` a 1e-4, T061) ;
// sinon repli lineaire (scene vide, echec de construction — defini).
// `noexcept`, sans allocation (R2/R3) : `HitRecord` sur pile uniquement.
[[nodiscard]] bool findClosestHit(const std::vector<std::unique_ptr<geometry::AObject>>& objs,
                                  const accel::Bvh* bvh, const Ray& ray, Real tMin, Real tMax,
                                  HitRecord& outRec) noexcept {
	if (bvh != nullptr && !bvh->empty()) {
		return bvh->traverse(ray, tMin, tMax, outRec, objs);
	}
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

// T052/T058 : transmittance d'un shadow ray (ARCHITECTURE.md §4.5).
// `origin` déjà décalé (`P + N*eps`), `dir` normalisée vers la lumière,
// `tMax = dist - eps` (la source elle-même n'occlut pas).
// T052 : binaire (opaque = 0). T058 (*Shadows and transparency*) : continue —
// chaque occulteur transparent laisse passer une fraction :
// `trans_eff = clamp(transparency,0,1) * iorFactor` avec
// `iorFactor = clamp(1.5/ior, 0.5, 1)` (verre dense `ior=3` absorbe ~2× plus
// que `ior=1.5`, `ior=1` = sans atténuation ior ; Descartes `n1*sin(t1) =
// n2*sin(t2)` explique la déviation, ici on modèle l'absorption/Fresnel
// moyenne par ce facteur documenté), `transmit *= trans_eff` sur tout le
// trajet (8 occulteurs max, boucle bornée, anti-boucle infinie).
// Opaque (`transparency == 0`) -> 0 immédiat (octet-identique à T052).
// `tMax <= eps` / dégénéré -> 1 (pas d'ombre, défini). `noexcept`, sans
// allocation (R2/R3, `HitRecord` sur pile).
// T065 : l'occlusion passe par la BVH quand elle est fournie (comme le
// primaire), sinon repli lineaire.
[[nodiscard]] float shadowTransmittance(
    const std::vector<std::unique_ptr<geometry::AObject>>& objs, const accel::Bvh* bvh,
    const std::vector<shading::MaterialParams>* mats, Vec3 origin, Vec3 dir,
    Real tMax) noexcept {
	if (!(tMax > kPrimaryTMin)) {
		return 1.0F;
	}
	if (mats == nullptr) {
		return 1.0F;
	}
	if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) ||
	    !std::isfinite(dir.x) || !std::isfinite(dir.y) || !std::isfinite(dir.z) ||
	    !std::isfinite(tMax)) {
		return 1.0F;
	}
	const Real dirLenSq = dot(dir, dir);
	if (!std::isfinite(dirLenSq) || dirLenSq <= kEpsilon * kEpsilon) {
		return 1.0F;
	}
	float transmit = 1.0F;
	Vec3 curOrigin = origin;
	Real curTMax = tMax;
	for (int iter = 0; iter < 8; ++iter) {
		if (!(curTMax > kPrimaryTMin)) {
			break;
		}
		HitRecord rec;
		if (!findClosestHit(objs, bvh, Ray(curOrigin, dir), kPrimaryTMin, curTMax, rec)) {
			break;
		}
		float transp = 0.0F;
		float ior = 1.5F;
		if (rec.materialIndex < mats->size()) {
			const shading::MaterialParams& mat = (*mats)[rec.materialIndex];
			if (std::isfinite(mat.transparency) && mat.transparency > 0.0F) {
				transp = mat.transparency > 1.0F ? 1.0F : mat.transparency;
			}
			if (std::isfinite(mat.ior) && mat.ior >= 1.0F && mat.ior <= 3.0F) {
				ior = mat.ior;
			}
		}
		if (!(transp > 0.0F)) {
			return 0.0F;
		}
		float iorFactor = 1.5F / ior;
		if (!std::isfinite(iorFactor)) {
			iorFactor = 1.0F;
		}
		if (iorFactor > 1.0F) {
			iorFactor = 1.0F;
		} else if (iorFactor < 0.5F) {
			iorFactor = 0.5F;
		}
		float eff = transp * iorFactor;
		if (!(eff > 0.0F)) {
			return 0.0F;
		}
		if (eff > 1.0F) {
			eff = 1.0F;
		}
		transmit *= eff;
		if (!(transmit > 0.003F)) {
			return 0.0F;
		}
		if (!std::isfinite(rec.t) || !(rec.t > 0.0F) || !std::isfinite(rec.point.x)) {
			break;
		}
		curTMax = curTMax - rec.t - kPrimaryTMin;
		curOrigin = rec.point + dir * kPrimaryTMin;
	}
	if (!std::isfinite(transmit) || transmit < 0.0F) {
		return 0.0F;
	}
	if (transmit > 1.0F) {
		return 1.0F;
	}
	return transmit;
}

// T056/T058 : contexte de trace (pile uniquement, R3). Pointeurs vers les
// tableaux du chemin froid (aucune copie par rayon), ambiant + fond par
// valeur (petits POD), `maxDepth` borne 0..32 (profondeur de recursion).
// T058 ajoute `spots` (spots orientés, cône + aveuglement).
// T065 ajoute `bvh` (traversal au lieu du lineaire, `nullptr` = repli).
struct TraceCtx {
	const std::vector<std::unique_ptr<geometry::AObject>>* objs = nullptr;
	const accel::Bvh* bvh = nullptr;
	const std::vector<shading::MaterialParams>* mats = nullptr;
	const std::vector<shading::PointLightParams>* points = nullptr;
	const std::vector<shading::DirectionalLightParams>* dirs = nullptr;
	const std::vector<shading::SpotLightParams>* spots = nullptr;
	// T103 : images textures alignees sur `mats` (meme index, `nullptr` =
	// pas de texture). Pointeurs froids (le `TextureCache` vit dans
	// `render()`), lecture seule en boucle chaude (R3, pas d'alloc).
	const std::vector<const io::TextureImage*>* texImages = nullptr;
	shading::AmbientParams ambient;
	Vec3 background;
	int maxDepth = 4;
};

// T056-T058 : éclairage direct d'un hit (ambiant + multi-spot T052 avec ombres
// continues T058 et atténuation T051 + directionnelles T055 + spots T058 avec
// cône + speculaire Blinn-Phong T053). `reflectivity == 0` + `transparency ==
// 0` + 0 spot rend donc l'octet identique à l'ancien chemin (DoD bornes).
// `noexcept`, sans allocation (R2/R3).
[[nodiscard]] Vec3 shadeDirect(const HitRecord& rec, Vec3 viewDir, const TraceCtx& ctx,
                               const shading::MaterialParams& mat) noexcept {
	shading::PointLightParams zeroLight;
	zeroLight.intensity = 0.0F;
	Vec3 total = shading::shadeLambert(mat, rec.normal, rec.point, zeroLight, ctx.ambient);
	shading::MaterialParams matNoAmb = mat;
	matNoAmb.ambient = 0.0F;
	const Vec3 unitN = normalize(rec.normal);
	const Vec3 shadowOrigin =
	    nearZero(unitN) ? rec.point : rec.point + unitN * kPrimaryTMin;
	for (const shading::PointLightParams& light : *ctx.points) {
		const Vec3 toLight = light.position - rec.point;
		if (!std::isfinite(toLight.x) || !std::isfinite(toLight.y) ||
		    !std::isfinite(toLight.z)) {
			continue;
		}
		const Real distSq = dot(toLight, toLight);
		if (!std::isfinite(distSq) || distSq <= kEpsilon * kEpsilon) {
			continue;
		}
		const float dist = static_cast<float>(length(toLight));
		if (!std::isfinite(dist) || dist <= kPrimaryTMin) {
			continue;
		}
		const float att =
		    lighting::attenuationFactor(light.attenuation, dist, light.range);
		if (!(att > 0.0F) || !std::isfinite(att)) {
			continue;
		}
		const Vec3 lightDir = toLight / dist;
		const Real tMax = static_cast<Real>(dist) - kPrimaryTMin;
		const float vis =
		    shadowTransmittance(*ctx.objs, ctx.bvh, ctx.mats, shadowOrigin, lightDir, tMax);
		if (!(vis > 0.0F) || !std::isfinite(vis)) {
			continue;
		}
		shading::PointLightParams effLight = light;
		effLight.intensity = light.intensity * att * vis;
		total += shading::shadeLambert(matNoAmb, rec.normal, rec.point, effLight,
		                               ctx.ambient);
		total += shading::specularTerm(rec.normal, viewDir, lightDir, mat,
		                               effLight.color, effLight.intensity);
	}
	for (const shading::DirectionalLightParams& dirLight : *ctx.dirs) {
		const Vec3 lightDir = lighting::toLightDir(dirLight.direction);
		if (nearZero(lightDir)) {
			continue;
		}
		if (!std::isfinite(dirLight.intensity) || !(dirLight.intensity > 0.0F)) {
			continue;
		}
		const float vis = shadowTransmittance(*ctx.objs, ctx.bvh, ctx.mats, shadowOrigin, lightDir,
		                                               kInfinity);
		if (!(vis > 0.0F) || !std::isfinite(vis)) {
			continue;
		}
		shading::AmbientParams nullAmbient;
		nullAmbient.color = Vec3{};
		nullAmbient.intensity = 0.0F;
		shading::DirectionalLightParams effDir = dirLight;
		effDir.intensity = dirLight.intensity * vis;
		total += shading::shadeLambertDirectional(matNoAmb, rec.normal, effDir,
		                                          nullAmbient);
		total += shading::specularTerm(rec.normal, viewDir, lightDir, mat, effDir.color,
		                               effDir.intensity);
	}
	// T058 : spots orientés (cône `spotConeFactor` + ombre continue + atténuation).
	// Hors cône (`cone == 0`) -> aucune contribution (défini, pas de fuite).
	for (const shading::SpotLightParams& spot : *ctx.spots) {
		const Vec3 toLight = spot.position - rec.point;
		if (!std::isfinite(toLight.x) || !std::isfinite(toLight.y) ||
		    !std::isfinite(toLight.z)) {
			continue;
		}
		const Real distSq = dot(toLight, toLight);
		if (!std::isfinite(distSq) || distSq <= kEpsilon * kEpsilon) {
			continue;
		}
		const float dist = static_cast<float>(length(toLight));
		if (!std::isfinite(dist) || dist <= kPrimaryTMin) {
			continue;
		}
		const float cone =
		    lighting::spotConeFactor(rec.point, spot.position, spot.target, spot.angle);
		if (!(cone > 0.0F) || !std::isfinite(cone)) {
			continue;
		}
		const float att =
		    lighting::attenuationFactor(spot.attenuation, dist, spot.range);
		if (!(att > 0.0F) || !std::isfinite(att)) {
			continue;
		}
		const Vec3 lightDir = toLight / dist;
		const Real tMax = static_cast<Real>(dist) - kPrimaryTMin;
		const float vis =
		    shadowTransmittance(*ctx.objs, ctx.bvh, ctx.mats, shadowOrigin, lightDir, tMax);
		if (!(vis > 0.0F) || !std::isfinite(vis)) {
			continue;
		}
		if (!std::isfinite(spot.intensity) || !(spot.intensity > 0.0F)) {
			continue;
		}
		const float effIntensity = spot.intensity * att * cone * vis;
		if (!(effIntensity > 0.0F) || !std::isfinite(effIntensity)) {
			continue;
		}
		shading::PointLightParams effLight;
		effLight.position = spot.position;
		effLight.color = spot.color;
		effLight.intensity = effIntensity;
		effLight.attenuation = Vec3(1.0F, 0.0F, 0.0F);
		effLight.range = 0.0F;
		total += shading::shadeLambert(matNoAmb, rec.normal, rec.point, effLight,
		                               ctx.ambient);
		total += shading::specularTerm(rec.normal, viewDir, lightDir, mat, effLight.color,
		                               effLight.intensity);
	}
	return shading::saturate(total);
}

// T056+T057 : trace un rayon avec reflexion et refraction bornees
// (SPECIFICATIONS §5.2 F, ARCHITECTURE.md §4.6). `depth` = generation
// (0 = primaire) ; `maxDepth` = nombre max de rebonds (0 = direct seul,
// jamais de boucle infinie : chaque rebond incremente `depth`, pile bornee
// 0..32).
// Reflexion (T056) : `R = D - 2*(D.N)*N` (`Vec3::reflect`, N unitaire),
// `reflechi = trace(P+N*eps, reflect(D, N), depth+1)`,
// `base = saturate(direct*(1-R) + reflechi*R)` avec `R = clamp(reflectivity,
// 0, 1)` (NaN -> 0). `R = 0` -> direct seul (octet identique), `R = 1` ->
// miroir pur (aucune part diffuse).
// Refraction Descartes/Snell (T057, formule cherchee par le correcteur) :
// `n1*sin(theta1) = n2*sin(theta2)`, `eta = n1/n2 = frontFace ? 1/ior : ior`
// (entree air->objet : vers la normale ; sortie objet->air : loin de la
// normale, courbure exterieure), `cos1 = dot(-D, N)`,
// `rPerp = eta*(D + cos1*N)`, `rPar = -sqrt(1-|rPerp|^2)*N`, `T = rPerp+rPar`
// (via `shading::refractDir` -> `rt::refract`), origine `P-N*eps` (cote
// transmis, anti-acne), `transmis = trace(P-N*eps, T, depth+1)`,
// `out = saturate(base*(1-T2) + transmis*T2)` avec `T2 = clamp(transparency,
// 0, 1)` (NaN -> 0). `T2 = 0` -> `base` seul (octet identique au chemin T056),
// `T2 = 1` -> transmis pur. `ior = 1` -> `eta = 1` -> aucune deviation
// (DoD). Reflexion totale interne (`|rPerp|^2 > 1`, `refractDir` nul) ->
// repli miroir (100 % reflechi, `transmis = reflechi` ou trace du miroir si
// `R = 0`, jamais de trou noir ni de NaN). Degeneres (normale nulle,
// direction nulle/NaN, `depth >= maxDepth`) -> `direct`/`base` seul, defini.
// `noexcept`, sans allocation (R2/R3, `HitRecord` sur pile). T058 : ombres
// continues (`shadowTransmittance`, translucide = moins sombre qu'opaque,
// proportionnel à `transparency` × `1.5/ior`) + spots orientés (cône dans
// `shadeDirect`, aveuglement sur les manqués ci-dessous).
[[nodiscard]] Vec3 traceRay(const Ray& ray, int depth, const TraceCtx& ctx) noexcept {
	HitRecord rec;
	if (!findClosestHit(*ctx.objs, ctx.bvh, ray, kPrimaryTMin, kInfinity, rec)) {
		// T058 (*Direct light*) : spot face à l'observateur qui aveugle.
		// Le rayon manqué qui vise une source dont le cône éclaire
		// l'observateur sature en blanc (mélange fond + source selon
		// `spotBlindingFactor`, centre = 1). Sans spot : fond seul
		// (octet-identique à T052-T057). `noexcept`, sans allocation.
		float bestBlind = 0.0F;
		Vec3 blindColor{};
		for (const shading::SpotLightParams& spot : *ctx.spots) {
			if (!std::isfinite(spot.intensity) || !(spot.intensity > 0.0F)) {
				continue;
			}
			const float f = lighting::spotBlindingFactor(ray.origin, ray.direction,
			                                              spot.position, spot.target,
			                                              spot.angle);
			if (!(f > 0.0F) || !std::isfinite(f)) {
				continue;
			}
			if (f > bestBlind) {
				bestBlind = f > 1.0F ? 1.0F : f;
				const float boost = spot.intensity > 1.0F ? spot.intensity : 1.0F;
				blindColor = Vec3(spot.color.x * boost, spot.color.y * boost,
				                  spot.color.z * boost);
			}
		}
		if (bestBlind > 0.0F) {
			const float inv = 1.0F - bestBlind;
			const Vec3 mixed = Vec3(ctx.background.x * inv + blindColor.x * bestBlind,
			                        ctx.background.y * inv + blindColor.y * bestBlind,
			                        ctx.background.z * inv + blindColor.z * bestBlind);
			return shading::saturate(mixed);
		}
		return ctx.background;
	}
	shading::MaterialParams mat;
	if (rec.materialIndex < ctx.mats->size()) {
		mat = (*ctx.mats)[rec.materialIndex];
	}
	// T103 : texture image remplace l'albedo (`material.texture->sample`,
	// OPTIONS_GUIDE §5.1). `rec.uv` vient de la primitive (4 types, T041–
	// T044) ; pavage + plus proche dans `sampleTexture` (fract, `noexcept`,
	// sans alloc). `nullptr` ou image vide -> albedo fichier (repli defini,
	// jamais de crash). L'echantillon module ensuite tout l'eclairage
	// (diffus + speculaire via `mat`).
	if (ctx.texImages != nullptr && rec.materialIndex < ctx.texImages->size()) {
		const io::TextureImage* tex = (*ctx.texImages)[rec.materialIndex];
		if (tex != nullptr && tex->width > 0 && tex->height > 0 && !tex->rgba.empty()) {
			// T104 : `u' = u*sx + ox`, `v' = v*sy + oy` par objet
			// (etirer/compresser + decaler, sous-criteres 3-4).
			// `scale <= 0` ou NaN -> 1 defini (borne schema R1, garde ici).
			float sx = mat.texScale.x;
			float sy = mat.texScale.y;
			if (!std::isfinite(sx) || sx <= 0.0F) {
				sx = 1.0F;
			}
			if (!std::isfinite(sy) || sy <= 0.0F) {
				sy = 1.0F;
			}
			float ox = std::isfinite(mat.texOffset.x) ? mat.texOffset.x : 0.0F;
			float oy = std::isfinite(mat.texOffset.y) ? mat.texOffset.y : 0.0F;
			mat.albedo =
			    io::sampleTexture(*tex, rec.uv.x * sx + ox, rec.uv.y * sy + oy);
		}
	}
	const Vec3 viewDir = ray.direction * -1.0F;
	const Vec3 direct = shadeDirect(rec, viewDir, ctx, mat);
	float refl = 0.0F;
	if (std::isfinite(mat.reflectivity) && mat.reflectivity > 0.0F) {
		refl = mat.reflectivity > 1.0F ? 1.0F : mat.reflectivity;
	}
	float trans = 0.0F;
	if (std::isfinite(mat.transparency) && mat.transparency > 0.0F) {
		trans = mat.transparency > 1.0F ? 1.0F : mat.transparency;
	}
	if (!(refl > 0.0F) && !(trans > 0.0F)) {
		return direct;
	}
	if (depth >= ctx.maxDepth) {
		return direct;
	}
	const Vec3 unitN = normalize(rec.normal);
	if (nearZero(unitN)) {
		return direct;
	}
	if (!std::isfinite(unitN.x) || !std::isfinite(unitN.y) || !std::isfinite(unitN.z)) {
		return direct;
	}
	// Reflexion (T056, inchangee quand `trans == 0` : octet-identique).
	Vec3 base = direct;
	Vec3 reflected{};
	bool hasReflected = false;
	if (refl > 0.0F) {
		// Reflexion miroir : R = D - 2*(D.N)*N (`Vec3::reflect`, N unitaire).
		const Vec3 reflDir = reflect(ray.direction, unitN);
		if (!nearZero(reflDir) && std::isfinite(reflDir.x) && std::isfinite(reflDir.y) &&
		    std::isfinite(reflDir.z)) {
			const Vec3 unitR = normalize(reflDir);
			if (!nearZero(unitR)) {
				const Vec3 origin = rec.point + unitN * kPrimaryTMin;
				reflected = traceRay(Ray(origin, unitR), depth + 1, ctx);
				hasReflected = true;
				const float inv = 1.0F - refl;
				base = shading::saturate(Vec3(direct.x * inv + reflected.x * refl,
				                              direct.y * inv + reflected.y * refl,
				                              direct.z * inv + reflected.z * refl));
			}
		}
	}
	if (!(trans > 0.0F)) {
		return base;
	}
	// Refraction (T057) : Descartes `n1*sin(t1) = n2*sin(t2)` via
	// `shading::refractDir` (voir son contrat pour `eta` et la sentinelle
	// nulle en reflexion totale interne). Origine `P-N*eps` (cote transmis).
	const Vec3 refrDir = shading::refractDir(ray.direction, unitN, rec.frontFace, mat.ior);
	Vec3 refracted{};
	if (nearZero(refrDir) || !std::isfinite(refrDir.x) || !std::isfinite(refrDir.y) ||
	    !std::isfinite(refrDir.z)) {
		// Reflexion totale interne (`sin(t2) > 1`, sortie rasante) : le rayon
		// ne transmet rien, tout est reflechi (Fresnel = 1). Repli miroir
		// (meme direction que la reflexion) pour ne jamais rendre de trou
		// noir ni de NaN — l'image reste finie et bornee.
		if (hasReflected) {
			refracted = reflected;
		} else {
			const Vec3 fallbackDir = reflect(ray.direction, unitN);
			if (nearZero(fallbackDir) || !std::isfinite(fallbackDir.x)) {
				return base;
			}
			const Vec3 unitF = normalize(fallbackDir);
			if (nearZero(unitF)) {
				return base;
			}
			const Vec3 originF = rec.point + unitN * kPrimaryTMin;
			refracted = traceRay(Ray(originF, unitF), depth + 1, ctx);
		}
	} else {
		const Vec3 unitT = normalize(refrDir);
		if (nearZero(unitT)) {
			return base;
		}
		const Vec3 originT = rec.point - unitN * kPrimaryTMin;
		refracted = traceRay(Ray(originT, unitT), depth + 1, ctx);
	}
	const float invT = 1.0F - trans;
	const Vec3 blended =
	    Vec3(base.x * invT + refracted.x * trans, base.y * invT + refracted.y * trans,
	         base.z * invT + refracted.z * trans);
	return shading::saturate(blended);
}

// T063 : rend une region rectangulaire `[x0,x0+w) × [y0,y0+h)` pour
// l'echantillon `s` (un batch). `noexcept`, sans allocation (R2/R3) :
// `Rng` sur pile, `traceRay` recursif borne, `addSample` disjoint (tuiles
// disjointes -> TSan-vert, chaque thread ecrit ses pixels).
// C'est le `renderRegion(x0, y0, w, h)` du Prompt.
void renderTile(const Camera& camera, const TraceCtx& ctx, Framebuffer& fb, std::uint32_t sceneSeed,
                int sample, int x0, int y0, int w, int h) noexcept {
	for (int y = y0; y < y0 + h; ++y) {
		for (int x = x0; x < x0 + w; ++x) {
			Rng rng = rngFor(x, y, sample, sceneSeed);
			const Vec2 jitter(rng.nextFloat() - 0.5F, rng.nextFloat() - 0.5F);
			const Ray ray = camera.rayForPixel(x, y, jitter);
			Vec3 color = traceRay(ray, 0, ctx);
			color.x += (rng.nextFloat() - 0.5F) * kDitherAmp;
			color.y += (rng.nextFloat() - 0.5F) * kDitherAmp;
			color.z += (rng.nextFloat() - 0.5F) * kDitherAmp;
			fb.addSample(x, y, color);
		}
	}
}

struct Tile {
	int x0 = 0;
	int y0 = 0;
	int w = 0;
	int h = 0;
};

// Decoupe l'image en tuiles `kTileSize × kTileSize` (derniere ligne/colonne
// eventuellement plus petite, 0 pixel de recouvrement). Chemin froid.
void splitTiles(int width, int height, std::vector<Tile>& out) {
	out.clear();
	out.reserve(static_cast<std::size_t>((width + kTileSize - 1) / kTileSize) *
	            static_cast<std::size_t>((height + kTileSize - 1) / kTileSize));
	for (int y0 = 0; y0 < height; y0 += kTileSize) {
		for (int x0 = 0; x0 < width; x0 += kTileSize) {
			int w = width - x0;
			if (w > kTileSize) {
				w = kTileSize;
			}
			int h = height - y0;
			if (h > kTileSize) {
				h = kTileSize;
			}
			out.push_back(Tile{x0, y0, w, h});
		}
	}
}

} // namespace

Status render(const scene::Scene& scene, Framebuffer& fb, const RenderParams& params,
              RenderStats* stats) {
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
	if (params.threads < kMinThreads || params.threads > kMaxThreads) {
		return Status::error(StatusCode::InvalidArgument, "bad render threads: expected 1..256",
		                     __LINE__);
	}
	// T064 : chronometrage par phase (chemin froid, `steady_clock`, sans
	// allocation). `buildMs` = camera + framebuffer + collecte scene ;
	// `renderMs` = boucle tuiles + `present()` ; `totalMs` = les deux.
	const auto buildStart = std::chrono::steady_clock::now();
	Camera camera;
	if (Status status = camera.init(scene.camera, params.width, params.height);
	    status.isError()) {
		return status;
	}
	if (Status status = fb.init(params.width, params.height); status.isError()) {
		return status;
	}
	// Chemin froid (T046) : conversion scene -> objets monde (geometrie +
	// materiaux), lumieres et ambiance. Allocation unique avant la boucle ;
	// la boucle chaude n'alloue plus (R3).
	// T103 : le cache textures vit ici (froid, `shared_ptr`, borne
	// `limits`) ; `worldTex` pointe vers ses images (meme index que
	// `worldMats`), `nullptr` = pas de texture.
	std::vector<std::unique_ptr<geometry::AObject>> worldObjs;
	std::vector<shading::MaterialParams> worldMats;
	std::vector<const io::TextureImage*> worldTex;
	io::TextureCache texCache;
	collectSceneObjects(scene, worldObjs, worldMats, worldTex, texCache);
	// T052 : toutes les ponctuelles (multi-spot, ordre du fichier).
	// T055 : toutes les directionnelles (paralleles, ordre du fichier).
	// `reserve()` dans les `collect*()` : aucune realloc courante.
	std::vector<shading::PointLightParams> pointLights;
	collectPointLights(scene, pointLights);
	std::vector<shading::DirectionalLightParams> dirLights;
	collectDirectionalLights(scene, dirLights);
	std::vector<shading::SpotLightParams> spotLights;
	collectSpotLights(scene, spotLights);
	const shading::AmbientParams ambient = toAmbientParams(scene);
	// T065 (optimisation mesuree, un seul point) : la BVH remplace la boucle
	// lineaire (T060-T062). Construite **une fois** par rendu (chemin froid,
	// `reserve(2N)`, incluse dans `buildMs`) ; `traverse()` en boucle chaude
	// (`noexcept`, pile fixe, sans allocation). Echec (impossible ici :
	// `worldObjs` sans `nullptr`) -> repli lineaire defini (`bvhPtr` nul).
	// Scenes a 101 objets : le goulot etait les intersections (T061 : 26×
	// en micro-bench, 3.76s en rendu lineaire).
	accel::Bvh bvh;
	const accel::Bvh* bvhPtr = nullptr;
	if (bvh.build(worldObjs).isOk() && !bvh.empty()) {
		bvhPtr = &bvh;
	}
	// Boucle chaude : registres + pile uniquement (R3), batches externes (T036).
	// Chaque echantillon `s` utilise `rngFor(x, y, s, seed)` (coordonnees
	// absolues, T016) : jitter sous-pixel pour le rayon + dithering pour
	// la couleur (hit comme miss). Memes `spp` + meme `seed` -> memes
	// pixels, octet par octet, que ce soit en plein ou par tuile/bande
	// (couture impossible). `onProgress(s+1, spp)` apres chaque batch
	// (1 appel par batch, pas par pixel : hors hot path fin).
	// T056 : l'eclairage passe par `traceRay()` (direct + reflexion bornee
	// par `maxDepth`, `reflectivity` 0 = mat / 1 = miroir pur).
	// T058 : + ombres continues + spots (cône + aveuglement).
	const auto sceneSeed = static_cast<std::uint32_t>(params.seed);
	const TraceCtx traceCtx{&worldObjs, bvhPtr, &worldMats, &pointLights, &dirLights, &spotLights,
	                        &worldTex, ambient, scene.background.color, params.maxDepth};
	const auto buildEnd = std::chrono::steady_clock::now();
	// T064 : remplissage des compteurs (sans atomique : calcule apres
	// `waitIdle`, jamais dans la boucle chaude). `bvhBuilds` = 1 par rendu
	// (BVH locale T065 ; le `BvhCache` T062 servira l'interactif T076).
	auto fillStats = [&](const std::chrono::steady_clock::time_point& renderStart,
	                     const std::chrono::steady_clock::time_point& renderEnd) {
		if (stats == nullptr) {
			return;
		}
		const double buildMs =
		    std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();
		const double renderMs =
		    std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();
		const long long rays =
		    static_cast<long long>(params.width) * static_cast<long long>(params.height) *
		    static_cast<long long>(params.spp);
		stats->primaryRays = rays;
		stats->objects = static_cast<int>(worldObjs.size());
		stats->lights = static_cast<int>(scene.lights.size());
		stats->threadsUsed = params.threads;
		stats->buildMs = buildMs;
		stats->renderMs = renderMs;
		stats->totalMs = buildMs + renderMs;
		stats->raysPerSec = renderMs > 0.0 ? (static_cast<double>(rays) / (renderMs / 1000.0)) : 0.0;
		stats->bvhBuilds = bvhPtr != nullptr ? 1 : 0;
	};
	// T063 : mono-thread historique (octet par octet identique au T032-T058)
	// ou tuiles paralleles (memes pixels via la graine absolue T016).
	if (params.threads <= 1) {
		const auto renderStart = std::chrono::steady_clock::now();
		for (int s = 0; s < params.spp; ++s) {
			for (int y = 0; y < params.height; ++y) {
				for (int x = 0; x < params.width; ++x) {
					Rng rng = rngFor(x, y, s, sceneSeed);
					const Vec2 jitter(rng.nextFloat() - 0.5F, rng.nextFloat() - 0.5F);
					const Ray ray = camera.rayForPixel(x, y, jitter);
					Vec3 color = traceRay(ray, 0, traceCtx);
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
		fillStats(renderStart, std::chrono::steady_clock::now());
		return Status::ok();
	}
	// Multi-thread : pool cree **une fois** par rendu (`jthread`), batches
	// externes sequentiels (1 `onProgress` par batch, depuis ce thread),
	// tuiles 32×32 disjointes en parallele (determinisme absolu, TSan-vert).
	sched::ThreadPool pool(static_cast<std::size_t>(params.threads));
	std::vector<Tile> tiles;
	const auto renderStart = std::chrono::steady_clock::now();
	for (int s = 0; s < params.spp; ++s) {
		splitTiles(params.width, params.height, tiles);
		for (const Tile& tile : tiles) {
			pool.submit([&camera, &traceCtx, &fb, sceneSeed, s, tile] {
				renderTile(camera, traceCtx, fb, sceneSeed, s, tile.x0, tile.y0, tile.w,
				           tile.h);
			});
		}
		pool.waitIdle();
		if (pool.hasError()) {
			return Status::error(StatusCode::Internal, "render task failed", __LINE__);
		}
		if (params.onProgress != nullptr) {
			params.onProgress(s + 1, params.spp, params.progressUser);
		}
	}
	fb.present();
	fillStats(renderStart, std::chrono::steady_clock::now());
	return Status::ok();
}

} // namespace rt::render
