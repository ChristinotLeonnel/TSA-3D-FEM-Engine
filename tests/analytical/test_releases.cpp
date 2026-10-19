// Relâchements d'extrémité (rotules My, Mz) : condensation statique, solutions analytiques, mécanismes.
// Tolérance relative 1e-9 (éléments exacts, condensation exacte).

#include "TestFramework.h"
#include "TestModels.h"

#include "Fem3D/Elements/EulerBernoulli3D.h"
#include "Fem3D/Elements/StaticCondensation.h"

using namespace Fem3DTest;

namespace
{
constexpr double kExact = 1e-9;

/// Poutre selon X entre deux nœuds encastrés ; rotules définies sur l'élément.
StructuralModel3D clampedNodes(double l, BeamFormulation f, EndReleases releases)
{
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle());
    m.addNode(1, Vector3::Zero());
    m.addNode(2, Vector3(l, 0, 0));
    ElementDefinition e{ 1, 1, 2, "S", "R", f, std::nullopt };
    e.releases = releases;
    m.addElement(e);
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.setRestraint(NodeRestraint::fullyFixed(2));
    return m;
}

const ElementResult& onlyElement(const LoadCaseResults& r) { return r.elements.front(); }

const StationResult& stationAt(const ElementResult& e, double x)
{
    const StationResult* best = &e.stations.front();
    for (const auto& s : e.stations)
        if (std::abs(s.position - x) < std::abs(best->position - x)) best = &s;
    return *best;
}
} // namespace

FEM3D_TEST(Releases_CondensedStiffnessIsProppedCantilever)
{
    const double l = 4.0;
    EndReleases r;
    r.momentZEnd = true;
    const EulerBernoulli3D e(1, 1, 2, LocalCoordinateSystem3D(Vector3::Zero(), Vector3(l, 0, 0)), steel(), rectangle(), r);
    const Matrix12 k = e.effectiveLocalStiffnessMatrix();
    const double ei = kE * rectangle().inertiaZ;
    CHECK_NEAR(k(1, 1), 3 * ei / (l * l * l), kExact, "k_vv = 3EI/L³ (console sur appui)");
    CHECK_NEAR(k(1, 5), 3 * ei / (l * l), kExact, "k_vθ = 3EI/L²");
    CHECK_NEAR(k(5, 5), 3 * ei / l, kExact, "k_θθ = 3EI/L");
    CHECK(k.row(11).isZero(0.0) && k.col(11).isZero(0.0), "ligne et colonne du DDL relâché nulles");
    CHECK((k - k.transpose()).norm() <= 1e-12 * k.norm(), "symétrie conservée");
    CHECK_NEAR(k(2, 2), 12 * kE * rectangle().inertiaY / (l * l * l), kExact, "plan x-z non relâché inchangé");
}

FEM3D_TEST(Releases_PinnedBeamBetweenClampedNodes)
{
    // Rotules aux deux extrémités (My et Mz) entre deux nœuds encastrés : poutre sur appuis simples en flexion
    const double l = 6.0, q = -10.0;
    const BeamSection3D s = rectangle();
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = clampedNodes(l, f, EndReleases::pinnedBothEnds());
        m.addLoadCase({ 1, "q", {}, { { 1, LoadFrame::Local, Vector3(0, 0, q), Vector3(0, 0, q) } } });
        const auto r = LinearStaticAnalysis(m).run(1);
        const auto& e = onlyElement(r);
        const double timo = f == BeamFormulation::Timoshenko ? q * l * l / (8 * kG * *s.shearAreaZ) : 0.0;
        CHECK_NEAR(stationAt(e, l / 2).localDisplacement.z(), 5 * q * l * l * l * l / (384 * kE * s.inertiaY) + timo, kExact, "flèche 5qL⁴/384EI (+ qL²/8GAs)");
        CHECK_NEAR(stationAt(e, l / 2).forces.momentY, q * l * l / 8, kExact, "My(L/2) = qL²/8");
        CHECK(e.endForcesLocal(4) == 0.0 && e.endForcesLocal(10) == 0.0 && e.endForcesLocal(5) == 0.0 && e.endForcesLocal(11) == 0.0,
              "moments d'extrémité exactement nuls aux rotules");
        CHECK(std::abs(stationAt(e, 0.0).forces.momentY) < 1e-9 && std::abs(stationAt(e, l).forces.momentY) < 1e-9, "My nul aux extrémités");
        // Rotation de la barre à la rotule (≠ rotation du nœud, nulle) : |θy(0)| = |q| L³ / (24 EI)
        CHECK_NEAR(std::abs(e.localDisplacements(4)), std::abs(q) * l * l * l / (24 * kE * s.inertiaY), kExact, "rotation de la barre à la rotule");
        CHECK(std::abs(r.nodes[0].displacement[4]) < 1e-15, "rotation du nœud encastré nulle");
        CHECK(r.equilibrium.forceResidual < 1e-9 && r.equilibrium.momentResidual < 1e-9, "équilibre global");
    }
}

