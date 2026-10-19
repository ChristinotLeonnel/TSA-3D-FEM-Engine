# Chapitre 6 — Élément poutre 3D Euler–Bernoulli

> **Objectifs**
>
> - Comprendre l'hypothèse d'Euler–Bernoulli et ses conséquences.
> - Écrire la matrice de rigidité 12×12 exactement telle que la code `EulerBernoulli3D::localStiffnessMatrix`.
> - Connaître les fonctions de forme d'Hermite et le signe du plan x-z.
> - Vérifier l'élément sur des solutions analytiques.
>
> **Prérequis**
>
> - [Chapitre 2](02-fondements-mef.md) (énergie, rigidité), [chapitre 3](03-modele-physique.md) (DDL),
>   [chapitre 5](05-unites-signes.md) (signes, $\theta_y = -w'$).

## 6.1 Intuition physique

Une poutre élancée fléchit sans que ses sections se déforment par cisaillement : chaque section reste **plane** et
**perpendiculaire** à la ligne moyenne déformée. La rotation d'une section est alors la pente de la ligne moyenne, et
toute la flexion vient de la courbure. C'est une très bonne approximation quand la longueur est grande devant la
hauteur.

## 6.2 Symboles

| Symbole | Signification |
|---|---|
| $E$, $G$ | modules d'Young et de cisaillement |
| $A$, $J$ | aire, constante de torsion de Saint-Venant |
| $I_z$ | moment quadratique autour de z : flexion dans le plan x-y ($v$, $\theta_z$) |
| $I_y$ | moment quadratique autour de y : flexion dans le plan x-z ($w$, $\theta_y$) |
| $L$, $\xi = x/L$ | longueur, abscisse réduite |
| $k_a = EA/L$, $k_t = GJ/L$ | rigidités axiale et de torsion |
| $c_{xy} = EI_z/L^3$, $c_{xz} = EI_y/L^3$ | facteurs de flexion |

## 6.3 Hypothèses

- Celles du [chapitre 3](03-modele-physique.md) (statique linéaire, élasticité isotrope, section constante, axes
  principaux = axes locaux, centre de cisaillement au centre de gravité).
- **Cisaillement transversal négligé** : $\theta_z = v'$, $\theta_y = -w'$, $M_z = EI_z\,v''$.
- Traction, torsion et flexion dans chaque plan sont **découplées**.

## 6.4 Équations

**Équations différentielles** sans charge répartie : $EA\,u'' = 0$, $GJ\,\theta_x'' = 0$, $EI_z\,v'''' = 0$,
$EI_y\,w'''' = 0$. Leurs solutions sont des polynômes de degré 1 (traction, torsion) et 3 (flexion).

**Fonctions de forme** ([`shapeFunctions`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L62-L75)) : axiales $(1-\xi,\ \xi)$ ;
cubiques d'Hermite

$$
H_1 = 1 - 3\xi^2 + 2\xi^3,\quad H_2 = L(\xi - 2\xi^2 + \xi^3),\quad H_3 = 3\xi^2 - 2\xi^3,\quad H_4 = L(-\xi^2 + \xi^3),
$$

avec $v(x) = H_1 v_i + H_2\theta_{z,i} + H_3 v_j + H_4\theta_{z,j}$ et, à cause de $\theta_y = -w'$,
$w(x) = H_1 w_i - H_2\theta_{y,i} + H_3 w_j - H_4\theta_{y,j}$.

**Matrice de rigidité locale** (ordre $u_i, v_i, w_i, \theta_{x,i}, \theta_{y,i}, \theta_{z,i}, u_j, \dots, \theta_{z,j}$),
transcrite des tableaux `kxy` et `kxz` de
[`EulerBernoulli3D::localStiffnessMatrix`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L37-L60) :

$$
\mathbf{k}_{EB} =
\begin{bmatrix}
k_a & 0 & 0 & 0 & 0 & 0 & -k_a & 0 & 0 & 0 & 0 & 0 \\
0 & 12c_{xy} & 0 & 0 & 0 & 6Lc_{xy} & 0 & -12c_{xy} & 0 & 0 & 0 & 6Lc_{xy} \\
0 & 0 & 12c_{xz} & 0 & -6Lc_{xz} & 0 & 0 & 0 & -12c_{xz} & 0 & -6Lc_{xz} & 0 \\
0 & 0 & 0 & k_t & 0 & 0 & 0 & 0 & 0 & -k_t & 0 & 0 \\
0 & 0 & -6Lc_{xz} & 0 & 4L^2c_{xz} & 0 & 0 & 0 & 6Lc_{xz} & 0 & 2L^2c_{xz} & 0 \\
0 & 6Lc_{xy} & 0 & 0 & 0 & 4L^2c_{xy} & 0 & -6Lc_{xy} & 0 & 0 & 0 & 2L^2c_{xy} \\
-k_a & 0 & 0 & 0 & 0 & 0 & k_a & 0 & 0 & 0 & 0 & 0 \\
0 & -12c_{xy} & 0 & 0 & 0 & -6Lc_{xy} & 0 & 12c_{xy} & 0 & 0 & 0 & -6Lc_{xy} \\
0 & 0 & -12c_{xz} & 0 & 6Lc_{xz} & 0 & 0 & 0 & 12c_{xz} & 0 & 6Lc_{xz} & 0 \\
0 & 0 & 0 & -k_t & 0 & 0 & 0 & 0 & 0 & k_t & 0 & 0 \\
0 & 0 & -6Lc_{xz} & 0 & 2L^2c_{xz} & 0 & 0 & 0 & 6Lc_{xz} & 0 & 4L^2c_{xz} & 0 \\
0 & 6Lc_{xy} & 0 & 0 & 0 & 2L^2c_{xy} & 0 & -6Lc_{xy} & 0 & 0 & 0 & 4L^2c_{xy}
\end{bmatrix}
$$

avec $k_a = EA/L$, $k_t = GJ/L$, $c_{xy} = EI_z/L^3$, $c_{xz} = EI_y/L^3$.

**Interprétation.**

- $12c_{xy} = 12EI_z/L^3$ est la force à appliquer en i pour imposer $v_i = 1$, toutes les autres translations et
  rotations étant bloquées ; $4L^2c_{xy} = 4EI_z/L$ est le moment pour une rotation unitaire, et $2EI_z/L$ le moment
  « transmis » à l'autre extrémité.
- Les termes croisés $v$-$\theta_z$ valent $+6EI_z/L^2$, les termes $w$-$\theta_y$ valent $-6EI_y/L^2$ : c'est le
  signe $\theta_y = -w'$, seule différence entre les deux plans.
- Les blocs de traction (DDL 0, 6), de torsion (3, 9), de flexion x-y (1, 5, 7, 11) et x-z (2, 4, 8, 10) n'ont aucun
  terme commun : les quatre comportements sont découplés.

## 6.5 Algorithme

1. [`addAxialAndTorsion`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L24-L31) ajoute $EA/L$ aux DDL 0 et 6 et $GJ/L$ aux DDL
   3 et 9 (code commun aux deux formulations).
2. Les blocs 4×4 `kxy` (multiplié par $EI_z/L^3$) et `kxz` (multiplié par $EI_y/L^3$) sont dispersés aux indices
   `py = {1, 5, 7, 11}` et `pz = {2, 4, 8, 10}`.
3. La matrice globale est $\mathbf{T}^T\mathbf{k}\,\mathbf{T}$ ([chapitre 4](04-reperes.md)), condensée au préalable
   s'il y a des rotules ([`effectiveLocalStiffnessMatrix`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L13-L16)).

**Exactitude.** Les cubiques d'Hermite sont les solutions exactes de $EI\,v'''' = 0$ : la rigidité est exacte pour une
barre prismatique, et un seul élément donne les déplacements nodaux exacts pour des charges nodales
([chapitre 2](02-fondements-mef.md), §2.6). J'ai refait symboliquement (sympy) l'intégrale
$\int_0^L EI\,H_k''H_l''\,\mathrm{d}x$ : elle redonne le bloc $[12,\ 6L,\ 4L^2,\ 2L^2]$ ci-dessus.

## 6.6 Exemples vérifiables

**Termes de la matrice** (test `Element_ExplicitStiffnessTerms`) : L = 4 m, section 0,30 × 0,50 m
($A = 0{,}15$ m², $I_y = 3{,}125\cdot10^{-3}$ m⁴, $I_z = 1{,}125\cdot10^{-3}$ m⁴, $J = 2{,}817371\cdot10^{-3}$ m⁴),
E = 210·10⁶ kN/m², G = 81·10⁶ kN/m². Recalcul Python :

| Terme | Formule | Valeur |
|---|---|---|
| $k_{0,0}$ | $EA/L$ | 7 875 000 kN/m |
| $k_{3,3}$ | $GJ/L$ | 57 051,76 kN·m/rad |
| $k_{1,1}$ | $12EI_z/L^3$ | 44 296,875 kN/m |
| $k_{2,2}$ | $12EI_y/L^3$ | 123 046,875 kN/m |
| $k_{1,5}$ | $+6EI_z/L^2$ | 88 593,75 kN |
| $k_{2,4}$ | $-6EI_y/L^2$ | −246 093,75 kN |

La matrice recalculée est symétrique et a exactement 6 valeurs propres nulles.

**Console sous charge en bout** (test `Analytical_CantileverTipLoad_BothPlanesBothFormulations`, cas OpenSees
`03_flexion_y` et `04_flexion_z`) : L = 3 m, F = 20 kN. J'ai résolu le système 6×6 des DDL libres avec la matrice
ci-dessus :

- $F_z$ : $w = FL^3/(3EI_y) = 2{,}742857\cdot10^{-4}$ m, $\theta_y = -FL^2/(2EI_y) = -1{,}371429\cdot10^{-4}$ rad ;
- $F_y$ : $v = FL^3/(3EI_z) = 7{,}619048\cdot10^{-4}$ m, $\theta_z = +FL^2/(2EI_z) = 3{,}809524\cdot10^{-4}$ rad.

**Console sous charge répartie** (test `Analytical_CantileverUniformLoad_ExactAlongMember`) : L = 3 m,
$q_z = -8$ kN/m : $w(L) = qL^4/(8EI_y) = -1{,}234286\cdot10^{-4}$ m et $M_y(0) = -q L^2/2 = +36$ kN·m. Le test vérifie
aussi, à chaque station, la parabole $M_y(x) = -q(L-x)^2/2$ et la déformée
$w(x) = q x^2 (6L^2 - 4Lx + x^2)/(24EI_y)$ : la réponse est exacte **entre** les nœuds, et pas seulement aux nœuds.

**Poutre sur deux appuis** (test `Analytical_SimplySupportedUniform_OneElement`) : L = 6 m, $q_z = -10$ kN/m, un seul
élément : flèche à mi-portée $5qL^4/(384EI_y) = -2{,}571429\cdot10^{-4}$ m, $M_y(L/2) = qL^2/8 = -45$ kN·m,
$\sigma = |M|\,z_{ext}/I_y = 3600$ kN/m².

## 6.7 Limites

- Cisaillement négligé : pour une poutre courte (L/h de l'ordre de 1 à 5) la flèche est sous-estimée ; voir
  Timoshenko ([chapitre 7](07-timoshenko.md)).
- Torsion uniforme seulement ([chapitre 8](08-torsion-gauchissement.md)).
- Section constante ; pas de couplage flexion-torsion (centre de cisaillement supposé au centre de gravité).
- Dans TSA, c'est la seule formulation utilisée par le moteur `fem3d`.

## À retenir

- Sections planes et normales : $\theta_z = v'$, $\theta_y = -w'$, pas de déformation d'effort tranchant.
- Matrice 12×12 exacte pour une barre prismatique ; termes croisés $+6EI_z/L^2$ (x-y) et $-6EI_y/L^2$ (x-z).
- $I_z$ gouverne le plan x-y, $I_y$ le plan x-z.
- Efforts internes et déformée exacts entre les nœuds, charges réparties comprises.

## Dans le code source

- [`EulerBernoulli3D::localStiffnessMatrix`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L37-L60), dont le plan x-z
  [lignes 49 à 52](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L49-L52).
- [`EulerBernoulli3D::shapeFunctions`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L62-L75).
- [`BeamElementBase::addAxialAndTorsion`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L24-L31).
- Déclaration : [`EulerBernoulli3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/EulerBernoulli3D.h#L11-L21).

## Vérifier par les tests

```bat
build\fem3d_tests.exe | findstr /C:"Element_" /C:"Analytical_Cantilever" /C:"SimplySupported"
```

- [`Element_ExplicitStiffnessTerms`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L95-L111).
- [`Element_SymmetryAndRigidBodyModes`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L65-L82).
- [`Analytical_CantileverTipLoad_BothPlanesBothFormulations`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L75-L100).
- [`Analytical_CantileverUniformLoad_ExactAlongMember`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L102-L123).
- [`Analytical_SimplySupportedUniform_OneElement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L125-L140).
- Cas OpenSees 01 à 10 (`elasticBeamColumn`), voir [VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md).

## Références

- [THEORY.md](../../theory/THEORY.md) §2 ; [FORMULATIONS.md](../../formulations/FORMULATIONS.md) §1. Diagramme :
  [elements-poutre.md](../../diagrams/elements-poutre.md).
- Équivalence avec `elasticBeamColumn` d'OpenSees : vérifiée terme à terme par l'audit indépendant et par les benchmarks.
- A6 — Przemieniecki : notice vérifiée ; ch. 5 à vérifier. A7 — McGuire et al. : notice vérifiée ; chapitres à
  vérifier. A8 — Weaver, Gere : notice vérifiée. A5 — Cook et al. : notice vérifiée. A3 — Bathe : §5.4 à vérifier.

---

Chapitre précédent : [Chapitre 5 — Conventions d'unités et de signes](05-unites-signes.md) ·
Chapitre suivant : [Chapitre 7 — Élément poutre 3D Timoshenko](07-timoshenko.md)
