#include "Fem3D/Results/BeamResponse.h"

#include "Fem3D/Core/Types.h"
#include "Fem3D/Elements/StaticCondensation.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace TSA::Fem3D
{

namespace
{
// Gauss-Legendre à 4 points sur [−1, 1] : exacte jusqu'au degré 7.
constexpr double kGaussPoints[4] = { -0.8611363115940526, -0.3399810435848563, 0.3399810435848563, 0.8611363115940526 };
constexpr double kGaussWeights[4] = { 0.3478548451374538, 0.6521451548625461, 0.6521451548625461, 0.3478548451374538 };
constexpr int kDeflectionSearchSubdivisions = 200;
constexpr int kBendingXY[4] = { 1, 5, 7, 11 };   // (v_i, θz_i, v_j, θz_j)
constexpr int kBendingXZ[4] = { 2, 4, 8, 10 };   // (w_i, θy_i, w_j, θy_j)

/// Pentes dN/dx des fonctions de forme de flexion. Les fonctions de forme des deux formulations sont des cubiques en ξ
/// (Hermite ; Timoshenko exact) : leur interpolant de Lagrange en ξ = 0, 1/3, 2/3, 1 leur est identique, et sa dérivée
/// est exacte.
class ShapeSlopes
{
public:
    explicit ShapeSlopes(const IBeamElement3D& element)
        : m_length(element.length())
    {
        for (int j = 0; j < 4; ++j) m_samples[static_cast<std::size_t>(j)] = element.shapeFunctions(kNodes[j]);
    }

    /// dN_k/dx en ξ, pour les deux plans de flexion.
    void at(double xi, std::array<double, 4>& xy, std::array<double, 4>& xz) const
    {
        double weight[4];
        for (int j = 0; j < 4; ++j)
        {
            // L_j'(ξ) = Σ_{m≠j} Π_{n≠j,m} (ξ − z_n) / Π_{n≠j} (z_j − z_n)
            double denominator = 1.0, sum = 0.0;
            for (int n = 0; n < 4; ++n)
                if (n != j) denominator *= kNodes[j] - kNodes[n];
            for (int m = 0; m < 4; ++m)
            {
                if (m == j) continue;
                double product = 1.0;
                for (int n = 0; n < 4; ++n)
                    if (n != j && n != m) product *= xi - kNodes[n];
                sum += product;
            }
            weight[j] = sum / denominator / m_length;   // d/dx = (1/L) d/dξ
        }
        xy.fill(0.0);
        xz.fill(0.0);
        for (int j = 0; j < 4; ++j)
            for (std::size_t k = 0; k < 4; ++k)
            {
                xy[k] += weight[j] * m_samples[static_cast<std::size_t>(j)].bendingXY[k];
                xz[k] += weight[j] * m_samples[static_cast<std::size_t>(j)].bendingXZ[k];
            }
    }

private:
    static constexpr double kNodes[4] = { 0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0 };
    double m_length;
    std::array<BeamShapeValues, 4> m_samples{};
};

/// Intégrale de Gauss sur [a, b] : f(x, poids) appelée aux 4 points.
template <typename F>
void gaussOver(double a, double b, F&& f)
{
    const double mid = 0.5 * (a + b), half = 0.5 * (b - a);
    for (int g = 0; g < 4; ++g) f(mid + half * kGaussPoints[g], kGaussWeights[g] * half);
}
} // namespace

LocalElementLoads LocalElementLoads::fromLoadCase(const IBeamElement3D& element, const LoadCase3D& loadCase)
{
    LocalElementLoads out;
    const Matrix3& r = element.localAxes().rotation();
    const double l = element.length();
    // Charge globale : composantes projetées sur le repère local (a_local = R a_global), par unité de longueur réelle.
    // Positions déjà contrôlées par StructuralModel3D::validate (à Tolerance::relativeLoadPosition près) : ramenées dans [0, L].
    auto local = [&](LoadFrame frame, const Vector3& v) { return frame == LoadFrame::Global ? Vector3(r * v) : v; };
    auto position = [&](double x) { return std::clamp(x, 0.0, l); };
    for (const DistributedLoad3D& d : loadCase.distributedLoads)
        if (d.elementId == element.id())
            out.distributed.push_back({ position(d.startPosition), position(d.endPosition.value_or(l)), local(d.frame, d.startIntensity),
                                        local(d.frame, d.endIntensity) });
    for (const DistributedMoment3D& c : loadCase.distributedMoments)
        if (c.elementId == element.id())
            out.moments.push_back({ position(c.startPosition), position(c.endPosition.value_or(l)), local(c.frame, c.startIntensity),
                                    local(c.frame, c.endIntensity) });
    for (const PointLoad3D& p : loadCase.pointLoads)
        if (p.elementId == element.id()) out.points.push_back({ position(p.position), local(p.frame, p.force) });
    return out;
}

Vector12 equivalentNodalLoads(const IBeamElement3D& element, const LocalElementLoads& loads)
{
    Vector12 f = Vector12::Zero();
    const double l = element.length();
    // Force concentrée p en x : travail sur les fonctions de forme (charge répartie : p = q dx en un point de Gauss)
    auto addForce = [&](double x, const Vector3& p) {
        const BeamShapeValues n = element.shapeFunctions(x / l);
        f(0) += n.axial[0] * p.x();
        f(6) += n.axial[1] * p.x();
        for (std::size_t k = 0; k < 4; ++k)
        {
            f(kBendingXY[k]) += n.bendingXY[k] * p.y();
            f(kBendingXZ[k]) += n.bendingXZ[k] * p.z();
        }
    };
    for (const LocalLoadSegment& s : loads.distributed)
        gaussOver(s.start, s.end, [&](double x, double weight) { addForce(x, s.at(x) * weight); });
    for (const LocalPointForce& p : loads.points) addForce(p.position, p.force);

    if (!loads.moments.empty())
    {
        // Couple réparti : travail sur les rotations de section des solutions homogènes. Torsion : θx interpolée
        // linéairement (Saint-Venant). Flexion : Θz = v′ − γy et Θy = −w′ + γz, où la distorsion γ = V / (G As) est
        // constante sous déplacements d'extrémité, avec V = −(K e_k) à l'extrémité i (nulle pour Euler–Bernoulli).
        const ShapeSlopes slopes(element);
        const Matrix12 k = element.localStiffnessMatrix();
        const double flexY = element.shearFlexibilityY(), flexZ = element.shearFlexibilityZ();
        std::array<double, 4> dxy{}, dxz{};
        for (const LocalLoadSegment& s : loads.moments)
            gaussOver(s.start, s.end, [&](double x, double weight) {
                const Vector3 c = s.at(x) * weight;
                const BeamShapeValues n = element.shapeFunctions(x / l);
                slopes.at(x / l, dxy, dxz);
                f(3) += n.axial[0] * c.x();
                f(9) += n.axial[1] * c.x();
                for (std::size_t m = 0; m < 4; ++m)
                {
                    f(kBendingXY[m]) += (dxy[m] + flexY * k(1, kBendingXY[m])) * c.z();
                    f(kBendingXZ[m]) += (-dxz[m] - flexZ * k(2, kBendingXZ[m])) * c.y();
                }
            });
    }
    return f;
}

BeamResponse::BeamResponse(const IBeamElement3D& element, const Vector12& localDisplacements, const LocalElementLoads& loads)
    : m_element(element)
    , m_localDisplacements(localDisplacements)
{
    const Matrix12 k = element.localStiffnessMatrix();
    const Vector12 equivalent = equivalentNodalLoads(element, loads);
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

    // Abscisses de rupture : extrémités, bornes des charges réparties, forces ponctuelles (confondues à t L près)
    const double l = element.length(), tolerance = Tolerance::relativeLoadPosition * l;
    std::vector<double> cuts{ 0.0, l };
    for (const auto* list : { &loads.distributed, &loads.moments })
        for (const LocalLoadSegment& s : *list)
        {
            cuts.push_back(s.start);
            cuts.push_back(s.end);
        }
    for (const LocalPointForce& p : loads.points) cuts.push_back(p.position);
    std::sort(cuts.begin(), cuts.end());
    std::vector<double> breaks;
    for (double x : cuts)
        if (breaks.empty() || x - breaks.back() > tolerance) breaks.push_back(x);
    breaks.back() = l;
    const std::size_t pieceCount = breaks.size() - 1;

    // Force ponctuelle : appliquée au début du morceau le plus proche ; en L, elle n'agit sur aucun morceau (reprise par
    // l'effort d'extrémité j).
    std::vector<Vector3> jumps(breaks.size(), Vector3::Zero());
    for (const LocalPointForce& p : loads.points)
    {
        std::size_t nearest = 0;
        for (std::size_t b = 1; b < breaks.size(); ++b)
            if (std::abs(breaks[b] - p.position) < std::abs(breaks[nearest] - p.position)) nearest = b;
        jumps[nearest] += p.force;
    }

    const ElasticMaterial& mat = element.material();
    const BeamSection3D& sec = element.section();
    const double flexY = element.shearFlexibilityY(), flexZ = element.shearFlexibilityZ();
    const Vector12& f = m_endForces;
    const Vector12& d = m_localDisplacements;
    // État au début du morceau courant : efforts (à droite d'une éventuelle force ponctuelle), cinématique (continue)
    double n = -f(0), vy = -f(1), vz = -f(2), t = -f(3), my = -f(4), mz = -f(5);
    double u = d(0), v = d(1), w = d(2), tx = d(3), ty = d(4), tz = d(5);
    std::vector<double> starts(breaks.begin(), breaks.end() - 1);
    std::vector<Polynomial> pn, pvy, pvz, pt, pmy, pmz, pu, pv, pw, ptx, pty, ptz, psxy, psxz;
    for (std::size_t piece = 0; piece < pieceCount; ++piece)
    {
        const double s = breaks[piece], e = breaks[piece + 1], mid = 0.5 * (s + e), len = e - s;
        n -= jumps[piece].x();
        vy -= jumps[piece].y();
        vz -= jumps[piece].z();
        // Charges du morceau, linéaires en t = x − s (somme des charges qui le couvrent)
        Polynomial q[3], c[3];
        auto addSegments = [&](const std::vector<LocalLoadSegment>& list, Polynomial* out) {
            for (const LocalLoadSegment& seg : list)
            {
                if (!(seg.start < mid && mid < seg.end)) continue;
                const Vector3 at = seg.at(s), slope = (seg.endIntensity - seg.startIntensity) / (seg.end - seg.start);
                for (int i = 0; i < 3; ++i) out[i] += Polynomial::linear(at(i), slope(i));
            }
        };
        addSegments(loads.distributed, q);
        addSegments(loads.moments, c);

        const Polynomial axial = Polynomial::constant(n) - q[0].integral();
        const Polynomial shearY = Polynomial::constant(vy) - q[1].integral();
        const Polynomial shearZ = Polynomial::constant(vz) - q[2].integral();
        const Polynomial torsion = Polynomial::constant(t) - c[0].integral();
        const Polynomial momentZ = Polynomial::constant(mz) + ((-1.0) * shearY - c[2]).integral();   // dMz/dx = −Vy − cz
        const Polynomial momentY = Polynomial::constant(my) + (shearZ - c[1]).integral();            // dMy/dx =  Vz − cy
        const Polynomial dispU = Polynomial::constant(u) + (1.0 / (mat.youngModulus * sec.area)) * axial.integral();
        const Polynomial rotX = Polynomial::constant(tx) + (1.0 / (mat.shearModulus * sec.torsionConstant)) * torsion.integral();
        const Polynomial rotZ = Polynomial::constant(tz) + (1.0 / (mat.youngModulus * sec.inertiaZ)) * momentZ.integral();
        const Polynomial rotY = Polynomial::constant(ty) + (1.0 / (mat.youngModulus * sec.inertiaY)) * momentY.integral();
        const Polynomial slopeXY = rotZ + flexY * shearY;
        const Polynomial slopeXZ = (-1.0) * rotY + flexZ * shearZ;
        const Polynomial dispV = Polynomial::constant(v) + slopeXY.integral();
        const Polynomial dispW = Polynomial::constant(w) + slopeXZ.integral();

        pn.push_back(axial), pvy.push_back(shearY), pvz.push_back(shearZ), pt.push_back(torsion), pmy.push_back(momentY), pmz.push_back(momentZ);
        pu.push_back(dispU), pv.push_back(dispV), pw.push_back(dispW), ptx.push_back(rotX), pty.push_back(rotY), ptz.push_back(rotZ);
        psxy.push_back(slopeXY), psxz.push_back(slopeXZ);

        n = axial(len), vy = shearY(len), vz = shearZ(len), t = torsion(len), my = momentY(len), mz = momentZ(len);
        u = dispU(len), v = dispV(len), w = dispW(len), tx = rotX(len), ty = rotY(len), tz = rotZ(len);
    }
    auto piecewise = [&](std::vector<Polynomial>& p) { return PiecewisePolynomial(starts, std::move(p)); };
    m_axial = piecewise(pn), m_shearY = piecewise(pvy), m_shearZ = piecewise(pvz), m_torsion = piecewise(pt);
    m_momentY = piecewise(pmy), m_momentZ = piecewise(pmz);
    m_u = piecewise(pu), m_v = piecewise(pv), m_w = piecewise(pw);
    m_thetaX = piecewise(ptx), m_thetaY = piecewise(pty), m_thetaZ = piecewise(ptz);
    m_slopeXY = piecewise(psxy), m_slopeXZ = piecewise(psxz);
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
