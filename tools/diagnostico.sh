#!/bin/sh
# Diagnostico de bennugd2-studio: por que falla con
#   "undefined symbol: g3d_zone_load"
#
# Se puede ejecutar DESDE CUALQUIER SITIO: las rutas se resuelven a partir de la
# ubicacion de este script, no del directorio actual.

SELF=$(cd "$(dirname "$0")" 2>/dev/null && pwd)
ED=$(dirname "$SELF")          # raiz del editor  (tools/ -> editor3d)
LM3D="$ED/../libmod_3d"        # fuentes del modulo 3D (repo hermano)

echo "Editor    : $ED"
echo "libmod_3d : $LM3D"
echo

echo "===== 1) FUENTES ====="
if [ -f "$LM3D/libmod_3d_zone.c" ]; then
    echo "  OK  existe libmod_3d_zone.c"
else
    echo "  MAL no existe $LM3D/libmod_3d_zone.c"
    echo "      -> ese libmod_3d no es el que necesita el editor"
    echo "      -> https://github.com/Rufidj/libmod_3d"
fi
if grep -q "libmod_3d_zone.c" "$LM3D/CMakeLists.txt" 2>/dev/null; then
    echo "  OK  esta listado en el CMakeLists de libmod_3d"
else
    echo "  MAL no esta listado en $LM3D/CMakeLists.txt"
fi

echo "===== 2) TODAS las libmod_3d.so del sistema ====="
LIST=$(find / -name "libmod_3d.so" -not -path "/proc/*" 2>/dev/null)
if [ -z "$LIST" ]; then
    echo "  (ninguna encontrada: BennuGD2 no esta compilado?)"
else
    echo "$LIST" | while read -r f; do
        if nm -D --defined-only "$f" 2>/dev/null | grep -qw g3d_zone_load; then
            S="TIENE la API"
        else
            S="LE FALTA la API"
        fi
        echo "  [$S] $f   ($(date -r "$f" '+%Y-%m-%d %H:%M' 2>/dev/null))"
    done
fi

echo "===== 3) La que CARGA el editor ====="
if [ -x "$ED/bin/editor3d" ]; then
    SO=$(ldd "$ED/bin/editor3d" 2>/dev/null | grep -i libmod_3d | awk '{print $3}')
    echo "  carga: ${SO:-no resuelta}"
    if [ -n "$SO" ] && nm -D --defined-only "$SO" 2>/dev/null | grep -qw g3d_zone_load; then
        echo "  OK  esa copia SI tiene la API -> el editor deberia arrancar"
    else
        echo "  MAL esa copia NO tiene la API -> ESTE es el fallo"
    fi
    readelf -d "$ED/bin/editor3d" 2>/dev/null | grep -E "RPATH|RUNPATH" | sed 's/^/  /'
else
    echo "  (aun no hay $ED/bin/editor3d compilado)"
fi

echo "===== 4) Contra que se configuro el editor ====="
if [ -f "$ED/build/CMakeCache.txt" ]; then
    grep -E "^BGD_BIN|^BGD_ROOT|^LIBMOD3D" "$ED/build/CMakeCache.txt" | sed 's/^/  /'
else
    echo "  (sin build/ todavia)"
fi

echo
echo "===== QUE HACER ====="
echo "  * Si (1) sale MAL: falta el libmod_3d del editor. Ponlo en"
echo "    BennuGD2/modules/libmod_3d y RECOMPILA BennuGD2."
echo "  * Si en (2) hay varias copias y la que carga en (3) no tiene la API, suele"
echo "    ser una copia vieja (p.ej. en ~/.bennugd2/runtime/) tapando a la buena."
echo "    Reconfigura el editor desde cero:"
echo "        rm -rf \"$ED/build\""
echo "        cmake -S \"$ED\" -B \"$ED/build\" && cmake --build \"$ED/build\" -j4"
echo "    El 'rm -rf build' es lo importante: la cache vieja manda sobre lo demas."
echo "  * Se puede forzar cual usar:"
echo "        cmake -S . -B build -DBGD_BIN=/ruta/a/BennuGD2/build/<plataforma>/bin"
