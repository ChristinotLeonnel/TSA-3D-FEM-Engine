# Chapitre 16 — Déplacements, rotations et contraintes

> **Objectifs**
>
> - Savoir quels déplacements Fem3D publie (nœuds, stations le long des barres) et dans quel repère.
> - Comprendre la déformée **exacte** obtenue par intégration des efforts, et la différence entre pente et rotation de
>   section en Timoshenko.
> - Connaître les contraintes de résistance des matériaux calculées, leurs formules et leurs approximations.
>
> **Prérequis**
>
> - Efforts internes polynomiaux ([chapitre 15](15-efforts-internes.md)).
> - Sections : $A$, $I_y$, $I_z$, $J$, $A_{sy}$, $A_{sz}$, fibres extrêmes, $W_t$ ([chapitre 9](09-materiaux-sections.md)).
> - Théorie de Timoshenko ([chapitre 7](07-timoshenko.md)).

## 16.1 Déplacements et rotations nodaux

**Intuition.** Le vecteur $U$ complet (DDL libres calculés et DDL imposés) donne directement, pour chaque nœud, trois
translations et trois rotations.

**Convention.** `NodalResult::displacement` = (Ux, Uy, Uz, Rx, Ry, Rz), **repère global**, rotations en radians
selon la règle de la main droite. Aux nœuds d'une barre articulée, la rotation publiée est celle du **nœud** ; la
rotation de la barre à la rotule figure dans `localDisplacements` de l'élément ([chapitre 15](15-efforts-internes.md) §15.4).

**Exemple vérifiable.** Console d'Euler–Bernoulli de 3 m, section 0,30 × 0,50 m ($E I_y = 656\,250$ kN·m²),
force $F_z = 20$ kN en bout : $w = FL^3/(3EI_y) = 2{,}7429 \cdot 10^{-4}$ m et
$\theta_y = -FL^2/(2EI_y) = -1{,}3714 \cdot 10^{-4}$ rad (signe moins : $\theta_y = -dw/dx$). En Timoshenko, on ajoute
$FL/(G A_{sz}) = 5{,}926 \cdot 10^{-6}$ m, soit $w = 2{,}8021 \cdot 10^{-4}$ m (+2,16 %). Valeurs recalculées en Python ;
test `Analytical_CantileverTipLoad_BothPlanesBothFormulations`.

## 16.2 Déformée exacte le long d'une barre

**Intuition.** Les efforts internes sont connus exactement ; les lois de comportement de la poutre relient ces efforts
aux dérivées des déplacements. En intégrant à partir des valeurs en i, on obtient la déformée exacte, et non une
interpolation par les fonctions de forme.

**Équations** (celles de `BeamResponse`, repère local, primitives nulles en $x = 0$) :

$$
\begin{aligned}
u(x) &= u_i + \int_0^x \frac{N}{EA}, &
\theta_x(x) &= \theta_{x,i} + \int_0^x \frac{T}{GJ}, \\
\theta_z(x) &= \theta_{z,i} + \int_0^x \frac{M_z}{E I_z}, &
v(x) &= v_i + \int_0^x \Big(\theta_z + \frac{V_y}{G A_{sy}}\Big), \\
\theta_y(x) &= \theta_{y,i} + \int_0^x \frac{M_y}{E I_y}, &
w(x) &= w_i + \int_0^x \Big(-\theta_y + \frac{V_z}{G A_{sz}}\Big).
\end{aligned}
$$

En Euler–Bernoulli, les flexibilités de cisaillement $1/(G A_s)$ valent 0 (`shearFlexibilityY/Z`) ; c'est le seul point
où la formulation intervient, avec $K_\text{loc}$ et les fonctions de forme.

**Interprétation.**

- Sous charge répartie, $M$ est de degré 3, donc $v$ et $w$ de degré 5 : une interpolation cubique d'Hermite ne
  serait pas exacte entre les nœuds ; l'intégration l'est.
- **Pente et rotation.** En Timoshenko, $dv/dx = \theta_z + V_y/(G A_{sy})$ et $dw/dx = -\theta_y + V_z/(G A_{sz})$ :
  la pente de la ligne moyenne diffère de la rotation de la section de la distorsion de cisaillement. Les stations
  publient les deux (`slopeXY`, `slopeXZ`, `sectionRotation`).
- **Cohérence.** En $x = L$, l'intégration doit retrouver le déplacement et la rotation calculés au nœud j : c'est un
  contrôle indépendant de la solution nodale (test `Integration_ResponseConsistentWithNodalDisplacements`).
- Le déplacement global d'une station vaut $R^T$ fois le déplacement local (`globalDisplacement`).

