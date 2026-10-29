// Charges sur barre : force ponctuelle, charge répartie sur une partie de la barre, moment réparti.
// Trois familles de preuves indépendantes :
//   1. solutions analytiques de la résistance des matériaux (Euler–Bernoulli, et Timoshenko quand elles existent) ;
//   2. compatibilité exacte : barre bi-encastrée, la déformée intégrée depuis i revient à zéro en j et les efforts
//      internes en L égalent les efforts d'extrémité j (forces nodales équivalentes exactes, pour chaque type de charge,
//      chaque formulation, avec ou sans rotule) ;
//   3. invariance par subdivision : la barre chargée est comparée à la même barre découpée aux abscisses des charges,
//      chargée par le chemin déjà validé (charges nodales, charges réparties sur toute la longueur d'un élément).
// Tolérance relative 1e-9, comme les autres tests analytiques : les formulations sont exactes, seul l'arrondi subsiste.

#include "TestFramework.h"
#include "TestModels.h"

using namespace Fem3DTest;

namespace
{
constexpr double kExact = 1e-9;

LoadCaseResults solveCase(StructuralModel3D& model, LoadCase3D loadCase, int stations = 41)
{
    loadCase.id = 1;
    model.addLoadCase(loadCase);
    AnalysisOptions options;
    options.stationsPerElement = stations;
    return LinearStaticAnalysis(model, options).run(1);
}

const ElementResult& elementOf(const LoadCaseResults& r, int id = 1)
{
    for (const auto& e : r.elements)
        if (e.elementId == id) return e;
    throw std::runtime_error("élément absent");
}

const StationResult& stationNear(const ElementResult& e, double x)
{
    const StationResult* best = &e.stations.front();
    for (const auto& s : e.stations)
        if (std::abs(s.position - x) < std::abs(best->position - x)) best = &s;
    return *best;
}

const ReactionResult& reactionOf(const LoadCaseResults& r, int node)
{
    for (const auto& x : r.reactions)
        if (x.nodeId == node) return x;
    throw std::runtime_error("réaction absente");
}

const NodalResult& nodeOf(const LoadCaseResults& r, int node)
{
    for (const auto& n : r.nodes)
        if (n.nodeId == node) return n;
    throw std::runtime_error("nœud absent");
}

/// Barre droite de A à B, une seule barre ; appuis fournis par l'appelant.
StructuralModel3D straightBeam(const Vector3& a, const Vector3& b, BeamFormulation form, std::optional<Vector3> vectorXZ = std::nullopt,
                               EndReleases releases = {})
{
    StructuralModel3D m;
    m.addMaterial(steel());
    m.addSection(rectangle());
    m.addNode(1, a);
    m.addNode(2, b);
    ElementDefinition e{ 1, 1, 2, "S", "R", form, vectorXZ };
    e.releases = releases;
    m.addElement(e);
    return m;
}

const char* name(BeamFormulation f) { return f == BeamFormulation::Timoshenko ? "Timoshenko" : "Euler–Bernoulli"; }
} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// 1. Solutions analytiques
// ---------------------------------------------------------------------------------------------------------------------

