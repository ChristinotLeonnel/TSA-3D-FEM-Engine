"""Vérifie que tous les diagrammes Mermaid de la documentation se rendent sans erreur.

  python tools/docs/check_mermaid.py [<dossier>...]

Chaque bloc ```mermaid des fichiers Markdown est extrait et rendu en SVG par mermaid-cli (mmdc). L'exécutable est pris
dans la variable d'environnement MMDC, sinon « mmdc » du PATH (npm install -g @mermaid-js/mermaid-cli).
Code de sortie : nombre de diagrammes en erreur.
"""

import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
BLOCK = re.compile(r"^```mermaid[ \t]*\r?\n(.*?)^```", re.S | re.M)


def main(targets) -> int:
    mmdc = os.environ.get("MMDC") or shutil.which("mmdc")
    if not mmdc:
        sys.exit("mmdc introuvable : définir MMDC ou installer @mermaid-js/mermaid-cli")
    files = []
    for t in targets or [ROOT / "docs"]:
        p = pathlib.Path(t)
        files += sorted(p.rglob("*.md")) if p.is_dir() else [p]
    errors, count = 0, 0
    with tempfile.TemporaryDirectory() as tmp:
        for f in files:
            for n, m in enumerate(BLOCK.finditer(f.read_text(encoding="utf-8")), 1):
                count += 1
                src = pathlib.Path(tmp) / f"d{count}.mmd"
                src.write_text(m.group(1), encoding="utf-8")
                r = subprocess.run([mmdc, "-q", "-i", str(src), "-o", str(src.with_suffix(".svg"))],
                                   capture_output=True, text=True, encoding="utf-8", errors="replace", shell=mmdc.endswith(".cmd"))
                if r.returncode != 0 or not src.with_suffix(".svg").exists():
                    errors += 1
                    detail = (r.stderr or r.stdout).strip().splitlines()
                    shown = f.relative_to(ROOT) if f.is_relative_to(ROOT) else f
                    print(f"ERREUR {shown} (diagramme {n}) : {' | '.join(detail[:3])}")
    print(f"{count} diagramme(s) Mermaid rendu(s), {errors} en erreur")
    return errors


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
