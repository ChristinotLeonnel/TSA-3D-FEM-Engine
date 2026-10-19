#include "Fem3D/Elements/EulerBernoulli3D.h"
#include "Fem3D/Elements/StaticCondensation.h"
#include "Fem3D/Elements/Timoshenko3D.h"

namespace TSA::Fem3D
{

const char* formulationName(BeamFormulation formulation)
{
    return formulation == BeamFormulation::Timoshenko ? "Timoshenko" : "Euler-Bernoulli";
}

Matrix12 IBeamElement3D::effectiveLocalStiffnessMatrix() const
{
    return releases().any() ? condensedStiffness(localStiffnessMatrix(), releases().mask()) : localStiffnessMatrix();
}

Matrix12 IBeamElement3D::globalStiffnessMatrix() const
{
    const Matrix12 t = localAxes().transformation();
    return t.transpose() * effectiveLocalStiffnessMatrix() * t;
}

void BeamElementBase::addAxialAndTorsion(Matrix12& k) const
{
    const double l = length();
    const double ea = m_material.youngModulus * m_section.area / l;
    const double gj = m_material.shearModulus * m_section.torsionConstant / l;
    k(0, 0) += ea;  k(0, 6) -= ea;  k(6, 0) -= ea;  k(6, 6) += ea;
    k(3, 3) += gj;  k(3, 9) -= gj;  k(9, 3) -= gj;  k(9, 9) += gj;
}

// ---------------------------------------------------------------------------------------------------------------
// Euler–Bernoulli
// ---------------------------------------------------------------------------------------------------------------

Matrix12 EulerBernoulli3D::localStiffnessMatrix() const
{
    Matrix12 k = Matrix12::Zero();
    addAxialAndTorsion(k);
    const double l = length(), l2 = l * l, l3 = l2 * l;
    const double e = material().youngModulus;

    // Flexion dans le plan x-y (v, θz ; DDL 1, 5, 7, 11), θz = dv/dx :
    //   k = E Iz / L³ [ 12  6L  −12  6L ;  6L 4L² −6L 2L² ;  −12 −6L 12 −6L ;  6L 2L² −6L 4L² ]
    const double cz = e * section().inertiaZ / l3;
    const int py[4] = { 1, 5, 7, 11 };
    const double kxy[4][4] = { { 12, 6 * l, -12, 6 * l }, { 6 * l, 4 * l2, -6 * l, 2 * l2 }, { -12, -6 * l, 12, -6 * l }, { 6 * l, 2 * l2, -6 * l, 4 * l2 } };
    // Flexion dans le plan x-z (w, θy ; DDL 2, 4, 8, 10), θy = −dw/dx : signes des termes croisés w-θy inversés
    const double cy = e * section().inertiaY / l3;
    const int pz[4] = { 2, 4, 8, 10 };
    const double kxz[4][4] = { { 12, -6 * l, -12, -6 * l }, { -6 * l, 4 * l2, 6 * l, 2 * l2 }, { -12, 6 * l, 12, 6 * l }, { -6 * l, 2 * l2, 6 * l, 4 * l2 } };
    for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 4; ++b)
        {
            k(py[a], py[b]) += cz * kxy[a][b];
            k(pz[a], pz[b]) += cy * kxz[a][b];
        }
    return k;
}

BeamShapeValues EulerBernoulli3D::shapeFunctions(double xi) const
{
    // Hermite cubiques : v(ξ) = H1 v_i + H2 θz_i + H3 v_j + H4 θz_j
    const double l = length(), x2 = xi * xi, x3 = x2 * xi;
    const double h1 = 1.0 - 3.0 * x2 + 2.0 * x3;
    const double h2 = l * (xi - 2.0 * x2 + x3);
    const double h3 = 3.0 * x2 - 2.0 * x3;
    const double h4 = l * (-x2 + x3);
    BeamShapeValues n;
    n.axial = { 1.0 - xi, xi };
    n.bendingXY = { h1, h2, h3, h4 };
    n.bendingXZ = { h1, -h2, h3, -h4 };   // θy = −dw/dx
    return n;
}

// ---------------------------------------------------------------------------------------------------------------
// Timoshenko
// ---------------------------------------------------------------------------------------------------------------

