# C. Validation contre OpenSees

D'après le programme de comparaison
[opensees_benchmarks.cpp](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp)
(fonction `main`). Tolérance fixée avant les essais : écart absolu ≤ max(1e-6 × réf, 1e-9 × échelle).

```mermaid
flowchart TD
    S["Pour chaque cas (01 à 20)<br/>et chaque formulation (EB, Timoshenko)"] --> M["Modèle C++ unique<br/>StructuralModel3D"]
    M --> T{"Script Tcl équivalent<br/>writeTcl"}
    T -->|"pas de référence exacte<br/>(trapèze, rotule Timoshenko hors axes)"| NC["NON COMPARABLE<br/>raison écrite"]
    T -->|"script écrit"| OS["Exécution d'OpenSees 3.8.0<br/>nodeDisp, nodeReaction, localForce"]
    M --> FEM["Calcul Fem3D<br/>LinearStaticAnalysis::run"]
    OS --> N["Normalisation des conventions<br/>réaction = maître + esclave (equalDOF),<br/>ressort = réaction du nœud fixe"]
    FEM --> X{"Cas de mécanisme ?"}
    N --> X
    X -->|"oui (11, 17)"| MC{"Fem3D refuse ET OpenSees<br/>échoue ou diverge (‖u‖ > 1e6)"}
    MC -->|"oui"| OK1["OK"]
    MC -->|"non"| EC1["ECART"]
    X -->|"non"| C["Comparaison valeur par valeur<br/>déplacements, réactions, ressorts,<br/>efforts d'extrémité, rotations aux rotules"]
    C --> SC["Échelles par nature de grandeur<br/>planchers : M ≥ F·L, θ ≥ u/L"]
    SC --> TOL{"Toutes les valeurs<br/>dans la tolérance ?"}
    TOL -->|"oui"| OK2["OK"]
    TOL -->|"non"| EC2["ECART"]
    OK1 & OK2 & EC1 & EC2 & NC --> REP["Rapports<br/>CSV par cas, synthese.md, synthese.csv"]
    REP --> EXIT["Code de sortie = nombre de cas en ECART"]
```

Résultats mesurés : [VALIDATION_OPENSEES.md](../validation/VALIDATION_OPENSEES.md). Méthode :
[SCIENTIFIC_VALIDATION_GUIDE.md](../validation/SCIENTIFIC_VALIDATION_GUIDE.md).
