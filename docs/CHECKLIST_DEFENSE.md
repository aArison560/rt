# RT — Checklist de soutenance et script de démonstration

> Ce document sert **le jour J** et pendant les deux semaines qui précèdent.
> Il reprend **littéralement** les items de `docs/evalsheet/evalsheet.md` : chaque case ☐
> correspond à une case de la fiche.
>
> ⚠ Rappels fataux :
> - **Partie obligatoire imparfaite → 0.** Les options ne sont même pas regardées.
> - **Segfault / arrêt inattendu pendant toute la soutenance → 0.**
> - **Fichier `author` absent / norme KO / un membre absent → 0.**
> - **Triche → −42.**

---

## Sommaire

1. [Chronogramme J-15 → J](#1-chronogramme-j-15--j)
2. [Contrôles « Basic stuff »](#2-contrôles-basic-stuff)
3. [Script de la partie obligatoire](#3-script-de-la-partie-obligatoire)
4. [Script des options](#4-script-des-options)
5. [Checklist technique pré-démo](#5-checklist-technique-pré-démo)
6. [Répétition (mock defense)](#6-répétition-mock-defense)
7. [Questions fréquentes et réponses](#7-questions-fréquentes-et-réponses)
8. [Pièges qui coûtent la soutenance](#8-pièges-qui-coûtent-la-soutenance)
9. [Livrables du dépôt](#9-livrables-du-dépôt)

---

## 1. Chronogramme J-15 → J

| Quand | Action | Responsable |
|-------|--------|-------------|
| **J-15** | Figer la **liste des options** qui seront démontrées ; ne plus rien ajouter ensuite | tous |
| **J-14** | Arbitrer les **points ouverts** (`author`, norme, format de scène) — [SPECIFICATIONS §8](SPECIFICATIONS.md) | resp. qualité |
| **J-12** | Réaliser les **3 scènes obligatoires** et valider les 4 contrôles de la fiche | M2 + M3 |
| **J-10** | Créer les scènes `opt_*.rt` ; tester **chaque option** une par une | tous |
| **J-8** | **Nettoyage** : `make re`, valgrind sur **toutes** les scènes, aucun crash | resp. qualité |
| **J-7** | **Répétition n°1** complète (chronométrée) avec un membre en « correcteur » | tous |
| **J-6** | Corriger les points faibles révélés | concernés |
| **J-5** | **Répétition n°2** ; vérifier que chaque membre maîtrise tout le code | tous |
| **J-3** | Mettre à jour `README`, `author`, journal d'organisation, backlog | resp. organisation |
| **J-2** | **Gel de fonctionnalités** : uniquement bugs et scènes | tous |
| **J-1** | Dernière passe : build propre, scènes lancées 3× de suite sans erreur | resp. qualité |
| **J** | **Les 3 membres présents**, dépôt propre, matériel vérifié (voir §5) | tous |

---

## 2. Contrôles « Basic stuff »

> « If at least one isn't ok, defence is over and final grade is 0. »

| # | Contrôle de la fiche | Preuve à montrer | ✅ |
|---|----------------------|------------------|----|
| B1 | **Something was submitted** | `git log`, build fonctionnel | ☐ |
| B2 | **Author file at the root of the repository, formatted as explained in the subject** | fichier `author` à la racine du dépôt | ☐ |
| B3 | **Norm is OK** | commande/outil de norme exécuté devant le correcteur | ☐ |
| B4 | **The whole group is present** | **les 3 membres** | ☐ |
| B5 | Rien n'est vide / le dépôt est bien celui de l'équipe | `git remote -v`, clone dans un dossier vide | ☐ |

**Avant de partir, vérifier :**
- [ ] `git clone <repo> /tmp/rt_check && cd /tmp/rt_check && make && ./rt scenes/01_*.rt`
      fonctionne **depuis une copie fraîche** (le correcteur clone dans un dossier vide)
- [ ] Aucun **alias** ni script piégé détournant le contenu officiel du dépôt
- [ ] Tous les **scripts d'aide à la correction** ont été relus par l'équipe
- [ ] Aucun fichier `*.pdb` / binaire / artefact de build commité (`.gitignore` à jour)

---

## 3. Script de la partie obligatoire

> Déroulé exactement dans l'ordre de la fiche. Préparer **une commande par ligne**.

### 3.1 ⚠ Rappel permanent : pas de crash

> « No segfault, nor other unexpected, premature, uncontrolled or unexpected termination of the
> program, **else the final grade is 0**. This rule is active **throughout the whole defence**. »

- Ne jamais taper un chemin inexistant sans message d'erreur propre.
- Ne jamais ouvrir un fichier de scène corrompu **sans l'avoir testé**.

### 3.2 « Exposes without recalculation »

| Étape | Action du correcteur | Notre démonstration | ✅ |
|-------|----------------------|---------------------|----|
| 1 | Glisser une fenêtre **au-dessus** de `rt` | image retrouvée intacte | ☐ |
| 2 | Changer le **focus clavier** entre 2 fenêtres | l'image se redessine | ☐ |
| 3 | Chercher l'événement dédié (`mlx_expose_hook` ou équivalent) | montrer `EventHandler.cpp` : `SDL_WINDOWEVENT_EXPOSED` → callback dédié **qui n'appelle pas `render()`** | ☐ |
| 4 | Ajouter un `printf` dans le callback (correction en direct) | relancer → le `printf` s'affiche à chaque expose | ☐ |
| 5 | Vérifier que **le calcul n'est pas refait** | montrer : le callback ne fait que `SDL_UpdateTexture` + `RenderPresent` ; **afficher le temps : rendu XXX ms vs redraw X ms** | ☐ |

> **Préparer le script** : option `--debug-expose` ou un `printf` **temporaire** déjà présent
> que l'on active via variable d'environnement (`RT_DEBUG_EXPOSE=1`) — le correcteur peut
> l'activer sans recompiler.

### 3.3 « Objets »

| Critère de la fiche | Démonstration | ✅ |
|---------------------|---------------|----|
| Les **4 formes de base** présentes | scène `01_…` : plan, sphère, cylindre, cône visibles ensemble | ☐ |
| **Plusieurs objets du même type** coexistent | compter : 2 sphères, 2 cylindres… dans la scène | ☐ |
| **Fonction d'intersection propre à chacune** | ouvrir `src/geometry/Sphere.cpp`, `Plane.cpp`, `Cylinder.cpp`, `Cone.cpp` → 1 `intersect()` par classe | ☐ |
| **N'importe quelle position et direction** (translation + rotation) | scène dédiée : sphère à `(42,42,42)` ; cylindre/cône avec axes **inclinés** ; montrer le bloc `transform` (ou la directive) | ☐ |
| **Intersections cohérentes** (plan/sphère…) | zoom sur la jonction sphère ↔ plan : l'ombre et la coupe sont justes | ☐ |

### 3.4 « Did you know? » (œil libre)

| Critère | Démonstration | ✅ |
|---------|---------------|----|
| L'œil peut être **n'importe où**, regarder **n'importe où** | déplacer la caméra en direct (clavier/UI) dans les 6 directions | ☐ |
| **Image 2 = image 1, seul l'œil déplacé** | lancer `01_obligatoire_4objets.rt` puis `02_obligatoire_oeil_deplace.rt` : **seule la ligne `camera` diffère** (`diff` des deux fichiers devant le correcteur) | ☐ |

> **Astuce décisive** : montrer `diff scenes/01_*.rt scenes/02_*.rt` — une seule ligne différente.
> C'est la preuve la plus propre et la plus rapide de ce critère.

### 3.5 « Lights »

| Critère | Démonstration | ✅ |
|---------|---------------|----|
| **Brillance** (dégradé clair → sombre) | sphère éclairée par 1 spot : pointer le dégradé | ☐ |
| **Ombres** | plan + sphère : ombre nette | ☐ |
| **Effet de brillance / spéculaire** (petit point blanc) | orienter la caméra → le point spéculaire se déplace (Phong) | ☐ |
| **Multi-spot correct** : mélange, plusieurs dégradients, ombres selon les sources visibles | scène `03_obligatoire_mix_ombres.rt` (figure VI.3) : **2+ lumières**, zone d'ombre recouverte partiellement par la 2ᵉ lumière | ☐ |
| Mêmes objets / mêmes spots | revenir à la scène 1 pour montrer que c'est bien la **même** scène | ☐ |

### 3.6 Les 3 scènes obligatoires — preuve finale

```bash
./rt scenes/01_obligatoire_4objets.rt        # figure VI.1 : 4 objets, 2 spots, ombres + brillance
./rt scenes/02_obligatoire_oeil_deplace.rt    # figure VI.2 : même scène, autre point de vue
./rt scenes/03_obligatoire_mix_ombres.rt      # figure VI.3 : mélange d'ombres
```

- [ ] Les 3 scènes sont **dans le dépôt**, nommées clairement
- [ ] Elles se lancent **en une commande** depuis la racine
- [ ] Elles **ressemblent** aux figures du sujet (garder le PDF ouvert à côté pour comparer)

---

## 4. Script des options

> Rappel : « You will have to **demonstrate all your options** … Get prepared with multiple
> configured scenes, ready to be calculated. »

### 4.1 Matrice de démonstration

Remplir **avant** la soutenance. Colonne « commande » = ce qu'on tape devant le correcteur.

| # | Option (fiche) | Points | Scène / commande | Valeur que le correcteur peut changer | ✅ |
|---|----------------|--------|------------------|----------------------------------------|----|
| 1 | Scene files | 1 | `./rt scenes/01_*.rt` | toute valeur du fichier | ☐ |
| 2 | File ++ | 1 | `cat scenes/opt_*.rt` | — (hiérarchie visible) | ☐ |
| 3 | Ambiance light | 1 | `opt_ambiance.rt` | — | ☐ |
| 4 | Ambiance ++ | 1 | `opt_ambiance.rt` | `ambient { intensity }` | ☐ |
| 5 | Limited objects | 0–5 | `opt_limited_objects.rt` | `slice { axis, min, max }` | ☐ |
| 6 | Disruptions | 0–5 | `opt_disruptions.rt` | `pattern { type, scale }` | ☐ |
| 7 | Direct light | 1 | `opt_spot.rt` | position/angle du spot | ☐ |
| 8 | Parallel light | 1 | `opt_directionnelle.rt` | `direction` | ☐ |
| 9 | Reflection & transparency | 0–5 | `opt_reflexion_transparence.rt` | `reflect`, `transparency`, `ior` | ☐ |
| 10 | Shadows and transparency | 1 | `opt_ombres_transparentes.rt` | `transparency` de la sphère | ☐ |
| 11 | Textures | 0–5 | `opt_textures.rt` | `scale`, `offset`, chemin png/jpg | ☐ |
| 12 | More texture applications | 0–5 | `opt_texture_avancee.rt` | `bump`, `alpha` | ☐ |
| 13 | Composed elements | 1 | `opt_composes.rt` | position du groupe instancié | ☐ |
| 14 | Negative objects | 1 | `opt_negatif.rt` | rayon de la sphère négative | ☐ |
| 15 | Simple native objects | 1 | `opt_natifs.rt` | — | ☐ |
| 16 | Usual visual effects | 0–5 | `opt_effets_visuels.rt` | `post { filter … }`, spp | ☐ |
| 17 | Technical effects | 0–5 | UI + `scripts/batch_render.sh` | spp, threads, `--tile` | ☐ |
| 18 | Environment | 0–5 | UI (progress, load) + scripts | — | ☐ |
| 19 | Group organization | 1 | `docs/JOURNAL.md` + git log | — | ☐ |
| 20 | Exotic objects | 0–5 | `opt_tore.rt`, `opt_cube_perfore.rt` | — | ☐ |
| 21 | In bulk | 0–5 | `opt_area_softshadows.rt`, vidéo, import | taille de la source | ☐ |
| 22 | Moebius ribbon | 1 | `opt_mobius.rt` | — | ☐ |
| 23 | Caustics / GI | 1 | `opt_caustiques.rt` | — | ☐ |
| 24 | Is it beautiful? | 1 | scènes de démo soignées | — | ☐ |

### 4.2 Ordre de passage recommandé (20–25 min)

```
1.  Rappel du contexte (30 s) .............. « Ray tracer en C++23, CPU, 3 personnes »
2.  Partie obligatoire (5 min) ............. scènes 01 → 02 (diff) → 03
    + expose avec mesure rendu vs redraw
    + transformations (42,42,42)
3.  Fichiers de scène (2 min) .............. hiérarchie, puis modification en direct
4.  Lumière (2 min) ....................... spot / directionnel / ambiance
5.  Matériaux (4 min) ..................... réflexion, transparence, ior, ombre transparente
6.  Textures & patterns (4 min) ........... 4 primitives, scale/offset, damier/sinus/Perlin
7.  Objets (4 min) ....................... slicing, composés, négatifs, natifs
8.  Effets & perfs (3 min) ................ AA/filtres, UI + progression, screenshot, multithread
9.  Exotiques si temps (2 min) ............ tore, Möbius, soft shadows
10. Organisation du groupe (2 min) ........ journal, git, répartition
11. Bilan / questions (reste du temps)
```

> **Règle de priorité** : si le temps manque, **abandonner la fin de la liste**, jamais le
> début. Les items obligatoires et les options binaires faciles viennent toujours en premier.

---

## 5. Checklist technique pré-démo

### 5.1 Build et qualité

- [ ] `make fclean && make` → **0 warning**, 0 erreur
- [ ] `make test` → tous les tests verts
- [ ] `valgrind --leak-check=full ./rt scenes/<chacune>.rt 100 100` → **0 fuite**
- [ ] `make asan` (ASan + UBSan) et **`make tsan`** (rendu multithread) → 0 erreur
      → flags et cibles : [OUTILS.md §8](OUTILS.md)
- [ ] **Aucune exception** n'est levée dans la boucle de rendu
      (`Vec3::operator/`, `normalize()`, `Matrix4x4::inverse()`) → [MEMORY_STRATEGY.md §3](MEMORY_STRATEGY.md)
- [ ] `./rt` sans argument → comportement défini (message d'aide ou scène par défaut), **pas de crash**
- [ ] `./rt fichier_inexistant.rt` → message d'erreur + exit code ≠ 0, **pas de crash**
- [ ] `./rt fichier_corrompu.rt` → message avec **numéro de ligne**, **pas de crash**
- [ ] `./rt scenes/x.rt 0 0` / résolution absurde → comportement sûr

### 5.2 Scènes

- [ ] **Toutes** les scènes du dossier `scenes/` se lancent sans erreur
- [ ] Les 3 scènes obligatoires sont présentes et ressemblent aux figures du sujet
- [ ] Une scène `opt_*.rt` **par option** cochée dans §4.1
- [ ] Aucune scène ne dépend d'un chemin absolu ni d'un fichier absent du dépôt
- [ ] Les textures utilisées sont **dans le dépôt** (`textures/`)

### 5.3 Environnement de démo

- [ ] Machine chargée, **alimentation** branchée, notifications coupées
- [ ] Écran/résolution identiques à ceux de la répétition
- [ ] Repo **clone à jour** (`git pull`) et buildé sur la machine de démo
- [ ] Sauvegarde : vidéo/image des 3 scènes obligatoires (en secours, **jamais comme preuve unique**)
- [ ] Fichiers ouverts à côté : sujet PDF, fiche d'évaluation, backlog
- [ ] Les **3 membres** savent manipuler l'interface et lancer les scènes

---

## 6. Répétition (mock defense)

**Deux répétitions complètes obligatoires** (J-7 et J-5), chronométrées.

### Protocole

1. Un membre joue le **correcteur**, suit la fiche **à la lettre**, sans indulgence.
2. Il tente volontairement de :
   - lancer une scène **inexistante**,
   - modifier une valeur au hasard dans un fichier,
   - glisser une fenêtre au-dessus,
   - demander « montre-moi le code de l'intersection du cône »,
   - demander « et ça, c'est toi qui l'as écrit ou l'IA ? ».
3. **Chronomètre** : objectif ≤ 25 min pour les points obligatoires + options principales.
4. Comptage : combien de cases ☐ cochées à la fin → **objectif 100 % sur l'obligatoire**.
5. Rétro immédiate : liste des incidents (hésitations, plantages, réponses approximatives).

### Quiz croisé (chaque membre, les 3 modules)

À 48 h de la soutenance, chaque membre doit répondre **sans aide** :

| Question | Attendu |
|----------|---------|
| Où se fait le calcul d'intersection du cône ? Quelle équation ? | `Cone::intersect`, forme quadratique + test de hauteur + base |
| Pourquoi un `EPSILON` sur l'origine des rayons ? | éviter l'auto-intersection (*shadow acne*) |
| Où est géré l'expose ? Pourquoi pas de recalcul ? | `EventHandler` → callback → reblit du framebuffer |
| Quelle est la différence entre `reflectivity` et `transparency` dans le shading ? | Fresnel + pondération récursive |
| Comment ajoute-t-on une nouvelle primitive ? | classe `AObject` + `intersect()` + parser + scène |
| Comment prouve-t-on le multithreading ? | `ThreadPool`, affichage du nombre de threads, temps de rendu |
| Quel est le format du fichier `author` ? | réponse attendue (voir SPECIFICATIONS §8-O1) |
| Quelles options sont dans quel fichier de scène ? | cartographie complète §4.1 |

---

## 7. Questions fréquentes et réponses

| Question probable | Réponse préparée |
|-------------------|------------------|
| « Montre-moi où le rayon est généré. » | `Camera::generateRay` / `Renderer::render` — base orthonormée + FOV |
| « Pourquoi ce choix d'architecture ? » | séparation calcul / affichage (expose), décorateurs pour CSG et slicing, points d'extension listés dans `ARCHITECTURE.md §10` |
| « Pourquoi CPU et pas GPU ? » | le sujet **interdit** le rendu par pipeline GPU ; on a gardé le calcul CPU (+ parallélisé) |
| « Tu utilises l'IA ? » | réponse honnête et structurée ([PLAN_TRAVAIL §5](PLAN_TRAVAIL.md)) : ce qui a été généré, ce qui a été relu, ce que chacun sait expliquer |
| « Combien de temps par semaine ? » | chiffres réels du journal |
| « Pourquoi ce format de scène ? » | exigence `File ++` de la fiche + zéro dépendance |
| « Comment ça se comporte sur un fichier invalide ? » | démo en direct : message + numéro de ligne + code retour |
| « Explique la formule de réfraction. » | Snell/Descartes + Fresnel Schlick + cas de réflexion totale interne |
| « Pourquoi ta BVH est-elle construite ainsi ? » | SAH, invalidation par `objectVersion` |
| « Qu'est-ce que tu ferais si tu avais 10 de plus ? » | items P3 : cluster, GI, import .pov/.3ds (cf. OPTIONS_GUIDE §9–11) |

---

## 8. Pièges qui coûtent la soutenance

| # | Piège | Conséquence | Parade |
|---|-------|-------------|--------|
| 1 | `./rt` lancé avec un chemin fausse → **segfault** | **0** | messages d'erreur + tests de robustesse (§5.1) |
| 2 | Une scène manque dans le dépôt | option non notée | scènes versionnées + test post-clone |
| 3 | Option prouvée par une **image PNG pré-rendue** | non acceptée | toujours recalculer en direct |
| 4 | Option « codée en dur », modifiable qu'en recompilant | non notée | piloter par fichier / UI |
| 5 | Un membre absent | **0** | réserver la date à 3 |
| 6 | Fichier `author` absent/mal formaté | **0** | SPECIFICATIONS §8-O1 |
| 7 | Norme non conforme | **0** | trancher au J-14 (§8-O2) |
| 8 | Dépôt non vérifié (mauvais repo, alias) | 0 / −42 | `git remote -v`, clone test |
| 9 | Ajouter une option la veille | crash le jour J | **gel J-2** |
| 10 | Ne pas savoir expliquer son propre code | item *Group organization* + crédibilité | quiz croisés (§6) |
| 11 | Expose qui **recalcule** quand même | case obligatoire à Non → **0** | mesure rendu vs redraw (§3.2) |
| 12 | Scène 2 ≠ scène 1 (objet déplacé entre-temps) | case « Did you know? » à Non | `diff` des deux fichiers (§3.4) |

---

## 9. Livrables du dépôt

### 9.1 Indispensables (sinon risque de 0)

| Livrable | Où | ✅ |
|----------|-----|----|
| Code buildable, exécutable nommé **`rt`** | racine | ☐ |
| Fichier **`author`** à la racine | racine | ☐ |
| **README** : build, usage, format de scène | racine | ☐ |
| **3 scènes obligatoires** | `scenes/01…03_*` | ☐ |
| Tous les **scripts** utilisés (relus par l'équipe) | `scripts/` | ☐ |

### 9.2 Indispensables pour les points

| Livrable | Où | ✅ |
|----------|-----|----|
| 1 scène `opt_*.rt` **par option** | `scenes/` | ☐ |
| Textures de démonstration | `textures/` | ☐ |
| Script de rendu automatique (**Environment 4**) | `scripts/batch_render.sh` | ☐ |
| Générateur de scènes (**Environment 5**) | `scripts/gen_scene.py` | ☐ |
| Scène cluster / mode `--tile` (**Technical 1**) | code + doc | ☐ |

### 9.3 Pour l'organisation et la crédibilité

| Livrable | Où | ✅ |
|----------|-----|----|
| **Journal d'organisation** (réunions, décisions) | `docs/JOURNAL.md` | ☐ |
| **Backlog** à jour (assignations + statuts) | `docs/` ou issues | ☐ |
| **Transparence IA** (ce qui a été généré / relu) | `docs/` ou README | ☐ |
| Historique Git propre (branches, revues, messages) | `git log --graph` | ☐ |
| Cette documentation à jour | `docs/*.md` | ☐ |

---

## 10. La checklist du matin (à imprimer)

```
□ Dépôt à jour, make fclean && make → 0 warning
□ make test → vert
□ valgrind → 0 fuite sur les scènes clés
□ Les 3 scènes obligatoires se lancent en 1 commande
□ Toutes les scènes opt_* se lancent sans erreur
□ Fichier author présent à la racine
□ Scénarios d'erreur testés (fichier absent, fichier corrompu)
□ Les 3 membres sont présents
□ Sujet + fiche ouverts à côté
□ Répétition n°2 réalisée avec succès

-> On est prêts.
```
