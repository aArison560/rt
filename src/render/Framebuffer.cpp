// Framebuffer prealloue et persistant (T030) — implementation sans exception.
// Voir `include/rt/render/Framebuffer.hpp` pour le contrat et
// `docs/MEMORY_STRATEGY.md` §4.1 pour le budget memoire (W*H*20).
// `init()` seul touche l'allocateur (chemin froid) ; `clear()`,
// `addSample()` et `present()` n'allouent jamais (R3, registres/arena
// uniquement). `present()` sature 0..1 (NaN/Inf -> 0) puis gamma 2.2.

#include "rt/render/Framebuffer.hpp"

#include <algorithm>
#include <cmath>

namespace rt::render {

namespace {

constexpr int kMinDim = 1;
constexpr int kMaxDim = 8192;
constexpr float kGamma = 1.0F / 2.2F;

[[nodiscard]] float tonemapChannel(float c) noexcept {
	if (!std::isfinite(c)) {
		return 0.0F;
	}
	if (c <= 0.0F) {
		return 0.0F;
	}
	if (c >= 1.0F) {
		return 1.0F;
	}
	return c;
}

[[nodiscard]] std::uint8_t encodeChannel(float linear) noexcept {
	const float clamped = tonemapChannel(linear);
	const float corrected = std::pow(clamped, kGamma);
	const float scaled = corrected * 255.0F + 0.5F;
	if (scaled <= 0.0F) {
		return 0;
	}
	if (scaled >= 255.0F) {
		return 255;
	}
	return static_cast<std::uint8_t>(scaled);
}

} // namespace

Status Framebuffer::init(int width, int height) {
	if (width < kMinDim || width > kMaxDim || height < kMinDim || height > kMaxDim) {
		return Status::error(StatusCode::InvalidArgument, "bad framebuffer size: expected 1..8192",
		                     __LINE__);
	}
	if (width == width_ && height == height_ && !display_.empty()) {
		clear();
		return Status::ok();
	}
	const auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
	display_.assign(count, Rgba8{});
	accum_.assign(count, Vec3{});
	counts_.assign(count, 0);
	width_ = width;
	height_ = height;
	return Status::ok();
}

void Framebuffer::clear() noexcept {
	std::fill(display_.begin(), display_.end(), Rgba8{});
	std::fill(accum_.begin(), accum_.end(), Vec3{});
	std::fill(counts_.begin(), counts_.end(), 0);
}

void Framebuffer::addSample(int x, int y, Vec3 color) noexcept {
	if (!inBounds(x, y)) {
		return;
	}
	const std::size_t idx = indexOf(x, y);
	accum_[idx] += color;
	++counts_[idx];
}

void Framebuffer::present() noexcept {
	const std::size_t count = pixelCount();
	for (std::size_t idx = 0; idx < count; ++idx) {
		const int samples = counts_[idx];
		Vec3 avg{};
		if (samples > 0) {
			avg = accum_[idx] / static_cast<float>(samples);
		}
		Rgba8& out = display_[idx];
		out.r = encodeChannel(avg.x);
		out.g = encodeChannel(avg.y);
		out.b = encodeChannel(avg.z);
		out.a = 255;
	}
}

Vec3 Framebuffer::accumAt(int x, int y) const noexcept {
	if (!inBounds(x, y)) {
		return Vec3{};
	}
	return accum_[indexOf(x, y)];
}

int Framebuffer::samplesAt(int x, int y) const noexcept {
	if (!inBounds(x, y)) {
		return 0;
	}
	return counts_[indexOf(x, y)];
}

} // namespace rt::render
