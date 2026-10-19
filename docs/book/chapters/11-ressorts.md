# Chapitre 11 — Ressorts d'appui

> **Objectifs**
>
> - Modéliser un appui élastique (pied semi-rigide, appui sur sol ou sur élastomère) par des ressorts nodaux.
> - Savoir comment un ressort entre dans la matrice de rigidité et comment sa force est publiée.
> - Connaître les règles de validité et la façon dont les ressorts ont été validés contre OpenSees.
>
> **Prérequis**
>
> - Appuis rigides et déplacements imposés ([chapitre 10](10-charges-conditions-limites.md) §10.5).
> - Énergie de déformation d'un ressort linéaire, $\tfrac12 k u^2$.

## 11.1 Modèle physique

**Intuition.** Un appui réel n'est jamais parfaitement rigide : une semelle tourne un peu sur le sol, un appareil
d'appui en élastomère se déforme. Un ressort relie le nœud à un point fixe et oppose une force proportionnelle au
déplacement.

**Symboles.** `NodalSpring` : six raideurs pour un nœud,

- $k_X, k_Y, k_Z$ en translation [F/L] ;
- $k_{RX}, k_{RY}, k_{RZ}$ en rotation [F·L/rad].

**Hypothèses** ([THEORY.md](../../theory/THEORY.md) §7) :

- ressorts **linéaires** et **découplés** : chaque raideur n'agit que sur son DDL ;
- orientés selon les **axes globaux** ;
- reliés à un point fixe (le sol), pas à un autre nœud ;
- une raideur nulle signifie « pas de ressort sur ce DDL ».

## 11.2 Contribution à la matrice de rigidité

**Équation.** L'énergie d'un ressort sur le DDL global $g$ vaut $\tfrac12 k\,u_g^2$. Sa dérivée seconde ajoute
simplement $k$ au terme diagonal :

$$
K_{gg} \leftarrow K_{gg} + k .
$$

**Interprétation.** Le ressort est en **parallèle** avec la structure sur ce DDL : leurs raideurs s'additionnent.
Comme un ressort n'est autorisé que sur un DDL **libre** (§11.5), sa raideur entre toujours dans $K_{ff}$, jamais dans
les blocs liés aux DDL imposés ([chapitre 12](12-assemblage.md)).

**Algorithme** (`GlobalAssembler::assembleStiffness`, après la boucle des éléments) : pour chaque nœud muni de
ressorts et chaque DDL de raideur non nulle, ajouter le triplet $(p, p, k)$ à $K_{ff}$, où $p$ est l'indice libre du
DDL, et $(g, g, k)$ à la matrice complète. Les doublons sont sommés par `setFromTriplets`.

## 11.3 Force du ressort, réactions et équilibre

**Équation.** Force (ou moment) exercée **par le ressort sur la structure**, repère global :

$$
F_\text{ressort} = -k\,u .
$$

**Publication.** Ces forces ne sont **pas** mélangées aux réactions des appuis rigides : elles sont publiées à part
(`LoadCaseResults::springs`, clé `springForces` du JSON, avec la convention de signe écrite dans le fichier). Le
contrôle d'équilibre les compte avec les réactions :

$$
\sum F_\text{appliquées} + \sum R + \sum F_\text{ressorts} = 0 .
$$

**Interprétation.** Sur un nœud qui a à la fois des DDL bloqués et des ressorts, la réaction rigide est nulle sur
les DDL retenus par ressort, et la force du ressort est nulle sur les DDL sans ressort. Chaque DDL n'a qu'un seul
mode d'appui, ce qui rend la répartition des efforts univoque.

## 11.4 Exemples vérifiables

Section rectangulaire 0,30 × 0,50 m, $E = 210$ GPa, d'où $E I_y = 656\,250$ kN·m² ; Euler–Bernoulli. Valeurs
recalculées en Python ; chaque test vérifie aussi la variante Timoshenko (souplesse $L/(G A_s)$ en plus).

