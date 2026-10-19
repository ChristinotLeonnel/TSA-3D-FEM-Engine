#pragma once

// Conditions aux limites d'un nœud : chacun des 6 DDL (Core/Types.h) est libre ou imposé. Un DDL imposé a une
// valeur (0 pour un appui, ≠ 0 pour un déplacement imposé). Aucun appui n'est jamais transformé en encastrement.

#include "Fem3D/Core/Types.h"

#include <array>

namespace TSA::Fem3D
{

struct NodeRestraint
{
    int nodeId = 0;
    std::array<bool, kDofsPerNode> fixed{};       ///< DDL bloqué / imposé
    std::array<double, kDofsPerNode> prescribed{}; ///< valeur imposée (translation [L], rotation [rad])

    static NodeRestraint fullyFixed(int nodeId) { return { nodeId, { true, true, true, true, true, true }, {} }; }
    static NodeRestraint pinned(int nodeId) { return { nodeId, { true, true, true, false, false, false }, {} }; }
    static NodeRestraint custom(int nodeId, std::array<bool, kDofsPerNode> fixed) { return { nodeId, fixed, {} }; }
};

/// Appui élastique d'un nœud : un ressort linéaire découplé par DDL, dans le repère global, relié à un point fixe.
/// Raideurs ≥ 0 (0 : pas de ressort sur ce DDL) ; un ressort sur un DDL bloqué est refusé (réaction ambiguë).
/// Force exercée par le ressort sur la structure : −k · u (u : déplacement ou rotation du nœud).
struct NodalSpring
{
    int nodeId = 0;
    std::array<double, kDofsPerNode> stiffness{};   ///< kx ky kz [F/L], krx kry krz [F·L/rad]
};

} // namespace TSA::Fem3D
