#include "Fem3D/Geometry/LocalCoordinateSystem3D.h"

#include "Fem3D/Core/Fem3DError.h"

#include <cmath>
#include <sstream>

namespace TSA::Fem3D
{

LocalCoordinateSystem3D::LocalCoordinateSystem3D(const Vector3& start, const Vector3& end, const std::optional<Vector3>& vectorXZ)
{
    const Vector3 chord = end - start;
    m_length = chord.norm();
    const double scale = std::max({ start.cwiseAbs().maxCoeff(), end.cwiseAbs().maxCoeff(), 1.0 });
    if (!std::isfinite(m_length) || m_length <= Tolerance::relativeMinimumLength * scale)
    {
        std::ostringstream os;
        os << "élément de longueur nulle ou quasi nulle (L = " << m_length << ")";
        throw Fem3DError(ErrorCode::DegenerateGeometry, os.str());
    }
    m_x = chord / m_length;

    if (vectorXZ)
    {
        m_vectorXZ = *vectorXZ;
    }
    else
    {
        const Vector3 globalZ = Vector3::UnitZ();
        m_vectorXZ = std::abs(m_x.dot(globalZ)) >= Tolerance::parallelCosine ? Vector3(-1.0, 0.0, 0.0) : globalZ;
    }
    const double vNorm = m_vectorXZ.norm();
    if (!std::isfinite(vNorm) || vNorm == 0.0 || std::abs(m_x.dot(m_vectorXZ / vNorm)) >= Tolerance::parallelCosine)
        throw Fem3DError(ErrorCode::DegenerateGeometry, "vecteur d'orientation v_xz nul ou parallèle à l'axe de l'élément : repère local indéterminé");

    m_y = m_vectorXZ.cross(m_x).normalized();
    m_z = m_x.cross(m_y);   // déjà unitaire : x et y orthonormés
    m_rotation.row(0) = m_x.transpose();
    m_rotation.row(1) = m_y.transpose();
    m_rotation.row(2) = m_z.transpose();
}

Matrix12 LocalCoordinateSystem3D::transformation() const
{
    Matrix12 t = Matrix12::Zero();
    for (int block = 0; block < 4; ++block) t.block<3, 3>(3 * block, 3 * block) = m_rotation;
    return t;
}

} // namespace TSA::Fem3D
