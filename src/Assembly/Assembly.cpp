#include "Fem3D/Assembly/GlobalAssembler.h"

#include "Fem3D/Elements/StaticCondensation.h"
#include "Fem3D/Results/BeamResponse.h"

namespace TSA::Fem3D
{

DofMap::DofMap(const StructuralModel3D& model)
    : m_model(model)
{
    const std::size_t total = 6 * model.nodes().size();
    m_isFree.assign(total, true);
    m_partitionIndex.assign(total, -1);
    m_prescribed.assign(total, 0.0);
    for (const auto& [nodeId, r] : model.restraints())
        for (int k = 0; k < kDofsPerNode; ++k)
            if (r.fixed[static_cast<std::size_t>(k)])
            {
                const int g = globalDof(nodeId, k);
                m_isFree[static_cast<std::size_t>(g)] = false;
                m_prescribed[static_cast<std::size_t>(g)] = r.prescribed[static_cast<std::size_t>(k)];
            }
    for (int g = 0; g < static_cast<int>(total); ++g)
    {
        auto& list = m_isFree[static_cast<std::size_t>(g)] ? m_free : m_restrained;
        m_partitionIndex[static_cast<std::size_t>(g)] = static_cast<int>(list.size());
        list.push_back(g);
    }
}

std::string DofMap::label(int dof) const
{
    const auto& node = m_model.nodes()[static_cast<std::size_t>(dof / 6)];
    return "nœud " + std::to_string(node.id) + ", " + kDofNames[static_cast<std::size_t>(dof % 6)];
}

std::array<int, kElementDofs> DofMap::elementDofs(int startNodeId, int endNodeId) const
{
    std::array<int, kElementDofs> dofs{};
    for (int k = 0; k < kDofsPerNode; ++k)
    {
        dofs[static_cast<std::size_t>(k)] = globalDof(startNodeId, k);
        dofs[static_cast<std::size_t>(k + 6)] = globalDof(endNodeId, k);
    }
    return dofs;
}

GlobalAssembler::GlobalAssembler(const DofMap& dofs, const std::vector<std::unique_ptr<IBeamElement3D>>& elements)
    : m_dofs(dofs)
    , m_elements(elements)
{
}

PartitionedStiffness GlobalAssembler::assembleStiffness() const
{
    using Triplet = Eigen::Triplet<double>;
    std::vector<Triplet> ff, fr, rf, rr, full;
    const std::size_t reserve = m_elements.size() * kElementDofs * kElementDofs;
    full.reserve(reserve);
    ff.reserve(reserve);
    for (const auto& element : m_elements)
    {
        const Matrix12 k = element->globalStiffnessMatrix();
        const auto dofs = m_dofs.elementDofs(element->startNodeId(), element->endNodeId());
        for (int a = 0; a < kElementDofs; ++a)
            for (int b = 0; b < kElementDofs; ++b)
            {
                const double value = k(a, b);
                if (value == 0.0) continue;
                const int ga = dofs[static_cast<std::size_t>(a)], gb = dofs[static_cast<std::size_t>(b)];
                full.emplace_back(ga, gb, value);
                const int pa = m_dofs.partitionIndex(ga), pb = m_dofs.partitionIndex(gb);
                const bool fa = m_dofs.isFree(ga), fb = m_dofs.isFree(gb);
                (fa ? (fb ? ff : fr) : (fb ? rf : rr)).emplace_back(pa, pb, value);
            }
    }
    // Appuis élastiques : ressort entre le DDL et un point fixe → terme diagonal k (DDL libre, cf. validate)
    for (const auto& [nodeId, spring] : m_dofs.model().springs())
        for (int d = 0; d < kDofsPerNode; ++d)
        {
            const double value = spring.stiffness[static_cast<std::size_t>(d)];
            if (value == 0.0) continue;
            const int g = m_dofs.globalDof(nodeId, d), p = m_dofs.partitionIndex(g);
            full.emplace_back(g, g, value);
            ff.emplace_back(p, p, value);
        }
    const int nf = m_dofs.freeDofs(), nr = m_dofs.restrainedDofs(), n = m_dofs.totalDofs();
    PartitionedStiffness k;
    k.freeFree.resize(nf, nf);
    k.freeRestrained.resize(nf, nr);
    k.restrainedFree.resize(nr, nf);
    k.restrainedRestrained.resize(nr, nr);
    k.full.resize(n, n);
    k.freeFree.setFromTriplets(ff.begin(), ff.end());   // doublons sommés : contributions des éléments voisins
    k.freeRestrained.setFromTriplets(fr.begin(), fr.end());
    k.restrainedFree.setFromTriplets(rf.begin(), rf.end());
    k.restrainedRestrained.setFromTriplets(rr.begin(), rr.end());
    k.full.setFromTriplets(full.begin(), full.end());
    return k;
}

DenseVector GlobalAssembler::assembleLoads(const LoadCase3D& loadCase) const
{
    DenseVector f = DenseVector::Zero(m_dofs.totalDofs());
    for (const NodalLoad3D& load : loadCase.nodalLoads)
        for (int k = 0; k < 3; ++k)
        {
            f(m_dofs.globalDof(load.nodeId, k)) += load.force(k);
            f(m_dofs.globalDof(load.nodeId, k + 3)) += load.moment(k);
        }
    for (const auto& element : m_elements)
    {
        const LocalElementLoads loads = LocalElementLoads::fromLoadCase(*element, loadCase);
        if (loads.empty()) continue;
        // f_global = Tᵀ f_local ; rotules : forces équivalentes condensées (aucune charge transmise aux DDL relâchés)
        Vector12 local = equivalentNodalLoads(*element, loads);
        if (element->releases().any()) local = condensedLoads(element->localStiffnessMatrix(), local, element->releases().mask());
        const Vector12 global = element->localAxes().transformation().transpose() * local;
        const auto dofs = m_dofs.elementDofs(element->startNodeId(), element->endNodeId());
        for (int a = 0; a < kElementDofs; ++a) f(dofs[static_cast<std::size_t>(a)]) += global(a);
    }
    return f;
}

} // namespace TSA::Fem3D
