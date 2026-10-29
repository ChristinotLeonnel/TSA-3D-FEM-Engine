# Théorie : poutres 3D d'Euler–Bernoulli et de Timoshenko

## 1. Cadre et hypothèses

- Analyse **statique linéaire** : petits déplacements, petites rotations, matériau élastique linéaire isotrope (E, G).
- Éléments poutre-colonne à **2 nœuds et 12 DDL**, de section constante, dont les axes principaux sont les axes
  locaux y et z (centre de gravité et centre de cisaillement confondus).
- Torsion de **Saint-Venant uniforme** : rigidité GJ/L. Le gauchissement non uniforme, le bimoment et la torsion
  mixte ne sont **pas** pris en compte.
- Le moteur ne traite **ni** la non-linéarité (géométrique, matérielle), **ni** la plasticité, **ni** la dynamique.

## 2. Euler–Bernoulli

Hypothèse : les sections restent planes et normales à la ligne moyenne, donc le cisaillement transversal est
négligé. Dans le plan x-y, on a θz = v′ et Mz = E Iz v″. Pour un élément prismatique :

```text
traction-compression  k = E A / L [ 1 −1 ; −1 1 ]                                  (u_i, u_j)
torsion               k = G J / L [ 1 −1 ; −1 1 ]                                  (θx_i, θx_j)
flexion plan x-y      k = E Iz / L³ [ 12  6L −12  6L ;  6L 4L² −6L 2L² ;
                                      −12 −6L 12 −6L ;  6L 2L² −6L 4L² ]          (v_i, θz_i, v_j, θz_j)
flexion plan x-z      même matrice avec Iy et les termes croisés w-θy de signe opposé (θy = −w′)
```

Les fonctions de forme sont les cubiques d'Hermite. Pour une poutre prismatique, cette rigidité est **exacte** : un
seul élément donne la solution exacte aux nœuds pour des charges nodales.

## 3. Timoshenko

On prend en compte la déformation de cisaillement transversal : γxy = v′ − θz = Vy / (G Asy) et
γxz = w′ + θy = Vz / (G Asz). Asy et Asz sont les aires effectives de cisaillement, par exemple 5/6 A pour un
rectangle.

Formulation retenue : l'**élément exact à interpolation interdépendante**, en forme fermée, avec

```text
Φy = 12 E Iz / (G Asy L²)     (plan x-y)          Φz = 12 E Iy / (G Asz L²)     (plan x-z)

k (plan x-y) = E Iz / ((1 + Φ) L³) [ 12  6L  −12  6L ;  6L (4+Φ)L²  −6L (2−Φ)L² ;
                                     −12 −6L  12 −6L ;  6L (2−Φ)L²  −6L (4+Φ)L² ]
N1 = μ (1 − 3ξ² + 2ξ³ + Φ (1 − ξ))        N2 = μ L (ξ − 2ξ² + ξ³ + Φ/2 (ξ − ξ²))
N3 = μ (3ξ² − 2ξ³ + Φ ξ)                  N4 = μ L (−ξ² + ξ³ − Φ/2 (ξ − ξ²))         μ = 1/(1+Φ), ξ = x/L
```

**Choix et verrouillage.** Cette rigidité résout exactement l'équation différentielle de Timoshenko pour une poutre
prismatique.

- Il n'y a **pas de verrouillage en cisaillement** : quand Φ → 0 (poutre élancée), on retrouve exactement
  Euler–Bernoulli.
- Il n'y a donc **pas d'intégration réduite** : la rigidité n'est pas intégrée numériquement.

Une interpolation indépendante linéaire (v, θ) intégrée exactement verrouillerait pour les poutres élancées. Le test
`Analytical_TimoshenkoDeepBeam_NoShearLocking` vérifie trois points :

