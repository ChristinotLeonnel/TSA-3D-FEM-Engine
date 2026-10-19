#include "Fem3D/Elements/StaticCondensation.h"

#include <Eigen/Cholesky>
#include <Eigen/Dense>

#include <vector>

namespace TSA::Fem3D
{

namespace
{
struct Partition
{
    std::vector<int> kept, released;
};

Partition partition(const DofMask& mask)
{
    Partition p;
    for (int k = 0; k < kElementDofs; ++k) (mask[static_cast<std::size_t>(k)] ? p.released : p.kept).push_back(k);
    return p;
}

Eigen::MatrixXd block(const Matrix12& m, const std::vector<int>& rows, const std::vector<int>& cols)
{
    Eigen::MatrixXd b(static_cast<Eigen::Index>(rows.size()), static_cast<Eigen::Index>(cols.size()));
    for (std::size_t r = 0; r < rows.size(); ++r)
        for (std::size_t c = 0; c < cols.size(); ++c) b(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c)) = m(rows[r], cols[c]);
    return b;
}
} // namespace

Matrix12 condensedStiffness(const Matrix12& stiffness, const DofMask& released)
{
    const Partition p = partition(released);
    if (p.released.empty()) return stiffness;
    const Eigen::MatrixXd kab = block(stiffness, p.kept, p.released);
    const Eigen::LDLT<Eigen::MatrixXd> kbb(block(stiffness, p.released, p.released));
    const Eigen::MatrixXd kc = block(stiffness, p.kept, p.kept) - kab * kbb.solve(kab.transpose());   // K_ba = K_abᵀ
    Matrix12 out = Matrix12::Zero();
    for (std::size_t r = 0; r < p.kept.size(); ++r)
        for (std::size_t c = 0; c < p.kept.size(); ++c) out(p.kept[r], p.kept[c]) = kc(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c));
    return out;
}

Vector12 condensedLoads(const Matrix12& stiffness, const Vector12& loads, const DofMask& released)
{
    const Partition p = partition(released);
    if (p.released.empty()) return loads;
    Eigen::VectorXd fb(static_cast<Eigen::Index>(p.released.size()));
    for (std::size_t k = 0; k < p.released.size(); ++k) fb(static_cast<Eigen::Index>(k)) = loads(p.released[k]);
    const Eigen::LDLT<Eigen::MatrixXd> kbb(block(stiffness, p.released, p.released));
    const Eigen::VectorXd correction = block(stiffness, p.kept, p.released) * kbb.solve(fb);
    Vector12 out = Vector12::Zero();
    for (std::size_t k = 0; k < p.kept.size(); ++k) out(p.kept[k]) = loads(p.kept[k]) - correction(static_cast<Eigen::Index>(k));
    return out;
}

Vector12 completeReleasedDisplacements(const Matrix12& stiffness, const Vector12& displacements, const Vector12& loads, const DofMask& released)
{
    const Partition p = partition(released);
    if (p.released.empty()) return displacements;
    Eigen::VectorXd da(static_cast<Eigen::Index>(p.kept.size())), fb(static_cast<Eigen::Index>(p.released.size()));
    for (std::size_t k = 0; k < p.kept.size(); ++k) da(static_cast<Eigen::Index>(k)) = displacements(p.kept[k]);
    for (std::size_t k = 0; k < p.released.size(); ++k) fb(static_cast<Eigen::Index>(k)) = loads(p.released[k]);
    const Eigen::LDLT<Eigen::MatrixXd> kbb(block(stiffness, p.released, p.released));
    const Eigen::VectorXd db = kbb.solve(fb - block(stiffness, p.released, p.kept) * da);
    Vector12 out = displacements;
    for (std::size_t k = 0; k < p.released.size(); ++k) out(p.released[k]) = db(static_cast<Eigen::Index>(k));
    return out;
}

} // namespace TSA::Fem3D
