"""Permaliens GitHub vérifiés pour la documentation de Fem3D.

Deux usages :

  python tools/docs/gitlinks.py permalink <fichier> [<motif>] [--repo fem3d|tsa] [--commit <hash>]
      Affiche le lien GitHub du fichier à la révision donnée ; avec un motif (expression régulière), le lien pointe
      sur les lignes de la première définition qui y correspond (accolades équilibrées : corps de fonction, classe).

  python tools/docs/gitlinks.py check [<dossier ou fichier>...]
      Vérifie tous les liens GitHub des fichiers Markdown : commit existant, fichier présent à cette révision, plage de
      lignes valide. Les liens vers un autre dépôt sont vérifiés si son clone local est trouvé (../TSA), sinon signalés
      « non vérifiables ici ». Code de sortie : nombre de liens invalides.

Seul Git local est interrogé (git cat-file, git show) : aucune requête réseau.
"""

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
REPOS = {
    "fem3d": {"slug": "ChristinotLeonnel/TSA-3D-FEM-Engine", "path": ROOT},
    "tsa": {"slug": "ChristinotLeonnel/Tsaraloha-Structural-Analysis", "path": ROOT.parent / "TSA"},
}
# Révisions de référence de la documentation (docs/traceability/GIT_TRACEABILITY.md).
REFERENCE_COMMITS = {
    "fem3d": "75700d2bfd33c29b2fadc795fd3051f25399ad19",
    "tsa": "2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d",
}
LINK = re.compile(r"https://github\.com/(?P<slug>[\w.-]+/[\w.-]+)/(?P<kind>blob|tree|commit)/(?P<rev>[0-9a-f]{7,40})"
                  r"(?:/(?P<path>[^)\s#>\"'`]*))?(?:#L(?P<a>\d+)(?:-L(?P<b>\d+))?)?")


def git(repo: pathlib.Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True, encoding="utf-8", errors="replace")


def file_at(repo: pathlib.Path, rev: str, path: str):
    r = git(repo, "show", f"{rev}:{path}")
    return r.stdout.splitlines() if r.returncode == 0 else None


def definition_range(lines, pattern: str):
    rx = re.compile(pattern)
    for i, line in enumerate(lines):
        if rx.search(line):
            depth, opened = 0, False
            for j in range(i, len(lines)):
                code = re.sub(r'"(?:\\.|[^"\\])*"', "", lines[j].split("//")[0])
                depth += code.count("{") - code.count("}")
                opened = opened or "{" in code
                if opened and depth <= 0:
                    return i + 1, j + 1
                if not opened and code.rstrip().endswith(";"):
                    return i + 1, j + 1
            return i + 1, i + 1
    return None


def permalink(path: str, pattern=None, repo="fem3d", commit=None) -> str:
    info, rev = REPOS[repo], commit or REFERENCE_COMMITS[repo]
    lines = file_at(info["path"], rev, path)
    if lines is None:
        sys.exit(f"{path} absent de {repo}@{rev[:7]}")
    url = f"https://github.com/{info['slug']}/blob/{rev}/{path}"
    if pattern:
        rng = definition_range(lines, pattern)
        if not rng:
            sys.exit(f"motif « {pattern} » introuvable dans {path}@{rev[:7]}")
        url += f"#L{rng[0]}-L{rng[1]}"
    return url


def check(targets) -> int:
    files = []
    for t in targets or [ROOT / "docs", ROOT / "README.md"]:
        p = pathlib.Path(t).resolve()
        files += sorted(p.rglob("*.md")) if p.is_dir() else [p]
    slugs = {v["slug"].lower(): k for k, v in REPOS.items()}
    bad, unverifiable, ok = [], [], 0
    for f in files:
        if "_downloads" in f.parts:
            continue
        text = f.read_text(encoding="utf-8")
        for m in LINK.finditer(text):
            repo = slugs.get(m["slug"].lower())
            where = f"{f.relative_to(ROOT)} : {m.group(0)}"
            if repo is None:
                continue                      # dépôt étranger (OpenSees…) : hors du contrôle Git local
            local = REPOS[repo]["path"]
            if not (local / ".git").exists():
                unverifiable.append(where)
                continue
            rev = m["rev"]
            if git(local, "cat-file", "-e", f"{rev}^{{commit}}").returncode != 0:
                bad.append(f"{where} — commit inconnu")
                continue
            if len(rev) < 40:
                bad.append(f"{where} — hash abrégé (utiliser les 40 caractères)")
                continue
            if m["kind"] == "commit" or not m["path"]:
                ok += 1
                continue
            path = m["path"].rstrip("/")
            kind = "tree" if m["kind"] == "tree" else "blob"
            if git(local, "cat-file", "-e", f"{rev}:{path}").returncode != 0:
                bad.append(f"{where} — fichier absent à cette révision")
                continue
            if m["a"]:
                lines = file_at(local, rev, path) or []
                a, b = int(m["a"]), int(m["b"] or m["a"])
                if not (1 <= a <= b <= len(lines)):
                    bad.append(f"{where} — lignes hors du fichier ({len(lines)} lignes)")
                    continue
            ok += 1
    for u in unverifiable:
        print("NON VÉRIFIABLE ICI :", u)
    for b in bad:
        print("INVALIDE :", b)
    print(f"{ok} lien(s) GitHub vérifié(s), {len(bad)} invalide(s), {len(unverifiable)} non vérifiable(s) localement")
    return len(bad)


if __name__ == "__main__":
    args = sys.argv[1:]
    if not args or args[0] not in ("permalink", "check"):
        sys.exit(__doc__)
    if args[0] == "check":
        sys.exit(check(args[1:]))
    rest, opts, it = [], {}, iter(args[1:])
    for a in it:
        if a.startswith("--"):
            opts[a] = next(it, None)          # option suivie de sa valeur
        else:
            rest.append(a)
    print(permalink(rest[0], rest[1] if len(rest) > 1 else None, opts.get("--repo", "fem3d"), opts.get("--commit")))
