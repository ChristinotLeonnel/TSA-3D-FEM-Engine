# Chapitre 19 — Validation scientifique et reproductibilité

> **Objectifs**
>
> - Distinguer vérification (le code résout-il bien les équations ?) et validation (les équations et leur mise en
>   œuvre donnent-elles les bonnes réponses face à une référence indépendante ?).
> - Comprendre pourquoi les tolérances sont fixées **avant** les essais, et comment elles sont choisies.
> - Savoir reproduire toute la validation et interpréter un écart.
>
> **Prérequis**
>
> - Chaîne de calcul complète ([chapitres 10](10-charges-conditions-limites.md) à [16](16-deplacements-contraintes.md)).
> - Intégration dans TSA ([chapitre 18](18-integration-tsa-opensees.md)).

Documents de référence : [SCIENTIFIC_VALIDATION_GUIDE.md](../../validation/SCIENTIFIC_VALIDATION_GUIDE.md) (méthode),
[VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) (verdicts),
[VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) (benchmarks),
[MATURITY_LEVELS.md](../../validation/MATURITY_LEVELS.md) (niveaux). Ce chapitre explique la logique d'ensemble.

## 19.1 Trois sources de preuve, de plus en plus indépendantes

| Source | Ce qu'elle prouve | Outil | Tolérance |
|---|---|---|---|
| Solutions analytiques (RDM) | les éléments et la chaîne de calcul reproduisent des solutions **exactes** | `fem3d_tests` (41 tests) | écart relatif $\le 10^{-9}$ pour les solutions exactes ; tolérances plus larges pour quelques vérifications (séries de torsion $6 \cdot 10^{-3}$, quasi-mécanisme, invariances ; [chapitre 20](20-tests-limites.md) §20.1) |
| OpenSees 3.8.0 | un code indépendant, d'une autre équipe, donne les mêmes résultats sur des modèles 3D quelconques | `fem3d_opensees_benchmarks` (20 cas × 2 formulations) | $\max(10^{-6}\lvert\text{réf}\rvert,\ 10^{-9}\,\text{échelle})$ |
| Mode comparaison de TSA | l'intégration ne déforme rien sur les modèles réels de TSA | tests 476 à 480 ; à étendre pour le niveau 5 | mêmes critères que les benchmarks |

**Pourquoi trois ?** Une solution analytique n'existe que pour des cas simples (une console, une poutre bi-encastrée).
OpenSees couvre des portiques spatiaux quelconques, mais il pourrait partager une erreur de convention avec Fem3D si
les données lui étaient mal transmises. Le mode comparaison de TSA, enfin, éprouve la conversion du modèle réel. Aucune
source ne suffit seule.

## 19.2 Pourquoi $10^{-9}$ contre l'analytique

Les deux éléments sont **exacts** pour une barre prismatique : rigidité exacte, forces équivalentes exactes, efforts
et déformée reconstruits par intégration exacte de polynômes ([chapitres 6](06-euler-bernoulli.md),
[7](07-timoshenko.md), [10](10-charges-conditions-limites.md), [15](15-efforts-internes.md)). Il ne reste que
l'arrondi en double précision, de l'ordre de $10^{-15}$ à $10^{-12}$ selon le conditionnement. Une tolérance de
$10^{-9}$ laisse une marge à l'arrondi et ne laisse **aucune** place à une erreur de formulation.

La macro `CHECK_NEAR(a, b, rel, msg)` accepte $|a - b| \le \max(10^{-12},\ \text{rel}\,|b|)$ : relative, avec un
plancher absolu près de zéro. En cas d'échec, elle affiche le fichier, la ligne, le message, la valeur obtenue et la
valeur attendue.

**Conséquence pédagogique.** Avec Fem3D, il est inutile de « raffiner le maillage » d'une barre prismatique : un seul
élément donne la solution exacte. Le test `Integration_SpatialFrame_EquilibriumAndMeshInvariance` le vérifie : couper
chaque barre d'un portique spatial en 4 éléments ne change pas les déplacements des nœuds communs.

## 19.3 Pourquoi $10^{-6}$ contre OpenSees

Les formulations comparées sont identiques (`elasticBeamColumn` pour Euler–Bernoulli, `ElasticTimoshenkoBeam` pour
Timoshenko), mais les deux codes factorisent différemment (OpenSees : `BandGeneral` avec numérotation RCM ; Fem3D :
LDLᵀ creuse) et échangent des valeurs écrites en texte à 17 chiffres. La tolérance relative $10^{-6}$ est large devant
l'arrondi, et pourtant **$10^4$ à $10^6$ fois plus petite** que l'effet d'une erreur de formulation : un signe, un
$\Phi$ ou un axe faux produit un écart de 1 à 100 %.

