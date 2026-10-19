# Chapitre 7 — Élément poutre 3D Timoshenko

> **Objectifs**
>
> - Comprendre la déformation d'effort tranchant et la différence entre pente et rotation de section.
> - Connaître les paramètres $\Phi_y = 12EI_z/(GA_{sy}L^2)$ et $\Phi_z = 12EI_y/(GA_{sz}L^2)$ et leur association aux plans.
> - Écrire la matrice 12×12 exactement telle que la code `Timoshenko3D::localStiffnessMatrix`.
> - Savoir pourquoi l'élément de Fem3D ne verrouille pas, et quand Timoshenko est utile.
>
> **Prérequis**
>
> - [Chapitre 6](06-euler-bernoulli.md) (Euler–Bernoulli), [chapitre 9](09-materiaux-sections.md) pour les aires de
>   cisaillement (lecture possible en parallèle).

## 7.1 Intuition physique

Dans une poutre courte et haute, l'effort tranchant fait glisser les sections les unes par rapport aux autres, comme
les cartes d'un paquet. La section reste plane mais n'est plus perpendiculaire à la ligne moyenne : sa rotation
$\theta$ diffère de la pente $v'$ d'un angle de glissement $\gamma$. La flèche totale est la somme d'une flèche de
flexion et d'une flèche d'effort tranchant. Pour une poutre élancée, la seconde devient négligeable.

## 7.2 Symboles

| Symbole | Signification |
|---|---|
| $A_{sy}$, $A_{sz}$ | aires effectives de cisaillement selon y et z (par exemple $\tfrac56 A$ pour un rectangle) |
| $\gamma_{xy}$, $\gamma_{xz}$ | glissements moyens de la section |
| $\Phi_y$ | paramètre de cisaillement du plan x-y (avec $I_z$ et $A_{sy}$) |
| $\Phi_z$ | paramètre de cisaillement du plan x-z (avec $I_y$ et $A_{sz}$) |
| $c_{xy} = \dfrac{EI_z}{(1+\Phi_y)L^3}$, $c_{xz} = \dfrac{EI_y}{(1+\Phi_z)L^3}$ | facteurs de flexion |
| $\mu = 1/(1+\Phi)$ | facteur des fonctions de forme |

## 7.3 Hypothèses

- Celles du [chapitre 3](03-modele-physique.md), plus : sections planes, **non** nécessairement normales à la ligne
  moyenne ; glissement uniforme représenté par une aire effective $A_s$ constante le long de la barre.
- Élément **exact à interpolation interdépendante** : les fonctions de forme sont les solutions exactes des équations
  de Timoshenko pour une barre prismatique sans charge.
- Traction et torsion identiques à Euler–Bernoulli (même code, [`addAxialAndTorsion`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L24-L31)).

## 7.4 Équations

**Relations cinématiques et de comportement** ([THEORY.md](../../theory/THEORY.md) §3) :

$$
\gamma_{xy} = v' - \theta_z = \frac{V_y}{G A_{sy}},\qquad
\gamma_{xz} = w' + \theta_y = \frac{V_z}{G A_{sz}},\qquad
M_z = EI_z\,\theta_z',\quad M_y = EI_y\,\theta_y' .
$$

**Paramètres de cisaillement** ([`shearParameterY`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L91-L95),
[`shearParameterZ`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L97-L101)) :

$$
\Phi_y = \frac{12\,E I_z}{G A_{sy} L^2}\quad(\text{plan x-y}),\qquad
\Phi_z = \frac{12\,E I_y}{G A_{sz} L^2}\quad(\text{plan x-z}).
$$

L'indice suit l'aire de cisaillement : $\Phi_y$ associe $A_{sy}$ à $I_z$, car l'effort tranchant $V_y$ accompagne la
flexion autour de z.

**Matrice de rigidité locale**, transcrite de
[`Timoshenko3D::localStiffnessMatrix`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L103-L134) :

