# RT — Format de scène `.rt` structuré (item *File ++*)

> Statut : **gelé par T020** · Date : 2026-10-07
>
> Références : [SPECIFICATIONS.md §5.2 A](SPECIFICATIONS.md) (items *Scene files*,
> *File ++*), [ARCHITECTURE.md §5](ARCHITECTURE.md) (format actuel et proposition),
> [ADR-001 §3](../ADR/001-decisions.md) (choix du format imbriqué).
> Mise en œuvre : lexer T022, parser T023, validation T024, schéma unique T021
> (table §5 ci-dessous = source déclarative à porter en `src/schema/`).

---

## 1. Pourquoi ce format

Le critère *File ++* de la fiche exige **« XML, ou une structure/hiérarchie
appropriée »** et exclut explicitement « juste un fichier avec une information
par ligne ou des blocs de base séparés par une ligne vide »
([SPECIFICATIONS.md §5.2 A](SPECIFICATIONS.md)).

L'ancien format (une directive par ligne : `bg …`, `A …`, `L …`, `sp …`,
`material …` postfixe) **ne passe pas** ce critère (voir
[ARCHITECTURE.md §5.1](ARCHITECTURE.md)).

Le présent format est donc un **arbre de blocs imbriqués `{ }`** :

```
scene
├── limits            (résolution, échantillons, garde-fous mémoire)
├── camera            (position + cible + up + fov)
├── background        (couleur de fond)
├── ambient           (lumière ambiante globale, item Ambiance ++)
├── lights
│   └── light × N     (point | spot | dir | area)
└── objects
    ├── object × N    (sphere | plane | cylinder | cone + material + transform)
    └── group × N     (nom + transform optionnel + object/group enfants, récursif)
```

Preuves de hiérarchie (exigées par le DoD de T020) :

- imbrication réelle : `scene → objects → group → object → material`
  (profondeur ≥ 4 dans l'exemple §7) ;
- un `material` n'existe que **attaché à son objet** (plus de `material`
  postfixe orphelin) ;
- l'annexe XML (§8) transcrit le même arbre élément par élément : le modèle
  est un arbre, pas une séquence de lignes.

---

## 2. Principes lexicaux

