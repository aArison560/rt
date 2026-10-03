# RT — Spécifications complètes (sujet + fiche d'évaluation)

> Sources : `docs/subjects/fr.subject.pdf` (sujet v4.1) et `docs/evalsheet/evalsheet.md` (42 EvalHub).
> Ce document est **la référence unique** pour décider ce qui est fait, dans quel ordre, et ce qui
> est noté. Toute tâche du plan de travail doit pouvoir être reliée à un item de ce document.

---

## Sommaire

1. [Le projet en une phrase](#1-le-projet-en-une-phrase)
2. [Règles générales et contraintes techniques](#2-règles-générales-et-contraintes-techniques)
3. [Partie obligatoire (éliminatoire, 0 point)](#3-partie-obligatoire-éliminatoire-0-point)
4. [Règles de la soutenance et flags d'arrêt](#4-règles-de-la-soutenance-et-flags-darrêt)
5. [Matrice complète des options et des points](#5-matrice-complète-des-options-et-des-points)
6. [Bonus et plafond de note](#6-bonus-et-plafond-de-note)
7. [Remise, dépôt Git et preuves acceptées](#7-remise-dépôt-git-et-preuves-acceptées)
8. [Points ouverts à trancher par l'équipe](#8-points-ouverts-à-trancher-par-léquipe)

---

## 1. Le projet en une phrase

> Générer des **images de synthèse** par **ray tracing** (CPU), à partir d'une scène décrite par des
> objets géométriques simples et des sources lumineuses, avec une caméra déplaçable.

Contrairement à `miniRT`, on attend un **second traceur abouti** : C, C++ ou Rust, beaucoup
d'options, et une démonstration en direct pendant la soutenance.

**Mécanique de notation à connaître par cœur :**

```
Partie obligatoire  = 0 point, mais ÉLIMINATOIRE (100 % requise)
Options             = seule source de points, et elles ne sont évaluées
                      QUE si la partie obligatoire est PARFAITE
Bonus               = au-delà, plafond final 125
```

Conséquence directe : **ne jamais commencer les options avant d'avoir stabilisé la partie
obligatoire**, mais **concevoir l'architecture pour les options dès le départ** (c'est le piège
classique : devoir tout réécrire pour ajouter la transparence ou les textures).

---

## 2. Règles générales et contraintes techniques

### 2.1 Environnement de compilation et d'exécution

| Contrainte | Détail |
|------------|--------|
| Langage | **C, C++ ou Rust**, version récente du langage + bonnes pratiques actuelles |
| Exécutable | Doit s'appeler **`rt`** |
| Fuites mémoire | **Interdites** (`valgrind` propre) |
| Organisation des fichiers | Libre (« you are free to organise and name your files as you want ») |
| Correction | Par des **humains** uniquement |

### 2.2 Bibliothèques autorisées

| Périmètre | Autorisé |
|-----------|----------|
| Partie obligatoire | **libc entière**, **libstdc++ entière** (ou équivalents Rust) |
| Maths | Toutes les fonctions de `libm` (`-lm`) |
| Formats d'image | Bibliothèques natives externes : **libpng, libjpeg**, … |
| Affichage | Toutes les fonctions de la **MiniLibX**, ou équivalent dans une autre bibliothèque graphique (**SDL, XCB**, …) : ouvrir une fenêtre, écrire un pixel, afficher une image, gérer les événements |
| Bonus (hors options) | Autres fonctions/bibliothèques **si justifiées pendant la soutenance** |
| Calcul GPU | **Autorisé** pour la performance : OpenCL, CUDA, compute shaders (OpenGL/Vulkan/Metal) |

### 2.3 Interdits

| Interdit | Précision |
|----------|-----------|
| **Rendu par pipeline GPU** | L'image finale ne doit **pas** être produite par des shaders vertex/fragment/geometry (OpenGL, Metal, Vulkan, DirectX…). Le calcul des pixels doit rester côté **CPU** (le GPU ne peut être utilisé que pour *calculer*, pas pour *afficher* le rendu final). |
| Triangles/vertex pour les objets obligatoires | Si vous importez des `.pov` / `.3ds`, les objets simples du mandat doivent être gérés **par équations**, pas par maillage. |
| Images pré-rendues comme preuve | Voir [section 7](#7-remise-dépôt-git-et-preuves-acceptées). |

> **Point de vigilance architecture** : toute la chaîne actuelle (CPU + framebuffer CPU + texture
> SDL affichée) respecte la règle. Ne pas introduire de rendu final via shader.

### 2.4 Qualité de code attendue

- Bonnes pratiques à jour du langage choisi (smart pointers, RAII, `const`/`[[nodiscard]]`, …).
- Pas de fuite mémoire, gestion d'erreurs propre.
- **Zéro crash** pendant toute la soutenance (voir [section 4](#4-règles-de-la-soutenance-et-flags-darrêt)).

---

## 3. Partie obligatoire (éliminatoire, 0 point)

> « The mandatory part is worth 0 points and options will only bring you points IF the mandatory
> part is 100% complete. »

Si **un seul** élément manque, la soutenance s'arrête et la note finale est **0**.

### 3.1 Liste exhaustive des exigences obligatoires

| # | Exigence (sujet) | Vérification pendant la soutenance (fiche) |
|---|------------------|--------------------------------------------|
| M1 | Code en **C, C++ ou Rust**, dernière version du langage, bonnes pratiques | Présentation du code |
| M2 | Implémenter la méthode du **ray tracing** pour créer une image de synthèse | Images affichées à l'écran |
| M3 | Au moins **4 objets simples non composés** : **plan, sphère, cylindre, cône** | Voir §3.2 « Objets » |
| M4 | **Translations et rotations** appliquées aux objets avant affichage (ex. : sphère à `(0,0,0)` déplaçable en `(42,42,42)`) | Voir §3.2 « Objets » |
| M5 | **Position et direction de la caméra/œil** modifiables facilement | Voir §3.2 « Did you know? » |
| M6 | **Redessiner la vue (ou une partie) sans recalculer l'image entière** (ex. : `mlx_expose_hook` avec une fonction dédiée) | Voir §3.2 « Exposes without recalculation » |
| M7 | **Gestion de la lumière** : luminosités différentes, ombres, multi-spot, effet de brillance (spéculaire) | Voir §3.2 « Lights » |
| M8 | **Reproduire les 3 scènes** du sujet (figures VI.1, VI.2, VI.3) | « The subject requires 3 scenes to validate quickly and easily the mandatory part » |

### 3.2 Les 4 contrôles obligatoires de la fiche

Chaque bloc a une case ☐ Yes / ☐ No. **Tous doivent être Yes.**

#### a) « Exposes without recalculation » (M6)

Procédure du correcteur :
1. Glisser une fenêtre **au-dessus** de la fenêtre `rt`, puis changer le focus clavier d'une fenêtre à l'autre.
2. Constater que l'image **se redessine** (ou non).
3. Vérifier qu'il y a bien une **gestion d'événement dédiée** : `mlx_expose_hook` avec une fonction dédiée (ou l'équivalent SDL : `SDL_WINDOWEVENT_EXPOSED`).
4. **Preuve par le code** : le correcteur ajoutera un `printf` à chaque expose pour vérifier l'événement, puis regardera si le calcul est rejoué.
5. **Critère décisif** : le redisplay doit être **plus rapide** sans recalcul. « The great classic is the use of images of the minilibX » → conserver le **framebuffer rendu** et le re-copier.

> **Implémentation attendue** : framebuffer persistant en mémoire + callback expose qui
> **reblit** l'image existante (aucun appel à `Renderer::render()` dans ce chemin).

#### b) « Objets »

- Les **4 formes de base** présentes.
- Elles peuvent coexister **dans la même scène**.
- **Plusieurs objets du même type** peuvent coexister.
- **Chaque objet possède sa propre fonction d'intersection simple** (pas une macro générique).
- Tous les objets peuvent être à **n'importe quelle position et direction** (subir translations et rotations).
- Les **intersections entre objets sont cohérentes** (plan/sphère doit ressembler à la démo/vidéo d'intro).

#### c) « Did you know? » (caméra libre)

- L'œil peut être placé **n'importe où** dans la scène et regarder **n'importe quelle direction**.
- **Preuve décisive** : l'image 2 doit être **la même scène que l'image 1**, avec **seul l'œil déplacé**.

> **Action concrète** : prévoir **2 fichiers de scène identiques** ne différant que par la
> directive caméra. C'est le test exact que fera le correcteur.

#### d) « Lights »

- **Brillance** présente : dégradé de couleur sur l'objet, du côté le plus clair côté spot jusqu'au plus sombre côté non éclairé.
- **Ombres** présentes.
- **Effet de brillance / spéculaire** : la couleur du spot s'ajoute à celle de l'objet → petit point blanc (saturation).
- **Gestion multi-spot correcte** : luminosités mélangées, plusieurs dégradients selon la position des spots, ombres assombries selon le nombre de sources visibles/saturées.
- **Preuve** : l'image 3 du sujet (« Shadow mixing »).

### 3.3 Les 3 scènes de référence (M8)

Le sujet **conseille vivement** de reproduire au moins ces 3 scènes :

| Scène | Description (figure du sujet) | Ce qu'elle prouve |
|-------|-------------------------------|-------------------|
| **Scène 1** — `figure VI.1` | Les 4 objets de base, 2 spots, ombres et brillance | M3, M4, M7 |
| **Scène 2** — `figure VI.2` | **La même scène**, vue depuis un autre angle | M5 (caméra libre, « Did you know? ») |
| **Scène 3** — `figure VI.3` | Mélange d'ombres | M7 (multi-spot, mélange d'ombres) |

> Ces 3 scènes sont le **minimum vital** : elles servent de preuve rapide pour toute la partie
> obligatoire. Elles doivent être versionnées dans le dépôt et se lancer en **une commande**.

---

## 4. Règles de la soutenance et flags d'arrêt

### 4.1 Flags d'arrêt (note = 0, ou −42)

| Flag | Déclencheur | Conséquence |
|------|-------------|-------------|
| **Segfault / crash** | « no segfault, nor other unexpected, premature, uncontrolled or unexpected termination of the program » — **actif pendant toute la soutenance** | Note finale **0** |
| **Dépôt vide** | Repository vide | **0**, défense terminée |
| **Programme non fonctionnel** | — | **0**, défense terminée |
| **Erreur de norme** | Norm check (voir [§8](#8-points-ouverts-à-trancher-par-léquipe)) | **0**, défense terminée |
| **Triche / cheats** | Détectée | **−42** |

> Même en cas de flag, **poursuivre la discussion** est encouragé (sauf triche) : l'objectif est
> d'identifier les problèmes pour ne pas les reproduire.

### 4.2 Contrôles « Basic stuff » (tous obligatoires)

| # | Contrôle | Point de vigilance pour l'équipe |
|---|----------|----------------------------------|
| B1 | **Quelque chose a été soumis** | Le dépôt contient du code buildable |
| B2 | **Fichier `author` à la racine**, au format expliqué dans le sujet | Voir [§8 — point ouvert 1](#8-points-ouverts-à-trancher-par-léquipe) |
| B3 | **Norme OK** (« using the norminette ») | Voir [§8 — point ouvert 2](#8-points-ouverts-à-trancher-par-léquipe) |
| B4 | **Tout le groupe est présent** | **Les 3 membres doivent assister à la soutenance** |

> **Un seul échec sur ces 4 points = défense terminée, note 0.**

### 4.3 Règles de bon comportement (Introduction de la fiche)

- Politesse, respect, constructivité pendant toute la correction.
- Discuter et débattre des dysfonctionnements identifiés.
- Rester ouvert : les pairs ont pu comprendre le sujet autrement.

### 4.4 Vérifications faites par le correcteur (Guidelines)

1. Seul le contenu **du dépôt Git de l'étudiant/de l'équipe** est noté.
2. Vérification que le dépôt **appartient bien** à l'équipe et au bon projet ; `git clone` dans un **dossier vide**.
3. Vérification qu'**aucun alias malveillant** ne détourne l'évaluation du contenu officiel.
4. Relecture des scripts utilisés pour faciliter la correction (les deux parties doivent les avoir relus).
5. Si le correcteur n'a pas fait le projet, il doit avoir **lu l'intégralité du sujet** avant la défense.

### 4.5 Pièges classiques à éviter (synthèse)

- `./rt` qui plante sur un fichier de scène mal formé → **0**.
- Fichier de scène qui n'existe pas dans le dépôt → preuve manquante.
- Fonctionnalité démontrée uniquement via une image PNG pré-rendue → **non acceptée**.
- Une option « marche » mais n'est **modifiable que par recompiler** → non notée (il faut la piloter depuis un fichier ou en direct).
- Oublier un membre du groupe ce jour-là → **0**.

---

## 5. Matrice complète des options et des points

> ⚠ Rappel : **aucune option n'est évaluée si la partie obligatoire n'est pas parfaite.**
>
> Deux types de notation :
> - **Oui/Non** : 1 item binaire.
> - **Score 0 à 5** : 1 point par sous-critère atteint (0 = Failed … 5 = Excellent).

### 5.1 Lecture de la grille

| Bloc (fiche) | Items | Type |
|--------------|-------|------|
| §4 Options | 18 items | mixte |
| §5 Interlude | 1 item (organisation du groupe) | Oui/Non |
| §6 More options | 5 items | mixte |

### 5.2 Options — détail item par item

#### A. Fichiers de scène

| Item | Type | Sous-critères / preuve |
|------|------|------------------------|
| **Scene files** | Oui/Non | Il existe un **fichier de description de scène**. |
| **File ++** | Oui/Non | Les fichiers sont en **XML**, ou suivent une **structure/hiérarchie appropriée**. « Ce n'est pas juste un fichier avec une information par ligne ou des blocs de base séparés par une ligne vide ». |

> **Écart à combler** : le format `.rt` actuel est *une directive par ligne* → il **ne passe pas**
> `File ++`. Voir la proposition de format structuré dans [ARCHITECTURE.md §5](ARCHITECTURE.md).

#### B. Ambiance

| Item | Type | Sous-critères |
|------|------|---------------|
| **Ambiance light** | Oui/Non | Aucun objet n'est **jamais vraiment dans le noir** (lumière ambiante globale). |
| **Ambiance ++** | Oui/Non | De **meilleurs points** si l'ambiance se pilote **depuis le fichier de configuration**. |

#### C. Limited objects (objets limités / tranchés)

**1 point par sous-critère (0→5)** :

1. Possibilité de **trancher (slice) les objets sur les axes x, y, z**.
2. Possibilité de choisir le tranché **en coordonnées objet ou monde** (un cylindre peut être tranché selon **son propre axe** ou selon un **axe réel**).
3. **Rotations et translations continuent de fonctionner** après le tranché.
4. L'effet de tranché est **propre à chaque objet**, pas appliqué uniformément à tous.
5. Possibilité de trancher **autrement que selon les axes** (ex. : limiter sur x et y donne un carré) : **triangle, disque**, etc.

#### D. Disruptions (perturbations / textures procédurales)

**1 point par perturbation implémentée (0→5)** :

1. **Perturbation de normale** : par ex. avec `sinus` → effet vague/onde.
2. **Perturbation de couleur** : *checkerboard* (damier).
3. **Perturbation de couleur** : algorithme plus compliqué.
4. **Perturbation de couleur** : algorithme très compliqué, ex. **bruit de Perlin** → **2 points**, sauf s'il est le seul implémenté (alors ne pas compter le dernier point).

#### E. Lumières supplémentaires

| Item | Type | Sous-critères |
|------|------|---------------|
| **Direct light** | Oui/Non | On est **aveuglé par un spot qui nous fait face** (lumière vers l'observateur — *headlight / spotlight orienté vers la caméra*). |
| **Parallel light** | Oui/Non | Une lumière **parallèle** éclaire la scène selon une **direction précise** (≠ un spot qui émet vers un point). |

#### F. Réflexion & transparence

**1 point par option implémentée (0→5)** :

1. La **réflexion fonctionne** → effet miroir.
2. Possibilité de changer le **% de réflexion** (pas tout ou rien).
3. La **transparence fonctionne** → on voit à travers.
4. L'**indice de réfraction** fonctionne (si besoin, vérifier la **formule de Descartes** dans le code).
5. Possibilité de changer le **% de transparence**.

| Item adjacent | Type | Sous-critères |
|---------------|------|---------------|
| **Shadows and transparency** | Oui/Non | L'ombre est **plus ou moins assombrie selon la transparence** de l'objet. |

#### G. Textures

**1 point par sous-critère (0→5)** :

1. Appliquer une texture sur **au moins 1 des 4 objets** de base.
2. Appliquer une texture sur **les 4 objets** de base.
3. Possibilité d'**étirer** (ou l'inverse) une texture sur un objet.
4. Possibilité de **décaler** une texture sur un objet.
5. Utiliser une **autre bibliothèque que MiniLibX et ses XPM** pour charger la texture (**jpeg, png**, …).

#### H. Plus d'applications de textures

**1 point par effet (0→5)** :

1. Une texture peut être ** projetée / grossièrement mappée** sur un objet.
2. Une texture peut être utilisée pour sa **transparence** (alpha).
3. Une texture peut **perturber la normale** d'un objet (*bump mapping*).
4. Une texture peut modifier [la couleur/l'albedo] **à certains endroits** d'un objet.
5. Une texture peut **limiter ou trancher** un objet.
6. Un objet **semi-transparent sert de diapositive** (« slide ») devant un autre objet.

#### I. Composition et négatif

| Item | Type | Sous-critères |
|------|------|---------------|
| **Composed elements** | Oui/Non | Définir un élément composé à partir d'objets simples (cube = 6 plans limités ; « verre » = cône limité + cylindre + sphère). **Réutilisable plusieurs fois** à des positions/orientations différentes. |
| **Negative objects** | Oui/Non | **Soustraire** un objet à un autre : sphère négative qui perce un trou ou déforme un plan ; cylindre qui se creuse perpendiculairement à un autre cylindre. |
| **Simple native objects** | Oui/Non | Objets de complexité ≤ sphère/cylindre/cône (deuxième degré) : surtout **paraboloïde** et **hyperboloïde**. Un seul suffit. |

#### J. Effets visuels usuels

**1 point par effet (0→5)** :

1. **Antialiasing**
2. **Cartoon effect** (quantification des couleurs + contours)
3. **Motion blur**
4. **Sépia ou tout autre filtre de couleur**
5. **Stereoscopie simple** (lunettes rouge/cyan)

#### K. Effets techniques

**1 point par effet (0→5)** :

1. **Rendu groupé (clustering)** sur plusieurs ordinateurs → **2 points**
2. **Multi-thread**
3. Le rendu est **vraiment rapide**
4. Possibilité, **dans le RT**, de **sauvegarder / screenshot** l'image rendue

#### L. Environment (environnement / interface)

**5 possibilités pour 5 points** :

1. Une **interface de synthèse** : message de chargement graphique, **barre de progression**, plus que des messages terminal.
2. Une **jolie interface** (gtk ou QT) avec des éléments de configuration : chargement de fichier, contrôle du rendu, etc. (si vrai, compter aussi le point 1).
3. Possibilité d'**interagir avec la scène en live** (caméra, position d'objet, couleurs, textures…) **sans relancer le programme**.
4. Possibilité de **rendre automatiquement avec modifications entre les rendus** (pas d'interface nécessaire ; une série de scripts peut convenir).
5. Possibilité de **rendre automatiquement des objets pour une scène** (ex. : tore composé d'une série de sphères, hélice faite de sphères et de cylindres…).

### 5.3 Interlude

| Item | Type | Critère |
|------|------|---------|
| **Group organization** | Oui/Non | Évaluer **comment le groupe s'est organisé** pour travailler. Beaucoup de scénarios sont acceptables, rester ouvert. **Ne pas valider** si le groupe est désorganisé et n'a montré ni organisation ni gestion du temps. Jugement objectif. |

> ⚠ Item **purement subjectif mais décisif** : il faut pouvoir **raconter** son organisation
> (réunions, répartition, revues de code, gestion des conflits). Voir [PLAN_TRAVAIL.md](PLAN_TRAVAIL.md).

### 5.4 More options

#### Exotic objects — 1 point par objet (0→5)

1. Cube perforé (*perforated cube*)
2. Nappe de table (*table cloth* — surface paramétrique)
3. **Tore** (*torus*)
4. **Résolution d'équations aléatoires depuis un fichier de configuration** (la lib GNU le fait bien)
5. Autre (objets **fractals**, etc.)

#### In bulk — 1 point par option (0→5)

1. Une **vidéo** réalisée à partir de votre RT (à partager sur le forum)
2. **Fichiers de modeleurs** : import de fichiers **pov** ou **3ds** (par ex.) rendus par votre RT
3. Technologie **3D TV** ou **Oculus Rift**
4. **Spot non ponctuel** : ex. filament d'une ampoule, la source est étendue et les ombres n'ont pas de netteté (*soft shadows*)
5. Tout autre **truc de fou**

#### Items binaires de fin

| Item | Type | Critère |
|------|------|---------|
| **The Moebius ribbon** | Oui/Non | Un ruban de Möbius *cool* et bien implémenté |
| **Caustics and/or Global illumination** | Oui/Non | « Super cool, n'oubliez pas de partager les images sur le forum et slack » |
| **The last.... and the least** | Oui/Non | **Est-ce que c'est beau ?** 100 % subjectif |

---

## 6. Bonus et plafond de note

- La fiche attribue des **points bonus** pour options/extensions/fonctionnalités encore plus remarquables.
- **La note finale peut aller jusqu'à 125.**
- « Le seuil de validation du projet est bas : **plus vous créerez d'options, plus vous obtiendrez d'XP** ».
- « Avec un nombre correct d'options, ce devrait être un **pass**, mais **moins d'XP qu'avec PLUS d'options** ».

**Stratégie implicite** : couvrir **largement** les items 0–3 des grilles (faciles) avant de
viser les items 4–5 (coûteux), et surtout **ne jamais sacrifier la partie obligatoire**.

---

## 7. Remise, dépôt Git et preuves acceptées

| Règle | Détail |
|-------|--------|
| Dépôt | Dépôt Git habituel ; **seul le contenu du dépôt est évalué** |
| Noms | Vérifier soigneusement les noms de dossiers et fichiers |
| Démonstration | Il faut **démontrer toutes les options** pendant l'évaluation pour obtenir les points |
| Préparation | Prévoir **plusieurs scènes configurées, prêtes à être calculées** |
| **Images déjà créées** (jpeg, png…) | **Non autorisées** pour prouver les options |
| Manipulations en direct | On s'attend à devoir faire des **manipulations en direct** avec **vos propres outils** (et non des outils publics existants) → d'où l'importance du **fichier de configuration** ou de la **configuration in-program** |

> **Conséquence opérationnelle** : chaque option doit être **prouvée par un fichier de scène**
> qui la déclenche, et (si possible) par un **bouton/paramètre en direct** pendant la démo.

---

## 8. Points ouverts à trancher par l'équipe

Ces éléments ne sont pas tranchés par le sujet **et** par la fiche. Ils doivent l'être **avant**
la soutenance (idéalement auprès d'un corrected ou du campus).

| # | Point ouvert | Écart constaté | Décision à prendre | Échéance |
|---|--------------|----------------|--------------------|----------|
| **O1** | **Fichier `author`** | La fiche exige « the author file is at the root of the repository **and formatted as explained in the subject** », mais le **sujet v4.1 ne mentionne aucun fichier `author`** (vérifié dans les deux PDF). | Appliquer la **convention 42** : fichier `author` (ou `authors`) à la racine, **un login par ligne**, dans l'ordre attendu ; vérifier sur un autre projet 42 du campus. | Immédiat |
| **O2** | **Norminette** | La fiche exige « Norm is OK (using the **norminette**) », alors que le sujet autorise **C++/Rust** et une **liberté totale d'organisation des fichiers**. La norminette ne s'applique qu'au C. | Confirmer si la norme est **réellement contrôlée** pour ce projet en C++ ; si oui, quel outil/variant. **Ne pas ignorer** : c'est un flag d'arrêt (note 0). | Immédiat |
| **O3** | **Format « File ++ »** | Le format actuel est « une directive par ligne » → **explicitement exclu** par le critère. | Valider le **format structuré hiérarchique** proposé dans [ARCHITECTURE.md §5](ARCHITECTURE.md) (ou XML). | Avant le lot « fichiers de scène » |
| **O4** | **« Direct light »** | Le sens exact (« on est aveuglé par un spot face à nous ») n'est pas formellement défini : *spotlight orienté caméra* vs *effet headlight/blindage spéculaire*. | Démo prête dans les **deux** interprétations (voir [OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)). | Avant la soutenance |
| **O5** | **Rendu GPU** | Le calcul GPU est autorisé, le rendu final par pipeline GPU est interdit. | Rester 100 % CPU pour l'image finale ; documenter tout usage GPU éventuel pour le justifier. | Continu |
| **O6** | **`mlx_expose_hook` en SDL** | La fiche cite l'API MiniLibX, le projet utilise SDL. | Argumenter : équivalent SDL = `SDL_WINDOWEVENT_EXPOSED` + reblit du framebuffer, avec **preuve par le code** et **gain de temps mesuré**. | Avant la soutenance |
| **O7** | **Organisation du groupe** | Item noté Oui/Non, subjectif. | Tenir un **journal d'organisation** (réunions, répartitions, revues) depuis le premier jour. | Dès J1 |

---

## 9. Synthèse : ordre de priorité imposé par la notation

```
PRIORITÉ 0 — Sans ça, tout est 0 :
   M1..M8 (obligatoire) + B1..B4 (basic stuff) + aucun crash

PRIORITÉ 1 — Rapide et gros points :
   Scene files • File ++ • Ambiance light • Ambiance ++
   Parallel light • Reflection & transparency (5 sous-critères)
   Textures (5 sous-critères) • Technical effects (MT, screenshot, vitesse)
   Environment 1-3 (interface + live)

PRIORITÉ 2 — Moyen effort :
   Disruptions • Direct light • Shadows & transparency
   Usual visual effects (AA, cartoon, sépia…) • Composed elements
   Simple native objects (paraboloïde/hyperboloïde) • Environment 4-5

PRIORITÉ 3 — Gros effort / à réserver au dernier temps :
   Limited objects (5 sous-critères) • More texture applications
   Negative objects • Exotic objects • In bulk
   Moebius • Caustics / GI • « Is it beautiful? »
```

La ventilation détaillée (implémentation, fichiers touchés, test de preuve) se trouve dans
**[OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)**.
