// Lecture validée du JSON « tsa-fem3d-results/1 » (QtCore uniquement).

#include "fem3dpost/ResultsReader.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace fem3dpost {

// ---------------------------------------------------------------------------
// Recherches dans le modèle
// ---------------------------------------------------------------------------
namespace {
template <class Vec, class Pred>
auto findIn(const Vec& v, Pred pred) -> decltype(&v.front())
{
    for (const auto& item : v)
        if (pred(item))
            return &item;
    return nullptr;
}
} // namespace

const NodalValues* LoadCase::displacementOf(int node) const
{
    return findIn(displacements, [node](const NodalValues& n) { return n.node == node; });
}
const NodalValues* LoadCase::reactionOf(int node) const
{
    return findIn(reactions, [node](const NodalValues& n) { return n.node == node; });
}
const NodalValues* LoadCase::springForceOf(int node) const
{
    return findIn(springForces, [node](const NodalValues& n) { return n.node == node; });
}
const ElementResult* LoadCase::elementResult(int elementId) const
{
    return findIn(elements, [elementId](const ElementResult& e) { return e.id == elementId; });
}
const Node* Results::node(int id) const
{
    return findIn(nodes, [id](const Node& n) { return n.id == id; });
}
const Element* Results::element(int id) const
{
    return findIn(elements, [id](const Element& e) { return e.id == id; });
}
const Support* Results::support(int nodeId) const
{
    return findIn(supports, [nodeId](const Support& s) { return s.node == nodeId; });
}
const LoadCase* Results::loadCase(int id) const
{
    return findIn(loadCases, [id](const LoadCase& c) { return c.id == id; });
}

