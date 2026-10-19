# Chapitre 18 — Intégration avec TSA et OpenSees

> **Objectifs**
>
> - Comprendre comment TSA transforme son modèle en modèle Fem3D, et les résultats de Fem3D en résultats TSA.
> - Connaître le périmètre de Fem3D dans TSA, la logique de refus, de secours et de comparaison avec OpenSees.
> - Savoir pourquoi OpenSees reste le moteur 3D par défaut.
>
> **Prérequis**
>
> - Architecture du moteur ([chapitre 17](17-architecture-cpp.md)).
> - Repères et conventions de signes ([chapitres 4](04-reperes.md) et [5](05-unites-signes.md)).

Document de référence : [INTEGRATION_TSA.md](../../INTEGRATION_TSA.md) ; diagramme de décision :
[selection-moteur-tsa.md](../../diagrams/selection-moteur-tsa.md). Les liens vers le code de TSA sont figés à la
révision `2f5d7d4` du dépôt TSA ; les fichiers cités n'ont pas changé depuis.

## 18.1 Un moteur secondaire derrière une interface commune

**Principe.** TSA calcule par des **moteurs** interchangeables qui implémentent la même interface
(`AnalysisEngine` : capacités, disponibilité, validation, calcul). Trois moteurs sont enregistrés, dans cet ordre :
OpenSees, Custom2D, puis Fem3D. Le premier enregistré est le moteur par défaut : **OpenSees reste le moteur 3D par
défaut** tant que Fem3D n'a pas atteint le niveau 5 de qualification
([QUALIFICATION.md](../../validation/QUALIFICATION.md)).

**Dépendance facultative.** TSA cherche ce dépôt dans `../3DDesplacement` (variable CMake `TSA_FEM3D_DIR`). S'il le
trouve, il compile la bibliothèque `fem3d` seule et définit `TSA_HAS_FEM3D` ; sinon il se construit sans Fem3D et le
signale à la configuration.

## 18.2 Conversion du modèle : réutiliser le chemin d'OpenSees

**Idée directrice.** Pour que Fem3D et OpenSees calculent **le même modèle**, l'adaptateur ne relit pas le projet TSA
à sa façon : il réutilise les briques qui préparent le script OpenSees (`OpenSeesModelMap`, motifs de charge,
charges sur barre en axes locaux). Les deux moteurs voient donc les mêmes barres, les mêmes axes, les mêmes appuis et
les mêmes charges.

