#include "fem3dpost/Tables.h"

#include "fem3dpost/Calculations.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fem3dpost {

std::optional<std::size_t> Table::column(std::string_view label) const
{
    for (std::size_t i = 0; i < columns.size(); ++i)
        if (columns[i].label == label)
            return i;
    return std::nullopt;
}

namespace {

Cell opt(const OptDouble& v)
{
    return v ? Cell{*v} : Cell{};
}

Cell id(int v)
{
    return Cell{static_cast<long long>(v)};
}

} // namespace

Table displacementTable(const Results& r)
{
    Table t;
    t.title = "Déplacements nodaux (repère global)";
    const std::string& L = r.units.length;
    const std::string& R = r.units.rotation;
    t.columns = {{kColNode, ""}, {kColCase, ""}, {"Ux", L}, {"Uy", L}, {"Uz", L},
                 {"Rx", R},      {"Ry", R},      {"Rz", R}};
    for (const LoadCase& lc : r.loadCases) {
        for (const Node& n : r.nodes) {
            std::vector<Cell> row{id(n.id), id(lc.id)};
            const NodalValues* d = lc.displacementOf(n.id);
            for (std::size_t k = 0; k < 6; ++k)
                row.push_back(d ? opt(d->values[k]) : Cell{});  // nœud absent du cas : indisponible
            t.rows.push_back(std::move(row));
        }
    }
    return t;
}

Table reactionTable(const Results& r)
{
    Table t;
    t.title = "Réactions d'appui (repère global, exercées par l'appui sur la structure)";
    const std::string& F = r.units.force;
    const std::string& M = r.units.moment;
    t.columns = {{kColNode, ""}, {kColCase, ""}, {"Fx", F}, {"Fy", F}, {"Fz", F},
                 {"Mx", M},      {"My", M},      {"Mz", M}};
    // Seuls les nœuds déclarés dans « supports » ont une réaction ; une éventuelle entrée du
    // JSON pour un nœud libre (valeurs nulles) est ignorée.
    for (const LoadCase& lc : r.loadCases) {
        for (const Support& s : r.supports) {
            std::vector<Cell> row{id(s.node), id(lc.id)};
            const NodalValues* rv = lc.reactionOf(s.node);
            for (std::size_t k = 0; k < 6; ++k)
                row.push_back(rv ? opt(rv->values[k]) : Cell{});
            t.rows.push_back(std::move(row));
        }
    }
    return t;
}

Table springForceTable(const Results& r)
{
    Table t;
    t.title = "Forces des ressorts (repère global, −k·u, exercées par le ressort sur la structure)";
    const std::string& F = r.units.force;
    const std::string& M = r.units.moment;
    t.columns = {{kColNode, ""}, {kColCase, ""}, {"Fx", F}, {"Fy", F}, {"Fz", F},
                 {"Mx", M},      {"My", M},      {"Mz", M}};
    for (const LoadCase& lc : r.loadCases) {
        for (const Spring& s : r.springs) {
            std::vector<Cell> row{id(s.node), id(lc.id)};
            const NodalValues* sv = lc.springForceOf(s.node);
            for (std::size_t k = 0; k < 6; ++k)
                row.push_back(sv ? opt(sv->values[k]) : Cell{});
            t.rows.push_back(std::move(row));
        }
    }
    return t;
}

Table sectionForceTable(const Results& r)
{
    Table t;
    t.title = "Sollicitations aux stations (face positive, repère local)";
    const std::string& F = r.units.force;
    const std::string& M = r.units.moment;
    t.columns = {{kColElement, ""}, {kColCase, ""}, {kColPosition, r.units.length},
                 {"N", F},          {"Vy", F},      {"Vz", F},
                 {"T", M},          {"My", M},      {"Mz", M}};
    for (const LoadCase& lc : r.loadCases)
        for (const ElementResult& er : lc.elements)
            for (const Station& s : er.stations) {
                std::vector<Cell> row{id(er.id), id(lc.id), Cell{s.x}};
                for (const auto& f : s.forces)
                    row.push_back(opt(f));
                t.rows.push_back(std::move(row));
            }
    return t;
}

Table strainStressTable(const Results& r)
{
    Table t;
    t.title = "Déformations et contraintes aux stations";
    const std::string& L = r.units.length;
    const std::string& R = r.units.rotation;
    const std::string& S = r.units.stress;
    t.columns = {{kColElement, ""},
                 {kColCase, ""},
                 {kColPosition, L},
                 {"Flèche locale √(v²+w²) / corde", L},
                 {"Rotation de section θx", R},
                 {"Rotation de section θy", R},
                 {"Rotation de section θz", R},
                 {"Contrainte normale extrême", S},
                 {"Cisaillement moyen max", S},
                 {"Torsion", S},
                 {"von Mises indicatif", S}};
    for (const LoadCase& lc : r.loadCases)
        for (const ElementResult& er : lc.elements) {
            const Element* e = r.element(er.id);
            for (const Station& s : er.stations) {
                std::vector<Cell> row{id(er.id), id(lc.id), Cell{s.x}};
                const auto c = e ? chordDeflection(*e, er, s) : std::nullopt;
                row.push_back(c ? Cell{c->magnitude} : Cell{});
                for (const auto& th : s.sectionRotation)
                    row.push_back(opt(th));
                row.push_back(opt(s.stresses.extremeNormal));
                row.push_back(opt(maxMeanShear(s.stresses)));
                row.push_back(opt(s.stresses.torsion));
                row.push_back(opt(s.stresses.vonMisesIndicative));
                t.rows.push_back(std::move(row));
            }
        }
    return t;
}

OptDouble cellNumber(const Cell& c)
{
    if (const auto* i = std::get_if<long long>(&c))
        return static_cast<double>(*i);
    if (const auto* d = std::get_if<double>(&c))
        return *d;
    return std::nullopt;
}

namespace {

Table filterById(const Table& t, const char* label, int value)
{
    const auto col = t.column(label);
    if (!col)
        throw std::invalid_argument(std::string("colonne « ") + label + " » absente du tableau « " + t.title + " »");
    Table out;
    out.title = t.title;
    out.columns = t.columns;
    for (const auto& row : t.rows) {
        const auto v = cellNumber(row[*col]);
        if (v && *v == static_cast<double>(value))
            out.rows.push_back(row);
    }
    return out;
}

} // namespace

Table filterByCase(const Table& t, int caseId)
{
    return filterById(t, kColCase, caseId);
}

Table filterByElement(const Table& t, int elementId)
{
    return filterById(t, kColElement, elementId);
}

Table sortByAbsDescending(const Table& t, std::size_t column)
{
    if (column >= t.columns.size())
        throw std::out_of_range("colonne de tri hors du tableau « " + t.title + " »");
    Table out = t;
    std::stable_sort(out.rows.begin(), out.rows.end(), [column](const auto& a, const auto& b) {
        const auto va = cellNumber(a[column]);
        const auto vb = cellNumber(b[column]);
        if (!va || !vb)
            return va.has_value() && !vb.has_value();  // les cellules vides en fin
        return std::fabs(*va) > std::fabs(*vb);
    });
    return out;
}

Table head(const Table& t, std::size_t n)
{
    Table out;
    out.title = t.title;
    out.columns = t.columns;
    out.rows.assign(t.rows.begin(), t.rows.begin() + static_cast<std::ptrdiff_t>(std::min(n, t.rows.size())));
    return out;
}

} // namespace fem3dpost