- une seule poutre courte (L/h = 1) redonne la solution exacte FL³/3EI + FL/(G As) ;
- 10 éléments donnent la même valeur ;
- à L/h = 100, l'écart avec Euler–Bernoulli est inférieur à 0,01 %.

**Domaine de validité.** Timoshenko n'est pas « toujours meilleur ».

- Il améliore la flèche des poutres courtes ou de faible rigidité de cisaillement, si les aires effectives Asy et
  Asz sont correctes.
- Pour les poutres élancées, il coïncide avec Euler–Bernoulli.
- Pour les sections minces à fort gauchissement, ou les poutres très courtes (L/h < 1), les deux théories de poutre
  sont insuffisantes : un modèle de coque ou de volume est alors nécessaire.

## 4. Charges réparties

Forces nodales équivalentes : `f_eq,k = ∫₀ᴸ N_k(x) q(x) dx`, calculées avec les fonctions de forme **de la
formulation** et une quadrature de Gauss à 4 points. Celle-ci est exacte, car l'intégrande est un polynôme de degré
inférieur ou égal à 4.

Ces fonctions de forme sont les déformées exactes de la poutre bi-encastrée sous un déplacement unitaire d'une
extrémité. Par le théorème de Betti, `f_eq` est donc exactement l'opposé des forces d'encastrement parfait, et ce
pour les deux formulations, chacune avec ses propres valeurs. Pour une charge triangulaire (0 en i, w en j), les tests
analytiques retrouvent :

- en Euler–Bernoulli (`FixedFixedTriangularLoad_EulerBernoulli`) : wL²/30, wL²/20, 3wL/20 et 7wL/20 ;
- en Timoshenko (`FixedFixedTriangularLoad_Timoshenko`) : M_i = wL²(4 + 5Φ)/(120(1 + Φ)),
  M_j = wL²(6 + 5Φ)/(120(1 + Φ)) et R_i = wL(9 + 10Φ)/(60(1 + Φ)), qui redonnent les valeurs précédentes pour Φ = 0.

Ces valeurs de Timoshenko ont été dérivées indépendamment par résolution symbolique des équations de Timoshenko
(docs/verification/INDEPENDENT_FORMULATION_AUDIT.md).

### 4.1 Charges sur barre : partielles, ponctuelles, moments répartis (0.3.0)

Le même théorème de Betti donne les forces nodales équivalentes exactes de toute charge sur barre :

```text
charge répartie q sur [a, b] :   f_eq,k = ∫ₐᵇ N_k(x) q(x) dx            (Gauss à 4 points sur [a, b], exacte)
force ponctuelle P en x_P :      f_eq,k = N_k(x_P) P
moment réparti c sur [a, b] :    f_eq,k = ∫ₐᵇ Θ_k(x) · c(x) dx          (Gauss à 4 points, exacte : degré ≤ 3)
```

Θ_k est la **rotation de section** de la solution homogène associée au DDL k : la torsion θx est interpolée
linéairement (Saint-Venant) ; en flexion, Θz = v′ − γy et Θy = −w′ + γz, où la distorsion γ = V / (G As) est
constante sous des déplacements d'extrémité (V = −(K e_k) à l'extrémité i). En Euler–Bernoulli γ = 0 et Θz = v′.
Les dérivées N_k′ sont exactes : les fonctions de forme des deux formulations sont cubiques en ξ.

**Statut de vérification** : propriétés vérifiées numériquement dans le dépôt (barre bi-encastrée : la déformée
intégrée depuis i revient à zéro en j pour toutes ces charges, test `MemberLoads_ExactCompatibility_FixedFixed_AllLoadTypes` ;
solutions de résistance des matériaux ; invariance par subdivision) ; pas encore d'audit indépendant écrit.

## 5. Efforts internes et déformée

**Efforts d'extrémité** : `f = K_local d_local − f_eq`.

**Efforts internes** : ils découlent de l'équilibre du tronçon [0, x], avec la charge répartie q, le moment
réparti c et les forces ponctuelles P_k en x_k :