**Près de zéro.** Une composante théoriquement nulle a un écart relatif instable. On lui applique un critère absolu,
$10^{-9}$ fois l'échelle de la grandeur dans le cas (plus grande translation, rotation, force ou moment). Une grandeur
toute nulle (les moments d'une barre bi-articulée) aurait une échelle nulle : les échelles ont donc des planchers
dimensionnellement cohérents (moments $\ge$ forces × plus grande longueur ; rotations $\ge$ translations / plus grande
longueur).

**Fixées avant les essais.** Une tolérance choisie après avoir vu l'écart ne prouve rien : on finirait toujours par
« valider ». Les tolérances sont écrites dans le code du générateur et dans
[VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) §3 ; les modifier exige une justification
scientifique écrite (règle 6 de `AGENTS.md`).

**Résultat mesuré.** 2 657 valeurs comparées, 0 hors tolérance ; les écarts relatifs maximaux observés sont de
l'ordre de $10^{-16}$ à $10^{-12}$ selon les cas ([VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md)
§4).

## 19.4 Invariances : des tests sans référence extérieure

Certaines propriétés doivent être vraies quelle que soit la solution. Elles détectent des erreurs que des cas
analytiques simples, souvent alignés sur les axes, laisseraient passer.

| Invariance | Test |
|---|---|
| Rotation : une console inclinée quelconque fléchit comme la console horizontale, sans déplacement parasite | `Integration_InclinedCantilever_OrientationInvariance` |
| Maillage : subdiviser les barres ne change pas les nœuds communs | `Integration_SpatialFrame_EquilibriumAndMeshInvariance` |
| Repère des charges : charge globale = sa projection locale | `Integration_GlobalAndLocalDistributedLoadsEquivalent` |
| Unités : le critère de mécanisme donne la même décision en kN-m et en N-mm | `Solver_MechanismCriterionIndependentOfUnits` |
| Limite : Timoshenko tend vers Euler–Bernoulli quand $\Phi \to 0$ | `Element_TimoshenkoTendsToEulerBernoulli` |
| Corps rigide : 6 valeurs propres nulles par élément, aucune négative | `Element_SymmetryAndRigidBodyModes` |
| Cohérence : la déformée d'élément redonne les déplacements nodaux | `Integration_ResponseConsistentWithNodalDisplacements` |

## 19.5 Reproduire

Tout se relance par des commandes uniques ([VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) §4) :

```bat
build\fem3d_tests.exe
build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier]
```

Le générateur de benchmarks :

1. construit chaque cas avec l'API de Fem3D ;
2. écrit le script Tcl équivalent (`benchmarks/OpenSees/cases/*.tcl`), **sans chemin local absolu** ;
3. lance OpenSees et lit ses sorties brutes (`results/*_opensees.txt`) ;
4. compare valeur par valeur et écrit `results/*_comparaison.csv` (grandeur, identifiant, composante, Fem3D,
   OpenSees, écarts, statut).

Les scripts, les sorties et les CSV sont **versionnés** : on peut relire chaque comparaison sans relancer OpenSees.
Les références sont toujours régénérées par OpenSees, jamais retouchées à la main (règle 7 de `AGENTS.md`). Version
de référence : OpenSees 3.8.0 (`TSA/thirdparty/OpenSees`).

**Exécution constatée.** Le 19/10/2026, `build\fem3d_tests.exe` (exécutable compilé le même jour depuis des sources
identiques à la révision `75700d2`) affiche « RESULTATS : 41 / 41 tests reussis ».

## 19.6 Interpréter un écart

Démarche ([SCIENTIFIC_VALIDATION_GUIDE.md](../../validation/SCIENTIFIC_VALIDATION_GUIDE.md) §4) :

1. **Lire la ligne du CSV** : quelle grandeur, quel nœud ou élément, quelle composante, quel ordre de grandeur ?
2. **Vérifier la transmission** avant le calcul : unités, $v_{xz}$, aires de cisaillement, appuis, charges dans le
   script Tcl.
3. **Chercher la convention** : un écart de signe pur ou un facteur constant trahit une convention différente plutôt
   qu'une erreur de rigidité.
4. **Isoler** : réduire le modèle jusqu'au plus petit cas qui reproduit l'écart, puis le confronter à une solution
   analytique.
5. **Ne jamais** ajuster la tolérance ni la référence pour faire disparaître l'écart.

**Exemples réels** ([VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md) §5) :

- Neuf réactions semblaient en écart dans les cas 12 et 13. Cause : avec `equalDOF`, la réaction d'OpenSees se
  répartit entre le nœud maître et le nœud esclave dédoublé ; la réaction réelle est leur somme. L'extraction a été
  corrigée, aucune référence modifiée.
