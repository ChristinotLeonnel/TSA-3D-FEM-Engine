# F. Choix du moteur dans TSA : Fem3D, comparaison, secours

D'après `AnalysisManager::prepare` / `run` et `Fem3DEngine::validate` / `run` (dépôt TSA, révision 2f5d7d4) :
[Fem3DEngine.cpp](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DEngine.cpp),
[AnalysisManager.cpp](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engine/AnalysisManager.cpp).
OpenSees est le moteur par défaut ; Fem3D n'est utilisé que si l'utilisateur le choisit.

```mermaid
flowchart TD
    U["Utilisateur : moteur fem3d<br/>réglages mode et fallback"] --> G{"Validation générique<br/>(capacités : 3D, poutres, ressorts, statique)"}
    G -->|"treillis, câble, non linéaire,<br/>modal, coques maillées…"| R0["Refus avant calcul<br/>(aucun secours possible à ce stade)"]
    G --> C["Fem3DAdapter::convert"]
    C --> E{"Données refusées ?<br/>(relâchement N/V/T, axes différents,<br/>plusieurs cas sans combinaison…)"}
    E -->|"oui"| R1["Refus : erreur listée"]
    E -->|"non"| O{"Hors périmètre ?<br/>(charge ponctuelle ou partielle,<br/>moment réparti)"}
    O -->|"oui, fallback = none"| R2["Refus : raisons listées<br/>« choisir OpenSees ou activer le secours »"]
    O -->|"oui, fallback = opensees"| AV{"OpenSees disponible ?"}
    AV -->|"non"| R3["Refus"]
    AV -->|"oui"| FB["Calcul par OpenSees<br/>annoncé à la validation"]
    FB --> PB["producedBy = OpenSees :<br/>résultats étiquetés OpenSees,<br/>raison dans les avertissements"]
    O -->|"non"| F3["Calcul Fem3D"]
    F3 --> ME{"Fem3DError ?"}
    ME -->|"mécanisme ou autre"| R4["Échec, diagnostic affiché<br/>JAMAIS de secours"]
    ME -->|"non"| MR["Résultats Fem3D<br/>(étiquette Fem3D)"]
    MR --> CM{"mode = compare ?"}
    CM -->|"non"| END["Affichage"]
    CM -->|"oui"| OSR["Calcul OpenSees de contrôle"]
    OSR --> OSK{"Réussi ?"}
    OSK -->|"non"| WC["Avertissement :<br/>comparaison impossible"]
    OSK -->|"oui"| TAB["Table « Comparaison Fem3D / OpenSees »<br/>avertissement si écart ;<br/>résultats affichés = Fem3D, sans fusion"]
    WC & TAB --> END
```

Tests : TSA, suite `engines`, tests 476 à 480 ([INTEGRATION_TSA.md](../INTEGRATION_TSA.md)).