$$
\mathbf{k}_{T} =
\begin{bmatrix}
k_a & 0 & 0 & 0 & 0 & 0 & -k_a & 0 & 0 & 0 & 0 & 0 \\
0 & 12c_{xy} & 0 & 0 & 0 & 6Lc_{xy} & 0 & -12c_{xy} & 0 & 0 & 0 & 6Lc_{xy} \\
0 & 0 & 12c_{xz} & 0 & -6Lc_{xz} & 0 & 0 & 0 & -12c_{xz} & 0 & -6Lc_{xz} & 0 \\
0 & 0 & 0 & k_t & 0 & 0 & 0 & 0 & 0 & -k_t & 0 & 0 \\
0 & 0 & -6Lc_{xz} & 0 & (4+\Phi_z)L^2c_{xz} & 0 & 0 & 0 & 6Lc_{xz} & 0 & (2-\Phi_z)L^2c_{xz} & 0 \\
0 & 6Lc_{xy} & 0 & 0 & 0 & (4+\Phi_y)L^2c_{xy} & 0 & -6Lc_{xy} & 0 & 0 & 0 & (2-\Phi_y)L^2c_{xy} \\
-k_a & 0 & 0 & 0 & 0 & 0 & k_a & 0 & 0 & 0 & 0 & 0 \\
0 & -12c_{xy} & 0 & 0 & 0 & -6Lc_{xy} & 0 & 12c_{xy} & 0 & 0 & 0 & -6Lc_{xy} \\
0 & 0 & -12c_{xz} & 0 & 6Lc_{xz} & 0 & 0 & 0 & 12c_{xz} & 0 & 6Lc_{xz} & 0 \\
0 & 0 & 0 & -k_t & 0 & 0 & 0 & 0 & 0 & k_t & 0 & 0 \\
0 & 0 & -6Lc_{xz} & 0 & (2-\Phi_z)L^2c_{xz} & 0 & 0 & 0 & 6Lc_{xz} & 0 & (4+\Phi_z)L^2c_{xz} & 0 \\
0 & 6Lc_{xy} & 0 & 0 & 0 & (2-\Phi_y)L^2c_{xy} & 0 & -6Lc_{xy} & 0 & 0 & 0 & (4+\Phi_y)L^2c_{xy}
\end{bmatrix}
$$

avec $k_a = EA/L$, $k_t = GJ/L$, $c_{xy} = EI_z/((1+\Phi_y)L^3)$, $c_{xz} = EI_y/((1+\Phi_z)L^3)$. Pour
$\Phi_y = \Phi_z = 0$, on retrouve exactement la matrice du [chapitre 6](06-euler-bernoulli.md).

**Fonctions de forme** ([`Timoshenko3D::shapeFunctions`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L136-L155)), $\mu = 1/(1+\Phi)$ :

$$
\begin{aligned}
N_1 &= \mu\,(1 - 3\xi^2 + 2\xi^3 + \Phi(1-\xi)), &
N_2 &= \mu L\,(\xi - 2\xi^2 + \xi^3 + \tfrac{\Phi}{2}(\xi - \xi^2)),\\
N_3 &= \mu\,(3\xi^2 - 2\xi^3 + \Phi\xi), &
N_4 &= \mu L\,(-\xi^2 + \xi^3 - \tfrac{\Phi}{2}(\xi - \xi^2)).
\end{aligned}
$$

Elles sont utilisées pour les forces nodales équivalentes ; dans le plan x-z, $N_2$ et $N_4$ changent de signe et
portent sur la **rotation de section** $\theta_y$.

