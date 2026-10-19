# Conventions : degrés de liberté, repères, signes, unités

## 1. Degrés de liberté

Chaque nœud a 6 DDL, dans cet ordre partout (matrices, transformations, charges, résultats) :

| Rang | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| Nœud (repère global) | Ux | Uy | Uz | Rx | Ry | Rz |
| Extrémité d'élément (repère local) | u | v | w | θx | θy | θz |

Vecteur élémentaire : `d_e = [u_i v_i w_i θx_i θy_i θz_i  u_j v_j w_j θx_j θy_j θz_j]ᵀ`.
Le DDL global vaut `g = 6 · indice interne du nœud + rang`. L'indice interne est la position du nœud dans le modèle,
jamais son identifiant (classe `DofMap`). Les rotations suivent la règle de la main droite.

## 2. Repère local d'un élément

Ce repère reprend la convention de `geomTransf` d'OpenSees :

- `x = (X_j − X_i) / L`, du nœud i vers le nœud j ;
- `y = (v_xz × x) / ‖v_xz × x‖`, où `v_xz` est un vecteur quelconque du plan local x-z (orientation de la section) ;
- `z = x × y`.

Sans `v_xz` fourni, on prend `v_xz = Z` global ; pour un élément vertical, `v_xz = −X` (poteau vers +Z : y = +Y,
z = −X). Un élément de longueur nulle, ou dont `v_xz` est parallèle à son axe, est refusé (`DegenerateGeometry`).

Rotation `R` (3×3), dont les lignes sont x, y, z exprimés en global : `a_local = R a_global`.
Transformation `T = diag(R, R, R, R)` (12×12) : `d_local = T d_global`, `K_global = Tᵀ K_local T`,
`f_global = Tᵀ f_local`.

Plans de flexion : `Iz` gouverne la flexion dans le plan x-y (v, θz), avec `θz = dv/dx` en Euler–Bernoulli ;
`Iy` gouverne la flexion dans le plan x-z (w, θy), avec `θy = −dw/dx`. Ce signe moins vient de la règle de la main
droite : une rotation positive autour de y fait descendre w quand x croît.

## 3. Efforts

**Efforts d'extrémité** (`endForcesLocal`, 12 valeurs) : forces et moments exercés **sur l'élément** par les nœuds
i et j, dans le repère local. On a `f = K_local d_local − f_eq`, où `f_eq` sont les forces nodales équivalentes des
charges réparties. C'est la convention de `localForce` d'OpenSees. Aux rotules, les moments relâchés valent exactement 0.

**Efforts internes** (stations, `N Vy Vz T My Mz`) : résultantes sur la **face positive** de la coupe (normale +x
local), dans le repère local.

| Effort | Signe positif | Relation |
|---|---|---|
| N | traction | `dN/dx = −q_x` |
| Vy, Vz | dans le sens de +y, +z sur la face positive | `dVy/dx = −q_y`, `dVz/dx = −q_z` |
| T | moment de torsion autour de +x | constant (pas de couple réparti en version 0.1) |
| My, Mz | moments autour de +y, +z | `dMz/dx = −Vy`, `dMy/dx = +Vz` |

Exemple : pour une console encastrée en x = 0 et chargée en bout par +Fz, on a `Vz = +F` et `My(x) = −F (L − x)`.
Pour une charge +Fy, on a `Vy = +F` et `Mz(x) = +F (L − x)`.

**Réactions** : forces exercées **par l'appui sur la structure**, dans le repère global. On les obtient par
`R = K_rf U_f + K_rr U_r − F_r`. Elles valent 0 sur les DDL libres d'un nœud d'appui.

**Forces des ressorts** (`springs`) : forces et moments exercés **par le ressort sur la structure**, dans le repère
global : `F = −k · u`. Un ressort tendu par un déplacement positif donne donc une force négative. L'équilibre global
s'écrit `ΣF_appliquées + ΣR + ΣF_ressorts = 0`.

## 4. Charges

- Charge nodale : force (Fx, Fy, Fz) et moment (Mx, My, Mz) dans le repère global.
- Charge répartie : force par unité de **longueur réelle** de l'élément, linéaire de `q(0)` à `q(L)` sur toute la
  longueur.
  - En repère local : composantes (qx, qy, qz) ; qx est axiale.
  - En repère global : composantes (qX, qY, qZ), projetées par `q_local = R q_global`. Une charge de gravité sur une
    barre inclinée est donc donnée par unité de longueur de la barre, et non par unité de longueur horizontale.
- Une charge répartie n'est jamais remplacée par une charge nodale approchée : elle donne des forces nodales
  équivalentes exactes et elle est reprise exactement dans les efforts internes et la déformée.

## 5. Déformations et contraintes

- Aux rotules, les DDL d'élément (`localDisplacements` de l'élément, stations) portent la **rotation de la barre**,
  distincte de la rotation du nœud.
- `localDisplacement` (u, v, w) et `sectionRotation` (θx, θy, θz) sont donnés en repère local ;
  `globalDisplacement` en repère global.
- `slopes` (dv/dx, dw/dx) : en Timoshenko, elles diffèrent des rotations de section. On a
  `dv/dx = θz + Vy/(G Asy)` et `dw/dx = −θy + Vz/(G Asz)`.
- Flèche locale (`maxDeflection`) : écart transversal √(v² + w²) à la corde reliant les extrémités déplacées. Elle
  est cherchée sur L/200.
- Contraintes : σ = N/A ; σ_flexion = |My| zExt/Iy et |Mz| yExt/Iz ; τ_V = |V|/As (contrainte de cisaillement de calcul ; la moyenne vraie sur la section vaut |V|/A) ; τ_T = |T|/Wt. Une valeur
  est absente si la donnée de section manque.

## 6. Unités

Le moteur ne convertit rien. L'utilisateur choisit un système cohérent (exemples et tests : kN, m, donc kN/m²,
kN·m, rad), déclaré dans `UnitSystem` pour l'export.
