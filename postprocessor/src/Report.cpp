// Note de calcul HTML. Le HTML produit reste dans le sous-ensemble compris par QTextDocument
// (tableaux avec attributs bgcolor/border, CSS simple) pour que l'export PDF lui ressemble.

#include "fem3dpost/Report.h"

#include "fem3dpost/Calculations.h"
#include "fem3dpost/Diagrams.h"
#include "fem3dpost/Tables.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace fem3dpost {

const std::vector<std::string>& reportSectionTitles()
{
    static const std::vector<std::string> t{
        "Identification",
        "Hypothèses et modèle",
        "Formulation éléments finis",
        "Matériaux et sections",
        "Géométrie et conditions aux limites",
        "Charges et cas de charge",
        "Méthode de résolution",
        "Déplacements nodaux",
        "Réactions d'appui",
        "Efforts internes",
        "Diagrammes et flèches",
        "Contraintes",
        "Contrôle d'équilibre global",
        "Comparaison avec OpenSees",
        "Limites et avertissements",
    };
    return t;
}

namespace {

std::string esc(const std::string& s)
{
    std::string o;
    o.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': o += "&amp;"; break;
        case '<': o += "&lt;"; break;
        case '>': o += "&gt;"; break;
        case '"': o += "&quot;"; break;
        default: o += c;
        }
    }
    return o;
}

const char* kNA = "<span class=\"na\" title=\"indisponible\">—</span>";

std::string fmt(const OptDouble& v, int sig = 5)
{
    return v ? esc(formatShort(*v, sig)) : std::string(kNA);
}

std::string fmtAt(const std::optional<Extremum>& e, int sig = 5)
{
    if (!e)
        return kNA;
    return esc(formatShort(e->value, sig)) + " <span class=\"pos\">(x = " + esc(formatShort(e->x, 4)) + ")</span>";
}

std::string vec3(const std::array<double, 3>& v)
{
    return "(" + formatShort(v[0], 4) + ", " + formatShort(v[1], 4) + ", " + formatShort(v[2], 4) + ")";
}

enum class Kind { Input, Hypothesis, Result, Check, Note };

std::string block(Kind k, const std::string& inner)
{
    const char* bg = "#f4f4f4";
    const char* fg = "#444";
    const char* tag = "REMARQUE";
    const char* cls = "note";
    switch (k) {
    case Kind::Input: bg = "#e8f0fe"; fg = "#1f5fae"; tag = "DONNÉES SAISIES — lues dans le fichier de résultats"; cls = "input"; break;
    case Kind::Hypothesis: bg = "#fff4d6"; fg = "#8a5a00"; tag = "HYPOTHÈSES"; cls = "hypothesis"; break;
    case Kind::Result: bg = "#e7f6ec"; fg = "#1e7b45"; tag = "RÉSULTATS CALCULÉS"; cls = "result"; break;
    case Kind::Check: bg = "#f1e9fb"; fg = "#6b3fa0"; tag = "VÉRIFICATIONS"; cls = "check"; break;
    case Kind::Note: break;
    }
    std::string h = "<table class=\"blk blk-";
    h += cls;
    h += "\" width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"8\" bgcolor=\"";
    h += bg;
    h += "\" style=\"background-color:";
    h += bg;
    h += ";border-left:5px solid ";
    h += fg;
    h += "\"><tr><td bgcolor=\"";  // QTextDocument (PDF) n'applique le fond que sur la cellule
    h += bg;
    h += "\"><p class=\"tag\" style=\"color:";
    h += fg;
    h += "\"><b>";
    h += tag;
    h += "</b></p>\n";
    h += inner;
    h += "</td></tr></table>\n";
    return h;
}

std::string cellHtml(const Cell& c, int sig)
{
    if (const auto* i = std::get_if<long long>(&c))
        return std::to_string(*i);
    if (const auto* d = std::get_if<double>(&c))
        return esc(formatShort(*d, sig));
    if (const auto* s = std::get_if<std::string>(&c))
        return esc(*s);
    return kNA;
}

std::string tableOpen()
{
    return "<table class=\"data\" border=\"1\" cellspacing=\"0\" cellpadding=\"3\">";
}

std::string headerRow(const std::vector<std::string>& headers)
{
    std::string h = "<tr>";
    for (const auto& s : headers)
        h += "<th bgcolor=\"#e6e6e6\">" + s + "</th>";
    return h + "</tr>\n";
}

std::string htmlTable(const Table& t, int sig = 5)
{
    std::string h = tableOpen();
    std::vector<std::string> headers;
    for (const auto& c : t.columns)
        headers.push_back(esc(c.header()));
    h += headerRow(headers);
    for (const auto& row : t.rows) {
        h += "<tr>";
        for (const auto& c : row)
            h += "<td align=\"right\">" + cellHtml(c, sig) + "</td>";
        h += "</tr>\n";
    }
    return h + "</table>\n";
}

