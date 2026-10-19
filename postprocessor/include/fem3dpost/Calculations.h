#pragma once
// Grandeurs dérivées calculées par le post-traitement à partir des seules données du fichier.

#include "fem3dpost/Results.h"

#include <optional>
#include <vector>

namespace fem3dpost {

// Flèche locale : écart du déplacement transversal (v, w) à la corde qui relie les
// déplacements transversaux des nœuds i et j (localDisplacements[1,2] et [7,8]).
struct ChordDeflection {
    double v = 0.0;          // écart selon y local
    double w = 0.0;          // écart selon z local
    double magnitude = 0.0;  // √(v² + w²)
};

// Indisponible si une des données nécessaires vaut null.
std::optional<ChordDeflection> chordDeflection(const Element& element, const ElementResult& result,
                                               const Station& station);

// Cisaillement moyen maximal : max(|shearY|, |shearZ|) ; indisponible si l'un des deux manque.
OptDouble maxMeanShear(const Stresses& s);

// Valeur extrême d'une série discrète et abscisse correspondante.
struct Extremum {
    double value = 0.0;
    double x = 0.0;
};
struct SeriesExtremes {
    std::optional<Extremum> min, max, maxAbs;  // maxAbs : valeur signée de plus grande valeur absolue
};
SeriesExtremes seriesExtremes(const std::vector<double>& x, const std::vector<OptDouble>& values);

// Résultantes des réactions recalculées à partir du tableau des réactions des nœuds d'appui,
// moments pris à l'origine du repère global : Σ F et Σ (r × F + M).
struct Resultant {
    std::array<OptDouble, 3> force{}, moment{};
};
Resultant reactionResultant(const Results& results, const LoadCase& loadCase);

// Norme de a + b ; indisponible si une composante manque.
OptDouble normOfSum(const std::array<OptDouble, 3>& a, const std::array<OptDouble, 3>& b);
OptDouble norm(const std::array<OptDouble, 3>& a);

} // namespace fem3dpost
