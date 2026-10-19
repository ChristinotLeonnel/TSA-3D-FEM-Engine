# Contribuer à TSA-3D-FEM-Engine

Les règles de fond sont dans [AGENTS.md](AGENTS.md) (vérité technique, tests de référence, traçabilité, publication).
Ce document donne la procédure. Les contributions sont acceptées sous la licence MIT du dépôt ([LICENSE](LICENSE)).

## 1. Avant de commencer

- Lire [docs/developer/DEVELOPER_GUIDE.md](docs/developer/DEVELOPER_GUIDE.md) (construction, architecture, ajout
  d'une fonctionnalité) et [docs/limitations/KNOWN_LIMITATIONS.md](docs/limitations/KNOWN_LIMITATIONS.md).
- Créer une branche à partir de `main`.

## 2. Modifier le moteur

1. Écrire d'abord le test analytique (tolérance relative 1e-9 pour une solution exacte), puis le code.
2. Ajouter un benchmark OpenSees si une référence OpenSees existe ; sinon, écrire pourquoi dans
   [docs/validation/VALIDATION_PLAN.md](docs/validation/VALIDATION_PLAN.md).
3. Vérifier :

   ```bat
   cmake --build build
   build\fem3d_tests.exe
   build\fem3d_opensees_benchmarks.exe
   postprocessor\build.cmd test
   ```

4. Mettre à jour la théorie (`docs/theory/`), les formulations, les limites, le plan de validation et, si le format
   change, `docs/RESULTS_FORMAT.md`.

## 3. Modifier la documentation

1. Vérifier dans le code chaque affirmation ; marquer « à vérifier » ce qui ne peut pas l'être.
2. Liens vers le code : `python tools/docs/gitlinks.py permalink <fichier> "<définition>"` (hash complet).
3. Vérifier :

   ```bat
   python tools/docs/gitlinks.py check
   python tools/docs/gen_dependency_graph.py --check
   python tools/docs/gen_api_reference.py --check
   mkdocs build --strict
   ```

## 4. Commit et revue

- Un commit = un changement cohérent, avec un message qui dit quoi et pourquoi ; les résultats de tests cités dans le
  message ont réellement été obtenus.
- La revue relit le diff complet ; le rapport d'un outil ou d'un agent n'est pas une preuve.
- Pas de push forcé ni de réécriture de l'historique publié.
