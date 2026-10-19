#pragma once

// Condensation statique des DDL locaux relâchés d'un élément (indices a : conservés, b : relâchés, repère local).
// L'effort aux DDL relâchés est nul : f_b = K_ba d_a + K_bb d_b − f_eq,b = 0, d'où
//   d_b = K_bb⁻¹ (f_eq,b − K_ba d_a)                                (rotations de la barre aux rotules)
//   K_c = K_aa − K_ab K_bb⁻¹ K_ba                                    (rigidité condensée, lignes/colonnes b nulles)
//   f_c = f_eq,a − K_ab K_bb⁻¹ f_eq,b                                (forces nodales équivalentes condensées)
// K_bb (sous-matrice de flexion, 1 à 4 DDL) est définie positive pour les deux formulations.

#include "Fem3D/Core/Types.h"

#include <array>

namespace TSA::Fem3D
{

using DofMask = std::array<bool, kElementDofs>;

Matrix12 condensedStiffness(const Matrix12& stiffness, const DofMask& released);
Vector12 condensedLoads(const Matrix12& stiffness, const Vector12& loads, const DofMask& released);
/// Vecteur de DDL complété : d_a inchangés, d_b remplacés par les rotations de la barre aux rotules.
Vector12 completeReleasedDisplacements(const Matrix12& stiffness, const Vector12& displacements, const Vector12& loads, const DofMask& released);

} // namespace TSA::Fem3D
