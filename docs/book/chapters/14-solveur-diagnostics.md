# Chapitre 14 — Solveur numérique et diagnostics

> **Objectifs**
>
> - Comprendre la factorisation LDLᵀ creuse employée (`Eigen::SimplicialLDLT`) et pourquoi elle est calculée une
>   seule fois.
> - Connaître le critère de mécanisme de Fem3D (pivot relatif à la diagonale propre de son DDL) et savoir pourquoi il
>   ne dépend pas des unités.
> - Savoir comment les DDL d'un mécanisme sont désignés (modes de valeur propre nulle de $S K_{ff} S$).
> - Interpréter les deux avertissements non bloquants : quasi-mécanisme et résidu de résolution.
>
> **Prérequis**
>
> - Système partitionné et mécanismes ([chapitre 13](13-conditions-limites-stabilite.md)).
> - Élimination de Gauss, factorisation de Cholesky ou LDLᵀ, valeurs propres d'une matrice symétrique.

Le diagramme [stabilite.md](../../diagrams/stabilite.md) montre l'enchaînement des décisions décrites ici ; le
diagramme [calcul.md](../../diagrams/calcul.md) situe le solveur dans le calcul complet.

## 14.1 Factorisation LDLᵀ creuse

**Intuition.** $K_{ff}$ est symétrique, creuse et, pour une structure bien retenue, définie positive. On la décompose
une fois pour toutes en facteurs triangulaires ; chaque cas de charge ne demande ensuite que deux descentes
triangulaires, bien moins coûteuses. On n'inverse jamais la matrice.

**Symboles.** $P$ : permutation symétrique qui réduit le remplissage ; $L$ : triangulaire inférieure à diagonale
unité ; $D = \mathrm{diag}(d_1,\dots,d_{n_f})$ : pivots.

**Équation.**

$$
P\,K_{ff}\,P^T = L\,D\,L^T ,
\qquad
U_f = P^T L^{-T} D^{-1} L^{-1} P\,(F_f - K_{fr} U_r) .
$$

**Hypothèses.** `SimplicialLDLT` est documentée pour les matrices autoadjointes **définies positives**, avec par
défaut une permutation `AMDOrdering` ([REFERENCES.md](../../references/REFERENCES.md) §C8, contenu vérifié). La
factorisation n'utilise pas de racine carrée.

**Interprétation.** Pour une matrice définie positive, tous les pivots sont strictement positifs. Un pivot nul,
négatif ou négligeable trahit une matrice singulière : c'est ce que le diagnostic exploite (§14.2). La documentation
d'Eigen ne garantit pas formellement le comportement de la factorisation sur une matrice **non** définie positive :
le diagnostic repose donc sur un comportement couvert par des tests (en kN-m et en N-mm), et non sur une garantie
de la bibliothèque ([REFERENCES.md](../../references/REFERENCES.md) §C8).

**Algorithme** (`LinearSystemSolver::factorize`, appelé une fois par le constructeur de `LinearStaticAnalysis`) :

1. si $n_f = 0$ : aucun calcul (modèle valable, [chapitre 13](13-conditions-limites-stabilite.md) §13.2) ;
2. relever la diagonale $K_{ii}$ ;
3. `compute` ; si `info()` n'est pas `Success`, la matrice est déclarée singulière (cas d'un pivot exactement nul) ;
4. sinon, examiner chaque pivot (§14.2) ;
5. `solve` : la même factorisation sert à tous les cas de charge (`LinearStaticAnalysis::runAll`).

**Limites.** Solveur direct en mémoire, séquentiel ; aucune option de solveur itératif. La numérotation AMD est celle
d'Eigen par défaut : le code ne la choisit pas explicitement (TSA l'inscrit comme « AMD (Eigen) » dans la
traçabilité du calcul).

## 14.2 Critère de mécanisme relatif à la diagonale propre

**Intuition.** Lors de l'élimination, le pivot $d_k$ est ce qui reste de la rigidité du $k$-ième DDL une fois les DDL
précédents éliminés. S'il ne reste presque rien de la rigidité **propre** de ce DDL, ce DDL n'est retenu par rien :
c'est un mécanisme.

**Équation.** Avec $\tilde K = P K_{ff} P^T$ :