| Point | Règle |
|-------|-------|
| Encodage | **UTF-8**, fins de ligne `LF`. Un octet non UTF-8 valide = erreur lexicale. |
| Commentaires | `#` → jusqu'à la fin de la ligne, partout **sauf** dans une chaîne. Il n'y a pas de commentaire multi-lignes. |
| Espaces | Espaces, tabulations, retours ligne et commentaires sont des séparateurs équivalents (le format est *free-form*). |
| Chaînes | `"…"` guillemets doubles obligatoires ; échappements `\"` et `\\` uniquement ; guillemet non fermé = erreur. Utilisées pour les noms (`scene "vitrine"`, `name "sol"`, chemins de texture). |
| Nombres | `[-+]? ( chiffres [. chiffres]? | . chiffres ) ([eE] [-+]? chiffres)?`. Ex. : `1.5`, `-2`, `+0.25`, `1e-3`, `-2E3`. `abc`, `1.2.3`, `e5` = erreur. Les bornes (couleurs 0–1, etc.) sont validées en T024, pas au lexer. |
| Vecteurs | `(` nombre nombre nombre `)` — **exactement 3** nombres. Ex. : `(0 2 6)`, `(1.0 -2.0 0.5)`. Utilisés pour positions, couleurs, normales. |
| Identifiants | `[A-Za-z_][A-Za-z0-9_]*`. Les mots-clés sont en **minuscules** et sensibles à la casse (`light` ≠ `Light`). |
| Blocs | `ident [args…] { … }`. Arguments de tête autorisés uniquement pour `scene`, `light`, `object` (voir §4). Tout autre argument de tête = erreur. |
| Profondeur | Imbrication bornée à **32** niveaux (garde-fou T022 contre l'imbrication folle). |
| Ordre | Ordre des sous-blocs **libre** ; chaque bloc 0/1 sauf `light`, `object`, `group` (0..N). Les doublons d'un bloc 0/1 = erreur (`camera` déclarée deux fois). |
| Directives inconnues | **Erreur fatale** (jamais ignorée silencieusement). |
| `include` | **Il n'y a pas de directive `include`** dans cette version (évite les cycles, cf. T025). |

Alias canoniques (un seul nom retenu par T021, l'autre accepté au parsing
puis normalisé ; les deux formes restent valides dans cette spec) :

| Canonique | Alias accepté | Contexte |
|-----------|---------------|----------|
| `target` | `lookAt` | `camera` : point visé (les fixtures T017 utilisent `target`, ARCHITECTURE §5.2 utilisait `lookAt`) |
| `albedo` | `color` | `material` : couleur de base (fixtures : `albedo`, ARCHITECTURE : `color`) |
| `center` | `position` | `object` sphère/cylindre/cône : centre en espace objet |
| `reflectivity` | `reflect` | `material` : % de réflexion |
| `directional` (valeur) | `dir` | `light.type` : lumière parallèle |

---

## 3. Structure générale

```
scene ["nom"] {
    limits { … }        # 0/1 — résolution + échantillons + garde-fous
    camera { … }        # 1 — exactement une caméra (M5)
    background { … }    # 0/1 — défaut : noir (0 0 0)
    ambient { … }       # 0/1 — défaut : (0.06 0.06 0.08) × 1.0 (Ambiance ++)
    lights { … }        # 0/1 — conteneur, 0..N light
    objects { … }       # 1 — conteneur, 0..N object/group
}
```

- Un fichier contient **exactement un** bloc `scene` racine (vide interdit :
  `camera` et `objects` sont requis).
- `lights { light { … } light { … } }` : chaque `light` peut aussi s'écrire
  avec tête explicite `light point "key" { … }` (type + nom optionnels en
  tête, voir §4) — les deux formes sont équivalentes.
- `objects { object { … } group { … } }` : `group` contient `name` optionnel,
  `transform` optionnel et une suite récursive `object`/`group`.

---

## 4. Grammaire (EBNF)

Notation : `=` définition, `|` alternative, `[…]` optionnel, `{…}` répétition,
`(…)` groupement, `"…"` terminal littéral. `(* … *)` commentaire.

```ebnf
(* ---------- fichier ---------- *)
file        = ws , "scene" , [ ws , string ] , ws , "{" , ws ,
              { limits | camera | background | ambient | lights | objects } ,
              ws , "}" , ws , EOF ;

(* ---------- conteneurs ---------- *)
limits      = "limits" , ws , "{" , ws , { limitProp } , ws , "}" , ws ;
camera      = "camera" , ws , "{" , ws , { cameraProp } , ws , "}" , ws ;
background  = "background" , ws , "{" , ws , [ "color" , ws , vector ] , ws , "}" , ws ;
ambient     = "ambient" , ws , "{" , ws , { ambientProp } , ws , "}" , ws ;
lights      = "lights" , ws , "{" , ws , { light } , ws , "}" , ws ;
objects     = "objects" , ws , "{" , ws , { object | group } , ws , "}" , ws ;

(* ---------- lumière : tête `light [type] ["nom"]` ---------- *)
light       = "light" , [ ws , lightType ] , [ ws , string ] , ws ,
              "{" , ws , { lightProp } , ws , "}" , ws ;
lightType   = "point" | "spot" | "directional" | "dir" | "area" ;

(* ---------- objet : tête `object [type] ["nom"]` ---------- *)
object      = "object" , [ ws , objectType ] , [ ws , string ] , ws ,
              "{" , ws , { objectProp | material | transform | slice } ,
              ws , "}" , ws ;
objectType  = "sphere" | "plane" | "cylinder" | "cone" ;
group       = "group" , [ ws , string ] , ws , "{" , ws ,
              [ "name" , ws , string , ws ] ,
              [ transform ] ,
              { object | group } ,
              ws , "}" , ws ;

(* ---------- sous-blocs ---------- *)
material    = "material" , ws , "{" , ws , { materialProp | texture | pattern } ,
              ws , "}" , ws ;
transform   = "transform" , ws , "{" , ws , { transformProp } ,
              ws , "}" , ws ;
texture     = "texture" , ws , string , ws ,
              "{" , ws , { textureProp } , ws , "}" , ws ;
pattern     = "pattern" , ws , "{" , ws , { patternProp } ,
              ws , "}" , ws ;
slice       = "slice" , ws , "{" , ws , { sliceProp } ,
              ws , "}" , ws ;

(* ---------- propriétés : `clé valeur…` ---------- *)
limitProp     = ( "width" , ws , integer
                | "height" , ws , integer
                | "samples" , ws , integer
                | "max_depth" , ws , integer
                | "seed" , ws , integer
                | "max_objects" , ws , integer
                | "max_lights" , ws , integer
                | "max_texture_bytes" , ws , integer ) , ws ;
cameraProp    = ( "position" , ws , vector
                | "target" , ws , vector
                | "lookAt" , ws , vector
                | "up" , ws , vector
                | "fov" , ws , number ) , ws ;
ambientProp   = ( "color" , ws , vector
                | "intensity" , ws , number ) , ws ;
lightProp     = ( "type" , ws , lightType
                | "name" , ws , string
                | "position" , ws , vector
                | "color" , ws , vector
                | "intensity" , ws , number
                | "direction" , ws , vector
                | "target" , ws , vector
                | "angle" , ws , number
                | "size" , ws , number , ws , number
                | "attenuation" , ws , number , ws , number , ws , number
                | "range" , ws , number ) , ws ;
objectProp    = ( "type" , ws , objectType
                | "name" , ws , string
                | "center" , ws , vector
                | "position" , ws , vector
                | "point" , ws , vector
                | "normal" , ws , vector
                | "radius" , ws , number
                | "axis" , ws , vector
                | "angle" , ws , number
                | "height" , ws , number ) , ws ;
materialProp  = ( "albedo" , ws , vector
                | "color" , ws , vector
                | "ambient" , ws , number
                | "diffuse" , ws , number
                | "specular" , ws , number
                | "shininess" , ws , number
                | "reflectivity" , ws , number
                | "reflect" , ws , number
                | "transparency" , ws , number
                | "ior" , ws , number
                | "bump" , ws , number ) , ws ;
transformProp = ( "translate" , ws , vector
                | "scale" , ws , ( vector | number )
                | "rotate" , ws , "axis" , ws , ( "x" | "y" | "z" ) ,
                  ws , "angle" , ws , number ) , ws ;
textureProp   = ( "scale" , ws , number , ws , number
                | "offset" , ws , number , ws , number ) , ws ;
patternProp   = ( "type" , ws , ( "sine" | "checker" | "perlin" )
                | "scale" , ws , number
                | "frequency" , ws , number ) , ws ;
sliceProp     = ( "axis" , ws , ( "x" | "y" | "z" )
                | "min" , ws , number
                | "max" , ws , number
                | "frame" , ws , ( "object" | "world" )
                | "shape" , ws , ( "slab" | "circle" | "triangle" ) ) , ws ;

(* ---------- lexicaux ---------- *)
vector      = "(" , ws , number , ws , number , ws , number , ws , ")" ;
number      = [ "+" | "-" ] ,
              ( digits , [ "." , [ digits ] ] | "." , digits ) ,
              [ ( "e" | "E" ) , [ "+" | "-" ] , digits ] ;
integer     = [ "+" | "-" ] , digits ;
digits      = digit , { digit } ;
digit       = "0" | … | "9" ;
string      = '"' , { escape | char } , '"' ;
escape      = '\\' , ( '"' | '\\' ) ;
char        = ? tout caractère UTF-8 sauf '"' , '\\' et fin de ligne ? ;
ws          = { " " | "\t" | "\n" | "\r" | comment } ;
comment     = "#" , { ? tout sauf fin de ligne ? } ;
EOF         = ? fin de fichier ? ;
```

Notes de lecture :

- `object sphere "boule" { … }` ≡ `object { type sphere  name "boule"  … }`.
- `light point "key" { … }` ≡ `light { type point  name "key"  … }`.
- En cas de conflit tête/contenu (`object sphere { type plane … }`),
  le **contenu gagne** et la tête est ignorée avec un avertissement
  (règle tranchée pour T023 ; les deux écritures restent valides).
- `height` (cylindre/cône fini), `texture`, `pattern`, `slice` sont
  **définis syntaxiquement dès maintenant** mais marqués *réservé option*
  dans la table §5 : le parser les accepte, la validation sémantique
  complète viendra avec les tâches d'options (cylindre/cône infini en
  T043–T044, tranché en option *Limited objects*).

