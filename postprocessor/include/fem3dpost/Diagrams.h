#pragma once
// Données de diagrammes par élément et par cas, et rendu SVG simple.
// Seules les stations fournies par le moteur sont tracées ; entre deux stations le tracé est
// une interpolation linéaire, ce que la légende de chaque SVG indique. Aucune enveloppe de cas.

#include "fem3dpost/Results.h"
#include "fem3dpost/Tables.h"

#include <string>
#include <vector>

namespace fem3dpost {

struct DiagramSeries {
    std::string label;
    std::vector<OptDouble> values;  // une valeur par station (indisponible possible)
};

struct Diagram {
    std::string quantity;  // N, Vy, Vz, T, My, Mz, deformee, fleches, rotations
    std::string title;     // titre lisible
    std::string unit;
    std::string signNote;  // rappel de convention
    int caseId = 0;
    int elementId = 0;
    double length = 0.0;
    std::vector<double> x;  // abscisses des stations
    std::vector<DiagramSeries> series;
};

// Les 9 grandeurs tracées, dans l'ordre : N, Vy, Vz, T, My, Mz, deformee, fleches, rotations.
const std::vector<std::string>& diagramQuantities();

std::vector<Diagram> elementDiagrams(const Results& r, const LoadCase& lc, const ElementResult& er);

// Données de diagrammes d'un élément pour un cas (une ligne par station).
Table diagramDataTable(const Results& r, const LoadCase& lc, const ElementResult& er);

// Nom de fichier sans extension : « cas<k>_elem<e>_<grandeur> ».
std::string diagramStem(const Diagram& d);
std::string diagramDataStem(int caseId, int elementId);

// SVG autonome (680 × 300 px), signe respecté (valeurs positives vers le haut),
// stations marquées, extrêmes annotés, légende mentionnant l'interpolation linéaire.
std::string diagramSvg(const Diagram& d);

// Format d'affichage court (n chiffres significatifs, point décimal).
std::string formatShort(double v, int significant = 4);

} // namespace fem3dpost
