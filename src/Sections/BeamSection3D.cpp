#include "Fem3D/Sections/BeamSection3D.h"

#include "Fem3D/Core/Fem3DError.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace TSA::Fem3D
{

namespace
{
constexpr double kRectangleShearFactor = 5.0 / 6.0;   ///< aire de cisaillement d'un rectangle : 5/6 A

void requirePositive(const std::string& section, const char* what, double value)
{
    if (!(std::isfinite(value) && value > 0.0))
        throw Fem3DError(ErrorCode::InvalidInput, std::string(what) + " non fini ou ≤ 0 (section « " + section + " »)");
}
} // namespace

BeamSection3D BeamSection3D::rectangle(std::string name, double widthY, double heightZ)
{
    requirePositive(name, "largeur", widthY);
    requirePositive(name, "hauteur", heightZ);
    BeamSection3D s;
    s.name = std::move(name);
    s.area = widthY * heightZ;
    s.inertiaY = widthY * heightZ * heightZ * heightZ / 12.0;   // flexion dans le plan x-z (hauteur selon z)
    s.inertiaZ = heightZ * widthY * widthY * widthY / 12.0;
    // Torsion de Saint-Venant, grand côté a, petit côté b, r = b/a (formules de Roark, section rectangulaire pleine) :
    //   J ≈ a b³ [1/3 − 0,21 r (1 − r⁴/12)] ;
    //   τ_max = T / Wt, Wt = a b² / (3 [1 + 0,6095 r + 0,8865 r² − 1,8023 r³ + 0,9100 r⁴]).
    // Écarts aux séries exactes de Saint-Venant pour a/b de 1 à 50 : J ≤ 0,5 %, Wt ≤ 0,3 % (test
    // Section_RectangleTorsionMatchesExactSeries) ; l'ancienne forme a b²/(3 + 1,8 r) surestimait Wt de 4,4 %.
    const double a = std::max(widthY, heightZ), b = std::min(widthY, heightZ), r = b / a;
    s.torsionConstant = a * b * b * b * (1.0 / 3.0 - 0.21 * r * (1.0 - r * r * r * r / 12.0));
    s.torsionModulus = a * b * b / (3.0 * (1.0 + r * (0.6095 + r * (0.8865 + r * (-1.8023 + r * 0.9100)))));
    s.shearAreaY = kRectangleShearFactor * s.area;
    s.shearAreaZ = kRectangleShearFactor * s.area;
    s.yExtreme = widthY / 2.0;
    s.zExtreme = heightZ / 2.0;
    return s;
}

BeamSection3D BeamSection3D::circle(std::string name, double diameter, double poisson)
{
    requirePositive(name, "diamètre", diameter);
    if (!(poisson > -1.0 && poisson < 0.5))
        throw Fem3DError(ErrorCode::InvalidInput, "coefficient de Poisson hors de ]−1 ; 0,5[ (section « " + name + " »)");
    const double pi = std::numbers::pi;
    BeamSection3D s;
    s.name = std::move(name);
    s.area = pi * diameter * diameter / 4.0;
    s.inertiaY = s.inertiaZ = pi * std::pow(diameter, 4) / 64.0;
    s.torsionConstant = pi * std::pow(diameter, 4) / 32.0;   // exact pour le cercle (= Iy + Iz)
    s.torsionModulus = pi * std::pow(diameter, 3) / 16.0;
    // Coefficient de cisaillement de Cowper pour le cercle plein : k = 6 (1 + ν) / (7 + 6 ν)
    const double k = 6.0 * (1.0 + poisson) / (7.0 + 6.0 * poisson);
    s.shearAreaY = s.shearAreaZ = k * s.area;
    s.yExtreme = s.zExtreme = diameter / 2.0;
    return s;
}

void BeamSection3D::validate(bool requiresShearAreas) const
{
    requirePositive(name, "aire A", area);
    requirePositive(name, "moment quadratique Iy", inertiaY);
    requirePositive(name, "moment quadratique Iz", inertiaZ);
    requirePositive(name, "constante de torsion J", torsionConstant);
    if (requiresShearAreas)
    {
        if (!shearAreaY || !shearAreaZ)
            throw Fem3DError(ErrorCode::InvalidInput, "aires de cisaillement Asy et Asz requises par Timoshenko (section « " + name + " »)");
        requirePositive(name, "aire de cisaillement Asy", *shearAreaY);
        requirePositive(name, "aire de cisaillement Asz", *shearAreaZ);
    }
    for (const auto& [what, value] : { std::pair{ "distance de fibre yExtreme", yExtreme }, std::pair{ "distance de fibre zExtreme", zExtreme },
                                        std::pair{ "module de torsion Wt", torsionModulus } })
        if (value) requirePositive(name, what, *value);
}

} // namespace TSA::Fem3D
