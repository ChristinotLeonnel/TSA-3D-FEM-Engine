# Chapitre 15 — Reconstruction des efforts internes

> **Objectifs**
>
> - Calculer les efforts d'extrémité d'une barre par $f = K_\text{loc}\,d_\text{loc} - f_{eq}$ et connaître leur
>   convention de signe.
> - Obtenir les efforts internes en toute abscisse par l'équilibre d'un tronçon, sous forme de polynômes exacts.
> - Comprendre la condensation statique des rotules My, Mz et la reconstitution de la rotation de la barre à la rotule.
>
> **Prérequis**
>
> - Résolution du système et vecteur des déplacements ([chapitres 13](13-conditions-limites-stabilite.md) et
>   [14](14-solveur-diagnostics.md)).
> - Forces nodales équivalentes ([chapitre 10](10-charges-conditions-limites.md)), conventions de signe
>   ([SIGN_CONVENTIONS.md](../../theory/SIGN_CONVENTIONS.md) §3).
> - Équilibre d'un tronçon de poutre (résistance des matériaux).

## 15.1 Efforts d'extrémité

**Intuition.** Une fois les déplacements des nœuds connus, chaque barre est isolée : ses extrémités ont bougé de
$d_\text{loc}$. La rigidité de la barre dit quels efforts il faut lui appliquer pour cela ; si la barre est chargée,
on retranche la part des efforts déjà « portée » par les forces nodales équivalentes.

**Symboles.** $d_\text{glob}$ : les 12 DDL globaux de l'élément, extraits de $U$ ; $d_\text{loc} = T\,d_\text{glob}$ ;
$K_\text{loc}$ : rigidité locale de la formulation ; $f_{eq}$ : forces nodales équivalentes de la charge répartie
(repère local) ; $f = [N_i\ V_{y,i}\ V_{z,i}\ T_i\ M_{y,i}\ M_{z,i}\ N_j \dots M_{z,j}]^T$ dans l'ordre des DDL.

**Hypothèses.** Élément prismatique, élastique linéaire ; charge répartie linéaire sur toute la longueur.

**Équation.**

$$
f = K_\text{loc}\, d_\text{loc} - f_{eq} .
$$

**Convention.** $f$ contient les forces et moments exercés **sur l'élément** par les nœuds i et j, dans le repère
local : c'est la convention `localForce` d'OpenSees, vérifiée sur un cas de traction ($N_i = -P$,
[VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) §1). Sous une traction $P$, l'élément est tiré vers
$-x$ en i et vers $+x$ en j : $f_0 = -P$, $f_6 = +P$.

**Interprétation.** Les forces nodales équivalentes exactes sont l'opposé des forces d'encastrement parfait
(théorème de Betti, [THEORY.md](../../theory/THEORY.md) §4) ; $-f_{eq}$ ajoute donc les efforts d'encastrement
parfait aux efforts dus aux déplacements nodaux : c'est la superposition classique de la méthode des déplacements.

**Algorithme** (`LinearStaticAnalysis::run`, puis constructeur de `BeamResponse`) : extraire $d_\text{glob}$ de $U$ ;
$d_\text{loc} = T\,d_\text{glob}$ ; recalculer $f_{eq}$ ; si l'élément a des rotules, compléter $d_\text{loc}$
(§15.4) ; $f = K_\text{loc}\,d_\text{loc} - f_{eq}$.

**Limites.** $f$ est exact pour la formulation choisie ; il ne contient pas d'effet du second ordre.

## 15.2 Efforts internes par équilibre d'un tronçon

**Intuition.** Au lieu d'interpoler les efforts entre les nœuds, on coupe la barre en $x$ et on écrit l'équilibre du
tronçon $[0, x]$ : efforts d'extrémité en i plus charge répartie entre $0$ et $x$. Avec une charge linéaire, le
résultat est un polynôme, connu exactement en tout point.

