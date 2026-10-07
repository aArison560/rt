// Passe de validation `Scene` (T024) — implementation sans exception (R2).
// Voir `include/rt/scene/Validator.hpp` pour le contrat et
// `docs/MEMORY_STRATEGY.md` §2 pour les invariants memoire (capacite declaree
// dans `limits`, erreur propre si depassee, pas d'allocation surprise).
// Toute borne passe par `schema::check*()` (R1) : ajouter une directive ne
// demande qu'une ligne dans `src/schema/Directives.cpp`, la validation suit.
// Le budget textures est estime par `stat` (fichiers distincts, 0 si absent)
// sans charger aucun contenu.

#include "rt/scene/Validator.hpp"

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "rt/base/Scalar.hpp"
#include "rt/base/Vec.hpp"
#include "rt/schema/Directives.hpp"

namespace rt::scene {

namespace {

using rt::schema::checkEnum;
using rt::schema::checkInt;
using rt::schema::checkNumber;
using rt::schema::checkRotate;
using rt::schema::checkString;

constexpr float kCrossEps = 1e-6F;

[[nodiscard]] bool isZeroVec(Vec3 v) noexcept {
	return dot(v, v) <= kCrossEps * kCrossEps;
}

[[nodiscard]] Status checkVec3Components(std::string_view path, Vec3 v) {
	if (Status st = checkNumber(path, static_cast<double>(v.x)); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, static_cast<double>(v.y)); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, static_cast<double>(v.z)); st.isError()) {
		return st;
	}
	return Status::ok();
}

[[nodiscard]] Status checkFloat2Components(std::string_view path, Vec3 v) {
	if (Status st = checkNumber(path, static_cast<double>(v.x)); st.isError()) {
		return st;
	}
	if (Status st = checkNumber(path, static_cast<double>(v.y)); st.isError()) {
		return st;
	}
	return Status::ok();
}

[[nodiscard]] Status checkFloat3Components(std::string_view path, Vec3 v) {
	return checkVec3Components(path, v);
}

[[nodiscard]] std::string describeLight(std::size_t index, const Light& light) {
	std::string out("light #");
	out.append(std::to_string(index));
	if (!light.name.empty()) {
		out.push_back(' ');
		out.push_back('\'');
		out.append(light.name);
		out.push_back('\'');
	}
	return out;
}

[[nodiscard]] std::string describeObject(std::size_t index, const Object& object) {
	std::string out("object #");
	out.append(std::to_string(index));
	if (!object.name.empty()) {
		out.push_back(' ');
		out.push_back('\'');
		out.append(object.name);
		out.push_back('\'');
	}
	return out;
}

[[nodiscard]] Status validateLimits(const Limits& limits) {
	if (Status st = checkInt("scene.limits.width", static_cast<long long>(limits.width));
	    st.isError()) {
		return st;
	}
	if (Status st = checkInt("scene.limits.height", static_cast<long long>(limits.height));
	    st.isError()) {
		return st;
	}
	if (Status st = checkInt("scene.limits.samples", static_cast<long long>(limits.samples));
	    st.isError()) {
		return st;
	}
	if (Status st = checkInt("scene.limits.max_depth", static_cast<long long>(limits.maxDepth));
	    st.isError()) {
		return st;
	}
	if (Status st = checkInt("scene.limits.seed", limits.seed); st.isError()) {
		return st;
	}
	if (Status st =
	        checkInt("scene.limits.max_objects", static_cast<long long>(limits.maxObjects));
	    st.isError()) {
		return st;
	}
	if (Status st =
	        checkInt("scene.limits.max_lights", static_cast<long long>(limits.maxLights));
	    st.isError()) {
		return st;
	}
	if (Status st = checkInt("scene.limits.max_texture_bytes", limits.maxTextureBytes);
	    st.isError()) {
		return st;
	}
	return Status::ok();
}

