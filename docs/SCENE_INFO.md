# Scène RT - Fichier de configuration

## Directives de scène

### Couleur de fond : `bg`
Créer un fond de scène avec une couleur.

```
bg 0.3 0.3 0.35    # Gris-bleu doux
bg 1.0 1.0 1.0    # Blanc
bg 0.0 0.0 0.0    # Noir
```

**Syntaxe :**
```
bg r g b
```

**Paramètres :**
- `r`: Rouge (0.0-1.0)
- `g`: Vert (0.0-1.0)
- `b`: Bleu (0.0-1.0)


### Lumière ambiante : `A`
Ajoute une lumière ambiante (ou `A` avec une intensité pour du blanc).

```
A 0.4 0.4 0.4           # Lumière ambiante grise
A 0.8               # Lumière ambiante blanche avec intensité 0.8
```

**Syntaxe :**
```
A r g b                  # Lumière ambiante colorée
A intensity              # Lumière ambiante blanche
```

**Paramètres :**
- `r,g,b` ou `intensity`: Composantes de couleur ou intensité (0.0-1.0)


### Lumière ponctuelle : `L`
Crée une lumière ponctuelle à une position.

```
L 8.0 8.0 8.0 1.0 1.0 0.9 1.0    # Lumière ponctuelle blanche chaude
L 0.0 5.0 0.0 1.0 0.8 0.2 0.5    # Lumière ponctuelle orange ambrée
L -3.0 2.0 -1.0 0.2 0.2 0.8 1.2  # Lumière ponctuelle bleue froide
```

**Syntaxe :**
```
L px py pz r g b intensity
```

**Paramètres :**
- `px, py, pz`: Position de la lumière
- `r, g, b`: Couleur (0.0-1.0)
- `intensity`: Puissance de la lumière (0.0-1.0)


### Lumière directionnelle : `directional`
Crée une lumière directionnelle (comme le soleil).

```
directional 1.0 1.0 0.5 1.0 1.0 0.8 0.95    # Lumière directionnelle estivale chaude
```

**Syntaxe :**
```
directional dx dy dz r g b intensity
```

**Paramètres :**
- `dx, dy, dz`: Direction de la lumière (vecteur unitaire)
- `r, g, b`: Couleur (0.0-1.0)
- `intensity`: Puissance de la lumière (0.0-1.0)


### Caméra : `c`
Définit la caméra et son orientation.

**Syntaxe avec vecteur up explicite (10 valeurs) :**
```
c 0.0 2.5 10.0 0.0 1.0 -1.2 0.0 1.0 0.0 45.0
```

**Syntaxe avec up par défaut (7 valeurs) :**
```
c 0.0 3.0 12.0 0.0 0.5 -2.0 40.0
```

**Syntaxe avec up par défaut (5 valeurs) :**
```
c 0.0 3.0 12.0 0.0 0.5 -2.0
```

