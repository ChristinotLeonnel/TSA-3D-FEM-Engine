#include "Fem3D/Materials/ElasticMaterial.h"

#include "Fem3D/Core/Fem3DError.h"

#include <cmath>

namespace TSA::Fem3D
{

ElasticMaterial ElasticMaterial::fromPoisson(std::string name, double youngModulus, double poissonRatio)
{
    if (!(poissonRatio > -1.0 && poissonRatio < 0.5))
        throw Fem3DError(ErrorCode::InvalidInput, "coefficient de Poisson hors de ]-1 ; 0,5[ pour le matériau « " + name + " »");
    return ElasticMaterial{ std::move(name), youngModulus, youngModulus / (2.0 * (1.0 + poissonRatio)) };
}

void ElasticMaterial::validate() const
{
    if (!(std::isfinite(youngModulus) && youngModulus > 0.0))
        throw Fem3DError(ErrorCode::InvalidInput, "module d'Young E non fini ou ≤ 0 (matériau « " + name + " »)");
    if (!(std::isfinite(shearModulus) && shearModulus > 0.0))
        throw Fem3DError(ErrorCode::InvalidInput, "module de cisaillement G non fini ou ≤ 0 (matériau « " + name + " »)");
}

} // namespace TSA::Fem3D