---

## 5. Table des directives

Chemin hiérarchique = l'adresse déclarative reprise par T021
(`src/schema/` : une ligne par directive). Types : `int`, `float`,
`vec3`, `string`, `enum`, `bloc`.

### 5.1 `scene` et `limits`

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `scene` | bloc racine, 1× | — | nom `[string]` optionnel | Racine du fichier. `camera` + `objects` requis. |
| `scene.limits` | bloc, 0/1 | voir défauts ci-dessous | — | Résolution, échantillonnage, garde-fous mémoire (T024). La CLI (`--spp`, `--seed`, dimensions) **surcharge** ces valeurs. |
| `scene.limits.width` | int | `640` | 1–8192 | Largeur image (px). |
| `scene.limits.height` | int | `480` | 1–8192 | Hauteur image (px). |
| `scene.limits.samples` | int | `4` | 1–1024 | Échantillons par pixel (`--spp` surcharge). |
| `scene.limits.max_depth` | int | `4` | 0–16 | Profondeur max de récursion (réflexion/réfraction). |
| `scene.limits.seed` | int | `0` | 0–2³²−1 | Graine de scène (combinée aux coords absolues, cf. RNG T016). |
| `scene.limits.max_objects` | int | `256` | 1–100000 | Au-delà → erreur propre `scene too large`, pas d'allocation surprise (T024). |
| `scene.limits.max_lights` | int | `16` | 1–1024 | Idem pour les lumières. |
| `scene.limits.max_texture_bytes` | int | `67108864` (64 Mio) | 0–2³¹−1 | Budget textures cumulé (T024). |

