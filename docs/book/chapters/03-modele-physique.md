# Chapitre 3 — Modèle physique, hypothèses et degrés de liberté

> **Objectifs**
>
> - Énoncer précisément les hypothèses physiques du modèle de poutre de Fem3D.
> - Connaître les 6 degrés de liberté d'un nœud, leur ordre et leur numérotation globale.
> - Savoir ce que ces 6 DDL ne peuvent pas représenter.
> - Connaître les contrôles d'entrée et les codes d'erreur du moteur.
>
> **Prérequis**
>
> - [Chapitre 2](02-fondements-mef.md) : méthode des déplacements, partition libres / imposés.

## 3.1 Intuition physique

Une poutre est un solide dont une dimension (la longueur) domine les deux autres. On la remplace par sa **ligne
moyenne**, et l'on suppose que chaque section se déplace comme un corps rigide : trois translations et trois rotations.
Toute la cinématique de la section tient alors en 6 nombres, qui sont les degrés de liberté du nœud situé sur la ligne
moyenne.

## 3.2 Hypothèses du modèle

D'après [THEORY.md](../../theory/THEORY.md) §1, et vérifié dans le code :

| Hypothèse | Conséquence dans le code |
|---|---|
| Statique linéaire : petits déplacements, petites rotations | une seule résolution linéaire par cas ([`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115)) |
| Matériau élastique linéaire isotrope, E et G | [`ElasticMaterial`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Materials/ElasticMaterial.h#L11-L22) ne porte que E et G |
| Élément à 2 nœuds, 12 DDL, section constante | une seule [`BeamSection3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Sections/BeamSection3D.h#L18-L40) par élément |
| Axes principaux de la section = axes locaux y et z | aucun produit d'inertie $I_{yz}$ dans la section |
| Centre de gravité et centre de cisaillement confondus | aucun couplage flexion-torsion dans les matrices |
| Torsion de Saint-Venant uniforme, GJ/L | [`addAxialAndTorsion`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L24-L31) ; pas de gauchissement ([chapitre 8](08-torsion-gauchissement.md)) |
| Ni non-linéarité, ni plasticité, ni dynamique | aucune matrice de masse ni rigidité géométrique dans le code |

**Limites.** Une section dissymétrique (cornière) dont les axes principaux sont inclinés doit être orientée par son
vecteur v_xz ([chapitre 4](04-reperes.md)) ; un profil dont le centre de cisaillement est décalé (profil en U) subit en
réalité un couplage flexion-torsion que Fem3D ignore.

## 3.3 Les six degrés de liberté

**Définition des symboles.** Chaque nœud a 6 DDL, **dans le même ordre partout** : matrices, transformations, charges,
résultats ([`enum class Dof`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Types.h#L28-L36)).

| Rang | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| Nœud (repère global) | Ux | Uy | Uz | Rx | Ry | Rz |
| Extrémité d'élément (repère local) | u | v | w | θx | θy | θz |

Le vecteur élémentaire, en repère local, est

$$
\mathbf{d}_e = \left[u_i\ v_i\ w_i\ \theta_{x,i}\ \theta_{y,i}\ \theta_{z,i}\ \ u_j\ v_j\ w_j\ \theta_{x,j}\ \theta_{y,j}\ \theta_{z,j}\right]^{T}.
$$

Les rotations sont des vecteurs, positives selon la règle de la main droite.

**Numérotation globale.** Le DDL global d'un nœud vaut

$$
g = 6 \times (\text{indice interne du nœud}) + \text{rang}.
$$

L'indice interne est la **position du nœud dans l'ordre d'ajout au modèle**, jamais son identifiant
([`DofMap`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Assembly/DofMap.h#L15-L45)). Les DDL sont ensuite répartis en libres ($f$) et imposés
($r$) dans l'ordre croissant de $g$ ([`DofMap::DofMap`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L9-L30)).

**Exemple vérifiable.** Dans le test `Integration_MechanismDetected`, les nœuds ont pour identifiants 10 et 20, ajoutés
dans cet ordre : leurs indices internes sont 0 et 1. Le DDL Rz du nœud 20 a donc pour numéro global
$g = 6 \times 1 + 5 = 11$, et Rx du nœud 10 a pour numéro $g = 3$. Le nœud 10 est articulé (Ux, Uy, Uz bloqués), le
nœud 20 retient Uy et Uz : les DDL imposés sont $\{0, 1, 2, 7, 8\}$ et les 7 autres sont libres. Comme aucun appui ne
retient la rotation autour de l'axe de la barre, le test vérifie que le mécanisme est détecté et que son message
désigne un DDL « Rx ».

## 3.4 Ce que six DDL ne représentent pas

- **Le gauchissement** : la section est supposée se déplacer en bloc ; son gauchissement hors de son plan, le
  bimoment et la torsion non uniforme exigeraient un septième DDL ([chapitre 8](08-torsion-gauchissement.md)).
- **La déformation de la section** (voilement local, distorsion) : hors du modèle de poutre.
- **Les excentrements** : les extrémités de barre coïncident avec les nœuds ; il n'y a pas de bras rigide.

## 3.5 Modes de corps rigide

Un élément libre doit pouvoir se déplacer en bloc sans effort : trois translations et trois rotations
infinitésimales. Sa matrice de rigidité globale a donc **exactement 6 valeurs propres nulles** et aucune négative.
J'ai recalculé les valeurs propres de la matrice locale d'Euler–Bernoulli (L = 4 m, section 0,30 × 0,50 m) avec
numpy : 6 valeurs propres nulles (relativement à la plus grande), 6 positives. Le test
`Element_SymmetryAndRigidBodyModes` le vérifie sur une barre inclinée quelconque, pour les deux formulations.

Conséquence pour une structure : si les appuis ne bloquent pas tous les mouvements de corps rigide, $\mathbf{K}_{ff}$
est singulière ; c'est un **mécanisme**, détecté et désigné par le solveur (chapitre 12 et suivants).

## 3.6 Contrôles d'entrée et codes d'erreur

Avant tout calcul, [`StructuralModel3D::validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L113-L158) contrôle les
références (nœuds, matériaux, sections, éléments), les propriétés (> 0 et finies), la géométrie, les ressorts et les
charges. Toute erreur lève une `Fem3DError` portant un code stable
([`enum class ErrorCode`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Fem3DError.h#L11-L20)) :

| Code | Exemple |
|---|---|
| `InvalidInput` | E ≤ 0, J nul, Timoshenko sans aire de cisaillement, raideur de ressort négative |
| `UnknownReference` | nœud, matériau, section ou cas de charge inexistant |
| `DuplicateId` | identifiant déjà utilisé |
| `DegenerateGeometry` | longueur nulle, v_xz parallèle à l'axe ([chapitre 4](04-reperes.md)) |
| `Mechanism` | matrice $\mathbf{K}_{ff}$ singulière |
| `NoFreeDof` | déclaré dans l'énumération ; **aucune occurrence** dans `src/` à la révision 75700d2 (un modèle sans DDL libre est désormais accepté, [AUDIT.md](../../AUDIT.md) constat 10) |
| `IoError` | écriture de fichier impossible |

## À retenir

- Ligne moyenne + section rigide : 6 DDL par nœud, ordre Ux, Uy, Uz, Rx, Ry, Rz partout.
- $g = 6 \times$ indice interne $+$ rang ; l'indice interne n'est pas l'identifiant du nœud.
- Hypothèses : statique linéaire, élasticité isotrope, axes principaux = axes locaux, centre de cisaillement au centre
  de gravité, torsion uniforme.
- Un élément libre a 6 modes de corps rigide ; une structure mal appuyée est un mécanisme.

## Dans le code source

- Ordre des DDL et tolérances : [`Dof`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Types.h#L28-L36),
  [`Tolerance`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Types.h#L43-L55).
- Numérotation : [`DofMap`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Assembly/DofMap.h#L15-L45),
  [`DofMap::DofMap`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L9-L30).
- Contrôles : [`StructuralModel3D::validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L113-L158),
  [`createElement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L96-L111).
- Codes d'erreur : [`ErrorCode`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Fem3DError.h#L11-L20).

## Vérifier par les tests

```bat
build\fem3d_tests.exe | findstr /C:"RigidBodyModes" /C:"Input_Invalid" /C:"MechanismDetected"
```

- [`Element_SymmetryAndRigidBodyModes`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L65-L82) : symétrie, 6 modes
  de corps rigide sans effort, exactement 6 valeurs propres nulles.
- [`Input_InvalidPropertiesRejected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L121-L134) : E ≤ 0, J nul,
  Timoshenko sans aire de cisaillement, identifiant en double.
- [`Integration_MechanismDetected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L137-L167) : mécanisme de torsion désigné
  (Rx), structure sans appui.
- Cas OpenSees `11_mecanisme` et `17_mecanisme_rotule`.

## Références

- [THEORY.md](../../theory/THEORY.md) §1 ; [SIGN_CONVENTIONS.md](../../theory/SIGN_CONVENTIONS.md) §1 (sources du dépôt).
- A6 — Przemieniecki, *Theory of Matrix Structural Analysis* : notice vérifiée ; ch. 5 à vérifier.
- A7 — McGuire, Gallagher, Ziemian : notice vérifiée ; chapitres à vérifier.

---

Chapitre précédent : [Chapitre 2 — Fondements de la méthode des éléments finis](02-fondements-mef.md) ·
Chapitre suivant : [Chapitre 4 — Repères locaux et globaux](04-reperes.md)