**Symboles.** Charge locale $q(s) = a + b\,s$ par composante ($a = q(0)$, $b = (q(L) - q(0))/L$) ; efforts internes
$N, V_y, V_z, T, M_y, M_z$ : résultantes sur la **face positive** de la coupe (normale $+x$ local).

**Équations** (celles du code, `BeamResponse`) :

$$
\begin{aligned}
N(x)   &= -f_{0} - \int_0^x q_x\,ds, &
V_y(x) &= -f_{1} - \int_0^x q_y\,ds, &
V_z(x) &= -f_{2} - \int_0^x q_z\,ds, \\
T(x)   &= -f_{3}, &
M_z(x) &= -f_{5} + x\,f_{1} + \int_0^x (x-s)\,q_y\,ds, &
M_y(x) &= -f_{4} - x\,f_{2} - \int_0^x (x-s)\,q_z\,ds,
\end{aligned}
$$

avec $\int_0^x q\,ds = a x + b x^2/2$ et $\int_0^x (x-s)\,q\,ds = a x^2/2 + b x^3/6$.

**Interprétation.**

- Les signes vérifient $dN/dx = -q_x$, $dV/dx = -q$, $dM_z/dx = -V_y$ et $dM_y/dx = +V_z$
  ([SIGN_CONVENTIONS.md](../../theory/SIGN_CONVENTIONS.md) §3) ; la différence de signe entre les deux plans vient de
  la règle de la main droite ($\theta_y = -dw/dx$).
- $N$ et $V$ sont de degré au plus 2, $M$ de degré au plus 3, $T$ est constant (pas de couple réparti).
- **Exactitude** : ce ne sont pas des approximations éléments finis, mais les efforts exacts de la barre pour la
  charge donnée. Les stations (11 par défaut, `AnalysisOptions::stationsPerElement`) ne sont qu'un choix
  d'affichage : le polynôme est connu partout.