### 5.2 `camera` (M5)

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `scene.camera` | bloc, **1×** | — | — | Œil + visée. Deux fichiers ne différant que par ce bloc = test *Did you know?*. |
| `scene.camera.position` | vec3 | `(0 1 4)` | — | Position de l'œil, n'importe où (M5). |
| `scene.camera.target` (alias `lookAt`) | vec3 | `(0 0 0)` | ≠ `position` | Point visé. `target == position` ou colinéaire à `up` → erreur (cas dégénéré, T031). |
| `scene.camera.up` | vec3 | `(0 1 0)` | norme > ε | Verticale monde. |
| `scene.camera.fov` | float (deg) | `60` | 1–179 | Champ de vision vertical. |

### 5.3 `background` et `ambient`

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `scene.background` | bloc, 0/1 | noir | — | Couleur des rayons manqués. |
| `scene.background.color` | vec3 | `(0 0 0)` | 0–1 par canal | Fond de scène. |
| `scene.ambient` | bloc, 0/1 | voir ci-dessous | — | Ambiance globale pilotée par fichier (*Ambiance ++*). **Aucun objet jamais totalement noir.** |
| `scene.ambient.color` | vec3 | `(0.06 0.06 0.08)` | 0–1 par canal | Teinte ambiante. |
| `scene.ambient.intensity` | float | `1.0` | 0–10 | Intensité globale. |

