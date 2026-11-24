#!/usr/bin/env python3
"""Pasa la carretera de los .kart antiguos a la escena a la que apuntan (lineas ROAD / ROADNODE).

Uso:  tools/kart2road.py <carpeta Scenes> [--aplicar]

Sin --aplicar solo cuenta lo que haria. Con --aplicar escribe las lineas al final de la escena (antes copia
<escena>.bak) y NO toca el .kart. Una escena que ya tiene lineas ROAD se salta. Solo se pasa el trazado (nodos, ancho,
peralte, tangentes, altura y superficie): todo lo demas de los karts (salida, rampas, turbos, pilotos...) se descarta.
"""
import os, sys, shutil

def leer_kart(ruta):
    escena, caminos = "", []
    for ln in open(ruta, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        if not ln or ln.startswith("#") or "=" not in ln:
            continue
        k, v = ln.split("=", 1)
        if k == "escena":
            escena = v.strip()
        elif k == "camino":
            p = v.split()
            caminos.append({"cerrado": int(p[1]) if len(p) > 1 else 1, "nodos": []})
        elif k == "nodo":
            if not caminos:
                caminos.append({"cerrado": 1, "nodos": []})
            p = v.split()
            if len(p) < 13:
                continue
            x, z, y, ancho, per, tx, tz = (float(q) for q in p[:7])
            sup = min(max(int(p[8]), 0), 3)
            sigue, dy = (int(p[13]), float(p[14])) if len(p) >= 15 else (0, 0.3)
            caminos[-1]["nodos"].append((x, z, y, ancho, per, sigue, dy, sup, tx, tz))
    return escena, caminos

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 1
    carpeta, aplicar = sys.argv[1], "--aplicar" in sys.argv
    proyecto = os.path.dirname(os.path.abspath(carpeta.rstrip("/")))
    for f in sorted(os.listdir(carpeta)):
        if not f.endswith(".kart"):
            continue
        escena, caminos = leer_kart(os.path.join(carpeta, f))
        caminos = [c for c in caminos if len(c["nodos"]) >= 2]
        if not escena or not caminos:
            print("%s: sin escena vinculada o sin trazado: nada que pasar" % f); continue
        dest = os.path.join(proyecto, escena)
        if not os.path.exists(dest):
            print("%s: la escena %s no existe" % (f, escena)); continue
        if any(l.startswith("ROAD ") for l in open(dest, encoding="utf-8", errors="replace")):
            print("%s: %s ya tiene carretera: se salta" % (f, escena)); continue
        nn = sum(len(c["nodos"]) for c in caminos)
        print("%s -> %s: %d carretera(s), %d nodos%s" % (f, escena, len(caminos), nn, "" if aplicar else "   (simulacion)"))
        if not aplicar:
            continue
        shutil.copy2(dest, dest + ".bak")
        with open(dest, "a", encoding="utf-8") as o:
            for c in caminos:
                o.write("ROAD %d\n" % c["cerrado"])
                for n in c["nodos"]:
                    o.write("ROADNODE %.3f %.3f %.3f %.3f %.3f %d %.3f %d %.3f %.3f\n" % n)
    return 0

if __name__ == "__main__":
    sys.exit(main())
