# Référence de l'API (générée)

Générée par `python tools/docs/gen_api_reference.py` à partir des en-têtes publics `include/Fem3D/` :
**ne pas modifier à la main**. Espace de noms : `TSA::Fem3D`. Les explications sont dans le
[livre](../book/Fem3D_Technical_Book.md) et le [guide développeur](../developer/DEVELOPER_GUIDE.md).

## Analysis

### `LinearStaticAnalysis.h`

[include/Fem3D/Analysis/LinearStaticAnalysis.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Analysis/LinearStaticAnalysis.h)

Analyse statique linéaire (petits déplacements, élasticité linéaire) : 1. contrôle du modèle ; 2. éléments finis et numérotation ; 3. assemblage et factorisation de K_ff (une fois) ; 4. par cas : F, U_f = K_ff⁻¹ (F_f − K_fr U_r), réactions R = K_rf U_f + K_rr U_r − F_r, efforts et déformées, contrôle de l'équilibre global. Ne traite ni la non-linéarité géométrique ou matérielle, ni la dynamique.

| Déclaration | Description |
|---|---|
| **struct AnalysisOptions** | — |
| `int stationsPerElement = 11;` | points de résultats le long de chaque élément (extrémités comprises) |
| **class LinearStaticAnalysis** | — |
| `explicit LinearStaticAnalysis(const StructuralModel3D& model, AnalysisOptions options = {});` | Contrôle, assemble et factorise. @throws Fem3DError (modèle invalide, mécanisme…) |
| `DenseVector loadVector(int loadCaseId) const;` | F global (DDL globaux) d'un cas, pour vérification ou export. |

## Assembly

### `DofMap.h`

[include/Fem3D/Assembly/DofMap.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Assembly/DofMap.h)

Numérotation des degrés de liberté. DDL global g = 6 · (indice interne du nœud) + rang (Core/Types.h) ; l'indice interne est la position du nœud dans le modèle, jamais son identifiant. Partition en DDL libres (f) et imposés (r).

| Déclaration | Description |
|---|---|
| **class DofMap** | — |
| `int partitionIndex(int dof) const { … }` | Rang du DDL dans sa partition (libre ou imposée). |
| `double prescribedValue(int dof) const { … }` | Valeur imposée d'un DDL de la partition r. |
| `std::string label(int dof) const;` | « nœud 12, Rz » : désignation lisible d'un DDL global. |
| `std::array<int, kElementDofs> elementDofs(int startNodeId, int endNodeId) const;` | Les 12 DDL globaux d'un élément (i puis j). |

### `GlobalAssembler.h`

[include/Fem3D/Assembly/GlobalAssembler.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Assembly/GlobalAssembler.h)

Assemblage de K U = F (méthode des déplacements) directement en blocs partitionnés : [ K_ff  K_fr ] [ U_f ]   [ F_f ] [ K_rf  K_rr ] [ U_r ] = [ F_r + R ] f : DDL libres, r : DDL imposés (U_r connus), R : réactions d'appui.

| Déclaration | Description |
|---|---|
| **struct PartitionedStiffness** | — |
| `SparseMatrix freeFree;` | K_ff |
| `SparseMatrix freeRestrained;` | K_fr |
| `SparseMatrix restrainedFree;` | K_rf |
| `SparseMatrix restrainedRestrained;` | K_rr |
| `SparseMatrix full;` | K complète (ordre des DDL globaux), pour export et vérification |
| **class GlobalAssembler** | — |
| `DenseVector assembleLoads(const LoadCase3D& loadCase) const;` | F global (DDL globaux) d'un cas : charges nodales + forces nodales équivalentes des charges réparties. |

## Constraints

### `BoundaryConditions.h`

[include/Fem3D/Constraints/BoundaryConditions.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Constraints/BoundaryConditions.h)

Conditions aux limites d'un nœud : chacun des 6 DDL (Core/Types.h) est libre ou imposé. Un DDL imposé a une valeur (0 pour un appui, ≠ 0 pour un déplacement imposé). Aucun appui n'est jamais transformé en encastrement.

