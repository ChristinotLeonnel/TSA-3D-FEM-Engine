// Tests analytiques : solutions de la résistance des matériaux (Euler–Bernoulli et Timoshenko).
// Tolérance relative 1e-9 : les deux éléments sont exacts pour une poutre prismatique (charges nodales et charges
// réparties linéaires, efforts et déformée reconstruits par intégration exacte) ; l'écart attendu n'est que l'arrondi.

#include "TestFramework.h"
#include "TestModels.h"

using namespace Fem3DTest;

namespace
{
constexpr double kExact = 1e-9;

LoadCaseResults solve(StructuralModel3D& model, LoadCase3D loadCase)
{
    loadCase.id = 1;
    model.addLoadCase(loadCase);
    return LinearStaticAnalysis(model).run(1);
}

const ElementResult& element(const LoadCaseResults& r, int id = 1)
{
    for (const auto& e : r.elements)
        if (e.elementId == id) return e;
    throw std::runtime_error("élément absent");
}

/// Station la plus proche de x.
const StationResult& stationAt(const ElementResult& e, double x)
{
    const StationResult* best = &e.stations.front();
    for (const auto& s : e.stations)
        if (std::abs(s.position - x) < std::abs(best->position - x)) best = &s;
    return *best;
}

/// Poutre sur deux appuis simples le long de X (nœud 1 : articulation avec torsion bloquée ; nœud 2 : appui glissant).
StructuralModel3D simplySupported(double length, BeamFormulation formulation)
{
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle());
    m.addNode(1, Vector3::Zero());
    m.addNode(2, Vector3(length, 0, 0));
    m.addElement({ 1, 1, 2, "S", "R", formulation, std::nullopt });
    m.setRestraint(NodeRestraint::custom(1, { true, true, true, true, false, false }));
    m.setRestraint(NodeRestraint::custom(2, { false, true, true, false, false, false }));
    return m;
}
} // namespace

FEM3D_TEST(Analytical_AxialTension)
{
    const double l = 4.0, p = 100.0;
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = cantilever(l, f);
        const auto r = solve(m, { 0, "N", { { 2, Vector3(p, 0, 0), Vector3::Zero() } }, {} });
        CHECK_NEAR(tipValue(r, 2, Dof::Ux), p * l / (kE * rectangle().area), kExact, "u = PL/EA");
        CHECK_NEAR(stationAt(element(r), l / 2).forces.axial, p, kExact, "N = +P (traction)");
        CHECK_NEAR(r.reactions.front().force[0], -p, kExact, "réaction −P");
    }
}

FEM3D_TEST(Analytical_SaintVenantTorsion)
{
    const double l = 4.0, t = 12.0;
    StructuralModel3D m = cantilever(l, BeamFormulation::EulerBernoulli);
    const auto r = solve(m, { 0, "T", { { 2, Vector3::Zero(), Vector3(t, 0, 0) } }, {} });
    CHECK_NEAR(tipValue(r, 2, Dof::Rx), t * l / (kG * rectangle().torsionConstant), kExact, "θx = TL/GJ");
    CHECK_NEAR(stationAt(element(r), 1.0).forces.torsion, t, kExact, "T = +12");
    CHECK_NEAR(*stationAt(element(r), 1.0).stresses.torsion, t / *rectangle().torsionModulus, kExact, "τ = T / Wt");
}

FEM3D_TEST(Analytical_CantileverTipLoad_BothPlanesBothFormulations)
{
    const double l = 3.0, f = 20.0;
    const BeamSection3D s = rectangle();
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        const bool timo = form == BeamFormulation::Timoshenko;
        StructuralModel3D mz = cantilever(l, form);
        const auto rz = solve(mz, { 0, "Fz", { { 2, Vector3(0, 0, f), Vector3::Zero() } }, {} });
        const double wExpected = f * l * l * l / (3 * kE * s.inertiaY) + (timo ? f * l / (kG * *s.shearAreaZ) : 0.0);
        CHECK_NEAR(tipValue(rz, 2, Dof::Uz), wExpected, kExact, "w = FL³/3EIy (+ FL/GAsz)");
        CHECK_NEAR(tipValue(rz, 2, Dof::Ry), -f * l * l / (2 * kE * s.inertiaY), kExact, "θy = −FL²/2EIy (θy = −dw/dx)");
        CHECK_NEAR(stationAt(element(rz), 0.0).forces.momentY, -f * l, kExact, "My(0) = −FL");
        CHECK_NEAR(stationAt(element(rz), 0.0).forces.shearZ, f, kExact, "Vz = +F");

        StructuralModel3D my = cantilever(l, form);
        const auto ry = solve(my, { 0, "Fy", { { 2, Vector3(0, f, 0), Vector3::Zero() } }, {} });
        const double vExpected = f * l * l * l / (3 * kE * s.inertiaZ) + (timo ? f * l / (kG * *s.shearAreaY) : 0.0);
        CHECK_NEAR(tipValue(ry, 2, Dof::Uy), vExpected, kExact, "v = FL³/3EIz (+ FL/GAsy)");
        CHECK_NEAR(tipValue(ry, 2, Dof::Rz), f * l * l / (2 * kE * s.inertiaZ), kExact, "θz = FL²/2EIz");
        CHECK_NEAR(stationAt(element(ry), 0.0).forces.momentZ, f * l, kExact, "Mz(0) = +FL");
        // Timoshenko : la pente diffère de la rotation de section (γ = V / G As)
        const auto& tip = element(ry).stations.back();
        CHECK_NEAR(tip.slopeXY - tip.sectionRotation.z(), timo ? f / (kG * *s.shearAreaY) : 0.0, 1e-7, "pente − rotation = V/(G Asy)");
    }
}

