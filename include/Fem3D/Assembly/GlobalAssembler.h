#pragma once

// Assemblage de K U = F (méthode des déplacements) directement en blocs partitionnés :
//   [ K_ff  K_fr ] [ U_f ]   [ F_f ]
//   [ K_rf  K_rr ] [ U_r ] = [ F_r + R ]
// f : DDL libres, r : DDL imposés (U_r connus), R : réactions d'appui.

#include "Fem3D/Assembly/DofMap.h"

#include <Eigen/SparseCore>

#include <memory>
#include <vector>

namespace TSA::Fem3D
{

using SparseMatrix = Eigen::SparseMatrix<double>;
using DenseVector = Eigen::VectorXd;

struct PartitionedStiffness
{
    SparseMatrix freeFree;         ///< K_ff
    SparseMatrix freeRestrained;   ///< K_fr
    SparseMatrix restrainedFree;   ///< K_rf
    SparseMatrix restrainedRestrained; ///< K_rr
    SparseMatrix full;             ///< K complète (ordre des DDL globaux), pour export et vérification
};

class GlobalAssembler
{
public:
    GlobalAssembler(const DofMap& dofs, const std::vector<std::unique_ptr<IBeamElement3D>>& elements);

    PartitionedStiffness assembleStiffness() const;
    /// F global (DDL globaux) d'un cas : charges nodales + forces nodales équivalentes des charges réparties.
    DenseVector assembleLoads(const LoadCase3D& loadCase) const;

private:
    const DofMap& m_dofs;
    const std::vector<std::unique_ptr<IBeamElement3D>>& m_elements;
};

} // namespace TSA::Fem3D