FEM3D_TEST(Releases_ProppedCantileverOneHinge)
{
    // Rotule My au nœud j seulement : M_i = qL²/8, R_i = 5qL/8, R_j = 3qL/8 (Euler–Bernoulli)
    const double l = 5.0, q = -12.0;
    EndReleases rel;
    rel.momentYEnd = true;
    StructuralModel3D m = clampedNodes(l, BeamFormulation::EulerBernoulli, rel);
    m.addLoadCase({ 1, "q", {}, { { 1, LoadFrame::Local, Vector3(0, 0, q), Vector3(0, 0, q) } } });
    const auto r = LinearStaticAnalysis(m).run(1);
    CHECK_NEAR(std::abs(r.reactions[0].force[4]), std::abs(q) * l * l / 8, kExact, "|M_i| = qL²/8");
    CHECK_NEAR(r.reactions[0].force[2], -5 * q * l / 8, kExact, "R_i = 5qL/8");
    CHECK_NEAR(r.reactions[1].force[2], -3 * q * l / 8, kExact, "R_j = 3qL/8");
    CHECK(std::abs(r.reactions[1].force[4]) < 1e-9, "moment nul transmis par la rotule");
}

FEM3D_TEST(Releases_GerberHingeBetweenTwoCantilevers)
{
    // Deux consoles de même longueur reliées par une rotule (My et Mz) au nœud 2 chargé : chacune reprend P/2
    const double l = 3.0, p = 40.0;
    const BeamSection3D s = rectangle();
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m;
        m.addMaterial(steel());
        m.addSection(s);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(l, 0, 0));
        m.addNode(3, Vector3(2 * l, 0, 0));
        ElementDefinition left{ 1, 1, 2, "S", "R", f, std::nullopt };
        left.releases.momentYEnd = left.releases.momentZEnd = true;
        m.addElement(left);
        m.addElement({ 2, 2, 3, "S", "R", f, std::nullopt });
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.setRestraint(NodeRestraint::fullyFixed(3));
        m.addLoadCase({ 1, "P", { { 2, Vector3(0, 0, -p), Vector3::Zero() } }, {} });
        const auto r = LinearStaticAnalysis(m).run(1);
        const double flex = l * l * l / (3 * kE * s.inertiaY) + (f == BeamFormulation::Timoshenko ? l / (kG * *s.shearAreaZ) : 0.0);
        CHECK_NEAR(tipValue(r, 2, Dof::Uz), -p / 2 * flex, kExact, "δ = (P/2) (L³/3EI (+ L/GAs))");
        CHECK_NEAR(r.reactions[0].force[2], p / 2, kExact, "chaque appui reprend P/2");
    }
}

