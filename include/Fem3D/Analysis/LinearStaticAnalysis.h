#pragma once

// Analyse statique linéaire (petits déplacements, élasticité linéaire) :
//   1. contrôle du modèle ; 2. éléments finis et numérotation ; 3. assemblage et factorisation de K_ff (une fois) ;
//   4. par cas : F, U_f = K_ff⁻¹ (F_f − K_fr U_r), réactions R = K_rf U_f + K_rr U_r − F_r, efforts et déformées,
//      contrôle de l'équilibre global.
// Ne traite ni la non-linéarité géométrique ou matérielle, ni la dynamique.

#include "Fem3D/Assembly/GlobalAssembler.h"
#include "Fem3D/Results/AnalysisResults.h"
#include "Fem3D/Solver/LinearSystemSolver.h"

#include <memory>
#include <string>

namespace TSA::Fem3D
{

struct AnalysisOptions
{
    int stationsPerElement = 11;   ///< points de résultats le long de chaque élément (extrémités comprises)
};

class LinearStaticAnalysis
{
public:
    /// Contrôle, assemble et factorise. @throws Fem3DError (modèle invalide, mécanisme…)
    explicit LinearStaticAnalysis(const StructuralModel3D& model, AnalysisOptions options = {});

    LoadCaseResults run(int loadCaseId) const;
    AnalysisResults runAll() const;

    const DofMap& dofs() const { return *m_dofs; }
    const PartitionedStiffness& stiffness() const { return m_stiffness; }
    const std::vector<std::unique_ptr<IBeamElement3D>>& elements() const { return m_elements; }
    /// F global (DDL globaux) d'un cas, pour vérification ou export.
    DenseVector loadVector(int loadCaseId) const;

private:
    EquilibriumCheck checkEquilibrium(const LoadCase3D& loadCase, const std::vector<ReactionResult>& reactions,
                                      const std::vector<SpringResult>& springs) const;

    const StructuralModel3D& m_model;
    AnalysisOptions m_options;
    std::vector<std::unique_ptr<IBeamElement3D>> m_elements;
    std::unique_ptr<DofMap> m_dofs;
    std::unique_ptr<GlobalAssembler> m_assembler;
    PartitionedStiffness m_stiffness;
    LinearSystemSolver m_solver;
    std::string m_conditioningWarning;   ///< quasi-mécanisme détecté à la factorisation (vide sinon)
};

/// Export Matrix Market (coordonnées, réels, général) d'une matrice creuse. @throws Fem3DError (IoError)
void exportMatrixMarket(const SparseMatrix& matrix, const std::string& path);

} // namespace TSA::Fem3D