**Paramètres :**
- Variante 10 valeurs : `px py pz lx ly lz ux uy uz fov`
  - `px, py, pz`: Position de la caméra
  - `lx, ly, lz`: Direction avant (c'est-à-dire où la caméra regarde)
  - `ux, uy, uz`: Vecteur up
  - `fov`: Champ de vision en degrés
- Variante 7 valeurs : `px py pz lx ly lz fov` (avec up=(0,1,0))
- Variante 5 valeurs : `px py pz lx ly lz` (avec up=(0,1,0) et fov=45)


### Sphère : `sp`
Crée une sphère.

```
sp 0.0 0.5 0.0 1.0    # Petite sphère grise au centre
sp 1.0 -0.5 2.0 0.8   # Plus grande sphère orange décalée
sp -1.5 1.0 -2.0 0.3  # Petite sphère rouge
```

**Syntaxe :**
```
sp cx cy cz radius
```

**Paramètres :**
- `cx, cy, cz`: Centre de la sphère
- `radius`: Rayon de la sphère (longueur positive)


### Plan : `pl`
Crée un plan infini.

```
pl 0.0 -2.0 0.0 0.0 1.0 0.0    # Sol (à l'origine, est orienté vers le haut)
pl 0.0 0.0 0.0 0.0 1.0 0.0     # Sol mondial (à l'origine, est orienté vers le haut)
pl 1.0 1.0 1.0 0.0 1.0 0.0     # Plan décalé vers le haut à l'origine
pl 0.0 0.0 5.0 0.0 0.0 1.0     # Plafond
```

**Syntaxe :**
```
pl px py pz nx ny nz
```

**Paramètres :**
- `px, py, pz`: Un point sur le plan
- `nx, ny, nz`: Vecteur normal (doit être unitaire)


### Cylindre : `cy`
Crée un cylindre (segmenté avec des caps).

```
cy 0.0 -1.5 -4.0 0.0 1.0 0.0 0.5 1.5    # Cylindre vertical vertical
```

**Syntaxe :**
```
cy cx cy cz ax ay az radius height
```

**Paramètres :**
- `cx, cy, cz`: Centre du cylindre (milieu entre les caps)
- `ax, ay, az`: Vecteur d'axe directeur (doit être unitaire)
- `radius`: Rayon du cylindre
- `height`: Hauteur totale du cylindre segmenté


### Cône : `co`
Crée un cône (segmenté avec une base circulaire).

```
co 2.5 -1.8 -6.0 0.0 1.0 0.0 25.0 1.8
```

**Syntaxe :**
```
co ax ay az dx dy dz halfAngle height
```

**Paramètres :**
- `ax, ay, az`: Point d apex du cône
- `dx, dy, dz`: Direction du cône (direction loin du sommet)
- `halfAngle`: Angle demi-ouverture en degrés
- `height`: Hauteur du cône à partir du sommet


### Matériau : `material`
Crée un matériau (appliqué au dernier objet créé).

```
sp 0.5 -0.5 0.0 0.7
material 0.9 0.2 0.1 0.8 0.6 0.1 32.0 0.1
```

**Syntaxe :**
```
material r g b amb diff spec shininess reflect
```

**Paramètres :**
- `r, g, b`: Couleur de base (0.0-1.0)
- `amb`: Coefficient de réflexion ambiante (0.0-1.0)
- `diff`: Coefficient de réflexion diffuse (0.0-1.0)
- `spec`: Coefficient de réflexion spéculaire (0.0-1.0)
- `shininess`: Shininess phong (0.0-128.0 généralement)
- `reflect`: Coefficient de réflexion (0.0-1.0, 0.0=pas de réflexion)

**Parser quirk :** La directive `material` est postfixée — elle s'applique au dernier objet déclaré.

## Exemples de scènes complètes

### Scène minimale (fond + lampe + sphère)

```
bg 0.2 0.2 0.25
A 0.1 0.1 0.1
L 5.0 10.0 5.0 1.0 1.0 1.0 0.8
c 0.0 0.0 8.0 0.0 0.0 -1.0 60.0
sp 0.0 0.0 0.0 1.0
material 0.8 0.3 0.3 0.2 0.4 0.3 16.0 0.1
```

### Scène complexe (tous types d'objets)

```
bg 0.15 0.15 0.18
A 0.05 0.05 0.08
L 0.0 8.0 0.0 1.0 0.8 0.8 0.9
directional 0.2 0.5 0.1 0.9 0.9 0.85 0.4
c 0.0 1.5 15.0 0.0 0.2 -1.0 50.0

# Sol
pl 0.0 -2.0 0.0 0.0 1.0 0.0
material 0.2 0.2 0.15 0.3 0.3 0.1 5.0 0.0

# Cylindre principal
sp 1.0 0.0 -2.0 0.6
material 0.6 0.8 1.0 0.1 0.3 0.8 32.0 0.2

# Cylindre vertical
cy 0.0 -1.0 -4.0 0.0 1.0 0.0 0.4 2.5
material 0.3 0.8 0.3 0.2 0.1 0.1 8.0 0.15

# Cône de couverture
co -1.0 -1.5 -3.0 0.0 1.0 0.0 30.0 1.2
material 0.8 0.4 0.2 0.2 0.2 0.1 16.0 0.0
```

## Notes rapides pour les syntaxes

- **Couleur** : formatée comme trois nombres flottants 0.0-1.0 (ou un seul nombre pour le blanc)
- **Positions/vecteurs** : coordonnées x,y,z souvent exprimées en mètres
- **Unités** : toutes les distances dans les mêmes unités (distance de la caméra typique : 5-20)
- **Normales** : doivent être des vecteurs unitaires (échelle de manière appropriée)
- **Ordre des objets :** les objets ultérieurs masquent les objets précédents si chevauchants

## Pile de matériaux

Le parser met en pile les objets et attache le dernier `material` comme postfixé au sommet de la pile. Si aucun matériau n'est donné, des matériaux par défaut sont utilisés.
