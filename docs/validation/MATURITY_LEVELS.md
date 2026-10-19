# Niveaux de maturité du moteur

## 1. Origine et portée de cette définition

**Audit du 2026-10-19.** Les occurrences de « niveau », « level », « maturity » et « qualification » ont été
recherchées dans le code, les documents, les commits et les branches des deux dépôts :

- une seule définition de niveaux de maturité existe : les **niveaux 1 à 5** de [QUALIFICATION.md](QUALIFICATION.md),
  introduits par le commit
  [75700d2](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/commit/75700d2bfd33c29b2fadc795fd3051f25399ad19)
  (ils datent de la mission d'intégration à TSA, qui demandait une « qualification par niveaux 1 à 5 ») ;
- ils qualifient **chaque fonction du moteur** (par exemple « rotules My, Mz »), puis le moteur sur un périmètre ;
- les autres usages du mot dans TSA sont sans rapport : niveaux de bâtiment (`LevelManager`), niveau d'extraction des
  résultats OpenSees (`ExtractionLevel`).

Ce document **conserve** ces cinq niveaux et leurs exigences. Il les **précise** (critères logiciels, tests et
benchmarks obligatoires, conditions d'entrée et de sortie, preuves), car la définition d'origine tenait en une ligne
par niveau. Ces précisions sont marquées « précision 2026-10-19 » ; elles ne changent ni l'ordre ni le sens des
niveaux. Aucun niveau nouveau n'est créé.

**À ne pas confondre** avec les niveaux pédagogiques 0 à 6 du [parcours d'apprentissage](../learning/index.md), qui
organisent l'enseignement et ne disent rien de la maturité du moteur.

## 2. Définitions

### Niveau 1 — Implémenté

| Rubrique | Contenu |
|---|---|
| Définition | Le code existe et refuse les données invalides avec un message qui désigne l'objet fautif. |
| Fonctionnalités requises | La fonction est accessible par l'API publique (`include/Fem3D/`). |
| Critères scientifiques | Hypothèses écrites dans [THEORY.md](../theory/THEORY.md). |
| Critères logiciels (précision 2026-10-19) | Compilation sans avertissement (/W4) ; erreurs par `Fem3DError` avec un code stable. |
| Tests obligatoires | Tests unitaires : données invalides refusées, propriétés élémentaires (symétrie, modes rigides). |
| Benchmarks obligatoires | Aucun. |
| Limites acceptables | Toutes, si elles sont écrites dans [KNOWN_LIMITATIONS.md](../limitations/KNOWN_LIMITATIONS.md). |
| Preuves exigées | Noms des tests ; ligne de la théorie. |
| Entrée / sortie | Entrée : code fusionné. Sortie vers 2 : solutions analytiques disponibles. |

### Niveau 2 — Vérifié

| Rubrique | Contenu |
|---|---|
| Définition | Les résultats égalent des solutions analytiques exactes. |
| Critères scientifiques | Écart relatif ≤ 1e-9 (éléments exacts : seul l'arrondi subsiste). Solutions dérivées ou re-dérivées indépendamment. |
| Critères logiciels (précision) | Tests automatisés dans `fem3d_tests`, exécutés par CTest. |
| Tests obligatoires | Au moins un cas analytique par effet (traction, torsion, flexion dans chaque plan, effet propre à la fonction). |
| Benchmarks obligatoires | Aucun. |
| Preuves exigées | Tests nommés, valeurs attendues écrites dans le test avec leur formule. |
| Entrée / sortie | Entrée : niveau 1. Sortie vers 3 : une référence indépendante est disponible. |

### Niveau 3 — Validé

| Rubrique | Contenu |
|---|---|
| Définition | Les résultats égalent une référence indépendante sur des cas élémentaires. |
| Critères scientifiques | \|écart\| ≤ max(1e-6·\|réf\|, 1e-9·échelle), fixé avant les essais ([VALIDATION_PLAN.md](VALIDATION_PLAN.md) §2). |
| Critères logiciels (précision) | Comparaison automatisée et reproductible (`fem3d_opensees_benchmarks`) ; références régénérées, jamais retouchées. |
| Benchmarks obligatoires | Au moins un cas OpenSees par effet de la fonction, dans les deux formulations si elles sont concernées. |
| Limites acceptables | Cas « non comparable » si la référence n'existe pas, avec la raison écrite. |
| Preuves exigées | Cas, fichiers CSV, synthèse générée. |
| Entrée / sortie | Entrée : niveau 2. Sortie vers 4 : modèles représentatifs et audit. |

### Niveau 4 — Validé sur modèles représentatifs

| Rubrique | Contenu |
|---|---|
| Définition | Niveau 3 sur des modèles multi-éléments, et audit indépendant des formulations. |
| Critères scientifiques | Portiques spatiaux, orientations quelconques, sections et charges combinées ; aucun écart. |
| Critères logiciels (précision) | Invariances testées (orientation, maillage, unités) ; diagnostics (mécanismes) testés. |
| Benchmarks obligatoires | Cas multi-éléments (08 à 10, 14, 20 pour les fonctions actuelles). |
| Preuves exigées | Benchmarks ; rapport d'audit indépendant avec suites données. |
| Entrée / sortie | Entrée : niveau 3. Sortie vers 5 : qualification dans TSA. |

### Niveau 5 — Qualifié dans TSA

| Rubrique | Contenu |
|---|---|
| Définition | Comparaison systématique Fem3D / OpenSees, dans TSA, sur les modèles réels, sans aucun écart ; relecture humaine de la théorie. |
| Critères scientifiques | Mode comparaison de TSA, mêmes tolérances, sur les projets d'exemple et les cas de `TSA/docs/validation` qui entrent dans le périmètre. |
| Critères logiciels (précision) | Tables de comparaison archivées ; tests de non-régression de TSA verts. |
| Preuves exigées | Rapport de qualification listant chaque modèle, sa table de comparaison et son résultat ; trace de la relecture. |
| Conséquence | Seul ce niveau autorise à faire de Fem3D le moteur par défaut de TSA, sur son périmètre. |

## 3. Statut actuel (vérifié le 2026-10-19)

| Fonction | Niveau atteint | Ce qui manque pour le niveau suivant |
|---|---|---|
| Euler–Bernoulli, charges nodales, charges uniformes, rotules My/Mz, ressorts, mécanismes | **4** | Niveau 5 : comparaison systématique dans TSA, relecture humaine. |
| Timoshenko | **4** dans Fem3D ; non exposé dans TSA | Exposer la formulation dans TSA (aires de cisaillement), puis niveau 5. |
| Charges linéaires (trapézoïdales) sur toute la longueur | **2**, plus une comparaison indirecte à OpenSees dans TSA (test 477) | Niveau 3 : référence directe (OpenSees plus récent ou second code). |
| Rotules de Timoshenko hors axes globaux, déplacements imposés, contraintes | **2** | Niveau 3 : référence indépendante. |
| Charges ponctuelles, partielles, moments répartis, treillis | **non implémenté** (niveau 0) | Niveau 1. |
| **Moteur dans TSA** | **4 sur le périmètre qualifié** ; **niveau 5 non atteint** | Voir la matrice ci-dessous. |

Le niveau 5 n'est **pas** déclaré atteint parce que les tests passent : deux de ses trois critères n'ont aucune preuve.

## 4. Traçabilité des critères

La matrice critère → preuves, avec les liens vers le code et les tests à une révision figée, est dans
[GIT_TRACEABILITY.md](../traceability/GIT_TRACEABILITY.md) §3. Les verdicts par fonction sont dans
[VALIDATION_PLAN.md](VALIDATION_PLAN.md) §3, la décision dans [QUALIFICATION.md](QUALIFICATION.md).
