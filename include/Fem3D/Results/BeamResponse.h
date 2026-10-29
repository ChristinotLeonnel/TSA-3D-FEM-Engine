#pragma once

// Réponse d'un élément : efforts aux extrémités, efforts internes et déformée le long de l'élément.
//
// Efforts aux extrémités (repère local, forces exercées sur l'élément) : f = K_local d_local − f_eq, où f_eq sont les
// forces nodales équivalentes des charges sur barre (= forces d'encastrement parfait changées de signe). Avec des
// rotules, d_local est d'abord complété par les rotations de la barre aux extrémités relâchées
// (Elements/StaticCondensation.h).
// Efforts internes par l'équilibre du tronçon [0, x] (charge répartie q, moment réparti c, forces ponctuelles P_k en x_k) :
//   N(x)  = −f_ix − ∫₀ˣ qx − Σ_{x_k ≤ x} P_kx      Vy(x) = −f_iy − ∫₀ˣ qy − Σ P_ky      Vz(x) = −f_iz − ∫₀ˣ qz − Σ P_kz
//   T(x)  = −m_ix − ∫₀ˣ cx
//   Mz(x) = −m_iz + x f_iy + ∫₀ˣ (x − s) qy ds + Σ (x − x_k) P_ky − ∫₀ˣ cz      (dMz/dx = −Vy − cz)
//   My(x) = −m_iy − x f_iz − ∫₀ˣ (x − s) qz ds − Σ (x − x_k) P_kz − ∫₀ˣ cy      (dMy/dx =  Vz − cy)
// Déformée, intégration exacte des polynômes (Timoshenko : termes de cisaillement ; Euler–Bernoulli : nuls) :
//   u = u_i + ∫ N/EA      θx = θx_i + ∫ T/GJ
//   θz = θz_i + ∫ Mz/EIz   v = v_i + ∫ (θz + Vy/(G Asy))
//   θy = θy_i + ∫ My/EIy   w = w_i + ∫ (−θy + Vz/(G Asz))
// Les champs sont polynomiaux par morceaux entre les abscisses des charges (docs/theory/THEORY.md, charges sur barre) :
// N et V sautent sous une force ponctuelle, les déplacements et les rotations de section restent continus.

#include "Fem3D/Core/Polynomial.h"
#include "Fem3D/Elements/IBeamElement3D.h"
#include "Fem3D/Loads/Loads3D.h"
#include "Fem3D/Results/AnalysisResults.h"

#include <vector>

namespace TSA::Fem3D
{

/// Charge linéaire sur [start, end] (abscisses locales depuis le nœud i), composantes locales : force répartie ou
/// moment réparti selon la liste qui la contient.
struct LocalLoadSegment
{
    double start = 0.0;
    double end = 0.0;
    Vector3 startIntensity = Vector3::Zero();
    Vector3 endIntensity = Vector3::Zero();

    Vector3 at(double x) const { return startIntensity + (endIntensity - startIntensity) * ((x - start) / (end - start)); }
};

struct LocalPointForce
{
    double position = 0.0;
    Vector3 force = Vector3::Zero();
};

/// Charges d'un élément pour un cas, en composantes locales, positions ramenées dans [0, L].
struct LocalElementLoads
{
    std::vector<LocalLoadSegment> distributed;   ///< forces réparties
    std::vector<LocalLoadSegment> moments;       ///< moments répartis
    std::vector<LocalPointForce> points;         ///< forces ponctuelles

    bool empty() const { return distributed.empty() && moments.empty() && points.empty(); }

    /// Charges du cas portées par l'élément, projetées dans son repère local (a_local = R a_global).
    static LocalElementLoads fromLoadCase(const IBeamElement3D& element, const LoadCase3D& loadCase);
};

/// Forces nodales équivalentes (repère local, 12) : f_eq,k = ∫₀ᴸ (N_k q + Θ_k c) dx + Σ N_k(x_p) P, avec les fonctions de
/// forme N_k de la formulation et les rotations de section Θ_k associées (Timoshenko : Θ = v′ − γ). Exactes (égales aux
/// forces d'encastrement parfait changées de signe) : ces champs sont les solutions homogènes exactes de l'élément
/// (théorème de réciprocité de Betti). Quadrature de Gauss à 4 points sur chaque intervalle chargé, exacte (intégrande
/// polynomiale de degré ≤ 4).
Vector12 equivalentNodalLoads(const IBeamElement3D& element, const LocalElementLoads& loads);

class BeamResponse
{
public:
    BeamResponse(const IBeamElement3D& element, const Vector12& localDisplacements, const LocalElementLoads& loads);

    const Vector12& endForces() const { return m_endForces; }
    /// À l'abscisse d'une force ponctuelle : valeurs juste après la charge (côté du nœud j).
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
    PiecewisePolynomial m_axial, m_shearY, m_shearZ, m_torsion, m_momentY, m_momentZ;
    PiecewisePolynomial m_u, m_v, m_w, m_thetaX, m_thetaY, m_thetaZ, m_slopeXY, m_slopeXZ;
};

} // namespace TSA::Fem3D
