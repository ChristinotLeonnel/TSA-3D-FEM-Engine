# Diagrammes

Les diagrammes sont écrits en **Mermaid** dans ces pages (sources versionnées, rendues par GitHub et par le site).
Le graphe des fichiers est en **Graphviz DOT**, plus adapté aux nombreuses dépendances (`dependances-code.dot`).
PlantUML n'est pas utilisé : Mermaid couvre les besoins (flux, classes) sans dépendance Java.

| Diagramme | Contenu | Source |
|---|---|---|
| [A. Architecture](architecture.md) | moteur, post-traitement, intégration TSA, OpenSees | écrit à la main, vérifié sur le code |
| [B. Calcul](calcul.md) | étapes d'un calcul et branches d'erreur | écrit à la main, d'après `LinearStaticAnalysis` |
| [C. Validation OpenSees](validation-opensees.md) | programme de comparaison | d'après `opensees_benchmarks.cpp` |
| [D. Éléments poutre](elements-poutre.md) | DDL, repère local, transformation, classes | d'après `Elements/`, `Geometry/` |
| [E. Stabilité](stabilite.md) | mécanismes, quasi-mécanismes, résidu | d'après `LinearSystemSolver` |
| [F. Choix du moteur dans TSA](selection-moteur-tsa.md) | modes, refus, secours | d'après `Fem3DEngine` (TSA) |
| [H. Dépendances du code](dependances-code.md) | modules C++ | **généré** par `tools/docs/gen_dependency_graph.py` |

Les diagrammes pédagogiques (G) sont dans chaque niveau du [parcours](../learning/index.md).
