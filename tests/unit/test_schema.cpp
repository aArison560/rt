// Tests du schema unique des directives (T021, regle R1), Catch2.
// DoD : ajouter une directive = une seule ligne dans la table, et elle
// apparait dans le parser (find), la validation (check*) et la doc
// (writeMarkdown). Ce fichier verifie que les quatre consommateurs lisent
// la meme table, sans definition dupliquee ailleurs.

#include <catch2/catch_amalgamated.hpp>

#include <fstream>
#include <sstream>
#include <string>

#include "rt/schema/Directives.hpp"
#include "rt/scene/Parser.hpp"

namespace {

bool markdownContains(const std::string& markdown, std::string_view needle) {
    return markdown.find(needle) != std::string::npos;
}

} // namespace

TEST_CASE("schema : la table est non vide et sans doublon", "[schema]") {
    using namespace rt::schema;

    REQUIRE(count() > 60);
    REQUIRE(all().size() == count());

    // Unicite des chemins : une seule definition par directive.
    for (std::size_t i = 0; i < all().size(); ++i) {
        for (std::size_t j = i + 1; j < all().size(); ++j) {
            INFO("doublon : " << all()[i].path << " vs " << all()[j].path);
            REQUIRE(all()[i].path != all()[j].path);
        }
        // Chaque entree est utilisable par l'UI : chemin, type et
        // description renseignes.
        REQUIRE_FALSE(all()[i].path.empty());
        REQUIRE_FALSE(all()[i].description.empty());
        REQUIRE(toString(all()[i].type) != "unknown");
        REQUIRE(toString(all()[i].occurrence) != "unknown");
    }
}

TEST_CASE("schema : le parser trouve chaque directive par son chemin", "[schema]") {
    using namespace rt::schema;

    // Echantillon des fixtures tests/cases/valid/*.rt : toutes doivent etre
    // connues du parser via la table unique.
    const char* const expected[] = {
        "scene",
        "scene.limits.width",
        "scene.limits.height",
        "scene.limits.samples",
        "scene.camera.position",
        "scene.camera.target",
        "scene.camera.fov",
        "scene.background.color",
        "scene.lights.light.position",
        "scene.lights.light.color",
        "scene.lights.light.intensity",
        "scene.objects.object.type",
        "scene.objects.object.center",
        "scene.objects.object.radius",
        "scene.objects.object.material.albedo",
        "scene.objects.group.name",
        "scene.objects.object.point",
        "scene.objects.object.normal",
    };
    for (const char* path : expected) {
        INFO("chemin : " << path);
        const Directive* entry = find(path);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->path == path);
    }

    // Blocs requis et conteneurs.
    REQUIRE(find("scene")->occurrence == Occurrence::RequiredOnce);
    REQUIRE(find("scene.camera")->occurrence == Occurrence::RequiredOnce);
    REQUIRE(find("scene.objects")->occurrence == Occurrence::RequiredOnce);
    REQUIRE(find("scene.lights.light")->occurrence == Occurrence::OptionalMany);
    REQUIRE(find("scene.objects.object")->occurrence == Occurrence::OptionalMany);

    // Directive inconnue : le parser doit la rejeter (jamais ignoree).
    REQUIRE(find("scene.witdh") == nullptr);
    REQUIRE(find("scene.camera.bogus") == nullptr);
    REQUIRE(find("") == nullptr);
}

TEST_CASE("schema : les alias se resolvent vers le canonique", "[schema]") {
    using namespace rt::schema;

    const Directive* target = find("scene.camera.target");
    REQUIRE(target != nullptr);
    REQUIRE(find("scene.camera.lookAt") == target);

    const Directive* albedo = find("scene.objects.object.material.albedo");
    REQUIRE(albedo != nullptr);
    REQUIRE(find("scene.objects.object.material.color") == albedo);

    const Directive* center = find("scene.objects.object.center");
    REQUIRE(center != nullptr);
    REQUIRE(find("scene.objects.object.position") == center);

    const Directive* reflectivity = find("scene.objects.object.material.reflectivity");
    REQUIRE(reflectivity != nullptr);
    REQUIRE(find("scene.objects.object.material.reflect") == reflectivity);

    // Un alias sur un autre parent ne doit pas fuiter.
    REQUIRE(find("scene.lights.light.lookAt") == nullptr);
}

