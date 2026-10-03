# RT — Documentation du projet

> **Objectif** : documenter intégralement la réalisation du projet **RT** (Ray Tracer, 42)
> par une équipe de **3 personnes**, depuis la lecture du sujet jusqu'à la soutenance.
>
> **Audience** : les 3 membres de l'équipe (équivalents Dev A / Dev B / Dev C).

---

## 1. Sources de vérité

Toute la documentation de ce dossier est dérivée de deux documents officiels présents dans le dépôt :

| Source | Chemin | Contenu |
|--------|--------|---------|
| **Sujet RT v4.1** | `docs/subjects/fr.subject.pdf`, `docs/subjects/en.subject.pdf` | Objectifs, instructions générales, partie obligatoire, liste des options, remise |
| **Fiche d'évaluation** | `docs/evalsheet/evalsheet.md` (+ page sauvegardée `docs/evalsheet/html/`) | Ce qui est **réellement noté** pendant la soutenance, item par item |

Règle d'or : **le sujet décrit ce qu'il faut construire, la fiche décrit ce qui rapporte des points.**
En cas d'écart entre les deux, la fiche d'évaluation prime pour l'organisation du travail.

---

## 2. Ordre de lecture conseillé

### Documents principaux (ordre imposé)

| # | Document | À lire quand | Réponse à la question |
|---|----------|--------------|------------------------|
| 1 | **[SPECIFICATIONS.md](SPECIFICATIONS.md)** | Immédiatement | *Que faut-il faire, et combien de points ça vaut ?* |
| 2 | **[ARCHITECTURE.md](ARCHITECTURE.md)** | Avant d'écrire du code | *Comment est structuré le moteur, quels algorithmes, quel format de scène ?* |
| 3 | **[PLAN_TRAVAIL.md](PLAN_TRAVAIL.md)** | Dès le lancement, puis chaque semaine | *Qui fait quoi, dans quel ordre, avec quelles règles de collaboration ?* |
| 4 | **[OPTIONS_GUIDE.md](OPTIONS_GUIDE.md)** | Pendant le développement | *Comment implémenter chaque option, et comment la prouver ?* |
| 5 | **[CHECKLIST_DEFENSE.md](CHECKLIST_DEFENSE.md)** | 2 semaines avant la soutenance | *Sommes-nous prêts ? Comment dérouler la démonstration ?* |

### Documents d'approfondissement (au fil du projet)

| # | Document | À lire quand | Réponse à la question |
|---|----------|--------------|------------------------|
| 6 | **[OUTILS.md](OUTILS.md)** | Configuration de l'environnement | *Quels outils installer et pour quoi faire ?* |
| 7 | **[MEMORY_STRATEGY.md](MEMORY_STRATEGY.md)** | Avant d'optimiser la mémoire | *Faut-il suivre la méthode « sans malloc » de Webserv ?* |
| 8 | **[DISTRIBUTED_RENDERING.md](DISTRIBUTED_RENDERING.md)** | Après le jalon J2 | *Comment lancer les calculs sur d'autres PC (2 points) ?* |
| 9 | **[INSPIRATION_BLENDER.md](INSPIRATION_BLENDER.md)** | Pour consolider le design | *Comment un logiciel 3D de référence est-il architecturé ?* |

```
SPECIFICATIONS ──► ARCHITECTURE ──► OPTIONS_GUIDE ──► CHECKLIST_DEFENSE
       │                │                                     ▲
       └──────────► PLAN_TRAVAIL ◄─────────────────────────────┘
                        │
        ┌───────────────┼────────────────┐
        ▼               ▼                ▼
     OUTILS   MEMORY_STRATEGY   DISTRIBUTED_RENDERING
                                          │
                                          ▼
                                  INSPIRATION_BLENDER
```

---

## 3. Documentation existante (branche `main`)

Le dépôt contient déjà, sur la branche `main`, une documentation technique héritée de la phase
de développement. Elle reste **valable et complémentaire** :

| Fichier | Rôle | Où le trouver |
|---------|------|---------------|
| `README.md` | Installation, build, utilisation | `git show main:README.md` |
| `AGENTS.md` | Référence rapide des commandes et de l'état des modules | `git show main:AGENTS.md` |
| `docs/GIT_STRATEGY.md` | Stratégie de branches et de merge détaillée | `git show main:docs/GIT_STRATEGY.md` |
| `docs/TASK_BACKLOG.md` | Backlog d'origine (46 tâches, 14 phases) | `git show main:docs/TASK_BACKLOG.md` |
| `docs/IMPLEMENTATION_GUIDE.md` | Guide de démarrage par développeur + pièges classiques | `git show main:docs/IMPLEMENTATION_GUIDE.md` |
| `docs/SCENE_INFO.md` | Référence du format de scène `.rt` **actuel** | `git show main:docs/SCENE_INFO.md` |
| `SUGGEST.md` | Propositions d'interface interactive (historique) | `git show main:SUGGEST.md` |

> **Ces fichiers ne sont pas présents sur la branche courante** (`docs_tasks`) : consultez-les
> avec les commandes ci-dessus, ou basculez avec `git checkout main`.

Les nouveaux documents de ce dossier **ne remplacent pas** ces fichiers : ils ajoutent la
couche « spécification / évaluation / planification » qui manquait.

---

## 4. Convention de rédaction

- Langue : **français**, avec conservation des termes techniques anglais tels qu'ils
  apparaissent dans la fiche d'évaluation (`expose`, `bump mapping`, `shadows`, …).
- Toute affirmation issue d'un **audit statique du code** est marquée `⚠ audit statique`
  et doit être revalidée par un test avant d'être considérée comme acquise.
- Tout point non tranché par le sujet ou la fiche est regroupé dans la section
  **« Points ouverts »** de [SPECIFICATIONS.md](SPECIFICATIONS.md) et doit être arbitré par l'équipe.
