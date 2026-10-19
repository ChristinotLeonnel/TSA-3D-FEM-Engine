# Chapitre 17 — Architecture logicielle C++

> **Objectifs**
>
> - Situer chaque notion des chapitres précédents dans un module, une classe et un fichier.
> - Comprendre les choix de conception : interface d'élément minimale, modèle sans équations, polynômes exacts,
>   erreurs à code stable.
> - Savoir construire, lancer et lire les programmes du dépôt (moteur, tests, benchmarks, post-traitement).
>
> **Prérequis**
>
> - C++ moderne : classes abstraites, `std::unique_ptr`, exceptions.
> - Chaîne de calcul de bout en bout ([chapitres 12](12-assemblage.md) à [16](16-deplacements-contraintes.md)).

Le document de référence est [ARCHITECTURE.md](../../ARCHITECTURE.md) ; les diagrammes sont dans
[diagrams/architecture.md](../../diagrams/architecture.md) et le graphe des inclusions, produit par un script, dans
[diagrams/dependances-code.md](../../diagrams/dependances-code.md). Ce chapitre explique **pourquoi** le code est
organisé ainsi.

## 17.1 Une bibliothèque autonome

**Principe.** Le moteur est une bibliothèque statique C++20, `fem3d`, qui ne dépend que d'**Eigen 3.4** : ni Qt, ni
OCCT, ni rien de TSA. Elle peut donc être compilée, testée et publiée seule, et TSA l'utilise par un adaptateur qui
lui appartient ([chapitre 18](18-integration-tsa-opensees.md)). La dépendance va dans un seul sens.

