#pragma once

// Passe de validation `Scene` (T024) — bornes, limites et coherence croisee.
// Derive de la table unique `schema/` (regle R1) : chaque valeur est
// recontrolee par `check*()` (min/max, enums, couleurs 0-1), les compteurs
// sont compares a `limits { max_objects max_lights max_texture_bytes }`
// (erreur propre `scene too large: N ..., limit M`, code `LimitExceeded`),
// puis les regles croisees sont verifiees (ex. : `ior > 1` si transparence).
// Appelee par le parser en fin de `parseSceneTokens` (l'ordre des sous-blocs
// est libre) avant tout usage ; les vecteurs n'ont grandi que par `push_back`
// borne par la taille du fichier et les garde-fous durs ci-dessous, jamais
// par `reserve(max)` sur entree non validee (cf. docs/MEMORY_STRATEGY.md §2).
// Aucun `throw` ici (R2) ; les echecs partent en `rt::Status` avec un message
// clair, localise `fichier:ligne:colonne` par l'appelant. Le budget textures
// est estime par `stat` (taille des fichiers distincts, 0 si absent ou
// illisible) sans charger aucun contenu : pas d'allocation surprise.

#include "rt/base/Status.hpp"
#include "rt/scene/Scene.hpp"

namespace rt::scene {

// Garde-fous durs (bornes max du schema) : bornent la memoire meme si le
// bloc `limits` apparait apres `objects`/`lights` ou est absent (defauts).
// `max_objects` max = 100000, `max_lights` max = 1024 (table `schema/`).
inline constexpr int kHardMaxObjects = 100000;
inline constexpr int kHardMaxLights = 1024;

// Valide une scene complete : bornes du schema + limites + croisee.
// Succes : `Status::ok()`. Echec : code precise (`LimitExceeded` pour les
// depassements, `OutOfRange`/`InvalidArgument` pour les bornes, `IoError`
// jamais ici) et message sans prefixe de localisation (le parser ajoute
// `fichier:ligne:colonne`).
[[nodiscard]] Status validate(const Scene& scene);

// Estimation du budget textures sans chargement : somme des tailles `stat`
// des fichiers distincts references par les materiaux (`present == true`).
// Fichier absent ou illisible : contribue 0 (l'existence sera controlee en
// T102 au chargement ; ici on ne borne que la memoire reellement chargeable).
// Ne lance jamais (code d'erreur `error_code`, pas d'exception).
[[nodiscard]] unsigned long long estimateTextureBytes(const Scene& scene);

} // namespace rt::scene