**Ressort vertical en bout de console.** $L = 4$ m, $k = 5000$ kN/m, $F_Z = -20$ kN. La console a une raideur en bout
$3EI/L^3 = 30\,761{,}7$ kN/m ; console et ressort sont en parallèle :

$$
\delta = \frac{F}{3EI/L^3 + k} = -5{,}593 \cdot 10^{-4}\ \text{m},
\qquad
F_\text{ressort} = -k\,\delta = 2{,}796\ \text{kN},
\qquad
R_Z = -F - F_\text{ressort} = 17{,}204\ \text{kN}.
$$

Test `Springs_TipSpringInParallelWithCantilever`.

**Appui articulé retenu par un ressort de rotation.** Console horizontale selon X de $L = 3$ m, dont le nœud 1 est
bloqué sauf Ry, retenu par $k_\theta = 2 \cdot 10^4$ kN·m/rad ; $F_Z = -20$ kN (transversale) au nœud 2 :

$$
\delta = F \left( \frac{L^3}{3EI} + \frac{L^2}{k_\theta} \right) = -9{,}274 \cdot 10^{-3}\ \text{m},
\qquad
\theta_y = -\frac{F L}{k_\theta} = 3 \cdot 10^{-3}\ \text{rad},
\qquad
M_\text{ressort} = F L = -60\ \text{kN·m}.
$$

Ici, **97 %** de la flèche vient de la rotation de l'appui : la souplesse de l'appui domine celle de la barre. C'est
pourquoi un encastrement supposé parfait peut fausser beaucoup les déplacements. Test
`Springs_RotationalSpringAtBase`.

**Structure posée uniquement sur des ressorts.** Aucun appui rigide : la console repose sur un nœud muni de six
ressorts. Les translations ($u_1 = F_X/k_X$), la rotation du nœud élastique et la souplesse de la barre
s'additionnent ; l'équilibre global est assuré par les seuls ressorts. Test `Springs_StructureSupportedOnlyBySprings`.

## 11.5 Règles et refus

| Situation | Comportement | Raison |
|---|---|---|
| Raideur négative ou non finie | refus `InvalidInput` | énergie négative, matrice non définie positive |
| Ressort non nul sur un DDL bloqué | refus `InvalidInput` | la réaction se répartirait de façon ambiguë entre appui et ressort |
| Ressort sur un nœud inconnu | refus `UnknownReference` | référence invalide |
| Raideurs nulles sur un nœud encastré | accepté | aucun ressort effectif |
| DDL retenu seulement par une raideur nulle | mécanisme, refus `Mechanism` | une raideur nulle ne retient rien |
| Deuxième `setSpring` sur le même nœud | remplace le précédent | pas de cumul implicite |

Tests `Springs_InvalidInputRejected` et `Springs_ZeroStiffnessDoesNotHideMechanism`.

**Ressort très faible ou très fort.** Un ressort très faible devant la rigidité de la structure peut produire un
**quasi-mécanisme** : le calcul se poursuit avec un avertissement de conditionnement
([chapitre 14](14-solveur-diagnostics.md) §14.4). Pour représenter un appui **rigide**, on bloque le DDL plutôt que
d'y mettre une raideur énorme : l'élimination des DDL imposés est exacte et ne dégrade pas le conditionnement
([chapitre 13](13-conditions-limites-stabilite.md) §13.1).

**Choisir la raideur.** Fem3D ne calcule pas la raideur d'un sol ni d'un appareil d'appui : c'est une donnée de
l'utilisateur, à justifier par ailleurs.

## 11.6 Validation contre OpenSees

**Modèle de référence.** Dans les scripts générés, chaque nœud à ressorts reçoit un nœud fixe doublé (numéro
200000 + nœud), relié par un élément `zeroLength` dont l'orientation par défaut suit les axes globaux, avec un
matériau `Elastic` par DDL retenu. La force du ressort sur la structure est lue comme la réaction du nœud fixe
(`nodeReaction`) et comparée à $-k\,u$ de Fem3D.

