# Historique des versions

Les versions sont celles de `kEngineVersion` (`include/Fem3D/Results/AnalysisResults.h`) et du projet CMake. Les
commits cités existent dans le dépôt ([traçabilité](traceability/GIT_TRACEABILITY.md)).

## Non publiée — documentation

- Licence MIT (`LICENSE`), [LICENSING.md](LICENSING.md), règles de travail (`AGENTS.md`, `CONTRIBUTING.md`).
- Livre technique, parcours pédagogique (niveaux 0 à 6), niveaux de maturité détaillés, traçabilité Git, diagrammes,
  référence API générée, guides (développeur, validation, étudiant et chercheur), site MkDocs et workflow.
- Outils `tools/docs/` (permaliens vérifiés, générateurs, contrôle des diagrammes). Aucun changement du code de calcul.

## 0.3.0 — non publiée (branche `feature/charges-sur-barre`)

- Charges sur barre : `PointLoad3D` (force ponctuelle), `DistributedLoad3D` sur [a, b] (`startPosition`,
  `endPosition` ; défaut : toute la longueur), `DistributedMoment3D` (moment réparti linéaire sur [a, b]) ;
  repère local ou global ; positions contrôlées (`Tolerance::relativeLoadPosition`).
- Forces nodales équivalentes exactes pour chaque type (Betti) ; moments répartis : travail sur la rotation de section
  (Timoshenko : Θ = v′ − γ). Efforts et déformée exacts, polynômes par morceaux (`PiecewisePolynomial`).
- Contrôle d'équilibre global étendu à ces charges.
- Tests : 8 nouveaux (analytiques, compatibilité exacte, invariance par subdivision, données invalides) ; 49 / 49.
- Benchmarks 21 (forces ponctuelles) et 22 (charges uniformes partielles) : Euler–Bernoulli OK (168 valeurs chacun,
  0 hors tolérance) ; Timoshenko non comparable (OpenSees 3.8.0 ignore ces charges sur `ElasticTimoshenkoBeam3d`).
  Résultats régénérés avec l'OpenSees 3.8.0 de conda-forge : écarts sur les cas existants ≤ 1,3e-12 en relatif
  (OpenSees) et ≤ 2,8e-13 (Fem3D, ordre des sommes), tous les statuts inchangés.
- TSA : le moteur `fem3d` calcule ces charges au lieu de les refuser (test 530 : égalité avec OpenSees).

## 0.2.0 — 2026-10-19

Commit [75700d2](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/commit/75700d2bfd33c29b2fadc795fd3051f25399ad19)
(premier commit du code ; fusion `bcbb438`).

- Appuis élastiques nodaux (`NodalSpring`), benchmarks 18 à 20.
- Critère de mécanisme indépendant des unités ; avertissement des quasi-mécanismes ; résidu de résolution.
- Wt du rectangle (polynôme de Roark), ν du coefficient de Cowper en paramètre.
- Champs JSON `springs`, `springForces`, `solutionResidual`, `warnings`.
- Post-traitement intégré au dépôt (`postprocessor/`).
- Intégration dans TSA comme moteur secondaire `fem3d` (commit TSA
  [2f5d7d4](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/commit/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d)).

## 0.1.0 — non versionnée dans Git

Développement antérieur au dépôt (non versionné) : poutres d'Euler–Bernoulli et de Timoshenko, charges nodales et
réparties, rotules My et Mz, format JSON, 17 premiers benchmarks. Son contenu est entièrement repris dans 75700d2 ;
son historique détaillé n'est pas reconstituable.
