# Guide développeur

Pour contribuer au moteur, à ses tests ou à sa documentation. Règles : [AGENTS.md](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/main/AGENTS.md) ;
procédure : [CONTRIBUTING.md](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/main/CONTRIBUTING.md).

## 1. Construire et tester

Prérequis : CMake ≥ 3.21, compilateur C++20 (MSVC 2022 ou plus récent, GCC, Clang), Eigen 3.4 (pris dans
`../TSA/thirdparty/eigen-3.4.1/include/eigen3`, sinon `-DFEM3D_EIGEN_DIR=<dossier contenant Eigen/>`).

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\fem3d_tests.exe                       :: 41 tests
build\fem3d_opensees_benchmarks.exe         :: 20 cas × 2 formulations (OpenSees 3.8.0 requis)
build\fem3d_example.exe sortie.json         :: portique spatial → JSON
```

Post-traitement (Qt 6, variable `FEM3DPOST_QT_DIR`) : `postprocessor\build.cmd test`.

Dans TSA, le moteur est compilé comme sous-projet (`add_subdirectory`, `FEM3D_BUILD_TESTS=OFF`) s'il est présent dans
`../3DDesplacement` ([INTEGRATION_TSA.md](../INTEGRATION_TSA.md)).

## 2. Organisation du dépôt

| Dossier | Contenu |
|---|---|
| `include/Fem3D/`, `src/` | moteur, un dossier par module ([architecture](../diagrams/architecture.md), [dépendances](../diagrams/dependances-code.md)) |
| `tests/` | `unit/`, `analytical/`, `integration/` ; mini-cadre `TestFramework.h` |
| `benchmarks/OpenSees/` | programme de comparaison, résultats versionnés |
| `examples/` | exemple d'utilisation et sa sortie JSON |
| `postprocessor/` | lecture du JSON, tableaux, diagrammes, note de calcul |
| `docs/` | documentation (source du site) |
| `tools/docs/` | outils de documentation : permaliens, traçabilité, API, dépendances, diagrammes |

## 3. Conventions de code

Relevées dans le code existant :

- C++20, espace de noms `TSA::Fem3D`, aucun état global, aucune dépendance à Qt dans le moteur.
- Types Eigen fixes pour les éléments (`Matrix12`, `Vector12`), matrices creuses pour le système global.
- Erreurs : `Fem3DError` avec un code stable (`ErrorCode`) et un message qui désigne l'objet fautif ; jamais de
  valeur inventée pour masquer une erreur.
- Commentaires en français ; un commentaire d'en-tête par fichier (repris dans la [référence API](../reference/API.md)) ;
  `///` pour documenter une déclaration publique, `///<` en fin de ligne pour un champ.
- Tolérances numériques regroupées dans `Tolerance` (`include/Fem3D/Core/Types.h`).

## 4. Ajouter une formulation d'élément

1. Écrire la théorie (hypothèses, matrice, fonctions de forme) dans `docs/theory/THEORY.md`, avec ses références et
   leur statut ; la faire relire.
2. Dériver une classe de `BeamElementBase` (interface `IBeamElement3D`) : `localStiffnessMatrix`, fonctions de forme,
   flexibilités de cisaillement ; l'ajouter à la fabrique `StructuralModel3D::createElement`.
3. Tests analytiques (écart ≤ 1e-9) : symétrie, modes rigides, consoles, cas limites (comme
   `Element_TimoshenkoTendsToEulerBernoulli`).
4. Benchmarks OpenSees si un élément équivalent existe ; sinon, référence analytique documentée.
5. Mettre à jour [FORMULATIONS.md](../formulations/FORMULATIONS.md), [VALIDATION_PLAN.md](../validation/VALIDATION_PLAN.md),
   [MATURITY_LEVELS.md](../validation/MATURITY_LEVELS.md) (niveau atteint, avec preuves).

## 5. Ajouter un type de charge

Prochaine étape de la feuille de route (charges ponctuelles, partielles, moments répartis) :