**Cas et résultats** ([VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md)) :

| Cas | Modèle |
|---|---|
| 18 | console dont le bout repose sur deux ressorts ($k_Y$, $k_Z$), charges nodales et répartie |
| 19 | poteau à pied articulé retenu par trois ressorts de rotation, charges spatiales |
| 20 | portique spatial : trois pieds articulés avec ressorts de rotation, un pied sans appui rigide sur six ressorts |

Pour les deux formulations, les fichiers de comparaison de ces trois cas contiennent 528 valeurs (déplacements,
réactions, 72 forces de ressort, efforts d'extrémité), toutes dans les tolérances fixées avant les essais.

## Limites

- Pas de ressort **couplé** (matrice 6 × 6 pleine), ni en **repère incliné**, ni **unilatéral** (appui qui se
  soulève), ni non linéaire.
- Pas de ressort **entre deux nœuds**, ni de rotule **semi-rigide** en extrémité de barre
  ([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §2).
- Dans TSA, un ressort défini sur un DDL bloqué est ignoré par l'intégration et signalé, car il n'aurait aucun effet
  ([chapitre 18](18-integration-tsa-opensees.md)).

## À retenir

- Un ressort ajoute $k$ sur la diagonale de $K_{ff}$ : il travaille en parallèle avec la structure.
- Sa force $-k\,u$ est publiée à part des réactions et entre dans l'équilibre global.
- Ressort et blocage sont exclusifs sur un même DDL ; une raideur nulle ne retient rien.
- Validé analytiquement et contre OpenSees (`zeroLength`, cas 18 à 20).

## Dans le code source

- Données : [`NodalSpring`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Constraints/BoundaryConditions.h#L27-L31).
- Contrôles : [`StructuralModel3D::validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L113-L158).
- Assemblage : [`GlobalAssembler::assembleStiffness`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L55-L101).
- Forces des ressorts : [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115) ; équilibre :
  [`LinearStaticAnalysis::checkEquilibrium`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L124-L170).
- Benchmarks : [cas 18](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L247-L252), [cas 20](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L263-L273).

## Vérifier par les tests

| Test ou cas | Vérification |
|---|---|
| [`Springs_TipSpringInParallelWithCantilever`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L37-L54) | raideurs en parallèle ; $-k\delta$ ; réaction complémentaire |
| [`Springs_RotationalSpringAtBase`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L56-L74) | $\delta = F(L^3/3EI + L^2/k_\theta)$ ; moment du ressort $FL$ |
| [`Springs_StructureSupportedOnlyBySprings`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L76-L97) | équilibre assuré par les seuls ressorts |
| [`Springs_ZeroStiffnessDoesNotHideMechanism`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L99-L110) | raideur nulle : mécanisme maintenu |
| [`Springs_InvalidInputRejected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L112-L126) | refus des données incohérentes |
| [`Springs_ExportedInResultsJson`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp#L128-L137) | raideurs, forces et convention exportées |
| Cas OpenSees 18, 19, 20 | `zeroLength` : 528 valeurs, toutes « OK » |

Lancer : `build\fem3d_tests.exe` ; `build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier]`.

## Références

- A8 Weaver, Gere ; A7 McGuire, Gallagher, Ziemian — appuis élastiques ajoutés sur la diagonale de $K$ : **à
  vérifier** dans le texte.
- Documentation d'OpenSees, `zeroLength` (`-mat`, `-dir`, `-orient`) — vérifié (contenu),
  [REFERENCES.md](../../references/REFERENCES.md) §C9.

---

Précédent : [Chapitre 10 — Charges et conditions aux limites](10-charges-conditions-limites.md) · Suivant :
[Chapitre 12 — Assemblage des matrices globales](12-assemblage.md)
