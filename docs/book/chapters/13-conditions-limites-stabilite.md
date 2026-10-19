# Chapitre 13 — Conditions aux limites et stabilité

> **Objectifs**
>
> - Écrire et interpréter le système partitionné $K_{ff} U_f = F_f - K_{fr} U_r$ et la formule des réactions.
> - Savoir comment Fem3D traite un déplacement imposé non nul (tassement d'appui) et un modèle sans DDL libre.
> - Reconnaître un **mécanisme** (stabilité cinématique), ses causes typiques et la façon dont Fem3D le refuse.
>
> **Prérequis**
>
> - Assemblage partitionné ([chapitre 12](12-assemblage.md)).
> - Appuis rigides et élastiques ([chapitres 10](10-charges-conditions-limites.md) et [11](11-ressorts.md)).
> - Algèbre linéaire : noyau d'une matrice, matrice définie positive.

Dans ce chapitre, **stabilité** veut dire stabilité **cinématique** : la structure est-elle retenue contre tout
mouvement d'ensemble ou de partie ? Il ne s'agit **pas** de flambement, que Fem3D ne calcule pas
([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §1). Le diagramme
[stabilite.md](../../diagrams/stabilite.md) résume la chaîne de décision.

## 13.1 Partition des DDL libres et imposés

**Intuition.** Aux DDL bloqués, le déplacement est connu (zéro, ou une valeur imposée) et la force est inconnue (la
réaction). Aux DDL libres, c'est l'inverse. On sépare donc les équations en deux groupes : on résout le premier, puis
on lit les réactions dans le second.

**Symboles.** $U_f$ (inconnu) et $U_r$ (imposé) ; $F_f$, $F_r$ : charges assemblées aux DDL libres et imposés ;
$R$ : réactions des appuis rigides, forces exercées **par l'appui sur la structure**, repère global.

**Hypothèses.** Appuis rigides selon les axes globaux (`NodeRestraint` : six booléens et six valeurs imposées) ; aucune
contrainte cinématique entre DDL ; statique linéaire.

**Équations** (lignes du système partitionné du [chapitre 12](12-assemblage.md)) :

$$
K_{ff}\,U_f = F_f - K_{fr}\,U_r ,
\qquad
R = K_{rf}\,U_f + K_{rr}\,U_r - F_r .
$$

**Interprétation.**

- Le second membre $F_f - K_{fr} U_r$ montre qu'un déplacement imposé agit comme une charge sur les DDL libres.
- $-F_r$ dans $R$ : une charge appliquée directement sur un DDL bloqué (ou la part des forces nodales équivalentes
  qui y tombe) est reprise par l'appui sans déformer la structure.
- Il n'y a **ni pénalité ni grand nombre** sur la diagonale : les DDL imposés sont éliminés exactement, ce qui ne
  dégrade pas le conditionnement de $K_{ff}$.
- Les réactions sont publiées pour chaque nœud qui a un enregistrement d'appui ; elles valent 0 sur ses DDL libres.
  Les forces des ressorts ($-k\,u$) sont publiées à part ([chapitre 11](11-ressorts.md)).

**Algorithme** (`LinearStaticAnalysis::run`) : assembler $F$ ; extraire $F_f$, $F_r$ et $U_r$ (valeurs imposées) ;
former le second membre ; résoudre avec la factorisation déjà calculée ([chapitre 14](14-solveur-diagnostics.md)) ;
calculer $R$ ; reconstituer le vecteur complet $U$ pour les résultats nodaux et les éléments.

**Exemple vérifiable.** Console axiale du [chapitre 12](12-assemblage.md) §12.2 : $R_1 = K_{rf} U_f = -k\,u_2 = -100$ kN,
opposée à la charge de 100 kN ; test `Analytical_AxialTension` (réaction $-P$).

**Limites.** Appuis selon les axes globaux uniquement (pas d'appui incliné) ; pas d'appui unilatéral ; un ressort sur
un DDL bloqué est refusé à la validation, car la répartition de la réaction serait ambiguë.

## 13.2 Déplacements imposés non nuls

**Intuition.** Un tassement d'appui est un déplacement connu, non nul. Il ne coûte rien de plus : c'est une valeur de
$U_r$ différente de zéro.

**Mise en œuvre.** `NodeRestraint::prescribed[k]` donne la valeur imposée du DDL $k$ **s'il est bloqué**
(`fixed[k] = true`) ; pour un DDL libre, cette valeur est ignorée par `DofMap` (elle doit seulement être finie :
`StructuralModel3D::validate` refuse toute valeur non finie). Unité : celle d'une translation ou d'un radian.

**Exemple vérifiable.** Poutre bi-encastrée de 5 m (section 0,30 × 0,50 m, $E I_y = 656\,250$ kN·m²) dont l'appui j
tasse de $\delta = 0{,}01$ m selon z :

$$
|M| = \frac{6 E I_y \delta}{L^2} = 1575\ \text{kN·m},
\qquad
|V| = \frac{12 E I_y \delta}{L^3} = 630\ \text{kN}.
$$

Valeurs recalculées en Python ; test `Analytical_SupportSettlement` (le déplacement imposé est aussi relu dans les
résultats à $10^{-15}$ près).

**Cas particulier : aucun DDL libre.** Dans cet exemple, les deux nœuds sont encastrés : $n_f = 0$. Le modèle est
**valable** : $U = U_r$, et les réactions se réduisent à $R = K_{rr} U_r - F_r$. Le solveur détecte la matrice vide
et ne factorise rien (`m_empty`). Une première version refusait à tort ce modèle ; le défaut, relevé pendant le
développement, est corrigé et testé ([AUDIT.md](../../AUDIT.md), constat 10). Le code d'erreur `NoFreeDof` est
encore déclaré dans `ErrorCode`, mais n'est plus levé nulle part.

**Limites.** Validation analytique seule : OpenSees n'a pas servi de référence pour les déplacements imposés
([VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) §3, verdict « validé avec réserves »).

## 13.3 Mécanismes : définition et causes

**Intuition.** Une structure est un mécanisme si une partie peut bouger sans déformer aucune barre : elle n'oppose
alors aucune rigidité à ce mouvement, et $K_{ff}$ a un noyau non nul. Le système n'a pas de solution unique.

**Équation.** Il existe $\phi \neq 0$ tel que

$$
K_{ff}\,\phi = 0 \quad\Longleftrightarrow\quad \tfrac{1}{2}\,\phi^T K_{ff}\,\phi = 0 ,
$$

c'est-à-dire un mode de déplacement d'énergie de déformation nulle. Pour une structure correctement retenue,
$K_{ff}$ est symétrique **définie positive**.

**Chaque élément a exactement six modes de rigidité nulle** (ses mouvements de corps rigide) : c'est vérifié par
`Element_SymmetryAndRigidBodyModes` (6 valeurs propres nulles, aucune négative, pour les deux formulations). Les
appuis et les liaisons entre éléments doivent bloquer tous ces modes.

**Causes typiques, chacune couverte par un test ou un benchmark.**

| Cause | Exemple | Test ou cas |
|---|---|---|
| Structure sans appui | barre flottante | `Integration_MechanismDetected` (2ᵉ partie) |
| Rotation autour de l'axe de la barre non retenue | poutre sur deux articulations sans blocage de Rx | `Integration_MechanismDetected`, cas OpenSees 11 |
| Rotule au droit de l'encastrement d'une console | rotule My en i, aucun autre maintien | `Releases_HingeAtSupportCreatesMechanism`, cas 17 |
| DDL retenu seulement par un ressort de raideur nulle | pied articulé, « ressorts » nuls | `Springs_ZeroStiffnessDoesNotHideMechanism` |
| Articulation en pied sans ressort | pied articulé dans les trois rotations | `Solver_MechanismCriterionIndependentOfUnits` (variante mécanisme) |

**Interprétation.** Une poutre relâchée en My et Mz aux deux bouts garde sa rigidité de torsion : ce n'est pas un
treillis ([FORMULATIONS.md](../../formulations/FORMULATIONS.md) §7). Inversement, une rotule au droit d'un encastrement
ne crée un mécanisme que si la barre n'a **aucun autre** maintien en rotation : dans un portique, elle transforme
seulement l'appui en articulation (correction D2 de l'audit indépendant).

**Ce que fait Fem3D.** Le mécanisme est détecté **à la factorisation**, dans le constructeur de
`LinearStaticAnalysis` : `Fem3DError` de code `Mechanism`, avec un message qui désigne les DDL du mode
(« … DDL des modes de mécanisme : nœud 2, Uz, nœud 2, Ry »). Le calcul n'est jamais poursuivi, et aucun appui n'est
ajouté automatiquement : un appui n'est jamais transformé en encastrement (commentaire de
`BoundaryConditions.h`). Le critère et la localisation sont détaillés au [chapitre 14](14-solveur-diagnostics.md).

**Exemple vérifiable.** Cas OpenSees 17 (console relâchée en My au droit de l'encastrement) : Fem3D refuse le calcul
et désigne le nœud 2, Uz et Ry ; OpenSees 3.8.0 ne signale pas d'échec (`analyze` = 0) mais rend une solution non
bornée (Uz ≈ 6·10¹¹ m), que le générateur de benchmarks reconnaît par le critère $|u| > 10^6$
([VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) §5). Le verdict « OK » de ce cas signifie que les
deux codes **refusent** le modèle.

**Limites.**

- Un **quasi-mécanisme** (rigidité très faible mais non nulle) n'est pas refusé : il produit un avertissement
  ([chapitre 14](14-solveur-diagnostics.md) §14.4).
- Les DDL du mode ne sont désignés que pour $n_f \le 2000$ ; au-delà, le mécanisme est détecté sans être localisé.
- Dans TSA, un mécanisme n'est jamais confié à un autre moteur ([chapitre 18](18-integration-tsa-opensees.md)).

## À retenir

- $K_{ff} U_f = F_f - K_{fr} U_r$, puis $R = K_{rf} U_f + K_{rr} U_r - F_r$ : élimination exacte des DDL imposés,
  sans pénalité.
- Un tassement est une valeur non nulle de $U_r$ ; un modèle sans DDL libre est valable.
- Un mécanisme est un mode d'énergie nulle de $K_{ff}$ ; Fem3D le refuse avec le code `Mechanism` et désigne ses DDL.
- « Stabilité » signifie ici stabilité cinématique, pas flambement.

## Dans le code source

- Conditions d'appui : [`NodeRestraint`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Constraints/BoundaryConditions.h#L13-L22), [`NodalSpring`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Constraints/BoundaryConditions.h#L27-L31).
- Partition : [`DofMap::DofMap`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L9-L30).
- Résolution et réactions : [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115).
- Contrôle des données (valeurs imposées finies, ressort sur DDL bloqué) : [`StructuralModel3D::validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L113-L158).
- Détection à la factorisation : [`LinearSystemSolver::factorize`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L59-L98), modèle sans DDL libre :
  [`LinearSystemSolver::solve`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L100-L104).
- Codes d'erreur : [`ErrorCode`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Fem3DError.h#L11-L20).

## Vérifier par les tests

| Test ou cas | Vérification |
|---|---|
| [`Analytical_SupportSettlement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L190-L209) | tassement : $6EI\delta/L^2$, $12EI\delta/L^3$, aucun DDL libre |
| [`Analytical_AxialTension`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L52-L63) | réaction $-P$ |
| [`Element_SymmetryAndRigidBodyModes`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L65-L82) | 6 modes de corps rigide par élément |
| [`Integration_MechanismDetected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L137-L167) | mécanisme de torsion désigné (Rx) ; structure sans appui |
| [`Releases_HingeAtSupportCreatesMechanism`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp#L125-L144) | rotule à l'encastrement : nœud 2 désigné |
| [`Springs_ZeroStiffnessDoesNotHideMechanism`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L99-L110) | ressort nul : mécanisme maintenu |
| [`Springs_InvalidInputRejected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L112-L126) | ressort sur DDL bloqué refusé |
| Cas OpenSees 11 et 17 ([`cases()`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L114-L296)) | refus par les deux codes |

Lancer : `build\fem3d_tests.exe` ; `build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier]`.

## Références

- A3 Bathe, *Finite Element Procedures* — vérifié (notice) ; §8.1-8.2 (élimination de Gauss, LDLᵀ) : correspondance
  vérifiée par MIT OCW ; contenu non lu.
- A7 McGuire, Gallagher, Ziemian — vérifié (notice) ; traitement des appuis **à vérifier**.
- A8 Weaver, Gere, *Matrix Analysis of Framed Structures* — vérifié (notice) ; chapitres **à vérifier**.

---

Précédent : [Chapitre 12 — Assemblage des matrices globales](12-assemblage.md) · Suivant :
[Chapitre 14 — Solveur numérique et diagnostics](14-solveur-diagnostics.md)
