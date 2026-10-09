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
### t067-figvi3-8 — 2026-10-09 10:49:13 EAT

- **Commande** : `./rt scenes/fig_vi3.rt 320 240 --out /tmp/bench_t067_figvi3_8.png --quiet --threads 8`
- **Note** : T067: fig_vi3.rt 320x240 spp4 8 threads (obligatoire)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `bae2e2b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.171684 | 0.008573 | 0.162673 | 0.184031 | 4.99 |

Détail brut :

```json
{"command":"./rt scenes/fig_vi3.rt 320 240 --out /tmp/bench_t067_figvi3_8.png --quiet --threads 8","label":"t067-figvi3-8","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.171684,"stddev":0.008573,"min":0.162673,"max":0.184031,"times":[0.171452,0.162673,0.164866,0.184031,0.175398],"timestamp":"2026-10-09T10:49:13+03:00"}
```
### t067-figvi3-1 — 2026-10-09 10:49:06 EAT

- **Commande** : `./rt scenes/fig_vi3.rt 320 240 --out /tmp/bench_t067_figvi3_1.png --quiet --threads 1`
- **Note** : T067: fig_vi3.rt 320x240 spp4 1 thread (obligatoire, melange d'ombres)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `bae2e2b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.477937 | 0.036972 | 0.434435 | 0.525046 | 7.74 |

Détail brut :

```json
{"command":"./rt scenes/fig_vi3.rt 320 240 --out /tmp/bench_t067_figvi3_1.png --quiet --threads 1","label":"t067-figvi3-1","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.477937,"stddev":0.036972,"min":0.434435,"max":0.525046,"times":[0.503544,0.525046,0.451635,0.475026,0.434435],"timestamp":"2026-10-09T10:49:06+03:00"}
```
### t067-default-8 — 2026-10-09 10:49:04 EAT

- **Commande** : `./rt scenes/default.rt 320 240 --out /tmp/bench_t067_default8.png --quiet --threads 8`
- **Note** : T067: default.rt 320x240 spp4 8 threads (demo)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `bae2e2b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.198453 | 0.011155 | 0.182820 | 0.213303 | 5.62 |

Détail brut :

```json
{"command":"./rt scenes/default.rt 320 240 --out /tmp/bench_t067_default8.png --quiet --threads 8","label":"t067-default-8","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.198453,"stddev":0.011155,"min":0.182820,"max":0.213303,"times":[0.182820,0.194271,0.199710,0.202160,0.213303],"timestamp":"2026-10-09T10:49:04+03:00"}
```
### t067-default-1 — 2026-10-09 10:48:59 EAT

- **Commande** : `./rt scenes/default.rt 320 240 --out /tmp/bench_t067_default1.png --quiet --threads 1`
- **Note** : T067: default.rt 320x240 spp4 1 thread (demo)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `bae2e2b`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.439644 | 0.012922 | 0.424591 | 0.453072 | 2.94 |

Détail brut :

```json
{"command":"./rt scenes/default.rt 320 240 --out /tmp/bench_t067_default1.png --quiet --threads 1","label":"t067-default-1","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.439644,"stddev":0.012922,"min":0.424591,"max":0.453072,"times":[0.429034,0.451829,0.424591,0.453072,0.439696],"timestamp":"2026-10-09T10:48:59+03:00"}
```
### t065-bvh-on — 2026-10-09 10:34:13 EAT

- **Commande** : `./rt scenes/perf_many.rt 320 240 --out /tmp/bench_t065_after.png --quiet --threads 1`
- **Note** : T065 apres: BVH traversal (1 build/rendu), 101 objets 320x240 spp2, memes pixels que lineaire
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `5f5e070`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.320736 | 0.012089 | 0.311432 | 0.335023 | 3.77 |

Détail brut :

```json
{"command":"./rt scenes/perf_many.rt 320 240 --out /tmp/bench_t065_after.png --quiet --threads 1","label":"t065-bvh-on","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":0.320736,"stddev":0.012089,"min":0.311432,"max":0.335023,"times":[0.312173,0.335023,0.312178,0.311432,0.332875],"timestamp":"2026-10-09T10:34:13+03:00"}
```
### t065-bvh-off — 2026-10-09 10:32:37 EAT

- **Commande** : `./rt scenes/perf_many.rt 320 240 --out /tmp/bench_t065_before.png --quiet --threads 1`
- **Note** : T065 avant: recherche lineaire, 101 objets 320x240 spp2 (goulot intersections)
- **Protocole** : 5 exécution(s) mesurée(s), 1 échauffement(s), outil `date +%s.%N`, horloge `date +%s.%N`
- **Machine** : Linux 6.12.111+deb13-amd64 x86_64 · epsilon
- **Commit au moment de la mesure** : `5f5e070`

| runs | moyenne (s) | écart-type (s) | min (s) | max (s) | CV (%) |
|---:|---:|---:|---:|---:|---:|
| 5 | 3.759435 | 0.142151 | 3.570467 | 3.939661 | 3.78 |

Détail brut :

```json
{"command":"./rt scenes/perf_many.rt 320 240 --out /tmp/bench_t065_before.png --quiet --threads 1","label":"t065-bvh-off","n":5,"warmup":1,"tool":"date +%s.%N","clock":"date +%s.%N","unit":"s","mean":3.759435,"stddev":0.142151,"min":3.570467,"max":3.939661,"times":[3.570467,3.806515,3.939661,3.670676,3.809858],"timestamp":"2026-10-09T10:32:37+03:00"}
```
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

## T065 — Profiling et optimisation : boucle linéaire → BVH (un seul point)

> `perf`/`callgrind` absents du poste (`sh scripts/check_env.sh`) :
> profiling par instrumentation interne (`RenderStats` + `bench_bvh.sh` +
> scaling objets). **Goulot réel** : les intersections — `fig_vi1.rt`
> (5 objets, 160×120 spp2) rend en 90 ms quand `perf_many.rt` (101 objets,
> même résolution/spp) prend 456 ms (×5 pour ×20 objets, quasi linéaire) ;
> micro-bench T061 (`bench_bvh.sh`, 1000 sphères × 2000 rayons) : linéaire
> 0.239 s → BVH 0.009 s (≈ 26×, mêmes 757 hits). **Un seul point optimisé** :
> `render()` construit la BVH **une fois** par rendu (chemin froid, dans
> `buildMs`) et `traceRay()` + `shadowTransmittance()` traversent au lieu de
> boucler (`findClosestHit` avec `bvh`, repli linéaire si vide/échec ;
> `TraceCtx::bvh`, `noexcept`, sans allocation). Aucune autre optimisation
> (ni `pow`, ni `HitRecord`, ni tuiles — inchangés).
> Commandes reproductibles :
> `sh scripts/bench.sh scenes/perf_many.rt --args "320 240 --out /tmp/bench_t065_X.png --quiet --threads 1" --label t065-bvh-off/on --runs 5`.

| variante | moyenne (s) | écart-type (s) | min / max (s) | pixels |
|---|---:|---:|---:|---|
| linéaire (`t065-bvh-off`) | 3.759435 | 0.142151 | 3.570 / 3.940 | référence |
| BVH (`t065-bvh-on`) | 0.320736 | 0.012089 | 0.311 / 0.335 | **octet-identiques** (`perf_many.rt` 160×120 spp1 sha256 `2ea2609d` avant/après, `default.rt` 64×48 sha256 `0c585954` inchangé) |

- **Gain mesuré** : 3.76 s → 0.32 s (≈ **11.7×**, variances disjointes :
  3.57 min avant > 0.34 max après — DoD T065).
- **Non-régression** : `rt_test [golden]` vert (4 cas, tolérance 5 pixels),
  `rt_test [threads]` vert (1/2/4/8 identiques conservés avec BVH).

## T067 — Rapport de performance complet (synthèse manuelle, chiffres mesurés)

> Tous les chiffres ci-dessous viennent de `scripts/bench.sh` (sections
> `t064-*`, `t065-*`, `t067-*` : moyenne + écart-type sur 5 runs + 1 warmup,
> `date +%s.%N`) ou de `getrusage(RUSAGE_CHILDREN).ru_maxrss` (pic RSS,
> `/usr/bin/time` absent du poste — voir Mémoire). **Aucun chiffre estimé.**
> Rejouer : commandes sous chaque tableau (`--label` identique = section
> remplacée, comparaison avant/après propre).

### Matériel et versions (mesurés le 2026-10-09)

- **Machine** : `Linux 6.12.111+deb13-amd64 x86_64` · 8 cœurs (`nproc`) ·
  15 GiB RAM (`free -h`) — relevés par `uname -srm`, `nproc`, `free -h`.
- **Compilateur** : `c++ (Debian 14.2.0-19) 14.2.0` (`c++ --version`,
  flags `-Wall -Wextra -Werror -O2 -std=c++2c`).
- **Libs** : `libpng 1.6.48` (`pkg-config --modversion libpng`),
  `valgrind-3.24.0` ; `hyperfine`/`perf`/`bear` absents
  (`sh scripts/check_env.sh`, `command -v`).
- **Commit mesuré** : chaque section `bench.sh` cite son commit (T064-T067 :
  `5f5e070`→`cd92759`, BVH branchée en T065).

### Par scène (320×240, 5 runs, `threads 1` vs `8`)

> Commandes : `sh scripts/bench.sh scenes/<nom>.rt --args "320 240
> --out /tmp/bench_t067_<nom><t>.png --quiet --threads <t>"
> --label t067-<nom>-<t> --runs 5`.
> `rays/s ≈ W×H×spp / moyenne` (mur incl. démarrage/parse/PNG : borne basse
> honnête ; le `[stats] rays/s` interne, hors IO, est ~2× supérieur).
> `spp` = 4 (`default`, `fig_vi1`, `fig_vi3`, `limits.samples`) sauf
> `perf_many` (`spp` = 2).

| scène | objets | threads | moyenne (s) | écart-type (s) | rays/s ≈ | speedup vs 1 |
|---|---:|---:|---:|---:|---:|---:|
| `default.rt` (démo) | 2 | 1 | 0.439644 | 0.012922 | ~699 000 | 1.00 |
| `default.rt` (démo) | 2 | 8 | 0.198453 | 0.011155 | ~1 548 000 | 2.22 |
| `fig_vi1.rt` (obligatoire VI.1) | 5 | 1 | 0.735890 | 0.027068 | ~417 000 | 1.00 |
| `fig_vi1.rt` (obligatoire VI.1) | 5 | 8 | 0.252267 | 0.003176 | ~1 218 000 | 2.92 |
| `fig_vi3.rt` (obligatoire VI.3) | 4 | 1 | 0.477937 | 0.036972 | ~643 000 | 1.00 |
| `fig_vi3.rt` (obligatoire VI.3) | 4 | 8 | 0.171684 | 0.008573 | ~1 789 000 | 2.78 |
| `perf_many.rt` (101 objets, BVH) | 101 | 1 | 0.320736 | 0.012089 | ~479 000 | 11.72 vs linéaire |
| `perf_many.rt` (101 objets, linéaire) | 101 | 1 | 3.759435 | 0.142151 | ~41 000 | 1.00 (référence) |

### Mémoire pic (RSS, mesurée par processus frais)

> `/usr/bin/time -v` absent (`ls /usr/bin/*time*` : `timedatectl`/`timeout`/
> `uptime` seuls — même constat qu'en T024). Méthode de repli documentée et
> reproductible : `python3 -c "subprocess.run([...]); getrusage(RUSAGE_CHILDREN).ru_maxrss"`
> (un processus frais par cas, sinon le max est cumulatif — vérifié).
> Budget théorique cohérent : framebuffer 320×240×20 o = 1.5 Mo
> (`MEMORY_STRATEGY.md` §4.1) + BVH 101 objets ≈ 201 nœuds × 32 o ≈ 6 Ko.

| cas (320×240) | pic RSS mesuré |
|---|---:|
| `default.rt` 1 thread | 11.4 Mo |
| `fig_vi1.rt` 1 thread | 11.4 Mo |
| `fig_vi3.rt` 1 thread | 11.4 Mo |
| `perf_many.rt` (101 objets) 1 thread | 11.4 Mo |
| `fig_vi1.rt` 8 threads | 11.6 Mo |

- Mémoire **plate** selon scènes et threads (binaire + libs dominent à cette
  résolution ; ni la BVH ni les 8 piles `jthread` ne se voient au Mo près).

### Le rendu est vraiment rapide — sous-titré de chiffres

> **Le rendu est vraiment rapide** : `fig_vi3.rt` (obligatoire, mélange
> d'ombres) en **0.17 s** à 8 threads (1.8 M rays/s), `default.rt` en
> **0.20 s**, `fig_vi1.rt` en **0.25 s** ; même à 1 thread aucune obligatoire
> ne dépasse **0.74 s** en 320×240 spp4 ; 101 objets passent de 3.76 s à
> **0.32 s** grâce à la BVH (≈ 11.7×) pour **11.4 Mo** pic. Mesures
> reproductibles ci-dessus (5 runs + variances, commandes fournies).
