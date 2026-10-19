# D. Éléments poutre : degrés de liberté, repères, transformations

## Degrés de liberté d'un élément

Deux nœuds, six DDL chacun, dans l'ordre [u v w θx θy θz] (repère local) — voir
[SIGN_CONVENTIONS.md](../theory/SIGN_CONVENTIONS.md) §1.

```mermaid
flowchart LR
    subgraph I["Nœud i (x = 0)"]
        i1["u_i : traction N"]
        i2["v_i, θz_i : flexion plan x-y (Iz, Asy)"]
        i3["w_i, θy_i : flexion plan x-z (Iy, Asz)"]
        i4["θx_i : torsion T (GJ)"]
    end
    subgraph J["Nœud j (x = L)"]
        j1["u_j"]
        j2["v_j, θz_j"]
        j3["w_j, θy_j"]
        j4["θx_j"]
    end
    I ---|"élément de longueur L<br/>d_e = [d_i ; d_j] (12 DDL)"| J
```

## Repère local et transformation

```mermaid
flowchart TD
    P["Coordonnées des nœuds X_i, X_j<br/>vecteur d'orientation v_xz"] --> X["x = (X_j − X_i) / L"]
    X --> Y["y = (v_xz × x) / ‖v_xz × x‖"]
    Y --> Z["z = x × y"]
    Z --> R["R : lignes x, y, z (3×3)<br/>a_local = R a_global"]
    R --> T["T = diag(R, R, R, R) (12×12)"]
    T --> KG["K_global = Tᵀ K_local T<br/>f_global = Tᵀ f_local"]
    P -.->|"v_xz absent"| D["Défaut : Z global,<br/>ou −X pour une barre verticale"]
    D --> Y
```

Code : [LocalCoordinateSystem3D.cpp](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Geometry/LocalCoordinateSystem3D.cpp),
[IBeamElement3D::globalStiffnessMatrix](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L18-L22).

## Hiérarchie des classes

```mermaid
classDiagram
    class IBeamElement3D {
        <<interface>>
        +localStiffnessMatrix() Matrix12
        +effectiveLocalStiffnessMatrix() Matrix12
        +globalStiffnessMatrix() Matrix12
        +releases() EndReleases
    }
    class BeamElementBase {
        +nœuds, axes, matériau, section
    }
    class EulerBernoulli3D {
        +rigidité exacte, Hermite
    }
    class Timoshenko3D {
        +Φy, Φz, interpolation interdépendante
    }
    IBeamElement3D <|-- BeamElementBase
    BeamElementBase <|-- EulerBernoulli3D
    BeamElementBase <|-- Timoshenko3D
```

Grandeurs physiques : N (EA/L), T (GJ/L, torsion de Saint-Venant uniforme, **sans gauchissement**), flexion
Mz, Vy (EIz, et G·Asy en Timoshenko) et My, Vz (EIy, G·Asz). Détails : livre, chapitres 6 à 8.
