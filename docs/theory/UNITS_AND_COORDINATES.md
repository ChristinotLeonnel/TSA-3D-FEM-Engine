# Unités et repères

## 1. Unités

Le moteur **ne convertit aucune unité**. Toutes les données d'un modèle doivent être exprimées dans un même système
cohérent, choisi par l'appelant et déclaré dans `UnitSystem` pour l'export JSON.

| Grandeur | Système des exemples et des tests | Autre système cohérent |
|---|---|---|
| Longueur, déplacement | m | mm |
| Force | kN | N |
| Moment | kN·m | N·mm |
| Module E, G ; contrainte | kN/m² (kPa) | N/mm² (MPa) |
| Aire, inertie | m², m⁴ | mm², mm⁴ |
| Charge répartie | kN/m | N/mm |
| Raideur de ressort | kN/m ; kN·m/rad | N/mm ; N·mm/rad |
| Rotation | rad | rad |

Le résultat ne dépend pas du système choisi, aux conversions près : c'est vérifié par
`Solver_MechanismCriterionIndependentOfUnits` (même modèle en kN-m et en N-mm, critère de mécanisme compris).

**Intégration dans TSA** (`Fem3DAdapter`) : TSA calcule avec Fem3D en **kN et m**. E est converti de Pa en kN/m²
(× 10⁻³), G = E / (2 (1 + ν)), les charges de TSA sont déjà en kN et en kN/m, les raideurs d'appui en kN/m et en
kN·m/rad : exactement les conversions du script OpenSees de TSA (`useKiloNewtons`).

## 2. Repère global

Repère cartésien direct (X, Y, Z). Les rotations sont des vecteurs, positives selon la règle de la main droite.
Aucune direction n'est privilégiée par le moteur : la gravité est une charge comme une autre (TSA l'applique selon −Z).

## 3. Degrés de liberté

Six DDL par nœud, dans cet ordre partout : Ux, Uy, Uz, Rx, Ry, Rz (repère global). Détails :
[SIGN_CONVENTIONS.md](SIGN_CONVENTIONS.md) §1.

## 4. Repère local d'une barre

Convention de `geomTransf` d'OpenSees ([SIGN_CONVENTIONS.md](SIGN_CONVENTIONS.md) §2) :

- x va du nœud i vers le nœud j ;
- y = (v_xz × x) / ‖v_xz × x‖, où v_xz est un vecteur du plan local x-z ;
- z = x × y.

Valeur par défaut de v_xz : Z global, ou −X pour une barre exactement verticale. **Limite** : pour une barre presque
verticale, le repère par défaut peut tourner brusquement de 90° selon le sens de l'inclinaison ; fournir v_xz
explicitement dans ce cas. TSA le fournit toujours : c'est le vecteur qu'il transmet à `geomTransf` pour OpenSees,
et l'adaptateur vérifie que Fem3D en déduit exactement les mêmes axes (sinon, le calcul est refusé).

Flexion : Iz gouverne le plan x-y (v, θz = dv/dx), Iy le plan x-z (w, θy = −dw/dx).

## 5. Abscisses le long d'une barre

Abscisse locale x ∈ [0, L], depuis le nœud i. Les charges réparties sont données par unité de **longueur réelle** de
la barre, de q(0) en i à q(L) en j. Les stations de résultats sont équidistantes, extrémités comprises.
