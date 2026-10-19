# Chapitre 10 — Charges et conditions aux limites

> **Objectifs**
>
> - Décrire un cas de charge Fem3D : charges nodales, charges réparties linéaires en repère local ou global.
> - Calculer à la main les forces nodales équivalentes d'une charge répartie et savoir pourquoi elles sont exactes.
> - Déclarer des appuis rigides et des déplacements imposés, et connaître les contrôles faits avant le calcul.
>
> **Prérequis**
>
> - Repères local et global, matrice de rotation $R$ ([chapitre 4](04-reperes.md)).
> - Conventions de signes et d'unités ([chapitre 5](05-unites-signes.md)).
> - Fonctions de forme d'Euler–Bernoulli et de Timoshenko ([chapitres 6](06-euler-bernoulli.md) et
>   [7](07-timoshenko.md)).

Ce chapitre décrit les **données** : ce que l'utilisateur fournit et la façon dont le moteur le transforme en
vecteur de charges. L'usage de ces données dans le système partitionné est traité aux
[chapitres 12](12-assemblage.md) et [13](13-conditions-limites-stabilite.md) ; les appuis élastiques au
[chapitre 11](11-ressorts.md).

## 10.1 Cas de charge

**Intuition.** Un cas de charge est un ensemble de charges appliquées en même temps (poids propre, vent, neige…).
La rigidité de la structure ne dépend pas des charges : elle est assemblée et factorisée **une seule fois**, puis
chaque cas de charge ne coûte qu'une descente-remontée.

**Mise en œuvre.** `LoadCase3D` contient un identifiant, un nom, une liste de `NodalLoad3D` et une liste de
`DistributedLoad3D`. Le constructeur de `LinearStaticAnalysis` valide le modèle, assemble $K$ et le factorise ;
`run(id)` traite un cas, `runAll()` les traite tous dans l'ordre de déclaration. Deux cas de même identifiant sont
refusés (`DuplicateId`).

**Limites.** Le moteur ne construit **ni combinaison ni enveloppe** : en statique linéaire, une combinaison est une
somme pondérée de résultats, que TSA construit et transmet comme un cas de charge ordinaire
([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §5).

## 10.2 Charges nodales

**Symboles.** Force $(F_X, F_Y, F_Z)$ et moment $(M_X, M_Y, M_Z)$ appliqués à un nœud, **repère global**, moment
selon la règle de la main droite.

**Algorithme** (`GlobalAssembler::assembleLoads`) : chaque composante est ajoutée au DDL global correspondant du
vecteur $F$. Plusieurs charges sur le même nœud s'additionnent.

**Interprétation.** Une charge nodale placée sur un DDL **bloqué** tombe dans $F_r$ : elle est reprise directement
par l'appui ($R = \dots - F_r$, [chapitre 13](13-conditions-limites-stabilite.md) §13.1) sans déformer la structure.
Ce n'est pas une erreur, et le moteur ne la signale pas.

**Limites.** Pas de charge nodale en repère local d'un nœud (il n'existe pas de repère nodal incliné).

## 10.3 Charges réparties linéaires

**Intuition.** Une charge répartie est décrite par sa valeur au nœud i et sa valeur au nœud j ; entre les deux, elle
varie linéairement. Une charge uniforme a deux valeurs égales, une charge triangulaire a une valeur nulle.

**Symboles.** $q(x) = q_1 + (q_2 - q_1)\,x/L$, par composante, en force **par unité de longueur réelle** de la
barre ; $x \in [0, L]$ mesuré depuis le nœud i.

**Deux repères** (`LoadFrame`) :

- `Local` : composantes $(q_x, q_y, q_z)$ dans le repère de la barre ; $q_x$ est axiale ;
- `Global` : composantes $(q_X, q_Y, q_Z)$ projetées sur le repère local par

$$
q_\text{local} = R\, q_\text{global}.
$$

Plusieurs charges réparties sur la même barre sont projetées puis **sommées** en une seule charge linéaire
(`LocalLinearLoad::fromLoads`) : c'est permis parce que la projection et la somme sont linéaires.

**Exemple vérifiable.** Arbalétrier de 4 m incliné de 30° dans le plan X-Z, poids propre $q_Z = -5$ kN/m (global).
Composantes locales : axiale $-5 \sin 30° = -2{,}5$ kN/m, transversale $-5 \cos 30° \approx -4{,}330$ kN/m ;
résultante $-20$ kN, verticale (valeurs recalculées en Python). Le test
`Integration_GlobalAndLocalDistributedLoadsEquivalent` vérifie sur une barre inclinée quelconque qu'une charge
globale et sa projection locale donnent les mêmes six déplacements à $10^{-12}$ près, et que la résultante d'une
charge trapézoïdale vaut $(q_1 + q_2) L / 2$.