- **Contrôle d'équilibre local** : en $x = L$, les efforts internes doivent égaler les efforts d'extrémité $f_j$
  (forces exercées sur l'élément par le nœud j, sur sa face positive) ; c'est vérifié par
  `Integration_ResponseConsistentWithNodalDisplacements`.

**Algorithme.** Construire six objets `Polynomial` (coefficients, évaluation par Horner, primitive exacte) à partir
de $f$ et de $(a, b)$ ; `forcesAt(x)` évalue les six polynômes.

**Exemple vérifiable.** Console de 3 m encastrée en $x = 0$, charge uniforme locale $q_z = -8$ kN/m. Par équilibre :
$f_2 = -q L = 24$ kN et $f_4 = q L^2/2 = -36$ kN·m. D'où

$$
V_z(x) = q\,(L - x), \qquad M_y(x) = -\frac{q\,(L-x)^2}{2},
$$

soit $V_z(0) = -24$ kN, $M_y(0) = 36$ kN·m, et à mi-longueur $V_z(1{,}5) = -12$ kN, $M_y(1{,}5) = 9$ kN·m.
Valeurs recalculées en Python ; test `Analytical_CantileverUniformLoad_ExactAlongMember`, qui compare **chaque
station** à la parabole exacte, pour les deux formulations.

**Limites.** Couple réparti, charge ponctuelle et charge partielle non pris en charge : les efforts seraient alors
polynomiaux par morceaux, ce que la classe ne sait pas représenter ([chapitre 21](21-extensions.md)).

## 15.3 Efforts dans TSA

Dans TSA, l'adaptateur ne reprend pas les stations de Fem3D : il transmet les efforts d'extrémité $f$ (même convention
que `localForce` d'OpenSees) à la fonction de reconstruction commune de TSA, `beamResultsFromEndForces`, qui produit
les stations exactement comme pour OpenSees ([chapitre 18](18-integration-tsa-opensees.md)). Les deux moteurs sont
ainsi présentés de la même façon.

## 15.4 Rotules : condensation statique et reconstitution

**Intuition.** Une rotule de flexion en extrémité de barre annule le moment correspondant ; la rotation de la barre à
cet endroit devient indépendante de celle du nœud. On élimine ce DDL « interne » au niveau de l'élément avant
l'assemblage, puis on le retrouve après la résolution.

**Symboles.** $a$ : DDL locaux conservés ; $b$ : DDL relâchés parmi $\theta_{y,i}$ (rang 4), $\theta_{z,i}$ (5),
$\theta_{y,j}$ (10), $\theta_{z,j}$ (11) (`EndReleases::mask`).

**Hypothèses.** Seuls My et Mz sont relâchables ; ni N, ni V, ni T ; pas de rotule semi-rigide
([THEORY.md](../../theory/THEORY.md) §6). La méthode est **commune** aux deux formulations, qui restent inchangées.

**Équations.** La condition $f_b = K_{ba} d_a + K_{bb} d_b - f_{eq,b} = 0$ donne :

$$
d_b = K_{bb}^{-1}\,(f_{eq,b} - K_{ba}\,d_a), \qquad
K_c = K_{aa} - K_{ab} K_{bb}^{-1} K_{ba}, \qquad
f_c = f_{eq,a} - K_{ab} K_{bb}^{-1} f_{eq,b} .
$$

$K_c$ (lignes et colonnes $b$ nulles) est assemblée à la place de $K_\text{loc}$ ; $f_c$ à la place de $f_{eq}$
([chapitre 12](12-assemblage.md) §12.3). $K_{bb}$, sous-matrice de flexion de dimension 1 à 4, est définie positive ;
elle est factorisée par `Eigen::LDLT`.

**Reconstitution.** Après la résolution, `completeReleasedDisplacements` remplace les composantes $b$ de $d_\text{loc}$
par $d_b$ : ce sont les **rotations de la barre** à la rotule, distinctes de la rotation du nœud. Les efforts sont
ensuite calculés avec la rigidité **non condensée** et le vecteur complété, $f = K_\text{loc}\,[d_a; d_b] - f_{eq}$ ;
les composantes $f_b$, nulles à l'arrondi près, sont mises **exactement** à zéro. La reconstruction du §15.2
s'applique alors sans modification.

**Exemples vérifiables.**

- Rigidité condensée d'une barre de 4 m relâchée en $\theta_{z,j}$ ($E I_z = 236\,250$ kN·m²) :
  $3EI/L^3 = 11\,074{,}22$ kN/m, $3EI/L^2 = 44\,296{,}88$ kN, $3EI/L = 177\,187{,}5$ kN·m (console sur appui) ;
  recalculé en Python par la formule de $K_c$ sur la matrice d'Euler–Bernoulli ; test
  `Releases_CondensedStiffnessIsProppedCantilever`.
- Console sur appui (rotule My en j), $L = 5$ m, $q = -12$ kN/m : $|M_i| = qL^2/8 = 37{,}5$ kN·m,
  $R_i = 5|q|L/8 = 37{,}5$ kN, $R_j = 3|q|L/8 = 22{,}5$ kN ; test `Releases_ProppedCantileverOneHinge`.
- Barre bi-articulée de 6 m entre nœuds encastrés, $q = -10$ kN/m : rotation de la barre à la rotule
  $|q| L^3/(24 E I_y) = 1{,}3714 \cdot 10^{-4}$ rad, alors que le nœud encastré ne tourne pas ; test
  `Releases_PinnedBeamBetweenClampedNodes`.

**Limites.** Une rotule au droit d'un encastrement de console produit un mécanisme ([chapitre 13](13-conditions-limites-stabilite.md)).
En Timoshenko, les rotules sur un axe local qui n'est pas un axe global n'ont pas de référence OpenSees (cas 16
« non comparable », [VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) §2).

## À retenir

- $f = K_\text{loc}\,d_\text{loc} - f_{eq}$ : forces exercées sur l'élément, repère local, convention `localForce`.
- Efforts internes par équilibre du tronçon $[0, x]$ : polynômes exacts ($N, V$ : degré ≤ 2 ; $M$ : degré ≤ 3).
- Rotules : condensation statique commune aux deux formulations ; rotation de la barre reconstituée ; moments
  relâchés exactement nuls.

## Dans le code source

- Efforts d'extrémité et polynômes : [`BeamResponse::BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98),
  [`BeamResponse::forcesAt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L100-L103), déclaration [`BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/BeamResponse.h#L41-L64).
- Forces nodales équivalentes : [`equivalentNodalLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L31-L51) ; charge locale : [`LocalLinearLoad::fromLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L18-L29).
- Polynômes : [`Polynomial`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Polynomial.h#L14-L74) ; efforts : [`SectionForces`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h#L39-L47).
- Extraction de $d_\text{loc}$ : [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115).
- Rotules : [`EndReleases`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/EndReleases.h#L16-L38), [`condensedStiffness`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L34-L45), [`condensedLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L47-L58),
  [`completeReleasedDisplacements`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L60-L72), [`IBeamElement3D::effectiveLocalStiffnessMatrix`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L13-L16).

## Vérifier par les tests

| Test ou cas | Vérification |
|---|---|
| [`Analytical_CantileverUniformLoad_ExactAlongMember`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L102-L123) | $V_z$, $M_y$ exacts à chaque station |
| [`Analytical_CantileverTipLoad_BothPlanesBothFormulations`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L75-L100) | signes : $M_y(0) = -FL$, $M_z(0) = +FL$, $V = +F$ |
| [`Integration_ResponseConsistentWithNodalDisplacements`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L111-L135) | efforts internes en $L$ = efforts d'extrémité j |
| [`Analytical_FixedFixedTriangularLoad_Timoshenko`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L40-L57) | moments d'encastrement exacts en Timoshenko |
| [`Releases_CondensedStiffnessIsProppedCantilever`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp#L43-L57) | $K_c$ : $3EI/L^3$, $3EI/L^2$, $3EI/L$ |
| [`Releases_ProppedCantileverOneHinge`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp#L83-L96) | $qL^2/8$, $5qL/8$, $3qL/8$ |
| [`Releases_PinnedBeamBetweenClampedNodes`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp#L59-L81) | moments nuls aux rotules, rotation de barre |
| [`Analytical_ProppedCantileverTriangularLoad_Timoshenko`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L59-L83) | rotule et charge triangulaire en Timoshenko |
| [`Releases_ResponseConsistentInFrame`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp#L146-L183) | portique articulé : équilibre, cohérence de la déformée |
| Cas OpenSees 01 à 20 | efforts d'extrémité comparés à `localForce` (valeurs `effort_extremite` des CSV) |

Lancer : `build\fem3d_tests.exe` ; `build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier]`.

## Références

- B12 Wilson (1974), *The static condensation algorithm* — vérifié (notice Crossref), texte non lu.
- A6 Przemieniecki, ch. 6 (condensation des matrices de rigidité) — vérifié (table des matières partielle).
- A9 Timoshenko, Gere, *Mechanics of Materials* — vérifié (notice) ; équilibre d'un tronçon : chapitres **à vérifier**.
- A7 McGuire et al. (relâchements, forces d'encastrement) — vérifié (notice), chapitres **à vérifier**.
- OpenSees `localForce`, `-releasey`/`-releasez` d'`elasticBeamColumn` — vérifié (contenu de la documentation),
  [REFERENCES.md](../../references/REFERENCES.md) §C6.

---

Précédent : [Chapitre 14 — Solveur numérique et diagnostics](14-solveur-diagnostics.md) · Suivant :
[Chapitre 16 — Déplacements, rotations et contraintes](16-deplacements-contraintes.md)