FEM3D_TEST(MemberLoads_PointLoad_FixedFixed_EulerBernoulli)
{
    // Poutre bi-encastrée, force P vers −y en x = a (b = L − a) :
    //   R_i = P b² (3a + b) / L³, R_j = P a² (a + 3b) / L³, M_i = P a b² / L², M_j = −P a² b / L²,
    //   Mz(a) = 2 P a² b² / L³, v(a) = −P a³ b³ / (3 EI L³)
    const double l = 4.0, a = 1.0, b = l - a, p = 12.0;
    StructuralModel3D m = straightBeam(Vector3::Zero(), Vector3(l, 0, 0), BeamFormulation::EulerBernoulli);
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.setRestraint(NodeRestraint::fullyFixed(2));
    LoadCase3D c{ 0, "P", {}, {} };
    c.pointLoads.push_back({ 1, LoadFrame::Local, a, Vector3(0, -p, 0) });
    const auto r = solveCase(m, c);
    const double ei = kE * rectangle().inertiaZ, l3 = l * l * l;
    CHECK_NEAR(reactionOf(r, 1).force[1], p * b * b * (3 * a + b) / l3, kExact, "R_i = P b²(3a+b)/L³");
    CHECK_NEAR(reactionOf(r, 2).force[1], p * a * a * (a + 3 * b) / l3, kExact, "R_j = P a²(a+3b)/L³");
    CHECK_NEAR(reactionOf(r, 1).force[5], p * a * b * b / (l * l), kExact, "M_i = P a b²/L²");
    CHECK_NEAR(reactionOf(r, 2).force[5], -p * a * a * b / (l * l), kExact, "M_j = −P a² b/L²");
    const StationResult& under = stationNear(elementOf(r), a);
    CHECK_NEAR(under.position, a, 1e-12, "station sous la charge");
    CHECK_NEAR(under.forces.momentZ, 2 * p * a * a * b * b / l3, kExact, "Mz(a) = 2 P a² b²/L³");
    CHECK_NEAR(under.localDisplacement.y(), -p * a * a * a * b * b * b / (3 * ei * l3), kExact, "v(a) = −P a³ b³/(3 EI L³)");
    // Saut d'effort tranchant sous la charge : à x = a, valeur côté j (Vy = −R_i + P)
    CHECK_NEAR(under.forces.shearY, -p * b * b * (3 * a + b) / l3 + p, kExact, "Vy(a⁺) = P − R_i");
    CHECK_NEAR(stationNear(elementOf(r), a - 0.1).forces.shearY, -p * b * b * (3 * a + b) / l3, kExact, "Vy(x < a) = −R_i");
    CHECK(r.equilibrium.forceResidual < 1e-10 && r.equilibrium.momentResidual < 1e-10, "équilibre global");
}

FEM3D_TEST(MemberLoads_PointLoad_SimplySupported_BothFormulations)
{
    // Poutre sur deux appuis simples, force P vers −y en x = a : v(a) = −P a² b² / (3 EI L) − P a b / (L G As)
    const double l = 5.0, a = 2.0, b = l - a, p = 30.0;
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = straightBeam(Vector3::Zero(), Vector3(l, 0, 0), form);
        m.setRestraint(NodeRestraint::custom(1, { true, true, true, true, false, false }));
        m.setRestraint(NodeRestraint::custom(2, { false, true, true, false, false, false }));
        LoadCase3D c{ 0, "P", {}, {} };
        c.pointLoads.push_back({ 1, LoadFrame::Global, a, Vector3(0, -p, 0) });
        const auto r = solveCase(m, c, 51);
        const double ei = kE * rectangle().inertiaZ, gas = kG * *rectangle().shearAreaY;
        const bool timo = form == BeamFormulation::Timoshenko;
        const StationResult& under = stationNear(elementOf(r), a);
        CHECK_NEAR(under.localDisplacement.y(), -p * a * a * b * b / (3 * ei * l) - (timo ? p * a * b / (l * gas) : 0.0), kExact,
                   name(form) << " : v(a) = −P a² b²/(3EIL) (− P a b/(L G As))");
        CHECK_NEAR(under.forces.momentZ, p * a * b / l, kExact, name(form) << " : Mz(a) = P a b / L");
    }
}

FEM3D_TEST(MemberLoads_PartialUniform_Cantilever_BothFormulations)
{
    // Console encastrée en i, charge q (axe z local) sur [a, L] puis sur [0, b] :
    //   [a, L] : w_L = q (3L⁴ − 4a³L + a⁴) / (24 EI) + q ((L − a) a + (L − a)²/2) / (G As)
    //   [0, b] : w_L = q b³ (4L − b) / (24 EI) + q b² / (2 G As)
    const double l = 3.0, q = -6.0, a = 1.2, b = 1.8;
    const double ei = kE * rectangle().inertiaY, gas = kG * *rectangle().shearAreaZ;
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        const bool timo = form == BeamFormulation::Timoshenko;
        {
            StructuralModel3D m = cantilever(l, form);
            LoadCase3D c{ 0, "q[a,L]", {}, { { 1, LoadFrame::Local, Vector3(0, 0, q), Vector3(0, 0, q), a } } };
            const auto r = solveCase(m, c);
            CHECK_NEAR(tipValue(r, 2, Dof::Uz),
                       q * (3 * l * l * l * l - 4 * a * a * a * l + a * a * a * a) / (24 * ei) + (timo ? q * ((l - a) * a + (l - a) * (l - a) / 2) / gas : 0.0),
                       kExact, name(form) << " : flèche, charge sur [a, L]");
            CHECK_NEAR(stationNear(elementOf(r), 0.0).forces.shearZ, q * (l - a), kExact, name(form) << " : Vz(0) = q (L − a)");
        }
        {
            StructuralModel3D m = cantilever(l, form);
            LoadCase3D c{ 0, "q[0,b]", {}, { { 1, LoadFrame::Local, Vector3(0, 0, q), Vector3(0, 0, q), 0.0, b } } };
            const auto r = solveCase(m, c);
            CHECK_NEAR(tipValue(r, 2, Dof::Uz), q * b * b * b * (4 * l - b) / (24 * ei) + (timo ? q * b * b / (2 * gas) : 0.0), kExact,
                       name(form) << " : flèche, charge sur [0, b]");
            // Au-delà de b, la console n'est plus chargée : efforts nuls
            CHECK_NEAR(stationNear(elementOf(r), 2.4).forces.momentY, 0.0, kExact, name(form) << " : My = 0 au-delà de la charge");
        }
    }
}