**Attention : longueur réelle et projection horizontale.** Une charge réglementaire donnée par mètre de projection
horizontale (la neige, par exemple) doit être convertie par l'utilisateur : $s$ kN/m² sur une bande de 1 m donne
$s \cos\alpha$ kN par mètre de barre (1,5 kN/m² à 30° : 1,299 kN/m). Fem3D ne fait pas cette conversion
([SIGN_CONVENTIONS.md](../../theory/SIGN_CONVENTIONS.md) §4).

**Limites.** Charge sur **toute la longueur** seulement : pas de charge ponctuelle sur barre, pas de charge
partielle, pas de moment réparti. Dans TSA, ces charges sont refusées par le moteur `fem3d`
([chapitre 18](18-integration-tsa-opensees.md)). Leur ajout est l'étape 2 de la
[feuille de route](../../ROADMAP.md) ([chapitre 21](21-extensions.md)).

## 10.4 Forces nodales équivalentes

**Intuition.** Le système global ne connaît que des forces aux nœuds. Il faut donc remplacer la charge répartie par
des forces nodales qui produisent **le même travail** sur tout déplacement de l'élément.

**Équation** (travaux virtuels, [THEORY.md](../../theory/THEORY.md) §4) :

$$
f_{\text{eq},k} = \int_0^L N_k(x)\, q(x)\, dx ,
$$

avec les fonctions de forme $N_k$ **de la formulation** (Euler–Bernoulli ou Timoshenko). Le vecteur local de 12
composantes est ensuite passé au repère global par $f_\text{global} = T^T f_\text{local}$ et ajouté à $F$.

**Pourquoi c'est exact.** Les fonctions de forme des deux éléments sont les déformées exactes de la poutre
bi-encastrée sous un déplacement unitaire d'extrémité. Par le théorème de Betti, $f_\text{eq}$ est alors exactement
**l'opposé des forces d'encastrement parfait**. Les déplacements nodaux obtenus sont donc exacts, et l'erreur ne
porte que sur l'arrondi ([REFERENCES.md](../../references/REFERENCES.md) §C4).

**Algorithme** (`equivalentNodalLoads`) : quadrature de Gauss–Legendre à 4 points sur $[0, L]$. L'intégrande est un
polynôme de degré au plus 4 (fonction de forme cubique × charge linéaire) ; 4 points intègrent exactement jusqu'au
degré 7.

**Charge uniforme, Euler–Bernoulli.** Dans le plan x-y (DDL $v_i, \theta_{z,i}, v_j, \theta_{z,j}$) :

$$
f_\text{eq} = \left(\frac{q_y L}{2},\ \frac{q_y L^2}{12},\ \frac{q_y L}{2},\ -\frac{q_y L^2}{12}\right).
$$

Dans le plan x-z (DDL $w_i, \theta_{y,i}, w_j, \theta_{y,j}$), les termes de moment **changent de signe**, parce que
$dw/dx = -\theta_y$ ([chapitre 5](05-unites-signes.md)) :
$\left(\tfrac{q_z L}{2}, -\tfrac{q_z L^2}{12}, \tfrac{q_z L}{2}, \tfrac{q_z L^2}{12}\right)$.

**Exemple vérifiable.** Poutre bi-encastrée de 5 m, charge triangulaire selon y local de 0 (nœud i) à
$w = 12$ kN/m (nœud j), Euler–Bernoulli :

| Grandeur | Formule | Valeur |
|---|---|---|
| $f_{\text{eq}}$ en $v_i$ | $3wL/20$ | 9 kN |
| $f_{\text{eq}}$ en $\theta_{z,i}$ | $wL^2/30$ | 10 kN·m |
| $f_{\text{eq}}$ en $v_j$ | $7wL/20$ | 21 kN |
| $f_{\text{eq}}$ en $\theta_{z,j}$ | $-wL^2/20$ | −15 kN·m |

Valeurs recalculées par un script Python qui reproduit la quadrature. Les réactions sont leurs opposées :
$R_i = -9$ kN, $R_j = -21$ kN (test `Analytical_FixedFixedTriangularLoad_EulerBernoulli`). En Timoshenko, les
moments d'encastrement dépendent de $\Phi$ ([THEORY.md](../../theory/THEORY.md) §4) ; ils sont vérifiés par
`Analytical_FixedFixedTriangularLoad_Timoshenko`.

**Barres à rotules.** Une extrémité relâchée ne doit recevoir aucune charge : les forces équivalentes sont
**condensées** comme la rigidité,

