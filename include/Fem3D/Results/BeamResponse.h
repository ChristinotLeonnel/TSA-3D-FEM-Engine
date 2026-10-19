#pragma once

// Réponse d'un élément : efforts aux extrémités, efforts internes et déformée le long de l'élément.
//
// Efforts aux extrémités (repère local, forces exercées sur l'élément) : f = K_local d_local − f_eq, où f_eq sont les
// forces nodales équivalentes de la charge répartie (= forces d'encastrement parfait changées de signe). Avec des rotules,
// d_local est d'abord complété par les rotations de la barre aux extrémités relâchées (Elements/StaticCondensation.h).
// Efforts internes par l'équilibre du tronçon [0, x] (charge q(s) = a + b s par composante) :
//   N(x)  = −f_ix − ∫₀ˣ qx        Vy(x) = −f_iy − ∫₀ˣ qy        Vz(x) = −f_iz − ∫₀ˣ qz        T(x) = −m_ix
//   Mz(x) = −m_iz + x f_iy + ∫₀ˣ (x − s) qy ds                    My(x) = −m_iy − x f_iz − ∫₀ˣ (x − s) qz ds
// Déformée, intégration exacte des polynômes (Timoshenko : termes de cisaillement ; Euler–Bernoulli : nuls) :
//   u = u_i + ∫ N/EA      θx = θx_i + ∫ T/GJ
//   θz = θz_i + ∫ Mz/EIz   v = v_i + ∫ (θz + Vy/(G Asy))
//   θy = θy_i + ∫ My/EIy   w = w_i + ∫ (−θy + Vz/(G Asz))

#include "Fem3D/Core/Polynomial.h"
#include "Fem3D/Elements/IBeamElement3D.h"
#include "Fem3D/Loads/Loads3D.h"
#include "Fem3D/Results/AnalysisResults.h"

#include <vector>

namespace TSA::Fem3D
{

/// Charge répartie linéaire en composantes locales : q(x) = start + (end − start) x / L.
struct LocalLinearLoad
{
    Vector3 start = Vector3::Zero();
    Vector3 end = Vector3::Zero();

    /// Somme des charges réparties d'un élément, projetées dans son repère local.
    static LocalLinearLoad fromLoads(const IBeamElement3D& element, const std::vector<const DistributedLoad3D*>& loads);
};

/// Forces nodales équivalentes (repère local, 12) : f_eq,k = ∫₀ᴸ N_k(x) q(x) dx avec les fonctions de forme de la
/// formulation (exactes : égales aux forces d'encastrement parfait changées de signe). Quadrature de Gauss exacte
/// (intégrande polynomiale de degré ≤ 4).
Vector12 equivalentNodalLoads(const IBeamElement3D& element, const LocalLinearLoad& load);

class BeamResponse
{
public:
    BeamResponse(const IBeamElement3D& element, const Vector12& localDisplacements, const LocalLinearLoad& load);

    const Vector12& endForces() const { return m_endForces; }
    SectionForces forcesAt(double x) const;
    Vector3 displacementAt(double x) const;   ///< u v w local
    Vector3 rotationAt(double x) const;       ///< θx θy θz local
    double slopeXYAt(double x) const { return m_slopeXY(x); }
    double slopeXZAt(double x) const { return m_slopeXZ(x); }
    SectionStresses stressesAt(const SectionForces& forces) const;

    /// Résultat complet de l'élément, à stationCount points équidistants (≥ 2) ; flèche maximale cherchée sur une
    /// subdivision fine (L / 200).
    ElementResult result(int stationCount) const;

private:
    const IBeamElement3D& m_element;
    Vector12 m_localDisplacements;
    Vector12 m_endForces;
    Polynomial m_axial, m_shearY, m_shearZ, m_torsion, m_momentY, m_momentZ;
    Polynomial m_u, m_v, m_w, m_thetaX, m_thetaY, m_thetaZ, m_slopeXY, m_slopeXZ;
};

} // namespace TSA::Fem3D