// Sous-tableau limité aux colonnes nommées (dans l'ordre donné), pour l'affichage.
Table selectColumns(const Table& t, const std::vector<std::string>& labels)
{
    std::vector<std::size_t> idx;
    for (const auto& l : labels)
        if (const auto c = t.column(l))
            idx.push_back(*c);
    Table out;
    out.title = t.title;
    for (std::size_t i : idx)
        out.columns.push_back(t.columns[i]);
    for (const auto& row : t.rows) {
        std::vector<Cell> r;
        for (std::size_t i : idx)
            r.push_back(row[i]);
        out.rows.push_back(std::move(r));
    }
    return out;
}

std::string htmlTextTable(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows)
{
    std::string h = tableOpen();
    h += headerRow(headers);
    for (const auto& row : rows) {
        h += "<tr>";
        for (const auto& c : row)
            h += "<td>" + c + "</td>";
        h += "</tr>\n";
    }
    return h + "</table>\n";
}

std::string p(const std::string& s)
{
    return "<p>" + s + "</p>\n";
}

std::string caseLabel(const LoadCase& lc)
{
    return "Cas " + std::to_string(lc.id) + " — " + esc(lc.name);
}

std::string dofList(const std::array<bool, 6>& fixed, const std::vector<std::string>& names)
{
    std::string s;
    for (std::size_t i = 0; i < 6; ++i)
        if (fixed[i]) {
            if (!s.empty())
                s += " ";
            s += i < names.size() ? names[i] : std::to_string(i + 1);
        }
    return s.empty() ? "aucun" : s;
}