- Les charges trapézoïdales donnaient les mêmes efforts qu'une charge uniforme dans OpenSees 3.8.0 : `-beamUniform`
  accepte la syntaxe à 8 arguments en 3D mais ignore la variation, sans message. Le cas est déclaré **non
  comparable** ; ces charges sont validées analytiquement et, indirectement, dans TSA (test 477).
- Cas 17 (mécanisme) : OpenSees ne signale pas d'échec mais rend $U_z \approx 6 \cdot 10^{11}$ m ; le générateur
  reconnaît une solution non bornée ($|u| > 10^6$).

## 19.7 Ce que la validation ne prouve pas

- Elle ne vaut que pour la **statique linéaire de poutres prismatiques élastiques** ; rien n'est dit du flambement,
  du second ordre, de la dynamique ni du gauchissement, qui ne sont pas implémentés.
- L'accord avec OpenSees prouve l'accord de **deux mises en œuvre de la même théorie** ; il ne prouve pas que la
  théorie des poutres convient à la structure étudiée (voiles, nœuds rigides épais, sections minces ouvertes).
- Le contrôle d'équilibre global ne vérifie pas la rigidité ([chapitre 10](10-charges-conditions-limites.md) §10.7).
- Les contraintes sont des formules de RDM, sans vérification réglementaire.
- Aucun outil, ni aucune IA, ne valide seul un calcul de structure : la relecture humaine de la théorie est une
  condition du niveau 5.

## 19.8 Verdicts et niveaux

Le [plan de validation](../../validation/VALIDATION_PLAN.md) §3 donne un verdict par fonction (**validé**, **validé
avec réserves**, **non testé**) ; [QUALIFICATION.md](../../validation/QUALIFICATION.md) en déduit le niveau de chaque
fonction et le périmètre autorisé dans TSA. Au 19/10/2026 : niveau 4 pour Euler–Bernoulli, rotules, ressorts,
charges uniformes et détection des mécanismes ; niveau 2 pour les charges linéaires (plus une comparaison indirecte),
les déplacements imposés et les contraintes ; **niveau 5 non atteint**.

## À retenir

- Analytique ($10^{-9}$), OpenSees ($10^{-6}$ et plancher absolu), mode comparaison de TSA : trois preuves
  complémentaires.
- Tolérances fixées avant les essais, justifiées par l'effet d'une vraie erreur ; références jamais retouchées.
- Les invariances détectent des erreurs que des cas alignés sur les axes laisseraient passer.
- Tout se reproduit par deux commandes, et chaque comparaison est relisible dans un CSV versionné.

## Dans le code source

- Tolérances des benchmarks : [`opensees_benchmarks.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L48-L49) ; cas : [`cases()`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L114-L296).
- Comparaison analytique : [`CHECK_NEAR`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/TestFramework.h#L65-L76).

## Vérifier par les tests

| Test | Vérification |
|---|---|
| [`Integration_InclinedCantilever_OrientationInvariance`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L54-L75) | invariance par rotation |
| [`Integration_SpatialFrame_EquilibriumAndMeshInvariance`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L77-L92) | équilibre ; invariance au maillage |
| [`Integration_ResponseConsistentWithNodalDisplacements`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L111-L135) | déformée cohérente avec les nœuds |
| [`Solver_MechanismCriterionIndependentOfUnits`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L97-L128) | même décision en kN-m et en N-mm |
| [`Element_TimoshenkoTendsToEulerBernoulli`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L84-L93) | limite $\Phi \to 0$ |
| [`Analytical_TimoshenkoDeepBeam_NoShearLocking`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L164-L188) | pas de verrouillage en cisaillement |
| 20 cas OpenSees × 2 formulations | 2 657 valeurs, 0 hors tolérance |

## Références

- La distinction vérification / validation suit la méthode de
  [SCIENTIFIC_VALIDATION_GUIDE.md](../../validation/SCIENTIFIC_VALIDATION_GUIDE.md) §1 ; aucune source normative
  n'est encore inventoriée pour elle dans [REFERENCES.md](../../references/REFERENCES.md).
- Documentation d'OpenSees (`elasticBeamColumn`, `ElasticTimoshenkoBeam`, `eleLoad`, `equalDOF`) — vérifié
  (contenu), [REFERENCES.md](../../references/REFERENCES.md) §C1, §C2, §C4, §C9.

---

Précédent : [Chapitre 18 — Intégration avec TSA et OpenSees](18-integration-tsa-opensees.md) · Suivant :
[Chapitre 20 — Tests, limites et erreurs connues](20-tests-limites.md)
