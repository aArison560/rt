# RT — Stratégie mémoire et méthode « sans malloc » (apports de Webserv)

> **Projet source analysé** : `/home/nherimam/Git/Webserv` — serveur HTTP/1.1 en C++,
> **sans une seule allocation dynamique** (`malloc`, `new`, `free`, `std::vector`,
> `std::string` : **0 occurrence** dans `src/`, vérifié par grep).
>
> **Question** : leur méthode est-elle intéressante pour RT ?
>
> **Réponse courte** : **oui, sélectivement**. Le sujet RT **autorise toute la libstdc++**
> ([SPECIFICATIONS.md §2.2](SPECIFICATIONS.md)) : « sans malloc » n'est pas une contrainte,
> c'est un **choix d'ingénierie**. Son bénéfice réel est le **déterminisme**, pas l'économie.
> À adopter sur le *hot path* ; à refuser comme principe global.

---

## Sommaire

1. [Ce que fait Webserv (constaté dans le code)](#1-ce-que-fait-webserv-constaté-dans-le-code)
2. [Verdict : à adopter / à adapter / à refuser](#2-verdict--à-adopter--à-adapter--à-refuser)
3. [🚨 Bug repéré : exceptions dans la boucle de rendu](#-bug-reperté-exceptions-dans-la-boucle-de-rendu)
4. [Plan mémoire concret pour RT](#4-plan-mémoire-concret-pour-rt)
5. [Cibles Makefile à copier](#5-cibles-makefile-à-copier)
6. [Comment en parler à la soutenance](#6-comment-en-parler-à-la-soutenance)

---

## 1. Ce que fait Webserv (constaté dans le code)

| Technique | Preuve dans le code | Principe |
|-----------|---------------------|----------|
| **0 allocation dynamique** | aucun `malloc`/`new`/`free`/`std::vector`/`std::string` dans `src/` | tout est prévu à l'avance |
| **2 arenas** | `Server.hpp` : `alpha = {(u8*)&connections, 0, sizeof(connections)}` (64 Mo) ; `beta = {storage, …}` (4 Mo) | *bump allocator* : `size += ALIGN_UP(bytes, 64)` → un simple ajout |
| **Pool fixe** | `ConnectionPool.hpp` : `Connection connections[4096]` ; `Connection.hpp` : `STATIC_ASSERT(sizeof(Connection) == 16384)` | 16 Ko/connexion, 64 Mo, **mémoire contiguë** |
| **Recouvrement d'états** | `Connection.hpp` : `union { {recvBuffer, sendBuffer}; {parseBuffer, Request req}; }` | les mêmes octets servent à 2 phases exclusives → moitié de mémoire |
| **Allocation O(1) à bitmaps** | `ConnectionPool::acquire_slot()` : `block.find_first_clear()` puis `element[…].find_first_clear()` | recherche de slot libre en quelques instructions (2 niveaux : 64 blocs × 64 connexions) |
| **POD + `init/reset/clear`** | convention documentée dans `docs/README.md` | zéro constructeur utilisateur ; tout est réinitialisable sans realloc |
| **Spans compressés** | README : 6 vues de chaînes passent de 96 à 24 octets (4 B par vue au lieu de 16) | encodage relatif → plus d'objets tiennent dans le cache |
| **SIMD + padding « clobberable »** | intrinsics SSE/AVX, `ascii_memchr`, `Xoroshiro128_simd` | lecture au-delà de la fin assumée, **protégée par un padding alloué en amont** |
| **Branchless / LUT** | `Status::num_to_status` : table de 64 octets (1 cache line), 18 lignes d'asm, **aucune branche** | ID HTTP → index linéaire sans `if` |
| **Build agressif** | `Makefile` : `-fno-exceptions`, `-nostdlib++` (70 Ko d'exceptions économisés), cibles `asan`/`tsan`/`fast`/`compdb` | le lien avec la stdlib n'est pas gratuit |
| **Boucle I/O** | `epoll` limité au FD client, IO atomiques `ATOMIC_IOSIZE` | supprimer les états intermédiaires = moins de bugs |
| **Invariants documentés** | `docs/README.md` : *Parsing Invariants*, *Padding Invariants*, *Location Invariants* | chaque contrainte mémoire est **écrite et justifiée** |

### 1.1 L'idée maîtresse

> **Une allocation = un ajout d'entier.** Une *arena* (bloc unique + compteur d'offset)
> remplace l'appel à l'allocateur système : ni fragmentation, ni verrouillage entre threads,
> ni fuite possible (on libère tout d'un coup en remettant `size = 0`).
>
> Corollaire pour un ray tracer : **pendant le rendu, aucun appel à `malloc`**.

---

## 2. Verdict : à adopter / à adapter / à refuser

### ✅ À adopter (fort impact, faible risque)

| # | Principe | Bénéfice concret pour RT | Effort |
|---|----------|--------------------------|--------|
| 1 | **Arena par thread** pour le *hot path* (rayons, `HitRecord`, file de tuiles) | le rendu multithread **ne touche jamais `malloc`** → pas de contention de l'allocateur *ptmalloc* (verrou par thread), meilleure localité cache, et **valgrind propre par construction** (exigence « no memory leaks ») | 1 j |
| 2 | **Framebuffer et tuiles préalloués une seule fois** | zéro allocation par frame ; mémoire **préditable** : `W × H × 4` octets | déjà partiel |
| 3 | **`static_assert(sizeof(…))`** sur `Ray`, `HitRecord`, `Tuile` | détecte toute croissance silencieuse d'une structure dans la boucle | 1 h |
| 4 | **Convention `init/reset/clear`** | recharger une scène ou réinitialiser l'état **sans realloc** → parfait pour l'item *Environment 3* (interaction live) | 2 h |
| 5 | **Cibles `asan`/`tsan`/`fast`/`compdb`** | **TSan est critique** : les tuiles sont partagées entre threads | 1 h |
| 6 | **Benchmarks avec variance** (idée du `TODO.md` de Webserv : *« un profiler doit prendre la variance en compte »*) | item *Technical effects* « le rendu est vraiment rapide » : 10 runs + écart-type, pas un ressenti | 2 h |
| 7 | **Invariants mémoire documentés** (style *Padding Invariants*) | preuve d'organisation et de rigueur → *Group organization* + crédibilité | 2 h |

### ⚠️ À adapter

| # | Principe | Adaptation requise |
|---|----------|--------------------|
| 8 | **Tailles fixes** | OK pour les **structures** ; mauvais pour les **données de scène** (objets/textures inconnus à l'avance). Solution : capacité déclarée dans le fichier (`limits { max_objects 256 }`) + **erreur propre si dépassée** |
| 9 | **Codes de retour au lieu d'exceptions** | leur `Status.hpp` / `result.hpp` est un bon modèle pour le **parser** — mais il faut être **cohérent** : soit on migre tout, soit on garde les exceptions. Voir §3 |
| 10 | **LUT *branchless*** | utile pour les patterns (`checker`, `perlin`), tables de couleurs, index de matériau — pas pour tout |
| 11 | **« 16 Ko par connexion »** | équivalent RT : **« N octets par pixel/tuile »** budgété à l'avance, affiché dans l'UI |

### ❌ À refuser

| # | Principe | Pourquoi |
|---|----------|----------|
| 12 | **64 Mo statiques globaux** | nos scènes sont *data-driven* ; allouer selon la résolution et la scène |
| 13 | **Supprimer RAII partout** | SDL, fichiers, textures PNG = **ressources OS** → RAII obligatoire, sinon fuites de *handles* |
| 14 | **SIMD écrite à la main partout** | conclusion de leurs propres auteurs : *« definitely not worth the headache »* ; le `__m128d` déjà présent dans `src/core/Vec3.cpp` suffit tant qu'il est testé |
| 15 | **Boucle `epoll`** | Webserv est *I/O-bound*, RT est *CPU-bound* ; notre boucle SDL + drapeaux `dirty` est déjà le bon modèle |
| 16 | **`-fno-exceptions` brut** | incompatible avec le code actuel **et** avec la règle « aucun arrêt inattendu → 0 » |

### Validation T024 — limites déclarées et passe croisée (implémentée)

> Capacité déclarée dans le fichier (`limits { max_objects 256 }`) + erreur propre
> si dépassée : c'est l'adaptation de la ligne 8 ci-dessus, implémentée en T024
> dans `include/rt/scene/Validator.hpp` / `src/scene/Validator.cpp` (règle R1 :
> toute borne passe par `schema::check*()`, aucun `throw` R2).

| Contrôle | Règle | Erreur |
|----------|-------|--------|
| `max_objects` | `totalObjectCount() <= limits.max_objects` | `LimitExceeded` : `scene too large: 300 objects, limit 256` |
| `max_lights` | `lights.size() <= limits.max_lights` | `LimitExceeded` : `scene too large: 2 lights, limit 1` |
| `max_texture_bytes` | somme `stat` des fichiers distincts (`0` si absent/illisible, sans charger) `<= limits.max_texture_bytes` | `LimitExceeded` : `scene too large: 2048 texture bytes, limit 1000` |
| Bornes du schéma | chaque valeur revalidée par `checkInt`/`checkNumber`/`checkEnum`/`checkString` (couleurs 0-1, `fov` 1-179, etc.) | `OutOfRange` / `InvalidArgument` avec le chemin |
| Croisée caméra | `target != position`, `up` non nul et non colinéaire à la visée | `InvalidArgument` : `invalid camera: …` |
| Croisée lumières | `point/spot/area` exigent `position`, `directional` exige `direction` non nulle, `spot` exige `target` | `InvalidArgument` : `light #i 'nom': … requires …` |
| Croisée objets | `plane` exige `point` + `normal` non nulle, `cylinder/cone` exigent `axis` non nul, `slice` exige `min <= max` | `InvalidArgument` : `object #i 'nom': …` |
| Croisée matériau | `transparency > 0` exige `ior > 1` | `InvalidArgument` : `… transparent material requires ior > 1 …` |

Invariants mémoire (pas d'allocation surprise) : les vecteurs ne grandissent que par
`push_back` (jamais `reserve(max)` sur entrée non validée), bornés en cours de parse par
les garde-fous durs `kHardMaxObjects = 100000` / `kHardMaxLights = 1024` (max du schéma),
puis contrôlés contre `limits` final en fin de parse (l'ordre des sous-blocs est libre).
Le budget textures est estimé par `stat` uniquement. Mesuré en T024 : scène 300 objets /
limite 256 → exit 1 avec `fichier:ligne:colonne`, 0 crash (ni 139 ni 134), valgrind 0 fuite
(2747 allocs / 2747 frees sur le cas 300 objets, 30 / 30 sur le cas nominal) ; `/usr/bin/time`
absent du poste, remplacé par valgrind + taille fichier (15 Ko) + temps (< 10 ms).

---


## 3. 🚨 Bug repéré : exceptions dans la boucle de rendu

### 3.1 Le constat

Des exceptions sont levées **au cœur du calcul**, dans des chemins atteignables à chaque rayon :

| Fichier | Code |
|---------|------|
| `src/core/Vec3.cpp` | `if (std::abs(scalar) <= EPSILON) throw std::runtime_error("Division by zero in Vec3::operator/");` |
| `src/core/Vec3.cpp` | `if (mag <= EPSILON) throw std::runtime_error("Cannot normalize zero-length vector");` (×2) |
| `src/core/Matrix4x4.cpp` | `throw std::runtime_error("Matrix4x4::inverse singular matrix");` |
| `src/core/Ray.cpp`, `src/scene/Camera.cpp` | `try { … }` imbriqués dans la génération de rayons |

### 3.2 Pourquoi c'est un risque « note = 0 »

Direction nulle après une réfraction dégénérée, normale nulle sur un sommet de cône, matrice
singulière après une transformation d'objet → exception levée dans `trace()` → si elle n'est
pas attrapée **au bon niveau** : `std::terminate` → `abort` →
> « no segfault, nor other **unexpected, premature, uncontrolled or unexpected termination**
> of the program, else the final grade is **0** » ([SPECIFICATIONS.md §4.1](SPECIFICATIONS.md)).

Un `catch` trop large a l'inverse du problème : il **masque** le bug et produit une image
incorrecte sans prévenir.

### 3.3 Correctifs (à faire avant tout le reste — priorité P0)

| Option | Fait | Recommandation |
|--------|------|----------------|
| **A. Comportement défini sans exception** | `operator/` par un scalaire quasi nul → renvoyer le vecteur inchangé ou `Vec3{0,0,0}` documenté ; `normalize()` d'un vecteur nul → renvoyer `(0,0,0)` + `frontFace` corrigé | **✔ recommandé** pour les chemins chauds : c'est exactement l'esprit « sentinel sans branche » de Webserv |
| **B. Code d'erreur propagé** | `intersect()` renvoie `bool`/`std::optional`, pas d'exception | pour le **parser** et le chargement de fichiers (déjà le cas : `reportError()`) |
| **C. Garde-fou unique** | un `try/catch` **au niveau `Renderer::render()`** seulement, avec message clair + code retour non nul | filet de sécurité, **pas** le mécanisme principal |

**Ordre de traitement :** A sur `Vec3`/`Matrix4x4` → vérifier `Ray.cpp`/`Camera.cpp` → C en
filet de sécurité → tester avec une scène volontairement dégénérée.

---

## 4. Plan mémoire concret pour RT

### 4.1 Ce qui doit être alloué **une fois** au démarrage

| Bloc | Taille | Remarque |
|------|--------|----------|
| Framebuffer | `W × H × 4` | 1920×1080 → **8 Mo** |
| Trames de travail (par thread) | `nthreads × (rayons + HitRecords)` | arena de 1–4 Mo par thread |
| Scène | `maxObjects × sizeof(AObject)` + lumières | capacité déclarée dans le fichier |
| BVH | `2 × maxObjects × sizeof(BVNode)` | reconstruite uniquement quand `objectVersion` change |
| Cache de textures | `Σ (w × h × 3)` | **poinsons partagés `shared_ptr`**, sinon fuite |
| File de tuiles | `nTuiles × sizeof(Tuile)` | `static_assert` sur `sizeof(Tuile)` |

### 4.2 Ce qui ne doit **jamais** allouer pendant le rendu

```
Renderer::render()
 └─ par pixel : Ray, HitRecord          → trames locales (registres / arena)
 └─ par lumière : calculs scalaires     → aucune allocation
 └─ récursion : rayons réfléchis/réfractés → pile + profondeur bornée
 └─ écriture : framebuffer[i]           → mémoire déjà réservée
```

**Règle à écrire dans le code et à vérifier** : *aucun appel à `malloc`/`new`/`std::vector::push_back`
dans `src/rendering/`, `src/geometry/`, `src/core/`* (hors construction de scène).

### 4.3 Ce qui reste libre (data-driven)

| Élément | Pourquoi | Comment |
|---------|----------|---------|
| Scène (objets, lumières, matériaux) | nombre inconnu à l'avance | `std::vector` avec `reserve()` à la capacité annoncée |
| Textures | fichiers externes | cache `std::map<path, shared_ptr<Texture>>` + RAII |
| Fenêtre / contexte SDL | ressources OS | RAII (destructeur = `SDL_Destroy*`) |
| Images exportées | ponctuel | RAII fichier |

> **Pourquoi ne pas tout fixer ?** Parce que nous devons pouvoir charger une scène créée par le
> correcteur pendant la démo : une capacité **déclarée dans le fichier** + une erreur propre
> si elle est dépassée donne la prévisibilité de Webserv **sans** la rigidité d'une constante
> globale.

### 4.4 Mesures à publier (item *Technical effects* « vraiment rapide »)

| Métrique | Outil | Où l'afficher |
|----------|-------|---------------|
| Temps de rendu, rays/s | compteurs déjà présents (`bvhCount`, `shadowRayCount`) | UI + logs |
| Variance sur 10 runs | `hyperfine --runs 10 './rt scenes/x.rt --out /tmp/x.png'` | doc de benchmark |
| Mémoire pic | `/usr/bin/time -v` (RSS) | README |
| Fuites | `valgrind --leak-check=full` | CI |
| Races | **TSan** | CI |
| Cache misses | `perf stat -e cache-misses,cycles,instructions` | si `perf` dispo (absent ici) |

---

### 4.5 Invariants d'`Arena` / `FixedVector` (T014)

Implémentés dans `include/rt/base/Arena.hpp` (header-only) :

| # | Invariant | Conséquence |
|---|-----------|-------------|
| I1 | Le stockage est alloué **une seule fois** au constructeur | zéro `malloc`/`new` pendant le rendu (vérifié par grep, DoD T014) |
| I2 | `alloc()` ne lève **jamais** d'exception : overflow → `nullptr` | conforme R2, pas de `terminate` dans le hot path |
| I3 | L'alignement demandé doit être une **puissance de 2** (sinon `nullptr`) | pas de UB sur arrondi |
| I4 | `reset()` invalide **tous** les pointeurs servis depuis la dernière construction | durée de vie des objets bornée par le reset, jamais de pointeur persistant |
| I5 | `alloc()` renvoie de la mémoire **brute** (placement new / POD trivial) | la responsabilité d'initialisation est explicite |

`FixedVector<T,N>` : capacité fixe, `push` → `false` si plein (code d'erreur,
jamais de crash), `clear()` → réutilisation sans realloc.

### 4.6 Modèle `Scene` : cycle de vie et borne `limits` (T028)

`include/rt/scene/Scene.hpp` / `src/scene/Scene.cpp` : `init()` restaure les
défauts (`FORMAT_SCENE.md` §5, mêmes valeurs que `src/schema/`) et pré-réserve
à la capacité annoncée par défaut (16 lumières, 256 objets, 16 groupes) ;
`reset()` / `clear()` conservent la capacité (`clear` sans `shrink`, sans
realloc) ; `touchObjects()` incrémente `objectVersion` et lève `sceneDirty` /
`displayDirty` (R5, invalidera la BVH en T062) ; `markClean()` les referme.
Les calques supérieurs prennent `const Scene&` (lecture seule).

Mémoire bornée par `limits` (validé en T024, `push_back` seul, jamais
`reserve(max)` sur entrée non validée) :

```
mem_scene <= max_objects * sizeof(Object) + max_lights * sizeof(Light)
             + groupes (comptés dans max_objects via totalObjectCount)
             + textures (stat seul, <= max_texture_bytes)
```

Mesuré : `sizeof(Object) = 448`, `sizeof(Light) = 144` (donc défauts
256/16 → ~115 Ko + groupes). Testé dans `tests/unit/test_scene.cpp` :
`reset` réutilise la capacité, `objectVersion` incrémente à chaque mutation,
`totalObjectCount <= max_objects`.

## 5. Cibles Makefile à copier

> **Implémentées en T002** — recette réelle ci-dessous ; les cibles passent par `$(MAKE) re`
> pour garantir une compilation homogène (mêmes flags pour tous les objets).

```make
# Dans le Makefile de RT (inspiré de Webserv)
ASANFLAGS = -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
TSANFLAGS = -g -O0 -fsanitize=thread -fno-omit-frame-pointer
FASTFLAGS = -O3 -march=native

asan:
	$(MAKE) re CXXFLAGS="$(CXXFLAGS) $(ASANFLAGS)" LDFLAGS="$(ASANFLAGS)"

tsan:
	$(MAKE) re CXXFLAGS="$(CXXFLAGS) $(TSANFLAGS)" LDFLAGS="$(TSANFLAGS)"

fast:
	$(MAKE) re CXXFLAGS="$(CXXFLAGS) $(FASTFLAGS)"

compdb:
	@command -v bear >/dev/null 2>&1 || \
		{ echo "bear introuvable : installe-le (sudo apt install bear)…"; exit 1; }
	bear --output compile_commands.json -- $(MAKE) re
```

> ⚠ `-ffast-math` peut briser les NaN/infinais nécessaires à certaines détections de
> discrimination : à valider sur les tests avant de l'adopter comme build de démonstration.

---

## 6. Comment en parler à la soutenance

| Question probable | Réponse préparée |
|-------------------|------------------|
| « D'où viennent ces techniques ? » | Analyse d'un serveur HTTP écrit par l'équipe (Webserv) : arenas à bump allocator, pool à bitmaps, POD réinitialisable — nous en avons repris **le principe d'allocation déterministe**, pas le code |
| « Pourquoi ne pas tout allouer dynamiquement ? » | Pendant le rendu, un `malloc` par rayon coûterait plus que le calcul lui-même (verrouillage par thread) ; en plus, la mémoire devient **prévisible** : on sait exactement combien de mémoire une scène consomme avant de la lancer |
| « Montre-moi la preuve qu'il n'y a pas de fuite » | `valgrind` sur toutes les scènes + `ASan` + règle « aucune allocation dans le hot path » vérifiée par revue |
| « Pourquoi avoir gardé des `std::vector` ? » | Les données de scène sont *data-driven* : imposer une taille fixe rendrait impossible le chargement d'une scène inconnue pendant la démo |
| « Tu utilises l'IA ? » | Voir [PLAN_TRAVAIL.md §5](PLAN_TRAVAIL.md) : transparence sur ce qui a été généré, relu et compris |

---

## Voir aussi

- Contrainte mémoire du sujet : [SPECIFICATIONS.md §2](SPECIFICATIONS.md)
- Architecture et hot path : [ARCHITECTURE.md §4](ARCHITECTURE.md)
- Outils de mesure et de qualité : [OUTILS.md](OUTILS.md)
- Rendu distribué : [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)
- Organisation et preuves : [PLAN_TRAVAIL.md](PLAN_TRAVAIL.md)
