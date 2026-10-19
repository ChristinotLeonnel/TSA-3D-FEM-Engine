# Architecture

Le moteur est une bibliothèque C++20 autonome (`fem3d`), qui ne dépend que d'Eigen 3.4 et d'aucun composant Qt
ni graphique. Le post-traitement est un sous-projet séparé du même dépôt ([`../postprocessor`](../postprocessor), Qt), qui ne communique avec le
moteur que par le fichier de résultats versionné ([RESULTS_FORMAT.md](RESULTS_FORMAT.md)).

```text
include/Fem3D/                         src/
  Core/        Types.h (DDL, Matrix12, tolérances), Fem3DError.h (codes d'erreur), Polynomial.h
  Geometry/    Node3D.h, LocalCoordinateSystem3D.h         → repère local, R, T
  Materials/   ElasticMaterial.h                           → E, G, contrôle
  Sections/    BeamSection3D.h                             → A, Iy, Iz, J, Asy, Asz, fibres, Wt
  Elements/    IBeamElement3D.h (interface)                → K_local, fonctions de forme, flexibilités
               BeamElementBase.h (données communes)
               EulerBernoulli3D.h, Timoshenko3D.h          → équations propres à chaque formulation
               EndReleases.h, StaticCondensation.h         → rotules My/Mz, condensation commune aux formulations
  Loads/       Loads3D.h (charges nodales, réparties, cas)
  Constraints/ BoundaryConditions.h (DDL imposés, valeurs ; appuis élastiques NodalSpring)
  Model/       StructuralModel3D.h                         → données saisies, contrôle, fabrique d'éléments
  Assembly/    DofMap.h, GlobalAssembler.h                 → numérotation f/r, K partitionnée, F
  Solver/      LinearSystemSolver.h                        → LDLᵀ creuse, mécanismes (critère indépendant des unités),
                                                             quasi-mécanismes
  Results/     AnalysisResults.h, BeamResponse.h           → efforts et déformée exacts, contraintes
               ResultsJsonWriter.h                         → export « tsa-fem3d-results/1 »
  Analysis/    LinearStaticAnalysis.h                      → orchestration, équilibre, export Matrix Market
tests/         unit/, analytical/, integration/ (fem3d_tests)
postprocessor/ sous-projet Qt : lecture du JSON, tableaux, diagrammes, note de calcul (fem3dpost, fem3d_report)
benchmarks/OpenSees/  générateur de scripts Tcl, exécution, comparaison (fem3d_opensees_benchmarks)
examples/      fem3d_example (portique spatial → JSON)
```

## Dépendances (sans cycle)

`Core ← Geometry ← Elements ← Model ← Assembly ← Solver ← Analysis` ; `Results` dépend d'`Elements` et des charges ;
`Analysis` réunit le tout.

## Responsabilités

- **Formulation** (`EulerBernoulli3D`, `Timoshenko3D`) : la matrice de rigidité locale, les fonctions de forme et les
  flexibilités de cisaillement. Rien d'autre : ces classes ne connaissent ni l'assemblage, ni les charges globales,
  ni les résultats.
- **Modèle** : il ne contient que des données. Les éléments finis sont créés par `createElement` selon la formulation
  de chaque définition, ce qui permet de choisir l'élément par barre.
- **Assemblage** : il produit les blocs `K_ff`, `K_fr`, `K_rf`, `K_rr` et `F` par cas de charge.
- **Solveur** : une factorisation unique, réutilisée pour tous les cas.
- **Résultats** (`BeamResponse`) : la seule classe qui reconstruit efforts internes, déformée et contraintes, de façon
  commune aux deux formulations grâce aux trois éléments de l'interface.

## Erreurs

Les données invalides et les situations numériques lèvent `Fem3DError`, avec un code stable (`InvalidInput`,
`UnknownReference`, `DuplicateId`, `DegenerateGeometry`, `Mechanism`, `IoError`) et un message qui désigne l'objet
fautif : élément, nœud, DDL.

## Choix techniques

- Eigen est déjà livré avec TSA (`TSA/thirdparty/eigen-3.4.1`). On utilise des matrices fixes 12×12 pour les éléments
  et des matrices creuses pour le système global.
- Les efforts et la déformée sont représentés par des **polynômes exacts**, plutôt que par un échantillonnage :
  les stations ne sont qu'un choix d'affichage.
- Aucune variable globale. Les éléments sont possédés par `LinearStaticAnalysis` (`std::unique_ptr`) ;
  `BeamResponse` ne garde qu'une référence vers un élément vivant.

## Intégration dans TSA

TSA compile la bibliothèque `fem3d` seule (`add_subdirectory`, `FEM3D_BUILD_TESTS=OFF`) et l'utilise par un
adaptateur qui lui est propre (`TSA/src/Analysis/Engines/Fem3D`). Le moteur ne connaît pas TSA : la dépendance va
dans un seul sens. Voir [INTEGRATION_TSA.md](INTEGRATION_TSA.md).
