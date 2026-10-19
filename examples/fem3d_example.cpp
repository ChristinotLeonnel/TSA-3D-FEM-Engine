// Exemple : portique spatial calculé en Timoshenko, résultats exportés au format « tsa-fem3d-results/1 ».
// Usage : fem3d_example [fichier de sortie .json] [nombre de stations par élément]

#include "Fem3D/Analysis/LinearStaticAnalysis.h"
#include "Fem3D/Core/Fem3DError.h"
#include "Fem3D/Results/ResultsJsonWriter.h"

#include <iostream>
#include <string>

using namespace TSA::Fem3D;

int main(int argc, char** argv)
{
    const std::string output = argc > 1 ? argv[1] : "portique_spatial_resultats.json";
    AnalysisOptions options;
    if (argc > 2) options.stationsPerElement = std::stoi(argv[2]);
    try
    {
        StructuralModel3D m;
        m.addMaterial(ElasticMaterial::fromPoisson("S235", 210e6, 0.3));
        m.addSection(BeamSection3D::rectangle("R30x50", 0.3, 0.5));
        m.addSection(BeamSection3D::rectangle("R20x40", 0.2, 0.4));
        const Vector3 foot[4] = { { 0, 0, 0 }, { 6, 0, 0 }, { 6.5, 4, 0 }, { -0.5, 4.5, 0 } };
        const double height[4] = { 3.0, 3.5, 4.0, 3.2 };
        for (int k = 0; k < 4; ++k)
        {
            m.addNode(1 + k, foot[k]);
            m.addNode(11 + k, foot[k] + Vector3(0, 0, height[k]));
            m.setRestraint(NodeRestraint::fullyFixed(1 + k));
            m.addElement({ 1 + k, 1 + k, 11 + k, "S235", "R30x50", BeamFormulation::Timoshenko, Vector3(-1, 0, 0) });
        }
        for (int k = 0; k < 4; ++k) m.addElement({ 11 + k, 11 + k, 11 + (k + 1) % 4, "S235", "R20x40", BeamFormulation::Timoshenko, Vector3(0, 0, 1) });
        LoadCase3D gravity{ 1, "G : poids propre et charges permanentes", {}, {} };
        for (int e = 11; e <= 14; ++e) gravity.distributedLoads.push_back({ e, LoadFrame::Global, Vector3(0, 0, -12), Vector3(0, 0, -12) });
        LoadCase3D wind{ 2, "W : vent selon X", {}, {} };
        wind.nodalLoads.push_back({ 11, Vector3(18, 0, 0), Vector3::Zero() });
        wind.distributedLoads.push_back({ 1, LoadFrame::Global, Vector3(3, 0, 0), Vector3(5, 0, 0) });   // pression croissante avec la hauteur
        m.addLoadCase(gravity);
        m.addLoadCase(wind);

        const AnalysisResults results = LinearStaticAnalysis(m, options).runAll();
        writeResultsJson(output, "Portique spatial d'exemple", m, results);
        for (const auto& c : results.loadCases)
            std::cout << "cas " << c.loadCaseId << " (" << c.loadCaseName << ") : |ΣF| = " << c.equilibrium.forceResidual << ", |ΣM| = " << c.equilibrium.momentResidual << '\n';
        std::cout << "résultats écrits : " << output << '\n';
        return 0;
    }
    catch (const Fem3DError& e)
    {
        std::cerr << "erreur : " << e.what() << '\n';
        return 1;
    }
}
