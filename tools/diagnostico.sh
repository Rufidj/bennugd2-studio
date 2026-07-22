#!/bin/sh
# Diagnostico del editor bennugd2-studio. Ejecutar DESDE la carpeta del editor.
echo "===== 1) FUENTES ====="
Z=../libmod_3d/libmod_3d_zone.c
[ -f "$Z" ] && echo "  OK  existe libmod_3d_zone.c" || echo "  MAL no existe $Z  -> no tienes el libmod_3d correcto"
grep -q "libmod_3d_zone.c" ../libmod_3d/CMakeLists.txt 2>/dev/null \
  && echo "  OK  esta en el CMakeLists de libmod_3d" || echo "  MAL no esta en el CMakeLists"

echo "===== 2) TODAS las libmod_3d.so del sistema ====="
find / -name "libmod_3d.so" -not -path "/proc/*" 2>/dev/null | while read f; do
  if nm -D --defined-only "$f" 2>/dev/null | grep -qw g3d_zone_load; then S="TIENE la API"; else S="LE FALTA la API"; fi
  echo "  [$S] $f   ($(date -r "$f" '+%Y-%m-%d %H:%M'))"
done

echo "===== 3) La que CARGA el editor ====="
if [ -x ./bin/editor3d ]; then
  SO=$(ldd ./bin/editor3d 2>/dev/null | grep -i libmod_3d | awk '{print $3}')
  echo "  carga: ${SO:-no resuelta}"
  if [ -n "$SO" ] && nm -D --defined-only "$SO" 2>/dev/null | grep -qw g3d_zone_load; then
    echo "  OK  esa copia SI tiene la API  -> el editor deberia arrancar"
  else
    echo "  MAL esa copia NO tiene la API -> ESTE es el fallo"
  fi
  readelf -d ./bin/editor3d 2>/dev/null | grep -E "RPATH|RUNPATH" | sed 's/^/  /'
else
  echo "  (no hay ./bin/editor3d compilado)"
fi

echo "===== 4) Contra que se configuro ====="
grep -E "^BGD_BIN|^BGD_ROOT|^LIBMOD3D" build/CMakeCache.txt 2>/dev/null | sed 's/^/  /' || echo "  (sin build/)"

echo "===== QUE HACER ====="
echo "  Si en (2) la unica copia con API NO es la que carga en (3):"
echo "    rm -rf build && cmake -S . -B build && cmake --build build -j4"
echo "  Si NINGUNA copia tiene la API: recompila BennuGD2 tras poner el libmod_3d correcto."
echo "  Ojo con copias viejas en ~/.bennugd2/runtime/ que pueden tapar a la buena."
