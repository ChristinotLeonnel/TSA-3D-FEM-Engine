# Historique des versions

Les versions sont celles de `kEngineVersion` (`include/Fem3D/Results/AnalysisResults.h`) et du projet CMake. Les
commits cités existent dans le dépôt ([traçabilité](traceability/GIT_TRACEABILITY.md)).

## Non publiée — documentation

- Licence MIT (`LICENSE`), [LICENSING.md](LICENSING.md), règles de travail (`AGENTS.md`, `CONTRIBUTING.md`).
- Livre technique, parcours pédagogique (niveaux 0 à 6), niveaux de maturité détaillés, traçabilité Git, diagrammes,
  référence API générée, guides (développeur, validation, étudiant et chercheur), site MkDocs et workflow.
- Outils `tools/docs/` (permaliens vérifiés, générateurs, contrôle des diagrammes). Aucun changement du code de calcul.

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