| Déclaration | Description |
|---|---|
| **struct NodeRestraint** | — |
| `std::array<bool, kDofsPerNode> fixed{};` | DDL bloqué / imposé |
| `std::array<double, kDofsPerNode> prescribed{};` | valeur imposée (translation [L], rotation [rad]) |
| **struct NodalSpring** | Appui élastique d'un nœud : un ressort linéaire découplé par DDL, dans le repère global, relié à un point fixe. Raideurs ≥ 0 (0 : pas de ressort sur ce DDL) ; un ressort sur un DDL bloqué est refusé (réaction ambiguë). Force exercée par le ressort sur la structure : −k · u (u : déplacement ou rotation du nœud). |
| `std::array<double, kDofsPerNode> stiffness{};` | kx ky kz [F/L], krx kry krz [F·L/rad] |

## Core

### `Fem3DError.h`

[include/Fem3D/Core/Fem3DError.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Fem3DError.h)

Erreur diagnostique du moteur : un code stable (testable) et un message lisible.

| Déclaration | Description |
|---|---|
| **enum class ErrorCode** | — |
| **class Fem3DError** | — |

### `Polynomial.h`

[include/Fem3D/Core/Polynomial.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Core/Polynomial.h)

Polynôme p(x) = c0 + c1 x + c2 x² + … (abscisse locale x de l'élément, 0 ≤ x ≤ L). Les efforts internes sous charges réparties linéaires sont polynomiaux (N, V : degré ≤ 2 ; M : degré ≤ 3) : leurs intégrales (rotations, déplacements) sont calculées exactement, sans quadrature.

| Déclaration | Description |
|---|---|
| **class Polynomial** | — |
| `Polynomial integral() const` | Primitive nulle en x = 0 : ∫₀ˣ p(s) ds. |
| `Polynomial timesX() const` | Produit par le monôme x. |

### `Types.h`

`include/Fem3D/Core/Types.h` (modifié depuis la révision de référence 75700d2)

Types et constantes communs du moteur EF 3D (docs/theory/SIGN_CONVENTIONS.md). Ordre des degrés de liberté d'un nœud (repère global) ou d'une extrémité d'élément (repère local) : 0 u (translation x)   1 v (translation y)   2 w (translation z) 3 θx (rotation x)     4 θy (rotation y)      5 θz (rotation z) Vecteur élémentaire (12 DDL) : [u_i v_i w_i θx_i θy_i θz_i  u_j v_j w_j θx_j θy_j θz_j]ᵀ.

| Déclaration | Description |
|---|---|
| **enum class Dof** | Rang d'un degré de liberté dans un nœud (0..5). |
| **struct Tolerance** | Tolérances géométriques et numériques (sans unité implicite : valeurs relatives, ou longueurs en unités du modèle). |

## Elements

### `BeamElementBase.h`

[include/Fem3D/Elements/BeamElementBase.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/BeamElementBase.h)

Données communes d'un élément poutre (identifiants, repère local, matériau, section). Les formulations dérivent de cette base et n'y ajoutent que leurs équations.

| Déclaration | Description |
|---|---|
| **class BeamElementBase** | — |

### `EndReleases.h`

[include/Fem3D/Elements/EndReleases.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/EndReleases.h)

Relâchements d'extrémité (rotules) des moments de flexion My et Mz, aux nœuds i et j de l'élément. Un moment relâché est nul à l'extrémité : la rotation correspondante de la barre devient indépendante de celle du nœud. Mise en œuvre par condensation statique des DDL locaux relâchés (Elements/StaticCondensation.h), identique pour Euler–Bernoulli et Timoshenko (docs/theory/THEORY.md §6). Ni l'effort normal, ni l'effort tranchant, ni la torsion ne sont relâchables dans cette version.

| Déclaration | Description |
|---|---|
| **struct EndReleases** | — |
| `bool momentYStart = false;` | My = 0 au nœud i (DDL local θy_i, rang 4) |
| `bool momentZStart = false;` | Mz = 0 au nœud i (θz_i, rang 5) |
| `bool momentYEnd = false;` | My = 0 au nœud j (θy_j, rang 10) |
| `bool momentZEnd = false;` | Mz = 0 au nœud j (θz_j, rang 11) |
| `static EndReleases pinnedBothEnds() { return { … }` | Rotules de flexion aux deux extrémités (barre bi-articulée en flexion, torsion transmise). |
| `std::array<bool, kElementDofs> mask() const` | Masque des 12 DDL locaux relâchés. |

### `EulerBernoulli3D.h`

[include/Fem3D/Elements/EulerBernoulli3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/EulerBernoulli3D.h)

Poutre 3D d'Euler–Bernoulli (sections planes et normales à la ligne moyenne : cisaillement transversal négligé). Rigidité exacte pour une poutre prismatique élastique ; fonctions de forme cubiques d'Hermite (docs/theory/THEORY.md §2).

| Déclaration | Description |
|---|---|
| **class EulerBernoulli3D** | — |

### `IBeamElement3D.h`

[include/Fem3D/Elements/IBeamElement3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/IBeamElement3D.h)

Interface commune des éléments poutre-colonne 3D à deux nœuds et 12 DDL (docs/theory/THEORY.md). Une formulation fournit : sa matrice de rigidité locale, ses fonctions de forme (charges consistantes) et ses flexibilités de cisaillement (nulles pour Euler–Bernoulli). Le reste (repère, assemblage, efforts internes) est commun et ne dépend que de ces trois éléments.

| Déclaration | Description |
|---|---|
| **enum class BeamFormulation** | — |
| **struct BeamShapeValues** | Valeurs des fonctions de forme en ξ = x / L. |
| `std::array<double, 2> axial{};` | u(x)  = Σ axial[k]  · (u_i, u_j)[k] |
| `std::array<double, 4> bendingXY{};` | v(x)  = Σ bendingXY[k] · (v_i, θz_i, v_j, θz_j)[k] |
| `std::array<double, 4> bendingXZ{};` | w(x)  = Σ bendingXZ[k] · (w_i, θy_i, w_j, θy_j)[k]   (θy = −dw/dx) |
| **class IBeamElement3D** | — |
| `virtual Matrix12 localStiffnessMatrix() const = 0;` | Matrice de rigidité 12×12 dans le repère local (ordre des DDL : Core/Types.h). |
| `virtual BeamShapeValues shapeFunctions(double xi) const = 0;` | Fonctions de forme exactes de la formulation en ξ ∈ [0, 1]. |
| `virtual double shearFlexibilityY() const = 0;` | Flexibilités de cisaillement 1 / (G Asy) et 1 / (G Asz) ; 0 pour Euler–Bernoulli. |
| `virtual const EndReleases& releases() const = 0;` | Rotules de flexion aux extrémités (aucune par défaut). |
| `Matrix12 effectiveLocalStiffnessMatrix() const;` | Rigidité locale effective : celle de la formulation, condensée aux DDL relâchés s'il y a des rotules. |
| `Matrix12 globalStiffnessMatrix() const;` | K_global = Tᵀ K_effective T (12×12). |

### `StaticCondensation.h`

[include/Fem3D/Elements/StaticCondensation.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/StaticCondensation.h)

Condensation statique des DDL locaux relâchés d'un élément (indices a : conservés, b : relâchés, repère local). L'effort aux DDL relâchés est nul : f_b = K_ba d_a + K_bb d_b − f_eq,b = 0, d'où d_b = K_bb⁻¹ (f_eq,b − K_ba d_a)                                (rotations de la barre aux rotules) K_c = K_aa − K_ab K_bb⁻¹ K_ba                                    (rigidité condensée, lignes/colonnes b nulles) f_c = f_eq,a − K_ab K_bb⁻¹ f_eq,b                                (forces nodales équivalentes condensées) K_bb (sous-matrice de flexion, 1 à 4 DDL) est définie positive pour les deux formulations.

| Déclaration | Description |
|---|---|
| `Vector12 completeReleasedDisplacements(const Matrix12& stiffness, const Vector12& displacements, const Vector12& loads, const DofMask& released);` | Vecteur de DDL complété : d_a inchangés, d_b remplacés par les rotations de la barre aux rotules. |

### `Timoshenko3D.h`

[include/Fem3D/Elements/Timoshenko3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/Timoshenko3D.h)

Poutre 3D de Timoshenko (déformation de cisaillement transversal, aires effectives Asy, Asz). Élément exact à interpolation interdépendante : rigidité en forme fermée avec les paramètres Φy = 12 E Iz / (G Asy L²)   (flexion dans le plan x-y)      Φz = 12 E Iy / (G Asz L²)   (plan x-z) Solution exacte de l'équation différentielle de Timoshenko pour une poutre prismatique : pas de verrouillage en cisaillement (Φ → 0 redonne Euler–Bernoulli), pas d'intégration réduite (docs/theory/THEORY.md §3).

| Déclaration | Description |
|---|---|
| **class Timoshenko3D** | — |
| `double shearParameterY() const;` | Paramètres de cisaillement Φy (plan x-y) et Φz (plan x-z), sans dimension. |

## Geometry

### `LocalCoordinateSystem3D.h`

[include/Fem3D/Geometry/LocalCoordinateSystem3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Geometry/LocalCoordinateSystem3D.h)

Repère local d'un élément poutre (convention de geomTransf d'OpenSees, docs/theory/SIGN_CONVENTIONS.md §2) : x = (X_j − X_i) / L                          axe de l'élément, du nœud i vers le nœud j y = (v_xz × x) / ‖v_xz × x‖                   v_xz : vecteur quelconque du plan local x-z z = x × y Sans v_xz fourni : v_xz = Z global, sauf élément vertical (|x·Z| ≈ 1) : v_xz = −X global, de sorte qu'un poteau orienté vers +Z ait y = +Y et z = −X. Pour reproduire un modèle OpenSees, fournir le même v_xz que geomTransf. R (3×3) a pour lignes x, y, z exprimés dans le repère global : a_local = R a_global. T (12×12) = diag(R, R, R, R) : d_local = T d_global ; K_global = Tᵀ K_local T ; f_global = Tᵀ f_local.

