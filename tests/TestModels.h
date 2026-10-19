#pragma once

// Modèles de test communs (unités : kN, m).

#include "Fem3D/Analysis/LinearStaticAnalysis.h"
#include "Fem3D/Core/Fem3DError.h"

namespace Fem3DTest
{
using namespace TSA::Fem3D;

inline constexpr double kE = 210e6;     // kN/m²
inline constexpr double kG = 81e6;      // kN/m²

inline ElasticMaterial steel() { return ElasticMaterial{ "S", kE, kG }; }
inline BeamSection3D rectangle(double b = 0.3, double h = 0.5) { return BeamSection3D::rectangle("R", b, h); }

/// Console : nœud 1 encastré en (0,0,0), nœud 2 libre à l'extrémité de direction `axis` (longueur L).
inline StructuralModel3D cantilever(double length, BeamFormulation formulation, const Vector3& axis = Vector3::UnitX(),
                                    const BeamSection3D& section = rectangle(), std::optional<Vector3> vectorXZ = std::nullopt)
{
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(section);
    m.addNode(1, Vector3::Zero());
    m.addNode(2, length * axis.normalized());
    m.addElement({ 1, 1, 2, "S", section.name, formulation, vectorXZ });
    m.setRestraint(NodeRestraint::fullyFixed(1));
    return m;
}

inline double tipValue(const LoadCaseResults& r, int node, Dof dof)
{
    for (const auto& n : r.nodes)
        if (n.nodeId == node) return n.displacement[static_cast<std::size_t>(index(dof))];
    throw std::runtime_error("nœud absent des résultats");
}

} // namespace Fem3DTest
