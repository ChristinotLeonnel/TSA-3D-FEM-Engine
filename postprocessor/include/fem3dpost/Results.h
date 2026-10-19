#pragma once
// Modèle de données en mémoire du format « tsa-fem3d-results/1 ».
// Toute valeur numérique de résultat qui vaut null dans le JSON est conservée comme
// « indisponible » (std::nullopt) : elle n'est jamais remplacée par 0.

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace fem3dpost {

inline constexpr const char* kExpectedFormat = "tsa-fem3d-results/1";
inline constexpr const char* kPostProcessorVersion = "fem3dpost 0.1.0";

using OptDouble = std::optional<double>;

struct Units {
    std::string force, length, moment, rotation, stress;
};

struct Conventions {
    std::vector<std::string> dofOrder;
    std::string localAxes, sectionForces, endForces, reactions;
    std::string springForces;  // facultatif (absent sans ressort)
};

struct Node {
    int id = 0;
    std::array<double, 3> xyz{};
};

struct Material {
    std::string name;
    OptDouble E, G;
};

struct Section {
    std::string name;
    OptDouble A, Iy, Iz, J, Asy, Asz, yExtreme, zExtreme, Wt;
};

struct Element {
    int id = 0;
    int nodeI = 0, nodeJ = 0;
    std::string formulation;  // "Euler-Bernoulli" ou "Timoshenko"
    double length = 0.0;
    std::array<double, 3> localX{}, localY{}, localZ{};
    Material material;
    Section section;
};

struct Support {
    int node = 0;
    std::array<bool, 6> fixed{};
    std::array<OptDouble, 6> prescribed{};
};

// Appui élastique : raideurs kx, ky, kz, krx, kry, krz (repère global). Champ « springs » facultatif.
struct Spring {
    int node = 0;
    std::array<OptDouble, 6> stiffness{};
};

struct Stresses {
    OptDouble axial, bendingY, bendingZ, extremeNormal, shearY, shearZ, torsion, vonMisesIndicative;
};

struct Station {
    double x = 0.0;                                // abscisse locale 0 ≤ x ≤ L
    std::array<OptDouble, 6> forces{};             // N, Vy, Vz, T, My, Mz (face positive, repère local)
    std::array<OptDouble, 3> localDisplacement{};  // u, v, w (repère local, déplacement total)
    std::array<OptDouble, 3> sectionRotation{};    // θx, θy, θz
    std::array<OptDouble, 2> slopes{};             // dv/dx, dw/dx
    std::array<OptDouble, 3> globalDisplacement{}; // Ux, Uy, Uz
    Stresses stresses;
};

struct ElementResult {
    int id = 0;
    std::array<OptDouble, 12> endForcesLocal{};
    std::array<OptDouble, 12> localDisplacements{};
    OptDouble maxDeflection, maxDeflectionPosition;
    std::vector<Station> stations;
};

struct NodalValues {
    int node = 0;
    std::array<OptDouble, 6> values{};
};

struct Equilibrium {
    std::array<OptDouble, 3> appliedForce{}, appliedMoment{}, reactionForce{}, reactionMoment{};
    OptDouble forceResidual, momentResidual;
    OptDouble solutionResidual;  // facultatif : précision de la résolution ‖K U − F‖ / ‖F‖
};

struct LoadCase {
    int id = 0;
    std::string name;
    std::vector<NodalValues> displacements;
    std::vector<NodalValues> reactions;
    std::vector<NodalValues> springForces;  // −k·u, exercées par le ressort sur la structure (facultatif)
    std::vector<ElementResult> elements;
    Equilibrium equilibrium;
    std::vector<std::string> warnings;  // avertissements non bloquants du moteur (facultatif)

    const NodalValues* displacementOf(int node) const;
    const NodalValues* reactionOf(int node) const;
    const NodalValues* springForceOf(int node) const;
    const ElementResult* elementResult(int elementId) const;
};

struct Results {
    std::string format, engine, project;
    Units units;
    Conventions conventions;
    std::vector<Node> nodes;
    std::vector<Element> elements;
    std::vector<Support> supports;
    std::vector<Spring> springs;
    std::vector<LoadCase> loadCases;

    const Node* node(int id) const;
    const Element* element(int id) const;
    const Support* support(int nodeId) const;
    const LoadCase* loadCase(int id) const;
    bool isSupport(int nodeId) const { return support(nodeId) != nullptr; }
};

} // namespace fem3dpost
