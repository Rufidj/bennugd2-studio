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
    std::string project_dir = PROJECT_DIR;   // carpeta raiz del proyecto
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
    io.Fonts->AddFontFromFileTTF(FONT_DIR "/" FONT_ICON_FILE_NAME_FAS, 15.0f, &fa_cfg, fa_range);

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
                T_HOLE, T_ZONE };
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
        o.entity = g3d_model_spawn(scene, m, o.x, o.y, o.z, o.ry, 0.0f);
        if (o.entity < 0) return -1;
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


    // carga el script de un objeto en el editor (o una plantilla si no existe)
    // ---------------------------------------------------------------------------
    // PLANTILLA DEL SCRIPT DE UN OBJETO
    // El comportamiento de un objeto vive en SU script, no escondido en el main:
    // controles si es el jugador, cuerpo rigido si tiene fisica. El editor solo la
    // escribe la primera vez; a partir de ahi el fichero es tuyo y no se toca.
    // ---------------------------------------------------------------------------
    auto object_script_template = [&](const SObj& o) -> std::string {
        char b[4096];

        // ---------------- JUGADOR ----------------
        if (o.is_player) {
            std::string s =
                "// ===== JUGADOR '" + o.name + "' =====\n"
                "// Creado por el editor con los controles dentro: a partir de aqui es TUYO.\n"
                "// Cambia las teclas, la velocidad o anade lo que quieras (doble salto, dash...).\n"
                "//   id     = la entidad de este objeto en la escena\n"
                "//   modelo = su modelo, necesario para las animaciones\n"
                "PROCESS " + o.name + "(int id, int modelo)\n"
                "PRIVATE\n"
                "    int ch; int gnd; int inw; int moja;\n"
                "    float wx; float wz; float wl; float spd; float t; float facing;\n"
                "    float px; float py; float pz; float prevx; float prevz; float dt; float ript;\n"
                "END\n"
                "BEGIN\n"
                "    dt = 1.0 / 60.0;\n";
            snprintf(b, sizeof(b),
                "    // capsula de colision del personaje (x, y, z, radio, altura)\n"
                "    ch = g3d_char_create(%.3f, %.3f, %.3f, %.3f, %.3f);\n"
                "    g3d_char_set_tuning(ch, 0.8, 46.0);\n"
                "    // Fuerza con la que aparta barriles, cajas y demas cuerpos fisicos.\n"
                "    // Cuanto mas pesado sea el objeto menos se movera, y si no puede con\n"
                "    // el le cortara el paso. 0 = choca pero no mueve nada.\n"
                "    g3d_char_set_push(ch, 200.0);\n"
                "    facing = 0.0; t = 0.0;\n\n"
                "    LOOP\n"
                "        prevx = g3d_char_x(ch); prevz = g3d_char_z(ch);\n\n"
                "        // ---------- CONTROLES ----------\n"
                "        wx = 0.0; wz = 0.0;\n"
                "        IF (key(_W)) wz = wz + 1.0; END\n"
                "        IF (key(_S)) wz = wz - 1.0; END\n"
                "        IF (key(_D)) wx = wx + 1.0; END\n"
                "        IF (key(_A)) wx = wx - 1.0; END\n"
                "        spd = %.3f;\n"
                "        IF (key(_L_SHIFT) OR key(_R_SHIFT)) spd = %.3f; END\n\n"
                "        wl = sqrt(wx * wx + wz * wz);\n"
                "        IF (wl > 0.001)\n"
                "            wx = wx / wl * spd; wz = wz / wl * spd;\n"
                "            facing = atan2(wx, wz);       // mira hacia donde anda\n"
                "        END\n"
                "        g3d_char_move(ch, wx, wz);\n"
                "        IF (key(_SPACE)) g3d_char_jump(ch, %.3f); END\n"
                "        g3d_char_update(ch, dt);\n\n"
                "        px = g3d_char_x(ch); py = g3d_char_y(ch); pz = g3d_char_z(ch);\n",
                o.x, o.y, o.z, o.char_radius, o.char_height, o.walk_speed, o.run_speed, o.jump_force);
            s += b;

            if (water_on && o.buoyant) {
                snprintf(b, sizeof(b),
                    "\n        // ---------- AGUA: flota y nada ----------\n"
                    "        inw = 0;\n"
                    "        IF (py < %.3f) inw = 1; END\n"
                    "        g3d_char_set_water(ch, inw, %.3f);\n",
                    water_level - 1.2f, water_level);
                s += b;
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
            if (water_on) {   // ondas al andar/nadar por el agua + chapuzon al entrar
                snprintf(b, sizeof(b),
                    "\n        // ---------- ONDAS EN EL AGUA ----------\n"
                    "        IF (py < %.3f)\n"
                    "            IF (moja == 0) g3d_water_splash(px, %.3f, pz, 1.0); END   // chapuzon al entrar\n"
                    "            moja = 1;\n"
                    "            ript = ript + dt;\n"
                    "            IF (ript > 0.12 AND wl > 0.001)\n"
                    "                g3d_water_ripple(px, pz, 0.6);   // estela mientras se mueve\n"
                    "                ript = 0.0;\n"
                    "            END\n"
                    "        ELSE\n"
                    "            moja = 0;\n"
                    "        END\n", water_level, water_level);
                s += b;
            }

            s += "\n        g3d_entity_set_position(id, px, py, pz);\n"
                 "        g3d_entity_set_rotation(id, 0.0, facing, 0.0);\n";

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
                    (water_on && o.buoyant && o.anim_swim >= 0) ? "        IF (inw == 0)\n" : "",
                    o.anim_jump, o.anim_run, o.anim_walk, o.anim_idle,
                    (water_on && o.buoyant && o.anim_swim >= 0) ? "        END\n" : "");
                s += b;
                if (water_on && o.buoyant && o.anim_swim >= 0) {
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
            std::string s =
                "// ===== OBJETO CON FISICA '" + o.name + "' =====\n"
                "// Creado por el editor con su cuerpo rigido dentro: a partir de aqui es TUYO.\n"
                "// El motor avanza el mundo fisico una vez por frame; aqui solo se crea el\n"
                "// cuerpo y se coloca el modelo donde la fisica lo haya llevado.\n"
                "PROCESS " + o.name + "(int id)\n"
                "PRIVATE\n"
                "    int cuerpo; int moja; float bx; float by; float bz; float mov;\n"
                "    float prevx; float prevy; float prevz; float dt; float ript;\n"
                "END\n"
                "BEGIN\n"
                "    dt = 1.0 / 60.0;\n";
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
            if (water_on && o.buoyant && o.mass > 0.0f) {
                // Flotacion real: el motor reparte el empuje por el volumen sumergido,
                // no lo aplica en el centro. De ahi salen solas la profundidad a la que
                // queda, que se enderece, y que vuelque si la forma es inestable o el
                // golpe es bastante fuerte. Nada de eso hay que programarlo aqui.
                snprintf(b, sizeof(b),
                         "    g3d_rigidbody_set_buoyancy(cuerpo, %.3f, %.3f);   // nivel del agua, densidad\n",
                         water_level, o.density > 0.05f ? o.density : 0.05f);
                s += b;
            }
            s += "\n    LOOP\n";

            // Agua: el chapuzon y la estela son por CONTACTO, flote o no. Un barril
            // que rueda al agua y se hunde tambien salpica. La flotacion es aparte.
            // Un cuerpo fijo no se mueve, asi que nada de esto le aplica.
            if (water_on && o.mass > 0.0f) {
                snprintf(b, sizeof(b),
                    "        // ---------- CONTACTO CON EL AGUA ----------\n"
                    "        bx = g3d_rigidbody_x(cuerpo); by = g3d_rigidbody_y(cuerpo);\n"
                    "        bz = g3d_rigidbody_z(cuerpo);\n"
                    "        IF (by - %.3f < %.3f)\n"
                    "            IF (moja == 0)\n"
                    "                g3d_water_splash(bx, %.3f, bz, 1.0);   // chapuzon al entrar\n"
                    "                // El agua frena. Sin esto, un objeto que flota conserva su\n"
                    "                // velocidad para siempre y cruza el lago patinando como una tabla.\n"
                    "                g3d_rigidbody_set_damping(cuerpo, 2.5, 3.0);\n"
                    "            END\n"
                    "            moja = 1;\n"
                    "            // Solo agita el agua mientras SE MUEVE y esta cerca de la\n"
                    "            // superficie: si se hunde y se queda quieto en el fondo, las\n"
                    "            // ondas paran. Si flota, el oleaje lo mece y siguen.\n"
                    "            mov = (bx-prevx)*(bx-prevx) + (by-prevy)*(by-prevy) + (bz-prevz)*(bz-prevz);\n"
                    "            IF (mov > 0.0004 AND by + %.3f > %.3f)\n"
                    "                ript = ript + dt;\n"
                    "                IF (ript > 0.15)\n"
                    "                    g3d_water_ripple(bx, bz, 0.4);\n"
                    "                    ript = 0.0;\n"
                    "                END\n"
                    "            END\n",
                    c, water_level, water_level, c, water_level - 1.0f);
                s += b;

                s += "        ELSE\n"
                     "            // al salir del agua se recupera la resistencia del aire\n"
                     "            IF (moja == 1) g3d_rigidbody_set_damping(cuerpo, 0.05, 0.05); END\n"
                     "            moja = 0;\n        END\n"
                     "        prevx = bx; prevy = by; prevz = bz;\n\n";
            }
            s += "        // colocar el modelo donde lo haya llevado la fisica\n"
                 "        g3d_entity_set_position(id, g3d_rigidbody_render_x(cuerpo),\n"
                 "                                    g3d_rigidbody_render_y(cuerpo),\n"
                 "                                    g3d_rigidbody_render_z(cuerpo));\n"
                 "        g3d_entity_set_rotation(id, g3d_rigidbody_angle_x(cuerpo),\n"
                 "                                    g3d_rigidbody_angle_y(cuerpo),\n"
                 "                                    g3d_rigidbody_angle_z(cuerpo));\n"
                 "        FRAME;\n    END\nEND\n";
            return s;
        }

        // ---------------- OBJETO NORMAL ----------------
        return "// Componente del objeto '" + o.name + "'.\n"
               "// Es un PROCESS BennuGD2 que el juego instancia sobre el objeto.\n"
               "PROCESS " + o.name + "(int id)\n"
               "BEGIN\n"
               "    LOOP\n"
               "        // ... logica del objeto ...\n"
               "        FRAME;\n"
               "    END\n"
               "END\n";
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
        fprintf(f, "CAMERA %d %d %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f\n",
                cam_mode, cam_follow, cam_pos[0], cam_pos[1], cam_pos[2],
                cam_look[0], cam_look[1], cam_look[2], gcam_dist, cam_height);
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
        for (auto& o : objects) g3d_entity_impl_set_position(o.entity, 0, -99999, 0);
        objects.clear(); obj_sel = -1;
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
            int cm, cfol; float px,py,pz, lx,ly,lz, cd, ch;
            if (sscanf(line, "CAMERA %d %d %f %f %f %f %f %f %f %f",
                       &cm, &cfol, &px,&py,&pz, &lx,&ly,&lz, &cd, &ch) == 10) {
                cam_mode = cm; cam_follow = cfol;
                cam_pos[0]=px; cam_pos[1]=py; cam_pos[2]=pz;
                cam_look[0]=lx; cam_look[1]=ly; cam_look[2]=lz;
                gcam_dist = cd; cam_height = ch;
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
    // ---- main.prg: se crea una vez y a partir de ahi es del usuario ----
    bool ask_regen_main = false;
    auto main_template = []() -> std::string {
        return
            "// ===== main del juego =====\n"
            "// Este fichero es TUYO: el editor lo crea una vez y no lo vuelve a\n"
            "// tocar. Lo que cambia con la escena esta en __escena.prg, que si se\n"
            "// rehace cada vez que generas (y por eso no hay que editarlo).\n\n"
            "import \"libmod_gfx\"; import \"libmod_misc\"; import \"libmod_input\"; import \"libmod_3d\";\n\n"
            "#include \"__escena.prg\"\n\n"
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
        // El escenario va en __escena.prg, que SE REHACE SIEMPRE. main.prg se crea
        // una sola vez y no se vuelve a tocar: es tuyo, para tu codigo de arranque,
        // menus, estados del juego... Los import van en main.prg (deben ir los
        // primeros del fichero), asi que aqui no se repiten.
        std::string mainp  = project_dir + "/main.prg";
        std::string escenp = project_dir + "/__escena.prg";
        FILE* f = fopen(escenp.c_str(), "w");
        if (!f) { status = "ERROR generando __escena.prg"; return; }
        fputs("// ===== Escenario generado por el editor BennuGD2 =====\n"
              "// NO EDITES ESTE FICHERO: se rehace entero cada vez que generas el\n"
              "// juego. Tu codigo va en main.prg, que el editor no toca.\n\n", f);
        fputs("GLOBAL int scene; int camera; int light; END\n\n", f);
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
            bool necesita = o.is_player || (o.phys >= 1 && o.phys <= 4);
            if (!necesita) continue;
            std::string psp = scripts_dir + "/" + o.name + ".prg";
            FILE* t = fopen(psp.c_str(), "r");
            bool existe = (t != nullptr);
            if (t) fclose(t);
            if (existe && !script_untouched(o.name)) continue;   // es tuyo
            if (script_write_generated(o))
                console_add((existe ? "Actualizado Scripts/" : "Creado Scripts/") + o.name + ".prg (" +
                            (o.is_player ? "controles del jugador" : "cuerpo fisico") + ")\n");
        }

        // componentes (scripts de cada objeto)
        for (auto& o : objects) {
            std::string sp = scripts_dir + "/" + o.name + ".prg";
            FILE* s = fopen(sp.c_str(), "r");
            if (!s) continue;
            fprintf(f, "// ---- componente: %s ----\n", o.name.c_str());
            char buf[1024]; size_t n;
            while ((n = fread(buf, 1, sizeof(buf), s)) > 0) fwrite(buf, 1, n, f);
            fputs("\n", f);
            fclose(s);
        }
        // main
        // Lo que comparten el montaje y el bucle tiene que ser GLOBAL: antes era
        // todo PRIVATE de un unico PROCESS main, y al partirlo en dos deja de valer.
        fputs("GLOBAL\n"
              "    int follow_ent;          // entidad a la que sigue la camara\n"
              "    int pplayer; int pmodel; // entidad y modelo del jugador\n"
              "    int atc[32]; int atn[32];// enganches a huesos: entidad y nodo\n"
              "    float escena_dt;\n"
              "END\n\n", f);
        fputs("// Monta el escenario: terreno, agua, objetos, sus procesos y la camara.\n"
              "FUNCTION escena_iniciar()\n", f);
        fputs("PRIVATE int e; int m; int tex; int mat;\nEND\nBEGIN\n", f);
        fputs("    set_mode(1280,720); set_fps(60,0); window_set_title(\"EDITOR_PLAY\");\n", f);
        fputs("    scene = g3d_scene_create(\"juego\"); g3d_scene_set_active(scene);\n", f);
        fputs("    camera = g3d_camera_create(); g3d_camera_set_active(camera);\n", f);
        fputs("    light = g3d_light_create(0,1.0,0.96,0.86); g3d_light_set_direction(light,-0.45,-0.75,-0.35);\n", f);
        fputs("    g3d_light_set_intensity(light,1.5); g3d_light_enable_shadow(light,1); g3d_set_shadows(1);\n", f);
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
        // objetos + sus componentes (+ cuerpos fisicos Jolt)
        fputs("    escena_dt = 1.0 / 60.0;\n", f);
        int pj = 0;   // indice de cuerpo fisico (literal)
        for (size_t i = 0; i < objects.size(); i++) {
            auto& o = objects[i];
            const char* loader = "g3d_load_gltf";
            { std::string a=o.asset; for(auto&c:a)c=(char)tolower(c);
              if (a.size()>4 && a.substr(a.size()-4)==".fbx") loader="g3d_load_fbx"; }
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
            // capturar la entidad de los objetos enganchados a un hueso
            for (size_t k = 0; k < attach_list.size(); k++)
                if (attach_list[k] == (int)i)
                    fprintf(f, "    atc[%d] = e;   // enganchado a hueso\n", (int)k);
            // ---- muro invisible: colisionador estatico alto + ocultar el modelo ----
            if (o.phys == 5 && !o.is_player) {
                float hx = o.csize > 0.05f ? o.csize : 0.5f;
                fprintf(f, "    g3d_collider_add_box(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n",
                        o.x - hx, o.y - 5.0f, o.z - hx, o.x + hx, o.y + 30.0f, o.z + hx);
                fputs("    g3d_entity_set_scale(e, 0.0001, 0.0001, 0.0001);   // invisible en el juego\n", f);
            }
            // Los cuerpos rigidos los crea cada objeto en SU script.
            std::string sp = scripts_dir + "/" + o.name + ".prg";
            // El jugador lleva sus controles en SU script: si aun no existe, se crea
            // con la plantilla para que el juego salga jugable a la primera.
            if ((int)i == player_idx) {   // su script ya se aseguro mas arriba
                fprintf(f, "    %s(e, m);   // jugador: entidad + modelo (para animar)\n", o.name.c_str());
            } else {
                FILE* s = fopen(sp.c_str(), "r");
                if (s) { fclose(s); fprintf(f, "    %s(e);\n", o.name.c_str()); }
            }
        }
        // ---- huesos de enganche (el jugador se crea a si mismo en SU script) ----
        if (player_idx >= 0) {
            for (size_t k = 0; k < attach_list.size(); k++)
                fprintf(f, "    atn[%d] = g3d_model_node_find(pmodel, \"%s\");\n",
                        (int)k, objects[attach_list[k]].attach_bone.c_str());
        }
        // ---- CAMARA: modo elegido en el editor ----
        bool follow = (cam_mode != 0 && cam_follow >= 0 && cam_follow < (int)objects.size());
        if (!follow) {
            // Fija (o modo-seguir sin objetivo valido): posicion + mira estaticas
            fprintf(f, "    g3d_camera_set_position(camera, %.3f, %.3f, %.3f);\n",
                    cam_pos[0], cam_pos[1], cam_pos[2]);
            fprintf(f, "    g3d_camera_look_at(camera, %.3f, %.3f, %.3f, 0.0, 1.0, 0.0);\n",
                    cam_look[0], cam_look[1], cam_look[2]);
        }
        // codigo de camara por-frame (vacio si es Fija)
        std::string camf;
        if (follow) {
            camf  = "        g3d_entity_get_position(follow_ent, &tx, &ty, &tz);\n";
            char b[512];
            if (cam_mode == 1) {          // tercera persona: detras y arriba
                snprintf(b, sizeof(b),
                    "        g3d_camera_set_position(camera, tx, ty + %.3f, tz - %.3f);\n"
                    "        g3d_camera_look_at(camera, tx, ty + 1.0, tz, 0.0, 1.0, 0.0);\n",
                    cam_height, gcam_dist);
            } else if (cam_mode == 2) {   // primera persona (FPS): a la altura de los ojos
                snprintf(b, sizeof(b),
                    "        g3d_camera_set_position(camera, tx, ty + %.3f, tz);\n"
                    "        g3d_camera_look_at(camera, tx, ty + %.3f, tz + 10.0, 0.0, 1.0, 0.0);\n",
                    cam_height, cam_height);
            } else {                      // cenital (top-down): justo encima mirando abajo
                snprintf(b, sizeof(b),
                    "        g3d_camera_set_position(camera, tx, ty + %.3f, tz + 0.5);\n"
                    "        g3d_camera_look_at(camera, tx, ty, tz, 0.0, 1.0, 0.0);\n",
                    gcam_dist);
            }
            camf += b;
        }
        // Aqui acaba el montaje. El bucle por frame va en su propio proceso para
        // que main.prg pueda ser tuyo: arrancas el escenario, lanzas el motor y
        // encima escribes lo que quieras.
        fputs("    RETURN;\nEND\n\n", f);
        fputs("// Mueve la fisica, los enganches y la camara, un paso por frame.\n"
              "PROCESS escena_motor()\n", f);
        fputs("PRIVATE float tx; float ty; float tz;\n"
              "  float px; float py; float pz; float pfacing;\n"
              "  float nx; float ny; float nz; float wx2; float wz2; float a2;\nEND\nBEGIN\n", f);
        fputs("    LOOP\n", f);
        // --- fisica: solo se avanza el MUNDO una vez por frame; cada objeto
        //     coloca su modelo desde su propio script ---
        fputs("        g3d_rigidbody_step(escena_dt);\n", f);
        // ---- objetos enganchados a un hueso del jugador (arma en la mano) ----
        // El jugador lo mueve SU script; aqui solo leemos donde ha quedado su
        // entidad para colgarle el arma del hueso.
        if (player_idx >= 0 && !attach_list.empty()) {
            auto& p = objects[player_idx];
            fputs("        g3d_entity_get_position(pplayer, &px, &py, &pz);\n", f);
            fputs("        g3d_entity_get_rotation(pplayer, &tx, &pfacing, &tz);\n", f);
            for (size_t k = 0; k < attach_list.size(); k++) {
                auto& a = objects[attach_list[k]];
                float s = a.att_scale;
                char c[1024];
                snprintf(c, sizeof(c),
                    "        IF (atn[%d] >= 0)\n"
                    "            nx = g3d_model_node_x(pmodel, atn[%d]) * %.3f + %.3f;\n"
                    "            ny = g3d_model_node_y(pmodel, atn[%d]) * %.3f + %.3f;\n"
                    "            nz = g3d_model_node_z(pmodel, atn[%d]) * %.3f + %.3f;\n"
                    "            a2 = pfacing;\n"
                    "            wx2 =  nx * cos(a2) + nz * sin(a2);\n"
                    "            wz2 = -nx * sin(a2) + nz * cos(a2);\n"
                    "            g3d_entity_set_position(atc[%d], px + wx2, py + ny, pz + wz2);\n"
                    "            g3d_entity_set_rotation(atc[%d], 0.0, a2 + %.1f, 0.0);\n"
                    "            g3d_entity_set_scale(atc[%d], %.3f, %.3f, %.3f);\n"
                    "        END\n",
                    (int)k,
                    (int)k, p.scale, a.att_off[0],
                    (int)k, p.scale, a.att_off[1],
                    (int)k, p.scale, a.att_off[2],
                    (int)k, (int)k, a.att_yaw * 1000.0f,
                    (int)k, s, s, s);
                fputs(c, f);
            }
        }
        fputs(camf.c_str(), f);
        fputs("        FRAME;\n    END\nEND\n", f);
        fclose(f);

        // main.prg: se crea la PRIMERA vez y despues no se toca nunca mas. Es el
        // sitio para tu codigo (menus, estados, lo que sea) sin que el editor lo
        // pise cada vez que cambias algo en la escena.
        { FILE* mf = fopen(mainp.c_str(), "r");
          if (mf) fclose(mf);
          else {
            if (!write_main_prg()) { status = "ERROR creando main.prg"; return; }
            console_add("Creado main.prg (a partir de ahora es tuyo, el editor no lo toca)\n");
          }
        }
        // bgdc/bgdi localizan los modulos (.so) via PATH/LD_LIBRARY_PATH; el popen
        // no hereda el PATH del perfil -> los fijamos al directorio de los modulos.
        std::string bindir = std::string(BGDC_PATH); bindir = bindir.substr(0, bindir.rfind('/'));
        std::string env = "PATH=\"" + bindir + ":$PATH\" LD_LIBRARY_PATH=\"" + bindir + ":$LD_LIBRARY_PATH\" ";
        std::string proj = project_dir;
        std::string cmd = "cd \"" + proj + "\" && " + env + "\"" + BGDC_PATH + "\" main.prg 2>&1";
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
    struct SimBody { int ent, bid, buoy, wet; float half, mass, bk, prevx, prevy, prevz, ript; };
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
                int buoy=(water_on&&o.buoyant&&o.mass>0.0f)?1:0; float bk=0.0f;
                // flotacion real: la hace el motor repartiendo el empuje por el
                // volumen sumergido (profundidad, adrizamiento y vuelco salen solos)
                if(buoy) g3d_rigidbody_set_buoyancy(bid, water_level,
                                                    o.density>0.05f?o.density:0.05f);
                sim_bodies.push_back({ o.entity, bid, buoy, 0, c, o.mass, bk, o.x, by0, o.z, 0.0f });
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
            // Agua: chapuzon y estela por CONTACTO (flote o no); la flotacion aparte.
            if (water_on) {
                float bx = g3d_rigidbody_x(b.bid), by = g3d_rigidbody_y(b.bid), bz = g3d_rigidbody_z(b.bid);
                if (by - b.half < water_level) {
                    if (!b.wet) {
                        g3d_water_splash(bx, water_level, bz, 1.0f); b.wet = 1;
                        // el agua frena: si no, lo que flota patina por el lago sin parar
                        g3d_rigidbody_set_damping(b.bid, 2.5f, 3.0f);
                    }
                    // solo agita el agua mientras SE MUEVE cerca de la superficie:
                    // hundido y en reposo, las ondas paran; flotando, el oleaje lo mece
                    float dx=bx-b.prevx, dy=by-b.prevy, dz=bz-b.prevz;
                    if (dx*dx+dy*dy+dz*dz > 0.0004f && by + b.half > water_level - 1.0f) {
                        b.ript += dt;
                        if (b.ript > 0.15f) { g3d_water_ripple(bx, bz, 0.4f); b.ript = 0.0f; }
                    }
                } else {
                    if (b.wet) g3d_rigidbody_set_damping(b.bid, 0.05f, 0.05f);   // fuera del agua
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
            if (water_on && p.buoyant){ if (py<water_level-1.2f) inw=1; g3d_char_set_water(sim_pch,inw,water_level); }
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
            g3d_camera_set_position(cam,tx,ty+cam_height,tz);
            g3d_camera_look_at(cam,tx,ty+cam_height,tz+10.0f,0.0f,1.0f,0.0f);
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

        // --- Atajos: copiar / pegar / duplicar / borrar ---
        // Solo si no se esta escribiendo (nombres, rutas, el editor de scripts) ni
        // corriendo el Play: si no, Ctrl+C en el codigo borraria un objeto.
        if (!ImGui::GetIO().WantTextInput && !show_script && !playing) {
            bool ctrl = ImGui::GetIO().KeyCtrl;
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
            if (ImGui::BeginMenu("Escena")) {
                if (ImGui::MenuItem("Vaciar escena")) {
                    for (auto& o : objects) g3d_entity_impl_set_position(o.entity, 0, -99999, 0);
                    objects.clear(); obj_sel = -1;
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
                    else if (cam_mode == 2) { cp[0]=tx; cp[1]=ty+cam_height; cp[2]=tz; ct[0]=tx; ct[1]=ty+cam_height; ct[2]=tz+10.0f; }
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
            } else if (!terr_tool && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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
        ImGui::SeparatorText(ICON_FA_WATER "  Agua");
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
                ImGui::SliderFloat("Distancia", &gcam_dist, 1.0f, 30.0f, "%.1f");
                ImGui::SliderFloat("Altura", &cam_height, 0.0f, 20.0f, "%.1f");
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
                bool con_plantilla = o.is_player || (o.phys >= 1 && o.phys <= 4);
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
                    char bonebuf[256]; strncpy(bonebuf, o.attach_bone.c_str(), 255); bonebuf[255] = 0;
                    if (ImGui::InputText("Hueso", bonebuf, sizeof(bonebuf))) o.attach_bone = bonebuf;
                    ImGui::TextDisabled("Parte del nombre: RightHand, Hand_R, mano...");
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
            ImGui::TextDisabled("El escenario (__escena.prg) no se toca: ese se rehace solo al generar.");
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
                std::string cmd = std::string(BGDC_PATH) + " " + tmp + " 2>&1";
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
