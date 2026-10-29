# TSA-3D-FEM-Engine

**Fem3D** : moteur EF 3D de poutres par la méthode des déplacements.

Bibliothèque C++20 autonome (Eigen, sans Qt) pour l'analyse **statique linéaire** de structures à barres 3D, avec deux
formulations au choix pour chaque élément :

- **Euler–Bernoulli** (`EulerBernoulli3D`) ;
- **Timoshenko** (`Timoshenko3D`), élément exact sans verrouillage en cisaillement.

Elle calcule les déplacements et rotations nodaux, les réactions (appuis rigides et élastiques), les efforts aux
extrémités, les efforts internes et la déformée exacts le long des barres (charges réparties comprises), ainsi que les
contraintes de résistance des matériaux. Elle contrôle l'équilibre global, désigne les mécanismes et avertit des
quasi-mécanismes.

Validation : **22 cas × 2 formulations comparés à OpenSees 3.8.0 (2 993 valeurs, 0 hors tolérance ; 3 comparaisons non
faisables, raison écrite)**, 49 tests
analytiques et unitaires, audit indépendant des formulations ([docs/AUDIT.md](docs/AUDIT.md)).

> Fem3D est intégré à TSA comme **moteur secondaire** `fem3d` : OpenSees reste le moteur 3D par défaut tant que la
> qualification dans TSA n'est pas complète ([docs/INTEGRATION_TSA.md](docs/INTEGRATION_TSA.md),
> [docs/validation/QUALIFICATION.md](docs/validation/QUALIFICATION.md)).

## Compiler

Il faut CMake ≥ 3.21 et un compilateur C++20. Eigen est pris par défaut dans `../TSA/thirdparty/eigen-3.4.1` (sinon,
définir `FEM3D_EIGEN_DIR`).

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Intégré comme sous-projet (`add_subdirectory`, comme le fait TSA), seule la bibliothèque est compilée
(`FEM3D_BUILD_TESTS=OFF`).

## Tester

```bat
build\fem3d_tests.exe            :: 49 tests unitaires, analytiques et d'intégration
ctest --test-dir build           :: mêmes tests, puis comparaison OpenSees
```

## Comparer à OpenSees

```bat
build\fem3d_opensees_benchmarks.exe [chemin\OpenSees.exe] [dossier de sortie]
```

Par défaut, le programme utilise `../TSA/thirdparty/OpenSees/bin/OpenSees.exe` et écrit dans `benchmarks/OpenSees`.
Voir [benchmarks/OpenSees/README.md](benchmarks/OpenSees/README.md) et
[docs/validation/VALIDATION_OPENSEES.md](docs/validation/VALIDATION_OPENSEES.md).

## Utiliser

```cpp
using namespace TSA::Fem3D;
StructuralModel3D m;
m.addMaterial(ElasticMaterial::fromPoisson("S235", 210e6, 0.3));      // kN/m²
m.addSection(BeamSection3D::rectangle("R30x50", 0.3, 0.5));           // m
m.addNode(1, {0, 0, 0});
m.addNode(2, {4, 0, 0});
ElementDefinition beam{1, 1, 2, "S235", "R30x50", BeamFormulation::Timoshenko, Vector3(0, 0, 1)};
beam.releases.momentYEnd = true;                                      // rotule My au nœud j (facultatif)
m.addElement(beam);
m.setRestraint(NodeRestraint::fullyFixed(1));
m.setRestraint(NodeRestraint::fullyFixed(2));                         // la rotule libère le moment de la barre
// appui élastique sur les DDL libres d'un nœud : m.setSpring({ id, { kx, ky, kz, krx, kry, krz } });
m.addLoadCase({1, "G", {}, {{1, LoadFrame::Global, {0, 0, -5}, {0, 0, -5}}}});
const AnalysisResults r = LinearStaticAnalysis(m).runAll();
writeResultsJson("resultats.json", "Exemple", m, r);                  // format tsa-fem3d-results/1
```

Exemple complet : `examples/fem3d_example.cpp` (`build\fem3d_example.exe sortie.json`).

## Post-traitement

Sous-projet séparé [postprocessor/](postprocessor/) (Qt Core et Gui) : il lit le fichier de résultats et produit les
tableaux CSV, les diagrammes SVG et une note de calcul HTML/PDF. Il ne dépend pas du moteur, seulement du format
[docs/RESULTS_FORMAT.md](docs/RESULTS_FORMAT.md).

## Documentation

Index : [docs/README.md](docs/README.md) — théorie, conventions, unités et repères, formulations, audit indépendant,
plan de validation, résultats OpenSees, qualification, bibliographie et webographie vérifiées, limites connues,
feuille de route.

## Domaine et limites

Le moteur couvre :

- les petits déplacements et l'élasticité linéaire ;
- les sections constantes et la torsion de Saint-Venant uniforme, **sans gauchissement ni bimoment** ;
- les charges nodales ; sur les barres, en repère local ou global : charges réparties linéaires sur toute la longueur
  ou sur une partie [a, b], forces ponctuelles, moments répartis (torsion et flexion) linéaires sur [a, b] ;
- les appuis, les déplacements imposés et les appuis élastiques (ressorts nodaux découplés, axes globaux) ;
- les relâchements d'extrémité : rotules My et Mz en i et/ou en j, par condensation statique.

Il ne traite **pas** : la non-linéarité, la dynamique, les barres de treillis, les relâchements de N, V ou T, les
rotules semi-rigides, les ressorts couplés ou en repère incliné, les excentrements, les couples concentrés sur barre
ni les charges thermiques. Détails : [docs/limitations/KNOWN_LIMITATIONS.md](docs/limitations/KNOWN_LIMITATIONS.md).

## Licence

Le code est destiné à être publié. La licence n'est pas encore choisie par l'auteur ; en attendant, aucun droit de
réutilisation n'est accordé au-delà de la consultation. OpenSees, utilisé seulement comme référence de validation,
n'est ni copié ni distribué ici.
