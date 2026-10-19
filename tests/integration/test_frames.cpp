// Tests d'intégration : orientation spatiale, structures multiéléments, charges locales / globales, mécanismes, export.

#include "TestFramework.h"
#include "TestModels.h"

#include "Fem3D/Results/ResultsJsonWriter.h"

using namespace Fem3DTest;

namespace
{
constexpr double kExact = 1e-9;

/// Portique spatial irrégulier : 4 poteaux de hauteurs différentes, 4 poutres, nœuds de pied encastrés.
StructuralModel3D spatialFrame(BeamFormulation formulation, int subdivisions = 1)
{
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle(0.3, 0.5));
    const Vector3 base[4] = { { 0, 0, 0 }, { 6, 0, 0 }, { 6.5, 4, 0 }, { -0.5, 4.5, 0 } };
    const double height[4] = { 3.0, 3.5, 4.0, 3.2 };
    int nextNode = 1, nextElement = 1;
    auto addMember = [&](int a, const Vector3& pa, int b, const Vector3& pb) {
        int previous = a;
        for (int k = 1; k <= subdivisions; ++k)
        {
            const int current = k == subdivisions ? b : nextNode++;
            if (k < subdivisions) m.addNode(current, pa + (pb - pa) * (double(k) / subdivisions));
            m.addElement({ nextElement++, previous, current, "S", "R", formulation, std::nullopt });
            previous = current;
        }
    };
    int foot[4], head[4];
    Vector3 top[4];
    for (int k = 0; k < 4; ++k)
    {
        foot[k] = nextNode++;
        m.addNode(foot[k], base[k]);
        head[k] = nextNode++;
        top[k] = base[k] + Vector3(0, 0, height[k]);
        m.addNode(head[k], top[k]);
        m.setRestraint(NodeRestraint::fullyFixed(foot[k]));
    }
    for (int k = 0; k < 4; ++k) addMember(foot[k], base[k], head[k], top[k]);
    for (int k = 0; k < 4; ++k) addMember(head[k], top[k], head[(k + 1) % 4], top[(k + 1) % 4]);
    LoadCase3D c{ 1, "vent + gravité", {}, {} };
    c.nodalLoads.push_back({ head[0], Vector3(15, -5, -40), Vector3(0, 0, 3) });
    c.nodalLoads.push_back({ head[2], Vector3(0, 10, -60), Vector3::Zero() });
    m.addLoadCase(c);
    return m;
}
} // namespace

FEM3D_TEST(Integration_InclinedCantilever_OrientationInvariance)
{
    // Console inclinée quelconque, charge perpendiculaire à l'axe dans le plan local x-z : même flèche que la console
    // horizontale (invariance par rotation), quelle que soit l'orientation.
    const double l = 4.0, f = 25.0;
    const Vector3 axis = Vector3(2, -1, 3).normalized();
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D inclined = cantilever(l, form, axis);
        const auto element = inclined.createElement(inclined.elements().front());
        const Vector3 zLocal = element->localAxes().zAxis();
        inclined.addLoadCase({ 1, "F", { { 2, f * zLocal, Vector3::Zero() } }, {} });
        const auto r = LinearStaticAnalysis(inclined).run(1);
        const Vector3 tip(tipValue(r, 2, Dof::Ux), tipValue(r, 2, Dof::Uy), tipValue(r, 2, Dof::Uz));

        StructuralModel3D straight = cantilever(l, form);
        straight.addLoadCase({ 1, "F", { { 2, Vector3(0, 0, f), Vector3::Zero() } }, {} });
        const double wStraight = tipValue(LinearStaticAnalysis(straight).run(1), 2, Dof::Uz);
        CHECK_NEAR(tip.dot(zLocal), wStraight, kExact, "flèche selon z local = flèche de la console horizontale");
        CHECK(std::abs(tip.dot(element->localAxes().yAxis())) < 1e-12 && std::abs(tip.dot(axis)) < 1e-12, "aucun déplacement parasite");
    }
}

FEM3D_TEST(Integration_SpatialFrame_EquilibriumAndMeshInvariance)
{
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D coarse = spatialFrame(form, 1), fine = spatialFrame(form, 4);
        const auto rc = LinearStaticAnalysis(coarse).run(1), rf = LinearStaticAnalysis(fine).run(1);
        CHECK(rc.equilibrium.forceResidual < 1e-8 && rc.equilibrium.momentResidual < 1e-8, "équilibre global (ΣF = 0, ΣM = 0)");
        // Éléments exacts : subdiviser les barres ne change pas les déplacements des nœuds communs (1 à 8)
        for (int node = 1; node <= 8; ++node)
            for (Dof d : { Dof::Ux, Dof::Uy, Dof::Uz, Dof::Rx, Dof::Ry, Dof::Rz })
            {
                const double a = tipValue(rc, node, d), b = tipValue(rf, node, d);
                CHECK(Fem3DTest::near(a, b, 1e-8, 1e-14), "invariance au maillage, nœud " << node << " " << kDofNames[static_cast<std::size_t>(index(d))]);
            }
    }
}

