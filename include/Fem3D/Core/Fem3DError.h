#pragma once

// Erreur diagnostique du moteur : un code stable (testable) et un message lisible.

#include <stdexcept>
#include <string>

namespace TSA::Fem3D
{

enum class ErrorCode
{
    InvalidInput,        ///< donnée hors domaine (propriété ≤ 0, valeur non finie…)
    UnknownReference,    ///< nœud, élément ou cas de charge inexistant
    DuplicateId,         ///< identifiant déjà utilisé
    DegenerateGeometry,  ///< longueur nulle, repère local indéterminé
    Mechanism,           ///< matrice de rigidité singulière (degrés de liberté non retenus)
    NoFreeDof,           ///< tous les degrés de liberté sont bloqués
    IoError              ///< écriture ou lecture de fichier
};

class Fem3DError : public std::runtime_error
{
public:
    Fem3DError(ErrorCode code, const std::string& message)
        : std::runtime_error(message)
        , m_code(code)
    {
    }

    ErrorCode code() const noexcept { return m_code; }

private:
    ErrorCode m_code;
};

} // namespace TSA::Fem3D
