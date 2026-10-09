# RT — Guide d'implémentation des options

> Un document **par option de la fiche** : ce qui rapporte des points, comment l'implémenter,
> **comment le prouver** pendant la soutenance, et les pièges à éviter.
>
> Rappel impératif (sujet) : **les options ne sont évaluées QUE si la partie obligatoire est
> parfaite.** Ce guide n'est donc à ouvrir qu'après la porte `J2` de
> [PLAN_TRAVAIL.md](PLAN_TRAVAIL.md).

---

## Sommaire

0. [Méthode : la règle des preuves](#0-méthode-la-règle-des-preuves)
1. [Gain de points vs effort (priorisation)](#1-gain-de-points-vs-effort-priorisation)
2. [Fichiers de scène et ambiance](#2-fichiers-de-scène-et-ambiance)
3. [Lumières](#3-lumières)
4. [Réflexion, transparence, ombres](#4-réflexion-transparence-ombres)
5. [Textures et perturbations](#5-textures-et-perturbations)
6. [Objets limités (slicing)](#6-objets-limités-slicing)
7. [Objets composés, négatifs et natifs](#7-objets-composés-négatifs-et-natifs)
8. [Effets visuels usuels](#8-effets-visuels-usuels)
9. [Effets techniques et performance](#9-effets-techniques-et-performance)
10. [Environnement (interface et automatisation)](#10-environnement-interface-et-automatisation)
11. [Options exotiques](#11-options-exotiques)

---

## 0. Méthode : la règle des preuves

### 0.1 Trois exigences de preuve (fiche + sujet)

1. **Preuve par le fichier de scène** : l'option est pilotée par un fichier `scenes/opt_*.rt`.
   > « Get prepared with multiple configured scenes, ready to be calculated. »
2. **Preuve par le direct** : pour toute option « modifiable », le correcteur doit pouvoir
   **changer la valeur et voir l'effet** (fichier ou interface), **sans recompiler**.
   > « Manipulations in direct of your scenes, using your own tools. »
3. **Jamais d'image pré-rendue** comme unique preuve :
   > « Already created images (jpeg, png …) are **not allowed** to prove options. »

### 0.2 Convention de nommage

```
scenes/
├── 01_obligatoire_4objets.rt          # figure VI.1
├── 02_obligatoire_oeil_deplace.rt     # figure VI.2  (même scène, caméra seule changée)
├── 03_obligatoire_mix_ombres.rt       # figure VI.3
├── opt_ambiance.rt
├── opt_reflexion_transparence.rt
├── opt_textures.rt
├── opt_disruptions.rt
├── opt_limited_objects.rt
├── opt_lumiere_spot.rt
├── opt_composes.rt
├── opt_negatif.rt
├── opt_natifs.rt
├── opt_effets_visuels.rt
└── ...
```

### 0.3 Fiche de suivi (à remplir au fur et à mesure)

| Item fiche | Points max | Scène de preuve | Démontré en répétition ? |
|------------|-----------|-----------------|--------------------------|
| Scene files | 1 | `01_…rt` | ☐ |
| … | … | … | ☐ |

---

## 1. Gain de points vs effort (priorisation)

Ordre recommandé (ROI décroissant). Les détails sont dans les sections suivantes.

| # | Option | Points | Effort estimé | Section |
|---|--------|--------|---------------|---------|
| 1 | **Scene files** | 1 (binaire) | ✔ déjà fait | §2 |
| 2 | **Ambiance light + ++** | 2 (binaires) | 1–2 h | §2 |
| 3 | **Parallel light** | 1 (binaire) | 2 h (déjà en code) | §3 |
| 4 | **Reflection & transparency** | 5 | 4–6 h (pilotabilité) | §4 |
| 5 | **Technical — multithreading, screenshot, vitesse** | 3 | ✔ déjà fait + mesures | §9 |
| 6 | **Textures** | 5 | 1–1,5 j | §5 |
| 7 | **File ++** | 1 (binaire) | 1 jour | §2 |
| 8 | **Usual visual effects — AA, sépia, cartoon** | 3–5 | 1–2 j | §8 |
| 9 | **Direct light (spot)** | 1 (binaire) | 0,5 j | §3 |
| 10 | **Environment 1-3** | 3 | 1–2 j | §10 |
| 11 | **Disruptions** | 5 | 2–3 j | §5 |
| 12 | **Composed + Negative + Native** | 3 binaires | 3–4 j | §7 |
| 13 | **Shadows & transparency** | 1 (binaire) | 0,5 j | §4 |
| 14 | **Limited objects** | 5 | 3–4 j | §6 |
| 15 | **More texture applications** | 5 | 3 j | §5 |
| 16 | **Environment 4-5** | 2 | 1 j | §10 |
| 17 | **Exotic / In bulk / Möbius / Caustics** | jusqu'à 12+ | 5 j+ | §11 |

> **Stratégie** : viser **~30 points d'options solides** avant de monter vers les cases à
> effort élevé. Chaque case binaire rapide (1 h de travail) vaut autant que 1 point d'une grille
> de 5 — empiler d'abord les binaires faciles.

---

## 2. Fichiers de scène et ambiance

### 2.1 Scene files — ☐ Oui/Non

**Critère** : « There is a description file for the scene. »

- **État** ✔ : `scenes/*.rt` + `SceneParser`.
- **Preuve** : `./rt scenes/01_obligatoire_4objets.rt` → le correcteur modifie un nombre, relance.
- **Piège** : le fichier doit être **dans le dépôt** et se lancer sans chemin absolu.

### 2.2 File ++ — ☐ Oui/Non

**Critère** : XML **ou** « proper structure or hierarchy » — pas « une information par ligne ».

- **État** ✖ : le format actuel est *une directive par ligne*.
- **Implémentation** : voir [ARCHITECTURE.md §5.2](ARCHITECTURE.md) (lexer récursif sur blocs `{}`).
- **Étapes** :
  1. Lexer : tokens `{ } ident nombre chaîne commentaire`.
  2. Parser récursif : `parseBlock("scene") → parseBlock("object"|"light"|"camera"…)`.
  3. Table de dispatch par type de bloc + nom d'attribut.
  4. **Détection auto** du format (legacy vs nouveau) pour ne pas casser les scènes existantes.
  5. Migration des scènes existantes.
- **Effort** : ~1 jour. **Priorité haute** (bloque l'écriture propre de toutes les autres options).
- **Preuve** : ouvrir le fichier devant le correcteur → hiérarchie visible, objets nommés.

### 2.3 Ambiance light — ☐ Oui/Non

**Critère** : « No object is never really in the dark. »

- **État** ✔ probablement (directive `A`, `AmbientLight`, `ambientMultiplier`).
- **Implémentation** :
  ```
  ambientColor = ambientGlobal * material.ambient * material.color
  finalColor  += ambientColor          // AVANT toute contribution de lumière
  ```
- **Vérification obligatoire** : orienter un objet **dos** à toutes les lumières → il doit
  rester visible (dégradé sombre mais **pas noir pur**).
- **Preuve** : scène où tous les points lumineux sont derrière les objets.

> Implémenté (T033) : `include/rt/shading/Material.hpp` — `shadeLambert()` (albedo,
> ambient, diffuse + 1 ponctuelle + ambiante globale, saturé [0,1], NaN → 0, gamma
> en sortie via `Framebuffer::present()`). Dos à la lumière → plancher ambiant > 0 ;
> doubler l'intensité double la part diffuse (testé). Le `Renderer` (T032, miss →
> fond) rend déjà une image non noire (fond + ambiance) ; P4 branchera `shadeLambert()`
> sur chaque intersection sans changer la boucle. Tests : `tests/unit/test_material.cpp`
> (Lambert, dégénérés, DoD luminosité minimale > 0 et `isfinite` sur `default.rt`).

### 2.4 Ambiance ++ — ☐ Oui/Non

**Critère** : l'ambiance se pilote **depuis le fichier de configuration**.

- **Implémentation** : bloc `ambient { color r g b  intensity x }` dans le nouveau format,
  lu par le parser → `scene.setAmbientMultiplier()`.
- **Effort** : 1–2 h (une fois le format structuré fait).
- **Preuve** : le correcteur change `intensity 0.05 → 0.5` et relance → la scène s'éclaircit.

> Implémenté (T054) : `scene.ambient { color 0-1, intensity 0-10 }` (schéma
> R1, parser T023, validation T024) → `shading::AmbientParams` via `render/`
> (`toAmbientParams`) + `shadeLambert()` (plancher `albedo*ambiant > 0`,
> dos à la lumière inclus). 0 lumière = ambiant seul (valide). Tests :
> `tests/unit/test_ambient.cpp` (DoD : min display > 0 éclairée et sans
> lumière, `0.5 → 2.0` visible au centre, borne 99 rejetée).

---

## 3. Lumières

### 3.1 Parallel light — ☐ Oui/Non

**Critère** : une lumière **parallèle** selon une **direction précise** (≠ spot vers un point).

- **État** : `DirectionalLight` existe ✔, directive `directional` ✔.
- **Implémentation** : `L = -normalize(direction)` constant pour tous les points ; **pas
  d'atténuation** par distance.
- **Preuve** : scène `opt_lumiere_directionnelle.rt` — ombres **parallèles** (toutes de même
  longueur/direction) vs une `PointLight` (ombres divergentes). Montrer les deux côte à côte.

> Implémenté (T055) : `lighting::toLightDir()` (`L = -norm(dir)`, nulle/NaN
> → ignorée, `noexcept`, R2/R3) + `shading::shadeLambertDirectional()` (même
> ambiant que ponctuelle, `NdotL` constant, dos → ambiant seul) branchés dans
> `render/` (`collectDirectionalLights`, ordre fichier, `tMax` infini pour
> ombres parallèles, diffus `matNoAmb` + speculaire `specularTerm` T053).
> Preuve : `scenes/opt_parallel.rt` (soleil + sol + 2 sphères, ombres
> parallèles) ; test comparatif `tests/unit/test_directional.cpp` (2 sphères
> symétriques : ponctuelle latérale droite +8.7 vs directionnelle −1.0,
> ombres avec/sans occultrice 3.3 vs 164, `opt_parallel.rt` non-fond >5%).

### 3.2 Direct light — ☑ Oui (T058)

**Critère** : « We're blinded by light spot facing us. »

> Implémenté (T058) : `lighting::spotAxis()` (`normalize(target-position)`)
> + `spotConeFactor()` (cône `cosAngle vs cos(angle)`, pénombre `smoothstep`
> 0.02, hors cône = 0) branchés dans `render::shadeDirect()` (diffus +
> speculaire pondérés par `att * cone * vis`, `noexcept` R2/R3) +
> `spotBlindingFactor()` (observateur dans le cône ET rayon vers la source
> dans 8° → `render::traceRay()` sur les manqués mélange fond + source
> saturée, centre = blanc). `SpotLightParams` POD (R3) via `collectSpotLights`
> (ordre fichier). Tests : `tests/unit/test_t058.cpp` (cône dedans/dehors +
> dégénérés, face vs opposé : centre > 0.9 vs < 0.3, scènes de preuve rendent).
> Preuve : `scenes/opt_direct.rt` (spot (0 1 -2) → caméra (0 1 5), 30°,
> intensité 5, centre saturé 254 vs côtés sombres ; caméra à l'opposé =
> sombre).

- **État** ✖ (aucun `spot`).
- **Implémentation** :
  ```cpp
  // spotlight : cone entre (P - lightPos) et targetDir
  Vec3  toFrag   = normalize(P - lightPos);
  float cosAngle = dot(toFrag, normalize(target - lightPos));
  float cutoff   = cos(radians(angle));            // demi-angle du cône
  if (cosAngle < cutoff) contribution = 0;
  else  contribution *= smoothstep(cutoff, cutoff + softness, cosAngle);
  ```
  + **la intensité augmente fortement quand la caméra est dans l'axe** (composante spéculaire
  vers l'observateur → effet « aveuglé »).
- **Effort** : 0,5 j. **Étape clé** : nouvelle classe `SpotLight : ALight` + directive
  `light spot { position target angle color intensity }`.
- **⚠ Point ouvert O4** : le libellé est ambigu. **Préparer les deux démos** :
  - (a) spot orienté face à la caméra → image surexposée au centre ;
  - (b) lumière *headlight* attachée à la caméra → tout le premier plan est éclairé en face.
- **Preuve** : rotation de la caméra vers le spot → l'image blanchit.

### 3.3 Multi-spot (obligatoire M7) + soft shadows (item *In bulk*)

- **Multi-spot** : somme des contributions (voir [ARCHITECTURE.md §4.4](ARCHITECTURE.md)).
- **Spot non ponctuel / ombres sans netteté** (*In bulk* #4) : `AreaLight` existe ✔
  (`samplePoint()`) — il faut l'**exposer au parseur** et l'utiliser pour échantillonner :
  ```
  visibility = (1/N) Σ  anyHit(P + εN → samplePoint_i)
  ```
  N = 8–32 échantillons → l'ombre devient **douce** (bord flou).
- **Preuve** : même scène, `size 0.0` (ombre dure) vs `size 2.0` (ombre floue).

---

## 4. Réflexion, transparence, ombres

### 4.1 Reflection and transparency — 0…5 points

| # | Sous-critère | État | Action |
|---|--------------|------|--------|
| 1 | Réflexion fonctionne (miroir) | ✔ (`Vec3::reflect`, récursion) | Scène miroir |
| 2 | **% de réflexion modifiable** | ✔ (`reflect` dans `material`) | Scène avec `reflect 0.0 / 0.3 / 0.9` |
| 3 | Transparence fonctionne (on voit à travers) | ✔ côté renderer | **Ajouter au parseur** |
| 4 | **Indice de réfraction** fonctionne (formule de Descartes) | ✔ (`Vec3::refract`) | **Ajouter `ior` au parseur** |
| 5 | **% de transparence modifiable** | ✔ (`setTransparency`) | **Ajouter au parseur** |

> T050 : le bloc `material` est désormais complet côté données — `albedo`,
> `ambient`, `diffuse`, `specular`, `shininess`, `reflectivity`,
> `transparency`, `ior`, `texture`, `pattern` sont parsés (T023), validés
> (T024, `ior > 1` si transparence) et recopiés dans
> `shading::MaterialParams` (POD, R3) par `render/` — chaque champ est
> pilotable depuis le fichier et testé par champ
> (`tests/unit/test_material.cpp`, `[t050]`). L'effet visuel arrive avec sa
> tâche : spéculaire T053, réflexion T056, réfraction T057, textures T102,
> patterns T105.

- **Implémentation restante (≈ 4 h)** : étendre le bloc `material` :
  ```
  material { ... reflect 0.2  transparency 0.7  ior 1.5 }
  ```
  et propaguer jusqu'à `Material`.
- **Formules à pouvoir expliquer** :
  - Snell/Descartes : `n₁ sin θ₁ = n₂ sin θ₂`
  - Réflexion totale interne si `sin θ₂ > 1` → **le rayon rebondit**
  - Fresnel Schlick : `F = F₀ + (1 − F₀)(1 − cos θ)⁵` pour équilibrer réflexion/réfraction
- **Preuve** : `opt_reflexion_transparence.rt` avec, côte à côte :
  miroir (`reflect 1`), verre (`transparency 0.9, ior 1.5`), eau (`ior 1.33`), demi-miroir
  (`reflect 0.3`). Le correcteur **modifie `ior`** et relance → le décalage de l'image change.
- **Pièges** :
  - Oublier d'inverser la normale à l'entrée/sortie de l'objet → verre « plein » au lieu de creux.
  - Oublier le *bias* → acné et *self-intersection*.
  - Récursion non bornée → explosion de temps (déjà : `maxRecursionDepth`).

> Implémenté (T056, sous-critères 1–2) : réflexion pilotée par
> `material { reflectivity/reflect 0..1 }` (schéma R1, validé T024) +
> `limits { max_depth }` (défaut 4, 0..16 schéma / 0..32 moteur) via
> `render::traceRay()` (`out = direct*(1-R) + réfléchi*R`, `R=0` = mat
> identique au sans-miroir, `R=1` = miroir pur). Tests :
> `tests/unit/test_reflection.cpp` (4 cas, DoD bornes + profondeur bornée).
>
> Implémenté (T057, sous-critères 3–5) : réfraction pilotée par
> `material { transparency 0..1 + ior 1..3 }` (schéma R1, validé T024 dont
> `transparency > 0` exige `ior > 1`) via `shading::refractDir()` (Descartes
> `n1*sin(t1) = n2*sin(t2)`, `eta = frontFace ? 1/ior : ior`, `ior = 1` = sans
> déviation, TIR = sentinelle nulle repliée sur le miroir) + `traceRay()`
> (`out = base*(1-T) + transmis*T`, `T=0` = opaque identique, `T=1` = transmis
> pur). Tests : `tests/unit/test_refraction.cpp` (6 cas, DoD `ior` + formule
> commentée). Preuve : `scenes/opt_glass.rt` (verre `transparency 0.9 ior 1.5`
> + opaque, `sh scripts/render_all.sh` → `docs/preuves/opt_glass.png`).

### 4.2 Shadows and transparency — ☑ Oui (T058)

**Critère** : l'ombre est **plus ou moins assombrie** selon la transparence de l'objet.

> Implémenté (T058) : `render::shadowTransmittance()` remplace le test binaire
> (`isOccluded`) — `transmit = 1`, pour chaque occulteur sur le trajet :
> `trans_eff = clamp(transparency,0,1) * clamp(1.5/ior,0.5,1)` (opaque = 0
> immédiat, octet-identique à T052 ; verre `0.8/1.5` ≈ 0.8, dense `0.8/3.0` =
> 0.4 ; 8 occulteurs max, `P+N*eps`, `tMax = dist-eps`, `noexcept` R2/R3),
> `contribution *= transmit` (diffus + speculaire, ponctuelles +
> directionnelles + spots). Note : le guide proposait `visibility *=
> (1-transparency)` (inversé : opaque = 1 = plein jour) — implémenté
> `transmit *= transparency_eff` (opaque = 0 = ombre, correct). Tests :
> `tests/unit/test_t058.cpp` (même sphère opaque vs `0.8/1.5` : moyenne ombre
> +0.03, `1.5` vs `3.0` : +0.005) + ajustement `test_refraction.cpp` (face
> arrière éclaircie +0.12, marge 0.05 → 0.15 documentée). Preuve :
> `scenes/opt_transparent_shadow.rt` (opaque gauche/ombre noire + verre
> droite/ombre claire, même spot latéral).

- **Implémentation** : passer d'un test binaire à une **visibilité continue** :
  ```
  visibility = 1.0
  pour chaque transparence sur le trajet du rayon d'ombre :
      visibility *= (1 - material.transparency)     // ou moyenne des échantillons
  contribution *= visibility
  ```
  Version simple (acceptable) : si l'objet touché dans l'ombre est transparent, atténuer
  partiellement au lieu de couper complètement.
- **Effort** : 0,5 j.
- **Preuve** : sphère opaque → ombre noire ; la **même** sphère avec `transparency 0.8` →
  l'ombre s'éclaircit ; une sphère de verre → ombre nettement plus claire avec un halo.

---

## 5. Textures et perturbations

### 5.1 Textures — 0…5 points

| # | Sous-critère | État | Action |
|---|--------------|------|--------|
| 1 | Texture sur **≥ 1 des 4 objets** | △ (`Texture` PNG/JPEG ✔) | Directive `texture` au parseur + UV sphère/plan |
| 2 | Texture sur **les 4 objets** | △ | UV cylindre/cône (azimut + hauteur) |
| 3 | **Étirer** une texture | ✖ | `scale u v` dans le mapping |
| 4 | **Décaler** une texture | ✖ | `offset u v` |
| 5 | **Autre lib que MiniLibX/XPM** (jpeg, png) | ✔ (libpng/libjpeg) | Preuve : charger un `.png` **et** un `.jpg` |

**Implémentation :**

```cpp
// mapping générique
u = u * texScaleU + texOffsetU;
v = v * texScaleV + texOffsetV;
color = material.texture->sample(fract(u), fract(v));   // répétition tiling
```

- **Étapes** : (1) générer `(u,v)` pour les 4 primitives, (2) brancher `sample()` dans le
  shading **en remplaçant** `material.color`, (3) ajouter `scale`/`offset`, (4) directive
  `texture "textures/chess.png" { scale 4 4  offset 0.1 0.0 }`.
- **Attention** : `Material::setTexture(Texture*)` prend un **pointeur brut** → prévoir une
  **cache de textures** (`map<path, shared_ptr<Texture>>`) pour éviter de recharger et de
  fuiter (règle : pas de fuite mémoire).
- **Effort** : 1–1,5 j.
- **Preuve** : `opt_textures.rt` avec les 4 primitives texturées, puis changer `scale 1 → 8`
  devant le correcteur.
- **État (T102)** : `include/rt/io/Texture.hpp` + `src/io/Texture.cpp` — `TextureCache`
  (`map` + `shared_ptr`, RAII, `clear()`), PNG via libpng + JPEG via libjpeg (magie, pas
  l'extension), absent → `IoError` avec chemin complet, `maxBytes` → `LimitExceeded` ;
  `textures/checker.png` (64×64) + `textures/gradient.jpg` (64×64) ; tests
  `tests/unit/test_texture.cpp` (`[t102]`, 3 cas : charge, partage, erreurs).
- **État (T103)** : `sampleTexture()` (pavage `fract` + plus proche, `noexcept` R3) + `render/`
  (cache froid, `worldTex` aligné sur `worldMats`, `mat.albedo = texel`, repli albedo si
  `nullptr`) ; `scenes/opt_textures4.rt` (plan/cylindre = damier PNG, sphère/cône = dégradé
  JPEG) ; tests `tests/unit/test_textures4.cpp` (`[t103]` : sampler + damier 4 objets +
  avec/sans texture >5% pixels).

### 5.2 More texture applications — 0…5 points

| # | Sous-critère | Implémentation | Effort |
|---|--------------|----------------|--------|
| 1 | **Projection / mappage grossier** | choix du mode : planaire, sphérique, cylindrique (`texture { projection cylindrical }`) | 0,5 j |
| 2 | Texture utilisée pour sa **transparence** | canal alpha : `alpha = tex(u,v).a` → `visibility *= alpha` (déjà partiels dans les shaders) | 0,5 j |
| 3 | **Bump mapping** | perturber la normale : `N' = normalize(N + s·∇texture)` en espace tangent | 1 j |
| 4 | Texture qui modifie [la couleur] **à certains endroits** | multiplier l'albedo localement (masque) | 0,5 j |
| 5 | Texture qui **limite/tranche** un objet | `alpha < 0.5 → discard` sur l'intersection (transparence binaire par texture) | 0,5 j |
| 6 | Objet semi-transparent servant de **diapositive** devant un autre | combiner 2 + 4 : plan `transparency 0.6` + texture translucide devant une sphère | 0,5 j |

> Note : la grille compte **6** propositions pour **5** points → **5 suffisent**.

### 5.3 Disruptions — 0…5 points (perturbations procédurales)

| # | Sous-critère | Implémentation | Points |
|---|--------------|----------------|--------|
| 1 | **Perturbation de normale** avec `sin` → effet vague | `N' = normalize(N + A·sin(k·P + φ))` ; visuellement : onde sur le plan | 1 |
| 2 | **Damier** (checkerboard) | `checker = (floor(u·n) + floor(v·n)) % 2` → mélange 2 couleurs | 1 |
| 3 | **Algorithme plus compliqué** | tramage/voronoi/marbre (`sin(u·40 + noise)`), ou damier **3D** (selon x,y,z) | 1 |
| 4 | **Bruit de Perlin** | bruit gradient 3D → couleur **et/ou** normale | **2** (sauf si c'est le seul : alors 1) |

- **Implémentation** : hook unique dans le shading :
  ```cpp
  Pattern p = material.pattern;              // none | sine | checker | voronoi | perlin
  Vec2   uv = mapUV(object, hitPoint);
  color     *= p.colorAt(uv, hitPoint);      // couleurs
  normal     = p.perturbNormal(normal, uv);  // normales
  ```
  + directive `pattern { type checker  scale 4  colorA … colorB … }`.
- **Piège important** : **Perlin seul ne vaut que 1 point au lieu de 2** — implémenter
  **Perlin + damier + sinus** pour prendre 4 points.
- **Effort** : 2–3 j pour 4 points.
- **Preuve** : `opt_disruptions.rt` avec 4 objets côte à côte montrant chacun un pattern.

---

## 6. Objets limités (slicing)

**Grille 0…5.** La plus **structurante** des options : elle touche toutes les primitives.

| # | Sous-critère | Implémentation clé |
|---|--------------|--------------------|
| 1 | Trancher sur **x, y, z** | demi-espace : `if (P.x < min) pas d'intersection` |
| 2 | Trancher en **coordonnées objet ou monde** | tester dans le **repère de l'objet** (`M⁻¹·P`) **ou** en monde |
| 3 | **Rotations/translations toujours valables** après tranché | conséquence directe de (2) : la coupe suit l'objet |
| 4 | Effet **propre à chaque objet** | stocker le `slice` **dans l'objet**, jamais en global |
| 5 | Trancher **autrement que par les axes** (triangle, disque) | masque de forme : disque `‖(u,v)‖ ≤ r`, triangle par demi-plans, carré = 2 coupes axes |

**Implémentation recommandée** (décorateur, une seule fois pour les 4 primitives) :

```cpp
// dans AObject::intersect(), après avoir trouvé t
if (slice.enabled) {
    Vec3 local = worldToLocal(hitPoint);          // ou hitPoint si frame == world
    if (!slice.contains(local)) chercher la racine suivante;   // ← important
}
```

> ⚠ **Piège n°1** : ne pas **abandonner** à la première racine rejetée — un cylindre tranché
> peut avoir sa 2ᵉ racine (la face arrière) valide. Il faut **parcourir les racines**.
>
> ⚠ **Piège n°2** : la coupe doit être évaluée **en espace objet** pour que le sous-critère 3
> passe (translation/rotation de l'objet → la coupe bouge avec lui).

**Effort** : 3–4 j pour 5 points.
**Preuve** : `opt_limited_objects.rt` avec 6 sphères/cylindres : coupés sur x / y / z, un coupé
selon **son propre axe**, un **déplacé après coup** (montrer qu'il suit), un **triangle**,
un **disque**.

---

## 7. Objets composés, négatifs et natifs

### 7.1 Composed elements — ☐ Oui/Non

**Critère** : définir un élément composé d'objets simples, **réutilisable à plusieurs
positions/orientations différentes**.

- **Implémentation** :
  ```cpp
  class Group : public AObject {
      std::vector<std::shared_ptr<AObject>> children;
      Transform localTransform;
      // intersect = min des t des enfants (dans le repère du groupe)
  };
  ```
  + bloc dans le format :
  ```
  group "cube" { ... 6 x object plane { ... } ... }
  object group "cube" { transform { translate 0 0 0 } }
  object group "cube" { transform { translate 4 0 0  rotate axis y angle 45 } }
  ```
- **Effort** : 1–1,5 j (le décorateur réutilise le même mécanisme que le slicing).
- **Preuve** : **le même cube posé 3 fois**, à 3 endroits/orientations différentes. Échouer si
  « c'est recopié en dur » (le critère insiste : « if it's not the case, the composed element is
  useless »).
- **État (T101)** : `scenes/opt_group.rt` — composition « verre » (cône + cylindre + sphère)
  instanciée 2 fois (`verre_gauche` à x=-2.2, `verre_droite` à x=+2.2, même patron, `transform`
  parent) ; test `tests/unit/test_group.cpp` (`[t101]` : même définition ≥2 fois + partage
  `shared_ptr`, 2 instances à 2 endroits) ; `./rt scenes/opt_group.rt --out docs/preuves/opt_group.png`.

### 7.2 Negative objects — ☐ Oui/Non

**Critère** : soustraire un objet à un autre (sphère qui perce un plan, cylindre creusé par un
autre perpendiculaire).

- **Implémentation** : CSG en décorateur
  ```cpp
  // difference : A \ B
  //   on garde t_A, et on l'ignore si B est intersecté entre t_A et +∞
  //   (avec epsilon pour éviter les surfaces coplanaires)
  intersectA(r, recA);  intersectB(r, recB);
  if (recB.hit && recB.t < recA.t + EPS) → rejet
  ```
- **Effort** : 1–1,5 j.
- **Pièges** : surfaces coplanaires (z-fighting) → epsilon ; ombres internes (le rayon d'ombre
  doit aussi respecter le CSG).
- **Preuve** : `opt_negatif.rt` → (a) plan percé d'un trou circulaire, (b) cylindre creusé par
  un cylindre perpendiculaire. Faire **pivoter** la caméra pour voir le trou en lumière.

### 7.3 Simple native objects — ☐ Oui/Non

**Critère** : un objet de complexité **≤ sphère/cylindre/cône** (degré 2) : **paraboloïde** ou
**hyperboloïde** — **un seul suffit**.

- **Implémentation** : forme quadratique générique. Exemple paraboloïde (axe Y) :
  ```
  y = k·(x² + z²)  ⇒  k(x² + z²) − y = 0
  a = k(dx² + dz²),  b = −2k(dx·ox + dz·oz) − dy,  c = k(ox² + oz²) − oy
  ```
  + plan de troncature (hauteur) comme le cylindre.
- **Effort** : 1 j (réutiliser le squelette du cylindre).
- **Preuve** : `opt_natifs.rt` — paraboloïde **et** hyperboloïde, avec matière métallique pour
  montrer la courbure.

---

## 8. Effets visuels usuels

**Grille 0…5, 1 point par effet.** Ordre de rentabilité décroissant :

| # | Effet | Implémentation | Effort |
|---|-------|----------------|--------|
| 1 | **Antialiasing** | sur-échantillonnage N×N (ou jitter) + moyenne ; `samplesPerPixel` pilotable par fichier/UI. Déjà en place pour les ombres (`shadowSamples`) → étendre aux pixels | **0,5 j** |
| 2 | **Sépia / filtre de couleur** | matrice 3×3 en post-traitement : `sepia = clamp(…)` ; un seul `if (filter == Sepia)` dans le post-proc | 0,25 j |
| 3 | **Cartoon effect** | quantifier l'intensité en 3–4 paliers **+ contour** : si `\|dot(N, V)\| < seuil` → noir (outline) | 0,5 j |
| 4 | **Motion blur** | sur-échantillonner la direction du rayon entre la position caméra `t` et `t+Δ` (ou accumulation de N frames d'une caméra en mouvement) | 1 j |
| 5 | **Stereoscopie rouge/cyan** | 2 rendus légèrement décalés (IPD) ; composer : canal rouge du gauche + canaux GB du droit | 0,5 j |

- **Preuve** : `opt_effets_visuels.rt` avec un **sélecteur de filtre** dans l'UI ou une directive
  `post { filter antialias|sepia|cartoon|stereo }` — montrer en direct en changeant la valeur.
- **Conseil** : les 3 premiers = **1,25 j pour 3 points**.

---

## 9. Effets techniques et performance

| # | Critère | Points | État | Action |
|---|---------|--------|------|--------|
| 1 | **Clustering** sur plusieurs machines | **2** | ✖ | découpage par tuiles (voir ci-dessous) |
| 2 | **Multi-thread** | 1 | ✔ (`ThreadPool`) | preuve : `nproc`, affichage du nombre de threads |
| 3 | Le rendu est **vraiment rapide** | 1 | △ | mesurer + afficher rays/s, BVH/SAH, tiling |
| 4 | **Screenshot / save in-program** | 1 | ✔ (`P`/bouton `Save PNG` → `io::saveScreenshot`, `docs/preuves/`, `RT_SCREENSHOT_DIR`) | démo clavier + UI (T077) |

### 9.1 Clustering (2 points — le meilleur ratio de la grille)

> **Guide complet, script SSH prêt à l'emploi et plan d'action :**
> [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md).

```bash
# découper l'image en tuiles et rendre sur N machines
rt scenes/showcase.rt --tile 0/2 --out tile_0.png     # machine 1
rt scenes/showcase.rt --tile 1/2 --out tile_1.png     # machine 2
montage tile_*.png -tile 2x1 final.png                # montage
```
- Implémenter : `--tile col/row` (offset de rendu) + `--out` (pas d'affichage, headless).
- Effort : ~0,5 j. **Preuve** : exécuter sur 2 postes/containers et montrer l'image finale
  assemblée.

### 9.2 Performance

> **Outils de mesure et cibles Makefile :** [OUTILS.md §4](OUTILS.md) et
> [MEMORY_STRATEGY.md §4.4](MEMORY_STRATEGY.md).

- Continuer le travail déjà en place : BVH, tiling 32×32, `ThreadPool`, *Russian roulette*.
- Ajouter l'**affichage** des métriques : temps de rendu, rays/s, nombre de traversées BVH
  (les compteurs `bvhCount`/`shadowRayCount` existent).
- Preuve chiffrée dans l'UI pendant la démo (item « really fast »).

---

## 10. Environnement (interface et automatisation)

**Grille 0…5.**

| # | Critère | État | Action | Effort |
|---|---------|------|--------|--------|
| 1 | Interface de synthèse : message de chargement + **barre de progression** | ✖/△ | callback `onProgress(done,total)` → barre microui + log | 0,5 j |
| 2 | « Jolie interface » avec **chargement de fichier** et **contrôle du rendu** | △ | microui ✔ (settings, création/édition) ; ajouter **Load/Save** et toggle shadows/reflections | 1 j |
| 3 | Interagir avec la scène **sans relancer** | △ ✔ | sliders → `scene` modifié → re-render ; démo en direct | à valider |
| 4 | Rendu **automatique avec modifications entre les rendus** | ✖ | script shell (voir ci-dessous) | 0,25 j |
| 5 | Rendu **automatique d'objets générés** (tore de sphères, hélice) | ✖ | générateur de scène | 0,5 j |

```bash
#!/bin/sh
# scripts/batch_render.sh  → item Environment #4
for i in 1 2 3 4 5; do
  sed "s/rotate    axis y  angle .*/rotate axis y  angle $((i * 15))/" \
      scenes/opt_rotation.rt > /tmp/r.rt
  ./rt /tmp/r.rt --out frames/frame_$(printf %02d $i).png
done
```

```bash
# scripts/gen_helice.rt → item Environment #5 : générer une hélice de sphères et cylindres
python3 scripts/gen_scene.py helix --turns 4 --out scenes/gen_helice.rt
./rt scenes/gen_helice.rt
```

- **Preuve** : lancer un script devant le correcteur, montrer les images produites **à la suite**.

---

## 11. Options exotiques

### 11.1 Exotic objects — 0…5 (1 point par objet)

| Objet | Implémentation | Effort |
|-------|----------------|--------|
| **Tore** | quartique en `t` : `(x²+y²+z² + R² − r²)² = 4R²(x²+z²)` → résolution par **Newton** sur le polynôme de degré 4 | 1,5 j |
| **Cube perforé** | cube (6 plans limités) **\** trous cylindriques → CSG de §7.2 | 1 j |
| **Nappe de table** | surface paramétrique `z = A·sin(k·x)·sin(k·y)` → intersection par **marching/résolution locale** ou maillage analytique simplifié | 1,5 j |
| **Équations aléatoires depuis un fichier** | lire `a b c d` d'un bloc `equation { … }`, résoudre `ax⁴+bx³+…=0` | 1 j |
| **Fractales** | SDF + *sphere tracing* (`d = min distance`) sur Julia/Menger | 2 j+ |

> Conseil : **tore + cube perforé = 2,5 j pour 2 points** ; le solveur d'équations ajoute 1 pt
> et réutilise la résolution du tore.

### 11.2 In bulk — 0…5

| # | Option | Implémentation | Effort |
|---|--------|----------------|--------|
| 1 | **Vidéo** à partir du RT | script : N rotations de caméra → PNG → `ffmpeg` | 0,5 j |
| 2 | **Import .pov / .3ds** | parseur vers **primitives équationnelles** (rappel du sujet : les objets obligatoires doivent être gérés **par équations, pas par triangles**) | 2–3 j |
| 3 | **3D TV / Oculus** | stéréoscopie (déjà §8) + rendu `--stereo` side-by-side | 0,5 j |
| 4 | **Spot non ponctuel** (ombre sans netteté) | `AreaLight` + moyennage (§3.3) | 0,5 j |
| 5 | Autre truc de fou | au choix : **rendu progressif**, **SAH**, **cluster**… | — |

### 11.3 Möbius ribbon — ☐ Oui/Non

```x(u,v) = (R + v·cos(u/2))·cos(u)
y(u,v) = (R + v·cos(u/2))·sin(u)
z(u,v) =  v·sin(u/2)          u ∈ [0, 2π], v ∈ [−w, w]
```
- **Implémentation réaliste** : discrétiser `u` en ~200 segments → **bande de quads** traitée
  comme une surface (ou intersection par région paramétrique + test d'appartenance).
  *Alternative propre* : traiter comme une **surface paramétrée** avec recherche de `t` locale
  (Newton sur l'équation paramétrique).
- Effort : 1,5–2 j. **Preuve** : objet seul, matière métallique, caméra en orbite.

### 11.4 Caustics / Global illumination — ☐ Oui/Non

- **Coûteux**. Pistes réalistes :
  - **Photon mapping simplifié** : tirer des photons depuis les lumières, stocker leurs impacts
    sur les surfaces réfléchissantes/transmissibles, densité de photons au rendu.
  - **Irradiance environnementale** : une 2ᵉ bounce diffuse (rayons aléatoires par pixel +
    moyennage, temps de calcul ×3–5).
- Effort : 3–5 j. **À tenter seulement** si les jalons J1–J5 sont tenus.
- **Preuve** : ombre d'une sphère de verre montrant un **halo lumineux** (caustique) au sol.

### 11.5 « The last… and the least » — ☐ Oui/Non (« est-ce que c'est beau ? »)

100 % subjectif. Concrètement :
- 3 **scènes de démo soignées** : cadrage, palette de couleurs cohérente, matériaux variés
  (miroir, verre, métal, mat), composition en tiers.
- Menu de scènes dans l'UI (chargement en 1 clic).
- Soigner l'**affichage** : barre de progression, temps de rendu, pas de texte terminal brut.

---

## Checklist finale « options »

Avant la soutenance, chaque ligne doit être **cochée et démontrée** :

- [ ] Chaque option a une scène `scenes/opt_*.rt` **versionnée et testée**
- [ ] Chaque valeur « modifiable » l'est **depuis le fichier ou l'UI**, sans recompiler
- [ ] Aucune scène ne provoque de crash, de warning ni de fuite
- [ ] Les scènes se lancent en **une commande** depuis la racine du dépôt
- [ ] Les preuves ne reposent **sur aucune image pré-rendue**
- [ ] Chaque membre peut expliquer **n'importe quelle** option du dépôt
