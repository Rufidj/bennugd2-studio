cmake_minimum_required(VERSION 3.10)
project(editor3d CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(LIBMOD3D ${CMAKE_CURRENT_SOURCE_DIR}/../libmod_3d)

# ---- Engine CORE (todos los libmod_3d_*.c menos libmod_3d.c = capa BennuGD2) ----
# libmod_3d.c NO casa con "libmod_3d_*.c" (no lleva '_'), asi que el glob ya lo
# excluye. Jolt (.cpp) queda fuera: sin USE_JOLT usa la fisica interna.
file(GLOB ENGINE_CORE ${LIBMOD3D}/libmod_3d_*.c)
list(APPEND ENGINE_CORE ${LIBMOD3D}/ufbx.c)

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
    ${ENGINE_CORE}
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
    ${SDL2_INCLUDE_DIRS}
    /usr/include            # para los #include "SDL2/SDL_image.h" del motor
)

target_link_libraries(editor3d
    ${SDL2_LIBRARIES}
    SDL2_image
    ${OPENGL_LIBRARIES}
    ${X11_LIBRARIES}
    ${CMAKE_DL_LIBS}
    m
)
