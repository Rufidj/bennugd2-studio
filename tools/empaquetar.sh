#!/bin/sh
# Arma una carpeta portable de bennugd2-studio: el editor con TODO lo que necesita
# al lado, para que arranque en un ordenador que no tenga BennuGD2 compilado.
#
# Hace falta porque un simple zip del binario NO funciona: el ejecutable lleva
# grabadas rutas absolutas de la maquina donde se compilo (RPATH de las
# librerias, ruta de bgdc, ruta de la fuente de iconos). En otro ordenador esas
# rutas no existen y el editor ni arranca.
#
#   ./tools/empaquetar.sh            -> crea dist/bennugd2-studio/ y el .tar.gz
#
# Se puede ejecutar desde cualquier sitio: las rutas salen de donde esta el script.

set -e

SELF=$(cd "$(dirname "$0")" && pwd)
ED=$(dirname "$SELF")                 # tools/ -> raiz del editor
OUT="$ED/dist/bennugd2-studio"

BIN="$ED/bin/editor3d"
if [ ! -x "$BIN" ]; then
    echo "No encuentro $BIN"
    echo "Compila primero:  cmake -S \"$ED\" -B \"$ED/build\" && cmake --build \"$ED/build\" -j4"
    exit 1
fi

echo "==> Preparando $OUT"
rm -rf "$OUT"
mkdir -p "$OUT/lib"

# El ejecutable va DENTRO de lib/: arriba solo queda el lanzador, para que nadie
# arranque el binario a pelo. Suelto no encuentra las librerias (las de BennuGD2
# traen grabada la ruta de quien las compilo) y falla con un error de enlazador.
cp "$BIN" "$OUT/lib/"
cp -r "$ED/fonts" "$OUT/fonts"

# Carpeta de proyecto por defecto: el editor la busca junto al ejecutable antes
# que la ruta de compilacion (que en otro ordenador no existe).
mkdir -p "$OUT/project/Assets" "$OUT/project/Scenes" "$OUT/project/Scripts"

# --- librerias propias (las del sistema NO se copian: se usan las del usuario) ---
# Solo las de BennuGD2 y sus dependencias; copiar libc o los drivers de video daria
# mas problemas de los que resuelve.
echo "==> Copiando librerias de BennuGD2"
ldd "$BIN" | awk '{print $3}' | grep -E '^/' | while read -r so; do
    case "$so" in
        *libmod_*|*libbgdrtm*|*libbggfx*|*libsdlhandler*|*libSDL2_gpu*|*libJolt*)
            cp -Lv "$so" "$OUT/lib/" ;;
    esac
done

