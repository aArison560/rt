// Panneau microui (T075) — implementation.
// `fieldNames()` lit la table unique `schema/Directives` (R1) : une ligne
// ajoutee la-bas apparait ici (test). Les setters modifient la `Scene`
// attachee et levent R5. `frame()` pilote un `mu_Context` vendored
// (boutons + sliders minimaux, sans SDL ici ; le dessin SDL vit dans
// `platform/` ou `app/` via les requetes `take*`).

#include "rt/ui/Panel.hpp"

#include <cmath>
#include <cstring>
#include <new>

extern "C" {
#include "microui/microui.h"
}

#include "rt/scene/Scene.hpp"
#include "rt/schema/Directives.hpp"

namespace rt::ui {

namespace {

constexpr float kMinFov = 10.0F;
constexpr float kMaxFov = 120.0F;

int textWidth(mu_Font font, const char* text, int len) {
	(void)font;
	if (text == nullptr || len <= 0) {
		return 0;
	}
	return len * 8;
}

int textHeight(mu_Font font) {
	(void)font;
	return 14;
}

} // namespace

Panel::~Panel() {
	if (ctx_ != nullptr) {
		delete static_cast<mu_Context*>(ctx_);
		ctx_ = nullptr;
	}
}

void Panel::attach(scene::Scene* scene) noexcept {
	if (ctx_ == nullptr) {
		auto* ctx = new (std::nothrow) mu_Context();
		if (ctx == nullptr) {
			return;
		}
		mu_init(ctx);
		ctx->text_width = &textWidth;
		ctx->text_height = &textHeight;
		ctx_ = ctx;
	}
	scene_ = scene;
	launchRequested_ = false;
	saveRequested_ = false;
	progressDone_ = 0;
	progressTotal_ = 0;
}

std::vector<std::string> Panel::fieldNames() const {
	std::vector<std::string> names;
	const auto table = schema::all();
	names.reserve(table.size());
	for (const auto& entry : table) {
		names.emplace_back(entry.path);
	}
	return names;
}

bool Panel::setBackground(Vec3 color) noexcept {
	if (scene_ == nullptr) {
		return false;
	}
	scene_->background.color = color;
	scene_->sceneDirty = true;
	scene_->displayDirty = true;
	return true;
}

bool Panel::setAmbientIntensity(float intensity) noexcept {
	if (scene_ == nullptr) {
		return false;
	}
	if (intensity < 0.0F) {
		intensity = 0.0F;
	}
	if (intensity > 10.0F) {
		intensity = 10.0F;
	}
	scene_->ambient.intensity = intensity;
	scene_->sceneDirty = true;
	scene_->displayDirty = true;
	return true;
}

bool Panel::setCameraFov(float fov) noexcept {
	if (scene_ == nullptr) {
		return false;
	}
	if (fov < kMinFov) {
		fov = kMinFov;
	}
	if (fov > kMaxFov) {
		fov = kMaxFov;
	}
	scene_->camera.fov = fov;
	scene_->sceneDirty = true;
	scene_->displayDirty = true;
	return true;
}

bool Panel::setFirstAlbedo(Vec3 albedo) noexcept {
	if (scene_ == nullptr || scene_->objects.empty()) {
		return false;
	}
	scene_->objects[0].material.albedo = albedo;
	scene_->touchObjects();
	return true;
}

bool Panel::setFirstObjectX(float x) noexcept {
	if (scene_ == nullptr || scene_->objects.empty()) {
		return false;
	}
	if (!std::isfinite(x) || x < -100.0F || x > 100.0F) {
		return false;
	}
	scene_->objects[0].center.x = x;
	scene_->objects[0].point.x = x;
	scene_->touchObjects();
	return true;
}

bool Panel::setFirstTextureScale(float scale) noexcept {
	if (scene_ == nullptr || scene_->objects.empty()) {
		return false;
	}
	if (!std::isfinite(scale) || scale < 0.25F || scale > 8.0F) {
		return false;
	}
	auto& texture = scene_->objects[0].material.texture;
	if (!texture.present || texture.file.empty()) {
		return false;
	}
	texture.scale = Vec3(scale, scale, 0.0F);
	scene_->sceneDirty = true;
	scene_->displayDirty = true;
	return true;
}

void Panel::frame() noexcept {
	if (ctx_ == nullptr || scene_ == nullptr) {
		return;
	}
	auto* ctx = static_cast<mu_Context*>(ctx_);
	mu_begin(ctx);
	if (mu_begin_window(ctx, "RT controls", mu_rect(10, 10, 240, 340))) {
		mu_layout_row(ctx, 1, nullptr, 0);
		mu_label(ctx, "scene (schema-driven)");
		// T108 (*Environment 1*) : barre de progression (batches spp
		// `done/total`, pourcentage + barre textuelle). Affichee pendant
		// le rendu via `setProgress()` (callback `onProgress`, 1x/batch).
		if (progressTotal_ > 0 && progressDone_ >= 0) {
			char bar[64];
			int done = progressDone_ > progressTotal_ ? progressTotal_ : progressDone_;
			const int pct = (done * 100) / progressTotal_;
			int blocks = pct / 10;
			if (blocks < 0) {
				blocks = 0;
			} else if (blocks > 10) {
				blocks = 10;
			}
			int pos = 0;
			bar[pos++] = '[';
			for (int i = 0; i < 10; ++i) {
				bar[pos++] = i < blocks ? '#' : '-';
			}
			bar[pos++] = ']';
			bar[pos++] = ' ';
			// Pourcentage (itoa local, sans allocation).
			int hundreds = pct / 100;
			int tens = (pct / 10) % 10;
			int ones = pct % 10;
			if (hundreds > 0) {
				bar[pos++] = static_cast<char>('0' + hundreds);
			}
			if (hundreds > 0 || tens > 0) {
				bar[pos++] = static_cast<char>('0' + tens);
			}
			bar[pos++] = static_cast<char>('0' + ones);
			bar[pos++] = '%';
			bar[pos] = '\0';
			mu_label(ctx, bar);
		} else {
			mu_label(ctx, "idle");
		}
		// Boutons : lancement + sauvegarde (consommes via take*).
		if (mu_button(ctx, "Render")) {
			launchRequested_ = true;
		}
		if (mu_button(ctx, "Save PNG")) {
			saveRequested_ = true;
		}
		// Sliders minimaux (FOV + ambiance) branchés sur la scene.
		static float fovSlider = 60.0F;
		static float ambSlider = 1.0F;
		static float objXSlider = 0.0F;
		static float texScaleSlider = 1.0F;
		fovSlider = scene_->camera.fov;
		ambSlider = scene_->ambient.intensity;
		if (!scene_->objects.empty()) {
			objXSlider = scene_->objects[0].center.x;
			if (scene_->objects[0].material.texture.present) {
				texScaleSlider = scene_->objects[0].material.texture.scale.x;
			}
		}
		mu_label(ctx, "fov");
		if (mu_slider_ex(ctx, &fovSlider, kMinFov, kMaxFov, 1.0F, "%.0f", MU_OPT_ALIGNCENTER) != 0) {
			setCameraFov(fovSlider);
		}
		mu_label(ctx, "ambient");
		if (mu_slider_ex(ctx, &ambSlider, 0.0F, 5.0F, 0.1F, "%.1f", MU_OPT_ALIGNCENTER) != 0) {
			setAmbientIntensity(ambSlider);
		}
		// T109 (*Environment 3*) : objet et texture en direct (1er objet).
		// Chaque edition leve R5 -> appercu 1 spp immediat puis affinage
		// (machine `Interactive`, T076), sans relancer le programme.
		mu_label(ctx, "obj.x");
		if (mu_slider_ex(ctx, &objXSlider, -5.0F, 5.0F, 0.1F, "%.1f", MU_OPT_ALIGNCENTER) != 0) {
			setFirstObjectX(objXSlider);
		}
		mu_label(ctx, "tex.scale");
		if (mu_slider_ex(ctx, &texScaleSlider, 0.25F, 8.0F, 0.25F, "%.2f", MU_OPT_ALIGNCENTER) != 0) {
			setFirstTextureScale(texScaleSlider);
		}
		mu_end_window(ctx);
	}
	mu_end(ctx);
}

bool Panel::takeLaunchRequest() noexcept {
	const bool pending = launchRequested_;
	launchRequested_ = false;
	return pending;
}

bool Panel::takeSaveRequest() noexcept {
	const bool pending = saveRequested_;
	saveRequested_ = false;
	return pending;
}

void Panel::setProgress(int done, int total) noexcept {
	if (total <= 0 || done < 0) {
		progressDone_ = 0;
		progressTotal_ = 0;
		return;
	}
	progressDone_ = done > total ? total : done;
	progressTotal_ = total;
}

float Panel::progressFraction() const noexcept {
	if (progressTotal_ <= 0 || progressDone_ < 0) {
		return 0.0F;
	}
	const float done = static_cast<float>(progressDone_);
	const float total = static_cast<float>(progressTotal_);
	if (!(total > 0.0F)) {
		return 0.0F;
	}
	float frac = done / total;
	if (!(frac >= 0.0F)) {
		return 0.0F;
	}
	if (frac > 1.0F) {
		return 1.0F;
	}
	return frac;
}

} // namespace rt::ui
