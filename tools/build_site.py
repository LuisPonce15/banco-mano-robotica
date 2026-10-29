#!/usr/bin/env python3
"""Genera las páginas de GitHub Pages a partir de web/*.src.html.

  web/index.src.html  →  docs/index.html       (banco E-01, dedo)
  web/fsr.src.html    →  docs/fsr/index.html   (banco FSR)

En cada página incrusta los archivos de texto del repositorio que le
corresponden (pestaña «Código»), las fotos (data: URI) y los scripts
compartidos de web/comun/, para que funcione sin depender de la red.

Uso:
    python3 tools/build_site.py           # genera las páginas
    python3 tools/build_site.py --check   # falla si alguna no está al día (CI)
"""
import base64
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

TEXT_EXT = {".ino", ".h", ".cpp", ".c", ".md", ".csv", ".py", ".yml", ".yaml", ".json", ".txt", ".js"}
TEXT_NAMES = {"LICENSE", ".gitignore"}
SKIP_DIRS = {".git", "docs", "node_modules", "build", "__pycache__", ".pio", ".vscode"}
IMG_EXT = {".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".png": "image/png", ".svg": "image/svg+xml"}

PAGES = [
    {
        "src": "web/index.src.html",
        "out": "docs/index.html",
        # todo menos el banco FSR
        "include": lambda p: not p.startswith("fsr/") and p not in ("web/fsr.src.html",) and not p.startswith("web/comun/"),
        "lazy": {"web/index.src.html"},
        "images": [],
    },
    {
        "src": "web/fsr.src.html",
        "out": "docs/fsr/index.html",
        "include": lambda p: p.startswith("fsr/") or p.startswith("web/comun/") or p.startswith(".github/")
        or p in ("tools/build_site.py", "web/fsr.src.html"),
        "lazy": {"web/fsr.src.html"},
        "images": ["fsr/fotos/prensa.jpg", "fsr/fotos/cupula-silicona.jpg"],
    },
]


def all_files():
    for p in sorted(ROOT.rglob("*")):
        rel = p.relative_to(ROOT)
        if p.is_file() and not any(part in SKIP_DIRS for part in rel.parts):
            yield rel.as_posix(), p


def repo_files(page):
    files = []
    for key, p in all_files():
        if not page["include"](key):
            continue
        if key in page["lazy"]:
            files.append({"path": key, "lazy": True, "size": p.stat().st_size})
            continue
        if p.suffix.lower() not in TEXT_EXT and p.name not in TEXT_NAMES:
            continue
        text = p.read_text(encoding="utf-8").replace("\r\n", "\n")
        files.append({"path": key, "text": text, "lines": len(text.split("\n"))})
    return files


def script_json(id_, data):
    s = json.dumps(data, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    return f'<script type="application/json" id="{id_}">{s}</script>'


def build(page):
    src = (ROOT / page["src"]).read_text(encoding="utf-8")
    if src.count("<!--@REPO_FILES@-->") != 1:
        sys.exit(f"{page['src']}: falta el marcador <!--@REPO_FILES@--> (o está repetido)")

    def images():
        out = {}
        for rel in page["images"]:
            p = ROOT / rel
            out[rel] = f"data:{IMG_EXT[p.suffix.lower()]};base64," + base64.b64encode(p.read_bytes()).decode()
        return out

    def repl(m):
        tag = m.group(1)
        if tag == "REPO_FILES":
            return script_json("repoFiles", repo_files(page))
        if tag == "REPO_IMAGES":
            return script_json("repoImages", images())
        code = (ROOT / tag.split(" ", 1)[1]).read_text(encoding="utf-8")
        return "<script>\n" + code.replace("</script", "<\\/script") + "\n</script>"

    # Una sola pasada: el texto incrustado (por ejemplo este mismo script) puede
    # contener los marcadores y no debe volver a reemplazarse.
    return re.sub(r"<!--@(REPO_FILES|REPO_IMAGES|INCLUDE [\w./-]+)@-->", repl, src)


def main():
    check = "--check" in sys.argv
    stale = []
    for page in PAGES:
        html = build(page)
        out = ROOT / page["out"]
        if check:
            if not out.exists() or out.read_text(encoding="utf-8") != html:
                stale.append(page["out"])
            continue
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(html, encoding="utf-8")
        print(f"Escrito {page['out']} ({len(html) // 1024} KB)")
    (ROOT / "docs" / ".nojekyll").touch()
    if check:
        if stale:
            sys.exit("No están al día: " + ", ".join(stale) + ". Corre: python3 tools/build_site.py")
        print("Las páginas están al día")


if __name__ == "__main__":
    main()
