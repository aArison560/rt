// Table unique des directives `.rt` (T021, regle R1).
// Une seule source de verite : chaque directive = une seule ligne du tableau
// `kDirectives` ci-dessous (entre `clang-format off/on` pour garder cette
// propriete apres `make format`). Le parser (T023) utilise `find()`, la
// validation (T024) `check*()`, l'UI (T075) `all()`, la doc `writeMarkdown()`
// (`scripts/gen_doc.sh`). Aucune definition dupliquee ailleurs.
// Regle R2 : sans exception ici ; les echecs partent en `rt::Status`.

#include "rt/schema/Directives.hpp"

#include <cmath>
#include <ostream>
#include <string>

namespace rt::schema {

namespace {

constexpr std::string_view kUnknown = "unknown";

} // namespace

std::string_view toString(ValueType type) noexcept {
    switch (type) {
    case ValueType::Int:
        return "int";
    case ValueType::Float:
        return "float";
    case ValueType::Vec3:
        return "vec3";
    case ValueType::String:
        return "string";
    case ValueType::Enum:
        return "enum";
    case ValueType::Block:
        return "bloc";
    case ValueType::Float2:
        return "float2";
    case ValueType::Float3:
        return "float3";
    case ValueType::Scale:
        return "scale";
    case ValueType::Rotate:
        return "rotate";
    }
    return kUnknown;
}

std::string_view toString(Occurrence occurrence) noexcept {
    switch (occurrence) {
    case Occurrence::OptionalOnce:
        return "0/1";
    case Occurrence::RequiredOnce:
        return "1";
    case Occurrence::OptionalMany:
        return "0..N";
    }
    return kUnknown;
}

// clang-format off
// Chaque ligne = une directive (DoD T021 : ajouter une directive = une ligne).
static constexpr Directive kDirectives[] = {
    {"scene", ValueType::Block, "", 0.0, 0.0, false, "", "Racine du fichier. camera + objects requis.", "", "", Occurrence::RequiredOnce, false},
    {"scene.name", ValueType::String, "", 0.0, 0.0, false, "", "Nom de scene en tete, optionnel.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits", ValueType::Block, "", 0.0, 0.0, false, "", "Resolution, echantillonnage, garde-fous memoire.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.width", ValueType::Int, "640", 1.0, 8192.0, true, "px", "Largeur image.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.height", ValueType::Int, "480", 1.0, 8192.0, true, "px", "Hauteur image.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.samples", ValueType::Int, "4", 1.0, 1024.0, true, "spp", "Echantillons par pixel.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.max_depth", ValueType::Int, "4", 0.0, 16.0, true, "", "Profondeur max de recursion.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.seed", ValueType::Int, "0", 0.0, 4294967295.0, true, "", "Graine combinee aux coords absolues (RNG).", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.max_objects", ValueType::Int, "256", 1.0, 100000.0, true, "", "Au-dela = erreur scene too large.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.max_lights", ValueType::Int, "16", 1.0, 1024.0, true, "", "Idem pour les lumieres.", "", "", Occurrence::OptionalOnce, false},
    {"scene.limits.max_texture_bytes", ValueType::Int, "67108864", 0.0, 2147483647.0, true, "o", "Budget textures cumule.", "", "", Occurrence::OptionalOnce, false},
    {"scene.camera", ValueType::Block, "", 0.0, 0.0, false, "", "Oeil + visee. Seule difference autorisee pour Did-you-know.", "", "", Occurrence::RequiredOnce, false},
    {"scene.camera.position", ValueType::Vec3, "(0 1 4)", 0.0, 0.0, false, "", "Position de l oeil.", "", "", Occurrence::OptionalOnce, false},
    {"scene.camera.target", ValueType::Vec3, "(0 0 0)", 0.0, 0.0, false, "", "Point vise. Doit differer de position.", "", "lookAt", Occurrence::OptionalOnce, false},
    {"scene.camera.up", ValueType::Vec3, "(0 1 0)", 0.0, 0.0, false, "", "Verticale monde.", "", "", Occurrence::OptionalOnce, false},
    {"scene.camera.fov", ValueType::Float, "60", 1.0, 179.0, true, "deg", "Champ vertical.", "", "", Occurrence::OptionalOnce, false},
    {"scene.background", ValueType::Block, "", 0.0, 0.0, false, "", "Couleur des rayons manques.", "", "", Occurrence::OptionalOnce, false},
    {"scene.background.color", ValueType::Vec3, "(0 0 0)", 0.0, 1.0, true, "", "Fond de scene.", "", "", Occurrence::OptionalOnce, false},
    {"scene.ambient", ValueType::Block, "", 0.0, 0.0, false, "", "Ambiance globale pilotee par fichier.", "", "", Occurrence::OptionalOnce, false},
    {"scene.ambient.color", ValueType::Vec3, "(0.06 0.06 0.08)", 0.0, 1.0, true, "", "Teinte ambiante.", "", "", Occurrence::OptionalOnce, false},
    {"scene.ambient.intensity", ValueType::Float, "1.0", 0.0, 10.0, true, "", "Intensite globale.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights", ValueType::Block, "", 0.0, 0.0, false, "", "Conteneur. 0 lumiere = ambiant seul.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light", ValueType::Block, "", 0.0, 0.0, false, "", "Une source. Tete [type] [nom] equivaut a type et name.", "", "", Occurrence::OptionalMany, false},
    {"scene.lights.light.type", ValueType::Enum, "point", 0.0, 0.0, false, "", "Type de source. dir est un alias de directional. area reserve.", "point|spot|directional|dir|area", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.name", ValueType::String, "", 0.0, 0.0, false, "", "Nom pour logs et UI.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.position", ValueType::Vec3, "", 0.0, 0.0, false, "", "Position si point, spot ou area.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.color", ValueType::Vec3, "(1 1 1)", 0.0, 1.0, true, "", "Couleur source.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.intensity", ValueType::Float, "1.0", 0.0, 1000.0, true, "", "Doubler double la contribution.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.direction", ValueType::Vec3, "", 0.0, 0.0, false, "", "Direction si directional.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.target", ValueType::Vec3, "", 0.0, 0.0, false, "", "Point vise si spot.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.angle", ValueType::Float, "30", 1.0, 90.0, true, "deg", "Demi-ouverture si spot.", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.size", ValueType::Float2, "1 1", 0.000001, 1000000000.0, true, "", "Taille source etendue. Reserve spot non ponctuel.", "", "", Occurrence::OptionalOnce, true},
    {"scene.lights.light.attenuation", ValueType::Float3, "1 0 0", 0.0, 1000000000.0, true, "", "Attenuation 1 sur (c + l d + q d2).", "", "", Occurrence::OptionalOnce, false},
    {"scene.lights.light.range", ValueType::Float, "0", 0.0, 1000000000.0, true, "", "Portee max, 0 = infinie.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects", ValueType::Block, "", 0.0, 0.0, false, "", "Conteneur. Append simple, doublons coexistent.", "", "", Occurrence::RequiredOnce, false},
    {"scene.objects.object", ValueType::Block, "", 0.0, 0.0, false, "", "Un objet. Intersection propre a chaque type.", "", "", Occurrence::OptionalMany, false},
    {"scene.objects.object.type", ValueType::Enum, "", 0.0, 0.0, false, "", "Type. Tete ou propriete, requis.", "sphere|plane|cylinder|cone", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.name", ValueType::String, "", 0.0, 0.0, false, "", "Nom pour logs et UI.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.center", ValueType::Vec3, "(0 0 0)", 0.0, 0.0, false, "", "Centre en espace objet.", "", "position", Occurrence::OptionalOnce, false},
    {"scene.objects.object.radius", ValueType::Float, "1.0", 0.000001, 1000000000.0, true, "", "Rayon sphere et cylindre.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.point", ValueType::Vec3, "", 0.0, 0.0, false, "", "Plan : un point en espace objet.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.normal", ValueType::Vec3, "", 0.0, 0.0, false, "", "Plan : normale en espace objet.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.axis", ValueType::Vec3, "(0 1 0)", 0.0, 0.0, false, "", "Axe local du cylindre et cone infinis.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.angle", ValueType::Float, "20", 1.0, 89.0, true, "deg", "Cone : demi-angle au sommet.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.height", ValueType::Float, "", 0.000001, 1000000000.0, true, "", "Reserve : troncature Limited objects. Absent = infini.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.group", ValueType::Block, "", 0.0, 0.0, false, "", "Element compose reutilisable, recursif.", "", "", Occurrence::OptionalMany, false},
    {"scene.objects.group.name", ValueType::String, "", 0.0, 0.0, false, "", "Nom du groupe, tete ou propriete.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material", ValueType::Block, "", 0.0, 0.0, false, "", "Attache a l objet. Absent = gris mat.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.albedo", ValueType::Vec3, "(0.8 0.8 0.8)", 0.0, 1.0, true, "", "Couleur de base.", "", "color", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.ambient", ValueType::Float, "0.1", 0.0, 1.0, true, "", "Poids de l ambiance globale.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.diffuse", ValueType::Float, "0.7", 0.0, 1.0, true, "", "Poids Lambert.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.specular", ValueType::Float, "0.5", 0.0, 1.0, true, "", "Poids Blinn-Phong, petit point blanc.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.shininess", ValueType::Float, "32", 1.0, 1024.0, true, "", "Exposant speculaire.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.reflectivity", ValueType::Float, "0.0", 0.0, 1.0, true, "", "Miroir continu, 0 = mat et 1 = miroir pur.", "", "reflect", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.transparency", ValueType::Float, "0.0", 0.0, 1.0, true, "", "Transparence.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.ior", ValueType::Float, "1.5", 1.0, 3.0, true, "", "Indice de Descartes. 1 = pas de deviation.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.material.bump", ValueType::Float, "0.0", 0.0, 10.0, true, "", "Force de bump. Reserve textures.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.texture", ValueType::Block, "", 0.0, 0.0, false, "", "Texture image. Reserve item Textures.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.texture.file", ValueType::String, "", 0.0, 0.0, false, "", "Chemin image en tete. Requis si texture presente.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.texture.scale", ValueType::Float2, "1 1", 0.000001, 1000000000.0, true, "", "Etirement de texture.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.texture.offset", ValueType::Float2, "0 0", 0.0, 0.0, false, "", "Decalage de texture.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.pattern", ValueType::Block, "", 0.0, 0.0, false, "", "Procedural. Reserve item Disruptions.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.pattern.type", ValueType::Enum, "checker", 0.0, 0.0, false, "", "Type de motif.", "sine|checker|perlin", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.pattern.scale", ValueType::Float, "1", 0.000001, 1000000000.0, true, "", "Echelle du motif.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.material.pattern.frequency", ValueType::Float, "1", 0.000001, 1000000000.0, true, "", "Frequence du motif.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.transform", ValueType::Block, "", 0.0, 0.0, false, "", "Translations et rotations. Rayon en espace objet via M-1.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.transform.translate", ValueType::Vec3, "(0 0 0)", 0.0, 0.0, false, "", "Translation. Repetables dans l ordre d ecriture.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.transform.scale", ValueType::Scale, "(1 1 1)", 0.000001, 1000000000.0, true, "", "Echelle, 1 nombre = uniforme.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.transform.rotate", ValueType::Rotate, "", 0.0, 360.0, true, "deg", "Rotation axis x ou y ou z angle d.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.group.transform", ValueType::Block, "", 0.0, 0.0, false, "", "Transform parent appliquee aux enfants.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.group.transform.translate", ValueType::Vec3, "(0 0 0)", 0.0, 0.0, false, "", "Translation du groupe.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.group.transform.scale", ValueType::Scale, "(1 1 1)", 0.000001, 1000000000.0, true, "", "Echelle du groupe.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.group.transform.rotate", ValueType::Rotate, "", 0.0, 360.0, true, "deg", "Rotation du groupe.", "", "", Occurrence::OptionalOnce, false},
    {"scene.objects.object.slice", ValueType::Block, "", 0.0, 0.0, false, "", "Reserve Limited objects. Accepte syntaxiquement.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.slice.axis", ValueType::Enum, "y", 0.0, 0.0, false, "", "Axe de decoupe.", "x|y|z", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.slice.min", ValueType::Float, "", 0.0, 0.0, false, "", "Borne min de decoupe.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.slice.max", ValueType::Float, "", 0.0, 0.0, false, "", "Borne max de decoupe.", "", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.slice.frame", ValueType::Enum, "object", 0.0, 0.0, false, "", "Referentiel de decoupe.", "object|world", "", Occurrence::OptionalOnce, true},
    {"scene.objects.object.slice.shape", ValueType::Enum, "slab", 0.0, 0.0, false, "", "Forme de decoupe.", "slab|circle|triangle", "", Occurrence::OptionalOnce, true},
};
// clang-format on

std::span<const Directive> all() noexcept {
    return std::span<const Directive>(kDirectives);
}

std::size_t count() noexcept {
    return sizeof(kDirectives) / sizeof(kDirectives[0]);
}

const Directive* find(std::string_view path) noexcept {
    for (const Directive& entry : kDirectives) {
        if (entry.path == path) {
            return &entry;
        }
    }
    const std::string_view::size_type dot = path.rfind('.');
    if (dot == std::string_view::npos) {
        return nullptr;
    }
    const std::string_view parent(path.data(), dot);
    const std::string_view leaf(path.data() + dot + 1, path.size() - dot - 1);
    if (leaf.empty()) {
        return nullptr;
    }
    for (const Directive& entry : kDirectives) {
        if (entry.alias.empty()) {
            continue;
        }
        if (entry.alias != leaf) {
            continue;
        }
        const std::string_view::size_type entryDot = entry.path.rfind('.');
        if (entryDot == std::string_view::npos) {
            continue;
        }
        const std::string_view entryParent(entry.path.data(), entryDot);
        if (entryParent == parent) {
            return &entry;
        }
    }
    return nullptr;
}

Status checkInt(std::string_view path, long long value) {
    const Directive* entry = find(path);
    if (entry == nullptr) {
        return Status(StatusCode::NotFound, std::string("unknown directive: ") + std::string(path), __LINE__);
    }
    if (entry->type != ValueType::Int) {
        return Status(StatusCode::InvalidArgument, std::string("not an int directive: ") + std::string(path), __LINE__);
    }
    if (entry->hasRange) {
        const double asDouble = static_cast<double>(value);
        if (asDouble < entry->minValue || asDouble > entry->maxValue) {
            return Status(StatusCode::OutOfRange, std::string("out of range: ") + std::string(path), __LINE__);
        }
    }
    return Status::ok();
}

Status checkNumber(std::string_view path, double value) {
    const Directive* entry = find(path);
    if (entry == nullptr) {
        return Status(StatusCode::NotFound, std::string("unknown directive: ") + std::string(path), __LINE__);
    }
    switch (entry->type) {
    case ValueType::Float:
    case ValueType::Vec3:
    case ValueType::Float2:
    case ValueType::Float3:
    case ValueType::Scale:
    case ValueType::Rotate:
        break;
    default:
        return Status(StatusCode::InvalidArgument, std::string("not a numeric directive: ") + std::string(path), __LINE__);
    }
    if (!std::isfinite(value)) {
        return Status(StatusCode::OutOfRange, std::string("not finite: ") + std::string(path), __LINE__);
    }
    if (entry->hasRange && (value < entry->minValue || value > entry->maxValue)) {
        return Status(StatusCode::OutOfRange, std::string("out of range: ") + std::string(path), __LINE__);
    }
    return Status::ok();
}

Status checkEnum(std::string_view path, std::string_view value) {
    const Directive* entry = find(path);
    if (entry == nullptr) {
        return Status(StatusCode::NotFound, std::string("unknown directive: ") + std::string(path), __LINE__);
    }
    if (entry->type != ValueType::Enum) {
        return Status(StatusCode::InvalidArgument, std::string("not an enum directive: ") + std::string(path), __LINE__);
    }
    std::string_view rest = entry->enumValues;
    while (!rest.empty()) {
        const std::string_view::size_type bar = rest.find('|');
        const std::string_view token = (bar == std::string_view::npos) ? rest : std::string_view(rest.data(), bar);
        if (token == value) {
            return Status::ok();
        }
        if (bar == std::string_view::npos) {
            break;
        }
        rest = std::string_view(rest.data() + bar + 1, rest.size() - bar - 1);
    }
    return Status(StatusCode::InvalidArgument, std::string("bad enum value: ") + std::string(path), __LINE__);
}

Status checkString(std::string_view path, std::string_view value) {
    const Directive* entry = find(path);
    if (entry == nullptr) {
        return Status(StatusCode::NotFound, std::string("unknown directive: ") + std::string(path), __LINE__);
    }
    if (entry->type != ValueType::String) {
        return Status(StatusCode::InvalidArgument, std::string("not a string directive: ") + std::string(path), __LINE__);
    }
    if (value.size() > 128) {
        return Status(StatusCode::LimitExceeded, std::string("string too long: ") + std::string(path), __LINE__);
    }
    return Status::ok();
}

Status checkRotate(std::string_view path, std::string_view axis, double angle) {
    const Directive* entry = find(path);
    if (entry == nullptr) {
        return Status(StatusCode::NotFound, std::string("unknown directive: ") + std::string(path), __LINE__);
    }
    if (entry->type != ValueType::Rotate) {
        return Status(StatusCode::InvalidArgument, std::string("not a rotate directive: ") + std::string(path), __LINE__);
    }
    if (axis != "x" && axis != "y" && axis != "z") {
        return Status(StatusCode::InvalidArgument, std::string("bad rotate axis: ") + std::string(path), __LINE__);
    }
    return checkNumber(path, angle);
}

namespace {

void writeEscaped(std::ostream& out, std::string_view text) {
    for (char c : text) {
        if (c == '|') {
            out << "\\|";
        } else if (c == '\n' || c == '\r') {
            out << ' ';
        } else {
            out << c;
        }
    }
}

void writeRow(std::ostream& out, const Directive& entry) {
    out << "| `";
    writeEscaped(out, entry.path);
    out << "` | ";
    writeEscaped(out, toString(entry.type));
    out << " | ";
    if (entry.defaultValue.empty()) {
        out << "--";
    } else {
        out << "`";
        writeEscaped(out, entry.defaultValue);
        out << "`";
    }
    out << " | ";
    if (entry.hasRange) {
        out << entry.minValue << " - " << entry.maxValue;
    } else {
        out << "--";
    }
    out << " | ";
    if (entry.unit.empty()) {
        out << "--";
    } else {
        writeEscaped(out, entry.unit);
    }
    out << " | ";
    writeEscaped(out, toString(entry.occurrence));
    out << " | ";
    out << (entry.reserved ? "oui" : "--");
    out << " | ";
    const bool hasAlias = !entry.alias.empty();
    const bool hasEnum = !entry.enumValues.empty();
    if (!hasAlias && !hasEnum) {
        out << "--";
    } else {
        if (hasAlias) {
            out << "alias `";
            writeEscaped(out, entry.alias);
            out << "`";
        }
        if (hasAlias && hasEnum) {
            out << "; ";
        }
        if (hasEnum) {
            out << "`";
            writeEscaped(out, entry.enumValues);
            out << "`";
        }
    }
    out << " | ";
    writeEscaped(out, entry.description);
    out << " |\n";
}

} // namespace

void writeMarkdown(std::ostream& out) {
    out << "| Chemin | Type | Defaut | Bornes | Unite | Cardinalite | Reserve | Alias / Enum | Description |\n";
    out << "|---|---|---|---|---|---|---|---|---|\n";
    for (const Directive& entry : kDirectives) {
        writeRow(out, entry);
    }
}

} // namespace rt::schema
