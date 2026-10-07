#pragma once

// Schema unique des directives `.rt` (T021, regle R1).
// Une seule table declarative decrit chaque directive : chemin hierarchique,
// type, defaut, bornes, unite, description, valeurs enum, alias, cardinalite,
// flag reserve. Le parser (T023), la validation (T024), l'UI (T075) et la doc
// (`scripts/gen_doc.sh`) derivent tous de cette table : aucune definition
// dupliquee ailleurs. Ajouter une directive = une seule ligne dans
// `src/schema/Directives.cpp`.
// Reference format : `docs/FORMAT_SCENE.md` §5. Inspire de RNA/Blender :
// `docs/INSPIRATION_BLENDER.md` §5.
// Regle R2 : aucune exception levee ici (pas de `throw`) ; les echecs sont
// rapportes par `rt::Status`. Les recherches sont `noexcept` et sans
// allocation (vues sur litteraux).

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string_view>

#include "rt/base/Status.hpp"

namespace rt::schema {

enum class ValueType : std::uint8_t {
    Int = 0,
    Float = 1,
    Vec3 = 2,
    String = 3,
    Enum = 4,
    Block = 5,
    Float2 = 6,
    Float3 = 7,
    Scale = 8,
    Rotate = 9,
};

[[nodiscard]] std::string_view toString(ValueType type) noexcept;

enum class Occurrence : std::uint8_t {
    OptionalOnce = 0,
    RequiredOnce = 1,
    OptionalMany = 2,
};

[[nodiscard]] std::string_view toString(Occurrence occurrence) noexcept;

struct Directive {
    std::string_view path;
    ValueType type = ValueType::String;
    std::string_view defaultValue;
    double minValue = 0.0;
    double maxValue = 0.0;
    bool hasRange = false;
    std::string_view unit;
    std::string_view description;
    std::string_view enumValues;
    std::string_view alias;
    Occurrence occurrence = Occurrence::OptionalOnce;
    bool reserved = false;
};

// Vue sur la table unique (definie dans `src/schema/Directives.cpp`).
[[nodiscard]] std::span<const Directive> all() noexcept;
[[nodiscard]] std::size_t count() noexcept;

// Point d'entree du parser (T023) : resout le chemin exact ou son alias
// de feuille (`scene.camera.lookAt` -> `scene.camera.target`). Retourne
// `nullptr` si la directive est inconnue (erreur fatale, jamais ignoree).
[[nodiscard]] const Directive* find(std::string_view path) noexcept;

// Points d'entree de la validation (T024) : verifient une valeur contre
// les bornes de la meme entree. Type incoherent -> `InvalidArgument`,
// hors bornes -> `OutOfRange`, enum inconnue -> `InvalidArgument`,
// chaine trop longue (> 128) -> `LimitExceeded`.
[[nodiscard]] Status checkInt(std::string_view path, long long value);
[[nodiscard]] Status checkNumber(std::string_view path, double value);
[[nodiscard]] Status checkEnum(std::string_view path, std::string_view value);
[[nodiscard]] Status checkString(std::string_view path, std::string_view value);
[[nodiscard]] Status checkRotate(std::string_view path, std::string_view axis, double angle);

// Point d'entree de la doc : table Markdown generee depuis la meme table
// (`scripts/gen_doc.sh` -> `docs/FORMAT_SCENE.md` §5.8). Une ligne par
// directive, ordre de la table. Point d'entree de l'UI (T075) : `all()`
// expose type/min/max/defaut/description pour chaque champ editable.
void writeMarkdown(std::ostream& out);

} // namespace rt::schema
