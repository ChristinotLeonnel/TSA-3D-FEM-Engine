// Appuis élastiques (ressorts nodaux) : solutions analytiques, réactions des ressorts, équilibre, refus des données
// incohérentes. Tolérance relative 1e-9 (éléments exacts, ressorts linéaires).

#include "TestFramework.h"
#include "TestModels.h"

#include "Fem3D/Results/ResultsJsonWriter.h"

using namespace Fem3DTest;

namespace
{
constexpr double kExact = 1e-9;

const SpringResult& springAt(const LoadCaseResults& r, int node)
{
    for (const auto& s : r.springs)
        if (s.nodeId == node) return s;
    throw std::runtime_error("ressort absent des résultats");
}

const ReactionResult& reactionAt(const LoadCaseResults& r, int node)
{
    for (const auto& s : r.reactions)
        if (s.nodeId == node) return s;
    throw std::runtime_error("réaction absente des résultats");
}

/// Souplesse en bout d'une console de longueur L (charge transversale z) : L³/(3EIy) (+ L/(G Asz) en Timoshenko).
double cantileverFlexibility(double l, BeamFormulation f)
{
    const BeamSection3D s = rectangle();
    return l * l * l / (3 * kE * s.inertiaY) + (f == BeamFormulation::Timoshenko ? l / (kG * *s.shearAreaZ) : 0.0);
}
} // namespace

FEM3D_TEST(Springs_TipSpringInParallelWithCantilever)
{
    // Ressort vertical en bout de console : raideurs en parallèle, δ = F / (k_console + k)
    const double l = 4.0, k = 5000.0, fz = -20.0;
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = cantilever(l, f);
        m.setSpring({ 2, { 0, 0, k, 0, 0, 0 } });
        m.addLoadCase({ 1, "Fz", { { 2, Vector3(0, 0, fz), Vector3::Zero() } }, {} });
        const auto r = LinearStaticAnalysis(m).run(1);
        const double delta = fz / (1.0 / cantileverFlexibility(l, f) + k);
        CHECK_NEAR(tipValue(r, 2, Dof::Uz), delta, kExact, "δ = F / (k_console + k)");
        CHECK_NEAR(springAt(r, 2).force[2], -k * delta, kExact, "force du ressort −k·δ");
        CHECK_NEAR(reactionAt(r, 1).force[2], -fz + k * delta, kExact, "l'encastrement reprend le reste : R = −F − F_ressort");
        CHECK(springAt(r, 2).force[0] == 0.0 && springAt(r, 2).force[4] == 0.0, "aucune force sur les DDL sans ressort");
        CHECK(r.equilibrium.forceResidual < 1e-9 && r.equilibrium.momentResidual < 1e-9, "équilibre global, ressorts compris");
    }
}

FEM3D_TEST(Springs_RotationalSpringAtBase)
{
    // Console articulée en pied autour de y, retenue par un ressort de rotation kθ :
    // δ = F (L³/3EI [+ L/GAs] + L²/kθ) ; moment du ressort = F·L (équilibre autour de l'origine)
    const double l = 3.0, kTheta = 2.0e4, fz = -20.0;
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = cantilever(l, f);
        m.setRestraint(NodeRestraint::custom(1, { true, true, true, true, false, true }));
        m.setSpring({ 1, { 0, 0, 0, 0, kTheta, 0 } });
        m.addLoadCase({ 1, "Fz", { { 2, Vector3(0, 0, fz), Vector3::Zero() } }, {} });
        const auto r = LinearStaticAnalysis(m).run(1);
        CHECK_NEAR(tipValue(r, 2, Dof::Uz), fz * (cantileverFlexibility(l, f) + l * l / kTheta), kExact, "δ = F (L³/3EI + L²/kθ)");
        CHECK_NEAR(tipValue(r, 1, Dof::Ry), -fz * l / kTheta, kExact, "rotation du pied θy = −F L / kθ");
        CHECK_NEAR(springAt(r, 1).force[4], fz * l, kExact, "moment du ressort = F·L");
        CHECK(reactionAt(r, 1).force[4] == 0.0, "aucune réaction rigide sur le DDL libre retenu par le ressort");
        CHECK(r.equilibrium.forceResidual < 1e-9 && r.equilibrium.momentResidual < 1e-9, "équilibre global, ressorts compris");
    }
}