**Flèche locale.** `maxTransverseDeflection` est l'écart transversal maximal $\sqrt{\Delta v^2 + \Delta w^2}$ entre la
déformée et la **corde** qui joint les extrémités déplacées, cherché sur 201 points (pas $L/200$) ; sa position est
publiée. C'est la flèche « relative », pas le déplacement absolu.

**Exemple vérifiable.** Console de 3 m sous $q_z = -8$ kN/m :
$w(x) = q x^2 (6L^2 - 4Lx + x^2)/(24 E I_y)$, soit $w(1{,}5) = -4{,}3714 \cdot 10^{-5}$ m et
$w(3) = qL^4/(8EI_y) = -1{,}2343 \cdot 10^{-4}$ m en Euler–Bernoulli ; Timoshenko ajoute
$q(Lx - x^2/2)/(G A_{sz})$, d'où $w(3) = -1{,}2698 \cdot 10^{-4}$ m. Poutre simplement appuyée de 6 m sous
$q = -10$ kN/m : flèche à mi-portée $5qL^4/(384 E I_y) = -2{,}5714 \cdot 10^{-4}$ m (Timoshenko :
$-2{,}6159 \cdot 10^{-4}$ m), flèche maximale à $L/2$. Valeurs recalculées en Python ; tests
`Analytical_CantileverUniformLoad_ExactAlongMember` (chaque station) et `Analytical_SimplySupportedUniform_OneElement`.

**Limites.** Petits déplacements : la déformée est tracée sur la géométrie initiale. Pas de gauchissement : $\theta_x$
varie linéairement sous torsion uniforme.

## 16.3 Contraintes de résistance des matériaux

**Intuition.** Les efforts internes donnent des contraintes de calcul par les formules classiques de la résistance des
matériaux. Elles sont utiles pour un premier dimensionnement, mais ne remplacent pas une vérification réglementaire.

**Formules** (`BeamResponse::stressesAt`, valeurs en un point de station) :

| Grandeur | Formule | Remarque |
|---|---|---|
| `axial` | $\sigma_N = N/A$ | signée (traction positive) |
| `bendingY` | $\lvert M_y\rvert\, z_\text{ext}/I_y$ | si `zExtreme` est fourni |
| `bendingZ` | $\lvert M_z\rvert\, y_\text{ext}/I_z$ | si `yExtreme` est fourni |
| `extremeNormal` | $\lvert\sigma_N\rvert + \sigma_{M_y} + \sigma_{M_z}$ | exact pour un rectangle, majorant sinon |
| `shearY`, `shearZ` | $\lvert V\rvert / A_s$ (ou $/A$ sans $A_s$) | contrainte **de calcul** ; la moyenne vraie vaut $\lvert V\rvert/A$ |
| `torsion` | $\lvert T\rvert / W_t$ | si $W_t$ est fourni |
| `vonMisesIndicative` | $\sqrt{\sigma_\text{ext}^2 + 3\,\tau_T^2}$ | indicatif, non réglementaire |

Une donnée de section absente donne une valeur absente (`std::nullopt`, `null` dans le JSON), jamais une valeur
inventée.

**Hypothèses et approximations.**

- Axes locaux y et z principaux, centre de gravité et centre de cisaillement confondus.
- `extremeNormal` additionne les maxima des trois termes : ils coïncident au coin d'un rectangle, pas forcément pour
  une autre forme, d'où le caractère majorant.
- `vonMisesIndicative` combine la contrainte normale extrême et la contrainte de torsion maximale, qui ne se
  produisent pas forcément au même point, et ignore le cisaillement d'effort tranchant : c'est un indicateur, pas une
  contrainte équivalente vraie.
- $W_t$ et $J$ du rectangle viennent des formules approchées de Roark ($W_t$ à 0,3 % et $J$ à 0,5 % des séries exactes
  de Saint-Venant pour $a/b$ de 1 à 50) ; pour le cercle plein, $J = \pi d^4/32$ et $W_t = \pi d^3/16$ sont exacts.
- Pas de contrainte de gauchissement (bimoment) : pour les profils ouverts minces, la contrainte normale de torsion
  non uniforme manque ([chapitre 8](08-torsion-gauchissement.md)).

**Exemple vérifiable.** Section 0,30 × 0,50 m ($I_y = 3{,}125 \cdot 10^{-3}$ m⁴, $z_\text{ext} = 0{,}25$ m,
$A_s = 0{,}125$ m²).

- Poutre simplement appuyée de 6 m sous $q = -10$ kN/m : $M_y(L/2) = qL^2/8 = -45$ kN·m,
  $\sigma = 45 \times 0{,}25 / 3{,}125 \cdot 10^{-3} = 3600$ kN/m² (3,6 MPa) ; test
  `Analytical_SimplySupportedUniform_OneElement`.