# --- el compilador y el interprete, y TODOS los modulos que el juego carga ---
# El editor los invoca para 'Generar y ejecutar'. bgdi carga los modulos con
# dlopen sin ruta, asi que tienen que estar en el mismo sitio que el.
BGD_BIN=$(ldd "$BIN" | awk '/libmod_3d/ {print $3}' | sed 's|/[^/]*$||')
if [ -n "$BGD_BIN" ] && [ -d "$BGD_BIN" ]; then
    echo "==> Copiando bgdc/bgdi y los modulos desde $BGD_BIN"
    for f in bgdc bgdi; do
        [ -f "$BGD_BIN/$f" ] && cp -v "$BGD_BIN/$f" "$OUT/lib/"
    done
    for f in "$BGD_BIN"/libmod_*.so "$BGD_BIN"/lib*.so; do
        [ -f "$f" ] && cp -Ln "$f" "$OUT/lib/" 2>/dev/null || true
    done
    # dependencias precompiladas (SDL2_gpu y demas) de la misma plataforma
    PLAT=$(basename "$(dirname "$BGD_BIN")")
    DEPS="$BGD_BIN/../../../dependencies/$PLAT"
    if [ -d "$DEPS" ]; then
        for f in "$DEPS"/*.so*; do
            [ -f "$f" ] && cp -Ln "$f" "$OUT/lib/" 2>/dev/null || true
        done
    fi
else
    echo "AVISO: no he localizado los binarios de BennuGD2; el paquete no podra"
    echo "       compilar ni ejecutar juegos (el editor si arrancara)."
fi

# --- lanzador ---
# El interprete carga los modulos con dlopen, y para eso mira PATH/LD_LIBRARY_PATH.
cat > "$OUT/bennugd2-studio.sh" <<'EOF'
#!/bin/sh
# Lanzador. Por defecto usa el BennuGD2 que viene dentro (lib/).
#
# Para usar TU propia compilacion de BennuGD2:
#     BGD_BIN=/ruta/a/BennuGD2/build/linux-gnu/bin ./bennugd2-studio.sh
#
# Ojo: ese BennuGD2 tiene que traer el libmod_3d con la API de edicion. El editor
# llama a funciones (zonas pintadas, carga de relieve, colisionador de terreno...)
# que NO estan en el libmod_3d original. Sin ellas el editor no arranca, y el
# error del cargador ("undefined symbol") no explica nada, asi que se comprueba
# aqui antes y se dice lo que pasa.
AQUI=$(cd "$(dirname "$0")" && pwd)

if [ -n "$BGD_BIN" ]; then
    SO="$BGD_BIN/libmod_3d.so"
    if [ ! -f "$SO" ]; then
        echo "No encuentro $SO"
        echo "BGD_BIN debe apuntar a los binarios ya compilados, por ejemplo:"
        echo "    BGD_BIN=/ruta/a/BennuGD2/build/linux-gnu/bin"
        exit 1
    fi
    if command -v nm >/dev/null 2>&1; then
        FALTAN=""
        for f in g3d_zone_load g3d_zone_paint g3d_terrain_load \
                 g3d_scene_set_terrain_collider g3d_model_is_skinned \
                 g3d_model_node_find g3d_model_bounds g3d_model_animation_name; do
            nm -D --defined-only "$SO" 2>/dev/null | grep -qw "$f" || FALTAN="$FALTAN $f"
        done
        if [ -n "$FALTAN" ]; then
            echo "Ese libmod_3d no trae la API que necesita el editor."
            echo "  Fichero: $SO"
            echo "  Le faltan:"
            for f in $FALTAN; do echo "     $f"; done
            echo
            echo "El editor usa una version ampliada de libmod_3d:"
            echo "    https://github.com/Rufidj/libmod_3d"
            echo "Clonala en TuBennuGD2/modules/libmod_3d, recompila BennuGD2 y vuelve"
            echo "a intentarlo. O ejecuta sin BGD_BIN para usar el que viene dentro."
            exit 1
        fi
    fi
    PATH="$BGD_BIN:$AQUI/lib:$PATH"
    LD_LIBRARY_PATH="$BGD_BIN:$AQUI/lib:$LD_LIBRARY_PATH"
    echo "Usando tu BennuGD2: $BGD_BIN"
else
    PATH="$AQUI/lib:$PATH"
    LD_LIBRARY_PATH="$AQUI/lib:$LD_LIBRARY_PATH"
fi
export PATH LD_LIBRARY_PATH
exec "$AQUI/lib/editor3d" "$@"
EOF
chmod +x "$OUT/bennugd2-studio.sh"

cat > "$OUT/LEEME.txt" <<'EOF'
bennugd2-studio - carpeta portable
==================================

Arrancar:
    ./bennugd2-studio.sh

No hace falta tener BennuGD2 compilado: va todo dentro de lib/.

Si prefieres usar TU propia compilacion de BennuGD2:

    BGD_BIN=/ruta/a/BennuGD2/build/linux-gnu/bin ./bennugd2-studio.sh

Eso si: tiene que traer el libmod_3d con la API de edicion, que NO esta en el
libmod_3d original de BennuGD2. Esta en https://github.com/Rufidj/libmod_3d;
se clona en TuBennuGD2/modules/libmod_3d y se recompila BennuGD2. El lanzador lo
comprueba antes de arrancar y te dice que funciones faltan.

Si algo falla, el editor lo dice por la terminal indicando QUE fichero busca y en
que rutas ha mirado. Arrancalo desde una terminal para verlo.

Requisitos del sistema: SDL2, SDL2_image y drivers de OpenGL. Esas si se usan las
del ordenador, para no pelearse con los drivers de video.
EOF

echo "==> Comprobando que no queden rutas del ordenador de origen"
if readelf -d "$OUT/lib/editor3d" | grep -q '\$ORIGIN'; then
    echo "    OK  el ejecutable busca sus librerias en \$ORIGIN/lib"
else
    echo "    MAL el ejecutable no tiene \$ORIGIN en el RPATH: recompila,"
    echo "        el CMakeLists nuevo lo anade."
fi

TGZ="$ED/dist/bennugd2-studio-$(uname -m).tar.gz"
tar -czf "$TGZ" -C "$ED/dist" bennugd2-studio
echo
echo "Listo:"
echo "   carpeta:  $OUT"
echo "   paquete:  $TGZ"
echo
echo "Comprueba que funciona fuera del arbol de compilacion con:"
echo "   $SELF/probar-paquete.sh"