// ---------------------------------------------------------------------------
// Aides de lecture : chaque fonction connaît le chemin du champ pour un message explicite.
// ---------------------------------------------------------------------------
namespace {

[[noreturn]] void fail(const std::string& path, const std::string& message)
{
    throw FormatError(path + " : " + message);
}

std::string sub(const std::string& path, const char* key)
{
    return path.empty() ? std::string(key) : path + "." + key;
}

std::string at(const std::string& path, qsizetype i)
{
    return path + "[" + std::to_string(i) + "]";
}

QJsonValue member(const QJsonObject& o, const char* key, const std::string& path)
{
    if (!o.contains(QLatin1String(key)))
        fail(sub(path, key), "champ obligatoire absent");
    return o.value(QLatin1String(key));
}

QJsonObject asObject(const QJsonValue& v, const std::string& path)
{
    if (!v.isObject())
        fail(path, "objet JSON attendu");
    return v.toObject();
}

QJsonArray asArray(const QJsonValue& v, const std::string& path, qsizetype expectedSize = -1)
{
    if (!v.isArray())
        fail(path, "tableau JSON attendu");
    QJsonArray a = v.toArray();
    if (expectedSize >= 0 && a.size() != expectedSize)
        fail(path, "tableau de " + std::to_string(expectedSize) + " valeurs attendu, " +
                       std::to_string(a.size()) + " trouvées");
    return a;
}

double asNumber(const QJsonValue& v, const std::string& path)
{
    if (v.isNull())
        fail(path, "valeur obligatoire, null non admis");
    if (!v.isDouble())
        fail(path, "nombre attendu");
    return v.toDouble();
}

// null → indisponible ; tout autre type qu'un nombre est une erreur.
OptDouble asOptNumber(const QJsonValue& v, const std::string& path)
{
    if (v.isNull())
        return std::nullopt;
    if (!v.isDouble())
        fail(path, "nombre ou null attendu");
    return v.toDouble();
}

int asInt(const QJsonValue& v, const std::string& path)
{
    const double d = asNumber(v, path);
    if (std::floor(d) != d || d < std::numeric_limits<int>::min() || d > std::numeric_limits<int>::max())
        fail(path, "entier attendu");
    return static_cast<int>(d);
}

std::string asString(const QJsonValue& v, const std::string& path)
{
    if (!v.isString())
        fail(path, "chaîne attendue");
    return v.toString().toStdString();
}

// Chaîne facultative en valeur (null → chaîne vide) mais clé obligatoire.
std::string asOptString(const QJsonValue& v, const std::string& path)
{
    if (v.isNull())
        return {};
    return asString(v, path);
}

bool asBool(const QJsonValue& v, const std::string& path)
{
    if (!v.isBool())
        fail(path, "booléen attendu");
    return v.toBool();
}

template <std::size_t N>
std::array<double, N> numArray(const QJsonValue& v, const std::string& path)
{
    const QJsonArray a = asArray(v, path, static_cast<qsizetype>(N));
    std::array<double, N> out{};
    for (std::size_t i = 0; i < N; ++i)
        out[i] = asNumber(a[static_cast<qsizetype>(i)], at(path, static_cast<qsizetype>(i)));
    return out;
}

// Tableau de résultats : le tableau entier peut valoir null (tout indisponible),
// chaque composante peut valoir null.
template <std::size_t N>
std::array<OptDouble, N> optArray(const QJsonValue& v, const std::string& path)
{
    std::array<OptDouble, N> out{};
    if (v.isNull())
        return out;
    const QJsonArray a = asArray(v, path, static_cast<qsizetype>(N));
    for (std::size_t i = 0; i < N; ++i)
        out[i] = asOptNumber(a[static_cast<qsizetype>(i)], at(path, static_cast<qsizetype>(i)));
    return out;
}

Material readMaterial(const QJsonValue& v, const std::string& path)
{
    const QJsonObject o = asObject(v, path);
    Material m;
    m.name = asOptString(member(o, "name", path), sub(path, "name"));
    m.E = asOptNumber(member(o, "E", path), sub(path, "E"));
    m.G = asOptNumber(member(o, "G", path), sub(path, "G"));
    return m;
}

Section readSection(const QJsonValue& v, const std::string& path)
{
    const QJsonObject o = asObject(v, path);
    Section s;
    s.name = asOptString(member(o, "name", path), sub(path, "name"));
    auto num = [&](const char* key) { return asOptNumber(member(o, key, path), sub(path, key)); };
    s.A = num("A");
    s.Iy = num("Iy");
    s.Iz = num("Iz");
    s.J = num("J");
    s.Asy = num("Asy");
    s.Asz = num("Asz");
    s.yExtreme = num("yExtreme");
    s.zExtreme = num("zExtreme");
    s.Wt = num("Wt");
    return s;
}

Stresses readStresses(const QJsonValue& v, const std::string& path)
{
    Stresses s;
    if (v.isNull())  // donnée de section manquante : toutes les contraintes indisponibles
        return s;
    const QJsonObject o = asObject(v, path);
    auto num = [&](const char* key) { return asOptNumber(member(o, key, path), sub(path, key)); };
    s.axial = num("axial");
    s.bendingY = num("bendingY");
    s.bendingZ = num("bendingZ");
    s.extremeNormal = num("extremeNormal");
    s.shearY = num("shearY");
    s.shearZ = num("shearZ");
    s.torsion = num("torsion");
    s.vonMisesIndicative = num("vonMisesIndicative");
    return s;
}

std::vector<NodalValues> readNodalList(const QJsonValue& v, const std::string& path)
{
    const QJsonArray a = asArray(v, path);
    std::vector<NodalValues> out;
    out.reserve(static_cast<std::size_t>(a.size()));
    for (qsizetype i = 0; i < a.size(); ++i) {
        const std::string p = at(path, i);
        const QJsonObject o = asObject(a[i], p);
        NodalValues n;
        n.node = asInt(member(o, "node", p), sub(p, "node"));
        n.values = optArray<6>(member(o, "values", p), sub(p, "values"));
        out.push_back(n);
    }
    return out;
}

Station readStation(const QJsonValue& v, const std::string& path)
{
    const QJsonObject o = asObject(v, path);
    Station s;
    s.x = asNumber(member(o, "x", path), sub(path, "x"));
    s.forces = optArray<6>(member(o, "forces", path), sub(path, "forces"));
    s.localDisplacement = optArray<3>(member(o, "localDisplacement", path), sub(path, "localDisplacement"));
    s.sectionRotation = optArray<3>(member(o, "sectionRotation", path), sub(path, "sectionRotation"));
    s.slopes = optArray<2>(member(o, "slopes", path), sub(path, "slopes"));
    s.globalDisplacement = optArray<3>(member(o, "globalDisplacement", path), sub(path, "globalDisplacement"));
    s.stresses = readStresses(member(o, "stresses", path), sub(path, "stresses"));
    return s;
}

ElementResult readElementResult(const QJsonValue& v, const std::string& path)
{
    const QJsonObject o = asObject(v, path);
    ElementResult r;
    r.id = asInt(member(o, "id", path), sub(path, "id"));
    r.endForcesLocal = optArray<12>(member(o, "endForcesLocal", path), sub(path, "endForcesLocal"));
    r.localDisplacements = optArray<12>(member(o, "localDisplacements", path), sub(path, "localDisplacements"));
    r.maxDeflection = asOptNumber(member(o, "maxDeflection", path), sub(path, "maxDeflection"));
    r.maxDeflectionPosition =
        asOptNumber(member(o, "maxDeflectionPosition", path), sub(path, "maxDeflectionPosition"));
    const std::string sp = sub(path, "stations");
    const QJsonArray st = asArray(member(o, "stations", path), sp);
    for (qsizetype i = 0; i < st.size(); ++i)
        r.stations.push_back(readStation(st[i], at(sp, i)));
    return r;
}

Equilibrium readEquilibrium(const QJsonValue& v, const std::string& path)
{
    const QJsonObject o = asObject(v, path);
    Equilibrium e;
    e.appliedForce = optArray<3>(member(o, "appliedForce", path), sub(path, "appliedForce"));
    e.appliedMoment = optArray<3>(member(o, "appliedMoment", path), sub(path, "appliedMoment"));
    e.reactionForce = optArray<3>(member(o, "reactionForce", path), sub(path, "reactionForce"));
    e.reactionMoment = optArray<3>(member(o, "reactionMoment", path), sub(path, "reactionMoment"));
    e.forceResidual = asOptNumber(member(o, "forceResidual", path), sub(path, "forceResidual"));
    e.momentResidual = asOptNumber(member(o, "momentResidual", path), sub(path, "momentResidual"));
    if (o.contains(QLatin1String("solutionResidual")))
        e.solutionResidual = asOptNumber(member(o, "solutionResidual", path), sub(path, "solutionResidual"));
    return e;
}

// Contrôles de cohérence entre les différentes parties du fichier.
void crossValidate(const Results& r)
{
    std::set<int> nodeIds, elementIds, caseIds, supportIds, springIds;
    for (std::size_t i = 0; i < r.nodes.size(); ++i)
        if (!nodeIds.insert(r.nodes[i].id).second)
            fail(at("nodes", static_cast<qsizetype>(i)), "identifiant de nœud en double (" +
                                                             std::to_string(r.nodes[i].id) + ")");
    for (std::size_t i = 0; i < r.elements.size(); ++i) {
        const Element& e = r.elements[i];
        const std::string p = at("elements", static_cast<qsizetype>(i));
        if (!elementIds.insert(e.id).second)
            fail(p, "identifiant d'élément en double (" + std::to_string(e.id) + ")");
        if (!nodeIds.count(e.nodeI) || !nodeIds.count(e.nodeJ))
            fail(sub(p, "nodes"), "nœud inconnu dans la connectivité de l'élément " + std::to_string(e.id));
        if (!(e.length > 0.0))
            fail(sub(p, "length"), "longueur strictement positive attendue");
    }
    for (std::size_t i = 0; i < r.supports.size(); ++i) {
        const std::string p = at("supports", static_cast<qsizetype>(i));
        if (!nodeIds.count(r.supports[i].node))
            fail(sub(p, "node"), "appui sur un nœud inconnu (" + std::to_string(r.supports[i].node) + ")");
        if (!supportIds.insert(r.supports[i].node).second)
            fail(sub(p, "node"), "appui en double sur le nœud " + std::to_string(r.supports[i].node));
    }
    for (std::size_t i = 0; i < r.springs.size(); ++i) {
        const std::string p = at("springs", static_cast<qsizetype>(i));
        if (!nodeIds.count(r.springs[i].node))
            fail(sub(p, "node"), "ressort sur un nœud inconnu (" + std::to_string(r.springs[i].node) + ")");
        if (!springIds.insert(r.springs[i].node).second)
            fail(sub(p, "node"), "ressort en double sur le nœud " + std::to_string(r.springs[i].node));
    }
    for (std::size_t c = 0; c < r.loadCases.size(); ++c) {
        const LoadCase& lc = r.loadCases[c];
        const std::string p = at("loadCases", static_cast<qsizetype>(c));
        if (!caseIds.insert(lc.id).second)
            fail(sub(p, "id"), "identifiant de cas en double (" + std::to_string(lc.id) + ")");
        for (std::size_t i = 0; i < lc.displacements.size(); ++i)
            if (!nodeIds.count(lc.displacements[i].node))
                fail(at(sub(p, "displacements"), static_cast<qsizetype>(i)), "nœud inconnu");
        for (std::size_t i = 0; i < lc.reactions.size(); ++i)
            if (!nodeIds.count(lc.reactions[i].node))
                fail(at(sub(p, "reactions"), static_cast<qsizetype>(i)), "nœud inconnu");
        for (std::size_t i = 0; i < lc.springForces.size(); ++i)
            if (!springIds.count(lc.springForces[i].node))
                fail(at(sub(p, "springForces"), static_cast<qsizetype>(i)), "force de ressort sur un nœud sans ressort");
        std::set<int> seen;
        for (std::size_t i = 0; i < lc.elements.size(); ++i) {
            const ElementResult& er = lc.elements[i];
            const std::string pe = at(sub(p, "elements"), static_cast<qsizetype>(i));
            const Element* e = r.element(er.id);
            if (!e)
                fail(sub(pe, "id"), "élément inconnu (" + std::to_string(er.id) + ")");
            if (!seen.insert(er.id).second)
                fail(sub(pe, "id"), "résultats en double pour l'élément " + std::to_string(er.id));
            if (er.stations.size() < 2)
                fail(sub(pe, "stations"), "au moins 2 stations attendues");
            const double tol = 1e-9 * std::max(1.0, e->length);
            double previous = -std::numeric_limits<double>::infinity();
            for (std::size_t s = 0; s < er.stations.size(); ++s) {
                const double x = er.stations[s].x;
                const std::string ps = sub(at(sub(pe, "stations"), static_cast<qsizetype>(s)), "x");
                if (x < -tol || x > e->length + tol)
                    fail(ps, "abscisse hors de l'élément (0 ≤ x ≤ L attendu)");
                if (!(x > previous))
                    fail(ps, "abscisses strictement croissantes attendues");
                previous = x;
            }
        }
    }
}

} // namespace

