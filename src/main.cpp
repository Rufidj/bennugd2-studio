// ============================================================================
//  BennuGD2 3D Editor  -  standalone Dear ImGui application
//  Paso 2: viewport del motor. Compilamos el CORE de libmod_3d dentro del editor
//  y llamamos a g3d_renderer_render() contra ESTE contexto OpenGL, con ImGui
//  dibujado por encima. (El viewport en ventana ImGui acoplable vendra despues.)
// ============================================================================
#include <SDL.h>
#include <SDL_syswm.h>
#include <cstdio>
#include <cmath>


#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

// Lista ficheros de <dir> cuya extension esta en exts (minusculas, con punto).
static std::vector<std::string> scan_dir(const std::string& dir,
                                         std::initializer_list<const char*> exts) {
    std::vector<std::string> out;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(dir, ec)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for (auto x : exts) if (ext == x) { out.push_back(e.path().filename().string()); break; }
    }
    std::sort(out.begin(), out.end());
    return out;
}
static std::vector<std::string> scan_assets(const std::string& dir) {
    return scan_dir(dir, { ".glb", ".gltf", ".fbx" });
}
static std::vector<std::string> scan_textures(const std::string& dir) {
    return scan_dir(dir, { ".png", ".jpg", ".jpeg", ".tga", ".bmp" });
}

#include "imgui.h"
#include "imgui_internal.h"      // DockBuilder* (layout inicial)
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "TextEditor.h"          // ImGuiColorTextEdit
#include "bennugd2_lang.h"       // resaltado de sintaxis BennuGD2
#include "ImGuizmo.h"            // gizmos mover/rotar/escalar
#include "imfilebrowser.h"       // explorador de archivos (ImGui::FileBrowser)
#include "IconsFontAwesome6.h"   // macros ICON_FA_* para la toolbar
#include <SDL_opengl.h>

// ---- API C del motor (core de libmod_3d, sin la capa BennuGD2) --------------
extern "C" {
    int   g3d_renderer_init(unsigned width, unsigned height);
    void  g3d_renderer_render(void);
    void  g3d_renderer_set_viewport_physical(unsigned x, unsigned y, unsigned w, unsigned h);
    void  g3d_renderer_set_hdr(int enabled);
    void  g3d_renderer_set_shadows(int enabled);
    void  g3d_renderer_set_shadow_resolution(unsigned resolution);
    void  g3d_renderer_set_clear_color(float r, float g, float b, float a);
    void  g3d_renderer_set_target(unsigned fbo);
    int   g3d_scene_create(const char *name);
    int   g3d_scene_set_active(int scene_id);
    int   g3d_camera_create(void);
    int   g3d_camera_set_active(int camera_id);
    int   g3d_camera_set_position(int camera_id, float x, float y, float z);
    int   g3d_camera_look_at(int camera_id, float tx, float ty, float tz,
                             float ux, float uy, float uz);
    int   g3d_light_create(int type, float r, float g, float b);
    int   g3d_light_set_direction(int light_id, float dx, float dy, float dz);
    int   g3d_light_set_intensity(int light_id, float intensity);
    void *g3d_gltf_load(const char *filepath);
    void *g3d_fbx_load(const char *filepath);
    void  g3d_fbx_set_recenter(int enabled);
    void  g3d_model_animate_all(void *model, float time, int loop);
    int   g3d_model_animation_count(void *model);
    int   g3d_model_is_skinned(void *model);
    void  g3d_model_animate(void *model, int anim, float time, int loop);
    float g3d_model_animation_duration(void *model, int anim);
    const char *g3d_model_animation_name(void *model, int i);
    int   g3d_model_bounds(void *model, float *out_min, float *out_max);
    int   g3d_model_node_find(void *model, const char *name);
    int   g3d_model_node_count(void *model);
    const char *g3d_model_node_name(void *model, int i);
    float g3d_model_node_axis(void *model, int node, int comp);
    // ---- fisica / personaje / colision (para el PLAY en vivo dentro del editor) ----
    int   g3d_char_create(float x, float y, float z, float radius, float height);
    void  g3d_char_clear_all(void);
    void  g3d_char_move(int id, float vx, float vz);
    void  g3d_char_jump(int id, float speed);
    void  g3d_char_set_water(int id, int in_water, float surface_y);
    void  g3d_char_update(int id, float dt);
    void  g3d_char_set_position(int id, float x, float y, float z);
    void  g3d_char_set_tuning(int id, float step, float slope_deg);
    void  g3d_char_set_push(int id, float fuerza);   // empuja cuerpos fisicos
    float g3d_char_x(int id); float g3d_char_y(int id); float g3d_char_z(int id);
    int   g3d_char_grounded(int id);
    int   g3d_rigidbody_create(float x, float y, float z, float hx, float hy, float hz, float mass);
    int   g3d_rigidbody_create_sphere(float x, float y, float z, float radius, float mass);
    int   g3d_rigidbody_create_capsule(float x, float y, float z, float radius, float half_h, float mass);
    int   g3d_rigidbody_create_cylinder(float x, float y, float z, float radius, float half_h, float mass);
    void  g3d_rigidbody_clear(void);
    void  g3d_rigidbody_step(float dt);
    void  g3d_rigidbody_set_bounce(int id, float restitution, float friction);
    void  g3d_rigidbody_set_damping(int id, float lin, float ang);   // resistencia del medio (agua)
    void  g3d_rigidbody_apply_angular_impulse(int id, float ax, float ay, float az);
    void  g3d_rigidbody_set_buoyancy(int id, float water_y, float rel_density);  // flotacion real
    void  g3d_rigidbody_apply_impulse(int id, float ix, float iy, float iz);
    float g3d_rigidbody_x(int id);
    float g3d_rigidbody_y(int id);
    float g3d_rigidbody_z(int id);
    float g3d_rigidbody_render_x(int id); float g3d_rigidbody_render_y(int id); float g3d_rigidbody_render_z(int id);
    float g3d_rigidbody_angle_x(int id); float g3d_rigidbody_angle_y(int id); float g3d_rigidbody_angle_z(int id);
    int   g3d_collider_add_box(float minx, float miny, float minz, float maxx, float maxy, float maxz);
    void  g3d_collider_clear(void);
    void  g3d_physics_set_gravity(float g);
    int   g3d_scene_set_terrain_collider(void *mesh);
    void  g3d_zone_init(int side, float world_size);
    void  g3d_zone_paint(float wx, float wz, float radius, int layer, int on);
    int   g3d_zone_blocked(float wx, float wz, int layer);
    int   g3d_zone_value(float wx, float wz);
    int   g3d_zone_side(void);
    int   g3d_zone_save(const char *path);
    int   g3d_zone_load(const char *path);
    void  g3d_editor_dbg_camera(void);
    int   g3d_editor_ray_plane(float sx, float sy, float w, float h, float planeY, float *out);
    int   g3d_entity_impl_set_position(int entity_id, float x, float y, float z);
    int   g3d_entity_impl_set_rotation(int entity_id, float pitch, float yaw, float roll);
    int   g3d_entity_impl_set_scale(int entity_id, float sx, float sy, float sz);
    int   g3d_entity_impl_destroy(int entity_id);
    void  g3d_editor_get_view(float *m16);
    void  g3d_editor_get_proj(float *m16);
    void  g3d_editor_set_aspect(float a);
    void *g3d_editor_make_terrain(int scene_id, int grid, float worldsize, float tiling, const char *texpath);
    int   g3d_editor_terrain_pick(float sx, float sy, float w, float h, void *mesh, float *out);
    void  g3d_editor_terrain_raise(void *mesh, float x, float z, float r, float amt);
    void  g3d_editor_terrain_smooth(void *mesh, float x, float z, float r, float amt);
    void  g3d_editor_terrain_flatten(void *mesh, float x, float z, float r, float amt);
    void  g3d_editor_terrain_flatten_to(void *mesh, float x, float z, float r, float target_h, float amt);
    void  g3d_editor_terrain_hole(void *mesh, float x, float z, float r, int on);
    float g3d_editor_terrain_height(void *mesh, float x, float z);
    int   g3d_editor_world_to_screen(float wx, float wy, float wz, float w, float h, float *out2);
    void  g3d_editor_terrain_save(void *mesh, const char *path);
    int   g3d_editor_terrain_load(void *mesh, const char *path);
    void *g3d_editor_load_texture(const char *path);
    void  g3d_editor_terrain_paint(void *mesh, void *tex, float tiling, float x, float z, float r, float opacity);
    void  g3d_editor_paint_save(const char *path);
    int   g3d_editor_paint_load(const char *path);
    void  g3d_editor_water_update(int enabled, float level, float size,
                                  float amp, float wavelen, float speed, float swell,
                                  float dr, float dg, float db, float sr, float sg, float sb);
    void  g3d_editor_water_set_texture(void *tex);
    void  g3d_editor_fluid_set_texture(void *tex);
    void  g3d_editor_flow_set_texture(void *tex);
    void  g3d_flow_set_color(float r, float g, float b);
    void  g3d_flow_set_foam(float foam);
    void  g3d_flow_set_speed(float mul);
    int   g3d_river_add_falls(const float *pts_xyz, int n, float width);
    int   g3d_waterfall_add(float tx, float ty, float tz, float bx, float bz, float width, float arc);
    int   g3d_editor_terrain_vcount(void *mesh);
    void  g3d_editor_terrain_snapshot(void *mesh, float *out);
    void  g3d_editor_terrain_restore(void *mesh, const float *in);
    float g3d_water_level_at(float x, float z);
    void  g3d_water_add_ripple_source(float x, float z, float strength);
    void  g3d_water_clear_ripple_sources(void);
    void  g3d_fluid_block_reset(void);
    void  g3d_fluid_block_river(const float *pts_xyz, int n, float width);
    int   g3d_lake_covers(float x, float z);
    // ---- lagos por flood-fill (agua colocada donde quieras, con la forma del hoyo) ----
    int   g3d_lake_add(float seed_x, float seed_z, float surface_y, float depth);
    float g3d_lake_spill_level(float seed_x, float seed_z);
    float g3d_lake_spill_level_r(float seed_x, float seed_z, float max_radius);
    int   g3d_lake_add_r(float seed_x, float seed_z, float surface_y, float depth, float max_radius);
    int   g3d_hydrology_analyze(float river_thresh, float min_lake_depth, const unsigned char *exclude);
    int   g3d_hydrology_lake_count(void);
    void  g3d_hydrology_lake(int i, float *x, float *z, float *level);
    int   g3d_hydrology_river_count(void);
    int   g3d_hydrology_river_len(int i);
    void  g3d_hydrology_river_point(int i, int k, float *x, float *z);
    // --- simulacion de agua (fluye con fisica: rios/cascadas automaticos) ---
    void  g3d_watersim_init(const float *heights, int side, float world_size);
    void  g3d_watersim_set_terrain(const float *heights);
    void  g3d_watersim_shutdown(void);
    int   g3d_watersim_active(void);
    int   g3d_watersim_add_source(float x, float z, float rate);
    void  g3d_watersim_clear_sources(void);
    int   g3d_watersim_source_count(void);
    int   g3d_watersim_get_source(int i, float *x, float *z, float *rate);
    void  g3d_watersim_set_rain(float rate);
    void  g3d_watersim_set_sea_level(float y);
    void  g3d_watersim_set_evaporation(float rate);
    void  g3d_watersim_set_flow_scale(float s);
    void  g3d_watersim_settle(float seconds);
    void  g3d_fluid_clear(void);
    void  g3d_fluid_set_style(float amp, float len, float speed, float dr, float dg, float db,
                              float sr, float sg, float sb, unsigned int tex, float opacity);
    // ---- rios (agua por un camino de puntos + cascadas) ----
    int   g3d_river_add(const float* pts_xyz, int n, float width);
    void  g3d_flow_clear(void);
    void  g3d_flow_set_color(float r, float g, float b);
    float g3d_water_level_at(float x, float z);   // nivel del agua (mar/lago/rio) en un punto
    void  g3d_water_ripple(float x, float z, float strength);
    void  g3d_water_splash(float x, float y, float z, float strength);
    int   g3d_editor_cave_enter(int scene, void *terrain, float world_size);
    void  g3d_editor_cave_exit(void);
    void  g3d_editor_cave_reset(void);
    int   g3d_editor_cave_carve(float sx, float sy, float w, float h, float radius, int mode, float strength, float *out);
    int   g3d_editor_cave_place(int scene_id, void *terrain, float wx, float wz, float radius, float depth, const char *texpath);
    void  g3d_editor_cave_dig(float wx, float wy, float wz, float radius, int mode, float strength);
    void  g3d_sky_set_gradient(float tr, float tg, float tb, float br, float bg, float bb);
    void  g3d_sky_set_enabled(int enabled);
    void  g3d_ibl_set_enabled(int enable);
    int   g3d_light_enable_shadow(int light_id, int enabled);
    int   g3d_model_spawn(int scene_id, void *model, float x, float y, float z,
                          float height, float yaw);
}