FEM3D_TEST(Springs_StructureSupportedOnlyBySprings)
{
    // Aucun appui rigide : barre selon X posée sur un nœud élastique (6 ressorts). Traction F en bout :
    // u1 = F / kx, u2 = u1 + F L / (E A) ; flexion : rotation du nœud élastique + console
    const double l = 4.0, kTrans = 1.0e5, kRot = 3.0e4, fx = 100.0, fz = -10.0;
    for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = cantilever(l, f);
        m.setRestraint(NodeRestraint::custom(1, { false, false, false, false, false, false }));
        m.setSpring({ 1, { kTrans, kTrans, kTrans, kRot, kRot, kRot } });
        m.addLoadCase({ 1, "F", { { 2, Vector3(fx, 0, fz), Vector3::Zero() } }, {} });
        const auto r = LinearStaticAnalysis(m).run(1);
        CHECK_NEAR(tipValue(r, 1, Dof::Ux), fx / kTrans, kExact, "u1 = F / kx");
        CHECK_NEAR(tipValue(r, 2, Dof::Ux), fx / kTrans + fx * l / (kE * rectangle().area), kExact, "u2 = u1 + F L / (E A)");
        CHECK_NEAR(tipValue(r, 2, Dof::Uz), fz * (1.0 / kTrans + l * l / kRot + cantileverFlexibility(l, f)), kExact,
                   "δ = F (1/kz + L²/kθ + souplesse de la console)");
        for (int d = 0; d < kDofsPerNode; ++d)
            CHECK_NEAR(springAt(r, 1).force[static_cast<std::size_t>(d)], -m.springs().at(1).stiffness[static_cast<std::size_t>(d)] * r.nodes[0].displacement[static_cast<std::size_t>(d)],
                       kExact, "force du ressort = −k·u sur chaque DDL");
        CHECK(r.equilibrium.forceResidual < 1e-9 && r.equilibrium.momentResidual < 1e-9, "équilibre global assuré par les seuls ressorts");
    }
}

FEM3D_TEST(Springs_ZeroStiffnessDoesNotHideMechanism)
{
    // Pied articulé dans les trois rotations : un ressort de raideur nulle ne retient rien, le mécanisme reste désigné
    StructuralModel3D m = cantilever(3.0, BeamFormulation::EulerBernoulli);
    m.setRestraint(NodeRestraint::pinned(1));
    m.setSpring({ 1, { 0, 0, 0, 0, 0, 0 } });
    m.addLoadCase({ 1, "Fz", { { 2, Vector3(0, 0, -1), Vector3::Zero() } }, {} });
    bool mechanism = false;
    try { LinearStaticAnalysis(m).run(1); }
    catch (const Fem3DError& e) { mechanism = e.code() == ErrorCode::Mechanism; }
    CHECK(mechanism, "mécanisme signalé malgré un ressort de raideur nulle");
}

FEM3D_TEST(Springs_InvalidInputRejected)
{
    auto rejected = [](const NodalSpring& spring) {
        StructuralModel3D m = cantilever(3.0, BeamFormulation::EulerBernoulli);
        m.setSpring(spring);
        try { m.validate(); }
        catch (const Fem3DError& e) { return e.code(); }
        return ErrorCode::IoError;   // valeur sentinelle : rien n'a été refusé
    };
    CHECK(rejected({ 2, { 0, 0, -1.0, 0, 0, 0 } }) == ErrorCode::InvalidInput, "raideur négative refusée");
    CHECK(rejected({ 2, { 0, 0, std::nan(""), 0, 0, 0 } }) == ErrorCode::InvalidInput, "raideur non finie refusée");
    CHECK(rejected({ 1, { 0, 0, 1000.0, 0, 0, 0 } }) == ErrorCode::InvalidInput, "ressort sur un DDL bloqué refusé");
    CHECK(rejected({ 99, { 1000.0, 0, 0, 0, 0, 0 } }) == ErrorCode::UnknownReference, "ressort sur un nœud inconnu refusé");
    CHECK(rejected({ 1, { 0, 0, 0, 0, 0, 0 } }) == ErrorCode::IoError, "raideurs nulles sur un nœud encastré acceptées");
}

FEM3D_TEST(Springs_ExportedInResultsJson)
{
    StructuralModel3D m = cantilever(4.0, BeamFormulation::EulerBernoulli);
    m.setSpring({ 2, { 0, 0, 5000.0, 0, 0, 0 } });
    m.addLoadCase({ 1, "Fz", { { 2, Vector3(0, 0, -20), Vector3::Zero() } }, {} });
    const std::string json = resultsToJson("Essai", m, LinearStaticAnalysis(m).runAll());
    CHECK(json.find("\"springs\":[{\"node\":2,\"stiffness\":[") != std::string::npos, "raideurs exportées");
    CHECK(json.find("\"springForces\":[{\"node\":2,\"values\":[") != std::string::npos, "forces des ressorts exportées");
    CHECK(json.find("\"springForces\":\"") != std::string::npos, "convention de signe des ressorts exportée");
}
