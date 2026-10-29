#pragma once

// Résultats d'une analyse statique linéaire, par cas de charge (docs/theory/SIGN_CONVENTIONS.md §5).
// Valeurs non disponibles (fibre extrême, module de torsion non fournis) : std::nullopt, jamais une valeur inventée.

#include "Fem3D/Core/Types.h"
#include "Fem3D/Elements/IBeamElement3D.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace TSA::Fem3D
{

inline constexpr char kEngineVersion[] = "TSA Fem3D 0.3.0";

struct NodalResult
{
    int nodeId = 0;
    std::array<double, kDofsPerNode> displacement{};   ///< Ux Uy Uz [L], Rx Ry Rz [rad], repère global
};

struct ReactionResult
{
    int nodeId = 0;
    std::array<double, kDofsPerNode> force{};   ///< Fx Fy Fz [F], Mx My Mz [F·L], repère global, exercées par l'appui
};

struct SpringResult
{
    int nodeId = 0;
    std::array<double, kDofsPerNode> force{};   ///< −k · u : forces et moments exercés PAR le ressort sur la structure, repère global
};

/// Efforts internes sur la face positive de la coupe (normale +x local), repère local :
/// N > 0 traction ; dMz/dx = −Vy ; dMy/dx = +Vz.
struct SectionForces
{
    double axial = 0.0;        ///< N   [F]
    double shearY = 0.0;       ///< Vy  [F]
    double shearZ = 0.0;       ///< Vz  [F]
    double torsion = 0.0;      ///< T   [F·L]
    double momentY = 0.0;      ///< My  [F·L]
    double momentZ = 0.0;      ///< Mz  [F·L]
};

struct SectionStresses
{
    double axial = 0.0;                     ///< N / A
    std::optional<double> bendingY;         ///< |My| zExtreme / Iy
    std::optional<double> bendingZ;         ///< |Mz| yExtreme / Iz
    std::optional<double> extremeNormal;    ///< |N/A| + flexions (exact pour un rectangle, majorant sinon)
    double shearY = 0.0;                    ///< |Vy| / Asy (ou / A sans Asy) : contrainte de calcul (la moyenne vraie vaut |Vy| / A)
    double shearZ = 0.0;                    ///< |Vz| / Asz (ou / A)
    std::optional<double> torsion;          ///< |T| / Wt
    std::optional<double> vonMisesIndicative; ///< √(σ_extrême² + 3 τ_torsion²) : indicatif, non réglementaire
};

struct StationResult
{
    double position = 0.0;                 ///< abscisse locale x ∈ [0, L]
    SectionForces forces;
    Vector3 localDisplacement = Vector3::Zero();   ///< u v w (repère local)
    Vector3 sectionRotation = Vector3::Zero();     ///< θx θy θz (repère local)
    double slopeXY = 0.0;                  ///< dv/dx (= θz en Euler–Bernoulli)
    double slopeXZ = 0.0;                  ///< dw/dx (= −θy en Euler–Bernoulli)
    Vector3 globalDisplacement = Vector3::Zero();  ///< Ux Uy Uz du point de la ligne moyenne
    SectionStresses stresses;
};

struct ElementResult
{
    int elementId = 0;
    BeamFormulation formulation = BeamFormulation::EulerBernoulli;
    double length = 0.0;
    Vector12 localDisplacements = Vector12::Zero();
    Vector12 endForcesLocal = Vector12::Zero();   ///< forces exercées SUR l'élément aux nœuds i et j, repère local
    std::vector<StationResult> stations;
    double maxTransverseDeflection = 0.0;         ///< max √(v² + w²) relatif à la corde (flèche locale)
    double maxTransverseDeflectionPosition = 0.0;
};

struct EquilibriumCheck
{
    Vector3 appliedForce = Vector3::Zero();
    Vector3 appliedMoment = Vector3::Zero();     ///< par rapport à l'origine globale
    Vector3 reactionForce = Vector3::Zero();     ///< appuis rigides et ressorts
    Vector3 reactionMoment = Vector3::Zero();
    double forceResidual = 0.0;                  ///< ‖ΣF_appliquées + ΣR + ΣF_ressorts‖
    double momentResidual = 0.0;
    double solutionResidual = 0.0;               ///< ‖K_ff U_f − (F_f − K_fr U_r)‖ / ‖F_f − K_fr U_r‖ (précision de la résolution)
};

struct LoadCaseResults
{
    int loadCaseId = 0;
    std::string loadCaseName;
    std::vector<NodalResult> nodes;
    std::vector<ReactionResult> reactions;
    std::vector<SpringResult> springs;           ///< appuis élastiques (vide sans ressort)
    std::vector<ElementResult> elements;
    EquilibriumCheck equilibrium;
    std::vector<std::string> warnings;           ///< avertissements non bloquants (quasi-mécanisme, résidu élevé)
};

struct AnalysisResults
{
    std::string engineVersion = kEngineVersion;
    std::vector<LoadCaseResults> loadCases;
};

} // namespace TSA::Fem3D
