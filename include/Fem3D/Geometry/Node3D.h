#pragma once

#include "Fem3D/Core/Types.h"

namespace TSA::Fem3D
{

/// Nœud du modèle : identifiant utilisateur (libre, non contigu) et coordonnées globales. L'indice interne des degrés
/// de liberté est attribué par DofMap, jamais déduit de l'identifiant.
struct Node3D
{
    int id = 0;
    Vector3 position = Vector3::Zero();
};

} // namespace TSA::Fem3D
