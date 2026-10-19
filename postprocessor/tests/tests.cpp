// Tests de fem3dpost sur le fichier réel d'exemple et sur des variantes modifiées en mémoire.

#include "MiniTest.h"

#include "fem3dpost/Calculations.h"
#include "fem3dpost/Csv.h"
#include "fem3dpost/Diagrams.h"
#include "fem3dpost/Report.h"
#include "fem3dpost/ResultsReader.h"
#include "fem3dpost/Tables.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <cmath>
#include <cstdlib>
#include <functional>
#include <set>
#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifndef FEM3DPOST_EXAMPLE_JSON
#error "FEM3DPOST_EXAMPLE_JSON doit être défini par CMake"
#endif

using namespace fem3dpost;

namespace {

QByteArray exampleBytes()
{
    QFile f(QString::fromUtf8(FEM3DPOST_EXAMPLE_JSON));
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(std::string("fichier d'exemple introuvable : ") + FEM3DPOST_EXAMPLE_JSON);
    return f.readAll();
}

const Results& example()
{
    static const Results r = readResultsFile(QString::fromUtf8(FEM3DPOST_EXAMPLE_JSON));
    return r;
}

QJsonObject exampleRoot()
{
    return QJsonDocument::fromJson(exampleBytes()).object();
}

QByteArray toBytes(const QJsonObject& root)
{
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool contains(const std::string& s, const std::string& what)
{
    return s.find(what) != std::string::npos;
}

std::size_t countOf(const std::string& s, const std::string& what)
{
    std::size_t n = 0;
    for (std::size_t p = s.find(what); p != std::string::npos; p = s.find(what, p + what.size()))
        ++n;
    return n;
}

std::string errorMessage(const QByteArray& bytes)
{
    try {
        parseResults(bytes);
    } catch (const FormatError& e) {
        return e.what();
    }
    return {};
}

// Modifie un objet imbriqué : root[k1][i1]... via une fonction appliquée à la feuille.
QJsonObject editCase(QJsonObject root, int caseIndex, const std::function<void(QJsonObject&)>& f)
{
    QJsonArray cases = root["loadCases"].toArray();
    QJsonObject lc = cases[caseIndex].toObject();
    f(lc);
    cases[caseIndex] = lc;
    root["loadCases"] = cases;
    return root;
}

QJsonObject editStation(QJsonObject root, int caseIndex, int elementIndex, int stationIndex,
                        const std::function<void(QJsonObject&)>& f)
{
    return editCase(std::move(root), caseIndex, [&](QJsonObject& lc) {
        QJsonArray els = lc["elements"].toArray();
        QJsonObject el = els[elementIndex].toObject();
        QJsonArray st = el["stations"].toArray();
        QJsonObject s = st[stationIndex].toObject();
        f(s);
        st[stationIndex] = s;
        el["stations"] = st;
        els[elementIndex] = el;
        lc["elements"] = els;
    });
}

std::vector<std::vector<std::string>> csvRows(const Table& t)
{
    return parseDelimited(toCsv(t), ';');
}

} // namespace

// ---------------------------------------------------------------------------
TEST_CASE("lecture du fichier réel : nœuds, éléments, cas")
{
    const Results& r = example();
    CHECK_EQ(r.format, std::string("tsa-fem3d-results/1"));
    CHECK_EQ(r.nodes.size(), std::size_t(8));
    CHECK_EQ(r.elements.size(), std::size_t(8));
    CHECK_EQ(r.loadCases.size(), std::size_t(2));
    CHECK_EQ(r.supports.size(), std::size_t(4));
    CHECK_EQ(r.units.force, std::string("kN"));
    CHECK_EQ(r.units.stress, std::string("kN/m2"));
    CHECK_EQ(r.conventions.dofOrder.size(), std::size_t(6));
    for (const LoadCase& lc : r.loadCases) {
        CHECK_EQ(lc.elements.size(), std::size_t(8));
        CHECK_EQ(lc.displacements.size(), std::size_t(8));
        for (const ElementResult& er : lc.elements)
            CHECK_EQ(er.stations.size(), std::size_t(11));
    }
    REQUIRE(r.loadCase(1) && r.loadCase(2));
    CHECK(contains(r.loadCase(2)->name, "vent"));
    REQUIRE(r.element(11) != nullptr);
    CHECK_EQ(r.element(11)->formulation, std::string("Timoshenko"));
    CHECK_EQ(r.element(11)->nodeI, 11);
    CHECK_EQ(r.element(11)->nodeJ, 12);
}

TEST_CASE("refus d'un format inconnu")
{
    QJsonObject root = exampleRoot();
    root["format"] = "tsa-fem3d-results/2";
    const std::string msg = errorMessage(toBytes(root));
    CHECK(!msg.empty());
    CHECK(contains(msg, "tsa-fem3d-results/2"));
    CHECK(contains(msg, "tsa-fem3d-results/1"));

    root.remove("format");
    CHECK(contains(errorMessage(toBytes(root)), "format"));
    CHECK_THROWS(parseResults(QByteArray("{ pas du JSON")), FormatError);
    CHECK_THROWS(readResultsFile(QStringLiteral("Z:/inexistant/absent.json")), FormatError);
}

TEST_CASE("refus d'un champ obligatoire manquant")
{
    {
        QJsonObject root = exampleRoot();
        root.remove("units");
        const std::string msg = errorMessage(toBytes(root));
        CHECK(contains(msg, "units"));
        CHECK(contains(msg, "absent"));
    }
    {
        QJsonObject root = exampleRoot();
        QJsonArray els = root["elements"].toArray();
        QJsonObject e = els[0].toObject();
        e.remove("length");
        els[0] = e;
        root["elements"] = els;
        CHECK(contains(errorMessage(toBytes(root)), "elements[0].length"));
    }
    {
        const QJsonObject root = editStation(exampleRoot(), 1, 2, 3, [](QJsonObject& s) { s.remove("forces"); });
        CHECK(contains(errorMessage(toBytes(root)), "loadCases[1].elements[2].stations[3].forces"));
    }
    {
        const QJsonObject root = editCase(exampleRoot(), 0, [](QJsonObject& lc) { lc.remove("equilibrium"); });
        CHECK(contains(errorMessage(toBytes(root)), "loadCases[0].equilibrium"));
    }
    {   // tableau de mauvaise taille
        const QJsonObject root =
            editStation(exampleRoot(), 0, 0, 0, [](QJsonObject& s) { s["forces"] = QJsonArray{1.0, 2.0}; });
        CHECK(contains(errorMessage(toBytes(root)), "6 valeurs"));
    }
}

TEST_CASE("null conservé comme indisponible, jamais remplacé par 0")
{
    QJsonObject root = editStation(exampleRoot(), 0, 0, 3, [](QJsonObject& s) {
        QJsonObject st = s["stresses"].toObject();
        st["vonMisesIndicative"] = QJsonValue::Null;
        s["stresses"] = st;
        QJsonArray f = s["forces"].toArray();
        f[4] = QJsonValue::Null;  // My
        s["forces"] = f;
    });
    root = editStation(root, 0, 1, 2, [](QJsonObject& s) { s["stresses"] = QJsonValue::Null; });
    root = editCase(root, 0, [](QJsonObject& lc) {
        QJsonArray d = lc["displacements"].toArray();
        QJsonObject n = d[1].toObject();
        QJsonArray v = n["values"].toArray();
        v[2] = QJsonValue::Null;  // Uz du 2e nœud
        n["values"] = v;
        d[1] = n;
        lc["displacements"] = d;
    });
    const Results r = parseResults(toBytes(root));
    const Station& s = r.loadCases[0].elements[0].stations[3];
    CHECK(!s.stresses.vonMisesIndicative.has_value());
    CHECK(s.stresses.extremeNormal.has_value());
    CHECK(!s.forces[4].has_value());
    CHECK(s.forces[3].has_value());
    const Station& s2 = r.loadCases[0].elements[1].stations[2];
    CHECK(!s2.stresses.axial && !s2.stresses.extremeNormal && !s2.stresses.vonMisesIndicative);
    const int node = r.loadCases[0].displacements[1].node;
    CHECK(!r.loadCases[0].displacements[1].values[2].has_value());

    // Tableau des déplacements : cellule vide (monostate), CSV : champ vide.
    const Table disp = filterByCase(displacementTable(r), 1);
    bool found = false;
    for (const auto& row : disp.rows)
        if (cellNumber(row[0]) == double(node)) {
            found = true;
            CHECK(std::holds_alternative<std::monostate>(row[4]));  // Uz
            CHECK(!std::holds_alternative<std::monostate>(row[3])); // Ux
        }
    CHECK(found);
    for (const auto& row : csvRows(disp))
        if (row[0] == std::to_string(node))
            CHECK_EQ(row[4], std::string(""));

    // Sollicitations et contraintes : cellule vide aux stations concernées.
    const Table forces = filterByElement(filterByCase(sectionForceTable(r), 1), r.loadCases[0].elements[0].id);
    CHECK(std::holds_alternative<std::monostate>(forces.rows[3][*forces.column("My")]));
    const Table stress = filterByElement(filterByCase(strainStressTable(r), 1), r.loadCases[0].elements[0].id);
    CHECK(std::holds_alternative<std::monostate>(stress.rows[3][*stress.column("von Mises indicatif")]));
    CHECK(!std::holds_alternative<std::monostate>(stress.rows[3][*stress.column("Contrainte normale extrême")]));

    // Un type incorrect (chaîne au lieu de nombre) reste une erreur.
    const QJsonObject bad = editStation(exampleRoot(), 0, 0, 0, [](QJsonObject& s) {
        QJsonArray f = s["forces"].toArray();
        f[0] = "12";
        s["forces"] = f;
    });
    CHECK(contains(errorMessage(toBytes(bad)), "nombre ou null"));
}

TEST_CASE("valeurs des tableaux identiques au JSON (y compris après relecture du CSV)")
{
    const Results& r = example();
    const QJsonObject root = exampleRoot();
    const QJsonArray cases = root["loadCases"].toArray();

    // Déplacements.
    const Table disp = displacementTable(r);
    const auto dispCsv = csvRows(disp);
    std::size_t compared = 0;
    for (std::size_t i = 0; i < disp.rows.size(); ++i) {
        const int nodeId = int(*cellNumber(disp.rows[i][0]));
        const int caseId = int(*cellNumber(disp.rows[i][1]));
        for (const auto& cv : cases) {
            const QJsonObject lc = cv.toObject();
            if (lc["id"].toInt() != caseId)
                continue;
            for (const auto& dv : lc["displacements"].toArray()) {
                const QJsonObject d = dv.toObject();
                if (d["node"].toInt() != nodeId)
                    continue;
                const QJsonArray vals = d["values"].toArray();
                for (int k = 0; k < 6; ++k) {
                    const double json = vals[k].toDouble();
                    CHECK_EQ(*cellNumber(disp.rows[i][std::size_t(2 + k)]), json);
                    // relecture du CSV : valeur exactement identique (représentation aller-retour)
                    CHECK_EQ(std::strtod(dispCsv[i + 1][std::size_t(2 + k)].c_str(), nullptr), json);
                    ++compared;
                }
            }
        }
    }
    CHECK_EQ(compared, std::size_t(2 * 8 * 6));

    // Sollicitations : ordre cas → éléments → stations, comme dans le JSON.
    const Table forces = sectionForceTable(r);
    std::size_t row = 0;
    bool allEqual = true;
    for (const auto& cv : cases)
        for (const auto& ev : cv.toObject()["elements"].toArray())
            for (const auto& sv : ev.toObject()["stations"].toArray()) {
                const QJsonObject s = sv.toObject();
                const QJsonArray f = s["forces"].toArray();
                allEqual = allEqual && *cellNumber(forces.rows[row][2]) == s["x"].toDouble();
                for (int k = 0; k < 6; ++k)
                    allEqual = allEqual && *cellNumber(forces.rows[row][std::size_t(3 + k)]) == f[k].toDouble();
                ++row;
            }
    CHECK(allEqual);
    CHECK_EQ(row, forces.rows.size());
    CHECK_EQ(row, std::size_t(2 * 8 * 11));

    // Contraintes : contrainte normale extrême identique au JSON.
    const Table stress = strainStressTable(r);
    const auto col = stress.column("Contrainte normale extrême");
    REQUIRE(col.has_value());
    const QJsonObject s0 = cases[0].toObject()["elements"].toArray()[0].toObject()["stations"].toArray()[0].toObject();
    CHECK_EQ(*cellNumber(stress.rows[0][*col]), s0["stresses"].toObject()["extremeNormal"].toDouble());
}

TEST_CASE("tri et filtrage")
{
    const Results& r = example();
    const Table forces = sectionForceTable(r);
    const Table c2 = filterByCase(forces, 2);
    CHECK_EQ(c2.rows.size(), std::size_t(8 * 11));
    for (const auto& row : c2.rows)
        CHECK_EQ(*cellNumber(row[1]), 2.0);
    const Table e11 = filterByElement(forces, 11);
    CHECK_EQ(e11.rows.size(), std::size_t(2 * 11));
    CHECK(filterByCase(forces, 99).rows.empty());

    const std::size_t my = *forces.column("My");
    const Table sorted = sortByAbsDescending(forces, my);
    CHECK_EQ(sorted.rows.size(), forces.rows.size());
    bool monotone = true;
    for (std::size_t i = 1; i < sorted.rows.size(); ++i)
        monotone = monotone && std::fabs(*cellNumber(sorted.rows[i - 1][my])) >= std::fabs(*cellNumber(sorted.rows[i][my]));
    CHECK(monotone);
    double maxAbs = 0.0;
    for (const auto& row : forces.rows)
        maxAbs = std::max(maxAbs, std::fabs(*cellNumber(row[my])));
    CHECK_EQ(std::fabs(*cellNumber(sorted.rows[0][my])), maxAbs);
    CHECK_EQ(head(sorted, 3).rows.size(), std::size_t(3));

    // Cellules vides en fin, tri stable.
    Table t;
    t.title = "essai";
    t.columns = {{kColCase, ""}, {"V", "m"}};
    t.rows = {{Cell{1LL}, Cell{}}, {Cell{1LL}, Cell{-3.0}}, {Cell{2LL}, Cell{2.0}}, {Cell{2LL}, Cell{3.0}}};
    const Table ts = sortByAbsDescending(t, 1);
    CHECK_EQ(*cellNumber(ts.rows[0][1]), -3.0);  // |−3| = |3| : ordre d'origine conservé
    CHECK_EQ(*cellNumber(ts.rows[1][1]), 3.0);
    CHECK_EQ(*cellNumber(ts.rows[2][1]), 2.0);
    CHECK(std::holds_alternative<std::monostate>(ts.rows[3][1]));
    CHECK_THROWS(filterByElement(t, 1), std::invalid_argument);
    CHECK_THROWS(sortByAbsDescending(t, 5), std::out_of_range);
}

TEST_CASE("contenu CSV : en-têtes, unités, séparateur, point décimal")
{
    const Results& r = example();
    const std::string disp = toCsv(displacementTable(r));
    CHECK(disp.rfind(kUtf8Bom, 0) == 0);
    const std::string firstLine = disp.substr(3, disp.find("\r\n") - 3);
    CHECK_EQ(firstLine, std::string("Nœud;Cas;Ux [m];Uy [m];Uz [m];Rx [rad];Ry [rad];Rz [rad]"));

    auto header = [](const std::string& csv) { return csv.substr(3, csv.find("\r\n") - 3); };
    CHECK_EQ(header(toCsv(reactionTable(r))),
             std::string("Nœud;Cas;Fx [kN];Fy [kN];Fz [kN];Mx [kN.m];My [kN.m];Mz [kN.m]"));
    CHECK_EQ(header(toCsv(sectionForceTable(r))),
             std::string("Élément;Cas;Position x [m];N [kN];Vy [kN];Vz [kN];T [kN.m];My [kN.m];Mz [kN.m]"));
    CHECK_EQ(header(toCsv(strainStressTable(r))),
             std::string("Élément;Cas;Position x [m];Flèche locale √(v²+w²) / corde [m];Rotation de section θx [rad];"
                         "Rotation de section θy [rad];Rotation de section θz [rad];Contrainte normale extrême [kN/m2];"
                         "Cisaillement moyen max [kN/m2];Torsion [kN/m2];von Mises indicatif [kN/m2]"));

    const Table forces = sectionForceTable(r);
    const auto rows = csvRows(forces);
    CHECK_EQ(rows.size(), forces.rows.size() + 1);
    bool sameWidth = true, noComma = true;
    for (const auto& row : rows) {
        sameWidth = sameWidth && row.size() == forces.columns.size();
        for (const auto& c : row)
            noComma = noComma && c.find(',') == std::string::npos;
    }
    CHECK(sameWidth);
    CHECK(noComma);
    CHECK_EQ(formatNumber(0.5), std::string("0.5"));
    CHECK_EQ(formatNumber(-1.25e-7), std::string("-1.25e-07"));

    const Table dd = diagramDataTable(r, r.loadCases[0], r.loadCases[0].elements[0]);
    CHECK_EQ(dd.rows.size(), std::size_t(11));
    CHECK(contains(toCsv(dd), "Flèche √(v²+w²) / corde [m]"));
    CHECK(contains(toCsv(dd), "Ux global [m]"));
}

TEST_CASE("réactions uniquement pour les nœuds d'appui")
{
    const Results& r = example();
    const Table reac = reactionTable(r);
    CHECK_EQ(reac.rows.size(), std::size_t(2 * 4));
    std::set<int> nodes;
    for (const auto& row : reac.rows)
        nodes.insert(int(*cellNumber(row[0])));
    CHECK(nodes == std::set<int>({1, 2, 3, 4}));
    for (int n : nodes)
        CHECK(r.isSupport(n));

    // Une entrée de réaction pour un nœud libre (valeurs nulles) n'apparaît pas dans le tableau ;
    // un appui sans entrée de réaction donne des cellules vides (indisponible), pas des zéros.
    QJsonObject root = editCase(exampleRoot(), 0, [](QJsonObject& lc) {
        QJsonArray re = lc["reactions"].toArray();
        re.removeAt(0);  // nœud d'appui 1 sans réaction
        re.append(QJsonObject{{"node", 11}, {"values", QJsonArray{0, 0, 0, 0, 0, 0}}});
        lc["reactions"] = re;
    });
    const Results m = parseResults(toBytes(root));
    const Table t = filterByCase(reactionTable(m), 1);
    CHECK_EQ(t.rows.size(), std::size_t(4));
    bool node11 = false, node1Empty = false;
    for (const auto& row : t.rows) {
        if (cellNumber(row[0]) == 11.0)
            node11 = true;
        if (cellNumber(row[0]) == 1.0)
            node1Empty = std::holds_alternative<std::monostate>(row[2]) && std::holds_alternative<std::monostate>(row[7]);
    }
    CHECK(!node11);
    CHECK(node1Empty);
}

TEST_CASE("flèche locale par rapport à la corde")
{
    const Results& r = example();
    const LoadCase& lc = *r.loadCase(1);
    const ElementResult& er = *lc.elementResult(1);
    const Element& e = *r.element(1);
    const auto c0 = chordDeflection(e, er, er.stations.front());
    const auto c10 = chordDeflection(e, er, er.stations.back());
    REQUIRE(c0 && c10);
    CHECK(c0->magnitude < 1e-15);
    CHECK(c10->magnitude < 1e-15);
    // Valeur de référence recalculée indépendamment (Python) : 3.111729e-05 m à x = 1.8 m.
    const auto c6 = chordDeflection(e, er, er.stations[6]);
    REQUIRE(c6.has_value());
    CHECK_EQ(er.stations[6].x, 1.8);
    CHECK(std::fabs(c6->magnitude - 3.111729e-05) < 1e-11);
    // Le maximum aux stations ne dépasse pas maxDeflection du moteur (évalué plus finement).
    for (const LoadCase& c : r.loadCases)
        for (const ElementResult& x : c.elements)
            for (const Station& s : x.stations)
                CHECK(chordDeflection(*r.element(x.id), x, s)->magnitude <= *x.maxDeflection * (1 + 1e-12));
}

TEST_CASE("diagrammes : 9 grandeurs, stations, extrêmes, légende d'interpolation")
{
    const Results& r = example();
    const LoadCase& lc = r.loadCases[1];
    const auto diagrams = elementDiagrams(r, lc, lc.elements[0]);
    REQUIRE(diagrams.size() == 9);
    CHECK_EQ(diagrams[4].quantity, std::string("My"));
    CHECK_EQ(diagramStem(diagrams[4]), std::string("cas2_elem1_My"));
    const std::string svg = diagramSvg(diagrams[4]);
    CHECK(svg.rfind("<svg", 0) == 0);
    CHECK_EQ(countOf(svg, "class=\"station\""), std::size_t(11));
    CHECK_EQ(countOf(svg, "class=\"extreme\""), std::size_t(2));
    CHECK(contains(svg, "interpolation linéaire"));
    CHECK(contains(svg, "11 stations fournies"));
    // Le maximum annoté est la valeur du JSON (My = 33.76… à x = 0 pour le cas 2, élément 1).
    CHECK(contains(svg, "max = 33.76 (x = 0)"));
    const std::string multi = diagramSvg(diagrams[8]);  // rotations : 3 séries
    CHECK_EQ(countOf(multi, "class=\"station\""), std::size_t(33));
    CHECK_EQ(countOf(multi, "class=\"extreme\""), std::size_t(3));
}

TEST_CASE("note de calcul HTML : 15 sections et section OpenSees")
{
    const Results& r = example();
    ReportOptions opt;
    opt.inputFile = "portique_spatial_resultats.json";
    opt.generatedAt = "essai";
    const Report rep = buildReport(r, opt);
    const auto& titles = reportSectionTitles();
    REQUIRE(titles.size() == 15);
    for (std::size_t i = 0; i < 15; ++i) {
        const std::string n = std::to_string(i + 1);
        CHECK(contains(rep.html, "<h2 id=\"s" + n + "\">"));
        CHECK(contains(rep.html, n + ". " + titles[i]));
    }
    CHECK(contains(rep.html, "non fournie"));
    CHECK(contains(rep.html, "ne contient, pour chaque cas, que son identifiant et son nom"));
    CHECK(contains(rep.html, "interpolation linéaire"));
    CHECK(contains(rep.html, "blk-input") && contains(rep.html, "blk-hypothesis") && contains(rep.html, "blk-result") &&
          contains(rep.html, "blk-check"));
    CHECK_EQ(rep.diagrams.size(), std::size_t(2 * 8 * 9));
    CHECK_EQ(countOf(rep.html, "<svg"), std::size_t(2 * 8 * 9));

    ReportOptions withOs = opt;
    withOs.openSees = TextTable{{"Grandeur", "TSA", "OpenSees"}, {{"Uz nœud 13", "1", "1"}}};
    withOs.openSeesSource = "synthese.csv";
    const Report rep2 = buildReport(r, withOs);
    CHECK(!contains(rep2.html, "non fournie"));
    CHECK(contains(rep2.html, "synthese.csv"));
    CHECK(contains(rep2.html, "Uz nœud 13"));

    ReportOptions one = opt;
    one.caseId = 2;
    one.svgMode = SvgMode::ImageReference;
    const Report rep3 = buildReport(r, one);
    CHECK_EQ(rep3.diagrams.size(), std::size_t(8 * 9));
    CHECK_EQ(countOf(rep3.html, "<svg"), std::size_t(0));
    CHECK(contains(rep3.html, "<img src=\"diag_cas2_elem1_N.png\""));
    one.caseId = 42;
    CHECK_THROWS(buildReport(r, one), std::invalid_argument);
}

TEST_CASE("contrôles de cohérence et équilibre recalculé")
{
    QJsonObject root = exampleRoot();
    QJsonArray els = root["elements"].toArray();
    QJsonObject e = els[0].toObject();
    e["nodes"] = QJsonArray{1, 999};
    els[0] = e;
    root["elements"] = els;
    CHECK(contains(errorMessage(toBytes(root)), "nœud inconnu"));

    const Results& r = example();
    for (const LoadCase& lc : r.loadCases) {
        const Resultant rr = reactionResultant(r, lc);
        for (std::size_t k = 0; k < 3; ++k) {
            REQUIRE(rr.force[k] && rr.moment[k]);
            CHECK(std::fabs(*rr.force[k] - *lc.equilibrium.reactionForce[k]) < 1e-9);
            CHECK(std::fabs(*rr.moment[k] - *lc.equilibrium.reactionMoment[k]) < 1e-9);
        }
        const auto res = normOfSum(lc.equilibrium.appliedForce, lc.equilibrium.reactionForce);
        REQUIRE(res.has_value());
        CHECK(std::fabs(*res - *lc.equilibrium.forceResidual) < 1e-12);
    }
}

TEST_CASE("appuis élastiques : lecture facultative, tableau, équilibre recalculé")
{
    // Fichier sans ressort : champs absents acceptés, tableau vide
    CHECK(example().springs.empty());
    CHECK(springForceTable(example()).rows.empty());

    // Ressort ajouté sur le nœud 11 : sa force entre dans la résultante, comme dans le bloc « equilibrium » du moteur
    const std::array<double, 6> f{1.0, -2.0, 3.0, 0.5, -0.25, 0.125};
    const auto& p = example().node(11)->xyz;
    QJsonObject root = exampleRoot();
    root["springs"] = QJsonArray{QJsonObject{{"node", 11}, {"stiffness", QJsonArray{1e4, 0, 2e4, 0, 0, 5e3}}}};
    for (int c = 0; c < root["loadCases"].toArray().size(); ++c)
        root = editCase(root, c, [&](QJsonObject& lc) {
            lc["springForces"] = QJsonArray{QJsonObject{{"node", 11}, {"values", QJsonArray{f[0], f[1], f[2], f[3], f[4], f[5]}}}};
            QJsonObject e = lc["equilibrium"].toObject();
            QJsonArray rf = e["reactionForce"].toArray(), rm = e["reactionMoment"].toArray();
            const double m[3] = {p[1] * f[2] - p[2] * f[1] + f[3], p[2] * f[0] - p[0] * f[2] + f[4],
                                 p[0] * f[1] - p[1] * f[0] + f[5]};
            for (int k = 0; k < 3; ++k) {
                rf[k] = rf[k].toDouble() + f[static_cast<std::size_t>(k)];
                rm[k] = rm[k].toDouble() + m[k];
            }
            e["reactionForce"] = rf;
            e["reactionMoment"] = rm;
            lc["equilibrium"] = e;
        });
    const Results r = parseResults(toBytes(root));
    REQUIRE(r.springs.size() == 1);
    CHECK(*r.springs[0].stiffness[2] == 2e4);
    for (const LoadCase& lc : r.loadCases) {
        const Resultant rr = reactionResultant(r, lc);
        for (std::size_t k = 0; k < 3; ++k) {
            REQUIRE(rr.force[k] && rr.moment[k]);
            CHECK(std::fabs(*rr.force[k] - *lc.equilibrium.reactionForce[k]) < 1e-9);
            CHECK(std::fabs(*rr.moment[k] - *lc.equilibrium.reactionMoment[k]) < 1e-9);
        }
    }
    const Table t = filterByCase(springForceTable(r), 1);
    REQUIRE(t.rows.size() == 1);
    CHECK(cellNumber(t.rows[0][0]) == 11.0 && cellNumber(t.rows[0][4]) == 3.0);
    ReportOptions opt;
    opt.inputFile = "ressorts.json";
    opt.generatedAt = "essai";
    const Report rep = buildReport(r, opt);
    CHECK(contains(rep.html, "Appuis élastiques (ressorts découplés"));
    CHECK(contains(rep.html, "— ressorts</h3>"));
    CHECK(contains(rep.html, "somme des réactions des appuis rigides et des forces des ressorts"));

    // Force de ressort sur un nœud sans ressort : refusée
    QJsonObject bad = root;
    bad.remove("springs");
    CHECK(contains(errorMessage(toBytes(bad)), "nœud sans ressort"));
}

TEST_CASE("avertissements du moteur et résidu de résolution reproduits dans la note")
{
    const Results& r = example();
    REQUIRE(!r.loadCases.empty());
    CHECK(r.loadCases[0].equilibrium.solutionResidual.has_value());
    ReportOptions opt;
    opt.inputFile = "portique_spatial_resultats.json";
    opt.generatedAt = "essai";
    CHECK(contains(buildReport(r, opt).html, "Avertissements du moteur : aucun."));

    QJsonObject root = editCase(exampleRoot(), 0, [](QJsonObject& lc) {
        lc["warnings"] = QJsonArray{QStringLiteral("conditionnement faible (quasi-mécanisme possible) : essai")};
    });
    const Results w = parseResults(toBytes(root));
    REQUIRE(w.loadCases[0].warnings.size() == 1);
    const std::string html = buildReport(w, opt).html;
    CHECK(contains(html, "Avertissements du moteur</b>"));
    CHECK(contains(html, "quasi-mécanisme possible) : essai"));
}

TEST_CASE("lecture d'un fichier délimité (synthèse externe)")
{
    const auto rows = parseDelimited("\xEF\xBB\xBF" "a;\"b;c\";\"d\"\"e\"\r\n1;2;3\r\n\r\n", ';');
    REQUIRE(rows.size() == 2);
    CHECK_EQ(rows[0].size(), std::size_t(3));
    CHECK_EQ(rows[0][1], std::string("b;c"));
    CHECK_EQ(rows[0][2], std::string("d\"e"));
    CHECK_EQ(rows[1][2], std::string("3"));
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    return minitest::runAll();
}
