// Comparaison du moteur Fem3D avec OpenSees (benchmarks/OpenSees/README.md).
//
// Pour chaque cas et chaque formulation : calcul TSA, script Tcl équivalent (mêmes nœuds, mêmes vecteurs v_xz des
// geomTransf, mêmes propriétés, mêmes charges), exécution d'OpenSees, comparaison des déplacements nodaux, des réactions
// et des efforts d'extrémité (localForce). Correspondance des éléments :
//   Euler–Bernoulli → elasticBeamColumn (rigidité de poutre élancée, cisaillement transversal négligé)
//   Timoshenko      → ElasticTimoshenkoBeam (aires de cisaillement Avy = Asy, Avz = Asz)
// Rotules (relâchements My, Mz) :
//   Euler–Bernoulli → options natives -releasey / -releasez d'elasticBeamColumn (1 : nœud i, 2 : nœud j, 3 : les deux)
//   Timoshenko      → ElasticTimoshenkoBeam n'a pas ces options : nœud doublé à l'extrémité, relié au nœud par
//                     equalDOF sur tous les DDL sauf la rotation globale de l'axe local relâché (exact si cet axe est
//                     un axe global ; sinon cas « NON COMPARABLE »). La rotation de ce nœud doublé est la rotation de
//                     la barre à la rotule : elle est comparée à celle reconstituée par TSA.
// Appuis élastiques (ressorts nodaux, repère global) : élément zeroLength (orientation par défaut = axes globaux)
// entre un nœud fixe doublé (200000 + nœud) et le nœud, un matériau Elastic par DDL retenu. La force du ressort sur la
// structure est la réaction du nœud fixe doublé (nodeReaction) : elle est comparée à −k·u de TSA.
//
// Tolérances fixées avant les essais (justification : docs/validation/VALIDATION_OPENSEES.md §3) :
//   |TSA − OpenSees| ≤ max(1e-6 · |OpenSees|, 1e-9 · échelle de la grandeur)
// L'échelle est la plus grande valeur absolue de la grandeur dans le cas (translations, rotations, forces, moments) :
// critère absolu pour les valeurs proches de zéro, où l'écart relatif n'a pas de sens.
//
// Usage : fem3d_opensees_benchmarks [chemin d'OpenSees.exe] [dossier de sortie]

#include "Fem3D/Analysis/LinearStaticAnalysis.h"
#include "Fem3D/Core/Fem3DError.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace TSA::Fem3D;
namespace fs = std::filesystem;