FEM3D_TEST(MemberLoads_DistributedMoments_Cantilever_BothFormulations)
{
    // Console encastrée en i, couples uniformes sur [a, b] (sans effort tranchant : mêmes résultats pour les deux
    // formulations). Un couple ponctuel C en s donne en bout θ = C s / EI et v = C s² / (2 EI) + C s (L − s) / EI ;
    // intégré sur [a, b] :
    //   θz_L = cz (b² − a²) / (2 EIz),  v_L = cz (L (b² − a²)/2 − (b³ − a³)/6) / EIz ;  θy_L, w_L : idem avec −w′ = θy
    //   torsion : θx_L = t ((b − a) a + (b − a)²/2) / GJ
    const double l = 4.0, a = 0.8, b = 3.1, cz = 5.0, cy = -3.0, t = 2.5;
    const BeamSection3D s = rectangle();
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D m = cantilever(l, form);
        LoadCase3D c{ 0, "couples", {}, {} };
        c.distributedMoments.push_back({ 1, LoadFrame::Local, Vector3(t, cy, cz), Vector3(t, cy, cz), a, b });
        const auto r = solveCase(m, c);
        const double b2a2 = b * b - a * a, lever = l * b2a2 / 2 - (b * b * b - a * a * a) / 6;
        CHECK_NEAR(tipValue(r, 2, Dof::Rz), cz * b2a2 / (2 * kE * s.inertiaZ), kExact, name(form) << " : θz en bout");
        CHECK_NEAR(tipValue(r, 2, Dof::Uy), cz * lever / (kE * s.inertiaZ), kExact, name(form) << " : v en bout");
        CHECK_NEAR(tipValue(r, 2, Dof::Ry), cy * b2a2 / (2 * kE * s.inertiaY), kExact, name(form) << " : θy en bout");
        CHECK_NEAR(tipValue(r, 2, Dof::Uz), -cy * lever / (kE * s.inertiaY), kExact, name(form) << " : w en bout");
        CHECK_NEAR(tipValue(r, 2, Dof::Rx), t * ((b - a) * a + (b - a) * (b - a) / 2) / (kG * s.torsionConstant), kExact, name(form) << " : torsion en bout");
        // Réactions : couple résultant (b − a) c, aucune force
        CHECK_NEAR(reactionOf(r, 1).force[3], -t * (b - a), kExact, name(form) << " : Mx à l'encastrement");
        CHECK_NEAR(reactionOf(r, 1).force[5], -cz * (b - a), kExact, name(form) << " : Mz à l'encastrement");
        CHECK_NEAR(reactionOf(r, 1).force[1], 0.0, kExact, name(form) << " : aucune force");
        CHECK(r.equilibrium.forceResidual < 1e-10 && r.equilibrium.momentResidual < 1e-10, name(form) << " : équilibre global");
    }
}

