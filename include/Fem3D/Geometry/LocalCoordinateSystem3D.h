#pragma once

// Repère local d'un élément poutre (convention de geomTransf d'OpenSees, docs/theory/SIGN_CONVENTIONS.md §2) :
//   x = (X_j − X_i) / L                          axe de l'élément, du nœud i vers le nœud j
//   y = (v_xz × x) / ‖v_xz × x‖                   v_xz : vecteur quelconque du plan local x-z
//   z = x × y
// Sans v_xz fourni : v_xz = Z global, sauf élément vertical (|x·Z| ≈ 1) : v_xz = −X global, de sorte qu'un poteau
// orienté vers +Z ait y = +Y et z = −X. Pour reproduire un modèle OpenSees, fournir le même v_xz que geomTransf.
//
// R (3×3) a pour lignes x, y, z exprimés dans le repère global : a_local = R a_global.
// T (12×12) = diag(R, R, R, R) : d_local = T d_global ; K_global = Tᵀ K_local T ; f_global = Tᵀ f_local.

#include "Fem3D/Core/Types.h"

#include <optional>

namespace TSA::Fem3D
{

class LocalCoordinateSystem3D
{
public:
    /// @throws Fem3DError (DegenerateGeometry) longueur nulle ou v_xz parallèle à l'axe de l'élément.
    LocalCoordinateSystem3D(const Vector3& start, const Vector3& end, const std::optional<Vector3>& vectorXZ = std::nullopt);

    double length() const { return m_length; }
    const Vector3& xAxis() const { return m_x; }
    const Vector3& yAxis() const { return m_y; }
    const Vector3& zAxis() const { return m_z; }

    /// R : vecteur global → local (3×3).
    const Matrix3& rotation() const { return m_rotation; }
    /// T : vecteur élémentaire global → local (12×12).
    Matrix12 transformation() const;

    /// Vecteur v_xz retenu (fourni ou par défaut).
    const Vector3& vectorXZ() const { return m_vectorXZ; }

private:
    double m_length = 0.0;
    Vector3 m_x, m_y, m_z, m_vectorXZ;
    Matrix3 m_rotation;
};

} // namespace TSA::Fem3D