$$
\text{mécanisme} \iff \exists k :\ \tilde K_{kk} \le 0 \ \text{ ou }\ d_k \le \varepsilon_\text{piv}\,\tilde K_{kk},
\qquad \varepsilon_\text{piv} = 10^{-12}
\ (\texttt{Tolerance::relativePivot}).
$$

**Pourquoi ce critère ne dépend pas des unités.** Changer d'unités multiplie une ligne et une colonne de $K$ par le
même facteur (par exemple $10^3$ pour une translation exprimée en mm). Le pivot $d_k$ et la diagonale $\tilde K_{kk}$
du **même** DDL sont multipliés par le **même** facteur au carré : leur rapport est invariant. En revanche, comparer
$d_k$ au plus grand terme diagonal de la matrice mélange des raideurs de translation (F/L) et de rotation (F·L) :
leur rapport change avec les unités.

**Exemple vérifiable** (le défaut de la première version). Console d'Euler–Bernoulli de 3 m, section
0,30 × 0,50 m : au nœud libre, $k_{ww} = 12EI/L^3$ et $k_{\theta\theta} = 4EI/L$.

| Système | $k_{ww}$ | $k_{\theta\theta}$ | $k_{\theta\theta}/k_{ww}$ |
|---|---|---|---|
| kN, m | 291 666,7 kN/m | 875 000 kN·m/rad | 3 |
| N, mm | 291 666,7 N/mm | 8,75·10¹¹ N·mm/rad | 3·10⁶ |

Le même modèle voit le rapport entre ses termes diagonaux changer d'un facteur $10^6$ : un seuil relatif au plus
grand terme diagonal accepte le modèle dans un système et le refuse dans l'autre. C'est exactement le défaut relevé
par l'audit indépendant (un modèle stable refusé en N-mm, accepté en kN-m), corrigé et couvert par
`Solver_MechanismCriterionIndependentOfUnits`, qui échoue avec l'ancien critère ([AUDIT.md](../../AUDIT.md),
constat 1). Valeurs recalculées en Python. Autre illustration, tirée de l'audit : la matrice définie positive
$\mathrm{diag}(1,\ 10^{-13})$ était déclarée mécanisme (rapport au plus grand terme : $10^{-13}$), alors que chaque
pivot vaut exactement sa diagonale propre (rapport 1).

**Algorithme.** Dans la boucle sur les pivots : `ownDiagonal = P · diag(K_ff)` ; test ci-dessus ; mémorisation du
plus petit rapport $d_k/\tilde K_{kk}$ et de son DDL d'origine (indice de la permutation inverse).

**Limites.** Le seuil $10^{-12}$ est une tolérance numérique, fixée dans `Tolerance` ; un vrai mécanisme donne un
pivot fait de bruit d'arrondi (l'audit relève $-5{,}7 \cdot 10^{-17}$ sur une sonde), très en dessous du seuil.

## 14.3 Désignation des DDL du mécanisme

**Intuition.** Savoir **qu'il y a** un mécanisme ne suffit pas à l'utilisateur ; il faut lui dire **où**. Les modes
de rigidité nulle sont les vecteurs propres de valeur propre nulle ; leurs grandes composantes désignent les DDL
concernés.

**Équations.** Équilibrage par la diagonale, pour que toutes les valeurs propres soient comparables quelles que soient
les unités :

$$
S = \mathrm{diag}\!\left(s_i\right),\quad s_i = \begin{cases} 1/\sqrt{K_{ii}} & K_{ii} > 0 \\ 1 & \text{sinon}\end{cases},
\qquad
(S K_{ff} S)\,\psi_m = \lambda_m\,\psi_m .
$$

$S K_{ff} S$ a une diagonale unité. Un mode $m$ est retenu si $\lambda_m \le 10^{-12}\,\max|\lambda|$ ; dans ce mode,
un DDL $i$ est désigné si $|\psi_{m,i}| \ge 0{,}3\,\max_j |\psi_{m,j}|$.

**Algorithme** (`mechanismDofs`) : si $n_f > 2000$, abandon (décomposition dense trop coûteuse) ; sinon matrice dense
$S K_{ff} S$, `SelfAdjointEigenSolver`, sélection des modes et des composantes, sans doublon. Le message liste au
plus 12 DDL, puis « (+n) ».