$$
f_c = f_{\text{eq},a} - K_{ab} K_{bb}^{-1} f_{\text{eq},b},
$$

où $b$ désigne les DDL relâchés et $a$ les autres (`condensedLoads`, [THEORY.md](../../theory/THEORY.md) §6). Le
détail est au [chapitre 15](15-efforts-internes.md) §15.4 et dans [FORMULATIONS.md](../../formulations/FORMULATIONS.md) §4.

**La charge n'est jamais oubliée entre les nœuds.** Les forces équivalentes ne servent qu'au système global. Les
efforts internes et la déformée sont reconstruits avec la charge répartie réelle ([chapitre 15](15-efforts-internes.md)),
ce qui donne la parabole exacte du moment sous charge uniforme (test
`Analytical_CantileverUniformLoad_ExactAlongMember`).

## 10.5 Appuis rigides et déplacements imposés

**Symboles.** `NodeRestraint` : pour chacun des six DDL d'un nœud, un booléen `fixed` et une valeur `prescribed`
(translation en unité de longueur, rotation en radians).

**Raccourcis.**

| Fonction | DDL bloqués | Usage |
|---|---|---|
| `NodeRestraint::fullyFixed(id)` | les six | encastrement |
| `NodeRestraint::pinned(id)` | Ux, Uy, Uz | articulation sphérique (les trois rotations libres) |
| `NodeRestraint::custom(id, fixed)` | au choix | appui glissant, rouleau, torsion bloquée… |

**Points à connaître.**

- Les appuis sont orientés selon les **axes globaux** : pas d'appui incliné.
- `setRestraint` **remplace** les conditions d'un nœud déjà restreint (elles ne se cumulent pas).
- Une valeur imposée n'est lue que si le DDL est bloqué ; elle doit être finie dans tous les cas.
- Un nœud peut avoir un enregistrement d'appui dont tous les DDL sont libres (utile avec des ressorts,
  [chapitre 11](11-ressorts.md)) ; ses réactions sont alors publiées, nulles.
- Une **articulation** (`pinned`) laisse libre la rotation autour de l'axe de la barre : sur une poutre isolée, il
  faut bloquer Rx à l'un des appuis, sinon le modèle est un mécanisme de torsion
  ([chapitre 13](13-conditions-limites-stabilite.md) §13.3). C'est ce que fait le modèle `simplySupported` des tests.

**Exemple vérifiable.** Poutre sur deux appuis de 6 m, $q_Z = -10$ kN/m global (nœud 1 : Ux, Uy, Uz, Rx bloqués ;
nœud 2 : Uy, Uz bloqués) : $M_y(L/2) = qL^2/8 = -45$ kN·m et flèche $5qL^4/(384 EI)$, retrouvées avec **un seul
élément** (test `Analytical_SimplySupportedUniform_OneElement`). Le déplacement imposé non nul est traité au
[chapitre 13](13-conditions-limites-stabilite.md) §13.2.

## 10.6 Contrôles avant calcul

`StructuralModel3D::validate` est appelé par le constructeur de `LinearStaticAnalysis`. Il lève une `Fem3DError`
dont le message désigne l'objet fautif.

| Donnée | Refus | Code |
|---|---|---|
| Nœud d'un appui, d'une charge nodale ou d'un ressort | nœud inconnu | `UnknownReference` |
| Élément d'une charge répartie | élément inconnu | `UnknownReference` |
| Valeur imposée, force, moment, intensité répartie | valeur non finie (NaN, infini) | `InvalidInput` |
| Ressort | raideur négative, non finie, ou sur un DDL bloqué | `InvalidInput` |

Le matériau et la section sont contrôlés plus tôt, dès leur ajout (`addMaterial`, `addSection`). La géométrie de
chaque barre, et les aires de cisaillement exigées par Timoshenko, sont contrôlées en créant l'élément, avec un
message préfixé par « élément n : » ([chapitre 9](09-materiaux-sections.md)) ; test `Input_InvalidPropertiesRejected`.

## 10.7 Contrôle d'équilibre global

**Intuition.** Après le calcul, la somme des charges appliquées et des réactions doit être nulle, en force et en
moment. C'est une vérification indépendante de la façon dont les charges ont été transformées en forces nodales.

**Équations** (`checkEquilibrium`). Pour une charge trapézoïdale, résultante et premier moment exacts :

$$
\mathbf{R}_q = \frac{L}{2}\,(q_1 + q_2),
\qquad
\int_0^L s\, q(s)\, ds = L^2 \left(\frac{q_1}{6} + \frac{q_2}{3}\right),
$$

