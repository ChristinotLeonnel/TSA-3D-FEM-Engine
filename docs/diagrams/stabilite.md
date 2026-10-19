# E. Diagnostic de stabilité

D'après [LinearSystemSolver::factorize](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L59-L98)
et `LinearStaticAnalysis` (avertissements, résidu).

```mermaid
flowchart TD
    V{"Données valides ?<br/>StructuralModel3D::validate"} -->|"non"| E0["Fem3DError (InvalidInput…)<br/>objet fautif désigné"]
    V -->|"oui"| N{"K_ff vide ?<br/>(aucun DDL libre)"}
    N -->|"oui"| OK0["Modèle valable : réponse due aux<br/>déplacements imposés et charges"]
    N -->|"non"| F["Factorisation P K_ff Pᵀ = L D Lᵀ<br/>(SimplicialLDLT)"]
    F --> FI{"Factorisation réussie ?"}
    FI -->|"non"| MEC
    FI -->|"oui"| P{"Pour chaque pivot d_k :<br/>d_k > 1e-12 · K_kk (diagonale propre) ?"}
    P -->|"non"| MEC["Mécanisme"]
    MEC --> DIAG{"n ≤ 2000 DDL libres ?"}
    DIAG -->|"oui"| MODES["Modes propres nuls de S K_ff S<br/>S = diag(K_ff)^(−1/2)<br/>DDL participants listés"]
    DIAG -->|"non"| NOLIST["Mécanisme non localisé"]
    MODES & NOLIST --> EM["Fem3DError Mechanism<br/>aucun résultat"]
    P -->|"oui"| Q{"Plus petit d_k / K_kk < 1e-8 ?"}
    Q -->|"oui"| WQ["Avertissement : quasi-mécanisme,<br/>DDL et chiffres perdus estimés"]
    Q -->|"non"| S["Résolution"]
    WQ --> S
    S --> RES{"‖K U − F‖ / ‖F‖ > 1e-8 ?"}
    RES -->|"oui"| WR["Avertissement : résidu élevé"]
    RES -->|"non"| OK["Résultats"]
    WR --> OK
```

Le critère compare chaque pivot à la rigidité de **son propre** DDL : il ne dépend pas des unités (test
`Solver_MechanismCriterionIndependentOfUnits`). Dans TSA, un mécanisme n'est jamais confié à un autre moteur
([selection-moteur-tsa.md](selection-moteur-tsa.md)).