**Exemple vérifiable.** Poutre sur deux articulations sans blocage de la rotation axiale : le message désigne Rx
(`Integration_MechanismDetected`) ; console relâchée à l'encastrement : le message désigne le nœud 2
(`Releases_HingeAtSupportCreatesMechanism`).

**Limites.** Au-delà de 2000 DDL libres, le mécanisme est détecté mais non localisé
([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §4). Les constantes 0,3, 12 et 2000 sont des
choix de présentation, non justifiés par une référence.

## 14.4 Quasi-mécanismes

**Intuition.** Entre « bien retenu » et « mécanisme », il existe des modèles presque instables : un DDL retenu par une
rigidité dérisoire devant celle des barres voisines. Le calcul reste possible, mais il perd des chiffres significatifs.

**Équation.** Soit $\rho_\text{min} = \min_k d_k / \tilde K_{kk}$. Si $10^{-12} < \rho_\text{min} < 10^{-8}$
(`Tolerance::weakPivot`), l'analyse ajoute à chaque cas l'avertissement :

> conditionnement faible (quasi-mécanisme possible) : pivot relatif minimal … au DDL … ; les résultats peuvent perdre
> jusqu'à $\lceil -\log_{10} \rho_\text{min} \rceil$ chiffres significatifs.

**Exemple vérifiable** (`Solver_QuasiMechanismWarnsButSolves`). Console de 3 m dont le pied est libre en torsion et
retenu par un ressort de rotation $k = 10^{-10}\,GJ/L$ ; $GJ/L = 76\,069{,}01$ kN·m/rad, donc
$k = 7{,}607 \cdot 10^{-6}$ kN·m/rad. Le bloc de torsion est
$\begin{bmatrix} GJ/L + k & -GJ/L \\ -GJ/L & GJ/L \end{bmatrix}$ ; dans les deux ordres d'élimination possibles, le
second pivot vaut $\frac{GJ}{L}\frac{k}{GJ/L + k} \approx k$ et son rapport à sa diagonale propre
$\rho \approx 9{,}99999 \cdot 10^{-11}$. Comme $-\log_{10}\rho = 10{,}000\,0002$, le message annonce **11** chiffres
perdus au plus. La rotation en bout vaut $T/k + TL/GJ = 2{,}629 \cdot 10^{5}$ rad pour $T = 2$ kN·m : valeur absurde
physiquement, mais c'est la solution exacte de ce modèle, vérifiée à $10^{-5}$ près. Valeurs recalculées en Python.

**Limites.** L'avertissement est **non bloquant** : il est reproduit dans les résultats (`warnings`), dans le JSON et
dans TSA, mais c'est à l'utilisateur de juger. Un quasi-mécanisme dont le rapport dépasse $10^{-8}$ ne produit aucun
avertissement.

## 14.5 Résidu de résolution et contrôle d'équilibre

**Résidu.** Après chaque résolution :

$$
r = \frac{\lVert K_{ff} U_f - (F_f - K_{fr} U_r) \rVert}{\lVert F_f - K_{fr} U_r \rVert}
\quad (r = 0 \text{ si le second membre est nul}).
$$

Si $r$ n'est pas inférieur ou égal à $10^{-8}$ (`Tolerance::solutionResidual`), un avertissement « résidu de
résolution élevé » est ajouté ; la forme `!(r <= seuil)` attrape aussi une valeur non finie. Dans l'exemple du
§14.4, le conditionnement de l'ordre de $10^{10}$ fait dépasser ce seuil : le test attend **deux** avertissements. Sur
un modèle sain, le test exige $r < 10^{-12}$.

**Contrôle d'équilibre global.** Indépendamment du solveur, `checkEquilibrium` somme les charges appliquées (charges
réparties exactes comprises, résultante et premier moment calculés analytiquement), les réactions et les forces des
ressorts :

$$
\Big\lVert \sum F_\text{app} + \sum R + \sum F_\text{ressorts} \Big\rVert,
\qquad
\Big\lVert \sum (M_\text{app} + p \times F_\text{app}) + \sum (M_R + p \times R) + \dots \Big\rVert ,
$$

les moments étant pris par rapport à l'origine globale. Les deux normes sont publiées (`forceResidual`,
`momentResidual`) ; les tests exigent des valeurs inférieures à $10^{-9}$ ou $10^{-8}$ selon le modèle.