[[nodiscard]] Status validateCamera(const Camera& camera) {
	if (Status st = checkVec3Components("scene.camera.position", camera.position);
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.camera.target", camera.target);
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.camera.up", camera.up); st.isError()) {
		return st;
	}
	if (Status st =
	        checkNumber("scene.camera.fov", static_cast<double>(camera.fov));
	    st.isError()) {
		return st;
	}
	const Vec3 view = camera.target - camera.position;
	if (isZeroVec(view)) {
		return Status(StatusCode::InvalidArgument,
		              std::string("invalid camera: target equals position"),
		              __LINE__);
	}
	if (isZeroVec(camera.up)) {
		return Status(StatusCode::InvalidArgument,
		              std::string("invalid camera: up is zero"), __LINE__);
	}
	const double viewLen2 = static_cast<double>(dot(view, view));
	const double upLen2 = static_cast<double>(dot(camera.up, camera.up));
	const double viewDotUp = static_cast<double>(dot(view, camera.up));
	const double cos2 =
	    (viewDotUp * viewDotUp) / (viewLen2 * upLen2);
	if (!(cos2 < 1.0 - 1e-6)) {
		return Status(StatusCode::InvalidArgument,
		              std::string("invalid camera: up is collinear with view direction"),
		              __LINE__);
	}
	return Status::ok();
}

[[nodiscard]] Status validateBackground(const Background& background) {
	return checkVec3Components("scene.background.color", background.color);
}

[[nodiscard]] Status validateAmbient(const Ambient& ambient) {
	if (Status st = checkVec3Components("scene.ambient.color", ambient.color);
	    st.isError()) {
		return st;
	}
	return checkNumber("scene.ambient.intensity", static_cast<double>(ambient.intensity));
}

[[nodiscard]] Status validateLightBounds(std::size_t index, const Light& light) {
	(void)index;
	if (Status st =
	        checkEnum("scene.lights.light.type", std::string_view(toString(light.type)));
	    st.isError()) {
		return st;
	}
	if (Status st = checkString("scene.lights.light.name", light.name); st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.lights.light.position", light.position);
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.lights.light.color", light.color);
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.lights.light.intensity",
	                            static_cast<double>(light.intensity));
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.lights.light.direction", light.direction);
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.lights.light.target", light.target);
	    st.isError()) {
		return st;
	}
	if (Status st =
	        checkNumber("scene.lights.light.angle", static_cast<double>(light.angle));
	    st.isError()) {
		return st;
	}
	if (Status st = checkFloat2Components("scene.lights.light.size", light.size);
	    st.isError()) {
		return st;
	}
	if (Status st =
	        checkFloat3Components("scene.lights.light.attenuation", light.attenuation);
	    st.isError()) {
		return st;
	}
	return checkNumber("scene.lights.light.range", static_cast<double>(light.range));
}

[[nodiscard]] Status validateLightCross(std::size_t index, const Light& light) {
	const std::string who = describeLight(index, light);
	switch (light.type) {
	case LightType::Point:
	case LightType::Spot:
	case LightType::Area:
		if (!light.hasPosition) {
			return Status(StatusCode::InvalidArgument,
			              who + ": type " + toString(light.type) + " requires position",
			              __LINE__);
		}
		break;
	case LightType::Directional:
		if (!light.hasDirection) {
			return Status(StatusCode::InvalidArgument,
			              who + ": type directional requires direction", __LINE__);
		}
		if (isZeroVec(light.direction)) {
			return Status(StatusCode::InvalidArgument,
			              who + ": direction is zero", __LINE__);
		}
		break;
	}
	if (light.type == LightType::Spot && !light.hasTarget) {
		return Status(StatusCode::InvalidArgument, who + ": type spot requires target",
		              __LINE__);
	}
	return Status::ok();
}

[[nodiscard]] Status validateTransformOps(const std::vector<TransformOp>& ops,
                                          std::string_view translatePath,
                                          std::string_view scalePath,
                                          std::string_view rotatePath) {
	for (const TransformOp& op : ops) {
		switch (op.kind) {
		case TransformOp::Kind::Translate:
			if (Status st = checkVec3Components(translatePath, op.translate);
			    st.isError()) {
				return st;
			}
			break;
		case TransformOp::Kind::Scale:
			if (Status st = checkVec3Components(scalePath, op.scale); st.isError()) {
				return st;
			}
			break;
		case TransformOp::Kind::Rotate: {
			std::string axis(1, op.rotateAxis);
			if (Status st = checkRotate(rotatePath, axis,
			                             static_cast<double>(op.rotateAngle));
			    st.isError()) {
				return st;
			}
			break;
		}
		}
	}
	return Status::ok();
}

