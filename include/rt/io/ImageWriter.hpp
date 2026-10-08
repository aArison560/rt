#pragma once

// Ecriture d'image (T034) — PNG via libpng (systeme) + fallback PPM.
// `writeImage()` choisit selon l'extension : `.ppm` -> PPM binaire P6
// toujours (sans dependance) ; sinon PNG si libpng est disponible a la
// compilation (`__has_include(<png.h>)`), PPM sinon (fallback documente,
// jamais de crash). Le framebuffer doit etre `present()` (T030/T032) :
// on ecrit `displayData()` (RGBA8). Nom valide, echec -> `Status`
// (`InvalidArgument` si vide, `IoError` sinon), jamais d'exception (R2)
// hors `bad_alloc` du chemin froid (remonte au filet `main` comme
// `Scene`/`Framebuffer`). Verifie par `file`/`identify` (OUTILS.md §5).

#include <string>

#include "rt/base/Status.hpp"

namespace rt::render {
class Framebuffer;
}

namespace rt::io {

// Ecrit `fb` vers `path` (PNG sauf `.ppm` -> PPM). Dossiers inexistants,
// chemins vides, framebuffer vide -> `Status` d'erreur, code retour != 0
// dans `main`, jamais de crash.
[[nodiscard]] Status writeImage(const render::Framebuffer& fb, const std::string& path);

} // namespace rt::io
