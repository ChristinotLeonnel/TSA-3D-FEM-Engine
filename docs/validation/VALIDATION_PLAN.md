# Plan de validation

Ce plan dit, pour chaque fonction du moteur, **contre quoi** elle est validée, **avec quelle tolérance** et **avec
quel verdict**. Les tolérances ont été fixées avant les essais et n'ont jamais été modifiées pour masquer un écart.

## 1. Verdicts

| Verdict | Signification |
|---|---|
| **Validé** | Toutes les valeurs comparées sont dans la tolérance, contre une référence indépendante (OpenSees) **et** contre des solutions analytiques. |
| **Validé avec réserves** | Valeurs dans la tolérance, mais la référence est incomplète : solution analytique seule, référence indirecte, ou cas limité. La réserve est écrite. |
| **Échec** | Au moins une valeur hors tolérance, ou un calcul impossible là où il devrait aboutir. |
| **Non testé** | Aucune comparaison n'a été faite. La fonction ne doit pas être utilisée pour un calcul qui compte. |

## 2. Références et tolérances

| Référence | Tolérance | Justification |
|---|---|---|
| Solutions analytiques (tests `fem3d_tests`) | écart relatif ≤ 1e-9 | Éléments exacts pour des barres prismatiques : seul l'arrondi subsiste. |
| OpenSees 3.8.0 (`fem3d_opensees_benchmarks`) | \|écart\| ≤ max(1e-6 · \|réf\|, 1e-9 · échelle) | [VALIDATION_OPENSEES.md](VALIDATION_OPENSEES.md) §3 : 10⁴ à 10⁶ fois plus petit que l'effet d'une erreur de formulation. Échelle par nature de grandeur, avec planchers dimensionnels. |
| Séries exactes de Saint-Venant (torsion du rectangle) | écart relatif ≤ 6e-3 | Précision annoncée des formules approchées de Roark (≤ 0,5 %), plus une marge. |
| Mode comparaison de TSA (Fem3D contre OpenSees de TSA) | mêmes critères que les benchmarks | `Fem3DEngine::kCompareRelative`, `kCompareAbsoluteFactor` (dépôt TSA). |

## 3. Cas et verdicts au 19/10/2026

| Fonction | Analytique | OpenSees (cas) | TSA | Verdict |
|---|---|---|---|---|
| Euler–Bernoulli : traction, torsion, flexion dans les deux plans | `Analytical_*` | 01 à 06 | test 477 | **Validé** |
| Timoshenko (élément exact, Φy et Φz) | console, poutre courte, Asy ≠ Asz | 01 à 10 | non exposé dans TSA | **Validé** |
| Repère local (v_xz, barres inclinées et verticales) | invariance par rotation | 07, 10, 16 | test 477 | **Validé** |
| Charges nodales | console, portique | 01 à 10 | test 477 | **Validé** |
| Charges réparties uniformes, locales et globales, sur toute la longueur | bi-encastrée | 05, 07, 09, 10 | test 477 | **Validé** |
| Charges réparties linéaires (trapézoïdales) sur toute la longueur | EB et Timoshenko, avec et sans rotule | non comparable dans OpenSees 3.8.0 | test 477 (référence : OpenSees de TSA, charge ramenée aux nœuds par TSA) | **Validé avec réserves** : comparaison à OpenSees indirecte |
| Rotules My, Mz (Euler–Bernoulli) | console sur appui, Gerber, mécanisme | 12 à 17 | test 477 | **Validé** |
| Rotules My, Mz (Timoshenko), axe relâché = axe global | idem | 12 à 15 | — | **Validé** |
| Rotules (Timoshenko), axe relâché quelconque | condensation commune | 16 : non comparable | — | **Validé avec réserves** : analytique seul |
| Appuis élastiques (ressorts nodaux, axes globaux) | parallèle, rotation, structure sur ressorts | 18 à 20 | test 477 | **Validé** |
| Déplacements imposés non nuls | tassement d'appui | — | — | **Validé avec réserves** : analytique seul |
| Détection des mécanismes, invariance par les unités | kN-m et N-mm | 11, 17 | test 479 | **Validé** |
| Quasi-mécanismes, résidu de résolution (avertissements) | ressort de 10⁻¹⁰ GJ/L | — | — | **Validé avec réserves** : analytique seul |
| Contraintes de RDM (σ, τ, Wt, J) | τ = T/Wt, séries de Saint-Venant | OpenSees ne les calcule pas | — | **Validé avec réserves** : formules approchées documentées |
| Forces ponctuelles sur barre (0.3.0) | bi-encastrée, sur deux appuis (EB et Timoshenko), aux extrémités, compatibilité, subdivision | 21 (EB) ; Timoshenko non comparable | test 530 | **Validé** (EB) ; **validé avec réserves** (Timoshenko : analytique) |
| Charges réparties sur une partie [a, b] (0.3.0) | console chargée sur [a, L] et [0, b] (EB et Timoshenko), compatibilité, subdivision | 22 (EB, uniformes) ; Timoshenko non comparable | test 530 | **Validé** (EB, uniformes) ; **validé avec réserves** (trapézoïdales, Timoshenko) |
| Moments répartis, torsion et flexion (0.3.0) | console (deux formulations), compatibilité exacte bi-encastrée | aucune commande OpenSees | test 530 (référence : OpenSees de TSA, couple ramené aux nœuds par TSA) | **Validé avec réserves** : analytique et comparaison indirecte |
| Barres de treillis, câbles, coques, non-linéarité, dynamique, gauchissement | — | — | refusés | **Non testé** (non implémenté) |

Aucun **échec** n'est ouvert. Les deux défauts trouvés en cours de route (critère de mécanisme dépendant des unités, Wt
non conservatif) ont été corrigés ; ils sont couverts par des tests qui échouaient avec l'ancien code
([../AUDIT.md](../AUDIT.md)).

## 4. Pour reproduire

```bat
build\fem3d_tests.exe
build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier]
```

Dans TSA : suite native `engines`, tests 476 à 480 et 530 (`TSA_Tests`).

## 5. Cas à ajouter (avant toute extension du périmètre)

1. Charges sur barre en Timoshenko et moments répartis : deuxième code indépendant (OpenSees 3.8.0 ne les applique
   pas, ou n'a pas de commande).
2. Charges trapézoïdales (toute la longueur ou partielles) : version d'OpenSees qui les applique en 3D, ou deuxième
   code indépendant.
3. Audit indépendant des formulations des charges sur barre (condition du niveau 4).
4. Qualification dans TSA (niveau 5, [QUALIFICATION.md](QUALIFICATION.md)) : mode comparaison sur les projets
   d'exemple et les cas de `TSA/docs/validation`.
