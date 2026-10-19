# Audit indépendant des formulations — moteur EF poutres 3D (Fem3D)

Date : 2026-10-19. Audit en lecture seule : aucun fichier de code n'a été modifié.

**Périmètre.** Les formulations d'élément, le repère local, les forces équivalentes, la reconstitution des efforts et
de la déformée, les rotules, les sections, le critère de mécanisme, les réactions et le contrôle d'équilibre, la
couverture des tests et la cohérence avec `docs/THEORY.md` et `docs/SIGN_CONVENTIONS.md`.

**Références de numérotation.** Les numéros de ligne cités valent pour les fichiers de formulation, qui ne changent
pas : `BeamElements.cpp`, `BeamResponse.cpp`, `StaticCondensation.cpp`, `BeamSection3D.cpp` et
`LocalCoordinateSystem3D.cpp`. Pour `Assembly.cpp`, `LinearStaticAnalysis.cpp` et `LinearSystemSolver.cpp`, on cite
le nom de la fonction, car ces fichiers sont en cours de modification (ressorts d'appui).

## Méthode

Trois moyens indépendants du code audité ont servi :

1. **Dérivation symbolique (sympy 1.14).** On résout l'équation de Timoshenko pour une poutre prismatique, avec les
   conventions du moteur : face positive, `dVy/dx = −qy`, `dMz/dx = −Vy`, `θz′ = Mz/EIz`, `v′ = θz + Vy/GAsy`, et
   dans le plan x-z `dMy/dx = +Vz`, `θy′ = My/EIy`, `w′ = −θy + Vz/GAsz`. On en tire :
   - la rigidité 4×4 de chaque plan, efforts exercés sur l'élément pour des déplacements unitaires ;
   - les déformées associées, c'est-à-dire les fonctions de forme ;
   - les forces d'encastrement parfait sous charge uniforme, triangulaire et trapézoïdale ;
   - la console sur appui (rotule en j) sous charge triangulaire.

   Script : `…/scratchpad/audit_b/timo_derivation.py` et `checks2.py` (sorties dans `timo_out.txt`).
2. **Sonde C++.** `probe.cpp` est compilé avec MSVC 14.51 à partir des sources mêmes du moteur (éléments, condensation,
   repère, sections, réponse, solveur). Il appelle directement `Timoshenko3D`, `BeamResponse`, `condensedLoads` et
   `LinearSystemSolver::factorize`. Sorties dans `probe_out.txt`.
3. **Références d'ouvrage.**
   - J.S. Przemieniecki, *Theory of Matrix Structural Analysis*, McGraw-Hill 1968, ch. 5 (poutre avec cisaillement).
   - J.N. Reddy, *An Introduction to the Finite Element Method*, 3e éd., ch. 5 (élément de Timoshenko à
     interpolation interdépendante, IIE).
   - W. McGuire, R. Gallagher, R. Ziemian, *Matrix Structural Analysis*, 2e éd., ch. 4–5 (repère, transformation,
     condensation, forces d'encastrement).
   - S. Timoshenko, J.N. Goodier, *Theory of Elasticity*, 3e éd., ch. 10 §109 (torsion du rectangle, séries exactes).
   - W.C. Young, R.G. Budynas, *Roark's Formulas for Stress and Strain*, 7e éd., tableau 10.1 cas 4.
   - G.R. Cowper, « The shear coefficient in Timoshenko's beam theory », *J. Appl. Mech.* 33 (1966) 335–340.
   - Documentation OpenSees, `geomTransf Linear` (repère local).

Dossier du scratchpad :
`C:\Users\TSARAL~1\AppData\Local\Temp\claude\d--Tsaraloha-Platforme\410db04f-8abd-424d-9937-75a7327dbb96\scratchpad\audit_b\`.
Les résultats essentiels sont recopiés ci-dessous, pour que le rapport reste vérifiable sans ce dossier.

---

## 1. `src/Elements/BeamElements.cpp` — matrices et fonctions de forme

### 1.1 Euler–Bernoulli — CONFORME

- **Plan x-y** (`localStiffnessMatrix`, l. 44–48) : `EIz/L³ [12 6L −12 6L ; 6L 4L² −6L 2L² ; …]`. C'est la matrice
  classique (Przemieniecki, éq. 5.116 ; McGuire, éq. 4.34).
- **Plan x-z** (l. 49–52) : les termes croisés w-θy sont de signe opposé, ce qui découle de `θy = −w′`. La dérivation
  sympy redonne exactement cette matrice comme limite Φ → 0 de la matrice de Timoshenko :
  `[12 −6L −12 −6L ; −6L 4L² 6L 2L² ; −12 6L 12 6L ; −6L 2L² 6L 4L²]`.
- **Axial et torsion** (`addAxialAndTorsion`, l. 24–31) : EA/L et GJ/L sont corrects.
- **Fonctions de forme d'Hermite** (l. 62–75) : H1…H4 sont exactes. Pour le plan x-z, `(H1, −H2, H3, −H4)`, ce qui
  est cohérent avec θy = −w′ (sympy : écart nul).

### 1.2 Timoshenko — CONFORME

- **Paramètres de cisaillement** (l. 88–101) : `Φy = 12 E Iz /(G Asy L²)` et `Φz = 12 E Iy /(G Asz L²)`.
  - L'association **Iz ↔ Asy** est juste : la flexion dans le plan x-y met en jeu l'effort tranchant Vy, repris par
    l'aire Asy.
  - De même, **Iy ↔ Asz** pour le plan x-z.
  - Référence : Przemieniecki, éq. 5.115, Φ = 12EI/(GAs L²).
- **Rigidité** (l. 110–126). Résultat sympy, « K exacte − K code », pour les deux plans :

  ```text
  Plan xy : K exacte − K code = 0 (4×4)      Plan xz : K exacte − K code = 0 (4×4)
  ```

- **Fonctions de forme interdépendantes** (l. 136–154). Les déformées exactes de la poutre de Timoshenko bi-encastrée,
  sous un déplacement unitaire de chaque DDL (θ = rotation de section), redonnent exactement N1…N4 du code :
  écart sympy nul pour les 8 fonctions. Pour le plan x-z, on a `(N1, −N2, N3, −N4)`.
  - Les N2 et N4 de x-z sont associées à θy, rotation de section (et non pente), comme l'indique le commentaire
    l. 153.
  - Référence : Reddy, ch. 5, IIE.
- **Limite Φ → 0** : on retrouve exactement Euler–Bernoulli (sympy). Le test `Element_TimoshenkoTendsToEulerBernoulli`
  le vérifie aussi.

## 2. `src/Geometry/LocalCoordinateSystem3D.cpp` — repère local — CONFORME (une réserve mineure)

- **Construction des axes** (l. 37–41) : `y = v_xz × x / ‖·‖`, `z = x × y`, R a pour lignes (x, y, z). C'est
  exactement la construction de `CrdTransf` d'OpenSees (`yAxis = vecxz × xAxis`, `zAxis = xAxis × yAxis`).
- **Vérification par la sonde (P3)** sur la barre (1,2,3) → (4,−1,7), avec v_xz = (0,3 ; 0,2 ; 1) :

  ```text
  det R = 1,000000000000000    ‖R Rᵀ − I‖ = 3,9e-16    y·v_xz = 0    z·v_xz = 0,766 > 0
  ```

  Donc v_xz est bien dans le plan x-z, du côté +z.
- **Transformation** : `T = diag(R,R,R,R)`, `d_l = T d_g`. L'invariance de l'énergie,
  `½ d_lᵀ K_l d_l = ½ d_gᵀ (Tᵀ K_l T) d_g`, donne `K_g = Tᵀ K_l T` (l. 44–49 et `globalStiffnessMatrix`, l. 18–22).
  Pour les forces, `f_g = Tᵀ f_l` (`assembleLoads`). C'est correct (McGuire, §5.2).
- **Valeur par défaut pour un poteau vertical** : v_xz = −X, d'où y = +Y et z = −X (sonde et test unitaire).
- **RÉSERVE (mineure)** : discontinuité de l'orientation par défaut pour une barre **quasi verticale**.
  - Si la barre penche vers ±X, aucune discontinuité. Exemple : pour x ≈ (ε, 0, 1), on a `Z × x ∥ +Y`, comme pour la
    verticale (sonde : y = (0 1 0) à 3,3·10⁻⁵ et à 3,3·10⁻⁴ rad).
  - Si elle penche vers ±Y, x ≈ (0, ε, 1) donne `y = −X`, alors que la verticale donne `y = +Y` : l'orientation
    tourne de 90° pour un écart d'angle infinitésimal.
  - C'est inhérent à toute orientation par défaut : OpenSees impose v_xz.
  - **Proposition** : dire dans `SIGN_CONVENTIONS.md` §2 que v_xz doit être fourni pour les barres quasi verticales
    (seuil |x·Z| ≥ 1 − 10⁻⁹, soit environ 4,5·10⁻⁵ rad), ou émettre un avertissement.

## 3. `src/Results/BeamResponse.cpp` — forces équivalentes, efforts, déformée

### 3.1 Quadrature de Gauss à 4 points — CONFORME

- **Exactitude de la quadrature** (l. 31–51) : l'intégrande `N_k(ξ) q(ξ)` est de degré 3 (N, coefficients fonctions
  de Φ) + 1 (q linéaire) = 4. Gauss–Legendre à 4 points est exacte jusqu'au degré 7.
- **Points et poids** (l. 13–14) : corrects à 10⁻¹⁶ près (somme des poids = 2 − 2·10⁻¹⁶).
- **Changement de variable** : `x = L(t+1)/2`, poids `w L/2`, correct.
- **Écart numérique** entre Gauss et l'intégrale exacte, pour Φ = 0,7 : au plus 3·10⁻¹⁵.

### 3.2 Forces équivalentes de Timoshenko = forces d'encastrement exactes — CONFORME (démontré)

**Démonstration (Betti).**

- **État A** : poutre bi-encastrée sous q(x), réactions nodales R_A, déplacements nodaux nuls.
- **État B** : déplacement unitaire du DDL k, les autres nuls, sans charge de travée. Les forces nodales valent
  `K e_k`, et la déformée du centre est la solution homogène exacte `w_k(x)`.
- **Réciprocité** : `∫ q w_k dx + R_A·e_k = (K e_k)·0 = 0`, donc `−R_A,k = ∫ q w_k dx`.
- **Conclusion** :
  - la charge est appliquée sur la ligne moyenne, donc seule w (et non θ) travaille ;
  - le théorème vaut pour tout corps élastique linéaire, donc aussi pour Timoshenko ;
  - `f_eq = ∫ Nᵀ q` est exactement l'opposé des forces d'encastrement **si et seulement si** les N_k sont les
    déformées homogènes exactes ;
  - le §1.2 établit que c'est le cas.

**Vérification sympy indépendante.** On résout le problème aux limites avec charge, puis on compare à `∫ N q`
(écart nul dans les deux plans). Charges uniforme a, triangulaire 0 → b et trapézoïdale ; résultats pour le plan x-y :

```text
uniforme     : f_eq = [aL/2, aL²/12, aL/2, −aL²/12]                       (indépendant de Φ)
triangulaire : f_eq = [bL(10Φ+9)/(60(1+Φ)), bL²(5Φ+4)/(120(1+Φ)), bL(20Φ+21)/(60(1+Φ)), −bL²(5Φ+6)/(120(1+Φ))]
écart (exacte − ∫N q) = [0, 0, 0, 0] pour les trois charges et les deux plans
```

Pour Φ = 0, on retrouve wL²/30, wL²/20, 3wL/20 et 7wL/20. Quand Φ → ∞, on obtient wL²/24 (aux deux extrémités),
wL/6 et wL/3.

**Vérification par le code réel (sonde P1).** Timoshenko, L = 2 m, rectangle 0,3 × 1,0 (Φy = 0,07, Φz = 0,778),
charge triangulaire 0 → 12 dans les deux plans, d = 0 :

```text
plan x-y : f1 −3,6261682243 (exact −3,6261682243)  f5 −1,6261682243 (−1,6261682243)  f7 −8,3738317757  f11 2,3738317757
plan x-z : f2 −3,775 (exact −3,775)  f4 1,775 (1,775)  f8 −8,225 (−8,225)  f10 −2,225 (−2,225)
EB aurait M_i = 1,6 et M_j = 2,4 : l'effet de Φ est bien pris en compte (≈ 11 % sur M_i pour Φz = 0,78)
reconstitution en x = L : v, w, θy, θz ≈ 1e-21 (attendu 0)
```

### 3.3 Efforts d'extrémité et efforts internes — CONFORME

- **Efforts d'extrémité** : `f = K d − f_eq` (l. 64, 70). Ce sont les efforts exercés sur l'élément, avec la même
  convention que `localForce` d'OpenSees.
- **Équilibre du tronçon [0, x]**. On écrit la somme des forces et la somme des moments au point de coupe, la face de
  coupe ayant pour normale +x :
  - `N = −f0 − ∫qx` et `Vy = −f1 − ∫qy` ;
  - `Mz = −f5 + x f1 + ∫(x−s)qy` (moment de f1 e_y au bras −x e_x : `−x f1 e_z`) ;
  - `My = −f4 − x f2 − ∫(x−s)qz` (`(−x e_x) × (f2 e_z) = +x f2 e_y`).

  C'est identique aux l. 80–85 (`Polynomial::linear(c0, c1) = c0 + c1 x`).
- **Relations différentielles** : `dMz/dx = f1 + ∫qy = −Vy` et `dMy/dx = −f2 − ∫qz = +Vz`. Vérifié aussi par
  différences finies dans la sonde : résidus ≈ 1·10⁻¹⁰, l'erreur de troncature.
- **Signes des contraintes de flexion**, cohérents avec la rotation :
  - `Mz = −∫ y σ dA = E Iz θz′` avec `ε = −y θz′` ;
  - `My = ∫ z σ dA = E Iy θy′` avec `u = z θy`.

### 3.4 Intégration de la déformée — CONFORME

- **Rotations et allongements** (l. 90–93) : `θ′ = M/EI`, `u′ = N/EA`, `θx′ = T/GJ`.
- **Pentes** (l. 94–97) :
  - `v′ = θz + Vy/(G Asy)`, car `γxy = ∂v/∂x + ∂u/∂y = v′ − θz` ;
  - `w′ = −θy + Vz/(G Asz)`, car `γxz = w′ + θy`, avec `u = z θy`.
- **Euler–Bernoulli** : les flexibilités de cisaillement valent 0 (`EulerBernoulli3D.h`).
- **Fermeture** : l'intégration part de x = 0 et ne réimpose pas les valeurs en j. Elle se referme sur
  `d_j` à 10⁻²¹ près (sonde P1 et P2), ce qui confirme l'exactitude de l'ensemble.

### 3.5 Remarques sur les contraintes (`stressesAt`) — RÉSERVE (mineure, documentaire)

- `τ_V = |V|/As`. `SIGN_CONVENTIONS.md` §5 la qualifie de « moyenne ». Or la moyenne est V/A : V/As vaut
  1,2 V/A pour un rectangle, alors que le maximum de Jourawski est 1,5 V/A.
  **Proposition** : écrire « contrainte conventionnelle V/As (ni moyenne ni maximale) ».
- `extremeNormal` = somme des valeurs absolues : majorant pour le cercle, d'un facteur jusqu'à √2. C'est conforme à
  `RESULTS_FORMAT.md` (« majorant sinon »).

## 4. Rotules : `StaticCondensation.cpp` et `Assembly.cpp` — CONFORME

- **Formules de condensation** (l. 34–72), conformes à McGuire, §4.6 :
  - `K_c = K_aa − K_ab K_bb⁻¹ K_ba` ;
  - `f_c = f_a − K_ab K_bb⁻¹ f_b` ;
  - `d_b = K_bb⁻¹ (f_b − K_ba d_a)`.
- **Inversibilité de K_bb**. Le bloc de flexion d'un plan, pour θ_i et θ_j, a pour déterminant
  `(4+Φ)² − (2−Φ)² = 12(1+Φ) > 0`. K_bb est donc définie positive pour toute combinaison des 4 relâchements, et les
  deux plans sont découplés.
- **Équivalence statique conservée**. Pour un mode rigide r, `K r = 0` donne `K_bb⁻¹ K_ba r_a = −r_b`, d'où
  `r_aᵀ f_c = r_aᵀ f_a + r_bᵀ f_b = rᵀ f`.
- **Cohérence de l'assemblage** :
  - `assembleStiffness` utilise `globalStiffnessMatrix()`, qui renvoie la rigidité condensée ;
  - `assembleLoads` condense avec la même `localStiffnessMatrix()` ;
  - `BeamResponse` complète d_b avec la matrice complète et les forces équivalentes **non condensées** (l. 59–67).

  L'ensemble est cohérent.
- **Vérification indépendante (sonde P2)**. Timoshenko encastré en i, rotules My et Mz en j, appui simple en j,
  charge triangulaire 0 → 12, comparé à la solution sympy du problème aux limites (w(0) = θ(0) = 0, w(L) = 0, M(L) = 0) :

  ```text
  code  x-y : f1 −5,37592137592  f5 −2,75184275184  f7 −6,62407862408  f11 0   θz_j(barre) −2,6416026416e-06
  exact x-y : −5,375921375921   −2,751842751843    −6,624078624079    0       −2,641602641603e-06
  code  x-z : f2 −5,17209302326  f4 2,34418604651   f8 −6,82790697674  f10 0   θy_j(barre)  3,15393133998e-07
  exact x-z : −5,172093023256   2,344186046512     −6,827906976744    0        3,153931339978e-07
  f_c = −f_a (d = 0) : 5,37592137592  2,75184275184  6,62407862408 ; v(L) ≈ 3e-21, Mz(L) ≈ 9e-16
  ```

## 5. `src/Sections/BeamSection3D.cpp` — sections

### 5.1 J du rectangle (formule de Roark) — CONFORME

- **Formule** (l. 225–228) : `J = a b³ [1/3 − 0,21 (b/a)(1 − b⁴/12a⁴)]`. C'est celle du tableau 10.1, cas 4, de
  Roark (2a × 2b, K = ab³[16/3 − 3,36 b/a (1 − b⁴/12a⁴)]), ramenée aux côtés complets.
- **Comparaison avec la série exacte** de Timoshenko–Goodier, §109 :

  | a/b | β exact | β code | écart |
  |---|---|---|---|
  | 1 | 0,14058 | 0,14083 | +0,18 % |
  | 2 | 0,22868 | 0,22888 | +0,09 % |
  | 4 | 0,28081 | 0,28085 | +0,01 % |
  | 10 | 0,31233 | 0,31233 | 0,00 % |

  L'annonce « erreur < 0,5 % » de `BeamSection3D.h` est vérifiée.

### 5.2 Wt du rectangle — RÉSERVE (non conservatif, jusqu'à −4 % sur τ)

- **Formule** (l. 229) : `Wt = a b² / (3 + 1,8 b/a)`. C'est la forme `Q = 8a²b²/(3a + 1,8b)` de Roark, ramenée aux
  côtés complets.
- **Comparaison avec la solution exacte**, `τ_max = Gθ′ b [1 − 8/π² Σ 1/(n² cosh(nπa/2b))]` et `T = Gθ′J`. Les
  valeurs α exactes redonnent les tables classiques : 0,208, 0,246, 0,267, 0,282, 0,299, 0,312.

  | a/b | α = Wt/ab² exact | code | erreur sur τ |
  |---|---|---|---|
  | 1 | 0,20817 | 0,20833 | −0,08 % |
  | 1,5 | 0,23097 | 0,23810 | −2,99 % |
  | 2 | 0,24588 | 0,25641 | **−4,11 %** |
  | 3 | 0,26721 | 0,27778 | −3,81 % |
  | 4 | 0,28167 | 0,28986 | −2,83 % |
  | 10 | 0,31233 | 0,31447 | −0,68 % |

- **Conséquence** : τ_T = T/Wt est **sous-estimé** de 3 à 4 % pour 1,5 ≤ a/b ≤ 3, les proportions courantes des
  poutres rectangulaires.
- **Proposition** (`BeamSection3D::rectangle`, l. 229) : prendre la forme polynomiale de Roark, tableau 10.1 cas 4,
  qui reste à 0,2 % près sur toute la plage (écart de signe variable, au plus +0,1 % sur α pour a/b = 3) :

  ```cpp
  // τ_max = 3T/(a b²) · [1 + 0,6095 r + 0,8865 r² − 1,8023 r³ + 0,9100 r⁴],  r = b/a (côtés complets a ≥ b)
  s.torsionModulus = a * b * b / (3.0 * (1.0 + 0.6095 * r + 0.8865 * r * r - 1.8023 * r * r * r + 0.9100 * r * r * r * r));
  ```

  Contrôles : α = 0,20786 pour a/b = 1 (exact 0,20817) ; α = 0,24546 pour a/b = 2 (exact 0,24588).

### 5.3 Cercle : coefficient de Cowper — CONFORME (code) ; ÉCART documentaire

- **Code** (l. 247–249) : `k = 6(1+ν)/(7+6ν)`, Cowper (1966), avec ν = 0,3, d'où k = **0,886**. J = πd⁴/32 et
  Wt = πd³/16 sont exacts.
- **ÉCART documentaire** : `include/Fem3D/Sections/BeamSection3D.h`, commentaire de `circle()` (l. 34), annonce
  « aires de cisaillement 0,9 A (coefficient de Timoshenko ν = 0,3) ». Or :
  - la valeur codée est 0,886 ;
  - le coefficient de Timoshenko (1922), `6(1+ν)²/(7+12ν+4ν²)`, vaudrait 0,925.

  **Proposition** : « aires de cisaillement 0,886 A (coefficient de Cowper 6(1+ν)/(7+6ν), ν = 0,3) ».
- **RÉSERVE mineure** : ν = 0,3 est figé, indépendamment du matériau. k varie de 0,857 (ν = 0) à 0,897 (ν = 0,45),
  soit au plus 3 % sur As, et seulement sur la flèche d'effort tranchant. On peut le documenter, ou prendre
  `ν = E/2G − 1`.

### 5.4 Rectangle : As = 5/6 A — CONFORME

- C'est la valeur classique (Reissner, approche énergétique).
- Cowper donnerait `10(1+ν)/(12+11ν)` = 0,850 pour ν = 0,3 : écart de 2 %, choix admissible. Le code mélange donc
  deux origines (5/6 pour le rectangle, Cowper pour le cercle). Ce n'est pas une erreur ; il faudrait le signaler
  dans THEORY.md §3.

## 6. `src/Solver/LinearSystemSolver.cpp` — critère de pivot et mécanismes

### 6.1 Faux positifs et dépendance aux unités — ÉCART (gravité moyenne)

**Critère actuel** (`LinearSystemSolver::factorize`) : mécanisme si `d_k ≤ 10⁻¹² · max_i |K_ii|`.

- La plus grande diagonale mêle des termes de dimensions différentes : F/L en translation, F·L/rad en rotation.
- Un même modèle donne donc des rapports `d_k / max K_ii` qui changent d'un facteur (unité de longueur)² selon les
  unités choisies.
- Un DDL **légitimement souple** est déclaré « mécanisme » dès que sa rigidité est inférieure à 10⁻¹² fois le terme
  le plus raide **de tout le modèle**, même si la matrice est parfaitement définie positive.

**Démonstration (sonde P4, code réel).** On factorise K_ff d'une console : un bloc 0,3 × 0,5 court, encastré, puis
une tige circulaire. Toutes ces structures sont stables :

```text
SPD diag(1, 1e-13) (aucun mécanisme)                             min d/maxDiag = 1,0e-13 -> MÉCANISME signalé
bloc L=0,05 m + tige d=6 mm L=12 m, kN-m                         min d/maxDiag = 1,5e-15 -> MÉCANISME signalé
bloc L=0,05 m + tige d=20 mm L=6 m, kN-m                         min d/maxDiag = 1,5e-12 -> accepté (de justesse)
même modèle, N-mm                                                min d/maxDiag = 1,7e-15 -> MÉCANISME signalé
bloc L=0,2 m + tige d=40 mm L=4 m, kN-m                          min d/maxDiag = 5,0e-09 -> accepté
même modèle, N-mm                                                min d/maxDiag = 3,8e-13 -> MÉCANISME signalé
```

Le **même modèle physique** est accepté en kN-m et refusé en N-mm. Les unités N-mm sont courantes dans les
bureaux d'études, et le moteur ne convertit rien (`SIGN_CONVENTIONS.md` §6).

**Correction proposée** (`LinearSystemSolver::factorize`, sans l'appliquer) :

- comparer chaque pivot à la diagonale **d'origine du même DDL**, dans l'ordre de permutation de la factorisation.
  C'est le « rapport de pivot » des codes industriels (par exemple MAXRATIO de Nastran) :
  `singulier si d_k ≤ τ · K_kk` (ou `K_kk / d_k > 1/τ`), avec τ ≈ 10⁻¹⁰…10⁻¹².
  - Le critère devient invariant par changement d'unités et par mise à l'échelle des DDL.
  - Il reste insensible aux autres éléments du modèle.
  - Avec `SimplicialLDLT`, il faut appliquer la permutation : `K_perm = P K Pᵀ` (`m_ldlt.permutationP()`).
- à défaut, équilibrer K par `S = diag(K_ii)^{-1/2}` avant le test, ce qui est équivalent au premier ordre.
- appliquer le même changement à `mechanismDofs`. Ses valeurs propres « nulles » se jugent aussi relativement à
  λ_max, donc avec la même dépendance aux unités. Il suffit de calculer les modes de `S K S`.

### 6.2 Faux négatifs — RÉSERVE (peu probables)

- **Vrai mécanisme** : son pivot calculé n'est fait que de l'erreur d'arrondi, de l'ordre de
  ε·(termes couplés)·(croissance). Sonde : −5,7·10⁻¹⁷ pour deux barres bi-articulées pliées (mécanisme hors plan).
  Si le pivot est exactement nul (barres alignées), `info() ≠ Success`. Dans les deux cas, la détection a lieu.
- **Cas limite** : un faux négatif exigerait une croissance de l'erreur supérieure à 10⁴, possible seulement pour des
  modèles très mal conditionnés.
- **Quasi-mécanisme** : un pivot relatif entre 10⁻¹² et environ 10⁻⁸ (par exemple deux barres bi-articulées quasi
  alignées, pli inférieur à 10⁻⁴ rad) est accepté sans avertissement. Pourtant, `κ(K) ≥ 10⁸` peut coûter 8 chiffres
  significatifs sur les déplacements.
- **Proposition** : un avertissement non bloquant (bande 10⁻¹² < rapport de pivot < 10⁻⁸). Ajouter aussi un contrôle
  du résidu `‖K_ff U_f − F‖ / ‖F‖` après résolution.

## 7. `src/Analysis/LinearStaticAnalysis.cpp` — réactions et équilibre

### 7.1 Réactions — CONFORME

- **Formule** (`LinearStaticAnalysis::run`) : `R = K_rf U_f + K_rr U_r − F_r`.
  - L'équilibre du nœud d'appui s'écrit `K u = F + R`.
  - F_r contient les charges nodales appliquées aux appuis et les forces équivalentes : R est bien la seule action de
    l'appui.
  - Le signe est conforme à `SIGN_CONVENTIONS.md` §3 : `Analytical_AxialTension` donne R = −P.
- **Déplacements imposés** : `K_ff U_f = F_f − K_fr U_r`, correct.

### 7.2 Contrôle d'équilibre — CONFORME (formules) ; RÉSERVE (portée)

- **Moment des charges trapézoïdales** (`checkEquilibrium`) :
  - formule : `∫₀ᴸ (P_i + s x̂) × q_g ds = P_i × R + x̂ × ∫ s q_g ds` ;
  - moment statique : `∫₀ᴸ s (q1 + (q2 − q1)s/L) ds = L² (q1/6 + q2/3)`, exact ;
  - la résultante `(q1 + q2)L/2` est exacte.
- **RÉSERVE** : ce contrôle est presque **tautologique** vis-à-vis de K. Soit r un mode rigide.
  - Pour tout élément, `rᵀ K_e = 0`, donc `rᵀ (K u − F) = −rᵀ F`.
  - Le contrôle ne vérifie donc que trois choses :
    1. les forces équivalentes ont la même résultante et le même moment que la charge réelle ;
    2. la résolution est précise (les résidus aux DDL libres sont nuls) ;
    3. les K_e ont bien leurs modes rigides.
  - Une erreur de rigidité qui garde les modes rigides le passerait. C'est le cas d'une mauvaise association
    Iz ↔ Asz, d'un mauvais Φ ou d'un mauvais J.
  - **Proposition** : le présenter comme tel dans THEORY.md §7. Ajouter le résidu relatif de la résolution, ainsi
    que la cohérence « intégration de la déformée en x = L = d_j » pour chaque élément : ce dernier contrôle est
    lui sensible à K et à f_eq (voir §8).

## 8. Adéquation des tests et cohérence des documents

### 8.1 Effets non couverts (RÉSERVE)

| # | Effet | Constat |
|---|---|---|
| T1 | Association **Iz ↔ Asy / Iy ↔ Asz** | Tous les tests utilisent `rectangle()` ou `circle()`, donc Asy = Asz. Une inversion Asy ↔ Asz (ou Φy ↔ Φz) passerait tous les tests (`Element_ExplicitStiffnessTerms` compris). **Ajouter** un test Timoshenko avec Asy ≠ Asz (par exemple Asy = 0,3 A, Asz = 0,8 A) : flèches de console dans les deux plans. |
| T2 | Timoshenko sous **charge non uniforme**, cas hyperstatique | Aucun test analytique : `FixedFixedTriangularLoad` est Euler–Bernoulli seulement. Couverture indirecte par `Integration_ResponseConsistentWithNodalDisplacements` (charges trapézoïdales sur un portique, fermeture de la déformée en x = L à 10⁻⁸). Ce contrôle est sensible à f_eq, mais avec Φ faible (≈ 0,02–0,09). **Ajouter** le cas P1 de ce rapport : Φ ≈ 0,8, `M_i = wL²(4+5Φ)/(120(1+Φ))`. |
| T3 | **Rotule + Timoshenko + charge triangulaire** | Non couvert. Le cas P2 de ce rapport fournit les valeurs exactes. |
| T4 | **Invariance du critère de mécanisme par changement d'unités** | Non couvert. Cas P4 : modèle kN-m accepté, N-mm refusé. |
| T5 | **Déplacement imposé** en rotation, ou avec Timoshenko et rotules | Seul un tassement en translation est testé, en Euler–Bernoulli. |
| T6 | **Torsion** en Timoshenko, **J ≠ Iy + Iz** dans une structure | Code commun, risque faible ; non testé en Timoshenko. |
| T7 | **Contraintes de torsion** (Wt) | Le test vérifie seulement `τ = T/Wt` avec le Wt du moteur, de façon tautologique. Aucune comparaison avec α tabulé (voir §5.2). |
| T8 | **Orientation par défaut des barres quasi verticales** | Non couvert (voir §2). |
| T9 | **Robustesse de la condensation** : rotules multiples avec charges réparties dans les deux plans et repère incliné | Partiellement couvert : `Releases_ResponseConsistentInFrame` couvre les deux formulations. |

### 8.2 Affirmations des documents confrontées au code

| # | Document | Affirmation | Verdict |
|---|---|---|---|
| D1 | `THEORY.md` §4 | « … f_eq est donc exactement l'opposé des forces d'encastrement parfait, et ce pour les deux formulations. Le test analytique `FixedFixedTriangularLoad` le vérifie : wL²/30, wL²/20, 3wL/20, 7wL/20. » | **ÉCART (documentaire)**. La propriété est **vraie** pour les deux formulations (§3.2), mais le test est Euler–Bernoulli seul (`Analytical_FixedFixedTriangularLoad_EulerBernoulli`). Pour Timoshenko, ces valeurs ne valent que pour Φ = 0 : en général, `M_i = wL²(4+5Φ)/(120(1+Φ))`, `M_j = wL²(6+5Φ)/(120(1+Φ))`, `R_i = wL(9+10Φ)/(60(1+Φ))`, `R_j = wL(21+20Φ)/(60(1+Φ))`. **Correction** : « Le test … le vérifie en Euler–Bernoulli ; pour Timoshenko, les forces d'encastrement dépendent de Φ (formules ci-dessus). » La même nuance vaut pour `VALIDATION_OPENSEES_3D.md` l. 148. |
| D2 | `THEORY.md` §6, Contrôles | « une rotule au droit d'un encastrement crée un mécanisme, qui est détecté » | **ÉCART (formulation)**. Ce n'est vrai que si la barre n'a pas d'autre maintien en rotation : console relâchée à l'encastrement, cas du test `Releases_HingeAtSupportCreatesMechanism`. Dans un portique, une rotule au droit d'un encastrement fait seulement de l'appui une articulation. **Correction** : « une console relâchée au droit de son encastrement est un mécanisme, qui est détecté ». |
| D3 | `THEORY.md` §7 | « un pivot non strictement positif, relatif au plus grand terme diagonal, déclenche le diagnostic » | Conforme au code ; le critère lui-même est en écart (§6.1). |
| D4 | `include/Fem3D/Sections/BeamSection3D.h`, `circle()` | « 0,9 A (coefficient de Timoshenko ν = 0,3) » | **ÉCART** : le code vaut 0,886 A (Cowper) (§5.3). |
| D5 | `include/Fem3D/Elements/EndReleases.h`, en-tête | renvoie à « docs/THEORY.md §7 » | **ÉCART mineur** : les relâchements sont au **§6** (le §7 traite de la résolution). |
| D6 | `SIGN_CONVENTIONS.md` §5 | « τ_V = \|V\|/As (moyenne) » | **ÉCART mineur** : V/As n'est pas la moyenne, qui vaut V/A (§3.5). |
| D7 | `SIGN_CONVENTIONS.md` §2 et §3, `THEORY.md` §2, §3 et §5 | repère, θy = −w′, signes des efforts, relations dM/dx, formules de reconstitution | **CONFORME**, vérifié par la dérivation indépendante (§1–3). |
| D8 | `BeamSection3D.h`, `rectangle()` | « J de Saint-Venant (série tronquée, erreur < 0,5 %) » | **CONFORME** (≤ 0,18 %) ; rien n'est affirmé sur Wt, qui est l'objet de la réserve §5.2. |

---

## Synthèse des écarts par gravité

**Gravité moyenne (corriger avant usage en bureau d'études)**

1. **Critère de mécanisme dépendant des unités et faux positifs** (§6.1). Fonction
   `LinearSystemSolver::factorize`, ainsi que `mechanismDofs`. Un modèle stable est refusé en N-mm et accepté en kN-m.
   Correction : rapport de pivot relatif à la diagonale propre du DDL, ou équilibrage diagonal.

**Gravité faible (résultats secondaires)**

2. **Wt du rectangle non conservatif**, τ de torsion sous-estimé jusqu'à 4,1 % (§5.2). `BeamSection3D.cpp` l. 229.
   Correction : polynôme de Roark.
3. **Contrôle d'équilibre aveugle aux erreurs de rigidité** (§7.2). Ce n'est pas une erreur de calcul, mais une
   portée à documenter, à compléter par un résidu de résolution.
4. **Quasi-mécanismes acceptés sans avertissement** (§6.2).
5. **ν = 0,3 figé** dans le coefficient de Cowper du cercle (§5.3), et orientation par défaut discontinue pour les
   barres quasi verticales (§2).

**Documentation**

6. D1 (THEORY §4 : triangulaire « pour les deux formulations » avec des valeurs Euler–Bernoulli et un test
   Euler–Bernoulli seul), D2, D4, D5, D6.

**Tests à ajouter (couverture)**

7. Les cas T1 (Asy ≠ Asz), T2 et T3 (Timoshenko sous charge triangulaire, avec et sans rotule ; valeurs exactes
   fournies en §3.2 et §4), T4 (unités) et T7 (α tabulé).

**Aucun écart** n'a été trouvé dans les formulations d'élément :

- matrices Euler–Bernoulli et Timoshenko, signes du plan x-z ;
- Φy/Φz et l'association Iz ↔ Asy, Iy ↔ Asz ;
- fonctions de forme d'Hermite et interdépendantes, toutes deux retrouvées exactement ;
- repère geomTransf et `K_g = Tᵀ K_l T` ;
- quadrature de Gauss (exacte) ;
- exactitude des forces équivalentes de Timoshenko pour toute charge linéaire, démontrée par Betti et vérifiée ;
- efforts internes et leurs signes ;
- intégration de la déformée ;
- condensation des rotules et reconstitution de d_b ;
- J du rectangle ;
- réactions.

## Suites données (ajouté le 2026-10-19 par l'intégrateur)

Les chemins et numéros de section ci-dessus sont ceux de la date de l'audit. Depuis, la documentation a été rangée en
sous-dossiers (`docs/theory/`, `docs/validation/`…) et THEORY.md a reçu un §7 (appuis élastiques) : la résolution est
désormais au §8.

| Constat | Suite |
|---|---|
| §6.1 Critère de mécanisme dépendant des unités | Corrigé : pivot comparé à la diagonale de son DDL ; modes propres de S K S. Test `Solver_MechanismCriterionIndependentOfUnits`, vérifié en échec avec l'ancien critère. |
| §5.2 Wt du rectangle | Corrigé : polynôme de Roark ; écart ≤ 0,3 % aux séries exactes de Saint-Venant, re-calculées indépendamment (l'ancienne forme : jusqu'à +4,4 %). Test `Section_RectangleTorsionMatchesExactSeries`. |
| §6.2 Quasi-mécanismes ; résidu | Corrigé : avertissement (pivot relatif < 1e-8, DDL désigné) et résidu relatif de résolution, exportés. Test `Solver_QuasiMechanismWarnsButSolves`. |
| §5.3 ν figé (Cowper) | Corrigé : paramètre `poisson` de `BeamSection3D::circle`. |
| §7.2 Portée du contrôle d'équilibre | Documentée (THEORY §8). |
| §2 Orientation par défaut discontinue | Documentée (UNITS_AND_COORDINATES §4, KNOWN_LIMITATIONS §4). |
| D1, D2, D4, D5, D6 | Corrigés. |
| T1, T2, T3, T4, T7 | Ajoutés (`tests/analytical/test_audit_gaps.cpp`). Les valeurs exactes de Timoshenko sous charge triangulaire ont été re-dérivées indépendamment (sympy) avant d'être utilisées. |
