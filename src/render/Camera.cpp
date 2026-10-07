// Camera de rendu (T031) — implementation sans exception (R2).
// Voir `include/rt/render/Camera.hpp` pour le contrat et
// `docs/ARCHITECTURE.md` §4.1 pour les formules (Gram-Schmidt).
// `init()` seul valide (chemin froid, `Status`) ; `rayForPixel()` est
// `noexcept` sans allocation (R3, hot path du futur `Renderer` T032).

#include "rt/render/Camera.hpp"

#include <cmath>

#include "rt/scene/Scene.hpp"

namespace rt::render {

namespace {

constexpr int kMinDim = 1;
constexpr int kMaxDim = 8192;
constexpr float kMinFov = 1.0F;
constexpr float kMaxFov = 179.0F;

} // namespace

Status Camera::init(const scene::Camera& desc, int width, int height) {
	return init(desc.position, desc.target, desc.up, desc.fov, width, height);
}

Status Camera::init(Vec3 position, Vec3 target, Vec3 up, float fovDeg, int width, int height) {
	valid_ = false;
	if (width < kMinDim || width > kMaxDim || height < kMinDim || height > kMaxDim) {
		return Status::error(StatusCode::InvalidArgument, "bad camera size: expected 1..8192",
		                     __LINE__);
	}
	if (!std::isfinite(fovDeg) || fovDeg < kMinFov || fovDeg > kMaxFov) {
		return Status::error(StatusCode::OutOfRange, "bad camera fov: expected 1..179",
		                     __LINE__);
	}
	const Vec3 view = target - position;
	if (lengthSquared(view) <= kEpsilon * kEpsilon) {
		return Status::error(StatusCode::InvalidArgument, "invalid camera: target == position",
		                     __LINE__);
	}
	if (lengthSquared(up) <= kEpsilon * kEpsilon) {
		return Status::error(StatusCode::InvalidArgument, "invalid camera: up degenerate",
		                     __LINE__);
	}
	const Vec3 forward = normalize(view);
	const Vec3 rightUnnorm = cross(forward, up);
	if (lengthSquared(rightUnnorm) <= kEpsilon * kEpsilon) {
		return Status::error(StatusCode::InvalidArgument,
		                     "invalid camera: up collinear with view direction", __LINE__);
	}
	const Vec3 right = normalize(rightUnnorm);
	const Vec3 trueUp = cross(right, forward);

	const float halfHeight = std::tan(fovDeg * 0.5F * kPi / 180.0F);
	if (!std::isfinite(halfHeight) || halfHeight <= 0.0F) {
		return Status::error(StatusCode::OutOfRange, "bad camera fov: tan out of range",
		                     __LINE__);
	}
	const float aspect = static_cast<float>(width) / static_cast<float>(height);

	position_ = position;
	forward_ = forward;
	right_ = right;
	trueUp_ = trueUp;
	halfHeight_ = halfHeight;
	halfWidth_ = aspect * halfHeight;
	fovDeg_ = fovDeg;
	aspect_ = aspect;
	width_ = width;
	height_ = height;
	valid_ = true;
	return Status::ok();
}

Ray Camera::rayForPixel(int x, int y, Vec2 jitter) const noexcept {
	const float sx = static_cast<float>(x) + 0.5F + jitter.x;
	const float sy = static_cast<float>(y) + 0.5F + jitter.y;
	const float u = (sx / static_cast<float>(width_)) * 2.0F - 1.0F;
	const float v = 1.0F - (sy / static_cast<float>(height_)) * 2.0F;
	const Vec3 dir = normalize(forward_ + right_ * (u * halfWidth_) + trueUp_ * (v * halfHeight_));
	return Ray(position_, dir, 0);
}

Ray Camera::rayForPixel(int x, int y) const noexcept {
	return rayForPixel(x, y, Vec2{});
}

} // namespace rt::render
