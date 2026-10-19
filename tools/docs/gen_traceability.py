"""Génère docs/traceability/GIT_TRACEABILITY.md : liens vérifiés entre notions, code, tests, benchmarks et commits.

  python tools/docs/gen_traceability.py          écrit la matrice
  python tools/docs/gen_traceability.py --check  échoue si le fichier versionné n'est plus à jour

La correspondance (notion → symbole C++ → tests → cas OpenSees) est écrite ci-dessous, à la main ; les plages de lignes
sont calculées à partir de Git, à la révision de référence (tools/docs/gitlinks.py). Un symbole introuvable arrête le
script : la matrice ne peut pas pointer vers du code qui n'existe pas à cette révision.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "docs"))
import gitlinks as g  # noqa: E402

OUT = ROOT / "docs" / "traceability" / "GIT_TRACEABILITY.md"
FEM, TSA = g.REFERENCE_COMMITS["fem3d"], g.REFERENCE_COMMITS["tsa"]
FEM_URL, TSA_URL = (f"https://github.com/{g.REPOS[k]['slug']}" for k in ("fem3d", "tsa"))

# (niveau pédagogique, notion, [(dépôt, fichier, motif de la définition, libellé)], [tests], [cas OpenSees])
ROWS = [
    (0, "Rôle du moteur : orchestration d'un calcul",
     [("fem3d", "src/Analysis/LinearStaticAnalysis.cpp", r"^LinearStaticAnalysis::LinearStaticAnalysis", "LinearStaticAnalysis (constructeur)"),
      ("fem3d", "src/Analysis/LinearStaticAnalysis.cpp", r"^LoadCaseResults LinearStaticAnalysis::run", "LinearStaticAnalysis::run")],
     ["Readme_ExampleRuns"], []),
    (0, "Place dans TSA : moteur secondaire",
     [("tsa", "src/Analysis/Engines/Fem3D/Fem3DEngine.cpp", r"^AnalysisRunResult Fem3DEngine::run", "Fem3DEngine::run"),
      ("tsa", "src/Analysis/Engines/BuiltInEngines.cpp", r"^void registerBuiltInEngines", "registerBuiltInEngines")],
     ["TSA test 476"], []),
    (1, "Modèle structurel : nœuds, éléments, appuis, ressorts, charges",
     [("fem3d", "include/Fem3D/Model/StructuralModel3D.h", r"^class StructuralModel3D", "StructuralModel3D"),
      ("fem3d", "src/Model/StructuralModel3D.cpp", r"^void StructuralModel3D::validate", "StructuralModel3D::validate")],
     ["Input_InvalidPropertiesRejected", "Springs_InvalidInputRejected"], []),
    (1, "Repère local (convention geomTransf) et matrice T",
     [("fem3d", "src/Geometry/LocalCoordinateSystem3D.cpp", r"^LocalCoordinateSystem3D::LocalCoordinateSystem3D", "LocalCoordinateSystem3D"),
      ("fem3d", "src/Geometry/LocalCoordinateSystem3D.cpp", r"^Matrix12 LocalCoordinateSystem3D::transformation", "transformation")],
     ["LocalAxes_HorizontalVerticalInclined", "LocalAxes_DegenerateGeometryRejected", "Integration_InclinedCantilever_OrientationInvariance"], ["07", "16"]),
    (1, "Matériaux et sections (J, Wt, Cowper)",
     [("fem3d", "src/Sections/BeamSection3D.cpp", r"^BeamSection3D BeamSection3D::rectangle", "BeamSection3D::rectangle"),
      ("fem3d", "src/Sections/BeamSection3D.cpp", r"^BeamSection3D BeamSection3D::circle", "BeamSection3D::circle")],
     ["Section_RectangleTorsionMatchesExactSeries", "Section_CircleShearCoefficientUsesPoisson"], []),
    (2, "Poutre d'Euler–Bernoulli : rigidité exacte 12×12",
     [("fem3d", "src/Elements/BeamElements.cpp", r"^Matrix12 EulerBernoulli3D::localStiffnessMatrix", "EulerBernoulli3D::localStiffnessMatrix"),
      ("fem3d", "src/Elements/BeamElements.cpp", r"^void BeamElementBase::addAxialAndTorsion", "addAxialAndTorsion (EA/L, GJ/L)")],
     ["Element_ExplicitStiffnessTerms", "Element_SymmetryAndRigidBodyModes", "Analytical_AxialTension", "Analytical_SaintVenantTorsion"], ["01", "02", "03", "04"]),
    (2, "Poutre de Timoshenko : Φy, Φz, élément exact",
     [("fem3d", "src/Elements/BeamElements.cpp", r"^Matrix12 Timoshenko3D::localStiffnessMatrix", "Timoshenko3D::localStiffnessMatrix"),
      ("fem3d", "src/Elements/BeamElements.cpp", r"^double Timoshenko3D::shearParameterY", "shearParameterY")],
     ["Element_TimoshenkoTendsToEulerBernoulli", "Analytical_TimoshenkoDeepBeam_NoShearLocking", "Timoshenko_DistinctShearAreasPerPlane"], ["06"]),
    (2, "Transformation locale → globale",
     [("fem3d", "src/Elements/BeamElements.cpp", r"^Matrix12 IBeamElement3D::globalStiffnessMatrix", "IBeamElement3D::globalStiffnessMatrix")],
     ["Integration_InclinedCantilever_OrientationInvariance"], ["07", "10"]),
    (3, "Numérotation et partition des DDL",
     [("fem3d", "src/Assembly/Assembly.cpp", r"^DofMap::DofMap", "DofMap")],
     ["Analytical_SupportSettlement"], []),
    (3, "Assemblage de K (et ressorts sur la diagonale)",
     [("fem3d", "src/Assembly/Assembly.cpp", r"^PartitionedStiffness GlobalAssembler::assembleStiffness", "GlobalAssembler::assembleStiffness")],
     ["Integration_SpatialFrame_EquilibriumAndMeshInvariance", "Springs_TipSpringInParallelWithCantilever"], ["08", "18", "19", "20"]),
    (3, "Vecteur des forces (charges nodales et réparties)",
     [("fem3d", "src/Assembly/Assembly.cpp", r"^DenseVector GlobalAssembler::assembleLoads", "GlobalAssembler::assembleLoads"),
      ("fem3d", "src/Results/BeamResponse.cpp", r"^Vector12 equivalentNodalLoads", "equivalentNodalLoads")],
     ["Analytical_FixedFixedTriangularLoad_EulerBernoulli", "Analytical_FixedFixedTriangularLoad_Timoshenko", "Integration_GlobalAndLocalDistributedLoadsEquivalent"], ["05", "09"]),
    (3, "Rotules : condensation statique",
     [("fem3d", "src/Elements/StaticCondensation.cpp", r"^Matrix12 condensedStiffness", "condensedStiffness"),
      ("fem3d", "src/Elements/StaticCondensation.cpp", r"^Vector12 condensedLoads", "condensedLoads")],
     ["Releases_CondensedStiffnessIsProppedCantilever", "Releases_PinnedBeamBetweenClampedNodes", "Analytical_ProppedCantileverTriangularLoad_Timoshenko"], ["12", "13", "14", "15"]),
    (3, "Résolution, mécanismes, quasi-mécanismes",
     [("fem3d", "src/Solver/LinearSystemSolver.cpp", r"^void LinearSystemSolver::factorize", "LinearSystemSolver::factorize")],
     ["Solver_MechanismCriterionIndependentOfUnits", "Solver_QuasiMechanismWarnsButSolves", "Integration_MechanismDetected", "Releases_HingeAtSupportCreatesMechanism"], ["11", "17"]),
    (4, "Réactions, forces des ressorts, équilibre global",
     [("fem3d", "src/Analysis/LinearStaticAnalysis.cpp", r"^EquilibriumCheck LinearStaticAnalysis::checkEquilibrium", "checkEquilibrium")],
     ["Springs_StructureSupportedOnlyBySprings", "Springs_RotationalSpringAtBase"], ["18", "19", "20"]),
    (4, "Efforts internes et déformée exacts",
     [("fem3d", "src/Results/BeamResponse.cpp", r"^BeamResponse::BeamResponse", "BeamResponse"),
      ("fem3d", "src/Results/BeamResponse.cpp", r"^SectionForces BeamResponse::forcesAt", "BeamResponse::forcesAt")],
     ["Analytical_CantileverUniformLoad_ExactAlongMember", "Integration_ResponseConsistentWithNodalDisplacements"], ["05", "09"]),
    (4, "Contraintes de résistance des matériaux",
     [("fem3d", "src/Results/BeamResponse.cpp", r"^SectionStresses BeamResponse::stressesAt", "BeamResponse::stressesAt")],
     ["Analytical_SaintVenantTorsion"], []),
    (4, "Export JSON et post-traitement",
     [("fem3d", "src/Results/ResultsJsonWriter.cpp", r"^std::string resultsToJson", "resultsToJson")],
     ["Integration_ResultsJsonFormat", "Springs_ExportedInResultsJson"], []),
    (5, "Comparaison à OpenSees : scripts, tolérances, verdicts",
     [("fem3d", "benchmarks/OpenSees/opensees_benchmarks.cpp", r"^bool writeTcl", "writeTcl"),
      ("fem3d", "benchmarks/OpenSees/opensees_benchmarks.cpp", r"^void compareGroup", "compareGroup")],
     [], ["01", "08", "12", "18"]),
    (5, "Comparaison Fem3D / OpenSees dans TSA",
     [("tsa", "src/Analysis/Engines/Fem3D/Fem3DEngine.cpp", r"^EngineComparison compareResults", "compareResults")],
     ["TSA test 477"], []),
    (6, "Interface des éléments et fabrique",
     [("fem3d", "include/Fem3D/Elements/IBeamElement3D.h", r"^class IBeamElement3D", "IBeamElement3D"),
      ("fem3d", "src/Model/StructuralModel3D.cpp", r"^std::unique_ptr<IBeamElement3D> StructuralModel3D::createElement", "createElement")],
     ["Element_SymmetryAndRigidBodyModes"], []),
    (6, "Adaptateur TSA : conversion et secours",
     [("tsa", "src/Analysis/Engines/Fem3D/Fem3DAdapter.cpp", r"^Conversion convert", "Fem3DIntegration::convert"),
      ("tsa", "src/Analysis/Engines/Fem3D/Fem3DEngine.cpp", r"^ValidationResult Fem3DEngine::validate", "Fem3DEngine::validate")],
     ["TSA test 478", "TSA test 479", "TSA test 480"], []),
]

# Niveaux de maturité (docs/validation/MATURITY_LEVELS.md) : (niveau, critère, preuves [(libellé, cible)])
MATURITY = [
    (1, "Code écrit, données invalides refusées avec l'objet désigné",
     [("validate", ("fem3d", "src/Model/StructuralModel3D.cpp", r"^void StructuralModel3D::validate")), ("test", "Input_InvalidPropertiesRejected"), ("test", "Springs_InvalidInputRejected")]),
    (2, "Solutions analytiques exactes (écart ≤ 1e-9)",
     [("test", "Analytical_CantileverTipLoad_BothPlanesBothFormulations"), ("test", "Analytical_FixedFixedTriangularLoad_Timoshenko"), ("test", "Releases_GerberHingeBetweenTwoCantilevers"), ("test", "Springs_RotationalSpringAtBase")]),
    (3, "Référence indépendante (OpenSees), tolérances fixées avant",
     [("code", ("fem3d", "benchmarks/OpenSees/opensees_benchmarks.cpp", r"^void compareGroup")), ("cas", "01"), ("cas", "06"), ("cas", "12")]),
    (4, "Modèles représentatifs multi-éléments",
     [("cas", "08"), ("cas", "10"), ("cas", "14"), ("cas", "20"), ("test", "Integration_SpatialFrame_EquilibriumAndMeshInvariance")]),
    (4, "Audit indépendant des formulations",
     [("doc", "../verification/INDEPENDENT_FORMULATION_AUDIT.md")]),
    (5, "Comparaison dans TSA sur un modèle réel (premier jalon, non suffisant)",
     [("test", "TSA test 477")]),
    (5, "Comparaison systématique sur les projets d'exemple et docs/validation de TSA",
     [("absent", "aucune preuve : non réalisé")]),
    (5, "Relecture humaine de la théorie",
     [("absent", "aucune preuve : non réalisé")]),
]


def symbol_link(repo, path, pattern, label=None):
    url = g.permalink(path, pattern, repo)
    return f"[{label or path}]({url})"


def anchors():
    """Permaliens des tests Fem3D, des cas OpenSees et des tests TSA (plages calculées)."""
    out = {}
    import subprocess
    files = subprocess.run(["git", "-C", str(ROOT), "ls-tree", "-r", "--name-only", FEM, "tests"],
                           capture_output=True, text=True).stdout.split()
    for f in (x for x in files if x.endswith(".cpp")):
        lines = g.file_at(ROOT, FEM, f)
        for i, line in enumerate(lines):
            m = re.match(r"FEM3D_TEST\((\w+)\)", line)
            if m:
                rng = g.definition_range(lines[i + 1:], r"\{") or (1, 1)
                out[m.group(1)] = f"{FEM_URL}/blob/{FEM}/{f}#L{i + 1}-L{i + rng[1] + 1}"
    bench = "benchmarks/OpenSees/opensees_benchmarks.cpp"
    for i, line in enumerate(g.file_at(ROOT, FEM, bench)):
        m = re.search(r'list\.push_back\(\{ "(\d\d)_(\w+)"', line)
        if m:
            out["cas " + m.group(1)] = f"{FEM_URL}/blob/{FEM}/{bench}#L{i + 1}"
            out["nom " + m.group(1)] = m.group(2)
    tf = "tests/test_analysis_engines.cpp"
    for i, line in enumerate(g.file_at(g.REPOS["tsa"]["path"], TSA, tf) or []):
        m = re.search(r'\[PASS\] Test (\d+):', line)
        if m:
            out["TSA test " + m.group(1)] = f"{TSA_URL}/blob/{TSA}/{tf}#L{i + 1}"
    return out


def render() -> str:
    a = anchors()

    def test(name):
        if name not in a:
            sys.exit(f"test introuvable à la révision de référence : {name}")
        return f"[{name}]({a[name]})"

    def case(n):
        return f"[{n} {a['nom ' + n]}]({a['cas ' + n]})"

    commit = {"fem3d": f"[75700d2]({FEM_URL}/commit/{FEM})", "tsa": f"[2f5d7d4]({TSA_URL}/commit/{TSA})"}
    lines = [
        "# Traçabilité Git : théorie, code, tests, commits", "",
        "Générée par `python tools/docs/gen_traceability.py` : **ne pas modifier à la main**. Tous les liens pointent vers",
        "une révision figée (hash complet) et ont été vérifiés par `tools/docs/gitlinks.py check` : ils restent valables",
        "quand `main` évolue. Les deux dépôts sont publics (vérifié le 2026-10-19 par une requête anonyme).", "",
        "## 1. Révisions de référence et historique", "",
        "| Dépôt | Révision de référence | Contenu | Origine des fonctionnalités |",
        "|---|---|---|---|",
        f"| [TSA-3D-FEM-Engine]({FEM_URL}) | {commit['fem3d']} | moteur, tests, benchmarks, post-traitement, docs (v0.2.0) | "
        "**Tout le code a été introduit par ce commit** : le développement antérieur n'était pas versionné. L'historique "
        "fin des fonctionnalités (rotules, ressorts, corrections de l'audit) n'est pas reconstituable dans Git ; il est "
        "décrit dans [AUDIT.md](../AUDIT.md) et [ROADMAP.md](../ROADMAP.md). Statut : **historique antérieur non confirmé**. |",
        f"| [Tsaraloha-Structural-Analysis]({TSA_URL}) | {commit['tsa']} | intégration du moteur `fem3d` dans TSA | "
        "Commit qui introduit l'adaptateur, le moteur, les options et les tests 476 à 480 (vérifié par `git show --stat`). |",
        "",
        f"Commits suivants du dépôt Fem3D : `ba5f19f` (premier commit : README d'une ligne, avant le code) et `bcbb438`",
        "(fusion de 75700d2 dans `main`). Commit suivant de TSA : `21cbb2e` (fusion de 2f5d7d4 dans `main`).", "",
        "## 2. Matrice par niveau pédagogique", "",
        "Niveaux pédagogiques : [parcours](../learning/index.md) ; ils sont distincts des niveaux de maturité (§3).", "",
        "| Niveau | Notion | Code | Test | Benchmark | Commit | Statut |",
        "|---|---|---|---|---|---|---|",
    ]
    for level, notion, code, tests, cases in ROWS:
        c = "<br>".join(symbol_link(r, p, pat, lab) for r, p, pat, lab in code)
        t = "<br>".join(test(x) for x in tests) or "—"
        b = "<br>".join(case(x) for x in cases) or "—"
        repos = sorted({r for r, *_ in code} | ({"tsa"} if any(x.startswith("TSA") for x in tests) else set()))
        lines.append(f"| {level} | {notion} | {c} | {t} | {b} | {' '.join(commit[r] for r in repos)} | vérifié (lien, lignes) ; origine : commit de référence |")
    lines += [
        "", "## 3. Matrice des niveaux de maturité", "",
        "Définitions : [MATURITY_LEVELS.md](../validation/MATURITY_LEVELS.md). Une ligne sans preuve n'est pas satisfaite.", "",
        "| Niveau | Critère | Preuves | Statut |", "|---|---|---|---|",
    ]
    for level, criterion, proofs in MATURITY:
        cells, status = [], "satisfait"
        for kind, target in proofs:
            if kind == "test":
                cells.append(test(target))
            elif kind == "cas":
                cells.append("cas " + case(target))
            elif kind in ("code", "validate"):
                cells.append(symbol_link(*target, label=target[2].strip("^")))
            elif kind == "doc":
                cells.append(f"[rapport d'audit]({target})")
            else:
                cells.append(target)
                status = "**non satisfait**"
        if level == 5 and status == "satisfait":
            status = "satisfait (jalon partiel)"
        lines.append(f"| {level} | {criterion} | {'<br>'.join(cells)} | {status} |")
    lines += [
        "", "## 4. Tous les tests et cas", "",
        "| Test ou cas | Lien (révision de référence) |", "|---|---|",
    ]
    for k in sorted(x for x in a if not x.startswith(("cas ", "nom ", "TSA"))):
        lines.append(f"| {k} | {test(k)} |")
    for n in sorted(x[4:] for x in a if x.startswith("cas ")):
        lines.append(f"| Cas OpenSees {case(n)} | [ligne de définition]({a['cas ' + n]}) |")
    for k in sorted(x for x in a if x.startswith("TSA test 47") or x == "TSA test 480"):
        lines.append(f"| {k} | [ligne du test]({a[k]}) |")
    lines += ["", "## 5. Comment vérifier", "",
              "```bat", "python tools/docs/gitlinks.py check        :: chaque lien : commit, fichier, lignes",
              "python tools/docs/gen_traceability.py --check", "```", "",
              "Le dépôt TSA doit être cloné à côté de celui-ci (`../TSA`) pour vérifier ses liens ; sinon ils sont signalés",
              "« non vérifiables ici ».", ""]
    return "\n".join(lines)


if __name__ == "__main__":
    text = render()
    if "--check" in sys.argv:
        cur = OUT.read_text(encoding="utf-8").replace("\r\n", "\n") if OUT.exists() else ""
        if cur != text:
            sys.exit("Matrice de traçabilité obsolète : relancer python tools/docs/gen_traceability.py")
        print("Matrice de traçabilité à jour.")
    else:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_text(text, encoding="utf-8", newline="\n")
        print(f"Écrit : {OUT.relative_to(ROOT)}")