| Déclaration | Description |
|---|---|
| **class LocalCoordinateSystem3D** | — |
| `LocalCoordinateSystem3D(const Vector3& start, const Vector3& end, const std::optional<Vector3>& vectorXZ = std::nullopt);` | @throws Fem3DError (DegenerateGeometry) longueur nulle ou v_xz parallèle à l'axe de l'élément. |
| `const Matrix3& rotation() const { … }` | R : vecteur global → local (3×3). |
| `Matrix12 transformation() const;` | T : vecteur élémentaire global → local (12×12). |
| `const Vector3& vectorXZ() const { … }` | Vecteur v_xz retenu (fourni ou par défaut). |

### `Node3D.h`

[include/Fem3D/Geometry/Node3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Geometry/Node3D.h)

| Déclaration | Description |
|---|---|
| **struct Node3D** | Nœud du modèle : identifiant utilisateur (libre, non contigu) et coordonnées globales. L'indice interne des degrés de liberté est attribué par DofMap, jamais déduit de l'identifiant. |

## Loads

### `Loads3D.h`

[include/Fem3D/Loads/Loads3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Loads/Loads3D.h)

Chargements (docs/theory/SIGN_CONVENTIONS.md §4). Unités cohérentes avec le modèle : force [F], moment [F·L], charge répartie [F/L] (par unité de longueur réelle de l'élément).

| Déclaration | Description |
|---|---|
| **struct NodalLoad3D** | Force et moment appliqués à un nœud, dans le repère global. |
| `Vector3 force = Vector3::Zero();` | Fx, Fy, Fz |
| `Vector3 moment = Vector3::Zero();` | Mx, My, Mz (règle de la main droite) |
| **enum class LoadFrame** | — |
| **struct DistributedLoad3D** | Charge répartie sur toute la longueur d'un élément, linéaire entre q(0) = startIntensity et q(L) = endIntensity (uniforme si les deux sont égales). Jamais remplacée par une approximation nodale : ses forces nodales équivalentes sont celles de la formulation, et elle est reprise exactement dans les efforts internes et la déformée. |
| **struct LoadCase3D** | — |

## Materials

### `ElasticMaterial.h`

[include/Fem3D/Materials/ElasticMaterial.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Materials/ElasticMaterial.h)

Matériau élastique linéaire isotrope. Unités cohérentes choisies par l'utilisateur (ex. kN, m → kN/m²) : le moteur ne convertit rien.

| Déclaration | Description |
|---|---|
| **struct ElasticMaterial** | — |
| `double youngModulus = 0.0;` | E  [force / longueur²] |
| `double shearModulus = 0.0;` | G  [force / longueur²] |
| `static ElasticMaterial fromPoisson(std::string name, double youngModulus, double poissonRatio);` | G = E / (2 (1 + ν)). |
| `void validate() const;` | @throws Fem3DError (InvalidInput) E ou G non finis ou ≤ 0. |

## Model

### `StructuralModel3D.h`

[include/Fem3D/Model/StructuralModel3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Model/StructuralModel3D.h)

Modèle structurel : données saisies (nœuds, définitions d'éléments, matériaux, sections, appuis, cas de charge). Il ne contient aucune équation ; LinearStaticAnalysis en crée les éléments finis selon leur formulation.

| Déclaration | Description |
|---|---|
| **struct ElementDefinition** | — |
| `std::optional<Vector3> vectorXZ;` | orientation de la section (repère local), défaut : LocalCoordinateSystem3D |
| `EndReleases releases;` | rotules de flexion aux extrémités (aucune par défaut) |
| **class StructuralModel3D** | — |
| `void setRestraint(const NodeRestraint& restraint);` | Remplace les conditions d'un nœud déjà restreint. |
| `void setSpring(const NodalSpring& spring);` | Remplace l'appui élastique d'un nœud qui en a déjà un. |
| `std::unique_ptr<IBeamElement3D> createElement(const ElementDefinition& definition) const;` | Élément fini de la formulation choisie (repère local construit à partir des coordonnées réelles). |
| `void validate() const;` | Contrôle complet avant calcul : références, propriétés, géométrie, charges. @throws Fem3DError avec un message qui désigne l'objet fautif. |

## Results

### `AnalysisResults.h`

[include/Fem3D/Results/AnalysisResults.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h)

Résultats d'une analyse statique linéaire, par cas de charge (docs/theory/SIGN_CONVENTIONS.md §5). Valeurs non disponibles (fibre extrême, module de torsion non fournis) : std::nullopt, jamais une valeur inventée.

| Déclaration | Description |
|---|---|
| **struct NodalResult** | — |
| `std::array<double, kDofsPerNode> displacement{};` | Ux Uy Uz [L], Rx Ry Rz [rad], repère global |
| **struct ReactionResult** | — |
| `std::array<double, kDofsPerNode> force{};` | Fx Fy Fz [F], Mx My Mz [F·L], repère global, exercées par l'appui |
| **struct SpringResult** | — |
| `std::array<double, kDofsPerNode> force{};` | −k · u : forces et moments exercés PAR le ressort sur la structure, repère global |
| **struct SectionForces** | Efforts internes sur la face positive de la coupe (normale +x local), repère local : N > 0 traction ; dMz/dx = −Vy ; dMy/dx = +Vz. |
| `double axial = 0.0;` | N   [F] |
| `double shearY = 0.0;` | Vy  [F] |
| `double shearZ = 0.0;` | Vz  [F] |
| `double torsion = 0.0;` | T   [F·L] |
| `double momentY = 0.0;` | My  [F·L] |
| `double momentZ = 0.0;` | Mz  [F·L] |
| **struct SectionStresses** | — |
| `double axial = 0.0;` | N / A |
| `std::optional<double> bendingY;` | \|My\| zExtreme / Iy |
| `std::optional<double> bendingZ;` | \|Mz\| yExtreme / Iz |
| `std::optional<double> extremeNormal;` | \|N/A\| + flexions (exact pour un rectangle, majorant sinon) |
| `double shearY = 0.0;` | \|Vy\| / Asy (ou / A sans Asy) : contrainte de calcul (la moyenne vraie vaut \|Vy\| / A) |
| `double shearZ = 0.0;` | \|Vz\| / Asz (ou / A) |
| `std::optional<double> torsion;` | \|T\| / Wt |
| `std::optional<double> vonMisesIndicative;` | √(σ_extrême² + 3 τ_torsion²) : indicatif, non réglementaire |
| **struct StationResult** | — |
| `double position = 0.0;` | abscisse locale x ∈ [0, L] |
| `Vector3 localDisplacement = Vector3::Zero();` | u v w (repère local) |
| `Vector3 sectionRotation = Vector3::Zero();` | θx θy θz (repère local) |
| `double slopeXY = 0.0;` | dv/dx (= θz en Euler–Bernoulli) |
| `double slopeXZ = 0.0;` | dw/dx (= −θy en Euler–Bernoulli) |
| `Vector3 globalDisplacement = Vector3::Zero();` | Ux Uy Uz du point de la ligne moyenne |
| **struct ElementResult** | — |
| `Vector12 endForcesLocal = Vector12::Zero();` | forces exercées SUR l'élément aux nœuds i et j, repère local |
| `double maxTransverseDeflection = 0.0;` | max √(v² + w²) relatif à la corde (flèche locale) |
| **struct EquilibriumCheck** | — |
| `Vector3 appliedMoment = Vector3::Zero();` | par rapport à l'origine globale |
| `Vector3 reactionForce = Vector3::Zero();` | appuis rigides et ressorts |
| `double forceResidual = 0.0;` | ‖ΣF_appliquées + ΣR + ΣF_ressorts‖ |
| `double solutionResidual = 0.0;` | ‖K_ff U_f − (F_f − K_fr U_r)‖ / ‖F_f − K_fr U_r‖ (précision de la résolution) |
| **struct LoadCaseResults** | — |
| `std::vector<SpringResult> springs;` | appuis élastiques (vide sans ressort) |
| `std::vector<std::string> warnings;` | avertissements non bloquants (quasi-mécanisme, résidu élevé) |
| **struct AnalysisResults** | — |

### `BeamResponse.h`

[include/Fem3D/Results/BeamResponse.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/BeamResponse.h)

Réponse d'un élément : efforts aux extrémités, efforts internes et déformée le long de l'élément. Efforts aux extrémités (repère local, forces exercées sur l'élément) : f = K_local d_local − f_eq, où f_eq sont les forces nodales équivalentes de la charge répartie (= forces d'encastrement parfait changées de signe). Avec des rotules, d_local est d'abord complété par les rotations de la barre aux extrémités relâchées (Elements/StaticCondensation.h). Efforts internes par l'équilibre du tronçon [0, x] (charge q(s) = a + b s par composante) : N(x)  = −f_ix − ∫₀ˣ qx        Vy(x) = −f_iy − ∫₀ˣ qy        Vz(x) = −f_iz − ∫₀ˣ qz        T(x) = −m_ix Mz(x) = −m_iz + x f_iy + ∫₀ˣ (x − s) qy ds                    My(x) = −m_iy − x f_iz − ∫₀ˣ (x − s) qz ds Déformée, intégration exacte des polynômes (Timoshenko : termes de cisaillement ; Euler–Bernoulli : nuls) : u = u_i + ∫ N/EA      θx = θx_i + ∫ T/GJ θz = θz_i + ∫ Mz/EIz   v = v_i + ∫ (θz + Vy/(G Asy)) θy = θy_i + ∫ My/EIy   w = w_i + ∫ (−θy + Vz/(G Asz))

| Déclaration | Description |
|---|---|
| **struct LocalLinearLoad** | Charge répartie linéaire en composantes locales : q(x) = start + (end − start) x / L. |
| `static LocalLinearLoad fromLoads(const IBeamElement3D& element, const std::vector<const DistributedLoad3D*>& loads);` | Somme des charges réparties d'un élément, projetées dans son repère local. |
| `Vector12 equivalentNodalLoads(const IBeamElement3D& element, const LocalLinearLoad& load);` | Forces nodales équivalentes (repère local, 12) : f_eq,k = ∫₀ᴸ N_k(x) q(x) dx avec les fonctions de forme de la formulation (exactes : égales aux forces d'encastrement parfait changées de signe). Quadrature de Gauss exacte (intégrande polynomiale de degré ≤ 4). |
| **class BeamResponse** | — |
| `Vector3 displacementAt(double x) const;` | u v w local |
| `Vector3 rotationAt(double x) const;` | θx θy θz local |
| `ElementResult result(int stationCount) const;` | Résultat complet de l'élément, à stationCount points équidistants (≥ 2) ; flèche maximale cherchée sur une subdivision fine (L / 200). |

### `ResultsJsonWriter.h`

[include/Fem3D/Results/ResultsJsonWriter.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/ResultsJsonWriter.h)

Format d'échange des résultats « tsa-fem3d-results/1 » (docs/RESULTS_FORMAT.md) : fichier JSON autonome lu par le post-traitement (sous-projet postprocessor/) sans dépendre du moteur.

| Déclaration | Description |
|---|---|
| **struct UnitSystem** | — |

## Sections

### `BeamSection3D.h`

[include/Fem3D/Sections/BeamSection3D.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Sections/BeamSection3D.h)

Propriétés d'une section de poutre, constantes le long de l'élément (axes principaux = axes locaux y, z). A            aire                                       [longueur²] Iy, Iz       moments quadratiques autour de y et de z   [longueur⁴]  (Iz : flexion dans le plan x-y) J            constante de torsion de Saint-Venant      [longueur⁴]  (≠ Iy + Iz sauf section circulaire) Asy, Asz     aires effectives de cisaillement selon y et z (Timoshenko uniquement) Facultatifs, pour les contraintes (non inventés s'ils manquent) : yExtreme, zExtreme   distances des fibres extrêmes au centre de gravité torsionModulus       module de torsion Wt (τ_max = T / Wt)

| Déclaration | Description |
|---|---|
| **struct BeamSection3D** | — |
| `static BeamSection3D rectangle(std::string name, double widthY, double heightZ);` | Section rectangulaire b (selon y) × h (selon z), aires de cisaillement 5/6 A, J de Saint-Venant (série tronquée, erreur < 0,5 %) et Wt correspondant. |
| `static BeamSection3D circle(std::string name, double diameter, double poisson = 0.3);` | Section circulaire pleine de diamètre d, aires de cisaillement k A avec le coefficient de Cowper k = 6 (1 + ν) / (7 + 6 ν) (k ≈ 0,886 pour ν = 0,3). La section ne connaît pas le matériau : passer son ν. |
| `void validate(bool requiresShearAreas) const;` | @throws Fem3DError (InvalidInput) propriété mécanique non finie ou ≤ 0 ; aires de cisaillement si requises. |

## Solver

### `LinearSystemSolver.h`

[include/Fem3D/Solver/LinearSystemSolver.h](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Solver/LinearSystemSolver.h)

Résolution de K_ff U_f = F_f par factorisation LDLᵀ creuse (Eigen::SimplicialLDLT), sans inversion explicite. Factorisation unique, réutilisée pour tous les cas de charge. Un pivot nul, négatif ou négligeable devant le terme diagonal de son propre DDL (critère indépendant des unités) signale un mécanisme : les DDL de ses modes de rigidité nulle (matrice équilibrée par sa diagonale) sont désignés dans le message. Aucun DDL libre (tous imposés) : modèle valable, la réponse ne dépend que des déplacements imposés et des charges.

| Déclaration | Description |
|---|---|
| **class LinearSystemSolver** | — |
| `void factorize(const SparseMatrix& stiffness, const std::function<std::string(int)>& dofLabel);` | @param dofLabel désignation d'un DDL de la partition libre (indice 0..n_f−1), pour les diagnostics. @throws Fem3DError (Mechanism) |
| `double minimumPivotRatio() const { … }` | Plus petit rapport pivot / diagonale propre (1 sans DDL libre) et indice libre du DDL concerné (−1 sinon). Un rapport faible (< Tolerance::weakPivot) signale un quasi-mécanisme : le calcul reste possible mais peut perdre jusqu'à −log10(rapport) chiffres significatifs. |
