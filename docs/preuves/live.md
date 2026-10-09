# Démonstration live (T109, *Environment 3*)

> Modifier caméra, objet, couleurs et texture **sans relancer le programme**.
> Chaque édition lève `sceneDirty` → aperçu 1 spp immédiat → affinage au `spp`
> cible (machine `Interactive`, T076). Scène : `scenes/live.rt`.

## Lancement

```sh
./rt scenes/live.rt --window
```

## Protocole en 5 gestes (devant le correcteur, < 1 min)

1. **Caméra** : glisser la souris (orbite autour de la cible) puis molette (FOV).
   L'image suit en direct (aperçu 1 spp puis affinage), la scène est inchangée.
2. **Couleur** : slider `ambient` (ou `setFirstAlbedo` via l'UI) — la teinte change
   sans redémarrage, aperçu immédiat.
3. **Objet** : slider `obj.x` (−5 … +5) — la sphère glisse sur le sol, la BVH est
   reconstruite (`objectVersion++`, `bvhBuilds` +1 en debug), puis affinage.
4. **Texture** : slider `tex.scale` (0.25 … 8) — le damier de la sphère se resserre
   ou s'étire en direct, sans toucher à la BVH (pas de `version++`).
5. **Capture** : touche `P` (ou bouton `Save PNG`) — le framebuffer courant est écrit
   horodaté dans `docs/preuves/`, sans recalcul.

## Résultat attendu

- Aucun redémarrage entre les gestes 1–4 (le programme tourne toujours).
- Chaque geste montre d'abord un aperçu basse qualité (1 spp) puis l'affinage.
- `displayDirty` seul (expose) ne relance aucun rendu (compteurs `Interactive`).