| TSA | Fem3D | Remarque |
|---|---|---|
| Nœud | `addNode` (identifiant conservé) | |
| Poutre, poteau | `ElementDefinition`, **Euler–Bernoulli** | identifiant = tag OpenSees ; $v_{xz}$ = vecteur de `geomTransf` |
| Matériau | `ElasticMaterial` | $E$ : Pa → kN/m² ; $G = E / (2(1 + \nu))$ |
| Section | `BeamSection3D` | $A$, $I_y$, $I_z$, $J$ (pas d'aires de cisaillement) |
| Rotules My, Mz | `EndReleases` | relâchement de N, V ou T : refusé |
| Appuis | `NodeRestraint::custom` | y compris les blocages automatiques de TSA |
| Ressorts | `NodalSpring` | ressort sur un DDL bloqué : ignoré et signalé, car sans effet |
| Cas, combinaison, poids propre | **un seul** `LoadCase3D` | mêmes motifs et mêmes facteurs qu'OpenSees |
| Charge nodale | `NodalLoad3D` | une charge nodale déclarée « locale » est appliquée en axes globaux, avec une note |
| Charge sur barre uniforme ou linéaire sur toute la longueur | `DistributedLoad3D` locale | |

**Unités.** Le modèle converti est en kN et m, comme le script OpenSees de TSA ; Fem3D ne convertit rien lui-même
([chapitre 5](05-unites-signes.md)).

**Garde-fou sur les axes.** Les charges sur barre sont exprimées dans les axes de TSA. Si Fem3D construisait d'autres
axes locaux, elles seraient projetées sur de mauvaises directions. L'adaptateur crée donc chaque élément et compare
ses axes $x$ et $y$ à ceux de TSA (écart $> 10^{-9}$ : calcul refusé, avec le nom de la barre).

**Un calcul = un cas.** TSA transmet un cas, tous les cas superposés ou une combinaison, toujours comme un seul
`LoadCase3D`. Les combinaisons sont construites par TSA ([chapitre 10](10-charges-conditions-limites.md) §10.1).

## 18.3 Périmètre, refus et secours

**Périmètre** (niveau 4 de qualification) : statique linéaire 3D ; poutres et poteaux d'Euler–Bernoulli ; rotules My,
Mz ; appuis rigides et élastiques ; charges nodales, réparties uniformes ou linéaires sur toute la longueur, poids
propre. Les dalles non maillées sont exclues avec avertissement, comme pour OpenSees.

**Hors périmètre** (refusé, avec la raison) : charge ponctuelle sur barre, charge partielle, moment réparti, coques,
barres articulées (treillis, câbles). La non-linéarité et l'analyse modale sont refusées en amont par la validation
générique de TSA, à partir des capacités déclarées par le moteur.

**Deux réglages** (onglet d'options de Fem3D, enregistrés dans les réglages d'analyse du projet) :

| Réglage | Valeur | Effet |
|---|---|---|
| mode | `fem3d` (défaut) | Fem3D seul |
| | `compare` | Fem3D, puis OpenSees ; résultats affichés : Fem3D ; table de comparaison |
| secours | `none` (défaut) | hors périmètre : calcul refusé, raisons listées |
| | `opensees` | hors périmètre : calcul confié à OpenSees, **annoncé à la validation** |

**Secours jamais silencieux.** Quand OpenSees calcule à la place de Fem3D, les résultats portent l'identité
d'OpenSees (`AnalysisRunResult::producedBy`), et la raison figure dans les avertissements et le journal.

**Ce qui n'est jamais délégué.** Un **mécanisme** ou une **donnée invalide** n'est confié à aucun autre moteur : c'est
une propriété du modèle (appuis, rotules), pas une panne du moteur. Pour un mécanisme, le message le dit
explicitement ; une donnée refusée est simplement listée comme telle
([chapitre 13](13-conditions-limites-stabilite.md)).

## 18.4 Retour des résultats

| Résultat Fem3D | Résultat TSA |
|---|---|
| Déplacements nodaux | déplacements nodaux (6 DDL) |
| Réactions des appuis rigides **et** forces des ressorts | **sommées** par nœud en une réaction TSA, comme OpenSees reporte la réaction du nœud auxiliaire de chaque ressort sur son nœud |
| Efforts d'extrémité locaux (convention `localForce` d'OpenSees) | stations le long des barres, par **la même reconstruction** que pour OpenSees (`beamResultsFromEndForces`) |
| Avertissements (quasi-mécanisme, résidu) | avertissements d'exécution, préfixés « Fem3D » |

**Pourquoi reconstruire les stations dans TSA ?** Les efforts d'extrémité de Fem3D sont exacts. En les passant par la
même fonction que ceux d'OpenSees, TSA garantit que les deux moteurs **présentent** leurs diagrammes de la même
façon : une différence affichée ne peut venir que du calcul, pas de la présentation
([chapitre 15](15-efforts-internes.md)). L'équilibre global et le contrôle des blocages automatiques sont aussi faits
par les fonctions communes de TSA.

## 18.5 Le mode comparaison

**Principe.** Le même modèle est calculé par Fem3D puis par OpenSees ; seuls les résultats de Fem3D sont affichés,
sans aucune fusion. Une table « Comparaison Fem3D / OpenSees » donne, par groupe (déplacements et rotations ;
réactions, appuis et ressorts compris ; efforts d'extrémité), le nombre de valeurs comparées, le nombre hors
tolérance et les écarts maximaux.

**Tolérance** (`kCompareRelative`, `kCompareAbsoluteFactor`) :

$$
|a - b| \le \max\left(10^{-6}\,|b|,\ 10^{-9}\,s\right),
$$

où $b$ est la valeur d'OpenSees et $s$ une échelle de la grandeur (plus grande translation, plus grand effort,
plus grand moment du modèle ; pour les rotations et les moments, l'échelle est aussi déduite des translations et des
efforts par la plus grande longueur de barre). Le terme absolu évite de juger « en écart » des valeurs presque nulles.

