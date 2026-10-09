#pragma once

// Capture d'ecran (T077, item screenshot) — ecrit le tampon courant avec
// horodatage, sans recalcul. Repertoire configurable (defaut `docs/preuves`),
// feedback par chemin retourne, erreur propre si chemin invalide (pas de crash).
// Aucune SDL ici (testable headless) ; la touche `P` + le bouton UI `Save PNG`
// (T075) appellent ce module depuis `app/`.

#include <string>

#include "rt/base/Result.hpp"

namespace rt::render {
class Framebuffer;
}

namespace rt::io {

// Sauve `fb` en PNG horodaté `screenshot_AAAAMMJJ_HHMMSS.png` dans `dir`.
// Retourne le chemin ecrit en cas de succes, `IoError` sinon (dossier absent,
// tampon vide, echec d'ecriture). Ne modifie jamais `fb`.
[[nodiscard]] Result<std::string> saveScreenshot(const render::Framebuffer& fb,
                                                const std::string& dir);

// Nom seul (sans ecriture), pour les tests et les messages.
[[nodiscard]] std::string screenshotName(long long epochSeconds);

} // namespace rt::io
