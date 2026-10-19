#pragma once

// Poutre 3D d'Euler–Bernoulli (sections planes et normales à la ligne moyenne : cisaillement transversal négligé).
// Rigidité exacte pour une poutre prismatique élastique ; fonctions de forme cubiques d'Hermite (docs/theory/THEORY.md §2).

#include "Fem3D/Elements/BeamElementBase.h"

namespace TSA::Fem3D
{

class EulerBernoulli3D final : public BeamElementBase
{
public:
    using BeamElementBase::BeamElementBase;

    BeamFormulation formulation() const override { return BeamFormulation::EulerBernoulli; }
    Matrix12 localStiffnessMatrix() const override;
    BeamShapeValues shapeFunctions(double xi) const override;
    double shearFlexibilityY() const override { return 0.0; }
    double shearFlexibilityZ() const override { return 0.0; }
};

} // namespace TSA::Fem3D