FEM3D_TEST(MemberLoads_PointLoadAtMemberEndsActsLikeNodalLoad)
{
    // Force en x = L : comme une charge nodale au nœud j ; en x = 0 : entièrement reprise par l'encastrement
    const double l = 2.5;
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
    {
        StructuralModel3D nodal = cantilever(l, form), atEnd = cantilever(l, form), atStart = cantilever(l, form);
        const Vector3 force(3.0, -7.0, 4.0);
        const auto rn = solveCase(nodal, { 0, "nodal", { { 2, force, Vector3::Zero() } }, {} });
        LoadCase3D end{ 0, "x = L", {}, {} }, start{ 0, "x = 0", {}, {} };
        end.pointLoads.push_back({ 1, LoadFrame::Global, l, force });
        start.pointLoads.push_back({ 1, LoadFrame::Global, 0.0, force });
        const auto re = solveCase(atEnd, end), rs = solveCase(atStart, start);
        for (int k = 0; k < 6; ++k)
        {
            CHECK_NEAR(nodeOf(re, 2).displacement[static_cast<std::size_t>(k)], nodeOf(rn, 2).displacement[static_cast<std::size_t>(k)], kExact,
                       name(form) << " : x = L ≡ charge nodale, DDL " << k);
            CHECK_NEAR(nodeOf(rs, 2).displacement[static_cast<std::size_t>(k)], 0.0, kExact, name(form) << " : x = 0, aucun déplacement, DDL " << k);
        }
        CHECK_NEAR(reactionOf(rs, 1).force[1], -force.y(), kExact, name(form) << " : x = 0, réaction = −P");
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// 2. Compatibilité exacte (forces nodales équivalentes de chaque type de charge)
// ---------------------------------------------------------------------------------------------------------------------

FEM3D_TEST(MemberLoads_ExactCompatibility_FixedFixed_AllLoadTypes)
{
    // Barre inclinée bi-encastrée : ses nœuds ne bougent pas. Si f_eq est exact, la déformée reconstruite depuis i
    // (efforts d'extrémité, intégration exacte des efforts internes) revient à zéro en j, et les efforts internes en L
    // égalent les efforts d'extrémité j. Toutes les charges à la fois, repères local et global, avec ou sans rotules.
    const Vector3 a(1.0, 2.0, 0.5), b(4.0, -1.0, 3.0);
    const double l = (b - a).norm();
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
        for (int hinge = 0; hinge < 2; ++hinge)
        {
            EndReleases releases;
            if (hinge) releases.momentYEnd = releases.momentZStart = true;
            StructuralModel3D m = straightBeam(a, b, form, Vector3(0.3, 0.2, 1.0), releases);
            m.setRestraint(NodeRestraint::fullyFixed(1));
            m.setRestraint(NodeRestraint::fullyFixed(2));
            LoadCase3D c{ 0, "toutes", {}, {} };
            c.pointLoads.push_back({ 1, LoadFrame::Local, 0.37 * l, Vector3(4.0, -9.0, 6.0) });
            c.pointLoads.push_back({ 1, LoadFrame::Global, 0.8 * l, Vector3(-2.0, 5.0, -7.0) });
            c.distributedLoads.push_back({ 1, LoadFrame::Local, Vector3(1.0, -3.0, 2.0), Vector3(-2.0, 4.0, 5.0), 0.2 * l, 0.7 * l });
            c.distributedLoads.push_back({ 1, LoadFrame::Global, Vector3(0.0, 0.0, -6.0), Vector3(0.0, 0.0, -6.0), 0.5 * l });
            c.distributedMoments.push_back({ 1, LoadFrame::Local, Vector3(2.0, -1.5, 3.0), Vector3(-1.0, 2.5, 0.5), 0.1 * l, 0.6 * l });
            c.distributedMoments.push_back({ 1, LoadFrame::Global, Vector3(1.0, 1.0, -2.0), Vector3(0.0, 3.0, 1.0), 0.3 * l, 0.9 * l });
            const auto r = solveCase(m, c, 61);
            const ElementResult& e = elementOf(r);
            double scaleU = 0.0, scaleR = 0.0;
            for (const auto& s : e.stations)
            {
                scaleU = std::max(scaleU, s.localDisplacement.norm());
                scaleR = std::max(scaleR, s.sectionRotation.norm());
            }
            const StationResult& end = e.stations.back();
            CHECK(scaleU > 0.0 && end.localDisplacement.norm() <= 1e-9 * scaleU,
                  name(form) << (hinge ? " avec rotules" : "") << " : déplacement en j = " << end.localDisplacement.norm() << " (échelle " << scaleU << ")");
            if (!hinge)
                CHECK(end.sectionRotation.norm() <= 1e-9 * scaleR,
                      name(form) << " : rotation en j = " << end.sectionRotation.norm() << " (échelle " << scaleR << ")");
            const SectionForces& f = end.forces;
            const double values[6] = { f.axial, f.shearY, f.shearZ, f.torsion, f.momentY, f.momentZ };
            double scaleF = 0.0;
            for (int k = 0; k < 12; ++k) scaleF = std::max(scaleF, std::abs(e.endForcesLocal(k)));
            for (int k = 0; k < 6; ++k)
                CHECK(std::abs(values[k] - e.endForcesLocal(6 + k)) <= 1e-9 * scaleF,
                      name(form) << (hinge ? " avec rotules" : "") << " : effort interne en L = effort d'extrémité j, composante " << k << " ("
                                 << values[k] << " / " << e.endForcesLocal(6 + k) << ")");
            CHECK(r.equilibrium.forceResidual <= 1e-9 * r.equilibrium.appliedForce.norm()
                      && r.equilibrium.momentResidual <= 1e-9 * r.equilibrium.appliedMoment.norm(),
                  name(form) << " : équilibre global (|ΣF| " << r.equilibrium.forceResidual << ", |ΣM| " << r.equilibrium.momentResidual << ")");
        }
}

// ---------------------------------------------------------------------------------------------------------------------
// 3. Invariance par subdivision
// ---------------------------------------------------------------------------------------------------------------------

FEM3D_TEST(MemberLoads_SubdivisionInvariance_PointAndPartialLoads)
{
    // Barre inclinée encastrée en A, appui à rotule en B ; force ponctuelle en a1 et charge trapézoïdale sur [a2, b2].
    // Référence : barre découpée en a1, a2, b2 ; force → charge nodale ; charge partielle → charge sur toute la longueur
    // de l'élément [a2, b2]. Rotule de flexion en j : portée par le dernier sous-élément.
    const Vector3 pa(0.5, 1.0, 0.0), pb(5.5, 2.0, 2.5), xz(0.0, 0.0, 1.0);
    const double l = (pb - pa).norm();
    const Vector3 axis = (pb - pa) / l;
    const double a1 = 0.3 * l, a2 = 0.45 * l, b2 = 0.8 * l;
    const Vector3 force(3.0, -12.0, 5.0), qa(1.0, -4.0, 2.5), qb(-0.5, -1.0, 6.0);
    for (auto form : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
        for (int hinge = 0; hinge < 2; ++hinge)
        {
            EndReleases releases;
            if (hinge) releases.momentZEnd = true;
            auto supports = [](StructuralModel3D& m, int nodeB) {
                m.setRestraint(NodeRestraint::fullyFixed(1));
                m.setRestraint(NodeRestraint::custom(nodeB, { true, true, true, true, false, false }));
            };
            StructuralModel3D single = straightBeam(pa, pb, form, xz, releases);
            supports(single, 2);
            LoadCase3D c{ 0, "barre", {}, {} };
            c.pointLoads.push_back({ 1, LoadFrame::Global, a1, force });
            c.distributedLoads.push_back({ 1, LoadFrame::Local, qa, qb, a2, b2 });
            const auto rs = solveCase(single, c, 21);

            StructuralModel3D split;
            split.addMaterial(steel());
            split.addSection(rectangle());
            split.addNode(1, pa);
            split.addNode(3, pa + a1 * axis);
            split.addNode(4, pa + a2 * axis);
            split.addNode(5, pa + b2 * axis);
            split.addNode(2, pb);
            split.addElement({ 1, 1, 3, "S", "R", form, xz });
            split.addElement({ 2, 3, 4, "S", "R", form, xz });
            split.addElement({ 3, 4, 5, "S", "R", form, xz });
            ElementDefinition last{ 4, 5, 2, "S", "R", form, xz };
            last.releases = releases;
            split.addElement(last);
            supports(split, 2);
            LoadCase3D d{ 0, "découpée", { { 3, force, Vector3::Zero() } }, { { 3, LoadFrame::Local, qa, qb } } };
            const auto rd = solveCase(split, d, 21);

            const ElementResult& e = elementOf(rs);
            double scale = 0.0;
            for (const auto& s : e.stations) scale = std::max(scale, s.globalDisplacement.norm());
            const std::pair<double, int> checkpoints[3] = { { a1, 3 }, { a2, 4 }, { b2, 5 } };
            for (const auto& [x, node] : checkpoints)
            {
                const StationResult& st = stationNear(e, x);
                CHECK_NEAR(st.position, x, 1e-12, "station à l'abscisse de découpe");
                const auto& u = nodeOf(rd, node).displacement;
                CHECK((st.globalDisplacement - Vector3(u[0], u[1], u[2])).norm() <= 1e-9 * scale,
                      name(form) << (hinge ? " avec rotule" : "") << " : déplacement en x = " << x << " (écart " << (st.globalDisplacement - Vector3(u[0], u[1], u[2])).norm()
                                 << ", échelle " << scale << ")");
            }
            for (int node : { 1, 2 })
                for (int k = 0; k < 6; ++k)
                    CHECK_NEAR(reactionOf(rs, node).force[static_cast<std::size_t>(k)], reactionOf(rd, node).force[static_cast<std::size_t>(k)], kExact,
                               name(form) << (hinge ? " avec rotule" : "") << " : réaction nœud " << node << ", composante " << k);
            for (int k = 0; k < 6; ++k)
                CHECK_NEAR(nodeOf(rs, 2).displacement[static_cast<std::size_t>(k)], nodeOf(rd, 2).displacement[static_cast<std::size_t>(k)], kExact,
                           name(form) << (hinge ? " avec rotule" : "") << " : déplacement du nœud B, DDL " << k);
            // Efforts internes de part et d'autre de la force ponctuelle : élément 2 de la référence commence en a1
            const SectionForces& after = stationNear(e, a1).forces;
            const SectionForces& ref = elementOf(rd, 2).stations.front().forces;
            CHECK_NEAR(after.shearY, ref.shearY, kExact, name(form) << " : Vy juste après la force");
            CHECK_NEAR(after.momentZ, ref.momentZ, kExact, name(form) << " : Mz sous la force");
            CHECK_NEAR(after.axial, ref.axial, kExact, name(form) << " : N juste après la force");
        }
}

// ---------------------------------------------------------------------------------------------------------------------
// Données invalides : refusées avec un message qui désigne la charge, le cas et la barre
// ---------------------------------------------------------------------------------------------------------------------

FEM3D_TEST(MemberLoads_InvalidPositionsRejected)
{
    const double l = 3.0;
    auto rejected = [&](LoadCase3D c, const std::string& expected) {
        StructuralModel3D m = cantilever(l, BeamFormulation::EulerBernoulli);
        c.id = 1;
        m.addLoadCase(c);
        try
        {
            LinearStaticAnalysis analysis(m);
        }
        catch (const Fem3DError& e)
        {
            const std::string what = e.what();
            CHECK(what.find(expected) != std::string::npos && what.find("élément 1") != std::string::npos,
                  "message « " << what << " » (attendu : " << expected << ")");
            return;
        }
        CHECK(false, "donnée invalide acceptée (attendu : " << expected << ")");
    };
    LoadCase3D c{ 0, "x", {}, {} };
    c.pointLoads.push_back({ 1, LoadFrame::Local, -0.1, Vector3(0, -1, 0) });
    rejected(c, "hors de la barre");
    c.pointLoads = { { 1, LoadFrame::Local, l + 0.01, Vector3(0, -1, 0) } };
    rejected(c, "hors de la barre");
    c.pointLoads = { { 1, LoadFrame::Local, 1.0, Vector3(0, std::nan(""), 0) } };
    rejected(c, "non finie");
    c.pointLoads.clear();
    c.distributedLoads = { { 1, LoadFrame::Local, Vector3(0, 0, -1), Vector3(0, 0, -1), 2.0, 1.0 } };
    rejected(c, "intervalle vide ou inversé");
    c.distributedLoads = { { 1, LoadFrame::Local, Vector3(0, 0, -1), Vector3(0, 0, -1), 1.0, 1.0 } };
    rejected(c, "intervalle vide ou inversé");
    c.distributedLoads = { { 1, LoadFrame::Local, Vector3(0, 0, -1), Vector3(0, 0, -1), 0.0, 4.0 } };
    rejected(c, "hors de la barre");
    c.distributedLoads.clear();
    c.distributedMoments = { { 1, LoadFrame::Local, Vector3(1, 0, 0), Vector3(1, 0, 0), 2.5, 0.5 } };
    rejected(c, "intervalle vide ou inversé");
    // Une position à l'arrondi près de L est acceptée et ramenée sur la barre
    StructuralModel3D m = cantilever(l, BeamFormulation::EulerBernoulli);
    LoadCase3D ok{ 1, "arrondi", {}, {} };
    ok.pointLoads.push_back({ 1, LoadFrame::Local, l * (1 + 1e-12), Vector3(0, -1, 0) });
    m.addLoadCase(ok);
    bool accepted = true;
    try { LinearStaticAnalysis(m).run(1); }
    catch (const Fem3DError&) { accepted = false; }
    CHECK(accepted, "position L (1 + 1e-12) acceptée");
}
