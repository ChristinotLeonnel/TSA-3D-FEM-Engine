# Algorithmes

Pseudo-code des algorithmes du moteur, fidèle au code de la révision de référence
[75700d2](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/commit/75700d2bfd33c29b2fadc795fd3051f25399ad19).
Les équations sont justifiées dans [THEORY.md](../theory/THEORY.md) et dans le [livre](../book/Fem3D_Technical_Book.md).
Notations : f = DDL libres, r = DDL imposés, n_f = nombre de DDL libres.

## 1. Analyse statique linéaire

Code : [constructeur](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L14-L33),
[run](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115).

```text
constructeur(modèle):
    modèle.validate()                                  # références, propriétés, géométrie, charges, ressorts
    pour chaque définition d'élément : créer l'élément de sa formulation (repère local, matrices)
    DofMap(modèle)                                     # g = 6·indice + rang ; partition f / r
    K ← assembler (§2) ; factoriser K_ff (§4)          # une seule fois pour tous les cas
    si pivot relatif minimal < 1e-8 : mémoriser l'avertissement de quasi-mécanisme

run(cas):
    F ← assembler les charges du cas (§3)
    b ← F_f − K_fr U_r                                 # U_r : valeurs imposées
    U_f ← résoudre K_ff U_f = b
    résidu ← ‖K_ff U_f − b‖ / ‖b‖                      # avertissement si > 1e-8
    R ← K_rf U_f + K_rr U_r − F_r                      # réactions des appuis rigides
    pour chaque ressort : F_ressort ← −k · u
    pour chaque élément : d_local ← T d_global ; réponse exacte (§6)
    contrôle d'équilibre (§7) ; avertissements ; résultats
```

## 2. Assemblage de la rigidité

Code : [assembleStiffness](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L55-L101),
[effectiveLocalStiffnessMatrix](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L13-L16),
[globalStiffnessMatrix](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L18-L22).

```text
pour chaque élément e:
    K_eff ← K_local ; si rotules : K_eff ← condensation (§5)
    K_e ← Tᵀ K_eff T                                   # 12×12, repère global
    pour a, b dans 0..11 (termes non nuls):
        ga, gb ← DDL globaux ; ajouter K_e[a,b] au bloc (ff, fr, rf ou rr) de (ga, gb) et à K complète
pour chaque ressort (nœud, DDL d, raideur k > 0):     # DDL libre (garanti par validate)
    ajouter k au terme diagonal (d, d) de K_ff et de K
construire les matrices creuses (les doublons sont sommés)
```

## 3. Vecteur des charges

Code : [assembleLoads](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Assembly/Assembly.cpp#L103-L126),
[equivalentNodalLoads](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L31-L51).

```text
charges nodales : ajoutées directement (repère global)
pour chaque élément chargé:
    q(x) ← somme des charges réparties, projetées dans le repère local (q_local = R q_global)
    f_eq ← ∫₀ᴸ Nᵀ(x) q(x) dx par Gauss à 4 points    # exact : intégrande de degré ≤ 4
    si rotules : f_eq ← f_a − K_ab K_bb⁻¹ f_b          # charges condensées
    F ← F + Tᵀ f_eq
```

## 4. Factorisation et diagnostic de stabilité

Code : [factorize](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L59-L98),
[mechanismDofs](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L33-L56),
[diagonalScaling](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L21-L30).
Diagramme : [stabilite.md](../diagrams/stabilite.md).

```text
si n_f = 0 : rien à factoriser (modèle valable)
P K_ff Pᵀ = L D Lᵀ (SimplicialLDLT, permutation AMD)
diag_perm ← P · diag(K_ff)                             # rigidité propre de chaque ligne permutée
pour k : singulier si diag_perm[k] ≤ 0 ou D[k] ≤ 1e-12 · diag_perm[k]
         sinon mémoriser min(D[k] / diag_perm[k]) et son DDL (P⁻¹)
si singulier:
    si n_f ≤ 2000 : S ← diag(K_ff)^(−1/2) ; valeurs propres de S K_ff S ;
                    pour chaque valeur ≤ 1e-12 · max : DDL dont |composante| ≥ 0,3 · max du vecteur
    lever Fem3DError(Mechanism, liste des DDL)
```

Le rapport D[k] / K_kk est invariant par changement d'unités : passer de kN·m à N·mm multiplie D[k] et K_kk par le
même facteur.

## 5. Condensation statique des rotules

Code : [condensedStiffness](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L34-L45),
[condensedLoads](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L47-L58),
[completeReleasedDisplacements](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/StaticCondensation.cpp#L60-L72).

```text
a ← DDL conservés, b ← DDL relâchés (θy et/ou θz en i et/ou j)
K_c ← K_aa − K_ab K_bb⁻¹ K_ba                         # lignes et colonnes b mises à zéro
f_c ← f_a − K_ab K_bb⁻¹ f_b
après résolution : d_b ← K_bb⁻¹ (f_b − K_ba d_a)       # rotation de la barre à la rotule
```

## 6. Réponse exacte d'un élément

Code : [BeamResponse](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98),
[result](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L124-L165).

```text
si rotules : compléter d_local par d_b (§5)
f ← K_local d_local − f_eq                            # efforts exercés SUR l'élément (convention localForce)
efforts internes par l'équilibre du tronçon [0, x] (polynômes exacts) :
    N = −f_ix − ∫ qx ; Vy = −f_iy − ∫ qy ; Vz = −f_iz − ∫ qz ; T = −m_ix
    Mz = −m_iz + x f_iy + ∫ (x − s) qy ds ; My = −m_iy − x f_iz − ∫ (x − s) qz ds
déformée par intégration exacte des polynômes :
    u = u_i + ∫ N/EA ; θx = θx_i + ∫ T/GJ ; θz = θz_i + ∫ Mz/EIz ; θy = θy_i + ∫ My/EIy
    v = v_i + ∫ (θz + Vy/(G Asy)) ; w = w_i + ∫ (−θy + Vz/(G Asz))   # termes de cisaillement nuls en EB
stations équidistantes (11 par défaut) ; contraintes RDM ; flèche maximale relative à la corde (pas L/200)
```

## 7. Contrôle d'équilibre global

Code : [checkEquilibrium](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L124-L170).

```text
ΣF_app, ΣM_app ← charges nodales + résultante exacte de chaque charge répartie
                  (moment : P_i × R + x̂ × ∫ s q(s) ds)
ΣR, ΣM_R ← réactions rigides + forces des ressorts (moments en r × F + M)
résidus ← ‖ΣF_app + ΣR‖, ‖ΣM_app + ΣM_R‖
```

Portée : ce contrôle vérifie l'équivalence statique des charges et la précision de la résolution, **pas** la rigidité.

## 8. Comparaison à OpenSees

Code : [main du programme de comparaison](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L531-L731).
Diagramme : [validation-opensees.md](../diagrams/validation-opensees.md).

```text
échelles : s_T = max|translation|, s_F = max|force|, s_R = max(max|rotation|, s_T / L_max),
           s_M = max(max|moment|, s_F · L_max)
pour chaque valeur (a = Fem3D, b = OpenSees) : OK si |a − b| ≤ max(1e-6·|b|, 1e-9·échelle de sa nature)
statut du cas : OK si toutes les valeurs sont OK ; NON COMPARABLE si pas de référence exacte
```
