#pragma once

// Chargements (docs/theory/SIGN_CONVENTIONS.md §4). Unités cohérentes avec le modèle :
//   force [F], moment [F·L], charge répartie [F/L] (par unité de longueur réelle de l'élément).

#include "Fem3D/Core/Types.h"

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

/// Charge répartie sur toute la longueur d'un élément, linéaire entre q(0) = startIntensity et q(L) = endIntensity
/// (uniforme si les deux sont égales). Jamais remplacée par une approximation nodale : ses forces nodales équivalentes
/// sont celles de la formulation, et elle est reprise exactement dans les efforts internes et la déformée.
struct DistributedLoad3D
{
    int elementId = 0;
    LoadFrame frame = LoadFrame::Local;
    Vector3 startIntensity = Vector3::Zero();
    Vector3 endIntensity = Vector3::Zero();
};

struct LoadCase3D
{
    int id = 0;
    std::string name;
    std::vector<NodalLoad3D> nodalLoads;
    std::vector<DistributedLoad3D> distributedLoads;
};

} // namespace TSA::Fem3D