**Portée.** Ce contrôle vérifie l'équivalence statique des forces nodales équivalentes et la précision de la
résolution. Il **ne vérifie pas** la rigidité : une matrice $K$ fausse mais symétrique donnerait aussi un équilibre
satisfait ([THEORY.md](../../theory/THEORY.md) §8, [AUDIT.md](../../AUDIT.md) constat 5). La rigidité est validée par les
solutions analytiques et par OpenSees ([chapitre 19](19-validation-reproductibilite.md)).

## À retenir

- Une seule factorisation `SimplicialLDLT` (permutation AMD par défaut) sert à tous les cas de charge ; aucune
  inversion explicite.
- Mécanisme si un pivot est $\le 10^{-12}$ fois la diagonale **de son propre DDL** (ou si la factorisation échoue) :
  critère invariant par changement d'unités.
- DDL du mécanisme : modes propres de valeur nulle de $S K_{ff} S$, pour $n_f \le 2000$.
- Quasi-mécanisme ($10^{-12} < \rho < 10^{-8}$) et résidu $> 10^{-8}$ : avertissements, calcul poursuivi.
- L'équilibre global ne prouve pas que $K$ est juste.

## Dans le code source

- Déclaration : [`LinearSystemSolver`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Solver/LinearSystemSolver.h#L19-L38) ; seuils : [`Tolerance`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Types.h#L43-L55) ; constantes de
  diagnostic : [`kMaxListedDofs` et suivantes](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L15-L15).
- Factorisation et critère : [`LinearSystemSolver::factorize`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L59-L98) ; résolution :
  [`LinearSystemSolver::solve`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L100-L104).
- Équilibrage et modes : [`diagonalScaling`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L21-L30), [`mechanismDofs`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L33-L56).
- Avertissement de quasi-mécanisme : [constructeur de `LinearStaticAnalysis`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L14-L33) ; résidu :
  [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115) ; équilibre : [`checkEquilibrium`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L124-L170) ; structure
  [`EquilibriumCheck`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h#L85-L94).
- Erreur : [`Fem3DError`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Fem3DError.h#L22-L35).

## Vérifier par les tests

| Test | Vérification |
|---|---|
| [`Solver_MechanismCriterionIndependentOfUnits`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L97-L128) | même modèle accepté en kN-m et en N-mm, mêmes résultats ; mécanisme détecté dans les deux systèmes |
| [`Solver_QuasiMechanismWarnsButSolves`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L154-L181) | deux avertissements, DDL Rx désigné, solution exacte à $10^{-5}$ ; modèle sain : $r < 10^{-12}$ |
| [`Integration_MechanismDetected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L137-L167) | mécanisme de torsion désigné, structure sans appui |
| [`Releases_HingeAtSupportCreatesMechanism`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp#L125-L144) | mécanisme par rotule, nœud désigné |
| [`Springs_ZeroStiffnessDoesNotHideMechanism`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L99-L110) | raideur nulle : mécanisme maintenu |
| [`Analytical_SupportSettlement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L190-L209) | $n_f = 0$ accepté |

Lancer : `build\fem3d_tests.exe`. Les cas OpenSees 11 et 17 éprouvent le refus des mécanismes ; les résidus
d'équilibre de Fem3D y sont inférieurs à $10^{-11}$ sur tous les cas comparés
([VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) §4).

## Références

- Documentation d'Eigen, `SimplicialLDLT` (SPD, `AMDOrdering`, `vectorD`, `info`) — vérifié (contenu),
  [REFERENCES.md](../../references/REFERENCES.md) §C8.
- A3 Bathe, *Finite Element Procedures*, §8.1-8.2 — vérifié (notice ; correspondance par MIT OCW), contenu non lu.
- Critère de « rapport de pivot » des codes industriels (MAXRATIO de Nastran) : cité par l'audit indépendant
  ([INDEPENDENT_FORMULATION_AUDIT.md](../../verification/INDEPENDENT_FORMULATION_AUDIT.md) §6) ; ce n'est pas une
  référence de REFERENCES.md : **à vérifier**.

---

Précédent : [Chapitre 13 — Conditions aux limites et stabilité](13-conditions-limites-stabilite.md) · Suivant :
[Chapitre 15 — Reconstruction des efforts internes](15-efforts-internes.md)
