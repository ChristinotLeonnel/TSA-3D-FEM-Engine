#pragma once

// Format d'échange des résultats « tsa-fem3d-results/1 » (docs/RESULTS_FORMAT.md) : fichier JSON autonome lu par le
// post-traitement (sous-projet postprocessor/) sans dépendre du moteur.

#include "Fem3D/Model/StructuralModel3D.h"
#include "Fem3D/Results/AnalysisResults.h"

#include <string>

namespace TSA::Fem3D
{

inline constexpr char kResultsFormat[] = "tsa-fem3d-results/1";

struct UnitSystem
{
    std::string force = "kN";
    std::string length = "m";
};

/// @throws Fem3DError (IoError)
void writeResultsJson(const std::string& path, const std::string& projectName, const StructuralModel3D& model,
                      const AnalysisResults& results, const UnitSystem& units = {});
/// Même contenu, en mémoire (tests).
std::string resultsToJson(const std::string& projectName, const StructuralModel3D& model, const AnalysisResults& results,
                          const UnitSystem& units = {});

} // namespace TSA::Fem3D