Results parseResults(const QByteArray& json)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError)
        throw FormatError("JSON invalide : " + err.errorString().toStdString() + " (position " +
                          std::to_string(err.offset) + ")");
    if (!doc.isObject())
        throw FormatError("JSON invalide : un objet racine est attendu");
    const QJsonObject root = doc.object();

    Results r;
    // Le format est contrôlé en premier : tout autre identifiant est refusé.
    if (!root.contains(QLatin1String("format")))
        fail("format", "champ obligatoire absent (« " + std::string(kExpectedFormat) + " » attendu)");
    const QJsonValue fv = root.value(QLatin1String("format"));
    if (!fv.isString() || fv.toString() != QLatin1String(kExpectedFormat))
        throw FormatError("format « " + (fv.isString() ? fv.toString().toStdString() : std::string("?")) +
                          " » non pris en charge : « " + kExpectedFormat + " » exigé");
    r.format = kExpectedFormat;
    r.engine = asOptString(member(root, "engine", ""), "engine");
    r.project = asOptString(member(root, "project", ""), "project");

    {
        const QJsonObject u = asObject(member(root, "units", ""), "units");
        r.units.force = asString(member(u, "force", "units"), "units.force");
        r.units.length = asString(member(u, "length", "units"), "units.length");
        r.units.moment = asString(member(u, "moment", "units"), "units.moment");
        r.units.rotation = asString(member(u, "rotation", "units"), "units.rotation");
        r.units.stress = asString(member(u, "stress", "units"), "units.stress");
    }
    {
        const QJsonObject c = asObject(member(root, "conventions", ""), "conventions");
        const QJsonArray dof = asArray(member(c, "dofOrder", "conventions"), "conventions.dofOrder", 6);
        for (qsizetype i = 0; i < dof.size(); ++i)
            r.conventions.dofOrder.push_back(asString(dof[i], at("conventions.dofOrder", i)));
        r.conventions.localAxes = asString(member(c, "localAxes", "conventions"), "conventions.localAxes");
        r.conventions.sectionForces =
            asString(member(c, "sectionForces", "conventions"), "conventions.sectionForces");
        r.conventions.endForces = asString(member(c, "endForces", "conventions"), "conventions.endForces");
        r.conventions.reactions = asString(member(c, "reactions", "conventions"), "conventions.reactions");
        if (c.contains(QLatin1String("springForces")))
            r.conventions.springForces =
                asString(member(c, "springForces", "conventions"), "conventions.springForces");
    }
    {
        const QJsonArray a = asArray(member(root, "nodes", ""), "nodes");
        for (qsizetype i = 0; i < a.size(); ++i) {
            const std::string p = at("nodes", i);
            const QJsonObject o = asObject(a[i], p);
            Node n;
            n.id = asInt(member(o, "id", p), sub(p, "id"));
            n.xyz = numArray<3>(member(o, "xyz", p), sub(p, "xyz"));
            r.nodes.push_back(n);
        }
    }
    {
        const QJsonArray a = asArray(member(root, "elements", ""), "elements");
        for (qsizetype i = 0; i < a.size(); ++i) {
            const std::string p = at("elements", i);
            const QJsonObject o = asObject(a[i], p);
            Element e;
            e.id = asInt(member(o, "id", p), sub(p, "id"));
            const QJsonArray nn = asArray(member(o, "nodes", p), sub(p, "nodes"), 2);
            e.nodeI = asInt(nn[0], at(sub(p, "nodes"), 0));
            e.nodeJ = asInt(nn[1], at(sub(p, "nodes"), 1));
            e.formulation = asString(member(o, "formulation", p), sub(p, "formulation"));
            e.length = asNumber(member(o, "length", p), sub(p, "length"));
            e.localX = numArray<3>(member(o, "localX", p), sub(p, "localX"));
            e.localY = numArray<3>(member(o, "localY", p), sub(p, "localY"));
            e.localZ = numArray<3>(member(o, "localZ", p), sub(p, "localZ"));
            e.material = readMaterial(member(o, "material", p), sub(p, "material"));
            e.section = readSection(member(o, "section", p), sub(p, "section"));
            r.elements.push_back(e);
        }
    }
    {
        const QJsonArray a = asArray(member(root, "supports", ""), "supports");
        for (qsizetype i = 0; i < a.size(); ++i) {
            const std::string p = at("supports", i);
            const QJsonObject o = asObject(a[i], p);
            Support s;
            s.node = asInt(member(o, "node", p), sub(p, "node"));
            const QJsonArray f = asArray(member(o, "fixed", p), sub(p, "fixed"), 6);
            for (qsizetype k = 0; k < 6; ++k)
                s.fixed[static_cast<std::size_t>(k)] = asBool(f[k], at(sub(p, "fixed"), k));
            s.prescribed = optArray<6>(member(o, "prescribed", p), sub(p, "prescribed"));
            r.supports.push_back(s);
        }
    }
    if (root.contains(QLatin1String("springs"))) {
        const QJsonArray a = asArray(member(root, "springs", ""), "springs");
        for (qsizetype i = 0; i < a.size(); ++i) {
            const std::string p = at("springs", i);
            const QJsonObject o = asObject(a[i], p);
            Spring s;
            s.node = asInt(member(o, "node", p), sub(p, "node"));
            s.stiffness = optArray<6>(member(o, "stiffness", p), sub(p, "stiffness"));
            r.springs.push_back(s);
        }
    }
    {
        const QJsonArray a = asArray(member(root, "loadCases", ""), "loadCases");
        for (qsizetype i = 0; i < a.size(); ++i) {
            const std::string p = at("loadCases", i);
            const QJsonObject o = asObject(a[i], p);
            LoadCase lc;
            lc.id = asInt(member(o, "id", p), sub(p, "id"));
            lc.name = asOptString(member(o, "name", p), sub(p, "name"));
            lc.displacements = readNodalList(member(o, "displacements", p), sub(p, "displacements"));
            lc.reactions = readNodalList(member(o, "reactions", p), sub(p, "reactions"));
            if (o.contains(QLatin1String("springForces")))
                lc.springForces = readNodalList(member(o, "springForces", p), sub(p, "springForces"));
            const std::string pe = sub(p, "elements");
            const QJsonArray els = asArray(member(o, "elements", p), pe);
            for (qsizetype k = 0; k < els.size(); ++k)
                lc.elements.push_back(readElementResult(els[k], at(pe, k)));
            lc.equilibrium = readEquilibrium(member(o, "equilibrium", p), sub(p, "equilibrium"));
            if (o.contains(QLatin1String("warnings"))) {
                const QJsonArray w = asArray(member(o, "warnings", p), sub(p, "warnings"));
                for (qsizetype k = 0; k < w.size(); ++k)
                    lc.warnings.push_back(asString(w[k], at(sub(p, "warnings"), k)));
            }
            r.loadCases.push_back(std::move(lc));
        }
    }
    crossValidate(r);
    return r;
}

Results readResultsFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw FormatError("impossible d'ouvrir « " + path.toStdString() + " » : " + f.errorString().toStdString());
    return parseResults(f.readAll());
}

} // namespace fem3dpost
