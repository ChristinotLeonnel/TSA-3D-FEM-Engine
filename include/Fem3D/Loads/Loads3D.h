#pragma once

// Chargements (docs/theory/SIGN_CONVENTIONS.md §4). Unités cohérentes avec le modèle :
//   force [F], moment [F·L], charge répartie [F/L] et moment réparti [F·L/L] (par unité de longueur réelle de
//   l'élément), positions sur la barre [L] mesurées depuis le nœud de début i.

#include "Fem3D/Core/Types.h"

#include <optional>
#include <string>
#include <vector>

namespace TSA::Fem3D
{

/// Force et moment appliqués à un nœud, dans le repère global.
struct NodalLoad3D
{
    int nodeId = 0;
    Vector3 force = Vector3::Zero();    ///< Fx, Fy, Fz
    Vector3 moment = Vector3::Zero();   ///< Mx, My, Mz (règle de la main droite)
};

enum class LoadFrame
{
    Local,   ///< composantes (qx, qy, qz) dans le repère local de l'élément (qx : axiale)
    Global   ///< composantes (qX, qY, qZ) dans le repère global, projetées sur le repère local
};

/// Charge répartie linéaire entre q(a) = startIntensity et q(b) = endIntensity (uniforme si les deux sont égales),
/// sur [a, b] = [startPosition, endPosition] ; par défaut toute la longueur [0, L]. Jamais remplacée par une
/// approximation nodale : ses forces nodales équivalentes sont celles de la formulation, et elle est reprise exactement
/// dans les efforts internes et la déformée.
struct DistributedLoad3D
{
    int elementId = 0;
    LoadFrame frame = LoadFrame::Local;
    Vector3 startIntensity = Vector3::Zero();
    Vector3 endIntensity = Vector3::Zero();
    double startPosition = 0.0;                 ///< a, depuis le nœud i
    std::optional<double> endPosition;          ///< b, depuis le nœud i ; absent : L
};

/// Force concentrée sur une barre, à la distance `position` du nœud i (0 ≤ position ≤ L).
struct PointLoad3D
{
    int elementId = 0;
    LoadFrame frame = LoadFrame::Local;
    double position = 0.0;
    Vector3 force = Vector3::Zero();
};

/// Moment réparti (couple par unité de longueur, règle de la main droite) : composante x = torsion répartie,
/// y et z = couples de flexion répartis ; linéaire entre c(a) = startIntensity et c(b) = endIntensity sur [a, b].
struct DistributedMoment3D
{
    int elementId = 0;
    LoadFrame frame = LoadFrame::Local;
    Vector3 startIntensity = Vector3::Zero();
    Vector3 endIntensity = Vector3::Zero();
    double startPosition = 0.0;
    std::optional<double> endPosition;
};

struct LoadCase3D
{
    int id = 0;
    std::string name;
    std::vector<NodalLoad3D> nodalLoads;
    std::vector<DistributedLoad3D> distributedLoads;
    std::vector<PointLoad3D> pointLoads;
    std::vector<DistributedMoment3D> distributedMoments;
};

} // namespace TSA::Fem3D
