#!/usr/bin/env python3
"""Genera docs/index.html (GitHub Pages) a partir de web/index.src.html.

Incrusta en la página todos los archivos de texto del repositorio para que la
pestaña «Código» los muestre sin depender de la red.

Uso:
    python3 tools/build_site.py           # genera docs/index.html
    python3 tools/build_site.py --check   # falla si docs/index.html no está al día (CI)
"""
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "web" / "index.src.html"
OUT = ROOT / "docs" / "index.html"
MARK = "<!--@REPO_FILES@-->"

TEXT_EXT = {".ino", ".h", ".cpp", ".c", ".md", ".csv", ".py", ".yml", ".yaml", ".json", ".txt"}
TEXT_NAMES = {"LICENSE", ".gitignore"}
SKIP_DIRS = {".git", "docs", "node_modules", "build", "__pycache__", ".pio", ".vscode"}
# La propia página es muy grande para incrustarla dentro de sí misma:
# la pestaña «Código» la descarga de GitHub cuando se abre.
LAZY = {"web/index.src.html"}


def repo_files():
    files = []
    for p in sorted(ROOT.rglob("*")):
        rel = p.relative_to(ROOT)
        if not p.is_file() or any(part in SKIP_DIRS for part in rel.parts):
            continue
        key = rel.as_posix()
        if key in LAZY:
            files.append({"path": key, "lazy": True, "size": p.stat().st_size})
            continue
        if p.suffix.lower() not in TEXT_EXT and p.name not in TEXT_NAMES:
            continue
        text = p.read_text(encoding="utf-8").replace("\r\n", "\n")
        files.append({"path": key, "text": text, "lines": len(text.split("\n"))})
    return files


def build():
    src = SRC.read_text(encoding="utf-8")
    if src.count(MARK) != 1:
        sys.exit(f"{SRC}: falta el marcador {MARK} (o está repetido)")
    data = json.dumps(repo_files(), ensure_ascii=False, separators=(",", ":"))
    data = data.replace("</", "<\\/")  # que ningún archivo cierre el <script> antes de tiempo
    block = f'<script type="application/json" id="repoFiles">{data}</script>'
    return src.replace(MARK, block)


def main():
    html = build()
    if "--check" in sys.argv:
        current = OUT.read_text(encoding="utf-8") if OUT.exists() else ""
        if current != html:
            sys.exit("docs/index.html no está al día. Corre: python3 tools/build_site.py")
        print("docs/index.html está al día")
        return
    OUT.parent.mkdir(parents=True, exist_ok=True)
    (OUT.parent / ".nojekyll").touch()
    OUT.write_text(html, encoding="utf-8")
    print(f"Escrito {OUT.relative_to(ROOT)} ({len(html) // 1024} KB)")


if __name__ == "__main__":
    main()
