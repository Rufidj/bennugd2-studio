#!/usr/bin/env python3
"""Compila con bgdc cada bloque ```bennugd compila de las guias. Sale con error si alguno falla."""
import glob, os, re, subprocess, sys, tempfile
AQUI = os.path.dirname(os.path.abspath(__file__))
# Prefiere el bgdc recien compilado del arbol de build (lleva el modulo 3D actual); si no existe, el de pruebas/
BUILD = os.path.abspath(os.path.join(AQUI, "..", "..", "..", "build", "linux-gnu", "bin"))
PRUEBAS = BUILD if os.path.exists(os.path.join(BUILD, "bgdc")) else os.path.join(AQUI, "..", "pruebas")
fallos = 0; total = 0
for f in sorted(glob.glob(os.path.join(AQUI, "guias", "*.md"))):
    txt = open(f, encoding="utf-8").read()
    for i, m in enumerate(re.finditer(r"```bennugd compila\n(.*?)```", txt, re.S)):
        total += 1
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "ej.prg"); open(p, "w", encoding="utf-8").write(m.group(1))
            r = subprocess.run(["./bgdc", p], cwd=PRUEBAS, capture_output=True, text=True,
                               env={**os.environ, "LD_LIBRARY_PATH": "."})
            ok = r.returncode == 0 and os.path.exists(os.path.join(d, "ej.dcb"))
            print(("OK   " if ok else "FALLA"), os.path.basename(f), "bloque", i + 1)
            if not ok:
                fallos += 1; print((r.stdout + r.stderr).strip()[-600:])
print("%d/%d compilan" % (total - fallos, total)); sys.exit(1 if fallos else 0)
