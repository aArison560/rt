#pragma once
#include "math/Vec3.hpp"
#include <string>
#include <vector>

// Mode de débordement UV hors [0,1] (convention Blender).
enum class TexWrap {
    Repeat = 0, // fract(u) : tuilage infini
    Clamp = 1   // bloque aux bords : étire le dernier pixel
};

// Image collage Phase 2 : décode stb_image -> sampler2D(u,v).
// u,v en [0,1) (sortie getUV). Non copiable (pixels lourds) : usage via shared_ptr.
class ImageTexture {
public:
    ImageTexture() = default;
    ~ImageTexture() = default;
    ImageTexture(const ImageTexture&) = delete;
    ImageTexture& operator=(const ImageTexture&) = delete;

    bool load(const std::string& path);   // stbi_load forcé RGB, false si absent/corrompu
    Vec3 sample(double u, double v) const; // plus proche voisin, retour 0-255 (magenta si vide)
    bool empty() const { return data.empty(); }

    int w = 0, h = 0;
    TexWrap wrap = TexWrap::Repeat;

private:
    std::vector<unsigned char> data; // pixels RGB, w*h*3 (RAII, sans new nu)
};
