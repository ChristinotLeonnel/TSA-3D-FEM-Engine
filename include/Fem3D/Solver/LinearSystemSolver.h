#pragma once

// Résolution de K_ff U_f = F_f par factorisation LDLᵀ creuse (Eigen::SimplicialLDLT), sans inversion explicite.
// Factorisation unique, réutilisée pour tous les cas de charge. Un pivot nul, négatif ou négligeable devant le terme
// diagonal de son propre DDL (critère indépendant des unités) signale un mécanisme : les DDL de ses modes de rigidité
// nulle (matrice équilibrée par sa diagonale) sont désignés dans le message.
// Aucun DDL libre (tous imposés) : modèle valable, la réponse ne dépend que des déplacements imposés et des charges.

#include "Fem3D/Assembly/GlobalAssembler.h"

#include <Eigen/SparseCholesky>

#include <functional>
#include <string>

namespace TSA::Fem3D
{

class LinearSystemSolver
{
public:
    /// @param dofLabel désignation d'un DDL de la partition libre (indice 0..n_f−1), pour les diagnostics.
    /// @throws Fem3DError (Mechanism)
    void factorize(const SparseMatrix& stiffness, const std::function<std::string(int)>& dofLabel);
    DenseVector solve(const DenseVector& rightHandSide) const;

    /// Plus petit rapport pivot / diagonale propre (1 sans DDL libre) et indice libre du DDL concerné (−1 sinon).
    /// Un rapport faible (< Tolerance::weakPivot) signale un quasi-mécanisme : le calcul reste possible mais peut
    /// perdre jusqu'à −log10(rapport) chiffres significatifs.
    double minimumPivotRatio() const { return m_minimumPivotRatio; }
    int minimumPivotDof() const { return m_minimumPivotDof; }

private:
    Eigen::SimplicialLDLT<SparseMatrix> m_ldlt;
    bool m_empty = false;
    double m_minimumPivotRatio = 1.0;
    int m_minimumPivotDof = -1;
};

} // namespace TSA::Fem3D
