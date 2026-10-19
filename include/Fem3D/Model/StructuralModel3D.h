#pragma once

// Modèle structurel : données saisies (nœuds, définitions d'éléments, matériaux, sections, appuis, cas de charge).
// Il ne contient aucune équation ; LinearStaticAnalysis en crée les éléments finis selon leur formulation.

#include "Fem3D/Constraints/BoundaryConditions.h"
#include "Fem3D/Elements/IBeamElement3D.h"
#include "Fem3D/Geometry/Node3D.h"
#include "Fem3D/Loads/Loads3D.h"
#include "Fem3D/Materials/ElasticMaterial.h"
#include "Fem3D/Sections/BeamSection3D.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace TSA::Fem3D
{

struct ElementDefinition
{
    int id = 0;
    int startNodeId = 0;
    int endNodeId = 0;
    std::string material;
    std::string section;
    BeamFormulation formulation = BeamFormulation::EulerBernoulli;
    std::optional<Vector3> vectorXZ;   ///< orientation de la section (repère local), défaut : LocalCoordinateSystem3D
    EndReleases releases;              ///< rotules de flexion aux extrémités (aucune par défaut)
};

class StructuralModel3D
{
public:
    void addMaterial(const ElasticMaterial& material);
    void addSection(const BeamSection3D& section);
    void addNode(int id, const Vector3& position);
    void addElement(const ElementDefinition& element);
    /// Remplace les conditions d'un nœud déjà restreint.
    void setRestraint(const NodeRestraint& restraint);
    /// Remplace l'appui élastique d'un nœud qui en a déjà un.
    void setSpring(const NodalSpring& spring);
    void addLoadCase(const LoadCase3D& loadCase);

    const std::vector<Node3D>& nodes() const { return m_nodes; }
    const std::vector<ElementDefinition>& elements() const { return m_elements; }
    const std::map<int, NodeRestraint>& restraints() const { return m_restraints; }
    const std::map<int, NodalSpring>& springs() const { return m_springs; }
    const std::vector<LoadCase3D>& loadCases() const { return m_loadCases; }
    const ElasticMaterial& material(const std::string& name) const;
    const BeamSection3D& section(const std::string& name) const;
    const Node3D& node(int id) const;
    const LoadCase3D& loadCase(int id) const;
    std::size_t nodeIndex(int id) const;

    /// Élément fini de la formulation choisie (repère local construit à partir des coordonnées réelles).
    std::unique_ptr<IBeamElement3D> createElement(const ElementDefinition& definition) const;

    /// Contrôle complet avant calcul : références, propriétés, géométrie, charges.
    /// @throws Fem3DError avec un message qui désigne l'objet fautif.
    void validate() const;

private:
    std::vector<Node3D> m_nodes;
    std::map<int, std::size_t> m_nodeIndex;
    std::vector<ElementDefinition> m_elements;
    std::map<std::string, ElasticMaterial> m_materials;
    std::map<std::string, BeamSection3D> m_sections;
    std::map<int, NodeRestraint> m_restraints;
    std::map<int, NodalSpring> m_springs;
    std::vector<LoadCase3D> m_loadCases;
};

} // namespace TSA::Fem3D