**Cibles CMake** ([`CMakeLists.txt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/CMakeLists.txt)) :

| Cible | Rôle | Compilée dans TSA |
|---|---|---|
| `fem3d` | bibliothèque du moteur (11 fichiers source) | oui |
| `fem3d_tests` | 41 tests unitaires, analytiques et d'intégration | non |
| `fem3d_opensees_benchmarks` | génération des scripts Tcl, exécution d'OpenSees, comparaison | non |
| `fem3d_example` | portique spatial calculé et exporté en JSON | non |

L'option `FEM3D_BUILD_TESTS` vaut `ON` quand le dépôt est compilé seul et `OFF` quand il est inclus par
`add_subdirectory` : TSA ne compile que la bibliothèque. Les avertissements sont stricts (`/W4 /permissive-` sous
MSVC, `-Wall -Wextra -Wpedantic` ailleurs).

**Construire et tester** ([DEVELOPER_GUIDE.md](../../developer/DEVELOPER_GUIDE.md) §1) :

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\fem3d_tests.exe
build\fem3d_opensees_benchmarks.exe
build\fem3d_example.exe sortie.json
```

## 17.2 Les modules et leurs responsabilités

L'espace de noms est `TSA::Fem3D`. Chaque dossier de `include/Fem3D/` a son pendant dans `src/` :

| Module | Contenu | Chapitres |
|---|---|---|
| `Core` | ordre des DDL, `Vector12`, `Matrix12`, tolérances, `Fem3DError`, `Polynomial` | [5](05-unites-signes.md), [14](14-solveur-diagnostics.md) |
| `Geometry` | `Node3D`, `LocalCoordinateSystem3D` ($R$, $T$) | [4](04-reperes.md) |
| `Materials`, `Sections` | `ElasticMaterial`, `BeamSection3D` | [9](09-materiaux-sections.md) |
| `Elements` | interface, base commune, deux formulations, rotules et condensation | [6](06-euler-bernoulli.md), [7](07-timoshenko.md), [15](15-efforts-internes.md) |
| `Loads`, `Constraints` | charges, cas, appuis, ressorts | [10](10-charges-conditions-limites.md), [11](11-ressorts.md) |
| `Model` | `StructuralModel3D` : données, contrôle, fabrique d'éléments | [10](10-charges-conditions-limites.md) |
| `Assembly` | `DofMap`, `GlobalAssembler` | [12](12-assemblage.md) |
| `Solver` | `LinearSystemSolver` (LDLᵀ creuse, mécanismes) | [13](13-conditions-limites-stabilite.md), [14](14-solveur-diagnostics.md) |
| `Results` | `BeamResponse`, `AnalysisResults`, `ResultsJsonWriter` | [15](15-efforts-internes.md), [16](16-deplacements-contraintes.md) |
| `Analysis` | `LinearStaticAnalysis` : orchestration, équilibre | [13](13-conditions-limites-stabilite.md) |

**Sans cycle.** `Core ← Geometry ← Elements ← Model ← Assembly ← Solver ← Analysis` ; `Results` dépend des éléments
et des charges. Un module ne connaît jamais un module situé après lui dans cette chaîne.

## 17.3 L'interface d'élément : trois choses seulement

**Choix de conception.** `IBeamElement3D` demande à une formulation exactement ce qui la distingue des autres :

1. sa **matrice de rigidité locale** 12 × 12 (`localStiffnessMatrix`) ;
2. ses **fonctions de forme** exactes (`shapeFunctions`), qui servent aux forces nodales équivalentes ;
3. ses **flexibilités de cisaillement** $1/(G A_{sy})$ et $1/(G A_{sz})$, nulles en Euler–Bernoulli
   (`shearFlexibilityY`, `shearFlexibilityZ`), qui servent à la déformée.

Tout le reste est **commun** et écrit une seule fois : repère et transformation $K_g = T^T K T$
(`globalStiffnessMatrix`), condensation des rotules (`effectiveLocalStiffnessMatrix`), assemblage, charges,
efforts internes, déformée, contraintes. `BeamElementBase` porte les données (identifiants, repère, matériau,
section, rotules) et les termes communs de traction et de torsion de Saint-Venant (`addAxialAndTorsion`).

**Conséquence.** Ajouter une formulation, c'est écrire une classe dérivée de `BeamElementBase` avec ces trois
méthodes ; aucune autre classe ne change, sauf la fabrique (§17.4). La procédure, avec ses tests obligatoires, est
dans [DEVELOPER_GUIDE.md](../../developer/DEVELOPER_GUIDE.md) §4 et au [niveau 6](../../learning/level-06/index.md)
du parcours.

**Pourquoi ne pas mettre les efforts internes dans l'élément ?** Parce qu'ils se déduisent de l'équilibre d'un
tronçon et de trois quantités de l'interface ([chapitre 15](15-efforts-internes.md)). Les écrire une fois dans
`BeamResponse` garantit que les deux formulations publient leurs résultats avec **les mêmes conventions**.

## 17.4 Le modèle ne contient pas d'équations

`StructuralModel3D` stocke des **données saisies** : nœuds, `ElementDefinition` (nœuds, matériau, section,
formulation, $v_{xz}$, rotules), matériaux et sections nommés, appuis, ressorts, cas de charge. Il ne calcule rien.

Deux méthodes font le lien avec le calcul :

- `createElement` : fabrique l'élément fini de la formulation demandée **par barre** (une structure peut mêler
  Euler–Bernoulli et Timoshenko) et préfixe toute erreur par « élément n : » ;
- `validate` : contrôle complet avant calcul ([chapitre 10](10-charges-conditions-limites.md) §10.6).

## 17.5 Orchestration et propriété des objets

`LinearStaticAnalysis` est le seul point d'entrée du calcul :

1. **constructeur** : `validate`, création des éléments (possédés par `std::unique_ptr`), `DofMap`, assemblage de
   $K$, factorisation de $K_{ff}$ et diagnostic de conditionnement ;
2. **`run(id)`** : vecteur de charges du cas, résolution, réactions, forces des ressorts, `BeamResponse` de chaque
   élément, contrôle d'équilibre, avertissements ;
3. **`runAll()`** : tous les cas, avec la même factorisation.

L'objet garde une **référence** vers le modèle : le modèle doit vivre plus longtemps que l'analyse. `BeamResponse`
garde une référence vers un élément possédé par l'analyse. Il n'y a aucune variable globale ; un même modèle peut
être calculé par plusieurs analyses indépendantes.

## 17.6 Erreurs : un code stable et un message qui désigne l'objet

Toute donnée invalide ou situation numérique lève `Fem3DError`, qui dérive de `std::runtime_error` :

| Code | Exemple |
|---|---|
| `InvalidInput` | module d'Young nul, valeur non finie, ressort négatif |
| `UnknownReference` | nœud, élément, matériau, section ou cas inexistant |
| `DuplicateId` | deux nœuds de même identifiant |
| `DegenerateGeometry` | barre de longueur nulle, $v_{xz}$ parallèle à la barre |
| `Mechanism` | $K_{ff}$ singulière ; le message désigne les DDL du mode |
| `NoFreeDof` | déclaré, mais plus levé ([chapitre 13](13-conditions-limites-stabilite.md) §13.2) |
| `IoError` | écriture d'un fichier impossible |

Le **code** sert aux programmes (les tests vérifient le code, pas le texte ; TSA refuse tout secours sur
`Mechanism`) ; le **message**, en français, sert à l'utilisateur. Les situations qui n'empêchent pas le calcul
(quasi-mécanisme, résidu élevé) ne lèvent pas d'exception : elles sont publiées dans `LoadCaseResults::warnings`.

## 17.7 Tolérances regroupées

Les seuils numériques sont dans une seule structure, `Tolerance` (`Core/Types.h`), sans unité implicite :

| Constante | Valeur | Usage |
|---|---|---|
| `relativeMinimumLength` | $10^{-9}$ | longueur minimale d'une barre, relative à la plus grande coordonnée de ses extrémités (au moins 1) |
| `parallelCosine` | $1 - 10^{-9}$ | $v_{xz}$ jugé parallèle à la barre |
| `relativePivot` | $10^{-12}$ | pivot nul : mécanisme |
| `weakPivot` | $10^{-8}$ | pivot faible : avertissement de quasi-mécanisme |
| `solutionResidual` | $10^{-8}$ | résidu relatif de résolution : avertissement |

Les modifier exige une justification scientifique écrite (règle 6 de `AGENTS.md`, à la racine du dépôt ;
[VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) §2).

## 17.8 Résultats exacts et format d'échange

**Polynômes plutôt qu'échantillons.** `BeamResponse` représente efforts, rotations et déplacements par des objets
`Polynomial` (coefficients, évaluation, intégration exacte). Les **stations** (11 par défaut,
`AnalysisOptions::stationsPerElement`) ne sont qu'un choix d'affichage : aucune valeur n'est interpolée entre elles.

**Format d'échange.** `ResultsJsonWriter` écrit le format versionné `tsa-fem3d-results/1`
([RESULTS_FORMAT.md](../../RESULTS_FORMAT.md)). Le fichier porte ses propres conventions de signe (repère local,
efforts d'extrémité, réactions, ressorts), ce qui le rend lisible sans le code.

## 17.9 Le post-traitement, un sous-projet séparé

Le dossier `postprocessor/` est un projet CMake distinct, en Qt 6, qui **ne lie pas** la bibliothèque `fem3d` : il ne
connaît que le fichier JSON. On peut donc le faire évoluer, ou le remplacer, sans toucher au moteur.

| Cible | Rôle |
|---|---|
| `fem3dpost` | bibliothèque (QtCore seul) : lecture du JSON, calculs, tableaux, CSV, diagrammes SVG, note de calcul |
| `fem3d_report` | outil en ligne de commande ; QtGui seulement pour l'export PDF |
| `fem3dpost_tests` | 15 tests, dont la relecture du fichier réel produit par `fem3d_example` |

Voir [postprocessor/README.md](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/postprocessor/README.md).

## Limites de l'architecture

- Un seul type d'analyse (statique linéaire) : `LinearStaticAnalysis` n'est pas une classe de base d'analyses.
  Ajouter le flambement ou la modale demandera une matrice géométrique ou de masse dans l'interface d'élément
  ([chapitre 21](21-extensions.md)).
- Numérotation dense à 6 DDL par nœud, sans DDL supplémentaire ni contrainte cinématique
  ([chapitre 12](12-assemblage.md)).
- Pas de lecteur de modèle : un modèle se construit par l'API C++ (TSA, tests, exemple). Le JSON est un format de
  **résultats**, qui recopie aussi les données du modèle.

## À retenir

- `fem3d` : C++20 + Eigen, sans Qt ; TSA l'inclut par un adaptateur, sans dépendance inverse.
- Une formulation = rigidité locale + fonctions de forme + flexibilités de cisaillement ; tout le reste est commun.
- Le modèle stocke, l'analyse calcule, `BeamResponse` reconstruit ; les erreurs ont un code stable.
- Le post-traitement ne connaît que le JSON versionné.

## Dans le code source

- Interface : [`IBeamElement3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/IBeamElement3D.h#L35-L64), base : [`BeamElementBase`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/BeamElementBase.h#L11-L47).
- Fabrique : [`StructuralModel3D::createElement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L96-L111).
- Orchestration : [`LinearStaticAnalysis`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Analysis/LinearStaticAnalysis.h#L24-L51).
- Erreurs : [`ErrorCode`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Fem3DError.h#L11-L20) ; tolérances : [`Tolerance`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Types.h#L43-L55).
- Exemple complet : [`fem3d_example.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/examples/fem3d_example.cpp) ; post-traitement :
  [`postprocessor/CMakeLists.txt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/postprocessor/CMakeLists.txt).
- Référence de l'API, générée depuis les en-têtes : [API.md](../../reference/API.md).

## Vérifier par les tests

| Commande | Vérification |
|---|---|
| `build\fem3d_tests.exe` | 41 tests du moteur |
| `build\fem3d_opensees_benchmarks.exe` | 20 cas × 2 formulations contre OpenSees 3.8.0 |
| `ctest --test-dir build --output-on-failure` (dans `postprocessor/`) | 15 tests du post-traitement et génération d'une note d'exemple |
| `python tools/docs/gen_dependency_graph.py` | graphe des inclusions régénéré ([dependances-code.md](../../diagrams/dependances-code.md)) |

## Références

- Documentation d'Eigen (`Matrix`, `SparseMatrix`, `SimplicialLDLT`) — vérifié (contenu),
  [REFERENCES.md](../../references/REFERENCES.md) §C8.
- [ARCHITECTURE.md](../../ARCHITECTURE.md), [DEVELOPER_GUIDE.md](../../developer/DEVELOPER_GUIDE.md).

---

Précédent : [Chapitre 16 — Déplacements, rotations et contraintes](16-deplacements-contraintes.md) · Suivant :
[Chapitre 18 — Intégration avec TSA et OpenSees](18-integration-tsa-opensees.md)
