#include "Fem3D/Solver/LinearSystemSolver.h"

#include "Fem3D/Core/Fem3DError.h"

#include <Eigen/Eigenvalues>

#include <cmath>
#include <sstream>

namespace TSA::Fem3D
{

namespace
{
constexpr int kMaxListedDofs = 12;
constexpr int kMaxDiagnosticSize = 2000;          ///< au-delà, pas de recherche des modes de mécanisme (coût dense)
constexpr double kModeComponentShare = 0.3;       ///< part de la composante maximale pour désigner un DDL d'un mode

/// Facteurs d'équilibrage s_i = 1/√K_ii (1 si K_ii ≤ 0) : S K S a une diagonale unité, quelles que soient les unités
/// (translations en F/L, rotations en F·L). Les critères relatifs ne mélangent ainsi plus des grandeurs différentes.
DenseVector diagonalScaling(const SparseMatrix& stiffness)
{
    DenseVector s(stiffness.rows());
    for (int i = 0; i < stiffness.rows(); ++i)
    {
        const double d = stiffness.coeff(i, i);
        s(i) = d > 0.0 ? 1.0 / std::sqrt(d) : 1.0;
    }
    return s;
}

/// DDL participant aux modes de rigidité nulle de K (vecteurs propres de valeur propre ≈ 0 de S K S).
std::vector<int> mechanismDofs(const SparseMatrix& stiffness)
{
    std::vector<int> dofs;
    if (stiffness.rows() > kMaxDiagnosticSize) return dofs;
    const DenseVector s = diagonalScaling(stiffness);
    const Eigen::MatrixXd dense = s.asDiagonal() * Eigen::MatrixXd(stiffness) * s.asDiagonal();
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(dense);
    if (eig.info() != Eigen::Success) return dofs;
    const double scale = eig.eigenvalues().cwiseAbs().maxCoeff();
    std::vector<bool> listed(static_cast<std::size_t>(stiffness.rows()), false);
    for (int m = 0; m < stiffness.rows(); ++m)
    {
        if (eig.eigenvalues()(m) > Tolerance::relativePivot * scale) continue;
        const auto mode = eig.eigenvectors().col(m);
        const double peak = mode.cwiseAbs().maxCoeff();
        for (int i = 0; i < stiffness.rows(); ++i)
            if (std::abs(mode(i)) >= kModeComponentShare * peak && !listed[static_cast<std::size_t>(i)])
            {
                listed[static_cast<std::size_t>(i)] = true;
                dofs.push_back(i);
            }
    }
    return dofs;
}
} // namespace

void LinearSystemSolver::factorize(const SparseMatrix& stiffness, const std::function<std::string(int)>& dofLabel)
{
    m_empty = stiffness.rows() == 0;   // tous les DDL imposés : réponse due aux seuls déplacements imposés et charges
    m_minimumPivotRatio = 1.0;
    m_minimumPivotDof = -1;
    if (m_empty) return;
    DenseVector diagonal(stiffness.rows());
    for (int i = 0; i < stiffness.rows(); ++i) diagonal(i) = stiffness.coeff(i, i);

    m_ldlt.compute(stiffness);
    bool singular = m_ldlt.info() != Eigen::Success;
    if (!singular)
    {
        // P K Pᵀ = L D Lᵀ : chaque pivot est comparé à la rigidité propre de son DDL (diagonale de P K Pᵀ). Ce rapport
        // ne dépend pas des unités ; un pivot non strictement positif ou négligeable devant elle signale un mécanisme.
        const auto& d = m_ldlt.vectorD();
        const DenseVector ownDiagonal = m_ldlt.permutationP() * diagonal;
        for (int k = 0; k < d.size() && !singular; ++k)
        {
            singular = !(ownDiagonal(k) > 0.0) || !(d(k) > Tolerance::relativePivot * ownDiagonal(k));
            if (!singular && d(k) / ownDiagonal(k) < m_minimumPivotRatio)
            {
                m_minimumPivotRatio = d(k) / ownDiagonal(k);
                m_minimumPivotDof = m_ldlt.permutationPinv().indices()(k);   // ligne k de P K Pᵀ → DDL d'origine
            }
        }
    }
    if (!singular) return;

    std::ostringstream os;
    os << "matrice de rigidité singulière : mécanisme (appuis insuffisants ou DDL sans rigidité)";
    const std::vector<int> suspects = mechanismDofs(stiffness);
    if (!suspects.empty())
    {
        os << ". DDL des modes de mécanisme :";
        for (std::size_t k = 0; k < suspects.size() && k < kMaxListedDofs; ++k) os << (k ? "," : "") << " " << dofLabel(suspects[k]);
        if (suspects.size() > kMaxListedDofs) os << " (+" << suspects.size() - kMaxListedDofs << ")";
    }
    throw Fem3DError(ErrorCode::Mechanism, os.str());
}

DenseVector LinearSystemSolver::solve(const DenseVector& rightHandSide) const
{
    if (m_empty) return DenseVector(0);
    return m_ldlt.solve(rightHandSide);
}

} // namespace TSA::Fem3D
