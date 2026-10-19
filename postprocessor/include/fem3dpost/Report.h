#pragma once
// Note de calcul HTML en 15 sections, construite uniquement à partir du fichier de résultats
// (et, si fournie, d'une synthèse de comparaison OpenSees). Aucune valeur illustrative.

#include "fem3dpost/Csv.h"
#include "fem3dpost/Results.h"

#include <optional>
#include <string>
#include <vector>

namespace fem3dpost {

enum class SvgMode {
    Inline,          // SVG intégrés dans le HTML (fichier autonome)
    ImageReference,  // <img src="diag_<nom>.png"> : ressources fournies par l'export PDF
};

struct ReportOptions {
    std::string inputFile;              // nom du fichier de résultats affiché en section 1
    std::string generatedAt;            // date de génération affichée en section 1
    std::optional<int> caseId;          // limite la note à un cas
    std::optional<TextTable> openSees;  // synthèse OpenSees importée (section 14)
    std::string openSeesSource;         // nom du fichier de synthèse
    SvgMode svgMode = SvgMode::Inline;
};

struct ReportDiagram {
    std::string stem;      // ex. cas1_elem11_My
    std::string imageRef;  // nom de ressource en mode ImageReference (diag_<stem>.png)
    std::string svg;
};

struct Report {
    std::string html;
    std::vector<ReportDiagram> diagrams;
};

// Titres des 15 sections, dans l'ordre (les ancres HTML sont s1 … s15).
const std::vector<std::string>& reportSectionTitles();

// Lève std::invalid_argument si options.caseId ne correspond à aucun cas.
Report buildReport(const Results& r, const ReportOptions& options);

// Largeur / hauteur d'affichage des diagrammes en mode ImageReference (px).
inline constexpr int kReportImageWidth = 560;
inline constexpr int kReportImageHeight = 247;

} // namespace fem3dpost