[[nodiscard]] Status validateMaterialBounds(const Material& material) {
	if (Status st = checkVec3Components("scene.objects.object.material.albedo",
	                                    material.albedo);
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.ambient",
	                            static_cast<double>(material.ambient));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.diffuse",
	                            static_cast<double>(material.diffuse));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.specular",
	                            static_cast<double>(material.specular));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.shininess",
	                            static_cast<double>(material.shininess));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.reflectivity",
	                            static_cast<double>(material.reflectivity));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.transparency",
	                            static_cast<double>(material.transparency));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.ior",
	                            static_cast<double>(material.ior));
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.material.bump",
	                            static_cast<double>(material.bump));
	    st.isError()) {
		return st;
	}
	if (!material.materialRef.empty() && material.materialRef.size() > 128) {
		return Status(StatusCode::LimitExceeded,
		              std::string("string too long: material reference"), __LINE__);
	}
	if (material.texture.present) {
		if (Status st = checkString("scene.objects.object.material.texture.file",
		                            material.texture.file);
		    st.isError()) {
			return st;
		}
		if (material.texture.file.empty()) {
			return Status(StatusCode::InvalidArgument,
			              std::string("invalid texture: empty file path"), __LINE__);
		}
		if (Status st = checkFloat2Components(
		        "scene.objects.object.material.texture.scale", material.texture.scale);
		    st.isError()) {
			return st;
		}
		if (Status st = checkNumber("scene.objects.object.material.texture.offset",
		                             static_cast<double>(material.texture.offset.x));
		    st.isError()) {
			return st;
		}
		if (Status st = checkNumber("scene.objects.object.material.texture.offset",
		                             static_cast<double>(material.texture.offset.y));
		    st.isError()) {
			return st;
		}
	}
	if (material.pattern.present) {
		if (Status st = checkEnum("scene.objects.object.material.pattern.type",
		                          material.pattern.type);
		    st.isError()) {
			return st;
		}
		if (Status st = checkNumber("scene.objects.object.material.pattern.scale",
		                             static_cast<double>(material.pattern.scale));
		    st.isError()) {
			return st;
		}
		if (Status st = checkNumber("scene.objects.object.material.pattern.frequency",
		                             static_cast<double>(material.pattern.frequency));
		    st.isError()) {
			return st;
		}
	}
	return Status::ok();
}

[[nodiscard]] Status validateMaterialCross(std::size_t index, const Object& object) {
	const Material& material = object.material;
	if (material.transparency > kCrossEps &&
	    material.ior <= 1.0F + kCrossEps) {
		std::string detail(describeObject(index, object));
		detail.append(": transparent material requires ior > 1 (got ior=");
		detail.append(std::to_string(static_cast<double>(material.ior)));
		detail.append(" transparency=");
		detail.append(std::to_string(static_cast<double>(material.transparency)));
		detail.append(")");
		return Status(StatusCode::InvalidArgument, detail, __LINE__);
	}
	return Status::ok();
}

[[nodiscard]] Status validateSliceBounds(const Slice& slice) {
	if (!slice.present) {
		return Status::ok();
	}
	if (Status st = checkEnum("scene.objects.object.slice.axis", slice.axis);
	    st.isError()) {
		return st;
	}
	if (Status st = checkEnum("scene.objects.object.slice.frame", slice.frame);
	    st.isError()) {
		return st;
	}
	if (Status st = checkEnum("scene.objects.object.slice.shape", slice.shape);
	    st.isError()) {
		return st;
	}
	if (slice.hasMin) {
		if (Status st = checkNumber("scene.objects.object.slice.min",
		                             static_cast<double>(slice.minValue));
		    st.isError()) {
			return st;
		}
	}
	if (slice.hasMax) {
		if (Status st = checkNumber("scene.objects.object.slice.max",
		                             static_cast<double>(slice.maxValue));
		    st.isError()) {
			return st;
		}
	}
	return Status::ok();
}

[[nodiscard]] Status validateSliceCross(std::size_t index, const Object& object) {
	const Slice& slice = object.slice;
	if (!slice.present) {
		return Status::ok();
	}
	if (slice.hasMin && slice.hasMax && slice.minValue > slice.maxValue) {
		std::string detail(describeObject(index, object));
		detail.append(": slice min > max");
		return Status(StatusCode::InvalidArgument, detail, __LINE__);
	}
	return Status::ok();
}

