cmake_minimum_required(VERSION 3.10)
project(editor3d CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(LIBMOD3D ${CMAKE_CURRENT_SOURCE_DIR}/../libmod_3d)

# ---- Motor: se ENLAZA libmod_3d.so (NO se compila el core estaticamente) ----
# Clave para el Play con scripts: el interprete de BennuGD2 (bgdrtm) carga ese
# mismo .so con dlopen, y en Linux un .so ya cargado se comparte -> editor y
# scripts ven UNA sola instancia del motor (mismas escenas/entidades/registros).
# Antes se compilaba el core aparte y eran DOS mundos separados.
set(BGD_ROOT /home/ruben/BennuGD2)
set(BGD_CORE ${BGD_ROOT}/core)
set(BGD_BIN  ${BGD_ROOT}/build/linux-gnu/bin)
set(BGD_DEPS ${BGD_ROOT}/dependencies/linux-gnu)
set(LIBMOD3D_SO ${BGD_BIN}/libmod_3d.so)
set(LIBBGDRTM_SO ${BGD_BIN}/libbgdrtm.so)   # interprete BennuGD2 embebido

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

find_package(X11)

add_executable(editor3d
    src/main.cpp
    src/engine_bridge.c
    src/embed_x11.cpp
    src/script_host.c        # embebe el interprete BennuGD2 (bgdrtm) para el Play
    ${IMGUI_SRC}
)

target_compile_definitions(editor3d PRIVATE
    GL_GLEXT_PROTOTYPES
    IMGUI_DEFINE_MATH_OPERATORS            # requerido por imgui_internal.h (DockBuilder, TextEditor)
    ASSET_DIR="${LIBMOD3D}/TEST/assets"   # ruta absoluta -> funciona desde cualquier CWD
    PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/project"   # carpeta de proyecto (Assets/Scenes/Scripts)
    FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fonts"        # fuente de iconos (Font Awesome)
    SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/objeto.prg"
    BGDC_PATH="/home/ruben/BennuGD2/build/linux-gnu/bin/bgdc"
)

target_include_directories(editor3d PRIVATE
    ${IMGUI_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${LIBMOD3D}
    ${BGD_CORE}/include      # cabeceras del runtime BennuGD2 (instance.h, i_procdef.h...)
    ${BGD_CORE}/bgdrtm       # bgdrtm.h
    ${SDL2_INCLUDE_DIRS}
    /usr/include            # para los #include "SDL2/SDL_image.h" del motor
)

target_link_libraries(editor3d
    ${LIBMOD3D_SO}          # motor compartido (arrastra bgdrtm/bggfx: base para el Play con scripts)
    ${LIBBGDRTM_SO}         # interprete BennuGD2 (dcb_load / instance_go_frame)
    ${SDL2_LIBRARIES}
    SDL2_image
    ${OPENGL_LIBRARIES}
    ${X11_LIBRARIES}
    ${CMAKE_DL_LIBS}
    m
)

# En runtime hay que encontrar libmod_3d.so y sus dependencias (bgdrtm, bggfx,
# sdlhandler, SDL2_gpu) sin depender de LD_LIBRARY_PATH.
set_target_properties(editor3d PROPERTIES
    BUILD_RPATH   "${BGD_BIN};${BGD_DEPS}"
    INSTALL_RPATH "${BGD_BIN};${BGD_DEPS}"
)