FEM3D_TEST(Integration_GlobalAndLocalDistributedLoadsEquivalent)
{
    // Barre inclinée : une charge globale (0, 0, −q) et sa projection locale donnent les mêmes résultats
    const double q = -6.0;
    const Vector3 axis = Vector3(3, 1, 2).normalized();
    StructuralModel3D global = cantilever(5.0, BeamFormulation::Timoshenko, axis);
    StructuralModel3D local = cantilever(5.0, BeamFormulation::Timoshenko, axis);
    const auto element = global.createElement(global.elements().front());
    const Vector3 qLocal = element->localAxes().rotation() * Vector3(0, 0, q);
    global.addLoadCase({ 1, "g", {}, { { 1, LoadFrame::Global, Vector3(0, 0, q), Vector3(0, 0, 0.5 * q) } } });
    local.addLoadCase({ 1, "l", {}, { { 1, LoadFrame::Local, qLocal, 0.5 * qLocal } } });
    const auto rg = LinearStaticAnalysis(global).run(1), rl = LinearStaticAnalysis(local).run(1);
    for (Dof d : { Dof::Ux, Dof::Uy, Dof::Uz, Dof::Rx, Dof::Ry, Dof::Rz }) CHECK_NEAR(tipValue(rg, 2, d), tipValue(rl, 2, d), 1e-12, "mêmes déplacements");
    CHECK_NEAR(rg.equilibrium.appliedForce.z(), q * 5.0 * 0.75, 1e-12, "résultante trapézoïdale (q1 + q2) L / 2");
    CHECK(rg.equilibrium.forceResidual < 1e-9 && rg.equilibrium.momentResidual < 1e-9, "équilibre global");
}

FEM3D_TEST(Integration_ResponseConsistentWithNodalDisplacements)
{
    // La déformée reconstruite par intégration retrouve exactement le déplacement et la rotation du nœud j
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = spatialFrame(form, 1);
        LoadCase3D c{ 2, "réparties", {}, {} };
        for (const auto& e : m.elements()) c.distributedLoads.push_back({ e.id, LoadFrame::Global, Vector3(1, -2, -9), Vector3(0, 1, -4) });
        m.addLoadCase(c);
        const auto r = LinearStaticAnalysis(m).run(2);
        for (const auto& e : r.elements)
        {
            const StationResult& end = e.stations.back();
            for (int k = 0; k < 3; ++k)
            {
                CHECK(Fem3DTest::near(end.localDisplacement(k), e.localDisplacements(6 + k), 1e-8, 1e-13), "déplacement en x = L (élément " << e.elementId << ")");
                CHECK(Fem3DTest::near(end.sectionRotation(k), e.localDisplacements(9 + k), 1e-8, 1e-13), "rotation en x = L (élément " << e.elementId << ")");
            }
            // Équilibre local des efforts d'extrémité : f_j + efforts internes en L = 0
            CHECK(Fem3DTest::near(end.forces.axial, e.endForcesLocal(6), 1e-8, 1e-9) && Fem3DTest::near(end.forces.momentZ, e.endForcesLocal(11), 1e-8, 1e-9),
                  "efforts internes en L = forces d'extrémité j");
        }
        CHECK(r.equilibrium.forceResidual < 1e-8 && r.equilibrium.momentResidual < 1e-8, "équilibre global sous charges réparties");
    }
}

FEM3D_TEST(Integration_MechanismDetected)
{
    // Poutre sur appuis simples sans blocage de la rotation autour de son axe : mécanisme de torsion désigné
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle());
    m.addNode(10, Vector3::Zero());
    m.addNode(20, Vector3(4, 0, 0));
    m.addElement({ 1, 10, 20, "S", "R", BeamFormulation::EulerBernoulli, std::nullopt });
    m.setRestraint(NodeRestraint::pinned(10));
    m.setRestraint(NodeRestraint::custom(20, { false, true, true, false, false, false }));
    bool detected = false;
    std::string message;
    try { LinearStaticAnalysis analysis(m); }
    catch (const Fem3DError& e)
    {
        detected = e.code() == ErrorCode::Mechanism;
        message = e.what();
    }
    CHECK(detected && message.find("Rx") != std::string::npos, "mécanisme de torsion détecté et désigné : " << message);

    StructuralModel3D floating;
    floating.addMaterial(steel());
    floating.addSection(rectangle());
    floating.addNode(1, Vector3::Zero());
    floating.addNode(2, Vector3(3, 0, 0));
    floating.addElement({ 1, 1, 2, "S", "R", BeamFormulation::Timoshenko, std::nullopt });
    detected = false;
    try { LinearStaticAnalysis analysis(floating); } catch (const Fem3DError& e) { detected = e.code() == ErrorCode::Mechanism; }
    CHECK(detected, "structure sans appui : mécanisme");
}

FEM3D_TEST(Integration_ResultsJsonFormat)
{
    StructuralModel3D m = spatialFrame(BeamFormulation::Timoshenko);
    const auto results = LinearStaticAnalysis(m).runAll();
    const std::string json = resultsToJson("Essai", m, results);
    CHECK(json.find("\"format\":\"tsa-fem3d-results/1\"") != std::string::npos && json.find("\"engine\":\"TSA Fem3D") != std::string::npos,
          "format et version du moteur");
    CHECK(json.find("\"formulation\":\"Timoshenko\"") != std::string::npos && json.find("\"stations\":[") != std::string::npos
              && json.find("\"equilibrium\":{") != std::string::npos,
          "éléments, stations et contrôle d'équilibre exportés");
    CHECK(json.find("nan") == std::string::npos && json.find("inf") == std::string::npos, "aucune valeur non finie");
}