namespace
{
constexpr double kRelativeTolerance = 1e-6;
constexpr double kAbsoluteToleranceFactor = 1e-9;
constexpr double kE = 210e6, kG = 81e6;   // kN/m²
constexpr int kHingeNodeBase = 100000;   // nœuds doublés des rotules : 100000 + 10 · élément + extrémité (0 : i, 1 : j)
constexpr int kSpringNodeBase = 200000;  // nœuds fixes des ressorts : 200000 + nœud (même tag pour l'élément zeroLength)

struct Case
{
    std::string id;
    std::string title;
    std::function<StructuralModel3D(BeamFormulation)> build;
    bool expectMechanism = false;
};

// ---------------------------------------------------------------------------------------------------------------
// Modèles de référence (unités kN, m)
// ---------------------------------------------------------------------------------------------------------------

StructuralModel3D base(BeamFormulation)
{
    StructuralModel3D m;
    m.addMaterial({ "S235", kE, kG });
    m.addSection(BeamSection3D::rectangle("R30x50", 0.3, 0.5));
    m.addSection(BeamSection3D::rectangle("R20x40", 0.2, 0.4));
    m.addSection(BeamSection3D::circle("D300", 0.3));
    return m;
}

StructuralModel3D cantileverCase(BeamFormulation f, double length, const std::string& section, const LoadCase3D& loads)
{
    StructuralModel3D m = base(f);
    m.addNode(1, Vector3::Zero());
    m.addNode(2, Vector3(length, 0, 0));
    m.addElement({ 1, 1, 2, "S235", section, f, Vector3(0, 0, 1) });
    m.setRestraint(NodeRestraint::fullyFixed(1));
    m.addLoadCase(loads);
    return m;
}

StructuralModel3D spatialFrame(BeamFormulation f, bool distributed)
{
    StructuralModel3D m = base(f);
    const Vector3 foot[4] = { { 0, 0, 0 }, { 6, 0, 0 }, { 6.5, 4, 0 }, { -0.5, 4.5, 0 } };
    const double height[4] = { 3.0, 3.5, 4.0, 3.2 };
    for (int k = 0; k < 4; ++k)
    {
        m.addNode(1 + k, foot[k]);
        m.addNode(11 + k, foot[k] + Vector3(0, 0, height[k]));
        m.setRestraint(NodeRestraint::fullyFixed(1 + k));
        m.addElement({ 1 + k, 1 + k, 11 + k, "S235", "R30x50", f, Vector3(-1, 0, 0) });
    }
    for (int k = 0; k < 4; ++k) m.addElement({ 11 + k, 11 + k, 11 + (k + 1) % 4, "S235", "R20x40", f, Vector3(0, 0, 1) });
    LoadCase3D c{ 1, "vent + gravité", {}, {} };
    c.nodalLoads.push_back({ 11, Vector3(15, -5, -40), Vector3(0, 0, 3) });
    c.nodalLoads.push_back({ 13, Vector3(0, 10, -60), Vector3(2, 0, 0) });
    if (distributed)
    {
        c.distributedLoads.push_back({ 11, LoadFrame::Local, Vector3(1.5, 2.0, -12.0), Vector3(1.5, 2.0, -12.0) });
        c.distributedLoads.push_back({ 12, LoadFrame::Global, Vector3(0, 0, -9.0), Vector3(0, 0, -9.0) });
        c.distributedLoads.push_back({ 13, LoadFrame::Global, Vector3(3.0, -1.0, -6.0), Vector3(3.0, -1.0, -6.0) });
        c.distributedLoads.push_back({ 2, LoadFrame::Global, Vector3(4.0, 0, 0), Vector3(4.0, 0, 0) });   // vent sur un poteau
    }
    m.addLoadCase(c);
    return m;
}

std::vector<Case> cases()
{
    std::vector<Case> list;
    list.push_back({ "01_traction", "Traction axiale (console, L = 4 m)", [](BeamFormulation f) {
        return cantileverCase(f, 4.0, "R30x50", { 1, "N", { { 2, Vector3(100, 0, 0), Vector3::Zero() } }, {} });
    } });
    list.push_back({ "02_torsion", "Torsion de Saint-Venant (console, L = 4 m)", [](BeamFormulation f) {
        return cantileverCase(f, 4.0, "R30x50", { 1, "T", { { 2, Vector3::Zero(), Vector3(12, 0, 0) } }, {} });
    } });
    list.push_back({ "03_flexion_y", "Flexion autour de y local (console, charge Fz)", [](BeamFormulation f) {
        return cantileverCase(f, 3.0, "R30x50", { 1, "Fz", { { 2, Vector3(0, 0, -20), Vector3::Zero() } }, {} });
    } });
    list.push_back({ "04_flexion_z", "Flexion autour de z local (console, charge Fy)", [](BeamFormulation f) {
        return cantileverCase(f, 3.0, "R30x50", { 1, "Fy", { { 2, Vector3(0, 20, 0), Vector3::Zero() } }, {} });
    } });
    list.push_back({ "05_console_transversale", "Console sous charges transversales combinées et charge répartie locale", [](BeamFormulation f) {
        return cantileverCase(f, 4.0, "R30x50",
                              { 1, "combinée", { { 2, Vector3(0, 15, -25), Vector3(0, 5, -3) } }, { { 1, LoadFrame::Local, Vector3(0, 0, -8), Vector3(0, 0, -8) } } });
    } });
    list.push_back({ "06_poutre_courte", "Poutre courte L/h = 2 (cisaillement significatif)", [](BeamFormulation f) {
        return cantileverCase(f, 1.0, "R30x50", { 1, "F", { { 2, Vector3(0, 30, -50), Vector3::Zero() } }, {} });
    } });
    list.push_back({ "07_barre_inclinee", "Barre inclinée dans l'espace, orientation v_xz imposée", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3(1, 2, 0.5));
        m.addNode(2, Vector3(4, 5, 4));
        m.addElement({ 1, 1, 2, "S235", "R30x50", f, Vector3(0.3, -0.2, 1.0) });
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.addLoadCase({ 1, "F", { { 2, Vector3(10, -20, 15), Vector3(1, 2, -3) } }, { { 1, LoadFrame::Global, Vector3(0, 0, -5), Vector3(0, 0, -5) } } });
        return m;
    } });
    list.push_back({ "08_portique_spatial", "Portique spatial multiéléments (hauteurs et portées différentes)", [](BeamFormulation f) {
        return spatialFrame(f, false);
    } });
    list.push_back({ "09_charges_reparties", "Portique spatial, charges réparties locales et globales", [](BeamFormulation f) {
        return spatialFrame(f, true);
    } });
    list.push_back({ "10_geometrie_irreguliere", "Géométrie irrégulière : arbalétriers inclinés, contreventement, sections différentes", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        const Vector3 p[] = { { 0, 0, 0 }, { 8, 0, 0 }, { 0, 0, 4 }, { 8, 0, 4.5 }, { 4, 0, 6.2 }, { 0, 5, 0 }, { 0, 5, 4 }, { 8, 5.5, 0 }, { 8, 5.5, 4.5 } };
        for (int k = 0; k < 9; ++k) m.addNode(k + 1, p[k]);
        for (int n : { 1, 2, 6, 8 }) m.setRestraint(NodeRestraint::fullyFixed(n));
        m.setRestraint(NodeRestraint::custom(8, { true, true, true, false, false, true }));   // appui partiel
        int e = 1;
        auto add = [&](int a, int b, const char* section, Vector3 v) { m.addElement({ e++, a, b, "S235", section, f, v }); };
        add(1, 3, "R30x50", Vector3(-1, 0, 0));
        add(2, 4, "R30x50", Vector3(-1, 0, 0));
        add(3, 5, "R20x40", Vector3(0, 0, 1));
        add(5, 4, "R20x40", Vector3(0, 0, 1));
        add(6, 7, "D300", Vector3(-1, 0, 0));
        add(8, 9, "D300", Vector3(-1, 0, 0));
        add(3, 7, "R20x40", Vector3(0, 0, 1));
        add(4, 9, "R20x40", Vector3(0, 0, 1));
        add(1, 7, "D300", Vector3(0, 0, 1));   // contreventement
        m.addLoadCase({ 1, "neige + vent",
                        { { 5, Vector3(5, 3, -30), Vector3::Zero() }, { 9, Vector3(0, -8, -12), Vector3(0, 1.5, 0) } },
                        { { 3, LoadFrame::Global, Vector3(0, 0, -7), Vector3(0, 0, -7) }, { 4, LoadFrame::Global, Vector3(0, 0, -7), Vector3(0, 0, -7) },
                          { 5, LoadFrame::Local, Vector3(0, 2.5, 0), Vector3(0, 2.5, 0) } } });
        return m;
    } });
    list.push_back({ "12_poutre_biarticulee", "Rotules My et Mz aux deux extrémités entre nœuds encastrés, charges réparties", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(6, 0, 0));
        ElementDefinition e{ 1, 1, 2, "S235", "R30x50", f, Vector3(0, 0, 1) };
        e.releases = EndReleases::pinnedBothEnds();
        m.addElement(e);
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.setRestraint(NodeRestraint::custom(2, { false, true, true, true, true, true }));
        m.addLoadCase({ 1, "q", { { 2, Vector3(50, 0, 0), Vector3::Zero() } }, { { 1, LoadFrame::Local, Vector3(0, 4, -10), Vector3(0, 4, -10) } } });
        return m;
    } });
    list.push_back({ "13_rotule_une_extremite", "Rotule Mz au nœud j seulement (barre selon Y), console sur appui", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(0, 5, 0));
        ElementDefinition e{ 1, 1, 2, "S235", "R30x50", f, Vector3(0, 0, 1) };
        e.releases.momentZEnd = true;
        m.addElement(e);
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.setRestraint(NodeRestraint::custom(2, { true, false, true, true, true, true }));
        m.addLoadCase({ 1, "q", { { 2, Vector3(0, 0, -15), Vector3::Zero() } }, { { 1, LoadFrame::Global, Vector3(-8, 0, 0), Vector3(-8, 0, 0) } } });
        return m;
    } });
    list.push_back({ "14_portique_poutres_articulees", "Portique spatial rectangulaire, poutres articulées (My, Mz) aux deux extrémités", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        const Vector3 foot[4] = { { 0, 0, 0 }, { 6, 0, 0 }, { 6, 4, 0 }, { 0, 4, 0 } };
        for (int k = 0; k < 4; ++k)
        {
            m.addNode(1 + k, foot[k]);
            m.addNode(11 + k, foot[k] + Vector3(0, 0, 3.5));
            m.setRestraint(NodeRestraint::fullyFixed(1 + k));
            m.addElement({ 1 + k, 1 + k, 11 + k, "S235", "R30x50", f, Vector3(-1, 0, 0) });
        }
        for (int k = 0; k < 4; ++k)
        {
            ElementDefinition beam{ 11 + k, 11 + k, 11 + (k + 1) % 4, "S235", "R20x40", f, Vector3(0, 0, 1) };
            beam.releases = EndReleases::pinnedBothEnds();
            m.addElement(beam);
        }
        LoadCase3D c{ 1, "G + W", { { 11, Vector3(12, 5, 0), Vector3::Zero() }, { 13, Vector3(0, -6, 0), Vector3(0, 0, 2) } }, {} };
        for (int k = 0; k < 4; ++k) c.distributedLoads.push_back({ 11 + k, LoadFrame::Global, Vector3(0, 0, -15), Vector3(0, 0, -15) });
        m.addLoadCase(c);
        return m;
    } });
    list.push_back({ "15_rotule_intermediaire", "Rotule intermédiaire (Gerber) entre deux consoles, charges nodale et répartie", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(3, 0, 0));
        m.addNode(3, Vector3(7, 0, 0));
        ElementDefinition left{ 1, 1, 2, "S235", "R30x50", f, Vector3(0, 0, 1) };
        left.releases.momentYEnd = left.releases.momentZEnd = true;
        m.addElement(left);
        m.addElement({ 2, 2, 3, "S235", "R30x50", f, Vector3(0, 0, 1) });
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.setRestraint(NodeRestraint::fullyFixed(3));
        m.addLoadCase({ 1, "P + q", { { 2, Vector3(0, 6, -40), Vector3(1.5, 0, 0) } }, { { 2, LoadFrame::Local, Vector3(0, 0, -5), Vector3(0, 0, -5) } } });
        return m;
    } });
    list.push_back({ "16_barre_inclinee_articulee", "Barre inclinée dans l'espace, rotule My au nœud i", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3(1, 2, 0.5));
        m.addNode(2, Vector3(4, 5, 4));
        m.addNode(3, Vector3(6, 3, 4));
        ElementDefinition e{ 1, 1, 2, "S235", "R30x50", f, Vector3(0.3, -0.2, 1.0) };
        e.releases.momentYStart = true;
        m.addElement(e);
        m.addElement({ 2, 2, 3, "S235", "R20x40", f, Vector3(0, 0, 1) });
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.setRestraint(NodeRestraint::fullyFixed(3));
        m.addLoadCase({ 1, "F", { { 2, Vector3(10, -20, 15), Vector3(1, 2, -3) } }, { { 1, LoadFrame::Global, Vector3(0, 0, -5), Vector3(0, 0, -5) } } });
        return m;
    } });
    list.push_back({ "18_ressorts_translation", "Console sur appui élastique en bout (ky, kz), charges nodale et répartie", [](BeamFormulation f) {
        StructuralModel3D m = cantileverCase(f, 4.0, "R30x50",
                                             { 1, "F", { { 2, Vector3(5, 12, -25), Vector3(0, 0, 2) } }, { { 1, LoadFrame::Local, Vector3(0, 0, -6), Vector3(0, 0, -6) } } });
        m.setSpring({ 2, { 0, 3000, 5000, 0, 0, 0 } });
        return m;
    } });
    list.push_back({ "19_ressorts_rotation", "Poteau sur pied articulé retenu par trois ressorts de rotation, charges spatiales", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(0, 0, 3.5));
        m.addElement({ 1, 1, 2, "S235", "R30x50", f, Vector3(-1, 0, 0) });
        m.setRestraint(NodeRestraint::pinned(1));
        m.setSpring({ 1, { 0, 0, 0, 4.0e4, 2.5e4, 1.0e4 } });
        m.addLoadCase({ 1, "F", { { 2, Vector3(8, -6, -30), Vector3(0, 0, 1.5) } }, { { 1, LoadFrame::Global, Vector3(2, 0, 0), Vector3(2, 0, 0) } } });
        return m;
    } });
    list.push_back({ "20_portique_appuis_elastiques", "Portique spatial sur appuis élastiques (pieds semi-rigides, un pied entièrement sur ressorts)", [](BeamFormulation f) {
        StructuralModel3D m = spatialFrame(f, true);
        for (int k = 1; k <= 3; ++k)
        {
            m.setRestraint(NodeRestraint::pinned(k));
            m.setSpring({ k, { 0, 0, 0, 6.0e4 * k, 4.0e4 * k, 2.0e4 } });
        }
        m.setRestraint(NodeRestraint::custom(4, { false, false, false, false, false, false }));
        m.setSpring({ 4, { 2.0e5, 1.5e5, 8.0e5, 5.0e4, 5.0e4, 3.0e4 } });
        return m;
    } });
    list.push_back({ "17_mecanisme_rotule", "Mécanisme : console relâchée (My) au droit de son encastrement", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(3, 0, 0));
        ElementDefinition e{ 1, 1, 2, "S235", "R30x50", f, Vector3(0, 0, 1) };
        e.releases.momentYStart = true;
        m.addElement(e);
        m.setRestraint(NodeRestraint::fullyFixed(1));
        m.addLoadCase({ 1, "F", { { 2, Vector3(0, 0, -10), Vector3::Zero() } }, {} });
        return m;
    }, true });
    list.push_back({ "11_mecanisme", "Mécanisme : rotation autour de l'axe libre aux deux appuis", [](BeamFormulation f) {
        StructuralModel3D m = base(f);
        m.addNode(1, Vector3::Zero());
        m.addNode(2, Vector3(4, 0, 0));
        m.addElement({ 1, 1, 2, "S235", "R30x50", f, Vector3(0, 0, 1) });
        m.setRestraint(NodeRestraint::pinned(1));
        m.setRestraint(NodeRestraint::custom(2, { false, true, true, false, false, false }));
        m.addLoadCase({ 1, "F", { { 2, Vector3(0, 0, -10), Vector3::Zero() } }, {} });
        return m;
    }, true });
    return list;
}

