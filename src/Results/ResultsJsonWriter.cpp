#include "Fem3D/Results/ResultsJsonWriter.h"

#include "Fem3D/Core/Fem3DError.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace TSA::Fem3D
{

namespace
{
// Écriture JSON minimale (le moteur ne dépend d'aucune bibliothèque JSON) : nombres en notation exacte à 17 chiffres,
// valeurs absentes en null.
class Json
{
public:
    explicit Json(std::ostringstream& out)
        : m_out(out)
    {
        m_out << std::setprecision(17);
    }
    void string(const std::string& s)
    {
        m_out << '"';
        for (char c : s)
        {
            if (c == '"' || c == '\\') m_out << '\\' << c;
            else if (c == '\n') m_out << "\\n";
            else m_out << c;
        }
        m_out << '"';
    }
    void number(double v)
    {
        if (std::isfinite(v)) m_out << (v == 0.0 ? 0.0 : v);   // pas de « -0 »
        else m_out << "null";
    }
    void optional(const std::optional<double>& v)
    {
        if (v) number(*v);
        else m_out << "null";
    }
    template <typename Seq> void numbers(const Seq& seq)
    {
        m_out << '[';
        bool first = true;
        for (double v : seq)
        {
            if (!first) m_out << ',';
            number(v);
            first = false;
        }
        m_out << ']';
    }
    void key(const char* k)
    {
        string(k);
        m_out << ':';
    }
    std::ostringstream& raw() { return m_out; }

private:
    std::ostringstream& m_out;
};

std::array<double, 3> asArray(const Vector3& v) { return { v.x(), v.y(), v.z() }; }
std::array<double, 12> asArray(const Vector12& v)
{
    std::array<double, 12> a{};
    for (int k = 0; k < 12; ++k) a[static_cast<std::size_t>(k)] = v(k);
    return a;
}
} // namespace

std::string resultsToJson(const std::string& projectName, const StructuralModel3D& model, const AnalysisResults& results, const UnitSystem& units)
{
    std::ostringstream os;
    Json j(os);
    auto& o = j.raw();
    o << '{';
    j.key("format"); j.string(kResultsFormat); o << ',';
    j.key("engine"); j.string(results.engineVersion); o << ',';
    j.key("project"); j.string(projectName); o << ',';
    j.key("units");
    o << '{'; j.key("force"); j.string(units.force); o << ','; j.key("length"); j.string(units.length); o << ',';
    j.key("moment"); j.string(units.force + "." + units.length); o << ','; j.key("rotation"); j.string("rad"); o << ',';
    j.key("stress"); j.string(units.force + "/" + units.length + "2"); o << "},";
    j.key("conventions");
    o << '{';
    j.key("dofOrder"); o << R"(["Ux","Uy","Uz","Rx","Ry","Rz"])" << ',';
    j.key("localAxes"); j.string("x = i->j ; y = (vecxz x x)/|.| ; z = x x y (geomTransf OpenSees)"); o << ',';
    j.key("sectionForces"); j.string("face positive (normale +x local) ; N > 0 traction ; dMz/dx = -Vy ; dMy/dx = +Vz"); o << ',';
    j.key("endForces"); j.string("forces exercees SUR l'element aux noeuds i et j, repere local"); o << ',';
    j.key("reactions"); j.string("forces exercees PAR l'appui sur la structure, repere global"); o << ',';
    j.key("springForces"); j.string("-k.u : forces exercees PAR le ressort sur la structure, repere global");
    o << "},";

    j.key("nodes"); o << '[';
    for (std::size_t i = 0; i < model.nodes().size(); ++i)
    {
        const Node3D& n = model.nodes()[i];
        o << (i ? "," : "") << '{'; j.key("id"); o << n.id << ','; j.key("xyz"); j.numbers(asArray(n.position)); o << '}';
    }
    o << "],";

    j.key("elements"); o << '[';
    for (std::size_t i = 0; i < model.elements().size(); ++i)
    {
        const ElementDefinition& d = model.elements()[i];
        const auto element = model.createElement(d);
        const ElasticMaterial& m = element->material();
        const BeamSection3D& s = element->section();
        o << (i ? "," : "") << '{';
        j.key("id"); o << d.id << ',';
        j.key("nodes"); o << '[' << d.startNodeId << ',' << d.endNodeId << "],";
        j.key("formulation"); j.string(formulationName(d.formulation)); o << ',';
        j.key("releases"); o << '[' << (d.releases.momentYStart ? "true" : "false") << ',' << (d.releases.momentZStart ? "true" : "false") << ','
                              << (d.releases.momentYEnd ? "true" : "false") << ',' << (d.releases.momentZEnd ? "true" : "false") << "],";
        j.key("length"); j.number(element->length()); o << ',';
        j.key("localX"); j.numbers(asArray(element->localAxes().xAxis())); o << ',';
        j.key("localY"); j.numbers(asArray(element->localAxes().yAxis())); o << ',';
        j.key("localZ"); j.numbers(asArray(element->localAxes().zAxis())); o << ',';
        j.key("material"); o << '{'; j.key("name"); j.string(m.name); o << ','; j.key("E"); j.number(m.youngModulus); o << ',';
        j.key("G"); j.number(m.shearModulus); o << "},";
        j.key("section"); o << '{'; j.key("name"); j.string(s.name); o << ','; j.key("A"); j.number(s.area); o << ',';
        j.key("Iy"); j.number(s.inertiaY); o << ','; j.key("Iz"); j.number(s.inertiaZ); o << ','; j.key("J"); j.number(s.torsionConstant); o << ',';
        j.key("Asy"); j.optional(s.shearAreaY); o << ','; j.key("Asz"); j.optional(s.shearAreaZ); o << ',';
        j.key("yExtreme"); j.optional(s.yExtreme); o << ','; j.key("zExtreme"); j.optional(s.zExtreme); o << ',';
        j.key("Wt"); j.optional(s.torsionModulus); o << "}}";
    }
    o << "],";

    j.key("supports"); o << '[';
    bool first = true;
    for (const auto& [id, r] : model.restraints())
    {
        o << (first ? "" : ",") << '{'; j.key("node"); o << id << ','; j.key("fixed"); o << '[';
        for (int k = 0; k < kDofsPerNode; ++k) o << (k ? "," : "") << (r.fixed[static_cast<std::size_t>(k)] ? "true" : "false");
        o << "],"; j.key("prescribed"); j.numbers(r.prescribed); o << '}';
        first = false;
    }
    o << "],";

    j.key("springs"); o << '[';
    first = true;
    for (const auto& [id, s] : model.springs())
    {
        o << (first ? "" : ",") << '{'; j.key("node"); o << id << ','; j.key("stiffness"); j.numbers(s.stiffness); o << '}';
        first = false;
    }
    o << "],";

    j.key("loadCases"); o << '[';
    for (std::size_t c = 0; c < results.loadCases.size(); ++c)
    {
        const LoadCaseResults& lc = results.loadCases[c];
        o << (c ? "," : "") << '{';
        j.key("id"); o << lc.loadCaseId << ','; j.key("name"); j.string(lc.loadCaseName); o << ',';
        j.key("displacements"); o << '[';
        for (std::size_t i = 0; i < lc.nodes.size(); ++i)
        {
            o << (i ? "," : "") << '{'; j.key("node"); o << lc.nodes[i].nodeId << ','; j.key("values"); j.numbers(lc.nodes[i].displacement); o << '}';
        }
        o << "],"; j.key("reactions"); o << '[';
        for (std::size_t i = 0; i < lc.reactions.size(); ++i)
        {
            o << (i ? "," : "") << '{'; j.key("node"); o << lc.reactions[i].nodeId << ','; j.key("values"); j.numbers(lc.reactions[i].force); o << '}';
        }
        o << "],"; j.key("springForces"); o << '[';
        for (std::size_t i = 0; i < lc.springs.size(); ++i)
        {
            o << (i ? "," : "") << '{'; j.key("node"); o << lc.springs[i].nodeId << ','; j.key("values"); j.numbers(lc.springs[i].force); o << '}';
        }
        o << "],"; j.key("elements"); o << '[';
        for (std::size_t e = 0; e < lc.elements.size(); ++e)
        {
            const ElementResult& er = lc.elements[e];
            o << (e ? "," : "") << '{';
            j.key("id"); o << er.elementId << ',';
            j.key("endForcesLocal"); j.numbers(asArray(er.endForcesLocal)); o << ',';
            j.key("localDisplacements"); j.numbers(asArray(er.localDisplacements)); o << ',';
            j.key("maxDeflection"); j.number(er.maxTransverseDeflection); o << ',';
            j.key("maxDeflectionPosition"); j.number(er.maxTransverseDeflectionPosition); o << ',';
            j.key("stations"); o << '[';
            for (std::size_t k = 0; k < er.stations.size(); ++k)
            {
                const StationResult& s = er.stations[k];
                o << (k ? "," : "") << '{';
                j.key("x"); j.number(s.position); o << ',';
                j.key("forces"); j.numbers(std::array<double, 6>{ s.forces.axial, s.forces.shearY, s.forces.shearZ, s.forces.torsion, s.forces.momentY, s.forces.momentZ }); o << ',';
                j.key("localDisplacement"); j.numbers(asArray(s.localDisplacement)); o << ',';
                j.key("sectionRotation"); j.numbers(asArray(s.sectionRotation)); o << ',';
                j.key("slopes"); j.numbers(std::array<double, 2>{ s.slopeXY, s.slopeXZ }); o << ',';
                j.key("globalDisplacement"); j.numbers(asArray(s.globalDisplacement)); o << ',';
                j.key("stresses"); o << '{';
                j.key("axial"); j.number(s.stresses.axial); o << ','; j.key("bendingY"); j.optional(s.stresses.bendingY); o << ',';
                j.key("bendingZ"); j.optional(s.stresses.bendingZ); o << ','; j.key("extremeNormal"); j.optional(s.stresses.extremeNormal); o << ',';
                j.key("shearY"); j.number(s.stresses.shearY); o << ','; j.key("shearZ"); j.number(s.stresses.shearZ); o << ',';
                j.key("torsion"); j.optional(s.stresses.torsion); o << ','; j.key("vonMisesIndicative"); j.optional(s.stresses.vonMisesIndicative);
                o << "}}";
            }
            o << "]}";
        }
        o << "],"; j.key("equilibrium"); o << '{';
        j.key("appliedForce"); j.numbers(asArray(lc.equilibrium.appliedForce)); o << ',';
        j.key("appliedMoment"); j.numbers(asArray(lc.equilibrium.appliedMoment)); o << ',';
        j.key("reactionForce"); j.numbers(asArray(lc.equilibrium.reactionForce)); o << ',';
        j.key("reactionMoment"); j.numbers(asArray(lc.equilibrium.reactionMoment)); o << ',';
        j.key("forceResidual"); j.number(lc.equilibrium.forceResidual); o << ',';
        j.key("momentResidual"); j.number(lc.equilibrium.momentResidual); o << ',';
        j.key("solutionResidual"); j.number(lc.equilibrium.solutionResidual); o << "},";
        j.key("warnings"); o << '[';
        for (std::size_t w = 0; w < lc.warnings.size(); ++w)
        {
            o << (w ? "," : "");
            j.string(lc.warnings[w]);
        }
        o << "]}";
    }
    o << "]}";
    return os.str();
}

void writeResultsJson(const std::string& path, const std::string& projectName, const StructuralModel3D& model, const AnalysisResults& results,
                      const UnitSystem& units)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) throw Fem3DError(ErrorCode::IoError, "écriture impossible : " + path);
    out << resultsToJson(projectName, model, results, units);
    if (!out) throw Fem3DError(ErrorCode::IoError, "écriture interrompue : " + path);
}

} // namespace TSA::Fem3D
