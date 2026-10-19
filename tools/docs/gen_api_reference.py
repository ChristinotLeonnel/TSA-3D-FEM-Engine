"""Génère la référence de l'API publique (en-têtes include/Fem3D/) en Markdown : docs/reference/API.md.

  python tools/docs/gen_api_reference.py          écrit docs/reference/API.md
  python tools/docs/gen_api_reference.py --check  échoue si le fichier versionné ne correspond plus aux en-têtes

Pour chaque en-tête : son commentaire de tête, ses types (struct, class, enum) et les déclarations documentées par
« /// » (au-dessus) ou « ///< » (en fin de ligne). Lien vers l'en-tête à la révision de référence de la documentation
(tools/docs/gitlinks.py) s'il n'a pas changé depuis ; sinon, la page le signale.
"""

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "docs"))
from gitlinks import REFERENCE_COMMITS, REPOS  # noqa: E402

OUT = ROOT / "docs" / "reference" / "API.md"
TYPE = re.compile(r"^\s*(?:template\s*<[^>]*>\s*)?(struct|class|enum class|enum)\s+(\w+)")
DECL = re.compile(r"^\s*(?!//)(?!return\b)(?!if\b)(?!for\b)[\w:<>,~\*&\s\[\]\"=().\-+/']*\w\s*\(.*\)\s*(?:const)?\s*(?:override)?\s*(?:noexcept)?\s*(?:=\s*\w+)?\s*(?:[;{].*)?$")
FIELD = re.compile(r"^\s*(?!//)[\w:<>,\s\*&]+\s+\w+(?:\s*=\s*[^;]+|\{[^;]*\})?;\s*///<")


def unchanged_since_reference(rel: str) -> bool:
    r = subprocess.run(["git", "-C", str(ROOT), "diff", "--quiet", REFERENCE_COMMITS["fem3d"], "--", rel])
    return r.returncode == 0


def header_comment(lines):
    out = []
    for line in lines:
        s = line.strip()
        if s.startswith("#pragma"):
            continue
        if s.startswith("//"):
            out.append(s.lstrip("/").strip())
        elif s == "" and not out:
            continue
        else:
            break
    return " ".join(x for x in out if x)


def clean(sig: str) -> str:
    sig = sig.split("///")[0].strip()
    if not sig.endswith(";") and "{" in sig:
        sig = sig[: sig.rindex("{")].strip() + " { … }"   # corps de fonction en ligne
    return re.sub(r"\s+", " ", sig).replace("|", "\\|")


def render() -> str:
    slug, rev = REPOS["fem3d"]["slug"], REFERENCE_COMMITS["fem3d"]
    headers = sorted((ROOT / "include" / "Fem3D").rglob("*.h"))
    out = ["# Référence de l'API (générée)", "",
           "Générée par `python tools/docs/gen_api_reference.py` à partir des en-têtes publics `include/Fem3D/` :",
           "**ne pas modifier à la main**. Espace de noms : `TSA::Fem3D`. Les explications sont dans le",
           "[livre](../book/Fem3D_Technical_Book.md) et le [guide développeur](../developer/DEVELOPER_GUIDE.md).", ""]
    current_module = None
    for h in headers:
        rel = h.relative_to(ROOT).as_posix()
        module = rel.split("/")[2]
        if module != current_module:
            out += [f"## {module}", ""]
            current_module = module
        lines = h.read_text(encoding="utf-8").splitlines()
        link = (f"[{rel}](https://github.com/{slug}/blob/{rev}/{rel})" if unchanged_since_reference(rel)
                else f"`{rel}` (modifié depuis la révision de référence {rev[:7]})")
        out += [f"### `{h.name}`", "", link, ""]
        desc = header_comment(lines)
        if desc:
            out += [desc, ""]
        rows, pending, hidden = [], [], False
        for i, line in enumerate(lines):
            s = line.strip()
            if s in ("private:", "protected:"):
                hidden = True
                continue
            if s == "public:":
                hidden = False
                continue
            if s.startswith("///") and not s.startswith("///<"):
                pending.append(s[3:].strip())
                continue
            t = TYPE.match(line)
            if t and not s.endswith(";"):
                hidden = t.group(1) == "class"          # class : privé jusqu'à « public: »
                rows.append((f"**{t.group(1)} {t.group(2)}**", " ".join(pending)))
            elif hidden:
                pass
            elif "///<" in line and (FIELD.match(line) or DECL.match(line.split("///<")[0] + ";")):
                rows.append((f"`{clean(line)}`", line.split("///<", 1)[1].strip()))
            elif pending and DECL.match(line):
                rows.append((f"`{clean(line)}`", " ".join(pending)))
            pending = [] if not s.startswith("///") else pending
        if rows:
            out += ["| Déclaration | Description |", "|---|---|"]
            out += [f"| {a} | {b.replace('|', '\\|') or '—'} |" for a, b in rows]
            out.append("")
    return "\n".join(out).rstrip() + "\n"


if __name__ == "__main__":
    text = render()
    if "--check" in sys.argv:
        current = OUT.read_text(encoding="utf-8").replace("\r\n", "\n") if OUT.exists() else ""
        if current != text:
            sys.exit("Référence API obsolète : relancer python tools/docs/gen_api_reference.py")
        print("Référence API à jour.")
    else:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_text(text, encoding="utf-8", newline="\n")
        print(f"Écrit : {OUT.relative_to(ROOT)}")
