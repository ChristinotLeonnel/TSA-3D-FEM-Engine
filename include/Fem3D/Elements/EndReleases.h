#pragma once

// Relâchements d'extrémité (rotules) des moments de flexion My et Mz, aux nœuds i et j de l'élément.
// Un moment relâché est nul à l'extrémité : la rotation correspondante de la barre devient indépendante de celle du
// nœud. Mise en œuvre par condensation statique des DDL locaux relâchés (Elements/StaticCondensation.h), identique
// pour Euler–Bernoulli et Timoshenko (docs/theory/THEORY.md §6).
// Ni l'effort normal, ni l'effort tranchant, ni la torsion ne sont relâchables dans cette version.

#include "Fem3D/Core/Types.h"

#include <array>

namespace TSA::Fem3D
{

struct EndReleases
{
    bool momentYStart = false;   ///< My = 0 au nœud i (DDL local θy_i, rang 4)
    bool momentZStart = false;   ///< Mz = 0 au nœud i (θz_i, rang 5)
    bool momentYEnd = false;     ///< My = 0 au nœud j (θy_j, rang 10)
    bool momentZEnd = false;     ///< Mz = 0 au nœud j (θz_j, rang 11)

    /// Rotules de flexion aux deux extrémités (barre bi-articulée en flexion, torsion transmise).
    static EndReleases pinnedBothEnds() { return { true, true, true, true }; }

    bool any() const { return momentYStart || momentZStart || momentYEnd || momentZEnd; }

    /// Masque des 12 DDL locaux relâchés.
    std::array<bool, kElementDofs> mask() const
    {
        std::array<bool, kElementDofs> m{};
        m[4] = momentYStart;
        m[5] = momentZStart;
        m[10] = momentYEnd;
        m[11] = momentZEnd;
        return m;
    }
};

} // namespace TSA::Fem3D