- À l'appui, $V = 30$ kN : $\tau_V = V/A_s = 240$ kN/m² (contrainte de calcul), alors que la moyenne vraie vaut
  $V/A = 200$ kN/m².
- Torsion $T = 12$ kN·m : $J = 2{,}8174 \cdot 10^{-3}$ m⁴, $W_t = 1{,}0612 \cdot 10^{-2}$ m³,
  $\tau_T = 1130{,}8$ kN/m² ; rotation d'une console de 4 m : $TL/(GJ) = 2{,}1034 \cdot 10^{-4}$ rad ; test
  `Analytical_SaintVenantTorsion`.

Valeurs recalculées en Python avec les formules du code.

**Défaut corrigé.** La première formule de $W_t$ du rectangle surestimait $W_t$ jusqu'à 4,4 %, donc sous-estimait
$\tau_T$ (non conservatif) ; elle a été remplacée par le polynôme de Roark et comparée aux séries exactes
(`Section_RectangleTorsionMatchesExactSeries`, [AUDIT.md](../../AUDIT.md) constat 2).

**Limites.** Aucune vérification normative (Eurocodes) ; contraintes non comparées à OpenSees, qui ne les calcule pas
pour ces éléments (verdict « validé avec réserves », [VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) §3).

## À retenir

- Déplacements nodaux en repère global ; déformée et rotations le long des barres en repère local, obtenues par
  intégration **exacte** des efforts.
- En Timoshenko, pente ≠ rotation de section (distorsion $V/(G A_s)$) ; les deux sont publiées.
- Contraintes de RDM : $\tau_V = V/A_s$ est une contrainte de calcul ; `extremeNormal` est majorant hors rectangle ;
  von Mises est indicatif ; une donnée absente donne une valeur absente.

## Dans le code source

- Déformée et contraintes : [`BeamResponse::BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98), [`BeamResponse::stressesAt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L108-L122),
  [`BeamResponse::result`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L124-L165).
- Structures de résultats : [`StationResult`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h#L61-L71), [`SectionStresses`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h#L49-L59),
  [`ElementResult`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h#L73-L83).
- Flexibilités de cisaillement et fonctions de forme : [`IBeamElement3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/IBeamElement3D.h#L35-L64).
- Sections : [`BeamSection3D::rectangle`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L23-L45), [`BeamSection3D::circle`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L47-L64), [`BeamSection3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Sections/BeamSection3D.h#L18-L40).
- Résultats nodaux : [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115) ; export : [`resultsToJson`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/ResultsJsonWriter.cpp#L78-L78).

## Vérifier par les tests

| Test | Vérification |
|---|---|
| [`Analytical_CantileverTipLoad_BothPlanesBothFormulations`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L75-L100) | flèches, rotations, pente − rotation = $V/(G A_s)$ |
| [`Analytical_CantileverUniformLoad_ExactAlongMember`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L102-L123) | déformée exacte entre les nœuds |
| [`Analytical_SimplySupportedUniform_OneElement`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L125-L140) | flèche $5qL^4/384EI$, position du maximum, $\sigma = Mz/I$ |
| [`Analytical_SaintVenantTorsion`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L65-L73) | $\theta = TL/GJ$, $\tau = T/W_t$ |
| [`Integration_ResponseConsistentWithNodalDisplacements`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L111-L135) | intégration en $L$ = valeurs nodales en j |
| [`Section_RectangleTorsionMatchesExactSeries`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L130-L152) | $J$ et $W_t$ à 0,6 % des séries exactes |
| [`Section_CircleShearCoefficientUsesPoisson`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L183-L192) | coefficient de Cowper du cercle |
| [`Integration_ResultsJsonFormat`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp#L169-L180) | stations exportées, aucune valeur non finie |

Lancer : `build\fem3d_tests.exe`. Le post-traitement (`postprocessor/`, 15 tests) relit ces valeurs dans le JSON
([chapitre 17](17-architecture-cpp.md)).

## Références

- A9 Timoshenko, Gere, *Mechanics of Materials* — vérifié (notice) ; formules de RDM, chapitres **à vérifier**.
- A13 *Roark's Formulas for Stress and Strain* — vérifié (notice) ; coefficients de $J$ et $W_t$ du rectangle
  **à vérifier** sur l'ouvrage imprimé ; recoupés indépendamment par les séries exactes ([REFERENCES.md](../../references/REFERENCES.md) §C3).
- B2 Cowper (1966) — vérifié (notice Crossref), texte non lu.
- B1 Timoshenko (1921) — vérifié (notice Crossref), texte non lu.

---

Précédent : [Chapitre 15 — Reconstruction des efforts internes](15-efforts-internes.md) · Suivant :
[Chapitre 17 — Architecture logicielle C++](17-architecture-cpp.md)