### 5.4 `lights` (M7)

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `scene.lights` | bloc, 0/1 | — | — | Conteneur. 0 lumière = rendu ambiant seul (valide). |
| `scene.lights.light` | bloc, 0..N | — | tête `[type] [nom]` | Une source. Tête et propriétés `type`/`name` équivalentes. |
| `…light.type` | enum `point\|spot\|directional\|dir\|area` | `point` | — | `dir` = alias de `directional`. `area` réservé (*spot non ponctuel*). |
| `…light.name` | string | `""` | ≤ 128 car. | Nom pour logs/UI. |
| `…light.position` | vec3 | requis si `point\|spot\|area` | — | Position de la source. |
| `…light.color` | vec3 | `(1 1 1)` | 0–1 par canal | Couleur de la source. |
| `…light.intensity` | float | `1.0` | 0–1000 | Doubler double la contribution (test T051). |
| `…light.direction` | vec3 | requis si `directional` | norme > ε | Direction d'éclairage (soleil). |
| `…light.target` | vec3 | requis si `spot` | — | Point visé du spot (*Direct light* : visé vers la caméra = aveuglement). |
| `…light.angle` | float (deg) | `30` si `spot` | 1–90 | Demi-ouverture du spot. |
| `…light.size` | 2× float | `1 1` si `area` | > 0 | Taille de la source étendue (**réservé**). |
| `…light.attenuation` | 3× float `(c l q)` | `(1 0 0)` | ≥ 0 | Atténuation `1/(c + l·d + q·d²)` (T051). |
| `…light.range` | float | `0` (= infini) | ≥ 0 | Portée max, `0` = infinie. |

### 5.5 `objects` : `object` et `group` (M3, M4)

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `scene.objects` | bloc, **1×** | — | — | Conteneur (peut être vide : fond seul). Append simple, **aucune déduplication** (plusieurs objets du même type coexistent, M3). |
| `scene.objects.object` | bloc, 0..N | — | tête `[type] [nom]` | Un objet. Intersection **propre à chaque type** (M3). |
| `…object.type` | enum `sphere\|plane\|cylinder\|cone` | **requis** | — | Tête ou propriété équivalentes. |
| `…object.name` | string | `""` | ≤ 128 car. | Nom pour logs/UI/picking. |
| `…object.center` (alias `position`) | vec3 | `(0 0 0)` | — | Sphère/cylindre/cône : centre en **espace objet**. |
| `…object.radius` | float | `1.0` | > ε | Sphère : rayon. Cylindre : rayon autour de l'axe. |
| `…object.point` | vec3 | requis si `plane` | — | Plan : un point du plan (espace objet). |
| `…object.normal` | vec3 | requis si `plane` | norme > ε | Plan : normale (espace objet). |
| `…object.axis` | vec3 | `(0 1 0)` | norme > ε | Cylindre/cône **infini** autour de cet axe local (T043–T044). |
| `…object.angle` | float (deg) | `20` si `cone` | 1–89 | Cône : demi-angle au sommet (deux nappes). |
| `…object.height` | float | non borné (= infini) | > 0 si présent | **Réservé** : troncature du cylindre/cône (option *Limited objects*, T133). Ignoré pour l'instant (= infini). |
| `scene.objects.group` | bloc récursif, 0..N | — | tête `[nom]` | Élément composé réutilisable (*Composed elements*) : `group "nom" { object… group… }`. |
| `…group.name` | string | `""` | ≤ 128 car. | Nom du groupe (tête ou propriété). |
| `…group.transform` | bloc, 0/1 | identité | — | Transform parent appliquée aux enfants. |

### 5.6 `material` (M7 + réflexion/transparence)

Tout champ est **pilotable depuis le fichier** (rien par recompilation —
règle du sujet, cf. T050).

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `…object.material` | bloc, 0/1 | défauts ci-dessous | — | Attaché à l'objet courant (fini le postfixe). Absent = matériau mat gris. |
| `…material.albedo` (alias `color`) | vec3 | `(0.8 0.8 0.8)` | 0–1 par canal | Couleur de base. |
| `…material.ambient` | float | `0.1` | 0–1 | Poids de l'ambiance globale. |
| `…material.diffuse` | float | `0.7` | 0–1 | Poids Lambert. |
| `…material.specular` | float | `0.5` | 0–1 | Poids Blinn-Phong (le « petit point blanc », M7). |
| `…material.shininess` | float | `32` | 1–1024 | Exposant spéculaire. |
| `…material.reflectivity` (alias `reflect`) | float | `0.0` | 0–1 | % miroir continu (`0` = mat, `1` = miroir pur, T056). |
| `…material.transparency` | float | `0.0` | 0–1 | % de transparence (T057). |
| `…material.ior` | float | `1.5` | 1–3 | Indice de réfraction, loi de Descartes (T057). `1` = pas de déviation. |
| `…material.bump` | float | `0.0` | 0–10 | Force de bump (**réservé** textures). |
| `…material.texture` | bloc, 0/1 | — | `texture "path" { … }` | Texture image (**réservé** item *Textures*). |
| `…material.texture.scale` | 2× float | `1 1` | > 0 | Étirement (sous-critère 3). |
| `…material.texture.offset` | 2× float | `0 0` | — | Décalage (sous-critère 4). |
| `…material.pattern` | bloc, 0/1 | — | — | Procédural (**réservé** *Disruptions* : `sine\|checker\|perlin`). |

