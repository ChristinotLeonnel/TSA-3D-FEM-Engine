#pragma once

// Matériau élastique linéaire isotrope. Unités cohérentes choisies par l'utilisateur (ex. kN, m → kN/m²) : le moteur
// ne convertit rien.

#include <string>

namespace TSA::Fem3D
{

struct ElasticMaterial
{
    std::string name;
    double youngModulus = 0.0;   ///< E  [force / longueur²]
    double shearModulus = 0.0;   ///< G  [force / longueur²]

    /// G = E / (2 (1 + ν)).
    static ElasticMaterial fromPoisson(std::string name, double youngModulus, double poissonRatio);

    /// @throws Fem3DError (InvalidInput) E ou G non finis ou ≤ 0.
    void validate() const;
};

} // namespace TSA::Fem3D