// ---------------------------------------------------------------------------------------------------------------
// Script OpenSees
// ---------------------------------------------------------------------------------------------------------------

std::string tclPath(const fs::path& p)
{
    std::string s = p.generic_string();
    return "{" + s + "}";
}

/// Écrit le script ; rend false (et la raison) si une charge n'a pas d'équivalent dans OpenSees 3.8.0.
bool writeTcl(const StructuralModel3D& m, BeamFormulation f, const fs::path& tcl, const fs::path& output, std::string* unsupported)
{
    std::ofstream o(tcl);
    o << std::setprecision(17);
    o << "# Généré par fem3d_opensees_benchmarks : modèle identique au calcul TSA (kN, m)\n";
    o << "wipe\nmodel BasicBuilder -ndm 3 -ndf 6\n";
    for (const auto& n : m.nodes()) o << "node " << n.id << ' ' << n.position.x() << ' ' << n.position.y() << ' ' << n.position.z() << '\n';
    for (const auto& [id, r] : m.restraints())
    {
        o << "fix " << id;
        for (bool b : r.fixed) o << ' ' << (b ? 1 : 0);
        o << '\n';
    }
    for (const auto& [id, spring] : m.springs())
    {
        const Vector3& p = m.node(id).position;
        const int ground = kSpringNodeBase + id;
        o << "node " << ground << ' ' << p.x() << ' ' << p.y() << ' ' << p.z() << "\nfix " << ground << " 1 1 1 1 1 1\n";
        std::string mats, dirs;
        for (int k = 0; k < kDofsPerNode; ++k)
        {
            const double value = spring.stiffness[static_cast<std::size_t>(k)];
            if (value == 0.0) continue;
            const int tag = 10 * ground + k + 1;
            std::ostringstream mat;
            mat << std::setprecision(17) << "uniaxialMaterial Elastic " << tag << ' ' << value << '\n';
            o << mat.str();
            mats += ' ' + std::to_string(tag);
            dirs += ' ' + std::to_string(k + 1);
        }
        if (!mats.empty()) o << "element zeroLength " << ground << ' ' << ground << ' ' << id << " -mat" << mats << " -dir" << dirs << '\n';
    }
    // Rotules de Timoshenko : nœud doublé + equalDOF sur les DDL non relâchés (axes locaux relâchés = axes globaux)
    std::map<int, std::pair<int, int>> ends;
    for (const auto& d : m.elements())
    {
        std::pair<int, int> nodes{ d.startNodeId, d.endNodeId };
        if (d.releases.any() && f == BeamFormulation::Timoshenko)
        {
            const Matrix3& r = m.createElement(d)->localAxes().rotation();
            auto globalAxisOf = [&](int localRow) {
                for (int g = 0; g < 3; ++g)
                    if (std::abs(std::abs(r(localRow, g)) - 1.0) < 1e-12) return g;
                return -1;
            };
            for (int end = 0; end < 2; ++end)
            {
                const bool releasedY = end == 0 ? d.releases.momentYStart : d.releases.momentYEnd;
                const bool releasedZ = end == 0 ? d.releases.momentZStart : d.releases.momentZEnd;
                if (!releasedY && !releasedZ) continue;
                std::array<bool, 6> constrained{ true, true, true, true, true, true };
                for (const auto& [released, localRow] : { std::pair{ releasedY, 1 }, std::pair{ releasedZ, 2 } })
                {
                    if (!released) continue;
                    const int g = globalAxisOf(localRow);
                    if (g < 0)
                    {
                        *unsupported = "rotule de Timoshenko sur un axe local non parallèle à un axe global (référence equalDOF inexacte, "
                                       "ElasticTimoshenkoBeam sans relâchements)";
                        return false;
                    }
                    constrained[static_cast<std::size_t>(3 + g)] = false;
                }
                const int node = end == 0 ? d.startNodeId : d.endNodeId;
                const int hinge = kHingeNodeBase + 10 * d.id + end;
                const Vector3& pos = m.node(node).position;
                o << "node " << hinge << ' ' << pos.x() << ' ' << pos.y() << ' ' << pos.z() << '\n';
                o << "equalDOF " << node << ' ' << hinge;
                for (int k = 0; k < 6; ++k)
                    if (constrained[static_cast<std::size_t>(k)]) o << ' ' << k + 1;
                o << '\n';
                (end == 0 ? nodes.first : nodes.second) = hinge;
            }
        }
        ends[d.id] = nodes;
    }
    for (const auto& d : m.elements())
    {
        const auto e = m.createElement(d);
        const Vector3 v = e->localAxes().vectorXZ();
        const int ni = ends[d.id].first, nj = ends[d.id].second;
        const int codeY = (d.releases.momentYStart ? 1 : 0) + (d.releases.momentYEnd ? 2 : 0);
        const int codeZ = (d.releases.momentZStart ? 1 : 0) + (d.releases.momentZEnd ? 2 : 0);
        const auto& s = e->section();
        const auto& mat = e->material();
        o << "geomTransf Linear " << d.id << ' ' << v.x() << ' ' << v.y() << ' ' << v.z() << '\n';
        if (f == BeamFormulation::Timoshenko)
            o << "element ElasticTimoshenkoBeam " << d.id << ' ' << ni << ' ' << nj << ' ' << mat.youngModulus << ' ' << mat.shearModulus << ' '
              << s.area << ' ' << s.torsionConstant << ' ' << s.inertiaY << ' ' << s.inertiaZ << ' ' << *s.shearAreaY << ' ' << *s.shearAreaZ << ' ' << d.id << '\n';
        else
            o << "element elasticBeamColumn " << d.id << ' ' << ni << ' ' << nj << ' ' << s.area << ' ' << mat.youngModulus << ' '
              << mat.shearModulus << ' ' << s.torsionConstant << ' ' << s.inertiaY << ' ' << s.inertiaZ << ' ' << d.id
              << (codeY ? " -releasey " + std::to_string(codeY) : std::string()) << (codeZ ? " -releasez " + std::to_string(codeZ) : std::string()) << '\n';
    }
    const LoadCase3D& c = m.loadCases().front();
    o << "timeSeries Linear 1\npattern Plain 1 1 {\n";
    for (const auto& l : c.nodalLoads)
        o << "  load " << l.nodeId << ' ' << l.force.x() << ' ' << l.force.y() << ' ' << l.force.z() << ' ' << l.moment.x() << ' ' << l.moment.y() << ' ' << l.moment.z() << '\n';
    for (const auto& l : c.distributedLoads)
    {
        if (!(l.startIntensity - l.endIntensity).isZero(0.0))
        {
            *unsupported = "charge répartie trapézoïdale (non prise en compte par -beamUniform en 3D dans OpenSees 3.8.0)";
            return false;
        }
        // OpenSees : charge uniforme en composantes locales (Wy, Wz, Wx) ; charge globale projetée (même projection que TSA)
        const auto e = m.createElement(*std::find_if(m.elements().begin(), m.elements().end(), [&](const ElementDefinition& d) { return d.id == l.elementId; }));
        const Vector3 q = l.frame == LoadFrame::Global ? Vector3(e->localAxes().rotation() * l.startIntensity) : l.startIntensity;
        o << "  eleLoad -ele " << l.elementId << " -type -beamUniform " << q.y() << ' ' << q.z() << ' ' << q.x() << '\n';
    }
    o << "}\nsystem BandGeneral\nnumberer RCM\nconstraints Transformation\nintegrator LoadControl 1.0\nalgorithm Linear\nanalysis Static\n";
    o << "set status [analyze 1]\nreactions\n";
    o << "set f [open " << tclPath(output) << " w]\n";
    o << "puts $f \"version [version]\"\nputs $f \"status $status\"\n";
    o << "proc line {f tag id values} { set s \"$tag $id\"; foreach v $values { append s \" \" [format %.17g $v] }; puts $f $s }\n";
    for (const auto& n : m.nodes()) o << "line $f node " << n.id << " [nodeDisp " << n.id << "]\n";
    for (const auto& [id, r] : m.restraints()) o << "line $f reaction " << id << " [nodeReaction " << id << "]\n";
    for (const auto& [id, spring] : m.springs()) o << "line $f spring " << id << " [nodeReaction " << kSpringNodeBase + id << "]\n";
    for (const auto& d : m.elements()) o << "line $f element " << d.id << " [eleResponse " << d.id << " localForce]\n";
    for (const auto& [id, nodes] : ends)
        for (int hinge : { nodes.first, nodes.second })
            if (hinge >= kHingeNodeBase)
            {
                o << "line $f hinge " << hinge << " [nodeDisp " << hinge << "]\n";
                o << "line $f hingereaction " << hinge << " [nodeReaction " << hinge << "]\n";
            }
    o << "close $f\n";
    return true;
}