### 5.7 `transform` (M4) et `slice` (réservé)

| Chemin | Type | Défaut | Bornes | Description |
|--------|------|--------|--------|-------------|
| `…object.transform` | bloc, 0/1 | identité | — | Translations/rotations/scale. Le rayon passe en espace objet par `M⁻¹`, la normale revient par `(M⁻¹)ᵀ` (approche A, T012). Ex. : sphère `(0 0 0)` → `(42 42 42)`. |
| `…transform.translate` | vec3 | `(0 0 0)` | — | Translation. Répétable, appliquée dans l'ordre d'écriture. |
| `…transform.scale` | vec3 ou float | `(1 1 1)` | > ε | Échelle (uniforme si 1 nombre). |
| `…transform.rotate` | `axis x\|y\|z angle d` | — | 0–360 deg | Rotation. Répétable. |
| `…object.slice` | bloc, 0/1 | — | — | **Réservé** *Limited objects* : `axis x\|y\|z`, `min`, `max`, `frame object\|world`, `shape slab\|circle\|triangle`. Accepté syntaxiquement, effet complet en option. |

---

## 6. Exemple 1 — scène minimale (valide)

> C'est la forme canonique acceptée par le parser T023 : `tests/cases/valid/minimal.rt`
> est une instance exacte de ce patron (au nom près).

```rt
# minimal.rt — 1 sphère, 1 lumière, caméra explicite.
scene "minimal" {
    limits {
        width 640
        height 480
        samples 4
    }
    camera {
        position (0 1 4)
        target (0 0 0)
        up (0 1 0)
        fov 60
    }
    background {
        color (0.05 0.05 0.1)
    }
    ambient {
        color (0.06 0.06 0.08)
        intensity 1.0
    }
    lights {
        light {
            type point
            name "key"
            position (2 4 3)
            color (1 1 1)
            intensity 1.0
        }
    }
    objects {
        object {
            type sphere
            name "boule"
            center (0 0 0)
            radius 1
            material {
                albedo (0.8 0.2 0.2)
                diffuse 0.7
                specular 0.5
                shininess 32
            }
        }
    }
}
```

---

## 7. Exemple 2 — scène à groupes (valide, 4 primitives)

> Patron de `tests/cases/valid/group.rt` étendu aux 4 objets obligatoires
> (M3) avec transform (M4) et 2 lumières (M7). Base de `fig_vi1_base.rt` (T048).

```rt
# vitrine.rt — plan + sphère + cylindre + cône, 2 groupes, 2 lumières.
scene "vitrine" {
    limits {
        width 320
        height 240
        samples 2
        max_depth 4
        seed 7
        max_objects 256
        max_lights 16
        max_texture_bytes 67108864
    }
    camera {
        position (0 2 6)
        target (0 0 0)
        up (0 1 0)
        fov 50
    }
    background {
        color (0.02 0.02 0.05)
    }
    ambient {
        color (0.06 0.06 0.08)
        intensity 1.0
    }
    lights {
        light point "key" {
            position (3 5 2)
            color (1 1 1)
            intensity 0.8
        }
        light point "fill" {
            position (-3 3 2)
            color (0.4 0.5 1)
            intensity 0.5
        }
    }
    objects {
        group "sol" {
            object plane "sol" {
                point (0 -1 0)
                normal (0 1 0)
                material {
                    albedo (0.5 0.5 0.5)
                    diffuse 0.7
                }
            }
        }
        group "vitrine" {
            object sphere "boule" {
                center (-1.2 0 0)
                radius 1
                transform {
                    translate (0 0.2 0)
                    rotate axis y angle 30
                }
                material {
                    albedo (0.9 0.3 0.2)
                    specular 0.8
                    shininess 32
                    reflectivity 0.2
                }
            }
            object sphere "boule2" {
                center (1.2 0 0)
                radius 1
                material {
                    albedo (0.2 0.3 0.9)
                    transparency 0.5
                    ior 1.5
                }
            }
            object cylinder "fut" {
                center (3 0 0)
                axis (0 1 0)
                radius 0.5
                material {
                    albedo (0.3 0.8 0.3)
                }
            }
            object cone "pointe" {
                center (-3 0 0)
                axis (0 1 0)
                angle 20
                material {
                    albedo (0.8 0.8 0.2)
                }
            }
        }
    }
}
```

