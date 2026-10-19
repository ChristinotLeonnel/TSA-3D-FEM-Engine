// Tests ajoutés après l'audit indépendant des formulations (docs/verification/INDEPENDENT_FORMULATION_AUDIT.md) :
// Timoshenko sous charge triangulaire (avec et sans rotule), aires de cisaillement distinctes, critère de mécanisme
// indépendant des unités, module de torsion du rectangle comparé aux valeurs tabulées.
// Valeurs exactes de Timoshenko re-dérivées indépendamment (sympy, équations de Timoshenko, Φ = 12EI/(G As L²)).

#include "TestFramework.h"
#include "TestModels.h"

using namespace Fem3DTest;

namespace
{
constexpr double kExact = 1e-9;

/// Barre selon X entre deux nœuds encastrés, section à aires de cisaillement imposées.
StructuralModel3D clampedBar(double l, BeamFormulation f, const BeamSection3D& s, EndReleases releases = {})
{
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(s);
    m.addNode(1, Vector3::Zero());
    m.addNode(2, Vector3(l, 0, 0));
    ElementDefinition e{ 1, 1, 2, "S", s.name, f, std::nullopt };
    e.releases = releases;
    m.addElement(e);
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.setRestraint(NodeRestraint::fullyFixed(2));
    return m;
}

BeamSection3D distinctShearAreas()
{
    BeamSection3D s = rectangle();
    s.shearAreaY = 0.5 * s.area;
    s.shearAreaZ = 0.8 * s.area;
    return s;
}
} // namespace

FEM3D_TEST(Analytical_FixedFixedTriangularLoad_Timoshenko)
{
    // q(0) = 0, q(L) = w selon y local (plan x-y : Iz, Asy) :
    //   M_i = wL² (4 + 5Φ) / (120 (1 + Φ)), M_j = wL² (6 + 5Φ) / (120 (1 + Φ)), R_i = wL (9 + 10Φ) / (60 (1 + Φ))
    const double l = 1.5, w = 12.0;
    const BeamSection3D s = distinctShearAreas();
    const double phi = 12 * kE * s.inertiaZ / (kG * *s.shearAreaY * l * l);
    StructuralModel3D m = clampedBar(l, BeamFormulation::Timoshenko, s);
    m.addLoadCase({ 1, "tri", {}, { { 1, LoadFrame::Local, Vector3::Zero(), Vector3(0, w, 0) } } });
    const auto r = LinearStaticAnalysis(m).run(1);
    const auto& ri = r.reactions[0].force;
    const auto& rj = r.reactions[1].force;
    CHECK(phi > 0.05, "cisaillement significatif (Φ = " << phi << ")");
    CHECK_NEAR(std::abs(ri[5]), w * l * l * (4 + 5 * phi) / (120 * (1 + phi)), kExact, "|M_i| = wL²(4+5Φ)/(120(1+Φ))");
    CHECK_NEAR(std::abs(rj[5]), w * l * l * (6 + 5 * phi) / (120 * (1 + phi)), kExact, "|M_j| = wL²(6+5Φ)/(120(1+Φ))");
    CHECK_NEAR(-ri[1], w * l * (9 + 10 * phi) / (60 * (1 + phi)), kExact, "R_i = wL(9+10Φ)/(60(1+Φ))");
    CHECK_NEAR(-ri[1] - rj[1], w * l / 2, kExact, "R_i + R_j = wL/2");
}

FEM3D_TEST(Analytical_ProppedCantileverTriangularLoad_Timoshenko)
{
    // Encastrement en i, rotule Mz en j entre nœuds encastrés (console sur appui), plan x-y :
    //   q(0) = 0, q(L) = w : M_i = 7 wL² / (30 (Φ + 4)), R_i = wL (5Φ + 27) / (30 (Φ + 4))
    //   q(0) = w, q(L) = 0 : M_i = 4 wL² / (15 (Φ + 4)), R_i = wL (5Φ + 24) / (15 (Φ + 4))
    const double l = 1.5, w = 12.0;
    const BeamSection3D s = distinctShearAreas();
    const double phi = 12 * kE * s.inertiaZ / (kG * *s.shearAreaY * l * l);
    EndReleases hinge;
    hinge.momentZEnd = true;
    struct Variant { Vector3 q0, q1; double moment, reaction; const char* label; };
    const Variant variants[] = {
        { Vector3::Zero(), Vector3(0, w, 0), 7 * w * l * l / (30 * (phi + 4)), w * l * (5 * phi + 27) / (30 * (phi + 4)), "charge croissante vers la rotule" },
        { Vector3(0, w, 0), Vector3::Zero(), 4 * w * l * l / (15 * (phi + 4)), w * l * (5 * phi + 24) / (15 * (phi + 4)), "charge décroissante vers la rotule" },
    };
    for (const Variant& v : variants)
    {
        StructuralModel3D m = clampedBar(l, BeamFormulation::Timoshenko, s, hinge);
        m.addLoadCase({ 1, "tri", {}, { { 1, LoadFrame::Local, v.q0, v.q1 } } });
        const auto r = LinearStaticAnalysis(m).run(1);
        CHECK_NEAR(std::abs(r.reactions[0].force[5]), v.moment, kExact, v.label << " : moment d'encastrement");
        CHECK_NEAR(-r.reactions[0].force[1], v.reaction, kExact, v.label << " : réaction à l'encastrement");
        CHECK(r.elements.front().endForcesLocal(11) == 0.0, v.label << " : moment nul à la rotule");
    }
}