**À quoi il sert.** C'est l'outil du **niveau 5** : calculer en mode comparaison tous les projets d'exemple et les cas
de `TSA/docs/validation` qui entrent dans le périmètre, et archiver les tables. Ce travail n'est **pas fait** à ce
jour ([QUALIFICATION.md](../../validation/QUALIFICATION.md) §4).

## 18.6 Non-régression dans TSA

- Les projets `.tsa` ne changent pas : le moteur et ses réglages sont des réglages d'analyse existants.
- Les suites de TSA restent inchangées, sauf le test 130, qui compte désormais 3 moteurs.
- Tests 476 à 480 : enregistrement et capacités ; portique spatial (rotules, ressort, charges uniformes et
  trapézoïdales, poids propre) calculé par Fem3D et par OpenSees, sans écart, stations comprises ; refus hors
  périmètre puis secours étiqueté ; mécanisme sans secours ; options.

## Limites

- **Timoshenko non exposé dans TSA** : les aires de cisaillement des sections de TSA ne sont pas transmises, et le
  moteur OpenSees de TSA utilise `elasticBeamColumn` ; la comparaison n'aurait pas de référence
  ([feuille de route](../../ROADMAP.md), étape 4).
- Les contraintes de Fem3D ne sont pas reprises : TSA calcule ses propres contraintes à partir des efforts.
- Niveau 5 non atteint ; Fem3D n'est pas le moteur par défaut.
- Un incident est ouvert sur l'exécutable de tests de TSA lancé directement en mode `offscreen`
  ([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §6).

## À retenir

- Fem3D est un moteur secondaire de TSA ; OpenSees reste le moteur par défaut jusqu'au niveau 5.
- La conversion réutilise le chemin d'OpenSees : même modèle, mêmes axes (contrôlés), mêmes charges.
- Hors périmètre : refus motivé, ou secours OpenSees choisi, annoncé et étiqueté ; un mécanisme n'est jamais délégué.
- Le mode comparaison mesure l'accord Fem3D / OpenSees sur les modèles réels : c'est la voie du niveau 5.

## Dans le code source (dépôt TSA)

- Capacités : [`Fem3DEngine::capabilities`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DEngine.cpp#L71-L85) ; validation et annonce du secours :
  [`Fem3DEngine::validate`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DEngine.cpp#L94-L127).
- Calcul, secours, mécanisme, comparaison : [`Fem3DEngine::run`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DEngine.cpp#L129-L216),
  [`compareResults`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DEngine.cpp#L223-L330), tolérance :
  [`kCompareRelative`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DEngine.h#L40-L40).
- Conversion : [`convert`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DAdapter.cpp#L38-L189) ; résultats : [`mapResults`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DAdapter.cpp#L191-L262).
- Reconstruction commune des stations : [`beamResultsFromEndForces`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/OpenSeesResultsReader.cpp#L909-L916).
- Enregistrement : [`BuiltInEngines.cpp`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/BuiltInEngines.cpp#L9-L38) ; options :
  [`Fem3DOptionsWidget.cpp`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/UI/Analysis/Fem3DOptionsWidget.cpp).

## Vérifier par les tests

| Test | Vérification |
|---|---|
| [Tests 476 à 480](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/tests/test_analysis_engines.cpp#L799-L817) | enregistrement, comparaison sans écart, refus et secours étiqueté, mécanisme sans secours, options |
| Test 130 | nombre de moteurs enregistrés |

Lancer dans TSA : `ctest --test-dir build-ninja-release --output-on-failure` (dossier de construction de TSA).

## Références

- Documentation d'OpenSees : `elasticBeamColumn`, `geomTransf`, `zeroLength`, `eleLoad` — vérifié (contenu),
  [REFERENCES.md](../../references/REFERENCES.md) §C4, §C7, §C9.
- [INTEGRATION_TSA.md](../../INTEGRATION_TSA.md), [QUALIFICATION.md](../../validation/QUALIFICATION.md).

---

Précédent : [Chapitre 17 — Architecture logicielle C++](17-architecture-cpp.md) · Suivant :
[Chapitre 19 — Validation scientifique et reproductibilité](19-validation-reproductibilite.md)
