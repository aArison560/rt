#include "core/Texture.hpp"
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION // 1 seule fois dans tout le binaire (sinon symboles dupliqués)
#include "stb/stb_image.h"

// Ramène t dans [0,1] selon le mode (tuilage vs bords).
static double wrapCoord(double t, TexWrap w) {
    if (w == TexWrap::Clamp) {
        if (t < 0) return 0;
        if (t > 1) return 1;
        return t;
    }
    // Repeat : partie fractionnaire (fract(1.0) -> 0).
    double f = t - std::floor(t);
    if (f < 0) f += 1.0;
    if (f >= 1.0) f = 0.999999;
    return f;
}

// Décode PNG/JPG/BMP/TGA/PPM en RGB (3 canaux forcés). False = fichier absent/corrompu.
bool ImageTexture::load(const std::string& path) {
    int n = 0;
    unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 3);
    if (!px || w <= 0 || h <= 0) {
        w = 0; h = 0;
        return false;
    }
    data.assign(px, px + (size_t)w * (size_t)h * 3); // copie RAII
    stbi_image_free(px); // libère le malloc interne stb
    return true;
}

// Sampler2D : (u,v) -> pixel 0-255, plus proche voisin.
Vec3 ImageTexture::sample(double u, double v) const {
    if (data.empty() || w <= 0 || h <= 0)
        return Vec3(255, 0, 255); // magenta = garde-fou (parser refuse avant en pratique)
    double uu = wrapCoord(u, wrap);
    double vv = wrapCoord(v, wrap);
    vv = 1.0 - vv; // flip V : stb ligne 0 en haut, UV Blender origine en bas
    int x = (int)(uu * w);
    int y = (int)(vv * h);
    if (x < 0) x = 0;
    if (x >= w) x = w - 1; // garde-fou flottant
    if (y < 0) y = 0;
    if (y >= h) y = h - 1;
    size_t i = ((size_t)y * (size_t)w + (size_t)x) * 3;
    return Vec3(data[i], data[i + 1], data[i + 2]);
}