[[nodiscard]] Status validateObjectBounds(std::size_t index, const Object& object) {
	(void)index;
	if (Status st = checkEnum("scene.objects.object.type",
	                          std::string_view(toString(object.type)));
	    st.isError()) {
		return st;
	}
	if (Status st = checkString("scene.objects.object.name", object.name);
	    st.isError()) {
		return st;
	}
	if (Status st =
	        checkVec3Components("scene.objects.object.center", object.center);
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.radius",
	                            static_cast<double>(object.radius));
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.objects.object.point", object.point);
	    st.isError()) {
		return st;
	}
	if (Status st =
	        checkVec3Components("scene.objects.object.normal", object.normal);
	    st.isError()) {
		return st;
	}
	if (Status st = checkVec3Components("scene.objects.object.axis", object.axis);
	    st.isError()) {
		return st;
	}
	if (Status st = checkNumber("scene.objects.object.angle",
	                            static_cast<double>(object.angle));
	    st.isError()) {
		return st;
	}
	if (object.hasHeight) {
		if (Status st = checkNumber("scene.objects.object.height",
		                             static_cast<double>(object.height));
		    st.isError()) {
			return st;
		}
	}
	if (Status st = validateMaterialBounds(object.material); st.isError()) {
		return st;
	}
	if (Status st = validateTransformOps(object.transform.ops,
	                                     "scene.objects.object.transform.translate",
	                                     "scene.objects.object.transform.scale",
	                                     "scene.objects.object.transform.rotate");
	    st.isError()) {
		return st;
	}
	return validateSliceBounds(object.slice);
}

[[nodiscard]] Status validateObjectCross(std::size_t index, const Object& object) {
	const std::string who = describeObject(index, object);
	if (!object.hasType) {
		return Status(StatusCode::InvalidArgument,
		              who + ": missing required 'type'", __LINE__);
	}
	switch (object.type) {
	case ObjectType::Sphere:
	case ObjectType::Cylinder:
	case ObjectType::Cone:
		break;
	case ObjectType::Plane:
		if (!object.hasPoint) {
			return Status(StatusCode::InvalidArgument,
			              who + ": type plane requires point", __LINE__);
		}
		if (!object.hasNormal) {
			return Status(StatusCode::InvalidArgument,
			              who + ": type plane requires normal", __LINE__);
		}
		if (isZeroVec(object.normal)) {
			return Status(StatusCode::InvalidArgument,
			              who + ": normal is zero", __LINE__);
		}
		break;
	}
	if (object.type == ObjectType::Cylinder || object.type == ObjectType::Cone) {
		if (isZeroVec(object.axis)) {
			return Status(StatusCode::InvalidArgument, who + ": axis is zero",
			              __LINE__);
		}
	}
	if (Status st = validateMaterialCross(index, object); st.isError()) {
		return st;
	}
	return validateSliceCross(index, object);
}

void collectTextureFiles(const std::vector<Object>& objects,
                         std::vector<std::string>& out) {
	for (const Object& object : objects) {
		if (object.material.texture.present && !object.material.texture.file.empty()) {
			bool seen = false;
			for (const std::string& known : out) {
				if (known == object.material.texture.file) {
					seen = true;
					break;
				}
			}
			if (!seen) {
				out.push_back(object.material.texture.file);
			}
		}
	}
}

void collectGroupTextures(const Group& group, std::vector<std::string>& out) {
	collectTextureFiles(group.objects, out);
	for (const std::shared_ptr<Group>& child : group.children) {
		if (child) {
			collectGroupTextures(*child, out);
		}
	}
}

[[nodiscard]] Status validateGroupBounds(const Group& group) {
	if (Status st = checkString("scene.objects.group.name", group.name);
	    st.isError()) {
		return st;
	}
	if (Status st = validateTransformOps(group.transform.ops,
	                                     "scene.objects.group.transform.translate",
	                                     "scene.objects.group.transform.scale",
	                                     "scene.objects.group.transform.rotate");
	    st.isError()) {
		return st;
	}
	for (std::size_t i = 0; i < group.objects.size(); ++i) {
		if (Status st = validateObjectBounds(i, group.objects[i]); st.isError()) {
			return st;
		}
	}
	for (const std::shared_ptr<Group>& child : group.children) {
		if (!child) {
			return Status(StatusCode::Internal,
			              std::string("invalid group: null child"), __LINE__);
		}
		if (Status st = validateGroupBounds(*child); st.isError()) {
			return st;
		}
	}
	return Status::ok();
}

