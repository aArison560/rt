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