**Pente et rotation.** Le long de la barre, les pentes diffèrent des rotations de section :
$\mathrm{d}v/\mathrm{d}x = \theta_z + V_y/(GA_{sy})$ et $\mathrm{d}w/\mathrm{d}x = -\theta_y + V_z/(GA_{sz})$
([`BeamResponse::BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98)).

## 7.5 Interprétation : pas de verrouillage

$\Phi$ est le rapport (à un facteur près) de la souplesse d'effort tranchant à la souplesse de flexion : il varie comme
$(h/L)^2$. Une interpolation **indépendante** et linéaire de $v$ et $\theta$, intégrée exactement, imposerait
artificiellement $\gamma \approx 0$ et deviendrait beaucoup trop raide pour une poutre élancée : c'est le verrouillage
en cisaillement, que l'on combat d'ordinaire par une intégration réduite. L'élément de Fem3D ne connaît pas ce
problème : sa rigidité est la solution exacte, écrite en forme fermée, et elle tend vers Euler–Bernoulli quand
$\Phi \to 0$. Il n'y a **aucune** intégration numérique de la rigidité.

## 7.6 Algorithme

1. Le constructeur exige les aires de cisaillement : `section().validate(true)` lève `InvalidInput` si $A_{sy}$ ou
   $A_{sz}$ manque ou n'est pas strictement positive ([constructeur](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L81-L86)).
2. $\Phi_y$ et $\Phi_z$ sont calculés à partir de $1/(GA_{sy})$ et $1/(GA_{sz})$.
3. Comme en Euler–Bernoulli, traction et torsion sont ajoutées, puis les blocs `kxy` (avec $\Phi_y$) et `kxz` (avec
   $\Phi_z$) sont dispersés aux indices {1, 5, 7, 11} et {2, 4, 8, 10}.

## 7.7 Exemples vérifiables

Section 0,30 × 0,50 m, $A_{sy} = A_{sz} = \tfrac56 A = 0{,}125$ m², E = 210·10⁶ kN/m², G = 81·10⁶ kN/m². Valeurs
recalculées avec Python (la matrice ci-dessus, retranscrite, résolue pour les DDL libres) :

**Console L = 3 m, F = 20 kN** (test `Analytical_CantileverTipLoad_BothPlanesBothFormulations`) :
$\Phi_y = 0{,}031111$, $\Phi_z = 0{,}086420$.

| Grandeur | Euler–Bernoulli | Timoshenko | Formule |
|---|---|---|---|
| $w$ sous $F_z$ | 2,742857·10⁻⁴ m | 2,802116·10⁻⁴ m | $FL^3/(3EI_y) + FL/(GA_{sz})$ |
| $v$ sous $F_y$ | 7,619048·10⁻⁴ m | 7,678307·10⁻⁴ m | $FL^3/(3EI_z) + FL/(GA_{sy})$ |
| $\theta_y$ sous $F_z$ | −1,371429·10⁻⁴ rad | −1,371429·10⁻⁴ rad | $-FL^2/(2EI_y)$ |

La rotation de section en bout ne dépend pas du cisaillement dans ce cas ; la différence pente − rotation vaut
$V/(GA_{sy}) = 20/(81\cdot10^6 \times 0{,}125) = 1{,}975309\cdot10^{-6}$ rad, comme le vérifie le test.

**Terme diagonal** (test `Element_ExplicitStiffnessTerms`), L = 4 m : $\Phi_y = 0{,}0175$ et
$k_{5,5} = (4+\Phi_y)EI_z/((1+\Phi_y)L) = 233\,202{,}55$ kN·m.

**Poutre profonde** (test `Analytical_TimoshenkoDeepBeam_NoShearLocking`) : console L = 1 m, section 0,30 × 1,00 m,
F = 50 kN. Flèche de flexion $3{,}174603\cdot10^{-6}$ m, flèche d'effort tranchant $2{,}469136\cdot10^{-6}$ m, total
$5{,}643739\cdot10^{-6}$ m : le cisaillement fait 43,75 % de la flèche. Un élément ou dix éléments donnent la même
valeur exacte. À L = 100 m, l'écart relatif Timoshenko / Euler–Bernoulli vaut $7{,}8\cdot10^{-5}$, sous le seuil de
10⁻⁴ du test.

**Aires distinctes par plan** (test `Timoshenko_DistinctShearAreasPerPlane`) : avec $A_{sy} = 0{,}5A$ et
$A_{sz} = 0{,}8A$, chaque flèche ne fait intervenir que l'aire de son plan ; une inversion de l'association serait
détectée.

**Charge triangulaire, barre bi-encastrée** (test `Analytical_FixedFixedTriangularLoad_Timoshenko`) : L = 1,5 m,
$q_y$ de 0 à w = 12 kN/m, $A_{sy} = 0{,}5A$, d'où $\Phi_y = 0{,}207407$. Formules de THEORY §4 :
$M_i = wL^2(4+5\Phi)/(120(1+\Phi)) = 0{,}938650$ kN·m, $M_j = wL^2(6+5\Phi)/(120(1+\Phi)) = 1{,}311350$ kN·m,
$R_i = wL(9+10\Phi)/(60(1+\Phi)) = 2{,}751534$ kN.

## 7.8 Limites

- Timoshenko n'est pas « toujours meilleur » : il améliore la flèche des poutres courtes **si** $A_{sy}$ et $A_{sz}$
  sont correctes ; pour les poutres élancées, il coïncide avec Euler–Bernoulli.
- Pour les sections minces à fort gauchissement ou les pièces très courtes (L/h < 1), les deux théories de poutre sont
  insuffisantes (THEORY §3).
- Aire de cisaillement constante, fournie par l'utilisateur ou par les sections prédéfinies ([chapitre 9](09-materiaux-sections.md)).
- **Non exposé dans TSA** : l'adaptateur crée des éléments Euler–Bernoulli, les sections de TSA ne transmettant pas
  d'aires de cisaillement.
- Rotules en Timoshenko sur un axe relâché quelconque : validées analytiquement seulement (cas OpenSees 16 non
  comparable, [VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) §3).

## À retenir

- $\Phi_y = 12EI_z/(GA_{sy}L^2)$ (plan x-y), $\Phi_z = 12EI_y/(GA_{sz}L^2)$ (plan x-z).
- Diagonale de rotation $(4+\Phi)EI/((1+\Phi)L)$, terme de report $(2-\Phi)EI/((1+\Phi)L)$.
- Élément exact, sans verrouillage ni intégration réduite ; $\Phi \to 0$ redonne Euler–Bernoulli.
- Pente $\neq$ rotation : $v' = \theta_z + V_y/(GA_{sy})$, $w' = -\theta_y + V_z/(GA_{sz})$.

## Dans le code source

- [`Timoshenko3D::localStiffnessMatrix`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L103-L134).
- [`shearParameterY`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L91-L95), [`shearParameterZ`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L97-L101).
- [`Timoshenko3D::shapeFunctions`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L136-L155).
- [Constructeur (aires de cisaillement exigées)](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L81-L86) ;
  déclaration [`Timoshenko3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/Timoshenko3D.h#L14-L29).
- Pente et rotation : [`BeamResponse::BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98).

## Vérifier par les tests

```bat
build\fem3d_tests.exe | findstr /C:"Timoshenko" /C:"CantileverTipLoad" /C:"ExplicitStiffness"
```

- [`Element_TimoshenkoTendsToEulerBernoulli`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L84-L93) : $A_s \times 10^9$ ⇒ même matrice et mêmes fonctions de forme.
- [`Element_ExplicitStiffnessTerms`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L95-L111) : $\Phi_y$ et $k_{5,5}$.
- [`Analytical_CantileverTipLoad_BothPlanesBothFormulations`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L75-L100).
- [`Analytical_TimoshenkoDeepBeam_NoShearLocking`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L164-L188).
- [`Timoshenko_DistinctShearAreasPerPlane`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L85-L95).
- [`Analytical_FixedFixedTriangularLoad_Timoshenko`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L40-L57).
- Cas OpenSees 01 à 10 avec `ElasticTimoshenkoBeam`, en particulier `06_poutre_courte` (L/h = 2).

## Références

- [THEORY.md](../../theory/THEORY.md) §3 et §4 ; [FORMULATIONS.md](../../formulations/FORMULATIONS.md) §2 ;
  dérivation indépendante dans [INDEPENDENT_FORMULATION_AUDIT.md](../../verification/INDEPENDENT_FORMULATION_AUDIT.md).
- Même forme que `ElasticTimoshenkoBeam3d.cpp` d'OpenSees (`setUp()`) : **vérifié (contenu)**, REFERENCES.md §C2.
- B1 — Timoshenko (1921) ; B5 — Davis, Henshell, Warburton (1972) ; B9 — Friedman, Kosmatka (1993) ; B10 — Reddy
  (1997) ; B11 — Bazoune, Khulief, Stephen (2003) : notices vérifiées (Crossref), textes non lus.
- B8 — Prathap, Bhashyam (1982), verrouillage : notice vérifiée. A4 — Hughes, ch. 4 et 5 : à vérifier.

---

Chapitre précédent : [Chapitre 6 — Élément poutre 3D Euler–Bernoulli](06-euler-bernoulli.md) ·
Chapitre suivant : [Chapitre 8 — Torsion de Saint-Venant et gauchissement](08-torsion-gauchissement.md)
