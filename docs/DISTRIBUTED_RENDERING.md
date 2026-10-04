# RT — Rendu distribué (clustering sur plusieurs machines)

> **Item de fiche** : *Technical effects* → « Clustering rendering (computed on several
> computers) » = **2 points** (grille 0–5).
>
> **Rappel de priorité** : l'obligatoire vaut tout le projet ; les options ne sont évaluées que
> si elle est parfaite. Ce sujet se traite **après le jalon J2**, et se **gèle à J-2** comme le
> reste ([PLAN_TRAVAIL.md §6.1](PLAN_TRAVAIL.md)).
>
> Le sujet autorise aussi le **calcul GPU** (OpenCL, CUDA, compute shaders) pour la performance —
> seul le **rendu final par pipeline GPU est interdit** ([SPECIFICATIONS.md §2.3](SPECIFICATIONS.md)).

---

## Sommaire

1. [Principe général](#1-principe-général)
2. [Prérequis dans le moteur (à faire en premier)](#2-prérequis-dans-le-moteur-à-faire-en-premier)
3. [Niveau 1 — Orchestration SSH](#3-niveau-1--orchestration-ssh)
4. [Niveau 2 — Coordinateur / workers maison](#4-niveau-2--coordinateur--workers-maison)
5. [Niveau 3 — Exotique (bonus « In bulk »)](#5-niveau-3--exotique-bonus-in-bulk)
6. [Pièges de rendu distribué](#6-pièges-de-rendu-distribué)
7. [Plan d'action et budget](#7-plan-daction-et-budget)
8. [Démonstration pendant la soutenance](#8-démonstration-pendant-la-soutenance)

---

## 1. Principe général

```
                ┌─────────────┐
                │  Image      │  découpage en K tuiles
                │  1920×1080  │
                └──────┬──────┘
       ┌───────────────┼───────────────┐
       ▼               ▼               ▼
  ┌─────────┐     ┌─────────┐     ┌─────────┐
  │ pc1     │     │ pc2     │     │ pc3     │
  │ tuile 0 │     │ tuile 1 │     │ tuile 2 │   = 1 processus ./rt
  └────┬────┘     └────┬────┘     └────┬────┘     par machine
       │               │               │
       └───────────────┼───────────────┘
                       ▼
              montage → image finale
```

Le rendu par tuiles est **embarrassingly parallel** : aucune communication n'est nécessaire
pendant le calcul, seule la **réassemblage** l'est. C'est la raison pour laquelle ce type de
rendu est simple à distribuer — et pourquoi les points sont rapides à obtenir.

---

## 2. Prérequis dans le moteur (à faire en premier)

Sans ces points, aucune stratégie de distribution ne fonctionne correctement.

### 2.1 Mode headless découpeur

```bash
./rt scenes/showcase.rt --width 1920 --height 1080 \
     --tile 3/4 \                 # colonne 3 sur 4
     --spp 64 --seed 42 \
     --out tiles/tile_3_4.png     # pas de fenêtre SDL
```

| Option | But |
|--------|-----|
| `--tile k/n` (ou `--tile-x --tile-w`) | découpe verticale simple, **chevauchante de 0 pixel** |
| `--out fichier.png` | écrit l'image et **sort immédiatement** (code retour 0) |
| `--spp`, `--seed` | qualité et reproductibilité **identiques** partout |
| pas de `SDL_Init` | les workers peuvent être sans `DISPLAY` (headless, conteneur) |

### 2.2 Exigences de cohérence (les vraies difficultés)

| Exigence | Pourquoi | Implémentation |
|----------|----------|----------------|
| **RNG seedée en coordonnées pixel absolues** | sinon **coutures visibles** aux bords de tuiles (l'antialiasing de la tuile 1 ne voit pas les pixels de la tuile 2) | `rt::seedFor(globalX, globalY, sample, sceneSeed)` (`include/rt/base/Rng.hpp`, PCG32) — jamais `hash(localX, localY)` |
| **Mêmes `spp`, même binaire, mêmes scènes** | sinon niveau de bruit hétérogène dans l'image finale | hashes vérifiés par le coordinateur |
| **`fps`/résolution/FOV identiques** | sinon décalage géométrique entre tuiles | tout vient du même fichier de scène |
| **Checksum par tuile** | détecter un worker défaillant ou un fichier corrompu | `sha256` du PNG transmis avec |
| **Idempotence** | une tuile peut être rejouée si un worker meurt | la 1ʳᵉ réponse **vérifiée** gagne, les suivantes sont ignorées |
| **Fallback local** | jamais de blocage le jour de la démo | 0 worker → rendu local complet |

#### 2.2.1 Implémentation de la graine (T016)

`include/rt/base/Rng.hpp` fournit le contrat ci-dessus, sans allocation et sans exception :

| Élément | Rôle |
|---------|------|
| `splitMix64(z)` | finaliseur d'avalanche `constexpr` (Steele et al.) |
| `seedFor(globalX, globalY, sample, sceneSeed)` | graine 64 bits dépendant **uniquement** des coordonnées absolues |
| `rngFor(...)` | `Rng` PCG32 déjà seedé pour le pixel `(x, y)` et l'échantillon `sample` |
| `Rng::nextUint32 / nextFloat / nextRange / discard` | tirages `noexcept`, période 2⁶⁴, état 16 octets (`static_assert`) |

Règle d'or pour les calques supérieurs (`render/`, `sched/`) : la fonction de rendu d'une
région reçoit des coordonnées **globales** (`x0`, `y0` de l'image, jamais de la tuile).
`tests/unit/test_rng.cpp` le vérifie en rendant une image de 32×16 px en 2 bandes puis en
image pleine, et compare les valeurs **octet par octet** (DoD T016).

### 2.3 Ajouts de build

```make
headless: CXXFLAGS += -DRT_HEADLESS      # compile sans dépendance à l'affichage
# ou bien : détecter l'absence de DISPLAY à l'exécution et basculer en mode fichier
```

---

## 3. Niveau 1 — Orchestration SSH

> **Recommandé pour la soutenance.** ~0,5 jour, **0 ligne de code réseau**, risque minimal,
> démontrable en 30 secondes avec deux postes (ou deux shells).

### 3.1 Script complet

```bash
#!/bin/sh
# scripts/cluster_render.sh — N machines, 1 image
set -e
SCENE=${1:-scenes/showcase.rt}
W=${2:-1920}; H=${3:-1080}
TILES=${#HOSTS}                       # 1 tuile par machine
HOSTS="pc1 pc2 pc3 pc4"               # ou "127.0.0.1 127.0.0.1" pour tester en local

mkdir -p tiles

# 1) Distribuer binaire + scène + textures (mêmes bytes = mêmes calculs)
for h in $HOSTS; do
  ssh "$h" "mkdir -p ~/rt/tiles ~/rt/scenes"
  rsync -a --checksum ./rt "$h:~/rt/"
  rsync -a --checksum "$SCENE" textures/ "$h:~/rt/scenes/" 2>/dev/null || true
done

# 2) Lancer une tuile par machine, en parallèle
i=0
for h in $HOSTS; do
  ssh "$h" "cd ~/rt && ./rt $SCENE --width $W --height $H \
            --tile $i/$TILES --spp 64 --seed 42 --out tiles/$i.png" &
  i=$((i + 1))
done
wait

# 3) Récupérer et monter
for h in $HOSTS; do :; done
i=0; ARGS=""
for h in $HOSTS; do
  scp -q "$h:~/rt/tiles/$i.png" "tiles/" && ARGS="$ARGS tiles/$i.png"
  i=$((i + 1))
done

montage $ARGS -tile ${TILES}x1 -geometry +0+0 final.png
echo "OK -> final.png"
```

### 3.2 Optimisations

| Astuce | Gain |
|--------|------|
| `ssh -o ControlMaster=auto -o ControlPath=/tmp/ssh-%r@%h -o ControlPersist=60` | une seule connexion TCP réutilisée pour `rsync` + `scp` + `ssh` |
| `rsync --checksum` | ne resynchronise que ce qui a changé |
| `xargs -P 8 -I{}` | parallélisme simple sans script |
| `pdsh -w host1,host2 'cmd'` | ssh en parallèle (non installé ici) |
| PNG plutôt que RAW | 8 Mo → ~500 Ko par tuile 480×1080 : transfert ×16 plus rapide |
| Clés SSH sans mot de passe | `ssh-keygen -t ed25519 && ssh-copy-id pc1` — **indispensable** pour un script non interactif |

### 3.3 Outils

`ssh` (avec clés), `rsync`, **ImageMagick** (`montage`) — *tous installés sur la machine de
développement* (vérifié) ; `pdsh` et `tmux` sont absents.

---

## 4. Niveau 2 — Coordinateur / workers maison

> Le **vrai** « clustering rendering ». ~1–2 jours. Réutilise directement les acquis de
> **Webserv** (boucle événementielle, file de slots, buffers fixes) — voir
> [MEMORY_STRATEGY.md](MEMORY_STRATEGY.md).

### 4.1 Protocole

HTTP simple (vous maîtrisez déjà le sujet) ou TCP brut :

```
GET  /job?worker=w2
  → 200 { "binaryHash": "...", "sceneHash": "...",
          "tile": {"x":480,"y":0,"w":480,"h":1080},
          "spp":64, "seed":42, "expiresAt": 1730000000 }
  → 204 No Content            (plus de travail)

POST /tile   (fichier PNG + sha256 + worker id)
  → 200 OK                    (tuile acceptée)
  → 409 Conflict              (hash incorrect → le worker a une version différente)

GET  /status  → { "done": 11, "total": 16, "eta": 42, "workers": 4 }
GET  /health  → supervision des workers
```

### 4.2 Tableau des décisions

| Problème | Solution |
|----------|----------|
| Worker avec un binaire ou une scène **différent** | rejet systématique si `binaryHash` / `sceneHash` ≠ valeur attendue |
| Worker qui meurt ou qui bloque | **timeout** → la tuile retourne dans la file (*re-dispatch*) |
| Double exécution | idempotence : la première réponse **vérifiée** (checksum) gagne |
| Rien à servir (0 worker) | **fallback local** : rendu complet sur la machine de démo |
| Volume réseau | PNG compressé plutôt que tampon RAW |
| Barre de progression | l'agrégation fournit l'**item Environment 1** gratuitement |
| Supervision | `GET /health` + logs horodatés → argument pour *Group organization* |

### 4.3 Architecture proposée

```
┌──────────────────────────────────────────────────────┐
│ Coordinateur (thread dédié, boucle poll/epoll)       │
│  • file de tuiles (bitmaps = slots libres)           │  ← cf. ConnectionPool de Webserv
│  • table workers {id, dernièreactivité, tuile}        │
│  • barre de progression + journal                    │
└──────────────────────────────────────────────────────┘
        ▲ HTTP/TCP                      │
        │                                ▼
   worker A                        worker B
   ./rt --tile 0/4 --out -         ./rt --tile 1/4 --out -
```

**Réutilisation conceptuelle de Webserv** (pas de copier-coller de code, mais les mêmes
solutions) : pool de slots à bitmaps, buffers de taille fixe, boucle événementielle unique,
codes de retour au lieu d'exceptions pour les chemins d'erreur.

### 4.4 Alternative : conteneurs

```bash
docker build -t rt-worker .                       # image = binaire + scènes + textures
docker run --rm rt-worker ./rt scenes/x.rt --tile 0/4 --out /out/0.png
```
L'image garantit un binaire **identique** partout (le hash n'est plus qu'une sécurité).
*`docker` est installé sur la machine de développement.*

---

## 5. Niveau 3 — Exotique (bonus « In bulk »)

| Option | Intérêt | Coût | Outil |
|--------|---------|------|-------|
| **MPI** (`mpirun -np 8 ./rt --mpi …`) | standard du calcul distribué, crédible académiquement | 1–2 j | OpenMPI/MPICH — `mpicc` **absent** ici |
| **GPU (OpenCL/CUDA)** pour le calcul | autorisé explicitement par le sujet, gros gain de vitesse | 3–5 j | toolkit CUDA/ROCm |
| **Calcul sur plusieurs GPUs** d'une machine | item *Technical effects* | 1 j | même code que le cluster |
| **Vidéo à partir des images** | item *In bulk* « video made from your RT » | 0,5 j | `ffmpeg` (absent) |
| Ferme type **Flamenco** | sur-ingenierie pour nous | — | hors sujet |

---

## 6. Pièges de rendu distribué

| # | Piège | Conséquence | Parade |
|---|-------|-------------|--------|
| 1 | Seed calculée sur les coordonnées **locales** | coutures nettes aux bords de tuiles | seed = `hash(globalX, globalY, sample)` |
| 2 | Machines avec des binaires différents | image inconsistante (bruit, géométrie) | `binaryHash` + `rsync --checksum` |
| 3 | `spp` différent entre tuiles | patchs plus ou moins bruités | tout vient du même fichier de scène |
| 4 | Worker sans `DISPLAY` qui tente d'ouvrir une fenêtre | plantage immédiat | mode `--out` headless obligatoire |
| 5 | Tuile perdue = image incomplète | **non démontrable** | timeout + *re-dispatch* + checksum |
| 6 | Dépendre du réseau le jour de la soutenance | blocage → items non montrés | **fallback local** toujours prêt |
| 7 | Trop tard dans le planning | met en péril l'obligatoire | gel à **J-2**, traitement après **J2** |
| 8 | Montage oublié / mauvais ordre de tuiles | image faux témoignage | nommer les fichiers `tuile_<k>_<n>.png` et monter par index |
| 9 | Fuite mémoire multipliée par N processus | bruit dans les mesures | `valgrind` sur **1 worker** + mesure RSS (`/usr/bin/time -v`) |

---

## 7. Plan d'action et budget

| Étape | Contenu | Effort | Quand | Points |
|-------|---------|--------|-------|--------|
| **D0** | Mode headless `--tile` / `--out` / `--seed` | 3–4 h | après J2 | prerequisite |
| **D1** | Seed absolue + tests de couture (bord de tuile visible ?) | 2 h | après J2 | qualité |
| **D2** | `scripts/cluster_render.sh` (SSH) + `montage` | 3 h | J3–J4 | **2 pts** |
| **D3** | Mesures : 1 PC vs 2 PC vs 4 PC (speedup réel) | 1 h | J4 | item « vraiment rapide » |
| **D4** *(facultatif)* | Coordinateur/worker HTTP + barre de progression | 1–2 j | J5 si temps | + *Environment 1* |
| **D5** *(facultatif)* | MPI ou GPU compute | 2–5 j | jamais si J6 menacé | bonus |

**Total minimum pour les 2 points : ~8–9 heures.**

---

## 8. Démonstration pendant la soutenance

### Script de démonstration (3 minutes)

```bash
# 1. Montrer que la scène est trop lente seule
./rt scenes/showcase.rt --width 1920 --height 1080 --out solo.png   # ex. 62 s

# 2. Lancer la version distribuée sur 2 postes
./scripts/cluster_render.sh scenes/showcase.rt 1920 1080             # ex. 33 s

# 3. Ouvrir l'image finale, zoomer sur une couture (il n'y en a pas)
# 4. Montrer le script et le mode --tile dans le code
```

### Preuves à avoir prêtes

- [ ] Deux postes (ou deux conteneurs) configurés **avant** la soutenance, clés SSH en place
- [ ] Un scénario **sans réseau** : le même script avec `HOSTS="127.0.0.1 127.0.0.1"`
      (deux processus locaux = preuve de découpage, même sans 2ᵉ machine)
- [ ] Mesures écrites : temps seul / à 2 / à 4
- [ ] Image finale **dans le dépôt** (preuve visuelle secondaire — jamais seule)
- [ ] Le correcteur peut lancer le script lui-même
- [ ] Fallback local vérifié (débrancher le réseau ne casse rien)

---

## Voir aussi

- Priorisation des options : [OPTIONS_GUIDE.md §9](OPTIONS_GUIDE.md)
- Outils nécessaires : [OUTILS.md](OUTILS.md)
- Mémoire et allocation : [MEMORY_STRATEGY.md](MEMORY_STRATEGY.md)
- Rendu par tuiles / *jobs* : [INSPIRATION_BLENDER.md §3](INSPIRATION_BLENDER.md)
