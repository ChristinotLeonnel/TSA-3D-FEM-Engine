# fem3dpost — post-traitement des résultats « tsa-fem3d-results/1 »

Module C++20 indépendant du moteur : il lit uniquement le fichier JSON de résultats
(`../docs/RESULTS_FORMAT.md`), le valide, puis produit tableaux CSV, diagrammes
(CSV + SVG) et une note de calcul HTML (PDF en option).

- `fem3dpost` : bibliothèque statique (QtCore seul) — lecture validée, calculs dérivés, tableaux,
  tri/filtrage, CSV, SVG, note HTML.
- `fem3d_report` : outil en ligne de commande (QtGui uniquement pour l'export PDF, plateforme `offscreen`).
- `fem3dpost_tests` : tests (mini-cadre maison, sans dépendance), enregistrés dans CTest.

## Compiler

Prérequis : Visual Studio 2026 (MSVC, Ninja, CMake ≥ 3.22) et Qt 6 (`C:/Qt/6.11.2/msvc2022_64`,
modifiable par `-DFEM3DPOST_QT_DIR=…`).

```bat
build.cmd          :: configure + compile dans .\build (appelle vcvars64.bat)
build.cmd test     :: idem, puis ctest
```

## Exécuter

```bat
set PATH=C:\Qt\6.11.2\msvc2022_64\bin;%PATH%
set QT_PLUGIN_PATH=C:/Qt/6.11.2/msvc2022_64/plugins
build\fem3d_report.exe ..\examples\output\portique_spatial_resultats.json --out sortie --pdf
```

Options : `--out <dossier>` (obligatoire), `--case <id>` (un seul cas), `--pdf`,
`--opensees-summary <synthese.csv>` (intégrée telle quelle en section 14 ; sinon « non fournie »).
Codes de retour : 0 succès, 1 erreur de lecture/écriture, 2 erreur d'arguments ou cas inconnu.

## Tester

```bat
ctest --test-dir build --output-on-failure
```

`fem3dpost_tests` lit le fichier réel d'exemple (chemin réglable par `-DFEM3DPOST_EXAMPLE_JSON=…`) ;
`fem3d_report_exemple` génère la sortie complète avec PDF dans `build\sortie_exemple`.

## Sorties

| Fichier | Contenu |
|---|---|
| `deplacements.csv` | Nœud, Cas, Ux, Uy, Uz [m], Rx, Ry, Rz [rad] (repère global) |
| `reactions.csv` | Nœud, Cas, Fx, Fy, Fz [kN], Mx, My, Mz [kN.m] — nœuds d'appui seulement |
| `sollicitations.csv` | Élément, Cas, Position x, N, Vy, Vz, T, My, Mz (face positive, repère local) |
| `deformations_contraintes.csv` | flèche locale √(v²+w²) / corde, rotations de section, contrainte normale extrême, cisaillement moyen max, torsion, von Mises indicatif |
| `diagrammes/casK_elemE.csv` | toutes les grandeurs aux stations d'un élément pour un cas |
| `diagrammes/casK_elemE_<grandeur>.svg` | N, Vy, Vz, T, My, Mz, deformee, fleches, rotations |
| `note_de_calcul.html` / `.pdf` | note en 15 sections, SVG intégrés |

CSV : UTF-8 avec BOM (lecture directe par Excel), séparateur `;`, point décimal, unités dans les
en-têtes, nombres écrits en représentation aller-retour exacte. Une valeur `null` du JSON donne une
cellule vide (« — » dans la note), jamais 0.

## Limites

- Les diagrammes ne tracent que les stations fournies ; entre deux stations, interpolation linéaire
  (indiquée dans chaque légende). Aucune combinaison ni enveloppe de cas.
- La flèche aux stations est calculée par le post-traitement (écart à la corde) ; `maxDeflection` du
  fichier, évaluée plus finement par le moteur, est aussi reproduite.
- Contraintes : approximations du moteur (cisaillement moyen V/As, contrainte extrême majorante hors
  rectangle, von Mises indicatif) ; aucune vérification réglementaire.
- Le fichier ne décrit pas le détail des charges : la note se limite aux noms, identifiants et
  résultantes des cas.
- PDF : diagrammes rasterisés (greffon `qsvg` requis, police Arial imposée) ; fichier de quelques Mo.
