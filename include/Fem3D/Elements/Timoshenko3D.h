#pragma once

// Poutre 3D de Timoshenko (déformation de cisaillement transversal, aires effectives Asy, Asz).
// Élément exact à interpolation interdépendante : rigidité en forme fermée avec les paramètres
//   Φy = 12 E Iz / (G Asy L²)   (flexion dans le plan x-y)      Φz = 12 E Iy / (G Asz L²)   (plan x-z)
// Solution exacte de l'équation différentielle de Timoshenko pour une poutre prismatique : pas de verrouillage en
// cisaillement (Φ → 0 redonne Euler–Bernoulli), pas d'intégration réduite (docs/theory/THEORY.md §3).

#include "Fem3D/Elements/BeamElementBase.h"

namespace TSA::Fem3D
{

class Timoshenko3D final : public BeamElementBase
{
public:
    Timoshenko3D(int id, int startNodeId, int endNodeId, LocalCoordinateSystem3D axes, ElasticMaterial material, BeamSection3D section,
                 EndReleases releases = {});

    BeamFormulation formulation() const override { return BeamFormulation::Timoshenko; }
    Matrix12 localStiffnessMatrix() const override;
    BeamShapeValues shapeFunctions(double xi) const override;
    double shearFlexibilityY() const override;
    double shearFlexibilityZ() const override;

    /// Paramètres de cisaillement Φy (plan x-y) et Φz (plan x-z), sans dimension.
    double shearParameterY() const;
    double shearParameterZ() const;
};

} // namespace TSA::Fem3D
