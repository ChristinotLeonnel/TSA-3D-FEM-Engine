#pragma once

// Interface commune des éléments poutre-colonne 3D à deux nœuds et 12 DDL (docs/theory/THEORY.md).
// Une formulation fournit : sa matrice de rigidité locale, ses fonctions de forme (charges consistantes) et ses
// flexibilités de cisaillement (nulles pour Euler–Bernoulli). Le reste (repère, assemblage, efforts internes) est
// commun et ne dépend que de ces trois éléments.

#include "Fem3D/Core/Types.h"
#include "Fem3D/Elements/EndReleases.h"
#include "Fem3D/Geometry/LocalCoordinateSystem3D.h"
#include "Fem3D/Materials/ElasticMaterial.h"
#include "Fem3D/Sections/BeamSection3D.h"

#include <array>

namespace TSA::Fem3D
{

enum class BeamFormulation
{
    EulerBernoulli,
    Timoshenko
};

const char* formulationName(BeamFormulation formulation);

/// Valeurs des fonctions de forme en ξ = x / L.
struct BeamShapeValues
{
    std::array<double, 2> axial{};     ///< u(x)  = Σ axial[k]  · (u_i, u_j)[k]
    std::array<double, 4> bendingXY{}; ///< v(x)  = Σ bendingXY[k] · (v_i, θz_i, v_j, θz_j)[k]
    std::array<double, 4> bendingXZ{}; ///< w(x)  = Σ bendingXZ[k] · (w_i, θy_i, w_j, θy_j)[k]   (θy = −dw/dx)
};

class IBeamElement3D
{
public:
    virtual ~IBeamElement3D() = default;

    virtual BeamFormulation formulation() const = 0;

    /// Matrice de rigidité 12×12 dans le repère local (ordre des DDL : Core/Types.h).
    virtual Matrix12 localStiffnessMatrix() const = 0;
    /// Fonctions de forme exactes de la formulation en ξ ∈ [0, 1].
    virtual BeamShapeValues shapeFunctions(double xi) const = 0;
    /// Flexibilités de cisaillement 1 / (G Asy) et 1 / (G Asz) ; 0 pour Euler–Bernoulli.
    virtual double shearFlexibilityY() const = 0;
    virtual double shearFlexibilityZ() const = 0;

    virtual int id() const = 0;
    virtual int startNodeId() const = 0;
    virtual int endNodeId() const = 0;
    virtual const LocalCoordinateSystem3D& localAxes() const = 0;
    virtual const ElasticMaterial& material() const = 0;
    virtual const BeamSection3D& section() const = 0;
    /// Rotules de flexion aux extrémités (aucune par défaut).
    virtual const EndReleases& releases() const = 0;

    double length() const { return localAxes().length(); }
    /// Rigidité locale effective : celle de la formulation, condensée aux DDL relâchés s'il y a des rotules.
    Matrix12 effectiveLocalStiffnessMatrix() const;
    /// K_global = Tᵀ K_effective T (12×12).
    Matrix12 globalStiffnessMatrix() const;
};

} // namespace TSA::Fem3D
