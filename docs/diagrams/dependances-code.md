# Dépendances du code (généré)

Généré par `python tools/docs/gen_dependency_graph.py` à partir des `#include "Fem3D/…"` de `include/` et
`src/` : **ne pas modifier à la main**. Une flèche A → B signifie « un fichier du module A inclut un en-tête du
module B ». Le graphe des fichiers est dans `dependances-code.dot` (Graphviz :
`dot -Tsvg dependances-code.dot -o dependances-code.svg`).

34 fichiers, 12 modules, 30 dépendances entre modules,
61 inclusions internes.

```mermaid
flowchart BT
    Analysis["Analysis"]
    Assembly["Assembly"]
    Constraints["Constraints"]
    Core["Core"]
    Elements["Elements"]
    Geometry["Geometry"]
    Loads["Loads"]
    Materials["Materials"]
    Model["Model"]
    Results["Results"]
    Sections["Sections"]
    Solver["Solver"]
    Analysis --> Assembly
    Analysis --> Core
    Analysis --> Results
    Analysis --> Solver
    Assembly --> Core
    Assembly --> Elements
    Assembly --> Model
    Assembly --> Results
    Constraints --> Core
    Elements --> Core
    Elements --> Geometry
    Elements --> Materials
    Elements --> Sections
    Geometry --> Core
    Loads --> Core
    Materials --> Core
    Model --> Constraints
    Model --> Core
    Model --> Elements
    Model --> Geometry
    Model --> Loads
    Model --> Materials
    Model --> Sections
    Results --> Core
    Results --> Elements
    Results --> Loads
    Results --> Model
    Sections --> Core
    Solver --> Assembly
    Solver --> Core
```

| Module | Dépend de |
|---|---|
| Analysis | Assembly, Core, Results, Solver |
| Assembly | Core, Elements, Model, Results |
| Constraints | Core |
| Core | — |
| Elements | Core, Geometry, Materials, Sections |
| Geometry | Core |
| Loads | Core |
| Materials | Core |
| Model | Constraints, Core, Elements, Geometry, Loads, Materials, Sections |
| Results | Core, Elements, Loads, Model |
| Sections | Core |
| Solver | Assembly, Core |
