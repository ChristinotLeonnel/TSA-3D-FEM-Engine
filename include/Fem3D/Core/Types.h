#pragma once

// Types et constantes communs du moteur EF 3D (docs/theory/SIGN_CONVENTIONS.md).
//
// Ordre des degrés de liberté d'un nœud (repère global) ou d'une extrémité d'élément (repère local) :
//   0 u (translation x)   1 v (translation y)   2 w (translation z)
//   3 θx (rotation x)     4 θy (rotation y)      5 θz (rotation z)
// Vecteur élémentaire (12 DDL) : [u_i v_i w_i θx_i θy_i θz_i  u_j v_j w_j θx_j θy_j θz_j]ᵀ.

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <array>
#include <cstddef>

namespace TSA::Fem3D
{

using Vector3 = Eigen::Vector3d;
using Matrix3 = Eigen::Matrix3d;
using Vector12 = Eigen::Matrix<double, 12, 1>;
using Matrix12 = Eigen::Matrix<double, 12, 12>;

inline constexpr int kDofsPerNode = 6;
inline constexpr int kElementDofs = 12;

/// Rang d'un degré de liberté dans un nœud (0..5).
enum class Dof : int
{
    Ux = 0,
    Uy = 1,
    Uz = 2,
    Rx = 3,
    Ry = 4,
    Rz = 5
};

inline constexpr std::array<const char*, kDofsPerNode> kDofNames = { "Ux", "Uy", "Uz", "Rx", "Ry", "Rz" };

inline constexpr int index(Dof d) { return static_cast<int>(d); }

/// Tolérances géométriques et numériques (sans unité implicite : valeurs relatives, ou longueurs en unités du modèle).
struct Tolerance
{
    /// Longueur minimale d'un élément, relative à max(|coordonnées de ses deux extrémités|, 1) (LocalCoordinateSystem3D).
    static constexpr double relativeMinimumLength = 1e-9;
    /// |cos| au-delà duquel le vecteur d'orientation est jugé parallèle à l'axe de l'élément.
    static constexpr double parallelCosine = 1.0 - 1e-9;
    /// Pivot de factorisation jugé nul, relatif au terme diagonal de son propre DDL (mécanisme).
    static constexpr double relativePivot = 1e-12;
    /// Pivot relatif en dessous duquel un avertissement de quasi-mécanisme est émis (calcul poursuivi).
    static constexpr double weakPivot = 1e-8;
    /// Résidu relatif ‖K_ff U_f − F‖ / ‖F‖ au-delà duquel un avertissement est émis.
    static constexpr double solutionResidual = 1e-8;
};

} // namespace TSA::Fem3D
