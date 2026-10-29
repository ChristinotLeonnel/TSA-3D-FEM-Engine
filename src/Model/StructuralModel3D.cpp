#include "Fem3D/Model/StructuralModel3D.h"

#include "Fem3D/Core/Fem3DError.h"
#include "Fem3D/Elements/EulerBernoulli3D.h"
#include "Fem3D/Elements/Timoshenko3D.h"

#include <cmath>
#include <map>

namespace TSA::Fem3D
{

namespace
{
std::string elementLabel(int id) { return "élément " + std::to_string(id); }

bool isFinite(const Vector3& v) { return v.allFinite(); }

/// Position x d'une charge sur une barre de longueur l : refusée hors de [−t l, (1 + t) l] (Tolerance::relativeLoadPosition).
bool onMember(double x, double l)
{
    const double t = Tolerance::relativeLoadPosition * l;
    return std::isfinite(x) && x >= -t && x <= l + t;
}
} // namespace

void StructuralModel3D::addMaterial(const ElasticMaterial& material)
{
    material.validate();
    if (!m_materials.emplace(material.name, material).second)
        throw Fem3DError(ErrorCode::DuplicateId, "matériau « " + material.name + " » déjà défini");
}

void StructuralModel3D::addSection(const BeamSection3D& section)
{
    section.validate(/*requiresShearAreas=*/false);
    if (!m_sections.emplace(section.name, section).second)
        throw Fem3DError(ErrorCode::DuplicateId, "section « " + section.name + " » déjà définie");
}

void StructuralModel3D::addNode(int id, const Vector3& position)
{
    if (!isFinite(position)) throw Fem3DError(ErrorCode::InvalidInput, "coordonnées non finies (nœud " + std::to_string(id) + ")");
    if (!m_nodeIndex.emplace(id, m_nodes.size()).second)
        throw Fem3DError(ErrorCode::DuplicateId, "nœud " + std::to_string(id) + " déjà défini");
    m_nodes.push_back({ id, position });
}

void StructuralModel3D::addElement(const ElementDefinition& element)
{
    for (const auto& e : m_elements)
        if (e.id == element.id) throw Fem3DError(ErrorCode::DuplicateId, elementLabel(element.id) + " déjà défini");
    m_elements.push_back(element);
}

void StructuralModel3D::setRestraint(const NodeRestraint& restraint)
{
    m_restraints[restraint.nodeId] = restraint;
}

void StructuralModel3D::setSpring(const NodalSpring& spring)
{
    m_springs[spring.nodeId] = spring;
}

void StructuralModel3D::addLoadCase(const LoadCase3D& loadCase)
{
    for (const auto& c : m_loadCases)
        if (c.id == loadCase.id) throw Fem3DError(ErrorCode::DuplicateId, "cas de charge " + std::to_string(loadCase.id) + " déjà défini");
    m_loadCases.push_back(loadCase);
}

const ElasticMaterial& StructuralModel3D::material(const std::string& name) const
{
    const auto it = m_materials.find(name);
    if (it == m_materials.end()) throw Fem3DError(ErrorCode::UnknownReference, "matériau « " + name + " » inconnu");
    return it->second;
}

const BeamSection3D& StructuralModel3D::section(const std::string& name) const
{
    const auto it = m_sections.find(name);
    if (it == m_sections.end()) throw Fem3DError(ErrorCode::UnknownReference, "section « " + name + " » inconnue");
    return it->second;
}

std::size_t StructuralModel3D::nodeIndex(int id) const
{
    const auto it = m_nodeIndex.find(id);
    if (it == m_nodeIndex.end()) throw Fem3DError(ErrorCode::UnknownReference, "nœud " + std::to_string(id) + " inconnu");
    return it->second;
}

const Node3D& StructuralModel3D::node(int id) const { return m_nodes[nodeIndex(id)]; }

const LoadCase3D& StructuralModel3D::loadCase(int id) const
{
    for (const auto& c : m_loadCases)
        if (c.id == id) return c;
    throw Fem3DError(ErrorCode::UnknownReference, "cas de charge " + std::to_string(id) + " inconnu");
}

std::unique_ptr<IBeamElement3D> StructuralModel3D::createElement(const ElementDefinition& d) const
{
    if (d.startNodeId == d.endNodeId)
        throw Fem3DError(ErrorCode::DegenerateGeometry, elementLabel(d.id) + " : ses deux nœuds sont identiques");
    try
    {
        LocalCoordinateSystem3D axes(node(d.startNodeId).position, node(d.endNodeId).position, d.vectorXZ);
        if (d.formulation == BeamFormulation::Timoshenko)
            return std::make_unique<Timoshenko3D>(d.id, d.startNodeId, d.endNodeId, axes, material(d.material), section(d.section), d.releases);
        return std::make_unique<EulerBernoulli3D>(d.id, d.startNodeId, d.endNodeId, axes, material(d.material), section(d.section), d.releases);
    }
    catch (const Fem3DError& e)
    {
        throw Fem3DError(e.code(), elementLabel(d.id) + " : " + e.what());
    }
}

void StructuralModel3D::validate() const
{
    if (m_nodes.empty()) throw Fem3DError(ErrorCode::InvalidInput, "modèle sans nœud");
    if (m_elements.empty()) throw Fem3DError(ErrorCode::InvalidInput, "modèle sans élément");
    std::map<int, double> elementLength;
    for (const auto& e : m_elements)
        elementLength[e.id] = createElement(e)->length();   // références, géométrie, propriétés de la formulation
    for (const auto& [id, r] : m_restraints)
    {
        nodeIndex(id);
        for (double v : r.prescribed)
            if (!std::isfinite(v)) throw Fem3DError(ErrorCode::InvalidInput, "déplacement imposé non fini (nœud " + std::to_string(id) + ")");
    }
    for (const auto& [id, s] : m_springs)
    {
        nodeIndex(id);
        const auto restraint = m_restraints.find(id);
        for (int k = 0; k < kDofsPerNode; ++k)
        {
            const double value = s.stiffness[static_cast<std::size_t>(k)];
            const std::string where = " (nœud " + std::to_string(id) + ", " + kDofNames[static_cast<std::size_t>(k)] + ")";
            if (!std::isfinite(value) || value < 0.0) throw Fem3DError(ErrorCode::InvalidInput, "raideur de ressort négative ou non finie" + where);
            if (value > 0.0 && restraint != m_restraints.end() && restraint->second.fixed[static_cast<std::size_t>(k)])
                throw Fem3DError(ErrorCode::InvalidInput, "ressort sur un DDL bloqué" + where);
        }
    }
    for (const auto& c : m_loadCases)
    {
        for (const auto& l : c.nodalLoads)
        {
            nodeIndex(l.nodeId);
            if (!isFinite(l.force) || !isFinite(l.moment))
                throw Fem3DError(ErrorCode::InvalidInput, "charge nodale non finie (cas " + std::to_string(c.id) + ", nœud " + std::to_string(l.nodeId) + ")");
        }
        // Charges sur barre : barre connue, valeurs finies, positions sur la barre, intervalle non vide
        auto where = [&](int elementId) { return " (cas " + std::to_string(c.id) + ", " + elementLabel(elementId) + ")"; };
        auto lengthOf = [&](const char* what, int elementId) {
            const auto it = elementLength.find(elementId);
            if (it == elementLength.end())
                throw Fem3DError(ErrorCode::UnknownReference, std::string(what) + " sur un " + elementLabel(elementId) + " inconnu (cas " + std::to_string(c.id) + ")");
            return it->second;
        };
        auto checkInterval = [&](const char* what, int elementId, double a, const std::optional<double>& end) {
            const double l = lengthOf(what, elementId), b = end.value_or(l);
            if (!onMember(a, l) || !onMember(b, l))
                throw Fem3DError(ErrorCode::InvalidInput, std::string(what) + " hors de la barre : [" + std::to_string(a) + ", " + std::to_string(b)
                                                              + "] pour une longueur " + std::to_string(l) + where(elementId));
            if (b - a <= Tolerance::relativeLoadPosition * l)
                throw Fem3DError(ErrorCode::InvalidInput, std::string(what) + " sur un intervalle vide ou inversé : [" + std::to_string(a) + ", "
                                                              + std::to_string(b) + "]" + where(elementId));
        };
        for (const auto& l : c.distributedLoads)
        {
            checkInterval("charge répartie", l.elementId, l.startPosition, l.endPosition);
            if (!isFinite(l.startIntensity) || !isFinite(l.endIntensity))
                throw Fem3DError(ErrorCode::InvalidInput, "charge répartie non finie" + where(l.elementId));
        }
        for (const auto& l : c.distributedMoments)
        {
            checkInterval("moment réparti", l.elementId, l.startPosition, l.endPosition);
            if (!isFinite(l.startIntensity) || !isFinite(l.endIntensity))
                throw Fem3DError(ErrorCode::InvalidInput, "moment réparti non fini" + where(l.elementId));
        }
        for (const auto& l : c.pointLoads)
        {
            const double length = lengthOf("charge ponctuelle", l.elementId);
            if (!onMember(l.position, length))
                throw Fem3DError(ErrorCode::InvalidInput, "charge ponctuelle hors de la barre : position " + std::to_string(l.position) + " pour une longueur "
                                                              + std::to_string(length) + where(l.elementId));
            if (!isFinite(l.force)) throw Fem3DError(ErrorCode::InvalidInput, "charge ponctuelle non finie" + where(l.elementId));
        }
    }
}

} // namespace TSA::Fem3D