FEM3D_TEST(Timoshenko_DistinctShearAreasPerPlane)
{
    // Asy gouverne le plan x-y (avec Iz), Asz le plan x-z (avec Iy) : une inversion serait détectée
    const double l = 1.2, fy = 30.0, fz = -50.0;
    const BeamSection3D s = distinctShearAreas();
    StructuralModel3D m = cantilever(l, BeamFormulation::Timoshenko, Vector3::UnitX(), s);
    m.addLoadCase({ 1, "F", { { 2, Vector3(0, fy, fz), Vector3::Zero() } }, {} });
    const auto r = LinearStaticAnalysis(m).run(1);
    CHECK_NEAR(tipValue(r, 2, Dof::Uy), fy * (l * l * l / (3 * kE * s.inertiaZ) + l / (kG * *s.shearAreaY)), kExact, "δy = F (L³/3EIz + L/G·Asy)");
    CHECK_NEAR(tipValue(r, 2, Dof::Uz), fz * (l * l * l / (3 * kE * s.inertiaY) + l / (kG * *s.shearAreaZ)), kExact, "δz = F (L³/3EIy + L/G·Asz)");
}

FEM3D_TEST(Solver_MechanismCriterionIndependentOfUnits)
{
    // Bloc court très raide (1 m × 1 m, L = 0,2 m) prolongé par une tige souple (Ø 40 mm, L = 4 m) : modèle stable.
    // Il doit être accepté en kN-m comme en N-mm (mêmes résultats, aux conversions près), et un mécanisme doit
    // rester détecté dans les deux systèmes.
    auto build = [](double toLength, double toForce, bool mechanism) {
        const double stress = toForce / (toLength * toLength);
        StructuralModel3D m;
        m.addMaterial({ "S", kE * stress, kG * stress });
        m.addSection(BeamSection3D::rectangle("bloc", 1.0 * toLength, 1.0 * toLength));
        m.addSection(BeamSection3D::circle("tige", 0.04 * toLength));
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(0.2 * toLength, 0, 0));
        m.addNode(3, Vector3(4.2 * toLength, 0, 0));
        m.addElement({ 1, 1, 2, "S", "bloc", BeamFormulation::EulerBernoulli, std::nullopt });
        m.addElement({ 2, 2, 3, "S", "tige", BeamFormulation::EulerBernoulli, std::nullopt });
        m.setRestraint(mechanism ? NodeRestraint::pinned(1) : NodeRestraint::fullyFixed(1));
        m.addLoadCase({ 1, "F", { { 3, Vector3(0, 0.01 * toForce, -0.02 * toForce), Vector3::Zero() } }, {} });
        return m;
    };
    const auto kNm = LinearStaticAnalysis(build(1.0, 1.0, false)).run(1);
    const auto nMm = LinearStaticAnalysis(build(1000.0, 1000.0, false)).run(1);
    CHECK_NEAR(tipValue(nMm, 3, Dof::Uz), 1000.0 * tipValue(kNm, 3, Dof::Uz), 1e-8, "même flèche en N-mm et en kN-m");
    CHECK_NEAR(tipValue(nMm, 3, Dof::Ry), tipValue(kNm, 3, Dof::Ry), 1e-8, "même rotation en N-mm et en kN-m");
    for (double unit : { 1.0, 1000.0 })
    {
        bool detected = false;
        try { LinearStaticAnalysis(build(unit, unit, true)).run(1); }
        catch (const Fem3DError& e) { detected = e.code() == ErrorCode::Mechanism; }
        CHECK(detected, "mécanisme détecté (unité de longueur × " << unit << ")");
    }
}

