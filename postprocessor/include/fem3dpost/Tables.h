#pragma once
// Tableaux de résultats (une ligne par nœud/station et par cas) et fonctions de tri / filtrage.

#include "fem3dpost/Results.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace fem3dpost {

// Cellule : vide (indisponible), entier (identifiants), réel ou texte.
using Cell = std::variant<std::monostate, long long, double, std::string>;

struct Column {
    std::string label;  // ex. « Ux »
    std::string unit;   // ex. « m » ; vide pour les colonnes sans unité (identifiants)
    std::string header() const { return unit.empty() ? label : label + " [" + unit + "]"; }
};

struct Table {
    std::string title;
    std::vector<Column> columns;
    std::vector<std::vector<Cell>> rows;

    // Index de la colonne dont le libellé (sans unité) vaut label.
    std::optional<std::size_t> column(std::string_view label) const;
};

// Libellés des colonnes d'identification, communs à tous les tableaux.
inline constexpr const char* kColNode = "Nœud";
inline constexpr const char* kColCase = "Cas";
inline constexpr const char* kColElement = "Élément";
inline constexpr const char* kColPosition = "Position x";

// Déplacements nodaux : Nœud, Cas, Ux, Uy, Uz, Rx, Ry, Rz (repère global).
Table displacementTable(const Results& r);
// Réactions : Nœud, Cas, Fx, Fy, Fz, Mx, My, Mz — uniquement les nœuds d'appui (section « supports »).
Table reactionTable(const Results& r);
// Forces des ressorts : Nœud, Cas, Fx … Mz — uniquement les nœuds de « springs » (tableau vide sans ressort).
Table springForceTable(const Results& r);
// Sollicitations aux stations : Élément, Cas, Position, N, Vy, Vz, T, My, Mz (repère local).
Table sectionForceTable(const Results& r);
// Déformations et contraintes aux stations.
Table strainStressTable(const Results& r);

// Valeur numérique d'une cellule (entier ou réel), sinon indisponible.
OptDouble cellNumber(const Cell& c);

// Filtrage : lève std::invalid_argument si la colonne d'identification est absente.
Table filterByCase(const Table& t, int caseId);
Table filterByElement(const Table& t, int elementId);
// Tri stable par valeur absolue décroissante de la colonne ; les cellules vides vont en fin.
Table sortByAbsDescending(const Table& t, std::size_t column);
// Les n premières lignes.
Table head(const Table& t, std::size_t n);

} // namespace fem3dpost