Timoshenko3D::Timoshenko3D(int id, int startNodeId, int endNodeId, LocalCoordinateSystem3D axes, ElasticMaterial material, BeamSection3D section,
                           EndReleases releases)
    : BeamElementBase(id, startNodeId, endNodeId, std::move(axes), std::move(material), std::move(section), releases)
{
    this->section().validate(/*requiresShearAreas=*/true);
}

double Timoshenko3D::shearFlexibilityY() const { return 1.0 / (material().shearModulus * *section().shearAreaY); }
double Timoshenko3D::shearFlexibilityZ() const { return 1.0 / (material().shearModulus * *section().shearAreaZ); }

double Timoshenko3D::shearParameterY() const
{
    const double l = length();
    return 12.0 * material().youngModulus * section().inertiaZ * shearFlexibilityY() / (l * l);
}

double Timoshenko3D::shearParameterZ() const
{
    const double l = length();
    return 12.0 * material().youngModulus * section().inertiaY * shearFlexibilityZ() / (l * l);
}

Matrix12 Timoshenko3D::localStiffnessMatrix() const
{
    Matrix12 k = Matrix12::Zero();
    addAxialAndTorsion(k);
    const double l = length(), l2 = l * l, l3 = l2 * l;
    const double e = material().youngModulus;

    // Plan x-y (v, θz), Φ = Φy :
    //   k = E Iz / ((1+Φ) L³) [ 12  6L  −12  6L ;  6L (4+Φ)L² −6L (2−Φ)L² ;  −12 −6L 12 −6L ;  6L (2−Φ)L² −6L (4+Φ)L² ]
    const double phiY = shearParameterY();
    const double cz = e * section().inertiaZ / ((1.0 + phiY) * l3);
    const int py[4] = { 1, 5, 7, 11 };
    const double kxy[4][4] = { { 12, 6 * l, -12, 6 * l },
                               { 6 * l, (4 + phiY) * l2, -6 * l, (2 - phiY) * l2 },
                               { -12, -6 * l, 12, -6 * l },
                               { 6 * l, (2 - phiY) * l2, -6 * l, (4 + phiY) * l2 } };
    // Plan x-z (w, θy), Φ = Φz, θy = −dw/dx
    const double phiZ = shearParameterZ();
    const double cy = e * section().inertiaY / ((1.0 + phiZ) * l3);
    const int pz[4] = { 2, 4, 8, 10 };
    const double kxz[4][4] = { { 12, -6 * l, -12, -6 * l },
                               { -6 * l, (4 + phiZ) * l2, 6 * l, (2 - phiZ) * l2 },
                               { -12, 6 * l, 12, 6 * l },
                               { -6 * l, (2 - phiZ) * l2, 6 * l, (4 + phiZ) * l2 } };
    for (int a = 0; a < 4; ++a)
        for (int b = 0; b < 4; ++b)
        {
            k(py[a], py[b]) += cz * kxy[a][b];
            k(pz[a], pz[b]) += cy * kxz[a][b];
        }
    return k;
}

BeamShapeValues Timoshenko3D::shapeFunctions(double xi) const
{
    // Fonctions de forme interdépendantes (solutions homogènes exactes), μ = 1 / (1 + Φ) :
    //   N1 = μ (1 − 3ξ² + 2ξ³ + Φ (1 − ξ))         N2 = μ L (ξ − 2ξ² + ξ³ + Φ/2 (ξ − ξ²))
    //   N3 = μ (3ξ² − 2ξ³ + Φ ξ)                   N4 = μ L (−ξ² + ξ³ − Φ/2 (ξ − ξ²))
    const double l = length(), x2 = xi * xi, x3 = x2 * xi;
    auto plane = [&](double phi) {
        const double mu = 1.0 / (1.0 + phi);
        return std::array<double, 4>{ mu * (1.0 - 3.0 * x2 + 2.0 * x3 + phi * (1.0 - xi)),
                                      mu * l * (xi - 2.0 * x2 + x3 + 0.5 * phi * (xi - x2)),
                                      mu * (3.0 * x2 - 2.0 * x3 + phi * xi),
                                      mu * l * (-x2 + x3 - 0.5 * phi * (xi - x2)) };
    };
    BeamShapeValues n;
    n.axial = { 1.0 - xi, xi };
    n.bendingXY = plane(shearParameterY());
    const auto xz = plane(shearParameterZ());
    n.bendingXZ = { xz[0], -xz[1], xz[2], -xz[3] };   // θy = −dw/dx (au sens de la rotation de section)
    return n;
}

} // namespace TSA::Fem3D
