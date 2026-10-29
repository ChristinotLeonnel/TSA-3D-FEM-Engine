#include "Fem3D/Analysis/LinearStaticAnalysis.h"

#include "Fem3D/Core/Fem3DError.h"
#include "Fem3D/Results/BeamResponse.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace TSA::Fem3D
{

LinearStaticAnalysis::LinearStaticAnalysis(const StructuralModel3D& model, AnalysisOptions options)
    : m_model(model)
    , m_options(options)
{
    model.validate();
    for (const auto& definition : model.elements()) m_elements.push_back(model.createElement(definition));
    m_dofs = std::make_unique<DofMap>(model);
    m_assembler = std::make_unique<GlobalAssembler>(*m_dofs, m_elements);
    m_stiffness = m_assembler->assembleStiffness();
    m_solver.factorize(m_stiffness.freeFree, [this](int i) { return m_dofs->label(m_dofs->freeList()[static_cast<std::size_t>(i)]); });
    if (m_solver.minimumPivotDof() >= 0 && m_solver.minimumPivotRatio() < Tolerance::weakPivot)
    {
        std::ostringstream os;
        os << "conditionnement faible (quasi-mécanisme possible) : pivot relatif minimal " << std::scientific << std::setprecision(2)
           << m_solver.minimumPivotRatio() << " au DDL " << m_dofs->label(m_dofs->freeList()[static_cast<std::size_t>(m_solver.minimumPivotDof())])
           << " ; les résultats peuvent perdre jusqu'à " << static_cast<int>(std::ceil(-std::log10(m_solver.minimumPivotRatio())))
           << " chiffres significatifs";
        m_conditioningWarning = os.str();
    }
}

DenseVector LinearStaticAnalysis::loadVector(int loadCaseId) const
{
    return m_assembler->assembleLoads(m_model.loadCase(loadCaseId));
}

LoadCaseResults LinearStaticAnalysis::run(int loadCaseId) const
{
    const LoadCase3D& loadCase = m_model.loadCase(loadCaseId);
    const DofMap& dofs = *m_dofs;
    const DenseVector f = m_assembler->assembleLoads(loadCase);

    DenseVector fFree(dofs.freeDofs()), fRestrained(dofs.restrainedDofs()), uRestrained(dofs.restrainedDofs());
    for (int i = 0; i < dofs.freeDofs(); ++i) fFree(i) = f(dofs.freeList()[static_cast<std::size_t>(i)]);
    for (int i = 0; i < dofs.restrainedDofs(); ++i)
    {
        const int g = dofs.restrainedList()[static_cast<std::size_t>(i)];
        fRestrained(i) = f(g);
        uRestrained(i) = dofs.prescribedValue(g);
    }
    // K_ff U_f = F_f − K_fr U_r
    const DenseVector rhs = fFree - m_stiffness.freeRestrained * uRestrained;
    const DenseVector uFree = m_solver.solve(rhs);
    const double rhsNorm = rhs.norm();
    const double solutionResidual = rhsNorm > 0.0 ? (m_stiffness.freeFree * uFree - rhs).norm() / rhsNorm : 0.0;
    // R = K_rf U_f + K_rr U_r − F_r
    const DenseVector reactions = m_stiffness.restrainedFree * uFree + m_stiffness.restrainedRestrained * uRestrained - fRestrained;

    DenseVector u(dofs.totalDofs());
    for (int i = 0; i < dofs.freeDofs(); ++i) u(dofs.freeList()[static_cast<std::size_t>(i)]) = uFree(i);
    for (int i = 0; i < dofs.restrainedDofs(); ++i) u(dofs.restrainedList()[static_cast<std::size_t>(i)]) = uRestrained(i);

    LoadCaseResults out;
    out.loadCaseId = loadCase.id;
    out.loadCaseName = loadCase.name;
    for (const Node3D& node : m_model.nodes())
    {
        NodalResult n{ node.id, {} };
        for (int k = 0; k < kDofsPerNode; ++k) n.displacement[static_cast<std::size_t>(k)] = u(dofs.globalDof(node.id, k));
        out.nodes.push_back(n);
    }
    for (const auto& [nodeId, restraint] : m_model.restraints())
    {
        ReactionResult r{ nodeId, {} };
        for (int k = 0; k < kDofsPerNode; ++k)
        {
            const int g = dofs.globalDof(nodeId, k);
            if (!dofs.isFree(g)) r.force[static_cast<std::size_t>(k)] = reactions(dofs.partitionIndex(g));
        }
        out.reactions.push_back(r);
    }
    for (const auto& [nodeId, spring] : m_model.springs())
    {
        SpringResult s{ nodeId, {} };
        for (int k = 0; k < kDofsPerNode; ++k)
            s.force[static_cast<std::size_t>(k)] = -spring.stiffness[static_cast<std::size_t>(k)] * u(dofs.globalDof(nodeId, k));
        out.springs.push_back(s);
    }
    for (const auto& element : m_elements)
    {
        Vector12 global;
        const auto elementDofs = dofs.elementDofs(element->startNodeId(), element->endNodeId());
        for (int a = 0; a < kElementDofs; ++a) global(a) = u(elementDofs[static_cast<std::size_t>(a)]);
        const Vector12 local = element->localAxes().transformation() * global;   // d_local = T d_global
        const BeamResponse response(*element, local, LocalElementLoads::fromLoadCase(*element, loadCase));
        out.elements.push_back(response.result(m_options.stationsPerElement));
    }
    out.equilibrium = checkEquilibrium(loadCase, out.reactions, out.springs);
    out.equilibrium.solutionResidual = solutionResidual;
    if (!m_conditioningWarning.empty()) out.warnings.push_back(m_conditioningWarning);
    if (!(solutionResidual <= Tolerance::solutionResidual))
    {
        std::ostringstream os;
        os << "résidu de résolution élevé : " << std::scientific << std::setprecision(2) << solutionResidual << " (seuil "
           << Tolerance::solutionResidual << ")";
        out.warnings.push_back(os.str());
    }
    return out;
}

AnalysisResults LinearStaticAnalysis::runAll() const
{
    AnalysisResults results;
    for (const LoadCase3D& c : m_model.loadCases()) results.loadCases.push_back(run(c.id));
    return results;
}

EquilibriumCheck LinearStaticAnalysis::checkEquilibrium(const LoadCase3D& loadCase, const std::vector<ReactionResult>& reactions,
                                                        const std::vector<SpringResult>& springs) const
{
    // Résultante et moment (par rapport à l'origine) des charges appliquées, charges sur barre exactes comprises,
    // puis des réactions : leur somme doit être nulle.
    EquilibriumCheck c;
    for (const NodalLoad3D& l : loadCase.nodalLoads)
    {
        const Vector3& p = m_model.node(l.nodeId).position;
        c.appliedForce += l.force;
        c.appliedMoment += l.moment + p.cross(l.force);
    }
    for (const auto& element : m_elements)
    {
        const LocalElementLoads loads = LocalElementLoads::fromLoadCase(*element, loadCase);
        if (loads.empty()) continue;
        const Matrix3 toGlobal = element->localAxes().rotation().transpose();
        const Vector3 start = m_model.node(element->startNodeId()).position, axis = element->localAxes().xAxis();
        // Charge linéaire sur [a, b] : résultante (q_a + q_b)(b − a)/2 ; moment exact ∫ (P(s) × q(s)) ds = P_i × R + x × ∫ s q(s) ds,
        // avec ∫ₐᵇ s q(s) ds = a R + (b − a)² (q_a/6 + q_b/3)
        for (const LocalLoadSegment& s : loads.distributed)
        {
            const double length = s.end - s.start;
            const Vector3 resultant = toGlobal * (0.5 * length * (s.startIntensity + s.endIntensity));
            const Vector3 firstMoment = s.start * resultant + toGlobal * (length * length * (s.startIntensity / 6.0 + s.endIntensity / 3.0));
            c.appliedForce += resultant;
            c.appliedMoment += start.cross(resultant) + axis.cross(firstMoment);
        }
        for (const LocalPointForce& p : loads.points)
        {
            const Vector3 force = toGlobal * p.force;
            c.appliedForce += force;
            c.appliedMoment += (start + p.position * axis).cross(force);
        }
        for (const LocalLoadSegment& s : loads.moments)   // couple résultant (c_a + c_b)(b − a)/2, sans force
            c.appliedMoment += toGlobal * (0.5 * (s.end - s.start) * (s.startIntensity + s.endIntensity));
    }
    for (const ReactionResult& r : reactions)
    {
        const Vector3 p = m_model.node(r.nodeId).position;
        const Vector3 force(r.force[0], r.force[1], r.force[2]), moment(r.force[3], r.force[4], r.force[5]);
        c.reactionForce += force;
        c.reactionMoment += moment + p.cross(force);
    }
    for (const SpringResult& s : springs)
    {
        const Vector3 p = m_model.node(s.nodeId).position;
        const Vector3 force(s.force[0], s.force[1], s.force[2]), moment(s.force[3], s.force[4], s.force[5]);
        c.reactionForce += force;
        c.reactionMoment += moment + p.cross(force);
    }
    c.forceResidual = (c.appliedForce + c.reactionForce).norm();
    c.momentResidual = (c.appliedMoment + c.reactionMoment).norm();
    return c;
}

void exportMatrixMarket(const SparseMatrix& matrix, const std::string& path)
{
    std::ofstream out(path);
    if (!out) throw Fem3DError(ErrorCode::IoError, "écriture impossible : " + path);
    out << "%%MatrixMarket matrix coordinate real general\n" << matrix.rows() << ' ' << matrix.cols() << ' ' << matrix.nonZeros() << '\n';
    out << std::setprecision(17);
    for (int k = 0; k < matrix.outerSize(); ++k)
        for (SparseMatrix::InnerIterator it(matrix, k); it; ++it) out << it.row() + 1 << ' ' << it.col() + 1 << ' ' << it.value() << '\n';
    if (!out) throw Fem3DError(ErrorCode::IoError, "écriture interrompue : " + path);
}

} // namespace TSA::Fem3D
