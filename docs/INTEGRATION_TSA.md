# Intégration dans TSA

Fem3D est intégré à TSA comme **moteur secondaire** `fem3d`, derrière l'interface d'analyse extensible de TSA
(`src/Analysis/Engine/AnalysisEngine.h`), à côté d'`OpenSeesEngine` et de `Custom2DEngine`. **OpenSees reste le
moteur 3D par défaut** tant que le niveau 5 de qualification n'est pas atteint
([validation/QUALIFICATION.md](validation/QUALIFICATION.md)). Les deux dépôts restent distincts.

## 1. Dépendance

- TSA cherche ce dépôt dans `../3DDesplacement` (variable CMake `TSA_FEM3D_DIR`), comme il cherche TSALab dans
  `../TSALab`. Présent, la bibliothèque `fem3d` est compilée (sans ses tests : `FEM3D_BUILD_TESTS=OFF`) et le moteur
  est enregistré (`TSA_HAS_FEM3D`). Absent, TSA se construit sans lui et le signale à la configuration.
- Fem3D n'ajoute aucune dépendance : Eigen est celui de TSA. La bibliothèque n'utilise ni Qt ni OCCT.

## 2. Fichiers dans TSA

| Fichier | Rôle |
|---|---|
| `src/Analysis/Engines/Fem3D/Fem3DAdapter.*` | Conversion `CalculationSnapshot` → `StructuralModel3D` ; diagnostic du périmètre ; remappage des résultats. |
| `src/Analysis/Engines/Fem3D/Fem3DEngine.*` | Moteur : capacités, validation, calcul, secours et comparaison. |
| `src/UI/Analysis/Fem3DOptionsWidget.*` | Options : mode (Fem3D seul / comparé à OpenSees), secours (refus / OpenSees). |
| `src/Analysis/Engines/BuiltInEngines.cpp` | Enregistrement après OpenSees et Custom2D. |
| `src/Analysis/OpenSeesResultsReader.*` | `beamResultsFromEndForces` : reconstruction commune des stations ; équilibre et blocages automatiques rendus publics. |
| `src/Analysis/Engine/AnalysisEngine.h`, `AnalysisManager.cpp` | `AnalysisRunResult::producedBy` : un calcul délégué porte l'identité du moteur qui l'a fait. |
| `tests/test_analysis_engines.cpp` | Tests 476 à 480. |

## 3. Correspondance

La conversion **réutilise les briques du chemin OpenSees**, pour que les deux moteurs calculent le même modèle et
que leurs résultats soient présentés de la même façon :

| TSA | Fem3D | Remarque |
|---|---|---|
| Nœuds du snapshot | `Node3D` (identifiant conservé) | |
| Poutres, poteaux (`OpenSeesModelMap`) | `ElementDefinition`, Euler–Bernoulli | id = tag OpenSees ; v_xz = vecteur de `geomTransf` ; axes contrôlés (refus s'ils diffèrent) |
| Matériau, section | `ElasticMaterial`, `BeamSection3D` | E : Pa → kN/m² ; G = E / 2(1 + ν) ; A, Iy, Iz, J |
| Rotules My, Mz | `EndReleases` | relâchements de N, V ou T refusés |
| Appuis (`fix*`) | `NodeRestraint` | y compris les blocages automatiques de TSA, contrôlés après le calcul |
| Ressorts (`OpenSeesModelMap::springs`) | `NodalSpring` | ressort sur un DDL bloqué ignoré et signalé (sans effet) |
| Cas, combinaison, poids propre (`loadPatterns`) | un `LoadCase3D` | mêmes facteurs |
| Charges nodales | `NodalLoad3D` | |
| Charges sur barre (`beamElementLoads`, axes locaux) | `DistributedLoad3D` locale | uniformes, ou linéaires sur toute la longueur ; le reste est hors périmètre |
| Résultats | `ResultsModel` | déplacements ; réactions = appuis + ressorts ; efforts d'extrémité Fem3D (convention `localForce`) → stations par la même reconstruction qu'OpenSees ; équilibre global et blocages automatiques par les fonctions communes |

## 4. Périmètre et logique de secours

**Périmètre** : statique linéaire 3D ; poutres et poteaux ; rotules My, Mz ; appuis et ressorts ; charges nodales,
réparties uniformes ou linéaires sur toute la longueur, poids propre.

**Hors périmètre** (refusé avec la raison) : charges ponctuelles ou partielles sur barre, moments répartis, coques.
Les treillis, les câbles, la non-linéarité et l'analyse modale sont refusés en amont par la validation générique de
TSA, à partir des capacités déclarées.

| Réglage | Effet |
|---|---|
| mode `fem3d` (défaut) | Fem3D seul. |
| mode `compare` | Fem3D puis OpenSees. Résultats affichés : Fem3D. Table « Comparaison Fem3D / OpenSees » (déplacements, réactions, efforts d'extrémité) et avertissement au-delà de la tolérance. Aucune fusion. |
| secours `none` (défaut) | Hors périmètre : calcul refusé, raisons listées. |
| secours `opensees` | Hors périmètre : annoncé à la validation, calcul fait par OpenSees, résultats étiquetés OpenSees (`producedBy`), raison dans les avertissements. |

Un **mécanisme** ou une **donnée invalide** n'est jamais confié à un autre moteur : c'est une propriété du modèle,
pas une panne du moteur. Le message le dit.

## 5. Non-régression

- Les suites de TSA restent inchangées, sauf le test 130 (nombre de moteurs enregistrés : 3 avec Fem3D).
- Les projets `.tsa` ne changent pas : le moteur choisi et ses réglages sont des réglages d'analyse existants.
- Tests 476 à 480 : enregistrement et capacités ; portique spatial (rotules, ressort, charges uniformes et
  trapézoïdales, poids propre) calculé par Fem3D et par OpenSees, sans écart, stations comprises ; refus hors
  périmètre puis secours étiqueté ; mécanisme sans secours ; options.
