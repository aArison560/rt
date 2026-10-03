# RT — Plan de travail de l'équipe (3 personnes)

> Ce document organise la **réalisation complète** du projet par une équipe de 3.
> Il répond directement à l'item de fiche **« Group organization »** (Oui/Non) : il faut pouvoir
> **raconter** concrètement comment le groupe s'est organisé.
>
> Sources : sujet v4.1 (instructions générales + instructions IA), fiche d'évaluation,
> `docs/GIT_STRATEGY.md` et `docs/TASK_BACKLOG.md` (branche `main`).

---

## Sommaire

1. [Principes directeurs](#1-principes-directeurs)
2. [Rôles et responsabilités](#2-rôles-et-responsabilités)
3. [Organisation hebdomadaire](#3-organisation-hebdomadaire)
4. [Workflow Git](#4-workflow-git)
5. [Cadre d'utilisation de l'IA](#5-cadre-dutilisation-de-lia)
6. [Planning par jalons](#6-planning-par-jalons)
7. [Backlog résiduel : matrice d'écarts](#7-backlog-résiduel-matrice-décarts)
8. [Quality gates (portes de qualité)](#8-quality-gates-portes-de-qualité)
9. [Risques et mitigations](#9-risques-et-mitigations)
10. [Preuves d'organisation à conserver](#10-preuves-dorganisation-à-conserver)

---

## 1. Principes directeurs

1. **L'obligatoire d'abord.** Aucune option n'est évaluée tant que la partie obligatoire n'est
   pas parfaite. Toute tâche d'option ne démarre qu'après validation de la porte `P0`.
2. **Architecture pour les options, dès le jour 1.** Choisir les points d'extension
   ([ARCHITECTURE.md §10](ARCHITECTURE.md)) avant de coder les options elles-mêmes.
3. **Une personne = un module principal**, mais **deux personnes connaissent chaque module**
   (pas de *bus factor* = 1).
4. **Chaque fonctionnalité est prouvée par un fichier de scène** dans le dépôt.
5. **Aucun code n'entre sur `develop` sans relecture** d'un pair.
6. **Zéro crash, zéro fuite** : vérifié avant chaque merge, pas la veille de la soutenance.

---

## 2. Rôles et responsabilités

Rôles alignés sur la stratégie existante (`docs/GIT_STRATEGY.md`), renommés **Membre 1/2/3**
pour coller à votre groupe. Adaptez les initiales.

| Rôle | Membre | Périmètre principal | Secondaire (backup) |
|------|--------|---------------------|---------------------|
| **M1 — Fondations math & caméra** | _(Dev A)_ | `core/` : `Vec3`, `Ray`, `Matrix4x4`, `HitRecord` ; `scene/Transform`, `scene/Camera` ; tests maths | Revue du parser |
| **M2 — Géométrie & scène** | _(Dev B)_ | `geometry/` : `AObject`, 4 primitives, CSG, objets composés, primitives natives ; `scene/Scene` | Revue du renderer |
| **M3 — Rendu, lumières & plateforme** | _(Dev C)_ | `rendering/` (Renderer, Texture, BVH, ThreadPool, ImageBuffer), `lighting/`, `platform/`, `gui/`, `scene/SceneParser` | Revue de la géométrie |

**Rôles transverses** (tournants ou fixes à convenir) :

| Rôle | Mission | Preuve pendant la soutenance |
|------|---------|------------------------------|
| **Responsable qualité** | Quality gates, valgrind, tests, CI locale | Peut montrer les résultats |
| **Responsable scènes/démo** | Les 3 scènes obligatoires + 1 scène `opt_*.rt` par option | Liste des scènes et de ce qu'elles prouvent |
| **Responsable organisation** | Journal des réunions, décisions, répartitions | Peut raconter l'organisation (item *Group organization*) |

> **Règle de recouvrement** : chaque module doit avoir un **auteur** et un **relecteur
> permanent**. Personne ne valide seul son propre code.

### 2.1 Frontière de fusion (éviter les conflits)

| Membre | Fichiers dont il est **seul** à écrire | Fichiers partagés (fusion coordonnée) |
|--------|------------------------------------------|----------------------------------------|
| M1 | `src/core/*`, `src/scene/Transform.cpp`, `src/scene/Camera.cpp` | `include/core/Material.hpp` (avec M3) |
| M2 | `src/geometry/*`, `src/scene/Scene.cpp` | `include/geometry/AObject.hpp`, `scenes/*.rt` |
| M3 | `src/rendering/*`, `src/lighting/*`, `src/platform/*`, `src/gui/*`, `src/scene/SceneParser.cpp` | `src/app/main.cpp`, `Makefile` |

Toute modification **hors de son périmètre** passe par une *pull request* + accord de l'auteur.

---

## 3. Organisation hebdomadaire

| Moment | Durée | Participants | Contenu |
|--------|-------|--------------|---------|
| **Kick-off lundi** | 45 min | les 3 | Objectifs de la semaine, répartition, blockers connus |
| **Point quotidien asynchrone** | 5 min/jour | les 3 | « fait hier / aujourd'hui / bloqué » (Slack/ forum) |
| **Point technique mi-semaine** | 30 min | les 3 | Arbitrages d'architecture, revues en direct |
| **Revue de code** | à la demande, < 24 h | 2 membres | Tout PR avant merge |
| **Revue hebdo vendredi** | 60 min | les 3 | Démo des avancées **visibles à l'écran**, mise à jour du backlog |
| **Rétro** | 15 min (vendredi) | les 3 | Ce qui a bloqué, ce qu'on change |

**Règles de gestion du temps :**
- Toute tâche > **4 h** est découpée avant d'être prise.
- Si un membre est **bloqué > 2 h**, il l'annonce immédiatement (règle de la fiche :
  « conflicts block work → immediate chat notification »).
- Chacun note ses **heures réelles** par tâche → alimente la rétrospective et prouve la
  gestion du temps.

---

## 4. Workflow Git

Résumé de `docs/GIT_STRATEGY.md` (branche `main`) — **à appliquer tel quel** :

```bash
# 1. Se synchroniser
git checkout develop && git pull origin develop

# 2. Branche de fonctionnalité
git checkout -b feature/dev-a/vec3-implementation
#     format : <type>/<developer>/<feature>
#     type   : feature | bugfix | refactor | test
#     dev    : dev-a | dev-b | dev-c

# 3. Commit (format imposé)
git commit -m "feat(dev-a): implement Vec3 dot product"
#     <type>(<scope>): <subject>
#     types : feat fix refactor test chore docs

# 4. Pousser + PR
git push origin feature/dev-a/vec3-implementation
# PR contre develop : description, tests, valgrind

# 5. Merge (après relecture d'1 autre membre)
git fetch origin && git rebase origin/develop
make clean && make -j4 && make test
git checkout develop && git merge --ff-only feature/dev-a/...
git push origin develop
```

**Branches** : `main` (stable) ← `develop` (intégration) ← `feature/*`.
**Checklist PR** : build propre ✔ · tests ✔ · valgrind ✔ · Doxygen ✔ · revue ✔.

**Règle pratique de la semaine** : **merger au moins une fois par jour sur `develop`** pour
limiter les conflits entre les 3 (jamais de branche qui vit plus de 48 h sans merge).

---

## 5. Cadre d'utilisation de l'IA

Le sujet contient un chapitre entier **« AI Instructions »** : à respecter et à pouvoir
expliquer pendant la soutenance.

### 5.1 Règles de l'apprenant (à appliquer littéralement)

- Garder le **leadership intellectuel** : ce sont **vos** décisions techniques.
- Donner la priorité à **l'intelligence collective** du groupe et des pairs.
- Rester informé de l'évolution des outils d'IA.
- **Ne jamais** laisser l'IA assumer une décision — surtout quand elle ignore vos contraintes
  et la dynamique de l'équipe.

### 5.2 Ce qui est bien / mal vu

| ✅ Bonne pratique | ❌ Mauvaise pratique |
|---|---|
| Demander à l'IA de générer des **tests unitaires**, les relire **avec un pair**, les ajuster sur les cas limites : gain de temps **et** apprentissage | Demander à l'IA de générer **l'architecture complète** : ça « marche », mais **impossible de la défendre** en soutenance → perte de crédibilité, échec |

### 5.3 Engagement de transparence

- **Être transparent** sur la façon dont l'IA a été utilisée et **identifier clairement** ce qui
  a été généré par un outil.
- Être capable de **diver en profondeur** dans **toute** partie du projet **sans** l'IA.
- À préparer pour la soutenance : une **liste honnête** « ce qui a été généré / ce qui a été
  écrit à la main / ce qui a été modifié après relecture ».

### 5.4 Règle d'or du groupe

> « AI can make you faster, but your peers make you better. »
> L'IA accélère, **les pairs rendent le travail meilleur** : toute réponse d'IA est relue par un
> membre avant d'être acceptée, et **chaque membre doit pouvoir expliquer le code du groupe**.

---

## 6. Planning par jalons

Durées indicatives : **~6 semaines** à raison de ~15–20 h/semaine/personne
(backlog historique : ~95 h au total, ~32 h par personne pour le cœur).

| Jalon | Semaine | Objectif | Porte de sortie (DoD) |
|-------|---------|----------|------------------------|
| **J0 — Cadrage** | S0 | Lire sujet + fiche, trancher les **points ouverts** (SPECIFICATIONS §8), poser l'architecture | Docs lues par les 3 ; décisions O1–O7 actées |
| **J1 — Squelette** | S1 | Build fiable, fenêtre SDL, framebuffer, parser legacy, scènes qui tournent | `make && ./rt scenes/default.rt` sans crash ; valgrind propre |
| **J2 — Obligatoire 100 %** | S2 | 4 primitives + transforms + caméra libre + lumière/ombres/spéculaire + expose sans recalcul | **3 scènes du sujet reproduites** ; checklist obligatoire cochée |
| **J3 — Fichiers structurés** | S3 | Format hiérarchique, ambiance depuis le fichier, transparence/ior/reflect pilotables | `Scene files` + `File ++` + `Ambiance ++` prouvés |
| **J4 — Options cœur** | S4 | Réflexion/transparence 5/5, textures 5/5, disruptions, lumières spot/dir, AA, UI live | 1 scène `opt_*.rt` par option, testée |
| **J5 — Options étendues** | S5 | Limited objects, composés, négatif, natifs, effets visuels, perf | Scènes dédiées + mesures de perf affichées |
| **J6 — Soutenance** | S6 | Répétition, checklist, organisation, fallback | **Répétition complète réussie 2 fois** |

### 6.1 Chemin critique

```
J1 (squelette)  ──►  J2 (OBLIGATOIRE 100 %)  ──►  J3 (fichiers structurés)
                                                        │
                        ┌───────────────────────────────┴─────────────┐
                        ▼                                             ▼
               M3 : options lumières/UI                    M2 : objets composés/négatifs/natifs
                        │                                             │
                        └──────────────► J5 ◄─────────────────────────┘
                                             │
                                             ▼
                                        J6 soutenance
```

**Règle de gel** : à partir de **J6 − 5 jours**, **gel de fonctionnalités** : uniquement bugs,
scènes de démo et préparation de la soutenance. Ajouter une option au dernier moment est le
meilleur moyen d'introduire un crash (→ note 0).

---

## 7. Backlog résiduel : matrice d'écarts

> ⚠ **Audit statique** du code sur la branche `main` (lecture de fichiers, sans compilation ni
> exécution). **À revalider par un test** lors du J1 avant d'être considéré comme vrai.
>
> **Documents associés à ce backlog** : [OUTILS.md](OUTILS.md) (environnement et qualité),
> [MEMORY_STRATEGY.md](MEMORY_STRATEGY.md) (allocation, sanitizers, bug des exceptions),
> [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md) (cluster, 2 points),
> [INSPIRATION_BLENDER.md](INSPIRATION_BLENDER.md) (transferts d'architecture).
>
> Légende — **État** : ✔ probablement fait · △ partiellement fait · ✖ absent · ? à vérifier
> — **P** : priorité (P0 = bloquant noté, P1 = gros points, P2 = points moyens, P3 = dernier).

### 7.1 Partie obligatoire (éliminatoire)

| Réf | Exigence | État | Action | P | Membre |
|-----|----------|------|--------|---|--------|
| M2 | Ray tracing produisant une image | ✔ | Validation visuelle | P0 | M3 |
| M3 | 4 primitives + plusieurs du même type + intersection propre à chacune | △ | Vérifier sphère/plan aussi (tests centrés cylindre/cône) ; scène mixte | P0 | M2 |
| M3 | Intersections cohérentes entre objets | ? | Scène plan+sphère+cylindre+cône comparée à la démo | P0 | M2 |
| M4 | Translations **et rotations** des objets | △ | `Transform` existe ; **ajouter les directives** `translate/rotate` au parseur + scène de preuve `(42,42,42)` | P0 | M2+M3 |
| M5 | Œil n'importe où / n'importe quelle direction | ? | 2 scènes identiques, caméra seule changée | P0 | M3 |
| M6 | **Redisplay sans recalcul** (expose) | △ | Callback `EXPOSED` présent ; **garantir l'absence de `render()`** dans ce chemin + **mesurer** le gain | P0 | M3 |
| M7 | Luminosité, ombres, **multi-spot**, spéculaire | △ | Vérifier la **mélange** multi-lumières + scène image 3 | P0 | M3 |
| M8 | **3 scènes du sujet** reproduites | ✖ | Créer `01_…`, `02_…`, `03_…` | P0 | M2 |

### 7.2 Contrôles de base et règles

| Réf | Contrôle | État | Action | P | Membre |
|-----|----------|------|--------|---|--------|
| B1 | Quelque chose de soumis | ✔ | — | P0 | tous |
| B2 | Fichier `author` à la racine | ✖ | **Créer** (voir SPECIFICATIONS §8-O1) | P0 | responsable organisation |
| B3 | Norme OK | ? | **Arbitrer** (SPECIFICATIONS §8-O2) | P0 | tous |
| B4 | Tout le groupe présent | n/a | Réserver la date à 3 | P0 | responsable organisation |
| C1 | Aucun crash possible | ? | Fuzz des scènes + mauvais fichiers | P0 | M3 |

### 7.3 Options

| Item fiche | État | Action | P | Membre |
|------------|------|--------|---|--------|
| Scene files | ✔ | — | P1 | — |
| **File ++** | ✖ | Format hiérarchique (ARCHITECTURE §5) + migration des scènes | **P0/P1** | M3 |
| Ambiance light | ✔ (`A`) | Vérifier le rendu | P1 | M3 |
| **Ambiance ++** | △ | Bloc `ambient` dans le nouveau format | P1 | M3 |
| Limited objects | ✖ | Filtre de découpe en espace objet (5 sous-critères) | P2 | M2 |
| Disruptions | ✖ | Patterns procéduraux (sinus, damier, Perlin) | P2 | M3 |
| **Direct light** | ✖ | `light spot` + démo « aveuglé » (O4) | P1 | M3 |
| **Parallel light** | △ (`directional`) | Exposer dans le format + scène dédiée | P1 | M3 |
| **Reflection and transparency (5)** | △ | `reflect` ✔ ; **ajouter `transparency` + `ior` au parseur** ; % pilotables | **P1** | M3 |
| Shadows and transparency | ? | Moduler la visibilité par la transparence | P2 | M3 |
| Textures (5) | △ | `Texture` (PNG/JPEG) ✔ ; **directive `texture` au parseur** ; UV sur les 4 primitives ; `scale`/`offset` | **P1** | M2+M3 |
| More texture applications | ✖ | alpha, bump, mappage, découpe par texture | P2/P3 | M3 |
| Composed elements | ✖ | `Group` réutilisable + scène `opt_composed.rt` | P2 | M2 |
| Negative objects | ✖ | CSG difference/union | P2 | M2 |
| Simple native objects | ✖ | Paraboloïde **ou** hyperboloïde (un suffit) | P2 | M2 |
| Usual visual effects | ✖ | **Antialiasing** (le plus rentable), sépia, cartoon, motion blur, stéréo | P1/P2 | M3 |
| Technical effects | △ | multithreading ✔, screenshot ✔, **perf mesurée**, **cluster** (2 pts) → [DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md) | P1/P3 | M3 |
| Environment (5) | △ | microui ✔ ; **progress bar** ; chargement de fichier depuis l'UI ; **scripts** batch ; **générateur** de scènes | P1/P2 | M3 |
| **Group organization** | à construire | Ce document + journal | P1 | responsable organisation |
| Exotic objects | ✖ | tore (prioritaire), cube perforé, nappe, solveur d'équations | P3 | M2 |
| In bulk | △/✖ | soft shadows (`AreaLight` ✔, à exposer), vidéo, import pov/3ds | P2/P3 | M3 |
| Moebius ribbon | ✖ | Surface paramétrique | P3 | M2 |
| Caustics / GI | ✖ | Hors périmètre raisonnable sauf temps excédentaire | P3 | M3 |
| « Is it beautiful ? » | △ | Scènes de démo soignées, cadrage, couleurs | P2 | tous |

### 7.4 Estimation d'effort des items P0

| Tâche | Estimation | Membre |
|-------|-----------|--------|
| 3 scènes obligatoires | 3–4 h | M2 |
| Directive `translate/rotate` + preuve `(42,42,42)` | 3 h | M2+M3 |
| Expose : garantie sans recalcul + mesure | 2–3 h | M3 |
| Scène image 3 (mix d'ombres) + vérif multi-spot | 3 h | M3 |
| Fichier `author` + arbitrage norme | 1 h | resp. organisation |
| Fuzz / scènes invalides (anti-crash) | 4 h | M3 |
| **Total P0** | **~16–18 h** | — |

---

## 8. Quality gates (portes de qualité)

### 8.1 Avant chaque merge sur `develop`

- [ ] `make` compile **sans erreur ni warning** (`-Wall -Wextra -Werror`)
- [ ] `make test` → tous les tests verts
- [ ] `valgrind --leak-check=full ./rt scenes/default.rt 100 100` → **0 fuite**
- [ ] Le code a été **relu par un autre membre**
- [ ] Les méthodes publiques sont documentées (Doxygen)
- [ ] Aucune donnée partagée non protégée (si multithread)

### 8.2 Avant chaque revue hebdomadaire (vendredi)

- [ ] Démo **visuelle** à l'écran (pas de « ça devrait marcher »)
- [ ] Une scène de preuve a été **ajoutée au dépôt** si une option a été livrée
- [ ] Le backlog résiduel (§7) est **mis à jour** (états △/✔/✖)
- [ ] Les blockers sont listés

### 8.3 Avant la soutenance (détail : [CHECKLIST_DEFENSE.md](CHECKLIST_DEFENSE.md))

- [ ] Partie obligatoire **100 %** et revérifiée avec la fiche sous les yeux
- [ ] Aucun crash sur **toutes** les scènes du dépôt
- [ ] Scènes de démo **prêtes à calculer** (aucune image pré-rendue comme preuve)
- [ ] Répétition complète **2 fois**
- [ ] Les 3 membres connaissent **tout** le code (quiz croisé)

---

## 9. Risques et mitigations

| # | Risque | Impact | Probabilité | Mitigation |
|---|--------|--------|-------------|------------|
| R1 | Partie obligatoire non achevée → **0** | Catastrophique | Moyenne | Porte J2 stricte ; options gelées avant |
| R2 | Crash pendant la démo → **0** | Catastrophique | Moyenne | Fuzz des scènes, gestion d'erreurs, valgrind, gel 5 jours avant |
| R3 | Format de scène non structuré → `File ++` perdu | Élevé | Haute | J3 dédié, avant les options |
| R4 | Conflits de fusion entre 3 sur les mêmes fichiers | Moyen | Haute | Frontières §2.1, merges quotidiens, PR systématiques hors périmètre |
| R5 | Un membre bloque les 2 autres | Élevé | Moyenne | Règle « bloqué > 2 h → on en parle » ; backup désigné sur chaque module |
| R6 | Surestimation des options → fin de projet stressée | Moyen | Haute | Priorisation P0→P3, rétrospective hebdo, gel de fonctionnalités |
| R7 | Item *Group organization* raté | Moyen | Faible | Journal d'organisation tenu (§10) |
| R8 | Non-respect de la règle IA en soutenance | Élevé | Faible | Liste de transparence §5.3, quiz croisés |
| R9 | Norme / fichier `author` non conforme → **0** | Critique | Faille/faible | **Trancher immédiatement** (SPECIFICATIONS §8-O1/O2) |
| R10 | Le rendu est trop lent → item « vraiment rapide » perdu | Faible | Moyenne | Mesures continues, BVH/SAH, tiling, multithreading |

---

## 10. Preuves d'organisation à conserver

L'item *Group organization* est noté **subjectivement**. Voici ce qui rend la démonstration
crédible et objective :

| Preuve | Format | Fréquence |
|--------|--------|-----------|
| Journal de réunions | fichier `docs/JOURNAL.md` : date, participants, décisions, répartitions | à chaque réunion |
| Suivi des tâches | tableau (type `TASK_BACKLOG.md` ou GitHub issues) avec **assigné à** et **statut** | continu |
| Historique Git | branches `feature/<dev>/<feat>`, revues, messages conventionnels | continu |
| Mesures qualité | résultats `make test` / valgrind archivés par jalon | par jalon |
| Heures réellement passées | estimation vs réel par tâche | hebdo |
| Décisions d'architecture | un paragraphe daté dans les docs (ex. choix du format de scène) | au fil de l'eau |
| Rétrospectives | ce qui a bloqué / ce qui change | hebdo |

**Script type à raconter en soutenance (30 secondes) :**
> « Nous sommes 3. Chacun est responsable d'un module avec un relecteur désigné. Nous avons
> un kick-off hebdomadaire, un point quotidien asynchrone, des revues de code obligatoires
> avant merge et une démo vendredi. Le backlog est priorisé par la fiche d'évaluation :
> partie obligatoire d'abord (portes de qualité), puis les options par gain de points. Les
> conflits sont arbitrés immédiatement, et chaque fonctionnalité livrée est prouvée par une
> scène versionnée. »