1. Données dans `include/Fem3D/Loads/Loads3D.h`, contrôles dans `StructuralModel3D::validate`.
2. Forces nodales équivalentes exactes (intégration par morceaux) dans `BeamResponse` et l'assemblage, y compris la
   condensation pour les barres à rotules.
3. Efforts internes et déformée exacts par morceaux (discontinuités aux points d'application).
4. Contrôle d'équilibre : résultante et moment exacts de la nouvelle charge.
5. Export JSON, post-traitement, et retrait du refus correspondant dans l'adaptateur TSA.
6. Tests analytiques, benchmark `-beamPoint` d'OpenSees pour la charge ponctuelle.

## 6. Stratégie de test

| Niveau | Où | Exigence |
|---|---|---|
| Unitaire | `tests/unit/` | propriétés de base, refus des données invalides |
| Analytique | `tests/analytical/` | solution exacte, écart relatif ≤ 1e-9, formule écrite dans le test |
| Intégration | `tests/integration/` | invariances (orientation, maillage, unités), équilibre, format JSON |
| Référence | `benchmarks/OpenSees/` | tolérances fixées avant les essais |
| TSA | dépôt TSA, suite `engines` | comparaison Fem3D / OpenSees sur des modèles TSA |

Un test qui échoue n'est jamais affaibli : on corrige le code, ou le test s'il est faux, avec une justification écrite.

## 7. Documentation

| Outil | Rôle |
|---|---|
| `tools/docs/gitlinks.py permalink <fichier> "<définition>"` | lien GitHub à un hash complet, lignes de la définition |
| `tools/docs/gitlinks.py check` | vérifie tous les liens GitHub (commit, fichier, lignes) |
| `tools/docs/gen_traceability.py` | matrice de traçabilité ([GIT_TRACEABILITY.md](../traceability/GIT_TRACEABILITY.md)) |
| `tools/docs/gen_api_reference.py` | [référence API](../reference/API.md) depuis les en-têtes |
| `tools/docs/gen_dependency_graph.py` | [graphe des dépendances](../diagrams/dependances-code.md) depuis les `#include` |
| `tools/docs/check_mermaid.py` | rendu de tous les diagrammes Mermaid (mermaid-cli) |
| `mkdocs build --strict` | construction du site, liens internes compris |

Les générateurs ont une option `--check` qui échoue si le fichier versionné n'est plus à jour (utilisée par la CI).
Quand le code change, mettre à jour la révision de référence (`REFERENCE_COMMITS` dans `gitlinks.py`) après le commit,
puis régénérer.

## 8. Site documentaire et publication

- Construction locale : `pip install -r docs/requirements.txt`, puis `mkdocs serve` (aperçu) ou
  `mkdocs build --strict` (dossier `site/`, non versionné).
- CI : **pas encore configurée** (vérifié le 2026-10-19) : le workflow `.github/workflows/docs.yml`, qui doit
  construire et vérifier le site à chaque push et pull request, est à créer ; la publication ci-dessous en dépend.
- **Publication actuelle** (depuis le 2026-10-19) : le site est construit en mode strict et publié par le site TSA
  Web (dépôt `Tsaraloha-Web`, workflow `deploy.yml`) à l'adresse
  `https://christinotleonnel.github.io/Tsaraloha-Web/fem3d/`, depuis la branche `main` de ce dépôt, à chaque
  déploiement de TSA Web. La variable `FEM3D_SITE_URL` y fixe l'adresse (`site_url` de `mkdocs.yml`).
- **Publication** (GitHub Pages) : jamais automatique. Prérequis à vérifier par le propriétaire du dépôt :
  1. *Settings → Pages → Build and deployment → Source : GitHub Actions* ;
  2. vérifier qu'aucun secret ni fichier personnel n'est dans `docs/` (`git grep -i -E "token|password|secret"`) ;
  3. lancer le workflow *Documentation* manuellement (*Actions → Documentation → Run workflow*, option `publish`).
  L'adresse du site sera `https://christinotleonnel.github.io/TSA-3D-FEM-Engine/`.

## 9. Git

Branche par sujet, fusion dans `main` après vérification, aucun push forcé ni réécriture de l'historique publié. Les
messages de commit disent ce qui a été vérifié et comment.
