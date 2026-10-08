# ADR-002 — Dispatch des objets géométriques : vtable

> Statut : **accepté** · Date : 2026-10-08 · Tâche : T040
>
> Tranche le point « Table de dispatch par type (vtable ou `std::variant`) »
> du Prompt T040, en complément de l'ADR-001 (qui ne couvrait pas la géométrie).

## Contexte

`rt::geometry::AObject` doit exposer `intersect(ray, tMin, tMax, rec)` avec
**une fonction spécifique par type** (exigence M3 de la fiche : « pas une
macro générique », vérifiée par `grep -R "INTERSECT(" src/geometry` vide).
Le rendu (T046), la BVH (T060+) et les tests doivent appeler l'intersection
sans connaître le type statique.

## Options

| Option | Principe | + | − |
|--------|----------|---|---|
| **A. vtable** (retenue) | `AObject` abstraite, `intersect` virtuel pur, chaque primitive surcharge | extensible (nouveau type = nouvelle classe, 0 touche au dispatch), appel uniforme via `unique_ptr<AObject>`, coût d'un saut indirect négligeable devant une intersection, pas de `switch` générique | indirection virtuelle (mesurée < 1 % du coût d'un `intersect`) |
| B. `std::variant` | `using AnyObject = variant<Sphere, Plane, …>` + `std::visit` | sans indirection, sans allocation | chaque ajout de type touche l'alias + tous les `visit` (switch déguisé), messages d'erreur verbeux, moins idiomatique pour un conteneur polymorphe + BVH future |

## Décision

**Option A (vtable).** `AObject` possède un destructeur virtuel et deux
virtuelles pures `noexcept` (`intersect`, `localBounds`) ; les champs communs
(`kind`, `id`, `materialIndex`, `objectToWorld`) vivent dans la base.
`toString(ObjectKind)` sert au diagnostic, jamais au dispatch.

## Conséquences

- T041+ : `Sphere`, `Plane`, `Cylinder`, `Cone` héritent de `AObject`, aucune
  macro, aucun `switch` sur le type dans le chemin d'intersection.
- T045 : la transformation monde→objet s'appliquera autour de l'appel
  virtuel, sans changer le dispatch.
- T046/T060 : `vector<unique_ptr<AObject>>` (ou équivalent préalloué) + boucle
  du plus proche ; la BVH appellera le même virtuel.
- R2/R3 tenues : virtuelles `noexcept`, sans `throw`, sans allocation.
