# Audit du moteur Fem3D

**État au 19/10/2026, version 0.2.0.** Cet audit rassemble ce qui a été vérifié, comment, ce qui a été trouvé et ce
qui a été corrigé. Les détails techniques sont dans les rapports cités.

## 1. Périmètre et méthode

| Volet | Méthode | Rapport |
|---|---|---|
| Formulations (matrices, fonctions de forme, charges, rotules, sections, solveur) | Audit indépendant en lecture seule : dérivation symbolique (sympy) des équations de Timoshenko, sonde C++ compilée avec les sources du moteur, séries exactes de Saint-Venant | [verification/INDEPENDENT_FORMULATION_AUDIT.md](verification/INDEPENDENT_FORMULATION_AUDIT.md) |
| Validation croisée | 20 cas × 2 formulations contre OpenSees 3.8.0, tolérances fixées avant les essais | [validation/VALIDATION_OPENSEES.md](validation/VALIDATION_OPENSEES.md) |
| Solutions analytiques | 41 tests (unitaires, analytiques, intégration) | `tests/` |
| Sources | Bibliographie et webographie vérifiées, manifeste des sources (statut de consultation, licence) | [references/](references/) |
| Intégration dans TSA | Moteur `fem3d` comparé à OpenSees dans TSA (tests 476 à 480) | [INTEGRATION_TSA.md](INTEGRATION_TSA.md) |

## 2. Constats et suites

| # | Constat | Gravité | Origine | Suite |
|---|---|---|---|---|
| 1 | Critère de mécanisme dépendant des unités : un modèle stable était refusé en N-mm et accepté en kN-m (pivot comparé à la plus grande diagonale, qui mélange F/L et F·L). | moyenne | audit indépendant | **Corrigé** : pivot comparé à la diagonale de son propre DDL ; modes propres de S K S. Test `Solver_MechanismCriterionIndependentOfUnits`, qui échoue avec l'ancien critère (vérifié). |
| 2 | Module de torsion Wt du rectangle surestimé jusqu'à 4,4 % : contrainte de torsion sous-estimée, non conservatif. | faible | audit indépendant, bibliographie | **Corrigé** : polynôme de Roark, à 0,3 % des séries exactes de Saint-Venant (sommées indépendamment). Test `Section_RectangleTorsionMatchesExactSeries`. |
| 3 | Quasi-mécanismes acceptés sans avertissement ; pas de résidu de résolution. | faible | audit indépendant | **Corrigé** : avertissement (pivot relatif < 10⁻⁸, DDL désigné) et résidu ‖K U − F‖/‖F‖, exportés en JSON. Test `Solver_QuasiMechanismWarnsButSolves`. |
| 4 | Coefficient de Cowper du cercle avec ν = 0,3 figé. | faible | audit indépendant | **Corrigé** : ν en paramètre (0,3 par défaut). Test `Section_CircleShearCoefficientUsesPoisson`. |
| 5 | Contrôle d'équilibre presque tautologique vis-à-vis de K. | faible | audit indépendant | **Documenté** (THEORY §8) : il ne valide pas la rigidité, que valident l'analytique et OpenSees. |
| 6 | Orientation par défaut discontinue pour les barres presque verticales. | faible | audit indépendant | **Documenté** (UNITS_AND_COORDINATES §4) ; TSA fournit toujours v_xz. |
| 7 | Documentation : charge triangulaire « pour les deux formulations » avec des valeurs Euler–Bernoulli ; rotule et mécanisme trop généraux ; « 0,9 A » ; renvoi §7 ; « moyenne » V/As. | documentaire | audit indépendant | **Corrigé**. |
| 8 | Tests manquants : Asy ≠ Asz, Timoshenko sous charge triangulaire avec et sans rotule. | couverture | audit indépendant | **Ajoutés**, valeurs exactes re-dérivées indépendamment. |
| 9 | Bibliographie : « GJ seul surestime la rigidité » avec gauchissement empêché. | documentaire | relecture | **Corrigé** : il la sous-estime. |
| 10 | Un modèle sans DDL libre était refusé ; un pivot exactement nul ne désignait pas les DDL. | moyenne | développement (v0.1) | **Corrigé** et testé. |
| 11 | Avec `equalDOF`, `nodeReaction` d'OpenSees omet l'effort transmis au nœud esclave. | — (référence) | benchmarks | Réaction de référence = maître + esclave ; aucune valeur de référence modifiée. |
| 12 | OpenSees 3.8.0 ignore sans message la variation de `-beamUniform` à 8 arguments en 3D. | — (référence) | benchmarks | Charges trapézoïdales déclarées « non comparables » ; comparées indirectement dans TSA. |

**Aucun écart n'a été trouvé dans les formulations d'élément** : matrices Euler–Bernoulli et Timoshenko, signes,
Φ et association des aires de cisaillement, fonctions de forme, repère local, quadrature, forces équivalentes
(exactes aussi en Timoshenko), efforts internes, déformée, condensation des rotules, réactions.

## 3. Risques résiduels

- Niveau 5 de qualification non atteint dans TSA ([validation/QUALIFICATION.md](validation/QUALIFICATION.md)) :
  OpenSees reste le moteur par défaut.
- Une seule version de référence (OpenSees 3.8.0) ; charges trapézoïdales comparées indirectement.
- Les numéros de chapitre et de page des ouvrages n'ont pas été vérifiés sur les textes : seules les notices l'ont
  été ([references/SOURCES_MANIFEST.md](references/SOURCES_MANIFEST.md)).
- Relecture humaine de la théorie à faire.
