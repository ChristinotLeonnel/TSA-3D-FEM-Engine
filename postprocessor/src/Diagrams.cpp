#include "fem3dpost/Diagrams.h"

#include "fem3dpost/Calculations.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <sstream>

namespace fem3dpost {

std::string formatShort(double v, int significant)
{
    if (v == 0.0)
        return "0";
    char buf[64];
    const auto res = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::general, significant);
    return std::string(buf, res.ptr);
}

const std::vector<std::string>& diagramQuantities()
{
    static const std::vector<std::string> q{"N", "Vy", "Vz", "T", "My", "Mz", "deformee", "fleches", "rotations"};
    return q;
}

std::vector<Diagram> elementDiagrams(const Results& r, const LoadCase& lc, const ElementResult& er)
{
    const Element* e = r.element(er.id);
    std::vector<double> x;
    for (const Station& s : er.stations)
        x.push_back(s.x);
    auto base = [&](const std::string& q, const std::string& title, const std::string& unit,
                    const std::string& note) {
        Diagram d;
        d.quantity = q;
        d.title = title;
        d.unit = unit;
        d.signNote = note;
        d.caseId = lc.id;
        d.elementId = er.id;
        d.length = e ? e->length : (x.empty() ? 0.0 : x.back());
        d.x = x;
        return d;
    };
    std::vector<Diagram> out;
    const std::string forceNote = "Convention du fichier : " + r.conventions.sectionForces;
    const char* names[6] = {"N", "Vy", "Vz", "T", "My", "Mz"};
    const char* titles[6] = {"Effort normal N", "Effort tranchant Vy", "Effort tranchant Vz",
                             "Moment de torsion T", "Moment fléchissant My", "Moment fléchissant Mz"};
    for (std::size_t k = 0; k < 6; ++k) {
        Diagram d = base(names[k], titles[k], k < 3 ? r.units.force : r.units.moment, forceNote);
        DiagramSeries s{names[k], {}};
        for (const Station& st : er.stations)
            s.values.push_back(st.forces[k]);
        d.series.push_back(std::move(s));
        out.push_back(std::move(d));
    }
    {
        Diagram d = base("deformee", "Déformée (déplacements globaux)", r.units.length,
                         "Composantes Ux, Uy, Uz du déplacement aux stations, repère global");
        const char* lab[3] = {"Ux", "Uy", "Uz"};
        for (std::size_t k = 0; k < 3; ++k) {
            DiagramSeries s{lab[k], {}};
            for (const Station& st : er.stations)
                s.values.push_back(st.globalDisplacement[k]);
            d.series.push_back(std::move(s));
        }
        out.push_back(std::move(d));
    }
    {
        Diagram d = base("fleches", "Flèches locales (écart à la corde)", r.units.length,
                         "v, w : écart au segment joignant les nœuds i et j, repère local ; f = √(v²+w²)");
        DiagramSeries sv{"v (y local)", {}}, sw{"w (z local)", {}}, sf{"f = √(v²+w²)", {}};
        for (const Station& st : er.stations) {
            const auto c = e ? chordDeflection(*e, er, st) : std::nullopt;
            sv.values.push_back(c ? OptDouble(c->v) : std::nullopt);
            sw.values.push_back(c ? OptDouble(c->w) : std::nullopt);
            sf.values.push_back(c ? OptDouble(c->magnitude) : std::nullopt);
        }
        d.series = {std::move(sv), std::move(sw), std::move(sf)};
        out.push_back(std::move(d));
    }
    {
        Diagram d = base("rotations", "Rotations de section", r.units.rotation,
                         "θx, θy, θz : rotations de section (sectionRotation), repère local");
        const char* lab[3] = {"θx", "θy", "θz"};
        for (std::size_t k = 0; k < 3; ++k) {
            DiagramSeries s{lab[k], {}};
            for (const Station& st : er.stations)
                s.values.push_back(st.sectionRotation[k]);
            d.series.push_back(std::move(s));
        }
        out.push_back(std::move(d));
    }
    return out;
}

Table diagramDataTable(const Results& r, const LoadCase& lc, const ElementResult& er)
{
    const Element* e = r.element(er.id);
    const std::string& L = r.units.length;
    const std::string& F = r.units.force;
    const std::string& M = r.units.moment;
    const std::string& R = r.units.rotation;
    Table t;
    t.title = "Données de diagrammes — cas " + std::to_string(lc.id) + ", élément " + std::to_string(er.id);
    t.columns = {{kColElement, ""},
                 {kColCase, ""},
                 {kColPosition, L},
                 {"N", F},
                 {"Vy", F},
                 {"Vz", F},
                 {"T", M},
                 {"My", M},
                 {"Mz", M},
                 {"Ux global", L},
                 {"Uy global", L},
                 {"Uz global", L},
                 {"Flèche v / corde", L},
                 {"Flèche w / corde", L},
                 {"Flèche √(v²+w²) / corde", L},
                 {"Rotation de section θx", R},
                 {"Rotation de section θy", R},
                 {"Rotation de section θz", R}};
    auto opt = [](const OptDouble& v) { return v ? Cell{*v} : Cell{}; };
    for (const Station& s : er.stations) {
        std::vector<Cell> row{Cell{static_cast<long long>(er.id)}, Cell{static_cast<long long>(lc.id)}, Cell{s.x}};
        for (const auto& f : s.forces)
            row.push_back(opt(f));
        for (const auto& g : s.globalDisplacement)
            row.push_back(opt(g));
        const auto c = e ? chordDeflection(*e, er, s) : std::nullopt;
        row.push_back(c ? Cell{c->v} : Cell{});
        row.push_back(c ? Cell{c->w} : Cell{});
        row.push_back(c ? Cell{c->magnitude} : Cell{});
        for (const auto& th : s.sectionRotation)
            row.push_back(opt(th));
        t.rows.push_back(std::move(row));
    }
    return t;
}

