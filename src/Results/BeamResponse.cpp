#include "Fem3D/Results/BeamResponse.h"

#include "Fem3D/Elements/StaticCondensation.h"

#include <cmath>

namespace TSA::Fem3D
{

namespace
{
// Gauss-Legendre à 4 points sur [−1, 1] : exacte jusqu'au degré 7.
constexpr double kGaussPoints[4] = { -0.8611363115940526, -0.3399810435848563, 0.3399810435848563, 0.8611363115940526 };
constexpr double kGaussWeights[4] = { 0.3478548451374538, 0.6521451548625461, 0.6521451548625461, 0.3478548451374538 };
constexpr int kDeflectionSearchSubdivisions = 200;
} // namespace

LocalLinearLoad LocalLinearLoad::fromLoads(const IBeamElement3D& element, const std::vector<const DistributedLoad3D*>& loads)
{
    LocalLinearLoad sum;
    const Matrix3& r = element.localAxes().rotation();
    for (const DistributedLoad3D* l : loads)
    {
        // Charge globale : composantes projetées sur le repère local (a_local = R a_global), par unité de longueur réelle
        sum.start += l->frame == LoadFrame::Global ? Vector3(r * l->startIntensity) : l->startIntensity;
        sum.end += l->frame == LoadFrame::Global ? Vector3(r * l->endIntensity) : l->endIntensity;
    }
    return sum;
}

Vector12 equivalentNodalLoads(const IBeamElement3D& element, const LocalLinearLoad& load)
{
    Vector12 f = Vector12::Zero();
    const double l = element.length();
    for (int g = 0; g < 4; ++g)
    {
        const double xi = 0.5 * (kGaussPoints[g] + 1.0);
        const double weight = 0.5 * kGaussWeights[g] * l;   // dx = L dξ, dξ = dt / 2
        const Vector3 q = load.start + (load.end - load.start) * xi;
        const BeamShapeValues n = element.shapeFunctions(xi);
        f(0) += weight * n.axial[0] * q.x();
        f(6) += weight * n.axial[1] * q.x();
        const int xy[4] = { 1, 5, 7, 11 }, xz[4] = { 2, 4, 8, 10 };
        for (int k = 0; k < 4; ++k)
        {
            f(xy[k]) += weight * n.bendingXY[k] * q.y();
            f(xz[k]) += weight * n.bendingXZ[k] * q.z();
        }
    }
    return f;
}

BeamResponse::BeamResponse(const IBeamElement3D& element, const Vector12& localDisplacements, const LocalLinearLoad& load)
    : m_element(element)
    , m_localDisplacements(localDisplacements)
{
    const Matrix12 k = element.localStiffnessMatrix();
    const Vector12 equivalent = equivalentNodalLoads(element, load);
    if (element.releases().any())
    {
        // Rotules : rotations de la barre aux extrémités relâchées (distinctes de celles des nœuds), moments nuls
        const DofMask released = element.releases().mask();
        m_localDisplacements = completeReleasedDisplacements(k, localDisplacements, equivalent, released);
        m_endForces = k * m_localDisplacements - equivalent;
        for (int i = 0; i < kElementDofs; ++i)
            if (released[static_cast<std::size_t>(i)]) m_endForces(i) = 0.0;   // nul par définition (résidu d'arrondi effacé)
    }
    else
    {
        m_endForces = k * localDisplacements - equivalent;
    }

    const double l = element.length();
    const Vector3 a = load.start, b = (load.end - load.start) / l;   // q(s) = a + b s
    const Vector12& f = m_endForces;
    // ∫₀ˣ q = a x + b x²/2 ;  ∫₀ˣ (x − s) q ds = a x²/2 + b x³/6
    auto integralOfLoad = [](double ac, double bc) { return Polynomial({ 0.0, ac, bc / 2.0 }); };
    auto momentOfLoad = [](double ac, double bc) { return Polynomial({ 0.0, 0.0, ac / 2.0, bc / 6.0 }); };

    m_axial = Polynomial::constant(-f(0)) - integralOfLoad(a.x(), b.x());
    m_shearY = Polynomial::constant(-f(1)) - integralOfLoad(a.y(), b.y());
    m_shearZ = Polynomial::constant(-f(2)) - integralOfLoad(a.z(), b.z());
    m_torsion = Polynomial::constant(-f(3));
    m_momentY = Polynomial::linear(-f(4), -f(2)) - momentOfLoad(a.z(), b.z());
    m_momentZ = Polynomial::linear(-f(5), f(1)) + momentOfLoad(a.y(), b.y());

    const ElasticMaterial& mat = element.material();
    const BeamSection3D& sec = element.section();
    const Vector12& d = m_localDisplacements;
    m_u = Polynomial::constant(d(0)) + (1.0 / (mat.youngModulus * sec.area)) * m_axial.integral();
    m_thetaX = Polynomial::constant(d(3)) + (1.0 / (mat.shearModulus * sec.torsionConstant)) * m_torsion.integral();
    m_thetaZ = Polynomial::constant(d(5)) + (1.0 / (mat.youngModulus * sec.inertiaZ)) * m_momentZ.integral();
    m_thetaY = Polynomial::constant(d(4)) + (1.0 / (mat.youngModulus * sec.inertiaY)) * m_momentY.integral();
    m_slopeXY = m_thetaZ + element.shearFlexibilityY() * m_shearY;
    m_slopeXZ = (-1.0) * m_thetaY + element.shearFlexibilityZ() * m_shearZ;
    m_v = Polynomial::constant(d(1)) + m_slopeXY.integral();
    m_w = Polynomial::constant(d(2)) + m_slopeXZ.integral();
}

SectionForces BeamResponse::forcesAt(double x) const
{
    return { m_axial(x), m_shearY(x), m_shearZ(x), m_torsion(x), m_momentY(x), m_momentZ(x) };
}

Vector3 BeamResponse::displacementAt(double x) const { return { m_u(x), m_v(x), m_w(x) }; }
Vector3 BeamResponse::rotationAt(double x) const { return { m_thetaX(x), m_thetaY(x), m_thetaZ(x) }; }

SectionStresses BeamResponse::stressesAt(const SectionForces& s) const
{
    const BeamSection3D& sec = m_element.section();
    SectionStresses out;
    out.axial = s.axial / sec.area;
    if (sec.zExtreme) out.bendingY = std::abs(s.momentY) * *sec.zExtreme / sec.inertiaY;
    if (sec.yExtreme) out.bendingZ = std::abs(s.momentZ) * *sec.yExtreme / sec.inertiaZ;
    if (out.bendingY && out.bendingZ) out.extremeNormal = std::abs(out.axial) + *out.bendingY + *out.bendingZ;
    out.shearY = std::abs(s.shearY) / sec.shearAreaY.value_or(sec.area);
    out.shearZ = std::abs(s.shearZ) / sec.shearAreaZ.value_or(sec.area);
    if (sec.torsionModulus) out.torsion = std::abs(s.torsion) / *sec.torsionModulus;
    if (out.extremeNormal && out.torsion)
        out.vonMisesIndicative = std::sqrt(*out.extremeNormal * *out.extremeNormal + 3.0 * *out.torsion * *out.torsion);
    return out;
}

ElementResult BeamResponse::result(int stationCount) const
{
    ElementResult r;
    r.elementId = m_element.id();
    r.formulation = m_element.formulation();
    r.length = m_element.length();
    r.localDisplacements = m_localDisplacements;
    r.endForcesLocal = m_endForces;

    const double l = r.length;
    const Matrix3 toGlobal = m_element.localAxes().rotation().transpose();
    const int count = stationCount < 2 ? 2 : stationCount;
    r.stations.reserve(static_cast<std::size_t>(count));
    for (int k = 0; k < count; ++k)
    {
        StationResult s;
        s.position = l * k / (count - 1);
        s.forces = forcesAt(s.position);
        s.localDisplacement = displacementAt(s.position);
        s.sectionRotation = rotationAt(s.position);
        s.slopeXY = slopeXYAt(s.position);
        s.slopeXZ = slopeXZAt(s.position);
        s.globalDisplacement = toGlobal * s.localDisplacement;
        s.stresses = stressesAt(s.forces);
        r.stations.push_back(s);
    }

    // Flèche locale : écart transversal à la corde des extrémités déplacées
    const double vi = m_v(0.0), vj = m_v(l), wi = m_w(0.0), wj = m_w(l);
    for (int k = 0; k <= kDeflectionSearchSubdivisions; ++k)
    {
        const double x = l * k / kDeflectionSearchSubdivisions, t = x / l;
        const double dv = m_v(x) - (vi + (vj - vi) * t), dw = m_w(x) - (wi + (wj - wi) * t);
        const double deflection = std::sqrt(dv * dv + dw * dw);
        if (deflection > r.maxTransverseDeflection)
        {
            r.maxTransverseDeflection = deflection;
            r.maxTransverseDeflectionPosition = x;
        }
    }
    return r;
}

} // namespace TSA::Fem3D