FEM3D_TEST(Analytical_CantileverUniformLoad_ExactAlongMember)
{
    const double l = 3.0, q = -8.0;
    const BeamSection3D s = rectangle();
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        const bool timo = form == BeamFormulation::Timoshenko;
        StructuralModel3D m = cantilever(l, form);
        const auto r = solve(m, { 0, "q", {}, { { 1, LoadFrame::Local, Vector3(0, 0, q), Vector3(0, 0, q) } } });
        const double ei = kE * s.inertiaY, gas = kG * *s.shearAreaZ;
        CHECK_NEAR(tipValue(r, 2, Dof::Uz), q * l * l * l * l / (8 * ei) + (timo ? q * l * l / (2 * gas) : 0.0), kExact, "w = qL⁴/8EI (+ qL²/2GAs)");
        for (const auto& st : element(r).stations)
        {
            const double x = st.position, a = l - x;
            CHECK_NEAR(st.forces.momentY, -q * a * a / 2.0, kExact, "My(x) = −q (L−x)²/2 (parabole exacte)");
            CHECK_NEAR(st.forces.shearZ, q * a, kExact, "Vz(x) = +q (L−x)");
            // w(x) = q x² (6L² − 4Lx + x²) / (24 EI) + q (Lx − x²/2) / (G As)
            const double wx = q * x * x * (6 * l * l - 4 * l * x + x * x) / (24 * ei) + (timo ? q * (l * x - x * x / 2) / gas : 0.0);
            CHECK_NEAR(st.localDisplacement.z(), wx, kExact, "déformée exacte entre les nœuds");
        }
    }
}

FEM3D_TEST(Analytical_SimplySupportedUniform_OneElement)
{
    const double l = 6.0, q = -10.0;
    const BeamSection3D s = rectangle();
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = simplySupported(l, form);
        const auto r = solve(m, { 0, "q", {}, { { 1, LoadFrame::Global, Vector3(0, 0, q), Vector3(0, 0, q) } } });
        const auto& mid = stationAt(element(r), l / 2);
        const double timo = form == BeamFormulation::Timoshenko ? q * l * l / (8 * kG * *s.shearAreaZ) : 0.0;
        CHECK_NEAR(mid.localDisplacement.z(), 5 * q * l * l * l * l / (384 * kE * s.inertiaY) + timo, kExact, "flèche à mi-portée 5qL⁴/384EI (+ qL²/8GAs)");
        CHECK_NEAR(mid.forces.momentY, q * l * l / 8, kExact, "My(L/2) = qL²/8");
        CHECK_NEAR(element(r).maxTransverseDeflectionPosition, l / 2, 1e-12, "flèche maximale à mi-portée");
        CHECK_NEAR(*mid.stresses.bendingY, std::abs(q) * l * l / 8 * *s.zExtreme / s.inertiaY, kExact, "σ = M z / Iy");
    }
}

