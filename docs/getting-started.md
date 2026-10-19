# Démarrage rapide

## 1. Récupérer et compiler

Prérequis : Git, CMake ≥ 3.21, un compilateur C++20, Eigen 3.4.

```bat
git clone https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine.git
cd TSA-3D-FEM-Engine
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFEM3D_EIGEN_DIR=<dossier contenant Eigen/>
cmake --build build
```

Sans `FEM3D_EIGEN_DIR`, Eigen est cherché dans `../TSA/thirdparty/eigen-3.4.1/include/eigen3`.

## 2. Vérifier l'installation

```bat
build\fem3d_tests.exe
```

Résultat attendu : `RESULTATS : 41 / 41 tests reussis`.

## 3. Premier calcul

Une poutre de 4 m encastrée à gauche, articulée en flexion à droite, sous une charge répartie de 5 kN/m (exemple du
README, exécuté par le test `Readme_ExampleRuns`) :

```cpp
#include "Fem3D/Analysis/LinearStaticAnalysis.h"
#include "Fem3D/Results/ResultsJsonWriter.h"

using namespace TSA::Fem3D;

int main()
{
    StructuralModel3D m;
    m.addMaterial(ElasticMaterial::fromPoisson("S235", 210e6, 0.3));   // kN/m²
    m.addSection(BeamSection3D::rectangle("R30x50", 0.3, 0.5));        // m
    m.addNode(1, { 0, 0, 0 });
    m.addNode(2, { 4, 0, 0 });
    ElementDefinition beam{ 1, 1, 2, "S235", "R30x50", BeamFormulation::Timoshenko, Vector3(0, 0, 1) };
    beam.releases.momentYEnd = true;                                   // rotule My au nœud j
    m.addElement(beam);
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.setRestraint(NodeRestraint::fullyFixed(2));
    m.addLoadCase({ 1, "G", {}, { { 1, LoadFrame::Global, { 0, 0, -5 }, { 0, 0, -5 } } } });
    const AnalysisResults r = LinearStaticAnalysis(m).runAll();
    writeResultsJson("resultats.json", "Exemple", m, r);
}
```

Le fichier `resultats.json` suit le format décrit dans [RESULTS_FORMAT.md](RESULTS_FORMAT.md).

## 4. Produire une note de calcul

Avec Qt 6 : `postprocessor\build.cmd`, puis
`postprocessor\build\fem3d_report.exe resultats.json --out sortie --pdf` (tableaux CSV, diagrammes SVG, note HTML et PDF).

## 5. Ensuite

- Comprendre ce qui s'est passé : [parcours pédagogique](learning/index.md).
- Comparer à OpenSees : [guide de validation](validation/SCIENTIFIC_VALIDATION_GUIDE.md).
- Utiliser Fem3D dans TSA : [intégration](INTEGRATION_TSA.md).