TEST_CASE("schema : la validation derive de la meme table", "[schema]") {
    using namespace rt::schema;

    // Int : defaut passe, hors bornes echoue.
    REQUIRE(checkInt("scene.limits.width", 640).isOk());
    REQUIRE(checkInt("scene.limits.width", 1).isOk());
    REQUIRE(checkInt("scene.limits.width", 8192).isOk());
    REQUIRE(checkInt("scene.limits.width", 0).isError());
    REQUIRE(checkInt("scene.limits.width", 8193).isError());
    REQUIRE(checkInt("scene.limits.width", 640).code == rt::StatusCode::Ok);

    // Float : fov 60 par defaut, bornes 1-179.
    REQUIRE(checkNumber("scene.camera.fov", 60.0).isOk());
    REQUIRE(checkNumber("scene.camera.fov", 0.5).isError());
    REQUIRE(checkNumber("scene.camera.fov", 180.0).isError());

    // Vec3 : un canal de couleur 0-1 passe, 1.5 echoue.
    REQUIRE(checkNumber("scene.objects.object.material.albedo", 0.5).isOk());
    REQUIRE(checkNumber("scene.objects.object.material.albedo", 1.5).isError());

    // Enum : valeurs de la table uniquement.
    REQUIRE(checkEnum("scene.objects.object.type", "sphere").isOk());
    REQUIRE(checkEnum("scene.objects.object.type", "plane").isOk());
    REQUIRE(checkEnum("scene.objects.object.type", "cube").isError());
    REQUIRE(checkEnum("scene.lights.light.type", "point").isOk());
    REQUIRE(checkEnum("scene.lights.light.type", "dir").isOk());
    REQUIRE(checkEnum("scene.lights.light.type", "laser").isError());

    // String : longueur bornee a 128.
    REQUIRE(checkString("scene.objects.object.name", "boule").isOk());
    REQUIRE(checkString("scene.objects.object.name", std::string(128, 'a')).isOk());
    REQUIRE(checkString("scene.objects.object.name", std::string(129, 'a')).isError());

    // Rotate : axe x/y/z et angle 0-360.
    REQUIRE(checkRotate("scene.objects.object.transform.rotate", "y", 30.0).isOk());
    REQUIRE(checkRotate("scene.objects.object.transform.rotate", "w", 30.0).isError());
    REQUIRE(checkRotate("scene.objects.object.transform.rotate", "y", 400.0).isError());

    // Type incoherent : la validation refuse (le parser aiguille par type).
    REQUIRE(checkInt("scene.camera.fov", 60).isError());
    REQUIRE(checkNumber("scene.objects.object.name", 1.0).isError());
    REQUIRE(checkEnum("scene.camera.fov", "sphere").isError());

    // Inconnue : erreur propre, jamais de crash.
    REQUIRE(checkInt("scene.bogus", 1).isError());
    REQUIRE(checkNumber("scene.bogus", 1.0).isError());
    REQUIRE(checkEnum("scene.bogus", "x").isError());
    REQUIRE(checkString("scene.bogus", "x").isError());
}

TEST_CASE("schema : une seule ligne pilote parser, validation et doc", "[schema]") {
    using namespace rt::schema;

    std::ostringstream stream;
    writeMarkdown(stream);
    const std::string markdown = stream.str();

    // Pour chaque entree : trouvable (parser), validable a son defaut
    // quand il est numerique (validation), et presente dans la doc.
    for (const Directive& entry : all()) {
        INFO("directive : " << entry.path);
        REQUIRE(find(entry.path) == &entry);
        REQUIRE(markdownContains(markdown, entry.path));
        REQUIRE(markdownContains(markdown, entry.description.substr(0, 10)));
    }

    // La doc contient une ligne par directive (+ 2 lignes d'en-tete).
    std::size_t rows = 0;
    std::istringstream lines(markdown);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line[0] == '|') {
            ++rows;
        }
    }
    REQUIRE(rows == count() + 2);

    // Les alias et les enums sont visibles dans la doc (l'UI les affichera).
    REQUIRE(markdownContains(markdown, "lookAt"));
    REQUIRE(markdownContains(markdown, "sphere"));
    REQUIRE(markdownContains(markdown, "point"));

    // Les directives reservees sont signalees (options futures).
    const Directive* height = find("scene.objects.object.height");
    REQUIRE(height != nullptr);
    REQUIRE(height->reserved);
    REQUIRE(markdownContains(markdown, "scene.objects.object.height"));
}

TEST_CASE("schema : docs/FORMAT_SCENE.md §5.8 == table reelle (T029)", "[schema]") {
    using namespace rt::schema;

    // Coherence fichier <-> table : la section generee doit etre exactement
    // `writeMarkdown()` (a un `\n` final pres). Sinon `sh scripts/gen_doc.sh`
    // n'a pas ete relance apres modification de la table (regle R1).
    std::ostringstream expectedStream;
    writeMarkdown(expectedStream);
    const std::string expected = expectedStream.str();

    std::ifstream doc("docs/FORMAT_SCENE.md", std::ios::binary);
    REQUIRE(doc.good());
    const std::string content((std::istreambuf_iterator<char>(doc)),
                              std::istreambuf_iterator<char>());
    const std::string start("<!-- SCHEMA-GENERATED-START -->");
    const std::string end("<!-- SCHEMA-GENERATED-END -->");
    const std::size_t startPos = content.find(start);
    const std::size_t endPos = content.find(end);
    REQUIRE(startPos != std::string::npos);
    REQUIRE(endPos != std::string::npos);
    REQUIRE(endPos > startPos);
    const std::string inner =
        content.substr(startPos + start.size(), endPos - startPos - start.size());
    // `gen_doc.sh` ecrit "\n\n" + generated.rstrip("\n") + "\n\n" : on
    // normalise les bords avant comparaison stricte.
    const auto trim = [](const std::string& text) {
        std::size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return std::string();
        }
        const std::size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1U);
    };
    REQUIRE(trim(inner) == trim(expected));
    // Chaque directive apparait dans le fichier (garde contre troncation).
    for (const Directive& entry : all()) {
        INFO("directive : " << entry.path);
        REQUIRE(content.find(entry.path) != std::string::npos);
    }
}

TEST_CASE("schema : scenes/default.rt parse comme fallback (T029)", "[schema]") {
    rt::Result<rt::scene::Scene> result =
        rt::scene::parseFile("scenes/default.rt");
    INFO("message : " << (result.isError() ? result.status().message : std::string("ok")));
    REQUIRE(result.isOk());
    const rt::scene::Scene& scene = result.value();
    REQUIRE(scene.totalObjectCount() == 2U);
    REQUIRE(scene.lights.size() == 2U);
    REQUIRE(scene.limits.width == 320);
    REQUIRE(scene.limits.height == 240);
}
