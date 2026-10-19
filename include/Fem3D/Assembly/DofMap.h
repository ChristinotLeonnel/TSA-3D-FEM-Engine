#pragma once

// Numérotation des degrés de liberté. DDL global g = 6 · (indice interne du nœud) + rang (Core/Types.h) ; l'indice
// interne est la position du nœud dans le modèle, jamais son identifiant. Partition en DDL libres (f) et imposés (r).

#include "Fem3D/Core/Types.h"
#include "Fem3D/Model/StructuralModel3D.h"

#include <string>
#include <vector>

namespace TSA::Fem3D
{

class DofMap
{
public:
    explicit DofMap(const StructuralModel3D& model);

    int totalDofs() const { return static_cast<int>(m_partitionIndex.size()); }
    int freeDofs() const { return static_cast<int>(m_free.size()); }
    int restrainedDofs() const { return static_cast<int>(m_restrained.size()); }

    int globalDof(int nodeId, int rank) const { return 6 * static_cast<int>(m_model.nodeIndex(nodeId)) + rank; }
    bool isFree(int dof) const { return m_isFree[static_cast<std::size_t>(dof)]; }
    /// Rang du DDL dans sa partition (libre ou imposée).
    int partitionIndex(int dof) const { return m_partitionIndex[static_cast<std::size_t>(dof)]; }
    const std::vector<int>& freeList() const { return m_free; }
    const std::vector<int>& restrainedList() const { return m_restrained; }
    /// Valeur imposée d'un DDL de la partition r.
    double prescribedValue(int dof) const { return m_prescribed[static_cast<std::size_t>(dof)]; }

    /// « nœud 12, Rz » : désignation lisible d'un DDL global.
    std::string label(int dof) const;
    const StructuralModel3D& model() const { return m_model; }
    /// Les 12 DDL globaux d'un élément (i puis j).
    std::array<int, kElementDofs> elementDofs(int startNodeId, int endNodeId) const;

private:
    const StructuralModel3D& m_model;
    std::vector<bool> m_isFree;
    std::vector<int> m_partitionIndex;
    std::vector<int> m_free, m_restrained;
    std::vector<double> m_prescribed;
};

} // namespace TSA::Fem3D