// ---- FBO donde el motor renderiza la escena (se muestra en una ventana ImGui) ----
struct ViewportFBO {
    GLuint fbo = 0, tex = 0, depth = 0;
    int w = 0, h = 0;
    void resize(int nw, int nh) {
        if (nw < 8) nw = 8;
        if (nh < 8) nh = 8;
        if (nw == w && nh == h && fbo) return;
        w = nw; h = nh;
        if (!fbo) glGenFramebuffers(1, &fbo);
        if (!tex) glGenTextures(1, &tex);
        if (!depth) glGenRenderbuffers(1, &depth);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

// Busca un fichero primero JUNTO AL EJECUTABLE y luego donde estaba al compilar.
// Las rutas que fija CMake son absolutas y del ordenador de quien compilo: si el
// editor se copia a otra maquina (un zip, una carpeta portable) no existen.
static std::string ruta_util(const std::string& rel_junto_al_exe,
                             const std::string& ruta_de_compilacion) {
    std::vector<std::string> cands;
    if (char* base = SDL_GetBasePath()) {
        cands.push_back(std::string(base) + rel_junto_al_exe);
        cands.push_back(std::string(base) + "../" + rel_junto_al_exe);
        SDL_free(base);
    }
    if (!ruta_de_compilacion.empty()) cands.push_back(ruta_de_compilacion);
    for (auto& c : cands) if (fs::exists(c)) return c;   // vale fichero o carpeta
    return ruta_de_compilacion;   // que falle con la ruta conocida, no con una inventada
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    // Perfil COMPATIBILITY como BennuGD2 (el motor hace draws que en CORE dan
    // GL_INVALID_OPERATION por falta de VAO por defecto).
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    int win_w = 1600, win_h = 900;
    SDL_Window* window = SDL_CreateWindow("BennuGD2 Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h,
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE));
    if (!window) { printf("CreateWindow error: %s\n", SDL_GetError()); return 1; }

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (!gl) { printf("GL context error: %s\n", SDL_GetError()); return 1; }
    SDL_GL_MakeCurrent(window, gl);
    SDL_GL_SetSwapInterval(1);
    printf("GL_VERSION = %s\n", (const char*)glGetString(GL_VERSION));


    // ---- Motor 3D ----
    int fbw, fbh; SDL_GL_GetDrawableSize(window, &fbw, &fbh);
    g3d_renderer_init((unsigned)fbw, (unsigned)fbh);
    g3d_renderer_set_viewport_physical(0, 0, (unsigned)fbw, (unsigned)fbh);

    // sombras ON: el shader del modelo muestrea el shadow map; con sombras off
    // esa textura de profundidad queda incompleta -> el driver tira el draw.
    g3d_renderer_set_hdr(0);
    g3d_renderer_set_shadows(1);
    g3d_renderer_set_shadow_resolution(2048);   // por defecto; se ajusta al cargar la escena
    g3d_renderer_set_clear_color(0.15f, 0.20f, 0.30f, 1.0f);

    int scene = g3d_scene_create("editor");
    g3d_scene_set_active(scene);
    int cam = g3d_camera_create();
    g3d_camera_set_active(cam);
    int light = g3d_light_create(0, 1.0f, 0.96f, 0.86f);
    g3d_light_set_direction(light, -0.45f, -0.75f, -0.35f);
    g3d_light_set_intensity(light, 1.5f);

    // cielo: da fondo real y, sobre todo, completa los cubemaps de IBL que el
    // shader PBR muestrea (sin esto el samplerCube queda incompleto -> el driver
    // tira el draw con "program texture usage").
    g3d_sky_set_gradient(0.35f, 0.55f, 0.85f, 0.82f, 0.88f, 0.96f);
    g3d_sky_set_enabled(1);
    g3d_light_enable_shadow(light, 1);

    // ---- PROYECTO actual (runtime; se puede abrir/crear otro .bgd2) ----
    // Carpeta de proyecto por defecto. PROJECT_DIR es absoluta y del ordenador de
    // quien compilo, asi que en un paquete portable no existe: se prefiere la que
    // venga junto al ejecutable.
    std::string project_dir = ruta_util("project", PROJECT_DIR);   // carpeta raiz del proyecto
    std::string project_name = "proyecto";

    // texturas del proyecto (para pintar el terreno) -- NO hardcodeadas
    std::string tex_dir = project_dir + "/Assets";
    struct PaintTex { std::string file; void* tex; };
    std::vector<PaintTex> paints;
    for (auto& f : scan_textures(tex_dir)) paints.push_back({ f, nullptr });   // carga perezosa
    int paint_sel = paints.empty() ? -1 : 0;
    float paint_tiling = 40.0f, paint_op = 0.5f;

    // agua (mar/lago): plano global a un nivel Y + oleaje/color/textura
    bool water_on = false;
    float water_level = 3.0f;
    float w_amp = 0.25f, w_len = 8.0f, w_speed = 1.0f, w_swell = 0.3f;
    float w_deep[3]    = { 0.02f, 0.11f, 0.20f };
    float w_shallow[3] = { 0.10f, 0.34f, 0.44f };
    int   water_tex_sel = -1;   // textura de agua (de project/Assets), -1 = ninguna

    // ---- CAMARA principal del juego (se genera en el main.prg) ----
    // modo: 0=Fija 1=Tercera persona 2=Primera persona(FPS) 3=Cenital(top-down)
    int   cam_mode   = 0;
    int   cam_follow = -1;                    // indice del objeto a seguir (-1 = ninguno)
    float cam_pos[3]  = { 0.0f, 45.0f, -90.0f };  // posicion (modo Fija)
    float cam_look[3] = { 0.0f,  2.0f,   0.0f };  // punto de mira (modo Fija)
    float gcam_dist   = 8.0f;                   // distancia detras/altura del objetivo
    float cam_height = 3.0f;                    // altura de la camara sobre el objetivo
    // FPS: cuanto se adelanta la camara respecto al centro del personaje. Sin esto
    // queda DENTRO del modelo y se ven las caras interiores de la cabeza y el
    // torso. Adelantada lo justo, se siguen viendo los brazos y las piernas.
    float cam_fwd = 0.45f;
    // Sensibilidad del raton en FPS: milesimas de grado por pixel de movimiento.
    // Con el modo relativo de SDL, dx/dy ya son el movimiento del frame, no una
    // posicion absoluta, asi que este numero se comporta de forma estable.
    float cam_sens = 120.0f;
    // Resolucion del shadow map. El motor arranca en 1024 (se ven los dientes en
    // terreno grande); 2048 es un buen equilibrio, 4096 muy nitido.
    int shadow_res = 2048;
    // carga (perezosa) la textura de pintura i; devuelve su puntero
    auto paint_tex = [&](int i) -> void* {
        if (i < 0 || i >= (int)paints.size()) return nullptr;
        if (!paints[i].tex) paints[i].tex = g3d_editor_load_texture((tex_dir + "/" + paints[i].file).c_str());
        return paints[i].tex;
    };

    // terreno esculpible: base = primera textura del proyecto (o sin textura)
    std::string base_path = paints.empty() ? "" : (tex_dir + "/" + paints[0].file);
    void* terrain = g3d_editor_make_terrain(scene, 160, 400.0f, 40.0f,
                                            base_path.empty() ? nullptr : base_path.c_str());
    g3d_zone_init(161, 400.0f);   // mascara de zonas: misma extension que el terreno (grid 160)

    // ---- lagos colocados (agua con la forma de un hoyo del terreno) ----
    // Efectos PROPIOS de cada masa de agua (olas, color, textura). Cada lago/rio
    // guarda los suyos; se siembran con los del panel Entorno al crearse y luego
    // se editan por separado en el Inspector.
    struct WaterFX {
        float amp = 0.12f, len = 5.0f, speed = 1.1f;
        float deep[3]    = { 0.02f, 0.11f, 0.20f };
        float shallow[3] = { 0.10f, 0.34f, 0.44f };
        int   tex = -1;   // indice en paints, -1 = ninguna
    };
    // Efectos PROPIOS de la cascada de un rio (shader de flujo, distinto del agua):
    // textura que cae, velocidad, espuma y color. Se editan aparte en el Inspector.
    struct WaterfallFX {
        int   tex = -1;          // indice en paints, -1 = ninguna (procedural)
        float speed = 1.0f;      // velocidad de caida
        float foam = 1.0f;       // intensidad de espuma
        float color[3] = { 0.60f, 0.78f, 0.85f };
    };
    struct Lake { float sx, sz, level, depth; float radius; WaterFX fx; };
    std::vector<Lake> lakes;
    bool  lake_auto  = true;    // nivel automatico (justo antes de desbordar)
    float lake_level = -1.0f;   // nivel manual del agua
    float lake_depth = 4.0f;    // profundidad (para el tinte y la fisica)
    float lake_radius = 0.0f;   // radio max del llenado (0 = sin limite); acota hoyos abiertos
    // previsualizacion en vivo del lago (como Planet Coaster): al pasar el raton se
    // ve el agua que se colocaria; la rueda sube/baja el nivel.
    bool  lake_prev_on = false;
    Lake  lake_prev{0,0,0,4.0f,0.0f,{}};
    long  lake_prev_key = -1;      // celda+nivel para no rehacer cada frame
    // ---- rios (agua por un camino de puntos clicados) ----
    // terrain_before = relieve del terreno JUSTO ANTES de excavar este rio, para
    // restaurarlo (rellenar el cauce) si el rio se borra.
    struct River { std::vector<float> pts; float width, depth; WaterFX fx;
                   WaterfallFX wf;                          // efectos de sus cascadas
                   std::vector<float> terrain_before; };   // pts = pares x,z
    std::vector<River> rivers;
    std::vector<float> river_draft;   // rio que se esta trazando (pares x,z)
    // ---- cascadas (elemento propio: borde arriba -> base abajo) ----
    struct Waterfall { float top[3]; float base[3]; float width; float arc; WaterfallFX fx; };
    std::vector<Waterfall> waterfalls;
    bool  wf_have_top = false;         // 1er clic puesto (esperando la base)
    float wf_top[3] = {0,0,0};         // borde superior en curso
    float wf_width = 6.0f;             // ancho de la proxima cascada
    float wf_arc = 0.0f;               // arco (comba) de la proxima cascada
    float river_width = 6.0f;
    float river_depth = 3.0f;    // cuanto se excava el lecho bajo el terreno
    bool  water_fx_dirty = false; // hay cambios de efectos pendientes de reconstruir
    // ---- agua automatica (hidrologia) ----
    std::vector<float> hyd_base;       // relieve base (antes de excavar cauces auto)
    float hyd_river_thresh = 600.0f;   // caudal minimo para que sea rio (sensibilidad)
    float hyd_lake_depth   = 2.0f;     // profundidad minima para que un hoyo sea lago
    // ---- simulacion de agua (manantiales que fluyen) ----
    struct WSource { float x, z, rate; };
    std::vector<WSource> wsources;     // fuentes colocadas (para guardar/regenerar)
    float ws_rate = 6.0f;              // caudal de la proxima fuente (potente: llena rapido)
    float ws_evap = 0.0f;             // evaporacion (0 = el agua se QUEDA, no se seca)
    float ws_flow = 1.0f;             // velocidad de flujo

    // Excava un cauce en el terreno siguiendo un camino de puntos (pares x,z): baja
    // el terreno a un lecho `depth` por debajo del relieve original, con pendiente
    // entre puntos. Se llama UNA vez al crear el rio (modifica el terreno, que se
    // guarda). Asi el agua tiene un lecho de verdad y se ve como un rio.
    auto carve_river = [&](const std::vector<float>& pts, float width, float depth) {
        if (!terrain || (int)pts.size() < 4) return;
        int n = (int)pts.size() / 2;
        float r = width * 0.6f;
        // Muestrea DENSO el relieve ORIGINAL a lo largo de todo el cauce ANTES de
        // excavar. El lecho = relieve original - profundidad, siguiendo el terreno
        // (incl. acantilados -> cascadas), en vez de una rampa recta entre los
        // puntos clicados (que aplanaba las caidas y mataba las cascadas).
        struct DP { float x, z, oh; };
        std::vector<DP> dense;
        for (int seg = 0; seg < n - 1; seg++) {
            float ax = pts[seg*2], az = pts[seg*2+1];
            float bx = pts[(seg+1)*2], bz = pts[(seg+1)*2+1];
            float dx = bx-ax, dz = bz-az; float len = sqrtf(dx*dx+dz*dz);
            int steps = (int)(len / (r * 0.5f)); if (steps < 1) steps = 1;
            for (int s = (seg == 0 ? 0 : 1); s <= steps; s++) {
                float t = (float)s / steps, x = ax + dx*t, z = az + dz*t;
                dense.push_back({ x, z, g3d_editor_terrain_height(terrain, x, z) });
            }
        }
        for (auto& d : dense)
            g3d_editor_terrain_flatten_to(terrain, d.x, d.z, r, d.oh - depth, 1.0f);
        // Suaviza los TALUDES a lo largo del cauce para que la hierba baje en
        // pendiente (no un borde duro), PERO no en los tramos con caida fuerte
        // (acantilados), para no alisar la cara de la cascada. Recorre los puntos
        // densos (que ya llevan la altura original) y salta donde el desnivel local
        // es grande.
        for (size_t k = 0; k + 1 < dense.size(); k++) {
            float ddx = dense[k+1].x - dense[k].x, ddz = dense[k+1].z - dense[k].z;
            float horiz = sqrtf(ddx*ddx + ddz*ddz) + 1e-4f;
            float drop = dense[k].oh - dense[k+1].oh;
            if (drop > 1.2f && drop > horiz*0.6f) continue;   // acantilado: no alisar
            g3d_editor_terrain_smooth(terrain, dense[k].x, dense[k].z, width*1.5f, 0.55f);
        }
    };

    // Copia el relieve actual del terreno (para poder deshacer un cauce despues).
    auto snapshot_terrain = [&]() {
        std::vector<float> snap;
        if (terrain) {
            int nv = g3d_editor_terrain_vcount(terrain);
            snap.resize(nv);
            if (nv) g3d_editor_terrain_snapshot(terrain, snap.data());
        }
        return snap;
    };

    // Reconstruye TODO el agua colocada (lagos + rios) en el motor, para el
    // preview del viewport. Se llama al colocar/borrar y tras esculpir el terreno,
    // porque el agua sigue la forma del relieve y hay que rehacerla.
    // Fija en el motor el estilo (olas + color) y la textura de UNA masa de agua,
    // justo antes de crearla, para que la zona capture ESE estilo como suyo.
    auto apply_fx = [&](const WaterFX& fx) {
        g3d_fluid_set_style(fx.amp, fx.len, fx.speed,
                            fx.deep[0], fx.deep[1], fx.deep[2],
                            fx.shallow[0], fx.shallow[1], fx.shallow[2], 0, 0.88f);
        g3d_editor_fluid_set_texture((fx.tex >= 0 && fx.tex < (int)paints.size())
                                     ? paint_tex(fx.tex) : nullptr);
    };
    // Fija en el motor los efectos de CASCADA (shader de flujo) antes de crear el
    // rio, para que sus cascadas capturen ESE estilo (textura, velocidad, espuma,
    // color) como propio, distinto del agua del rio.
    auto apply_wf = [&](const WaterfallFX& wf) {
        g3d_flow_set_color(wf.color[0], wf.color[1], wf.color[2]);
        g3d_flow_set_foam(wf.foam);
        g3d_flow_set_speed(wf.speed);
        g3d_editor_flow_set_texture((wf.tex >= 0 && wf.tex < (int)paints.size())
                                    ? paint_tex(wf.tex) : nullptr);
    };
    // FX por defecto para una masa de agua NUEVA: los del panel Entorno (agua).
    auto current_fx = [&]() {
        WaterFX fx;
        fx.amp = w_amp; fx.len = w_len; fx.speed = w_speed;
        fx.deep[0]=w_deep[0]; fx.deep[1]=w_deep[1]; fx.deep[2]=w_deep[2];
        fx.shallow[0]=w_shallow[0]; fx.shallow[1]=w_shallow[1]; fx.shallow[2]=w_shallow[2];
        fx.tex = water_tex_sel;
        return fx;
    };
    // Controles de efectos (textura + olas + color) de UNA masa de agua. uid da
    // IDs unicos por lago/rio. Devuelve true si algo cambio este frame.
    auto water_fx_editor = [&](WaterFX& fx, int uid) -> bool {
        bool ch = false;
        ImGui::PushID(uid);
        const char* cur = (fx.tex >= 0 && fx.tex < (int)paints.size())
                          ? paints[fx.tex].file.c_str() : "(ninguna)";
        if (ImGui::BeginCombo("Textura", cur)) {
            if (ImGui::Selectable("(ninguna)", fx.tex < 0)) { fx.tex = -1; ch = true; }
            for (int i = 0; i < (int)paints.size(); i++)
                if (ImGui::Selectable(paints[i].file.c_str(), fx.tex == i)) { fx.tex = i; ch = true; }
            ImGui::EndCombo();
        }
        ch |= ImGui::SliderFloat("Amplitud",  &fx.amp,   0.0f, 1.0f,  "%.2f");
        ch |= ImGui::SliderFloat("Longitud",  &fx.len,   1.0f, 40.0f, "%.1f");
        ch |= ImGui::SliderFloat("Velocidad", &fx.speed, 0.0f, 4.0f,  "%.2f");
        ch |= ImGui::ColorEdit3("Profundo",   fx.deep);
        ch |= ImGui::ColorEdit3("Superficie", fx.shallow);
        ImGui::PopID();
        return ch;
    };

    // Recorta un rio al tramo que NO esta cubierto por un lago/mar y suaviza sus
    // extremos hasta el nivel del agua que toca. Esto EVITA de raiz el parche feo de
    // dos superficies de agua transparentes solapadas (que buceando se veria fatal),
    // en vez de taparlo. Devuelve los triples xyz del tramo y los puntos de union
    // (donde el rio entra/sale del lago) para poner hondas ahi. Requiere que los
    // lagos ya esten en el motor (g3d_water_level_at los consulta).
    auto river_trimmed = [&](const River& rv, std::vector<float>& out_xyz,
                             std::vector<std::pair<float,float>>& junctions) {
        out_xyz.clear(); junctions.clear();
        int n = (int)rv.pts.size() / 2;
        if (n < 2 || !terrain) return;
        std::vector<float> surfY(n), lvlAt(n), bed(n);
        for (int k = 0; k < n; k++) {
            float x = rv.pts[k*2], z = rv.pts[k*2+1];
            bed[k]   = g3d_editor_terrain_height(terrain, x, z);
            surfY[k] = bed[k] + rv.depth*0.8f;
            lvlAt[k] = g3d_water_level_at(x, z);   // lagos/mar/rios ya anadidos
        }
        // Un punto se recorta si lo cubre el mar global (plano exacto) o la
        // superficie REAL de un lago (g3d_lake_covers, por celda del flood-fill, no
        // la caja). Como el cauce interior esta bloqueado, el lago solo cubre la boca
        // (el margen sin bloquear); ahi el rio se recorta EXACTO al borde del lago y
        // se unen sin lecho seco ni solape.
        auto covered = [&](int k){
            return (water_on && water_level > bed[k] + 0.3f) ||
                   g3d_lake_covers(rv.pts[k*2], rv.pts[k*2+1]);
        };
        int s = 0;     while (s < n  && covered(s)) s++;
        int e = n - 1; while (e >= 0 && covered(e)) e--;
        if (s > e) return;   // el rio va entero bajo un lago -> no se dibuja
        int m = e - s + 1;
        out_xyz.resize((size_t)m * 3);
        for (int k = 0; k < m; k++) {
            out_xyz[k*3]   = rv.pts[(s+k)*2];
            out_xyz[k*3+1] = surfY[s+k];
            out_xyz[k*3+2] = rv.pts[(s+k)*2+1];
        }
        if (s > 0) {           // emerge de un lago: suaviza el arranque a su nivel
            float lakeY = lvlAt[s-1];
            int bn = m < 6 ? m : 6;
            for (int i = 0; i < bn; i++) {
                float t = (bn > 1) ? (float)i/(bn-1) : 1.0f;
                out_xyz[i*3+1] = lakeY*(1.0f-t) + out_xyz[i*3+1]*t;
            }
        }
        if (e < n - 1) {       // desemboca en un lago: suaviza el final a su nivel
            float lakeY = lvlAt[e+1];
            int bn = m < 6 ? m : 6;
            for (int i = m - bn; i < m; i++) {
                float t = (bn > 1) ? (float)(i-(m-bn))/(bn-1) : 1.0f;
                out_xyz[i*3+1] = out_xyz[i*3+1]*(1.0f-t) + lakeY*t;
            }
        }
        // Honda en la union con un lago: si un extremo del tramo esta pegado a agua
        // de lago (sondeando un poco mas alla en la direccion del cauce), pon la
        // honda ahi. NO se toca la geometria del rio: el lago entra por la boca (el
        // cauce se deja sin bloquear en los extremos) y se unen solos.
        auto touches_lake = [&](float x, float z, float dx, float dz, float& jx, float& jz) {
            float L = sqrtf(dx*dx + dz*dz); if (L < 1e-4f) return false;
            dx /= L; dz /= L;
            for (float d = 1.0f; d <= 9.0f; d += 1.5f) {
                float px = x + dx*d, pz = z + dz*d;
                if (g3d_water_level_at(px, pz) > g3d_editor_terrain_height(terrain, px, pz) + 0.3f) {
                    jx = px; jz = pz; return true;
                }
            }
            return false;
        };
        if (m >= 2) {
            float jx, jz;
            if (touches_lake(out_xyz[0], out_xyz[2], out_xyz[0]-out_xyz[3], out_xyz[2]-out_xyz[5], jx, jz))
                junctions.push_back({ jx, jz });
            else if (s > 0) junctions.push_back({ out_xyz[0], out_xyz[2] });
            if (touches_lake(out_xyz[(m-1)*3], out_xyz[(m-1)*3+2],
                             out_xyz[(m-1)*3]-out_xyz[(m-2)*3], out_xyz[(m-1)*3+2]-out_xyz[(m-2)*3+2], jx, jz))
                junctions.push_back({ jx, jz });
            else if (e < n - 1) junctions.push_back({ out_xyz[(m-1)*3], out_xyz[(m-1)*3+2] });
        }
    };

    auto rebuild_water = [&]() {
        g3d_fluid_clear();
        g3d_flow_clear();
        g3d_water_clear_ripple_sources();
        if (lakes.empty() && rivers.empty() && waterfalls.empty() && !lake_prev_on) return;
        g3d_scene_set_terrain_collider(terrain);   // refresca el heightfield
        // Bloquea los cauces de los rios ANTES de crear los lagos: asi el relleno
        // del lago NO sube por el rio (antes el lago inundaba el cauce y el rio se
        // veia con agua de lago). Cada rio conserva su propia agua.
        g3d_fluid_block_reset();
        for (auto& rv : rivers) {
            int n = (int)rv.pts.size() / 2;
            if (n < 2) continue;
            std::vector<float> bx(n * 3);
            for (int k = 0; k < n; k++) { bx[k*3]=rv.pts[k*2]; bx[k*3+1]=0.0f; bx[k*3+2]=rv.pts[k*2+1]; }
            g3d_fluid_block_river(bx.data(), n, rv.width);
        }
        for (auto& lk : lakes) {
            apply_fx(lk.fx);   // cada lago captura SUS efectos
            g3d_lake_add_r(lk.sx, lk.sz, lk.level, lk.depth, lk.radius);
        }
        if (lake_prev_on) {   // lago de PREVISUALIZACION (mientras se coloca)
            apply_fx(lake_prev.fx);
            g3d_lake_add_r(lake_prev.sx, lake_prev.sz, lake_prev.level, lake_prev.depth, lake_prev.radius);
        }
        int dbg_rios = 0, dbg_jun = 0;
        for (auto& rv : rivers) {
            // Recorta el tramo cubierto por un lago (evita el parche solapado) y
            // detecta las uniones rio-lago (para hondas).
            std::vector<float> xyz;
            std::vector<std::pair<float,float>> jn;
            river_trimmed(rv, xyz, jn);
            if ((int)xyz.size() >= 6) {
                apply_fx(rv.fx);   // JUSTO antes de crear -> el rio captura SUS efectos
                g3d_river_add(xyz.data(), (int)xyz.size()/3, rv.width);   // superficie (recortada)
                dbg_rios++;
            }
            // CASCADAS del rio: con el camino COMPLETO (sin recortar). g3d_river_add_falls
            // pone una lamina donde el cauce CAE fuerte (un precipicio), siguiendo el
            // acantilado hasta la base. Asi un rio que llega a un desnivel forma cascada.
            int nf = (int)rv.pts.size() / 2;
            if (nf >= 2) {
                std::vector<float> full(nf * 3);
                for (int k = 0; k < nf; k++) { full[k*3]=rv.pts[k*2]; full[k*3+1]=0.0f; full[k*3+2]=rv.pts[k*2+1]; }
                apply_wf(rv.wf);
                g3d_river_add_falls(full.data(), nf, rv.width);
            }
            for (auto& j : jn) { g3d_water_add_ripple_source(j.first, j.second, 0.9f); dbg_jun++; }
        }
        // CASCADAS colocadas a mano (herramienta propia): cada una con su estilo.
        for (auto& wf : waterfalls) {
            apply_wf(wf.fx);
            g3d_waterfall_add(wf.top[0], wf.top[1], wf.top[2], wf.base[0], wf.base[2], wf.width, wf.arc);
        }
        (void)dbg_rios; (void)dbg_jun;
    };

    // Borra un rio Y rellena su cauce: restaura el terreno a como estaba antes de
    // excavarlo. Los rios posteriores se excavaron encima, asi que se recavan sobre
    // el terreno restaurado (y se actualiza su snapshot) para que sigan siendo
    // deshacibles. Nota: solo funciona en la sesion actual (el snapshot no se guarda).
    auto remove_river = [&](int idx) {
        if (idx < 0 || idx >= (int)rivers.size()) return;
        int nv = terrain ? g3d_editor_terrain_vcount(terrain) : 0;
        if (terrain && (int)rivers[idx].terrain_before.size() == nv && nv > 0)
            g3d_editor_terrain_restore(terrain, rivers[idx].terrain_before.data());
        for (int j = idx + 1; j < (int)rivers.size(); j++) {
            rivers[j].terrain_before = snapshot_terrain();
            carve_river(rivers[j].pts, rivers[j].width, rivers[j].depth);
        }
        rivers.erase(rivers.begin() + idx);
        rebuild_water();
    };
    auto remove_all_rivers = [&]() {
        int nv = terrain ? g3d_editor_terrain_vcount(terrain) : 0;
        if (!rivers.empty() && terrain && (int)rivers[0].terrain_before.size() == nv && nv > 0)
            g3d_editor_terrain_restore(terrain, rivers[0].terrain_before.data());
        rivers.clear();
        rebuild_water();
    };

    // AGUA AUTOMATICA (hidrologia): analiza el relieve y coloca lagos + rios
    // (excavando el cauce) + cascadas, todo conectado. Reemplaza el agua actual.
    // Luego puedes borrar a mano la masa que no quieras, o anadir mas.
    auto generate_water_auto = [&]() {
        if (!terrain) return;
        // NO se toca el terreno: el agua solo RELLENA lo que ya tengas excavado
        // (hoyos -> lagos, cauces encajados -> rios, desniveles -> cascadas).
        lakes.clear(); rivers.clear(); waterfalls.clear();
        g3d_scene_set_terrain_collider(terrain);
        if (!g3d_hydrology_analyze(hyd_river_thresh, hyd_lake_depth, nullptr)) return;
        int nr = g3d_hydrology_river_count();
        for (int i = 0; i < nr; i++) {
            int n = g3d_hydrology_river_len(i);
            if (n < 2) continue;
            std::vector<float> pts(n * 2);
            for (int k = 0; k < n; k++) { float x, z; g3d_hydrology_river_point(i, k, &x, &z); pts[k*2]=x; pts[k*2+1]=z; }
            River rv; rv.pts = pts; rv.width = river_width; rv.depth = river_depth;
            rv.fx = current_fx(); rv.wf = WaterfallFX();
            rivers.push_back(std::move(rv));   // sin excavar: el agua llena el cauce existente
        }
        int nl = g3d_hydrology_lake_count();
        for (int i = 0; i < nl; i++) {
            float x, z, lv; g3d_hydrology_lake(i, &x, &z, &lv);
            lakes.push_back({ x, z, lv, lake_depth, 0.0f, current_fx() });
        }
        rebuild_water();
    };

    // Simulacion de agua: (re)arranca sobre el terreno actual y re-aplica fuentes y
    // parametros. El agua fluye sola (rios/cascadas). Se llama al colocar la 1a
    // fuente y al esculpir (para que el agua siga el relieve nuevo).
    auto watersim_sync = [&](bool resettle) {
        if (!terrain) return;
        int nv = g3d_editor_terrain_vcount(terrain);
        int side = (int)(sqrtf((float)nv) + 0.5f);
        if (side * side != nv || side < 2) return;
        std::vector<float> hs(nv);
        g3d_editor_terrain_snapshot(terrain, hs.data());
        if (!g3d_watersim_active())
            g3d_watersim_init(hs.data(), side, 400.0f);
        else
            g3d_watersim_set_terrain(hs.data());
        g3d_watersim_set_sea_level(water_on ? water_level : -1e30f);
        g3d_watersim_set_evaporation(ws_evap);
        g3d_watersim_set_flow_scale(ws_flow);
        g3d_watersim_clear_sources();
        for (auto& s : wsources) g3d_watersim_add_source(s.x, s.z, s.rate);
        if (resettle) g3d_watersim_settle(60.0f);   // llena de golpe hasta su nivel estable
    };

    // AUTO-PINTAR el terreno con las texturas de Assets segun ALTURA y PENDIENTE:
    // hierba en lo llano/medio, roca en lo empinado, nieve en las cimas, arena/tierra
    // en lo bajo. Elige que textura es cada cosa por el NOMBRE del fichero.
    auto auto_paint_terrain = [&]() {
        if (!terrain || paints.empty()) return;
        auto find_tex = [&](std::initializer_list<const char*> keys) -> int {
            for (int i = 0; i < (int)paints.size(); i++) {
                std::string f = paints[i].file; for (auto& c : f) c = (char)tolower(c);
                for (auto k : keys) if (f.find(k) != std::string::npos) return i;
            }
            return -1;
        };
        int iGrass = find_tex({"grass","cesped","hierba","pasto","grama","green"});
        int iRock  = find_tex({"rock","roca","stone","piedra","cliff","montan","mountain"});
        int iSand  = find_tex({"sand","arena","beach","playa","dirt","tierra","desert","suelo"});
        int iSnow  = find_tex({"snow","nieve","ice","hielo"});
        if (iGrass < 0) iGrass = 0;           /* base = primera textura si no hay 'grass' */
        if (iRock  < 0) iRock  = iGrass;
        if (iSand  < 0) iSand  = iGrass;
        if (iSnow  < 0) iSnow  = iRock;
        const float half = 200.0f;            /* worldsize 400 -> [-200,200] */
        // rango de alturas (muestreo grueso)
        float hmin = 1e30f, hmax = -1e30f;
        for (float z = -half; z <= half; z += 10.0f)
            for (float x = -half; x <= half; x += 10.0f) {
                float h = g3d_editor_terrain_height(terrain, x, z);
                if (h < hmin) hmin = h; if (h > hmax) hmax = h;
            }
        float span = hmax - hmin; if (span < 1e-3f) span = 1.0f;
        // Pasada 1: base de hierba en todo el terreno
        for (float z = -half; z <= half; z += 9.0f)
            for (float x = -half; x <= half; x += 9.0f)
                if (paint_tex(iGrass)) g3d_editor_terrain_paint(terrain, paint_tex(iGrass), paint_tiling, x, z, 7.0f, 1.0f);
        // Pasada 2: roca/nieve/arena segun altura y pendiente
        const float s = 5.0f;
        for (float z = -half; z <= half; z += s)
            for (float x = -half; x <= half; x += s) {
                float h  = g3d_editor_terrain_height(terrain, x, z);
                float hL = g3d_editor_terrain_height(terrain, x-s, z), hR = g3d_editor_terrain_height(terrain, x+s, z);
                float hD = g3d_editor_terrain_height(terrain, x, z-s), hU = g3d_editor_terrain_height(terrain, x, z+s);
                float slope = (fabsf(hR-hL) + fabsf(hU-hD)) / (2.0f * s);
                float hn = (h - hmin) / span;
                int t = -1;
                if (slope > 0.7f)      t = iRock;    /* empinado -> roca */
                else if (hn > 0.82f)   t = iSnow;    /* cima -> nieve */
                else if (hn < 0.10f)   t = iSand;    /* bajo -> arena/tierra */
                if (t >= 0 && t != iGrass && paint_tex(t))
                    g3d_editor_terrain_paint(terrain, paint_tex(t), paint_tiling, x, z, s*1.3f, 1.0f);
            }
    };

    // TERRENO PROCEDURAL: genera relieve con ruido (colinas, valles y un
    // acantilado) y luego lanza el agua automatica -> paisaje con lagos, rios y
    // cascadas de un solo golpe. amplitud = cuanto relieve.
    float proc_seed = 1.0f, proc_amp = 22.0f;
    auto generate_procedural_terrain = [&](float amp) {
        if (!terrain) return;
        int nv = g3d_editor_terrain_vcount(terrain);
        int side = (int)(sqrtf((float)nv) + 0.5f);
        if (side * side != nv || side < 2) return;
        unsigned seed = (unsigned)(proc_seed) * 2654435761u + 12345u;
        auto hsh = [&](int x, int y) {
            unsigned h = seed + (unsigned)x*374761393u + (unsigned)y*668265263u;
            h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
            return (float)(h & 0xffffu) / 65535.0f;
        };
        auto vn = [&](float x, float z) {
            int xi = (int)floorf(x), zi = (int)floorf(z);
            float xf = x - xi, zf = z - zi;
            float u = xf*xf*(3-2*xf), v = zf*zf*(3-2*zf);
            float a = hsh(xi,zi), b = hsh(xi+1,zi), c = hsh(xi,zi+1), d = hsh(xi+1,zi+1);
            return (a*(1-u)+b*u)*(1-v) + (c*(1-u)+d*u)*v;   // 0..1
        };
        auto fbm = [&](float x, float z, int oct) {
            float e = 0, a = 1, f = 1, norm = 0;
            for (int o = 0; o < oct; o++) { e += a * vn(x*f, z*f); norm += a; a *= 0.5f; f *= 2.0f; }
            return e / norm;                                // 0..1
        };
        // ruido de CORDILLERA (ridged): crestas afiladas para montanas realistas
        auto ridged = [&](float x, float z, int oct) {
            float e = 0, a = 1, f = 1, norm = 0;
            for (int o = 0; o < oct; o++) {
                float r = 1.0f - fabsf(vn(x*f, z*f) * 2.0f - 1.0f); r *= r;
                e += a * r; norm += a; a *= 0.5f; f *= 2.0f;
            }
            return e / norm;
        };
        auto smooth01 = [](float a, float b, float x){ float t=(x-a)/(b-a); if(t<0)t=0; if(t>1)t=1; return t*t*(3-2*t); };
        std::vector<float> hs(nv);
        for (int j = 0; j < side; j++)
            for (int i = 0; i < side; i++) {
                float nx = (float)i / side * 3.0f, nz = (float)j / side * 3.0f;
                // DOMAIN WARP: desplaza las coords con otro ruido -> formas organicas
                float wx = fbm(nx + 5.2f, nz + 1.3f, 3) - 0.5f;
                float wz = fbm(nx + 8.3f, nz + 2.8f, 3) - 0.5f;
                float sx = nx + wx * 1.2f, sz = nz + wz * 1.2f;
                // colinas base + cordilleras (ridged) solo donde una mascara amplia dice
                float hills = fbm(sx * 1.2f, sz * 1.2f, 5);
                float mtn   = ridged(sx * 1.6f, sz * 1.6f, 5);
                float mask  = smooth01(0.42f, 0.72f, fbm(nx * 0.5f, nz * 0.5f, 2));
                float e = hills * 0.55f + mtn * mask * 1.1f;
                e = powf(e, 1.35f);             // valles mas planos, cimas marcadas
                hs[j*side+i] = (e - 0.32f) * amp;
            }
        g3d_editor_terrain_restore(terrain, hs.data());
        auto_paint_terrain();     // pinta hierba/roca/nieve/arena segun el relieve
        generate_water_auto();
    };

    // ---- ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // fuente base + iconos Font Awesome fusionados (para la toolbar profesional)
    io.Fonts->AddFontDefault();
    static const ImWchar fa_range[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig fa_cfg; fa_cfg.MergeMode = true; fa_cfg.PixelSnapH = true;
    fa_cfg.GlyphMinAdvanceX = 16.0f;
    // La fuente de iconos se busca PRIMERO junto al ejecutable y luego en la ruta
    // de compilacion. FONT_DIR se fija al configurar con CMake, asi que apunta al
    // arbol de fuentes de quien compilo: si el binario se mueve, se instala o se
    // reparte, esa ruta ya no existe. Y si no se encuentra, ImGui aborta sin
    // decir que fichero le falta, que es lo peor posible para quien lo estrena.
    {
        std::vector<std::string> cands;
        if (char* base = SDL_GetBasePath()) {
            cands.push_back(std::string(base) + "fonts/" FONT_ICON_FILE_NAME_FAS);
            cands.push_back(std::string(base) + "../fonts/" FONT_ICON_FILE_NAME_FAS);
            SDL_free(base);
        }
        cands.push_back(FONT_DIR "/" FONT_ICON_FILE_NAME_FAS);
        cands.push_back("fonts/" FONT_ICON_FILE_NAME_FAS);
        std::string hallada;
        for (auto& c : cands) { FILE* t = fopen(c.c_str(), "rb"); if (t) { fclose(t); hallada = c; break; } }
        if (!hallada.empty()) {
            io.Fonts->AddFontFromFileTTF(hallada.c_str(), 15.0f, &fa_cfg, fa_range);
        } else {
            // Sin iconos se puede trabajar: se avisa y se sigue, en vez de morir.
            fprintf(stderr,
                "AVISO: no encuentro la fuente de iconos '%s'.\n"
                "El editor arranca igual, pero los botones saldran sin icono.\n"
                "La he buscado en:\n", FONT_ICON_FILE_NAME_FAS);
            for (auto& c : cands) fprintf(stderr, "   %s\n", c.c_str());
            fprintf(stderr,
                "Copia la carpeta 'fonts' del repositorio junto al ejecutable, o\n"
                "ejecuta el editor desde la carpeta del proyecto.\n");
        }
    }

    ImGui_ImplSDL2_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ---- editor de scripts (ImGuiColorTextEdit + resaltado BennuGD2) ----
    TextEditor script;
    script.SetLanguageDefinition(BennuGD2Language());
    script.SetText(
        "// Script del objeto (proceso BennuGD2 = componente/script)\n"
        "PROCESS barril_explosivo(int id, int vida, float radio)\n"
        "PRIVATE\n"
        "    int t;\n"
        "BEGIN\n"
        "    LOOP\n"
        "        IF (vida <= 0)\n"
        "            // ... explota y avisa a los objetos en 'radio' ...\n"
        "            g3d_entity_set_scale(id, 0.0, 0.0, 0.0);\n"
        "            RETURN;\n"
        "        END\n"
        "        t = t + 1;\n"
        "        FRAME;\n"
        "    END\n"
        "END\n");

    bool show_script = false;              // el editor de script se abre a pantalla completa
    bool ask_regen = false;                // pedir confirmacion para regenerar un script
    std::string regen_obj;                 // objeto cuyo script se va a regenerar
    bool focus_script = false;             // dar foco al abrirlo (una vez)
    // ---- visor de animaciones (doble clic en un asset/objeto) ----
    bool  show_anim = false;               // ventana del visor abierta
    void* anim_model = nullptr;            // modelo previsualizado
    std::string anim_asset;                // nombre del asset (titulo)
    int   anim_sel = 0;                    // clip seleccionado
    Uint32 anim_t0 = 0;                    // tiempo base del clip (para reiniciar al cambiar)
    int   prev_scene = -1, prev_cam = -1, prev_ent = -1;   // escena/camara/entidad del preview
    void* prev_ent_model = nullptr;        // modelo que muestra prev_ent ahora
    float pv_yaw = 2.4f, pv_pitch = 0.2f, pv_dist = 4.0f;  // orbita del preview
    float pv_cx = 0, pv_cy = 0, pv_cz = 0;                 // centro del modelo (encuadre)
    // ---- Consola (panel acoplado bajo el viewport, no un popup modal) ----
    std::string game_out;                 // texto acumulado de la consola
    bool console_scroll = false;          // pedir auto-scroll al final
    bool console_focus  = false;          // traerla al frente al compilar/ejecutar
    bool last_compile_ok = false;
    FILE* run_log = nullptr;              // log del juego en ejecucion (bgdi), se lee por frames
    std::string run_log_path;
    auto console_add = [&](const std::string& s) {   // anadir texto y auto-scroll
        game_out += s; console_scroll = true;
        if (game_out.size() > 400000) game_out.erase(0, game_out.size() - 300000);  // no crecer sin fin
    };
    std::string script_obj = "barril_01";  // objeto cuyo script se edita (placeholder)
    std::string script_file;    // ruta completa del fichero abierto
    std::string script_title;   // como se llama en la barra del editor
    // ---- herramienta activa (toolbar con iconos) ----
    enum Tool { T_SELECT, T_MOVE, T_ROTATE, T_SCALE, T_PLACE, T_RAISE, T_LOWER, T_SMOOTH, T_FLATTEN, T_PAINT,
                T_HOLE, T_ZONE, T_LAKE, T_RIVER, T_WATERFALL, T_WATERSOURCE };
    bool hole_fill = false;   // T_HOLE: false=perforar, true=rellenar
    int tool = T_SELECT;
    int  zone_layer = 0;      // T_ZONE: capa (0..3) que se pinta
    bool zone_erase = false;  // T_ZONE: borrar en vez de pintar
    float brush_r = 30.0f, brush_str = 0.5f;   // pincel de terreno
    ImGuizmo::OPERATION gizmo_op = ImGuizmo::TRANSLATE;

    // ---- proyecto: navegador de Assets ----
    std::string assets_dir = project_dir + "/Assets";
    std::vector<std::string> assets = scan_assets(assets_dir);
    int asset_sel = -1;   // asset "armado" para colocar (-1 = modo seleccion)
    int drag_asset = -1;  // asset que se esta arrastrando (drag&drop desde Assets)
    int drag_ent   = -1;  // entidad "fantasma" que sigue el cursor mientras arrastras
    int last_obj_sel = -1;// para centrar la camara al cambiar de seleccion
    bool place_on_water = true;  // colocar sobre la superficie del agua (Alt = al fondo)
    printf("Proyecto: %s  (%zu assets)\n", project_dir.c_str(), assets.size());

    // ---- objetos colocados en la escena (nivel) ----
    // phys: 0=ninguna(decorativo) 1=caja 2=esfera 3=capsula 4=cilindro
    // (todos dinamicos; masa 0 se trata como estatico/inamovible)
    struct SObj {
        std::string name, asset; int entity; float x, y, z, ry, scale;
        int   phys = 0;
        float mass = 1.0f;
        float bounce = 0.2f;      // restitucion 0..1
        float friction = 0.5f;
        int   buoyant = 0;        // flota en el agua
        float density = 0.5f;     // 0..1 cuanto se hunde (0.5 = medio sumergido)
        float csize = 1.0f;       // tamano/radio de colision
        // jugador controlable (char controller + WASD + salto + animacion)
        int   is_player = 0;
        float walk_speed = 12.0f, run_speed = 26.0f, jump_force = 13.0f;
        int   anim_idle = 0, anim_walk = 1, anim_run = 2, anim_jump = 3;
        int   anim_swim = -1;    // animacion de nado (-1 = ninguna)
        float char_radius = 0.6f;  // radio de la capsula de colision del jugador
        float char_height = 3.0f;  // altura; en agua flota con la cabeza (py+altura) en la superficie
                                   //   -> menor altura = flota mas alto (util para barcos)
        // enganche a un hueso de otro objeto (p.ej. arma en la mano del jugador)
        int   attach_to = -1;            // indice del objeto padre (-1 = ninguno)
        std::string attach_bone;         // nombre (subcadena) del hueso
        float att_off[3] = { 0.0f, 0.0f, 0.0f };
        float att_scale = 1.0f;
        float att_yaw = 0.0f;            // giro extra (grados)
        int   zone_layer = -1;          // capa de zona que bloquea a este objeto (-1 = ninguna)
    };
    std::vector<SObj> objects;
    int obj_sel = -1;
    // Estado para deshacer/rehacer (los lambdas que lo usan estan mas abajo; se
    // declara aqui para poder vaciarlo al cargar una escena o un proyecto).
    struct EditState { std::vector<SObj> objs; int follow; };
    std::vector<EditState> undo_stack, redo_stack;
    EditState last_state;
    bool state_init = false;
    auto undo_reset = [&]() { undo_stack.clear(); redo_stack.clear(); state_init = false; };
    std::map<std::string, void*> model_cache;
    std::set<void*> posed_static;   // modelos sin esqueleto ya colocados (pose t=0)
    auto load_model = [&](const std::string& file) -> void* {
        auto it = model_cache.find(file);
        if (it != model_cache.end()) return it->second;
        std::string path = assets_dir + "/" + file;
        // por extension: .fbx -> loader FBX (ufbx), el resto glTF/GLB
        std::string ext; { auto d = file.rfind('.'); if (d != std::string::npos) ext = file.substr(d); }
        for (auto& c : ext) c = (char)tolower(c);
        void* m;
        // recenter por defecto (1) = como TEST_SNAKE/CHARACTER, que colocan bien el snake.
        if (ext == ".fbx") m = g3d_fbx_load(path.c_str());
        else               m = g3d_gltf_load(path.c_str());
        model_cache[file] = m;
        return m;
    };

    // Abre el visor de animaciones para un asset: carga el modelo, lo mete en una
    // escena de preview propia y encuadra la camara segun su tamano.
    auto open_anim_preview = [&](const std::string& file) {
        void* m = load_model(file);
        if (!m) return;
        anim_model = m; anim_asset = file; anim_sel = 0; anim_t0 = SDL_GetTicks();
        show_anim = true;
        if (prev_scene < 0) {                 // crear la escena de preview una sola vez
            prev_scene = g3d_scene_create("preview");
            prev_cam = g3d_camera_create();
        }
        if (prev_ent_model != m) {            // (re)spawnear el modelo a mostrar
            if (prev_ent >= 0) g3d_entity_impl_set_position(prev_ent, 0, -99999, 0);
            int save_scene_active = scene;    // spawn requiere una escena; usamos prev_scene
            g3d_scene_set_active(prev_scene);
            prev_ent = g3d_model_spawn(prev_scene, m, 0, 0, 0, 0.0f, 0.0f);
            g3d_scene_set_active(save_scene_active);
            prev_ent_model = m;
        }
        // encuadre: centro y tamano del AABB del modelo
        float mn[3], mx[3];
        if (g3d_model_bounds(m, mn, mx)) {
            pv_cx = (mn[0]+mx[0])*0.5f; pv_cy = (mn[1]+mx[1])*0.5f; pv_cz = (mn[2]+mx[2])*0.5f;
            float ex = mx[0]-mn[0], ey = mx[1]-mn[1], ez = mx[2]-mn[2];
            float s = ex; if (ey>s) s=ey; if (ez>s) s=ez;
            pv_dist = s * 1.8f + 0.5f;
        } else { pv_cx=pv_cy=pv_cz=0; pv_dist=4.0f; }
    };

    std::string scenes_dir = project_dir + "/Scenes";
    std::string scripts_dir = project_dir + "/Scripts";
    { std::error_code ec; fs::create_directories(scripts_dir, ec); }
    std::string scene_path = scenes_dir + "/level.scene";   // escena actual
    std::string status;
    bool playing = false;         // Play en marcha: no se puede editar la escena

    // ================== copiar / pegar / duplicar / borrar ==================
    SObj clipboard; bool has_clip = false;

    // Un nombre que no choque con ninguno. No vale usar objects.size(): tras
    // borrar y volver a poner salen nombres repetidos, y el nombre es el del
    // PROCESS y el del fichero de script, asi que dos iguales se pisan.
    auto unique_obj_name = [&](const std::string& base) -> std::string {
        for (int n = 1; ; n++) {
            std::string cand = base + "_" + std::to_string(n);
            bool libre = true;
            for (auto& o : objects) if (o.name == cand) { libre = false; break; }
            if (libre) return cand;
        }
    };
    auto name_base = [](const std::string& name) -> std::string {
        // "barrel_12" -> "barrel";  "pirate_ship_10" -> "pirate_ship"
        size_t u = name.rfind('_');
        if (u != std::string::npos && u + 1 < name.size()) {
            bool digits = true;
            for (size_t i = u + 1; i < name.size(); i++) if (!isdigit((unsigned char)name[i])) digits = false;
            if (digits) return name.substr(0, u);
        }
        return name;
    };
    // Al borrar un objeto, los indices de los que van detras BAJAN uno. Como
    // attach_to y cam_follow son indices, sin corregirlos el arma se engancharia
    // a otro objeto y la camara seguiria al equivocado, en silencio.
    auto fix_indices_after_erase = [&](int erased) {
        if (cam_follow == erased) cam_follow = -1;
        else if (cam_follow > erased) cam_follow--;
        for (auto& o : objects) {
            if (o.attach_to == erased) o.attach_to = -1;
            else if (o.attach_to > erased) o.attach_to--;
        }
    };
    auto delete_obj = [&](int i) {
        if (playing) { status = "Para el Play antes de editar la escena"; return; }
        if (i < 0 || i >= (int)objects.size()) return;
        g3d_entity_impl_destroy(objects[i].entity);
        objects.erase(objects.begin() + i);
        fix_indices_after_erase(i);
        if (obj_sel == i) obj_sel = -1;
        else if (obj_sel > i) obj_sel--;
    };
    // Pone una copia de `src` en el mundo. Se desplaza un poco para que no quede
    // exactamente encima del original y parezca que no ha pasado nada.
    auto spawn_copy = [&](const SObj& src, float dx, float dz) -> int {
        if (playing) { status = "Para el Play antes de editar la escena"; return -1; }
        void* m = load_model(src.asset);
        if (!m) return -1;
        SObj o = src;
        o.x += dx; o.z += dz;
        o.name = unique_obj_name(name_base(src.name));
        o.attach_to = -1;              // el enganche no se copia: apuntaria al padre del original
        // ojo con la firma: (escena, modelo, x, y, z, ALTURA, giro-Y en grados).
        o.entity = g3d_model_spawn(scene, m, o.x, o.y, o.z, 0.0f, 0.0f);
        if (o.entity < 0) return -1;
        g3d_entity_impl_set_rotation(o.entity, 0.0f, o.ry, 0.0f);
        g3d_entity_impl_set_scale(o.entity, o.scale, o.scale, o.scale);
        objects.push_back(o);
        return (int)objects.size() - 1;
    };
    auto duplicate_obj = [&](int i) {
        if (i < 0 || i >= (int)objects.size()) return;
        int n = spawn_copy(objects[i], 1.5f, 1.5f);
        if (n >= 0) { obj_sel = n; status = "Duplicado: " + objects[n].name; }
    };
    auto copy_obj = [&](int i) {
        if (i < 0 || i >= (int)objects.size()) return;
        clipboard = objects[i]; has_clip = true;
        status = "Copiado: " + objects[i].name;
    };
    auto paste_obj = [&]() {
        if (!has_clip) return;
        int n = spawn_copy(clipboard, 1.5f, 1.5f);
        if (n >= 0) { obj_sel = n; status = "Pegado: " + objects[n].name; }
    };

    // ====================== deshacer / rehacer ======================
    // Se guardan estados enteros de la escena (son 15-20 objetos, no duele) en vez
    // de instrumentar cada accion: asi no hay forma de olvidarse de una. Cada
    // frame se compara con el ultimo estado confirmado y, si cambio y no se esta
    // arrastrando nada, se apunta. Eso agrupa un arrastre entero del gizmo o de un
    // deslizador en UN solo paso de deshacer, en vez de uno por frame.
    auto obj_igual = [](const SObj& a, const SObj& b) -> bool {
        return a.name == b.name && a.asset == b.asset &&
               a.x == b.x && a.y == b.y && a.z == b.z && a.ry == b.ry && a.scale == b.scale &&
               a.phys == b.phys && a.mass == b.mass && a.bounce == b.bounce &&
               a.friction == b.friction && a.buoyant == b.buoyant && a.density == b.density &&
               a.csize == b.csize && a.is_player == b.is_player &&
               a.walk_speed == b.walk_speed && a.run_speed == b.run_speed &&
               a.jump_force == b.jump_force &&
               a.anim_idle == b.anim_idle && a.anim_walk == b.anim_walk &&
               a.anim_run == b.anim_run && a.anim_jump == b.anim_jump &&
               a.anim_swim == b.anim_swim &&
               a.char_radius == b.char_radius && a.char_height == b.char_height &&
               a.attach_to == b.attach_to && a.attach_bone == b.attach_bone &&
               a.att_off[0] == b.att_off[0] && a.att_off[1] == b.att_off[1] &&
               a.att_off[2] == b.att_off[2] &&
               a.att_scale == b.att_scale && a.att_yaw == b.att_yaw &&
               a.zone_layer == b.zone_layer;
    };
    auto state_igual = [&](const EditState& a, const EditState& b) -> bool {
        if (a.follow != b.follow || a.objs.size() != b.objs.size()) return false;
        for (size_t i = 0; i < a.objs.size(); i++)
            if (!obj_igual(a.objs[i], b.objs[i])) return false;
        return true;
    };
    // Volver a un estado: se destruyen las entidades actuales y se vuelven a crear
    // todas. Con estas cantidades es instantaneo, y sirve igual para deshacer un
    // borrado, un duplicado o un movimiento, sin casos especiales.
    auto apply_state = [&](const EditState& st) {
        for (auto& o : objects) if (o.entity >= 0) g3d_entity_impl_destroy(o.entity);
        objects = st.objs;
        cam_follow = st.follow;
        for (auto& o : objects) {
            void* m = load_model(o.asset);
            o.entity = m ? g3d_model_spawn(scene, m, o.x, o.y, o.z, 0.0f, 0.0f) : -1;
            if (o.entity >= 0) {
                g3d_entity_impl_set_rotation(o.entity, 0.0f, o.ry, 0.0f);
                g3d_entity_impl_set_scale(o.entity, o.scale, o.scale, o.scale);
            }
        }
        if (obj_sel >= (int)objects.size()) obj_sel = -1;
    };
    auto do_undo = [&]() {
        if (playing || undo_stack.empty()) return;
        redo_stack.push_back(EditState{ objects, cam_follow });
        apply_state(undo_stack.back());
        last_state = undo_stack.back();
        undo_stack.pop_back();
        status = "Deshecho (quedan " + std::to_string(undo_stack.size()) + ")";
    };
    auto do_redo = [&]() {
        if (playing || redo_stack.empty()) return;
        undo_stack.push_back(EditState{ objects, cam_follow });
        apply_state(redo_stack.back());
        last_state = redo_stack.back();
        redo_stack.pop_back();
        status = "Rehecho";
    };



    // carga el script de un objeto en el editor (o una plantilla si no existe)
    // ---------------------------------------------------------------------------
    // PLANTILLA DEL SCRIPT DE UN OBJETO
    // El comportamiento de un objeto vive en SU script, no escondido en el main:
    // controles si es el jugador, cuerpo rigido si tiene fisica. El editor solo la
    // escribe la primera vez; a partir de ahi el fichero es tuyo y no se toca.
    // ---------------------------------------------------------------------------
    auto object_script_template = [&](const SObj& o) -> std::string {
        char b[4096];

        // ---------------- OBJETO ENGANCHADO A UN HUESO (arma en la mano) ----------------
        // Sigue un hueso del personaje. Es un proceso nativo: cada frame lee donde
        // esta el hueso y se coloca ahi con x/y/z/angle_y; el motor lo dibuja al FRAME.
        if (o.attach_to >= 0 && o.attach_to < (int)objects.size()) {
            const SObj& padre = objects[o.attach_to];
            float ps = padre.scale;
            char wb[1600];
            snprintf(wb, sizeof(wb),
                "// ===== OBJETO ENGANCHADO '%s' (a un hueso de '%s') =====\n"
                "// Cada frame lee donde esta el hueso del personaje y se coloca ahi con\n"
                "// sus variables nativas. pent = entidad del personaje, pmod = su modelo,\n"
                "// nodo = el hueso. A partir de aqui es TUYO.\n"
                "PROCESS %s(int modelo, int pent, int pmod, int nodo)\n"
                "PRIVATE\n"
                "    float px; float py; float pz; float pfacing; float rr;\n"
                "    float nx; float ny; float nz; float wx2; float wz2; float a2;\n"
                "END\n"
                "BEGIN\n"
                "    ctype = C_3D; csubtype = C3D_ENTITY;\n"
                "    size = %.3f;   // escala en %% (100 = 1.0)\n"
                "    entity = g3d_model_spawn(scene, modelo, 0.0, 0.0, 0.0, 0.0, 0.0);\n"
                "    LOOP\n"
                "        IF (nodo >= 0)\n"
                "            g3d_entity_get_position(pent, &px, &py, &pz);\n"
                "            g3d_entity_get_rotation(pent, &rr, &pfacing, &rr);   // pfacing = giro del personaje\n"
                "            nx = g3d_model_node_x(pmod, nodo) * %.3f + %.3f;   // hueso * escala + offset\n"
                "            ny = g3d_model_node_y(pmod, nodo) * %.3f + %.3f;\n"
                "            nz = g3d_model_node_z(pmod, nodo) * %.3f + %.3f;\n"
                "            a2 = pfacing;\n"
                "            wx2 =  nx * cos(a2) + nz * sin(a2);   // girar el offset con el personaje\n"
                "            wz2 = -nx * sin(a2) + nz * cos(a2);\n"
                "            x = px + wx2; y = py + ny; z = pz + wz2;   // vars nativas\n"
                "            angle_y = a2 + %.1f;\n"
                "        END\n"
                "        FRAME;\n"
                "    END\n"
                "END\n",
                o.name.c_str(), padre.name.c_str(), o.name.c_str(),
                o.att_scale * 100.0f,
                ps, o.att_off[0], ps, o.att_off[1], ps, o.att_off[2],
                o.att_yaw * 1000.0f);
            return wb;
        }

        // ---------------- JUGADOR ----------------
        if (o.is_player) {
            // Con la camara en primera persona se mira con el raton y se anda
            // respecto a donde se mira, como en cualquier FPS. En tercera persona
            // o cenital se conserva el control clasico: andas y te giras solo.
            bool fps_look = (cam_mode == 2);
            std::string s =
                "// ===== JUGADOR '" + o.name + "' =====\n"
                "// Creado por el editor con los controles dentro: a partir de aqui es TUYO.\n"
                "// Cambia las teclas, la velocidad o anade lo que quieras (doble salto, dash...).\n"
                "//   id     = la entidad de este objeto en la escena\n"
                "//   modelo = su modelo, necesario para las animaciones\n"
                "PROCESS " + o.name + "(int id, int modelo)\n"
                "PRIVATE\n"
                "    int ch; int gnd; int inw; int moja;\n"
                "    float wx; float wz; float wl; float spd; float t; float facing; float wlev;\n"
                "    float mirax; float miray; float adel; float lat;\n"
                "    float px; float py; float pz; float prevx; float prevz; float dt; float ript;\n"
                "END\n"
                "BEGIN\n"
                "    // Proceso BennuGD2: usa sus variables nativas. La entidad la creo\n"
                "    // escena_iniciar (para que la camara pueda seguirla) y aqui se ata a\n"
                "    // la var nativa 'entity'; el motor la dibuja desde x/y/z/angle_y solo.\n"
                "    ctype = C_3D; csubtype = C3D_ENTITY;\n"
                "    entity = id;\n";
            { char sz[96]; snprintf(sz, sizeof(sz),
                "    size = %.3f;   // escala en %% (100 = 1.0)\n", o.scale * 100.0f);
              s += sz; }
            s += "    dt = 1.0 / 60.0;\n";
            // El formato se arma antes: segun el modo de camara cambian trozos
            // enteros, y eso no se puede hacer concatenando dentro del snprintf.
            std::string fmt =
                "    // capsula de colision del personaje (x, y, z, radio, altura)\n"
                "    ch = g3d_char_create(%.3f, %.3f, %.3f, %.3f, %.3f);\n"
                "    g3d_char_set_tuning(ch, 0.8, 46.0);\n"
                "    // Fuerza con la que aparta barriles, cajas y demas cuerpos fisicos.\n"
                "    // Cuanto mas pesado sea el objeto menos se movera, y si no puede con\n"
                "    // el le cortara el paso. 0 = choca pero no mueve nada.\n"
                "    g3d_char_set_push(ch, 200.0);\n"
                "    facing = 0.0; t = 0.0;\n";
            if (fps_look)
                fmt +=
                "    // Mirada con el raton, modo relativo de SDL: el puntero se oculta y\n"
                "    // se bloquea, y cada frame se lee cuanto se ha movido (dx, dy). Es lo\n"
                "    // correcto para un FPS; escribir mouse.x no recentraba y la camara se\n"
                "    // disparaba.\n"
                "    mirax = 0.0; miray = 0.0;\n"
                "    g3d_mouse_capture(1);   // ocultar y capturar el raton\n";
            fmt +=
                "\n    LOOP\n"
                "        prevx = g3d_char_x(ch); prevz = g3d_char_z(ch);\n\n";
            if (fps_look) {
                char lk[1024];
                snprintf(lk, sizeof(lk),
                "        // ---------- MIRAR (raton) ----------\n"
                "        g3d_mouse_update();   // lee el movimiento del raton de este frame\n"
                "        mirax = mirax - g3d_mouse_dx() * %.3f;   // %.3f = sensibilidad (md/pixel)\n"
                "        miray = miray - g3d_mouse_dy() * %.3f;\n"
                "        IF (miray >  85000.0) miray =  85000.0; END   // tope al mirar arriba\n"
                "        IF (miray < -85000.0) miray = -85000.0; END   // tope al mirar abajo\n"
                "        facing = mirax;\n"
                "        escena_yaw = mirax;             // la camara lo lee para girar\n"
                "        escena_pitch = miray;           // ...y para inclinarse\n\n",
                cam_sens, cam_sens, cam_sens);
                fmt += lk;
                fmt +=
                "        // ---------- ANDAR (respecto a donde miras) ----------\n"
                "        adel = 0.0; lat = 0.0;\n"
                "        IF (key(_W)) adel = adel + 1.0; END\n"
                "        IF (key(_S)) adel = adel - 1.0; END\n"
                "        IF (key(_D)) lat = lat - 1.0; END\n"
                "        IF (key(_A)) lat = lat + 1.0; END\n"
                "        wx = sin(facing) * adel + cos(facing) * lat;\n"
                "        wz = cos(facing) * adel - sin(facing) * lat;\n";
            } else {
                fmt +=
                "        // ---------- CONTROLES ----------\n"
                "        wx = 0.0; wz = 0.0;\n"
                "        IF (key(_W)) wz = wz + 1.0; END\n"
                "        IF (key(_S)) wz = wz - 1.0; END\n"
                "        IF (key(_D)) wx = wx + 1.0; END\n"
                "        IF (key(_A)) wx = wx - 1.0; END\n";
            }
            fmt +=
                "        spd = %.3f;\n"
                "        IF (key(_L_SHIFT) OR key(_R_SHIFT)) spd = %.3f; END\n\n"
                "        wl = sqrt(wx * wx + wz * wz);\n"
                "        IF (wl > 0.001)\n"
                "            wx = wx / wl * spd; wz = wz / wl * spd;\n";
            if (!fps_look)
                fmt += "            facing = atan2(wx, wz);       // mira hacia donde anda\n";
            fmt +=
                "        END\n"
                "        g3d_char_move(ch, wx, wz);\n"
                "        IF (key(_SPACE)) g3d_char_jump(ch, %.3f); END\n"
                "        g3d_char_update(ch, dt);\n\n"
                "        px = g3d_char_x(ch); py = g3d_char_y(ch); pz = g3d_char_z(ch);\n";
            snprintf(b, sizeof(b), fmt.c_str(),
                o.x, o.y, o.z, o.char_radius, o.char_height, o.walk_speed, o.run_speed, o.jump_force);
            s += b;

            {
                // Nado + ondas en el agua que haya DEBAJO (mar, lago o rio). Cada
                // frame se consulta el nivel en su posicion; si no hay agua, muy
                // negativo y no pasa nada. wl aqui es la velocidad de movimiento.
                s += "\n        // ---------- AGUA: nada y hace ondas (mar / lago / rio) ----------\n"
                     "        wlev = g3d_water_level_at(px, pz);\n"
                     "        inw = 0;\n"
                     "        IF (wlev > -100000.0)\n"
                     "            IF (py < wlev - 1.2) inw = 1; END\n"
                     "            g3d_char_set_water(ch, inw, wlev);\n"
                     "            IF (py < wlev)\n"
                     "                IF (moja == 0) g3d_water_splash(px, wlev, pz, 1.0); END   // chapuzon\n"
                     "                moja = 1;\n"
                     "                ript = ript + dt;\n"
                     "                IF (ript > 0.12 AND wl > 0.001) g3d_water_ripple(px, pz, 0.6); ript = 0.0; END\n"
                     "            ELSE\n"
                     "                moja = 0;\n"
                     "            END\n"
                     "        ELSE\n"
                     "            g3d_char_set_water(ch, 0, 0.0);   // fuera del agua: anda normal\n"
                     "            moja = 0;\n"
                     "        END\n";
            }
            if (o.zone_layer >= 0) {
                snprintf(b, sizeof(b),
                    "\n        // ---------- ZONAS: no puede entrar en la capa %d ----------\n"
                    "        IF (g3d_zone_blocked(px, pz, %d))\n"
                    "            g3d_char_set_position(ch, prevx, py, prevz);\n"
                    "            px = prevx; pz = prevz;\n"
                    "        END\n", o.zone_layer + 1, o.zone_layer);
                s += b;
            }

            s += "\n        // vars nativas: el motor coloca el modelo al hacer FRAME (sin set_position)\n"
                 "        x = px; y = py; z = pz;\n"
                 "        angle_y = facing;\n";

            void* mm = load_model(o.asset);
            if (mm && g3d_model_animation_count(mm) > 0) {
                snprintf(b, sizeof(b),
                    "\n        // ---------- ANIMACION segun lo que este haciendo ----------\n"
                    "        t = t + dt; gnd = g3d_char_grounded(ch);\n"
                    "%s"
                    "        IF (gnd == 0)\n"
                    "            g3d_model_animate(modelo, %d, t, 1);          // saltando\n"
                    "        ELSE\n"
                    "            IF (wl > 0.001)\n"
                    "                IF (key(_L_SHIFT) OR key(_R_SHIFT)) g3d_model_animate(modelo, %d, t, 1);   // corriendo\n"
                    "                ELSE g3d_model_animate(modelo, %d, t, 1); END                              // andando\n"
                    "            ELSE\n"
                    "                g3d_model_animate(modelo, %d, t, 1);      // quieto\n"
                    "            END\n"
                    "        END\n%s",
                    (o.anim_swim >= 0) ? "        IF (inw == 0)\n" : "",
                    o.anim_jump, o.anim_run, o.anim_walk, o.anim_idle,
                    (o.anim_swim >= 0) ? "        END\n" : "");
                s += b;
                if (o.anim_swim >= 0) {
                    snprintf(b, sizeof(b),
                        "        IF (inw) g3d_model_animate(modelo, %d, t, 1); END   // nadando\n", o.anim_swim);
                    s += b;
                }
            }
            s += "\n        FRAME;\n    END\nEND\n";
            return s;
        }

        // ---------------- OBJETO CON FISICA ----------------
        if (o.phys >= 1 && o.phys <= 4) {
            float c = o.csize > 0.05f ? o.csize : 0.5f;
            float by0 = o.y + c;             // el cuerpo se apoya donde lo colocaste
            char hdr[1400];
            snprintf(hdr, sizeof(hdr),
                "// ===== OBJETO CON FISICA '%s' =====\n"
                "// Proceso BennuGD2: es el objeto. Usa sus variables nativas (x, y, z,\n"
                "// angle_x/y/z) y el motor lo dibuja solo al hacer FRAME; no hay que\n"
                "// llamar a set_position. La fisica manda: cada frame se leen las coords\n"
                "// del cuerpo rigido y se escriben en x, y, z. A partir de aqui es TUYO.\n"
                "PROCESS %s(int modelo)\n"
                "PRIVATE\n"
                "    int cuerpo; int moja; float bx; float by; float bz; float mov; float wl;\n"
                "    float prevx; float prevy; float prevz; float dt; float ript;\n"
                "END\n"
                "BEGIN\n"
                "    ctype = C_3D; csubtype = C3D_ENTITY;\n"
                "    x = %.3f; y = %.3f; z = %.3f;   // posicion inicial (vars nativas)\n"
                "    angle_y = %.3f;                 // giro en milesimas de grado\n"
                "    size = %.3f;                    // escala en %% (100 = 1.0)\n"
                "    entity = g3d_model_spawn(scene, modelo, x, y, z, 0.0, 0.0);\n"
                "    dt = 1.0 / 60.0;\n",
                o.name.c_str(), o.name.c_str(),
                o.x, o.y, o.z, o.ry * 57295.78f, o.scale * 100.0f);
            std::string s = hdr;
            // Posar una vez los modelos sin esqueleto con piezas atadas a nodos.
            { void* mm = load_model(o.asset);
              if (mm && g3d_model_animation_count(mm) > 0 && !g3d_model_is_skinned(mm))
                  s += "    g3d_model_animate_all(modelo, 0.0, 0);   // posar (piezas en nodos)\n"; }
            const char* mk =
                (o.phys == 1) ? "    cuerpo = g3d_rigidbody_create(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n"
              : (o.phys == 2) ? "    cuerpo = g3d_rigidbody_create_sphere(%.3f, %.3f, %.3f, %.3f, %.3f);\n"
              : (o.phys == 3) ? "    cuerpo = g3d_rigidbody_create_capsule(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n"
                              : "    cuerpo = g3d_rigidbody_create_cylinder(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n";
            if (o.phys == 1)      snprintf(b, sizeof(b), mk, o.x, by0, o.z, c, c, c, o.mass);
            else if (o.phys == 2) snprintf(b, sizeof(b), mk, o.x, by0, o.z, c, o.mass);
            else                  snprintf(b, sizeof(b), mk, o.x, by0, o.z, c, c, o.mass);
            s += b;
            snprintf(b, sizeof(b), "    g3d_rigidbody_set_bounce(cuerpo, %.3f, %.3f);   // rebote, friccion\n",
                     o.bounce, o.friction);
            s += b;
            s += "\n    LOOP\n";

            // Agua: el objeto flota y salpica en el agua que tenga DEBAJO, sea el
            // mar, un lago o un rio. Cada frame se consulta el nivel de agua en su
            // posicion (g3d_water_level_at); si no hay agua ahi, devuelve un valor
            // muy negativo y no pasa nada. Un cuerpo fijo (masa 0) no se mueve, asi
            // que esto solo se genera para los que si.
            if (o.mass > 0.0f) {
                float dens = o.density > 0.05f ? o.density : 0.05f;
                snprintf(b, sizeof(b),
                    "        // ---------- AGUA BAJO EL OBJETO (mar / lago / rio) ----------\n"
                    "        bx = g3d_rigidbody_x(cuerpo); by = g3d_rigidbody_y(cuerpo);\n"
                    "        bz = g3d_rigidbody_z(cuerpo);\n"
                    "        wl = g3d_water_level_at(bx, bz);   // nivel del agua aqui (o muy negativo)\n"
                    "        IF (wl > -100000.0)\n", 0);
                s += b;
                if (o.buoyant) {
                    snprintf(b, sizeof(b),
                    "            // flota en ESTE nivel (el motor reparte el empuje por el volumen\n"
                    "            // sumergido: profundidad, enderezado y vuelco salen solos)\n"
                    "            g3d_rigidbody_set_buoyancy(cuerpo, wl, %.3f);\n", dens);
                    s += b;
                }
                snprintf(b, sizeof(b),
                    "            IF (by - %.3f < wl)\n"
                    "                IF (moja == 0)\n"
                    "                    g3d_water_splash(bx, wl, bz, 1.0);   // chapuzon al entrar\n"
                    "                    g3d_rigidbody_set_damping(cuerpo, 2.5, 3.0);   // el agua frena\n"
                    "                END\n"
                    "                moja = 1;\n"
                    "                // agita el agua solo mientras SE MUEVE cerca de la superficie\n"
                    "                mov = (bx-prevx)*(bx-prevx) + (by-prevy)*(by-prevy) + (bz-prevz)*(bz-prevz);\n"
                    "                IF (mov > 0.0004 AND by + %.3f > wl - 1.0)\n"
                    "                    ript = ript + dt;\n"
                    "                    IF (ript > 0.15) g3d_water_ripple(bx, bz, 0.4); ript = 0.0; END\n"
                    "                END\n"
                    "            ELSE\n"
                    "                IF (moja == 1) g3d_rigidbody_set_damping(cuerpo, 0.05, 0.05); END\n"
                    "                moja = 0;\n"
                    "            END\n"
                    "        ELSE\n"
                    "            IF (moja == 1) g3d_rigidbody_set_damping(cuerpo, 0.05, 0.05); END\n"
                    "            moja = 0;\n", c, c);
                s += b;
                if (o.buoyant)
                    s += "            g3d_rigidbody_set_buoyancy(cuerpo, wl, 0.0);   // sin agua: sin flotacion\n";
                s += "        END\n"
                     "        prevx = bx; prevy = by; prevz = bz;\n\n";
            }
            s += "        // La fisica manda: se escriben sus coords en las vars nativas y\n"
                 "        // el motor coloca el modelo solo al hacer FRAME (sin set_position).\n"
                 "        x = g3d_rigidbody_render_x(cuerpo);\n"
                 "        y = g3d_rigidbody_render_y(cuerpo);\n"
                 "        z = g3d_rigidbody_render_z(cuerpo);\n"
                 "        angle_x = g3d_rigidbody_angle_x(cuerpo);\n"
                 "        angle_y = g3d_rigidbody_angle_y(cuerpo);\n"
                 "        angle_z = g3d_rigidbody_angle_z(cuerpo);\n"
                 "        FRAME;\n    END\nEND\n";
            return s;
        }

        // ---------------- OBJETO DECORATIVO (sin fisica) ----------------
        // Modelos sin esqueleto con piezas atadas a nodos animados hay que posarlos
        // UNA vez o esas piezas no se colocan y no se ven.
        std::string pose;
        { void* mm = load_model(o.asset);
          if (mm && g3d_model_animation_count(mm) > 0 && !g3d_model_is_skinned(mm))
              pose = "    g3d_model_animate_all(modelo, 0.0, 0);   // posar (piezas en nodos)\n"; }
        char dh[900];
        snprintf(dh, sizeof(dh),
            "// ===== OBJETO '%s' =====\n"
            "// Proceso BennuGD2: es el objeto. Usa sus variables nativas (x, y, z,\n"
            "// angle_y, size) y el motor lo dibuja al hacer FRAME. Ponle aqui la\n"
            "// logica que quieras: girarlo, moverlo, lo que sea.\n"
            "PROCESS %s(int modelo)\n"
            "BEGIN\n"
            "    ctype = C_3D; csubtype = C3D_ENTITY;\n"
            "    x = %.3f; y = %.3f; z = %.3f;\n"
            "    angle_y = %.3f;   // milesimas de grado\n"
            "    size = %.3f;      // escala en %% (100 = 1.0)\n"
            "    entity = g3d_model_spawn(scene, modelo, x, y, z, 0.0, 0.0);\n"
            "%s"
            "    LOOP\n"
            "        // ... tu logica (por ejemplo: angle_y = angle_y + 500; para girarlo) ...\n"
            "        FRAME;\n"
            "    END\n"
            "END\n",
            o.name.c_str(), o.name.c_str(),
            o.x, o.y, o.z, o.ry * 57295.78f, o.scale * 100.0f, pose.c_str());
        return dh;
    };


    // ---- Scripts generados: marca para saber si los has tocado ----
    // Los que escribe el editor llevan una primera linea con un hash del resto.
    // Si al releerlo el hash cuadra, el script sigue siendo palabra por palabra el
    // que genero el editor: se puede rehacer sin que pierdas nada. Si no cuadra (o
    // no hay marca) es codigo tuyo y no se toca sin que lo pidas expresamente.
    const char* SCRIPT_MARK = "// [editor:generado ";
    auto script_hash = [](const std::string& body) -> unsigned {
        unsigned h = 2166136261u;                       // FNV-1a
        for (unsigned char c : body) { h ^= c; h *= 16777619u; }
        return h;
    };
    auto script_read = [&](const std::string& objname) -> std::string {
        std::string sp = scripts_dir + "/" + objname + ".prg";
        FILE* f = fopen(sp.c_str(), "r");
        if (!f) return std::string();
        std::string t; char buf[1024]; size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) t.append(buf, n);
        fclose(f);
        return t;
    };
    // ¿Existe y sigue tal cual lo dejo el editor?
    auto script_untouched = [&](const std::string& objname) -> bool {
        std::string t = script_read(objname);
        if (t.compare(0, strlen(SCRIPT_MARK), SCRIPT_MARK) != 0) return false;
        size_t nl = t.find('\n');
        if (nl == std::string::npos) return false;
        unsigned marcado = 0;
        if (sscanf(t.c_str() + strlen(SCRIPT_MARK), "%x", &marcado) != 1) return false;
        return script_hash(t.substr(nl + 1)) == marcado;
    };
    auto script_write_generated = [&](const SObj& o) -> bool {
        std::string body = object_script_template(o);
        char mark[128];
        snprintf(mark, sizeof(mark),
                 "%s%08x] lo mantiene el editor: si lo editas a mano, deja de tocarlo\n",
                 SCRIPT_MARK, script_hash(body));
        std::string sp = scripts_dir + "/" + o.name + ".prg";
        FILE* f = fopen(sp.c_str(), "w");
        if (!f) return false;
        fputs(mark, f);
        fwrite(body.data(), 1, body.size(), f);
        fclose(f);
        return true;
    };

    auto open_object_script = [&](const std::string& objname) {
        script_obj = objname;
        show_script = true; focus_script = true;
        std::string sp = scripts_dir + "/" + objname + ".prg";
        script_file = sp; script_title = "Script del objeto:  Scripts/" + objname + ".prg";
        FILE* f = fopen(sp.c_str(), "r");
        if (f) {
            std::string t; char buf[1024]; size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) t.append(buf, n);
            fclose(f);
            script.SetText(t);
        } else {
            // No existe: se crea con la plantilla que corresponda (jugador con sus
            // controles, objeto con fisica con su cuerpo rigido, o vacia).
            const SObj* po = nullptr;
            for (auto& o : objects) if (o.name == objname) { po = &o; break; }
            script.SetText(po ? object_script_template(*po)
                              : ("PROCESS " + objname + "(int id)\nBEGIN\n    LOOP\n        FRAME;\n    END\nEND\n"));
        }
    };
    // Guardar la escena: una linea OBJECT por objeto (fuente de verdad del juego).
    auto save_scene = [&](const std::string& path) {
        FILE* f = fopen(path.c_str(), "w");
        if (!f) { status = "ERROR guardando escena"; return; }
        fputs("# escena del editor BennuGD2\n", f);
        fprintf(f, "WATER %d %.4f %.4f %.4f %.4f %.4f %.3f %.3f %.3f %.3f %.3f %.3f\n",
                water_on ? 1 : 0, water_level, w_amp, w_len, w_speed, w_swell,
                w_deep[0], w_deep[1], w_deep[2], w_shallow[0], w_shallow[1], w_shallow[2]);
        fprintf(f, "CAMERA %d %d %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %d\n",
                cam_mode, cam_follow, cam_pos[0], cam_pos[1], cam_pos[2],
                cam_look[0], cam_look[1], cam_look[2], gcam_dist, cam_height, cam_fwd, cam_sens,
                shadow_res);
        auto fprint_fx = [&](const WaterFX& x) {
            fprintf(f, " FX %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %d",
                    x.amp, x.len, x.speed, x.deep[0], x.deep[1], x.deep[2],
                    x.shallow[0], x.shallow[1], x.shallow[2], x.tex);
        };
        for (auto& lk : lakes) {
            fprintf(f, "LAKE %.4f %.4f %.4f %.4f", lk.sx, lk.sz, lk.level, lk.depth);
            fprint_fx(lk.fx);
            fprintf(f, " R %.3f", lk.radius);
            fputc('\n', f);
        }
        for (auto& rv : rivers) {
            fprintf(f, "RIVER %.3f %.3f %d", rv.width, rv.depth, (int)rv.pts.size()/2);
            for (float c : rv.pts) fprintf(f, " %.3f", c);
            fprint_fx(rv.fx);
            fprintf(f, " WF %d %.3f %.3f %.4f %.4f %.4f",
                    rv.wf.tex, rv.wf.speed, rv.wf.foam, rv.wf.color[0], rv.wf.color[1], rv.wf.color[2]);
            fputc('\n', f);
        }
        for (auto& w : waterfalls) {
            fprintf(f, "WATERFALL %.3f %.3f %.3f %.3f %.3f %.3f %.3f %d %.3f %.3f %.4f %.4f %.4f %.3f\n",
                    w.top[0], w.top[1], w.top[2], w.base[0], w.base[1], w.base[2], w.width,
                    w.fx.tex, w.fx.speed, w.fx.foam, w.fx.color[0], w.fx.color[1], w.fx.color[2], w.arc);
        }
        for (auto& o : objects) {
            fprintf(f, "OBJECT %s %.4f %.4f %.4f %.4f %.4f SCRIPT %s PHYS %d %.3f %.3f %.3f %d %.3f %.3f",
                    o.asset.c_str(), o.x, o.y, o.z, o.ry, o.scale, o.name.c_str(),
                    o.phys, o.mass, o.bounce, o.friction, o.buoyant, o.density, o.csize);
            if (o.is_player)
                fprintf(f, " PLAYER %.3f %.3f %.3f %d %d %d %d %d %.3f %.3f",
                        o.walk_speed, o.run_speed, o.jump_force,
                        o.anim_idle, o.anim_walk, o.anim_run, o.anim_jump, o.anim_swim,
                        o.char_radius, o.char_height);
            if (o.zone_layer >= 0)
                fprintf(f, " ZONELAYER %d", o.zone_layer);
            // ATTACH va al final: el nombre del hueso ocupa el resto de la linea
            if (o.attach_to >= 0)
                fprintf(f, " ATTACH %d %.3f %.3f %.3f %.3f %.3f %s",
                        o.attach_to, o.att_off[0], o.att_off[1], o.att_off[2],
                        o.att_scale, o.att_yaw, o.attach_bone.c_str());
            fputs("\n", f);
        }
        fclose(f);
        if (terrain) g3d_editor_terrain_save(terrain, (path + ".terrain").c_str());
        g3d_editor_paint_save((path + ".paint.png").c_str());   // pintado del terreno
        g3d_zone_save((path + ".zones").c_str());               // zonas de barrera
        scene_path = path;
        status = "Guardada " + fs::path(path).filename().string() +
                 " (" + std::to_string(objects.size()) + " objetos + terreno)";
    };
    // Cargar la escena: oculta lo actual y respawnea desde el fichero.
    auto load_scene = [&](const std::string& path) {
        FILE* f = fopen(path.c_str(), "r");
        if (!f) { status = "No pude abrir la escena"; return; }
        scene_path = path;
        // Cargar es empezar de cero: la pila de deshacer de la escena anterior no
        // vale, y si se dejara, un Ctrl+Z de mas restauraria un estado vacio.
        undo_reset();
        for (auto& o : objects) if (o.entity >= 0) g3d_entity_impl_destroy(o.entity);
        objects.clear(); obj_sel = -1;
        lakes.clear(); rivers.clear(); river_draft.clear(); waterfalls.clear();
        // terreno primero: las cuevas/objetos se apoyan en su altura
        if (terrain) g3d_editor_terrain_load(terrain, (path + ".terrain").c_str());
        g3d_editor_paint_load((path + ".paint.png").c_str());
        if (!g3d_zone_load((path + ".zones").c_str())) g3d_zone_init(161, 400.0f);  // zonas (o limpia)
        char line[512], asset[256], name[256];
        while (fgets(line, sizeof(line), f)) {
            int won; float wl, wa, wln, wsp, wsw, d0,d1,d2, s0,s1,s2;
            int nw = sscanf(line, "WATER %d %f %f %f %f %f %f %f %f %f %f %f",
                            &won, &wl, &wa, &wln, &wsp, &wsw, &d0,&d1,&d2, &s0,&s1,&s2);
            if (nw >= 2) {
                water_on = won; water_level = wl;
                if (nw >= 6) { w_amp=wa; w_len=wln; w_speed=wsp; w_swell=wsw; }
                if (nw >= 12) { w_deep[0]=d0;w_deep[1]=d1;w_deep[2]=d2;
                                w_shallow[0]=s0;w_shallow[1]=s1;w_shallow[2]=s2; }
                continue;
            }
            // Lee un sufijo opcional "FX amp len speed d0 d1 d2 s0 s1 s2 tex" del
            // cursor p (si esta). Deja fx con los valores por defecto si no hay.
            auto parse_fx = [](char* p, WaterFX& fx) {
                char* q = strstr(p, "FX ");
                if (!q) return;
                float v[9]; int tex = -1;
                if (sscanf(q, "FX %f %f %f %f %f %f %f %f %f %d",
                           &v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7],&v[8],&tex) == 10) {
                    fx.amp=v[0]; fx.len=v[1]; fx.speed=v[2];
                    fx.deep[0]=v[3]; fx.deep[1]=v[4]; fx.deep[2]=v[5];
                    fx.shallow[0]=v[6]; fx.shallow[1]=v[7]; fx.shallow[2]=v[8];
                    fx.tex = tex;
                }
            };
            { float lsx, lsz, llv, ldp;
              if (sscanf(line, "LAKE %f %f %f %f", &lsx,&lsz,&llv,&ldp) == 4) {
                  Lake lk{ lsx, lsz, llv, ldp, 0.0f, {} };
                  parse_fx(line, lk.fx);
                  char* r = strstr(line, " R ");
                  if (r) sscanf(r, " R %f", &lk.radius);
                  lakes.push_back(lk); continue; } }
            if (strncmp(line, "RIVER ", 6) == 0) {
                char* p = line + 6; char* end;
                float rw = strtof(p, &end);
                if (end != p) {
                    p = end;
                    float rd = strtof(p, &end); p = end;
                    long rn = strtol(p, &end, 10);
                    if (end != p && rn >= 2 && rn < 256) {
                        p = end;
                        River rv; rv.width = rw; rv.depth = (rd > 0.1f ? rd : 3.0f);
                        for (int k = 0; k < (int)rn*2; k++) {
                            float val = strtof(p, &end);
                            if (end == p) break;
                            rv.pts.push_back(val); p = end;
                        }
                        parse_fx(p, rv.fx);
                        char* w = strstr(p, "WF ");
                        if (w) {
                            int wt = -1; float wsp, wfo, wr, wg, wb;
                            if (sscanf(w, "WF %d %f %f %f %f %f", &wt,&wsp,&wfo,&wr,&wg,&wb) == 6) {
                                rv.wf.tex=wt; rv.wf.speed=wsp; rv.wf.foam=wfo;
                                rv.wf.color[0]=wr; rv.wf.color[1]=wg; rv.wf.color[2]=wb;
                            }
                        }
                        if ((int)rv.pts.size() == (int)rn*2) rivers.push_back(rv);
                    }
                }
                continue;
            }
            { Waterfall w; int wt = -1; w.arc = 0.0f;
              if (sscanf(line, "WATERFALL %f %f %f %f %f %f %f %d %f %f %f %f %f %f",
                         &w.top[0],&w.top[1],&w.top[2], &w.base[0],&w.base[1],&w.base[2], &w.width,
                         &wt, &w.fx.speed, &w.fx.foam, &w.fx.color[0],&w.fx.color[1],&w.fx.color[2],
                         &w.arc) >= 13) {
                  w.fx.tex = wt; waterfalls.push_back(w); continue; } }
            int cm, cfol, sres = 2048; float px,py,pz, lx,ly,lz, cd, ch, cf = 0.45f, cs = 120.0f;
            int nleidos = sscanf(line, "CAMERA %d %d %f %f %f %f %f %f %f %f %f %f %d",
                       &cm, &cfol, &px,&py,&pz, &lx,&ly,&lz, &cd, &ch, &cf, &cs, &sres);
            if (nleidos >= 10) {   // las escenas de antes no traen el adelanto
                cam_mode = cm; cam_follow = cfol;
                cam_pos[0]=px; cam_pos[1]=py; cam_pos[2]=pz;
                cam_look[0]=lx; cam_look[1]=ly; cam_look[2]=lz;
                gcam_dist = cd; cam_height = ch; cam_fwd = cf; cam_sens = cs;
                shadow_res = sres; g3d_renderer_set_shadow_resolution((unsigned)shadow_res);
                continue;
            }
            float x, y, z, ry, sc;
            if (sscanf(line, "OBJECT %255s %f %f %f %f %f SCRIPT %255s",
                       asset, &x, &y, &z, &ry, &sc, name) >= 6) {
                void* m = load_model(asset);
                if (!m) continue;
                int e = g3d_model_spawn(scene, m, x, y, z, 0.0f, 0.0f);
                SObj o; o.asset = asset; o.name = name; o.entity = e;
                o.x = x; o.y = y; o.z = z; o.ry = ry; o.scale = sc;
                // fisica opcional (escenas antiguas no la traen -> se queda por defecto)
                const char* ph = strstr(line, " PHYS ");
                if (ph) {
                    int pt, bu; float ms, bo, fr, de, cs;
                    if (sscanf(ph, " PHYS %d %f %f %f %d %f %f", &pt,&ms,&bo,&fr,&bu,&de,&cs) == 7) {
                        o.phys=pt; o.mass=ms; o.bounce=bo; o.friction=fr;
                        o.buoyant=bu; o.density=de; o.csize=cs;
                    }
                }
                const char* pl = strstr(line, " PLAYER ");
                if (pl) {
                    float ws, rs, jf; int ai, aw, ar, aj, asw = -1;
                    float cr = 0.6f, chh = 3.0f;
                    int n = sscanf(pl, " PLAYER %f %f %f %d %d %d %d %d %f %f",
                                   &ws,&rs,&jf,&ai,&aw,&ar,&aj,&asw,&cr,&chh);
                    if (n >= 7) {
                        o.is_player = 1; o.walk_speed=ws; o.run_speed=rs; o.jump_force=jf;
                        o.anim_idle=ai; o.anim_walk=aw; o.anim_run=ar; o.anim_jump=aj; o.anim_swim=asw;
                        if (n >= 10) { o.char_radius=cr; o.char_height=chh; }
                    }
                }
                const char* zl = strstr(line, " ZONELAYER ");
                if (zl) { int z; if (sscanf(zl, " ZONELAYER %d", &z) == 1) o.zone_layer = z; }
                const char* at = strstr(line, " ATTACH ");
                if (at) {
                    int pidx; float ox, oy, oz, asc, ay; char bone[256] = {0};
                    if (sscanf(at, " ATTACH %d %f %f %f %f %f %255[^\n]",
                               &pidx,&ox,&oy,&oz,&asc,&ay, bone) >= 6) {
                        o.attach_to = pidx; o.att_off[0]=ox; o.att_off[1]=oy; o.att_off[2]=oz;
                        o.att_scale = asc; o.att_yaw = ay; o.attach_bone = bone;
                    }
                }
                objects.push_back(o);
            }
        }
        fclose(f);
        rebuild_water();   // dibujar los lagos cargados (con el relieve ya puesto)
        status = "Escena cargada (" + std::to_string(objects.size()) + " objetos)";
    };

    // ---- gestion de PROYECTO (.bgd2): abrir / crear / guardar ----
    // Un proyecto es una carpeta con <nombre>.bgd2 + Assets/ Scenes/ Scripts/.
    // El .bgd2 guarda el nombre y la escena principal (relativa a la carpeta).
    auto apply_project = [&](const std::string& dir, const std::string& pname) {
        project_dir = dir; project_name = pname;
        assets_dir  = dir + "/Assets";
        scenes_dir  = dir + "/Scenes";
        scripts_dir = dir + "/Scripts";
        tex_dir     = assets_dir;
        std::error_code ec;
        fs::create_directories(assets_dir, ec);
        fs::create_directories(scenes_dir, ec);
        fs::create_directories(scripts_dir, ec);
        // re-escanea contenidos del proyecto
        assets = scan_assets(assets_dir);
        paints.clear();
        for (auto& fpng : scan_textures(tex_dir)) paints.push_back({ fpng, nullptr });
        paint_sel = paints.empty() ? -1 : 0;
        model_cache.clear(); posed_static.clear();
        // vacia la escena actual
        for (auto& o : objects) g3d_entity_impl_set_position(o.entity, 0, -99999, 0);
        objects.clear(); obj_sel = -1;
        scene_path = scenes_dir + "/level.scene";
    };
    // escribe el manifiesto <nombre>.bgd2 (nombre + escena principal)
    auto write_manifest = [&]() {
        std::string bgd2 = project_dir + "/" + project_name + ".bgd2";
        FILE* f = fopen(bgd2.c_str(), "w");
        if (f) {
            std::string rel = fs::path(scene_path).lexically_relative(project_dir).string();
            fprintf(f, "BGD2PROJECT 1\nname=%s\nscene=%s\n", project_name.c_str(), rel.c_str());
            fclose(f);
        }
    };
    // GUARDAR PROYECTO: escena actual + manifiesto .bgd2
    auto save_project = [&]() {
        save_scene(scene_path);                 // objetos + terreno + pintado
        write_manifest();
        status = "Proyecto guardado: " + project_name;
    };
    // crea un proyecto nuevo: carpetas + fichero .bgd2
    auto create_project = [&](const std::string& bgd2path) {
        fs::path p(bgd2path);
        std::string dir = p.parent_path().string();
        std::string pname = p.stem().string();
        apply_project(dir, pname);
        write_manifest();
        status = "Proyecto creado: " + pname;
    };
    auto open_project = [&](const std::string& bgd2path) {
        fs::path p(bgd2path);
        apply_project(p.parent_path().string(), p.stem().string());
        // lee la escena principal del manifiesto y la carga si existe
        std::string scn;
        FILE* f = fopen(bgd2path.c_str(), "r");
        if (f) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                char buf[512];
                if (sscanf(line, "scene=%511[^\n]", buf) == 1) scn = buf;
            }
            fclose(f);
        }
        if (!scn.empty()) {
            std::string sp = (fs::path(project_dir) / scn).string();
            if (fs::exists(sp)) { scene_path = sp; load_scene(sp); }
        }
        status = "Proyecto abierto: " + project_name;
    };

    // ---- GENERAR JUEGO: main.prg con los componentes + escena + spawns ----
    // Convierte la escena en un juego BennuGD2 jugable: incluye el script de cada
    // objeto (PROCESS) y en el main coloca cada objeto y lanza su proceso.
    // ---- main.prg: mitad tuya, mitad del editor ----
    // El editor SOLO toca lo que hay entre estas dos marcas (los #include de cada
    // objeto y el montaje del escenario). Todo lo de fuera es tuyo y no se pisa.
    // Asi no hay un __escena.prg aparte y los objetos no se duplican: cada uno vive
    // en su Scripts/<nombre>.prg y se incluye una sola vez.
    const char* MK_BEGIN = "// >>> EDITOR: no toques este bloque, lo regenera el editor >>>";
    const char* MK_END   = "// <<< EDITOR: fin del bloque >>>";
    bool ask_regen_main = false;
    // Arma un main.prg completo alrededor del bloque del editor.
    auto main_skeleton = [&](const std::string& body) -> std::string {
        return
            std::string("// ===== main del juego =====\n"
            "// Lo de fuera del bloque del editor es TUYO; el editor no lo toca.\n"
            "// El bloque marcado lleva los objetos y el escenario, y se rehace al\n"
            "// generar el juego. Escribe tu logica en PROCESS main, mas abajo.\n\n"
            "import \"libmod_gfx\"; import \"libmod_misc\"; import \"libmod_input\"; import \"libmod_3d\";\n\n")
            + MK_BEGIN + "\n"
            + body
            + MK_END + "\n\n"
            "PROCESS main()\n"
            "BEGIN\n"
            "    escena_iniciar();   // terreno, agua, objetos y camara\n"
            "    escena_motor();     // fisica + camara, un paso por frame\n"
            "\n"
            "    LOOP\n"
            "        IF (key(_ESC)) exit(); END\n"
            "        // ---- tu codigo aqui ----\n"
            "        FRAME;\n"
            "    END\n"
            "END\n";
    };
    auto main_template = [&]() -> std::string {
        return main_skeleton("    // (los objetos y el escenario se rellenan al Generar el juego)\n");
    };
    // Mete `body` en el bloque marcado. Si main.prg ya tiene el bloque, cambia solo
    // su interior y respeta el resto. Si no lo tiene (main nuevo o del formato
    // viejo), guarda copia del viejo y crea uno con el esqueleto.
    auto update_main_block = [&](const std::string& body) -> bool {
        std::string mp = project_dir + "/main.prg";
        std::string cur;
        { FILE* mf = fopen(mp.c_str(), "r");
          if (mf) { char b[4096]; size_t n; while ((n = fread(b,1,sizeof(b),mf)) > 0) cur.append(b,n); fclose(mf); } }
        std::string out;
        size_t a = cur.find(MK_BEGIN), z = cur.find(MK_END);
        if (!cur.empty() && a != std::string::npos && z != std::string::npos && z > a) {
            out = cur.substr(0, a) + MK_BEGIN + "\n" + body + cur.substr(z);
        } else {
            if (!cur.empty()) {
                FILE* bf = fopen((mp + ".anterior").c_str(), "w");
                if (bf) { fwrite(cur.data(),1,cur.size(),bf); fclose(bf); }
                console_add("main.prg no tenia el bloque del editor (formato antiguo).\n"
                            "Copia guardada en main.prg.anterior; puesto uno nuevo con tu\n"
                            "codigo en PROCESS main.\n");
            }
            out = main_skeleton(body);
        }
        FILE* mf = fopen(mp.c_str(), "w");
        if (!mf) return false;
        fwrite(out.data(), 1, out.size(), mf); fclose(mf);
        return true;
    };
    auto write_main_prg = [&]() -> bool {
        FILE* mf = fopen((project_dir + "/main.prg").c_str(), "w");
        if (!mf) return false;
        std::string t = main_template();
        fwrite(t.data(), 1, t.size(), mf);
        fclose(mf);
        return true;
    };
    auto open_main_script = [&]() {
        std::string mp = project_dir + "/main.prg";
        FILE* mf = fopen(mp.c_str(), "r");
        if (!mf) {                       // aun no se ha generado nunca: se crea ahora
            if (!write_main_prg()) { status = "ERROR: no puedo crear main.prg"; return; }
            mf = fopen(mp.c_str(), "r");
            if (!mf) return;
        }
        std::string t; char b[1024]; size_t n;
        while ((n = fread(b, 1, sizeof(b), mf)) > 0) t.append(b, n);
        fclose(mf);
        script.SetText(t);
        script_file = mp; script_title = "main.prg  (tuyo: el editor no lo toca)";
        script_obj.clear();
        show_script = true; focus_script = true;
    };

    auto generate_game = [&](bool run) {
        save_scene(scene_path);   // asegura .terrain/.paint.png frescos antes de generar
        // rutas (relativas al proyecto) del relieve y el pintado del terreno
        std::string rel_scene  = fs::path(scene_path).lexically_relative(project_dir).string();
        std::string rel_relief = rel_scene + ".terrain";
        std::string rel_paint  = rel_scene + ".paint.png";
        std::string rel_zones  = rel_scene + ".zones";
        // El montaje (GLOBALs, includes de objetos, escena_iniciar, escena_motor)
        // se genera aqui y se inserta en el bloque marcado de main.prg. Los import
        // van fuera del bloque, arriba del todo del main, asi que aqui no se repiten.
        // El montaje se arma en memoria y luego se inserta en el bloque marcado de
        // main.prg (con un #include por objeto). No hay __escena.prg aparte.
        char* setup_buf = NULL; size_t setup_sz = 0;
        FILE* f = open_memstream(&setup_buf, &setup_sz);
        if (!f) { status = "ERROR preparando el escenario"; return; }
        fputs("GLOBAL int scene; int camera; int light;\n"
      "    float escena_pitch;   // hacia donde mira en vertical (FPS)\n"
      "    float escena_yaw;     // hacia donde mira en horizontal (FPS)\n"
      "END\n\n", f);
        // ---- localizar el jugador y los objetos enganchados a su esqueleto ----
        int player_idx = -1;
        for (size_t i = 0; i < objects.size(); i++)
            if (objects[i].is_player) { player_idx = (int)i; break; }
        // enganches validos = objetos con attach_to == el jugador (arma en la mano)
        std::vector<int> attach_list;
        for (size_t i = 0; i < objects.size(); i++)
            if (objects[i].attach_to == player_idx && player_idx >= 0 && (int)i != player_idx)
                attach_list.push_back((int)i);

        // El comportamiento vive en el script del objeto, asi que los que lo
        // necesiten (jugador o cuerpo fisico) deben TENER script antes de
        // concatenar los componentes: si no, el main llamaria a un PROCESS que no
        // esta en el fichero.
        // Se crea si falta. Si ya existe pero sigue siendo el que genero el editor
        // (nadie lo ha tocado), se rehace con los valores actuales del Inspector:
        // sin esto, cambiar la masa o la densidad de un objeto no se notaba en el
        // juego, porque su fisica vive en el script y el script no se rehacia nunca.
        // En cuanto lo editas a mano, la marca deja de cuadrar y ya no se toca.
        for (auto& o : objects) {
            bool necesita = o.is_player || (o.phys >= 0 && o.phys <= 4);   // 5 = muro (sin script)
            if (!necesita) continue;
            std::string psp = scripts_dir + "/" + o.name + ".prg";
            FILE* t = fopen(psp.c_str(), "r");
            bool existe = (t != nullptr);
            if (t) fclose(t);
            if (existe && !script_untouched(o.name)) continue;   // es tuyo
            if (script_write_generated(o))
                console_add((existe ? "Actualizado Scripts/" : "Creado Scripts/") + o.name + ".prg (" +
                            (o.is_player ? "controles del jugador"
                             : (o.phys == 0) ? "objeto decorativo" : "cuerpo fisico") + ")\n");
        }

        // Un #include por objeto (SIN duplicar: el codigo vive en Scripts/<n>.prg).
        // Van antes que escena_iniciar, que es quien instancia esos procesos.
        for (auto& o : objects) {
            std::string sp = scripts_dir + "/" + o.name + ".prg";
            FILE* s = fopen(sp.c_str(), "r");
            if (!s) continue;
            fclose(s);
            fprintf(f, "#include \"Scripts/%s.prg\"\n", o.name.c_str());
        }
        fputs("\n", f);
        // main
        // Lo que comparten el montaje y el bucle tiene que ser GLOBAL: antes era
        // todo PRIVATE de un unico PROCESS main, y al partirlo en dos deja de valer.
        fputs("GLOBAL\n"
              "    int follow_ent;          // entidad a la que sigue la camara\n"
              "    int pplayer; int pmodel; // entidad y modelo del jugador\n"
              "    int atc[32]; int atn[32];// enganches a huesos: entidad y nodo\n"
              "    float escena_dt;\n"
              "END\n\n", f);
        // ---- la luz del sol como proceso BennuGD2 (variables nativas) ----
        // Igual que cualquier objeto 3D: fija ctype/csubtype y sus datos en las
        // variables nativas, y el motor las envia a la luz cada FRAME. La direccion
        // sale de target - origen. Para cambiar la luz en marcha basta con tocar
        // intensity o color_r/g/b desde aqui.
        fprintf(f,
              "PROCESS escena_sol()\n"
              "BEGIN\n"
              "    ctype = C_3D; csubtype = C3D_LIGHT;\n"
              "    x = 0.0; y = 0.0; z = 0.0;\n"
              "    target_x = -0.45; target_y = -0.75; target_z = -0.35;   // direccion = target - origen\n"
              "    intensity = 1.5;\n"
              "    color_r = 255; color_g = 245; color_b = 219;   // 1.0, 0.96, 0.86\n"
              "    entity = g3d_light_create(0, 1.0, 1.0, 1.0);   // el color lo pone el hook\n"
              "    g3d_light_enable_shadow(entity, 1); g3d_set_shadows(1);\n"
              "    g3d_set_shadow_resolution(%d);   // nitidez de las sombras\n"
              "    LOOP\n"
              "        FRAME;\n"
              "    END\n"
              "END\n\n", shadow_res);

        // ---- la camara como proceso BennuGD2 (variables nativas) ----
        // La entidad la crea escena_iniciar y la activa (para que no haya un frame
        // sin camara); aqui se ata a 'entity' y se conduce con x/y/z (posicion) y
        // target_x/y/z (a donde mira). El hook las envia a la camara cada FRAME.
        bool follow = (cam_mode != 0 && cam_follow >= 0 && cam_follow < (int)objects.size());
        {
            std::string cp =
                "PROCESS escena_camara(int cam)\n"
                "PRIVATE float tx; float ty; float tz;\nEND\n"
                "BEGIN\n"
                "    ctype = C_3D; csubtype = C3D_CAMERA;\n"
                "    entity = cam;\n";
            char b[2048];
            if (!follow) {
                snprintf(b, sizeof(b),
                    "    x = %.3f; y = %.3f; z = %.3f;\n"
                    "    target_x = %.3f; target_y = %.3f; target_z = %.3f;\n",
                    cam_pos[0], cam_pos[1], cam_pos[2],
                    cam_look[0], cam_look[1], cam_look[2]);
                cp += b;
            }
            cp += "    LOOP\n";
            if (follow) {
                cp += "        g3d_entity_get_position(follow_ent, &tx, &ty, &tz);\n";
                if (cam_mode == 1) {          // tercera persona
                    snprintf(b, sizeof(b),
                        "        x = tx; y = ty + %.3f; z = tz - %.3f;\n"
                        "        target_x = tx; target_y = ty + 1.0; target_z = tz;\n",
                        cam_height, gcam_dist);
                } else if (cam_mode == 2) {   // primera persona (FPS)
                    snprintf(b, sizeof(b),
                        "        // escena_yaw / escena_pitch los escribe el jugador con el raton\n"
                        "        x = tx + sin(escena_yaw) * %.3f;\n"
                        "        y = ty + %.3f;\n"
                        "        z = tz + cos(escena_yaw) * %.3f;\n"
                        "        target_x = tx + sin(escena_yaw) * %.3f * cos(escena_pitch);\n"
                        "        target_y = ty + %.3f + sin(escena_pitch) * 10.0;\n"
                        "        target_z = tz + cos(escena_yaw) * %.3f * cos(escena_pitch);\n",
                        cam_fwd, cam_height, cam_fwd,
                        cam_fwd + 10.0f, cam_height, cam_fwd + 10.0f);
                } else {                      // cenital (top-down)
                    snprintf(b, sizeof(b),
                        "        x = tx; y = ty + %.3f; z = tz + 0.5;\n"
                        "        target_x = tx; target_y = ty; target_z = tz;\n",
                        gcam_dist);
                }
                cp += b;
            }
            cp += "        FRAME;\n    END\nEND\n\n";
            fputs(cp.c_str(), f);
        }

        fputs("// Monta el escenario: terreno, agua, objetos, sus procesos y la camara.\n"
              "FUNCTION escena_iniciar()\n", f);
        fputs("PRIVATE int e; int m; int tex; int mat;\nEND\nBEGIN\n", f);
        fputs("    set_mode(1280,720); set_fps(60,0); window_set_title(\"EDITOR_PLAY\");\n", f);
        fputs("    scene = g3d_scene_create(\"juego\"); g3d_scene_set_active(scene);\n", f);
        fputs("    camera = g3d_camera_create(); g3d_camera_set_active(camera);\n", f);
        fputs("    escena_sol();   // la luz del sol (proceso con vars nativas)\n", f);
        fputs("    g3d_sky_set_gradient(0.35,0.55,0.85, 0.82,0.88,0.96); g3d_sky_enable(1);\n", f);
        // terreno: mismo grid/worldsize que el editor (160 / 400), plano y luego
        // se le aplica el relieve esculpido y el pintado guardados en la escena.
        fputs("    m = g3d_primitive_terrain(160, 400.0, 0.0, 1.0, 1); e = g3d_entity_spawn(scene,0,0,0,0); g3d_entity_set_mesh(e, m);\n", f);
        fprintf(f, "    g3d_terrain_load(m, \"%s\");\n", rel_relief.c_str());
        fputs("    g3d_set_terrain_collider(m);   // fisica: los cuerpos se apoyan en el relieve\n", f);
        fputs("    g3d_collider_clear();   // limpia muros invisibles previos\n", f);
        fprintf(f, "    g3d_zone_load(\"%s\");   // zonas de barrera pintadas\n", rel_zones.c_str());
        fprintf(f, "    tex = g3d_load_texture(\"%s\");\n", rel_paint.c_str());
        fputs("    if (tex > 0)\n", f);
        fputs("        mat = g3d_material_create(); g3d_material_set_texture(mat, 0, tex);\n", f);
        fputs("        g3d_material_set_roughness(mat, 0.95); g3d_entity_set_material(e, mat);\n", f);
        fputs("    end\n", f);
        if (water_on) {
            // agua: mismo oleaje/color/textura que el editor (g3d_editor_water_update)
            fprintf(f, "    g3d_water_create(%.3f, 4000.0, 200); g3d_water_set_ssr(1, 0.6);\n", water_level);
            fprintf(f, "    g3d_water_set_waves(%.4f, %.4f, %.4f);\n", w_amp, w_len, w_speed);
            fprintf(f, "    g3d_water_set_ocean(1.0, 0.3, %.4f);\n", w_swell);
            fprintf(f, "    g3d_water_set_color(%.4f, %.4f, %.4f, %.4f, %.4f, %.4f);\n",
                    w_deep[0], w_deep[1], w_deep[2], w_shallow[0], w_shallow[1], w_shallow[2]);
            if (water_tex_sel >= 0 && water_tex_sel < (int)paints.size())
                fprintf(f, "    g3d_water_set_texture(g3d_load_texture(\"Assets/%s\"));\n",
                        paints[water_tex_sel].file.c_str());
            fputs("    g3d_water_set_enabled(1);\n", f);
        }
        // ---- lagos y rios: agua colocada (flood-fill / camino), no un mar global ----
        // Cada masa de agua fija SU estilo (olas/color/textura) justo antes de
        // crearse, para que el motor lo capture como propio de esa zona.
        if (!lakes.empty() || !rivers.empty() || !waterfalls.empty()) {
            auto emit_fx = [&](const WaterFX& x) {
                fprintf(f, "    g3d_fluid_style(%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, 0.88);\n",
                        x.amp, x.len, x.speed,
                        x.deep[0], x.deep[1], x.deep[2], x.shallow[0], x.shallow[1], x.shallow[2]);
                if (x.tex >= 0 && x.tex < (int)paints.size())
                    fprintf(f, "    g3d_fluid_set_texture(g3d_load_texture(\"Assets/%s\"));\n",
                            paints[x.tex].file.c_str());
                else
                    fputs("    g3d_fluid_set_texture(0);\n", f);
            };
            // Secuencia el estado del motor igual que el preview para poder recortar
            // cada rio contra los lagos + rios YA anadidos (no contra si mismo). Al
            // final se restaura con rebuild_water().
            g3d_fluid_clear(); g3d_flow_clear(); g3d_water_clear_ripple_sources();
            // Bloquea los cauces ANTES de crear los lagos (que no suban por el rio),
            // tanto en el motor (preview del recorte) como en el juego generado.
            g3d_fluid_block_reset();
            fputs("    g3d_fluid_block_reset();\n", f);
            for (auto& rv : rivers) {
                int n = (int)rv.pts.size() / 2; if (n < 2) continue;
                std::vector<float> bx(n * 3);
                for (int k = 0; k < n; k++) { bx[k*3]=rv.pts[k*2]; bx[k*3+1]=0.0f; bx[k*3+2]=rv.pts[k*2+1]; }
                g3d_fluid_block_river(bx.data(), n, rv.width);   // motor
                fprintf(f, "    g3d_river_begin(%.3f, %.3f);\n", rv.width, rv.depth*0.8f);
                for (int k = 0; k < n; k++)
                    fprintf(f, "    g3d_river_point(%.3f, %.3f);\n", rv.pts[k*2], rv.pts[k*2+1]);
                fputs("    g3d_river_block();   // marca el cauce (no lo dibuja aun)\n", f);
            }
            for (auto& lk : lakes) {
                emit_fx(lk.fx);
                fprintf(f, "    g3d_lake_add_r(%.3f, %.3f, %.3f, %.3f, %.3f);   // lago (radio max acota hoyos abiertos)\n",
                        lk.sx, lk.sz, lk.level, lk.depth, lk.radius);
                apply_fx(lk.fx); g3d_lake_add_r(lk.sx, lk.sz, lk.level, lk.depth, lk.radius);   // motor
            }
            for (auto& rv : rivers) {
                std::vector<float> xyz;
                std::vector<std::pair<float,float>> jn;
                river_trimmed(rv, xyz, jn);   // recorta el tramo cubierto por un lago
                int m = (int)xyz.size() / 3;
                if (m >= 2) {
                    emit_fx(rv.fx);
                    fprintf(f, "    g3d_river_begin(%.3f, %.3f);\n", rv.width, rv.depth*0.8f);
                    for (int k = 0; k < m; k++)
                        fprintf(f, "    g3d_river_point(%.3f, %.3f);\n", xyz[k*3], xyz[k*3+2]);
                    fputs("    g3d_river_end();   // superficie del rio (recortada)\n", f);
                    for (auto& j : jn)
                        fprintf(f, "    g3d_water_add_ripple_source(%.3f, %.3f, 0.9);   // honda en la union con el lago\n",
                                j.first, j.second);
                    apply_fx(rv.fx); g3d_river_add(xyz.data(), m, rv.width);   // motor: superficie
                }
                // CASCADAS del rio: camino COMPLETO -> lamina donde el cauce cae fuerte.
                int nf = (int)rv.pts.size() / 2;
                if (nf >= 2) {
                    fprintf(f, "    g3d_flow_set_color(%.4f, %.4f, %.4f);\n",
                            rv.wf.color[0], rv.wf.color[1], rv.wf.color[2]);
                    fprintf(f, "    g3d_flow_set_foam(%.3f); g3d_flow_set_speed(%.3f);\n", rv.wf.foam, rv.wf.speed);
                    fputs("    g3d_flow_set_texture(0);\n", f);
                    fprintf(f, "    g3d_river_begin(%.3f, %.3f);\n", rv.width, rv.depth*0.8f);
                    for (int k = 0; k < nf; k++)
                        fprintf(f, "    g3d_river_point(%.3f, %.3f);\n", rv.pts[k*2], rv.pts[k*2+1]);
                    fputs("    g3d_river_falls();   // cascadas del rio (camino completo)\n", f);
                    std::vector<float> full(nf * 3);
                    for (int k = 0; k < nf; k++) { full[k*3]=rv.pts[k*2]; full[k*3+1]=0.0f; full[k*3+2]=rv.pts[k*2+1]; }
                    apply_wf(rv.wf); g3d_river_add_falls(full.data(), nf, rv.width);
                }
            }
            // CASCADAS (colocadas a mano): elemento propio, con su estilo de flujo.
            for (auto& w : waterfalls) {
                fprintf(f, "    g3d_flow_set_color(%.4f, %.4f, %.4f);\n",
                        w.fx.color[0], w.fx.color[1], w.fx.color[2]);
                fprintf(f, "    g3d_flow_set_foam(%.3f); g3d_flow_set_speed(%.3f);\n", w.fx.foam, w.fx.speed);
                if (w.fx.tex >= 0 && w.fx.tex < (int)paints.size())
                    fprintf(f, "    g3d_flow_set_texture(g3d_load_texture(\"Assets/%s\"));\n",
                            paints[w.fx.tex].file.c_str());
                else
                    fputs("    g3d_flow_set_texture(0);\n", f);
                fprintf(f, "    g3d_waterfall_add(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n",
                        w.top[0], w.top[1], w.top[2], w.base[0], w.base[2], w.width, w.arc);
                apply_wf(w.fx); g3d_waterfall_add(w.top[0], w.top[1], w.top[2], w.base[0], w.base[2], w.width, w.arc);
            }
            rebuild_water();   // restaura el preview (deshace el secuenciado de arriba)
        }
        // objetos + sus componentes (+ cuerpos fisicos Jolt)
        fputs("    escena_dt = 1.0 / 60.0;\n", f);
        int pj = 0;   // indice de cuerpo fisico (literal)
        for (size_t i = 0; i < objects.size(); i++) {
            auto& o = objects[i];
            const char* loader = "g3d_load_gltf";
            { std::string a=o.asset; for(auto&c:a)c=(char)tolower(c);
              if (a.size()>4 && a.substr(a.size()-4)==".fbx") loader="g3d_load_fbx"; }

            // ---- muro invisible: solo un colisionador estatico, sin entidad ----
            // Es invisible en el juego, asi que no hay nada que dibujar: no se
            // spawnea modelo ni proceso, solo se registra la caja de colision.
            if (o.phys == 5 && !o.is_player) {
                float hx = o.csize > 0.05f ? o.csize : 0.5f;
                fprintf(f, "    g3d_collider_add_box(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);   // muro '%s'\n",
                        o.x - hx, o.y - 5.0f, o.z - hx, o.x + hx, o.y + 30.0f, o.z + hx,
                        o.name.c_str());
                continue;
            }

            // Objeto enganchado a un hueso: se lanza DESPUES del bucle, cuando ya se
            // conoce el nodo del hueso (necesita pmodel). Se aplaza aqui.
            bool es_hijo_enganche = false;
            for (int k : attach_list) if (k == (int)i) es_hijo_enganche = true;
            if (es_hijo_enganche) continue;

            // ¿Este objeto usa ya el idioma nativo (el proceso se crea su propia
            // entidad)? Los objetos con fisica y los decorativos. El jugador y la
            // camara-objetivo todavia necesitan que la entidad se cree aqui fuera.
            bool es_padre_enganche = false;
            for (auto& oo : objects) if (oo.attach_to == (int)i) es_padre_enganche = true;
            bool nativo = (o.phys >= 0 && o.phys <= 4) && !o.is_player &&
                          (int)i != cam_follow && !es_padre_enganche;
            if (nativo) {
                // El proceso recibe el modelo y hace el spawn dentro (con su posado
                // si hace falta). Aqui solo se carga el modelo y se lanza.
                std::string sp = scripts_dir + "/" + o.name + ".prg";
                FILE* s = fopen(sp.c_str(), "r");
                if (s) { fclose(s);
                    fprintf(f, "    m = %s(\"Assets/%s\"); %s(m);\n",
                            loader, o.asset.c_str(), o.name.c_str());
                }
                continue;
            }

            fprintf(f, "    m = %s(\"Assets/%s\"); e = g3d_model_spawn(scene, m, %.3f, %.3f, %.3f, 0.0, %.3f);",
                    loader, o.asset.c_str(), o.x, o.y, o.z, o.ry * 57.29578f);
            fprintf(f, " g3d_entity_set_scale(e, %.3f, %.3f, %.3f);\n", o.scale, o.scale, o.scale);
            // Modelos sin esqueleto con piezas atadas a nodos animados: hay que
            // posarlos UNA vez o esas piezas no se colocan y no se ven (igual que
            // en el editor). No se animan por frame para que no se descoloquen.
            { void* mm = load_model(o.asset);
              if (mm && g3d_model_animation_count(mm) > 0 && !g3d_model_is_skinned(mm) &&
                  (int)i != player_idx)
                  fputs("    g3d_model_animate_all(m, 0.0, 0);\n", f); }
            if ((int)i == cam_follow) fputs("    follow_ent = e;   // camara sigue a este objeto\n", f);
            if ((int)i == player_idx) fputs("    pplayer = e; pmodel = m;   // el jugador\n", f);
            // (los objetos enganchados a un hueso se lanzan mas abajo, como proceso)
            // (los muros invisibles se manejaron arriba: solo colisionador, sin entidad)
            // Los cuerpos rigidos los crea cada objeto en SU script.
            std::string sp = scripts_dir + "/" + o.name + ".prg";
            // El jugador lleva sus controles en SU script: si aun no existe, se crea
            // con la plantilla para que el juego salga jugable a la primera.
            if ((int)i == player_idx) {   // su script ya se aseguro mas arriba
                fprintf(f, "    %s(e, m);   // jugador: entidad + modelo (para animar)\n", o.name.c_str());
            } else {
                // Aqui solo llega un objeto fisico/decorativo si es objetivo de
                // camara o enganche (su plantilla se auto-crea la entidad, asi que
                // NO se le pasa 'e': se lanza nativo). La camara-a-otro-objeto y el
                // enganche a hueso se rehacen en su turno del plan; de momento no se
                // engancha follow_ent a estos (limitacion conocida, sin crash).
                FILE* s = fopen(sp.c_str(), "r");
                if (s) { fclose(s); fprintf(f, "    %s(m);\n", o.name.c_str()); }
            }
        }
        // ---- objetos enganchados a un hueso: se lanza su proceso ahora, que ya se
        //      conoce pplayer/pmodel. Cada uno busca su nodo y se lanza con el ----
        if (player_idx >= 0) {
            for (size_t k = 0; k < attach_list.size(); k++) {
                auto& a = objects[attach_list[k]];
                const char* loader = "g3d_load_gltf";
                { std::string s=a.asset; for(auto&c:s)c=(char)tolower(c);
                  if (s.size()>4 && s.substr(s.size()-4)==".fbx") loader="g3d_load_fbx"; }
                std::string sp = scripts_dir + "/" + a.name + ".prg";
                FILE* s = fopen(sp.c_str(), "r");
                if (!s) continue;
                fclose(s);
                fprintf(f, "    atn[%d] = g3d_model_node_find(pmodel, \"%s\");\n",
                        (int)k, a.attach_bone.c_str());
                fprintf(f, "    m = %s(\"Assets/%s\"); %s(m, pplayer, pmodel, atn[%d]);\n",
                        loader, a.asset.c_str(), a.name.c_str(), (int)k);
            }
        }
        // ---- CAMARA: se lanza su proceso (definido arriba). follow_ent ya esta
        //      puesto (el jugador se spawnea antes), asi que la camara ve al
        //      objetivo desde su primer frame. ----
        fputs("    escena_camara(camera);\n", f);
        // Aqui acaba el montaje. El bucle por frame va en su propio proceso para
        // que main.prg pueda ser tuyo: arrancas el escenario, lanzas el motor y
        // encima escribes lo que quieras.
        fputs("    RETURN;\nEND\n\n", f);
        fputs("// Avanza el mundo fisico una vez por frame. Cada objeto (barril, jugador,\n"
              "// arma, camara) es su propio proceso y se coloca solo; aqui solo la fisica.\n"
              "PROCESS escena_motor()\n", f);
        fputs("BEGIN\n", f);
        fputs("    LOOP\n", f);
        fputs("        g3d_rigidbody_step(escena_dt);\n", f);
        // El enganche de armas ya no va aqui: cada arma es su propio proceso que
        // sigue el hueso con sus variables nativas.
        fputs("        FRAME;\n    END\nEND\n", f);
        fclose(f);
        std::string setup(setup_buf ? setup_buf : "", setup_sz);
        free(setup_buf);

        // Volcar el montaje al bloque marcado de main.prg, sin tocar tu codigo.
        // Y borrar el __escena.prg de versiones anteriores, que ya no se usa.
        remove((project_dir + "/__escena.prg").c_str());
        if (!update_main_block(setup)) { status = "ERROR escribiendo main.prg"; return; }
        // bgdc/bgdi localizan los modulos (.so) via PATH/LD_LIBRARY_PATH; el popen
        // no hereda el PATH del perfil -> los fijamos al directorio de los modulos.
        std::string bgdc = ruta_util("lib/bgdc", BGDC_PATH);
        std::string bindir = bgdc.substr(0, bgdc.rfind('/'));
        std::string env = "PATH=\"" + bindir + ":$PATH\" LD_LIBRARY_PATH=\"" + bindir + ":$LD_LIBRARY_PATH\" ";
        std::string proj = project_dir;
        std::string cmd = "cd \"" + proj + "\" && " + env + "\"" + bgdc + "\" main.prg 2>&1";
        console_add("\n----- compilando main.prg -----\n");
        console_focus = true;
        std::string out; FILE* p = popen(cmd.c_str(), "r");
        int rc = -1;
        if (p) { char b[512]; size_t n; while ((n = fread(b,1,sizeof(b)-1,p))>0){ b[n]=0; out += b; } rc = pclose(p); }
        console_add(out);
        last_compile_ok = (rc == 0);
        console_add((rc == 0) ? "[OK] Juego compilado -> main.dcb\n"
                              : "[FALLO] revisa los errores de arriba.\n");
        if (rc == 0 && run) {
            // La salida del juego (say, errores en ejecucion) va a un log que la
            // consola va leyendo por frames, en vez de perderse en /dev/null.
            if (run_log) { fclose(run_log); run_log = nullptr; }
            run_log_path = proj + "/__run.log";
            std::string bgdi = bindir + "/bgdi";
            std::string rcmd = "cd \"" + proj + "\" && " + env + "\"" + bgdi +
                               "\" main.dcb > \"" + run_log_path + "\" 2>&1 &";
            if (system(rcmd.c_str()) == -1) console_add("[ERROR] no pude lanzar bgdi\n");
            console_add("----- ejecutando (salida del juego) -----\n");
            run_log = fopen(run_log_path.c_str(), "r");   // puede no existir aun; se reintenta
        }
    };

    // ---- exploradores de archivos (cargar / guardar como) ----
    ImGui::FileBrowser openDlg;
    openDlg.SetTitle("Cargar escena");
    openDlg.SetTypeFilters({ ".scene" });
    openDlg.SetPwd(scenes_dir);
    ImGui::FileBrowser saveDlg(ImGuiFileBrowserFlags_EnterNewFilename |
                               ImGuiFileBrowserFlags_CreateNewDir);
    saveDlg.SetTitle("Guardar escena como");
    saveDlg.SetTypeFilters({ ".scene" });
    saveDlg.SetPwd(scenes_dir);

    // exploradores de PROYECTO (.bgd2)
    ImGui::FileBrowser projOpenDlg;
    projOpenDlg.SetTitle("Abrir proyecto (.bgd2)");
    projOpenDlg.SetTypeFilters({ ".bgd2" });
    ImGui::FileBrowser projNewDlg(ImGuiFileBrowserFlags_EnterNewFilename |
                                  ImGuiFileBrowserFlags_CreateNewDir);
    projNewDlg.SetTitle("Nuevo proyecto (elige carpeta y nombre .bgd2)");
    projNewDlg.SetTypeFilters({ ".bgd2" });

    ViewportFBO vp;
    ViewportFBO prevFbo;      // FBO del visor de animaciones
    float cam_yaw = 3.6f, cam_pitch = 0.35f, cam_dist = 14.0f;
    float vcam_target[3] = { 0.0f, 2.0f, 0.0f };   // pivote de la camara del viewport (se mueve con WASD)
    int vp_w = 1280, vp_h = 720;      // tamano de la ventana de viewport (px)
    bool vp_hovered = false;

    // Punto de colocacion bajo el raton (sx,sy en pixeles del viewport):
    // sobre agua -> superficie del agua (para que floten); Alt o sin place_on_water
    // -> terreno/fondo del lago. Devuelve false si no acierta nada.
    auto place_point = [&](float sx, float sy, float* hit) -> bool {
        bool want_water = water_on && place_on_water && !ImGui::GetIO().KeyAlt;
        if (want_water &&
            g3d_editor_ray_plane(sx, sy, (float)vp.w, (float)vp.h, water_level, hit)) {
            float th = terrain ? g3d_editor_terrain_height(terrain, hit[0], hit[2]) : 1e9f;
            if (th < water_level) return true;   // hay agua ahi -> colocar en la superficie
        }
        if (terrain && g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit))
            return true;                          // sobre el terreno (o fondo del lago)
        return g3d_editor_ray_plane(sx, sy, (float)vp.w, (float)vp.h, 0.0f, hit) != 0;
    };

    // ================= PLAY EN VIVO (emulador integrado, sin BennuGD2) =================
    // Reproduce dentro del editor la MISMA logica que genera el juego (jugador, fisica,
    // flotacion, zonas, camaras, enganche a huesos). No ejecuta los scripts propios.
    // Play/Stop se PIDEN desde la UI y se ejecutan al principio del frame siguiente,
    // FUERA del frame de ImGui: play_start hace popen(fork) y carga modulos que
    // reinicializan SDL; hacerlo en mitad del frame se lleva por delante el contexto GL.
    int  play_req = 0;   // 0=nada 1=arrancar 2=parar
    struct SimBody { int ent, bid, buoy, wet; float half, mass, bk, prevx, prevy, prevz, ript, dens; };
    std::vector<SimBody> sim_bodies;
    struct SimAttach { int ent, node; float ox, oy, oz, sc, yaw; };
    std::vector<SimAttach> sim_attach;
    int sim_pch = -1, sim_player_ent = -1, sim_player_idx = -1;
    void* sim_player_model = nullptr;
    float sim_facing = 0, sim_t = 0, sim_pprevx = 0, sim_pprevz = 0;
    bool  sim_wet = false; float sim_ript = 0;   // ondas del jugador en el agua
    std::vector<std::array<float,5>> play_snap;   // x,y,z,ry,scale de cada objeto
    float play_cam_snap[6] = {0,0,0,0,0,0};

    auto play_start = [&]() {
        play_snap.clear();
        for (auto& o : objects) play_snap.push_back({ o.x, o.y, o.z, o.ry, o.scale });
        play_cam_snap[0]=vcam_target[0]; play_cam_snap[1]=vcam_target[1]; play_cam_snap[2]=vcam_target[2];
        play_cam_snap[3]=cam_yaw; play_cam_snap[4]=cam_pitch; play_cam_snap[5]=cam_dist;
        g3d_char_clear_all(); g3d_rigidbody_clear(); g3d_collider_clear();
        sim_bodies.clear(); sim_attach.clear();
        sim_pch=-1; sim_player_ent=-1; sim_player_idx=-1; sim_player_model=nullptr; sim_facing=0; sim_t=0;
        if (terrain) g3d_scene_set_terrain_collider(terrain);
        for (int i=0;i<(int)objects.size();i++) if (objects[i].is_player){ sim_player_idx=i; break; }
        for (int i=0;i<(int)objects.size();i++){
            auto& o = objects[i];
            if (o.phys==5){ float hx=o.csize>0.05f?o.csize:0.5f;
                g3d_collider_add_box(o.x-hx,o.y-5.0f,o.z-hx, o.x+hx,o.y+30.0f,o.z+hx);
                g3d_entity_impl_set_scale(o.entity,0.0001f,0.0001f,0.0001f); continue; }
            if (i==sim_player_idx){
                sim_player_ent=o.entity; sim_player_model=load_model(o.asset);
                sim_pch=g3d_char_create(o.x,o.y,o.z,o.char_radius,o.char_height);
                g3d_char_set_tuning(sim_pch,0.8f,46.0f); continue; }
            if (o.phys>=1 && o.phys<=4){
                float c=o.csize>0.05f?o.csize:0.5f; float by0=o.y+c; int bid;
                if(o.phys==1) bid=g3d_rigidbody_create(o.x,by0,o.z,c,c,c,o.mass);
                else if(o.phys==2) bid=g3d_rigidbody_create_sphere(o.x,by0,o.z,c,o.mass);
                else if(o.phys==3) bid=g3d_rigidbody_create_capsule(o.x,by0,o.z,c,c,o.mass);
                else bid=g3d_rigidbody_create_cylinder(o.x,by0,o.z,c,c,o.mass);
                g3d_rigidbody_set_bounce(bid,o.bounce,o.friction);
                // flota si esta marcado (da igual que el agua sea mar, lago o rio;
                // el nivel se consulta por frame con g3d_water_level_at).
                int buoy=(o.buoyant&&o.mass>0.0f)?1:0; float bk=0.0f;
                float dens=o.density>0.05f?o.density:0.05f;
                sim_bodies.push_back({ o.entity, bid, buoy, 0, c, o.mass, bk, o.x, by0, o.z, 0.0f, dens });
            }
        }
        if (sim_player_idx>=0 && sim_player_model)
            for (int i=0;i<(int)objects.size();i++){
                auto& a=objects[i];
                if (a.attach_to==sim_player_idx && i!=sim_player_idx){
                    int node=g3d_model_node_find(sim_player_model, a.attach_bone.c_str());
                    sim_attach.push_back({ a.entity, node, a.att_off[0],a.att_off[1],a.att_off[2], a.att_scale, a.att_yaw });
                }
            }

        playing=true;
    };
    auto play_stop = [&]() {
        g3d_char_clear_all(); g3d_rigidbody_clear(); g3d_collider_clear();
        sim_bodies.clear(); sim_attach.clear(); sim_pch=-1;
        for (size_t i=0;i<objects.size() && i<play_snap.size();i++){
            auto& o=objects[i]; auto& s=play_snap[i];
            o.x=s[0]; o.y=s[1]; o.z=s[2]; o.ry=s[3]; o.scale=s[4];
            g3d_entity_impl_set_position(o.entity,o.x,o.y,o.z);
            g3d_entity_impl_set_rotation(o.entity,0,o.ry,0);
            g3d_entity_impl_set_scale(o.entity,o.scale,o.scale,o.scale);
        }
        vcam_target[0]=play_cam_snap[0]; vcam_target[1]=play_cam_snap[1]; vcam_target[2]=play_cam_snap[2];
        cam_yaw=play_cam_snap[3]; cam_pitch=play_cam_snap[4]; cam_dist=play_cam_snap[5];
        playing=false;
    };
    // avanza un frame del juego emulado y coloca la camara del juego
    auto play_update = [&](float dt) {
        if (dt<=0.0f) dt=1.0f/60.0f; if (dt>0.05f) dt=0.05f;
        ImGuiIO& io = ImGui::GetIO();
        // --- fisica (cuerpos rigidos) ---
        g3d_rigidbody_step(dt);
        for (auto& b : sim_bodies){
            g3d_entity_impl_set_position(b.ent, g3d_rigidbody_render_x(b.bid), g3d_rigidbody_render_y(b.bid), g3d_rigidbody_render_z(b.bid));
            g3d_entity_impl_set_rotation(b.ent, g3d_rigidbody_angle_x(b.bid), g3d_rigidbody_angle_y(b.bid), g3d_rigidbody_angle_z(b.bid));
            // Agua: flota/salpica/ondas en el agua que tenga DEBAJO (mar, lago o
            // rio). Cada frame se consulta el nivel local, igual que en el juego.
            {
                float bx = g3d_rigidbody_x(b.bid), by = g3d_rigidbody_y(b.bid), bz = g3d_rigidbody_z(b.bid);
                float wlev = g3d_water_level_at(bx, bz);
                if (wlev > -100000.0f) {
                    if (b.buoy) g3d_rigidbody_set_buoyancy(b.bid, wlev, b.dens);
                    if (by - b.half < wlev) {
                        if (!b.wet) {
                            g3d_water_splash(bx, wlev, bz, 1.0f); b.wet = 1;
                            g3d_rigidbody_set_damping(b.bid, 2.5f, 3.0f);   // el agua frena
                        }
                        float dx=bx-b.prevx, dy=by-b.prevy, dz=bz-b.prevz;
                        if (dx*dx+dy*dy+dz*dz > 0.0004f && by + b.half > wlev - 1.0f) {
                            b.ript += dt;
                            if (b.ript > 0.15f) { g3d_water_ripple(bx, bz, 0.4f); b.ript = 0.0f; }
                        }
                    } else {
                        if (b.wet) g3d_rigidbody_set_damping(b.bid, 0.05f, 0.05f);
                        b.wet = 0;
                    }
                } else {
                    if (b.buoy) g3d_rigidbody_set_buoyancy(b.bid, wlev, 0.0f);   // sin agua
                    if (b.wet) g3d_rigidbody_set_damping(b.bid, 0.05f, 0.05f);
                    b.wet = 0;
                }
                b.prevx = bx; b.prevy = by; b.prevz = bz;
            }
        }
        // --- jugador ---
        float px=0,py=0,pz=0; float wl=0; int inw=0;
        if (sim_pch>=0 && sim_player_idx>=0 && sim_player_idx<(int)objects.size()){
            auto& p=objects[sim_player_idx];
            sim_pprevx=g3d_char_x(sim_pch); sim_pprevz=g3d_char_z(sim_pch);
            float wx=0,wz=0;
            if (!io.WantTextInput){
                if (ImGui::IsKeyDown(ImGuiKey_W)) wz+=1.0f;
                if (ImGui::IsKeyDown(ImGuiKey_S)) wz-=1.0f;
                if (ImGui::IsKeyDown(ImGuiKey_D)) wx+=1.0f;
                if (ImGui::IsKeyDown(ImGuiKey_A)) wx-=1.0f;
            }
            float spd = (io.KeyShift ? p.run_speed : p.walk_speed);
            wl=sqrtf(wx*wx+wz*wz);
            if (wl>0.001f){ wx=wx/wl*spd; wz=wz/wl*spd; sim_facing=atan2f(wx,wz); }
            g3d_char_move(sim_pch,wx,wz);
            if (!io.WantTextInput && ImGui::IsKeyDown(ImGuiKey_Space)) g3d_char_jump(sim_pch,p.jump_force);
            g3d_char_update(sim_pch,dt);
            px=g3d_char_x(sim_pch); py=g3d_char_y(sim_pch); pz=g3d_char_z(sim_pch);
            { float wlev = g3d_water_level_at(px, pz);   // mar, lago o rio bajo el jugador
              if (wlev > -100000.0f) { if (py < wlev - 1.2f) inw = 1; g3d_char_set_water(sim_pch, inw, wlev);
                  if (py < wlev) { if (wl > 0.001f) { sim_ript += dt;
                      if (sim_ript > 0.12f) { g3d_water_ripple(px, pz, 0.6f); sim_ript = 0.0f; } } } }
              else g3d_char_set_water(sim_pch, 0, 0.0f); }
            if (p.zone_layer>=0 && g3d_zone_blocked(px,pz,p.zone_layer)){
                g3d_char_set_position(sim_pch,sim_pprevx,py,sim_pprevz); px=sim_pprevx; pz=sim_pprevz;
            }
            g3d_entity_impl_set_position(sim_player_ent,px,py,pz);
            g3d_entity_impl_set_rotation(sim_player_ent,0.0f,sim_facing,0.0f);
            // animacion por estado
            if (sim_player_model && g3d_model_animation_count(sim_player_model)>0){
                sim_t+=dt; int gnd=g3d_char_grounded(sim_pch); int clip;
                if (inw && p.anim_swim>=0) clip=p.anim_swim;
                else if (gnd==0) clip=p.anim_jump;
                else if (wl>0.001f) clip=(io.KeyShift? p.anim_run : p.anim_walk);
                else clip=p.anim_idle;
                g3d_model_animate(sim_player_model, clip, sim_t, 1);
            }
            // enganches a huesos
            for (auto& a : sim_attach){
                if (a.node<0 || !sim_player_model) continue;
                float nx=g3d_model_node_axis(sim_player_model,a.node,0)*p.scale + a.ox;
                float ny=g3d_model_node_axis(sim_player_model,a.node,1)*p.scale + a.oy;
                float nz=g3d_model_node_axis(sim_player_model,a.node,2)*p.scale + a.oz;
                float c=cosf(sim_facing), s=sinf(sim_facing);
                float wx2=nx*c+nz*s, wz2=-nx*s+nz*c;
                g3d_entity_impl_set_position(a.ent, px+wx2, py+ny, pz+wz2);
                g3d_entity_impl_set_rotation(a.ent, 0.0f, sim_facing + a.yaw*0.0174533f, 0.0f);
                g3d_entity_impl_set_scale(a.ent, a.sc, a.sc, a.sc);
            }
        }
        // --- camara del juego (despues de todo, para seguir la posicion final) ---
        float tx=0,ty=0,tz=0; bool follow=false;
        if (cam_mode!=0 && cam_follow>=0 && cam_follow<(int)objects.size()){
            if (cam_follow==sim_player_idx && sim_pch>=0){ tx=px; ty=py; tz=pz; }
            else { tx=objects[cam_follow].x; ty=objects[cam_follow].y; tz=objects[cam_follow].z; }
            follow=true;
        }
        if (!follow){
            g3d_camera_set_position(cam,cam_pos[0],cam_pos[1],cam_pos[2]);
            g3d_camera_look_at(cam,cam_look[0],cam_look[1],cam_look[2],0.0f,1.0f,0.0f);
        } else if (cam_mode==1){
            g3d_camera_set_position(cam,tx,ty+cam_height,tz-gcam_dist);
            g3d_camera_look_at(cam,tx,ty+1.0f,tz,0.0f,1.0f,0.0f);
        } else if (cam_mode==2){
            // igual que en el juego: mira hacia donde mira, y adelantada para no
            // quedarse dentro del modelo
            float sf = sinf(sim_facing), cf = cosf(sim_facing);
            g3d_camera_set_position(cam, tx+sf*cam_fwd, ty+cam_height, tz+cf*cam_fwd);
            g3d_camera_look_at(cam, tx+sf*(cam_fwd+10.0f), ty+cam_height,
                                    tz+cf*(cam_fwd+10.0f), 0.0f,1.0f,0.0f);
        } else {
            g3d_camera_set_position(cam,tx,ty+gcam_dist,tz+0.5f);
            g3d_camera_look_at(cam,tx,ty,tz,0.0f,1.0f,0.0f);
        }
    };

    Uint32 last_ticks = SDL_GetTicks();
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE &&
                ev.window.windowID == SDL_GetWindowID(window)) running = false;
        }
        SDL_GL_GetDrawableSize(window, &fbw, &fbh);

        Uint32 now_ticks = SDL_GetTicks();
        float frame_dt = (now_ticks - last_ticks) / 1000.0f; last_ticks = now_ticks;

        // Atender Play/Stop pedidos desde la UI, AQUI (fuera del frame de ImGui).
        if (play_req == 1) { play_req = 0; play_start(); }
        else if (play_req == 2) { play_req = 0; play_stop(); }

        // --- Apuntar el estado para deshacer ---
        // Al final de cada frame: si la escena cambio y ya no se esta arrastrando
        // nada, se guarda el estado anterior. Lo de esperar a soltar es lo que hace
        // que mover con el gizmo cuente como UN paso y no como sesenta por segundo.
        if (!playing) {
            EditState ahora{ objects, cam_follow };
            if (!state_init) { last_state = ahora; state_init = true; }
            else if (!state_igual(ahora, last_state) &&
                     !ImGuizmo::IsUsing() && !ImGui::IsAnyItemActive() &&
                     !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                undo_stack.push_back(last_state);
                if (undo_stack.size() > 128) undo_stack.erase(undo_stack.begin());
                redo_stack.clear();
                last_state = ahora;
            }
        }

        // --- Atajos: copiar / pegar / duplicar / borrar ---
        // Solo si no se esta escribiendo (nombres, rutas, el editor de scripts) ni
        // corriendo el Play: si no, Ctrl+C en el codigo borraria un objeto.
        if (!ImGui::GetIO().WantTextInput && !show_script && !playing) {
            bool ctrl = ImGui::GetIO().KeyCtrl;
            bool shift = ImGui::GetIO().KeyShift;
            if (ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Z, false)) do_undo();
            if (ctrl && ((shift && ImGui::IsKeyPressed(ImGuiKey_Z, false)) ||
                         ImGui::IsKeyPressed(ImGuiKey_Y, false)))          do_redo();
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) duplicate_obj(obj_sel);
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) copy_obj(obj_sel);
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) paste_obj();
            if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_Delete, false) && obj_sel >= 0)
                delete_obj(obj_sel);
        }

        // DEPURACION: EDITOR_AUTOPLAY=1 lanza Play solo (para reproducir fallos sin GUI)
        { static int ap = 0;
          if (getenv("EDITOR_AUTOPLAY")) {
              ap++;
              if (ap == 10) {
                  const char* prj = getenv("EDITOR_AUTOPLAY_PROJECT");
                  if (prj) { fprintf(stderr, "[autoplay] -> open_project(%s)\n", prj); fflush(stderr); open_project(prj); }
                  else { fprintf(stderr, "[autoplay] -> load_scene(%s)\n", scene_path.c_str()); fflush(stderr); load_scene(scene_path); }
              }
              if (ap == 25 && getenv("EDITOR_AUTOGEN")) {   // solo generar+compilar y salir
                  fprintf(stderr, "[autogen] -> generate_game()\n"); fflush(stderr);
                  generate_game(false); running = false;
              }
              if (ap == 30) { fprintf(stderr, "[autoplay] -> play_start() #1 objetos=%d\n", (int)objects.size()); fflush(stderr); play_start(); }
              if (ap == 90) { fprintf(stderr, "[autoplay] -> play_stop() #1\n"); fflush(stderr); play_stop(); }
              if (ap == 110) { fprintf(stderr, "[autoplay] -> play_start() #2 (reinicio del runtime)\n"); fflush(stderr); play_start(); }
              if (ap == 170) { fprintf(stderr, "[autoplay] -> play_stop() #2\n"); fflush(stderr); play_stop(); }
              if (ap == 200) running = false;
          } }

        if (playing) {
            play_update(frame_dt);   // PLAY: emula el juego y coloca la camara del juego
        } else {
        // ---- camara del viewport: orbita (raton) + vuelo WASD (con boton derecho) ----
        // Como en Unreal/Unity: manten el BOTON DERECHO sobre el viewport y muevete
        // con WASD (adelante/izq/atras/der), E/Q (subir/bajar). El raton rota la vista.
        ImGuiIO& io2 = ImGui::GetIO();
        bool rmb_view = (vp_hovered || ImGui::IsMouseDown(ImGuiMouseButton_Right)) &&
                        ImGui::IsMouseDown(ImGuiMouseButton_Right);
        if (vp_hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            cam_yaw   += io2.MouseDelta.x * 0.01f;
            cam_pitch += io2.MouseDelta.y * 0.01f;
        }
        if (cam_pitch >  1.5f) cam_pitch =  1.5f;
        if (cam_pitch < -0.2f) cam_pitch = -0.2f;
        // rueda del raton = zoom (acercar/alejar el pivote)
        if (vp_hovered && io2.MouseWheel != 0.0f) {
            cam_dist -= io2.MouseWheel * (cam_dist * 0.1f + 0.5f);
            if (cam_dist < 1.0f)   cam_dist = 1.0f;
            if (cam_dist > 400.0f) cam_dist = 400.0f;
        }
        // vector "hacia delante" horizontal segun el yaw (el que mira la camara)
        float fwx = -sinf(cam_yaw), fwz = -cosf(cam_yaw);   // desde la camara hacia el pivote
        float rgx =  cosf(cam_yaw), rgz = -sinf(cam_yaw);   // derecha
        if (rmb_view && !io2.WantTextInput) {
            float sp = (io2.KeyShift ? 2.5f : 1.0f) * (cam_dist * 0.04f + 0.4f);  // velocidad ~ distancia
            if (ImGui::IsKeyDown(ImGuiKey_W)) { vcam_target[0]+=fwx*sp; vcam_target[2]+=fwz*sp; }
            if (ImGui::IsKeyDown(ImGuiKey_S)) { vcam_target[0]-=fwx*sp; vcam_target[2]-=fwz*sp; }
            if (ImGui::IsKeyDown(ImGuiKey_D)) { vcam_target[0]+=rgx*sp; vcam_target[2]+=rgz*sp; }
            if (ImGui::IsKeyDown(ImGuiKey_A)) { vcam_target[0]-=rgx*sp; vcam_target[2]-=rgz*sp; }
            if (ImGui::IsKeyDown(ImGuiKey_E)) vcam_target[1]+=sp;
            if (ImGui::IsKeyDown(ImGuiKey_Q)) vcam_target[1]-=sp;
        }
        // al seleccionar un objeto, centrar el pivote de la camara en el
        if (obj_sel != last_obj_sel) {
            last_obj_sel = obj_sel;
            if (obj_sel >= 0 && obj_sel < (int)objects.size()) {
                vcam_target[0] = objects[obj_sel].x;
                vcam_target[1] = objects[obj_sel].y;
                vcam_target[2] = objects[obj_sel].z;
            }
        }
        float cx = vcam_target[0] + sinf(cam_yaw) * cosf(cam_pitch) * cam_dist;
        float cy = vcam_target[1] + sinf(cam_pitch) * cam_dist + 1.0f;
        float cz = vcam_target[2] + cosf(cam_yaw) * cosf(cam_pitch) * cam_dist;
        g3d_camera_set_position(cam, cx, cy, cz);
        g3d_camera_look_at(cam, vcam_target[0], vcam_target[1], vcam_target[2], 0.0f, 1.0f, 0.0f);
        }   // fin modo edicion (durante Play manda play_update)

        // ---- construir la UI (dockspace + paneles) ----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        // Esc durante el Play = parar
        if (playing && ImGui::IsKeyPressed(ImGuiKey_Escape)) play_req = 2;
        // atajos de herramienta (no al escribir texto, ni volando con boton derecho, ni en Play)
        if (!playing && !ImGui::GetIO().WantTextInput && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Q)) tool = T_SELECT;
            if (ImGui::IsKeyPressed(ImGuiKey_W)) tool = T_MOVE;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) tool = T_ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) tool = T_SCALE;
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { asset_sel = -1; if (tool == T_PLACE) tool = T_SELECT; }
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) save_scene(scene_path);
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_P)) save_project();

        // ---- barra de menu ----
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Proyecto")) {
                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Guardar proyecto", "Ctrl+P")) save_project();
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_FILE " Nuevo proyecto...")) {
                    projNewDlg.SetPwd(fs::path(project_dir).parent_path()); projNewDlg.Open();
                }
                if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Abrir proyecto...")) {
                    projOpenDlg.SetPwd(fs::path(project_dir).parent_path()); projOpenDlg.Open();
                }
                ImGui::Separator();
                ImGui::TextDisabled("Actual: %s", project_name.c_str());
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Archivo")) {
                if (ImGui::MenuItem("Guardar escena", "Ctrl+S")) save_scene(scene_path);
                if (ImGui::MenuItem("Guardar como..."))          { saveDlg.SetPwd(scenes_dir); saveDlg.Open(); }
                if (ImGui::MenuItem("Cargar escena..."))         { openDlg.SetPwd(scenes_dir); openDlg.Open(); }
                ImGui::Separator();
                if (ImGui::MenuItem("Salir")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Editar")) {
                if (ImGui::MenuItem("Deshacer", "Ctrl+Z", false, !undo_stack.empty())) do_undo();
                if (ImGui::MenuItem("Rehacer", "Ctrl+Shift+Z", false, !redo_stack.empty())) do_redo();
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicar", "Ctrl+D", false, obj_sel >= 0)) duplicate_obj(obj_sel);
                if (ImGui::MenuItem("Copiar",   "Ctrl+C", false, obj_sel >= 0)) copy_obj(obj_sel);
                if (ImGui::MenuItem("Pegar",    "Ctrl+V", false, has_clip))     paste_obj();
                if (ImGui::MenuItem("Borrar",   "Supr",   false, obj_sel >= 0)) delete_obj(obj_sel);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Escena")) {
                if (ImGui::MenuItem("Vaciar escena")) {
                    for (auto& o : objects) g3d_entity_impl_set_position(o.entity, 0, -99999, 0);
                    objects.clear(); obj_sel = -1;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Terreno")) {
                ImGui::TextDisabled("TERRENO PROCEDURAL");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Relieve##m", &proc_amp, 5.0f, 60.0f, "%.0f");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Semilla##m", &proc_seed, 1.0f, 999.0f, "%.0f");
                if (ImGui::MenuItem("Semilla aleatoria")) proc_seed = (float)(1 + (rand() % 999));
                if (ImGui::MenuItem(ICON_FA_MOUNTAIN_SUN " Generar terreno")) {
                    generate_procedural_terrain(proc_amp);
                    status = "Terreno procedural + agua auto generados";
                }
                if (ImGui::MenuItem(ICON_FA_PAINTBRUSH " Auto-pintar texturas")) {
                    auto_paint_terrain();
                    status = "Terreno pintado por altura/pendiente";
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Pinta el terreno actual con las texturas de Assets:\nhierba, roca (empinado), nieve (cima), arena (bajo).");
                ImGui::Separator();
                ImGui::TextDisabled("AGUA AUTOMATICA");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Sensib. rio##m", &hyd_river_thresh, 50.0f, 2000.0f, "%.0f");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Prof. min lago##m", &hyd_lake_depth, 0.3f, 8.0f, "%.1f");
                if (ImGui::MenuItem(ICON_FA_WAND_MAGIC_SPARKLES " Generar agua")) {
                    generate_water_auto();
                    status = "Agua auto: " + std::to_string(g3d_hydrology_lake_count()) + " lago(s), " +
                             std::to_string(g3d_hydrology_river_count()) + " rio(s)";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Juego")) {
                if (ImGui::MenuItem("Generar y compilar")) generate_game(false);
                if (ImGui::MenuItem(ICON_FA_PLAY " Generar y ejecutar")) generate_game(true);
                ImGui::Separator();
                if (ImGui::MenuItem("Editar main.prg")) open_main_script();
                if (ImGui::MenuItem("Rehacer main.prg...")) ask_regen_main = true;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Vuelve a dejarlo como recien creado.\nSe pierde lo que hayas escrito.");
                ImGui::EndMenu();
            }
            // ---- toolbar de iconos (herramientas del editor) ----
            auto toolBtn = [&](const char* icon, int t, const char* tip) {
                bool on = (tool == t);
                if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
                if (ImGui::Button(icon)) tool = t;
                if (on) ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
            };
            ImGui::SameLine(0, 24);
            toolBtn(ICON_FA_ARROW_POINTER,       T_SELECT, "Seleccionar");
            toolBtn(ICON_FA_UP_DOWN_LEFT_RIGHT,  T_MOVE,   "Mover (W)");
            toolBtn(ICON_FA_ROTATE,              T_ROTATE, "Rotar (E)");
            toolBtn(ICON_FA_MAXIMIZE,            T_SCALE,  "Escalar (R)");
            ImGui::SameLine(0, 12); ImGui::TextDisabled("|"); ImGui::SameLine(0, 12);
            toolBtn(ICON_FA_CUBE,                T_PLACE,  "Colocar asset");
            ImGui::SameLine(0, 12); ImGui::TextDisabled("|"); ImGui::SameLine(0, 12);
            toolBtn(ICON_FA_MOUNTAIN,            T_RAISE,   "Terreno: subir (montanas)");
            toolBtn(ICON_FA_ARROW_DOWN,          T_LOWER,   "Terreno: bajar (valles)");
            toolBtn(ICON_FA_BROOM,               T_SMOOTH,  "Terreno: suavizar");
            toolBtn(ICON_FA_ARROWS_DOWN_TO_LINE, T_FLATTEN, "Terreno: nivelar");
            toolBtn(ICON_FA_PAINTBRUSH,          T_PAINT,   "Terreno: pintar textura");
            toolBtn(ICON_FA_CIRCLE_NOTCH,        T_HOLE,    "Terreno: agujero (para bocas de cueva)");
            ImGui::SameLine(0, 12); ImGui::TextDisabled("|"); ImGui::SameLine(0, 12);
            toolBtn(ICON_FA_DRAW_POLYGON,        T_ZONE,    "Pintar ZONAS de barrera (por donde no pasan ciertos objetos)");
            toolBtn(ICON_FA_DROPLET,             T_LAKE,    "Lago: clic en un hoyo del terreno para llenarlo de agua");
            toolBtn(ICON_FA_WATER,               T_RIVER,   "Rio: clic para poner puntos del cauce, doble clic para terminar");
            toolBtn(ICON_FA_ANGLES_DOWN,         T_WATERFALL, "Cascada: clic arriba (el borde) y clic abajo (la base/poza)");
            toolBtn(ICON_FA_FAUCET_DRIP,         T_WATERSOURCE, "Manantial: clic para poner una fuente; el agua fluye sola (rios/cascadas con fisica)");

            // ---- PLAY: compila el juego y lo ejecuta en su propia ventana ----
            // Es un proceso BennuGD2 normal, con todos sus hooks: fidelidad total
            // (2D, FPS, input, scripts) y si el juego falla no se lleva al editor.
            ImGui::SameLine(0, 24);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.25f, 1.0f));
            if (ImGui::Button(ICON_FA_PLAY " Play")) generate_game(true);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Compila el juego y lo ejecuta en su ventana.\nLa salida sale en la Consola.");

            // ---- Vista previa rapida DENTRO del viewport (sin compilar) ----
            ImGui::SameLine(0, 8);
            if (!playing) {
                if (ImGui::Button(ICON_FA_EYE " Vista previa")) play_req = 1;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Previsualiza aqui mismo, al instante y sin compilar:\n"
                                      "fisica, jugador (WASD), camaras y zonas.\n"
                                      "Es una aproximacion: NO ejecuta tus scripts.");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button(ICON_FA_STOP " Parar")) play_req = 2;
                ImGui::PopStyleColor();
                ImGui::SameLine(); ImGui::TextColored(ImVec4(0.3f,1,0.4f,1), "  vista previa");
            }

            if (!status.empty()) {
                ImGui::SameLine(ImGui::GetWindowWidth() - 320);
                ImGui::TextDisabled("%s", status.c_str());
            }
            ImGui::EndMainMenuBar();
        }
        // gizmo_op segun la herramienta activa
        if (tool == T_MOVE)   gizmo_op = ImGuizmo::TRANSLATE;
        if (tool == T_ROTATE) gizmo_op = ImGuizmo::ROTATE;
        if (tool == T_SCALE)  gizmo_op = ImGuizmo::SCALE;

        // ---- dialogos de archivo ----
        openDlg.Display();
        if (openDlg.HasSelected()) { load_scene(openDlg.GetSelected().string()); openDlg.ClearSelected(); }
        saveDlg.Display();
        if (saveDlg.HasSelected()) {
            std::string p = saveDlg.GetSelected().string();
            if (p.size() < 6 || p.substr(p.size() - 6) != ".scene") p += ".scene";
            save_scene(p); saveDlg.ClearSelected();
        }
        projOpenDlg.Display();
        if (projOpenDlg.HasSelected()) { open_project(projOpenDlg.GetSelected().string()); projOpenDlg.ClearSelected(); }
        projNewDlg.Display();
        if (projNewDlg.HasSelected()) {
            std::string p = projNewDlg.GetSelected().string();
            if (p.size() < 5 || p.substr(p.size() - 5) != ".bgd2") p += ".bgd2";
            create_project(p); projNewDlg.ClearSelected();
        }

        // ---- leer lo que va soltando el juego en ejecucion (bgdi) ----
        if (run_log || !run_log_path.empty()) {
            if (!run_log) run_log = fopen(run_log_path.c_str(), "r");  // aun no existia al lanzarlo
            if (run_log) {
                char b[1024]; size_t n;
                while ((n = fread(b, 1, sizeof(b) - 1, run_log)) > 0) { b[n] = 0; console_add(b); }
                clearerr(run_log);   // seguir leyendo cuando el juego escriba mas
            }
        }

        // Dockspace a pantalla completa: los paneles se acoplan alrededor.
        ImGuiID ds = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Layout por defecto la primera vez (Assets izq, Inspector der, Escena centro).
        static bool dock_init = false;
        if (!dock_init) {
            dock_init = true;
            ImGui::DockBuilderRemoveNode(ds);
            ImGui::DockBuilderAddNode(ds, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(ds, ImGui::GetMainViewport()->WorkSize);
            ImGuiID center = ds, left, right, lbottom;
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,   0.17f, &left,    &center);
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Right,  0.22f, &right,   &center);
            ImGui::DockBuilderSplitNode(left,   ImGuiDir_Down,   0.5f,  &lbottom, &left);
            ImGui::DockBuilderDockWindow("Assets",    left);
            ImGui::DockBuilderDockWindow("Jerarquia", lbottom);
            ImGui::DockBuilderDockWindow("Entorno",   lbottom);
            ImGui::DockBuilderDockWindow("Inspector", right);
            ImGuiID bottom;
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.26f, &bottom, &center);
            ImGui::DockBuilderDockWindow("Escena",    center);
            // OJO: el nombre tiene que coincidir EXACTAMENTE con el del Begin(),
            // icono incluido, o DockBuilder no encuentra la ventana y queda suelta.
            ImGui::DockBuilderDockWindow(ICON_FA_TERMINAL "  Consola", bottom);
            ImGui::DockBuilderFinish(ds);
        }

        // --- Panel: Escena (viewport 3D dibujado a textura) ---
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Escena");
        ImVec2 avail = ImGui::GetContentRegionAvail();
        vp_w = (int)avail.x; vp_h = (int)avail.y;
        vp.resize(vp_w, vp_h);
        // textura del motor (el motor ya la entrega en la orientacion correcta)
        ImGui::Image((ImTextureID)(intptr_t)vp.tex, avail, ImVec2(0, 0), ImVec2(1, 1));
        vp_hovered = ImGui::IsItemHovered();
        ImVec2 img_min = ImGui::GetItemRectMin();

        // ---- ARRASTRAR Y SOLTAR: colocar un asset arrastrado desde el panel Assets ----
        // Mientras arrastras, un "fantasma" del modelo sigue el cursor por el nivel;
        // al soltar se queda colocado ahi y queda seleccionado.
        if (!playing && ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* pl = ImGui::AcceptDragDropPayload(
                "ASSET_IDX", ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (pl && pl->Data) {
                int idx = *(const int*)pl->Data;
                ImVec2 mp = ImGui::GetIO().MousePos;
                float sx = mp.x - img_min.x, sy = mp.y - img_min.y;
                float hit[3];
                int ok = place_point(sx, sy, hit);
                if (ok && idx >= 0 && idx < (int)assets.size()) {
                    void* m = load_model(assets[idx]);
                    if (m) {
                        if (drag_ent < 0 || drag_asset != idx) {
                            if (drag_ent >= 0) g3d_entity_impl_set_position(drag_ent, 0, -99999, 0);
                            drag_ent = g3d_model_spawn(scene, m, hit[0], hit[1], hit[2], 0.0f, 0.0f);
                            drag_asset = idx;
                        }
                        g3d_entity_impl_set_position(drag_ent, hit[0], hit[1], hit[2]);
                        if (pl->IsDelivery()) {           // soltado -> objeto real
                            SObj o; o.asset = assets[idx];
                            o.name = assets[idx].substr(0, assets[idx].find('.')) +
                                     "_" + std::to_string((int)objects.size());
                            o.entity = drag_ent;
                            o.x = hit[0]; o.y = hit[1]; o.z = hit[2]; o.ry = 0; o.scale = 1;
                            objects.push_back(o); obj_sel = (int)objects.size() - 1;
                            drag_ent = -1; drag_asset = -1;
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        // si el arrastre termino fuera del viewport, retirar el fantasma
        if (drag_ent >= 0) {
            const ImGuiPayload* cur = ImGui::GetDragDropPayload();
            if (!cur || !cur->IsDataType("ASSET_IDX")) {
                g3d_entity_impl_set_position(drag_ent, 0, -99999, 0);
                drag_ent = -1; drag_asset = -1;
            }
        }


        // ---- GIZMO (solo con Mover/Rotar/Escalar y un objeto seleccionado) ----
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(img_min.x, img_min.y, avail.x, avail.y);
        bool gizmo_tool = !playing && (tool == T_MOVE || tool == T_ROTATE || tool == T_SCALE);
        if (gizmo_tool && obj_sel >= 0 && obj_sel < (int)objects.size()) {
            SObj& o = objects[obj_sel];
            float view[16], proj[16], model[16];
            g3d_editor_get_view(view); g3d_editor_get_proj(proj);
            float t[3] = { o.x, o.y, o.z };
            float r[3] = { 0.0f, o.ry * 57.29578f, 0.0f };
            float s[3] = { o.scale, o.scale, o.scale };
            ImGuizmo::RecomposeMatrixFromComponents(t, r, s, model);
            ImGuizmo::Manipulate(view, proj, gizmo_op, ImGuizmo::WORLD, model);
            if (ImGuizmo::IsUsing()) {
                ImGuizmo::DecomposeMatrixToComponents(model, t, r, s);
                o.x = t[0]; o.y = t[1]; o.z = t[2];
                o.ry = r[1] * 0.0174533f;
                o.scale = (s[0] + s[1] + s[2]) / 3.0f;
            }
        }

        // ---- PREVIEW de la camara principal del juego en el viewport ----
        // Dibuja un frustum amarillo donde quedara la camara y su linea de mira,
        // para saber donde se esta colocando (o desde donde seguira al objeto).
        {
            float cp[3], ct[3]; bool have = true;
            if (cam_mode == 0) {
                cp[0]=cam_pos[0]; cp[1]=cam_pos[1]; cp[2]=cam_pos[2];
                ct[0]=cam_look[0]; ct[1]=cam_look[1]; ct[2]=cam_look[2];
            } else {
                int fi = (cam_follow >= 0 && cam_follow < (int)objects.size()) ? cam_follow
                         : (obj_sel >= 0 ? obj_sel : -1);
                if (fi < 0) have = false;
                else {
                    float tx=objects[fi].x, ty=objects[fi].y, tz=objects[fi].z;
                    if (cam_mode == 1)      { cp[0]=tx; cp[1]=ty+cam_height; cp[2]=tz-gcam_dist; ct[0]=tx; ct[1]=ty+1.0f; ct[2]=tz; }
                    else if (cam_mode == 2) {
                        float ry = objects[cam_follow].ry, sf = sinf(ry), cf = cosf(ry);
                        cp[0]=tx+sf*cam_fwd; cp[1]=ty+cam_height; cp[2]=tz+cf*cam_fwd;
                        ct[0]=tx+sf*(cam_fwd+10.0f); ct[1]=ty+cam_height; ct[2]=tz+cf*(cam_fwd+10.0f);
                    }
                    else                    { cp[0]=tx; cp[1]=ty+gcam_dist; cp[2]=tz+0.5f; ct[0]=tx; ct[1]=ty; ct[2]=tz; }
                }
            }
            if (have) {
                float fwd[3]={ct[0]-cp[0],ct[1]-cp[1],ct[2]-cp[2]};
                float fl=sqrtf(fwd[0]*fwd[0]+fwd[1]*fwd[1]+fwd[2]*fwd[2]); if(fl<1e-4f)fl=1;
                fwd[0]/=fl; fwd[1]/=fl; fwd[2]/=fl;
                float up[3]={0,1,0};
                float rgt[3]={fwd[1]*up[2]-fwd[2]*up[1], fwd[2]*up[0]-fwd[0]*up[2], fwd[0]*up[1]-fwd[1]*up[0]};
                float rl=sqrtf(rgt[0]*rgt[0]+rgt[1]*rgt[1]+rgt[2]*rgt[2]); if(rl<1e-4f)rl=1;
                rgt[0]/=rl; rgt[1]/=rl; rgt[2]/=rl;
                float u2[3]={rgt[1]*fwd[2]-rgt[2]*fwd[1], rgt[2]*fwd[0]-rgt[0]*fwd[2], rgt[0]*fwd[1]-rgt[1]*fwd[0]};
                float d=8.0f, hw=5.0f, hh=3.0f;
                int sgn[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};
                float corners[4][3];
                for(int k=0;k<4;k++) for(int c=0;c<3;c++)
                    corners[k][c]=cp[c]+fwd[c]*d + rgt[c]*hw*sgn[k][0] + u2[c]*hh*sgn[k][1];
                auto proj=[&](const float* w, ImVec2& out)->bool{
                    float p2[2];
                    if(!g3d_editor_world_to_screen(w[0],w[1],w[2],(float)vp.w,(float)vp.h,p2)) return false;
                    out=ImVec2(img_min.x+p2[0], img_min.y+p2[1]); return true;
                };
                ImDrawList* dl=ImGui::GetWindowDrawList();
                ImU32 col=IM_COL32(255,215,60,235);
                ImVec2 sp, sc[4]; bool okp=proj(cp,sp); bool okc[4];
                for(int k=0;k<4;k++) okc[k]=proj(corners[k],sc[k]);
                for(int k=0;k<4;k++) if(okp&&okc[k])            dl->AddLine(sp,sc[k],col,2.0f);
                for(int k=0;k<4;k++) if(okc[k]&&okc[(k+1)%4])   dl->AddLine(sc[k],sc[(k+1)%4],col,2.0f);
                ImVec2 st;
                if(okp&&proj(ct,st)) dl->AddLine(sp,st,IM_COL32(255,120,120,180),1.5f);
                if(okp){ dl->AddCircleFilled(sp,5.0f,col); dl->AddText(ImVec2(sp.x+7,sp.y-8),col,"CAM"); }
            }
        }

        // ---- OVERLAY de las ZONAS pintadas (solo con la herramienta de zonas) ----
        if (tool == T_ZONE) {
            int zs = g3d_zone_side();
            if (zs >= 2) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImU32 lcol[4] = { IM_COL32(255,60,60,150), IM_COL32(80,220,90,150),
                                        IM_COL32(80,150,255,150), IM_COL32(240,220,60,150) };
                int grid = zs - 1; float ws = 400.0f;
                for (int iz = 0; iz <= grid; iz += 2)
                    for (int ix = 0; ix <= grid; ix += 2) {
                        float wx = ((float)ix / grid - 0.5f) * ws;
                        float wz = ((float)iz / grid - 0.5f) * ws;
                        int v = g3d_zone_value(wx, wz);
                        if (!v) continue;
                        int layer = 0; while (layer < 3 && !(v & (1 << layer))) layer++;
                        float wy = g3d_editor_terrain_height(terrain, wx, wz) + 0.2f;
                        float p2[2];
                        if (g3d_editor_world_to_screen(wx, wy, wz, (float)vp.w, (float)vp.h, p2))
                            dl->AddCircleFilled(ImVec2(img_min.x + p2[0], img_min.y + p2[1]), 3.5f, lcol[layer]);
                    }
            }
        }

        // quita la previsualizacion del lago si ya no aplica (otra herramienta,
        // fuera del viewport, o en Play)
        if (lake_prev_on && (tool != T_LAKE || !vp_hovered || playing || ImGuizmo::IsOver())) {
            lake_prev_on = false; lake_prev_key = -1;
            rebuild_water();
        }
        // ---- interaccion del viewport segun la herramienta ----
        if (!playing && vp_hovered && !ImGuizmo::IsOver()) {
            ImVec2 mp = ImGui::GetIO().MousePos;
            float sx = mp.x - img_min.x, sy = mp.y - img_min.y;
            float hit[3];
            bool terr_tool = (tool == T_RAISE || tool == T_LOWER || tool == T_SMOOTH ||
                              tool == T_FLATTEN || tool == T_PAINT || tool == T_HOLE || tool == T_ZONE);
            if (terr_tool && terrain &&
                g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                // ANILLO indicador de la zona del pincel (proyecta el circulo al terreno)
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const int N = 48;
                ImVec2 prev; bool have_prev = false;
                ImU32 col = (tool == T_LOWER) ? IM_COL32(255,140,90,230) : IM_COL32(90,190,255,230);
                for (int k = 0; k <= N; k++) {
                    float a = 6.2831853f * k / N;
                    float wx = hit[0] + cosf(a) * brush_r, wz = hit[2] + sinf(a) * brush_r;
                    float wy = g3d_editor_terrain_height(terrain, wx, wz);
                    float p2[2];
                    if (g3d_editor_world_to_screen(wx, wy, wz, (float)vp.w, (float)vp.h, p2)) {
                        ImVec2 p(img_min.x + p2[0], img_min.y + p2[1]);
                        if (have_prev) dl->AddLine(prev, p, col, 2.0f);
                        prev = p; have_prev = true;
                    } else have_prev = false;
                }
                // esculpir mientras se mantiene el boton izquierdo
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    float amt = brush_str * 0.5f;
                    if (tool == T_RAISE)   g3d_editor_terrain_raise(terrain, hit[0], hit[2], brush_r, amt);
                    if (tool == T_LOWER)   g3d_editor_terrain_raise(terrain, hit[0], hit[2], brush_r, -amt);
                    if (tool == T_SMOOTH)  g3d_editor_terrain_smooth(terrain, hit[0], hit[2], brush_r, brush_str);
                    if (tool == T_FLATTEN) g3d_editor_terrain_flatten(terrain, hit[0], hit[2], brush_r, brush_str);
                    if (tool == T_PAINT && paint_tex(paint_sel))
                        g3d_editor_terrain_paint(terrain, paint_tex(paint_sel), paint_tiling,
                                                 hit[0], hit[2], brush_r, paint_op);
                    if (tool == T_HOLE)
                        g3d_editor_terrain_hole(terrain, hit[0], hit[2], brush_r, hole_fill ? 0 : 1);
                    if (tool == T_ZONE)
                        g3d_zone_paint(hit[0], hit[2], brush_r, zone_layer, zone_erase ? 0 : 1);
                }
                // al soltar una pincelada, si hay agua hay que rehacerla: sigue
                // la forma del relieve y este acaba de cambiar.
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    if (!lakes.empty() || !rivers.empty()) rebuild_water();
                    if (g3d_watersim_active()) watersim_sync(true);   // el agua sigue el relieve nuevo
                }
            } else if (tool == T_RIVER && terrain &&
                       g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                // RIO: cada clic anade un punto del cauce; doble clic lo termina.
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if ((int)river_draft.size() >= 4) {   // al menos 2 puntos
                        // Si un extremo apunta a un lago cercano, ALARGA el trazo hasta
                        // el (unos puntos mas), para que el cauce excavado CONECTE con
                        // el lago y no quede un trozo de cauce seco entre ambos.
                        auto extend_to_lake = [&](bool front) {
                            int np = (int)river_draft.size() / 2; if (np < 2) return;
                            float ax, az, ex, ez;
                            if (front) { ax=river_draft[0]; az=river_draft[1];
                                         ex=ax-river_draft[2]; ez=az-river_draft[3]; }
                            else { ax=river_draft[(np-1)*2]; az=river_draft[(np-1)*2+1];
                                   ex=ax-river_draft[(np-2)*2]; ez=az-river_draft[(np-2)*2+1]; }
                            float L=sqrtf(ex*ex+ez*ez); if (L<1e-4f) return; ex/=L; ez/=L;
                            float hit=-1.0f;
                            for (float d=river_width*0.5f; d<=river_width*5.0f; d+=river_width*0.4f)
                                if (g3d_lake_covers(ax+ex*d, az+ez*d)) { hit=d; break; }
                            if (hit < 0.0f) return;   // no hay lago en esa direccion
                            std::vector<float> add;   // puntos hasta un poco DENTRO del lago
                            for (float d=river_width*0.5f; d<=hit+river_width; d+=river_width*0.5f)
                                { add.push_back(ax+ex*d); add.push_back(az+ez*d); }
                            if (front) {
                                std::vector<float> nd;
                                for (int i=(int)add.size()/2-1; i>=0; i--)
                                    { nd.push_back(add[i*2]); nd.push_back(add[i*2+1]); }
                                nd.insert(nd.end(), river_draft.begin(), river_draft.end());
                                river_draft.swap(nd);
                            } else {
                                river_draft.insert(river_draft.end(), add.begin(), add.end());
                            }
                        };
                        extend_to_lake(false);   // final del trazo
                        extend_to_lake(true);    // inicio del trazo
                        River nr{ river_draft, river_width, river_depth, current_fx(), {} };
                        nr.terrain_before = snapshot_terrain();               // ANTES de excavar
                        carve_river(river_draft, river_width, river_depth);   // excava el lecho
                        rivers.push_back(std::move(nr));
                        rebuild_water();
                        status = "Rio anadido (" + std::to_string(river_draft.size()/2) + " puntos)";
                    }
                    river_draft.clear();
                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    river_draft.push_back(hit[0]);
                    river_draft.push_back(hit[2]);
                }
            } else if (tool == T_LAKE && terrain &&
                       g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                // LAGO estilo editor pro: al pasar el raton se PREVISUALIZA el agua que
                // se colocaria (capa horizontal a un nivel, acotada por el terreno); la
                // RUEDA sube/baja el nivel; el clic lo confirma.
                g3d_scene_set_terrain_collider(terrain);   // heightfield fresco
                g3d_fluid_block_reset();                   // los rios bloquean el desborde
                for (auto& rv : rivers) {
                    int n = (int)rv.pts.size() / 2; if (n < 2) continue;
                    std::vector<float> bx(n * 3);
                    for (int k = 0; k < n; k++) { bx[k*3]=rv.pts[k*2]; bx[k*3+1]=0.0f; bx[k*3+2]=rv.pts[k*2+1]; }
                    g3d_fluid_block_river(bx.data(), n, rv.width);
                }
                float th = g3d_editor_terrain_height(terrain, hit[0], hit[2]);
                // rueda del raton -> ajusta el nivel manual (y desactiva el automatico)
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) { lake_auto = false; lake_level += wheel * 1.0f; }
                float lvl = lake_auto
                    ? g3d_lake_spill_level_r(hit[0], hit[2], lake_radius) - 0.3f   // borde LOCAL
                    : lake_level;
                if (lake_auto) lake_level = lvl;   // el slider sigue al automatico
                // previsualiza (solo rehace el agua cuando cambia la celda o el nivel)
                long key = ((long)(hit[0]*0.5f)*100003L + (long)(hit[2]*0.5f)) * 1000L + (long)(lvl*4.0f);
                lake_prev = { hit[0], hit[2], lvl, lake_depth, lake_radius, current_fx() };
                if (!lake_prev_on || key != lake_prev_key) {
                    lake_prev_on = true; lake_prev_key = key;
                    rebuild_water();
                }
                status = (lvl <= th + 0.05f)
                    ? "El nivel queda por debajo del suelo aqui (sube con la rueda)."
                    : "Rueda: nivel " + std::to_string((int)lvl) + "  |  clic para colocar el lago";
                // clic: confirma el lago de la previsualizacion
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && lvl > th + 0.05f) {
                    lakes.push_back({ hit[0], hit[2], lvl, lake_depth, lake_radius, current_fx() });
                    lake_prev_on = false; lake_prev_key = -1;
                    rebuild_water();
                    status = "Lago anadido (nivel " + std::to_string((int)lvl) + ")";
                }
            } else if (tool == T_WATERFALL && terrain &&
                       g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                // CASCADA: 1er clic = el borde (arriba), 2o clic = la base (poza).
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (!wf_have_top) {
                        wf_top[0]=hit[0]; wf_top[1]=hit[1]; wf_top[2]=hit[2];
                        wf_have_top = true;
                        status = "Cascada: ahora haz clic en la BASE (abajo)";
                    } else {
                        Waterfall wf;
                        wf.top[0]=wf_top[0]; wf.top[1]=wf_top[1]; wf.top[2]=wf_top[2];
                        wf.base[0]=hit[0]; wf.base[1]=hit[1]; wf.base[2]=hit[2];
                        wf.width = wf_width; wf.arc = wf_arc; wf.fx = WaterfallFX();
                        waterfalls.push_back(wf);
                        wf_have_top = false;
                        rebuild_water();
                        status = "Cascada anadida";
                    }
                }
            } else if (tool == T_WATERSOURCE && terrain &&
                       g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                // MANANTIAL: pone una fuente; el simulador hace fluir el agua (rios,
                // charcos y cascadas por fisica). Clic derecho quita la mas cercana.
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    wsources.push_back({ hit[0], hit[2], ws_rate });
                    watersim_sync(true);
                    status = "Manantial anadido (el agua fluira sola)";
                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !wsources.empty()) {
                    int bi = -1; float best = 1e30f;
                    for (int i = 0; i < (int)wsources.size(); i++) {
                        float dx = wsources[i].x - hit[0], dz = wsources[i].z - hit[2];
                        float d = dx*dx + dz*dz; if (d < best) { best = d; bi = i; }
                    }
                    if (bi >= 0) { wsources.erase(wsources.begin() + bi); watersim_sync(true); status = "Manantial quitado"; }
                }
            } else if (!terr_tool && tool != T_LAKE && tool != T_RIVER && tool != T_WATERFALL && tool != T_WATERSOURCE && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // punto: sobre agua->superficie, si no sobre el terreno/fondo
                int ok = place_point(sx, sy, hit);
                if (ok) {
                    if (tool == T_PLACE && asset_sel >= 0) {          // COLOCAR
                        void* m = load_model(assets[asset_sel]);
                        if (m) {
                            int e = g3d_model_spawn(scene, m, hit[0], hit[1], hit[2], 0.0f, 0.0f);
                            SObj o; o.asset = assets[asset_sel];
                            o.name = assets[asset_sel].substr(0, assets[asset_sel].find('.')) +
                                     "_" + std::to_string((int)objects.size());
                            o.entity = e; o.x = hit[0]; o.y = hit[1]; o.z = hit[2]; o.ry = 0; o.scale = 1;
                            objects.push_back(o); obj_sel = (int)objects.size() - 1;
                        }
                    } else {                                         // SELECCIONAR
                        float best = 1e12f; int bi = -1;
                        for (int i = 0; i < (int)objects.size(); i++) {
                            float dx = objects[i].x - hit[0], dz = objects[i].z - hit[2];
                            float d = dx*dx + dz*dz;
                            if (d < best) { best = d; bi = i; }
                        }
                        if (bi >= 0 && best < 36.0f) obj_sel = bi;
                    }
                }
            }
        }
        // dibujar el trazo del rio en curso (puntos + lineas) mientras se traza
        if (tool == T_RIVER && !river_draft.empty() && terrain) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 prev; bool have = false;
            int np = (int)river_draft.size() / 2;
            for (int k = 0; k < np; k++) {
                float wx = river_draft[k*2], wz = river_draft[k*2+1];
                float wy = g3d_editor_terrain_height(terrain, wx, wz) + 0.3f;
                float p2[2];
                if (g3d_editor_world_to_screen(wx, wy, wz, (float)vp.w, (float)vp.h, p2)) {
                    ImVec2 p(img_min.x + p2[0], img_min.y + p2[1]);
                    if (have) dl->AddLine(prev, p, IM_COL32(80,190,255,230), 3.0f);
                    dl->AddCircleFilled(p, 5.0f, IM_COL32(120,210,255,255));
                    prev = p; have = true;
                }
            }
        }
        // CASCADAS: dibuja la CURVA (arco) de cada cascada, y la que se esta
        // colocando siguiendo el raton, en tiempo real. Replica la geometria del
        // motor (parabola en la direccion de la caida) para que coincida.
        if (tool == T_WATERFALL && terrain) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            auto draw_arc = [&](const float* top, const float* base, float width, float arc,
                                ImU32 col, float th) {
                float tx=top[0], ty=top[1]+0.3f, tz=top[2];
                float bx=base[0], bz=base[2];
                float by=g3d_editor_terrain_height(terrain, bx, bz);
                float dxh=bx-tx, dzh=bz-tz; float hlen=sqrtf(dxh*dxh+dzh*dzh);
                float hx=(hlen>0.05f)?dxh/hlen:0.0f, hz=(hlen>0.05f)?dzh/hlen:1.0f;
                float fwd=width*0.5f+1.0f;
                ImVec2 prev; bool have=false;
                for (int j=0;j<=20;j++) {
                    float fv=(float)j/20.0f;
                    float cx=tx+(bx-tx)*fv, cy=ty+(by-ty)*fv, cz=tz+(bz-tz)*fv;
                    float pushf=fwd*(0.55f+0.45f*fv) + arc*4.0f*fv*(1.0f-fv);
                    cx+=hx*pushf; cz+=hz*pushf;
                    float p2[2];
                    if (g3d_editor_world_to_screen(cx, cy, cz, (float)vp.w, (float)vp.h, p2)) {
                        ImVec2 p(img_min.x+p2[0], img_min.y+p2[1]);
                        if (have) dl->AddLine(prev, p, col, th);
                        prev=p; have=true;
                    }
                }
            };
            for (auto& w : waterfalls)
                draw_arc(w.top, w.base, w.width, w.arc, IM_COL32(120,210,255,200), 2.5f);
            if (wf_have_top) {   // la que se esta colocando: hasta el raton
                ImVec2 mp = ImGui::GetIO().MousePos;
                float msx = mp.x - img_min.x, msy = mp.y - img_min.y, mhit[3];
                if (g3d_editor_terrain_pick(msx, msy, (float)vp.w, (float)vp.h, terrain, mhit))
                    draw_arc(wf_top, mhit, wf_width, wf_arc, IM_COL32(255,220,90,240), 3.0f);
            }
        }
        // LAGO: previsualiza el CIRCULO del radio max (la "presa" que acota el
        // llenado) bajo el raton, para que se vea cuanto abarcara el lago.
        if (tool == T_LAKE && terrain && lake_radius >= 1.0f) {
            ImVec2 mp = ImGui::GetIO().MousePos;
            float msx = mp.x - img_min.x, msy = mp.y - img_min.y, c[3];
            if (g3d_editor_terrain_pick(msx, msy, (float)vp.w, (float)vp.h, terrain, c)) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 prev; bool have = false, first = false; ImVec2 firstp;
                for (int k = 0; k <= 48; k++) {
                    float a = (float)k / 48.0f * 6.2831853f;
                    float wx = c[0] + cosf(a)*lake_radius, wz = c[2] + sinf(a)*lake_radius;
                    float wy = g3d_editor_terrain_height(terrain, wx, wz) + 0.2f;
                    float p2[2];
                    if (g3d_editor_world_to_screen(wx, wy, wz, (float)vp.w, (float)vp.h, p2)) {
                        ImVec2 p(img_min.x+p2[0], img_min.y+p2[1]);
                        if (have) dl->AddLine(prev, p, IM_COL32(90,200,255,200), 2.0f);
                        if (!first) { firstp = p; first = true; }
                        prev = p; have = true;
                    }
                }
                if (first && have) dl->AddLine(prev, firstp, IM_COL32(90,200,255,200), 2.0f);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // --- Panel: Consola (salida de bgdc al compilar y del juego al ejecutar) ---
        if (console_focus) { ImGui::SetNextWindowFocus(); console_focus = false; }
        ImGui::Begin(ICON_FA_TERMINAL "  Consola");
        if (ImGui::SmallButton("Limpiar")) game_out.clear();
        ImGui::SameLine();
        bool running_game = (run_log != nullptr);
        if (running_game) ImGui::TextColored(ImVec4(0.4f,1,0.5f,1), "  juego en ejecucion");
        else if (!game_out.empty())
            ImGui::TextColored(last_compile_ok ? ImVec4(0.4f,1,0.5f,1) : ImVec4(1,0.45f,0.4f,1),
                               last_compile_ok ? "  ultima compilacion: OK" : "  ultima compilacion: FALLO");
        ImGui::Separator();
        ImGui::BeginChild("consola_txt", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,1));
        ImGui::TextUnformatted(game_out.c_str());
        ImGui::PopStyleVar();
        if (console_scroll) { ImGui::SetScrollHereY(1.0f); console_scroll = false; }
        ImGui::EndChild();
        ImGui::End();

        // --- Panel: Assets (modelos del proyecto: <proyecto>/Assets) ---
        ImGui::Begin("Assets");
        ImGui::TextDisabled("Proyecto/Assets  (%d)", (int)assets.size());
        if (ImGui::SmallButton("Refrescar")) { assets = scan_assets(assets_dir); asset_sel = -1; }
        ImGui::Separator();
        if (asset_sel >= 0) {
            ImGui::TextColored(ImVec4(0.5f,0.8f,1,1), "COLOCAR: clic en la escena");
            ImGui::SameLine(); if (ImGui::SmallButton("x")) asset_sel = -1;
        } else ImGui::TextDisabled("clic en un asset para colocar");
        ImGui::TextDisabled("Arrastra a la escena para colocar - doble clic = animaciones");
        ImGui::BeginDisabled(!water_on);
        ImGui::Checkbox("Colocar sobre el agua", &place_on_water);
        ImGui::EndDisabled();
        if (water_on) ImGui::TextDisabled("(manten Alt para ponerlo en el fondo)");
        ImGui::BeginChild("lista_assets");
        for (int i = 0; i < (int)assets.size(); i++) {
            if (ImGui::Selectable(assets[i].c_str(), asset_sel == i))
                asset_sel = (asset_sel == i) ? -1 : i;    // solo resalta (colocar = arrastrar)
            // ORIGEN de arrastre: lleva el asset al viewport
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("ASSET_IDX", &i, sizeof(int));
                ImGui::Text(ICON_FA_CUBE " %s", assets[i].c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                open_anim_preview(assets[i]);             // visor de animaciones
        }
        ImGui::EndChild();
        ImGui::End();

        // --- Panel: Entorno (agua / mar / lago) ---
        ImGui::Begin("Entorno");
        ImGui::TextDisabled("Terreno y agua auto: menu 'Terreno' (arriba).");
        ImGui::SeparatorText(ICON_FA_WATER "  Agua (mar global)");
        ImGui::Checkbox("Activar agua (mar/lago)", &water_on);
        ImGui::BeginDisabled(!water_on);
        ImGui::SliderFloat("Nivel", &water_level, -20.0f, 40.0f, "%.1f");
        if (ImGui::TreeNodeEx("Oleaje", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Amplitud", &w_amp, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Longitud", &w_len, 1.0f, 40.0f, "%.1f");
            ImGui::SliderFloat("Velocidad", &w_speed, 0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Marejada (swell)", &w_swell, 0.0f, 3.0f, "%.2f");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Color")) {
            ImGui::ColorEdit3("Profundo", w_deep);
            ImGui::ColorEdit3("Superficie", w_shallow);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Textura")) {
            const char* wcur = (water_tex_sel >= 0 && water_tex_sel < (int)paints.size())
                               ? paints[water_tex_sel].file.c_str() : "(ninguna)";
            if (ImGui::BeginCombo("Textura agua", wcur)) {
                if (ImGui::Selectable("(ninguna)", water_tex_sel < 0)) {
                    water_tex_sel = -1; g3d_editor_water_set_texture(nullptr);
                }
                for (int i = 0; i < (int)paints.size(); i++)
                    if (ImGui::Selectable(paints[i].file.c_str(), water_tex_sel == i)) {
                        water_tex_sel = i; g3d_editor_water_set_texture(paint_tex(i));
                    }
                ImGui::EndCombo();
            }
            ImGui::TreePop();
        }
        ImGui::TextDisabled("Esculpe cuencas bajo el nivel -> se llenan.");
        ImGui::EndDisabled();

        // --- Camara principal del juego ---
        if (ImGui::CollapsingHeader(ICON_FA_VIDEO "  Camara principal", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* modes[] = { "Fija", "Tercera persona", "Primera persona (FPS)", "Cenital (arriba)" };
        ImGui::Combo("Modo", &cam_mode, modes, IM_ARRAYSIZE(modes));
        if (cam_mode == 0) {
            ImGui::DragFloat3("Posicion", cam_pos, 0.5f);
            ImGui::DragFloat3("Mira a", cam_look, 0.5f);
            if (obj_sel >= 0 && ImGui::Button("Poner donde el objeto seleccionado")) {
                cam_look[0]=objects[obj_sel].x; cam_look[1]=objects[obj_sel].y; cam_look[2]=objects[obj_sel].z;
            }
        } else {
            // modos que siguen a un objeto: elegir cual
            const char* fcur = (cam_follow >= 0 && cam_follow < (int)objects.size())
                               ? objects[cam_follow].name.c_str() : "(ninguno)";
            if (ImGui::BeginCombo("Seguir a", fcur)) {
                if (ImGui::Selectable("(ninguno)", cam_follow < 0)) cam_follow = -1;
                for (int i = 0; i < (int)objects.size(); i++)
                    if (ImGui::Selectable(objects[i].name.c_str(), cam_follow == i)) cam_follow = i;
                ImGui::EndCombo();
            }
            if (cam_mode == 3) ImGui::SliderFloat("Altura camara", &gcam_dist, 5.0f, 120.0f, "%.1f");
            else {
                if (cam_mode == 2) {
                    ImGui::SliderFloat("Altura (ojos)", &cam_height, 0.0f, 20.0f, "%.1f");
                    ImGui::SliderFloat("Adelanto", &cam_fwd, 0.0f, 3.0f, "%.2f");
                    ImGui::TextWrapped("El adelanto saca la camara del cuerpo. A 0 se ven las "
                                       "caras interiores del modelo; subelo hasta que solo se "
                                       "vean los brazos y las piernas.");
                    ImGui::SliderFloat("Sensibilidad raton", &cam_sens, 20.0f, 400.0f, "%.0f");
                    ImGui::TextDisabled("Cuanto gira la vista por cada pixel de raton. "
                                        "Mas bajo = mas suave.");
                } else {
                    ImGui::SliderFloat("Distancia", &gcam_dist, 1.0f, 30.0f, "%.1f");
                    ImGui::SliderFloat("Altura", &cam_height, 0.0f, 20.0f, "%.1f");
                }
            }
            if (cam_follow < 0)
                ImGui::TextColored(ImVec4(1,0.7f,0.2f,1), "Elige un objeto a seguir");
        }
        }   // fin CollapsingHeader Camara principal

        ImGui::End();

        // --- Panel: Jerarquia (objetos de la escena) ---
        ImGui::Begin("Jerarquia");
        ImGui::TextDisabled("Objetos: %d", (int)objects.size());
        ImGui::Separator();
        ImGui::BeginChild("lista_obj");
        int pedir_borrar = -1;                 // no se borra dentro del bucle: invalidaria el recorrido
        for (int i = 0; i < (int)objects.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::Selectable(objects[i].name.c_str(), obj_sel == i)) obj_sel = i;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                open_anim_preview(objects[i].asset);
            // Menu contextual: el clic derecho tambien selecciona, para que las
            // acciones sean siempre sobre el objeto en el que se ha pulsado.
            if (ImGui::BeginPopupContextItem("ctx_obj")) {
                obj_sel = i;
                ImGui::TextDisabled("%s", objects[i].name.c_str());
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicar", "Ctrl+D"))  duplicate_obj(i);
                if (ImGui::MenuItem("Copiar",   "Ctrl+C"))  copy_obj(i);
                if (ImGui::MenuItem("Pegar",    "Ctrl+V", false, has_clip)) paste_obj();
                ImGui::Separator();
                if (ImGui::MenuItem("Editar script"))       open_object_script(objects[i].name);
                if (ImGui::MenuItem("Ver animaciones"))     open_anim_preview(objects[i].asset);
                ImGui::Separator();
                if (ImGui::MenuItem("Borrar", "Supr"))      pedir_borrar = i;
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        // clic derecho en el hueco de la lista: solo pegar
        if (ImGui::BeginPopupContextWindow("ctx_vacio", ImGuiPopupFlags_MouseButtonRight |
                                                        ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Pegar", "Ctrl+V", false, has_clip)) paste_obj();
            ImGui::EndPopup();
        }
        ImGui::EndChild();
        if (pedir_borrar >= 0) delete_obj(pedir_borrar);
        ImGui::End();

        // --- Panel: Inspector (del objeto seleccionado / pincel de terreno) ---
        bool borrar_sel = false;   // se borra al cerrar el panel: dentro invalidaria `o`
        ImGui::Begin("Inspector");
        if (tool == T_HOLE) {
            ImGui::SeparatorText("Agujero de terreno");
            ImGui::SliderFloat("Radio", &brush_r, 3.0f, 60.0f, "%.0f");
            ImGui::Checkbox("Rellenar (quitar agujero)", &hole_fill);
            ImGui::TextWrapped("Perfora el terreno para ver la cueva de debajo. "
                               "Excava primero la cueva (modo cueva).");
            ImGui::Separator();
        }
        if (tool == T_RAISE || tool == T_LOWER || tool == T_SMOOTH || tool == T_FLATTEN) {
            const char* tn = tool==T_RAISE?"Subir": tool==T_LOWER?"Bajar":
                             tool==T_SMOOTH?"Suavizar":"Nivelar";
            ImGui::SeparatorText("Pincel de terreno");
            ImGui::Text("Herramienta: %s", tn);
            ImGui::SliderFloat("Radio",  &brush_r,   5.0f, 120.0f, "%.0f");
            ImGui::SliderFloat("Potencia", &brush_str, 0.05f, 3.0f, "%.2f");
            ImGui::TextDisabled("El anillo azul marca la zona.");
            ImGui::TextDisabled("Manten el boton izquierdo y arrastra.");
            ImGui::Separator();
        }
        if (tool == T_ZONE) {
            ImGui::SeparatorText(ICON_FA_DRAW_POLYGON "  Zonas de barrera");
            const char* lnames[] = { "Capa 1 (roja)", "Capa 2 (verde)", "Capa 3 (azul)", "Capa 4 (amarilla)" };
            ImGui::Combo("Capa a pintar", &zone_layer, lnames, IM_ARRAYSIZE(lnames));
            ImGui::SliderFloat("Radio", &brush_r, 3.0f, 120.0f, "%.0f");
            ImGui::Checkbox("Borrar (en vez de pintar)", &zone_erase);
            ImGui::TextWrapped("Pinta zonas por las que ciertos objetos NO pueden pasar. "
                               "Luego, en cada objeto (seccion 'Zonas'), elige que capa le bloquea. "
                               "Ej: el barco bloqueado por la capa del borde del lago; el personaje no.");
            ImGui::TextDisabled("Manten el boton izquierdo y arrastra.");
            ImGui::Separator();
        }
        if (tool == T_LAKE) {
            ImGui::SeparatorText(ICON_FA_DROPLET "  Lago");
            ImGui::TextWrapped("Primero esculpe un hoyo/valle con las herramientas de terreno. "
                               "Luego haz clic dentro del hoyo: el agua lo rellena con su forma.");
            ImGui::Checkbox("Nivel automatico (hasta el borde)", &lake_auto);
            if (!lake_auto)
                ImGui::SliderFloat("Nivel (altura)", &lake_level, -30.0f, 40.0f, "%.1f");
            ImGui::SliderFloat("Profundidad", &lake_depth, 0.5f, 20.0f, "%.1f");
            ImGui::SliderFloat("Radio max", &lake_radius, 0.0f, 200.0f, lake_radius < 1.0f ? "sin limite" : "%.0f");
            ImGui::TextDisabled("Radio max acota el llenado a un circulo: para hoyos ABIERTOS\n"
                                "(al borde de una montana) evita que el agua inunde todo.");
            ImGui::Separator();
            ImGui::Text("Lagos: %d", (int)lakes.size());
            if (!lakes.empty()) {
                if (ImGui::Button("Quitar el ultimo")) { lakes.pop_back(); rebuild_water(); }
                ImGui::SameLine();
                if (ImGui::Button("Quitar todos")) { lakes.clear(); rebuild_water(); }
            }
            ImGui::TextDisabled("Los nuevos lagos toman los efectos del panel Entorno (agua);\n"
                                "aqui abajo cada lago tiene los SUYOS propios.");
            // Efectos propios de cada lago (textura, olas, color)
            for (int i = 0; i < (int)lakes.size(); i++) {
                char hdr[64]; snprintf(hdr, sizeof(hdr), ICON_FA_DROPLET "  Lago %d##lkfx%d", i + 1, i);
                if (ImGui::TreeNode(hdr)) {
                    if (water_fx_editor(lakes[i].fx, 5000 + i)) water_fx_dirty = true;
                    if (ImGui::SmallButton("Quitar este lago")) {
                        lakes.erase(lakes.begin() + i); rebuild_water();
                        ImGui::TreePop(); break;
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::Separator();
        }
        if (tool == T_RIVER) {
            ImGui::SeparatorText(ICON_FA_WATER "  Rio");
            ImGui::TextWrapped("Clic en el terreno para poner los puntos del cauce (del nacimiento "
                               "a la desembocadura). Doble clic para terminar el rio. El agua baja "
                               "siguiendo el relieve; donde cae un desnivel fuerte sale una cascada.");
            ImGui::SliderFloat("Ancho", &river_width, 1.0f, 30.0f, "%.1f");
            ImGui::SliderFloat("Profundidad del cauce", &river_depth, 0.5f, 10.0f, "%.1f");
            ImGui::Separator();
            ImGui::Text("Rios: %d    Trazando: %d puntos", (int)rivers.size(), (int)river_draft.size()/2);
            if (!river_draft.empty() && ImGui::Button("Cancelar el trazo actual")) river_draft.clear();
            if (!rivers.empty()) {
                if (ImGui::Button("Quitar el ultimo rio")) remove_river((int)rivers.size() - 1);
                ImGui::SameLine();
                if (ImGui::Button("Quitar todos##rios")) remove_all_rivers();
            }
            ImGui::TextDisabled("Los nuevos rios toman los efectos del panel Entorno (agua);\n"
                                "aqui abajo cada rio tiene los SUYOS propios.");
            for (int i = 0; i < (int)rivers.size(); i++) {
                char hdr[64]; snprintf(hdr, sizeof(hdr), ICON_FA_WATER "  Rio %d##rvfx%d", i + 1, i);
                if (ImGui::TreeNode(hdr)) {
                    if (water_fx_editor(rivers[i].fx, 6000 + i)) water_fx_dirty = true;
                    if (ImGui::SmallButton("Quitar este rio")) {
                        remove_river(i);
                        ImGui::TreePop(); break;
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::Separator();
        }
        if (tool == T_WATERFALL) {
            ImGui::SeparatorText(ICON_FA_ANGLES_DOWN "  Cascada");
            ImGui::TextWrapped("Haz clic ARRIBA (el borde por donde cae) y luego ABAJO "
                               "(la base o la poza). La cascada cae recta entre esos dos "
                               "puntos; si la base esta en un lago/rio, aterriza en el agua.");
            ImGui::SliderFloat("Ancho", &wf_width, 1.0f, 30.0f, "%.1f");
            ImGui::SliderFloat("Arco (comba)", &wf_arc, 0.0f, 20.0f, "%.1f");
            if (wf_have_top) {
                ImGui::TextColored(ImVec4(1,0.8f,0.2f,1), "Borde puesto: ahora clic en la BASE");
                if (ImGui::Button("Cancelar")) wf_have_top = false;
            }
            ImGui::Separator();
            ImGui::Text("Cascadas: %d", (int)waterfalls.size());
            for (int i = 0; i < (int)waterfalls.size(); i++) {
                char hdr[64]; snprintf(hdr, sizeof(hdr), ICON_FA_ANGLES_DOWN "  Cascada %d##wf%d", i+1, i);
                if (ImGui::TreeNode(hdr)) {
                    Waterfall& w = waterfalls[i];
                    ImGui::PushID(8000 + i);
                    if (ImGui::SliderFloat("Ancho", &w.width, 1.0f, 30.0f, "%.1f")) water_fx_dirty = true;
                    if (ImGui::SliderFloat("Arco (comba)", &w.arc, 0.0f, 20.0f, "%.1f")) water_fx_dirty = true;
                    const char* wc = (w.fx.tex >= 0 && w.fx.tex < (int)paints.size())
                                     ? paints[w.fx.tex].file.c_str() : "(procedural)";
                    if (ImGui::BeginCombo("Textura", wc)) {
                        if (ImGui::Selectable("(procedural)", w.fx.tex < 0)) { w.fx.tex = -1; water_fx_dirty = true; }
                        for (int p = 0; p < (int)paints.size(); p++)
                            if (ImGui::Selectable(paints[p].file.c_str(), w.fx.tex == p)) { w.fx.tex = p; water_fx_dirty = true; }
                        ImGui::EndCombo();
                    }
                    if (ImGui::SliderFloat("Velocidad", &w.fx.speed, 0.2f, 4.0f, "%.2f")) water_fx_dirty = true;
                    if (ImGui::SliderFloat("Espuma",    &w.fx.foam,  0.0f, 2.0f, "%.2f")) water_fx_dirty = true;
                    if (ImGui::ColorEdit3("Color",      w.fx.color)) water_fx_dirty = true;
                    ImGui::PopID();
                    if (ImGui::SmallButton("Quitar esta cascada")) {
                        waterfalls.erase(waterfalls.begin() + i); rebuild_water();
                        ImGui::TreePop(); break;
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::Separator();
        }
        if (tool == T_WATERSOURCE) {
            ImGui::SeparatorText(ICON_FA_FAUCET_DRIP "  Agua que fluye (simulacion)");
            ImGui::TextWrapped("Pon un MANANTIAL con clic: el agua fluye cuesta abajo sola, "
                               "formando rios en los cauces, charcos en los hoyos y cascadas en "
                               "los precipicios, con fisica. Clic derecho quita el mas cercano.");
            if (ImGui::SliderFloat("Caudal fuente", &ws_rate, 0.5f, 30.0f, "%.1f")) {}
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Potencia del manantial: mas = llena mas rapido y hace mas rio.");
            if (ImGui::SliderFloat("Evaporacion", &ws_evap, 0.0f, 0.4f, "%.2f")) { if (g3d_watersim_active()) watersim_sync(true); }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = el agua se QUEDA (llena y no se seca). Mas = rios finos / charcos que se secan.");
            if (ImGui::SliderFloat("Velocidad flujo", &ws_flow, 0.3f, 3.0f, "%.1f")) { if (g3d_watersim_active()) watersim_sync(false); }
            ImGui::Separator();
            ImGui::Text("Manantiales: %d", (int)wsources.size());
            if (!wsources.empty() && ImGui::Button("Quitar todos los manantiales")) {
                wsources.clear(); g3d_watersim_clear_sources(); g3d_watersim_shutdown();
                status = "Simulacion de agua vaciada";
            }
            ImGui::TextDisabled("El mar global y los lagos siguen igual (panel Entorno / herramienta Lago).");
            ImGui::Separator();
        }
        // Reconstruye el agua UNA vez cuando se suelta el raton tras editar efectos,
        // para no rehacer las mallas en cada frame del arrastre.
        if (water_fx_dirty && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            rebuild_water(); water_fx_dirty = false;
        }
        if (tool == T_PAINT) {
            ImGui::SeparatorText("Pintar terreno");
            if (paints.empty()) {
                ImGui::TextWrapped("No hay texturas en el proyecto/Assets "
                                   "(.png/.jpg). Anade alguna.");
            } else {
                const char* cur = (paint_sel >= 0 && paint_sel < (int)paints.size())
                                  ? paints[paint_sel].file.c_str() : "(elige textura)";
                if (ImGui::BeginCombo("Textura", cur)) {
                    for (int i = 0; i < (int)paints.size(); i++) {
                        bool sel = (paint_sel == i);
                        if (ImGui::Selectable(paints[i].file.c_str(), sel)) paint_sel = i;
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::SliderFloat("Radio", &brush_r, 5.0f, 120.0f, "%.0f");
                ImGui::SliderFloat("Opacidad", &paint_op, 0.05f, 1.0f, "%.2f");
                ImGui::SliderFloat("Tiling", &paint_tiling, 5.0f, 100.0f, "%.0f");
                ImGui::TextDisabled("Manten el boton izquierdo y arrastra.");
            }
            ImGui::Separator();
        }
        if (obj_sel >= 0 && obj_sel < (int)objects.size()) {
            SObj& o = objects[obj_sel];
            ImGui::Text("%s", o.name.c_str());
            ImGui::TextDisabled("%s", o.asset.c_str());
            ImGui::Spacing();

            // Secciones plegables (clic en la cabecera para abrir/cerrar) -> cabe todo.
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Posicion", &o.x, 0.1f);
                ImGui::SliderAngle("Rotacion Y", &o.ry);
                ImGui::DragFloat("Escala", &o.scale, 0.01f, 0.02f, 100.0f);
            }
            if (ImGui::CollapsingHeader("Script (componente)")) {
                ImGui::TextDisabled("Scripts/%s.prg", o.name.c_str());
                if (ImGui::Button("Editar script del objeto", ImVec2(-1, 0)))
                    open_object_script(o.name);
                // El comportamiento (controles, cuerpo fisico) vive en el script, y
                // este solo se crea la primera vez. Si cambias masa, densidad,
                // velocidad, etc. en el Inspector, hay que rehacerlo para que se apliquen.
                bool con_plantilla = o.is_player || (o.phys >= 0 && o.phys <= 4);
                if (con_plantilla) {
                    if (ImGui::Button("Regenerar script desde los valores de arriba", ImVec2(-1, 0))) {
                        regen_obj = o.name; ask_regen = true;
                    }
                    if (script_untouched(o.name) || script_read(o.name).empty())
                        ImGui::TextDisabled("Lo mantiene el editor: se rehace solo al generar el juego.");
                    else
                        ImGui::TextWrapped("Este script lo has editado tu, asi que el editor ya no lo toca. "
                                           "Los valores de arriba NO llegan al juego hasta que regeneres "
                                           "(y entonces pierdes tus cambios).");
                }
            }
            if (ImGui::CollapsingHeader(ICON_FA_CUBES "  Fisica (Jolt)")) {
                const char* ptypes[] = { "Ninguna (decorativo)", "Caja", "Esfera", "Capsula",
                                         "Cilindro", "Muro invisible (colision)" };
                ImGui::Combo("Cuerpo", &o.phys, ptypes, IM_ARRAYSIZE(ptypes));
                if (o.phys >= 1 && o.phys <= 4) {         // cuerpo dinamico
                    ImGui::DragFloat("Masa / peso", &o.mass, 0.1f, 0.0f, 1000.0f, "%.2f kg");
                    ImGui::DragFloat("Tamano colision", &o.csize, 0.05f, 0.1f, 50.0f, "%.2f");
                    if (o.mass <= 0.0f) {
                        // Decorado solido: barcos, rocas, cajones que no se empujan.
                        ImGui::TextColored(ImVec4(0.4f,0.9f,0.4f,1), "FIJO: choca con todo pero nada lo mueve");
                        ImGui::TextWrapped("Los objetos fisicos chocan contra el y el jugador no puede "
                                           "atravesarlo. Para decorado solido sin poner muros a mano.");
                    } else {
                        ImGui::TextDisabled("Cuanto mas pesado, menos lo empujaran el jugador y los demas.");
                        ImGui::SliderFloat("Rebote", &o.bounce, 0.0f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Friccion", &o.friction, 0.0f, 1.0f, "%.2f");
                        ImGui::Checkbox("Flota en el agua", (bool*)&o.buoyant);
                        if (o.buoyant) {
                            ImGui::SliderFloat("Densidad", &o.density, 0.05f, 1.0f, "%.2f");
                            ImGui::TextDisabled("0.05 = corcho (flota alto)  ->  1 = casi se hunde");
                        }
                    }
                } else if (o.phys == 5) {                 // muro invisible
                    ImGui::DragFloat("Tamano (medio X/Z)", &o.csize, 0.05f, 0.1f, 200.0f, "%.2f");
                    ImGui::TextWrapped("Muro alto e invisible: bloquea el paso. Coloca varios "
                                       "para cerrar los bordes del lago, hacer vallas, limites...");
                } else {
                    ImGui::TextDisabled("Sin colision: los objetos y el jugador lo atraviesan.");
                }
            }
            if (ImGui::CollapsingHeader(ICON_FA_PERSON_RUNNING "  Jugador")) {
                ImGui::Checkbox("Es el jugador (controlable)", (bool*)&o.is_player);
                if (o.is_player) {
                    ImGui::SliderFloat("Vel. andar", &o.walk_speed, 1.0f, 40.0f, "%.1f");
                    ImGui::SliderFloat("Vel. correr", &o.run_speed, 1.0f, 60.0f, "%.1f");
                    ImGui::SliderFloat("Fuerza salto", &o.jump_force, 0.0f, 30.0f, "%.1f");
                    void* mm = load_model(o.asset);
                    int nan = mm ? g3d_model_animation_count(mm) : 0;
                    ImGui::Text("Animaciones del modelo: %d", nan);
                    ImGui::InputInt("Anim reposo", &o.anim_idle);
                    ImGui::InputInt("Anim andar",  &o.anim_walk);
                    ImGui::InputInt("Anim correr", &o.anim_run);
                    ImGui::InputInt("Anim saltar", &o.anim_jump);
                    ImGui::SliderFloat("Radio colision", &o.char_radius, 0.2f, 4.0f, "%.2f");
                    ImGui::SliderFloat("Altura / calado", &o.char_height, 0.3f, 6.0f, "%.2f");
                    ImGui::Checkbox("Flota / nada en agua", (bool*)&o.buoyant);
                    if (o.buoyant) {
                        ImGui::InputInt("Anim nadar", &o.anim_swim);
                        ImGui::TextDisabled("Flota con la 'cabeza' (altura) en la superficie.");
                        ImGui::TextDisabled("-> baja la Altura si se hunde demasiado (barcos).");
                    }
                    ImGui::TextDisabled("WASD mover, SHIFT correr, ESPACIO saltar");
                }
            }
            if (ImGui::CollapsingHeader(ICON_FA_HAND "  Enganchar a hueso")) {
                ImGui::TextWrapped("Engancha ESTE objeto ('%s') a un hueso de otro. "
                                   "Para un arma en la mano: selecciona el ARMA y en 'Pegar a' "
                                   "pon el personaje.", o.name.c_str());
                if (o.is_player)
                    ImGui::TextColored(ImVec4(1,0.7f,0.2f,1),
                        "Tienes seleccionado el JUGADOR. Normalmente el jugador es el PADRE: "
                        "para ponerle algo en la mano, selecciona el arma/antorcha, no el jugador.");
                ImGui::Spacing();
                const char* pcur = (o.attach_to >= 0 && o.attach_to < (int)objects.size())
                                   ? objects[o.attach_to].name.c_str() : "(ninguno)";
                if (ImGui::BeginCombo("Pegar a", pcur)) {
                    if (ImGui::Selectable("(ninguno)", o.attach_to < 0)) o.attach_to = -1;
                    for (int i = 0; i < (int)objects.size(); i++)
                        if (i != obj_sel && ImGui::Selectable(objects[i].name.c_str(), o.attach_to == i))
                            o.attach_to = i;
                    ImGui::EndCombo();
                }
                if (o.attach_to >= 0) {
                    // Lista de huesos del modelo PADRE, para no tener que adivinarlos.
                    void* pm = load_model(objects[o.attach_to].asset);
                    int nb = pm ? g3d_model_node_count(pm) : 0;
                    if (nb > 0) {
                        const char* cur = o.attach_bone.empty() ? "(elige un hueso)" : o.attach_bone.c_str();
                        if (ImGui::BeginCombo("Hueso", cur)) {
                            for (int b = 0; b < nb; b++) {
                                const char* bn = g3d_model_node_name(pm, b);
                                if (bn && bn[0] && ImGui::Selectable(bn, o.attach_bone == bn))
                                    o.attach_bone = bn;
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::TextDisabled("%d huesos en '%s'. La mano suele ser Hand/Mano_R.",
                                            nb, objects[o.attach_to].asset.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(1,0.7f,0.2f,1),
                            "'%s' no tiene huesos (es un prop, no un personaje).",
                            objects[o.attach_to].name.c_str());
                        ImGui::TextWrapped("Puede que lo tengas al reves: en 'Pegar a' va el "
                                           "PERSONAJE (con esqueleto), no un barril ni una antorcha.");
                    }
                    // Y por si prefieres escribirlo (subcadena del nombre):
                    char bonebuf[256]; strncpy(bonebuf, o.attach_bone.c_str(), 255); bonebuf[255] = 0;
                    if (ImGui::InputText("Hueso (texto)", bonebuf, sizeof(bonebuf))) o.attach_bone = bonebuf;
                    ImGui::DragFloat3("Offset", o.att_off, 0.01f);
                    ImGui::DragFloat("Escala", &o.att_scale, 0.01f, 0.01f, 20.0f);
                    ImGui::DragFloat("Giro", &o.att_yaw, 1.0f, -180.0f, 180.0f, "%.0f");
                    ImGui::TextDisabled("El padre debe ser el jugador (sigue su animacion).");
                }
            }
            if (ImGui::CollapsingHeader(ICON_FA_DRAW_POLYGON "  Zonas (barreras)")) {
                const char* zl[] = { "Ninguna (pasa por todo)", "Capa 1 (roja)", "Capa 2 (verde)",
                                     "Capa 3 (azul)", "Capa 4 (amarilla)" };
                int sel = o.zone_layer + 1;   // -1..3 -> 0..4
                if (ImGui::Combo("Bloqueado por", &sel, zl, IM_ARRAYSIZE(zl))) o.zone_layer = sel - 1;
                ImGui::TextDisabled("No podra entrar en las zonas de esa capa (solo el jugador por ahora).");
            }

            ImGui::Spacing();
            if (ImGui::Button("Duplicar", ImVec2(-1, 0))) duplicate_obj(obj_sel);
            if (ImGui::Button("Borrar objeto", ImVec2(-1, 0))) borrar_sel = true;
        } else {
            ImGui::TextDisabled("Nada seleccionado.");
            ImGui::TextWrapped("Elige un asset y haz clic en la escena para colocar. "
                               "Sin asset armado, clic selecciona el objeto mas cercano.");
        }
        ImGui::SeparatorText("Camara");
        ImGui::SliderFloat("Distancia", &cam_dist, 5.0f, 60.0f);

        ImGui::SeparatorText("Sombras");
        // Calidad del shadow map. Se aplica al viewport al momento para verlo.
        int sres_idx = (shadow_res >= 4096) ? 2 : (shadow_res >= 2048) ? 1 : 0;
        const char* sres_lbl[] = { "Baja (1024)", "Media (2048)", "Alta (4096)" };
        if (ImGui::Combo("Calidad", &sres_idx, sres_lbl, 3)) {
            shadow_res = (sres_idx == 2) ? 4096 : (sres_idx == 1) ? 2048 : 1024;
            g3d_renderer_set_shadow_resolution((unsigned)shadow_res);
        }
        ImGui::TextDisabled("Mas alta = bordes mas nitidos, algo mas de video.");
        ImGui::End();
        if (borrar_sel) delete_obj(obj_sel);

        // --- Confirmacion antes de rehacer main.prg ---
        // Es codigo del usuario: rehacerlo lo borra entero, asi que no puede pasar
        // por un clic despistado.
        if (ask_regen_main) { ImGui::OpenPopup("Rehacer main.prg"); ask_regen_main = false; }
        if (ImGui::BeginPopupModal("Rehacer main.prg", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("main.prg volvera a quedar como recien creado.\n"
                               "SE PERDERA todo lo que hayas escrito en el.");
            ImGui::Spacing();
            ImGui::TextDisabled("El bloque marcado del editor (objetos + escenario) se rehace solo al generar.");
            ImGui::Spacing();
            if (ImGui::Button("Rehacer", ImVec2(140, 0))) {
                if (write_main_prg()) {
                    status = "main.prg rehecho";
                    console_add(status + "\n");
                    if (show_script && script_obj.empty()) open_main_script();   // refrescar si estaba abierto
                } else {
                    status = "ERROR: no puedo escribir main.prg";
                    console_add(status + "\n");
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(140, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- Confirmacion antes de regenerar un script ---
        // Regenerar SOBREESCRIBE el fichero, asi que no puede ocurrir por un clic
        // despistado: hay que confirmarlo viendo el nombre del objeto.
        if (ask_regen) { ImGui::OpenPopup("Regenerar script"); ask_regen = false; }
        if (ImGui::BeginPopupModal("Regenerar script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Se va a rehacer  Scripts/%s.prg", regen_obj.c_str());
            ImGui::Spacing();
            ImGui::TextWrapped("Volvera a escribirse con los valores actuales del Inspector.\n"
                               "SE PERDERA cualquier cambio que hayas hecho a mano en ese script.");
            ImGui::Spacing();
            if (ImGui::Button("Regenerar", ImVec2(140, 0))) {
                const SObj* po = nullptr;
                for (auto& o : objects) if (o.name == regen_obj) { po = &o; break; }
                if (po) {
                    if (script_write_generated(*po)) {
                        status = "Script regenerado: " + regen_obj + ".prg";
                        console_add(status + "\n");
                        // Si estaba abierto en el editor, recargarlo para no dejar
                        // a la vista una version que ya no es la del disco.
                        if (show_script && script_obj == regen_obj) open_object_script(regen_obj);
                    } else {
                        status = "ERROR: no puedo escribir Scripts/" + regen_obj + ".prg";
                        console_add(status + "\n");
                    }
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(140, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- Editor de SCRIPT a pantalla completa (se abre desde el Inspector) ---
        static std::string compile_out;
        if (show_script) {
            ImGuiViewport* mv = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(mv->WorkPos);
            ImGui::SetNextWindowSize(mv->WorkSize);
            if (focus_script) { ImGui::SetNextWindowFocus(); focus_script = false; }
            ImGui::Begin("Editor de script", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoDocking);

            const std::string& obj_script = script_file;
            ImGui::TextUnformatted(script_title.c_str());
            ImGui::SameLine(0, 30);
            if (ImGui::Button("Guardar")) {
                FILE* f = fopen(obj_script.c_str(), "w");
                if (f) { std::string t = script.GetText(); fwrite(t.data(), 1, t.size(), f); fclose(f);
                         compile_out = "Guardado: " + script_file; }
                else     compile_out = "ERROR: no puedo escribir " + script_file;
            }
            ImGui::SameLine();
            if (ImGui::Button("Compilar")) {
                // guarda el componente del objeto...
                { FILE* f = fopen(obj_script.c_str(), "w");
                  if (f) { std::string t = script.GetText(); fwrite(t.data(), 1, t.size(), f); fclose(f); } }
                // ...y compila un main temporal que lo incluye en contexto de juego.
                std::string tmp = scripts_dir + "/__compile_check.prg";
                FILE* f = fopen(tmp.c_str(), "w");
                if (f) {
                    fputs("import \"libmod_gfx\";\nimport \"libmod_misc\";\n"
                          "import \"libmod_input\";\nimport \"libmod_3d\";\n\n", f);
                    std::string t = script.GetText();
                    fwrite(t.data(), 1, t.size(), f);
                    fputs("\nPROCESS main()\nBEGIN\nEND\n", f);
                    fclose(f);
                }
                compile_out.clear();
                std::string cmd = ruta_util("lib/bgdc", BGDC_PATH) + " " + tmp + " 2>&1";
                FILE* p = popen(cmd.c_str(), "r");
                if (p) { char buf[512]; size_t n;
                         while ((n = fread(buf, 1, sizeof(buf) - 1, p)) > 0) { buf[n] = 0; compile_out += buf; }
                         int rc = pclose(p);
                         compile_out += (rc == 0) ? "\n[OK] el componente compila en el contexto del juego."
                                                  : "\n[FALLO] revisa los errores.";
                } else compile_out = "ERROR: no pude ejecutar bgdc";
            }
            ImGui::SameLine();
            { auto c = script.GetCursorPosition();
              ImGui::Text("| Ln %d, Col %d", c.mLine + 1, c.mColumn + 1); }
            ImGui::SameLine(ImGui::GetWindowWidth() - 110);
            if (ImGui::Button("Cerrar", ImVec2(90, 0))) show_script = false;

            float out_h = compile_out.empty() ? 0.0f : 150.0f;
            script.Render("CodeEditor", ImVec2(0, -out_h));
            if (out_h > 0.0f) {
                ImGui::Separator();
                ImGui::BeginChild("salida", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(compile_out.c_str());
                ImGui::EndChild();
            }
            ImGui::End();
        }

        // --- VISOR DE ANIMACIONES (doble clic en un asset/objeto) ---
        if (show_anim && anim_model) {
            ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
            ImGui::Begin(("Animaciones - " + anim_asset).c_str(), &show_anim);
            int nc = g3d_model_animation_count(anim_model);
            if (nc <= 0) {
                ImGui::TextColored(ImVec4(1,0.7f,0.2f,1), "Este modelo no tiene animaciones.");
            }
            ImGui::Text("%d animaciones", nc);
            ImVec2 av = ImGui::GetContentRegionAvail();
            float img_h = av.y * 0.55f; if (img_h < 140.0f) img_h = 140.0f;
            ImVec2 isz(av.x > 16 ? av.x : 16, img_h);
            prevFbo.resize((int)isz.x, (int)isz.y);
            ImGui::Image((ImTextureID)(intptr_t)prevFbo.tex, isz, ImVec2(0,0), ImVec2(1,1));
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    pv_yaw   += ImGui::GetIO().MouseDelta.x * 0.01f;
                    pv_pitch += ImGui::GetIO().MouseDelta.y * 0.01f;
                }
                float w = ImGui::GetIO().MouseWheel;
                if (w != 0.0f) { pv_dist *= (1.0f - w * 0.12f); if (pv_dist < 0.2f) pv_dist = 0.2f; }
            }
            if (pv_pitch >  1.4f) pv_pitch = 1.4f;
            if (pv_pitch < -1.4f) pv_pitch = -1.4f;
            ImGui::TextDisabled("Arrastra = girar,  rueda = zoom.  Clic en una animacion para verla.");
            ImGui::Separator();
            ImGui::BeginChild("anims_list");
            for (int i = 0; i < nc; i++) {
                const char* nm = g3d_model_animation_name(anim_model, i);
                float d = g3d_model_animation_duration(anim_model, i);
                char lbl[192];
                snprintf(lbl, sizeof(lbl), "[%d]  %s   (%.2fs)", i,
                         (nm && nm[0]) ? nm : "(sin nombre)", d);
                if (ImGui::Selectable(lbl, anim_sel == i)) { anim_sel = i; anim_t0 = SDL_GetTicks(); }
            }
            ImGui::EndChild();
            ImGui::End();
        }

        ImGui::Render();

        // aplicar transformaciones de los objetos colocados (en Play manda el emulador)
        if (!playing)
            for (auto& o : objects) {
                g3d_entity_impl_set_position(o.entity, o.x, o.y, o.z);
                g3d_entity_impl_set_rotation(o.entity, 0.0f, o.ry, 0.0f);
                g3d_entity_impl_set_scale(o.entity, o.scale, o.scale, o.scale);
            }

        // ---- agua (mar/lago global) ----
        g3d_editor_water_update(water_on ? 1 : 0, water_level, 4000.0f,
                                w_amp, w_len, w_speed, w_swell,
                                w_deep[0], w_deep[1], w_deep[2],
                                w_shallow[0], w_shallow[1], w_shallow[2]);

        // ---- animar los modelos con esqueleto (si no, salen colapsados) ----
        // Todos comparten pose por modelo; para la vista previa del editor basta.
        {
            float atime = (float)SDL_GetTicks() / 1000.0f;
            for (auto& kv : model_cache)
                if (kv.second && g3d_model_animation_count(kv.second) > 0) {
                    // el modelo del visor lo anima el visor (con su clip elegido)
                    if (show_anim && kv.second == anim_model) continue;
                    // el modelo del jugador lo anima el emulador durante el Play
                    if (playing && kv.second == sim_player_model) continue;
                    if (!g3d_model_is_skinned(kv.second)) {
                        // Modelo SIN esqueleto. No hay que reproducirle la animacion
                        // cada frame (le mueve y escala las piezas y acaba deforme o
                        // bajo el suelo), pero sus submallas atadas a nodos animados
                        // se colocan con node_global, que SOLO calcula la animacion:
                        // sin ella no se dibujan. Asi que se posa UNA vez en t=0 y
                        // se deja quieto.
                        if (posed_static.insert(kv.second).second)
                            g3d_model_animate_all(kv.second, 0.0f, 0);
                        continue;
                    }
                    g3d_model_animate_all(kv.second, atime, 1);
                }
        }

        // ---- VISOR: render de la escena de preview a su FBO (clip elegido) ----
        if (show_anim && anim_model && prev_scene >= 0 && prevFbo.w > 0) {
            float at = (float)(SDL_GetTicks() - anim_t0) / 1000.0f;
            g3d_model_animate(anim_model, anim_sel, at, 1);
            float cx = pv_cx + sinf(pv_yaw) * cosf(pv_pitch) * pv_dist;
            float cy = pv_cy + sinf(pv_pitch) * pv_dist;
            float cz = pv_cz + cosf(pv_yaw) * cosf(pv_pitch) * pv_dist;
            g3d_scene_set_active(prev_scene);
            g3d_camera_set_active(prev_cam);
            g3d_camera_set_position(prev_cam, cx, cy, cz);
            g3d_camera_look_at(prev_cam, pv_cx, pv_cy, pv_cz, 0.0f, 1.0f, 0.0f);
            g3d_editor_set_aspect(prevFbo.h > 0 ? (float)prevFbo.w / (float)prevFbo.h : 1.0f);
            g3d_renderer_set_target(prevFbo.fbo);
            g3d_renderer_set_viewport_physical(0, 0, (unsigned)prevFbo.w, (unsigned)prevFbo.h);
            glBindFramebuffer(GL_FRAMEBUFFER, prevFbo.fbo);
            glViewport(0, 0, prevFbo.w, prevFbo.h);
            g3d_renderer_render();
            g3d_scene_set_active(scene);          // restaurar la escena/camara principal
            g3d_camera_set_active(cam);
        }

        // ---- render del MOTOR a la textura del viewport ----
        g3d_editor_set_aspect(vp.h > 0 ? (float)vp.w / (float)vp.h : 1.777f);
        g3d_renderer_set_target(vp.fbo);
        g3d_renderer_set_viewport_physical(0, 0, (unsigned)vp.w, (unsigned)vp.h);
        glBindFramebuffer(GL_FRAMEBUFFER, vp.fbo);
        glViewport(0, 0, vp.w, vp.h);
        g3d_renderer_render();

        // ---- componer la UI en la ventana real ----
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
