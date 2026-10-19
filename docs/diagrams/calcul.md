# B. Déroulement d'un calcul Fem3D

D'après `LinearStaticAnalysis` (constructeur puis `run`) :
[LinearStaticAnalysis.cpp](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp).
Les branches d'erreur lèvent `Fem3DError` avec un code stable ; les avertissements ne bloquent pas le calcul.

```mermaid
flowchart TD
    A["1. Construction du modèle<br/>StructuralModel3D : nœuds, matériaux, sections,<br/>éléments, appuis, ressorts, cas de charge"] --> B
    B{"2. Validation des données<br/>StructuralModel3D::validate"}
    B -->|"donnée invalide"| EB["Fem3DError<br/>InvalidInput, UnknownReference,<br/>DuplicateId, DegenerateGeometry"]
    B -->|"valide"| C["3. Génération des éléments<br/>createElement selon la formulation"]
    C --> D["4. Matrices élémentaires<br/>K_local (EB ou Timoshenko),<br/>condensation des rotules"]
    D --> E["5. Transformation locale → globale<br/>K_g = Tᵀ K_eff T"]
    E --> F["6. Assemblage partitionné<br/>K_ff, K_fr, K_rf, K_rr (DofMap)"]
    F --> G["7. Conditions aux limites et ressorts<br/>partition libres / imposés,<br/>k ajouté sur la diagonale"]
    G --> H{"8. Factorisation LDLᵀ<br/>et contrôle de stabilité"}
    H -->|"pivot nul ou négligeable"| EH["Fem3DError Mechanism<br/>DDL des modes de mécanisme"]
    H -->|"pivot faible (< 1e-8)"| W1["Avertissement<br/>quasi-mécanisme"]
    H -->|"stable"| I
    W1 --> I["9. Résolution par cas<br/>K_ff U_f = F_f − K_fr U_r"]
    I --> R{"Résidu relatif<br/>> 1e-8 ?"}
    R -->|"oui"| W2["Avertissement<br/>résidu élevé"]
    R -->|"non"| J
    W2 --> J["10. Déplacements, réactions<br/>R = K_rf U_f + K_rr U_r − F_r,<br/>forces des ressorts −k·u"]
    J --> K["11. Reconstruction des efforts<br/>BeamResponse : f = K d − f_eq,<br/>efforts et déformée exacts"]
    K --> L["Contrôle d'équilibre global<br/>ΣF + ΣR + ΣF_ressorts = 0"]
    L --> M["12. Résultats et export JSON<br/>writeResultsJson"]
    M --> N["13. Post-traitement et note de calcul<br/>postprocessor/ (fem3d_report)"]
```

Étapes 2 à 8 : une seule fois par modèle (constructeur) ; étapes 9 à 12 : pour chaque cas de charge (`run`).
Détails : [livre, chapitres 12 à 16](../book/Fem3D_Technical_Book.md).
