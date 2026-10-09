#pragma once

// Motifs proceduraux (T105-T107, *Disruptions*).
// `PatternKind` = type du bloc `material.pattern` (schema R1) : `checker`
// (T105, damier), `sine` (T107, onde/normale), `perlin` (T106, bruit).
// `checkerAlbedo()` evalue le damier en espace UV objet (option monde :
// l'appelant peut passer une UV derivee de la position monde) : taille
// reglable via `scale*frequency`, applicable a la couleur (facteur
// sombre/clair sur l'albedo courant, donc composable avec la texture
// T103). `noexcept`, sans allocation (R2/R3, hot path).
// Calque `shading/` : ne voit que `base/` (regle d'or §2.1).

#include "rt/base/Vec.hpp"

namespace rt::shading {

enum class PatternKind : unsigned char {
	None = 0,
	Checker = 1,
	Sine = 2,
	Perlin = 3,
};

[[nodiscard]] constexpr const char* toString(PatternKind kind) noexcept {
	switch (kind) {
	case PatternKind::None:
		return "none";
	case PatternKind::Checker:
		return "checker";
	case PatternKind::Sine:
		return "sine";
	case PatternKind::Perlin:
		return "perlin";
	}
	return "unknown";
}

// Convertit le nom du schema (`sine|checker|perlin`) en `PatternKind`
// (`present == false` -> `None`). Inconnu -> `None` defini.
[[nodiscard]] PatternKind patternKindFrom(const char* type, bool present) noexcept;

// Damier (T105) : `cell = floor(u*f) + floor(v*f)`, case claire si paire.
// `f = scale*frequency` sanitize (`!fini` ou `<= 0` -> 1). `albedo` = couleur
// courante (texel T103 ou albedo fichier) : claire -> inchangee, sombre ->
// `*0.15` (alternance visible sur plan comme sur sphere, sans NaN).
[[nodiscard]] Vec3 checkerAlbedo(Vec3 albedo, Vec2 uv, float scale, float frequency) noexcept;

// Bruit de Perlin 3D (T106, *Disruptions* 3-4) : gradient ameliore de Ken
// Perlin (fade/lerp/grad), table de permutation **deterministe et seedee**
// (`Perlin` construite froid depuis une graine, `splitmix64`, sans alloc
// ensuite), fractal 1-3 octaves (`fractal(octaves)`, persistance 0.5).
// `perlinAlbedo()` module la couleur (`0.5 + 0.5*n` en [0,1], 3 octaves) ;
// les hooks normale/transparence (`perlinNormal`, masque) reutilisent le
// meme `value()` (voir T107 pour l'onde). Tout est `noexcept`, sans
// allocation (R2/R3, hot path) : la table vit dans le froid (`TraceCtx`).
struct Perlin {
	// Table dupliquee (512) pour indexer sans modulo (classique).
	unsigned char perm[512]{};

	// Construit la table depuis `seed` (melange de Fisher-Yates sur
	// 0..255 avec `splitmix64`, froid). `seed` identique -> table
	// identique (DoD determinisme) ; deux graines differentes -> tables
	// (tres probablement) differentes.
	void init(unsigned long long seed) noexcept;
};

// Valeur de bruit en `p` ([-1,1] defini, NaN -> 0). `perlin` = table froide.
[[nodiscard]] float perlinValue(const Perlin& perlin, Vec3 p) noexcept;
// Fractal : somme `octaves` (1..3, borne, `oc <= 0` -> 1, `> 3` -> 3) avec
// lacunarite 2 et persistance 0.5, normalise en [-1,1].
[[nodiscard]] float perlinFractal(const Perlin& perlin, Vec3 p, int octaves) noexcept;
// Couleur marbree (T106) : `n = fractal(point*freq, 3)` en [-1,1] puis
// `k = 0.5 + 0.5*n` en [0,1] ; `out = albedo * (0.35 + 0.65*k)` (jamais
// noir pur, jamais sature seul). `freq = scale*frequency` sanitize -> 1.
[[nodiscard]] Vec3 perlinAlbedo(Vec3 albedo, Vec3 point, const Perlin& perlin, float scale,
                                float frequency) noexcept;

} // namespace rt::shading
