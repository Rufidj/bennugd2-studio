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
if(NOT BGD_BIN)
    file(GLOB _BGD_BIN_CANDIDATES "${BGD_ROOT}/build/*/bin" "${BGD_ROOT}/bin")
    foreach(_c ${_BGD_BIN_CANDIDATES})
        if(EXISTS "${_c}/libmod_3d${CMAKE_SHARED_LIBRARY_SUFFIX}")
            set(BGD_BIN "${_c}")
            break()
        endif()
    endforeach()
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
    ${SDL2_LIBRARIES}
    SDL2_image
    ${OPENGL_LIBRARIES}
    ${CMAKE_DL_LIBS}
    m
)

# En runtime hay que encontrar libmod_3d.so y sus dependencias (bgdrtm, bggfx,
# sdlhandler, SDL2_gpu) sin depender de LD_LIBRARY_PATH.
set_target_properties(editor3d PROPERTIES
    BUILD_RPATH   "${BGD_BIN};${BGD_DEPS}"
    INSTALL_RPATH "${BGD_BIN};${BGD_DEPS}"
)

# --disable-new-dtags => rpath como DT_RPATH y no DT_RUNPATH. El DT_RUNPATH del
# ejecutable NO se consulta en los dlopen que hace otra libreria, asi que con
# RUNPATH la carga de librerias solo funcionaba si LD_LIBRARY_PATH venia puesto
# en el entorno: por consola si, pero al abrir el editor con doble clic no.
target_link_options(editor3d PRIVATE "-Wl,--disable-new-dtags")
