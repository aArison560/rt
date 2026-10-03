#pragma once
#include "math/Vec3.hpp"
#include <memory>

class ImageTexture; // forward : évite d'inclure stb dans tous les .cpp

// Type futur Phase 3 (réflexion/réfraction) : réservé dès Phase 2 pour ne pas re-câbler.
enum class MaterialType {
    Diffuse = 0,
    Metal = 1,      // Phase 3 (réflexion)
    Dielectric = 2  // Phase 3 (réfraction)
};

// Matériau par objet : uni (albedo) -> checker (étape 1) -> image (étape 2, prioritaire).
struct Material {
    Vec3 albedo{255, 255, 255};   // couleur 1, 0-255 comme Object::color
    Vec3 albedo2{0, 0, 0};        // couleur 2 du damier
    MaterialType type = MaterialType::Diffuse;
    bool useChecker = false;      // false = rendu uni Phase 1 bit-identique
    double uvScale = 8.0;         // cases du damier par unité UV
    // Image collage : partagée (copiée avec Hit), prioritaire sur checker si actif.
    std::shared_ptr<ImageTexture> map = nullptr;
    bool useImage = false;

    Material() = default;
    explicit Material(const Vec3& col) : albedo(col) {}
};
