"""Génère le diagramme des dépendances réelles entre modules C++ de Fem3D à partir des #include.

  python tools/docs/gen_dependency_graph.py          écrit docs/diagrams/dependances-code.md (Mermaid, modules)
                                                     et docs/diagrams/dependances-code.dot (Graphviz, fichiers)
  python tools/docs/gen_dependency_graph.py --check  échoue si les fichiers versionnés ne correspondent plus au code

Un module est un dossier de include/Fem3D/ (Core, Geometry, Elements…) ; les sources de src/<Module>/ en font partie.
Seules les inclusions internes « Fem3D/… » sont prises en compte (Eigen et la bibliothèque standard sont omis).
"""

import pathlib
import re
import sys
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT_MD = ROOT / "docs" / "diagrams" / "dependances-code.md"
OUT_DOT = ROOT / "docs" / "diagrams" / "dependances-code.dot"
INCLUDE = re.compile(r'^\s*#\s*include\s+"Fem3D/([^"]+)"', re.M)


def module_of(path: pathlib.Path) -> str:
    rel = path.relative_to(ROOT).as_posix()
    if rel.startswith("include/Fem3D/"):
        return rel.split("/")[2]
    return rel.split("/")[1]          # src/<Module>/…


def scan():
    files = sorted(list((ROOT / "include" / "Fem3D").rglob("*.h")) + list((ROOT / "src").rglob("*.cpp")))
    file_edges, module_edges, modules = set(), defaultdict(set), set()
    for f in files:
        src_mod = module_of(f)
        modules.add(src_mod)
        name = f.relative_to(ROOT).as_posix()
        for inc in INCLUDE.findall(f.read_text(encoding="utf-8")):
            target = "include/Fem3D/" + inc
            file_edges.add((name, target))
            dst_mod = inc.split("/")[0]
            modules.add(dst_mod)
            if dst_mod != src_mod:
                module_edges[src_mod].add(dst_mod)
    return files, file_edges, module_edges, modules


def render():
    files, file_edges, module_edges, modules = scan()
    lines = ["# Dépendances du code (généré)", "",
             "Généré par `python tools/docs/gen_dependency_graph.py` à partir des `#include \"Fem3D/…\"` de `include/` et",
             "`src/` : **ne pas modifier à la main**. Une flèche A → B signifie « un fichier du module A inclut un en-tête du",
             "module B ». Le graphe des fichiers est dans `dependances-code.dot` (Graphviz :",
             "`dot -Tsvg dependances-code.dot -o dependances-code.svg`).", "",
             f"{len(files)} fichiers, {len(modules)} modules, {sum(len(v) for v in module_edges.values())} dépendances entre modules,",
             f"{len(file_edges)} inclusions internes.", "", "```mermaid", "flowchart BT"]
    for m in sorted(modules):
        lines.append(f"    {m}[\"{m}\"]")
    for src in sorted(module_edges):
        for dst in sorted(module_edges[src]):
            lines.append(f"    {src} --> {dst}")
    lines += ["```", "", "| Module | Dépend de |", "|---|---|"]
    for m in sorted(modules):
        deps = ", ".join(sorted(module_edges.get(m, []))) or "—"
        lines.append(f"| {m} | {deps} |")
    md = "\n".join(lines) + "\n"
    dot = ["// Généré par tools/docs/gen_dependency_graph.py : ne pas modifier à la main.",
           "digraph fem3d {", "  rankdir=BT;", "  node [shape=box, fontsize=10];"]
    by_module = defaultdict(list)
    for f in files:
        by_module[module_of(f)].append(f.relative_to(ROOT).as_posix())
    for m in sorted(by_module):
        dot.append(f'  subgraph "cluster_{m}" {{ label="{m}";')
        for n in sorted(by_module[m]):
            dot.append(f'    "{n}";')
        dot.append("  }")
    for a, b in sorted(file_edges):
        dot.append(f'  "{a}" -> "{b}";')
    dot.append("}")
    return md, "\n".join(dot) + "\n"


if __name__ == "__main__":
    md, dot = render()
    if "--check" in sys.argv:
        current = (OUT_MD.read_text(encoding="utf-8").replace("\r\n", "\n") if OUT_MD.exists() else "",
                   OUT_DOT.read_text(encoding="utf-8").replace("\r\n", "\n") if OUT_DOT.exists() else "")
        if current != (md, dot):
            sys.exit("Diagramme des dépendances obsolète : relancer python tools/docs/gen_dependency_graph.py")
        print("Diagramme des dépendances à jour.")
    else:
        OUT_MD.parent.mkdir(parents=True, exist_ok=True)
        OUT_MD.write_text(md, encoding="utf-8", newline="\n")
        OUT_DOT.write_text(dot, encoding="utf-8", newline="\n")
        print(f"Écrit : {OUT_MD.relative_to(ROOT)}, {OUT_DOT.relative_to(ROOT)}")
