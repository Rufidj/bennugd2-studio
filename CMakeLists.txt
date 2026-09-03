cmake_minimum_required(VERSION 3.10)
project(editor3d CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# =============================================================================
#  Localizacion de BennuGD2 y libmod_3d  (NADA de rutas absolutas: esto tiene
#  que compilar en la maquina de cualquiera)
# =============================================================================
# Por defecto se asume la disposicion normal del repo:
#     BennuGD2/modules/editor3d   <- este proyecto
#     BennuGD2/modules/libmod_3d  <- el modulo 3D
#     BennuGD2/build/<plat>/bin   <- binarios de BennuGD2 ya compilados
# Todo se puede sobreescribir desde la linea de comandos, por ejemplo:
#     cmake -S . -B build -DBGD_ROOT=/ruta/a/BennuGD2
#     cmake -S . -B build -DBGD_BIN=/ruta/a/binarios -DLIBMOD3D=/ruta/a/libmod_3d

get_filename_component(_DEF_BGD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(BGD_ROOT "${_DEF_BGD_ROOT}" CACHE PATH "Raiz del repositorio BennuGD2")

get_filename_component(_DEF_LIBMOD3D "${CMAKE_CURRENT_SOURCE_DIR}/../libmod_3d" ABSOLUTE)
set(LIBMOD3D "${_DEF_LIBMOD3D}" CACHE PATH "Fuentes del modulo libmod_3d")

# Directorio de binarios de BennuGD2: el nombre de la carpeta de build depende
# de la plataforma (linux-gnu, etc.), asi que se busca el que tenga el modulo.
find_program(NM_EXECUTABLE nm)
if(NOT BGD_BIN)
    file(GLOB _BGD_BIN_CANDIDATES "${BGD_ROOT}/build/*/bin" "${BGD_ROOT}/bin")
    # Puede haber varias compilaciones de BennuGD2 (y copias instaladas antiguas).
    # Se prefiere la que traiga la API del editor; si ninguna la trae, se coge la
    # primera que exista y mas abajo salta el aviso explicando que le falta.
    set(_BGD_BIN_FALLBACK "")
    foreach(_c ${_BGD_BIN_CANDIDATES})
        set(_cand "${_c}/libmod_3d${CMAKE_SHARED_LIBRARY_SUFFIX}")
        if(EXISTS "${_cand}")
            if(NOT _BGD_BIN_FALLBACK)
                set(_BGD_BIN_FALLBACK "${_c}")
            endif()
            if(NM_EXECUTABLE)
                execute_process(COMMAND ${NM_EXECUTABLE} -D --defined-only "${_cand}"
                                OUTPUT_VARIABLE _s ERROR_QUIET)
                string(FIND "${_s}" " g3d_zone_load" _p)
                if(NOT _p EQUAL -1)
                    set(BGD_BIN "${_c}")   # esta si sirve
                    break()
                endif()
            else()
                set(BGD_BIN "${_c}")
                break()
            endif()
        endif()
    endforeach()
    if(NOT BGD_BIN)
        set(BGD_BIN "${_BGD_BIN_FALLBACK}")
    endif()
endif()
set(BGD_BIN "${BGD_BIN}" CACHE PATH "Directorio con libmod_3d/libbgdrtm ya compilados")

# Dependencias precompiladas que acompanan a BennuGD2 (SDL2_gpu, etc.).
# Se toma la carpeta de la MISMA plataforma que los binarios (build/<plat>/bin
# -> dependencies/<plat>), para no meter en el rpath las de otras plataformas.
if(NOT BGD_DEPS AND BGD_BIN)
    get_filename_component(_BGD_PLAT_DIR "${BGD_BIN}" DIRECTORY)
    get_filename_component(_BGD_PLAT "${_BGD_PLAT_DIR}" NAME)
    if(IS_DIRECTORY "${BGD_ROOT}/dependencies/${_BGD_PLAT}")
        set(BGD_DEPS "${BGD_ROOT}/dependencies/${_BGD_PLAT}")
    endif()
endif()

# ---- Comprobaciones con mensajes utiles en vez de errores cripticos ----
if(NOT BGD_BIN OR NOT EXISTS "${BGD_BIN}/libmod_3d${CMAKE_SHARED_LIBRARY_SUFFIX}")
    message(FATAL_ERROR
        "No encuentro libmod_3d compilado.\n"
        "  Busque en: ${BGD_ROOT}/build/*/bin\n"
        "Compila primero BennuGD2 (con el modulo libmod_3d), o indica la ruta:\n"
        "  cmake -S . -B build -DBGD_BIN=/ruta/a/BennuGD2/build/<plat>/bin")
endif()
if(NOT EXISTS "${LIBMOD3D}/libmod_3d_zone.h")
    message(FATAL_ERROR
        "No encuentro las fuentes de libmod_3d en ${LIBMOD3D}.\n"
        "Indica la ruta:  cmake -S . -B build -DLIBMOD3D=/ruta/a/libmod_3d")
endif()

# ---- Motor: se ENLAZA libmod_3d (NO se compila el core estaticamente) ----
# Asi el editor usa exactamente el mismo motor que el juego, sin duplicar su
# estado ni su codigo.
set(LIBMOD3D_SO  "${BGD_BIN}/libmod_3d${CMAKE_SHARED_LIBRARY_SUFFIX}")
# El runtime de BennuGD2: de el sale la tabla de conversion de charset que usan
# sus fuentes (iso8859_1_to_cp850), para que el HUD del editor elija el mismo
# glifo que write() en el juego. Ya venia cargado como dependencia de libmod_3d,
# pero hay que enlazarlo explicitamente para poder usar sus simbolos.
set(BGDRTM_SO    "${BGD_BIN}/libbgdrtm${CMAKE_SHARED_LIBRARY_SUFFIX}")

# ---- El libmod_3d encontrado, ¿es el que necesita el editor? ----
# El editor usa una API de libmod_3d que NO existe en el modulo original de
# BennuGD2 (zonas, carga de relieve, colisionador de terreno, consultas de
# modelo...). Si se compila contra otro libmod_3d, el enlazado puede pasar pero
# el editor muere al arrancar con "undefined symbol: g3d_zone_load" o similar.
# Mejor detectarlo aqui y decir exactamente que falta y por que.
if(NM_EXECUTABLE)
    execute_process(COMMAND ${NM_EXECUTABLE} -D --defined-only "${LIBMOD3D_SO}"
                    OUTPUT_VARIABLE _LM3D_SYMS ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(_LM3D_MISSING "")
    foreach(_s g3d_zone_load g3d_zone_paint g3d_terrain_load
               g3d_scene_set_terrain_collider g3d_model_is_skinned
               g3d_model_node_find g3d_model_bounds g3d_model_animation_name)
        string(FIND "${_LM3D_SYMS}" " ${_s}" _pos)
        if(_pos EQUAL -1)
            list(APPEND _LM3D_MISSING ${_s})
        endif()
    endforeach()
    if(_LM3D_MISSING)
        string(REPLACE ";" "\n    " _LM3D_MISSING_TXT "${_LM3D_MISSING}")
        message(FATAL_ERROR
            "El libmod_3d encontrado no trae la API que necesita el editor.\n"
            "  Fichero: ${LIBMOD3D_SO}\n"
            "  Le faltan estas funciones:\n    ${_LM3D_MISSING_TXT}\n"
            "\n"
            "El editor necesita la version de libmod_3d con la API de edicion:\n"
            "  https://github.com/Rufidj/libmod_3d\n"
            "Clonala en BennuGD2/modules/libmod_3d, recompila BennuGD2 para que\n"
            "regenere libmod_3d, y vuelve a configurar el editor.\n"
            "Si tienes varias compilaciones de BennuGD2, comprueba que apunta a la\n"
            "correcta con -DBGD_BIN=/ruta/a/build/<plataforma>/bin")
    endif()
endif()

message(STATUS "BennuGD2  : ${BGD_ROOT}")
message(STATUS "  binarios: ${BGD_BIN}")
message(STATUS "  libmod_3d: ${LIBMOD3D}")

# ---- Dear ImGui (vendored, docking) + SDL2/OpenGL3 backends ----
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/TextEditor.cpp        # ImGuiColorTextEdit (editor de scripts con color)
    ${IMGUI_DIR}/ImGuizmo.cpp          # gizmos de mover/rotar/escalar
)

add_executable(editor3d
    src/main.cpp
    src/engine_bridge.c
    src/hud2d.c             # lectura de FPG/FNT nativos para el previo del HUD 2D
    ${IMGUI_SRC}
)

target_compile_definitions(editor3d PRIVATE
    GL_GLEXT_PROTOTYPES
    IMGUI_DEFINE_MATH_OPERATORS            # requerido por imgui_internal.h (DockBuilder, TextEditor)
    ASSET_DIR="${LIBMOD3D}/TEST/assets"   # ruta absoluta -> funciona desde cualquier CWD
    PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/project"   # carpeta de proyecto (Assets/Scenes/Scripts)
    FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fonts"        # fuente de iconos (Font Awesome)
    SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/objeto.prg"
    BGDC_PATH="${BGD_BIN}/bgdc"      # compilador BennuGD2 (se resuelve arriba)
)

target_include_directories(editor3d PRIVATE
    ${IMGUI_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${LIBMOD3D}
    ${SDL2_INCLUDE_DIRS}
    /usr/include            # para los #include "SDL2/SDL_image.h" del motor
)

target_link_libraries(editor3d
    ${LIBMOD3D_SO}          # el motor (arrastra bgdrtm/bggfx como dependencias suyas)
    ${BGDRTM_SO}            # runtime de BennuGD2 (tablas de charset de las fuentes)
    ${SDL2_LIBRARIES}
    SDL2_image
    ${OPENGL_LIBRARIES}
    ${CMAKE_DL_LIBS}
    z                       # los graficos de BennuGD2 (fpg/fnt) vienen con gzip
    m
)

# En runtime hay que encontrar libmod_3d.so y sus dependencias (bgdrtm, bggfx,
# sdlhandler, SDL2_gpu) sin depender de LD_LIBRARY_PATH.
# $ORIGIN/lib va PRIMERO para que un paquete portable (editor + lib/ al lado) se
# use a si mismo. Detras van las rutas de esta maquina, que son las que valen
# mientras se desarrolla aqui. Sin $ORIGIN, el binario copiado a otro ordenador
# busca las librerias en el disco de quien lo compilo y no arranca.
set_target_properties(editor3d PROPERTIES
    BUILD_RPATH   "$ORIGIN/lib;$ORIGIN;${BGD_BIN};${BGD_DEPS}"
    INSTALL_RPATH "$ORIGIN/lib;$ORIGIN;${BGD_BIN};${BGD_DEPS}")

# --disable-new-dtags => rpath como DT_RPATH y no DT_RUNPATH. El DT_RUNPATH del
# ejecutable NO se consulta en los dlopen que hace otra libreria, asi que con
# RUNPATH la carga de librerias solo funcionaba si LD_LIBRARY_PATH venia puesto
# en el entorno: por consola si, pero al abrir el editor con doble clic no.
target_link_options(editor3d PRIVATE "-Wl,--disable-new-dtags")

# Las fuentes de iconos se copian junto al ejecutable. FONT_DIR (arriba) apunta al
# arbol de fuentes de quien compila; si el binario se mueve o se reparte, esa ruta
# deja de existir. Con la copia al lado, el editor se encuentra siempre.
add_custom_command(TARGET editor3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
            "$<TARGET_FILE_DIR:editor3d>/fonts"
    COMMENT "Copiando fonts junto al ejecutable")