struct OpenSeesOutput
{
    std::string version;
    int status = -999;
    std::map<int, std::vector<double>> nodes, reactions, elements, hinges, hingeReactions, springs;
};

bool readOutput(const fs::path& file, OpenSeesOutput* out)
{
    std::ifstream in(file);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "version") std::getline(ls >> std::ws, out->version);
        else if (tag == "status") ls >> out->status;
        else
        {
            int id = 0;
            ls >> id;
            std::vector<double> v;
            std::string token;
            while (ls >> token) v.push_back(std::strtod(token.c_str(), nullptr));   // « nan », « -inf » lus aussi
            (tag == "node" ? out->nodes : tag == "reaction" ? out->reactions : tag == "hinge" ? out->hinges : tag == "hingereaction" ? out->hingeReactions : tag == "spring" ? out->springs : out->elements)[id] = v;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Comparaison
// ---------------------------------------------------------------------------------------------------------------

std::string scientific(double v)
{
    std::ostringstream s;
    s << std::scientific << std::setprecision(3) << v;
    return s.str();
}

struct Comparison
{
    int compared = 0, failed = 0;
    double maxAbsolute = 0.0, maxRelative = 0.0;   // écart relatif sur les valeurs hors critère absolu
    std::vector<std::string> rows;                 // CSV
};

void compareGroup(Comparison& c, const std::string& quantity, const std::map<int, std::array<double, 6>>& tsa,
                  const std::map<int, std::vector<double>>& ref, int offset, const char* const* labels, double scale)
{
    for (const auto& [id, values] : tsa)
    {
        const auto it = ref.find(id);
        for (int k = 0; k < 6; ++k)
        {
            const double a = values[static_cast<std::size_t>(k)];
            const bool missing = it == ref.end() || it->second.size() < static_cast<std::size_t>(offset + k + 1);
            const double b = missing ? std::nan("") : it->second[static_cast<std::size_t>(offset + k)];
            const double diff = std::abs(a - b);
            const double absTol = kAbsoluteToleranceFactor * scale;
            const bool nearZero = std::abs(b) * kRelativeTolerance <= absTol;
            const bool ok = !missing && std::isfinite(b) && diff <= std::max(kRelativeTolerance * std::abs(b), absTol);
            ++c.compared;
            if (!ok) ++c.failed;
            if (std::isfinite(diff)) c.maxAbsolute = std::max(c.maxAbsolute, diff);
            if (!nearZero && std::isfinite(diff)) c.maxRelative = std::max(c.maxRelative, diff / std::abs(b));
            std::ostringstream row;
            row << std::setprecision(17) << quantity << ',' << id << ',' << labels[k] << ',' << a << ',' << b << ',' << diff << ','
                << (nearZero ? std::string("absolu") : scientific(diff / std::abs(b))) << ',' << (ok ? "OK" : "ECART");
            c.rows.push_back(row.str());
        }
    }
}

double maxAbs(const std::map<int, std::array<double, 6>>& m, int from, int to)
{
    double s = 0.0;
    for (const auto& [id, v] : m)
        for (int k = from; k < to; ++k) s = std::max(s, std::abs(v[static_cast<std::size_t>(k)]));
    return s;
}

struct CaseOutcome
{
    std::string caseId, title, formulation, status, detail;
    Comparison comparison;
};
} // namespace