[[nodiscard]] Status validateGroupCross(const Group& group) {
	for (std::size_t i = 0; i < group.objects.size(); ++i) {
		if (Status st = validateObjectCross(i, group.objects[i]); st.isError()) {
			return st;
		}
	}
	for (const std::shared_ptr<Group>& child : group.children) {
		if (!child) {
			return Status(StatusCode::Internal,
			              std::string("invalid group: null child"), __LINE__);
		}
		if (Status st = validateGroupCross(*child); st.isError()) {
			return st;
		}
	}
	return Status::ok();
}

} // namespace

unsigned long long estimateTextureBytes(const Scene& scene) {
	std::vector<std::string> files;
	files.reserve(scene.objects.size());
	collectTextureFiles(scene.objects, files);
	for (const Group& group : scene.groups) {
		collectGroupTextures(group, files);
	}
	unsigned long long total = 0;
	for (const std::string& path : files) {
		std::error_code code;
		const auto size = std::filesystem::file_size(path, code);
		if (code) {
			continue;
		}
		total += static_cast<unsigned long long>(size);
	}
	return total;
}

Status validate(const Scene& scene) {
	if (Status st = checkString("scene.name", scene.name); st.isError()) {
		return st;
	}
	if (Status st = validateLimits(scene.limits); st.isError()) {
		return st;
	}
	if (Status st = validateCamera(scene.camera); st.isError()) {
		return st;
	}
	if (Status st = validateBackground(scene.background); st.isError()) {
		return st;
	}
	if (Status st = validateAmbient(scene.ambient); st.isError()) {
		return st;
	}
	for (std::size_t i = 0; i < scene.lights.size(); ++i) {
		if (Status st = validateLightBounds(i, scene.lights[i]); st.isError()) {
			return st;
		}
	}
	for (std::size_t i = 0; i < scene.lights.size(); ++i) {
		if (Status st = validateLightCross(i, scene.lights[i]); st.isError()) {
			return st;
		}
	}
	for (std::size_t i = 0; i < scene.objects.size(); ++i) {
		if (Status st = validateObjectBounds(i, scene.objects[i]); st.isError()) {
			return st;
		}
	}
	for (const Group& group : scene.groups) {
		if (Status st = validateGroupBounds(group); st.isError()) {
			return st;
		}
	}
	for (std::size_t i = 0; i < scene.objects.size(); ++i) {
		if (Status st = validateObjectCross(i, scene.objects[i]); st.isError()) {
			return st;
		}
	}
	for (const Group& group : scene.groups) {
		if (Status st = validateGroupCross(group); st.isError()) {
			return st;
		}
	}
	const std::size_t nObjects = scene.totalObjectCount();
	if (nObjects > static_cast<std::size_t>(kHardMaxObjects)) {
		std::string detail("scene too large: ");
		detail.append(std::to_string(nObjects));
		detail.append(" objects, hard limit ");
		detail.append(std::to_string(kHardMaxObjects));
		return Status(StatusCode::LimitExceeded, detail, __LINE__);
	}
	if (scene.lights.size() > static_cast<std::size_t>(kHardMaxLights)) {
		std::string detail("scene too large: ");
		detail.append(std::to_string(scene.lights.size()));
		detail.append(" lights, hard limit ");
		detail.append(std::to_string(kHardMaxLights));
		return Status(StatusCode::LimitExceeded, detail, __LINE__);
	}
	if (nObjects > static_cast<std::size_t>(scene.limits.maxObjects)) {
		std::string detail("scene too large: ");
		detail.append(std::to_string(nObjects));
		detail.append(" objects, limit ");
		detail.append(std::to_string(scene.limits.maxObjects));
		return Status(StatusCode::LimitExceeded, detail, __LINE__);
	}
	if (scene.lights.size() > static_cast<std::size_t>(scene.limits.maxLights)) {
		std::string detail("scene too large: ");
		detail.append(std::to_string(scene.lights.size()));
		detail.append(" lights, limit ");
		detail.append(std::to_string(scene.limits.maxLights));
		return Status(StatusCode::LimitExceeded, detail, __LINE__);
	}
	const unsigned long long textureBytes = estimateTextureBytes(scene);
	if (textureBytes >
	    static_cast<unsigned long long>(scene.limits.maxTextureBytes)) {
		std::string detail("scene too large: ");
		detail.append(std::to_string(textureBytes));
		detail.append(" texture bytes, limit ");
		detail.append(std::to_string(scene.limits.maxTextureBytes));
		return Status(StatusCode::LimitExceeded, detail, __LINE__);
	}
	return Status::ok();
}

} // namespace rt::scene