FEM3D_TEST(Releases_HingeAtSupportCreatesMechanism)
{
    // Console relâchée (My) au droit de l'encastrement : rotation libre de la barre autour de y local
    StructuralModel3D hinged;
    hinged.addMaterial(steel());
    hinged.addSection(rectangle());
    hinged.addNode(1, Vector3::Zero());
    hinged.addNode(2, Vector3(3, 0, 0));
    ElementDefinition e{ 1, 1, 2, "S", "R", BeamFormulation::EulerBernoulli, std::nullopt };
    e.releases.momentYStart = true;
    hinged.addElement(e);
    hinged.setRestraint(NodeRestraint::fullyFixed(1));
    std::string message;
    try { LinearStaticAnalysis analysis(hinged); }
    catch (const Fem3DError& error)
    {
        if (error.code() == ErrorCode::Mechanism) message = error.what();
    }
    CHECK(!message.empty() && message.find("nœud 2") != std::string::npos, "mécanisme détecté et désigné : " << message);
}

FEM3D_TEST(Releases_ResponseConsistentInFrame)
{
    // Portique dont les poutres sont articulées aux deux extrémités : équilibre, moments nuls aux rotules, déformée
    // reconstruite cohérente avec les rotations de barre complétées
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m;
        m.addMaterial(steel());
        m.addSection(rectangle());
        const Vector3 base[4] = { { 0, 0, 0 }, { 6, 0, 0 }, { 6, 4, 0 }, { 0, 4, 0 } };
        for (int k = 0; k < 4; ++k)
        {
            m.addNode(1 + k, base[k]);
            m.addNode(11 + k, base[k] + Vector3(0, 0, 3.5));
            m.setRestraint(NodeRestraint::fullyFixed(1 + k));
            m.addElement({ 1 + k, 1 + k, 11 + k, "S", "R", f, Vector3(-1, 0, 0) });
        }
        for (int k = 0; k < 4; ++k)
        {
            ElementDefinition beam{ 11 + k, 11 + k, 11 + (k + 1) % 4, "S", "R", f, Vector3(0, 0, 1) };
            beam.releases = EndReleases::pinnedBothEnds();
            m.addElement(beam);
        }
        LoadCase3D c{ 1, "G + W", { { 11, Vector3(10, 4, 0), Vector3::Zero() } }, {} };
        for (int k = 0; k < 4; ++k) c.distributedLoads.push_back({ 11 + k, LoadFrame::Global, Vector3(0, 0, -15), Vector3(0, 0, -15) });
        m.addLoadCase(c);
        const auto r = LinearStaticAnalysis(m).run(1);
        CHECK(r.equilibrium.forceResidual < 1e-8 && r.equilibrium.momentResidual < 1e-8, "équilibre global");
        for (const auto& e : r.elements)
        {
            if (e.elementId < 11) continue;
            CHECK(std::abs(e.stations.front().forces.momentY) < 1e-8 && std::abs(e.stations.back().forces.momentZ) < 1e-8, "moments nuls aux rotules");
            const StationResult& end = e.stations.back();
            for (int k = 0; k < 3; ++k)
                CHECK(Fem3DTest::near(end.sectionRotation(k), e.localDisplacements(9 + k), 1e-8, 1e-13), "rotation de barre en x = L (élément " << e.elementId << ")");
        }
    }
}

FEM3D_TEST(Readme_ExampleRuns)
{
    // Exemple du README.md, tel quel : console sur appui (rotule My au nœud j, nœud j encastré)
    StructuralModel3D m;
    m.addMaterial(ElasticMaterial::fromPoisson("S235", 210e6, 0.3));
    m.addSection(BeamSection3D::rectangle("R30x50", 0.3, 0.5));
    m.addNode(1, { 0, 0, 0 });
    m.addNode(2, { 4, 0, 0 });
    ElementDefinition beam{ 1, 1, 2, "S235", "R30x50", BeamFormulation::Timoshenko, Vector3(0, 0, 1) };
    beam.releases.momentYEnd = true;
    m.addElement(beam);
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.setRestraint(NodeRestraint::fullyFixed(2));
    m.addLoadCase({ 1, "G", {}, { { 1, LoadFrame::Global, { 0, 0, -5 }, { 0, 0, -5 } } } });
    const AnalysisResults r = LinearStaticAnalysis(m).runAll();
    CHECK(r.loadCases.size() == 1 && r.loadCases.front().equilibrium.forceResidual < 1e-9, "exemple du README calculé et équilibré");
}