std::string diagramStem(const Diagram& d)
{
    return "cas" + std::to_string(d.caseId) + "_elem" + std::to_string(d.elementId) + "_" + d.quantity;
}

std::string diagramDataStem(int caseId, int elementId)
{
    return "cas" + std::to_string(caseId) + "_elem" + std::to_string(elementId);
}

namespace {

std::string esc(const std::string& s)
{
    std::string o;
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

std::string num(double v)
{
    char buf[32];
    const auto res = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::fixed, 2);
    return std::string(buf, res.ptr);
}

const char* kColors[3] = {"#1f5fae", "#c0392b", "#2e8b57"};

} // namespace

std::string diagramSvg(const Diagram& d)
{
    constexpr double W = 680, H = 300;
    constexpr double left = 84, right = W - 24, top = 50, bottom = H - 86;

    // Domaine vertical : contient 0 et toutes les valeurs disponibles.
    double lo = 0.0, hi = 0.0;
    bool anyValue = false;
    for (const auto& s : d.series)
        for (const auto& v : s.values)
            if (v) {
                lo = std::min(lo, *v);
                hi = std::max(hi, *v);
                anyValue = true;
            }
    const bool flat = (hi - lo) == 0.0;
    if (flat) {
        lo = -1.0;
        hi = 1.0;
    } else {
        const double pad = 0.14 * (hi - lo);
        lo -= (lo < 0.0 ? pad : 0.0) + 0.02 * (hi - lo);
        hi += (hi > 0.0 ? pad : 0.0) + 0.02 * (hi - lo);
    }
    const double L = d.length > 0.0 ? d.length : 1.0;
    auto X = [&](double x) { return left + (right - left) * x / L; };
    auto Y = [&](double v) { return bottom - (bottom - top) * (v - lo) / (hi - lo); };

    std::ostringstream o;
    o << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << W << "\" height=\"" << H << "\" viewBox=\"0 0 "
      << W << ' ' << H << "\" font-family=\"Segoe UI, Arial, sans-serif\" font-size=\"11\">\n";
    o << "<rect x=\"0\" y=\"0\" width=\"" << W << "\" height=\"" << H << "\" fill=\"#ffffff\"/>\n";
    const std::string unit = d.unit.empty() ? "" : " [" + d.unit + "]";
    o << "<text x=\"" << left << "\" y=\"20\" font-size=\"14\" font-weight=\"bold\">" << esc("Cas " + std::to_string(d.caseId) + " — Élément " + std::to_string(d.elementId) + " — " + d.title + unit) << "</text>\n";
    o << "<text x=\"" << left << "\" y=\"36\" font-size=\"10\" fill=\"#555\">" << esc(d.signNote) << "</text>\n";

    // Cadre, axe des abscisses (ligne zéro) et graduations aux stations.
    o << "<rect x=\"" << left << "\" y=\"" << top << "\" width=\"" << (right - left) << "\" height=\"" << (bottom - top)
      << "\" fill=\"none\" stroke=\"#ccc\"/>\n";
    o << "<line x1=\"" << left << "\" y1=\"" << num(Y(0)) << "\" x2=\"" << right << "\" y2=\"" << num(Y(0))
      << "\" stroke=\"#000\" stroke-width=\"1.2\"/>\n";
    for (double x : d.x)
        o << "<line x1=\"" << num(X(x)) << "\" y1=\"" << bottom << "\" x2=\"" << num(X(x)) << "\" y2=\"" << (bottom + 4)
          << "\" stroke=\"#888\"/>\n";
    o << "<text x=\"" << left << "\" y=\"" << (bottom + 16) << "\" text-anchor=\"middle\">0</text>\n";
    o << "<text x=\"" << right << "\" y=\"" << (bottom + 16) << "\" text-anchor=\"middle\">" << esc(formatShort(L, 5))
      << "</text>\n";
    o << "<text x=\"" << ((left + right) / 2) << "\" y=\"" << (bottom + 16) << "\" text-anchor=\"middle\" fill=\"#555\">"
      << esc("x local [m] (nœud i → nœud j)") << "</text>\n";
    // Graduations verticales : 0 et bornes.
    o << "<text x=\"" << (left - 6) << "\" y=\"" << num(Y(0) + 4) << "\" text-anchor=\"end\">0</text>\n";
    if (!flat) {
        // Bornes affichées seulement si elles ne se confondent pas avec la ligne zéro.
        if (hi > 0.1 * (hi - lo))
            o << "<text x=\"" << (left - 6) << "\" y=\"" << num(top + 10) << "\" text-anchor=\"end\" fill=\"#777\">"
              << esc(formatShort(hi, 3)) << "</text>\n";
        if (-lo > 0.1 * (hi - lo))
            o << "<text x=\"" << (left - 6) << "\" y=\"" << num(bottom) << "\" text-anchor=\"end\" fill=\"#777\">"
              << esc(formatShort(lo, 3)) << "</text>\n";
    }

    const bool single = d.series.size() == 1;
    for (std::size_t si = 0; si < d.series.size(); ++si) {
        const auto& s = d.series[si];
        const char* color = kColors[si % 3];
        // Segments continus (une valeur indisponible interrompt le tracé).
        std::vector<std::vector<std::size_t>> runs;
        std::vector<std::size_t> cur;
        for (std::size_t i = 0; i < s.values.size() && i < d.x.size(); ++i) {
            if (s.values[i]) {
                cur.push_back(i);
            } else if (!cur.empty()) {
                runs.push_back(cur);
                cur.clear();
            }
        }
        if (!cur.empty())
            runs.push_back(cur);
        for (const auto& run : runs) {
            if (single) {
                o << "<polygon fill=\"" << color << "\" fill-opacity=\"0.15\" stroke=\"none\" points=\"";
                o << num(X(d.x[run.front()])) << ',' << num(Y(0)) << ' ';
                for (std::size_t i : run)
                    o << num(X(d.x[i])) << ',' << num(Y(*s.values[i])) << ' ';
                o << num(X(d.x[run.back()])) << ',' << num(Y(0)) << "\"/>\n";
            }
            o << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"1.8\" points=\"";
            for (std::size_t i : run)
                o << num(X(d.x[i])) << ',' << num(Y(*s.values[i])) << ' ';
            o << "\"/>\n";
            for (std::size_t i : run)
                o << "<circle class=\"station\" cx=\"" << num(X(d.x[i])) << "\" cy=\"" << num(Y(*s.values[i]))
                  << "\" r=\"2.6\" fill=\"" << color << "\"/>\n";
        }

        // Annotations des extrêmes.
        const SeriesExtremes ex = seriesExtremes(d.x, s.values);
        auto annotate = [&](const Extremum& e, const std::string& what, bool above) {
            const double px = X(e.x), py = Y(e.value);
            const char* anchor = px < left + 90 ? "start" : (px > right - 90 ? "end" : "middle");
            const double ty = std::clamp(above ? py - 7 : py + 15, top + 10, bottom - 3);
            o << "<text class=\"extreme\" x=\"" << num(px) << "\" y=\"" << num(ty) << "\" text-anchor=\"" << anchor
              << "\" fill=\"" << color << "\" font-weight=\"bold\">"
              << esc(what + " = " + formatShort(e.value) + " (x = " + formatShort(e.x, 4) + ")") << "</text>\n";
        };
        if (single) {
            if (ex.max && ex.min) {
                if (ex.max->value == ex.min->value) {
                    annotate(*ex.max, "constante", ex.max->value >= 0);
                } else {
                    annotate(*ex.max, "max", true);
                    annotate(*ex.min, "min", false);
                }
            }
        } else if (ex.maxAbs) {
            annotate(*ex.maxAbs, s.label + " |max|", ex.maxAbs->value >= 0);
        }
    }

    // Légende.
    double ly = H - 50;
    if (!single) {
        double lx = left;
        for (std::size_t si = 0; si < d.series.size(); ++si) {
            o << "<line x1=\"" << lx << "\" y1=\"" << (ly - 4) << "\" x2=\"" << (lx + 18) << "\" y2=\"" << (ly - 4)
              << "\" stroke=\"" << kColors[si % 3] << "\" stroke-width=\"2\"/>\n";
            o << "<text x=\"" << (lx + 22) << "\" y=\"" << ly << "\">" << esc(d.series[si].label) << "</text>\n";
            lx += 150;
        }
    }
    ly += 16;
    o << "<text class=\"legend\" x=\"" << left << "\" y=\"" << ly << "\" fill=\"#333\">"
      << esc("Points : valeurs aux " + std::to_string(d.x.size()) +
             " stations fournies par le moteur. Entre stations, le tracé est une interpolation linéaire.")
      << "</text>\n";
    ly += 14;
    o << "<text x=\"" << left << "\" y=\"" << ly << "\" fill=\"#333\">"
      << esc(anyValue ? "Valeurs positives vers le haut (signe conservé). Valeur indisponible : tracé interrompu."
                      : "Aucune valeur disponible pour cette grandeur.")
      << "</text>\n";
    o << "</svg>\n";
    return o.str();
}

} // namespace fem3dpost
