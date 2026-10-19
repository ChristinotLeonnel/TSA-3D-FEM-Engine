#pragma once

// Données communes d'un élément poutre (identifiants, repère local, matériau, section). Les formulations dérivent de
// cette base et n'y ajoutent que leurs équations.

#include "Fem3D/Elements/IBeamElement3D.h"

namespace TSA::Fem3D
{

class BeamElementBase : public IBeamElement3D
{
public:
    BeamElementBase(int id, int startNodeId, int endNodeId, LocalCoordinateSystem3D axes, ElasticMaterial material, BeamSection3D section,
                    EndReleases releases = {})
        : m_id(id)
        , m_startNodeId(startNodeId)
        , m_endNodeId(endNodeId)
        , m_axes(std::move(axes))
        , m_material(std::move(material))
        , m_section(std::move(section))
        , m_releases(releases)
    {
    }

    int id() const override { return m_id; }
    int startNodeId() const override { return m_startNodeId; }
    int endNodeId() const override { return m_endNodeId; }
    const LocalCoordinateSystem3D& localAxes() const override { return m_axes; }
    const ElasticMaterial& material() const override { return m_material; }
    const BeamSection3D& section() const override { return m_section; }
    const EndReleases& releases() const override { return m_releases; }

protected:
    /// Termes communs aux deux formulations : traction-compression EA/L (DDL 0, 6) et torsion de Saint-Venant
    /// GJ/L (DDL 3, 9), sans gauchissement (pas de bimoment).
    void addAxialAndTorsion(Matrix12& k) const;

private:
    int m_id;
    int m_startNodeId;
    int m_endNodeId;
    LocalCoordinateSystem3D m_axes;
    ElasticMaterial m_material;
    BeamSection3D m_section;
    EndReleases m_releases;
};

} // namespace TSA::Fem3D