FEM3D_TEST(Section_RectangleTorsionMatchesExactSeries)
{
    // Solution exacte de Saint-Venant du rectangle a × b, a ≥ b (Timoshenko & Goodier, Theory of Elasticity,
    // torsion du rectangle), séries sommées ici même :
    //   J = β a b³, β = 1/3 [1 − 192/π⁵ (b/a) Σ_{n impair} tanh(nπa/2b)/n⁵]
    //   τ_max = G θ' b k, k = 1 − 8/π² Σ_{n impair} 1/(n² cosh(nπa/2b)), d'où Wt = β a b² / k
    // Tolérance 0,6 % (précision annoncée des formules approchées de Roark, avec marge), fixée avant l'essai.
    const double pi = 3.14159265358979323846;
    for (double ratio : { 1.0, 1.2, 1.5, 2.0, 2.5, 3.0, 4.0, 6.0, 10.0, 20.0 })
    {
        double s1 = 0.0, s2 = 0.0;
        for (int n = 1; n < 400; n += 2)
        {
            s1 += std::tanh(n * pi * ratio / 2) / std::pow(n, 5);
            s2 += 1.0 / (n * n * std::cosh(std::min(n * pi * ratio / 2, 700.0)));
        }
        const double beta = (1.0 - 192.0 / std::pow(pi, 5) / ratio * s1) / 3.0, k = 1.0 - 8.0 / (pi * pi) * s2;
        const double b = 0.2, a = ratio * b;
        const BeamSection3D s = BeamSection3D::rectangle("R", b, a);
        CHECK(near(s.torsionConstant, beta * a * b * b * b, 6e-3), "J pour a/b = " << ratio << " : " << s.torsionConstant << " / " << beta * a * b * b * b);
        CHECK(near(*s.torsionModulus, beta * a * b * b / k, 6e-3), "Wt pour a/b = " << ratio << " : " << *s.torsionModulus << " / " << beta * a * b * b / k);
    }
}

FEM3D_TEST(Solver_QuasiMechanismWarnsButSolves)
{
    // Pied libre en torsion, retenu par un ressort 10¹⁰ fois plus souple que la barre (GJ/L) : pivot relatif ≈ 1e-10,
    // entre le seuil de mécanisme (1e-12) et celui d'avertissement (1e-8). Le calcul aboutit avec un avertissement.
    const double l = 3.0, t = 2.0;
    const BeamSection3D s = rectangle();
    const double gjOverL = kG * s.torsionConstant / l, k = 1e-10 * gjOverL;
    StructuralModel3D m = cantilever(l, BeamFormulation::EulerBernoulli);
    m.setRestraint(NodeRestraint::custom(1, { true, true, true, false, true, true }));
    m.setSpring({ 1, { 0, 0, 0, k, 0, 0 } });
    m.addLoadCase({ 1, "T", { { 2, Vector3::Zero(), Vector3(t, 0, 0) } }, {} });
    const auto r = LinearStaticAnalysis(m).run(1);
    // Deux avertissements attendus : le quasi-mécanisme (factorisation) et, conséquence d'un conditionnement de 10¹⁰,
    // un résidu de résolution supérieur au seuil de 1e-8.
    CHECK(r.warnings.size() == 2, "deux avertissements (" << r.warnings.size() << ")");
    CHECK(r.warnings[0].find("quasi-mécanisme") != std::string::npos
              && (r.warnings[0].find("nœud 1, Rx") != std::string::npos || r.warnings[0].find("nœud 2, Rx") != std::string::npos),
          "quasi-mécanisme désigné sur le DDL de torsion : " << r.warnings[0]);
    CHECK(r.warnings[1].find("résidu de résolution élevé") != std::string::npos, "résidu signalé : " << r.warnings[1]);
    CHECK_NEAR(tipValue(r, 2, Dof::Rx), t / k + t / gjOverL, 1e-5, "rotation = T/k + T L/GJ (précision réduite attendue)");

    // Modèle sain : aucun avertissement, résidu de résolution à la précision machine
    StructuralModel3D sound = cantilever(l, BeamFormulation::EulerBernoulli);
    sound.addLoadCase({ 1, "T", { { 2, Vector3::Zero(), Vector3(t, 0, 0) } }, {} });
    const auto ok = LinearStaticAnalysis(sound).run(1);
    CHECK(ok.warnings.empty(), "aucun avertissement sur un modèle bien conditionné");
    CHECK(ok.equilibrium.solutionResidual < 1e-12, "résidu de résolution " << ok.equilibrium.solutionResidual);
}

FEM3D_TEST(Section_CircleShearCoefficientUsesPoisson)
{
    // Cowper : k = 6 (1 + ν) / (7 + 6 ν) ; ν = 0,3 par défaut, ν du matériau si fourni
    const BeamSection3D steel = BeamSection3D::circle("C", 0.3), rubberLike = BeamSection3D::circle("C", 0.3, 0.49);
    CHECK_NEAR(*steel.shearAreaY / steel.area, 6 * 1.3 / (7 + 6 * 0.3), kExact, "k(ν = 0,3)");
    CHECK_NEAR(*rubberLike.shearAreaY / rubberLike.area, 6 * 1.49 / (7 + 6 * 0.49), kExact, "k(ν = 0,49)");
    bool rejected = false;
    try { BeamSection3D::circle("C", 0.3, 0.5); } catch (const Fem3DError& e) { rejected = e.code() == ErrorCode::InvalidInput; }
    CHECK(rejected, "ν = 0,5 refusé");
}
