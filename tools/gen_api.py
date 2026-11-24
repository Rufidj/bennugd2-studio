#!/usr/bin/env python3
"""Genera el catalogo de la API de BennuGD2/libmod_3d a partir de los *_exports.h.

Salidas:
  src/api_catalogo.h   tabla C++ embebida en el editor (autocompletado, firma, ayuda)
  docs/API.md          referencia legible, agrupada por modulo y categoria

Las descripciones NO se inventan: viven en tools/api_docs.json
  { "G3D_SPRITE_CREATE": { "desc": "...", "params": ["modelo","x","y","z"],
                           "ret": "id del sprite", "ejemplo": "..." } }
Lo que no tenga entrada sale solo con su firma (tipos). Para ver que falta
por documentar:  gen_api.py --pendientes
"""
import json, re, sys, glob, os

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EDIT = os.path.join(RAIZ, "editor3d")

TIPO_PARAM = {"I": "int", "D": "double", "F": "float", "S": "string",
              "P": "pointer", "W": "word", "B": "byte", "V": "..."}
TIPO_RET = {"TYPE_INT": "int", "TYPE_DWORD": "int", "TYPE_FLOAT": "float",
            "TYPE_DOUBLE": "double", "TYPE_STRING": "string",
            "TYPE_POINTER": "pointer", "TYPE_BYTE": "byte", "TYPE_QWORD": "int64",
            "TYPE_UNDEFINED": "", "0": ""}

RE_FUNC = re.compile(r'^\s*FUNC\s*\(\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*([A-Za-z_0-9]+)')


def categoria(nombre, modulo):
    """G3D_SPRITE_CREATE -> 'sprite'; el resto, 'general'."""
    p = nombre.split("_")
    if modulo == "3d" and p[0] == "G3D" and len(p) > 2:
        return p[1].lower()
    return "general"


def leer():
    funcs = []
    for f in sorted(glob.glob(os.path.join(RAIZ, "libmod_*", "libmod_*_exports.h"))):
        modulo = re.search(r"libmod_([a-z0-9]+)_exports", f).group(1)
        for linea in open(f, encoding="utf-8", errors="replace"):
            if linea.lstrip().startswith("//"):
                continue
            m = RE_FUNC.match(linea)
            if not m:
                continue
            nombre, params, ret = m.groups()
            funcs.append({
                "nombre": nombre, "modulo": modulo, "sig": params,
                "params": [TIPO_PARAM.get(c, "?") for c in params],
                "ret": TIPO_RET.get(ret, ""),
                "cat": categoria(nombre, modulo),
            })
    return funcs


def doc_de(docs, f):
    """Doc de una funcion: primero la de su firma exacta (NOMBRE(II)), luego la general (NOMBRE)."""
    return docs.get("%s(%s)" % (f["nombre"], f["sig"]), docs.get(f["nombre"], {}))


def firma(f, d):
    nombres = (d or {}).get("params", [])
    ps = []
    for i, t in enumerate(f["params"]):
        n = nombres[i] if i < len(nombres) else ""
        ps.append((t + " " + n).strip())
    return "%s(%s)%s" % (f["nombre"].lower(), ", ".join(ps),
                         (" -> " + f["ret"]) if f["ret"] else "")


def c_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def main():
    docs = {}
    ruta_docs = os.path.join(EDIT, "tools", "api_docs.json")
    if os.path.exists(ruta_docs):
        docs = json.load(open(ruta_docs, encoding="utf-8"))
    funcs = leer()
    nombres = {f["nombre"] for f in funcs}
    for k in docs:
        if k.split("(")[0] not in nombres:
            print("AVISO: api_docs.json documenta '%s', que ya no existe" % k, file=sys.stderr)

    if "--pendientes" in sys.argv:
        for f in funcs:
            if f["nombre"] not in docs and "%s(%s)" % (f["nombre"], f["sig"]) not in docs:
                print(f["modulo"], f["nombre"])
        return

    # ---- cabecera C++ ----
    out = ["// GENERADO por tools/gen_api.py. No editar a mano.",
           "#pragma once",
           "struct ApiFunc { const char *nombre, *modulo, *cat, *firma, *desc, *ejemplo; };",
           "static const ApiFunc API_FUNCS[] = {"]
    for f in funcs:
        d = doc_de(docs, f)
        out.append("  {%s,%s,%s,%s,%s,%s}," % (
            c_str(f["nombre"].lower()), c_str(f["modulo"]), c_str(f["cat"]),
            c_str(firma(f, d)), c_str(d.get("desc", "")), c_str(d.get("ejemplo", ""))))
    out.append("};")
    out.append("static const int API_FUNCS_N = sizeof(API_FUNCS)/sizeof(API_FUNCS[0]);")
    open(os.path.join(EDIT, "src", "api_catalogo.h"), "w", encoding="utf-8").write("\n".join(out) + "\n")

    # ---- Markdown ----
    md = ["# Referencia de la API", "",
          "Generada automaticamente desde los modulos del motor (%d funciones, %d documentadas)." %
          (len(funcs), sum(1 for f in funcs if f["nombre"] in docs or "%s(%s)" % (f["nombre"], f["sig"]) in docs)), ""]
    por_mod = {}
    for f in funcs:
        por_mod.setdefault(f["modulo"], {}).setdefault(f["cat"], []).append(f)
    for mod in sorted(por_mod):
        md.append("## Modulo %s" % mod)
        for cat in sorted(por_mod[mod]):
            md.append("### %s" % cat)
            for f in por_mod[mod][cat]:
                d = doc_de(docs, f)
                md.append("#### `%s`" % firma(f, d))
                if d.get("desc"):
                    md.append(d["desc"])
                if d.get("ret"):
                    md.append("\n**Devuelve:** " + d["ret"])
                if d.get("ejemplo"):
                    md += ["", "```", d["ejemplo"], "```"]
                md.append("")
    open(os.path.join(EDIT, "docs", "API.md"), "w", encoding="utf-8").write("\n".join(md) + "\n")
    print("%d funciones, %d documentadas" % (len(funcs), sum(1 for f in funcs if f["nombre"] in docs or "%s(%s)" % (f["nombre"], f["sig"]) in docs)))


if __name__ == "__main__":
    main()
