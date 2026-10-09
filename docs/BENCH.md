# RT — Benchmarks

> Document **produit et complété par `scripts/bench.sh`** (T019) : chaque mesure est
> générée par le script, jamais saisie à la main (règle du sujet : preuve
> régénérable). Les mesures vivent sous `## Résultats`, la méthode ci-dessous est
> fixe.

## Méthode

- **Harnais** : `sh scripts/bench.sh <scene.rt> [--runs N] [--warmup N] [--label L]
  [--note "..."] [--args "..."]`, ou `--cmd "<commande>"` pour mesurer autre chose
  qu'un rendu. Le script se place toujours à la racine du dépôt.
- **Chronométrage** : `hyperfine` s'il est installé (son JSON natif est exporté tel
  quel), sinon `date +%s.%N` autour de chaque exécution, en dernier recours une
  horloge `python3`. **Le harnais tourne sans hyperfine** : c'est le cas de ce poste
  et de la CI.
- **Protocole** : `--warmup` exécutions d'échauffement non comptabilisées (défaut 1)
  puis `--runs` exécutions mesurées (défaut 5). La sortie de la commande est avalée ;
  en cas d'échec elle est affichée et son code retour est propagé.
- **Statistiques** : moyenne arithmétique et **écart-type échantillon** (dénominateur
  n−1, mise à jour par la formule de Welford, `0` pour n = 1), plus min, max et
  `CV = écart-type / moyenne × 100`. Toutes les valeurs sont en **secondes**.
- **Format d'une mesure** : section `### <label> — <horodatage>` contenant la
  commande exacte, le protocole, un tableau Markdown **et** le détail brut en JSON
  (bloc fenced) : rien n'est recopié à la main.
- **Comparaison avant/après** : rejouer avec le **même `--label`** remplace la
  section homonyme ; deux labels différents coexistent pour comparer deux variantes.