```text
N  = −f_ix − ∫₀ˣ qx − Σ P_kx      Vy = −f_iy − ∫₀ˣ qy − Σ P_ky      Vz = −f_iz − ∫₀ˣ qz − Σ P_kz      T = −m_ix − ∫₀ˣ cx
Mz = −m_iz + x f_iy + ∫₀ˣ (x − s) qy ds + Σ (x − x_k) P_ky − ∫₀ˣ cz        (dMz/dx = −Vy − cz)
My = −m_iy − x f_iz − ∫₀ˣ (x − s) qz ds − Σ (x − x_k) P_kz − ∫₀ˣ cy        (dMy/dx =  Vz − cy)
```

(Σ sur les forces telles que x_k ≤ x.) Entre deux abscisses de charge (bornes des charges réparties, forces
ponctuelles), ces efforts sont des polynômes (N et V de degré au plus 2, M de degré au plus 3) ; sur la barre, ils sont
**polynomiaux par morceaux** et exacts en toute abscisse. N et V sautent sous une force ponctuelle ; à son abscisse,
la valeur donnée est celle du côté j.

**Déformée** : elle est obtenue par intégration exacte de ces polynômes.

```text
θz = θz_i + ∫ Mz / E Iz        v = v_i + ∫ (θz + Vy / G Asy)
θy = θy_i + ∫ My / E Iy        w = w_i + ∫ (−θy + Vz / G Asz)
u  = u_i + ∫ N / E A           θx = θx_i + ∫ T / G J
```