const char* kStyle = R"(
body { font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; color: #222; max-width: 1050px; margin: 0 auto; padding: 16px; }
h1 { font-size: 18pt; margin-bottom: 4px; }
h2 { font-size: 14pt; color: #1b2a4a; border-bottom: 1px solid #9aa; margin-top: 26px; }
h3 { font-size: 11pt; margin-top: 14px; }
table.data { border-collapse: collapse; margin: 4px 0 8px 0; }
table.data th, table.data td { border: 1px solid #bbb; padding: 2px 6px; }
table.data th { background-color: #e6e6e6; }
table.blk { margin: 8px 0; border-collapse: collapse; }
p.tag { font-size: 8pt; margin: 0 0 4px 0; }
.na { color: #999; }
.pos { color: #666; font-size: 8pt; }
.ok { color: #1e7b45; font-weight: bold; }
.ko { color: #b00020; font-weight: bold; }
div.diag { margin: 4px 0; }
div.diag svg { max-width: 100%; height: auto; }
)";

} // namespace

Report buildReport(const Results& r, const ReportOptions& opt)
{
    std::vector<const LoadCase*> cases;
    for (const LoadCase& lc : r.loadCases)
        if (!opt.caseId || lc.id == *opt.caseId)
            cases.push_back(&lc);
    if (opt.caseId && cases.empty())
        throw std::invalid_argument("cas de charge " + std::to_string(*opt.caseId) + " absent du fichier");

    const auto& titles = reportSectionTitles();
    const Units& u = r.units;
    Report rep;
    std::string h;
    h.reserve(1 << 20);
    auto section = [&](int n) {
        h += "<h2 id=\"s" + std::to_string(n) + "\"><a name=\"s" + std::to_string(n) + "\"></a>" + std::to_string(n) +
             ". " + esc(titles[static_cast<std::size_t>(n - 1)]) + "</h2>\n";
    };

    h += "<!DOCTYPE html>\n<html lang=\"fr\">\n<head>\n<meta charset=\"utf-8\">\n<title>Note de calcul — " +
         esc(r.project) + "</title>\n<style>" + kStyle + "</style>\n</head>\n<body>\n";
    h += "<h1>Note de calcul — " + esc(r.project.empty() ? std::string("projet sans nom") : r.project) + "</h1>\n";
    h += p("Post-traitement des résultats d'un calcul aux éléments finis de poutres 3D (" + esc(r.engine) + ").");

    // Légende des types de contenu.
    h += tableOpen() + "<tr><th bgcolor=\"#e6e6e6\" colspan=\"2\">Lecture de la note</th></tr>\n";
    h += "<tr><td bgcolor=\"#e8f0fe\" style=\"background-color:#e8f0fe;color:#1f5fae\"><b>Données saisies</b></td><td>valeurs lues telles quelles dans le fichier de résultats (modèle, matériaux, sections, appuis)</td></tr>\n";
    h += "<tr><td bgcolor=\"#fff4d6\" style=\"background-color:#fff4d6;color:#8a5a00\"><b>Hypothèses</b></td><td>hypothèses de modélisation et de calcul, approximations déclarées</td></tr>\n";
    h += "<tr><td bgcolor=\"#e7f6ec\" style=\"background-color:#e7f6ec;color:#1e7b45\"><b>Résultats calculés</b></td><td>résultats du moteur lus dans le fichier, et grandeurs dérivées par le post-traitement (signalées comme telles)</td></tr>\n";
    h += "<tr><td bgcolor=\"#f1e9fb\" style=\"background-color:#f1e9fb;color:#6b3fa0\"><b>Vérifications</b></td><td>contrôles de cohérence (équilibre global, comparaison externe)</td></tr>\n";
    h += "<tr><td>" + std::string(kNA) + "</td><td>valeur indisponible (null dans le fichier) : jamais remplacée par 0</td></tr>\n</table>\n";

    h += "<p><b>Sommaire</b></p>\n<ol>\n";
    for (std::size_t i = 0; i < titles.size(); ++i)
        h += "<li><a href=\"#s" + std::to_string(i + 1) + "\">" + esc(titles[i]) + "</a></li>\n";
    h += "</ol>\n";

    // ---------------------------------------------------------------- 1
    section(1);
    {
        std::string casesTxt;
        for (const LoadCase* lc : cases)
            casesTxt += (casesTxt.empty() ? "" : " ; ") + std::to_string(lc->id) + " (" + esc(lc->name) + ")";
        std::string in = tableOpen();
        auto row = [&](const std::string& k, const std::string& v) {
            in += "<tr><th bgcolor=\"#e6e6e6\" align=\"left\">" + k + "</th><td>" + v + "</td></tr>\n";
        };
        row("Projet", esc(r.project));
        row("Moteur de calcul", esc(r.engine));
        row("Format du fichier", esc(r.format));
        row("Fichier de résultats", esc(opt.inputFile));
        row("Unités", "force " + esc(u.force) + ", longueur " + esc(u.length) + ", moment " + esc(u.moment) +
                          ", rotation " + esc(u.rotation) + ", contrainte " + esc(u.stress));
        row("Contenu", std::to_string(r.nodes.size()) + " nœuds, " + std::to_string(r.elements.size()) +
                           " éléments, " + std::to_string(r.supports.size()) + " appuis, " +
                           std::to_string(r.loadCases.size()) + " cas de charge");
        row("Cas présentés dans cette note", casesTxt + (opt.caseId ? " (filtrage demandé)" : " (tous les cas)"));
        in += "</table>\n";
        h += block(Kind::Input, in);
        h += block(Kind::Note, p("Note générée par " + std::string(kPostProcessorVersion) + " le " +
                                 esc(opt.generatedAt) + ". Le post-traitement ne refait aucun calcul aux éléments "
                                 "finis : il lit le fichier, le contrôle, le met en forme et en dérive quelques grandeurs "
                                 "signalées comme telles."));
    }

    // ---------------------------------------------------------------- 2
    section(2);
    h += block(Kind::Hypothesis,
               "<p>Le fichier de résultats ne décrit pas le type d'analyse ; les hypothèses ci-dessous sont celles du "
               "cadre de calcul du moteur pour ce format.</p>\n<ul>\n"
               "<li><b>Analyse statique linéaire</b> : charges appliquées sans effet dynamique ; chaque cas est résolu "
               "séparément.</li>\n"
               "<li><b>Petits déplacements et petites rotations</b> : équilibre écrit sur la géométrie initiale, sans "
               "effet du second ordre (P-Δ).</li>\n"
               "<li><b>Élasticité linéaire</b> : matériau élastique, linéaire et isotrope, caractérisé par E et G.</li>\n"
               "<li><b>Modèle</b> : poutres droites 3D à 2 nœuds, 6 degrés de liberté par nœud, une section et un "
               "matériau constants par élément.</li>\n"
               "<li><b>Conditions aux limites</b> : degrés de liberté bloqués ou imposés aux nœuds d'appui "
               "(section 5).</li>\n</ul>\n");
    {
        std::string dof;
        for (const auto& d : r.conventions.dofOrder)
            dof += (dof.empty() ? "" : ", ") + esc(d);
        std::string in = tableOpen();
        auto row = [&](const std::string& k, const std::string& v) {
            in += "<tr><th bgcolor=\"#e6e6e6\" align=\"left\">" + k + "</th><td>" + v + "</td></tr>\n";
        };
        row("Ordre des DDL", dof);
        row("Axes locaux", esc(r.conventions.localAxes));
        row("Efforts de section", esc(r.conventions.sectionForces));
        row("Efforts d'extrémité", esc(r.conventions.endForces));
        row("Réactions", esc(r.conventions.reactions));
        in += "</table>\n";
        h += block(Kind::Input, "<p>Conventions déclarées dans le fichier (champ « conventions ») :</p>\n" + in);
    }

    // ---------------------------------------------------------------- 3
    section(3);
    {
        std::map<std::string, std::vector<int>> byForm;
        for (const Element& e : r.elements)
            byForm[e.formulation].push_back(e.id);
        std::vector<std::vector<std::string>> rows;
        for (const auto& [form, ids] : byForm) {
            std::string list;
            for (int id : ids)
                list += (list.empty() ? "" : ", ") + std::to_string(id);
            rows.push_back({esc(form), std::to_string(ids.size()), list});
        }
        h += block(Kind::Input, "<p>Formulation de chaque élément (champ « formulation ») :</p>\n" +
                                    htmlTextTable({"Formulation", "Nombre", "Éléments"}, rows));
        h += block(Kind::Hypothesis,
                   "<ul>\n<li><b>Euler-Bernoulli</b> : sections planes restant perpendiculaires à la fibre moyenne ; "
                   "déformation d'effort tranchant négligée ; rotation de section égale à la pente.</li>\n"
                   "<li><b>Timoshenko</b> : déformation d'effort tranchant prise en compte au moyen des aires "
                   "réduites Asy et Asz ; la rotation de section (sectionRotation) diffère alors de la pente de la "
                   "déformée (slopes).</li>\n"
                   "<li>Effort normal et torsion (Saint-Venant, constante J) découplés de la flexion.</li>\n</ul>\n");
    }

    // ---------------------------------------------------------------- 4
    section(4);
    {
        const std::string L2 = u.length + "²", L3 = u.length + "³", L4 = u.length + "⁴";
        std::map<std::string, std::pair<const Material*, std::vector<int>>> mats;
        std::map<std::string, std::pair<const Section*, std::vector<int>>> secs;
        for (const Element& e : r.elements) {
            auto& m = mats[e.material.name];
            if (!m.first)
                m.first = &e.material;
            m.second.push_back(e.id);
            auto& s = secs[e.section.name];
            if (!s.first)
                s.first = &e.section;
            s.second.push_back(e.id);
        }
        auto ids = [](const std::vector<int>& v) {
            std::string s;
            for (int i : v)
                s += (s.empty() ? "" : ", ") + std::to_string(i);
            return s;
        };
        std::vector<std::vector<std::string>> mrows, srows;
        for (const auto& [name, m] : mats)
            mrows.push_back({esc(name), fmt(m.first->E, 6), fmt(m.first->G, 6), ids(m.second)});
        for (const auto& [name, s] : secs) {
            const Section& x = *s.first;
            srows.push_back({esc(name), fmt(x.A), fmt(x.Iy), fmt(x.Iz), fmt(x.J), fmt(x.Asy), fmt(x.Asz),
                             fmt(x.yExtreme), fmt(x.zExtreme), fmt(x.Wt), ids(s.second)});
        }
        h += block(Kind::Input,
                   "<p>Matériaux :</p>\n" +
                       htmlTextTable({"Matériau", "E [" + esc(u.stress) + "]", "G [" + esc(u.stress) + "]", "Éléments"},
                                     mrows) +
                       "<p>Sections :</p>\n" +
                       htmlTextTable({"Section", "A [" + esc(L2) + "]", "Iy [" + esc(L4) + "]", "Iz [" + esc(L4) + "]",
                                      "J [" + esc(L4) + "]", "Asy [" + esc(L2) + "]", "Asz [" + esc(L2) + "]",
                                      "yExtreme [" + esc(u.length) + "]", "zExtreme [" + esc(u.length) + "]",
                                      "Wt [" + esc(L3) + "]", "Éléments"},
                                     srows) +
                       p("Les propriétés sont regroupées par nom ; celles du premier élément portant ce nom sont "
                         "affichées."));
    }

    // ---------------------------------------------------------------- 5
    section(5);
    {
        std::vector<std::vector<std::string>> nrows, erows, srows;
        for (const Node& n : r.nodes)
            nrows.push_back({std::to_string(n.id), esc(formatShort(n.xyz[0], 6)), esc(formatShort(n.xyz[1], 6)),
                             esc(formatShort(n.xyz[2], 6))});
        for (const Element& e : r.elements)
            erows.push_back({std::to_string(e.id), std::to_string(e.nodeI), std::to_string(e.nodeJ),
                             esc(formatShort(e.length, 6)), esc(vec3(e.localX)), esc(vec3(e.localY)),
                             esc(vec3(e.localZ)), esc(e.section.name), esc(e.material.name)});
        for (const Support& s : r.supports) {
            std::string pres;
            for (std::size_t i = 0; i < 6; ++i)
                if (s.fixed[i]) {
                    pres += (pres.empty() ? "" : ", ") + fmt(s.prescribed[i]);
                }
            srows.push_back({std::to_string(s.node), esc(dofList(s.fixed, r.conventions.dofOrder)),
                             pres.empty() ? "—" : pres});
        }
        std::vector<std::vector<std::string>> krows;
        for (const Spring& s : r.springs) {
            std::vector<std::string> row{std::to_string(s.node)};
            for (const auto& k : s.stiffness)
                row.push_back(k && *k == 0.0 ? "—" : fmt(k));
            krows.push_back(std::move(row));
        }
        const std::string L = esc(u.length);
        const std::string Fl = esc(u.force) + "/" + L, Mr = esc(u.moment) + "/" + esc(u.rotation);
        const std::string springs =
            krows.empty() ? std::string("<p>Appuis élastiques : aucun.</p>\n")
                          : "<p>Appuis élastiques (ressorts découplés, repère global ; « — » : pas de ressort) :</p>\n" +
                                htmlTextTable({"Nœud", "kx [" + Fl + "]", "ky [" + Fl + "]", "kz [" + Fl + "]",
                                               "krx [" + Mr + "]", "kry [" + Mr + "]", "krz [" + Mr + "]"},
                                              krows);
        h += block(Kind::Input,
                   "<p>Nœuds (repère global) :</p>\n" +
                       htmlTextTable({"Nœud", "x [" + L + "]", "y [" + L + "]", "z [" + L + "]"}, nrows) +
                       "<p>Éléments et axes locaux :</p>\n" +
                       htmlTextTable({"Élément", "Nœud i", "Nœud j", "L [" + L + "]", "x local", "y local", "z local",
                                      "Section", "Matériau"},
                                     erows) +
                       "<p>Appuis :</p>\n" +
                       htmlTextTable({"Nœud", "DDL bloqués", "Valeurs imposées (DDL bloqués, dans l'ordre)"}, srows) +
                       springs);
    }

    // ---------------------------------------------------------------- 6
    section(6);
    {
        h += block(Kind::Note,
                   p("Le fichier de résultats ne contient, pour chaque cas, que son identifiant et son nom : le détail "
                     "des charges appliquées (nature, position, intensité) n'y figure pas et ne peut donc pas être "
                     "décrit ici. Seules les résultantes globales des charges appliquées, fournies dans le bloc "
                     "« equilibrium », sont reproduites."));
        std::vector<std::vector<std::string>> rows;
        for (const LoadCase* lc : cases)
            rows.push_back({std::to_string(lc->id), esc(lc->name)});
        h += block(Kind::Input, htmlTextTable({"Cas", "Nom"}, rows));
        std::vector<std::vector<std::string>> res;
        for (const LoadCase* lc : cases) {
            const auto& e = lc->equilibrium;
            res.push_back({std::to_string(lc->id), fmt(e.appliedForce[0]), fmt(e.appliedForce[1]),
                           fmt(e.appliedForce[2]), fmt(e.appliedMoment[0]), fmt(e.appliedMoment[1]),
                           fmt(e.appliedMoment[2])});
        }
        const std::string F = esc(u.force), M = esc(u.moment);
        h += block(Kind::Result, "<p>Résultantes des charges appliquées (bloc « equilibrium » du fichier) :</p>\n" +
                                     htmlTextTable({"Cas", "ΣFx [" + F + "]", "ΣFy [" + F + "]", "ΣFz [" + F + "]",
                                                    "ΣMx [" + M + "]", "ΣMy [" + M + "]", "ΣMz [" + M + "]"},
                                                   res));
    }

    // ---------------------------------------------------------------- 7
    section(7);
    h += block(Kind::Hypothesis,
               "<ul>\n<li><b>Méthode des déplacements</b> : les inconnues sont les 6 déplacements et rotations de "
               "chaque nœud (repère global).</li>\n"
               "<li>Les matrices de rigidité élémentaires, écrites en repère local, sont transformées en repère global "
               "à l'aide des axes locaux (section 5) puis assemblées dans la matrice de rigidité K.</li>\n"
               "<li>Les charges sont assemblées dans le vecteur F ; les conditions d'appui bloquent ou imposent les "
               "degrés de liberté correspondants.</li>\n"
               "<li>Le système <b>K U = F</b> est résolu par factorisation de la matrice de rigidité réduite aux "
               "degrés de liberté libres ; le type de factorisation employé n'est pas indiqué dans le fichier.</li>\n"
               "<li>Les réactions, les efforts d'extrémité et les efforts, déplacements et contraintes aux stations "
               "sont déduits de U par le moteur.</li>\n</ul>\n");
    h += block(Kind::Note,
               p("Grandeurs dérivées par le post-traitement (et non par le moteur) : flèche locale par rapport à la "
                 "corde aux stations, cisaillement moyen maximal max(|τy|, |τz|), résultantes des réactions "
                 "recalculées, résidus recalculés. Toutes sont signalées comme telles là où elles apparaissent."));

    // ---------------------------------------------------------------- 8
    section(8);
    {
        const Table all = displacementTable(r);
        std::string in;
        for (const LoadCase* lc : cases) {
            in += "<h3>" + caseLabel(*lc) + "</h3>\n" + htmlTable(filterByCase(all, lc->id));
            // Nœud de plus grand déplacement de translation.
            double best = -1.0;
            int bestNode = 0;
            for (const NodalValues& d : lc->displacements) {
                if (!d.values[0] || !d.values[1] || !d.values[2])
                    continue;
                const double n = std::sqrt(*d.values[0] * *d.values[0] + *d.values[1] * *d.values[1] +
                                           *d.values[2] * *d.values[2]);
                if (n > best) {
                    best = n;
                    bestNode = d.node;
                }
            }
            if (best >= 0.0)
                in += p("Plus grand déplacement de translation ‖U‖ = √(Ux²+Uy²+Uz²) = " + esc(formatShort(best)) +
                        " " + esc(u.length) + " au nœud " + std::to_string(bestNode) + " (calculé par le post-traitement).");
        }
        h += block(Kind::Result, "<p>Déplacements et rotations nodaux, repère global.</p>\n" + in);
    }

    // ---------------------------------------------------------------- 9
    section(9);
    {
        const Table all = reactionTable(r);
        std::string in = p("Seuls les nœuds d'appui déclarés (section 5) figurent dans ce tableau. Convention : " +
                           esc(r.conventions.reactions) + ".");
        for (const LoadCase* lc : cases)
            in += "<h3>" + caseLabel(*lc) + "</h3>\n" + htmlTable(filterByCase(all, lc->id));
        if (!r.springs.empty()) {
            const Table springs = springForceTable(r);
            in += p("Forces des appuis élastiques. Convention : " +
                    esc(r.conventions.springForces.empty() ? std::string("-k.u, exercées par le ressort sur la structure")
                                                           : r.conventions.springForces) +
                    ".");
            for (const LoadCase* lc : cases)
                in += "<h3>" + caseLabel(*lc) + " — ressorts</h3>\n" + htmlTable(filterByCase(springs, lc->id));
        }
        h += block(Kind::Result, in);
    }

    // ---------------------------------------------------------------- 10
    section(10);
    {
        const std::string F = esc(u.force), M = esc(u.moment);
        std::string in = p("Pour chaque élément : valeur signée de plus grande valeur absolue parmi les stations, et "
                           "son abscisse locale x [" + esc(u.length) + "]. Efforts sur la face positive, repère local. "
                           "Le tableau complet station par station est fourni dans <i>sollicitations.csv</i>.");
        for (const LoadCase* lc : cases) {
            std::vector<std::vector<std::string>> rows;
            for (const ElementResult& er : lc->elements) {
                std::vector<double> x;
                for (const Station& s : er.stations)
                    x.push_back(s.x);
                std::vector<std::string> row{std::to_string(er.id), std::to_string(er.stations.size())};
                for (std::size_t k = 0; k < 6; ++k) {
                    std::vector<OptDouble> v;
                    for (const Station& s : er.stations)
                        v.push_back(s.forces[k]);
                    row.push_back(fmtAt(seriesExtremes(x, v).maxAbs));
                }
                rows.push_back(row);
            }
            in += "<h3>" + caseLabel(*lc) + "</h3>\n" +
                  htmlTextTable({"Élément", "Stations", "N [" + F + "]", "Vy [" + F + "]", "Vz [" + F + "]",
                                 "T [" + M + "]", "My [" + M + "]", "Mz [" + M + "]"},
                                rows);
        }
        h += block(Kind::Result, in);
    }

    // ---------------------------------------------------------------- 11
    section(11);
    {
        const std::string L = esc(u.length);
        std::string in = p("Flèche locale f = √(v²+w²) : écart du déplacement transversal local à la corde joignant "
                           "les extrémités déplacées de l'élément. La colonne « stations » est calculée par le "
                           "post-traitement aux seules stations fournies ; la colonne « fichier » reprend "
                           "maxDeflection, évaluée par le moteur (éventuellement entre les stations).");
        for (const LoadCase* lc : cases) {
            std::vector<std::vector<std::string>> rows;
            for (const ElementResult& er : lc->elements) {
                const Element* e = r.element(er.id);
                std::vector<double> x;
                std::vector<OptDouble> f;
                for (const Station& s : er.stations) {
                    x.push_back(s.x);
                    const auto c = chordDeflection(*e, er, s);
                    f.push_back(c ? OptDouble(c->magnitude) : std::nullopt);
                }
                const auto ex = seriesExtremes(x, f);
                std::string ratio = kNA;
                if (er.maxDeflection && *er.maxDeflection > 0.0)
                    ratio = "L / " + std::to_string(std::llround(e->length / *er.maxDeflection));
                rows.push_back({std::to_string(er.id), esc(formatShort(e->length, 5)), fmtAt(ex.max),
                                fmt(er.maxDeflection), fmt(er.maxDeflectionPosition), ratio});
            }
            in += "<h3>" + caseLabel(*lc) + "</h3>\n" +
                  htmlTextTable({"Élément", "L [" + L + "]", "f max aux stations [" + L + "] (post-traitement)",
                                 "f max fichier [" + L + "]", "Position fichier [" + L + "]",
                                 "Rapport (f fichier)"},
                                rows);
        }
        h += block(Kind::Result, in);

        std::string diag = p("Diagrammes par élément et par cas : seules les stations fournies sont tracées (points) ; "
                             "le tracé entre deux stations est une interpolation linéaire. Valeurs positives vers le "
                             "haut, signe conservé. Aucune enveloppe de cas n'est construite.");
        for (const LoadCase* lc : cases) {
            diag += "<h3>" + caseLabel(*lc) + "</h3>\n";
            for (const ElementResult& er : lc->elements) {
                const Element* e = r.element(er.id);
                diag += "<p><b>Élément " + std::to_string(er.id) + "</b> (nœuds " + std::to_string(e->nodeI) + " → " +
                        std::to_string(e->nodeJ) + ", L = " + esc(formatShort(e->length, 5)) + " " + L + ", " +
                        esc(e->formulation) + ")</p>\n";
                for (const Diagram& d : elementDiagrams(r, *lc, er)) {
                    ReportDiagram rd;
                    rd.stem = diagramStem(d);
                    rd.imageRef = "diag_" + rd.stem + ".png";
                    rd.svg = diagramSvg(d);
                    if (opt.svgMode == SvgMode::Inline)
                        diag += "<div class=\"diag\">" + rd.svg + "</div>\n";
                    else
                        diag += "<p><img src=\"" + rd.imageRef + "\" width=\"" + std::to_string(kReportImageWidth) +
                                "\" height=\"" + std::to_string(kReportImageHeight) + "\"></p>\n";
                    rep.diagrams.push_back(std::move(rd));
                }
            }
        }
        h += block(Kind::Result, diag);
    }

    // ---------------------------------------------------------------- 12
    section(12);
    {
        h += block(Kind::Hypothesis,
                   "<p>Approximations des contraintes fournies par le moteur (RESULTS_FORMAT.md) :</p>\n<ul>\n"
                   "<li>Cisaillements <b>shearY</b> et <b>shearZ</b> : contraintes moyennes V / As, une approximation "
                   "(la répartition réelle dans la section n'est pas uniforme).</li>\n"
                   "<li>Contrainte normale extrême <b>extremeNormal</b> = |N/A| + |My|·zExt/Iy + |Mz|·yExt/Iz : exacte "
                   "pour un rectangle, majorant pour les autres sections.</li>\n"
                   "<li><b>von Mises</b> : valeur indicative, qui ne constitue pas une vérification réglementaire.</li>\n"
                   "<li>Une contrainte vaut « indisponible » lorsque la donnée de section nécessaire manque.</li>\n"
                   "<li>« Cisaillement moyen max » = max(|shearY|, |shearZ|), dérivé par le post-traitement.</li>\n</ul>\n");
        const std::string S = esc(u.stress);
        const Table all = strainStressTable(r);
        std::string in = p("Pour chaque élément : maximum aux stations et abscisse locale correspondante.");
        for (const LoadCase* lc : cases) {
            std::vector<std::vector<std::string>> rows;
            for (const ElementResult& er : lc->elements) {
                std::vector<double> x;
                std::vector<OptDouble> sn, sh, to, vm;
                for (const Station& s : er.stations) {
                    x.push_back(s.x);
                    sn.push_back(s.stresses.extremeNormal);
                    sh.push_back(maxMeanShear(s.stresses));
                    to.push_back(s.stresses.torsion ? OptDouble(std::fabs(*s.stresses.torsion)) : std::nullopt);
                    vm.push_back(s.stresses.vonMisesIndicative);
                }
                rows.push_back({std::to_string(er.id), fmtAt(seriesExtremes(x, sn).max),
                                fmtAt(seriesExtremes(x, sh).max), fmtAt(seriesExtremes(x, to).max),
                                fmtAt(seriesExtremes(x, vm).max)});
            }
            in += "<h3>" + caseLabel(*lc) + "</h3>\n" +
                  htmlTextTable({"Élément", "Contrainte normale extrême [" + S + "]", "Cisaillement moyen max [" + S + "]",
                                 "|Torsion| [" + S + "]", "von Mises indicatif [" + S + "]"},
                                rows);
            const Table cs = filterByCase(all, lc->id);
            if (const auto col = cs.column("von Mises indicatif")) {
                in += p("Les 5 stations de plus grande contrainte de von Mises indicative (tri du tableau "
                        "déformations et contraintes) :");
                in += htmlTable(selectColumns(head(sortByAbsDescending(cs, *col), 5),
                                              {kColElement, kColPosition, "Contrainte normale extrême",
                                               "Cisaillement moyen max", "Torsion", "von Mises indicatif"}));
            }
        }
        h += block(Kind::Result, in);
    }

    // ---------------------------------------------------------------- 13
    section(13);
    {
        const std::string F = esc(u.force), M = esc(u.moment);
        std::string in = p("Équilibre global de chaque cas : la somme des charges appliquées et des réactions doit être "
                           "nulle. Les valeurs « fichier » proviennent du bloc « equilibrium » ; la résultante des "
                           "réactions est en outre recalculée par le post-traitement à partir du tableau des réactions "
                           "(moments pris à l'origine du repère global).");
        if (!r.springs.empty())
            in += p("Le modèle comporte des appuis élastiques : « réaction » désigne ici la somme des réactions des "
                    "appuis rigides et des forces des ressorts, dans le fichier comme dans le recalcul.");
        in += p("Critère retenu par le post-traitement : résidu relatif ≤ 1e-6, le résidu étant rapporté à la plus grande "
                "des normes de la charge appliquée et de la réaction (ou pris en absolu si ces normes sont nulles).");
        const char* comp[6] = {"Fx", "Fy", "Fz", "Mx", "My", "Mz"};
        for (const LoadCase* lc : cases) {
            const Equilibrium& e = lc->equilibrium;
            const Resultant rr = reactionResultant(r, *lc);
            std::vector<std::vector<std::string>> rows;
            for (std::size_t k = 0; k < 6; ++k) {
                const auto& app = k < 3 ? e.appliedForce[k] : e.appliedMoment[k - 3];
                const auto& rea = k < 3 ? e.reactionForce[k] : e.reactionMoment[k - 3];
                const auto& rec = k < 3 ? rr.force[k] : rr.moment[k - 3];
                const OptDouble sum = (app && rea) ? OptDouble(*app + *rea) : std::nullopt;
                const OptDouble gap = (rec && rea) ? OptDouble(*rec - *rea) : std::nullopt;
                rows.push_back({std::string(comp[k]) + " [" + (k < 3 ? F : M) + "]", fmt(app, 6), fmt(rea, 6),
                                fmt(sum, 4), fmt(rec, 6), fmt(gap, 4)});
            }
            in += "<h3>" + caseLabel(*lc) + "</h3>\n" +
                  htmlTextTable({"Composante", "Appliqué (fichier)", "Réaction (fichier)", "Somme",
                                 "Σ réactions recalculée (post-traitement)", "Écart recalcul − fichier"},
                                rows);
            auto residualRow = [&](const std::string& name, const OptDouble& fileRes,
                                   const std::array<OptDouble, 3>& a, const std::array<OptDouble, 3>& b) {
                const OptDouble recomputed = normOfSum(a, b);
                const OptDouble na = norm(a), nb = norm(b);
                std::string rel = kNA, verdict = kNA;
                if (fileRes && na && nb) {
                    const double ref = std::max(*na, *nb);
                    const double relative = ref > 0.0 ? *fileRes / ref : *fileRes;
                    rel = esc(formatShort(relative, 3));
                    verdict = relative <= 1e-6 ? "<span class=\"ok\">satisfait</span>"
                                               : "<span class=\"ko\">non satisfait</span>";
                }
                return std::vector<std::string>{name, fmt(fileRes, 4), fmt(recomputed, 4), rel, verdict};
            };
            std::vector<std::vector<std::string>> res{
                residualRow("Forces [" + F + "]", e.forceResidual, e.appliedForce, e.reactionForce),
                residualRow("Moments [" + M + "]", e.momentResidual, e.appliedMoment, e.reactionMoment)};
            in += htmlTextTable({"Résidu", "Résidu (fichier)", "‖appliqué + réaction‖ (post-traitement)",
                                 "Résidu relatif (fichier)", "Critère ≤ 1e-6"},
                                res);
            if (e.solutionResidual)
                in += p("Résidu relatif de la résolution ‖K U − F‖ / ‖F‖ (moteur) : " + fmt(e.solutionResidual, 3) +
                        ". Le contrôle d'équilibre ne valide pas la rigidité elle-même : il vérifie l'équivalence "
                        "statique des charges et la précision de la résolution.");
        }
        h += block(Kind::Check, in);
    }

    // ---------------------------------------------------------------- 14
    section(14);
    if (opt.openSees) {
        std::vector<std::string> headers;
        for (const auto& s : opt.openSees->header)
            headers.push_back(esc(s));
        std::vector<std::vector<std::string>> rows;
        for (const auto& row : opt.openSees->rows) {
            std::vector<std::string> rr;
            for (const auto& c : row)
                rr.push_back(esc(c));
            rows.push_back(rr);
        }
        h += block(Kind::Check,
                   p("Synthèse de comparaison fournie : <b>" + esc(opt.openSeesSource) + "</b> (" +
                     std::to_string(rows.size()) + " lignes). Elle est reproduite sans retraitement ; la concordance des "
                     "conventions de signes et d'unités entre les deux calculs n'est pas vérifiée automatiquement.") +
                       htmlTextTable(headers, rows));
    } else {
        h += block(Kind::Check, p("Comparaison OpenSees : <b>non fournie</b>. Aucune synthèse n'a été transmise "
                                  "(option --opensees-summary)."));
    }

    // ---------------------------------------------------------------- 15
    section(15);
    {
        std::string warnings;
        for (const LoadCase* lc : cases)
            for (const auto& w : lc->warnings)
                warnings += "<li>" + caseLabel(*lc) + " : " + esc(w) + "</li>\n";
        h += block(Kind::Check, warnings.empty()
                                    ? p("Avertissements du moteur : aucun.")
                                    : "<p><b>Avertissements du moteur</b> (non bloquants, à examiner avant d'exploiter "
                                      "les résultats) :</p>\n<ul>\n" + warnings + "</ul>\n");
    }
    h += block(Kind::Note,
               "<ul>\n"
               "<li>La note ne contient que ce que le fichier de résultats fournit : le détail des charges, le type de "
               "factorisation et les hypothèses de modélisation propres au projet n'y figurent pas.</li>\n"
               "<li>Les efforts sont exacts aux stations ; tout tracé entre stations est une interpolation linéaire, "
               "y compris sous charge répartie (le maximum réel peut se trouver entre deux stations).</li>\n"
               "<li>La flèche maximale aux stations peut être inférieure à maxDeflection du fichier, évaluée plus "
               "finement par le moteur.</li>\n"
               "<li>Les contraintes sont des approximations (section 12) : von Mises est indicatif et ne vaut pas "
               "vérification réglementaire ; aucune vérification selon une norme (Eurocodes ou autre) n'est faite.</li>\n"
               "<li>Aucune combinaison ni enveloppe de cas n'est construite ; chaque cas est présenté isolément.</li>\n"
               "<li>Analyse linéaire en petits déplacements : flambement, second ordre, plasticité et dynamique ne sont "
               "pas couverts.</li>\n"
               "<li>Une valeur indisponible (null) est affichée « — » et laissée vide dans les CSV ; elle n'est "
               "jamais remplacée par 0.</li>\n"
               "<li>Le critère d'équilibre (1e-6) est un choix du post-traitement, pas une exigence normative.</li>\n"
               "</ul>\n");

    h += "</body>\n</html>\n";
    rep.html = std::move(h);
    return rep;
}

} // namespace fem3dpost