- **Honnêteté** : la ligne *Commande* donne la commande réellement mesurée et la
  ligne *Note* le contexte (par ex. binaire lancé avant que le rendu n'existe) : une
  mesure n'est un temps de rendu que si la commande rend.

## Résultats
### t064-threads-8 — 2026-10-09 10:31:03 EAT

- **Commande** : `./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_8.png --quiet --threads 8`
- **Note** : T064: fig_vi1 320x240 spp4 8 threads, tuiles 32x32
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `67b8d5b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.252267 | 0.003176 | 0.248355 | 0.255622 | 1.26 |

Détail brut :

```json
{"command":"./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_8.png --quiet --threads 8","label":"t064-threads-8","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.252267,"stddev":0.003176,"min":0.248355,"max":0.255622,"times":[0.251909,0.255622,0.255297,0.248355,0.250151],"timestamp":"2026-10-09T10:31:03+03:00"}
```
### t064-threads-4 — 2026-10-09 10:30:58 EAT

- **Commande** : `./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_4.png --quiet --threads 4`
- **Note** : T064: fig_vi1 320x240 spp4 4 threads, tuiles 32x32
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `67b8d5b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.268519 | 0.017702 | 0.254885 | 0.298419 | 6.59 |

Détail brut :

```json
{"command":"./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_4.png --quiet --threads 4","label":"t064-threads-4","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.268519,"stddev":0.017702,"min":0.254885,"max":0.298419,"times":[0.265072,0.268419,0.254885,0.255802,0.298419],"timestamp":"2026-10-09T10:30:58+03:00"}
```
### t064-threads-2 — 2026-10-09 10:30:55 EAT

- **Commande** : `./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_2.png --quiet --threads 2`
- **Note** : T064: fig_vi1 320x240 spp4 2 threads, tuiles 32x32
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `67b8d5b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.411556 | 0.018911 | 0.391779 | 0.443069 | 4.60 |

Détail brut :

```json
{"command":"./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_2.png --quiet --threads 2","label":"t064-threads-2","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.411556,"stddev":0.018911,"min":0.391779,"max":0.443069,"times":[0.407978,0.443069,0.407773,0.407180,0.391779],"timestamp":"2026-10-09T10:30:55+03:00"}
```
### t064-threads-1 — 2026-10-09 10:30:47 EAT

- **Commande** : `./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_1.png --quiet --threads 1`
- **Note** : T064: fig_vi1 320x240 spp4 (scene) 1 thread, lineaire (BVH non branchee)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `67b8d5b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.735890 | 0.027068 | 0.708513 | 0.774556 | 3.68 |

Détail brut :

```json
{"command":"./rt scenes/fig_vi1.rt 320 240 --out /tmp/bench_t064_1.png --quiet --threads 1","label":"t064-threads-1","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.735890,"stddev":0.027068,"min":0.708513,"max":0.774556,"times":[0.774556,0.715364,0.751050,0.708513,0.729967],"timestamp":"2026-10-09T10:30:47+03:00"}
```
### t061-bvh-traverse — 2026-10-09 09:53:09 EAT

- **Commande** : `sh scripts/bench_bvh.sh traverse 1000 2000`
- **Note** : T061 apres : Bvh traverse pile fixe + Williams, memes 2000 rayons (graine fixe)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `bced1e3`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.009136 | 0.000237 | 0.008803 | 0.009421 | 2.59 |

Détail brut :

```json
{"command":"sh scripts/bench_bvh.sh traverse 1000 2000","label":"t061-bvh-traverse","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.009136,"stddev":0.000237,"min":0.008803,"max":0.009421,"times":[0.009421,0.009264,0.009018,0.009173,0.008803],"timestamp":"2026-10-09T09:53:09+03:00"}
```
### t061-bvh-linear — 2026-10-09 09:53:06 EAT

- **Commande** : `sh scripts/bench_bvh.sh linear 1000 2000`
- **Note** : T061 avant : brute-force lineaire, 1000 spheres x 2000 rayons deterministes (graine fixe)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `bced1e3`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.238512 | 0.001974 | 0.235118 | 0.240239 | 0.83 |

Détail brut :

```json
{"command":"sh scripts/bench_bvh.sh linear 1000 2000","label":"t061-bvh-linear","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.238512,"stddev":0.001974,"min":0.235118,"max":0.240239,"times":[0.239250,0.240239,0.239216,0.238738,0.235118],"timestamp":"2026-10-09T09:53:06+03:00"}
```
### t051-2lights — 2026-10-08 19:12:26 EAT

- **Commande** : `./rt scenes/default.rt 320 240 --out /tmp/bench_t051_2.png --quiet`
- **Note** : T051: 2 ponctuelles, attenuation (1 0 0), 320x240 spp4 (cout par lumiere)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 7.0.0-34-generic x86_64 · nherimam-ubuntu
- **Commit au moment de la mesure** : `4751033`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.195914 | 0.002369 | 0.193894 | 0.199956 | 1.21 |

Détail brut :

```json
{"command":"./rt scenes/default.rt 320 240 --out /tmp/bench_t051_2.png --quiet","label":"t051-2lights","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.195914,"stddev":0.002369,"min":0.193894,"max":0.199956,"times":[0.199956,0.194677,0.195832,0.195209,0.193894],"timestamp":"2026-10-08T19:12:26+03:00"}
```
### t051-1light — 2026-10-08 19:12:25 EAT

- **Commande** : `./rt tests/cases/valid/minimal.rt 320 240 --out /tmp/bench_t051_1.png --quiet`
- **Note** : T051: 1 ponctuelle, attenuation (1 0 0), 320x240 spp2 (cout par lumiere)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 7.0.0-34-generic x86_64 · nherimam-ubuntu
- **Commit au moment de la mesure** : `4751033`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.097920 | 0.001740 | 0.096203 | 0.100777 | 1.78 |

Détail brut :

```json
{"command":"./rt tests/cases/valid/minimal.rt 320 240 --out /tmp/bench_t051_1.png --quiet","label":"t051-1light","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.097920,"stddev":0.001740,"min":0.096203,"max":0.100777,"times":[0.100777,0.097615,0.098031,0.096203,0.096972],"timestamp":"2026-10-08T19:12:26+03:00"}
```
### minimal — 2026-10-06 16:45:10 EAT

- **Commande** : `./rt tests/cases/valid/minimal.rt`
- **Note** : Mesure T019 : ./rt ignore encore les arguments (parser = T023, boucle de rendu = T032) — chiffres = demarrage du binaire, les vrais temps de rendu viendront avec T032
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `8673e37`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.002134 | 0.000145 | 0.001938 | 0.002336 | 6.79 |

Détail brut :

```json
{"command":"./rt tests/cases/valid/minimal.rt","label":"minimal","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.002134,"stddev":0.000145,"min":0.001938,"max":0.002336,"times":[0.002336,0.002179,0.002139,0.002078,0.001938],"timestamp":"2026-10-06T16:45:10+03:00"}
```

## T064 — Synthèse speedup 1/2/4/8 threads (manuelle, chiffres de `bench.sh`)

> Synthèse **manuelle** des 4 sections `t064-threads-*` ci-dessus (mêmes
> moyennes/écarts-types, aucune valeur estimée). Commandes reproductibles :
> `sh scripts/bench.sh scenes/fig_vi1.rt --args "320 240 --out /tmp/bench_t064_N.png --quiet --threads N" --label t064-threads-N --runs 5`.
> Scène `fig_vi1.rt` (5 objets, 2 spots, 320×240 spp4), 8 cœurs, `date +%s.%N`.

| threads | moyenne (s) | écart-type (s) | speedup vs 1 | efficacité (speedup/N) |
|---:|---:|---:|---:|---:|
| 1 | 0.735890 | 0.027068 | 1.00 | 1.00 |
| 2 | 0.411556 | 0.018911 | 1.79 | 0.89 |
| 4 | 0.268519 | 0.017702 | 2.74 | 0.69 |
| 8 | 0.252267 | 0.003176 | 2.92 | 0.36 |

- **Speedup croissant** : 1.00 < 1.79 < 2.74 < 2.92 (DoD T064).
- **Efficacité < 1 attendue** : 0.89 / 0.69 / 0.36 (tuiles 32×32, `spp` batches
  séquentiels + `present()` mono, scène à 5 objets : le grain est fin, 8
  threads saturent — honnête et documenté).
- **Compteurs** : `RenderStats` (`rays/s`, objets, temps build/render, threads,
  `bvhBuilds`) affichés en fin de rendu sur `stderr` sauf `--quiet`
  (ex. `fig_vi1.rt` 160×120 spp2 : `rays=38400 … rays/s=425390`), prêts pour
  l'UI (T075).