En Euler–Bernoulli, les termes de cisaillement sont nuls. Ce n'est pas une interpolation arbitraire : c'est la
solution de la poutre pour les efforts exacts. L'intégration se fait morceau par morceau ; déplacements et rotations
de section restent continus (la pente v′ d'une poutre de Timoshenko saute avec V sous une force ponctuelle). Le test `ResponseConsistentWithNodalDisplacements` vérifie que
l'intégration retrouve, en x = L, le déplacement et la rotation calculés au nœud j.

## 6. Relâchements d'extrémité (rotules My, Mz)

Un moment relâché est nul à l'extrémité de la barre, dont la rotation devient indépendante de celle du nœud.

**Mise en œuvre** : condensation statique au niveau de l'élément, **commune aux deux formulations**, qui restent
inchangées. On note a les DDL locaux conservés et b les DDL relâchés (θy ou θz en i ou en j). La condition
`f_b = K_ba d_a + K_bb d_b − f_eq,b = 0` donne :

```text
d_b = K_bb⁻¹ (f_eq,b − K_ba d_a)             rotation de la barre à la rotule
K_c = K_aa − K_ab K_bb⁻¹ K_ba                 rigidité assemblée (lignes et colonnes b nulles)
f_c = f_eq,a − K_ab K_bb⁻¹ f_eq,b             forces nodales équivalentes assemblées
```

`K_bb`, une sous-matrice de flexion de dimension 1 à 4, est définie positive pour les deux formulations.

**Reconstitution de la réponse** : les efforts et la déformée sont calculés avec le vecteur complété (d_a, d_b). La
reconstitution exacte du §5 s'applique alors sans modification, et les moments d'extrémité relâchés sont nuls.

**Contrôles** :

- une rotule d'un seul côté redonne la console sur appui : k = 3EI/L³, 3EI/L², 3EI/L ; M = qL²/8 ; R = 5qL/8 et 3qL/8 ;
- des rotules aux deux extrémités donnent une poutre sur appuis simples : 5qL⁴/384EI ;
- une rotule intermédiaire (Gerber) donne (P/2) L³/3EI ;
- une rotule My au droit de l'encastrement d'une console sans autre appui crée un mécanisme (la barre tourne
  librement autour de la rotule), qui est détecté.

**Limites** : l'effort normal, l'effort tranchant et la torsion ne sont pas relâchables. Une rotule partielle
(ressort de rotation) n'est pas prise en charge.

## 7. Appuis élastiques (ressorts nodaux)

Un appui élastique relie un DDL global d'un nœud à un point fixe par un ressort linéaire de raideur k ≥ 0
(force [F/L] ou rotation [F·L/rad]). Les six ressorts d'un nœud sont **découplés** et orientés selon les **axes
globaux**. L'énergie de déformation ½ k u² ajoute simplement k au terme diagonal de ce DDL dans K. La force exercée
par le ressort sur la structure vaut −k·u. Elle est donnée à part des réactions des appuis rigides, et elle entre dans
le contrôle d'équilibre : ΣF_appliquées + ΣR + ΣF_ressorts = 0.

Règles :

- un ressort sur un DDL bloqué est refusé, car la répartition de la réaction serait ambiguë ;
- une raideur négative ou non finie est refusée ;
- une raideur nulle ne retient rien : un DDL retenu seulement par un ressort de raideur nulle reste un mécanisme,
  et il est désigné comme tel ;
- une structure peut ne reposer que sur des ressorts.

Solutions analytiques vérifiées par les tests (`tests/analytical/test_springs.cpp`) :

- ressort en bout de console : raideurs en parallèle, δ = F / (k_console + k), avec k_console = 1 / (L³/3EI) en
  Euler–Bernoulli et 1 / (L³/3EI + L/(G·As)) en Timoshenko ;
- pied articulé retenu par un ressort de rotation kθ : δ = F (L³/3EI [+ L/(G·As)] + L²/kθ), moment du ressort F·L ;
- barre sur un nœud entièrement élastique : u₁ = F/kx et u₂ = u₁ + FL/(EA).

**Limites** : pas de ressort couplé (matrice 6×6 pleine), ni orienté selon un repère local, ni non linéaire
(appui unilatéral, plastification). Pas de rotule semi-rigide en extrémité de barre (§6).

## 8. Résolution

On assemble les blocs partitionnés `K_ff`, `K_fr`, `K_rf` et `K_rr` (f : DDL libres, r : DDL imposés), puis on
résout `K_ff U_f = F_f − K_fr U_r` par factorisation creuse LDLᵀ (Eigen `SimplicialLDLT`, sans inversion
explicite). Les réactions valent `R = K_rf U_f + K_rr U_r − F_r`.

**Détection des mécanismes** : chaque pivot d_k de la factorisation P K_ff Pᵀ = L D Lᵀ est comparé au terme
diagonal de son propre DDL : d_k ≤ 10⁻¹² · K_kk signale un mécanisme. Ce rapport ne dépend pas des unités, alors
qu'une comparaison au plus grand terme diagonal de la matrice mélangerait des raideurs en F/L et en F·L. Le défaut
de cette première version (un modèle stable refusé en N-mm et accepté en kN-m) a été relevé par l'audit
indépendant ; il est corrigé et couvert par `Solver_MechanismCriterionIndependentOfUnits`. Les DDL concernés sont
désignés par les modes propres de valeur nulle de S K_ff S, avec S = diag(K_ff)^(−1/2), pour n ≤ 2000.

Un quasi-mécanisme (pivot faible mais au-dessus du seuil) est accepté sans avertissement : la limite est documentée
dans KNOWN_LIMITATIONS.md.

**Contrôle a posteriori** : on vérifie l'équilibre global des charges, charges réparties exactes comprises, et des
réactions (appuis et ressorts) : ΣF = 0 et ΣM = 0 par rapport à l'origine. **Portée** : ce contrôle vérifie
l'équivalence statique des forces nodales équivalentes et la précision de la résolution. Il ne vérifie pas la
rigidité : une matrice K fausse mais symétrique donnerait aussi un équilibre satisfait. La rigidité est validée par
les solutions analytiques et la comparaison à OpenSees.
