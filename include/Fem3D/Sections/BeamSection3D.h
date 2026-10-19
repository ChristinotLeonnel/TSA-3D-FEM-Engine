#pragma once

// Propriétés d'une section de poutre, constantes le long de l'élément (axes principaux = axes locaux y, z).
//   A            aire                                       [longueur²]
//   Iy, Iz       moments quadratiques autour de y et de z   [longueur⁴]  (Iz : flexion dans le plan x-y)
//   J            constante de torsion de Saint-Venant      [longueur⁴]  (≠ Iy + Iz sauf section circulaire)
//   Asy, Asz     aires effectives de cisaillement selon y et z (Timoshenko uniquement)
// Facultatifs, pour les contraintes (non inventés s'ils manquent) :
//   yExtreme, zExtreme   distances des fibres extrêmes au centre de gravité
//   torsionModulus       module de torsion Wt (τ_max = T / Wt)

#include <optional>
#include <string>

namespace TSA::Fem3D
{

struct BeamSection3D
{
    std::string name;
    double area = 0.0;
    double inertiaY = 0.0;
    double inertiaZ = 0.0;
    double torsionConstant = 0.0;
    std::optional<double> shearAreaY;
    std::optional<double> shearAreaZ;
    std::optional<double> yExtreme;
    std::optional<double> zExtreme;
    std::optional<double> torsionModulus;

    /// Section rectangulaire b (selon y) × h (selon z), aires de cisaillement 5/6 A, J de Saint-Venant (série
    /// tronquée, erreur < 0,5 %) et Wt correspondant.
    static BeamSection3D rectangle(std::string name, double widthY, double heightZ);
    /// Section circulaire pleine de diamètre d, aires de cisaillement k A avec le coefficient de Cowper
    /// k = 6 (1 + ν) / (7 + 6 ν) (k ≈ 0,886 pour ν = 0,3). La section ne connaît pas le matériau : passer son ν.
    static BeamSection3D circle(std::string name, double diameter, double poisson = 0.3);

    /// @throws Fem3DError (InvalidInput) propriété mécanique non finie ou ≤ 0 ; aires de cisaillement si requises.
    void validate(bool requiresShearAreas) const;
};

} // namespace TSA::Fem3D