passés au repère global ; le moment par rapport à l'origine vaut
$\mathbf{P}_i \times \mathbf{R}_q + \mathbf{x} \times \int_0^L s\, q(s)\, ds$ ($\mathbf{P}_i$ : position du nœud i,
$\mathbf{x}$ : axe de la barre). Les réactions des appuis rigides et les forces des ressorts sont ensuite ajoutées ;
les résidus `forceResidual` et `momentResidual` sont les normes des sommes.

**Ce que ce contrôle prouve, et ce qu'il ne prouve pas.** Il prouve que les forces équivalentes sont statiquement
équivalentes aux charges, et que la résolution est précise. Il ne prouve **pas** que la rigidité est juste : une
matrice $K$ fausse mais symétrique le satisferait ([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §4).
C'est le rôle des tests analytiques et des benchmarks ([chapitre 19](19-validation-reproductibilite.md)).

## À retenir

- Charges nodales en repère global ; charges réparties linéaires sur toute la longueur, en repère local ou global,
  par unité de longueur **réelle**.
- Forces nodales équivalentes $\int N_k q\,dx$ avec les fonctions de forme de la formulation : exactes (opposées des
  forces d'encastrement parfait), condensées aux rotules.
- Entre les nœuds, la charge réelle est reprise dans les efforts et la déformée : rien n'est approché.
- Appuis selon les axes globaux ; `pinned` laisse la torsion libre ; contrôles complets avant tout calcul.

## Dans le code source

- Charges : [`NodalLoad3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Loads/Loads3D.h#L15-L20), [`LoadFrame`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Loads/Loads3D.h#L22-L26),
  [`DistributedLoad3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Loads/Loads3D.h#L31-L37), [`LoadCase3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Loads/Loads3D.h#L39-L45).
- Projection et somme des charges d'une barre : [`LocalLinearLoad::fromLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L18-L29).
- Forces nodales équivalentes : [`equivalentNodalLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L31-L51), condensation aux rotules :
  [`condensedLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L47-L58).
- Vecteur de charges global : [`GlobalAssembler::assembleLoads`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L103-L126).
- Appuis : [`NodeRestraint`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Constraints/BoundaryConditions.h#L13-L22).
- Contrôles : [`StructuralModel3D::validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L113-L158).
- Factorisation unique et calcul par cas : [`LinearStaticAnalysis::LinearStaticAnalysis`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L14-L33),
  [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115).
- Équilibre global : [`LinearStaticAnalysis::checkEquilibrium`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L124-L170).

## Vérifier par les tests

| Test ou cas | Vérification |
|---|---|
| [`Integration_GlobalAndLocalDistributedLoadsEquivalent`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L94-L109) | charge globale = sa projection locale ; résultante trapézoïdale |
| [`Analytical_CantileverUniformLoad_ExactAlongMember`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L102-L123) | flèche, moment et déformée exacts entre les nœuds |
| [`Analytical_SimplySupportedUniform_OneElement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L125-L140) | $qL^2/8$, $5qL^4/384EI$ avec un élément |
| [`Analytical_FixedFixedTriangularLoad_EulerBernoulli`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L142-L162) | $3wL/20$, $7wL/20$, $wL^2/30$, $wL^2/20$ |
| [`Analytical_FixedFixedTriangularLoad_Timoshenko`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L40-L57) | moments d'encastrement fonction de $\Phi$ |
| [`Analytical_ProppedCantileverTriangularLoad_Timoshenko`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L59-L83) | charge triangulaire, poutre hyperstatique en Timoshenko |
| [`Input_InvalidPropertiesRejected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L121-L134) | données invalides refusées |
| Cas OpenSees 05, 09, 12, 15 | charges réparties locales et globales comparées à `eleLoad -beamUniform` |

Lancer : `build\fem3d_tests.exe` ; `build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier]`.

## Références

- B4 Archer (1965), charges consistantes — vérifié (notice Crossref), texte non lu.
- A3 Bathe, *Finite Element Procedures* — vérifié (notice) ; §4.2 (vecteurs de charges consistants) : correspondance
  vérifiée par le programme MIT OCW.
- A6 Przemieniecki ; A8 Weaver, Gere ; A7 McGuire, Gallagher, Ziemian — égalité avec les forces d'encastrement
  parfait : **à vérifier** dans le texte.
- Documentation d'OpenSees, `eleLoad -beamUniform` (axes locaux, charge uniforme seulement) — vérifié (contenu),
  [REFERENCES.md](../../references/REFERENCES.md) §C4.

---

Précédent : [Chapitre 9 — Matériaux et propriétés de section](09-materiaux-sections.md) · Suivant :
[Chapitre 11 — Ressorts d'appui](11-ressorts.md)