FEM3D_TEST(Analytical_FixedFixedTriangularLoad_EulerBernoulli)
{
    // q(0) = 0, q(L) = w selon y local : M_i = wL²/30, M_j = wL²/20 ; R_i = 3wL/20, R_j = 7wL/20
    const double l = 5.0, w = 12.0;
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle());
    m.addNode(1, Vector3::Zero());
    m.addNode(2, Vector3(l, 0, 0));
    m.addElement({ 1, 1, 2, "S", "R", BeamFormulation::EulerBernoulli, std::nullopt });
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.setRestraint(NodeRestraint::fullyFixed(2));
    const auto r = solve(m, { 0, "tri", {}, { { 1, LoadFrame::Local, Vector3::Zero(), Vector3(0, w, 0) } } });
    const auto& ri = r.reactions[0].force;
    const auto& rj = r.reactions[1].force;
    CHECK_NEAR(ri[1], -3 * w * l / 20, kExact, "R_i = 3wL/20");
    CHECK_NEAR(rj[1], -7 * w * l / 20, kExact, "R_j = 7wL/20");
    CHECK_NEAR(std::abs(ri[5]), w * l * l / 30, kExact, "|M_i| = wL²/30");
    CHECK_NEAR(std::abs(rj[5]), w * l * l / 20, kExact, "|M_j| = wL²/20");
    CHECK(r.equilibrium.forceResidual < 1e-9 && r.equilibrium.momentResidual < 1e-9, "équilibre global");
}

FEM3D_TEST(Analytical_TimoshenkoDeepBeam_NoShearLocking)
{
    // Console courte (L/h = 1) : une seule poutre de Timoshenko donne la solution exacte ; 10 éléments la même.
    const double l = 1.0, f = 50.0;
    const BeamSection3D s = rectangle(0.3, 1.0);
    const double exact = f * l * l * l / (3 * kE * s.inertiaY) + f * l / (kG * *s.shearAreaZ);
    StructuralModel3D one = cantilever(l, BeamFormulation::Timoshenko, Vector3::UnitX(), s);
    CHECK_NEAR(tipValue(solve(one, { 0, "F", { { 2, Vector3(0, 0, f), Vector3::Zero() } }, {} }), 2, Dof::Uz), exact, kExact, "1 élément : solution exacte");

    StructuralModel3D ten;
    ten.addMaterial(steel());
    ten.addSection(s);
    for (int i = 0; i <= 10; ++i) ten.addNode(i + 1, Vector3(l * i / 10.0, 0, 0));
    for (int i = 1; i <= 10; ++i) ten.addElement({ i, i, i + 1, "S", "R", BeamFormulation::Timoshenko, std::nullopt });
    ten.setRestraint(NodeRestraint::fullyFixed(1));
    CHECK_NEAR(tipValue(solve(ten, { 0, "F", { { 11, Vector3(0, 0, f), Vector3::Zero() } }, {} }), 11, Dof::Uz), exact, kExact, "10 éléments : même solution");

    // Poutre élancée (L/h = 100) : Timoshenko → Euler–Bernoulli (contribution du cisaillement < 0,01 %)
    const double ls = 100.0;
    StructuralModel3D te = cantilever(ls, BeamFormulation::Timoshenko, Vector3::UnitX(), s);
    StructuralModel3D eb = cantilever(ls, BeamFormulation::EulerBernoulli, Vector3::UnitX(), s);
    const double wt = tipValue(solve(te, { 0, "F", { { 2, Vector3(0, 0, f), Vector3::Zero() } }, {} }), 2, Dof::Uz);
    const double we = tipValue(solve(eb, { 0, "F", { { 2, Vector3(0, 0, f), Vector3::Zero() } }, {} }), 2, Dof::Uz);
    CHECK(wt > we && (wt - we) / we < 1e-4, "élancement 100 : écart Timoshenko / Euler–Bernoulli < 0,01 %");
}

FEM3D_TEST(Analytical_SupportSettlement)
{
    // Encastrement–encastrement, déplacement imposé δ selon z au nœud 2 : |M| = 6 EIy δ / L², |V| = 12 EIy δ / L³
    const double l = 5.0, delta = 0.01;
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle());
    m.addNode(1, Vector3::Zero());
    m.addNode(2, Vector3(l, 0, 0));
    m.addElement({ 1, 1, 2, "S", "R", BeamFormulation::EulerBernoulli, std::nullopt });
    m.setRestraint(NodeRestraint::fullyFixed(1));
    NodeRestraint settled = NodeRestraint::fullyFixed(2);
    settled.prescribed[static_cast<std::size_t>(index(Dof::Uz))] = delta;
    m.setRestraint(settled);
    const auto r = solve(m, { 0, "tassement", {}, {} });
    const double ei = kE * rectangle().inertiaY;
    CHECK_NEAR(std::abs(stationAt(element(r), 0.0).forces.momentY), 6 * ei * delta / (l * l), kExact, "|M| = 6EIδ/L²");
    CHECK_NEAR(std::abs(stationAt(element(r), 0.0).forces.shearZ), 12 * ei * delta / (l * l * l), kExact, "|V| = 12EIδ/L³");
    CHECK_NEAR(tipValue(r, 2, Dof::Uz), delta, 1e-15, "déplacement imposé respecté");
}
