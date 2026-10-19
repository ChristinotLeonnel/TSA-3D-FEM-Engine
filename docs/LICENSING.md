# Licences et dépendances

Ce document décrit les conditions qui s'appliquent à chaque partie du dépôt. Il ne constitue pas un avis juridique :
les points incertains sont signalés comme tels.

## 1. Code original de Fem3D : licence MIT

Le code écrit pour ce projet est distribué sous la **licence MIT** (fichier [LICENSE](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/main/LICENSE)),
copyright © 2026 Tsaraloha Christinot. Elle couvre :

| Partie | Chemins |
|---|---|
| Moteur | `include/Fem3D/`, `src/` |
| Tests et exemples | `tests/`, `examples/` |
| Programme de comparaison OpenSees | `benchmarks/OpenSees/opensees_benchmarks.cpp`, `benchmarks/OpenSees/README.md` |
| Post-traitement | `postprocessor/` (sources) |
| Outils de documentation | `tools/docs/` |
| Documentation | `docs/`, `README.md`, `AGENTS.md`, `CONTRIBUTING.md`, `mkdocs.yml` |

Aucun fichier de code tiers n'est copié dans ces dossiers (vérifié le 2026-10-19 par revue des fichiers versionnés :
pas d'en-tête de licence étrangère, pas de bibliothèque embarquée).

## 2. Dépendances (non incluses dans le dépôt)

| Composant | Utilisé par | Licence | Vérification | Obligations |
|---|---|---|---|---|
| **Eigen 3.4** (en-têtes) | moteur : `Core`, `Dense`, `Geometry`, `Cholesky`, `Eigenvalues`, `SparseCore`, `SparseCholesky` | **MPL 2.0** | En-tête de `SimplicialCholesky_impl.h` lu : MPL 2.0, avec l'autorisation de T. A. Davis (auteur de LDL) de distribuer le code dérivé sous MPL 2.0 | Eigen n'est pas copié ici : il est pris dans `../TSA/thirdparty` ou via `FEM3D_EIGEN_DIR`. Qui redistribue Eigen doit en conserver la licence et les notices ; le code MIT de Fem3D peut l'utiliser tel quel. |
| **Qt 6** (Core, Gui) | post-traitement seulement | LGPLv3 pour l'essentiel en open source (aussi GPL et commerciale) | Fichier `Licenses/LICENSE` de l'installation de Qt 6.11.2 (type « Opensource ») | Lien dynamique ; un binaire distribué du post-traitement doit respecter la LGPLv3 (permettre le remplacement des bibliothèques Qt, fournir les notices). Le moteur n'utilise pas Qt. |
| **OpenSees 3.8.0** | validation seulement (exécutable externe) | Licence propre de l'Université de Californie : reproduction et modification pour usage non commercial ou interne | Fichier `COPYRIGHT` d'OpenSees, lu par l'agent bibliographique (voir [references/REFERENCES.md](references/REFERENCES.md) §D) | OpenSees n'est ni copié ni distribué ; aucun extrait de son code n'est dans Fem3D. Voir §3. |
| **MkDocs**, **Material for MkDocs** | génération du site | BSD-2-Clause (MkDocs), MIT (Material) | Métadonnées des paquets installés | Outils de construction, non distribués avec le moteur. |
| **Mermaid** (rendu des diagrammes) | site documentaire (chargé par le navigateur depuis un CDN par Material) | MIT selon le projet mermaid-js | **Non vérifié localement** (bibliothèque non installée) | Aucune connue. |

## 3. Données générées et points incertains

- **Résultats d'OpenSees versionnés** (`benchmarks/OpenSees/results/*_opensees.txt`, CSV de comparaison) : ce sont
  des valeurs numériques produites en exécutant OpenSees, sans code d'OpenSees. Leur statut au regard de la licence
  d'OpenSees n'est pas établi formellement ; ils sont publiés comme pièces de validation. **Point à confirmer** si une
  diffusion commerciale est envisagée.
- **Scripts Tcl générés** (`benchmarks/OpenSees/cases/`) : non versionnés (ils contiennent des chemins locaux).
- **Documents téléchargés** pour la recherche bibliographique (`docs/references/_downloads/`) : jamais versionnés ;
  ils restent sous la licence de leurs auteurs.
- **Ouvrages et articles cités** : seules leurs références sont données ; aucun passage n'est reproduit.
- **TSA** (dépôt voisin, qui intègre Fem3D) a ses propres conditions ; voir ce dépôt.

## 4. Titulaire et contributions

Le titulaire indiqué est l'auteur des commits du dépôt. Les contributions sont acceptées sous la même licence MIT
([CONTRIBUTING.md](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/main/CONTRIBUTING.md)).