int main(int argc, char** argv)
{
    const fs::path openSees = argc > 1 ? fs::path(argv[1]) : fs::path(FEM3D_DEFAULT_OPENSEES);
    const fs::path outDir = argc > 2 ? fs::path(argv[2]) : fs::path(FEM3D_BENCHMARK_DIR);
    const fs::path tclDir = outDir / "cases", resultDir = outDir / "results";
    fs::create_directories(tclDir);
    fs::create_directories(resultDir);
    const bool haveOpenSees = fs::exists(openSees);
    if (!haveOpenSees) std::cout << "OpenSees introuvable (" << openSees.string() << ") : scripts générés, comparaison NON exécutée\n";

    static const char* const dispLabels[] = { "Ux", "Uy", "Uz", "Rx", "Ry", "Rz" };
    static const char* const reacLabels[] = { "Fx", "Fy", "Fz", "Mx", "My", "Mz" };
    static const char* const endLabelsI[] = { "N_i", "Vy_i", "Vz_i", "T_i", "My_i", "Mz_i" };
    static const char* const endLabelsJ[] = { "N_j", "Vy_j", "Vz_j", "T_j", "My_j", "Mz_j" };

    std::vector<CaseOutcome> outcomes;
    std::string openSeesVersion = "non exécuté";
    for (const Case& c : cases())
        for (auto f : { BeamFormulation::EulerBernoulli, BeamFormulation::Timoshenko })
        {
            CaseOutcome out{ c.id, c.title, formulationName(f), "", "", {} };
            const std::string stem = c.id + (f == BeamFormulation::Timoshenko ? "_timoshenko" : "_euler_bernoulli");
            const StructuralModel3D model = c.build(f);
            const fs::path tcl = tclDir / (stem + ".tcl"), osOut = resultDir / (stem + "_opensees.txt");
            std::string unsupported;
            const bool scripted = writeTcl(model, f, tcl, osOut, &unsupported);

            // Calcul TSA
            std::optional<LoadCaseResults> tsa;
            std::string tsaError;
            try { tsa = LinearStaticAnalysis(model).run(1); }
            catch (const Fem3DError& e) { tsaError = e.what(); }

            OpenSeesOutput ref;
            if (haveOpenSees && scripted)
            {
                fs::remove(osOut);
                const std::string cmd = "\"\"" + openSees.string() + "\" \"" + tcl.string() + "\" > \"" + (resultDir / (stem + "_opensees.log")).string() + "\" 2>&1\"";
                std::system(cmd.c_str());
                readOutput(osOut, &ref);
                // Avec equalDOF, nodeReaction du nœud d'appui (maître) n'inclut pas l'effort transmis par la contrainte au
                // nœud doublé (esclave) : la réaction d'appui est la somme des deux (nœud doublé = 100000 + 10·e + extrémité).
                for (const auto& [hinge, values] : ref.hingeReactions)
                {
                    const int element = (hinge - kHingeNodeBase) / 10, end = (hinge - kHingeNodeBase) % 10;
                    const auto def = std::find_if(model.elements().begin(), model.elements().end(), [&](const ElementDefinition& d) { return d.id == element; });
                    if (def == model.elements().end()) continue;
                    const int master = end == 0 ? def->startNodeId : def->endNodeId;
                    auto it = ref.reactions.find(master);
                    if (it == ref.reactions.end()) continue;
                    for (std::size_t k = 0; k < values.size() && k < it->second.size(); ++k) it->second[k] += values[k];
                }
                if (!ref.version.empty()) openSeesVersion = ref.version;
            }

            if (c.expectMechanism)
            {
                const bool tsaDetects = !tsaError.empty() && tsaError.find("mécanisme") != std::string::npos;
                bool osFails = ref.status < 0;
                for (const auto& [id, v] : ref.nodes)
                    for (double x : v) osFails = osFails || !std::isfinite(x) || std::abs(x) > 1e6;
                out.status = !haveOpenSees ? "NON EXÉCUTÉ" : (tsaDetects && osFails ? "OK" : "ECART");
                out.detail = "TSA : " + (tsaDetects ? tsaError : std::string("mécanisme non détecté")) + " ; OpenSees : analyze = " + std::to_string(ref.status)
                             + (osFails ? " (échec ou solution non bornée)" : " (solution finie : mécanisme non signalé)");
            }
            else if (!tsa)
            {
                out.status = "ECART";
                out.detail = "calcul TSA impossible : " + tsaError;
            }
            else if (!scripted)
            {
                out.status = "NON COMPARABLE";
                out.detail = unsupported;
            }
            else if (!haveOpenSees || ref.status == -999)
            {
                out.status = "NON EXÉCUTÉ";
                out.detail = haveOpenSees ? "sortie OpenSees absente (voir le journal)" : "OpenSees absent";
            }
            else
            {
                std::map<int, std::array<double, 6>> disp, reac, endI, endJ;
                for (const auto& n : tsa->nodes) disp[n.nodeId] = n.displacement;
                for (const auto& r : tsa->reactions) reac[r.nodeId] = r.force;
                std::map<int, std::array<double, 6>> spr;
                for (const auto& s : tsa->springs) spr[s.nodeId] = s.force;
                std::map<int, std::vector<double>> refEnd;
                for (const auto& e : tsa->elements)
                    for (int k = 0; k < 6; ++k)
                    {
                        endI[e.elementId][static_cast<std::size_t>(k)] = e.endForcesLocal(k);
                        endJ[e.elementId][static_cast<std::size_t>(k)] = e.endForcesLocal(6 + k);
                    }
                // Échelles par nature de grandeur (critère absolu près de zéro)
                double maxLength = 0.0;
                for (const auto& e : tsa->elements) maxLength = std::max(maxLength, e.length);
                const double sT = std::max(maxAbs(disp, 0, 3), 1e-30), sF = std::max({ maxAbs(reac, 0, 3), maxAbs(spr, 0, 3), maxAbs(endI, 0, 3), maxAbs(endJ, 0, 3), 1e-30 });
                // Une grandeur toute nulle (moments d'une barre bi-articulée) aurait une échelle nulle : plancher par l'échelle
                // de même dimension tirée des forces ou des translations
                const double sR = std::max(maxAbs(disp, 3, 6), sT / maxLength);
                const double sM = std::max({ maxAbs(reac, 3, 6), maxAbs(spr, 3, 6), maxAbs(endI, 3, 6), maxAbs(endJ, 3, 6), sF * maxLength });
                auto split = [](const std::map<int, std::array<double, 6>>& m, int from) {
                    std::map<int, std::array<double, 6>> a;
                    for (const auto& [id, v] : m)
                    {
                        a[id] = {};
                        for (int k = from; k < from + 3; ++k) a[id][static_cast<std::size_t>(k)] = v[static_cast<std::size_t>(k)];
                    }
                    return a;
                };
                // Comparaison par nature : translations et forces avec leur échelle, rotations et moments avec la leur
                Comparison& cmp = out.comparison;
                auto group = [&](const std::string& q, const std::map<int, std::array<double, 6>>& t, const std::map<int, std::vector<double>>& r, int offset,
                                 const char* const* labels, double s1, double s2) {
                    std::map<int, std::array<double, 6>> lin = t, ang = t;
                    Comparison a, b;
                    compareGroup(a, q, t, r, offset, labels, s1);
                    compareGroup(b, q, t, r, offset, labels, s2);
                    // ligne k : translation (0..2) jugée avec a, rotation (3..5) avec b
                    for (std::size_t i = 0; i < a.rows.size(); ++i)
                    {
                        const bool angular = (i % 6) >= 3;
                        cmp.rows.push_back(angular ? b.rows[i] : a.rows[i]);
                        const std::string& row = angular ? b.rows[i] : a.rows[i];
                        ++cmp.compared;
                        if (row.size() >= 5 && row.substr(row.size() - 5) == "ECART") ++cmp.failed;
                    }
                    cmp.maxAbsolute = std::max(cmp.maxAbsolute, std::max(a.maxAbsolute, b.maxAbsolute));
                    cmp.maxRelative = std::max(cmp.maxRelative, std::max(a.maxRelative, b.maxRelative));
                    (void)lin; (void)ang; (void)split;
                };
                group("deplacement", disp, ref.nodes, 0, dispLabels, sT, sR);
                group("reaction", reac, ref.reactions, 0, reacLabels, sF, sM);
                group("ressort", spr, ref.springs, 0, reacLabels, sF, sM);
                group("effort_extremite", endI, ref.elements, 0, endLabelsI, sF, sM);
                group("effort_extremite", endJ, ref.elements, 6, endLabelsJ, sF, sM);
                // Rotation de la barre à chaque rotule : rotation du nœud doublé OpenSees projetée sur le repère local
                for (const auto& er : tsa->elements)
                {
                    const ElementDefinition& d = *std::find_if(model.elements().begin(), model.elements().end(), [&](const ElementDefinition& x) { return x.id == er.elementId; });
                    const Matrix3 r = model.createElement(d)->localAxes().rotation();
                    for (int end = 0; end < 2; ++end)
                    {
                        const auto it = ref.hinges.find(kHingeNodeBase + 10 * er.elementId + end);
                        if (it == ref.hinges.end() || it->second.size() < 6) continue;
                        const Vector3 local = r * Vector3(it->second[3], it->second[4], it->second[5]);
                        const bool releasedY = end == 0 ? d.releases.momentYStart : d.releases.momentYEnd;
                        const bool releasedZ = end == 0 ? d.releases.momentZStart : d.releases.momentZEnd;
                        for (const auto& [released, axis] : { std::pair{ releasedY, 1 }, std::pair{ releasedZ, 2 } })
                        {
                            if (!released) continue;
                            const double a = er.localDisplacements(6 * end + 3 + axis), b = local(axis);
                            const double diff = std::abs(a - b), absTol = kAbsoluteToleranceFactor * sR;
                            const bool ok = diff <= std::max(kRelativeTolerance * std::abs(b), absTol);
                            ++cmp.compared;
                            if (!ok) ++cmp.failed;
                            cmp.maxAbsolute = std::max(cmp.maxAbsolute, diff);
                            if (std::abs(b) * kRelativeTolerance > absTol) cmp.maxRelative = std::max(cmp.maxRelative, diff / std::abs(b));
                            std::ostringstream row;
                            row << std::setprecision(17) << "rotation_rotule," << er.elementId << ',' << (end ? "j_" : "i_") << (axis == 1 ? "thetay" : "thetaz") << ','
                                << a << ',' << b << ',' << diff << ',' << scientific(std::abs(b) > 0 ? diff / std::abs(b) : 0.0) << ',' << (ok ? "OK" : "ECART");
                            cmp.rows.push_back(row.str());
                        }
                    }
                }
                out.status = cmp.failed == 0 ? "OK" : "ECART";
                std::ostringstream d;
                d << cmp.compared << " valeurs, " << cmp.failed << " hors tolérance ; équilibre TSA : |ΣF| = " << std::scientific << std::setprecision(2)
                  << tsa->equilibrium.forceResidual << ", |ΣM| = " << tsa->equilibrium.momentResidual << ", résidu de résolution "
                  << tsa->equilibrium.solutionResidual;
                for (const auto& w : tsa->warnings) d << " ; AVERTISSEMENT : " << w;
                out.detail = d.str();
                std::ofstream csv(resultDir / (stem + "_comparaison.csv"));
                csv << "grandeur,id,composante,tsa,opensees,ecart_absolu,ecart_relatif,statut\n";
                for (const auto& r : cmp.rows) csv << r << '\n';
            }
            std::cout << "[" << out.status << "] " << stem << " : " << out.detail << '\n';
            outcomes.push_back(std::move(out));
        }

    // Synthèse (CSV + Markdown)
    std::ofstream summary(resultDir / "synthese.csv");
    summary << "cas,formulation,statut,valeurs_comparees,hors_tolerance,ecart_absolu_max,ecart_relatif_max,detail\n";
    std::ofstream md(resultDir / "synthese.md");
    md << "# Comparaison TSA Fem3D / OpenSees — synthèse générée\n\nOpenSees " << openSeesVersion << " ; moteur " << kEngineVersion
       << ". Tolérance : |écart| ≤ max(" << kRelativeTolerance << " · |réf|, " << kAbsoluteToleranceFactor << " · échelle).\n\n";
    md << "| Cas | Formulation | Statut | Valeurs | Hors tolérance | Écart absolu max | Écart relatif max |\n|---|---|---|---|---|---|---|\n";
    int failed = 0;
    for (const auto& o : outcomes)
    {
        failed += o.status == "ECART";
        summary << o.caseId << ',' << o.formulation << ',' << o.status << ',' << o.comparison.compared << ',' << o.comparison.failed << ','
                << std::setprecision(3) << std::scientific << o.comparison.maxAbsolute << ',' << o.comparison.maxRelative << ",\"" << o.detail << "\"\n";
        md << "| " << o.title << " | " << o.formulation << " | " << o.status << " | " << o.comparison.compared << " | " << o.comparison.failed << " | "
           << std::setprecision(2) << std::scientific << o.comparison.maxAbsolute << " | " << o.comparison.maxRelative << " |\n";
    }
    md << "\nDétails par valeur : `results/<cas>_<formulation>_comparaison.csv` ; scripts : `cases/*.tcl`.\n";
    std::cout << "Synthèse : " << (resultDir / "synthese.md").string() << " ; cas en écart : " << failed << '\n';
    return failed;
}