Vérifications visuelles de hiérarchie : `scene → objects → group "vitrine" →
object → material` (profondeur 4), `scene → lights → light` (profondeur 2),
commentaires `#`, chaînes `"sol"`, nombres (`320`, `0.5`), vecteurs `(0 2 6)`.

---

## 8. Annexe — équivalent XML de l'exemple 1

Le même arbre avec une syntaxe XML : la preuve que le modèle est une
hiérarchie (le critère *File ++* accepte « XML **ou** hiérarchie appropriée »).
Le parseur n'accepte que la syntaxe `{ }` ; cette annexe est une projection
documentaire, pas un second format à maintenir.

```xml
<!-- minimal.xml — projection XML de l'exemple 1 (équivalence d'arbre). -->
<scene name="minimal">
  <limits width="640" height="480" samples="4" />
  <camera>
    <position x="0" y="1" z="4" />
    <target x="0" y="0" z="0" />
    <up x="0" y="1" z="0" />
    <fov>60</fov>
  </camera>
  <background color="0.05 0.05 0.1" />
  <ambient color="0.06 0.06 0.08" intensity="1.0" />
  <lights>
    <light type="point" name="key">
      <position x="2" y="4" z="3" />
      <color r="1" g="1" b="1" />
      <intensity>1.0</intensity>
    </light>
  </lights>
  <objects>
    <object type="sphere" name="boule">
      <center x="0" y="0" z="0" />
      <radius>1</radius>
      <material>
        <albedo r="0.8" g="0.2" b="0.2" />
        <diffuse>0.7</diffuse>
        <specular>0.5</specular>
        <shininess>32</shininess>
      </material>
    </object>
  </objects>
</scene>
```

Correspondance terme à terme : chaque bloc `{ }` = un élément, chaque
propriété = un sous-élément ou attribut, chaque vecteur = 3 composantes,
les groupes s'écriraient `<group name="…">…</group>`.

---

## 9. Validation et erreurs (contrat pour T022–T024)

- Toute erreur est rapportée `fichier:ligne:colonne` + message, code retour
  ≠ 0, **zéro crash** (fichiers `tests/cases/invalid/` : accolade manquante,
  nombre `abc`, directive `witdh`).
- Directive inconnue, valeur hors borne, bloc requis manquant (`camera`,
  `objects`), doublon d'un bloc 0/1, vecteur à 2 ou 4 composantes, chaîne non
  fermée, profondeur > 32 = **erreur fatale**.
- `limits` : `max_objects` / `max_lights` / `max_texture_bytes` dépassés →
  erreur propre `scene too large: …` **avant** toute grosse allocation (T024),
  vérifiable sous `/usr/bin/time -v` sans pic mémoire.
- Références futures (`texture "…"` inexistante, `material "verre"`
  par nom) : résolues après le parse avec le même format d'erreur (T023).

## 10. Compatibilité et migration

- L'ancien format une-directive-par-ligne n'est **pas** accepté par ce
  grammaire : détection auto prévue en T023 (ligne commençant par `scene` ou
  présence de `{` → nouveau format, sinon message « legacy non supporté,
  convertir »), sans crash.
- Les ~15 scènes legacy seront converties par script (T023/T029) vers
  `scenes/default.rt` et `scenes/fig_*.rt`.
- Toute nouvelle directive future (option) s'ajoute comme **une entrée** dans
  la table §5, reprise en une ligne de `src/schema/` (T021) : parser,
  validation, UI et doc en dérivent (règle R1).
