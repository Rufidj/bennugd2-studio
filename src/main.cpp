// ============================================================================
//  BennuGD2 3D Editor  -  standalone Dear ImGui application
//  Paso 2: viewport del motor. Compilamos el CORE de libmod_3d dentro del editor
//  y llamamos a g3d_renderer_render() contra ESTE contexto OpenGL, con ImGui
//  dibujado por encima. (El viewport en ventana ImGui acoplable vendra despues.)
// ============================================================================
#include <SDL.h>
#include <SDL_syswm.h>
#include <cstdio>
#include <cstring>
#include <cmath>


#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

/* Lista ficheros de <dir> Y DE SUS SUBCARPETAS cuya extension esta en exts.
   Devuelve la ruta relativa a <dir>: "barrel.glb" si esta suelto, o
   "Models/barrel.glb" si esta clasificado. Asi el proyecto se puede ordenar en
   carpetas sin que el editor deje de encontrar nada, y los proyectos viejos
   (todo suelto en Assets) siguen funcionando igual. */
static std::vector<std::string> scan_dir(const std::string& dir,
                                         std::initializer_list<const char*> exts) {
    std::vector<std::string> out;
    std::error_code ec;
    fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) return out;
    for (; it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        std::error_code e1;
        if (!it->is_regular_file(e1)) continue;
        auto ext = it->path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for (auto x : exts) if (ext == x) {
            std::error_code e2;
            std::string rel = fs::relative(it->path(), dir, e2).generic_string();
            out.push_back(e2 ? it->path().filename().string() : rel);
            break;
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}
static std::vector<std::string> scan_assets(const std::string& dir) {
    return scan_dir(dir, { ".glb", ".gltf", ".fbx" });
}
// Nombre valido para BennuGD2 a partir de un texto cualquiera (el nombre de un
// fichero, lo que ha escrito el usuario): minusculas, sin acentos ni espacios, y
// que no empiece por numero. La generacion de codigo y el editor de scripts usan
// la misma regla, o el fichero se llamaria de una manera y el PROCESS de otra.
static std::string ident_bgd(const std::string& t, const char* pref) {
    std::string o;
    for (char c : t) {
        if (isalnum((unsigned char)c)) o += (char)tolower(c);
        else if (!o.empty() && o.back() != '_') o += '_';
    }
    while (!o.empty() && o.back() == '_') o.pop_back();
    if (o.empty() || isdigit((unsigned char)o[0])) o = std::string(pref) + o;
    return o;
}

// Parte una linea de escena por '|'. Los campos que falten (una escena guardada
// por una version anterior) se quedan vacios, que es justo lo que hace falta para
// poder anadir campos al final sin romper lo ya guardado.
/* ---- Nombres de fichero que el JUEGO puede abrir ----
   Medido con bgdc: una ruta con cualquier byte fuera del ASCII (un emoji, pero
   tambien una simple enie o una tilde) no se abre en el juego -- el compilador
   recodifica las cadenas y el nombre deja de cuadrar con el del disco. Espacios,
   guiones y corchetes si valen. Asi que la regla es esa: solo ASCII. */
static bool nombre_ascii(const std::string& f) {
    for (unsigned char c : f) if (c < 32 || c > 126) return false;
    return true;
}
/* La version ASCII de un nombre: las vocales acentuadas y la enie se pasan a su
   letra (que es lo que uno espera) y lo que no tiene equivalente se cae. */
static std::string nombre_saneado(const std::string& f) {
    static const struct { const char* de; char a; } mapa[] = {
        {"á",'a'},{"à",'a'},{"ä",'a'},{"â",'a'},{"é",'e'},{"è",'e'},{"ë",'e'},{"ê",'e'},
        {"í",'i'},{"ì",'i'},{"ï",'i'},{"î",'i'},{"ó",'o'},{"ò",'o'},{"ö",'o'},{"ô",'o'},
        {"ú",'u'},{"ù",'u'},{"ü",'u'},{"û",'u'},{"ñ",'n'},{"ç",'c'},
        {"Á",'A'},{"É",'E'},{"Í",'I'},{"Ó",'O'},{"Ú",'U'},{"Ñ",'N'},
    };
    std::string out;
    for (size_t i = 0; i < f.size(); ) {
        unsigned char c = (unsigned char)f[i];
        if (c >= 32 && c <= 126) { out += (char)c; i++; continue; }
        bool hecho = false;
        for (auto& m : mapa) {
            size_t n = strlen(m.de);
            if (f.compare(i, n, m.de) == 0) { out += m.a; i += n; hecho = true; break; }
        }
        if (!hecho) i++;      // lo que no se sabe traducir se cae
    }
    // sin espacios repetidos ni bordes sucios, y que quede algo
    while (!out.empty() && (out.front() == ' ' || out.front() == '.')) out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    if (out.empty() || out[0] == '.') out = "audio" + out;
    return out;
}

static int trozos(const char* txt, std::string* out, int n) {
    std::string t(txt);
    while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
    int np = 0; size_t ini = 0;
    for (size_t k = 0; k <= t.size() && np < n; k++)
        if (k == t.size() || t[k] == '|') { out[np++] = t.substr(ini, k - ini); ini = k + 1; }
    return np;
}

/* La musica y los sonidos del proyecto. Van en Assets/Music y Assets/Sounds
   (como Assets, Scenes y Scripts, en ingles), pero se mira TAMBIEN la raiz de
   Assets: quien ya los tenia sueltos ahi no tiene que mover nada. Los nombres
   salen relativos a Assets ("Music/tema.ogg"), que es como los pide el juego. */
static void scan_sonoros(const std::string& assets, const char* sub,
                         std::initializer_list<const char*> exts,
                         std::vector<std::string>& out) {
    out.clear();
    std::error_code ec;
    for (auto& e : fs::directory_iterator(assets + "/" + sub, ec)) {
        if (!e.is_regular_file(ec)) continue;
        auto ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for (auto x : exts) if (ext == x) {
            out.push_back(std::string(sub) + "/" + e.path().filename().string()); break;
        }
    }
    for (auto& e : fs::directory_iterator(assets, ec)) {
        if (!e.is_regular_file(ec)) continue;
        auto ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for (auto x : exts) if (ext == x) { out.push_back(e.path().filename().string()); break; }
    }
    std::sort(out.begin(), out.end());
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
#include "hud2d.h"             // FPG/FNT/PNG nativos: el previo del HUD 2D

// ---- API C del motor (core de libmod_3d, sin la capa BennuGD2) --------------
extern "C" {
    /* ---- sprites 2D en el mundo 3D (personajes estilo HD-2D) ---- */
    int   g3d_sprite_create(int scene_id, float x, float y, float z);
    void  g3d_sprite_destroy(int id);
    void  g3d_sprite_set_position(int id, float x, float y, float z);
    void  g3d_sprite_set_texture(int id, unsigned tex, int px_w, int px_h,
                                 float uscale, float vscale);
    void  g3d_sprite_set_cell(int id, int x, int y, int w, int h);
    void  g3d_sprite_set_anchor(int id, float ax, float ay);
    void  g3d_sprite_set_height(int id, float h);
    void  g3d_sprite_set_pixels_per_unit(int id, float ppu);
    void  g3d_sprite_set_billboard(int id, int mode);
    void  g3d_sprite_set_cutout(int id, float t);
    void  g3d_sprite_set_shadow(int id, int on);
    void  g3d_sprite_set_smooth(int id, int on);
    void  g3d_sprite_set_lit(int id, int on);
    void  g3d_sprite_set_snap(int id, int on);
    void  g3d_sprite_set_visible(int id, int v);
    void  g3d_sprites_clear(void);

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
    int   g3d_light_impl_set_color(int light_id, float r, float g, float b);
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
    void  g3d_water_render_set_surf(float amount, float wavelength, float speed, float runup);
    void  g3d_water_render_set_surf_wave(float height, float direction_deg);
    void  g3d_water_render_set_detail(float foam, float refraction);
    void  g3d_water_splash_set_amount(float amount);
    void  g3d_water_splash_set_threshold(float speed);
    int   g3d_entity_impl_set_collider(int entity_id, int solid);
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
    int   g3d_editor_terrain_grid(void *mesh, int *side, float *size);
    int   g3d_editor_terrain_vertex(void *mesh, int i, int j, float *out_xyz);
    void  g3d_editor_terrain_set_vertex_y(void *mesh, int i, int j, float y, int commit);
    void  g3d_editor_terrain_set_vertex_xz(void *mesh, int i, int j, float x, float z,
                                           float max_frac, int commit);
    void  g3d_editor_terrain_save_xz(void *mesh, const char *path);
    int   g3d_editor_terrain_load_xz(void *mesh, const char *path);
    int   g3d_editor_terrain_erode(void *mesh, int iterations, float rain, float evap,
                                   float capacity, float dissolve, float deposit,
                                   float min_slope, float talus);
    void  g3d_editor_terrain_commit(void *mesh);
    void  g3d_scatter_set_base(const char *dir);
    void  g3d_scatter_set_kind_wind(const char *asset, float wind);
    float g3d_scatter_get_kind_wind(int kind);
    int   g3d_scatter_kind_groups(int kind);
    void  g3d_scatter_set_kind_distance(const char *asset, float dist);
    float g3d_scatter_get_kind_distance(int kind);
    void  g3d_instances_set_lod_distance(float d);
    void  g3d_scatter_set_kind_solid(const char *asset, int solid);
    void  g3d_scatter_kind_apply(int kind, float wind, float dist, int solid);
    int   g3d_scatter_get_kind_solid(int kind);
    int   g3d_scatter_solid_placed(void);
    int   g3d_scatter_set(int kind, int index, float x, float y, float z, float yaw, float scale);
    int   g3d_scatter_remove(int kind, int index);
    int   g3d_scatter_pick(float x, float z, float radius, int *k, int *i);
    void  g3d_scatter_clear(void);
    int   g3d_scatter_add(const char *asset, float x, float y, float z, float yaw, float scale);
    int   g3d_scatter_build(float wind);
    int   g3d_scatter_count(void);
    int   g3d_instances_free_slots(void);
    int   g3d_scatter_kinds(void);
    const char *g3d_scatter_kind_asset(int kind);
    int   g3d_scatter_kind_count(int kind);
    int   g3d_scatter_get(int kind, int index, float *out5);
    int   g3d_scatter_save(const char *path);
    int   g3d_scatter_load(const char *path, float wind);
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
    void  g3d_waterfield_clear_water(void);
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
    /* ---- SONIDO ----
       La musica de la escena (ambiente) y los sonidos que usan las reglas y los
       objetos. La musica es UNA por escena: SDL_mixer solo toca una a la vez. */
    std::vector<std::string> musicas, sonidos;
    std::string esc_musica;            // fichero (relativo a Assets), vacio = sin musica
    int   esc_mus_vol  = 90;           // 0..128
    int   esc_mus_loop = 1;
    float esc_mus_fade = 1.0f;         // segundos de entrada (0 = de golpe)
    struct ZonaSonido { int zona = 0; std::string sonido; int vol = 90; };
    std::vector<ZonaSonido> zsonidos;  // ambiente mientras el jugador esta en la zona

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
    // olas de PLAYA: rompientes en el bajio y lengua de agua que sube por la arena.
    // Distintas del oleaje de mar abierto: dependen de la profundidad, no del viento.
    float surf_amount = 0.55f, surf_len = 9.0f, surf_speed = 0.16f, surf_runup = 1.8f;
    float surf_height = 0.55f, surf_dir = 0.0f;   // cresta y rumbo (grados)
    float water_foam = 0.55f;                     // cuanta espuma lleva el agua
    float splash_amount = 1.0f;                   // gotas al chocar con algo
    float splash_speed  = 0.8f;                   // corriente minima para salpicar

    // ---- CAMARA principal del juego (se genera en el main.prg) ----
    // modo: 0=Fija 1=Tercera persona 2=Primera persona(FPS) 3=Cenital(top-down)
    int   cam_mode   = 0;
    int   cam_follow = -1;                    // indice del objeto a seguir (-1 = ninguno)
    float cam_pos[3]  = { 0.0f, 45.0f, -90.0f };  // posicion (modo Fija)
    float cam_look[3] = { 0.0f,  2.0f,   0.0f };  // punto de mira (modo Fija)
    float gcam_dist   = 8.0f;                   // distancia detras/altura del objetivo
    float cam_height = 3.0f;                    // altura de la camara sobre el objetivo
    // Alrededor de que angulo orbita la camara al personaje, en grados. 0 es a la
    // espalda, que es lo de siempre; 90 lo pone de PERFIL, que es como se ve un
    // plataformas 2.5D. Sin este angulo la tercera persona estaba clavada detras y
    // no habia forma de pedir otra cosa.
    float cam_orbit = 0.0f;
    // Plataformas de perfil: la profundidad se bloquea a proposito y solo se anda
    // por el eje de la pantalla. Sin esto el personaje se va al fondo y se pierde.
    bool  cam_25d = false;
    /* Tercera persona: que la camara la gire EL JUGADOR mientras juega, en vez de
       estar clavada en un angulo. El angulo pasa a ser una GLOBAL del juego
       (escena_orbita), y de ella salen tanto el brazo de la camara como los
       controles: si girase la camara y los controles siguieran atados al angulo
       de antes, la D dejaria de llevar a la derecha de la pantalla. */
    int   cam_girable = 0;        // 0 = angulo fijo, 1 = la gira el jugador
    int   cam_gira_con = 0;       // 0 = raton, 1 = teclas, 2 = stick derecho
    float cam_gira_vel = 120.0f;  // grados por segundo (teclas y mando)
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
    float ws_prefill = 60.0f;   // segundos de agua ya corrida al arrancar el juego
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
        /* Y el agua del campo unificado, que es donde acaban de verdad lagos y
           rios. Sin esto solo se podia ANADIR: borrar un lago dejaba su agua, y
           la previsualizacion inundaba el mapa con solo pasar el raton. */
        g3d_waterfield_clear_water();
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
        if (resettle) g3d_watersim_settle(ws_prefill);   // lo mismo que vera el juego
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
    /* Cualquier panel se puede arrancar y dejar suelto FUERA de la ventana del
       editor (en otro monitor, por ejemplo). Acoplado sigue siendo lo normal.
       Sin esta linea los paneles flotan, pero atrapados dentro del marco. */
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    /* ---- EL ASPECTO ----
       El tema de serie de ImGui es el de una ventana de depuracion: todo pegado,
       gris plano y esquinas vivas. Aqui se le da aire (padding y separacion),
       esquinas redondeadas y una paleta con UN color de acento, para que se lea
       como una herramienta y no como un panel de pruebas. */
    ImGui::StyleColorsDark();
    {
        ImGuiStyle& st = ImGui::GetStyle();
        // --- espacio: cambia la sensacion mas que el color ---
        st.WindowPadding     = ImVec2(12, 10);
        st.FramePadding      = ImVec2(10, 6);
        st.ItemSpacing       = ImVec2(10, 8);
        st.ItemInnerSpacing  = ImVec2(8, 6);
        st.CellPadding       = ImVec2(8, 5);
        st.IndentSpacing     = 20.0f;
        st.ScrollbarSize     = 13.0f;
        st.GrabMinSize       = 11.0f;
        // --- bordes y esquinas ---
        st.WindowBorderSize  = 1.0f;
        st.FrameBorderSize   = 0.0f;
        st.PopupBorderSize   = 1.0f;
        st.FrameRounding     = 5.0f;
        st.GrabRounding      = 5.0f;
        st.PopupRounding     = 6.0f;
        st.ScrollbarRounding = 6.0f;
        st.TabRounding       = 5.0f;
        st.ChildRounding     = 6.0f;
        // Un panel suelto es una ventana del sistema: sin esquinas redondeadas ni
        // transparencia, o se ve el corte contra el escritorio.
        st.WindowRounding    = 0.0f;
        st.Colors[ImGuiCol_WindowBg].w = 1.0f;
        // --- alineaciones ---
        st.WindowTitleAlign  = ImVec2(0.02f, 0.5f);
        st.ButtonTextAlign   = ImVec2(0.5f, 0.5f);
        st.SeparatorTextBorderSize = 1.0f;
        st.SeparatorTextPadding    = ImVec2(16, 6);

        /* La paleta: grises muy oscuros con un punto de azul (asi no parece
           apagada) y UN acento -- ambar -- para lo elegido y lo activo. Con dos
           acentos no destaca ninguno. */
        ImVec4* c = st.Colors;
        const ImVec4 fondo      = ImVec4(0.086f, 0.094f, 0.110f, 1.00f);
        const ImVec4 panel      = ImVec4(0.125f, 0.137f, 0.161f, 1.00f);
        const ImVec4 panel_alto = ImVec4(0.169f, 0.184f, 0.212f, 1.00f);
        const ImVec4 borde      = ImVec4(0.226f, 0.243f, 0.278f, 1.00f);
        const ImVec4 texto      = ImVec4(0.878f, 0.894f, 0.925f, 1.00f);
        const ImVec4 texto_ten  = ImVec4(0.514f, 0.545f, 0.600f, 1.00f);
        const ImVec4 acento     = ImVec4(0.949f, 0.686f, 0.259f, 1.00f);
        const ImVec4 acento_ap  = ImVec4(0.949f, 0.686f, 0.259f, 0.28f);

        c[ImGuiCol_Text]                 = texto;
        c[ImGuiCol_TextDisabled]         = texto_ten;
        c[ImGuiCol_WindowBg]             = fondo;
        c[ImGuiCol_ChildBg]              = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg]              = panel;
        c[ImGuiCol_Border]               = borde;
        c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_FrameBg]              = panel;
        c[ImGuiCol_FrameBgHovered]       = panel_alto;
        c[ImGuiCol_FrameBgActive]        = ImVec4(0.208f, 0.227f, 0.263f, 1.00f);
        c[ImGuiCol_TitleBg]              = fondo;
        c[ImGuiCol_TitleBgActive]        = panel;
        c[ImGuiCol_TitleBgCollapsed]     = fondo;
        c[ImGuiCol_MenuBarBg]            = ImVec4(0.106f, 0.114f, 0.133f, 1.00f);
        c[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.239f, 0.259f, 0.298f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.310f, 0.333f, 0.380f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive]  = acento;
        c[ImGuiCol_CheckMark]            = acento;
        c[ImGuiCol_SliderGrab]           = ImVec4(0.400f, 0.435f, 0.490f, 1.00f);
        c[ImGuiCol_SliderGrabActive]     = acento;
        c[ImGuiCol_Button]               = panel_alto;
        c[ImGuiCol_ButtonHovered]        = ImVec4(0.239f, 0.263f, 0.306f, 1.00f);
        c[ImGuiCol_ButtonActive]         = acento_ap;
        c[ImGuiCol_Header]               = panel_alto;
        c[ImGuiCol_HeaderHovered]        = ImVec4(0.239f, 0.263f, 0.306f, 1.00f);
        c[ImGuiCol_HeaderActive]         = acento_ap;
        c[ImGuiCol_Separator]            = borde;
        c[ImGuiCol_SeparatorHovered]     = acento_ap;
        c[ImGuiCol_SeparatorActive]      = acento;
        c[ImGuiCol_ResizeGrip]           = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ResizeGripHovered]    = acento_ap;
        c[ImGuiCol_ResizeGripActive]     = acento;
        c[ImGuiCol_Tab]                  = fondo;
        c[ImGuiCol_TabHovered]           = panel_alto;
        c[ImGuiCol_TabActive]            = panel;
        c[ImGuiCol_TabUnfocused]         = fondo;
        c[ImGuiCol_TabUnfocusedActive]   = panel;
        c[ImGuiCol_DockingPreview]       = acento_ap;
        c[ImGuiCol_DockingEmptyBg]       = fondo;
        c[ImGuiCol_TextSelectedBg]       = acento_ap;
        c[ImGuiCol_NavHighlight]         = acento;
        c[ImGuiCol_TableHeaderBg]        = panel;
        c[ImGuiCol_TableBorderStrong]    = borde;
        c[ImGuiCol_TableBorderLight]     = ImVec4(0.180f, 0.196f, 0.227f, 1.00f);
        c[ImGuiCol_PlotHistogram]        = acento;
    }

    /* ---- LA FUENTE ----
       La de ImGui por defecto es un mapa de bits de 13 px pensado para depurar, y
       es lo que le daba al editor esa cara de consola. Se carga una tipografia de
       verdad (Noto Sans, licencia OFL, va en fonts/) y encima se fusionan los
       iconos. Si no aparece, se cae con dignidad a la de siempre. */
    auto buscar_fuente = [&](const char* fichero) {
        std::vector<std::string> cands;
        if (char* base = SDL_GetBasePath()) {
            cands.push_back(std::string(base) + "fonts/" + fichero);
            cands.push_back(std::string(base) + "../fonts/" + fichero);
            SDL_free(base);
        }
        cands.push_back(std::string(FONT_DIR "/") + fichero);
        cands.push_back(std::string("fonts/") + fichero);
        for (auto& c : cands) { FILE* t = fopen(c.c_str(), "rb"); if (t) { fclose(t); return c; } }
        return std::string();
    };
    const float TAM_TEXTO = 16.0f;
    ImFont* fuente_titulo = nullptr;
    {
        ImFontConfig cfg;
        cfg.OversampleH = 2; cfg.OversampleV = 2; cfg.PixelSnapH = false;
        std::string reg = buscar_fuente("NotoSans-Regular.ttf");
        if (!reg.empty()) io.Fonts->AddFontFromFileTTF(reg.c_str(), TAM_TEXTO, &cfg);
        else              io.Fonts->AddFontDefault();
    }
    static const ImWchar fa_range[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig fa_cfg; fa_cfg.MergeMode = true; fa_cfg.PixelSnapH = true;
    fa_cfg.GlyphMinAdvanceX = 17.0f;
    fa_cfg.GlyphOffset = ImVec2(0.0f, 2.0f);   // los iconos, a la linea del texto
    // La fuente de iconos se busca PRIMERO junto al ejecutable y luego en la ruta
    // de compilacion. FONT_DIR se fija al configurar con CMake, asi que apunta al
    // arbol de fuentes de quien compilo: si el binario se mueve, se instala o se
    // reparte, esa ruta ya no existe. Y si no se encuentra, ImGui aborta sin
    // decir que fichero le falta, que es lo peor posible para quien lo estrena.
    {
        std::string hallada = buscar_fuente(FONT_ICON_FILE_NAME_FAS);
        std::vector<std::string> cands = { std::string("junto al ejecutable, en fonts/"),
                                           std::string(FONT_DIR "/" FONT_ICON_FILE_NAME_FAS) };
        if (!hallada.empty()) {
            io.Fonts->AddFontFromFileTTF(hallada.c_str(), TAM_TEXTO - 2.0f, &fa_cfg, fa_range);
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

    {   // la negrita, para titulos de seccion y lo que tenga que destacar
        std::string bold = buscar_fuente("NotoSans-Bold.ttf");
        if (!bold.empty()) {
            ImFontConfig cfg;
            cfg.OversampleH = 2; cfg.OversampleV = 2; cfg.PixelSnapH = false;
            fuente_titulo = io.Fonts->AddFontFromFileTTF(bold.c_str(), TAM_TEXTO, &cfg);
            ImFontConfig fa2 = fa_cfg;
            std::string ic = buscar_fuente(FONT_ICON_FILE_NAME_FAS);
            if (!ic.empty()) io.Fonts->AddFontFromFileTTF(ic.c_str(), TAM_TEXTO - 2.0f, &fa2, fa_range);
        }
    }
    (void)fuente_titulo;

    ImGui_ImplSDL2_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ---- editor de scripts (ImGuiColorTextEdit + resaltado BennuGD2) ----
    /* ---- Los ficheros abiertos en el editor de codigo ----
       Uno por pestania. El texto de disco se guarda aparte para saber si hay
       cambios sin guardar sin tener que comparar contra el fichero cada frame.
       Se guardan por puntero: TextEditor es pesado y no le sienta bien que el
       vector lo mueva de sitio al crecer. */
    struct CodeDoc {
        std::string ruta;        // ruta completa en disco
        std::string titulo;      // lo que se lee en la pestania
        std::string objeto;      // objeto del que es el script (vacio si no lo es)
        std::string en_disco;    // texto tal cual se leyo/guardo
        bool  sucio = false;
        TextEditor ed;
    };
    std::vector<std::unique_ptr<CodeDoc>> docs;
    int doc_sel = -1;                  // pestania activa
    int doc_cerrar = -1;               // pestania que se pide cerrar (con aviso si esta sucia)
    auto doc_activo = [&]() -> CodeDoc* {
        return (doc_sel >= 0 && doc_sel < (int)docs.size()) ? docs[doc_sel].get() : nullptr;
    };

    bool show_gvars = false;               // ventana de variables del juego
    bool show_escenas = false;             // ventana de escenas del proyecto
    bool show_menus = false;               // ventana de menus del juego
    bool show_dialogos = false;            // ventana de dialogos
    bool show_guardado = false;            // ventana de guardar partida
    bool pedir_ordenar = false;            // confirmar el orden de los assets
    /* ---- MODOS DE TRABAJO ----
       El editor no ensenia todo a la vez: cada modo saca sus herramientas y sus
       paneles, y esconde lo que no toca. Es lo que hace que no se amontone. */
    enum Modo { M_ESCENA, M_TERRENO, M_PERSONAJES, M_INTERFAZ, M_CODIGO, M_NUM };
    int  modo = M_ESCENA;
    int  modo_ant = -1;                    // para rehacer la disposicion al cambiar
    bool rehacer_layout = false;           // "restablecer disposicion"
    bool poner_delante = false;            // traer al frente las pestanas del modo
    /* Abrir un panel que ya estaba abierto PERO detras de otra pestania no hacia
       nada visible: parecia que el boton estaba roto. Se apunta cual hay que traer
       al frente y se hace al final del frame, cuando la ventana ya existe. */
    std::string enfocar_panel;
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
        // DEPURACION: con EDITOR_CONSOLA_STDOUT=1 la consola sale tambien por la
        // terminal, para ver los avisos sin abrir el panel (util sin GUI).
        if (getenv("EDITOR_CONSOLA_STDOUT")) { fputs(s.c_str(), stdout); fflush(stdout); }
        if (game_out.size() > 400000) game_out.erase(0, game_out.size() - 300000);  // no crecer sin fin
    };
    std::string script_obj = "barril_01";  // objeto cuyo script se edita (placeholder)
    // ---- herramienta activa (toolbar con iconos) ----
    enum Tool { T_SELECT, T_MOVE, T_ROTATE, T_SCALE, T_PLACE, T_RAISE, T_LOWER, T_SMOOTH, T_FLATTEN, T_PAINT,
                T_HOLE, T_ZONE, T_LAKE, T_RIVER, T_WATERFALL, T_WATERSOURCE, T_VERTEX,
                T_SCATTER, T_HUD, T_SPRITE };
    bool hole_fill = false;   // T_HOLE: false=perforar, true=rellenar
    int tool = T_SELECT;
    int  zone_layer = 0;      // T_ZONE: capa (0..3) que se pinta
    bool zone_erase = false;  // T_ZONE: borrar en vez de pintar
    bool  cam_gizmo = false;   // el gizmo mueve la CAMARA cuando no hay objeto elegido
    float brush_r = 30.0f, brush_str = 0.5f;   // pincel de terreno
    // --- T_VERTEX: tocar la rejilla vertice a vertice ---
    int   vx_px     = 14;      // separacion minima en pixeles entre lineas de la rejilla
    int   vx_soft   = 0;       // arrastra tambien los vecinos hasta este radio
    float vx_sens   = 0.06f;   // unidades de mundo por pixel arrastrado
    int   vx_i = -1, vx_j = -1;      // vertice agarrado
    float vx_y0 = 0.0f;              // su altura al empezar
    float vx_mouse_y0 = 0.0f;        // raton al empezar
    bool  vx_drag = false;
    std::vector<float> vx_y0_near;   // alturas del vecindario al agarrar
    std::set<int> vx_sel;                        // vertices seleccionados (j*lado+i)
    std::vector<std::pair<int,float>> vx_sel_y0; // sus alturas al empezar a mover
    bool   vx_box = false;                       // arrastrando el rectangulo
    ImVec2 vx_box_a;
    float  vx_snap = 0.0f;                       // paso de altura (0 = libre)
    bool  show_fps = false;   // el juego generado muestra fps y coste en pantalla
    // --- sol y ciclo dia/noche ---
    bool  sun_cycle = false;      // el sol se mueve solo
    float sun_day_sec = 120.0f;   // segundos que dura un dia entero
    float sun_hour = 90.0f;       // hora de partida, en grados (90 = mediodia)
    float sun_intensity = 1.5f;   // intensidad al mediodia
    float sun_azim = 215.0f;      // rumbo del sol cuando el ciclo esta parado
    float sun_elev = 55.0f;       // altura sobre el horizonte, en grados
    // --- sembrado de vegetacion ---
    float sc_density = 6.0f;      // ejemplares por pincelada
    float sc_scale_min = 0.8f, sc_scale_max = 1.4f;
    float sc_slope_max = 0.6f;    // pendiente maxima donde puede crecer
    float sc_ymin = -1000.0f, sc_ymax = 1000.0f;
    bool  sc_avoid_water = true;
    float sc_wind = 0.0f;          // balanceo de LA ESPECIE que estas sembrando
    float sc_dist = 250.0f;        // a que distancia deja de dibujarse esa especie
    float sc_lod  = 120.0f;        // a partir de aqui, malla de bajo poligono
    int   sc_mode = 0;             // 0 = sembrar, 1 = borrar, 2 = editar uno
    bool  sc_solid = false;        // la especie que siembras bloquea el paso
    int   sc_sel_k = -1, sc_sel_i = -1;   // ejemplar seleccionado
    bool  sc_erase = false;
    float sc_radius = 12.0f;
    // --- erosion hidraulica ---
    int   ero_iters = 120;
    float ero_rain = 0.012f, ero_evap = 0.020f, ero_cap = 0.60f;
    float ero_dis = 0.30f, ero_dep = 0.30f, ero_slope = 0.02f, ero_talus = 0.0f;
    int    vx_mode = 0;                          // 0 = mover vertice, 1 = seleccionar zona
    int    vx_axis = 0;                          // 0 = altura, 1 = lateral (XZ)
    float  vx_lat  = 0.45f;                      // tope lateral, en fraccion de celda
    float  vx_grab_x = 0.0f, vx_grab_z = 0.0f;   // punto del terreno al agarrar
    std::vector<std::pair<int,std::pair<float,float>>> vx_sel_xz0;
    ImGuizmo::OPERATION gizmo_op = ImGuizmo::TRANSLATE;

    // ---- proyecto: navegador de Assets ----
    std::string assets_dir = project_dir + "/Assets";
    /* ---- ENCONTRAR UN ASSET AUNQUE SE HAYA MOVIDO ----
       Las escenas, los menus y los dialogos guardan el nombre del fichero tal
       como estaba ("barrel.glb"). Si luego se ordena el proyecto y pasa a
       Assets/Models/barrel.glb, la referencia vieja seguiria apuntando a un sitio
       vacio. Esto lo resuelve: primero se prueba la ruta guardada y, si no esta,
       se busca ese mismo nombre por las subcarpetas. Asi ordenar no rompe nada. */
    std::map<std::string, std::string> asset_cache;   // nombre guardado -> ruta relativa real
    auto ruta_asset = [&](const std::string& nombre) -> std::string {
        if (nombre.empty()) return nombre;
        std::error_code ec;
        if (fs::exists(assets_dir + "/" + nombre, ec)) return nombre;
        auto it = asset_cache.find(nombre);
        if (it != asset_cache.end()) return it->second;
        std::string base = fs::path(nombre).filename().string();
        std::string hallado;
        fs::recursive_directory_iterator rit(assets_dir, fs::directory_options::skip_permission_denied, ec);
        if (!ec) for (; rit != fs::recursive_directory_iterator(); rit.increment(ec)) {
            if (ec) break;
            std::error_code e1;
            if (!rit->is_regular_file(e1)) continue;
            if (rit->path().filename().string() == base) {
                std::error_code e2;
                std::string rel = fs::relative(rit->path(), assets_dir, e2).generic_string();
                if (!e2) { hallado = rel; break; }
            }
        }
        if (!hallado.empty()) asset_cache[nombre] = hallado;
        return hallado.empty() ? nombre : hallado;
    };
    // La siembra guarda rutas relativas al proyecto (portables al juego); el
    // editor corre desde otro sitio, asi que le dice donde esta la raiz.
    g3d_scatter_set_base(project_dir.c_str());
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
    /* ================== REGLAS: cuando pasa algo, que pasa ==================
       "Si el jugador toca la moneda, suma 10 puntos". "Mientras este en la lava,
       pierde vida". "Cuando la vida llegue a 0, llama a mi PROCESS".
       Una regla es un DISPARADOR y una lista de cosas que hacer. Las cosas pueden
       ser codigo tuyo (un .prg) o de las que trae el editor, que es lo que evita
       tener que escribir un PROCESS para sumar diez puntos.
       Objetos 3D y personajes 2D llevan la misma lista: el disparador y las
       acciones son los mismos, y asi hay UN generador y UNA ficha, no dos. */
    struct Accion {
        int tipo = 0;      // 0 tu PROCESS, 1 variable, 2 destruir esto, 3 mostrar un texto,
                           // 4 sonar un sonido
        std::string archivo, proc;                        // 0
        std::string var; int op = 1; float valor = 1.0f;  // 1  (op: 0 poner, 1 sumar, 2 restar)
        std::string texto; float seg = 2.0f;              // 3
        std::string sonido; int vol = 100;                // 4  (volumen 0..128)
        std::string escena;                               // 5  (fichero .scene destino)
        // 6 = cerrar el menu, 7 = salir del juego (no llevan datos)
        std::string menu;                                 // 8  (menu que se abre)
        std::string dialogo;                              // 9  (dialogo que se saca)
        int ranura = 1;                                   // 10 y 11 (que partida)
    };
    struct Regla {
        int evento = 0;    // 0 al empezar, 1 cada frame, 2 acercarse y pulsar,
                           // 3 el jugador lo toca, 4 el jugador entra en la zona,
                           // 5 una variable cumple, 6 cada N segundos
        std::string tecla = "_E", boton;   // evento 2
        float radio = 3.0f;                // eventos 2 y 3
        int   zona = 0;                    // evento 4 (capa pintada)
        std::string var;                   // evento 5
        int   cmp = 4;                     // 0 ==, 1 !=, 2 <, 3 <=, 4 >, 5 >=
        float valor = 1.0f;                // evento 5
        float cada = 1.0f;                 // evento 6 (segundos)
        int   una_vez = 0;                 // solo la primera vez (un cofre, un checkpoint)
        int   mientras = 0;                // 0 = al cumplirse; 1 = todo el rato que se cumpla
        std::vector<Accion> acciones;
    };
    /* Las variables del juego: puntos, vida, llaves... Salen como GLOBAL de
       BennuGD2, asi que el HUD 2D las puede pintar con write_var y tu codigo las
       ve sin hacer nada. Enteras a proposito: es lo que se cuenta. */
    /* ================== MENUS ==================
       Un menu es una lista de opciones y lo que hace cada una: lo mismo que una
       regla, pero disparado desde una pantalla. Vale para el menu principal, para
       la pausa y para las opciones. Los menus son del PROYECTO, no de una escena:
       el de pausa tiene que estar en todas. */
    /* Una opcion de menu puede ser dos cosas: HACER algo al pulsarla (jugar,
       salir, ir a una escena...) o SER UN AJUSTE que se cambia con izquierda y
       derecha y se ve su valor al lado. Los ajustes valen para cualquier juego:
       ademas del volumen y la pantalla, cualquiera puede ajustar UNA VARIABLE DEL
       JUEGO -- dificultad, sensibilidad, vidas, idioma, lo que tenga sentido en el
       tuyo. */
    struct MenuOpc {
        std::string texto = "Opcion";
        std::vector<Accion> acciones;
        int clase = 0;            // 0 = hace algo, 1 = es un ajuste, 2 = una ranura de partida
        int ranura = 1;           // clase 2: que ranura
        int ranura_modo = 0;      // 0 = cargar al pulsar, 1 = guardar al pulsar
        int ajuste = 0;           // 0 musica, 1 sonidos, 2 pantalla completa, 3 una variable
        std::string var;          // si ajuste == 3, cual
        int vmin = 0, vmax = 128, paso = 8;
    };
    struct Menu {
        std::string nombre = "menu";       // nombre del PROCESS que se genera
        int  cuando = 0;                   // 0 al arrancar, 1 con una tecla/boton, 2 solo si lo llama una regla
        std::string tecla = "_ESC", boton = "JOY_BUTTON_START";
        int  x = 640, y = 260, sep = 44;
        std::string fuente;                // .fnt de Assets ("" = la del sistema)
        std::string fondo;                 // grafico de fondo (opcional)
        int  col[4]     = { 200, 200, 200, 255 };
        int  col_sel[4] = { 255, 230, 120, 255 };
        int  con_teclado = 1, con_mando = 1, con_raton = 1;
        std::string snd_mover, snd_elegir;
        int  pausa = 1;                    // congela el juego mientras esta abierto
        std::vector<MenuOpc> opciones;
    };
    std::vector<Menu> menus;
    int menu_sel = -1;

    /* ================== DIALOGOS ==================
       Un dialogo es lo que dice alguien: una lista de paginas, y cada pagina su
       texto (con quien habla y su retrato) y, si quieres, respuestas que llevan a
       otra pagina. El bocadillo es un grafico TUYO -- un PNG suelto o un grafico
       de un FPG -- estirado al tamanio que le des. Son del proyecto, como los
       menus: el mismo bocadillo vale en todas las escenas. */
    struct DlgOpc {
        std::string texto = "Respuesta";
        std::vector<Accion> acciones;
        int salto = -1;                 // -1 = cerrar; >= 0 = ir a esa pagina
    };
    struct DlgPag {
        std::string quien;              // quien habla (vacio = nadie)
        std::string texto = "...";
        std::string retrato;            // imagen o FPG del retrato (opcional)
        int  retrato_graf = 1;          // si el retrato es un FPG, que grafico
        /* Si el retrato es una HOJA con varias caras, cual de ellas. -1 = la
           imagen entera (una cara por fichero, como era antes). */
        int  retrato_cara = -1;
        std::vector<DlgOpc> opciones;   // si tiene, la pagina es una pregunta
    };
    struct Dialogo {
        std::string nombre = "dialogo";
        std::string caja;               // PNG o FPG del bocadillo ("" = sin caja)
        int  caja_graf = 1;             // si la caja es un FPG, que grafico
        int  cx = 640, cy = 560;        // centro del bocadillo
        int  cw = 900, ch = 200;        // y a que tamanio se estira
        std::string fuente;             // .fnt ("" = la del sistema)
        int  col[4]        = { 235, 235, 235, 255 };
        int  col_nombre[4] = { 255, 220, 120, 255 };
        int  mx = 28, my = 24;          // margenes del texto dentro de la caja
        int  vel = 40;                  // letras por segundo (0 = de golpe)
        std::string snd_letra, snd_pasar;
        std::string tecla = "_SPACE", boton = "JOY_BUTTON_A";
        int  pausa = 1;                 // congela el juego mientras habla
        std::vector<DlgPag> paginas;
    };
    std::vector<Dialogo> dialogos;
    int dlg_sel = -1;

    /* Una variable del juego puede entrar o no en la partida guardada: los puntos
       y la vida si, pero un contador temporal o un ajuste no tienen por que. */
    struct GameVar { std::string nombre; int valor = 0; bool guardar = true; };

    /* ================== GUARDAR PARTIDA ==================
       Que se guarda, en cuantas ranuras y con que nombre lo eliges tu: el editor
       no sabe si estas haciendo un RPG, un plataformas o un juego de coches, asi
       que no decide por ti que es "el progreso". */
    struct Guardado {
        int  ranuras = 3;                 // cuantas partidas caben
        std::string fichero = "partida";  // partida1.sav, partida2.sav...
        bool con_vars = true;             // las variables marcadas
        bool con_escena = true;           // en que escena estabas
        bool con_jugador = true;          // donde estabas y hacia donde mirabas
        bool con_reglas = true;           // las reglas ya cumplidas (puertas abiertas)
    };
    Guardado guardado;
    std::vector<GameVar> gvars;
    struct SObj {
        std::string name, asset; int entity; float x, y, z, ry, scale;
        std::vector<Regla> reglas;
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
        // sonido propio: se oye mas fuerte cuanto mas cerca estas (cascada, fuego)
        std::string amb_sonido;
        float amb_radio = 20.0f;
        int   amb_vol = 100;            // 0..128 pegado a el
    };
    std::vector<SObj> objects;
    int obj_sel = -1;

    // ---- HUD 2D: los graficos y los textos de BennuGD2 encima de la escena ----
    // Cada elemento acaba siendo un PROCESS de BennuGD2 con SUS LOCALES
    // (file/graph/x/y/z/size/angle/flags/alpha) o un write() con una fuente
    // .fnt. Aqui solo se guarda que hay y donde, y se dibuja igual que lo hara
    // el juego para poder colocarlo a ojo.
    // La pantalla del juego es la que monta escena_iniciar: set_mode(1280,720).
    const float HUD_W = 1280.0f, HUD_H = 720.0f;
    struct HudItem {
        int type = 0;                 // 0 = grafico, 1 = texto
        std::string name;             // nombre del PROCESS generado
        // --- grafico ---
        std::string asset;            // fichero de Assets (.png/.jpg o .fpg/.f16/.f32)
        int   code = 0;               // grafico dentro del FPG (el que va en graph)
        float size = 100.0f;          // local size (100 = tamano real)
        float angle = 0.0f;           // grados; el .prg los pasa a milesimas
        int   flags = 0;              // 1 = espejo horizontal, 2 = vertical, 3 = los dos
        // --- texto ---
        std::string font;             // .fnt/.fnx de Assets ("" = fuente 0 del sistema)
        std::string text = "texto";   // texto fijo
        std::string var;              // GLOBAL a mostrar con write_var ("" = texto fijo)
        int   vartype = 0;            // 0 = int, 1 = float, 2 = string
        int   align = 0;              // ALIGN_* (0 = arriba-izquierda)
        // --- comunes ---
        float x = 40.0f, y = 40.0f;   // en pixeles de la pantalla del juego
        int   z = -100;               // el 3D se dibuja el ultimo por detras: aqui solo ordena el HUD
        int   alpha = 255;
        int   col[4] = { 255, 255, 255, 255 };   // color del texto (write_set_rgba)
    };
    std::vector<HudItem> hud;
    int  hud_sel = -1;
    bool hud_show = true;             // ver el HUD encima del viewport
    bool hud_drag = false;            // arrastrando un elemento con el raton
    float hud_grab_x = 0.0f, hud_grab_y = 0.0f;
    char hud_fpg_filter[64] = "";     // filtro del selector visual de graficos del FPG

    // ---- HOJAS DE SPRITES (personajes 2D para el mundo 3D, estilo HD-2D) ----
    // Se carga un PNG con todos los fotogramas y el editor los detecta solo por
    // los huecos transparentes; luego se agrupan en animaciones. Cada fotograma
    // guarda su ancla (centro-abajo de su banda), que es lo que hace que el
    // personaje no baile al animar cuando los recortes son de distinto tamano.
    struct SprFrame { int x, y, w, h, ax, ay; int band = 0; };
    struct SprAnim  { std::string name; int fps = 10; std::vector<int> frames; };
    struct SheetDef {
        std::string image;              // fichero dentro de Assets
        int w = 0, h = 0;               // tamano de la hoja
        int cols = 0, rows = 0;         // rejilla, si la hoja es regular
        std::vector<SprFrame> frames;
        std::vector<SprAnim>  anims;
    };
    SheetDef sheet;
    // Todas las hojas que hagan falta (cada sprite colocado usa la suya). La
    // ABIERTA no se lee de aqui: manda 'sheet', que es la que estas editando.
    std::map<std::string, SheetDef> sheet_cache;
    std::vector<int> sheet_sel;         // fotogramas seleccionados (para hacer animaciones)
    float sheet_zoom = 2.0f;
    int   sheet_anim_sel = -1;
    float sheet_anim_t = 0.0f;          // reloj del previo de la animacion
    char  sheet_anim_name[64] = "andar";
    int   sheet_grid_cols = 0, sheet_grid_rows = 0;   // rejilla a mano
    int   sheet_split_n = 4;      // partir una animacion en trozos de N fotogramas
    bool  sheet_limpiar = true;   // quitar los creditos del rip y juntar trozos sueltos
    int   sheet_frame_split = 2;  // partir un fotograma en N (dos personajes pegados)
    // Recorte a mano: celda, margen y separacion, como en cualquier programa de
    // sprites. Se ve encima de la hoja antes de aplicarlo.
    int   sheet_cell_w = 32, sheet_cell_h = 32;
    int   sheet_off_x = 0, sheet_off_y = 0;
    int   sheet_gap_x = 0, sheet_gap_y = 0;
    bool  sheet_ver_rejilla = false;   // dibujar la rejilla encima de la hoja
    bool  sheet_dibujar = false;       // dibujar un fotograma arrastrando el raton
    ImVec2 sheet_drag_ini(0, 0);
    bool  sheet_drag_on = false;
    std::string sheet_msg;
    bool sheet_refrescar = false;   // se ha creado una hoja sin fondo: releer Assets
    bool sheet_quitar_fondo = true; // los rips con el fondo pintado -> copia transparente
    bool show_spr_win = false;      // la ventana de sprites 3D (flotante, no acoplada)
    size_t sheet_firma_guardada = 0; // como estaba la hoja la ultima vez que se guardo
    // Las animaciones las montas TU. El agrupado automatico esta ahi por si
    // quieres un punto de partida, pero apagado: llenaba la lista de fila1_1,
    // fila1_2... y tapaba las tuyas.
    bool sheet_auto_anims = false;

    // Fichero .sheet junto al PNG: lo escribe el editor y lo lee la generacion
    // del juego (fotogramas + animaciones).
    auto hud_is_fpg = [](const std::string& f) {
        std::string s = f; for (auto& c : s) c = (char)tolower(c);
        if (s.size() < 4) return false;
        std::string e = s.substr(s.size() - 4);
        return e == ".fpg" || e == ".f16" || e == ".f32";
    };
    auto sheet_path_of = [&](const std::string& img) {
        // el .sheet va pegado a su imagen: si la hoja esta en Sprites/, el .sheet
        // tambien, y si la referencia guardada era el nombre suelto, se resuelve
        std::string r = ruta_asset(img);
        return assets_dir + "/" + r.substr(0, r.rfind('.')) + ".sheet";
    };
    auto sheet_save = [&]() {
        if (sheet.image.empty()) return;
        FILE* f = fopen(sheet_path_of(sheet.image).c_str(), "w");
        if (!f) { sheet_msg = "ERROR: no puedo guardar la hoja"; return; }
        fprintf(f, "BGD2SHEET 1\nimage=%s\nsize=%d %d\ngrid=%d %d\n",
                sheet.image.c_str(), sheet.w, sheet.h, sheet.cols, sheet.rows);
        for (auto& fr : sheet.frames)
            fprintf(f, "frame=%d %d %d %d %d %d %d\n", fr.x, fr.y, fr.w, fr.h, fr.ax, fr.ay, fr.band);
        for (auto& an : sheet.anims) {
            fprintf(f, "anim=%s %d %d", an.name.c_str(), an.fps, (int)an.frames.size());
            for (int k : an.frames) fprintf(f, " %d", k);
            fputc('\n', f);
        }
        fclose(f);
        sheet_msg = "Hoja guardada en " + fs::path(sheet_path_of(sheet.image)).filename().string();
    };
    auto sheet_load_file = [&](const std::string& img) -> bool {
        FILE* f = fopen(sheet_path_of(img).c_str(), "r");
        if (!f) return false;
        sheet = SheetDef(); sheet.image = img;
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            int a, b, c, d, e, g;
            if (sscanf(line, "size=%d %d", &a, &b) == 2) { sheet.w = a; sheet.h = b; continue; }
            if (sscanf(line, "grid=%d %d", &a, &b) == 2) { sheet.cols = a; sheet.rows = b; continue; }
            {   int bd = 0;
                int nf = sscanf(line, "frame=%d %d %d %d %d %d %d", &a,&b,&c,&d,&e,&g,&bd);
                if (nf >= 6) { sheet.frames.push_back({ a,b,c,d,e,g,bd }); continue; }
            }
            if (!strncmp(line, "anim=", 5)) {
                char nom[128]; int fps = 10, n = 0, pos = 0;
                if (sscanf(line + 5, "%127s %d %d%n", nom, &fps, &n, &pos) >= 3) {
                    SprAnim an; an.name = nom; an.fps = fps;
                    const char* p2 = line + 5 + pos;
                    for (int k = 0; k < n; k++) {
                        int fr; int used = 0;
                        if (sscanf(p2, " %d%n", &fr, &used) != 1) break;
                        an.frames.push_back(fr); p2 += used;
                    }
                    sheet.anims.push_back(an);
                }
                continue;
            }
        }
        fclose(f);
        return !sheet.frames.empty();
    };
    // Detectar los fotogramas de la hoja (los huecos transparentes mandan).
    auto sheet_detect = [&](const std::string& img_orig) {
        std::string img = img_orig;
        // Los rips traen el fondo PINTADO (blanco, magenta, un panel de color...).
        // Si se deja asi, en el juego el personaje sale con su recuadro, porque el
        // recorte alfa no tiene nada que recortar. Se hace una copia con el fondo
        // en transparente y se trabaja con ella; el original no se toca.
        if (sheet_quitar_fondo) {
            std::string base = img.substr(0, img.rfind('.'));
            if (base.size() < 9 || base.substr(base.size() - 9) != "_sinfondo") {
                std::string real  = ruta_asset(img);
                std::string nuevo = real.substr(0, real.rfind('.')) + "_sinfondo.png";
                int r2 = h2_make_transparent((assets_dir + "/" + real).c_str(),
                                             (assets_dir + "/" + nuevo).c_str());
                if (r2 == 1) { img = nuevo; sheet_refrescar = true; }
            }
        }
        sheet = SheetDef(); sheet.image = img;
        sheet_sel.clear(); sheet_anim_sel = -1;
        std::vector<H2Rect> r(4096);
        int sw = 0, sh = 0;
        int n = h2_detect_frames((assets_dir + "/" + ruta_asset(img)).c_str(), r.data(), (int)r.size(),
                                 &sw, &sh, sheet_limpiar ? 1 : 0);
        sheet.w = sw; sheet.h = sh;
        for (int i = 0; i < n; i++)
            sheet.frames.push_back({ r[i].x, r[i].y, r[i].w, r[i].h, r[i].ax, r[i].ay, r[i].band });
        int c = 0, rr = 0;
        if (n > 0 && h2_guess_grid(r.data(), n, sw, sh, &c, &rr)) { sheet.cols = c; sheet.rows = rr; }
        sheet_grid_cols = sheet.cols; sheet_grid_rows = sheet.rows;

        // ---- y las ANIMACIONES, si las quieres automaticas ----
        // Por defecto NO: la lista se llenaba de fila1_1, fila1_2... y no habia
        // quien encontrara las tuyas. Con la casilla puesta salen como punto de
        // partida y luego las renombras o las rehaces.
        // Una hoja se lee por filas: cada banda es un grupo de poses. Dentro de la
        // banda, una animacion es una racha de fotogramas de tamano parecido; en
        // cuanto el tamano pega un salto (un ataque con lanza mide el doble que
        // andar) empieza otra.
        sheet.anims.clear();
        if (sheet_auto_anims && !sheet.frames.empty()) {
            int nb = 0;
            for (auto& fr : sheet.frames) if (fr.band + 1 > nb) nb = fr.band + 1;
            for (int b = 0; b < nb; b++) {
                std::vector<int> banda;
                for (int i = 0; i < (int)sheet.frames.size(); i++)
                    if (sheet.frames[i].band == b) banda.push_back(i);
                if (banda.empty()) continue;
                std::vector<std::vector<int>> grupos;
                std::vector<int> act;
                for (size_t k = 0; k < banda.size(); k++) {
                    const SprFrame& f2 = sheet.frames[banda[k]];
                    if (!act.empty()) {
                        const SprFrame& pv = sheet.frames[act.back()];
                        int tw = pv.w / 4 > 3 ? pv.w / 4 : 3;      // 25% (minimo 3 px)
                        int th = pv.h / 4 > 3 ? pv.h / 4 : 3;
                        if (abs(f2.w - pv.w) > tw || abs(f2.h - pv.h) > th) {
                            grupos.push_back(act); act.clear();
                        }
                    }
                    act.push_back(banda[k]);
                }
                if (!act.empty()) grupos.push_back(act);
                // Los grupos de UN fotograma seguidos son casi siempre la misma
                // secuencia (un ataque donde cada pose mide distinto porque el arma
                // sale mas o menos), no animaciones de un fotograma: se juntan.
                {
                    std::vector<std::vector<int>> fus;
                    for (auto& g2 : grupos) {
                        if (g2.size() == 1 && !fus.empty() && fus.back().size() <= 2 &&
                            fus.back().size() + 1 <= 8)
                            fus.back().push_back(g2[0]);
                        else
                            fus.push_back(g2);
                    }
                    grupos.swap(fus);
                }
                // En una rejilla regular cada fila es UNA animacion entera (esa es
                // la disposicion de siempre), asi que ahi no se parte por tamano.
                if (sheet.cols > 0 && sheet.rows > 0) { grupos.clear(); grupos.push_back(banda); }
                for (size_t gi = 0; gi < grupos.size(); gi++) {
                    SprAnim an;
                    an.name = "fila" + std::to_string(b + 1) +
                              (grupos.size() > 1 ? "_" + std::to_string((int)gi + 1) : "");
                    an.fps = 10;
                    an.frames = grupos[gi];
                    sheet.anims.push_back(an);
                }
            }
            // En rejilla, una animacion mas con TODAS las filas seguidas: es lo que
            // pide el sprite cuando tiene varias posturas (4, 8 o 16 direcciones),
            // que espera los fotogramas por direcciones, una detras de otra.
            if (sheet.cols > 0 && sheet.rows > 1) {
                SprAnim an; an.name = "todas_las_direcciones"; an.fps = 10;
                for (int i = 0; i < (int)sheet.frames.size(); i++) an.frames.push_back(i);
                sheet.anims.push_back(an);
            }
        }
        sheet_msg = std::to_string(n) + " fotogramas" +
                    (sheet.cols ? "  (rejilla " + std::to_string(sheet.cols) + "x" +
                                  std::to_string(sheet.rows) + ")"
                                : "  (hoja irregular)") +
                    "  y " + std::to_string((int)sheet.anims.size()) + " animaciones";
        if (img != img_orig)
            sheet_msg += "\nFondo quitado: se trabaja con " + img + " (el original se queda igual).";
        sheet_cache[sheet.image] = sheet;   // que los sprites ya colocados vean esto
    };
    // Rehacer los fotogramas como rejilla regular, cuando la deteccion no es lo
    // que quieres (la hoja partida en columnas x filas de celdas fijas).
    auto sheet_make_grid = [&](int cols, int rows) {
        if (cols < 1 || rows < 1 || sheet.w <= 0) return;
        sheet.frames.clear(); sheet_sel.clear();
        int cw = sheet.w / cols, ch = sheet.h / rows;
        for (int r2 = 0; r2 < rows; r2++)
            for (int c2 = 0; c2 < cols; c2++)
                sheet.frames.push_back({ c2 * cw, r2 * ch, cw, ch, cw / 2, ch - 1 });
        sheet.cols = cols; sheet.rows = rows;
        for (int r2 = 0; r2 < rows; r2++)
            for (int c2 = 0; c2 < cols; c2++) sheet.frames[r2 * cols + c2].band = r2;
        sheet.anims.clear();
        if (sheet_auto_anims) {      // una animacion por fila, y otra con todas
            for (int r2 = 0; r2 < rows; r2++) {
                SprAnim an; an.name = "fila" + std::to_string(r2 + 1); an.fps = 10;
                for (int c2 = 0; c2 < cols; c2++) an.frames.push_back(r2 * cols + c2);
                sheet.anims.push_back(an);
            }
            if (rows > 1) {
                SprAnim an; an.name = "todas_las_direcciones"; an.fps = 10;
                for (int i = 0; i < cols * rows; i++) an.frames.push_back(i);
                sheet.anims.push_back(an);
            }
        }
        sheet_msg = std::to_string(cols * rows) + " fotogramas (rejilla a mano " +
                    std::to_string(cols) + "x" + std::to_string(rows) + ") y " +
                    std::to_string((int)sheet.anims.size()) + " animaciones";
    };

    // Al tocar los fotogramas (partir, unir, borrar) los NUMEROS cambian. Antes se
    // vaciaban las animaciones y perdias el trabajo; ahora se renumeran: mapa[i] es
    // el numero nuevo del fotograma i, o -1 si ha desaparecido.
    auto sheet_remap = [&](const std::vector<int>& mapa) {
        for (auto& an : sheet.anims) {
            std::vector<int> nu;
            for (int fr : an.frames) {
                if (fr < 0 || fr >= (int)mapa.size()) continue;
                int n2 = mapa[fr];
                if (n2 < 0) continue;
                if (!nu.empty() && nu.back() == n2) continue;   // no repetir seguidos
                nu.push_back(n2);
            }
            an.frames.swap(nu);
        }
    };

    // La hoja de una imagen: del .sheet guardado, o detectandola al vuelo.
    // Abrir una hoja para trabajar: manda SIEMPRE lo que haya guardado en su
    // .sheet (fotogramas y animaciones que has hecho tu), y solo se detecta si no
    // hay nada. Se mira tambien el .sheet de la copia sin fondo, que es con la que
    // se trabaja de verdad -- si no, elegir el original en la lista re-detectaba y
    // se llevaba por delante las animaciones.
    auto sheet_open = [&](const std::string& img) -> const char* {
        // Un FPG tambien vale como hoja: se junta en un PNG con transparencia
        // (una vez) y a partir de ahi es una hoja normal. Los fotogramas y sus
        // anclas salen del propio FPG -- su punto de control 0, que en un FPG de
        // personajes suele estar en los pies.
        if (hud_is_fpg(img)) {
            // el PNG que sale del FPG se queda en la carpeta del FPG
            std::string real2 = ruta_asset(img);
            std::string base2 = real2.substr(0, real2.rfind('.'));
            std::string png = base2 + "_hoja.png";
            std::string rpng = assets_dir + "/" + png;
            std::vector<H2Rect> r(1024);
            int sw = 0, sh = 0, n = 0;
            if (!fs::exists(rpng))
                n = h2_fpg_to_sheet((assets_dir + "/" + real2).c_str(), rpng.c_str(),
                                    r.data(), (int)r.size(), &sw, &sh);
            if (n > 0) {
                sheet = SheetDef(); sheet.image = png; sheet.w = sw; sheet.h = sh;
                for (int i = 0; i < n; i++)
                    sheet.frames.push_back({ r[i].x, r[i].y, r[i].w, r[i].h,
                                             r[i].ax, r[i].ay, r[i].band });
                sheet_refrescar = true;
                sheet_msg = std::to_string(n) + " graficos del FPG juntados en " + png;
                return "detectada";
            }
            if (fs::exists(rpng)) {           // ya estaba hecha: se abre esa
                if (sheet_load_file(png)) return "guardada";
                sheet_detect(png);
                return "detectada";
            }
            return "detectada";
        }
        std::string base = img.substr(0, img.rfind('.'));
        bool ya_sin = (base.size() >= 9 && base.substr(base.size() - 9) == "_sinfondo");
        if (!ya_sin) {
            std::string sinf = base + "_sinfondo.png";
            if (fs::exists(assets_dir + "/" + ruta_asset(sinf)) && sheet_load_file(sinf)) return "guardada";
        }
        if (sheet_load_file(img)) return "guardada";
        sheet_detect(img);
        return "detectada";
    };

    // Resumen de como esta la hoja ahora mismo. Comparandolo con el del ultimo
    // guardado se sabe si hay cambios, sin marcarlos en cada sitio que la toca.
    auto sheet_firma = [&]() {
        size_t h = 1469598103934665603ull;
        auto mez = [&](long long v) { h ^= (size_t)v; h *= 1099511628211ull; };
        for (auto& f2 : sheet.frames) {
            mez(f2.x); mez(f2.y); mez(f2.w); mez(f2.h); mez(f2.ax); mez(f2.ay);
        }
        for (auto& a2 : sheet.anims) {
            for (char c : a2.name) mez(c);
            mez(a2.fps);
            for (int k : a2.frames) mez(k);
        }
        return h;
    };

    auto sheet_of = [&](const std::string& img) -> SheetDef* {
        if (img.empty()) return nullptr;
        // La hoja ABIERTA manda siempre: es la que estas editando. Antes se
        // devolvia la copia de la cache (la de cuando colocaste el sprite), y por
        // eso las animaciones que creabas a mano no salian en sus desplegables.
        if (img == sheet.image && !sheet.frames.empty()) return &sheet;
        auto it = sheet_cache.find(img);
        if (it != sheet_cache.end()) return &it->second;
        SheetDef d; d.image = img;
        // se reaprovecha la logica de arriba tirando de una copia temporal
        SheetDef guardar = sheet;
        sheet_open(img);
        d = sheet;
        sheet = guardar;
        return &(sheet_cache[img] = d);
    };

    // Los recursos se leen una vez y se quedan cacheados por nombre de fichero.
    std::map<std::string, H2Img>  hud_imgs;
    std::map<std::string, H2Fpg>  hud_fpgs;
    std::map<std::string, H2Font> hud_fonts;
    auto hud_scan_gfx  = [&] { return scan_dir(assets_dir, { ".png",".jpg",".jpeg",".bmp",".tga",".fpg",".f16",".f32" }); };
    auto hud_scan_fnt  = [&] { return scan_dir(assets_dir, { ".fnt",".fnx" }); };
    std::vector<std::string> hud_gfx_files  = hud_scan_gfx();
    std::vector<std::string> hud_font_files = hud_scan_fnt();
    auto hud_img = [&](const std::string& file) -> H2Img* {
        auto it = hud_imgs.find(file);
        if (it == hud_imgs.end()) {
            H2Img im = {};
            h2_load_image((assets_dir + "/" + ruta_asset(file)).c_str(), &im);
            it = hud_imgs.emplace(file, im).first;
        }
        return it->second.tex ? &it->second : nullptr;
    };
    auto hud_fpg = [&](const std::string& file) -> H2Fpg* {
        auto it = hud_fpgs.find(file);
        if (it == hud_fpgs.end()) {
            H2Fpg fp = {};
            h2_load_fpg((assets_dir + "/" + ruta_asset(file)).c_str(), &fp);
            it = hud_fpgs.emplace(file, fp).first;
        }
        return it->second.n ? &it->second : nullptr;
    };
    auto hud_font = [&](const std::string& file) -> H2Font* {
        auto it = hud_fonts.find(file);
        if (it == hud_fonts.end()) {
            H2Font ft = {};
            h2_load_fnt((assets_dir + "/" + ruta_asset(file)).c_str(), &ft);
            it = hud_fonts.emplace(file, ft).first;
        }
        return it->second.tex ? &it->second : nullptr;
    };
    // El grafico de un elemento: suelto (map_load) o uno de dentro del FPG.
    auto hud_item_img = [&](const HudItem& h) -> H2Img* {
        if (h.type != 0 || h.asset.empty()) return nullptr;
        if (!hud_is_fpg(h.asset)) return hud_img(h.asset);
        H2Fpg* fp = hud_fpg(h.asset);
        if (!fp) return nullptr;
        for (int i = 0; i < fp->n; i++) if (fp->g[i].code == h.code) return &fp->g[i].img;
        return nullptr;
    };
    // ---- SPRITES colocados en la escena ----
    // Cada uno acabara siendo un PROCESS con csubtype = C3D_SPRITE.
    // Una accion del personaje: se dispara con una tecla, con un boton del mando o
    // con los dos, y hace sonar una animacion, llamar a codigo tuyo, o ambas.
    struct SprAccion {
        std::string nombre = "accion";
        std::string tecla;        // vacia = sin tecla
        std::string boton;        // vacio = sin boton del mando
        std::string anim;         // animacion de la hoja (opcional)
        int espejo = 0;
        int una_vez = 1;          // 1 = suena entera al pulsar; 0 = mientras la aguantes
        std::string llama;        // PROCESS o FUNCTION tuyo que se llama al dispararla
        std::string archivo;      // el .prg de Scripts donde vive (vacio: lo crea el editor)
        std::string dialogo;      // o un dialogo, para que salga con su bocadillo
    };
    struct SprObj {
        std::string name;          // nombre del PROCESS generado
        std::string sheet;         // imagen (hoja) dentro de Assets
        std::string anim;          // animacion que reproduce ("" = primer fotograma)
        float x = 0, y = 0, z = 0; // posicion en el mundo
        float height = 2.5f;       // alto en unidades del fotograma mas alto
        int   dirs = 1;            // posturas segun la camara (1, 4, 8, 16)
        int   billboard = 0;       // 0 = de pie, 1 = de cara del todo
        int   shadow = 1;
        int   smooth = 0;
        float cutout = 0.5f;
        int   entity = -1;         // sprite del motor (para verlo en el editor)
        float t = 0.0f;            // reloj de la animacion en el editor
        // ---- comportamiento: lo mismo que un objeto 3D ----
        // fisica: 0 = ninguna (decorativo), 1 = caja, 2 = esfera, 3 = capsula,
        //         4 = cilindro, 5 = muro invisible (solo colisiona, no cae)
        int   phys = 0;
        float mass = 1.0f, bounce = 0.2f, friction = 0.5f, csize = 1.0f, density = 0.5f;
        int   buoyant = 0;
        // jugador: char controller con sus teclas y sus animaciones por estado
        int   is_player = 0;
        float walk_speed = 12.0f, run_speed = 26.0f, jump_force = 13.0f;
        float char_radius = 0.6f, char_height = 3.0f;
        int   zone_layer = -1;          // capa de barrera que le corta el paso
        std::string an_idle, an_walk, an_run, an_jump;   // animaciones por estado
        std::string k_up = "_W", k_down = "_S", k_left = "_A", k_right = "_D";
        std::string k_jump = "_SPACE", k_run = "_L_SHIFT";
        // Animacion POR TECLA: si la pones, manda sobre la de estado mientras esa
        // tecla este pulsada (asi se hace un juego 2D de toda la vida: una
        // animacion para cada direccion). Vacia = se usa la del estado.
        std::string an_up, an_down, an_left, an_right;
        // Espejo por tecla: con una sola animacion "hacia la izquierda" se
        // resuelve la derecha marcando esto (el motor la voltea con flags = 1).
        int fx_up = 0, fx_down = 0, fx_left = 0, fx_right = 0;
        // Mando: moverse con el stick/cruceta y botones para saltar y correr.
        // Velocidad de la animacion de ESTE personaje. 0 = la que traiga la
        // animacion en la hoja; con otro valor, manda este.
        int fps = 0;
        int iluminado = 1;         // le afecta la luz de la escena (se apaga de noche)
        int ajuste_px = 1;         // se ajusta a pixeles de pantalla (no tiembla)
        int usar_mando = 0;
        std::string b_jump, b_run;
        // Y todas las acciones que quieras (atacar, cubrirse, hablar...).
        std::vector<SprAccion> acciones;
        // ---- NPC: estorbar e interactuar ----
        // 'solido' le pone una caja de colision que le SIGUE (los muros normales
        // son fijos), para que corte el paso aunque patrulle.
        int   solido = 0;
        float sol_radio = 0.6f;
        // Interaccion por cercania: el clasico "acercate y pulsa E para hablar".
        int   inter_on = 0;
        float inter_radio = 3.0f;
        std::string inter_tecla = "_E", inter_boton = "JOY_BUTTON_A";
        std::string inter_anim, inter_llama, inter_arch;
        std::string inter_dialogo;  // el dialogo que suelta al hablarle (con su bocadillo)
        std::vector<Regla> reglas;      // "si pasa esto, haz esto" (lo mismo que un objeto)
        int   inter_mirar = 1;      // se gira hacia el jugador al acercarse
        // ---- comportamiento: que hace cuando nadie le toca ----
        // 0 = quieto, 1 = patrulla entre A y B, 2 = sigue al jugador, 3 = huye
        int   comport = 0;
        float com_vel = 3.0f;       // unidades por segundo
        float com_radio = 12.0f;    // a partir de que distancia se entera (seguir/huir)
        float com_bx = 0.0f, com_bz = 0.0f;   // el punto B de la patrulla
    };
    std::vector<SprObj> sprites;
    int spr_sel = -1;
    int spr_follow = -1;          // sprite al que sigue la camara (-1 = ninguno)
    // Teclas que se pueden elegir para los controles (constantes de BennuGD2).
    // Todas las de BennuGD2 (libbginput), no un punado: si quieres la K para el
    // ataque y la L para bloquear, tienen que estar.
    static const char* TECLAS[] = {
        "_W", "_A", "_S", "_D", "_UP", "_DOWN", "_LEFT", "_RIGHT",
        "_SPACE", "_ENTER", "_TAB", "_ESC", "_BACKSPACE",
        "_L_SHIFT", "_R_SHIFT", "_L_CONTROL", "_R_CONTROL", "_L_ALT", "_R_ALT",
        "_Q", "_E", "_R", "_T", "_Y", "_U", "_I", "_O", "_P",
        "_F", "_G", "_H", "_J", "_K", "_L",
        "_Z", "_X", "_C", "_V", "_B", "_N", "_M",
        "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9", "_0",
        "_F1", "_F2", "_F3", "_F4", "_F5", "_F6", "_F7", "_F8",
        "_F9", "_F10", "_F11", "_F12",
        "_HOME", "_END", "_PGUP", "_PGDN", "_INS", "_DEL",
        "_COMMA", "_POINT", "_MINUS", "_PLUS", "_SEMICOLON", "_APOSTROPHE"
    };
    const int NTECLAS = (int)(sizeof(TECLAS) / sizeof(TECLAS[0]));
    // Botones del mando, con los nombres de BennuGD2 (JOY_BUTTON_*).
    static const char* BOTONES[] = {
        "JOY_BUTTON_A", "JOY_BUTTON_B", "JOY_BUTTON_X", "JOY_BUTTON_Y",
        "JOY_BUTTON_LEFTSHOULDER", "JOY_BUTTON_RIGHTSHOULDER",
        "JOY_BUTTON_BACK", "JOY_BUTTON_START", "JOY_BUTTON_GUIDE",
        "JOY_BUTTON_LEFTSTICK", "JOY_BUTTON_RIGHTSTICK",
        "JOY_BUTTON_DPAD_UP", "JOY_BUTTON_DPAD_DOWN",
        "JOY_BUTTON_DPAD_LEFT", "JOY_BUTTON_DPAD_RIGHT"
    };
    const int NBOTONES = (int)(sizeof(BOTONES) / sizeof(BOTONES[0]));

    /* Elegir tecla y boton del mando. Los usan los personajes 2D y tambien los
       objetos 3D, asi que viven aqui y no dentro de una ficha. */
    auto combo_tecla_ui = [&](const char* et, std::string& tecla) {
        if (ImGui::BeginCombo(et, tecla.c_str())) {
            // Con 70 teclas en la lista, un filtro ahorra el scroll.
            static char filtro[16] = "";
            ImGui::SetNextItemWidth(90);
            ImGui::InputTextWithHint("##ft", "filtrar", filtro, sizeof(filtro));
            for (int k = 0; k < NTECLAS; k++) {
                if (filtro[0]) {
                    bool cuadra = false;
                    for (const char* a = TECLAS[k]; *a && !cuadra; a++) {
                        const char *x = a, *y = filtro;
                        while (*x && *y && toupper((unsigned char)*x) == toupper((unsigned char)*y)) { x++; y++; }
                        if (!*y) cuadra = true;
                    }
                    if (!cuadra) continue;
                }
                if (ImGui::Selectable(TECLAS[k], tecla == TECLAS[k])) tecla = TECLAS[k];
            }
            ImGui::EndCombo();
        }
    };
    auto combo_boton_ui = [&](const char* et, std::string& b) {
        const char* cur = b.empty() ? "(ninguno)" : b.c_str();
        ImGui::SetNextItemWidth(180);
        if (ImGui::BeginCombo(et, cur)) {
            if (ImGui::Selectable("(ninguno)", b.empty())) b.clear();
            for (int k = 0; k < NBOTONES; k++)
                if (ImGui::Selectable(BOTONES[k], b == BOTONES[k])) b = BOTONES[k];
            ImGui::EndCombo();
        }
    };

    // Refresca en el motor un sprite colocado, para verlo en el viewport igual
    // que se vera en el juego (mismo dibujante).
    auto spr_sync = [&](SprObj& o, float dt) {
        SheetDef* sh = sheet_of(o.sheet);
        H2Img* im = o.sheet.empty() ? nullptr : hud_img(o.sheet);
        if (!sh || !im || sh->frames.empty()) {
            if (o.entity >= 0) g3d_sprite_set_visible(o.entity, 0);
            return;
        }
        if (o.entity < 0) o.entity = g3d_sprite_create(scene, o.x, o.y, o.z);
        if (o.entity < 0) return;
        g3d_sprite_set_visible(o.entity, 1);
        g3d_sprite_set_position(o.entity, o.x, o.y, o.z);
        g3d_sprite_set_texture(o.entity, im->tex, im->w, im->h, 1.0f, 1.0f);
        g3d_sprite_set_billboard(o.entity, o.billboard);
        g3d_sprite_set_cutout(o.entity, o.cutout);
        g3d_sprite_set_shadow(o.entity, o.shadow);
        g3d_sprite_set_smooth(o.entity, o.smooth);
        g3d_sprite_set_lit(o.entity, o.iluminado);
        g3d_sprite_set_snap(o.entity, o.ajuste_px);
        // El alto lo marca el fotograma MAS ALTO de la hoja; los demas guardan su
        // proporcion (si no, un salto o un ataque cambiarian de tamano).
        int maxh = 1;
        for (auto& f : sh->frames) if (f.h > maxh) maxh = f.h;
        g3d_sprite_set_height(o.entity, 0.0f);
        g3d_sprite_set_pixels_per_unit(o.entity, (o.height > 0.01f) ? maxh / o.height : 32.0f);
        // fotograma que toca
        const SprAnim* an = nullptr;
        for (auto& a : sh->anims) if (a.name == o.anim) { an = &a; break; }
        int idx = 0;
        if (an && !an->frames.empty()) {
            o.t += dt * (o.fps > 0 ? o.fps : an->fps);   // los fps del personaje mandan
            idx = an->frames[((int)o.t) % (int)an->frames.size()];
        }
        if (idx < 0 || idx >= (int)sh->frames.size()) idx = 0;
        const SprFrame& f = sh->frames[idx];
        g3d_sprite_set_cell(o.entity, f.x, f.y, f.w, f.h);
        g3d_sprite_set_anchor(o.entity, f.w ? f.ax / (float)f.w : 0.5f,
                                        f.h ? f.ay / (float)f.h : 1.0f);
    };

    // Lo que el usuario teclea es UTF-8; BennuGD2 escribe bytes latin-1.
    auto hud_latin1 = [](const std::string& t) {
        std::vector<char> b(t.size() + 1);
        h2_utf8_to_latin1(t.c_str(), b.data(), (int)b.size());
        return std::string(b.data());
    };
    // Tamano en pixeles de un elemento tal como saldra en el juego.
    auto hud_item_size = [&](const HudItem& h, float* w, float* hh) {
        *w = *hh = 0.0f;
        if (h.type == 0) {
            H2Img* im = hud_item_img(h);
            if (!im) { *w = *hh = 32.0f; return; }          // sin grafico: un hueco visible
            *w = im->w * h.size * 0.01f; *hh = im->h * h.size * 0.01f;
        } else {
            H2Font* f = h.font.empty() ? nullptr : hud_font(h.font);
            std::string t = hud_latin1(h.var.empty() ? h.text : ("[" + h.var + "]"));
            if (f) { *w = (float)h2_text_width(f, t.c_str()); *hh = (float)f->maxheight; }
            else   { *w = t.size() * 8.0f; *hh = 8.0f; }     // fuente 0 del sistema (8x8)
        }
    };
    // Esquina superior izquierda: el grafico va CENTRADO en x,y (como BennuGD2),
    // y el texto se coloca segun su alineacion, igual que hace gr_text_new2.
    auto hud_item_origin = [&](const HudItem& h, float w, float hh, float* ox, float* oy) {
        if (h.type == 0) { *ox = h.x - w * 0.5f; *oy = h.y - hh * 0.5f; return; }
        *ox = h.x; *oy = h.y;
        int a = h.align;
        if (a == 1 || a == 4 || a == 7) *ox -= w * 0.5f;      // centrado
        if (a == 2 || a == 5 || a == 8) *ox -= w - 1;         // a la derecha
        if (a == 3 || a == 4 || a == 5) *oy -= hh * 0.5f;
        if (a == 6 || a == 7 || a == 8) *oy -= hh - 1;
    };
    // Estado para deshacer/rehacer (los lambdas que lo usan estan mas abajo; se
    // declara aqui para poder vaciarlo al cargar una escena o un proyecto).
    struct EditState { std::vector<SObj> objs; int follow; };
    std::vector<EditState> undo_stack, redo_stack;
    // El relieve son 25.921 alturas: meterlo en EditState haria enorme cada paso
    // de deshacer de objetos. Va en su propia pila, y una marca por accion dice
    // de cual toca sacar, para que Ctrl+Z siga un solo orden cronologico.
    std::vector<std::vector<float>> terr_undo, terr_redo;
    /* La siembra se guarda leyendola con sus propios accesores: no hace falta
       tocar el modulo, y una copia de diez mil ejemplares son ~200 KB. */
    struct ScSnapKind {
        std::string asset; float wind, dist; int solid;
        std::vector<std::array<float,5>> items;
    };
    typedef std::vector<ScSnapKind> ScSnap;
    std::vector<ScSnap> sc_undo, sc_redo;
    std::vector<char> undo_kind, redo_kind;   // 'o' objetos, 't' relieve
    EditState last_state;
    bool state_init = false;
    auto undo_reset = [&]() {
        undo_stack.clear(); redo_stack.clear();
        terr_undo.clear(); terr_redo.clear();
        sc_undo.clear(); sc_redo.clear();
        undo_kind.clear(); redo_kind.clear();
        vx_sel.clear(); vx_sel_y0.clear(); vx_i = vx_j = -1;
        state_init = false;
    };
    std::map<std::string, void*> model_cache;
    std::set<void*> posed_static;   // modelos sin esqueleto ya colocados (pose t=0)
    auto load_model = [&](const std::string& file) -> void* {
        auto it = model_cache.find(file);
        if (it != model_cache.end()) return it->second;
        std::string path = assets_dir + "/" + ruta_asset(file);
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
    // Y las de sonido, al arrancar (luego apply_project las rehace por proyecto).
    { std::error_code ec;
      fs::create_directories(assets_dir + "/Music", ec);
      fs::create_directories(assets_dir + "/Sounds", ec);
      scan_sonoros(assets_dir, "Music",  { ".ogg", ".mp3", ".mod", ".xm", ".it", ".s3m",
                                           ".mid", ".midi", ".flac", ".wav" }, musicas);
      scan_sonoros(assets_dir, "Sounds", { ".wav", ".ogg", ".flac" }, sonidos); }
    /* Las escenas del proyecto, en el mismo orden que las numera la generacion:
       alfabetico. El numero tiene que salir igual aqui (que es donde se elige la
       escena de destino) y alli (que es donde se escribe el codigo). */
    auto escenas_del_proyecto = [&]() {
        std::vector<std::string> out;
        std::error_code ec;
        for (auto& e : fs::directory_iterator(scenes_dir, ec)) {
            if (!e.is_regular_file(ec)) continue;
            if (e.path().extension() != ".scene") continue;
            out.push_back(e.path().filename().string());
        }
        std::sort(out.begin(), out.end());
        return out;
    };
    auto indice_escena = [&](const std::string& fichero) -> int {
        auto l = escenas_del_proyecto();
        for (size_t i = 0; i < l.size(); i++) if (l[i] == fichero) return (int)i;
        return -1;
    };

    std::string scene_path = scenes_dir + "/level.scene";   // escena actual
    // La escena por la que empieza el juego (relativa al proyecto). Es la que dice
    // el manifiesto .bgd2; las demas se llegan cambiando de escena desde una regla.
    std::string escena_inicial = "Scenes/level.scene";
    std::string status;

    // ================== editor de codigo: abrir, guardar, saltar ==================
    auto leer_todo = [](const std::string& ruta, std::string& out) -> bool {
        FILE* f = fopen(ruta.c_str(), "rb");
        if (!f) return false;
        out.clear();
        char b[4096]; size_t n;
        while ((n = fread(b, 1, sizeof(b), f)) > 0) out.append(b, n);
        fclose(f);
        return true;
    };
    auto escribir_todo = [](const std::string& ruta, const std::string& t) -> bool {
        FILE* f = fopen(ruta.c_str(), "wb");
        if (!f) return false;
        fwrite(t.data(), 1, t.size(), f);
        fclose(f);
        return true;
    };
    // Indice de la pestania que tiene abierto ese fichero (-1 si ninguna).
    auto doc_de = [&](const std::string& ruta) -> int {
        for (size_t i = 0; i < docs.size(); i++) if (docs[i]->ruta == ruta) return (int)i;
        return -1;
    };
    /* Abre un fichero en una pestania (o se pone en la suya si ya estaba) y deja
       el editor a la vista. `texto` sirve para el caso de un fichero que aun no
       existe: se abre con esa plantilla y sin guardar todavia. */
    auto code_abrir = [&](const std::string& ruta, const std::string& titulo,
                          const std::string& objeto, const char* texto = nullptr) -> int {
        int i = doc_de(ruta);
        if (i < 0) {
            std::string t;
            if (!leer_todo(ruta, t)) { if (texto) t = texto; else t.clear(); }
            docs.push_back(std::make_unique<CodeDoc>());
            i = (int)docs.size() - 1;
            docs[i]->ruta = ruta;
            docs[i]->ed.SetLanguageDefinition(BennuGD2Language());
            docs[i]->ed.SetText(t);
            docs[i]->en_disco = t;
            docs[i]->sucio = false;
        }
        docs[i]->titulo = titulo.empty() ? fs::path(ruta).filename().string() : titulo;
        docs[i]->objeto = objeto;
        doc_sel = i;
        show_script = true; focus_script = true;
        return i;
    };
    // Vuelve a leer del disco (tras regenerar el script de un objeto, por ejemplo).
    auto code_recargar = [&](int i) {
        if (i < 0 || i >= (int)docs.size()) return;
        std::string t;
        if (!leer_todo(docs[i]->ruta, t)) return;
        docs[i]->ed.SetText(t);
        docs[i]->en_disco = t;
        docs[i]->sucio = false;
    };
    auto code_guardar = [&](int i) -> bool {
        if (i < 0 || i >= (int)docs.size()) return false;
        std::string t = docs[i]->ed.GetText();
        if (!escribir_todo(docs[i]->ruta, t)) return false;
        docs[i]->en_disco = t;
        docs[i]->sucio = false;
        return true;
    };
    auto code_ir_a = [&](int i, int linea) {   // linea 1..n
        if (i < 0 || i >= (int)docs.size()) return;
        doc_sel = i;
        if (linea < 1) linea = 1;
        TextEditor::Coordinates c(linea - 1, 0);
        docs[i]->ed.SetCursorPosition(c);
        docs[i]->ed.SetSelection(c, c);
        show_script = true; focus_script = true;
    };

    /* ---- Los PROCESS y FUNCTION que hay en un .prg ----
       Es lo que se ofrece al asignarle codigo tuyo a un objeto: en vez de teclear
       el nombre a ciegas, eliges el fichero y de el sale la lista. El mismo
       barrido alimenta el esquema del editor.
       No es un analizador del lenguaje: busca la palabra al principio de linea,
       que es como se escriben en BennuGD2, y se salta lo comentado. */
    struct PrgSym { std::string nombre, firma; int linea; int es_func; };
    auto prg_simbolos = [](const std::string& texto) {
        std::vector<PrgSym> out;
        size_t i = 0; int linea = 1; bool bloque = false;
        while (i <= texto.size()) {
            size_t fin = texto.find('\n', i);
            if (fin == std::string::npos) fin = texto.size();
            std::string l = texto.substr(i, fin - i);
            // comentarios de bloque: no cuenta lo que hay dentro
            std::string sin;
            for (size_t k = 0; k < l.size(); k++) {
                if (bloque) { if (l[k] == '*' && k + 1 < l.size() && l[k+1] == '/') { bloque = false; k++; } continue; }
                if (l[k] == '/' && k + 1 < l.size() && l[k+1] == '*') { bloque = true; k++; continue; }
                if (l[k] == '/' && k + 1 < l.size() && l[k+1] == '/') break;
                sin += l[k];
            }
            size_t a = sin.find_first_not_of(" \t");
            if (a != std::string::npos) {
                std::string r = sin.substr(a);
                std::string may;
                for (char c : r) may += (char)toupper((unsigned char)c);
                int es_func = -1;
                if (may.rfind("PROCESS", 0) == 0 && (r.size() > 7 && isspace((unsigned char)r[7]))) es_func = 0;
                else if (may.rfind("FUNCTION", 0) == 0 && (r.size() > 8 && isspace((unsigned char)r[8]))) es_func = 1;
                if (es_func >= 0) {
                    std::string resto = r.substr(es_func ? 8 : 7);
                    size_t par = resto.find('(');
                    if (par != std::string::npos) {
                        // el nombre es la ultima palabra antes del parentesis
                        // (una FUNCTION puede llevar el tipo delante: "FUNCTION int vida()")
                        std::string ant = resto.substr(0, par);
                        size_t e = ant.find_last_not_of(" \t");
                        if (e != std::string::npos) {
                            ant = ant.substr(0, e + 1);
                            size_t b = ant.find_last_of(" \t");
                            std::string nom = (b == std::string::npos) ? ant : ant.substr(b + 1);
                            if (!nom.empty()) {
                                size_t cierra = resto.find(')', par);
                                std::string args = (cierra == std::string::npos) ? "" : resto.substr(par, cierra - par + 1);
                                out.push_back({ nom, nom + args, linea, es_func });
                            }
                        }
                    }
                }
            }
            if (fin >= texto.size()) break;
            i = fin + 1; linea++;
        }
        return out;
    };
    // Los .prg de la carpeta Scripts del proyecto, en orden alfabetico.
    auto prg_de_scripts = [&]() {
        std::vector<std::string> out;
        std::error_code ec;
        for (auto& e : fs::directory_iterator(scripts_dir, ec)) {
            if (!e.is_regular_file(ec)) continue;
            std::string n = e.path().filename().string();
            std::string ext = e.path().extension().string();
            for (auto& c : ext) c = (char)tolower((unsigned char)c);
            if (ext == ".prg" && n.rfind("__", 0) != 0) out.push_back(n);
        }
        std::sort(out.begin(), out.end());
        return out;
    };
    /* ---- Donde vive cada PROCESS/FUNCTION de Scripts ----
       Para las escenas de antes, que guardaban solo el nombre: si ese proceso
       existe en algun fichero, se encuentra solo y no hay que volver a elegirlo.
       Se rehace de vez en cuando, que leer toda la carpeta cada frame sobra. */
    static std::map<std::string, std::string> proc_idx;
    static double proc_idx_t = -1.0;
    auto indice_procs = [&]() -> std::map<std::string, std::string>& { return proc_idx; };
    // Los procesos/funciones de un .prg de Scripts (por nombre de fichero).
    static std::map<std::string, std::pair<double, std::vector<PrgSym>>> sim_cache;
    auto simbolos_de = [&](const std::string& fichero) {
        std::vector<PrgSym> out;
        if (fichero.empty()) return out;
        /* Con la ficha abierta esto se pide varias veces por frame (los
           desplegables y el aviso de "ese proceso ya no esta"), asi que el
           resultado se guarda un rato en vez de releer el fichero cada vez. */
        double ahora = ImGui::GetTime();
        auto it = sim_cache.find(fichero);
        if (it != sim_cache.end() && ahora - it->second.first < 0.5) return it->second.second;
        std::string t;
        int i = doc_de(scripts_dir + "/" + fichero);       // si esta abierto, lo que se ve
        if (i >= 0) t = docs[i]->ed.GetText();
        else if (!leer_todo(scripts_dir + "/" + fichero, t)) return out;
        out = prg_simbolos(t);
        sim_cache[fichero] = { ahora, out };
        return out;
    };

    /* ================= elegir CODIGO TUYO para un objeto =================
       Dos desplegables: el fichero .prg de Scripts y, de el, el PROCESS o la
       FUNCTION. Nada de teclear el nombre a ciegas y descubrir al ejecutar que
       no existe. "Nuevo" crea el esqueleto y lo abre, "Editar" salta a su linea.
       `sugerencia` es el nombre que se le pone a lo que se cree (la accion, el
       objeto), para no tener que inventarselo. */
    auto selector_codigo = [&](const char* id, std::string& archivo, std::string& proc,
                               const std::string& sugerencia) {
        ImGui::PushID(id);
        // Escenas viejas: solo guardaban el nombre. Si ese proceso esta en algun
        // fichero de Scripts, se rellena el fichero solo.
        if (archivo.empty() && !proc.empty()) {
            double ahora = ImGui::GetTime();
            if (proc_idx_t < 0.0 || ahora - proc_idx_t > 2.0) {
                proc_idx.clear(); proc_idx_t = ahora;
                for (auto& fn : prg_de_scripts())
                    for (auto& sy : simbolos_de(fn)) {
                        std::string k; for (char c : sy.nombre) k += (char)tolower((unsigned char)c);
                        proc_idx.emplace(k, fn);
                    }
            }
            std::string k; for (char c : proc) k += (char)tolower((unsigned char)c);
            auto it = proc_idx.find(k);
            if (it != proc_idx.end()) archivo = it->second;
        }

        /* En el Inspector no caben los dos desplegables uno al lado del otro (es
           la mitad de ancho que la ventana de personajes), asi que si hay poco
           sitio se ponen en dos filas en vez de salirse por la derecha. */
        float ancho = ImGui::GetContentRegionAvail().x;
        bool estrecho = (ancho < 430.0f);
        float wcombo = estrecho ? (ancho - 78.0f) : 190.0f;
        if (wcombo < 90.0f) wcombo = 90.0f;

        ImGui::SetNextItemWidth(wcombo);
        if (ImGui::BeginCombo("fichero", archivo.empty() ? "(ninguno)" : archivo.c_str())) {
            if (ImGui::Selectable("(ninguno)", archivo.empty())) { archivo.clear(); }
            for (auto& fn : prg_de_scripts())
                if (ImGui::Selectable(fn.c_str(), fn == archivo)) { archivo = fn; }
            ImGui::EndCombo();
        }
        if (!estrecho) ImGui::SameLine();
        ImGui::SetNextItemWidth(wcombo);
        if (ImGui::BeginCombo("llama a", proc.empty() ? "(nada)" : proc.c_str())) {
            if (ImGui::Selectable("(nada)", proc.empty())) proc.clear();
            for (auto& sy : simbolos_de(archivo)) {
                if (ImGui::Selectable((sy.nombre + "##" + std::to_string(sy.linea)).c_str(), sy.nombre == proc))
                    proc = sy.nombre;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", sy.firma.c_str());
            }
            ImGui::EndCombo();
        }
        if (!estrecho) ImGui::SameLine();
        if (ImGui::SmallButton("Nuevo")) {
            /* Crea el PROCESS y lo deja abierto para escribirlo. Si el fichero ya
               existe se le anade al final, que un fichero por accion acaba siendo
               una carpeta imposible de mirar. */
            std::string nom = ident_bgd(sugerencia.empty() ? "accion" : sugerencia, "accion");
            std::string fich = archivo.empty() ? (nom + ".prg") : archivo;
            std::string ruta = scripts_dir + "/" + fich;
            // que no choque con uno que ya exista
            {
                std::set<std::string> hay;
                for (auto& sy : simbolos_de(fich)) {
                    std::string k; for (char c : sy.nombre) k += (char)tolower((unsigned char)c);
                    hay.insert(k);
                }
                std::string base = nom;
                for (int k = 2; k < 100; k++) {
                    std::string kk; for (char c : nom) kk += (char)tolower((unsigned char)c);
                    if (!hay.count(kk)) break;
                    nom = base + "_" + std::to_string(k);
                }
            }
            std::string cuerpo =
                "\n// " + nom + ": codigo tuyo, lo llama el juego cuando toca.\n"
                "// Es un PROCESS normal de BennuGD2: puede durar varios FRAME (un\n"
                "// disparo, un golpe con su tiempo) o acabar enseguida con RETURN.\n"
                "PROCESS " + nom + "()\n"
                "BEGIN\n"
                "    // ---- tu codigo aqui ----\n"
                "    RETURN;\n"
                "END\n";
            std::string ya;
            if (leer_todo(ruta, ya)) escribir_todo(ruta, ya + cuerpo);
            else                     escribir_todo(ruta, cuerpo.substr(1));
            archivo = fich; proc = nom;
            proc_idx_t = -1.0;
            int di = doc_de(ruta);
            if (di >= 0) code_recargar(di);
            di = code_abrir(ruta, fich, "");
            // dejar el cursor en el que se acaba de crear
            for (auto& sy : prg_simbolos(docs[di]->ed.GetText()))
                if (sy.nombre == nom) { code_ir_a(di, sy.linea); break; }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Crea el PROCESS (en el fichero elegido, o en uno nuevo)\ny lo abre en el editor de codigo.");
        if (!proc.empty() && !archivo.empty()) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Editar")) {
                std::string ruta = scripts_dir + "/" + archivo;
                int di = doc_de(ruta);
                if (di < 0) di = code_abrir(ruta, archivo, "");
                for (auto& sy : prg_simbolos(docs[di]->ed.GetText()))
                    if (sy.nombre == proc) { code_ir_a(di, sy.linea); break; }
                show_script = true; focus_script = true;
            }
        }
        // Avisa de lo que no cuadra ANTES de ejecutar: es justo lo que rompia la
        // compilacion del juego ("Undefined procedure").
        if (!proc.empty()) {
            bool esta = false;
            for (auto& sy : simbolos_de(archivo)) if (sy.nombre == proc) { esta = true; break; }
            if (!esta)
                ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1),
                                   archivo.empty() ? "  Sin fichero: el editor creara el esqueleto al generar."
                                                   : "  Ese proceso ya no esta en el fichero.");
        }
        ImGui::PopID();
    };

    /* ================== la ficha de las REGLAS ==================
       La misma para un objeto 3D y para un personaje 2D. Arriba el disparador,
       debajo la lista de cosas que pasan. Cabe en el Inspector, que es estrecho:
       cada cosa en su linea. */
    auto combo_var = [&](const char* et, std::string& dest) {
        const char* cur = dest.empty() ? "(elige una)" : dest.c_str();
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo(et, cur)) {
            for (auto& v : gvars)
                if (ImGui::Selectable(v.nombre.c_str(), v.nombre == dest)) dest = v.nombre;
            if (gvars.empty()) ImGui::TextDisabled("(ninguna: Juego > Variables del juego)");
            ImGui::EndCombo();
        }
    };
    /* Renombra un fichero de audio a un nombre que el juego pueda abrir, y cambia
       de paso TODAS las referencias de la escena: la musica, los ambientes de
       zona, el sonido propio de los objetos y el de las reglas. Renombrar el
       fichero y dejar la escena apuntando al nombre viejo seria peor que no
       renombrar. */
    auto renombrar_audio = [&](const std::string& viejo, const std::string& nuevo) -> bool {
        std::error_code ec;
        if (fs::exists(assets_dir + "/" + nuevo, ec)) return false;
        fs::rename(assets_dir + "/" + viejo, assets_dir + "/" + nuevo, ec);
        if (ec) return false;
        auto cambia = [&](std::string& d) { if (d == viejo) d = nuevo; };
        cambia(esc_musica);
        for (auto& z : zsonidos) cambia(z.sonido);
        for (auto& o : objects) {
            cambia(o.amb_sonido);
            for (auto& r : o.reglas) for (auto& a : r.acciones) cambia(a.sonido);
        }
        for (auto& sp : sprites)
            for (auto& r : sp.reglas) for (auto& a : r.acciones) cambia(a.sonido);
        scan_sonoros(assets_dir, "Music",  { ".ogg", ".mp3", ".mod", ".xm", ".it", ".s3m",
                                             ".mid", ".midi", ".flac", ".wav" }, musicas);
        scan_sonoros(assets_dir, "Sounds", { ".wav", ".ogg", ".flac" }, sonidos);
        console_add("Renombrado: " + viejo + "  ->  " + nuevo + "\n");
        return true;
    };
    /* Lo que hay que decir de un fichero elegido: que el juego no va a poder
       abrirlo por el nombre, o que ya no esta donde estaba. */
    auto aviso_audio = [&](const std::string& f) {
        if (f.empty()) return;
        std::error_code ec;
        if (!nombre_ascii(f)) {
            std::string dir = fs::path(f).parent_path().string();
            std::string base = nombre_saneado(fs::path(f).filename().string());
            std::string nuevo = dir.empty() ? base : (dir + "/" + base);
            for (int k = 2; k < 50 && fs::exists(assets_dir + "/" + nuevo, ec); k++) {
                std::string tallo = fs::path(base).stem().string();
                std::string ext = fs::path(base).extension().string();
                nuevo = (dir.empty() ? "" : dir + "/") + tallo + "_" + std::to_string(k) + ext;
            }
            // envuelto: en el panel de Entorno, que es estrecho, se salia por la derecha
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.5f, 0.45f, 1));
            ImGui::TextWrapped("Ese nombre lleva letras que el juego no puede abrir "
                               "(emojis, tildes, enies): suena en el editor pero no al jugar.");
            ImGui::PopStyleColor();
            if (ImGui::SmallButton(("Renombrar a  " + fs::path(nuevo).filename().string()).c_str()))
                renombrar_audio(f, nuevo);
        } else if (!fs::exists(assets_dir + "/" + f, ec)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.75f, 0.3f, 1));
            ImGui::TextWrapped("Ese fichero ya no esta en Assets.");
            ImGui::PopStyleColor();
        }
    };
    auto combo_sonido = [&](const char* et, std::string& dest) {
        const char* cur = dest.empty() ? "(ninguno)" : dest.c_str();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
        if (ImGui::BeginCombo(et, cur)) {
            if (ImGui::Selectable("(ninguno)", dest.empty())) dest.clear();
            for (auto& sn : sonidos)
                if (ImGui::Selectable(sn.c_str(), sn == dest)) dest = sn;
            if (sonidos.empty()) ImGui::TextDisabled("(pon ficheros en Assets/Sounds)");
            ImGui::EndCombo();
        }
        aviso_audio(dest);
    };
    auto ui_acciones = [&](std::vector<Accion>& acc, const std::string& quien, int contexto) {
            int quitar = -1;
            for (int q = 0; q < (int)acc.size(); q++) {
                Accion& a = acc[q];
                ImGui::PushID(1000 + q);
                const char* tt[] = { "Llamar a codigo mio", "Cambiar una variable",
                                     "Quitar esto de la escena", "Ensenar un texto",
                                     "Sonar un sonido", "Ir a otra escena",
                                     "Cerrar el menu", "Salir del juego",
                                     "Abrir un menu", "Sacar un dialogo",
                                     "Guardar la partida", "Cargar la partida" };
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
                ImGui::Combo("que", &a.tipo, tt, 12);
                ImGui::SameLine();
                if (ImGui::SmallButton("x")) quitar = q;
                if (a.tipo == 0) {
                    selector_codigo("codregla", a.archivo, a.proc, quien);
                } else if (a.tipo == 1) {
                    combo_var("variable", a.var);
                    const char* oo[] = { "ponerla en", "sumarle", "restarle" };
                    ImGui::SetNextItemWidth(120); ImGui::Combo("operacion", &a.op, oo, 3);
                    ImGui::SetNextItemWidth(100); ImGui::DragFloat("cuanto", &a.valor, 1.0f, -100000.0f, 100000.0f, "%.0f");
                } else if (a.tipo == 2) {
                    if (contexto) ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "  (esto es para objetos, no para un menu)");
                    else          ImGui::TextDisabled("  desaparece y su proceso termina");
                } else if (a.tipo == 6) {
                    if (contexto) ImGui::TextDisabled("  se cierra y el juego sigue");
                    else          ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "  (esto es para un menu)");
                } else if (a.tipo == 7) {
                    ImGui::TextDisabled("  se acaba el programa (exit)");
                } else if (a.tipo == 10 || a.tipo == 11) {
                    ImGui::SetNextItemWidth(120);
                    ImGui::DragInt("en la ranura", &a.ranura, 0.2f, 1, guardado.ranuras);
                    ImGui::TextDisabled(a.tipo == 10
                        ? "  guarda lo que dijiste en Ventana > Guardar partida"
                        : "  vuelve a esa partida (si esa ranura tiene algo)");
                } else if (a.tipo == 9) {
                    const char* cur = a.dialogo.empty() ? "(elige uno)" : a.dialogo.c_str();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                    if (ImGui::BeginCombo("dialogo", cur)) {
                        for (auto& dd : dialogos)
                            if (ImGui::Selectable(dd.nombre.c_str(), dd.nombre == a.dialogo)) a.dialogo = dd.nombre;
                        if (dialogos.empty()) ImGui::TextDisabled("(ninguno: Escena > Dialogos)");
                        ImGui::EndCombo();
                    }
                    ImGui::TextDisabled("  se pone a hablar; no sale dos veces a la vez");
                } else if (a.tipo == 8) {
                    const char* cur = a.menu.empty() ? "(elige uno)" : a.menu.c_str();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                    if (ImGui::BeginCombo("menu", cur)) {
                        for (auto& mm : menus)
                            if (ImGui::Selectable(mm.nombre.c_str(), mm.nombre == a.menu)) a.menu = mm.nombre;
                        if (menus.empty()) ImGui::TextDisabled("(ninguno: Escena > Menus del juego)");
                        ImGui::EndCombo();
                    }
                } else if (a.tipo == 4) {
                    combo_sonido("sonido", a.sonido);
                    ImGui::SliderInt("volumen", &a.vol, 0, 128);
                } else if (a.tipo == 5) {
                    const char* cur = a.escena.empty() ? "(elige una)" : a.escena.c_str();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                    if (ImGui::BeginCombo("escena", cur)) {
                        for (auto& e : escenas_del_proyecto())
                            if (ImGui::Selectable(e.c_str(), e == a.escena)) a.escena = e;
                        ImGui::EndCombo();
                    }
                    if (!a.escena.empty() && indice_escena(a.escena) < 0)
                        ImGui::TextColored(ImVec4(1, 0.6f, 0.4f, 1), "  Esa escena ya no esta.");
                    else
                        ImGui::TextDisabled("  Se desmonta esta y se monta la otra.");
                } else {
                    char tb[192]; snprintf(tb, sizeof(tb), "%s", a.texto.c_str());
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                    if (ImGui::InputTextWithHint("texto", "+10 puntos!", tb, sizeof(tb))) a.texto = tb;
                    ImGui::SetNextItemWidth(90);
                    ImGui::DragFloat("segundos", &a.seg, 0.1f, 0.2f, 20.0f, "%.1f");
                }
                ImGui::PopID();
            }
            if (quitar >= 0) acc.erase(acc.begin() + quitar);
            if (ImGui::SmallButton("+ que pase algo mas")) acc.push_back(Accion());
            if (acc.empty())
                ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1),
                    contexto ? "  (sin nada que hacer: la opcion no hara nada)"
                             : "  (sin nada que hacer: la regla no se genera)");
    };

    auto ui_reglas = [&](std::vector<Regla>& rs, const std::string& quien) {
        int borrar = -1;
        for (int k = 0; k < (int)rs.size(); k++) {
            Regla& r = rs[k];
            ImGui::PushID(k);
            const char* ev[] = { "Al empezar (una vez)", "Cada frame", "Al acercarse y pulsar",
                                 "Cuando el jugador lo toca", "Cuando el jugador entra en la zona",
                                 "Cuando una variable cumple", "Cada N segundos" };
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.52f);
            ImGui::Combo("cuando", &r.evento, ev, 7);
            ImGui::SameLine();
            if (ImGui::SmallButton("Quitar")) borrar = k;

            if (r.evento == 2) {
                ImGui::SetNextItemWidth(110); combo_tecla_ui("tecla", r.tecla);
                combo_boton_ui("boton", r.boton);
                ImGui::SetNextItemWidth(150);
                ImGui::DragFloat("distancia", &r.radio, 0.1f, 0.5f, 40.0f, "%.1f");
            } else if (r.evento == 3) {
                ImGui::SetNextItemWidth(150);
                ImGui::DragFloat("a que distancia cuenta", &r.radio, 0.1f, 0.3f, 40.0f, "%.1f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Se mide desde el BORDE de este objeto, no desde su centro,\n"
                                      "asi que 2 o 3 vale igual para una moneda que para una casa.");
            } else if (r.evento == 4) {
                const char* zz[] = { "Capa 0", "Capa 1", "Capa 2", "Capa 3" };
                ImGui::SetNextItemWidth(150);
                ImGui::Combo("zona pintada", &r.zona, zz, 4);
            } else if (r.evento == 5) {
                combo_var("variable", r.var);
                const char* cc[] = { "es igual a", "no es", "es menor que", "es menor o igual",
                                     "es mayor que", "es mayor o igual" };
                ImGui::SetNextItemWidth(150); ImGui::Combo("condicion", &r.cmp, cc, 6);
                ImGui::SetNextItemWidth(100); ImGui::DragFloat("valor", &r.valor, 1.0f, -100000.0f, 100000.0f, "%.0f");
            } else if (r.evento == 6) {
                ImGui::SetNextItemWidth(120);
                ImGui::DragFloat("cada (segundos)", &r.cada, 0.1f, 0.05f, 600.0f, "%.2f");
            }
            if (r.evento >= 2) {
                bool uv = r.una_vez != 0;
                if (ImGui::Checkbox("solo la primera vez", &uv)) r.una_vez = uv ? 1 : 0;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Para un cofre, una llave, un checkpoint:\nse cumple una vez y no vuelve.");
                if (r.evento != 6) {
                    bool mi = r.mientras != 0;
                    if (ImGui::Checkbox("mientras se cumpla (cada frame)", &mi)) r.mientras = mi ? 1 : 0;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Sin marcar: salta AL cumplirse (tocar una moneda).\nMarcado: pasa todo el rato (perder vida en la lava).");
                }
            }

            ImGui::TextDisabled("...entonces:");
            ui_acciones(r.acciones, quien + "_" + std::to_string(k + 1), 0);
            ImGui::Separator();
            ImGui::PopID();
        }
        if (borrar >= 0) rs.erase(rs.begin() + borrar);
        if (ImGui::Button(ICON_FA_PLUS "  Anadir regla", ImVec2(-1, 0))) rs.push_back(Regla());
    };
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
        // Lo que se coloca es SOLIDO. Sin esto el agua no lo ve: una roca en
        // mitad del rio no lo desviaria ni salpicaria, la atravesaria.
        g3d_entity_impl_set_collider(o.entity, 1);
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
                g3d_entity_impl_set_collider(o.entity, 1);
                g3d_entity_impl_set_rotation(o.entity, 0.0f, o.ry, 0.0f);
                g3d_entity_impl_set_scale(o.entity, o.scale, o.scale, o.scale);
            }
        }
        if (obj_sel >= (int)objects.size()) obj_sel = -1;
    };
    auto scatter_snapshot = [&]() -> ScSnap {
        ScSnap snap;
        for (int k = 0; k < g3d_scatter_kinds(); k++) {
            const char* nm = g3d_scatter_kind_asset(k);
            if (!nm) continue;
            ScSnapKind sk;
            sk.asset = nm;
            sk.wind  = g3d_scatter_get_kind_wind(k);
            sk.dist  = g3d_scatter_get_kind_distance(k);
            sk.solid = g3d_scatter_get_kind_solid(k);
            int n = g3d_scatter_kind_count(k);
            sk.items.reserve(n);
            for (int i = 0; i < n; i++) {
                float v[5];
                if (g3d_scatter_get(k, i, v))
                    sk.items.push_back({v[0],v[1],v[2],v[3],v[4]});
            }
            snap.push_back(std::move(sk));
        }
        return snap;
    };
    auto scatter_restore = [&](const ScSnap& snap) {
        g3d_scatter_clear();
        for (auto& sk : snap) {
            for (auto& v : sk.items)
                g3d_scatter_add(sk.asset.c_str(), v[0], v[1], v[2], v[3], v[4]);
            g3d_scatter_set_kind_wind(sk.asset.c_str(), sk.wind);
            g3d_scatter_set_kind_distance(sk.asset.c_str(), sk.dist);
            g3d_scatter_set_kind_solid(sk.asset.c_str(), sk.solid);
        }
        g3d_scatter_build(1.0f);
    };
    /* Guarda la siembra ANTES de tocarla. Un trazo entero es UN paso: si se
       guardara por ejemplar, deshacer una pincelada de veinte arboles pediria
       veinte Ctrl+Z. */
    auto push_scatter_undo = [&]() {
        if (playing) return;
        sc_undo.push_back(scatter_snapshot());
        undo_kind.push_back('s');
        while (sc_undo.size() > 24) {
            sc_undo.erase(sc_undo.begin());
            for (size_t k = 0; k < undo_kind.size(); k++)
                if (undo_kind[k] == 's') { undo_kind.erase(undo_kind.begin()+k); break; }
        }
        redo_stack.clear(); terr_redo.clear(); sc_redo.clear(); redo_kind.clear();
    };

    /* Guarda el relieve ANTES de tocarlo. Se llama al empezar cada trazo, no en
       cada frame del arrastre: si no, un arrastre de dos segundos dejaria cien
       pasos de deshacer identicos. */
    auto push_terrain_undo = [&]() {
        if (playing || !terrain) return;
        int nv = g3d_editor_terrain_vcount(terrain);
        if (nv <= 0) return;
        std::vector<float> snap((size_t)nv);
        g3d_editor_terrain_snapshot(terrain, snap.data());
        terr_undo.push_back(std::move(snap));
        undo_kind.push_back('t');
        /* Acotado: cada copia son ~100 KB. Al tirar la mas vieja hay que quitar
           tambien su marca, o el orden de las dos pilas se descuadra. */
        while (terr_undo.size() > 24) {
            terr_undo.erase(terr_undo.begin());
            for (size_t k = 0; k < undo_kind.size(); k++)
                if (undo_kind[k] == 't') { undo_kind.erase(undo_kind.begin() + k); break; }
        }
        redo_stack.clear(); terr_redo.clear(); redo_kind.clear();
    };

    auto do_undo = [&]() {
        if (playing || undo_kind.empty()) return;
        char k = undo_kind.back();
        if (k == 's') {
            if (sc_undo.empty()) { undo_kind.pop_back(); return; }
            sc_redo.push_back(scatter_snapshot()); redo_kind.push_back('s');
            scatter_restore(sc_undo.back());
            sc_undo.pop_back(); undo_kind.pop_back();
            status = "Siembra deshecha";
            return;
        }
        if (k == 't') {
            if (terr_undo.empty() || !terrain) { undo_kind.pop_back(); return; }
            int nv = g3d_editor_terrain_vcount(terrain);
            std::vector<float> cur((size_t)nv);
            g3d_editor_terrain_snapshot(terrain, cur.data());
            terr_redo.push_back(std::move(cur)); redo_kind.push_back('t');
            g3d_editor_terrain_restore(terrain, terr_undo.back().data());
            terr_undo.pop_back(); undo_kind.pop_back();
            if (!lakes.empty() || !rivers.empty()) rebuild_water();
            if (g3d_watersim_active()) watersim_sync(true);
            status = "Relieve deshecho";
            return;
        }
        if (undo_stack.empty()) { undo_kind.pop_back(); return; }
        redo_stack.push_back(EditState{ objects, cam_follow }); redo_kind.push_back('o');
        apply_state(undo_stack.back());
        last_state = undo_stack.back();
        undo_stack.pop_back(); undo_kind.pop_back();
        status = "Deshecho (quedan " + std::to_string(undo_kind.size()) + ")";
    };
    auto do_redo = [&]() {
        if (playing || redo_kind.empty()) return;
        char k = redo_kind.back();
        if (k == 's') {
            if (sc_redo.empty()) { redo_kind.pop_back(); return; }
            sc_undo.push_back(scatter_snapshot()); undo_kind.push_back('s');
            scatter_restore(sc_redo.back());
            sc_redo.pop_back(); redo_kind.pop_back();
            status = "Siembra rehecha";
            return;
        }
        if (k == 't') {
            if (terr_redo.empty() || !terrain) { redo_kind.pop_back(); return; }
            int nv = g3d_editor_terrain_vcount(terrain);
            std::vector<float> cur((size_t)nv);
            g3d_editor_terrain_snapshot(terrain, cur.data());
            terr_undo.push_back(std::move(cur)); undo_kind.push_back('t');
            g3d_editor_terrain_restore(terrain, terr_redo.back().data());
            terr_redo.pop_back(); redo_kind.pop_back();
            if (!lakes.empty() || !rivers.empty()) rebuild_water();
            if (g3d_watersim_active()) watersim_sync(true);
            status = "Relieve rehecho";
            return;
        }
        if (redo_stack.empty()) { redo_kind.pop_back(); return; }
        undo_stack.push_back(EditState{ objects, cam_follow }); undo_kind.push_back('o');
        apply_state(redo_stack.back());
        last_state = redo_stack.back();
        redo_stack.pop_back(); redo_kind.pop_back();
        status = "Rehecho";
    };



    // carga el script de un objeto en el editor (o una plantilla si no existe)
    // ---------------------------------------------------------------------------
    // PLANTILLA DEL SCRIPT DE UN OBJETO
    // El comportamiento de un objeto vive en SU script, no escondido en el main:
    // controles si es el jugador, cuerpo rigido si tiene fisica. El editor solo la
    // escribe la primera vez; a partir de ahi el fichero es tuyo y no se toca.
    // ---------------------------------------------------------------------------
    /* ---- Los huecos de estado que gasta cada regla ----
       Una regla necesita recordar cosas entre frames: si ya se disparo (una vez
       para siempre), si su condicion se cumplia el frame anterior (para saltar al
       cumplirse y no sesenta veces por segundo) y su reloj. Van en arrays GLOBAL
       de main.prg, un hueco por regla, numerados SIEMPRE igual: primero los
       objetos en su orden y luego los personajes. Asi el numero sale el mismo
       generando el juego y editando el script de uno solo. */
    /* El nombre de la GLOBAL donde vive un sonido cargado. Tiene que salir igual
       en el sitio que lo carga y en el que lo hace sonar. */
    auto var_sonido = [](const std::string& fichero) {
        std::string base = fs::path(fichero).stem().string();
        return "snd_" + ident_bgd(base, "s");
    };
    auto total_reglas_obj = [&]() {
        int n = 0; for (auto& q : objects) n += (int)q.reglas.size(); return n;
    };
    auto base_reglas_obj = [&](const SObj& o) {
        int n = 0;
        for (auto& q : objects) { if (&q == &o) break; n += (int)q.reglas.size(); }
        return n;
    };
    auto base_reglas_spr = [&](const SprObj& sp) {
        int n = total_reglas_obj();
        for (auto& q : sprites) { if (&q == &sp) break; n += (int)q.reglas.size(); }
        return n;
    };
    auto total_reglas = [&]() {
        int n = total_reglas_obj();
        for (auto& q : sprites) n += (int)q.reglas.size();
        return n;
    };

    /* ---- Girar la camara: lo que se mete en el proceso del jugador ----
       Escribe escena_orbita, que es de donde salen el brazo de la camara y los
       controles. Con teclas y mando el giro es por segundo (por eso escena_dt);
       con el raton, por pixel movido. */
    auto girar_camara_codigo = [&]() -> std::string {
        if (!cam_girable || cam_mode != 1) return std::string();
        char b[900];
        if (cam_gira_con == 0) {
            snprintf(b, sizeof(b),
                "        // ---------- GIRAR LA CAMARA (raton) ----------\n"
                "        g3d_mouse_update();\n"
                "        escena_orbita = escena_orbita - g3d_mouse_dx() * %.3f;\n",
                cam_sens);
        } else if (cam_gira_con == 1) {
            snprintf(b, sizeof(b),
                "        // ---------- GIRAR LA CAMARA (teclas Q y E) ----------\n"
                "        IF (key(_Q)) escena_orbita = escena_orbita - %.1f * escena_dt; END\n"
                "        IF (key(_E)) escena_orbita = escena_orbita + %.1f * escena_dt; END\n",
                cam_gira_vel * 1000.0f, cam_gira_vel * 1000.0f);
        } else {
            snprintf(b, sizeof(b),
                "        // ---------- GIRAR LA CAMARA (stick derecho) ----------\n"
                "        // El eje va de -32767 a 32767. La zona muerta (un cuarto) evita\n"
                "        // que la camara derive sola con el stick en reposo.\n"
                "        IF (joy_getaxis(JOY_AXIS_RIGHTX) > 8000 OR joy_getaxis(JOY_AXIS_RIGHTX) < -8000)\n"
                "            escena_orbita = escena_orbita + joy_getaxis(JOY_AXIS_RIGHTX) / 32000.0 * %.1f * escena_dt;\n"
                "        END\n",
                cam_gira_vel * 1000.0f);
        }
        return b;
    };
    // Los controles del jugador, atados a la camara. Si la camara se puede girar,
    // el angulo se lee en marcha; si no, se hornean el seno y el coseno.
    auto ejes_camara_codigo = [&](const char* tab) -> std::string {
        char b[700];
        if (cam_girable && cam_mode == 1) {
            snprintf(b, sizeof(b),
                "%swx = adel * (0.0 - sin(escena_orbita)) + lat * (0.0 - cos(escena_orbita));\n"
                "%swz = adel * cos(escena_orbita) + lat * (0.0 - sin(escena_orbita));\n", tab, tab);
        } else {
            float ob = cam_orbit * 0.0174533f;
            snprintf(b, sizeof(b),
                "%swx = adel * %.4f + lat * %.4f;\n"
                "%swz = adel * %.4f + lat * %.4f;\n",
                tab, -sinf(ob), -cosf(ob), tab, cosf(ob), -sinf(ob));
        }
        return b;
    };

    /* ---- El sonido propio de un objeto, en codigo ----
       Una cascada o una hoguera suenan mas fuerte cuanto mas cerca estas. El canal
       se guarda en amb_ch[] (un hueco por objeto con sonido): se abre al entrar en
       el radio y se cierra al salir, que si no cada frame arrancaria una copia.
       Sin variables PRIVATE: las tres plantillas de objeto tienen bloques
       distintos, y las GLOBAL valen para todas. */
    auto amb_codigo = [&](const SObj& o, int slot) -> std::string {
        if (o.amb_sonido.empty() || slot < 0) return std::string();
        float r = o.amb_radio > 0.5f ? o.amb_radio : 0.5f;
        char b[1200];
        snprintf(b, sizeof(b),
            "        // ---- su sonido: se oye mas de cerca ----\n"
            "        IF ((jug_x - x) * (jug_x - x) + (jug_z - z) * (jug_z - z) < %.3f)\n"
            "            IF (amb_ch[%d] < 0) amb_ch[%d] = sound_play(%s, -1); END   // -1 = en bucle\n"
            "            channel_set_volume(amb_ch[%d], %d - %d * sqrt((jug_x - x) * (jug_x - x) + (jug_z - z) * (jug_z - z)) / %.3f);\n"
            "        ELSE\n"
            "            IF (amb_ch[%d] >= 0)  sound_stop(amb_ch[%d]);  amb_ch[%d] = -1;  END\n"
            "        END\n",
            r * r, slot, slot, var_sonido(o.amb_sonido).c_str(),
            slot, o.amb_vol, o.amb_vol, r, slot, slot, slot);
        return b;
    };
    // El hueco de amb_ch[] de cada objeto con sonido (y cuantos hay).
    auto slot_amb = [&](const SObj& o) -> int {
        int n = 0;
        for (auto& q : objects) {
            if (&q == &o) return q.amb_sonido.empty() ? -1 : n;
            if (!q.amb_sonido.empty()) n++;
        }
        return -1;
    };
    auto total_amb = [&]() {
        int n = 0; for (auto& q : objects) if (!q.amb_sonido.empty()) n++; return n;
    };

    /* ---- Las reglas, en codigo BennuGD2 ----
       `arranque` es lo que va antes del LOOP del proceso y `dentro` lo que va en
       el. `cuerpo` dice si el proceso tiene un cuerpo rigido (para deshacerlo al
       destruir) y `es_sprite` si lo que se destruye es un sprite y no una entidad. */
    /* 'radio_extra' es el medio-tamanio del objeto: la distancia de "tocar" se mide
       desde su BORDE y no desde su centro. Sin esto, una casa de 13 unidades de
       ancho con el radio de 3 que viene puesto no se dispara nunca -- para estar a
       3 del centro habria que meterse dentro, y la casa es solida. */
    auto reglas_codigo = [&](const std::vector<Regla>& rs, int base, int cuerpo, int es_sprite,
                             std::string& arranque, std::string& dentro, float radio_extra = 0.0f) {
        arranque.clear(); dentro.clear();
        for (int k = 0; k < (int)rs.size(); k++) {
            const Regla& r = rs[k];
            if (r.acciones.empty()) continue;
            int S = base + k;
            char b[900];

            // ---- lo que hay que hacer, con la sangria que le toque ----
            auto cuerpo_acciones = [&](const char* tab) {
                std::string out;
                for (auto& a : r.acciones) {
                    char l[700];
                    if (a.tipo == 0) {
                        if (a.proc.empty()) continue;
                        snprintf(l, sizeof(l), "%s%s();   // tu codigo\n", tab, a.proc.c_str());
                    } else if (a.tipo == 1) {
                        if (a.var.empty()) continue;
                        int v = (int)(a.valor >= 0.0f ? a.valor + 0.5f : a.valor - 0.5f);
                        if (a.op == 0)      snprintf(l, sizeof(l), "%s%s = %d;\n", tab, a.var.c_str(), v);
                        else if (a.op == 1) snprintf(l, sizeof(l), "%s%s = %s + %d;\n", tab, a.var.c_str(), a.var.c_str(), v);
                        else                snprintf(l, sizeof(l), "%s%s = %s - %d;\n", tab, a.var.c_str(), a.var.c_str(), v);
                    } else if (a.tipo == 2) {
                        std::string q = tab;
                        if (es_sprite) q += "g3d_sprite_destroy(entity);\n";
                        else           q += "g3d_entity_destroy(entity);\n";
                        if (cuerpo) { q += tab; q += "g3d_rigidbody_destroy(cuerpo);\n"; }
                        q += tab; q += "RETURN;   // este proceso se acaba aqui\n";
                        out += q; continue;
                    } else if (a.tipo == 10) {
                        snprintf(l, sizeof(l), "%spartida_guardar(%d);\n", tab, a.ranura);
                    } else if (a.tipo == 11) {
                        snprintf(l, sizeof(l), "%spartida_cargar(%d);\n", tab, a.ranura);
                    } else if (a.tipo == 9) {
                        if (a.dialogo.empty()) continue;
                        snprintf(l, sizeof(l), "%sIF (NOT exists(TYPE %s)) %s(); END\n",
                                 tab, a.dialogo.c_str(), a.dialogo.c_str());
                    } else if (a.tipo == 8) {
                        if (a.menu.empty()) continue;
                        // no se abre dos veces si ya esta puesto
                        snprintf(l, sizeof(l), "%sIF (NOT exists(TYPE %s)) %s(); END\n",
                                 tab, a.menu.c_str(), a.menu.c_str());
                    } else if (a.tipo == 7) {
                        snprintf(l, sizeof(l), "%sexit();\n", tab);
                    } else if (a.tipo == 5) {
                        int ne = indice_escena(a.escena);
                        if (ne < 0) continue;
                        /* No se cambia aqui mismo: quien pide el cambio es un objeto
                           de la escena que se va, y se moriria a media faena. Se deja
                           pedido y lo hace escena_gestor. */
                        snprintf(l, sizeof(l), "%sescena_pedida = %d;   // ir a %s\n",
                                 tab, ne, a.escena.c_str());
                    } else if (a.tipo == 4) {
                        if (a.sonido.empty()) continue;
                        // El sonido se carga UNA vez al montar la escena; aqui solo
                        // se toca. sound_play devuelve el canal, que no hace falta.
                        snprintf(l, sizeof(l), "%ssound_play(%s, 0);\n",
                                 tab, var_sonido(a.sonido).c_str());
                    } else {
                        // El texto vive en una GLOBAL que pinta escena_aviso con
                        // write_var: no hace falta crear y borrar el write.
                        std::string t = a.texto;
                        for (size_t i2 = 0; i2 < t.size(); i2++) if (t[i2] == '"') t[i2] = '\'';
                        snprintf(l, sizeof(l), "%saviso_txt = \"%s\"; aviso_t = %.2f;\n",
                                 tab, t.c_str(), a.seg > 0.05f ? a.seg : 2.0f);
                    }
                    out += l;
                }
                return out;
            };

            if (r.evento == 0) {                 // al empezar
                arranque += "    // ---- regla: al empezar ----\n";
                arranque += cuerpo_acciones("    ");
                continue;
            }
            if (r.evento == 1) {                 // cada frame
                dentro += "        // ---- regla: cada frame ----\n";
                dentro += cuerpo_acciones("        ");
                continue;
            }
            if (r.evento == 6) {                 // cada N segundos
                snprintf(b, sizeof(b),
                    "        // ---- regla: cada %.2f s ----\n"
                    "        regla_t[%d] = regla_t[%d] + escena_dt;\n"
                    "        IF (regla_t[%d] >= %.3f)\n"
                    "            regla_t[%d] = 0.0;\n",
                    r.cada, S, S, S, r.cada > 0.02f ? r.cada : 1.0f, S);
                dentro += b;
                if (r.una_vez) {
                    snprintf(b, sizeof(b), "            IF (regla_hecha[%d] == 0)\n", S);
                    dentro += b;
                    dentro += cuerpo_acciones("                ");
                    snprintf(b, sizeof(b), "                regla_hecha[%d] = 1;\n            END\n", S);
                    dentro += b;
                } else {
                    dentro += cuerpo_acciones("            ");
                }
                dentro += "        END\n";
                continue;
            }

            // ---- los que tienen condicion ----
            std::string cond;
            char c2[500];
            if (r.evento == 2) {                 // acercarse y pulsar
                std::string puls;
                if (!r.tecla.empty()) puls = "key(" + r.tecla + ")";
                if (!r.boton.empty()) { if (!puls.empty()) puls += " OR "; puls += "joy_getbutton(" + r.boton + ")"; }
                if (puls.empty()) continue;
                { float rr = r.radio + radio_extra;
                  snprintf(c2, sizeof(c2),
                    "(%s) AND (jug_x - x) * (jug_x - x) + (jug_z - z) * (jug_z - z) < %.3f",
                    puls.c_str(), rr * rr); }
                cond = c2;
            } else if (r.evento == 3) {          // el jugador lo toca
                /* radio + medio tamanio del objeto = "a esta distancia de su borde" */
                { float rr = r.radio + radio_extra;
                  snprintf(c2, sizeof(c2),
                    "(jug_x - x) * (jug_x - x) + (jug_z - z) * (jug_z - z) < %.3f", rr * rr); }
                cond = c2;
            } else if (r.evento == 4) {          // el jugador entra en la zona
                snprintf(c2, sizeof(c2), "g3d_zone_blocked(jug_x, jug_z, %d)", r.zona);
                cond = c2;
            } else if (r.evento == 5) {          // una variable cumple
                if (r.var.empty()) continue;
                const char* ops[] = { "==", "<>", "<", "<=", ">", ">=" };
                int v = (int)(r.valor >= 0.0f ? r.valor + 0.5f : r.valor - 0.5f);
                snprintf(c2, sizeof(c2), "%s %s %d", r.var.c_str(),
                         ops[(r.cmp >= 0 && r.cmp <= 5) ? r.cmp : 0], v);
                cond = c2;
            } else continue;

            const char* nombres[] = { "al empezar", "cada frame", "acercarse y pulsar",
                                      "el jugador lo toca", "el jugador entra en la zona",
                                      "una variable cumple", "cada N segundos" };
            snprintf(b, sizeof(b), "        // ---- regla: %s ----\n", nombres[r.evento]);
            dentro += b;
            std::string guarda;   // lo que abre el IF
            if (r.mientras) {
                snprintf(b, sizeof(b), "        IF (%s)\n", cond.c_str());
            } else {
                // al CUMPLIRSE: regla_ant recuerda si ya se cumplia el frame anterior,
                // que si no una moneda sumaria diez puntos sesenta veces por segundo.
                snprintf(b, sizeof(b), "        IF ((%s) AND regla_ant[%d] == 0)\n", cond.c_str(), S);
            }
            dentro += b;
            if (r.una_vez) {
                snprintf(b, sizeof(b), "            IF (regla_hecha[%d] == 0)\n", S);
                dentro += b;
                dentro += cuerpo_acciones("                ");
                snprintf(b, sizeof(b), "                regla_hecha[%d] = 1;\n            END\n", S);
                dentro += b;
            } else {
                dentro += cuerpo_acciones("            ");
            }
            dentro += "        END\n";
            if (!r.mientras) {
                snprintf(b, sizeof(b), "        regla_ant[%d] = (%s);\n", S, cond.c_str());
                dentro += b;
            }
        }
    };

    /* El tamanio de colision que le pega al modelo.
       El cuerpo que se genera es un cubo (o una esfera) de medio lado `csize`, y
       venia siempre a 1.0: en una roca de cinco unidades eso es una bolita
       enterrada en la base, y el personaje se metia dentro del dibujo como si no
       chocara. Se saca de la caja del modelo, a lo ancho, que es lo que corta el
       paso; con un tope para que algo muy plano y muy ancho no levante un muro
       invisible por encima. */
    auto csize_del_modelo = [&](const SObj& o) -> float {
        void* m = load_model(o.asset);
        float mn[3], mx[3];
        if (!m || !g3d_model_bounds(m, mn, mx)) return o.csize;
        float sc = o.scale > 0.001f ? o.scale : 1.0f;
        float ex = (mx[0] - mn[0]) * 0.5f * sc;
        float ey = (mx[1] - mn[1]) * 0.5f * sc;
        float ez = (mx[2] - mn[2]) * 0.5f * sc;
        float c = ex > ez ? ex : ez;
        if (c > ey * 1.5f) c = ey * 1.5f;      // tope: no levantar torres invisibles
        if (c < 0.1f) c = 0.1f; if (c > 50.0f) c = 50.0f;
        return c;
    };

    auto object_script_template = [&](const SObj& o) -> std::string {
        char b[4096];
        std::string acc_ini, acc_loop;
        reglas_codigo(o.reglas, base_reglas_obj(o),
                      ((o.phys >= 1 && o.phys <= 4) || o.phys == 7) ? 1 : 0, 0, acc_ini, acc_loop,
                      o.csize > 0.05f ? o.csize : 0.0f);
        acc_loop += amb_codigo(o, slot_amb(o));

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
                "//   ent    = la entidad de este objeto en la escena\n"
                "//   modelo = su modelo, necesario para las animaciones\n"
                /* El parametro NO puede llamarse 'id': en BennuGD2 'id' es el
                   identificador del propio proceso. Al pisarlo con el numero de la
                   entidad, cualquier cosa que preguntara por este proceso se volvia
                   loca -- signal(TYPE ...) se llamaba a si misma sin fin y tumbaba
                   el juego al cambiar de escena. */
                "PROCESS " + o.name + "(int ent, int modelo)\n"
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
                "    entity = ent;\n";
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
                "    facing = 0.0; t = 0.0;\n"
                /* Si venimos de cargar una partida, el jugador no empieza donde
                   esta puesto en la escena, sino donde lo dejaste. Va aqui, con la
                   capsula ya creada, no antes: 'ch' no existiria todavia. */
                "    IF (hay_vuelta)\n"
                "        g3d_char_set_position(ch, volver_x, volver_y, volver_z);\n"
                "        x = volver_x;  y = volver_y;  z = volver_z;\n";
            // el angulo solo si la camara es girable: si no, escena_orbita no existe
            if (cam_girable && cam_mode == 1)
                fmt += "        escena_orbita = volver_a;\n";
            fmt += "        hay_vuelta = 0;\n"
                   "    END\n";
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
                /* Los controles van RESPECTO A LA CAMARA, no a los ejes del mundo.
                   Estaban clavados a los ejes (W = +Z siempre), que solo coincide
                   con la camara a la espalda: girada de perfil, la D llevaba al
                   personaje HACIA la camara en vez de a la derecha de la pantalla.
                   El angulo es constante en la escena, asi que los factores se
                   calculan aqui una vez y no en cada frame. */
                /* La derecha de la pantalla es cross(mirada, arriba), que es lo que
                   hace mat4_look_at. Medido proyectando un paso con la matematica
                   del motor: con la camara a la espalda sale -X, o sea que la A y
                   la D iban CAMBIADAS desde siempre. El cenital se midio aparte y
                   ese si estaba bien, por eso solo se corrige la tercera persona. */
                float ob = cam_orbit * 0.0174533f;
                float fx = -sinf(ob), fz =  cosf(ob);   // hacia donde mira la camara
                float rx = -cosf(ob), rz = -sinf(ob);   // la derecha de la PANTALLA
                char mv[1400];
                // girar la camara va ANTES de leer los controles: si no, el paso de
                // este frame usaria el angulo del anterior y se notaria el retraso.
                s += girar_camara_codigo();
                if (cam_mode == 1 && cam_25d) {
                    snprintf(mv, sizeof(mv),
                "        // ---------- CONTROLES (2.5D: solo izquierda y derecha) ----------\n"
                "        // Plataformas de perfil: la profundidad esta bloqueada a proposito.\n"
                "        // Si quieres que ademas se pueda entrar y salir del plano, anade\n"
                "        // aqui el eje que falta (adel) como en la plantilla normal.\n"
                "        lat = 0.0;\n"
                "        IF (key(_D) OR key(_RIGHT)) lat = lat + 1.0; END\n"
                "        IF (key(_A) OR key(_LEFT))  lat = lat - 1.0; END\n"
                "        wx = lat * %.4f; wz = lat * %.4f;   // eje de la pantalla\n",
                        rx, rz);
                } else if (cam_mode == 1) {
                    snprintf(mv, sizeof(mv),
                "        // ---------- CONTROLES (respecto a la camara) ----------\n"
                "        // La camara mira al personaje desde %.0f grados, asi que W lleva\n"
                "        // hacia el fondo DE LA PANTALLA y D a su derecha, no a los ejes\n"
                "        // del mundo. Los factores son el seno y el coseno de ese angulo.\n"
                "        adel = 0.0; lat = 0.0;\n"
                "        IF (key(_W) OR key(_UP))    adel = adel + 1.0; END\n"
                "        IF (key(_S) OR key(_DOWN))  adel = adel - 1.0; END\n"
                "        IF (key(_D) OR key(_RIGHT)) lat  = lat  + 1.0; END\n"
                "        IF (key(_A) OR key(_LEFT))  lat  = lat  - 1.0; END\n"
                "%s",
                        cam_orbit, ejes_camara_codigo("        ").c_str());
                } else {
                    snprintf(mv, sizeof(mv),
                "        // ---------- CONTROLES ----------\n"
                "        wx = 0.0; wz = 0.0;\n"
                "        IF (key(_W)) wz = wz + 1.0; END\n"
                "        IF (key(_S)) wz = wz - 1.0; END\n"
                "        IF (key(_D)) wx = wx + 1.0; END\n"
                "        IF (key(_A)) wx = wx - 1.0; END\n");
                }
                fmt += mv;
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
                 "        angle_y = facing;\n"
                 "        jug_x = px; jug_y = py; jug_z = pz;   // lo leen los NPC y los objetos\n";

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
            s += acc_loop;
            s += "\n        FRAME;\n    END\nEND\n";
            return s;
        }

        // ---------------- OBJETO CON FISICA ----------------
        // 1..4 = formas sueltas (caja, esfera, capsula, cilindro); 7 = la forma
        // del propio modelo (envolvente convexa), que es la que no hay que ajustar.
        if ((o.phys >= 1 && o.phys <= 4) || o.phys == 7) {
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
            if (o.phys == 7) {
                /* LA FORMA DEL MODELO. Una envolvente convexa hecha con los vertices
                   del propio modelo: se ajusta sola, sin dar tamanios a ojo. Jolt no
                   admite malla exacta en algo que se mueve, asi que para lo que se
                   mueve esto es lo mas ajustado que hay. */
                snprintf(b, sizeof(b),
                         "    // la colision es LA FORMA DEL MODELO (envolvente convexa)\n"
                         "    cuerpo = g3d_rigidbody_create_convex_model(%.3f, %.3f, %.3f, modelo, %.3f, %.3f);\n",
                         o.x, o.y, o.z, o.scale, o.mass);
            } else {
                const char* mk =
                    (o.phys == 1) ? "    cuerpo = g3d_rigidbody_create(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n"
                  : (o.phys == 2) ? "    cuerpo = g3d_rigidbody_create_sphere(%.3f, %.3f, %.3f, %.3f, %.3f);\n"
                  : (o.phys == 3) ? "    cuerpo = g3d_rigidbody_create_capsule(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n"
                                  : "    cuerpo = g3d_rigidbody_create_cylinder(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n";
                if (o.phys == 1)      snprintf(b, sizeof(b), mk, o.x, by0, o.z, c, c, c, o.mass);
                else if (o.phys == 2) snprintf(b, sizeof(b), mk, o.x, by0, o.z, c, o.mass);
                else                  snprintf(b, sizeof(b), mk, o.x, by0, o.z, c, c, o.mass);
            }
            s += b;
            snprintf(b, sizeof(b), "    g3d_rigidbody_set_bounce(cuerpo, %.3f, %.3f);   // rebote, friccion\n",
                     o.bounce, o.friction);
            s += b;
            s += acc_ini;
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
                 "        angle_z = g3d_rigidbody_angle_z(cuerpo);\n";
            s += acc_loop;
            s += "        FRAME;\n    END\nEND\n";
            return s;
        }

        // ---------------- OBJETO DECORATIVO (sin fisica) ----------------
        // Modelos sin esqueleto con piezas atadas a nodos animados hay que posarlos
        // UNA vez o esas piezas no se colocan y no se ven.
        std::string pose;
        { void* mm = load_model(o.asset);
          if (mm && g3d_model_animation_count(mm) > 0 && !g3d_model_is_skinned(mm))
              pose = "    g3d_model_animate_all(modelo, 0.0, 0);   // posar (piezas en nodos)\n"; }
        char dh[4096];
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
            "    g3d_entity_set_collider(entity, 1);   // solido: el agua lo rodea y salpica en el\n"
            "%s%s"
            "    LOOP\n"
            "        // ... tu logica (por ejemplo: angle_y = angle_y + 500; para girarlo) ...\n"
            "%s"
            "        FRAME;\n"
            "    END\n"
            "END\n",
            o.name.c_str(), o.name.c_str(),
            o.x, o.y, o.z, o.ry * 57295.78f, o.scale * 100.0f, pose.c_str(),
            acc_ini.c_str(), acc_loop.c_str());
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
        std::string sp = scripts_dir + "/" + objname + ".prg";
        // Si aun no existe se abre con la plantilla que le toque (el jugador con
        // sus controles, un cuerpo fisico con su rigid body, o vacia): asi la
        // pestania nace con algo que compila, y se escribe al guardar.
        std::string plantilla;
        {
            FILE* f = fopen(sp.c_str(), "r");
            if (f) fclose(f);
            else {
                const SObj* po = nullptr;
                for (auto& o : objects) if (o.name == objname) { po = &o; break; }
                plantilla = po ? object_script_template(*po)
                               : ("PROCESS " + objname + "(int ent)\nBEGIN\n    LOOP\n        FRAME;\n    END\nEND\n");
            }
        }
        int i = code_abrir(sp, objname + ".prg", objname,
                           plantilla.empty() ? nullptr : plantilla.c_str());
        if (!plantilla.empty()) docs[i]->sucio = true;   // aun no esta en el disco
    };
    // Guardar la escena: una linea OBJECT por objeto (fuente de verdad del juego).
    auto save_scene = [&](const std::string& path) {
        FILE* f = fopen(path.c_str(), "w");
        if (!f) { status = "ERROR guardando escena"; return; }
        fputs("# escena del editor BennuGD2\n", f);
        // (las variables del juego van en el .bgd2 del proyecto, no aqui)
        // ---- sonido de la escena ----
        if (!esc_musica.empty())
            fprintf(f, "MUSICA %d|%d|%.2f|%s\n", esc_mus_vol, esc_mus_loop, esc_mus_fade,
                    esc_musica.c_str());
        for (auto& z : zsonidos)
            fprintf(f, "ZSONIDO %d|%d|%s\n", z.zona, z.vol, z.sonido.c_str());
        fprintf(f, "WATER %d %.4f %.4f %.4f %.4f %.4f %.3f %.3f %.3f %.3f %.3f %.3f %.4f %.4f %.4f %.4f %.4f %.2f %.4f %.4f %.4f\n",
                water_on ? 1 : 0, water_level, w_amp, w_len, w_speed, w_swell,
                w_deep[0], w_deep[1], w_deep[2], w_shallow[0], w_shallow[1], w_shallow[2],
                surf_amount, surf_len, surf_speed, surf_runup, surf_height, surf_dir,
                water_foam, splash_amount, splash_speed);
        /* Manantiales. Estaban en memoria con un comentario que decia "para
           guardar", pero nadie los escribia: colocabas uno, guardabas la escena
           y al reabrirla ya no estaba. */
        for (auto& sw : wsources)
            fprintf(f, "SOURCE %.4f %.4f %.4f\n", sw.x, sw.z, sw.rate);
        fprintf(f, "FLOWCFG %.4f %.4f %.1f\n", ws_evap, ws_rate, ws_prefill);
        fprintf(f, "SUN %d %.2f %.2f %.3f %.2f %.2f\n",
                sun_cycle ? 1 : 0, sun_day_sec, sun_hour, sun_intensity, sun_azim, sun_elev);
        fprintf(f, "CAMERA %d %d %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %d %.2f %d %d %d %.1f\n",
                cam_mode, cam_follow, cam_pos[0], cam_pos[1], cam_pos[2],
                cam_look[0], cam_look[1], cam_look[2], gcam_dist, cam_height, cam_fwd, cam_sens,
                shadow_res, cam_orbit, cam_25d ? 1 : 0,
                cam_girable, cam_gira_con, cam_gira_vel);
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
            if (!o.amb_sonido.empty())
                fprintf(f, "OBJAMB %.3f|%d|%s\n", o.amb_radio, o.amb_vol, o.amb_sonido.c_str());
            // Sus reglas, detras de su linea: una REGLA y las ACCIONES que cuelgan
            // de ella. Van por | porque los nombres y los textos llevan de todo.
            for (auto& r : o.reglas) {
                fprintf(f, "OBJREGLA %d|%.3f|%s|%s|%d|%s|%d|%.3f|%.3f|%d|%d\n",
                        r.evento, r.radio, r.tecla.c_str(), r.boton.c_str(), r.zona,
                        r.var.c_str(), r.cmp, r.valor, r.cada, r.una_vez, r.mientras);
                for (auto& a : r.acciones)
                    fprintf(f, "OBJRACC %d|%s|%s|%s|%d|%.3f|%.2f|%s|%s|%d|%s|%s|%s|%d\n",
                            a.tipo, a.archivo.c_str(), a.proc.c_str(), a.var.c_str(),
                            a.op, a.valor, a.seg, a.texto.c_str(),
                            a.sonido.c_str(), a.vol, a.escena.c_str(), a.menu.c_str(), a.dialogo.c_str(), a.ranura);
            }
        }
        // ---- SPRITES 2D del mundo (hojas de sprites) ----
        for (auto& sp : sprites) {
            fprintf(f, "SPRITE3D %.4f %.4f %.4f %.3f %d %d %d %d %.3f\n",
                    sp.x, sp.y, sp.z, sp.height, sp.dirs, sp.billboard,
                    sp.shadow, sp.smooth, sp.cutout);
            fprintf(f, "SPRNAME %s\n", sp.name.c_str());
            if (!sp.sheet.empty()) fprintf(f, "SPRSHEET %s\n", sp.sheet.c_str());
            if (!sp.anim.empty())  fprintf(f, "SPRANIM %s\n", sp.anim.c_str());
            fprintf(f, "SPRPHYS %d %.3f %.3f %.3f %.3f %d %.3f %d\n",
                    sp.phys, sp.mass, sp.bounce, sp.friction, sp.csize,
                    sp.buoyant, sp.density, sp.zone_layer);
            if (sp.is_player) {
                fprintf(f, "SPRPLAYER %.3f %.3f %.3f %.3f %.3f %d\n",
                        sp.walk_speed, sp.run_speed, sp.jump_force,
                        sp.char_radius, sp.char_height,
                        ((int)(&sp - &sprites[0]) == spr_follow) ? 1 : 0);
                fprintf(f, "SPRKEYS %s %s %s %s %s %s\n",
                        sp.k_up.c_str(), sp.k_down.c_str(), sp.k_left.c_str(),
                        sp.k_right.c_str(), sp.k_jump.c_str(), sp.k_run.c_str());
                fprintf(f, "SPRSTATES %s|%s|%s|%s\n",
                        sp.an_idle.c_str(), sp.an_walk.c_str(),
                        sp.an_run.c_str(), sp.an_jump.c_str());
                fprintf(f, "SPRKEYANIM %s|%s|%s|%s\n",
                        sp.an_up.c_str(), sp.an_down.c_str(),
                        sp.an_left.c_str(), sp.an_right.c_str());
                fprintf(f, "SPRKEYFLIP %d %d %d %d\n",
                        sp.fx_up, sp.fx_down, sp.fx_left, sp.fx_right);
                fprintf(f, "SPRFPS %d %d %d\n", sp.fps, sp.iluminado, sp.ajuste_px);
                fprintf(f, "SPRCOMP %d %.3f %.3f %.3f %.3f\n",
                        sp.comport, sp.com_vel, sp.com_radio, sp.com_bx, sp.com_bz);
                fprintf(f, "SPRNPC %d|%.3f|%d|%.3f|%d|%s|%s|%s|%s|%s|%s\n",
                        sp.solido, sp.sol_radio, sp.inter_on, sp.inter_radio, sp.inter_mirar,
                        sp.inter_tecla.c_str(), sp.inter_boton.c_str(),
                        sp.inter_anim.c_str(), sp.inter_llama.c_str(), sp.inter_arch.c_str(),
                        sp.inter_dialogo.c_str());
                fprintf(f, "SPRPAD %d|%s|%s\n", sp.usar_mando,
                        sp.b_jump.c_str(), sp.b_run.c_str());
                for (auto& r : sp.reglas) {
                    fprintf(f, "SPRREGLA %d|%.3f|%s|%s|%d|%s|%d|%.3f|%.3f|%d|%d\n",
                            r.evento, r.radio, r.tecla.c_str(), r.boton.c_str(), r.zona,
                            r.var.c_str(), r.cmp, r.valor, r.cada, r.una_vez, r.mientras);
                    for (auto& a : r.acciones)
                        fprintf(f, "SPRRACC %d|%s|%s|%s|%d|%.3f|%.2f|%s|%s|%d|%s|%s|%s|%d\n",
                                a.tipo, a.archivo.c_str(), a.proc.c_str(), a.var.c_str(),
                                a.op, a.valor, a.seg, a.texto.c_str(),
                                a.sonido.c_str(), a.vol, a.escena.c_str(), a.menu.c_str(), a.dialogo.c_str(), a.ranura);
                }
                for (auto& ac : sp.acciones)
                    fprintf(f, "SPRACCION %d|%d|%s|%s|%s|%s|%s|%s|%s\n",
                            ac.una_vez, ac.espejo, ac.nombre.c_str(), ac.tecla.c_str(),
                            ac.boton.c_str(), ac.anim.c_str(), ac.llama.c_str(), ac.archivo.c_str(),
                            ac.dialogo.c_str());
            }
        }
        // ---- HUD 2D ----
        // Una linea por dato: los nombres de fichero y los textos pueden llevar
        // espacios, asi que cada cadena ocupa su propia linea entera.
        for (auto& h : hud) {
            fprintf(f, "HUD2D %d %.0f %.0f %d %d %.2f %.2f %d %d %d %d %d %d %d %d\n",
                    h.type, h.x, h.y, h.z, h.alpha, h.size, h.angle, h.flags, h.code,
                    h.align, h.col[0], h.col[1], h.col[2], h.col[3], h.vartype);
            fprintf(f, "HUDNAME %s\n", h.name.c_str());
            if (!h.asset.empty()) fprintf(f, "HUDASSET %s\n", h.asset.c_str());
            if (!h.font.empty())  fprintf(f, "HUDFONT %s\n", h.font.c_str());
            if (!h.var.empty())   fprintf(f, "HUDVAR %s\n", h.var.c_str());
            if (!h.text.empty())  fprintf(f, "HUDTEXT %s\n", h.text.c_str());
        }
        fclose(f);
        if (terrain) {
            g3d_editor_terrain_save(terrain, (path + ".terrain").c_str());
            // La siembra tambien aparte: diez mil arboles ahogarian la escena.
            g3d_scatter_save((path + ".scatter").c_str());
            // Los desvios laterales van aparte: el .terrain lo leen escenas ya
            // hechas y ampliarlo las romperia.
            g3d_editor_terrain_save_xz(terrain, (path + ".terrain.xz").c_str());
        }
        if (!sheet.image.empty() && !sheet.frames.empty()) sheet_save();   // la hoja abierta
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
        hud.clear(); hud_sel = -1;   // el HUD tambien es de la escena
        for (auto& sp : sprites) if (sp.entity >= 0) g3d_sprite_destroy(sp.entity);
        sprites.clear(); spr_sel = -1; spr_follow = -1;
        esc_musica.clear(); zsonidos.clear();
        lakes.clear(); rivers.clear(); river_draft.clear(); waterfalls.clear();
        wsources.clear();   // los de la escena anterior no son de esta
        // terreno primero: las cuevas/objetos se apoyan en su altura
        if (terrain) {
            g3d_editor_terrain_load(terrain, (path + ".terrain").c_str());
            // Sin fichero de desvios, deja la rejilla limpia: si no, los de la
            // escena anterior se quedarian puestos en esta.
            g3d_editor_terrain_load_xz(terrain, (path + ".terrain.xz").c_str());
            // Sin fichero deja el campo vacio, para que una escena sin siembra no
            // herede la vegetacion de la anterior.
            g3d_scatter_load((path + ".scatter").c_str(), 1.0f);
            g3d_instances_set_lod_distance(sc_lod);
        }
        g3d_editor_paint_load((path + ".paint.png").c_str());
        if (!g3d_zone_load((path + ".zones").c_str())) g3d_zone_init(161, 400.0f);  // zonas (o limpia)
        char line[512], asset[256], name[256];
        while (fgets(line, sizeof(line), f)) {
            // ---- SPRITES 2D del mundo ----
            {   SprObj sp; int nl;
                nl = sscanf(line, "SPRITE3D %f %f %f %f %d %d %d %d %f",
                            &sp.x, &sp.y, &sp.z, &sp.height, &sp.dirs,
                            &sp.billboard, &sp.shadow, &sp.smooth, &sp.cutout);
                if (nl >= 5) { sprites.push_back(sp); continue; }
            }
            if (!sprites.empty()) {
                char val[384];
                if (sscanf(line, "SPRNAME %383[^\n]", val) == 1)  { sprites.back().name  = val; continue; }
                if (sscanf(line, "SPRSHEET %383[^\n]", val) == 1) { sprites.back().sheet = val; continue; }
                if (sscanf(line, "SPRANIM %383[^\n]", val) == 1)  { sprites.back().anim  = val; continue; }
                {   SprObj& sp = sprites.back();
                    int ph, bu, zl; float ms, bo, fr, cs, de;
                    if (sscanf(line, "SPRPHYS %d %f %f %f %f %d %f %d",
                               &ph,&ms,&bo,&fr,&cs,&bu,&de,&zl) == 8) {
                        sp.phys=ph; sp.mass=ms; sp.bounce=bo; sp.friction=fr;
                        sp.csize=cs; sp.buoyant=bu; sp.density=de; sp.zone_layer=zl;
                        continue;
                    }
                    float ws, rs, jf, cr, chh; int sig = 0;
                    if (sscanf(line, "SPRPLAYER %f %f %f %f %f %d",
                               &ws,&rs,&jf,&cr,&chh,&sig) >= 5) {
                        sp.is_player = 1; sp.walk_speed=ws; sp.run_speed=rs; sp.jump_force=jf;
                        sp.char_radius=cr; sp.char_height=chh;
                        if (sig) spr_follow = (int)sprites.size() - 1;
                        continue;
                    }
                    char k1[32],k2[32],k3[32],k4[32],k5[32],k6[32];
                    if (sscanf(line, "SPRKEYS %31s %31s %31s %31s %31s %31s",
                               k1,k2,k3,k4,k5,k6) == 6) {
                        sp.k_up=k1; sp.k_down=k2; sp.k_left=k3;
                        sp.k_right=k4; sp.k_jump=k5; sp.k_run=k6;
                        continue;
                    }
                    // trocea una linea "a|b|c|..." (los campos pueden ir vacios)
                    auto trocear = [](const char* txt, std::string* out, int n) {
                        std::string t(txt);
                        while (!t.empty() && (t.back()=='\n' || t.back()=='\r')) t.pop_back();
                        int np = 0; size_t ini = 0;
                        for (size_t k = 0; k <= t.size() && np < n; k++)
                            if (k == t.size() || t[k] == '|') { out[np++] = t.substr(ini, k - ini); ini = k + 1; }
                        return np;
                    };
                    {   int fp, il = 1, ap = 1;
                        if (sscanf(line, "SPRFPS %d %d %d", &fp, &il, &ap) >= 1) {
                            sp.fps = fp; sp.iluminado = il; sp.ajuste_px = ap; continue;
                        }
                    }
                    {   int cp; float cv, cr, bx, bz;
                        if (sscanf(line, "SPRCOMP %d %f %f %f %f", &cp,&cv,&cr,&bx,&bz) == 5) {
                            sp.comport = cp; sp.com_vel = cv; sp.com_radio = cr;
                            sp.com_bx = bx; sp.com_bz = bz;
                            continue;
                        }
                    }
                    if (!strncmp(line, "SPRNPC ", 7)) {
                        std::string p9[11];
                        if (trocear(line + 7, p9, 11) >= 5) {
                            sp.solido      = atoi(p9[0].c_str());
                            sp.sol_radio   = (float)atof(p9[1].c_str());
                            sp.inter_on    = atoi(p9[2].c_str());
                            sp.inter_radio = (float)atof(p9[3].c_str());
                            sp.inter_mirar = atoi(p9[4].c_str());
                            sp.inter_tecla = p9[5]; sp.inter_boton = p9[6];
                            sp.inter_anim  = p9[7]; sp.inter_llama = p9[8];
                            sp.inter_arch  = p9[9]; sp.inter_dialogo = p9[10];
                        }
                        continue;
                    }
                    if (!strncmp(line, "SPRPAD ", 7)) {
                        std::string p3[3];
                        if (trocear(line + 7, p3, 3) >= 1) {
                            sp.usar_mando = atoi(p3[0].c_str());
                            sp.b_jump = p3[1]; sp.b_run = p3[2];
                        }
                        continue;
                    }
                    if (!strncmp(line, "SPRREGLA ", 9)) {
                        std::string p[11]; trozos(line + 9, p, 11);
                        Regla r;
                        r.evento   = atoi(p[0].c_str());
                        r.radio    = (float)atof(p[1].c_str());
                        r.tecla    = p[2]; r.boton = p[3];
                        r.zona     = atoi(p[4].c_str());
                        r.var      = p[5];
                        r.cmp      = atoi(p[6].c_str());
                        r.valor    = (float)atof(p[7].c_str());
                        r.cada     = (float)atof(p[8].c_str());
                        r.una_vez  = atoi(p[9].c_str());
                        r.mientras = atoi(p[10].c_str());
                        sp.reglas.push_back(r);
                        continue;
                    }
                    if (!strncmp(line, "SPRRACC ", 8) && !sp.reglas.empty()) {
                        std::string p[14]; trozos(line + 8, p, 14);
                        Accion a;
                        a.tipo    = atoi(p[0].c_str());
                        a.archivo = p[1]; a.proc = p[2]; a.var = p[3];
                        a.op      = atoi(p[4].c_str());
                        a.valor   = (float)atof(p[5].c_str());
                        a.seg     = (float)atof(p[6].c_str());
                        a.texto   = p[7];
                        a.sonido  = p[8];
                        if (!p[9].empty()) a.vol = atoi(p[9].c_str());
                        a.escena  = p[10]; a.menu = p[11]; a.dialogo = p[12];
                if (!p[13].empty()) a.ranura = atoi(p[13].c_str());
                        sp.reglas.back().acciones.push_back(a);
                        continue;
                    }
                    if (!strncmp(line, "SPRACCION ", 10)) {
                        std::string p7[9];
                        if (trocear(line + 10, p7, 9) >= 6) {
                            SprAccion ac;
                            ac.una_vez = atoi(p7[0].c_str());
                            ac.espejo  = atoi(p7[1].c_str());
                            ac.nombre  = p7[2]; ac.tecla = p7[3];
                            ac.boton   = p7[4]; ac.anim  = p7[5]; ac.llama = p7[6];
                            ac.archivo = p7[7]; ac.dialogo = p7[8];
                            sp.acciones.push_back(ac);
                        }
                        continue;
                    }
                    {   int e1, e2, e3, e4;
                        if (sscanf(line, "SPRKEYFLIP %d %d %d %d", &e1,&e2,&e3,&e4) == 4) {
                            sp.fx_up=e1; sp.fx_down=e2; sp.fx_left=e3; sp.fx_right=e4;
                            continue;
                        }
                    }
                    if (!strncmp(line, "SPRKEYANIM ", 11)) {
                        std::string t(line + 11);
                        while (!t.empty() && (t.back()=='\n' || t.back()=='\r')) t.pop_back();
                        std::string part[4]; int np = 0; size_t ini = 0;
                        for (size_t k = 0; k <= t.size() && np < 4; k++)
                            if (k == t.size() || t[k] == '|') { part[np++] = t.substr(ini, k - ini); ini = k + 1; }
                        sp.an_up = part[0]; sp.an_down = part[1];
                        sp.an_left = part[2]; sp.an_right = part[3];
                        continue;
                    }
                    if (!strncmp(line, "SPRSTATES ", 10)) {
                        // cuatro nombres separados por | (pueden ir vacios)
                        std::string t(line + 10);
                        while (!t.empty() && (t.back()=='\n' || t.back()=='\r')) t.pop_back();
                        std::string part[4]; int np = 0; size_t ini = 0;
                        for (size_t k = 0; k <= t.size() && np < 4; k++)
                            if (k == t.size() || t[k] == '|') { part[np++] = t.substr(ini, k - ini); ini = k + 1; }
                        sp.an_idle = part[0]; sp.an_walk = part[1];
                        sp.an_run  = part[2]; sp.an_jump = part[3];
                        continue;
                    }
                }
            }
            // ---- HUD 2D: la cabecera con los numeros y detras una linea por cadena ----
            {   HudItem h; int nleido;
                nleido = sscanf(line, "HUD2D %d %f %f %d %d %f %f %d %d %d %d %d %d %d %d",
                                &h.type, &h.x, &h.y, &h.z, &h.alpha, &h.size, &h.angle,
                                &h.flags, &h.code, &h.align,
                                &h.col[0], &h.col[1], &h.col[2], &h.col[3], &h.vartype);
                if (nleido >= 9) { h.text.clear(); hud.push_back(h); continue; }
            }
            if (!hud.empty()) {
                // Cadena hasta el final de la linea (puede llevar espacios).
                char val[384];
                if (sscanf(line, "HUDNAME %383[^\n]", val) == 1)  { hud.back().name  = val; continue; }
                if (sscanf(line, "HUDASSET %383[^\n]", val) == 1) { hud.back().asset = val; continue; }
                if (sscanf(line, "HUDFONT %383[^\n]", val) == 1)  { hud.back().font  = val; continue; }
                if (sscanf(line, "HUDVAR %383[^\n]", val) == 1)   { hud.back().var   = val; continue; }
                if (sscanf(line, "HUDTEXT %383[^\n]", val) == 1)  { hud.back().text  = val; continue; }
            }
            {   float sx2, sz2, sr2;
                if (sscanf(line, "SOURCE %f %f %f", &sx2, &sz2, &sr2) == 3) {
                    wsources.push_back({ sx2, sz2, sr2 });
                    continue;
                }
                float fe, fr, fp;
                int nf = sscanf(line, "FLOWCFG %f %f %f", &fe, &fr, &fp);
                if (nf >= 2) {
                    ws_evap = fe; ws_rate = fr;
                    if (nf >= 3) ws_prefill = fp;   // escenas viejas: se queda el defecto
                    continue;
                }
            }
            {   int sc_on; float sd, sh, si, sa, se;
                if (sscanf(line, "SUN %d %f %f %f %f %f",
                           &sc_on, &sd, &sh, &si, &sa, &se) == 6) {
                    sun_cycle = sc_on != 0; sun_day_sec = sd; sun_hour = sh;
                    sun_intensity = si; sun_azim = sa; sun_elev = se;
                    continue;
                }
            }
            int won; float wl, wa, wln, wsp, wsw, d0,d1,d2, s0,s1,s2, sa,sl,ss,sr, sh,sd, wf,spa,sps;
            int nw = sscanf(line, "WATER %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                            &won, &wl, &wa, &wln, &wsp, &wsw, &d0,&d1,&d2, &s0,&s1,&s2,
                            &sa,&sl,&ss,&sr, &sh,&sd, &wf,&spa,&sps);
            if (nw >= 2) {
                water_on = won; water_level = wl;
                if (nw >= 6) { w_amp=wa; w_len=wln; w_speed=wsp; w_swell=wsw; }
                if (nw >= 12) { w_deep[0]=d0;w_deep[1]=d1;w_deep[2]=d2;
                                w_shallow[0]=s0;w_shallow[1]=s1;w_shallow[2]=s2; }
                /* Campos anadidos despues: una escena guardada antes simplemente
                   no los trae y se quedan los valores por defecto. */
                if (nw >= 16) { surf_amount=sa; surf_len=sl; surf_speed=ss; surf_runup=sr; }
                if (nw >= 18) { surf_height=sh; surf_dir=sd; }
                if (nw >= 21) { water_foam=wf; splash_amount=spa; splash_speed=sps; }
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
            float corb = 0.0f;   // las escenas de antes van a la espalda, angulo 0
            int c25 = 0;
            int cgir = 0, cgcon = 0; float cgvel = 120.0f;
            int nleidos = sscanf(line, "CAMERA %d %d %f %f %f %f %f %f %f %f %f %f %d %f %d %d %d %f",
                       &cm, &cfol, &px,&py,&pz, &lx,&ly,&lz, &cd, &ch, &cf, &cs, &sres, &corb, &c25,
                       &cgir, &cgcon, &cgvel);
            if (nleidos >= 10) {   // las escenas de antes no traen el adelanto
                cam_mode = cm; cam_follow = cfol;
                cam_pos[0]=px; cam_pos[1]=py; cam_pos[2]=pz;
                cam_look[0]=lx; cam_look[1]=ly; cam_look[2]=lz;
                gcam_dist = cd; cam_height = ch; cam_fwd = cf; cam_sens = cs;
                cam_orbit = corb; cam_25d = (c25 != 0);
                cam_girable = cgir; cam_gira_con = cgcon; cam_gira_vel = cgvel;
                shadow_res = sres; g3d_renderer_set_shadow_resolution((unsigned)shadow_res);
                continue;
            }
            float x, y, z, ry, sc;
            if (sscanf(line, "OBJECT %255s %f %f %f %f %f SCRIPT %255s",
                       asset, &x, &y, &z, &ry, &sc, name) >= 6) {
                /* Si el modelo no aparece (lo has renombrado, movido a otro
                   proyecto, o falta), el objeto NO se tira: se queda en la escena
                   sin dibujo y se avisa. Antes se descartaba en silencio y, como
                   al generar se guarda la escena, el objeto desaparecia del
                   fichero: perdias el trabajo por un asset que faltaba. */
                void* m = load_model(asset);
                if (!m)
                    console_add(std::string("[escena] no encuentro Assets/") + asset +
                                " (objeto '" + name + "'). Se queda en la escena, sin dibujo:\n"
                                "  vuelve a ponerle el modelo en el Inspector, o recuperalo en Assets.\n");
                int e = m ? g3d_model_spawn(scene, m, x, y, z, 0.0f, 0.0f) : -1;
                /* SOLIDO, igual que al colocarlo a mano. Faltaba justo aqui: lo
                   que colocabas en la sesion quedaba marcado, pero al reabrir la
                   escena no, y entonces el agua dejaba de verlo. Sin esta marca
                   el rio no rodea la roca, no salpica en ella y la cascada no se
                   parte -- las tres cosas leen la misma lista. */
                if (e >= 0) g3d_entity_impl_set_collider(e, 1);
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
                continue;
            }
            /* Las reglas del objeto de arriba. Y las OBJACC de antes, que solo
               sabian llamar a un PROCESS: se leen como una regla con una accion,
               para que las escenas de estos dias sigan abriendo igual. */
            if (!strncmp(line, "OBJACC ", 7) && !objects.empty()) {
                std::string p6[6]; trozos(line + 7, p6, 6);
                Regla r; Accion a;
                r.evento = atoi(p6[0].c_str());
                r.radio  = (float)atof(p6[1].c_str());
                r.tecla  = p6[2]; r.boton = p6[3];
                a.tipo = 0; a.archivo = p6[4]; a.proc = p6[5];
                r.acciones.push_back(a);
                objects.back().reglas.push_back(r);
                continue;
            }
            if (!strncmp(line, "OBJREGLA ", 9) && !objects.empty()) {
                std::string p[11]; trozos(line + 9, p, 11);
                Regla r;
                r.evento   = atoi(p[0].c_str());
                r.radio    = (float)atof(p[1].c_str());
                r.tecla    = p[2]; r.boton = p[3];
                r.zona     = atoi(p[4].c_str());
                r.var      = p[5];
                r.cmp      = atoi(p[6].c_str());
                r.valor    = (float)atof(p[7].c_str());
                r.cada     = (float)atof(p[8].c_str());
                r.una_vez  = atoi(p[9].c_str());
                r.mientras = atoi(p[10].c_str());
                objects.back().reglas.push_back(r);
                continue;
            }
            if (!strncmp(line, "OBJAMB ", 7) && !objects.empty()) {
                std::string p[3]; trozos(line + 7, p, 3);
                objects.back().amb_radio  = (float)atof(p[0].c_str());
                objects.back().amb_vol    = atoi(p[1].c_str());
                objects.back().amb_sonido = p[2];
                continue;
            }
            if (!strncmp(line, "MUSICA ", 7)) {
                std::string p[4]; trozos(line + 7, p, 4);
                esc_mus_vol  = atoi(p[0].c_str());
                esc_mus_loop = atoi(p[1].c_str());
                esc_mus_fade = (float)atof(p[2].c_str());
                esc_musica   = p[3];
                continue;
            }
            if (!strncmp(line, "ZSONIDO ", 8)) {
                std::string p[3]; trozos(line + 8, p, 3);
                ZonaSonido z;
                z.zona = atoi(p[0].c_str());
                z.vol  = atoi(p[1].c_str());
                z.sonido = p[2];
                if (!z.sonido.empty()) zsonidos.push_back(z);
                continue;
            }
            if (!strncmp(line, "OBJRACC ", 8) && !objects.empty() && !objects.back().reglas.empty()) {
                std::string p[14]; trozos(line + 8, p, 14);
                Accion a;
                a.tipo    = atoi(p[0].c_str());
                a.archivo = p[1]; a.proc = p[2]; a.var = p[3];
                a.op      = atoi(p[4].c_str());
                a.valor   = (float)atof(p[5].c_str());
                a.seg     = (float)atof(p[6].c_str());
                a.texto   = p[7];
                a.sonido  = p[8];
                if (!p[9].empty()) a.vol = atoi(p[9].c_str());
                a.escena  = p[10]; a.menu = p[11]; a.dialogo = p[12];
                if (!p[13].empty()) a.ranura = atoi(p[13].c_str());
                objects.back().reglas.back().acciones.push_back(a);
                continue;
            }
            // Las variables del juego (puntos, vida...): van con la escena.
            /* Las escenas de antes guardaban aqui las variables del juego. Ahora
               son del proyecto, asi que se recogen pero sin repetir: la escena solo
               aporta las que falten. */
            if (!strncmp(line, "GAMEVAR ", 8)) {
                std::string p[2]; trozos(line + 8, p, 2);
                bool ya = false;
                for (auto& v : gvars) if (v.nombre == p[0]) ya = true;
                if (!p[0].empty() && !ya) { GameVar v; v.nombre = p[0]; v.valor = atoi(p[1].c_str()); gvars.push_back(v); }
                continue;
            }
        }
        fclose(f);
        rebuild_water();   // dibujar los lagos cargados (con el relieve ya puesto)
        /* Y arrancar la simulacion si la escena trae manantiales. Todas las demas
           llamadas a watersim_sync van tras "if (g3d_watersim_active())", y al
           cargar una escena todavia NO hay campo -- asi que los manantiales
           guardados no se creaban nunca y el agua no aparecia. */
        if (!wsources.empty()) watersim_sync(true);
        /* Las referencias guardadas pueden ser del tiempo en que todo estaba
           suelto ("hoja.png") y el fichero estar ya en Sprites/. Aqui se pasan a
           la ruta real, para que los desplegables la enseñen elegida y para que al
           guardar quede escrita la ruta ordenada. */
        for (auto& o : objects) o.asset = ruta_asset(o.asset);
        for (auto& sp : sprites) if (!sp.sheet.empty()) sp.sheet = ruta_asset(sp.sheet);
        for (auto& h : hud) {
            if (!h.asset.empty()) h.asset = ruta_asset(h.asset);
            if (!h.font.empty())  h.font  = ruta_asset(h.font);
        }
        if (!esc_musica.empty()) esc_musica = ruta_asset(esc_musica);
        for (auto& z : zsonidos) if (!z.sonido.empty()) z.sonido = ruta_asset(z.sonido);
        status = "Escena cargada (" + std::to_string(objects.size()) + " objetos)";
    };

    // ---- gestion de PROYECTO (.bgd2): abrir / crear / guardar ----
    // Un proyecto es una carpeta con <nombre>.bgd2 + Assets/ Scenes/ Scripts/.
    // El .bgd2 guarda el nombre y la escena principal (relativa a la carpeta).
    auto apply_project = [&](const std::string& dir, const std::string& pname) {
        project_dir = dir; project_name = pname;
        assets_dir  = dir + "/Assets";
        /* El sembrado carga sus modelos relativos a ESTE directorio. Sin volver a
           fijarlo, seguia buscandolos en el proyecto anterior: los assets con el
           mismo nombre en los dos colaban, y los que solo estaban en el nuevo no
           se cargaban -- sembrabas y no aparecia nada, sin un mensaje. */
        g3d_scatter_set_base(project_dir.c_str());
        /* La cache de "donde esta cada asset" es de ESTE proyecto: sin vaciarla,
           al abrir otro seguiria dando rutas del anterior. Es el mismo fallo que
           tuvo el sembrado con su directorio base. */
        asset_cache.clear();
        scenes_dir  = dir + "/Scenes";
        scripts_dir = dir + "/Scripts";
        tex_dir     = assets_dir;
        std::error_code ec;
        fs::create_directories(assets_dir, ec);
        fs::create_directories(scenes_dir, ec);
        fs::create_directories(scripts_dir, ec);
        /* Las carpetas de siempre. Tenerlas creadas es media clasificacion: al
           copiar algo, uno ve donde va sin preguntarselo. */
        for (const char* c : { "Models", "Textures", "Sprites", "Fonts", "Music", "Sounds" })
            fs::create_directories(assets_dir + "/" + c, ec);
        // re-escanea contenidos del proyecto
        assets = scan_assets(assets_dir);
        scan_sonoros(assets_dir, "Music",  { ".ogg", ".mp3", ".mod", ".xm", ".it", ".s3m",
                                             ".mid", ".midi", ".flac", ".wav" }, musicas);
        scan_sonoros(assets_dir, "Sounds", { ".wav", ".ogg", ".flac" }, sonidos);
        paints.clear();
        for (auto& fpng : scan_textures(tex_dir)) paints.push_back({ fpng, nullptr });
        paint_sel = paints.empty() ? -1 : 0;
        model_cache.clear(); posed_static.clear();
        // El HUD tambien es del proyecto: sin esto el panel seguia ofreciendo los
        // graficos y las fuentes del proyecto ANTERIOR, y el juego generado hacia
        // map_load de un fichero que no esta en ESTE Assets -> no se veia nada.
        for (auto& kv : hud_imgs)  h2_free_image(&kv.second);
        for (auto& kv : hud_fpgs)  h2_free_fpg(&kv.second);
        for (auto& kv : hud_fonts) h2_free_font(&kv.second);
        hud_imgs.clear(); hud_fpgs.clear(); hud_fonts.clear();
        hud_gfx_files = hud_scan_gfx();
        hud_font_files = hud_scan_fnt();
        hud.clear(); hud_sel = -1;
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
            /* La escena INICIAL es la del juego, no la que tengas abierta: con
               varias escenas, editar la cueva no puede cambiar por donde empieza. */
            std::string rel = escena_inicial;
            if (rel.empty()) rel = fs::path(scene_path).lexically_relative(project_dir).string();
            fprintf(f, "BGD2PROJECT 1\nname=%s\nscene=%s\n", project_name.c_str(), rel.c_str());
            /* Como se guardan las partidas (cuantas ranuras y que entra). */
            fprintf(f, "guardado=%d|%s|%d|%d|%d|%d\n", guardado.ranuras, guardado.fichero.c_str(),
                    guardado.con_vars ? 1 : 0, guardado.con_escena ? 1 : 0,
                    guardado.con_jugador ? 1 : 0, guardado.con_reglas ? 1 : 0);
            /* Las variables del juego son del PROYECTO y no de una escena: los
               puntos y la vida tienen que sobrevivir al cambiar de mapa. El
               tercer numero dice si esa variable entra en la partida guardada. */
            for (auto& v : gvars)
                fprintf(f, "var=%s|%d|%d\n", v.nombre.c_str(), v.valor, v.guardar ? 1 : 0);
            fclose(f);
        }
    };
    /* Los menus se guardan aparte del manifiesto: son una lista con opciones y
       acciones dentro, y el .bgd2 es de clave=valor. Un fichero por proyecto. */
    auto guardar_menus = [&]() {
        std::string ruta = project_dir + "/menus.def";
        if (menus.empty()) { std::error_code ec; fs::remove(ruta, ec); return; }
        FILE* f = fopen(ruta.c_str(), "w");
        if (!f) return;
        fputs("# menus del proyecto (los hace el editor)\n", f);
        for (auto& m : menus) {
            fprintf(f, "MENU %d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s|%s|%s|%s\n",
                    m.cuando, m.x, m.y, m.sep, m.pausa,
                    m.con_teclado, m.con_mando, m.con_raton,
                    m.nombre.c_str(), m.tecla.c_str(), m.boton.c_str(),
                    m.fuente.c_str(), m.fondo.c_str(),
                    m.snd_mover.c_str(), m.snd_elegir.c_str());
            fprintf(f, "MCOLOR %d %d %d %d %d %d %d %d\n",
                    m.col[0], m.col[1], m.col[2], m.col[3],
                    m.col_sel[0], m.col_sel[1], m.col_sel[2], m.col_sel[3]);
            for (auto& o : m.opciones) {
                fprintf(f, "MOPC %s\n", o.texto.c_str());
                if (o.clase == 1)
                    fprintf(f, "MAJU %d|%s|%d|%d|%d\n", o.ajuste, o.var.c_str(),
                            o.vmin, o.vmax, o.paso);
                if (o.clase == 2)
                    fprintf(f, "MRAN %d|%d\n", o.ranura, o.ranura_modo);
                for (auto& a : o.acciones)
                    fprintf(f, "MACC %d|%s|%s|%s|%d|%.3f|%.2f|%s|%s|%d|%s|%s|%s|%d\n",
                            a.tipo, a.archivo.c_str(), a.proc.c_str(), a.var.c_str(),
                            a.op, a.valor, a.seg, a.texto.c_str(),
                            a.sonido.c_str(), a.vol, a.escena.c_str(), a.menu.c_str(), a.dialogo.c_str(), a.ranura);
            }
        }
        fclose(f);
    };
    auto cargar_menus = [&]() {
        menus.clear(); menu_sel = -1;
        FILE* f = fopen((project_dir + "/menus.def").c_str(), "r");
        if (!f) return;
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            if (!strncmp(line, "MENU ", 5)) {
                std::string p[15]; trozos(line + 5, p, 15);
                Menu m;
                m.cuando = atoi(p[0].c_str());
                m.x = atoi(p[1].c_str()); m.y = atoi(p[2].c_str()); m.sep = atoi(p[3].c_str());
                m.pausa = atoi(p[4].c_str());
                m.con_teclado = atoi(p[5].c_str()); m.con_mando = atoi(p[6].c_str()); m.con_raton = atoi(p[7].c_str());
                m.nombre = p[8]; m.tecla = p[9]; m.boton = p[10];
                m.fuente = p[11]; m.fondo = p[12];
                m.snd_mover = p[13]; m.snd_elegir = p[14];
                menus.push_back(m);
            } else if (!strncmp(line, "MCOLOR ", 7) && !menus.empty()) {
                Menu& m = menus.back();
                sscanf(line, "MCOLOR %d %d %d %d %d %d %d %d",
                       &m.col[0], &m.col[1], &m.col[2], &m.col[3],
                       &m.col_sel[0], &m.col_sel[1], &m.col_sel[2], &m.col_sel[3]);
            } else if (!strncmp(line, "MOPC ", 5) && !menus.empty()) {
                MenuOpc o;
                std::string t(line + 5);
                while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
                o.texto = t;
                menus.back().opciones.push_back(o);
            } else if (!strncmp(line, "MAJU ", 5) && !menus.empty() && !menus.back().opciones.empty()) {
                std::string p2[5]; trozos(line + 5, p2, 5);
                MenuOpc& o = menus.back().opciones.back();
                o.clase = 1;
                o.ajuste = atoi(p2[0].c_str());
                o.var = p2[1];
                o.vmin = atoi(p2[2].c_str()); o.vmax = atoi(p2[3].c_str()); o.paso = atoi(p2[4].c_str());
            } else if (!strncmp(line, "MRAN ", 5) && !menus.empty() && !menus.back().opciones.empty()) {
                std::string p2[2]; trozos(line + 5, p2, 2);
                MenuOpc& o = menus.back().opciones.back();
                o.clase = 2; o.ranura = atoi(p2[0].c_str()); o.ranura_modo = atoi(p2[1].c_str());
            } else if (!strncmp(line, "MACC ", 5) && !menus.empty() && !menus.back().opciones.empty()) {
                std::string p[14]; trozos(line + 5, p, 14);
                Accion a;
                a.tipo = atoi(p[0].c_str());
                a.archivo = p[1]; a.proc = p[2]; a.var = p[3];
                a.op = atoi(p[4].c_str());
                a.valor = (float)atof(p[5].c_str());
                a.seg = (float)atof(p[6].c_str());
                a.texto = p[7]; a.sonido = p[8];
                if (!p[9].empty()) a.vol = atoi(p[9].c_str());
                a.escena = p[10]; a.menu = p[11]; a.dialogo = p[12];
                if (!p[13].empty()) a.ranura = atoi(p[13].c_str());
                menus.back().opciones.back().acciones.push_back(a);
            }
        }
        fclose(f);
        for (auto& m : menus) {
            if (!m.fuente.empty()) m.fuente = ruta_asset(m.fuente);
            if (!m.fondo.empty())  m.fondo  = ruta_asset(m.fondo);
            if (!m.snd_mover.empty())  m.snd_mover  = ruta_asset(m.snd_mover);
            if (!m.snd_elegir.empty()) m.snd_elegir = ruta_asset(m.snd_elegir);
        }
        if (!menus.empty()) menu_sel = 0;
    };

    /* Los dialogos, como los menus, en su propio fichero del proyecto. */
    auto guardar_dialogos = [&]() {
        std::string ruta = project_dir + "/dialogos.def";
        if (dialogos.empty()) { std::error_code ec; fs::remove(ruta, ec); return; }
        FILE* f = fopen(ruta.c_str(), "w");
        if (!f) return;
        fputs("# dialogos del proyecto (los hace el editor)\n", f);
        for (auto& d : dialogos) {
            fprintf(f, "DLG %s|%s|%d|%d|%d|%d|%d|%s|%d|%d|%d|%s|%s|%s|%s|%d\n",
                    d.nombre.c_str(), d.caja.c_str(), d.caja_graf,
                    d.cx, d.cy, d.cw, d.ch, d.fuente.c_str(),
                    d.mx, d.my, d.vel, d.snd_letra.c_str(), d.snd_pasar.c_str(),
                    d.tecla.c_str(), d.boton.c_str(), d.pausa);
            fprintf(f, "DLGCOL %d %d %d %d %d %d %d %d\n",
                    d.col[0], d.col[1], d.col[2], d.col[3],
                    d.col_nombre[0], d.col_nombre[1], d.col_nombre[2], d.col_nombre[3]);
            for (auto& p2 : d.paginas) {
                fprintf(f, "DLGPAG %s|%s|%d|%d\n", p2.quien.c_str(), p2.retrato.c_str(),
                        p2.retrato_graf, p2.retrato_cara);
                fprintf(f, "DLGTXT %s\n", p2.texto.c_str());
                for (auto& o : p2.opciones) {
                    fprintf(f, "DLGOPC %d|%s\n", o.salto, o.texto.c_str());
                    for (auto& a : o.acciones)
                        fprintf(f, "DLGACC %d|%s|%s|%s|%d|%.3f|%.2f|%s|%s|%d|%s|%s|%s|%d\n",
                                a.tipo, a.archivo.c_str(), a.proc.c_str(), a.var.c_str(),
                                a.op, a.valor, a.seg, a.texto.c_str(),
                                a.sonido.c_str(), a.vol, a.escena.c_str(), a.menu.c_str(), a.dialogo.c_str(), a.ranura);
                }
            }
        }
        fclose(f);
    };
    auto cargar_dialogos = [&]() {
        dialogos.clear(); dlg_sel = -1;
        FILE* f = fopen((project_dir + "/dialogos.def").c_str(), "r");
        if (!f) return;
        char line[2048];
        while (fgets(line, sizeof(line), f)) {
            auto sinsalto = [](std::string t) {
                while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
                return t;
            };
            if (!strncmp(line, "DLG ", 4)) {
                std::string p2[16]; trozos(line + 4, p2, 16);
                Dialogo d;
                d.nombre = p2[0]; d.caja = p2[1]; d.caja_graf = atoi(p2[2].c_str());
                d.cx = atoi(p2[3].c_str()); d.cy = atoi(p2[4].c_str());
                d.cw = atoi(p2[5].c_str()); d.ch = atoi(p2[6].c_str());
                d.fuente = p2[7];
                d.mx = atoi(p2[8].c_str()); d.my = atoi(p2[9].c_str());
                d.vel = atoi(p2[10].c_str());
                d.snd_letra = p2[11]; d.snd_pasar = p2[12];
                if (!p2[13].empty()) d.tecla = p2[13];
                if (!p2[14].empty()) d.boton = p2[14];
                d.pausa = atoi(p2[15].c_str());
                dialogos.push_back(d);
            } else if (!strncmp(line, "DLGCOL ", 7) && !dialogos.empty()) {
                Dialogo& d = dialogos.back();
                sscanf(line, "DLGCOL %d %d %d %d %d %d %d %d",
                       &d.col[0], &d.col[1], &d.col[2], &d.col[3],
                       &d.col_nombre[0], &d.col_nombre[1], &d.col_nombre[2], &d.col_nombre[3]);
            } else if (!strncmp(line, "DLGPAG ", 7) && !dialogos.empty()) {
                std::string p2[4]; trozos(line + 7, p2, 4);
                DlgPag pg; pg.quien = p2[0]; pg.retrato = p2[1];
                pg.retrato_graf = p2[2].empty() ? 1 : atoi(p2[2].c_str());
                pg.retrato_cara = p2[3].empty() ? -1 : atoi(p2[3].c_str());
                dialogos.back().paginas.push_back(pg);
            } else if (!strncmp(line, "DLGTXT ", 7) && !dialogos.empty() && !dialogos.back().paginas.empty()) {
                dialogos.back().paginas.back().texto = sinsalto(line + 7);
            } else if (!strncmp(line, "DLGOPC ", 7) && !dialogos.empty() && !dialogos.back().paginas.empty()) {
                std::string p2[2]; trozos(line + 7, p2, 2);
                DlgOpc o; o.salto = atoi(p2[0].c_str()); o.texto = p2[1];
                dialogos.back().paginas.back().opciones.push_back(o);
            } else if (!strncmp(line, "DLGACC ", 7) && !dialogos.empty() &&
                       !dialogos.back().paginas.empty() && !dialogos.back().paginas.back().opciones.empty()) {
                std::string p2[14]; trozos(line + 7, p2, 14);
                Accion a;
                a.tipo = atoi(p2[0].c_str());
                a.archivo = p2[1]; a.proc = p2[2]; a.var = p2[3];
                a.op = atoi(p2[4].c_str());
                a.valor = (float)atof(p2[5].c_str());
                a.seg = (float)atof(p2[6].c_str());
                a.texto = p2[7]; a.sonido = p2[8];
                if (!p2[9].empty()) a.vol = atoi(p2[9].c_str());
                a.escena = p2[10]; a.menu = p2[11]; a.dialogo = p2[12];
                if (!p2[13].empty()) a.ranura = atoi(p2[13].c_str());
                dialogos.back().paginas.back().opciones.back().acciones.push_back(a);
            }
        }
        fclose(f);
        for (auto& d : dialogos) {
            if (!d.caja.empty())   d.caja   = ruta_asset(d.caja);
            if (!d.fuente.empty()) d.fuente = ruta_asset(d.fuente);
            if (!d.snd_letra.empty()) d.snd_letra = ruta_asset(d.snd_letra);
            if (!d.snd_pasar.empty()) d.snd_pasar = ruta_asset(d.snd_pasar);
            for (auto& pg : d.paginas) if (!pg.retrato.empty()) pg.retrato = ruta_asset(pg.retrato);
        }
        if (!dialogos.empty()) dlg_sel = 0;
    };

    /* ---- ORDENAR LOS ASSETS ----
       Mueve lo que este suelto en Assets/ a su carpeta. No hay que tocar ninguna
       referencia: las escenas guardan el nombre del fichero y ruta_asset() lo
       encuentra igual dentro de la subcarpeta. Las imagenes que se usan como hoja
       de sprites van a Sprites y el resto a Textures, que el editor SI sabe para
       que se usa cada una. */
    auto ordenar_assets = [&]() {
        std::error_code ec;
        for (const char* c : { "Models", "Textures", "Sprites", "Fonts", "Music", "Sounds" })
            fs::create_directories(assets_dir + "/" + c, ec);
        // las imagenes que ya se usan como hoja de sprites
        std::set<std::string> hojas;
        for (auto& sp : sprites) if (!sp.sheet.empty())
            hojas.insert(fs::path(sp.sheet).filename().string());
        auto carpeta_de = [&](const std::string& fichero) -> const char* {
            std::string e = fs::path(fichero).extension().string();
            for (auto& c : e) c = (char)tolower(c);
            if (e==".glb"||e==".gltf"||e==".fbx"||e==".obj"||e==".md3")   return "Models";
            if (e==".fnt"||e==".fnx")                                     return "Fonts";
            if (e==".mp3"||e==".mod"||e==".xm"||e==".it"||e==".s3m"||
                e==".mid"||e==".midi"||e==".flac")                        return "Music";
            if (e==".wav"||e==".ogg")                                     return "Sounds";
            if (e==".png"||e==".jpg"||e==".jpeg"||e==".bmp"||e==".tga"||
                e==".fpg"||e==".f16"||e==".f32")
                return hojas.count(fs::path(fichero).filename().string()) ? "Sprites" : "Textures";
            /* El .sheet son los fotogramas y las animaciones que detectaste de una
               hoja, y el editor los busca AL LADO de su imagen: si la imagen se va
               a Sprites y el .sheet se queda suelto, pierdes ese trabajo. */
            if (e==".sheet")                                              return "Sprites";
            return nullptr;   // lo que no se reconoce, se queda donde esta
        };
        int movidos = 0, fallos = 0;
        // 1) lo suelto dentro de Assets/
        std::vector<fs::path> sueltos;
        for (auto& e : fs::directory_iterator(assets_dir, ec)) {
            std::error_code e1;
            if (e.is_regular_file(e1)) sueltos.push_back(e.path());
        }
        for (auto& f2 : sueltos) {
            const char* dest = carpeta_de(f2.filename().string());
            if (!dest) continue;
            fs::path destino = fs::path(assets_dir) / dest / f2.filename();
            std::error_code e2;
            if (fs::exists(destino, e2)) { fallos++; continue; }   // ya hay uno igual: no se pisa
            fs::rename(f2, destino, e2);
            if (e2) fallos++; else movidos++;
        }
        // 2) las carpetas sueltas del proyecto que son de assets (un Fonts/ fuera
        //    de Assets no lo veia el editor: por eso no salian tus fuentes)
        for (const char* c : { "Fonts", "Models", "Textures", "Sprites", "Sounds", "Music" }) {
            std::string fuera = project_dir + "/" + c;
            std::error_code e1;
            if (!fs::is_directory(fuera, e1)) continue;
            for (auto& e : fs::directory_iterator(fuera, ec)) {
                std::error_code e2;
                if (!e.is_regular_file(e2)) continue;
                fs::path destino = fs::path(assets_dir) / c / e.path().filename();
                if (fs::exists(destino, e2)) { fallos++; continue; }
                fs::rename(e.path(), destino, e2);
                if (e2) fallos++; else movidos++;
            }
            fs::remove(fuera, ec);   // si queda vacia, fuera
        }
        asset_cache.clear();
        assets = scan_assets(assets_dir);
        hud_gfx_files  = hud_scan_gfx();
        hud_font_files = hud_scan_fnt();
        scan_sonoros(assets_dir, "Music",  { ".ogg", ".mp3", ".mod", ".xm", ".it", ".s3m",
                                             ".mid", ".midi", ".flac", ".wav" }, musicas);
        scan_sonoros(assets_dir, "Sounds", { ".wav", ".ogg", ".mp3", ".flac" }, sonidos);
        console_add("[assets] ordenados: " + std::to_string(movidos) + " ficheros movidos" +
                    (fallos ? (", " + std::to_string(fallos) + " sin mover (ya habia uno igual)") : "") +
                    "\nLas escenas siguen funcionando: el editor busca cada asset por su nombre.\n");
        status = "Assets ordenados (" + std::to_string(movidos) + " movidos)";
    };

    // GUARDAR PROYECTO: escena actual + manifiesto .bgd2
    auto save_project = [&]() {
        save_scene(scene_path);                 // objetos + terreno + pintado
        write_manifest();
        guardar_menus();
        guardar_dialogos();
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
            gvars.clear();
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                char buf[512];
                if (sscanf(line, "scene=%511[^\n]", buf) == 1) scn = buf;
                else if (!strncmp(line, "var=", 4)) {
                    std::string p2[3]; trozos(line + 4, p2, 3);
                    if (!p2[0].empty()) {
                        GameVar v; v.nombre = p2[0]; v.valor = atoi(p2[1].c_str());
                        v.guardar = p2[2].empty() ? true : (atoi(p2[2].c_str()) != 0);
                        gvars.push_back(v);
                    }
                }
                else if (!strncmp(line, "guardado=", 9)) {
                    std::string p2[6]; trozos(line + 9, p2, 6);
                    guardado.ranuras = atoi(p2[0].c_str());
                    if (guardado.ranuras < 1) guardado.ranuras = 1;
                    guardado.fichero = p2[1].empty() ? "partida" : p2[1];
                    guardado.con_vars    = atoi(p2[2].c_str()) != 0;
                    guardado.con_escena  = atoi(p2[3].c_str()) != 0;
                    guardado.con_jugador = atoi(p2[4].c_str()) != 0;
                    guardado.con_reglas  = atoi(p2[5].c_str()) != 0;
                }
            }
            fclose(f);
        }
        if (!scn.empty()) {
            escena_inicial = scn;          // por esta escena empieza el juego
            std::string sp = (fs::path(project_dir) / scn).string();
            if (fs::exists(sp)) { scene_path = sp; load_scene(sp); }
        }
        cargar_menus();
        cargar_dialogos();
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
            "import \"libmod_gfx\"; import \"libmod_misc\"; import \"libmod_input\";\n"
            "import \"libmod_sound\"; import \"libmod_3d\";\n\n")
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
            /* Los import viven en TU mitad del fichero, la que el editor no toca.
               Pero si la escena tiene sonido y ahi no esta libmod_sound, el juego
               no compila; asi que esa linea si se anade, avisando. Es lo unico que
               se toca de tu parte, y solo cuando hace falta. */
            if (cur.find("libmod_sound") == std::string::npos) {
                size_t imp = cur.find("import \"libmod_3d\"");
                if (imp != std::string::npos) {
                    cur.insert(imp, "import \"libmod_sound\"; ");
                    a = cur.find(MK_BEGIN); z = cur.find(MK_END);
                    console_add("Anadido import \"libmod_sound\" a main.prg (lo pide el sonido).\n");
                }
            }
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
        fclose(mf);
        script_obj.clear();
        int i = code_abrir(mp, "main.prg", "");
        code_recargar(i);      // el generador acaba de reescribirlo
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

        /* ---- lo que es del JUEGO y lo que es de UNA escena ----
           Con varias escenas en el mismo main.prg, las GLOBAL no pueden salir una
           vez por escena (estarian declaradas dos veces) y los procesos comunes
           tampoco. Asi que las declaraciones se recogen aparte, sin repetir, y se
           escriben todas juntas al principio; el cuerpo de cada escena va detras.
           Las tablas que dependen del tamanio de la escena (las reglas, los
           canales de sonido) se quedan con el maximo de todas. */
        std::vector<std::string> glob_lin;
        std::set<std::string>    glob_set;
        auto add_global = [&](const std::string& bloque) {
            size_t i = 0;
            while (i < bloque.size()) {
                size_t nl = bloque.find('\n', i);
                std::string l = bloque.substr(i, (nl == std::string::npos ? bloque.size() : nl) - i);
                if (!l.empty() && l != "GLOBAL" && l != "END" && glob_set.insert(l).second)
                    glob_lin.push_back(l);
                if (nl == std::string::npos) break;
                i = nl + 1;
            }
        };
        int max_reglas = 0, max_amb = 0, max_zamb = 0;
        /* Procesos que son del JUEGO y no de una escena (el cartelito de las
           reglas, el contador de FPS, el motor de fisica): se escriben una sola
           vez aunque haya diez escenas. */
        std::vector<std::string> proc_comun;
        std::set<std::string>    proc_comun_set;
        /* Los #include tambien son del juego: dos escenas pueden llamar al mismo
           .prg tuyo, y incluirlo dos veces es "Process/function already defined". */
        std::vector<std::string> includes;
        std::set<std::string>    includes_set;
        auto add_include = [&](const std::string& rel) {
            if (includes_set.insert(rel).second)
                includes.push_back("#include \"" + rel + "\"\n");
        };
        auto add_proc_comun = [&](const std::string& nombre, const std::string& texto) {
            if (proc_comun_set.insert(nombre).second) proc_comun.push_back(texto);
        };
        /* Siempre declaradas, aunque el juego no tenga guardado: el proceso del
           jugador las mira al nacer y si no existen no compila. */
        add_global("    // de donde viene el jugador al cargar una partida (0 = empieza donde toca)\n"
                   "    float volver_x; float volver_y; float volver_z; float volver_a;\n"
                   "    int hay_vuelta;\n");
        add_global("int scene; int camera; int light;\n"
                   "    float escena_pitch;   // hacia donde mira en vertical (FPS)\n"
                   "    float escena_yaw;     // hacia donde mira en horizontal (FPS)\n");
        // ---- localizar el jugador y los objetos enganchados a su esqueleto ----

        /* ---- el cuerpo de UNA escena ----
           Todo lo de aqui dentro se genera una vez por escena, y sus procesos
           llevan delante el nombre de la escena (pref) para no chocar entre
           ellos: dos escenas tienen cada una su sol, su camara y su montaje. */
        std::string pref = "escena";
        /* Lo que lanza cada escena, para poder matarlo al cambiar a otra. Se apunta
           segun se genera: es la unica forma de que la lista no se quede coja
           cuando manana se anada otro proceso al montaje. */
        std::vector<std::string> lanzados;
        bool hay_aviso_juego = false;   // alguna escena ensenia textos
        auto generar_escena = [&]() {
            lanzados.clear();
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
            // 5 = muro y 6 = malla exacta no llevan script (son colisionadores
            // estaticos, no hay nada que mover); 7 = envolvente si, que es un cuerpo.
            bool necesita = o.is_player || (o.phys >= 0 && o.phys <= 4) || o.phys == 7
                            || !o.reglas.empty();
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

        /* ---- lo que comparten personajes y objetos ----
           Va antes que nada: en BennuGD2 un GLOBAL tiene que estar declarado por
           encima del codigo que lo usa, y de aqui para abajo lo usan los NPC (para
           saber si te has acercado) y los objetos con acciones. Y va fuera del
           "si hay sprites": un objeto 3D con una accion tambien lo necesita. */
        /* ---- todos los sonidos que hace falta cargar ----
           Cada fichero se carga UNA vez en su GLOBAL, aunque lo usen diez reglas.
           El orden sale de un set: asi la lista es siempre la misma. */
        std::set<std::string> sonidos_usados;
        for (auto& o : objects) {
            if (!o.amb_sonido.empty()) sonidos_usados.insert(o.amb_sonido);
            for (auto& r : o.reglas) for (auto& a : r.acciones)
                if (a.tipo == 4 && !a.sonido.empty()) sonidos_usados.insert(a.sonido);
        }
        for (auto& sp : sprites)
            for (auto& r : sp.reglas) for (auto& a : r.acciones)
                if (a.tipo == 4 && !a.sonido.empty()) sonidos_usados.insert(a.sonido);
        for (auto& z : zsonidos) if (!z.sonido.empty()) sonidos_usados.insert(z.sonido);
        for (auto& mm : menus) {
            if (!mm.snd_mover.empty())  sonidos_usados.insert(mm.snd_mover);
            if (!mm.snd_elegir.empty()) sonidos_usados.insert(mm.snd_elegir);
            for (auto& o : mm.opciones) for (auto& a : o.acciones)
                if (a.tipo == 4 && !a.sonido.empty()) sonidos_usados.insert(a.sonido);
        }
        int n_amb = total_amb();
        /* El juego no puede abrir rutas con letras fuera del ASCII (medido: una
           tilde o un emoji en el nombre y music_load/sound_load devuelven 0). Aqui
           ya no hay UI donde ponerlo en rojo, asi que se avisa por la consola. */
        {
            std::vector<std::string> malos;
            if (!esc_musica.empty() && !nombre_ascii(esc_musica)) malos.push_back(esc_musica);
            for (auto& sn : sonidos_usados) if (!nombre_ascii(sn)) malos.push_back(sn);
            for (auto& m : malos)
                console_add("AVISO: '" + m + "' no se podra abrir en el juego: el nombre lleva\n"
                            "letras fuera del ASCII (emojis, tildes, enies). Renombralo desde\n"
                            "el panel de Sonido (boton 'Renombrar a...').\n");
        }

        int nreglas = total_reglas();
        bool hay_aviso = false;   // alguna regla ensenia un texto en pantalla
        for (auto& o : objects) for (auto& r : o.reglas) for (auto& a : r.acciones) if (a.tipo == 3) hay_aviso = true;
        for (auto& sp : sprites) for (auto& r : sp.reglas) for (auto& a : r.acciones) if (a.tipo == 3) hay_aviso = true;
        if (hay_aviso) hay_aviso_juego = true;
        // los menus tambien pueden ensenar textos
        for (auto& mm : menus) for (auto& o : mm.opciones) for (auto& a : o.acciones)
            if (a.tipo == 3) hay_aviso_juego = true;
        {
            bool hay_jug = false;
            for (auto& sp : sprites) if (sp.is_player) hay_jug = true;
            for (auto& o : objects)  if (o.is_player)  hay_jug = true;
            // Las reglas que miran donde esta el jugador tambien lo necesitan.
            for (auto& o : objects) for (auto& r : o.reglas)
                if (r.evento == 2 || r.evento == 3 || r.evento == 4) hay_jug = true;
            for (auto& sp : sprites) for (auto& r : sp.reglas)
                if (r.evento == 2 || r.evento == 3 || r.evento == 4) hay_jug = true;
            /* escena_dt se declara AQUI y no mas abajo con los demas: las reglas y
               el cartelito la usan, y en BennuGD2 un GLOBAL tiene que estar por
               encima del codigo que lo lee -- incluidos los #include. */
            std::string g = "    float escena_dt;         // lo que dura un frame\n";
            /* El angulo de la camara en tercera persona, cuando la gira el jugador.
               De aqui salen el brazo de la camara Y los controles, que si no la D
               dejaria de llevar a la derecha de la pantalla al girar. En milesimas
               de grado, como todos los angulos de BennuGD2. */
            if (cam_girable && cam_mode == 1)
                g += "    float escena_orbita;     // hacia donde mira la camara (la gira el jugador)\n";
            if (hay_jug)
                g += "    float jug_x; float jug_y; float jug_z;   // donde esta el jugador\n";
            // ---- las variables del juego ----
            for (auto& v : gvars)
                g += "    int " + v.nombre + " = " + std::to_string(v.valor) + ";\n";
            /* El estado de las reglas (regla_hecha / regla_ant / regla_t) no se
               declara aqui: los huecos se numeran DENTRO de cada escena, asi que
               las tablas se quedan con el maximo de todas y se declaran al final.
               Se ponen a cero al montar una escena. */
            if (nreglas > max_reglas) max_reglas = nreglas;
            if (hay_aviso)
                g += "    string aviso_txt; float aviso_t;   // el texto que ensenian las reglas\n";
            // ---- sonido ----
            if (!esc_musica.empty()) g += "    int musica;              // la musica de la escena\n";
            for (auto& sn : sonidos_usados)
                g += "    int " + var_sonido(sn) + ";" + std::string(14 > (int)var_sonido(sn).size() ? 14 - (int)var_sonido(sn).size() : 1, ' ')
                   + "// " + sn + "\n";
            if (n_amb > max_amb) max_amb = n_amb;
            if ((int)zsonidos.size() > max_zamb) max_zamb = (int)zsonidos.size();
            add_global(g);
        }
        /* El ambiente de las zonas pintadas: suena mientras el jugador esta dentro.
           Un canal por zona, que se abre al entrar y se cierra al salir. */
        if (!zsonidos.empty()) {
            fprintf(f, "// Ambiente de las zonas pintadas (lluvia, bosque, cueva).\n"
                  "PROCESS %s_ambiente()\n"
                  "BEGIN\n"
                  "    LOOP\n", pref.c_str());
            for (int i = 0; i < (int)zsonidos.size(); i++) {
                if (zsonidos[i].sonido.empty()) continue;
                fprintf(f,
                  "        IF (g3d_zone_blocked(jug_x, jug_z, %d))\n"
                  "            IF (zamb_ch[%d] < 0)\n"
                  "                zamb_ch[%d] = sound_play(%s, -1);\n"
                  "                channel_set_volume(zamb_ch[%d], %d);\n"
                  "            END\n"
                  "        ELSE\n"
                  "            IF (zamb_ch[%d] >= 0)  sound_stop(zamb_ch[%d]);  zamb_ch[%d] = -1;  END\n"
                  "        END\n",
                  zsonidos[i].zona, i, i, var_sonido(zsonidos[i].sonido).c_str(),
                  i, zsonidos[i].vol, i, i, i);
            }
            fputs("        FRAME;\n    END\nEND\n\n", f);
        }

        /* El cartelito de las reglas: un solo write_var pintando una GLOBAL, que
           se vacia cuando se acaba su tiempo. Sin crear y borrar writes. */
        if (hay_aviso)
            add_proc_comun("aviso",
                  "// Ensenia el texto de las reglas mientras dure su tiempo.\n"
                  "PROCESS escena_aviso()\n"
                  "BEGIN\n"
                  "    write_var(0, 640, 40, 4, aviso_txt);   // fuente del sistema, centrado arriba\n"
                  "    LOOP\n"
                  "        IF (aviso_t > 0.0)\n"
                  "            aviso_t = aviso_t - escena_dt;\n"
                  "            IF (aviso_t <= 0.0) aviso_txt = \"\"; END\n"
                  "        END\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n");

        // Las acciones pueden llamar a codigo TUYO. Si ese proceso no existe,
        // el juego no compilaria ("Undefined procedure"), asi que se crea el
        // esqueleto en Scripts/ y se incluye; luego lo rellenas tu.
        {
            std::set<std::string> ya;
            // Las llamadas pueden venir de una accion del jugador o de la
            // interaccion de un NPC: se juntan las dos listas. Cada una lleva
            // el fichero donde vive su codigo, si se eligio uno en el Inspector.
            struct Llamada { std::string llama, nombre, archivo; };
            std::vector<Llamada> llamadas;
            for (auto& sp : sprites) {
                for (auto& ac : sp.acciones)
                    if (!ac.llama.empty()) llamadas.push_back({ ac.llama, ac.nombre, ac.archivo });
                if (sp.inter_on && !sp.inter_llama.empty())
                    llamadas.push_back({ sp.inter_llama, "hablar con " + sp.name, sp.inter_arch });
            }
            // y las de las reglas, de objetos y de personajes
            for (auto& o : objects)
                for (auto& r : o.reglas)
                    for (auto& a : r.acciones)
                        if (a.tipo == 0 && !a.proc.empty())
                            llamadas.push_back({ a.proc, o.name, a.archivo });
            for (auto& sp : sprites)
                for (auto& r : sp.reglas)
                    for (auto& a : r.acciones)
                        if (a.tipo == 0 && !a.proc.empty())
                            llamadas.push_back({ a.proc, sp.name, a.archivo });
            // y las de las opciones de los menus
            for (auto& mm : menus)
                for (auto& o : mm.opciones)
                    for (auto& a : o.acciones)
                        if (a.tipo == 0 && !a.proc.empty())
                            llamadas.push_back({ a.proc, mm.nombre, a.archivo });
            for (auto& ac : llamadas) {
                /* Con fichero elegido solo hay que incluirlo: el codigo es
                   tuyo y puede tener dentro los procesos que quiera. Sin el,
                   se sigue creando el esqueleto con el nombre de la llamada,
                   o el juego no compilaria ("Undefined procedure"). */
                if (!ac.archivo.empty()) {
                    FILE* t = fopen((scripts_dir + "/" + ac.archivo).c_str(), "r");
                    if (t) {
                        fclose(t);
                        if (ya.insert(ac.archivo).second)
                            add_include("Scripts/" + ac.archivo);
                        continue;
                    }
                    console_add("AVISO: no encuentro Scripts/" + ac.archivo + " (accion '" + ac.nombre + "')\n");
                }
                {
                    std::string fn = ident_bgd(ac.llama, "f");
                    if (!ya.insert(fn + ".prg").second) continue;
                    std::string ruta = scripts_dir + "/" + fn + ".prg";
                    FILE* t = fopen(ruta.c_str(), "r");
                    if (t) fclose(t);
                    else {
                        FILE* n2 = fopen(ruta.c_str(), "w");
                        if (n2) {
                            fprintf(n2,
                                "// Accion '%s' del personaje: la llama el editor cuando pulsas\n"
                                "// la tecla o el boton que le has puesto. Es un PROCESS normal de\n"
                                "// BennuGD2, asi que puede durar varios FRAME (un disparo, un\n"
                                "// golpe con su tiempo...) o acabar enseguida con RETURN.\n"
                                "PROCESS %s()\n"
                                "BEGIN\n"
                                "    // ---- tu codigo aqui ----\n"
                                "    RETURN;\n"
                                "END\n",
                                ac.nombre.c_str(), fn.c_str());
                            fclose(n2);
                            console_add("Creado Scripts/" + fn + ".prg (accion '" + ac.nombre + "')\n");
                        }
                    }
                    add_include("Scripts/" + fn + ".prg");
                }
            }
            if (!ya.empty()) fputs("\n", f);
        }

        // Un #include por objeto (SIN duplicar: el codigo vive en Scripts/<n>.prg).
        // Van antes que escena_iniciar, que es quien instancia esos procesos.
        for (auto& o : objects) {
            std::string sp = scripts_dir + "/" + o.name + ".prg";
            FILE* s = fopen(sp.c_str(), "r");
            if (!s) continue;
            fclose(s);
            add_include("Scripts/" + o.name + ".prg");
        }
        fputs("\n", f);
        // main
        // Lo que comparten el montaje y el bucle tiene que ser GLOBAL: antes era
        // todo PRIVATE de un unico PROCESS main, y al partirlo en dos deja de valer.
        add_global("    int follow_ent;          // entidad a la que sigue la camara\n"
                   "    int pplayer; int pmodel; // entidad y modelo del jugador\n"
                   "    int atc[32]; int atn[32];// enganches a huesos: entidad y nodo\n");
        // ---- la luz del sol como proceso BennuGD2 (variables nativas) ----
        // Igual que cualquier objeto 3D: fija ctype/csubtype y sus datos en las
        // variables nativas, y el motor las envia a la luz cada FRAME. La direccion
        // sale de target - origen. Para cambiar la luz en marcha basta con tocar
        // intensity o color_r/g/b desde aqui.
        if (!sun_cycle) {
            /* Sol fijo: el proceso solo publica sus locales una vez y el hook
               los envia a la luz cada FRAME. */
            float az = sun_azim * 3.14159265f / 180.0f;
            float el = sun_elev * 3.14159265f / 180.0f;
            fprintf(f,
                  "PROCESS %s_sol()\n"
                  "BEGIN\n"
                  "    ctype = C_3D; csubtype = C3D_LIGHT;\n"
                  "    x = 0.0; y = 0.0; z = 0.0;\n"
                  "    target_x = %.4f; target_y = %.4f; target_z = %.4f;   // direccion = target - origen\n"
                  "    intensity = %.3f;\n"
                  "    color_r = 255; color_g = 245; color_b = 219;\n"
                  "    entity = g3d_light_create(0);   // sin color: lo ponen color_r/g/b\n"
                  "    g3d_light_enable_shadow(entity, 1); g3d_set_shadows(1);\n"
                  "    g3d_set_shadow_resolution(%d);\n"
                  "    LOOP\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n",
                  pref.c_str(),
                  -cosf(az) * cosf(el), -sinf(el), -sinf(az) * cosf(el),
                  sun_intensity, shadow_res);
        } else {
            /* CICLO DIA/NOCHE, entero dentro del proceso. Nada de una funcion de
               C que lo haga por detras: mover el sol es tocar sus locales, que es
               justo lo que el hook envia cada FRAME. Asi el ciclo se puede parar,
               acelerar o saltar a una hora desde el juego sin tocar el motor.
               La trigonometria de BennuGD2 va en MILESIMAS de grado (comprobado:
               sin(90000) = 1), de ahi los angulos x1000. */
            fprintf(f,
                  "PROCESS %s_sol()\n"
                  "PRIVATE\n"
                  "    float hora;    // 0 = amanecer, 90000 = mediodia, 180000 = ocaso\n"
                  "    float alt;     // altura del sol, -1 (medianoche) .. 1 (mediodia)\n"
                  "    float luz;     // cuanta luz hay: la altura recortada a 0 de noche\n"
                  "END\n"
                  "BEGIN\n"
                  "    ctype = C_3D; csubtype = C3D_LIGHT;\n"
                  "    x = 0.0; y = 0.0; z = 0.0;\n"
                  "    color_r = 255; color_g = 245; color_b = 219;   // el LOOP los mueve con el sol\n"
                  "    entity = g3d_light_create(0);   // sin color: lo ponen color_r/g/b\n"
                  "    g3d_light_enable_shadow(entity, 1); g3d_set_shadows(1);\n"
                  "    g3d_set_shadow_resolution(%d);\n"
                  "    hora = %.1f;\n"
                  "    LOOP\n"
                  "        // Un dia entero cada %.0f segundos.\n"
                  "        hora = hora + %.4f;\n"
                  "        IF (hora >= 360000) hora = hora - 360000; END\n"
                  "\n"
                  "        alt = sin(hora);\n"
                  "        luz = alt; IF (luz < 0) luz = 0; END\n"
                  "\n"
                  "        // El sol describe un arco: sale por un lado y se pone por el otro.\n"
                  "        target_x = -cos(hora) * 0.8;\n"
                  "        target_y = -alt;\n"
                  "        target_z = -0.35;\n"
                  "\n"
                  "        // De noche queda una pizca de luz para que no sea negro absoluto.\n"
                  "        intensity = 0.05 + %.3f * luz;\n"
                  "        // Calido al amanecer y al ocaso, blanco al mediodia.\n"
                  "        color_r = 255;\n"
                  "        color_g = 150 + 105 * luz;\n"
                  "        color_b = 90 + 165 * luz;\n"
                  "\n"
                  "        // El cielo acompana al sol, o se nota mucho el truco.\n"
                  "        g3d_sky_set_gradient(0.05 + 0.30*luz, 0.07 + 0.48*luz, 0.15 + 0.70*luz,\n"
                  "                             0.10 + 0.72*luz, 0.09 + 0.79*luz, 0.18 + 0.78*luz);\n"
                  "\n"
                  "        // Para parar el tiempo desde tu codigo: comenta la linea de 'hora'.\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n",
                  pref.c_str(), shadow_res, sun_hour * 1000.0f, sun_day_sec,
                  360000.0f / (sun_day_sec * 60.0f), sun_intensity);
        }

        if (show_fps) {
            /* Medir el coste tiene que ser barato: el texto se rehace una vez por
               segundo, no en cada frame. Un contador que cuesta lo que mide no
               sirve para medir. */
            /* write_var se enlaza UNA vez y el texto sigue a la variable. Con
               write() habria que borrar el anterior en cada refresco, y
               delete_text no esta disponible aqui. */
            fputs("GLOBAL string g3d_dbg_txt;\n"
                  "PROCESS depurar_coste()\n"
                  "PRIVATE\n"
                  "    int frames; int t0; int fps;\n"
                  "END\n"
                  "BEGIN\n"
                  "    frames = 0; t0 = timer[0]; fps = 0;\n"
                  "    g3d_dbg_txt = \"...\";\n"
                  "    write_var(0, 8, 8, 0, g3d_dbg_txt);   // se enlaza una sola vez\n"
                  "    LOOP\n"
                  "        frames = frames + 1;\n"
                  "        IF (timer[0] - t0 >= 100)   // timer va en centesimas\n"
                  "            fps = frames * 100 / (timer[0] - t0);\n"
                  "            frames = 0; t0 = timer[0];\n"
                  "            g3d_dbg_txt = \"FPS \" + fps + \"   dibujos \" + g3d_draw_calls() +\n"
                  "                          \"   triangulos \" + g3d_triangles();\n"
                  "        END\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n", f);
        }

        if (water_on) {
            /* El agua como proceso: lo que cambia en marcha (oleaje, espuma,
               rompiente, salpicaduras) sale de sus locales, asi que desde el
               juego se puede levantar una tormenta sin tocar C. Lo que es de
               autoria -- color, textura, nivel -- se queda como llamadas de una
               vez en escena_iniciar, porque no tiene sentido reenviarlo cada
               frame. */
            fprintf(f,
                  "PROCESS %s_agua()\n"
                  "BEGIN\n"
                  "    ctype = C_3D; csubtype = C3D_WATER;\n"
                  "    entity = 0;   // el agua es UNA: basta con no ser -1\n"
                  "    water.waves = %.4f; water.wave_len = %.4f; water.wave_speed = %.4f;\n"
                  "    water.foam = %.4f; water.surf = %.4f; water.splash = %.4f;\n"
                  "    water.evaporation = %.4f;   // sube = sequia, baja = crecida\n"
                  "    water.flow = %.3f;          // velocidad de la corriente\n"
                  "    target_x = %.2f;   // rumbo de las olas de playa, en grados\n"
                  "    LOOP\n"
                  "        // Sube water.waves y water.surf aqui y tienes tormenta.\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n",
                  pref.c_str(),
                  w_amp, w_len, w_speed, water_foam, surf_height, splash_amount,
                  ws_evap, ws_flow, surf_dir);
        }

        // ---- cada especie sembrada, como proceso BennuGD2 ----
        // Un bosque no es un objeto por arbol: el proceso ES la especie entera,
        // que se dibuja de una vez. Sus locales (wind, draw_dist, solid) mandan
        // sobre ella igual que intensity manda sobre una luz, asi que desde el
        // juego se puede parar el viento o acercar el recorte sin tocar C.
        for (int k = 0; k < g3d_scatter_kinds(); k++) {
            const char* nm = g3d_scatter_kind_asset(k);
            if (!nm) continue;
            std::string pn = nm;
            auto sl = pn.rfind('/'); if (sl != std::string::npos) pn = pn.substr(sl + 1);
            auto dt = pn.rfind('.');  if (dt != std::string::npos) pn = pn.substr(0, dt);
            for (auto& c : pn) if (!isalnum((unsigned char)c)) c = '_';
            fprintf(f,
                  "PROCESS %s_veg_%s()\n"
                  "BEGIN\n"
                  "    ctype = C_3D; csubtype = C3D_SCATTER;\n"
                  "    // 'entity' ata este proceso a SU especie; los ejemplares ya\n"
                  "    // los cargo g3d_scatter_load al montar la escena.\n"
                  "    entity = g3d_scatter_group(\"%s\");\n"
                  "    wind = %.3f;        // balanceo (0 = quieto: rocas, troncos)\n"
                  "    draw_dist = %.1f;   // mas alla no se dibuja\n"
                  "    solid = %d;         // 1 = bloquea el paso\n"
                  "    LOOP\n"
                  "        // Cambia wind o draw_dist aqui y se aplica solo.\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n",
                  pref.c_str(), pn.c_str(), nm,
                  g3d_scatter_get_kind_wind(k),
                  g3d_scatter_get_kind_distance(k),
                  g3d_scatter_get_kind_solid(k));
        }

        // ---- la camara como proceso BennuGD2 (variables nativas) ----
        // La entidad la crea escena_iniciar y la activa (para que no haya un frame
        // sin camara); aqui se ata a 'entity' y se conduce con x/y/z (posicion) y
        // target_x/y/z (a donde mira). El hook las envia a la camara cada FRAME.
        // La camara sigue a un objeto 3D... o a un sprite: el sprite se lleva un
        // marcador invisible y es ese el que va en follow_ent.
        bool follow = (cam_mode != 0 &&
                       ((cam_follow >= 0 && cam_follow < (int)objects.size()) ||
                        (spr_follow >= 0 && spr_follow < (int)sprites.size())));
        {
            std::string cp =
                "PROCESS " + pref + "_camara(int cam)\n"
                "PRIVATE float tx; float ty; float tz; float dist; float libre;\n"
                "        float ax; float ay; float az; float alen;\nEND\n"
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
            /* El brazo de camara ya no apunta solo hacia atras: lleva el angulo de
               orbita, que es lo que permite ver al personaje de perfil (2.5D) o de
               frente sin tocar el codigo a mano. Con angulo 0 sale exactamente lo
               de antes. */
            float ob_rad = cam_orbit * 0.0174533f;
            float armx = sinf(ob_rad) * gcam_dist;
            float army = cam_height - 1.0f;
            float armz = -cosf(ob_rad) * gcam_dist;
            float armlen = sqrtf(armx*armx + army*army + armz*armz);
            if (armlen < 0.01f) armlen = 0.01f;
            if (follow && cam_mode == 1) { snprintf(b, sizeof(b),
                    "    dist = %.3f;   // el brazo empieza entero\n", armlen); cp += b; }
            cp += "    LOOP\n";
            if (follow) {
                cp += "        g3d_entity_get_position(follow_ent, &tx, &ty, &tz);\n";
                if (cam_mode == 1 && cam_girable) {
                    /* Brazo GIRABLE: su direccion se recalcula cada frame desde
                       escena_orbita, que mueve el jugador. Con el brazo baked (el
                       caso de abajo) la camara se quedaba clavada en un angulo. */
                    snprintf(b, sizeof(b),
                        "        // el brazo gira con escena_orbita (lo mueve el jugador)\n"
                        "        ax = sin(escena_orbita) * %.4f;\n"
                        "        az = 0.0 - cos(escena_orbita) * %.4f;\n"
                        "        ay = %.4f;\n"
                        "        alen = sqrt(ax * ax + ay * ay + az * az);\n"
                        "        IF (alen < 0.01) alen = 0.01; END\n"
                        "        // se acorta si hay terreno o algo en medio\n"
                        "        libre = g3d_camera_safe_distance(tx, ty + 1.0, tz,\n"
                        "                                         ax, ay, az, alen, 0.6);\n"
                        "        IF (libre < dist) dist = libre;\n"
                        "        ELSE dist = dist + (libre - dist) * 0.08; END\n"
                        "        x = tx + dist * ax / alen; y = ty + 1.0 + dist * ay / alen;\n"
                        "        z = tz + dist * az / alen;\n"
                        "        target_x = tx; target_y = ty + 1.0; target_z = tz;\n",
                        gcam_dist, gcam_dist, cam_height - 1.0f);
                    // (el bloque se pega mas abajo, con el de los demas modos)
                } else if (cam_mode == 1) {          // tercera persona
                    /* Brazo de camara CON COLISION. Sin esto la camara se mete
                       dentro de una loma y el personaje desaparece: se ve el
                       interior del terreno. g3d_camera_safe_distance acorta el
                       brazo hasta justo antes del estorbo -- terreno incluido,
                       que es lo que el raycast normal no mira. */
                    snprintf(b, sizeof(b),
                        "        // Brazo de camara: se acorta si hay terreno o algo en medio.\n"
                        "        libre = g3d_camera_safe_distance(tx, ty + 1.0, tz,\n"
                        "                                         %.4f, %.4f, %.4f,\n"
                        "                                         %.3f, 0.6);\n"
                        "        // Entra DEPRISA y sale despacio. Al reves se ve el interior\n"
                        "        // de la loma un fotograma; y volviendo de golpe al salir, la\n"
                        "        // camara pega un tiron cada vez que rozas una roca.\n"
                        "        IF (libre < dist) dist = libre;   // estorbo: al sitio, ya\n"
                        "        ELSE dist = dist + (libre - dist) * 0.08; END   // libre: despacio\n"
                        "        x = tx + dist * %.4f; y = ty + 1.0 + dist * %.4f;\n"
                        "        z = tz + dist * %.4f;\n"
                        "        target_x = tx; target_y = ty + 1.0; target_z = tz;\n",
                        armx, army, armz, armlen,
                        armx / armlen, army / armlen, armz / armlen);
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

        // Nombre valido de BennuGD2 a partir de un texto cualquiera.
        auto gen_ident = [](const std::string& t, const char* pref) { return ident_bgd(t, pref); };

        // ---- SPRITES 2D del mundo (hojas de sprites, estilo HD-2D) ----
        // Por cada hoja se genera: su carga con map_load y las TABLAS de recortes
        // (un fotograma por posicion). Por cada personaje, un PROCESS que mueve
        // sus locales y pide el recorte del fotograma que toca.
        std::vector<std::string> spr_load, spr_launch;
        if (!sprites.empty()) {
            std::map<std::string, std::string> hojas;   // imagen -> prefijo de sus tablas
            std::string gl;
            for (auto& sp : sprites) {
                if (sp.sheet.empty() || hojas.count(sp.sheet)) continue;
                SheetDef* sh = sheet_of(sp.sheet);
                if (!sh || sh->frames.empty()) {
                    console_add("[SPRITE] '" + sp.sheet + "' no tiene fotogramas detectados: "
                                "abrela en el panel Sprites 3D y dale a Detectar.\n");
                    continue;
                }
                std::string pre = "hoja_" + gen_ident(sp.sheet.substr(0, sp.sheet.rfind('.')), "h");
                hojas[sp.sheet] = pre;
                int n = (int)sh->frames.size();
                gl += "    int " + pre + ";   // Assets/" + sp.sheet + "\n";
                const char* sufijos[6] = { "_x", "_y", "_w", "_h", "_ax", "_ay" };
                for (int k = 0; k < 6; k++) {
                    gl += "    int " + pre + sufijos[k] + "[" + std::to_string(n - 1) + "] =";
                    for (int i = 0; i < n; i++) {
                        const SprFrame& f2 = sh->frames[i];
                        int v = (k == 0) ? f2.x : (k == 1) ? f2.y : (k == 2) ? f2.w
                              : (k == 3) ? f2.h : (k == 4) ? f2.ax : f2.ay;
                        gl += (i ? "," : " ") + std::to_string(v);
                    }
                    gl += ";\n";
                }
                for (auto& an : sh->anims) {
                    if (an.frames.empty()) continue;
                    gl += "    int " + pre + "_" + gen_ident(an.name, "a") + "[" +
                          std::to_string((int)an.frames.size() - 1) + "] =";
                    for (size_t i = 0; i < an.frames.size(); i++)
                        gl += (i ? "," : " ") + std::to_string(an.frames[i]);
                    gl += ";\n";
                }
                spr_load.push_back("    " + pre + " = map_load(\"Assets/" + ruta_asset(sp.sheet) + "\");\n");
            }
            if (!gl.empty()) {
                fputs("// ===== SPRITES 2D del mundo (hojas colocadas en el editor) =====\n"
                      "// Las tablas son el recorte de cada fotograma dentro de la hoja:\n"
                      "// x,y,w,h y el ancla ax,ay (el punto que se apoya en el suelo).\n", f);
                add_global(gl);
            }
            // ---- un PROCESS por personaje ----
            // Un sprite se maneja como cualquier objeto del editor: puede ser
            // decorativo, un cuerpo fisico, un muro, o EL JUGADOR (con su capsula
            // de colision, sus teclas y sus animaciones por estado).
            std::vector<std::string> pn(sprites.size());
            for (size_t i = 0; i < sprites.size(); i++) {
                if (!hojas.count(sprites[i].sheet)) continue;
                std::string nom = gen_ident(sprites[i].name.empty() ? "sprite" : sprites[i].name, "spr_");
                for (int k = 2; ; k++) {
                    bool rep = false;
                    for (size_t j = 0; j < i; j++) if (pn[j] == nom) { rep = true; break; }
                    if (!rep) break;
                    nom = gen_ident(sprites[i].name, "spr_") + "_" + std::to_string(k);
                }
                pn[i] = nom;
            }
            // Tablas de estados del jugador: los fotogramas de quieto/andar/correr/
            // saltar, uno detras de otro, con donde empieza y cuantos tiene cada uno.
            std::string gl2;
            for (size_t i = 0; i < sprites.size(); i++) {
                SprObj& sp = sprites[i];
                if (pn[i].empty() || !sp.is_player) continue;
                SheetDef* sh = sheet_of(sp.sheet);
                if (!sh) continue;
                // 0..3 = lo que hace (quieto, andando, corriendo, saltando);
                // 4..7 = la animacion propia de cada tecla, si la has puesto.
                std::vector<const std::string*> est = { &sp.an_idle, &sp.an_walk, &sp.an_run,
                                                        &sp.an_jump, &sp.an_up, &sp.an_down,
                                                        &sp.an_left, &sp.an_right };
                for (auto& ac : sp.acciones) est.push_back(&ac.anim);   // 8, 9, 10... una por accion
                std::vector<int> todos, ini_, num_;
                for (int e = 0; e < (int)est.size(); e++) {
                    const SprAnim* an = nullptr;
                    for (auto& a : sh->anims) if (a.name == *est[e]) { an = &a; break; }
                    if (!an && e > 0) for (auto& a : sh->anims) if (a.name == sp.an_idle) { an = &a; break; }
                    ini_.push_back((int)todos.size());
                    if (an && !an->frames.empty()) {
                        for (int fr : an->frames) todos.push_back(fr);
                        num_.push_back((int)an->frames.size());
                    } else {                       // sin animacion: el primer fotograma
                        todos.push_back(0);
                        num_.push_back(1);
                    }
                }
                auto tab = [&](const char* suf, const std::vector<int>& v) {
                    gl2 += "    int " + pn[i] + suf + "[" + std::to_string((int)v.size() - 1) + "] =";
                    for (size_t k = 0; k < v.size(); k++) gl2 += (k ? "," : " ") + std::to_string(v[k]);
                    gl2 += ";\n";
                };
                gl2 += "    // " + pn[i] + ": quieto / andando / corriendo / saltando"
                       " / adelante / atras / izquierda / derecha\n";
                tab("_est", todos); tab("_ini", ini_); tab("_num", num_);
            }
            if (!gl2.empty()) {
                add_global(gl2);
            }

            /* Los controles van RESPECTO A LA CAMARA (igual que en los objetos):
               con la camara girada, "adelante" es el fondo de la PANTALLA. */
            float ob = cam_orbit * 0.0174533f;
            float fxk = -sinf(ob), fzk =  cosf(ob);
            float rxk = -cosf(ob), rzk = -sinf(ob);

            for (size_t i = 0; i < sprites.size(); i++) {
                SprObj& sp = sprites[i];
                if (pn[i].empty()) continue;
                SheetDef* sh = sheet_of(sp.sheet);
                std::string pre = hojas[sp.sheet], nom = pn[i];
                // sus reglas, en codigo: una parte antes del LOOP y otra dentro
                std::string spr_regla_ini, spr_regla_loop;
                reglas_codigo(sp.reglas, base_reglas_spr(sp),
                              (sp.phys >= 1 && sp.phys <= 4) ? 1 : 0, 1,
                              spr_regla_ini, spr_regla_loop);
                int maxh = 1;
                for (auto& fr : sh->frames) if (fr.h > maxh) maxh = fr.h;
                float ppu = (sp.height > 0.01f) ? maxh / sp.height : 32.0f;
                int dirs = (sp.dirs > 1) ? sp.dirs : 1;
                // cabecera comun
                fprintf(f, "// SPRITE 2D '%s'  (Assets/%s)%s\n", nom.c_str(), sp.sheet.c_str(),
                        sp.is_player ? "  - EL JUGADOR" :
                        (sp.phys >= 1 && sp.phys <= 4) ? "  - cuerpo fisico" :
                        (sp.phys == 5) ? "  - muro invisible" : "");
                fprintf(f, "PROCESS %s()\nPRIVATE\n", nom.c_str());
                if (sp.is_player) {
                    fputs("    int ch;        // capsula de colision del personaje\n"
                          "    int seguidor;  // marcador invisible: es a quien sigue la camara\n"
                          "    int fot; int paso; int tic; int dir; int est; int npasos; int inw;\n"
                          "    float wx; float wz; float wl; float spd; float facing; float dt;\n"
                          "    float adel; float lat; float px; float py; float pz; float wlev;\n"
                          "    float prevx; float prevz;\n", f);
                    if (!sp.acciones.empty())
                        fprintf(f, "    int acc; int acc_t;   // accion en marcha y lo que le queda\n"
                                   "    int ant[%d];          // si la tecla/boton ya venia pulsado\n",
                                (int)sp.acciones.size() - 1);
                    if (sp.usar_mando)
                        fputs("    float ejex; float ejey;   // stick del mando\n", f);
                    fputs("END\n", f);
                }
                else if (sp.phys >= 1 && sp.phys <= 4)
                    fputs("    int cuerpo;    // el cuerpo rigido (Jolt)\n"
                          "    int fot; int paso; int tic;\n"
                          "    float bx; float by; float bz; float wl2; float dt;\n"
                          "END\n", f);
                else {
                    fputs("    int fot; int paso; int tic; int dir;\n", f);
                    if (sp.comport > 0)
                        fputs("    float dx; float dz; float d; int andando;\n"
                              "    float destx; float destz; int ida;   // patrulla\n", f);
                    if (sp.solido)   fputs("    int caja;   // su colision, que le sigue\n", f);
                    if (sp.inter_on) {
                        fputs("    int cerca; int ant_i; int acc_t;   // interaccion\n", f);
                        if (sp.comport == 0) fputs("    float dx; float dz;\n", f);
                    }
                    fputs("END\n", f);
                }
                fputs("BEGIN\n", f);
                fputs("    ctype = C_3D;  csubtype = C3D_SPRITE;\n", f);
                fprintf(f, "    x = %.3f; y = %.3f; z = %.3f;\n", sp.x, sp.y, sp.z);
                fputs("    entity = g3d_sprite_create(scene, x, y, z);\n", f);
                fprintf(f, "    g3d_sprite_set_pixels_per_unit(entity, %.3f);   // el fotograma mas alto mide %.2f unidades\n",
                        ppu, sp.height);
                fprintf(f, "    g3d_sprite_set_billboard(entity, %s);\n",
                        sp.billboard ? "G3D_SPRITE_FACING" : "G3D_SPRITE_UPRIGHT");
                fprintf(f, "    g3d_sprite_set_cutout(entity, %.2f);\n", sp.cutout);
                fprintf(f, "    g3d_sprite_set_shadow(entity, %d);\n", sp.shadow ? 1 : 0);
                fprintf(f, "    g3d_sprite_set_smooth(entity, %d);\n", sp.smooth ? 1 : 0);
                fprintf(f, "    g3d_sprite_set_lit(entity, %d);   // se apaga con la luz de la escena\n",
                        sp.iluminado ? 1 : 0);
                fprintf(f, "    g3d_sprite_set_snap(entity, %d);   // ajuste a pixel (no tiembla)\n",
                        sp.ajuste_px ? 1 : 0);
                fprintf(f, "    file = 0;  graph = %s;   // la hoja entera: el recorte lo pone el fotograma\n",
                        pre.c_str());

                // Que fotograma dibujar: siempre igual, se saca del recorte.
                std::string poner_fot =
                    std::string("        g3d_sprite_set_cell(entity, ") + pre + "_x[fot], " + pre +
                    "_y[fot], " + pre + "_w[fot], " + pre + "_h[fot]);\n" +
                    "        g3d_sprite_set_anchor(entity, 1.0 * " + pre + "_ax[fot] / " + pre +
                    "_w[fot], 1.0 * " + pre + "_ay[fot] / " + pre + "_h[fot]);\n";

                if (sp.is_player) {
                    // ---------------- EL JUGADOR ----------------
                    // El ritmo de la animacion sale de los fps de SU animacion de
                    // andar (o la de quieto), no de un numero inventado.
                    int fps_jug = 10;
                    if (sh) for (auto& a : sh->anims)
                        if (a.name == sp.an_walk || (sp.an_walk.empty() && a.name == sp.an_idle))
                            { fps_jug = a.fps > 0 ? a.fps : 10; break; }
                    if (sp.fps > 0) fps_jug = sp.fps;      // los fps del personaje mandan
                    int tics_jug = 60 / fps_jug;
                    if (tics_jug < 1) tics_jug = 1;
                    fputs("    dt = 1.0 / 60.0;\n", f);
                    fprintf(f, "    ch = g3d_char_create(%.3f, %.3f, %.3f, %.3f, %.3f);   // x,y,z,radio,altura\n",
                            sp.x, sp.y, sp.z, sp.char_radius, sp.char_height);
                    fputs("    g3d_char_set_tuning(ch, 0.8, 46.0);\n"
                          "    g3d_char_set_push(ch, 200.0);   // aparta cajas y barriles\n", f);
                    fputs("    // La camara sigue a una entidad, y un sprite no lo es: se lleva un\n"
                          "    // marcador invisible pegado al personaje y la camara mira a ese.\n"
                          "    seguidor = g3d_entity_spawn(scene, 0, x, y, z);\n", f);
                    if ((int)i == spr_follow)
                        fputs("    follow_ent = seguidor;   // la camara le sigue\n", f);
                    // las reglas de este personaje ("si pasa esto, haz esto")
                    fputs(spr_regla_ini.c_str(), f);
                    fputs("\n    LOOP\n"
                          "        prevx = g3d_char_x(ch); prevz = g3d_char_z(ch);\n", f);
                    // girar la camara antes de leer los controles, que salen de su angulo
                    fputs(girar_camara_codigo().c_str(), f);
                    fprintf(f,
                          "        // ---------- CONTROLES (las teclas del editor, respecto a la camara) ----------\n"
                          "        adel = 0.0; lat = 0.0;\n"
                          "        IF (key(%s)) adel = adel + 1.0; END\n"
                          "        IF (key(%s)) adel = adel - 1.0; END\n"
                          "        IF (key(%s)) lat  = lat  + 1.0; END\n"
                          "        IF (key(%s)) lat  = lat  - 1.0; END\n",
                          sp.k_up.c_str(), sp.k_down.c_str(), sp.k_right.c_str(), sp.k_left.c_str());
                    if (sp.usar_mando)
                        fputs("        // ---------- MANDO: stick izquierdo y cruceta ----------\n"
                              "        IF (joy_is_attached())\n"
                              "            ejex = joy_getaxis(JOY_AXIS_LEFTX) / 32000.0;\n"
                              "            ejey = joy_getaxis(JOY_AXIS_LEFTY) / 32000.0;\n"
                              "            IF (ejex > 0.25 OR ejex < -0.25) lat = lat + ejex; END\n"
                              "            IF (ejey > 0.25 OR ejey < -0.25) adel = adel - ejey; END\n"
                              "            IF (joy_getbutton(JOY_BUTTON_DPAD_UP))    adel = adel + 1.0; END\n"
                              "            IF (joy_getbutton(JOY_BUTTON_DPAD_DOWN))  adel = adel - 1.0; END\n"
                              "            IF (joy_getbutton(JOY_BUTTON_DPAD_RIGHT)) lat  = lat  + 1.0; END\n"
                              "            IF (joy_getbutton(JOY_BUTTON_DPAD_LEFT))  lat  = lat  - 1.0; END\n"
                              "        END\n", f);
                    fputs(ejes_camara_codigo("        ").c_str(), f);
                    {
                        std::string correr = "key(" + sp.k_run + ")";
                        if (!sp.b_run.empty()) correr += " OR joy_getbutton(" + sp.b_run + ")";
                        std::string saltar = "key(" + sp.k_jump + ")";
                        if (!sp.b_jump.empty()) saltar += " OR joy_getbutton(" + sp.b_jump + ")";
                        fprintf(f,
                          "        spd = %.3f;\n"
                          "        IF (%s) spd = %.3f; END   // correr\n"
                          "        wl = sqrt(wx * wx + wz * wz);\n"
                          "        IF (wl > 0.001)\n"
                          "            wx = wx / wl * spd; wz = wz / wl * spd;\n"
                          "            facing = atan2(wx, wz);   // mira hacia donde anda\n"
                          "        END\n"
                          "        g3d_char_move(ch, wx, wz);\n"
                          "        IF (%s) g3d_char_jump(ch, %.3f); END\n"
                          "        g3d_char_update(ch, dt);\n"
                          "        px = g3d_char_x(ch); py = g3d_char_y(ch); pz = g3d_char_z(ch);\n",
                          sp.walk_speed, correr.c_str(), sp.run_speed,
                          saltar.c_str(), sp.jump_force);
                    }
                    fputs("        // ---------- AGUA: nada si la hay debajo ----------\n"
                          "        wlev = g3d_water_level_at(px, pz);\n"
                          "        IF (wlev > -100000.0)\n"
                          "            inw = 0;\n"
                          "            IF (py < wlev - 1.2) inw = 1; END\n"
                          "            g3d_char_set_water(ch, inw, wlev);\n"
                          "        ELSE\n"
                          "            g3d_char_set_water(ch, 0, 0.0);\n"
                          "        END\n", f);
                    if (sp.zone_layer >= 0)
                        fprintf(f,
                          "        // ---------- ZONAS: no puede entrar en la capa %d ----------\n"
                          "        IF (g3d_zone_blocked(px, pz, %d))\n"
                          "            g3d_char_set_position(ch, prevx, py, prevz);\n"
                          "            px = prevx; pz = prevz;\n"
                          "        END\n", sp.zone_layer, sp.zone_layer);
                    fputs("        // vars nativas: el motor lo dibuja solo al hacer FRAME\n"
                          "        x = px; y = py; z = pz;\n"
                          "        angle = facing;\n"
                          "        g3d_entity_set_position(seguidor, px, py, pz);\n"
                          "        jug_x = px; jug_y = py; jug_z = pz;   // los NPC lo leen\n", f);
                    fputs("        // ---------- ANIMACION segun lo que esta haciendo ----------\n"
                          "        est = 0;                                  // quieto\n"
                          "        IF (wl > 0.001) est = 1; END              // andando\n", f);
                    {
                        std::string correr = "key(" + sp.k_run + ")";
                        if (!sp.b_run.empty()) correr += " OR joy_getbutton(" + sp.b_run + ")";
                        fprintf(f, "        IF (wl > 0.001 AND (%s)) est = 2; END  // corriendo\n",
                                correr.c_str());
                    }
                    {   // Una animacion propia por tecla (y/o espejada): mientras la
                        // tengas pulsada manda sobre la del estado. 'flags = 1' voltea
                        // el sprite, que es lo que resuelve la derecha cuando solo
                        // tienes dibujada la izquierda.
                        const char* tk[4] = { sp.k_up.c_str(), sp.k_down.c_str(),
                                              sp.k_left.c_str(), sp.k_right.c_str() };
                        const std::string* ta[4] = { &sp.an_up, &sp.an_down, &sp.an_left, &sp.an_right };
                        const int fx[4] = { sp.fx_up, sp.fx_down, sp.fx_left, sp.fx_right };
                        const char* et[4] = { "adelante", "atras", "izquierda", "derecha" };
                        bool hay_espejo = (sp.fx_up || sp.fx_down || sp.fx_left || sp.fx_right);
                        if (hay_espejo)
                            fputs("        flags = 0;   // sin espejo salvo que la tecla lo pida\n", f);
                        for (int q = 0; q < 4; q++) {
                            if (ta[q]->empty() && !fx[q]) continue;
                            std::string com = et[q];
                            if (!ta[q]->empty()) com += ": " + *ta[q];
                            if (fx[q]) com += " (espejada)";
                            fprintf(f, "        IF (key(%s))", tk[q]);
                            if (!ta[q]->empty()) fprintf(f, " est = %d;", 4 + q);
                            if (fx[q])           fputs(" flags = 1;", f);
                            fprintf(f, " END   // %s\n", com.c_str());
                        }
                    }
                    // ---- ACCIONES: tus teclas y botones ----
                    if (!sp.acciones.empty()) {
                        SheetDef* sh2 = sheet_of(sp.sheet);
                        fputs("        // ---------- ACCIONES (las que pusiste en el editor) ----------\n"
                              "        IF (acc_t > 0) acc_t = acc_t - 1; END\n"
                              "        IF (acc_t <= 0)\n"
                              "            acc = 0;\n", f);
                        for (int q = 0; q < (int)sp.acciones.size(); q++) {
                            const SprAccion& ac = sp.acciones[q];
                            std::string cond;
                            if (!ac.tecla.empty()) cond = "key(" + ac.tecla + ")";
                            if (!ac.boton.empty())
                                cond += (cond.empty() ? "" : " OR ") + std::string("joy_getbutton(") + ac.boton + ")";
                            if (cond.empty()) continue;
                            // cuantos frames dura si suena entera (fotogramas x su ritmo)
                            int dur = 20;
                            if (sh2) for (auto& a2 : sh2->anims)
                                if (a2.name == ac.anim) {
                                    int t2 = 60 / (a2.fps > 0 ? a2.fps : 10);
                                    if (t2 < 1) t2 = 1;
                                    dur = (int)a2.frames.size() * t2;
                                    break;
                                }
                            fprintf(f, "            IF ((%s)%s)   // %s\n", cond.c_str(),
                                    ac.una_vez ? (" AND ant[" + std::to_string(q) + "] == 0").c_str() : "",
                                    ac.nombre.c_str());
                            fprintf(f, "                acc = %d;", q + 1);
                            if (ac.una_vez)
                                fprintf(f, " acc_t = %d; paso = 0; tic = 0;", dur);
                            if (!ac.dialogo.empty())
                                fprintf(f, " IF (NOT exists(TYPE %s)) %s(); END",
                                        ac.dialogo.c_str(), ac.dialogo.c_str());
                            if (!ac.llama.empty())
                                fprintf(f, " %s();", gen_ident(ac.llama, "f").c_str());
                            fputs("\n            END\n", f);
                        }
                        fputs("        END\n", f);
                        // memoria de si ya venia pulsado, para que una accion "entera"
                        // no se repita sola mientras aguantas la tecla
                        for (int q = 0; q < (int)sp.acciones.size(); q++) {
                            const SprAccion& ac = sp.acciones[q];
                            if (!ac.una_vez) continue;
                            std::string cond;
                            if (!ac.tecla.empty()) cond = "key(" + ac.tecla + ")";
                            if (!ac.boton.empty())
                                cond += (cond.empty() ? "" : " OR ") + std::string("joy_getbutton(") + ac.boton + ")";
                            if (cond.empty()) continue;
                            fprintf(f, "        ant[%d] = 0;  IF (%s) ant[%d] = 1; END\n",
                                    q, cond.c_str(), q);
                        }
                    }
                    fputs("        IF (g3d_char_grounded(ch) == 0) est = 3; END   // en el aire\n", f);
                    // Una accion en marcha manda sobre andar, correr y saltar. Va
                    // ANTES de calcular npasos: si no, el numero de pasos seria el
                    // del estado anterior y la animacion se cortaria a destiempo.
                    if (!sp.acciones.empty()) {
                        for (int q = 0; q < (int)sp.acciones.size(); q++) {
                            const SprAccion& ac = sp.acciones[q];
                            if (ac.anim.empty() && !ac.espejo) continue;
                            fprintf(f, "        IF (acc == %d)", q + 1);
                            if (!ac.anim.empty()) fprintf(f, " est = %d;", 8 + q);
                            if (ac.espejo)        fputs(" flags = 1;", f);
                            fprintf(f, " END   // %s\n", ac.nombre.c_str());
                        }
                    }
                    fprintf(f,
                          "        npasos = %s_num[est] / %d;\n"
                          "        IF (npasos < 1) npasos = 1; END\n"
                          "        tic = tic + 1;\n"
                          "        IF (tic >= %d)  tic = 0;  paso = paso + 1;  END   // %d fotogramas por segundo\n"
                          "        IF (paso >= npasos) paso = 0; END\n",
                          nom.c_str(), dirs, tics_jug, 60 / (tics_jug > 0 ? tics_jug : 1));
                    if (dirs > 1)
                        fprintf(f,
                          "        dir = g3d_sprite_dir(entity, angle, %d);   // postura segun la camara\n"
                          "        fot = %s_est[%s_ini[est] + dir * npasos + paso];\n",
                          dirs, nom.c_str(), nom.c_str());
                    else
                        fprintf(f, "        fot = %s_est[%s_ini[est] + paso];\n", nom.c_str(), nom.c_str());
                    fputs(poner_fot.c_str(), f);
                    fputs(spr_regla_loop.c_str(), f);
                    fputs("        FRAME;\n    END\nEND\n\n", f);

                } else if (sp.phys >= 1 && sp.phys <= 4) {
                    // ---------------- CUERPO FISICO ----------------
                    float c = sp.csize * 0.5f;
                    fputs("    dt = 1.0 / 60.0;\n", f);
                    if (sp.phys == 1)
                        fprintf(f, "    cuerpo = g3d_rigidbody_create(x, y, z, %.3f, %.3f, %.3f, %.3f);\n",
                                c, c, c, sp.mass);
                    else if (sp.phys == 2)
                        fprintf(f, "    cuerpo = g3d_rigidbody_create_sphere(x, y, z, %.3f, %.3f);\n",
                                c, sp.mass);
                    else if (sp.phys == 3)
                        fprintf(f, "    cuerpo = g3d_rigidbody_create_capsule(x, y, z, %.3f, %.3f, %.3f);\n",
                                c, c, sp.mass);
                    else
                        fprintf(f, "    cuerpo = g3d_rigidbody_create_cylinder(x, y, z, %.3f, %.3f, %.3f);\n",
                                c, c, sp.mass);
                    fprintf(f, "    g3d_rigidbody_set_bounce(cuerpo, %.3f, %.3f);   // rebote, friccion\n",
                            sp.bounce, sp.friction);
                    // las reglas de este personaje ("si pasa esto, haz esto")
                    fputs(spr_regla_ini.c_str(), f);
                    fputs("\n    LOOP\n"
                          "        // la fisica manda: se leen las coordenadas del cuerpo\n"
                          "        bx = g3d_rigidbody_x(cuerpo); by = g3d_rigidbody_y(cuerpo);\n"
                          "        bz = g3d_rigidbody_z(cuerpo);\n", f);
                    if (sp.buoyant && sp.mass > 0.0f)
                        fprintf(f,
                          "        wl2 = g3d_water_level_at(bx, bz);   // flota en el agua que haya\n"
                          "        IF (wl2 > -100000.0) g3d_rigidbody_set_buoyancy(cuerpo, wl2, %.3f); END\n",
                          sp.density > 0.05f ? sp.density : 0.05f);
                    fputs("        x = bx; y = by; z = bz;\n", f);
                    fputs("        fot = 0;\n", f);
                    fputs(poner_fot.c_str(), f);
                    fputs(spr_regla_loop.c_str(), f);
                    fputs("        FRAME;\n    END\nEND\n\n", f);

                } else {
                    // ---------------- DECORATIVO / NPC ----------------
                    if (sp.solido)
                        fprintf(f,
                          "    // colision propia, que se mueve con el (los muros son fijos)\n"
                          "    caja = g3d_collider_add_box(x - %.3f, y, z - %.3f, x + %.3f, y + %.3f, z + %.3f);\n",
                          sp.sol_radio, sp.sol_radio, sp.sol_radio, sp.height, sp.sol_radio);
                    if (sp.phys == 5) {
                        float half = sp.csize * 0.5f;
                        fprintf(f,
                          "    // muro invisible a su alrededor: no se mueve, pero corta el paso\n"
                          "    g3d_collider_add_box(x - %.3f, y, z - %.3f, x + %.3f, y + %.3f, z + %.3f);\n",
                          half, half, half, sp.height, half);
                    }
                    const SprAnim* an = nullptr;
                    if (sh) for (auto& a : sh->anims) if (a.name == sp.anim) { an = &a; break; }
                    int npasos = an ? (int)an->frames.size() / dirs : 0;
                    if (an && npasos < 1) { npasos = (int)an->frames.size(); dirs = 1; }
                    int fps_o = an ? (an->fps > 0 ? an->fps : 10) : 10;
                    if (sp.fps > 0) fps_o = sp.fps;        // los fps del objeto mandan
                    int tics = an ? (60 / fps_o) : 1;
                    if (tics < 1) tics = 1;
                    // las reglas de este personaje ("si pasa esto, haz esto")
                    fputs(spr_regla_ini.c_str(), f);
                    fputs("\n    LOOP\n", f);
                    if (sp.comport > 0) {
                        float paso = sp.com_vel / 60.0f;
                        fputs("        // ---------- COMPORTAMIENTO ----------\n"
                              "        andando = 0;\n", f);
                        if (sp.comport == 1) {
                            fprintf(f,
                              "        IF (destx == 0.0 AND destz == 0.0 AND ida == 0)\n"
                              "            destx = %.3f; destz = %.3f;   // el punto B\n"
                              "        END\n"
                              "        dx = destx - x;  dz = destz - z;\n"
                              "        d = sqrt(dx * dx + dz * dz);\n"
                              "        IF (d < 0.5)\n"
                              "            IF (ida == 0)  destx = %.3f; destz = %.3f; ida = 1;\n"
                              "            ELSE           destx = %.3f; destz = %.3f; ida = 0;  END\n"
                              "        ELSE\n"
                              "            x = x + dx / d * %.4f;\n"
                              "            z = z + dz / d * %.4f;\n"
                              "            angle = atan2(dx, dz);\n"
                              "            andando = 1;\n"
                              "        END\n",
                              sp.com_bx, sp.com_bz, sp.x, sp.z, sp.com_bx, sp.com_bz, paso, paso);
                        } else {
                            float signo = (sp.comport == 3) ? -1.0f : 1.0f;
                            fprintf(f,
                              "        dx = jug_x - x;  dz = jug_z - z;\n"
                              "        d = sqrt(dx * dx + dz * dz);\n"
                              "        IF (d < %.3f AND d > 1.2)\n"
                              "            x = x + dx / d * %.4f;\n"
                              "            z = z + dz / d * %.4f;\n"
                              "            angle = atan2(dx * %.1f, dz * %.1f);\n"
                              "            andando = 1;\n"
                              "        END\n",
                              sp.com_radio, paso * signo, paso * signo, signo, signo);
                        }
                        fputs("        y = g3d_scene_terrain_height(x, z);   // se apoya en el suelo\n", f);
                    }
                    if (an) {
                        std::string tab = pre + "_" + gen_ident(an->name, "a");
                        // Si anda (patrulla, sigue o huye) y tiene animacion propia
                        // para moverse, esa manda mientras se mueva.
                        const SprAnim* aw = nullptr;
                        if (sp.comport > 0 && !sp.an_walk.empty() && sh)
                            for (auto& a2 : sh->anims) if (a2.name == sp.an_walk) { aw = &a2; break; }
                        int nw = aw ? (int)aw->frames.size() / dirs : 0;
                        if (aw && nw < 1) nw = (int)aw->frames.size();
                        // El contador tiene que dar para la mas larga de las dos: con
                        // el numero de la de 'quieto' (a menudo 1 fotograma) la de
                        // andar no avanzaba nunca.
                        int pasos_max = npasos > nw ? npasos : nw;
                        if (pasos_max < 1) pasos_max = 1;
                        int fps_np = (aw ? aw->fps : an->fps) > 0 ? (aw ? aw->fps : an->fps) : 10;
                        if (sp.fps > 0) fps_np = sp.fps;
                        int tics_np = 60 / fps_np;
                        if (tics_np < 1) tics_np = 1;
                        fprintf(f, "        tic = tic + 1;   // %d fotogramas por segundo\n", fps_np);
                        fprintf(f, "        IF (tic >= %d)  tic = 0;  paso = (paso + 1) %% %d;  END\n",
                                tics_np, pasos_max);
                        if (dirs > 1) {
                            fprintf(f, "        dir = g3d_sprite_dir(entity, angle, %d);\n", dirs);
                            if (aw)
                                fprintf(f, "        IF (andando)  fot = %s_%s[dir * %d + paso %% %d];\n"
                                           "        ELSE          fot = %s[dir * %d + paso %% %d];  END\n",
                                        pre.c_str(), gen_ident(aw->name, "a").c_str(), nw, nw,
                                        tab.c_str(), npasos, npasos);
                            else
                                fprintf(f, "        fot = %s[dir * %d + paso %% %d];\n",
                                        tab.c_str(), npasos, npasos);
                        } else {
                            if (aw)
                                fprintf(f, "        IF (andando)  fot = %s_%s[paso %% %d];\n"
                                           "        ELSE          fot = %s[paso %% %d];  END\n",
                                        pre.c_str(), gen_ident(aw->name, "a").c_str(), nw,
                                        tab.c_str(), npasos);
                            else
                                fprintf(f, "        fot = %s[paso %% %d];\n", tab.c_str(), npasos);
                        }
                    } else {
                        fputs("        fot = 0;\n", f);
                    }
                    if (sp.solido)
                        fprintf(f,
                          "        // la colision le sigue alla donde vaya\n"
                          "        g3d_collider_set_box(caja, x - %.3f, y, z - %.3f, x + %.3f, y + %.3f, z + %.3f);\n",
                          sp.sol_radio, sp.sol_radio, sp.sol_radio, sp.height, sp.sol_radio);
                    if (sp.inter_on) {
                        std::string cond;
                        if (!sp.inter_tecla.empty()) cond = "key(" + sp.inter_tecla + ")";
                        if (!sp.inter_boton.empty())
                            cond += (cond.empty() ? "" : " OR ") +
                                    std::string("joy_getbutton(") + sp.inter_boton + ")";
                        if (cond.empty()) cond = "0";
                        fprintf(f,
                          "        // ---------- INTERACCION: acercarse y pulsar ----------\n"
                          "        dx = jug_x - x;  dz = jug_z - z;\n"
                          "        cerca = 0;\n"
                          "        IF (dx * dx + dz * dz < %.3f) cerca = 1; END\n",
                          sp.inter_radio * sp.inter_radio);
                        if (sp.inter_mirar)
                            fputs("        IF (cerca) angle = atan2(-dx, -dz); END   // se gira hacia ti\n", f);
                        fprintf(f,
                          "        IF (cerca AND (%s) AND ant_i == 0)\n", cond.c_str());
                        if (!sp.inter_dialogo.empty())
                            fprintf(f, "            IF (NOT exists(TYPE %s)) %s(); END   // lo que dice\n",
                                    sp.inter_dialogo.c_str(), sp.inter_dialogo.c_str());
                        if (!sp.inter_llama.empty())
                            fprintf(f, "            %s();\n", gen_ident(sp.inter_llama, "f").c_str());
                        if (!sp.inter_anim.empty()) {
                            int durf = 30;
                            if (sh) for (auto& a2 : sh->anims)
                                if (a2.name == sp.inter_anim) {
                                    int t2 = 60 / (a2.fps > 0 ? a2.fps : 10);
                                    if (t2 < 1) t2 = 1;
                                    durf = (int)a2.frames.size() * t2;
                                    break;
                                }
                            fprintf(f, "            acc_t = %d; paso = 0; tic = 0;   // %s\n",
                                    durf, sp.inter_anim.c_str());
                        }
                        fputs("        END\n", f);
                        fprintf(f, "        ant_i = 0;  IF (%s) ant_i = 1; END\n", cond.c_str());
                        if (!sp.inter_anim.empty()) {
                            // mientras dura la interaccion se dibuja SU animacion
                            std::string tabi = pre + "_" + gen_ident(sp.inter_anim, "a");
                            fprintf(f,
                              "        IF (acc_t > 0)\n"
                              "            acc_t = acc_t - 1;\n"
                              "            fot = %s[paso %% %d];\n"
                              "        END\n",
                              tabi.c_str(),
                              (int)[&]{ int n2 = 1; if (sh) for (auto& a2 : sh->anims)
                                        if (a2.name == sp.inter_anim) n2 = (int)a2.frames.size();
                                        return n2 < 1 ? 1 : n2; }());
                        }
                    }
                    fputs(poner_fot.c_str(), f);
                    fputs(spr_regla_loop.c_str(), f);
                    fputs("        FRAME;\n    END\nEND\n\n", f);
                }
                spr_launch.push_back("    " + nom + "();\n");
                lanzados.push_back(nom);
            }
            if (!spr_launch.empty())
                console_add("Sprites 2D: " + std::to_string((int)spr_launch.size()) +
                            " personajes puestos en main.prg\n");
        }

        // ---- HUD 2D: cada grafico y cada texto, un PROCESS de BennuGD2 ----
        // Se hace como se hace en BennuGD2: los recursos (map/fpg/fnt) se cargan
        // UNA vez en escena_iniciar y quedan en GLOBALs, y cada elemento es un
        // proceso que solo pone SUS LOCALES (file/graph/x/y/z/size/angle/flags/
        // alpha) o llama a write(). Nada de dibujar desde C: el motor grafico ya
        // dibuja los procesos, y el 3D queda siempre por detras.
        std::vector<std::string> hud_res_load;   // cargas para escena_iniciar
        std::vector<std::string> hud_launch;     // arranque de los procesos
        if (!hud.empty()) {
            auto& ident = gen_ident;
            // Texto para un literal de BennuGD2: a latin-1 (que es lo que escribe
            // el motor) y con las comillas y las barras escapadas.
            auto literal = [&](const std::string& t) {
                std::string o = "\"";
                for (char c : hud_latin1(t)) {
                    if (c == '"' || c == '\\') o += '\\';
                    o += c;
                }
                return o + "\"";
            };
            std::map<std::string, std::string> res;   // fichero -> GLOBAL que lo guarda
            std::string hud_globals;
            auto res_var = [&](const std::string& file, bool es_fuente) {
                auto it = res.find(file);
                if (it != res.end()) return it->second;
                const char* pre = es_fuente ? "hud_fnt_" : (hud_is_fpg(file) ? "hud_fpg_" : "hud_map_");
                const char* fn  = es_fuente ? "fnt_load" : (hud_is_fpg(file) ? "fpg_load" : "map_load");
                std::string v = pre + ident(file.substr(0, file.rfind('.')), "g");
                std::string base = v;                    // dos ficheros distintos pueden dar el mismo nombre
                for (int n = 2; ; n++) {
                    bool rep = false;
                    for (auto& r : res) if (r.second == v) { rep = true; break; }
                    if (!rep) break;
                    v = base + "_" + std::to_string(n);
                }
                if (!fs::exists(assets_dir + "/" + file))
                    console_add("[HUD] no encuentro Assets/" + file +
                                ": el elemento saldra en blanco (vuelve a elegirlo en el panel HUD 2D)\n");
                res[file] = v;
                hud_globals += "    int " + v + ";   // Assets/" + file + "\n";
                hud_res_load.push_back("    " + v + " = " + fn + "(\"Assets/" + ruta_asset(file) + "\");\n");
                return v;
            };
            // Nombres de proceso unicos (el nombre lo pone el usuario en el panel).
            std::vector<std::string> pname(hud.size());
            for (size_t i = 0; i < hud.size(); i++) {
                std::string n = ident(hud[i].name.empty() ? "hud" : hud[i].name, "hud_");
                std::string cand = n;
                for (int k = 2; ; k++) {
                    bool rep = false;
                    for (size_t j = 0; j < i; j++) if (pname[j] == cand) { rep = true; break; }
                    if (!rep) break;
                    cand = n + "_" + std::to_string(k);
                }
                pname[i] = cand;
            }
            // Las variables de los textos con write_var tambien son GLOBALs: el
            // texto se refresca solo cuando tu codigo les cambia el valor.
            std::string hud_vars;
            std::set<std::string> vistas;
            for (auto& h : hud) {
                if (h.type != 1 || h.var.empty()) continue;
                std::string v = ident(h.var, "v");
                if (!vistas.insert(v).second) continue;
                const char* tipo = (h.vartype == 1) ? "float" : (h.vartype == 2) ? "string" : "int";
                hud_vars += std::string("    ") + tipo + " " + v + ";\n";
            }
            // Los recursos hay que resolverlos antes de escribir el bloque GLOBAL.
            std::vector<std::string> gvar(hud.size());
            for (size_t i = 0; i < hud.size(); i++) {
                if (hud[i].type == 0 && !hud[i].asset.empty()) gvar[i] = res_var(hud[i].asset, false);
                if (hud[i].type == 1 && !hud[i].font.empty())  gvar[i] = res_var(hud[i].font, true);
            }
            fputs("// ===== HUD 2D (colocado en el editor) =====\n", f);
            // Un GLOBAL vacio no tiene sentido: un HUD de solo textos con la
            // fuente del sistema no carga ningun recurso ni declara variables.
            if (!hud_globals.empty() || !hud_vars.empty()) {
                if (!hud_globals.empty()) {
                    add_global("    // recursos del HUD: se cargan al montar la escena\n");
                    add_global(hud_globals);
                }
                if (!hud_vars.empty()) {
                    add_global("    // variables que muestran los textos: ponles valor desde tu codigo\n");
                    add_global(hud_vars);
                }
            }

            const char* ALIN[9] = { "ALIGN_TOP_LEFT", "ALIGN_TOP", "ALIGN_TOP_RIGHT",
                                    "ALIGN_CENTER_LEFT", "ALIGN_CENTER", "ALIGN_CENTER_RIGHT",
                                    "ALIGN_BOTTOM_LEFT", "ALIGN_BOTTOM", "ALIGN_BOTTOM_RIGHT" };
            for (size_t i = 0; i < hud.size(); i++) {
                HudItem& h = hud[i];
                if (h.type == 0) {
                    if (h.asset.empty()) {                     // sin grafico no hay nada que poner
                        console_add("[HUD] '" + h.name + "' no tiene grafico asignado: no se genera.\n"
                                    "      Eligelo en el panel HUD 2D (necesita un .png/.jpg o un .fpg en Assets).\n");
                        continue;
                    }
                    bool es_fpg = hud_is_fpg(h.asset);
                    fprintf(f, "// HUD: grafico '%s'  (Assets/%s)\n", pname[i].c_str(), h.asset.c_str());
                    fprintf(f, "PROCESS %s()\nBEGIN\n", pname[i].c_str());
                    if (es_fpg) {
                        fprintf(f, "    file  = %s;   // el FPG\n", gvar[i].c_str());
                        fprintf(f, "    graph = %d;   // grafico dentro del FPG\n", h.code);
                    } else {
                        fputs("    file  = 0;   // 0 = grafico suelto (map_load)\n", f);
                        fprintf(f, "    graph = %s;\n", gvar[i].c_str());
                    }
                    fprintf(f, "    x = %.0f; y = %.0f; z = %d;   // x,y = centro del grafico\n",
                            h.x, h.y, h.z);
                    fprintf(f, "    size = %.0f; angle = %d;   // angle en milesimas de grado\n",
                            h.size, (int)(h.angle * 1000.0f));
                    fprintf(f, "    flags = %d; alpha = %d;\n", h.flags, h.alpha);
                    fputs("    LOOP\n"
                          "        // aqui puedes moverlo, cambiarle el graph, el alpha...\n"
                          "        FRAME;\n"
                          "    END\n"
                          "END\n\n", f);
                } else {
                    const char* fnt = h.font.empty() ? "0" : gvar[i].c_str();
                    fprintf(f, "// HUD: texto '%s'  (%s)\n", pname[i].c_str(),
                            h.font.empty() ? "fuente 0 del sistema" : ("Assets/" + h.font).c_str());
                    // 'id' es variable reservada de BennuGD2 (el id del proceso):
                    // llamarla asi da "Variable already declared in another context".
                    fprintf(f, "PROCESS %s()\nPRIVATE\n"
                               "    int idtxt;   // write_move(idtxt,x,y) lo mueve; write_delete(idtxt) lo quita\n"
                               "END\nBEGIN\n", pname[i].c_str());
                    if (h.var.empty())
                        fprintf(f, "    idtxt = write(%s, %.0f, %.0f, %d, %s, %s);\n",
                                fnt, h.x, h.y, h.z, ALIN[h.align < 0 || h.align > 8 ? 0 : h.align],
                                literal(h.text).c_str());
                    else
                        fprintf(f, "    idtxt = write_var(%s, %.0f, %.0f, %d, %s, %s);\n",
                                fnt, h.x, h.y, h.z, ALIN[h.align < 0 || h.align > 8 ? 0 : h.align],
                                ident(h.var, "v").c_str());
                    fprintf(f, "    write_set_rgba(idtxt, %d, %d, %d, %d);\n",
                            h.col[0], h.col[1], h.col[2], h.col[3]);
                    fputs("    LOOP\n"
                          "        FRAME;\n"
                          "    END\n"
                          "END\n\n", f);
                }
                hud_launch.push_back("    " + pname[i] + "();\n");
                lanzados.push_back(pname[i]);
            }
            // Que se vea en la consola que el HUD ha entrado en el juego: si algo
            // no sale en pantalla, aqui se ve si siquiera se ha generado.
            {
                int ng = 0, nt = 0;
                for (size_t i = 0; i < hud.size(); i++)
                    if (hud[i].type == 0) { if (!hud[i].asset.empty()) ng++; } else nt++;
                console_add("HUD 2D: " + std::to_string(ng) + " graficos y " +
                            std::to_string(nt) + " textos puestos en main.prg\n");
            }
        }

        fputs("// Monta el escenario: terreno, agua, objetos, sus procesos y la camara.\n", f);
        fprintf(f, "FUNCTION %s_montar()\n", pref.c_str());
        fputs("PRIVATE int e; int m; int tex; int mat;\nEND\nBEGIN\n", f);
        fputs("    set_mode(1280,720); set_fps(60,0); window_set_title(\"EDITOR_PLAY\");\n", f);
        fputs("    scene = g3d_scene_create(\"juego\"); g3d_scene_set_active(scene);\n", f);
        fputs("    camera = g3d_camera_create(); g3d_camera_set_active(camera);\n", f);
        fprintf(f, "    %s_sol();   // la luz del sol (proceso con vars nativas)\n", pref.c_str());
        lanzados.push_back(pref + "_sol");
        if (show_fps) fputs("    depurar_coste();   // fps y coste en pantalla\n", f);
        if (water_on) { fprintf(f, "    %s_agua();   // el agua, con sus locales\n", pref.c_str());
                        lanzados.push_back(pref + "_agua"); }
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
                        ruta_asset(paints[water_tex_sel].file).c_str());
            fputs("    g3d_water_set_enabled(1);\n", f);
            // Las olas de playa tienen que viajar al juego: si se quedan en el
            // editor, lo que se ve al jugar no es lo que se ha compuesto.
            fprintf(f, "    g3d_water_set_surf(%.4f, %.4f, %.4f, %.4f);\n",
                    surf_amount, surf_len, surf_speed, surf_runup);
            fprintf(f, "    g3d_water_set_surf_wave(%.4f, %.2f);\n",
                    surf_height, surf_dir);
            fprintf(f, "    g3d_water_set_foam(%.4f, 1.0);\n", water_foam);
            fprintf(f, "    g3d_water_set_splash(%.4f, %.4f);\n",
                    splash_amount, splash_speed);
        }
        // ---- lagos y rios: agua colocada (flood-fill / camino), no un mar global ----
        // Cada masa de agua fija SU estilo (olas/color/textura) justo antes de
        // crearse, para que el motor lo capture como propio de esa zona.
        if (!lakes.empty() || !rivers.empty() || !waterfalls.empty()) {
            /* El estilo por masa de agua ya no se emite: con el campo unificado
               esas llamadas no las lee nadie -- son de las mallas viejas, que ya
               no se dibujan. Los exports siguen existiendo para scripts que los
               usen, pero el juego generado ya no los arrastra. */
            auto emit_fx = [&](const WaterFX& x) { (void)x; };
            // Secuencia el estado del motor igual que el preview para poder recortar
            // cada rio contra los lagos + rios YA anadidos (no contra si mismo). Al
            // final se restaura con rebuild_water().
            g3d_fluid_clear(); g3d_flow_clear(); g3d_water_clear_ripple_sources();
            // Bloquea los cauces ANTES de crear los lagos (que no suban por el rio),
            // tanto en el motor (preview del recorte) como en el juego generado.
            g3d_fluid_block_reset();
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
                    /* Sin g3d_flow_set_*: ese estilo era de las cintas viejas,
                       que el campo unificado ya no dibuja. */
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
                /* El estilo de flujo era de las cintas viejas: fuera. */
                fprintf(f, "    g3d_waterfall_add(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f);\n",
                        w.top[0], w.top[1], w.top[2], w.base[0], w.base[2], w.width, w.arc);
                apply_wf(w.fx); g3d_waterfall_add(w.top[0], w.top[1], w.top[2], w.base[0], w.base[2], w.width, w.arc);
            }
            rebuild_water();   // restaura el preview (deshace el secuenciado de arriba)
        }
        // La simulacion del agua tiene que arrancar como en el editor. Sin esto,
        // el juego empieza con el cauce SECO y tarda minutos en llenarse desde el
        // manantial, asi que lo que compones aqui no es lo que luego se ve.
        /* Los MANANTIALES tambien viajan al juego. Sin ellos, un rio nacido de
           una fuente se veia al componer y no existia al jugar: la simulacion
           arrancaba sin nada que verter. */
        for (auto& sw : wsources)
            fprintf(f, "    g3d_watersim_add_source(%.3f, %.3f, %.3f);\n",
                    sw.x, sw.z, sw.rate);
        fprintf(f, "    g3d_watersim_set_evaporation(%.4f);\n", ws_evap);
        /* PRE-LLENADO. En el editor es util ver el rio llenarse poco a poco; en
           el juego casi nunca -- quieres empezar con el agua donde va a estar.
           Estos segundos se simulan de golpe antes del primer frame. */
        fprintf(f, "    g3d_watersim_settle(%.1f);   // el agua ya corrida al arrancar\n",
                ws_prefill);
        // objetos + sus componentes (+ cuerpos fisicos Jolt)
        if (g3d_scatter_count() > 0)
            fprintf(f, "    g3d_set_lod(%.1f);   // malla de bajo poligono a lo lejos\n", sc_lod);
            fprintf(f, "    g3d_scatter_load(\"%s.scatter\", %.3f);   // vegetacion sembrada\n",
                    rel_relief.substr(0, rel_relief.rfind(".terrain")).c_str(), 1.0f);
            // Y un PROCESS por especie, para que desde el juego se pueda tocar
            // como cualquier otra cosa 3D: sus locales mandan sobre su bosque.
            for (int k = 0; k < g3d_scatter_kinds(); k++) {
                const char* nm = g3d_scatter_kind_asset(k);
                if (!nm) continue;
                std::string pn = nm;
                auto sl = pn.rfind('/'); if (sl != std::string::npos) pn = pn.substr(sl + 1);
                auto dt = pn.rfind('.');  if (dt != std::string::npos) pn = pn.substr(0, dt);
                for (auto& c : pn) if (!isalnum((unsigned char)c)) c = '_';
                { fprintf(f, "    %s_veg_%s();\n", pref.c_str(), pn.c_str());
                  lanzados.push_back(pref + "_veg_" + pn); }
            }
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
            /* MALLA EXACTA: la colision es la geometria del modelo, triangulo a
               triangulo. No se mueve (Jolt no admite malla movil), asi que se
               registra el colisionador y el objeto se dibuja como cualquier
               decorado: nada de tamanios que ajustar. */
            if (o.phys == 6 && !o.is_player) {
                fprintf(f, "    // colision EXACTA de '%s' (su propia malla)\n"
                           "    m = %s(\"Assets/%s\");\n"
                           "    IF (m > 0) g3d_collider_add_model(m, %.3f, %.3f, %.3f, %.3f); END\n",
                        o.name.c_str(), loader, ruta_asset(o.asset).c_str(),
                        o.x, o.y, o.z, o.scale);
            }
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
                            loader, ruta_asset(o.asset).c_str(), o.name.c_str());
                }
                continue;
            }

            fprintf(f, "    m = %s(\"Assets/%s\"); e = g3d_model_spawn(scene, m, %.3f, %.3f, %.3f, 0.0, %.3f);",
                    loader, ruta_asset(o.asset).c_str(), o.x, o.y, o.z, o.ry * 57.29578f);
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
                lanzados.push_back(o.name);
            } else {
                // Aqui solo llega un objeto fisico/decorativo si es objetivo de
                // camara o enganche (su plantilla se auto-crea la entidad, asi que
                // NO se le pasa 'e': se lanza nativo). La camara-a-otro-objeto y el
                // enganche a hueso se rehacen en su turno del plan; de momento no se
                // engancha follow_ent a estos (limitacion conocida, sin crash).
                FILE* s = fopen(sp.c_str(), "r");
                if (s) { fclose(s); fprintf(f, "    %s(m);\n", o.name.c_str()); lanzados.push_back(o.name); }
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
        fprintf(f, "    %s_camara(camera);\n", pref.c_str());
        lanzados.push_back(pref + "_camara");
        if (hay_aviso)
            fputs("    escena_aviso();   // el cartelito de las reglas\n", f);

        /* ---- SONIDO: cargar y arrancar ----
           Los ficheros se cargan aqui, una vez, y su GLOBAL la usan las reglas,
           los objetos y las zonas. Los canales empiezan callados (-1). */
        if (!sonidos_usados.empty() || !esc_musica.empty()) {
            fputs("    // ---- sonido ----\n", f);
            for (auto& sn : sonidos_usados)
                fprintf(f, "    %s = sound_load(\"Assets/%s\");\n",   // ruta real, ordenada o no
                        var_sonido(sn).c_str(), ruta_asset(sn).c_str());
            for (int i = 0; i < n_amb; i++)
                fprintf(f, "    amb_ch[%d] = -1;\n", i);
            for (int i = 0; i < (int)zsonidos.size(); i++)
                fprintf(f, "    zamb_ch[%d] = -1;\n", i);
            if (!zsonidos.empty())
                { fprintf(f, "    %s_ambiente();   // ambientes de las zonas pintadas\n", pref.c_str());
                  lanzados.push_back(pref + "_ambiente"); }
            if (!esc_musica.empty()) {
                fprintf(f, "    musica = music_load(\"Assets/%s\");\n", ruta_asset(esc_musica).c_str());
                fputs("    IF (musica > 0)\n", f);
                fprintf(f, "        music_set_volume(%d);\n", esc_mus_vol);
                if (esc_mus_fade > 0.05f)
                    fprintf(f, "        music_fade_in(musica, %d, %d);   // entra en %.1f s\n",
                            esc_mus_loop ? -1 : 0, (int)(esc_mus_fade * 1000.0f), esc_mus_fade);
                else
                    fprintf(f, "        music_play(musica, %d);\n", esc_mus_loop ? -1 : 0);
                fputs("    END\n", f);
            }
        }
        // El HUD va al final del montaje: primero los recursos (map/fpg/fnt) y
        // luego sus procesos, que ya encuentran cargado lo que van a dibujar.
        if (!spr_load.empty() || !spr_launch.empty()) {
            fputs("    // ---- sprites 2D del mundo ----\n", f);
            for (auto& l : spr_load)   fputs(l.c_str(), f);
            for (auto& l : spr_launch) fputs(l.c_str(), f);
        }
        if (!hud_res_load.empty() || !hud_launch.empty()) {
            fputs("    // ---- HUD 2D ----\n", f);
            for (auto& l : hud_res_load) fputs(l.c_str(), f);
            for (auto& l : hud_launch)   fputs(l.c_str(), f);
        }
        // Aqui acaba el montaje. El bucle por frame va en su propio proceso para
        // que main.prg pueda ser tuyo: arrancas el escenario, lanzas el motor y
        // encima escribes lo que quieras.
        fputs("    RETURN;\nEND\n\n", f);
        };

        /* ================= UNA PASADA POR ESCENA =================
           Cada .scene del proyecto se carga, se genera su cuerpo y se apunta lo que
           lanza. Al acabar se vuelve a la que estabas editando. */
        std::vector<std::string> esc_ruta, esc_pref, esc_nombre;
        std::vector<std::vector<std::string>> esc_lanzados;
        {
            std::string volver_a = scene_path;
            std::vector<std::string> rutas;
            { std::error_code ec;
              for (auto& e : fs::directory_iterator(scenes_dir, ec)) {
                  if (!e.is_regular_file(ec)) continue;
                  if (e.path().extension() != ".scene") continue;
                  rutas.push_back(e.path().string());
              }
              std::sort(rutas.begin(), rutas.end()); }
            if (rutas.empty()) rutas.push_back(scene_path);
            std::set<std::string> prefs_usados;
            std::map<std::string, std::string> nombre_visto;   // proceso -> escena donde esta
            for (auto& ruta : rutas) {
                if (ruta != scene_path) load_scene(ruta);
                scene_path = ruta;
                // las rutas de relieve, pintado y zonas son de ESTA escena
                rel_scene  = fs::path(scene_path).lexically_relative(project_dir).string();
                rel_relief = rel_scene + ".terrain";
                rel_paint  = rel_scene + ".paint.png";
                rel_zones  = rel_scene + ".zones";
                std::string base = ident_bgd(fs::path(ruta).stem().string(), "esc");
                std::string p2 = base;
                for (int k = 2; k < 100 && prefs_usados.count(p2); k++) p2 = base + "_" + std::to_string(k);
                prefs_usados.insert(p2);
                pref = p2;
                generar_escena();
                esc_ruta.push_back(ruta);
                esc_pref.push_back(pref);
                esc_nombre.push_back(fs::path(ruta).stem().string());
                esc_lanzados.push_back(lanzados);
                /* Dos escenas no pueden tener un objeto con el mismo nombre: cada
                   uno es un PROCESS, y dos PROCESS iguales no compilan. Se avisa
                   aqui, que es cuando se sabe, y no con un error de bgdc. */
                for (auto& n : lanzados) {
                    auto it = nombre_visto.find(n);
                    if (it != nombre_visto.end() && it->second != esc_nombre.back())
                        console_add("AVISO: '" + n + "' esta en '" + it->second + "' y en '" +
                                    esc_nombre.back() + "'. Los nombres no se pueden repetir\n"
                                    "  entre escenas: renombra uno de los dos o el juego no compilara.\n");
                    else nombre_visto[n] = esc_nombre.back();
                }
            }
            if (scene_path != volver_a) { load_scene(volver_a); scene_path = volver_a; }
        }

        fputs("// Avanza el mundo fisico una vez por frame. Cada objeto (barril, jugador,\n"
              "// arma, camara) es su propio proceso y se coloca solo; aqui solo la fisica.\n"
              "PROCESS escena_motor()\n", f);
        fputs("BEGIN\n", f);
        fputs("    LOOP\n", f);
        fputs("        g3d_rigidbody_step(escena_dt);\n", f);
        // El enganche de armas ya no va aqui: cada arma es su propio proceso que
        // sigue el hueso con sus variables nativas.
        fputs("        FRAME;\n    END\nEND\n\n", f);

        /* ================= GUARDAR PARTIDA =================
           Una partida es UNA tabla que se vuelca a fichero con save(), que es como
           guarda BennuGD2 (admite la variable entera, tabla incluida). Lo que entra
           en esa tabla lo has elegido tu, asi que aqui no se decide nada del
           genero del juego: solo se copian los huecos que hayas marcado. */
        std::vector<const GameVar*> vars_partida;
        if (guardado.con_vars)
            for (auto& v : gvars) if (v.guardar) vars_partida.push_back(&v);
        bool hay_partida = false;
        for (auto& o : objects) for (auto& r : o.reglas) for (auto& a : r.acciones)
            if (a.tipo == 10 || a.tipo == 11) hay_partida = true;
        for (auto& sp : sprites) for (auto& r : sp.reglas) for (auto& a : r.acciones)
            if (a.tipo == 10 || a.tipo == 11) hay_partida = true;
        for (auto& m : menus) for (auto& o : m.opciones) {
            if (o.clase == 2) hay_partida = true;
            for (auto& a : o.acciones) if (a.tipo == 10 || a.tipo == 11) hay_partida = true;
        }
        for (auto& d : dialogos) for (auto& pg : d.paginas) for (auto& o : pg.opciones)
            for (auto& a : o.acciones) if (a.tipo == 10 || a.tipo == 11) hay_partida = true;
        if (hay_partida) {
            /* La tabla: 0 = usada, 1 = escena, 2..4 = donde estaba el jugador,
               5 = hacia donde miraba, luego las variables y luego las reglas. */
            int base_vars = 6;
            int n_reglas = guardado.con_reglas ? max_reglas : 0;
            int total = base_vars + (int)vars_partida.size() + n_reglas;
            add_global("    // la partida guardada: se vuelca entera con save()\n"
                       "    int part[" + std::to_string(total) + "];\n"
                       "    int ranura_hay[" + std::to_string(guardado.ranuras + 1) + "];\n"
                       "    int ranura_esc[" + std::to_string(guardado.ranuras + 1) + "];\n");
            std::string g;
            g  = "// ---- GUARDAR Y CARGAR LA PARTIDA ----\n";
            g += "// Lo que entra aqui lo elegiste en el editor (Ventana > Guardar partida).\n";
            g += "FUNCTION partida_guardar(int ranura)\n"
                 "PRIVATE int i; string f;\nEND\n"
                 "BEGIN\n"
                 "    part[0] = 1;   // esta ranura ya tiene algo\n";
            if (guardado.con_escena)  g += "    part[1] = escena_actual;\n";
            if (guardado.con_jugador) {
                g += "    // donde estaba el jugador (en milesimas, que la tabla es de enteros)\n"
                     "    part[2] = jug_x * 1000;  part[3] = jug_y * 1000;  part[4] = jug_z * 1000;\n";
                if (cam_girable && cam_mode == 1) g += "    part[5] = escena_orbita;\n";
            }
            for (size_t i = 0; i < vars_partida.size(); i++)
                g += "    part[" + std::to_string(base_vars + (int)i) + "] = " + vars_partida[i]->nombre + ";\n";
            if (n_reglas > 0)
                g += "    FOR (i = 0; i < " + std::to_string(n_reglas) + "; i = i + 1)\n"
                     "        part[" + std::to_string(base_vars + (int)vars_partida.size()) + " + i] = regla_hecha[i];\n"
                     "    END\n";
            g += "    f = \"" + guardado.fichero + "\" + ranura + \".sav\";\n"
                 "    save(f, part);\n"
                 "    ranura_hay[ranura] = 1;  ranura_esc[ranura] = part[1];\n"
                 "    RETURN;\nEND\n\n";

            g += "FUNCTION partida_cargar(int ranura)\n"
                 "PRIVATE int i; string f;\nEND\n"
                 "BEGIN\n"
                 "    part[0] = 0;\n"
                 "    f = \"" + guardado.fichero + "\" + ranura + \".sav\";\n"
                 "    load(f, part);\n"
                 "    IF (part[0] == 0) RETURN; END   // ranura vacia: no se toca nada\n";
            for (size_t i = 0; i < vars_partida.size(); i++)
                g += "    " + vars_partida[i]->nombre + " = part[" + std::to_string(base_vars + (int)i) + "];\n";
            if (n_reglas > 0)
                g += "    FOR (i = 0; i < " + std::to_string(n_reglas) + "; i = i + 1)\n"
                     "        regla_hecha[i] = part[" + std::to_string(base_vars + (int)vars_partida.size()) + " + i];\n"
                     "    END\n";
            if (guardado.con_jugador)
                g += "    // al montar la escena, el jugador se pone donde estaba\n"
                     "    volver_x = part[2] / 1000.0;  volver_y = part[3] / 1000.0;  volver_z = part[4] / 1000.0;\n"
                     "    volver_a = part[5];  hay_vuelta = 1;\n";
            if (guardado.con_escena)
                g += "    escena_pedida = part[1];   // y se va a la escena en la que estabas\n";
            g += "    RETURN;\nEND\n\n";

            g += "// Mira que hay en cada ranura, para que un menu pueda ensenarlo.\n"
                 "FUNCTION partida_ojear()\n"
                 "PRIVATE int r; string f;\nEND\n"
                 "BEGIN\n"
                 "    FOR (r = 1; r <= " + std::to_string(guardado.ranuras) + "; r = r + 1)\n"
                 "        part[0] = 0;\n"
                 "        f = \"" + guardado.fichero + "\" + r + \".sav\";\n"
                 "        load(f, part);\n"
                 "        ranura_hay[r] = part[0];  ranura_esc[r] = part[1];\n"
                 "    END\n"
                 "    RETURN;\nEND\n\n";
            add_proc_comun("partida", g);

        }

        /* ================= LOS AJUSTES =================
           Un menu de opciones que no recuerda nada no sirve: los ajustes se
           guardan en un fichero con save() y se cargan al arrancar. Se guarda UNA
           tabla, que es como BennuGD2 guarda de una vez (save admite la variable
           entera, tabla incluida). */
        std::vector<const MenuOpc*> ajustes;
        for (auto& m : menus) for (auto& o : m.opciones) if (o.clase == 1) ajustes.push_back(&o);
        if (!ajustes.empty()) {
            add_global("    // los ajustes del jugador (volumen, pantalla, lo que pongas)\n"
                       "    int cfg[" + std::to_string((int)ajustes.size()) + "];\n"
                       "    int cfg_cargado;\n");
            std::string cargar = "// Los ajustes guardados de la ultima vez.\n"
                                 "FUNCTION opciones_cargar()\n"
                                 "PRIVATE int i;\nEND\n"
                                 "BEGIN\n"
                                 "    // valores de partida por si aun no hay fichero\n";
            for (size_t i = 0; i < ajustes.size(); i++) {
                const MenuOpc* o = ajustes[i];
                int def = (o->ajuste == 0 || o->ajuste == 1) ? 96 : (o->ajuste == 2 ? 0 : o->vmin);
                cargar += "    cfg[" + std::to_string((int)i) + "] = " + std::to_string(def) + ";\n";
            }
            cargar += "    load(\"opciones.cfg\", cfg);   // si no existe, se quedan los de arriba\n"
                      "    opciones_aplicar();\n"
                      "    cfg_cargado = 1;\n"
                      "    RETURN;\nEND\n\n";
            std::string aplicar = "// Pone los ajustes donde tienen efecto.\n"
                                  "FUNCTION opciones_aplicar()\n"
                                  "BEGIN\n";
            for (size_t i = 0; i < ajustes.size(); i++) {
                const MenuOpc* o = ajustes[i];
                std::string c = "cfg[" + std::to_string((int)i) + "]";
                if (o->ajuste == 0)      aplicar += "    music_set_volume(" + c + ");\n";
                else if (o->ajuste == 1) aplicar += "    sound_set_volume(" + c + ");\n";
                else if (o->ajuste == 2)
                    aplicar += "    IF (" + c + ") set_mode(1280, 720, MODE_FULLSCREEN);\n"
                               "    ELSE set_mode(1280, 720, MODE_WINDOW); END\n";
                else if (!o->var.empty())
                    aplicar += "    " + o->var + " = " + c + ";   // ajuste de tu juego\n";
            }
            aplicar += "    RETURN;\nEND\n\n";
            std::string guardar = "// Se guardan al tocarlos, para que la proxima vez esten puestos.\n"
                                  "FUNCTION opciones_guardar()\n"
                                  "BEGIN\n"
                                  "    save(\"opciones.cfg\", cfg);\n"
                                  "    RETURN;\nEND\n\n";
            add_proc_comun("opciones", aplicar + cargar + guardar);
        }

        /* ================= LOS MENUS =================
           Un PROCESS por menu. Los textos se crean una vez con write() y se
           colorean por su id con write_set_rgba: mover la seleccion es cambiar dos
           colores, no rehacer la pantalla. Se maneja con teclado, mando y raton
           (con text_width/text_height para saber donde esta cada opcion). */
        for (auto& m : menus) {
            if (m.opciones.empty()) continue;
            int n = (int)m.opciones.size();
            /* Un menu que congela el juego y no tiene NINGUNA opcion que lo cierre
               deja el juego parado para siempre: no es un fallo del motor, es que
               falta ponerle salida. Mejor decirlo al generar que descubrirlo
               jugando. */
            if (m.pausa) {
                bool tiene_salida = false;
                for (auto& o : m.opciones) {
                    if (o.clase == 2 && o.ranura_modo == 0) tiene_salida = true;   // cargar partida
                    for (auto& a : o.acciones)
                        if (a.tipo == 6 || a.tipo == 7 || a.tipo == 5) tiene_salida = true;
                }
                if (!tiene_salida)
                    console_add("AVISO: el menu '" + m.nombre + "' congela el juego y ninguna de sus\n"
                                "  opciones lo cierra. Ponle una con \"Cerrar el menu\" (o \"Ir a otra\n"
                                "  escena\" / \"Salir del juego\"), o al abrirlo no se podra jugar.\n");
            }
            fprintf(f, "// ===== MENU '%s' =====\n", m.nombre.c_str());
            fprintf(f, "PROCESS %s()\n"
                       "PRIVATE\n"
                       "    int sel; int i; int n; int idop[%d]; int fnt; int aj[%d]; string lin[%d];\n"
                       "    int izq; int der; int ant_izq; int ant_der; int v;\n"
                       "    int ant_arr; int ant_aba; int ant_ok; int arr; int aba; int ok;\n"
                       "    int tw; int th; int mx; int my;\n"
                       "END\n"
                       "BEGIN\n"
                       "    n = %d;  sel = 0;\n",
                    m.nombre.c_str(), n > 0 ? n : 1, n > 0 ? n : 1, n > 0 ? n : 1, n);
            if (!m.fuente.empty())
                fprintf(f, "    fnt = fnt_load(\"Assets/%s\");\n"
                           "    IF (fnt <= 0) fnt = 0; END   // si no carga, la del sistema\n",
                        ruta_asset(m.fuente).c_str());
            else fputs("    fnt = 0;   // la fuente del sistema\n", f);
            if (!m.fondo.empty()) {
                fprintf(f, "    // el fondo del menu\n"
                           "    file = 0;  graph = map_load(\"Assets/%s\");\n"
                           "    x = 640; y = 360; z = -400;\n", ruta_asset(m.fondo).c_str());
            } else {
                fputs("    z = -400;   // por encima del juego\n", f);
            }
            if (m.pausa)
                fputs("    signal(ALL_PROCESS, S_FREEZE);   // el mundo se queda quieto y a la vista\n", f);
            /* Una opcion normal es un texto fijo; un ajuste se pinta con su valor
               al lado y cambia con izquierda/derecha, asi que se escribe con
               write_var sobre una cadena que el propio menu va rehaciendo. */
            for (int i = 0; i < n; i++) {
                const MenuOpc& o = m.opciones[i];
                if (o.clase == 1) {
                    int idx = 0;
                    for (size_t k = 0; k < ajustes.size(); k++) if (ajustes[k] == &o) idx = (int)k;
                    fprintf(f, "    lin[%d] = \"%s\";\n"
                               "    idop[%d] = write_var(fnt, %d, %d, 4, lin[%d]);\n"
                               "    aj[%d] = %d;   // que ajuste es (hueco de cfg)\n",
                            i, o.texto.c_str(), i, m.x, m.y + i * m.sep, i, i, idx);
                } else {
                    fprintf(f, "    aj[%d] = -1;\n"
                               "    idop[%d] = write(fnt, %d, %d, 4, \"%s\");\n",
                            i, i, m.x, m.y + i * m.sep, o.texto.c_str());
                }
            }
            fputs("\n    LOOP\n"
                  "        // ---- moverse por las opciones ----\n", f);
            {
                std::string arr, aba, ok;
                if (m.con_teclado) { arr = "key(_UP)"; aba = "key(_DOWN)"; ok = "key(_ENTER) OR key(_SPACE)"; }
                if (m.con_mando) {
                    auto suma = [](std::string& d, const char* q) { if (!d.empty()) d += " OR "; d += q; };
                    suma(arr, "joy_getbutton(JOY_BUTTON_DPAD_UP) OR joy_getaxis(JOY_AXIS_LEFTY) < -16000");
                    suma(aba, "joy_getbutton(JOY_BUTTON_DPAD_DOWN) OR joy_getaxis(JOY_AXIS_LEFTY) > 16000");
                    suma(ok,  "joy_getbutton(JOY_BUTTON_A)");
                }
                if (arr.empty()) { arr = "0"; aba = "0"; ok = "0"; }
                fprintf(f, "        arr = (%s);\n        aba = (%s);\n        ok  = (%s);\n",
                        arr.c_str(), aba.c_str(), ok.c_str());
            }
            fputs("        // al PULSAR, no mientras se aguanta\n"
                  "        IF (arr AND ant_arr == 0)  sel = sel - 1;  IF (sel < 0) sel = n - 1; END\n", f);
            if (!m.snd_mover.empty())
                fprintf(f, "            sound_play(%s, 0);\n", var_sonido(m.snd_mover).c_str());
            fputs("        END\n"
                  "        IF (aba AND ant_aba == 0)  sel = sel + 1;  IF (sel >= n) sel = 0; END\n", f);
            if (!m.snd_mover.empty())
                fprintf(f, "            sound_play(%s, 0);\n", var_sonido(m.snd_mover).c_str());
            fputs("        END\n"
                  "        ant_arr = arr;  ant_aba = aba;\n", f);
            if (!ajustes.empty()) {
                fputs("        // ---- cambiar el ajuste elegido ----\n"
                      "        izq = (key(_LEFT)  OR joy_getbutton(JOY_BUTTON_DPAD_LEFT));\n"
                      "        der = (key(_RIGHT) OR joy_getbutton(JOY_BUTTON_DPAD_RIGHT));\n"
                      "        IF (aj[sel] >= 0)\n"
                      "            v = 0;\n"
                      "            IF (izq AND ant_izq == 0) v = -1; END\n"
                      "            IF (der AND ant_der == 0) v =  1; END\n"
                      "            IF (v <> 0)\n", f);
                for (int i = 0; i < n; i++) {
                    const MenuOpc& o = m.opciones[i];
                    if (o.clase != 1) continue;
                    int idx = 0;
                    for (size_t k = 0; k < ajustes.size(); k++) if (ajustes[k] == &o) idx = (int)k;
                    fprintf(f, "                IF (sel == %d)\n"
                               "                    cfg[%d] = cfg[%d] + v * %d;\n"
                               "                    IF (cfg[%d] < %d) cfg[%d] = %d; END\n"
                               "                    IF (cfg[%d] > %d) cfg[%d] = %d; END\n"
                               "                END\n",
                            i, idx, idx, o.paso,
                            idx, o.vmin, idx, o.vmin,
                            idx, o.vmax, idx, o.vmax);
                }
                fputs("                opciones_aplicar();\n"
                      "                opciones_guardar();   // que la proxima vez siga puesto\n", f);
                if (!m.snd_mover.empty())
                    fprintf(f, "                sound_play(%s, 0);\n", var_sonido(m.snd_mover).c_str());
                fputs("            END\n"
                      "        END\n"
                      "        ant_izq = izq;  ant_der = der;\n", f);
                // el texto de cada ajuste, con su valor
                for (int i = 0; i < n; i++) {
                    const MenuOpc& o = m.opciones[i];
                    if (o.clase != 1) continue;
                    int idx = 0;
                    for (size_t k = 0; k < ajustes.size(); k++) if (ajustes[k] == &o) idx = (int)k;
                    if (o.ajuste == 2)
                        fprintf(f, "        IF (cfg[%d]) lin[%d] = \"%s:  SI\";\n"
                                   "        ELSE lin[%d] = \"%s:  NO\"; END\n",
                                idx, i, o.texto.c_str(), i, o.texto.c_str());
                    else
                        fprintf(f, "        lin[%d] = \"%s:  \" + cfg[%d];\n",
                                i, o.texto.c_str(), idx);
                }
            }
            if (m.con_raton) {
                fprintf(f,
                  "        // ---- el raton: la opcion que este debajo ----\n"
                  "        mx = mouse.x;  my = mouse.y;\n"
                  "        FOR (i = 0; i < n; i = i + 1)\n"
                  "            th = text_height(fnt, \"Ay\");\n"
                  "            IF (my > %d + i * %d - th / 2 AND my < %d + i * %d + th / 2)\n"
                  "                sel = i;\n"
                  "                IF (mouse.left) ok = 1; END\n"
                  "            END\n"
                  "        END\n", m.y, m.sep, m.y, m.sep);
            }
            // el texto de cada ranura, con lo que hay dentro
            for (int i = 0; i < n; i++) {
                const MenuOpc& o = m.opciones[i];
                if (o.clase != 2) continue;
                fprintf(f, "        IF (ranura_hay[%d]) lin[%d] = \"%s:  escena \" + ranura_esc[%d];\n"
                           "        ELSE lin[%d] = \"%s:  vacia\"; END\n",
                        o.ranura, i, o.texto.c_str(), o.ranura, i, o.texto.c_str());
            }
            fprintf(f,
                  "        // ---- pintar: la elegida de otro color ----\n"
                  "        FOR (i = 0; i < n; i = i + 1)\n"
                  "            IF (i == sel) write_set_rgba(idop[i], %d, %d, %d, %d);\n"
                  "            ELSE          write_set_rgba(idop[i], %d, %d, %d, %d); END\n"
                  "        END\n",
                  m.col_sel[0], m.col_sel[1], m.col_sel[2], m.col_sel[3],
                  m.col[0], m.col[1], m.col[2], m.col[3]);
            fputs("        // ---- elegir ----\n"
                  "        IF (ok AND ant_ok == 0)\n", f);
            if (!m.snd_elegir.empty())
                fprintf(f, "            sound_play(%s, 0);\n", var_sonido(m.snd_elegir).c_str());
            for (int i = 0; i < n; i++) {
                fprintf(f, "            IF (sel == %d)\n", i);
                if (m.opciones[i].clase == 2) {
                    const MenuOpc& o = m.opciones[i];
                    if (o.ranura_modo == 1)
                        fprintf(f, "                partida_guardar(%d);\n"
                                   "                partida_ojear();\n", o.ranura);
                    else
                        fprintf(f, "                IF (ranura_hay[%d])\n"
                                   "                    partida_cargar(%d);\n"
                                   "                    FOR (i = 0; i < n; i = i + 1)  write_delete(idop[i]);  END\n"
                                   "                    %s\n"
                                   "                    RETURN;\n"
                                   "                END\n",
                                o.ranura, o.ranura,
                                m.pausa ? "signal(ALL_PROCESS, S_WAKEUP);" : "");
                }
                for (auto& a : m.opciones[i].acciones) {
                    if (a.tipo == 0 && !a.proc.empty())
                        fprintf(f, "                %s();\n", a.proc.c_str());
                    else if (a.tipo == 1 && !a.var.empty()) {
                        int v = (int)(a.valor >= 0.0f ? a.valor + 0.5f : a.valor - 0.5f);
                        if (a.op == 0)      fprintf(f, "                %s = %d;\n", a.var.c_str(), v);
                        else if (a.op == 1) fprintf(f, "                %s = %s + %d;\n", a.var.c_str(), a.var.c_str(), v);
                        else                fprintf(f, "                %s = %s - %d;\n", a.var.c_str(), a.var.c_str(), v);
                    }
                    else if (a.tipo == 3) {
                        std::string t = a.texto;
                        for (auto& c : t) if (c == '"') c = '\'';
                        fprintf(f, "                aviso_txt = \"%s\"; aviso_t = %.2f;\n", t.c_str(), a.seg);
                    }
                    else if (a.tipo == 4 && !a.sonido.empty())
                        fprintf(f, "                sound_play(%s, 0);\n", var_sonido(a.sonido).c_str());
                    else if (a.tipo == 5) {
                        int ne = indice_escena(a.escena);
                        if (ne >= 0) fprintf(f, "                escena_pedida = %d;   // ir a %s\n", ne, a.escena.c_str());
                    }
                    else if (a.tipo == 8 && !a.menu.empty())
                        fprintf(f, "                IF (NOT exists(TYPE %s)) %s(); END\n",
                                a.menu.c_str(), a.menu.c_str());
                    else if (a.tipo == 9 && !a.dialogo.empty())
                        fprintf(f, "                IF (NOT exists(TYPE %s)) %s(); END\n",
                                a.dialogo.c_str(), a.dialogo.c_str());
                    else if (a.tipo == 10)
                        fprintf(f, "                partida_guardar(%d);\n", a.ranura);
                    else if (a.tipo == 11)
                        fprintf(f, "                partida_cargar(%d);\n", a.ranura);
                    else if (a.tipo == 6 || a.tipo == 7) {
                        // cerrar o salir: los dos deshacen el menu primero
                        fputs("                FOR (i = 0; i < n; i = i + 1)  write_delete(idop[i]);  END\n", f);
                        if (m.pausa) fputs("                signal(ALL_PROCESS, S_WAKEUP);\n", f);
                        if (a.tipo == 7) fputs("                exit();\n", f);
                        else             fputs("                RETURN;\n", f);
                    }
                }
                fputs("            END\n", f);
            }
            fputs("        END\n"
                  "        ant_ok = ok;\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n", f);
        }

        // Los menus que se abren con una tecla llevan su vigilante: mira la tecla y
        // los saca, sin abrir dos veces el mismo.
        for (auto& m : menus) {
            if (m.cuando != 1 || m.opciones.empty()) continue;
            std::string puls;
            if (!m.tecla.empty()) puls = "key(" + m.tecla + ")";
            if (!m.boton.empty() && m.con_mando) {
                if (!puls.empty()) puls += " OR ";
                puls += "joy_getbutton(" + m.boton + ")";
            }
            if (puls.empty()) continue;
            fprintf(f, "// Saca el menu '%s' cuando lo pides.\n"
                       "PROCESS %s_vigila()\n"
                       "PRIVATE int ant;\nEND\n"
                       "BEGIN\n"
                       "    LOOP\n"
                       "        IF ((%s) AND ant == 0 AND NOT exists(TYPE %s))  %s();  END\n"
                       "        ant = (%s);\n"
                       "        FRAME;\n"
                       "    END\n"
                       "END\n\n",
                    m.nombre.c_str(), m.nombre.c_str(), puls.c_str(),
                    m.nombre.c_str(), m.nombre.c_str(), puls.c_str());
        }

        /* ================= LOS DIALOGOS =================
           Un PROCESS por dialogo. El bocadillo es TU grafico (un PNG o un grafico
           de un FPG) estirado con size_x/size_y al tamanio que le diste; el texto
           va escrito encima con write(). Las lineas se parten AQUI, midiendolas
           con la misma fuente que usara el juego, porque write() no parte solo. */
        {   // ¿algun dialogo usa retratos? entonces hace falta quien los dibuje
            bool hay_retrato = false;
            for (auto& d : dialogos) for (auto& p2 : d.paginas) if (!p2.retrato.empty()) hay_retrato = true;
            if (hay_retrato)
                add_proc_comun("retrato",
                    "// La cara de quien habla. Es su propio proceso porque el del\n"
                    "// dialogo ya gasta su grafico en el bocadillo.\n"
                    "PROCESS escena_retrato(int arch, int graf, int px, int py)\n"
                    "BEGIN\n"
                    "    file = arch; graph = graf;\n"
                    "    x = px; y = py; z = -310;   // por delante del bocadillo\n"
                    "    LOOP FRAME; END\n"
                    "END\n\n");
        }
        for (auto& d : dialogos) {
            if (d.paginas.empty()) continue;
            int npag = (int)d.paginas.size();
            /* cuantas lineas cabe cada pagina y como se parte su texto */
            int ancho_util = d.cw - 2 * d.mx;
            if (ancho_util < 40) ancho_util = 40;
            H2Font* fnt = d.fuente.empty() ? nullptr : hud_font(d.fuente);
            auto ancho_de = [&](const std::string& t) {
                if (fnt) return h2_text_width(fnt, t.c_str());
                return (int)t.size() * 8;   // la del sistema es de 8x8: mejor pasarse
            };
            std::vector<std::vector<std::string>> lineas(npag);
            int maxlin = 1;
            for (int p2 = 0; p2 < npag; p2++) {
                std::string resto = d.paginas[p2].texto, linea;
                std::string palabra;
                auto empujar = [&]() {
                    if (!linea.empty()) { lineas[p2].push_back(linea); linea.clear(); }
                };
                std::istringstream ss(resto);
                while (ss >> palabra) {
                    std::string prueba = linea.empty() ? palabra : linea + " " + palabra;
                    if (ancho_de(prueba) > ancho_util && !linea.empty()) empujar();
                    linea = linea.empty() ? palabra : linea + " " + palabra;
                }
                empujar();
                if (lineas[p2].empty()) lineas[p2].push_back("");
                if ((int)lineas[p2].size() > maxlin) maxlin = (int)lineas[p2].size();
            }
            int alto_linea = fnt ? (fnt->maxheight + 4) : 14;
            int maxopc = 1;
            for (auto& p2 : d.paginas) if ((int)p2.opciones.size() > maxopc) maxopc = (int)p2.opciones.size();

            fprintf(f, "// ===== DIALOGO '%s' =====\n", d.nombre.c_str());
            fprintf(f, "PROCESS %s()\n"
                       "PRIVATE\n"
                       "    int pag; int i; int fnt; int idl[%d]; int idnom; int idop[%d];\n"
                       "    int sel; int nop; int ok; int ant_ok; int arr; int aba; int ant_arr; int ant_aba;\n"
                       "    float letras; int total; float t; string linea[%d]; string nombre;\n"
                       "    int retrato; int hoja; int cara;\n"
                       "END\n"
                       "BEGIN\n",
                    d.nombre.c_str(), maxlin, maxopc, maxlin);
            if (!d.fuente.empty())
                fprintf(f, "    fnt = fnt_load(\"Assets/%s\");   IF (fnt <= 0) fnt = 0; END\n", ruta_asset(d.fuente).c_str());
            else fputs("    fnt = 0;   // la fuente del sistema\n", f);
            fputs("    z = -300;   // el bocadillo, por encima del juego\n", f);
            if (!d.caja.empty()) {
                bool es_fpg = d.caja.size() > 4 &&
                              (d.caja.substr(d.caja.size()-4) == ".fpg" ||
                               d.caja.substr(d.caja.size()-4) == ".f16" ||
                               d.caja.substr(d.caja.size()-4) == ".f32");
                if (es_fpg)
                    fprintf(f, "    // el bocadillo: grafico %d del FPG\n"
                               "    file = fpg_load(\"Assets/%s\");  graph = %d;\n",
                            d.caja_graf, ruta_asset(d.caja).c_str(), d.caja_graf);
                else
                    fprintf(f, "    // el bocadillo: una imagen suelta\n"
                               "    file = 0;  graph = map_load(\"Assets/%s\");\n", ruta_asset(d.caja).c_str());
                fprintf(f, "    x = %d; y = %d;\n"
                           "    // se estira al tamanio que le diste en el editor\n"
                           "    IF (graphic_info(file, graph, G_WIDTH) > 0)\n"
                           "        size_x = 100 * %d / graphic_info(file, graph, G_WIDTH);\n"
                           "        size_y = 100 * %d / graphic_info(file, graph, G_HEIGHT);\n"
                           "    END\n", d.cx, d.cy, d.cw, d.ch);
            }
            if (d.pausa)
                fputs("    signal(ALL_PROCESS, S_FREEZE);   // el mundo se para mientras se habla\n", f);
            fputs("    pag = 0;\n"
                  "    LOOP\n", f);
            for (int p2 = 0; p2 < npag; p2++) {
                DlgPag& pg = d.paginas[p2];
                fprintf(f, "        IF (pag == %d)\n", p2);
                // el retrato, si lo hay
                if (!pg.retrato.empty()) {
                    bool rf = pg.retrato.size() > 4 &&
                              (pg.retrato.substr(pg.retrato.size()-4) == ".fpg" ||
                               pg.retrato.substr(pg.retrato.size()-4) == ".f16" ||
                               pg.retrato.substr(pg.retrato.size()-4) == ".f32");
                    // a la izquierda del bocadillo, a su altura
                    int rx = d.cx - d.cw / 2 - 70, ry = d.cy;
                    if (rx < 70) rx = 70;
                    SheetDef* hj = rf ? nullptr : sheet_of(pg.retrato);
                    bool una_cara = (!rf && hj && pg.retrato_cara >= 0 &&
                                     pg.retrato_cara < (int)hj->frames.size());
                    if (rf) {
                        fprintf(f, "            retrato = escena_retrato(fpg_load(\"Assets/%s\"), %d, %d, %d);\n",
                                ruta_asset(pg.retrato).c_str(), pg.retrato_graf, rx, ry);
                    } else if (una_cara) {
                        /* La hoja trae varias caras: se recorta LA DE ESTA PAGINA a
                           un grafico propio (map_new + map_block_copy) y se ensenia
                           ese. Se hace al sacar la pagina, una vez. */
                        SprFrame& fr = hj->frames[pg.retrato_cara];
                        fprintf(f, "            hoja = map_load(\"Assets/%s\");\n"
                                   "            cara = map_new(%d, %d);\n"
                                   "            map_block_copy(0, cara, 0, 0, 0, hoja, %d, %d, %d, %d, 0);\n"
                                   "            retrato = escena_retrato(0, cara, %d, %d);   // cara %d de la hoja\n",
                                ruta_asset(pg.retrato).c_str(), fr.w, fr.h,
                                fr.x, fr.y, fr.w, fr.h, rx, ry, pg.retrato_cara);
                    } else {
                        fprintf(f, "            retrato = escena_retrato(0, map_load(\"Assets/%s\"), %d, %d);\n",
                                ruta_asset(pg.retrato).c_str(), rx, ry);
                    }
                }
                if (!pg.quien.empty()) {
                    std::string q = pg.quien; for (auto& c : q) if (c == '"') c = '\'';
                    fprintf(f, "            nombre = \"%s\";\n"
                               "            idnom = write(fnt, %d, %d, 0, nombre);\n"
                               "            write_set_rgba(idnom, %d, %d, %d, %d);\n",
                            q.c_str(), d.cx - d.cw / 2 + d.mx, d.cy - d.ch / 2 + d.my - 2,
                            d.col_nombre[0], d.col_nombre[1], d.col_nombre[2], d.col_nombre[3]);
                }
                // el texto, linea a linea (ya partido arriba)
                int y0 = d.cy - d.ch / 2 + d.my + (pg.quien.empty() ? 0 : alto_linea + 4);
                for (int L = 0; L < (int)lineas[p2].size(); L++) {
                    std::string t = lineas[p2][L]; for (auto& c : t) if (c == '"') c = '\'';
                    if (d.vel > 0)
                        fprintf(f, "            linea[%d] = \"\";\n", L);
                    else
                        fprintf(f, "            linea[%d] = \"%s\";\n", L, t.c_str());
                    fprintf(f, "            idl[%d] = write_var(fnt, %d, %d, 0, linea[%d]);\n"
                               "            write_set_rgba(idl[%d], %d, %d, %d, %d);\n",
                            L, d.cx - d.cw / 2 + d.mx, y0 + L * alto_linea, L,
                            L, d.col[0], d.col[1], d.col[2], d.col[3]);
                }
                if (d.vel > 0) {
                    /* Efecto maquina de escribir: el texto se va copiando letra a
                       letra con substr(). Al pulsar, sale entero de golpe. */
                    int total = 0;
                    for (auto& l : lineas[p2]) total += (int)l.size();
                    fprintf(f, "            letras = 0;  total = %d;\n"
                               "            LOOP\n"
                               "                letras = letras + %d * escena_dt;\n", total, d.vel);
                    int acum = 0;
                    for (int L = 0; L < (int)lineas[p2].size(); L++) {
                        std::string t = lineas[p2][L]; for (auto& c : t) if (c == '"') c = '\'';
                        fprintf(f, "                IF (letras > %d) linea[%d] = substr(\"%s\", 0, letras - %d);\n"
                                   "                ELSE linea[%d] = \"\"; END\n",
                                acum, L, t.c_str(), acum, L);
                        acum += (int)lineas[p2][L].size();
                    }
                    if (!d.snd_letra.empty())
                        fprintf(f, "                IF (letras < total) sound_play(%s, 0); END\n",
                                var_sonido(d.snd_letra).c_str());
                    fprintf(f, "                ok = (key(%s)", d.tecla.c_str());
                    if (!d.boton.empty()) fprintf(f, " OR joy_getbutton(%s)", d.boton.c_str());
                    fputs(" OR mouse.left);\n"
                          "                IF (letras >= total) BREAK; END\n"
                          "                IF (ok AND ant_ok == 0) letras = total; END\n"
                          "                ant_ok = ok;\n"
                          "                FRAME;\n"
                          "            END\n", f);
                    for (int L = 0; L < (int)lineas[p2].size(); L++) {
                        std::string t = lineas[p2][L]; for (auto& c : t) if (c == '"') c = '\'';
                        fprintf(f, "            linea[%d] = \"%s\";\n", L, t.c_str());
                    }
                }
                // ---- esperar: pasar de pagina, o elegir respuesta ----
                if (pg.opciones.empty()) {
                    fprintf(f, "            LOOP\n"
                               "                ok = (key(%s)", d.tecla.c_str());
                    if (!d.boton.empty()) fprintf(f, " OR joy_getbutton(%s)", d.boton.c_str());
                    fputs(" OR mouse.left);\n"
                          "                IF (ok AND ant_ok == 0) ant_ok = ok; BREAK; END\n"
                          "                ant_ok = ok;\n"
                          "                FRAME;\n"
                          "            END\n", f);
                    if (!d.snd_pasar.empty())
                        fprintf(f, "            sound_play(%s, 0);\n", var_sonido(d.snd_pasar).c_str());
                } else {
                    int nop = (int)pg.opciones.size();
                    fprintf(f, "            nop = %d;  sel = 0;\n", nop);
                    for (int q = 0; q < nop; q++) {
                        std::string t = pg.opciones[q].texto; for (auto& c : t) if (c == '"') c = '\'';
                        /* Las respuestas van DENTRO de la caja, debajo del texto:
                           puestas fuera se salian de la pantalla con el bocadillo
                           abajo del todo, que es donde suele ir. */
                        fprintf(f, "            idop[%d] = write(fnt, %d, %d, 0, \"%s\");\n",
                                q, d.cx - d.cw / 2 + d.mx + 20,
                                y0 + (int)lineas[p2].size() * alto_linea + 6 + q * alto_linea,
                                t.c_str());
                    }
                    fprintf(f, "            LOOP\n"
                               "                arr = (key(_UP) OR joy_getbutton(JOY_BUTTON_DPAD_UP));\n"
                               "                aba = (key(_DOWN) OR joy_getbutton(JOY_BUTTON_DPAD_DOWN));\n"
                               "                IF (arr AND ant_arr == 0) sel = sel - 1; IF (sel < 0) sel = nop - 1; END END\n"
                               "                IF (aba AND ant_aba == 0) sel = sel + 1; IF (sel >= nop) sel = 0; END END\n"
                               "                ant_arr = arr;  ant_aba = aba;\n"
                               "                FOR (i = 0; i < nop; i = i + 1)\n"
                               "                    IF (i == sel) write_set_rgba(idop[i], %d, %d, %d, %d);\n"
                               "                    ELSE          write_set_rgba(idop[i], %d, %d, %d, %d); END\n"
                               "                END\n"
                               "                ok = (key(%s)",
                            d.col_nombre[0], d.col_nombre[1], d.col_nombre[2], d.col_nombre[3],
                            d.col[0], d.col[1], d.col[2], d.col[3], d.tecla.c_str());
                    if (!d.boton.empty()) fprintf(f, " OR joy_getbutton(%s)", d.boton.c_str());
                    fputs(" OR mouse.left);\n"
                          "                IF (ok AND ant_ok == 0) ant_ok = ok; BREAK; END\n"
                          "                ant_ok = ok;\n"
                          "                FRAME;\n"
                          "            END\n"
                          "            FOR (i = 0; i < nop; i = i + 1) write_delete(idop[i]); END\n", f);
                }
                // ---- limpiar lo escrito ----
                for (int L = 0; L < (int)lineas[p2].size(); L++)
                    fprintf(f, "            write_delete(idl[%d]);\n", L);
                if (!pg.quien.empty()) fputs("            write_delete(idnom);\n", f);
                if (!pg.retrato.empty()) fputs("            signal(retrato, S_KILL);   // se va la cara con la pagina\n", f);
                // ---- a donde se va ----
                if (pg.opciones.empty()) {
                    if (p2 + 1 < npag) fprintf(f, "            pag = %d;\n", p2 + 1);
                    else               fputs("            BREAK;\n", f);
                } else {
                    for (int q = 0; q < (int)pg.opciones.size(); q++) {
                        fprintf(f, "            IF (sel == %d)\n", q);
                        for (auto& a : pg.opciones[q].acciones) {
                            if (a.tipo == 0 && !a.proc.empty())
                                fprintf(f, "                %s();\n", a.proc.c_str());
                            else if (a.tipo == 1 && !a.var.empty()) {
                                int v = (int)(a.valor >= 0.0f ? a.valor + 0.5f : a.valor - 0.5f);
                                if (a.op == 0)      fprintf(f, "                %s = %d;\n", a.var.c_str(), v);
                                else if (a.op == 1) fprintf(f, "                %s = %s + %d;\n", a.var.c_str(), a.var.c_str(), v);
                                else                fprintf(f, "                %s = %s - %d;\n", a.var.c_str(), a.var.c_str(), v);
                            } else if (a.tipo == 4 && !a.sonido.empty())
                                fprintf(f, "                sound_play(%s, 0);\n", var_sonido(a.sonido).c_str());
                            else if (a.tipo == 5) {
                                int ne = indice_escena(a.escena);
                                if (ne >= 0) fprintf(f, "                escena_pedida = %d;\n", ne);
                            } else if (a.tipo == 8 && !a.menu.empty())
                                fprintf(f, "                IF (NOT exists(TYPE %s)) %s(); END\n",
                                        a.menu.c_str(), a.menu.c_str());
                            else if (a.tipo == 9 && !a.dialogo.empty())
                                // un dialogo puede llevar a otro: se cierra este y arranca el otro
                                fprintf(f, "                IF (NOT exists(TYPE %s)) %s(); END\n",
                                        a.dialogo.c_str(), a.dialogo.c_str());
                            else if (a.tipo == 10)
                                fprintf(f, "                partida_guardar(%d);\n", a.ranura);
                            else if (a.tipo == 11)
                                fprintf(f, "                partida_cargar(%d);\n", a.ranura);
                            else if (a.tipo == 3) {
                                std::string t = a.texto;
                                for (auto& c2 : t) if (c2 == '"') c2 = '\'';
                                fprintf(f, "                aviso_txt = \"%s\"; aviso_t = %.2f;\n",
                                        t.c_str(), a.seg);
                            }
                            else if (a.tipo == 7) {
                                /* Salir del juego desde una respuesta. Faltaba: la
                                   opcion estaba en la lista pero aqui no se
                                   generaba nada, asi que elegirla no hacia nada. */
                                fputs("                exit();\n", f);
                            }
                            else if (a.tipo == 6) {
                                // cerrar el dialogo (lo mismo que dejarlo en "cerrar")
                                for (int L2 = 0; L2 < (int)lineas[p2].size(); L2++)
                                    fprintf(f, "                write_delete(idl[%d]);\n", L2);
                                if (!pg.quien.empty()) fputs("                write_delete(idnom);\n", f);
                                fputs("                FOR (i = 0; i < nop; i = i + 1) write_delete(idop[i]); END\n", f);
                                if (d.pausa) fputs("                signal(ALL_PROCESS, S_WAKEUP);\n", f);
                                fputs("                RETURN;\n", f);
                            }
                        }
                        if (pg.opciones[q].salto >= 0 && pg.opciones[q].salto < npag)
                            fprintf(f, "                pag = %d;\n", pg.opciones[q].salto);
                        else
                            fputs("                BREAK;\n", f);
                        fputs("            END\n", f);
                    }
                }
                fputs("        END\n", f);
            }
            fputs("        FRAME;\n"
                  "    END\n", f);
            if (d.pausa) fputs("    signal(ALL_PROCESS, S_WAKEUP);\n", f);
            fputs("END\n\n", f);
        }

        /* ================= CAMBIAR DE ESCENA =================
           Desmontar es matar los procesos de la escena que estaba puesta (por su
           nombre, uno a uno: let_me_alone() se llevaria por delante tu main) y
           soltar lo del motor -- entidades, cuerpos, muros, vegetacion, zonas,
           manantiales y personajes. Luego se monta la nueva. */
        {
            fputs("// Quita de en medio la escena que estuviera puesta.\n"
                  "FUNCTION escena_descargar()\n"
                  "BEGIN\n", f);
            for (size_t i = 0; i < esc_pref.size(); i++) {
                fprintf(f, "    IF (escena_actual == %d)\n", (int)i);
                for (auto& n : esc_lanzados[i])
                    fprintf(f, "        signal(TYPE %s, S_KILL);\n", n.c_str());
                fputs("    END\n", f);
            }
            fputs("    // y lo que guarda el motor de esa escena\n"
                  "    g3d_rigidbody_clear();\n"
                  "    g3d_collider_clear();\n"
                  "    g3d_char_clear_all();\n"
                  "    g3d_scatter_clear();\n"
                  "    g3d_zone_clear();\n"
                  "    g3d_watersim_clear_sources();\n"
                  "    music_stop();\n"
                  "    RETURN;\nEND\n\n", f);

            fprintf(f, "// Monta la escena n (0..%d).\n"
                       "FUNCTION escena_cargar(int n)\n"
                       "PRIVATE int i;\nEND\n"
                       "BEGIN\n"
                       "    escena_descargar();\n", (int)esc_pref.size() - 1);
            if (max_reglas > 0)
                fprintf(f,
                    "    // las reglas de la escena nueva empiezan de cero\n"
                    "    FOR (i = 0; i <= %d; i = i + 1)\n"
                    "        regla_hecha[i] = 0; regla_ant[i] = 0; regla_t[i] = 0.0;\n"
                    "    END\n", max_reglas);
            for (size_t i = 0; i < esc_pref.size(); i++)
                fprintf(f, "    IF (n == %d) %s_montar(); END\n", (int)i, esc_pref[i].c_str());
            fputs("    escena_actual = n;\n", f);
            if (!ajustes.empty())
                fputs("    /* Montar una escena hace set_mode(1280,720), o sea EN VENTANA, y\n"
                      "       eso deshacia tu pantalla completa cada vez que se cargaba una.\n"
                      "       Por eso se vuelven a aplicar aqui, con la escena ya puesta. */\n"
                      "    opciones_aplicar();\n", f);
            fputs("    RETURN;\nEND\n\n", f);

            fputs("/* Vigila las peticiones de cambio de escena. No se cambia en el sitio\n"
                  "   donde se pide (una regla de un objeto) porque ese objeto es de la\n"
                  "   escena que se va: se moriria a media faena. */\n"
                  "PROCESS escena_gestor()\n"
                  "BEGIN\n"
                  "    LOOP\n"
                  "        IF (escena_pedida >= 0)\n"
                  "            escena_cargar(escena_pedida);\n"
                  "            escena_pedida = -1;\n"
                  "        END\n"
                  "        FRAME;\n"
                  "    END\n"
                  "END\n\n", f);

            /* escena_iniciar() y escena_motor() son los dos nombres que llama TU
               main.prg desde siempre: se quedan como estan y por dentro arrancan
               la escena inicial del proyecto. */
            int inicial = 0;
            for (size_t i = 0; i < esc_ruta.size(); i++) {
                std::string rel = fs::path(esc_ruta[i]).lexically_relative(project_dir).string();
                if (rel == escena_inicial || esc_ruta[i] == escena_inicial) inicial = (int)i;
            }
            fputs("// Arranca el juego por su escena inicial.\n"
                  "FUNCTION escena_iniciar()\n"
                  "BEGIN\n"
                  "    escena_actual = -1; escena_pedida = -1;\n"
                  "    /* El mando hay que ELEGIRLO: BennuGD2 arranca sin ninguno puesto\n"
                  "       (-1) y hasta que no se elige, joy_getbutton() y joy_getaxis()\n"
                  "       devuelven 0 siempre -- el mando parecia no funcionar. */\n"
                  "    IF (joy_numjoysticks() > 0) joy_select(0); END\n", f);
            if (!ajustes.empty())
                fputs("    opciones_cargar();   // volumen, pantalla y demas, como los dejaste\n", f);
            if (hay_aviso_juego) fputs("    escena_aviso();   // el cartelito de las reglas\n", f);
            fprintf(f, "    escena_cargar(%d);\n"
                       "    escena_gestor();   // atiende los cambios de escena\n", inicial);
            for (auto& m : menus) {
                if (m.opciones.empty()) continue;
                if (m.cuando == 0) fprintf(f, "    %s();   // menu de arranque\n", m.nombre.c_str());
                if (m.cuando == 1) fprintf(f, "    %s_vigila();   // menu con tecla\n", m.nombre.c_str());
            }
            fputs("    RETURN;\nEND\n", f);
        }
        fclose(f);
        std::string cuerpo(setup_buf ? setup_buf : "", setup_sz);
        free(setup_buf);

        /* ---- se monta el fichero: primero lo del juego, luego las escenas ----
           En BennuGD2 un GLOBAL tiene que estar declarado por encima del codigo que
           lo usa, asi que las declaraciones recogidas van las primeras, detras los
           procesos comunes, y al final el cuerpo de cada escena. */
        std::string setup = "GLOBAL\n";
        for (auto& l : glob_lin) setup += l + "\n";
        setup += "    int escena_actual; int escena_pedida;   // que escena esta puesta / cual se pide\n";
        if (max_reglas > 0) {
            std::string n = std::to_string(max_reglas);
            setup += "    // el estado de las reglas (el mayor numero de reglas de una escena)\n";
            setup += "    int regla_hecha[" + n + "]; int regla_ant[" + n + "];\n";
            setup += "    float regla_t[" + n + "];\n";
        }
        if (max_amb > 0)
            setup += "    int amb_ch[" + std::to_string(max_amb) + "];   // canal del sonido de cada objeto (-1 = callado)\n";
        if (max_zamb > 0)
            setup += "    int zamb_ch[" + std::to_string(max_zamb) + "];  // canal del ambiente de cada zona\n";
        setup += "END\n\n";
        // el codigo tuyo y el de cada objeto, una sola vez aunque lo pidan dos escenas
        for (auto& in : includes) setup += in;
        if (!includes.empty()) setup += "\n";
        for (auto& pc : proc_comun) setup += pc;
        setup += cuerpo;

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
    /* Ventanita con lo que vera el jugador. Colocar la camara mirando un frustum
       de alambre es adivinar: lo que importa es el encuadre, y eso solo se ve
       renderizando de verdad desde ella. Cuesta un segundo pase, asi que viene
       apagada y se enciende cuando se esta colocando la camara. */
    ViewportFBO camFbo;
    bool show_cam_view = false;
    // Donde quedo la camara del juego este frame, para el pase de render.
    float camview_p[3] = { 0, 0, 0 }, camview_t[3] = { 0, 0, 1 };
    bool  camview_ok = false;
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
                /* Un cuerpo FIJO (masa 0) corta el paso en el juego porque el
                   personaje consulta a Jolt, y el editor no lleva Jolt dentro: aqui
                   se le pone ademas una caja, para que la vista previa se porte
                   igual y no parezca que el personaje lo atraviesa. */
                if (o.mass <= 0.0f)
                    g3d_collider_add_box(o.x - c, o.y, o.z - c, o.x + c, o.y + 2.0f*c, o.z + c);
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
            /* Mismos controles que el juego generado: relativos a la CAMARA, para
               que probar dentro del editor se parezca a jugar. Con la camara a la
               espalda (angulo 0) sale exactamente lo de siempre. */
            float wx=0,wz=0;
            if (!io.WantTextInput){
                float adel=0, lat=0;
                if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow))    adel+=1.0f;
                if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow))  adel-=1.0f;
                if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) lat +=1.0f;
                if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))  lat -=1.0f;
                if (cam_mode == 1 && cam_25d) adel = 0.0f;   // 2.5D: sin profundidad
                if (cam_mode == 1) {
                    /* Igual que el juego: la derecha es la de la PANTALLA. */
                    float ob = cam_orbit * 0.0174533f;
                    wx = adel * -sinf(ob) + lat * -cosf(ob);
                    wz = adel *  cosf(ob) + lat * -sinf(ob);
                } else {
                    wx = lat; wz = adel;   // fija y cenital: ejes del mundo, como estaban
                }
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
            float ob = cam_orbit * 0.0174533f;
            g3d_camera_set_position(cam, tx + sinf(ob)*gcam_dist, ty+cam_height,
                                         tz - cosf(ob)*gcam_dist);
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
                undo_stack.push_back(last_state); undo_kind.push_back('o');
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
                if (ImGui::MenuItem(ICON_FA_FOLDER_TREE " Ordenar los Assets")) pedir_ordenar = true;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Reparte lo que este suelto en Assets/ por carpetas:\n"
                                      "Models, Textures, Sprites, Fonts, Music y Sounds.\n"
                                      "Las escenas siguen funcionando: el editor busca\n"
                                      "cada asset por su nombre, este donde este.");
                ImGui::Separator();
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
            if (ImGui::BeginMenu("Ventana")) {
                ImGui::TextDisabled("MODO DE TRABAJO");
                const char* nm[] = { "Escena", "Terreno", "Personajes", "Interfaz", "Codigo" };
                for (int i = 0; i < M_NUM; i++)
                    if (ImGui::MenuItem(nm[i], nullptr, modo == i)) modo = i;
                ImGui::Separator();
                if (ImGui::MenuItem("Restablecer la disposicion")) rehacer_layout = true;
                ImGui::Separator();
                ImGui::TextDisabled("PANELES");
                /* Marcar la casilla no basta: si el panel ya estaba abierto pero
                   detras de otra pestania, no se ve nada y parece que no responde. */
                if (ImGui::MenuItem(ICON_FA_LAYER_GROUP "  Escenas del proyecto", nullptr, &show_escenas))
                    { if (show_escenas) enfocar_panel = "Escenas del proyecto"; }
                if (ImGui::MenuItem(ICON_FA_BARS "  Menus del juego", nullptr, &show_menus))
                    { if (show_menus) enfocar_panel = "Menus del juego"; }
                if (ImGui::MenuItem(ICON_FA_COMMENT "  Dialogos", nullptr, &show_dialogos))
                    { if (show_dialogos) enfocar_panel = "Dialogos del juego"; }
                if (ImGui::MenuItem(ICON_FA_PERSON_RUNNING "  Sprites 3D", nullptr, &show_spr_win))
                    { if (show_spr_win) enfocar_panel = ICON_FA_PERSON_RUNNING "  Sprites 3D"; }
                if (ImGui::MenuItem(ICON_FA_SLIDERS "  Variables del juego", nullptr, &show_gvars))
                    { if (show_gvars) enfocar_panel = "Variables del juego"; }
                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Guardar partida", nullptr, &show_guardado))
                    { if (show_guardado) enfocar_panel = "Guardar partida"; }
                ImGui::Separator();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Deja los paneles de este modo como venian de fabrica.");
                ImGui::TextDisabled("Cualquier panel se puede arrancar");
                ImGui::TextDisabled("y dejar suelto, incluso fuera del editor.");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Editar")) {
                if (ImGui::MenuItem("Vaciar la escena")) {
                    for (auto& o : objects) g3d_entity_impl_set_position(o.entity, 0, -99999, 0);
                    objects.clear(); obj_sel = -1;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Deshacer", "Ctrl+Z", false, !undo_kind.empty())) do_undo();
                if (ImGui::MenuItem("Rehacer", "Ctrl+Shift+Z", false, !redo_kind.empty())) do_redo();
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicar", "Ctrl+D", false, obj_sel >= 0)) duplicate_obj(obj_sel);
                if (ImGui::MenuItem("Copiar",   "Ctrl+C", false, obj_sel >= 0)) copy_obj(obj_sel);
                if (ImGui::MenuItem("Pegar",    "Ctrl+V", false, has_clip))     paste_obj();
                if (ImGui::MenuItem("Borrar",   "Supr",   false, obj_sel >= 0)) delete_obj(obj_sel);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Generar")) {
                ImGui::TextDisabled("TERRENO PROCEDURAL (las herramientas, en el rail)");
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
                ImGui::TextDisabled("EROSION HIDRAULICA");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderInt("Pasadas##ero", &ero_iters, 10, 600);
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Lluvia##ero", &ero_rain, 0.002f, 0.05f, "%.3f");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Evaporacion##ero", &ero_evap, 0.005f, 0.10f, "%.3f");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Arrastre##ero", &ero_cap, 0.1f, 2.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Cuanto material puede llevarse una corriente rapida.\n"
                                      "Mas alto = barrancos mas marcados.");
                ImGui::SetNextItemWidth(150);
                ImGui::SliderFloat("Talud##ero", &ero_talus, 0.0f, 1.5f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Pendiente a partir de la cual el material suelto se\n"
                                      "desliza. Redondea los picos imposibles. 0 = apagado.");
                if (ImGui::MenuItem(ICON_FA_WATER " Erosionar terreno") && terrain) {
                    push_terrain_undo();   // una pasada de erosion se puede deshacer
                    int n = g3d_editor_terrain_erode(terrain, ero_iters, ero_rain, ero_evap,
                                                     ero_cap, ero_dis, ero_dep,
                                                     ero_slope, ero_talus);
                    // El agua sigue el relieve, y el relieve acaba de cambiar entero.
                    if (!lakes.empty() || !rivers.empty()) rebuild_water();
                    if (g3d_watersim_active()) watersim_sync(true);
                    status = "Erosion aplicada (" + std::to_string(n) + " pasadas)";
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("El agua talla barrancos, los junta en valles y deja\n"
                                      "lo arrancado en abanicos al pie. Es lo que hace que\n"
                                      "un terreno parezca un sitio y no ruido.");
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
                if (ImGui::MenuItem("Variables del juego...")) show_gvars = true;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Puntos, vida, llaves... Salen como GLOBAL del juego:\nlas reglas las cambian y el HUD 2D las pinta.");
                ImGui::Separator();
                if (ImGui::MenuItem("Editar main.prg")) open_main_script();
                if (ImGui::MenuItem("Rehacer main.prg...")) ask_regen_main = true;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Vuelve a dejarlo como recien creado.\nSe pierde lo que hayas escrito.");
                ImGui::EndMenu();
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
        /* ---- SEGUNDA FILA: los modos de trabajo ----
           Va aparte de la barra de menus a proposito: son dos cosas distintas y
           juntas se confundian (habia un menu "Escena" y un modo "Escena" en la
           misma fila). Arriba las ordenes, aqui debajo en que estas trabajando. */
        if (ImGui::BeginViewportSideBar("##barra_modos", ImGui::GetMainViewport(), ImGuiDir_Up,
                                        ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f,
                                        ImGuiWindowFlags_NoSavedSettings)) {
            {
            /* Las herramientas no van aqui: viven en el rail de la izquierda,
               agrupadas y filtradas por el modo. Aqui solo los MODOS y el Play. */
            {
                struct { const char* icono; const char* nombre; const char* tip; } mm[] = {
                    { ICON_FA_CUBES,           "Escena",     "Colocar y mover objetos, jerarquia y propiedades" },
                    { ICON_FA_MOUNTAIN_SUN,    "Terreno",    "Esculpir, pintar, agua, zonas y vegetacion" },
                    { ICON_FA_PERSON_RUNNING,  "Personajes", "Sprites 3D, hojas y animaciones" },
                    { ICON_FA_FONT,            "Interfaz",   "HUD 2D y menus del juego" },
                    { ICON_FA_CODE,            "Codigo",     "Scripts, variables y consola" },
                };
                for (int i = 0; i < M_NUM; i++) {
                    bool on = (modo == i);
                    /* El modo activo va en ambar con el texto oscuro encima: se lee
                       de un vistazo cual esta puesto. Ojo con el equilibrio de
                       push/pop, que aqui se ponen dos colores y solo cuando toca. */
                    if (on) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.949f, 0.686f, 0.259f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text,   ImVec4(0.09f, 0.10f, 0.12f, 1.0f));
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    }
                    std::string et = std::string(mm[i].icono) + "  " + mm[i].nombre;
                    if (ImGui::Button(et.c_str())) modo = i;
                    ImGui::PopStyleColor(on ? 2 : 1);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", mm[i].tip);
                    ImGui::SameLine(0, 2);
                }
            }
            ImGui::SameLine(0, 24);

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
            }
        }
        ImGui::End();

        ImGuiID ds = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        /* La disposicion se rehace al cambiar de modo: cada uno pone sus paneles
           donde le tocan y esconde los que no vienen a cuento. Mientras estas
           dentro de un modo puedes moverlo todo a tu gusto. */
        bool cambio_modo = (modo != modo_ant);
        if (cambio_modo || rehacer_layout) {
            modo_ant = modo; rehacer_layout = false;
            ImGui::DockBuilderRemoveNode(ds);
            ImGui::DockBuilderAddNode(ds, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(ds, ImGui::GetMainViewport()->WorkSize);
            /* El reparto es siempre el mismo -- rail estrecho a la izquierda, un
               panel de listas debajo, el inspector a la derecha, la consola abajo
               y la escena en medio -- y lo que cambia es QUE se mete en cada sitio.
               El inspector se lleva mas ancho que antes: se cortaban los textos. */
            ImGuiID center = ds, rail, left, right, lbottom, bottom;
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.105f, &rail,    &center);
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.19f,  &left,    &center);
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f,  &right,   &center);
            ImGui::DockBuilderSplitNode(left,   ImGuiDir_Down,  0.5f,   &lbottom, &left);
            /* La banda de abajo es mas alta en los modos que tienen un panel ancho
               (la hoja de sprites, la ficha de un menu): metidos en una columna
               estrecha no se pueden usar. */
            bool banda_ancha = (modo == M_PERSONAJES || modo == M_INTERFAZ || modo == M_CODIGO);
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,
                                        banda_ancha ? 0.46f : 0.20f, &bottom, &center);
            // OJO: el nombre tiene que coincidir EXACTAMENTE con el del Begin(),
            // icono incluido, o DockBuilder no encuentra la ventana y queda suelta.
            ImGui::DockBuilderDockWindow(ICON_FA_TOOLBOX "  Herramientas", rail);
            ImGui::DockBuilderDockWindow("Escena", center);
            ImGui::DockBuilderDockWindow(ICON_FA_TERMINAL "  Consola", bottom);
            /* TODOS los paneles se acoplan siempre en algun sitio -- si dejas uno
               fuera se queda flotando en mitad de la pantalla, tapando. Los que no
               son del modo se van a una pestania de detras, y delante se pone el
               que toca (el foco se da mas abajo, ya construido el reparto). */
            ImGui::DockBuilderDockWindow("Assets",    left);
            ImGui::DockBuilderDockWindow("Escenas del proyecto", left);
            ImGui::DockBuilderDockWindow("Jerarquia", lbottom);
            ImGui::DockBuilderDockWindow("Entorno",   lbottom);
            /* Menus y Dialogos son fichas anchas (lista + ficha + acciones): en la
               columna estrecha de la izquierda no se pueden usar, y Dialogos
               directamente no estaba acoplado en ningun sitio -- salia flotando
               en medio de la pantalla. Los dos van a la banda de abajo. */
            ImGui::DockBuilderDockWindow("Menus del juego",    bottom);
            ImGui::DockBuilderDockWindow("Dialogos del juego", bottom);
            ImGui::DockBuilderDockWindow("Guardar partida",    lbottom);
            ImGui::DockBuilderDockWindow("Variables del juego", lbottom);
            ImGui::DockBuilderDockWindow("Inspector", right);
            ImGui::DockBuilderDockWindow(ICON_FA_FONT "  HUD 2D", right);
            ImGui::DockBuilderDockWindow(ICON_FA_PERSON_RUNNING "  Sprites 3D", bottom);
            ImGui::DockBuilderDockWindow("Editor de codigo", center);
            ImGui::DockBuilderFinish(ds);
            // las ventanas que ese modo necesita, abiertas
            if (modo == M_PERSONAJES) show_spr_win = true;
            if (modo == M_INTERFAZ) { show_menus = true; show_dialogos = true; }
            if (modo == M_CODIGO)   { show_escenas = true; show_gvars = true; }
            poner_delante = true;   // se hace al final del frame, ya con las ventanas creadas
        }

        /* ---- EL RAIL DE HERRAMIENTAS ----
           A la izquierda, en vertical, agrupadas por para que sirven y con su
           nombre debajo del grupo. Solo salen las del modo en el que estas: si
           estas esculpiendo un monte no te estorban las del HUD. */
        {
            ImGui::Begin(ICON_FA_TOOLBOX "  Herramientas");
            float ancho = ImGui::GetContentRegionAvail().x;
            auto grupo = [&](const char* titulo) {
                ImGui::Spacing();
                ImGui::TextDisabled("%s", titulo);
                ImGui::Separator();
            };
            auto btn = [&](const char* icon, int t, const char* nombre, const char* tip) {
                bool on = (tool == t), pulsado = false;
                if (on) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.949f, 0.686f, 0.259f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text,   ImVec4(0.09f, 0.10f, 0.12f, 1.0f));
                }
                // el boton ocupa el ancho del rail: se puede estrechar y sigue valiendo
                std::string et = ancho > 92 ? (std::string(icon) + "  " + nombre) : std::string(icon);
                if (ImGui::Button(et.c_str(), ImVec2(-1, 0))) { tool = t; pulsado = true; }
                if (on) ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
                return pulsado;
            };
            if (modo == M_ESCENA) {
                grupo("MANEJAR");
                btn(ICON_FA_ARROW_POINTER,      T_SELECT, "Seleccionar", "Elegir un objeto (clic en la escena)");
                btn(ICON_FA_UP_DOWN_LEFT_RIGHT, T_MOVE,   "Mover",       "Mover (W)");
                btn(ICON_FA_ROTATE,             T_ROTATE, "Rotar",       "Rotar (E)");
                btn(ICON_FA_MAXIMIZE,           T_SCALE,  "Escalar",     "Escalar (R)");
                grupo("PONER");
                btn(ICON_FA_CUBE,               T_PLACE,  "Colocar",     "Colocar el asset elegido en el panel Assets");
                btn(ICON_FA_DRAW_POLYGON,       T_ZONE,   "Zonas",       "Pintar zonas: barreras, ambientes y disparadores");
            } else if (modo == M_TERRENO) {
                grupo("ESCULPIR");
                btn(ICON_FA_MOUNTAIN,            T_RAISE,   "Subir",     "Levantar montanas");
                btn(ICON_FA_ARROW_DOWN,          T_LOWER,   "Bajar",     "Cavar valles");
                btn(ICON_FA_BROOM,               T_SMOOTH,  "Suavizar",  "Quitar los picos");
                btn(ICON_FA_ARROWS_DOWN_TO_LINE, T_FLATTEN, "Nivelar",   "Dejarlo plano a una altura");
                btn(ICON_FA_BORDER_ALL,          T_VERTEX,  "Rejilla",   "Vertice a vertice, para afinar");
                btn(ICON_FA_CIRCLE_NOTCH,        T_HOLE,    "Agujero",   "Perforar (bocas de cueva)");
                grupo("PINTAR Y SEMBRAR");
                btn(ICON_FA_PAINTBRUSH,          T_PAINT,   "Textura",   "Pintar la textura del suelo");
                btn(ICON_FA_SEEDLING,            T_SCATTER, "Vegetacion","Sembrar arboles, hierba y rocas");
                grupo("AGUA");
                btn(ICON_FA_DROPLET,             T_LAKE,        "Lago",      "Clic en un hoyo del terreno para llenarlo");
                btn(ICON_FA_WATER,               T_RIVER,       "Rio",       "Clic para el cauce, doble clic para acabar");
                btn(ICON_FA_ANGLES_DOWN,         T_WATERFALL,   "Cascada",   "Clic arriba (el borde) y clic abajo (la poza)");
                btn(ICON_FA_FAUCET_DRIP,         T_WATERSOURCE, "Manantial", "Una fuente: el agua fluye sola");
            } else if (modo == M_PERSONAJES) {
                grupo("PERSONAJES");
                {
                    bool ya = (tool == T_SPRITE);
                    if (btn(ICON_FA_PERSON_RUNNING, T_SPRITE, "Sprites 3D",
                            ya ? (show_spr_win ? "Esconder la ventana de hojas"
                                               : "Sacar la ventana de hojas")
                               : "Sprite 2D dentro del mundo 3D: hojas, animaciones y personajes"))
                        show_spr_win = ya ? !show_spr_win : true;
                }
                grupo("COLOCARLOS");
                btn(ICON_FA_ARROW_POINTER,      T_SELECT, "Seleccionar", "Elegir un personaje");
                btn(ICON_FA_UP_DOWN_LEFT_RIGHT, T_MOVE,   "Mover",       "Mover (W)");
                btn(ICON_FA_ROTATE,             T_ROTATE, "Rotar",       "Rotar (E)");
            } else if (modo == M_INTERFAZ) {
                grupo("PANTALLA");
                btn(ICON_FA_FONT, T_HUD, "HUD 2D", "Graficos y textos de pantalla (panel 'HUD 2D')");
                ImGui::Spacing();
                if (ImGui::Button(ICON_FA_BARS "   Menus", ImVec2(-1, 0))) { show_menus = true; enfocar_panel = "Menus del juego"; }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Menu principal, de pausa y de opciones");
                if (ImGui::Button(ICON_FA_COMMENT "   Dialogos", ImVec2(-1, 0))) { show_dialogos = true; enfocar_panel = "Dialogos del juego"; }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lo que dice la gente: bocadillos, paginas y respuestas");
            } else if (modo == M_CODIGO) {
                grupo("CODIGO");
                if (ImGui::Button(ICON_FA_FILE_CODE "   main.prg", ImVec2(-1, 0))) open_main_script();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Abrir el main.prg del juego");
                if (ImGui::Button(ICON_FA_SLIDERS "   Variables", ImVec2(-1, 0))) { show_gvars = true; enfocar_panel = "Variables del juego"; }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Puntos, vida, llaves... salen como GLOBAL");
                if (ImGui::Button(ICON_FA_FLOPPY_DISK "   Guardado", ImVec2(-1, 0))) { show_guardado = true; enfocar_panel = "Guardar partida"; }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Que entra en una partida guardada y en cuantas ranuras");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Puntos, vida, llaves... salen como GLOBAL");
                grupo("DEL PROYECTO");
                if (ImGui::Button(ICON_FA_LAYER_GROUP "   Escenas", ImVec2(-1, 0))) { show_escenas = true; enfocar_panel = "Escenas del proyecto"; }
                if (ImGui::Button(ICON_FA_BARS "   Menus", ImVec2(-1, 0)))          show_menus = true;
            }
            ImGui::End();
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
                            o.csize = csize_del_modelo(o);   // la colision, del tamanio del modelo
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
        /* Lo mismo para un SPRITE elegido: arrastrarlo con el gizmo en vez de ir
           a la ficha a teclear numeros. Solo mover y escalar (el alto), que un
           billboard no se rota: siempre mira a la camara. */
        else if (gizmo_tool && spr_sel >= 0 && spr_sel < (int)sprites.size()) {
            SprObj& o = sprites[spr_sel];
            float view[16], proj[16], model[16];
            g3d_editor_get_view(view); g3d_editor_get_proj(proj);
            float t[3] = { o.x, o.y, o.z };
            float r[3] = { 0.0f, 0.0f, 0.0f };
            float s2[3] = { o.height, o.height, o.height };
            ImGuizmo::RecomposeMatrixFromComponents(t, r, s2, model);
            ImGuizmo::OPERATION op = (gizmo_op == ImGuizmo::SCALE) ? ImGuizmo::SCALE
                                                                  : ImGuizmo::TRANSLATE;
            ImGuizmo::Manipulate(view, proj, op, ImGuizmo::WORLD, model);
            if (ImGuizmo::IsUsing()) {
                ImGuizmo::DecomposeMatrixToComponents(model, t, r, s2);
                o.x = t[0]; o.y = t[1]; o.z = t[2];
                if (op == ImGuizmo::SCALE) {
                    float h = (s2[0] + s2[1] + s2[2]) / 3.0f;
                    if (h > 0.05f) o.height = h;
                }
            }
        }

        /* ---- GIZMO DE LA CAMARA ----
           Colocar la camara a base de deslizadores es adivinar. Con el gizmo se
           arrastra donde se quiere y se ve el frustum moverse: el mismo gesto que
           para un objeto, que es lo que uno espera.
           Con Mover se lleva el ojo; con Rotar se gira el punto de mira
           alrededor de el, que es lo que de verdad se quiere decidir. */
        /* Donde esta DE VERDAD la camara del juego, y a que sigue.
           En modo libre es cam_pos. En los modos que siguen a un objeto NO hay
           posicion que arrastrar: la camara se calcula cada frame a partir del
           personaje, y cam_pos no se usa para nada. Poner el gizmo en cam_pos
           dejaba el gizmo lejisimos y sin efecto ninguno. */
        auto cam_anchor = [&](float out[3], float look[3], int *out_follow) -> bool {
            if (cam_mode == 0) {
                out[0]=cam_pos[0]; out[1]=cam_pos[1]; out[2]=cam_pos[2];
                if (look) { look[0]=cam_look[0]; look[1]=cam_look[1]; look[2]=cam_look[2]; }
                if (out_follow) *out_follow = -1;
                return true;
            }
            int fi = (cam_follow >= 0 && cam_follow < (int)objects.size()) ? cam_follow
                     : (obj_sel >= 0 ? obj_sel : -1);
            if (fi < 0) return false;
            if (out_follow) *out_follow = fi;
            float tx = objects[fi].x, ty = objects[fi].y, tz = objects[fi].z;
            if (cam_mode == 1)      { float ob = cam_orbit * 0.0174533f;
                                      out[0]=tx+sinf(ob)*gcam_dist; out[1]=ty+cam_height;
                                      out[2]=tz-cosf(ob)*gcam_dist;
                                      if (look) { look[0]=tx; look[1]=ty+1.0f; look[2]=tz; } }
            else if (cam_mode == 2) {
                float ry = objects[fi].ry, sf = sinf(ry), cf = cosf(ry);
                out[0]=tx+sf*cam_fwd; out[1]=ty+cam_height; out[2]=tz+cf*cam_fwd;
                if (look) { look[0]=tx+sf*(cam_fwd+10.0f); look[1]=ty+cam_height;
                            look[2]=tz+cf*(cam_fwd+10.0f); }
            }
            else                    { out[0]=tx; out[1]=ty+gcam_dist; out[2]=tz+0.5f;
                                      if (look) { look[0]=tx; look[1]=ty; look[2]=tz; } }
            return true;
        };

        if (gizmo_tool && obj_sel < 0 && cam_gizmo) {
            float anchor[3]; int follow = -1;
            if (cam_anchor(anchor, nullptr, &follow)) {
            float view[16], proj[16], model[16];
            g3d_editor_get_view(view); g3d_editor_get_proj(proj);
            float t[3] = { anchor[0], anchor[1], anchor[2] };
            float r[3] = { 0.0f, 0.0f, 0.0f };
            float sc[3] = { 1.0f, 1.0f, 1.0f };
            /* El giro se expresa como el rumbo actual hacia el objetivo, para que
               al rotar el gizmo la mira se mueva desde donde ya estaba. */
            float dx = cam_look[0] - cam_pos[0];
            float dz = cam_look[2] - cam_pos[2];
            float dy = cam_look[1] - cam_pos[1];
            float horiz = sqrtf(dx*dx + dz*dz);
            r[1] = atan2f(dx, dz) * 57.29578f;
            r[0] = -atan2f(dy, horiz > 1e-4f ? horiz : 1e-4f) * 57.29578f;
            /* En tercera persona SI se puede girar: es lo que decide desde donde
               se ve al personaje -- a la espalda, de perfil para un plataformas
               2.5D, o de frente. En primera persona y cenital el angulo lo manda
               el modo, asi que ahi solo queda mover.
               Rotando, el gizmo arranca desde el angulo de orbita actual. */
            ImGuizmo::OPERATION op = (cam_mode == 0 || cam_mode == 1)
                                   ? gizmo_op : ImGuizmo::TRANSLATE;
            if (cam_mode == 1) { r[0] = 0.0f; r[1] = cam_orbit; r[2] = 0.0f; }
            ImGuizmo::RecomposeMatrixFromComponents(t, r, sc, model);
            ImGuizmo::Manipulate(view, proj, op, ImGuizmo::WORLD, model);
            if (ImGuizmo::IsUsing()) {
                ImGuizmo::DecomposeMatrixToComponents(model, t, r, sc);
                if (cam_mode == 0) {
                    cam_pos[0] = t[0]; cam_pos[1] = t[1]; cam_pos[2] = t[2];
                    /* La distancia al objetivo se conserva: girar apunta a otro
                       sitio, no acerca ni aleja. */
                    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                    if (dist < 0.1f) dist = 0.1f;
                    float yaw = r[1] * 0.0174533f, pitch = r[0] * 0.0174533f;
                    cam_look[0] = cam_pos[0] + sinf(yaw) * cosf(pitch) * dist;
                    cam_look[1] = cam_pos[1] - sinf(pitch) * dist;
                    cam_look[2] = cam_pos[2] + cosf(yaw) * cosf(pitch) * dist;
                } else {
                    /* Aqui el arrastre se traduce a los parametros que SI manda el
                       modo: a que altura va la camara y cuanto se separa. Es lo
                       que uno quiere ajustar a ojo de una camara de seguimiento. */
                    float tx = objects[follow].x, ty = objects[follow].y, tz = objects[follow].z;
                    if (cam_mode == 1) {
                        cam_height = t[1] - ty;
                        if (gizmo_op == ImGuizmo::ROTATE) {
                            /* Girando se cambia SOLO el angulo: la distancia se
                               conserva, que es lo que uno espera al rotar. */
                            cam_orbit = r[1];
                        } else {
                            /* Arrastrando, la posicion se lee como orbita: cuanto
                               se ha separado y desde que lado se mira. Asi el
                               mismo gesto sirve para ponerla de perfil. */
                            float ax = t[0] - tx, az = t[2] - tz;
                            float rad = sqrtf(ax*ax + az*az);
                            if (rad > 0.05f) {
                                gcam_dist = rad;
                                cam_orbit = atan2f(ax, -az) * 57.29578f;
                            }
                        }
                        while (cam_orbit < 0.0f)   cam_orbit += 360.0f;
                        while (cam_orbit >= 360.0f) cam_orbit -= 360.0f;
                    } else if (cam_mode == 2) {
                        float ry = objects[follow].ry, sf = sinf(ry), cf = cosf(ry);
                        cam_height = t[1] - ty;
                        cam_fwd    = (t[0] - tx) * sf + (t[2] - tz) * cf;
                    } else {
                        gcam_dist  = t[1] - ty;
                    }
                    /* Solo se limita lo que este arrastre ha tocado: pasarle un
                       minimo a un parametro que ni se movio lo cambiaria a
                       traicion. */
                    if (cam_mode != 2 && gcam_dist  < 0.5f) gcam_dist  = 0.5f;
                    if (cam_mode != 3 && cam_height < 0.1f) cam_height = 0.1f;
                    if (cam_mode == 3)
                        status = "Camara cenital: altura " + std::to_string((int)gcam_dist);
                    else if (cam_mode == 1)
                        status = "Camara: altura " + std::to_string((int)cam_height) +
                                 "  distancia " + std::to_string((int)gcam_dist) +
                                 "  angulo " + std::to_string((int)cam_orbit) + " grados";
                    else
                        status = "Camara: altura " + std::to_string((int)cam_height) +
                                 "  adelanto " + std::to_string((int)cam_fwd);
                }
            }
            } else {
                status = "El gizmo de camara necesita un objeto al que seguir";
            }
        }

        // ---- PREVIEW de la camara principal del juego en el viewport ----
        // Dibuja un frustum amarillo donde quedara la camara y su linea de mira,
        // para saber donde se esta colocando (o desde donde seguira al objeto).
        {
            /* El mismo calculo que usan el gizmo y la ventana de previsualizacion.
               Estaba repetido aqui, y ESA es la razon de que el gizmo saliera en
               otro sitio: dos copias de la misma cuenta que dejaron de coincidir.
               Con una sola funcion ya no pueden discrepar. */
            float cp[3], ct[3];
            bool have = cam_anchor(cp, ct, nullptr);
            camview_ok = have;
            if (have) { for (int q = 0; q < 3; q++) { camview_p[q] = cp[q]; camview_t[q] = ct[q]; } }
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

        /* ---- La COLISION del objeto elegido, dibujada ----
           El cuerpo que se genera es un cubo (o una esfera) de medio lado "tamanio
           colision", y sin verlo no habia forma de saber que se quedaba corto: el
           personaje se mete dentro del dibujo y parece que la fisica no va. */
        if (!playing && obj_sel >= 0 && obj_sel < (int)objects.size()) {
            const SObj& oc = objects[obj_sel];
            if (oc.phys >= 1) {
                float c = oc.csize > 0.05f ? oc.csize : 0.5f;
                auto pw = [&](float wx, float wy, float wz, ImVec2& out) -> bool {
                    float p2[2];
                    if (!g3d_editor_world_to_screen(wx, wy, wz, (float)vp.w, (float)vp.h, p2)) return false;
                    out = ImVec2(img_min.x + p2[0], img_min.y + p2[1]);
                    return true;
                };
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImU32 col = (oc.phys == 5) ? IM_COL32(255, 170, 80, 200)
                                           : IM_COL32(120, 235, 150, 210);
                if (oc.phys == 1 || oc.phys == 5) {
                    // caja (y el muro invisible, que es una caja alta)
                    float y0 = (oc.phys == 5) ? oc.y - 5.0f : oc.y;
                    float y1 = (oc.phys == 5) ? oc.y + 30.0f : oc.y + 2.0f * c;
                    float px4[4] = { oc.x - c, oc.x + c, oc.x + c, oc.x - c };
                    float pz4[4] = { oc.z - c, oc.z - c, oc.z + c, oc.z + c };
                    ImVec2 a[4], b[4]; bool oa[4], ob[4];
                    for (int k = 0; k < 4; k++) {
                        oa[k] = pw(px4[k], y0, pz4[k], a[k]);
                        ob[k] = pw(px4[k], y1, pz4[k], b[k]);
                    }
                    for (int k = 0; k < 4; k++) {
                        int n = (k + 1) % 4;
                        if (oa[k] && oa[n]) dl->AddLine(a[k], a[n], col, 2.0f);
                        if (ob[k] && ob[n]) dl->AddLine(b[k], b[n], col, 2.0f);
                        if (oa[k] && ob[k]) dl->AddLine(a[k], b[k], col, 2.0f);
                    }
                } else {
                    // esfera, capsula y cilindro: dos anillos, uno tumbado y otro de pie
                    float cy = oc.y + c;
                    const int N = 28;
                    ImVec2 pr, p0; bool hr = false, h0 = false;
                    for (int pl = 0; pl < 2; pl++) {
                        hr = false;
                        for (int k = 0; k <= N; k++) {
                            float t = 6.2831853f * (float)k / (float)N;
                            float wx, wy, wz;
                            if (pl == 0) { wx = oc.x + cosf(t) * c; wy = cy;                wz = oc.z + sinf(t) * c; }
                            else         { wx = oc.x + cosf(t) * c; wy = cy + sinf(t) * c;  wz = oc.z; }
                            ImVec2 q; bool ok = pw(wx, wy, wz, q);
                            if (ok && hr) dl->AddLine(pr, q, col, 2.0f);
                            if (k == 0) { p0 = q; h0 = ok; }
                            pr = q; hr = ok;
                        }
                        (void)p0; (void)h0;
                    }
                }
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
        // ---- rejilla de vertices ----
        // Se dibuja el terreno ENTERO, no un parche alrededor del cursor: una
        // rejilla que persigue al raton no deja ver la forma que estas
        // modelando, que es justo para lo que sirve verla.
        //
        // La malla son 161x161 = 25.921 vertices; dibujarlos todos serian ~52.000
        // lineas por frame. Asi que el paso se ADAPTA a lo que ocupa una celda en
        // pantalla: de lejos se salta vertices (una rejilla mas gruesa, que es lo
        // unico legible a esa distancia) y al acercarte llega a la rejilla real,
        // vertice a vertice.
        if (!playing && tool == T_VERTEX && terrain) {
            int side = 0; float wsize = 0.0f;
            if (g3d_editor_terrain_grid(terrain, &side, &wsize)) {
                if (vx_i < 0 || vx_j < 0) { vx_i = side / 2; vx_j = side / 2; }

                ImDrawList* dl = ImGui::GetWindowDrawList();
                auto vert_screen = [&](int i, int j, ImVec2& out) -> bool {
                    float p[3], p2[2];
                    if (!g3d_editor_terrain_vertex(terrain, i, j, p)) return false;
                    if (!g3d_editor_world_to_screen(p[0], p[1], p[2],
                                                    (float)vp.w, (float)vp.h, p2)) return false;
                    out = ImVec2(img_min.x + p2[0], img_min.y + p2[1]);
                    return true;
                };
                // Dentro del viewport, con margen para que las lineas que cruzan
                // el borde se sigan dibujando.
                // Una linea de menos de unos pocos pixeles no aporta nada y en el
                // horizonte se juntan miles: alli la rejilla se convertia en una
                // banda solida de ruido.
                auto too_short = [&](const ImVec2& a, const ImVec2& b) {
                    float dx = b.x - a.x, dy = b.y - a.y;
                    return (dx*dx + dy*dy) < 16.0f;   /* < 4 px */
                };
                auto on_screen = [&](const ImVec2& p) {
                    return p.x > img_min.x - 200.0f && p.x < img_min.x + vp.w + 200.0f &&
                           p.y > img_min.y - 200.0f && p.y < img_min.y + vp.h + 200.0f;
                };

                // Cuantos pixeles ocupa una celda junto al vertice activo: de ahi
                // sale el paso.
                int stride = 1;
                {
                    ImVec2 a, b;
                    if (vert_screen(vx_i, vx_j, a) &&
                        vert_screen(vx_i + 1 < side ? vx_i + 1 : vx_i - 1, vx_j, b)) {
                        float dx = b.x - a.x, dy = b.y - a.y;
                        float px = sqrtf(dx*dx + dy*dy);
                        if (px > 0.01f) stride = (int)ceilf((float)vx_px / px);
                    }
                    if (stride < 1)  stride = 1;
                    if (stride > 32) stride = 32;
                }

                const ImU32 c_dark = IM_COL32(10, 20, 35, 170);
                const ImU32 c_lite = IM_COL32(150, 225, 255, 225);
                for (int pass = 0; pass < 2; pass++) {
                    ImU32 col = pass ? c_lite : c_dark;
                    float th  = pass ? 1.3f : 2.8f;
                    for (int j = 0; j < side; j += stride)
                        for (int i = 0; i < side; i += stride) {
                            ImVec2 a, b;
                            if (!vert_screen(i, j, a)) continue;
                            bool va = on_screen(a);
                            // Una linea se pinta si ELLA se ve Y su vecina de al
                            // lado tambien: en el horizonte las filas se juntan
                            // pero las lineas a lo largo siguen siendo largas, y
                            // miles superpuestas hacian una banda solida.
                            ImVec2 nx, nz;
                            bool wide = (i + stride < side) && vert_screen(i + stride, j, nx)
                                        && !too_short(a, nx);
                            bool deep = (j + stride < side) && vert_screen(i, j + stride, nz)
                                        && !too_short(a, nz);
                            if (wide && deep && (va || on_screen(nx)))
                                dl->AddLine(a, nx, col, th);
                            if (deep && wide && (va || on_screen(nz)))
                                dl->AddLine(a, nz, col, th);
                        }
                }
                // Los puntos solo cuando la rejilla es la de verdad (paso 1); con
                // paso mayor marcarian vertices que no son los que vas a tocar.
                if (stride == 1)
                    for (int j = 0; j < side; j++)
                        for (int i = 0; i < side; i++) {
                            ImVec2 a, nb;
                            if (!vert_screen(i, j, a) || !on_screen(a)) continue;
                            // Y solo donde la celda sea legible: en el horizonte
                            // caben miles de vertices por pixel y el punto deja de
                            // significar nada -- se convierte en una banda de ruido.
                            if (i + 1 < side && vert_screen(i + 1, j, nb) && too_short(a, nb))
                                continue;
                            if (j + 1 < side && vert_screen(i, j + 1, nb) && too_short(a, nb))
                                continue;
                            int di = i - vx_i, dj = j - vx_j;
                            bool sel  = vx_sel.count(j * side + i) != 0;
                            bool soft = (!sel && vx_soft > 0 && di*di + dj*dj <= vx_soft*vx_soft);
                            float rr = sel ? 4.2f : (soft ? 4.0f : 2.8f);
                            dl->AddCircleFilled(a, rr, IM_COL32(10, 20, 35, 200));
                            dl->AddCircleFilled(a, sel ? 3.0f : (soft ? 2.8f : 1.7f),
                                                sel  ? IM_COL32(255, 170, 60, 250) :
                                                soft ? IM_COL32(120, 235, 205, 245)
                                                     : IM_COL32(220, 240, 255, 220));
                        }

                // Los elegidos se dibujan SIEMPRE, aunque la rejilla vaya a paso
                // grueso o el filtro de legibilidad se coma sus puntos: si no, de
                // lejos seleccionas y parece que no ha pasado nada.
                for (int cell : vx_sel) {
                    ImVec2 a;
                    if (!vert_screen(cell % side, cell / side, a) || !on_screen(a)) continue;
                    dl->AddCircleFilled(a, 4.5f, IM_COL32(10, 20, 35, 210));
                    dl->AddCircleFilled(a, 3.0f, IM_COL32(255, 170, 60, 250));
                }

                // El cierre del rectangulo va AQUI y no en el bloque del raton:
                // aquel exige el cursor sobre el viewport, y durante un arrastre
                // ImGui deja de reportarlo en cuanto otro elemento se vuelve
                // activo. La soltada se perdia, la seleccion no llegaba a hacerse
                // nunca y el rectangulo se quedaba pintado.
                if (vx_box && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    ImGuiIO& io2 = ImGui::GetIO();
                    ImVec2 b = io2.MousePos;
                    float x0 = vx_box_a.x < b.x ? vx_box_a.x : b.x;
                    float x1 = vx_box_a.x < b.x ? b.x : vx_box_a.x;
                    float y0 = vx_box_a.y < b.y ? vx_box_a.y : b.y;
                    float y1 = vx_box_a.y < b.y ? b.y : vx_box_a.y;
                    // Un clic sin arrastre es un clic, no "no seleccionar nada":
                    // si limpiara, cualquier toque perderia lo que llevas elegido.
                    if ((x1 - x0) >= 3.0f || (y1 - y0) >= 3.0f) {
                        if (!io2.KeyCtrl) vx_sel.clear();   // Ctrl suma
                        for (int j = 0; j < side; j++)
                            for (int i = 0; i < side; i++) {
                                float p[3], p2[2];
                                if (!g3d_editor_terrain_vertex(terrain, i, j, p)) continue;
                                if (!g3d_editor_world_to_screen(p[0], p[1], p[2],
                                                                (float)vp.w, (float)vp.h, p2)) continue;
                                float px = img_min.x + p2[0], py = img_min.y + p2[1];
                                if (px >= x0 && px <= x1 && py >= y0 && py <= y1)
                                    vx_sel.insert(j * side + i);
                            }
                        status = "Seleccionados " + std::to_string(vx_sel.size()) + " vertices";
                    }
                    vx_box = false;
                }

                if (vx_box) {
                    ImVec2 b = ImGui::GetIO().MousePos;
                    dl->AddRectFilled(vx_box_a, b, IM_COL32(255, 170, 60, 40));
                    dl->AddRect(vx_box_a, b, IM_COL32(255, 190, 80, 220), 0.0f, 0, 1.5f);
                }

                // El vertice activo. Este SI sigue al raton: es el que vas a mover.
                ImVec2 c;
                if (vert_screen(vx_i, vx_j, c)) {
                    ImU32 hot = vx_drag ? IM_COL32(255, 190, 60, 255)
                                        : IM_COL32(255, 255, 255, 255);
                    dl->AddCircleFilled(c, 7.0f, IM_COL32(10, 20, 35, 220));
                    dl->AddCircleFilled(c, 5.0f, hot);
                    dl->AddCircle(c, 11.0f, hot, 0, 2.5f);
                    float pv[3];
                    if (g3d_editor_terrain_vertex(terrain, vx_i, vx_j, pv)) {
                        char lbl[64];
                        snprintf(lbl, sizeof lbl, "y=%.2f", pv[1]);
                        dl->AddText(ImVec2(c.x + 15.0f, c.y - 7.0f), IM_COL32(10,20,35,220), lbl);
                        dl->AddText(ImVec2(c.x + 14.0f, c.y - 8.0f), IM_COL32(255,255,255,240), lbl);
                    }
                }
            }
        }

        // ---- HUD 2D: previo encima de la escena (lo que se vera en el juego) ----
        // El juego corre a 1280x720 (set_mode de escena_iniciar) y el viewport no
        // tiene por que tener esa proporcion, asi que se dibuja el rectangulo de la
        // pantalla del juego centrado y el HUD se escala a el. Se pinta con las
        // MISMAS metricas que usara BennuGD2: el grafico centrado en x,y y el texto
        // glifo a glifo con los offsets de la fuente .fnt.
        if (hud_show && !playing && (!hud.empty() || tool == T_HUD)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float gw = avail.x, gh = avail.x * HUD_H / HUD_W;
            if (gh > avail.y) { gh = avail.y; gw = avail.y * HUD_W / HUD_H; }
            ImVec2 g0(img_min.x + (avail.x - gw) * 0.5f, img_min.y + (avail.y - gh) * 0.5f);
            float sc = gw / HUD_W;
            if (tool == T_HUD) {
                dl->AddRect(g0, ImVec2(g0.x + gw, g0.y + gh), IM_COL32(120, 180, 255, 90));
                dl->AddText(ImVec2(g0.x + 4, g0.y + 4), IM_COL32(120, 180, 255, 140), "pantalla del juego 1280x720");
            }
            // De mayor a menor z: en BennuGD2 el z mas alto se dibuja antes (al fondo).
            std::vector<int> orden(hud.size());
            for (size_t i = 0; i < hud.size(); i++) orden[i] = (int)i;
            std::stable_sort(orden.begin(), orden.end(),
                             [&](int a, int b) { return hud[a].z > hud[b].z; });
            for (int idx : orden) {
                HudItem& h = hud[idx];
                float w, hh, ox, oy;
                hud_item_size(h, &w, &hh);
                hud_item_origin(h, w, hh, &ox, &oy);
                ImVec2 p0(g0.x + ox * sc, g0.y + oy * sc);
                ImVec2 p1(p0.x + w * sc, p0.y + hh * sc);
                if (h.type == 0) {
                    H2Img* im = hud_item_img(h);
                    if (im) {
                        ImVec2 uv0(0, 0), uv1(1, 1);
                        if (h.flags & 1) std::swap(uv0.x, uv1.x);   // espejo horizontal
                        if (h.flags & 2) std::swap(uv0.y, uv1.y);   // espejo vertical
                        ImU32 tint = IM_COL32(255, 255, 255, h.alpha);
                        if (h.angle != 0.0f) {
                            // BennuGD2 gira en sentido antihorario alrededor de x,y
                            float a = -h.angle * 3.14159265f / 180.0f;
                            float ca = cosf(a), sa = sinf(a);
                            ImVec2 c(g0.x + h.x * sc, g0.y + h.y * sc);
                            auto rot = [&](float dx, float dy) {
                                return ImVec2(c.x + dx * ca - dy * sa, c.y + dx * sa + dy * ca);
                            };
                            float hw = w * sc * 0.5f, hhh = hh * sc * 0.5f;
                            dl->AddImageQuad((ImTextureID)(intptr_t)im->tex,
                                             rot(-hw, -hhh), rot(hw, -hhh), rot(hw, hhh), rot(-hw, hhh),
                                             uv0, ImVec2(uv1.x, uv0.y), uv1, ImVec2(uv0.x, uv1.y), tint);
                        } else {
                            dl->AddImage((ImTextureID)(intptr_t)im->tex, p0, p1, uv0, uv1, tint);
                        }
                    } else {
                        // Sin grafico cargado: un hueco, para verlo y poder moverlo.
                        dl->AddRect(p0, p1, IM_COL32(255, 120, 120, 200));
                        dl->AddLine(p0, p1, IM_COL32(255, 120, 120, 120));
                    }
                } else {
                    std::string txt = hud_latin1(h.var.empty() ? h.text : ("[" + h.var + "]"));
                    H2Font* f = h.font.empty() ? nullptr : hud_font(h.font);
                    ImU32 col = IM_COL32(h.col[0], h.col[1], h.col[2], h.col[3]);
                    if (f) {
                        float cx = ox;
                        for (unsigned char c : txt) {
                            int gi = h2_glyph_index(f, c);
                            const auto& gl = f->glyph[gi];
                            if (gl.w && gl.h) {
                                ImVec2 q0(g0.x + (cx + gl.xoffset) * sc, g0.y + (oy + gl.yoffset) * sc);
                                ImVec2 q1(q0.x + gl.w * sc, q0.y + gl.h * sc);
                                dl->AddImage((ImTextureID)(intptr_t)f->tex, q0, q1,
                                             ImVec2(gl.u / (float)f->aw, gl.v / (float)f->ah),
                                             ImVec2((gl.u + gl.w) / (float)f->aw, (gl.v + gl.h) / (float)f->ah),
                                             col);
                            }
                            cx += gl.xadvance;
                        }
                    } else {
                        // Fuente 0 (la del sistema): el previo es aproximado.
                        dl->AddText(p0, col, txt.c_str());
                    }
                }
                if (idx == hud_sel && tool == T_HUD) {
                    dl->AddRect(ImVec2(p0.x - 2, p0.y - 2), ImVec2(p1.x + 2, p1.y + 2),
                                IM_COL32(255, 200, 60, 220));
                    dl->AddCircleFilled(ImVec2(g0.x + h.x * sc, g0.y + h.y * sc), 3.0f,
                                        IM_COL32(255, 200, 60, 220));   // el punto x,y
                }
            }
            // ---- coger elementos con el raton (solo con la herramienta HUD) ----
            if (tool == T_HUD && vp_hovered && !playing) {
                ImVec2 mp = ImGui::GetIO().MousePos;
                float mx = (mp.x - g0.x) / sc, my = (mp.y - g0.y) / sc;   // pixeles del juego
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    hud_sel = -1;
                    for (int k = (int)orden.size() - 1; k >= 0; k--) {   // el de delante primero
                        HudItem& h = hud[orden[k]];
                        float w, hh, ox, oy;
                        hud_item_size(h, &w, &hh);
                        hud_item_origin(h, w, hh, &ox, &oy);
                        if (mx >= ox && mx <= ox + w && my >= oy && my <= oy + hh) {
                            hud_sel = orden[k];
                            hud_drag = true;
                            hud_grab_x = mx - h.x; hud_grab_y = my - h.y;
                            break;
                        }
                    }
                }
                if (hud_drag && hud_sel >= 0 && hud_sel < (int)hud.size() &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    hud[hud_sel].x = floorf(mx - hud_grab_x + 0.5f);   // pixeles enteros
                    hud[hud_sel].y = floorf(my - hud_grab_y + 0.5f);
                }
            }
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) hud_drag = false;
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
                // el relieve de antes del trazo, para poder deshacerlo
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                    tool != T_PAINT && tool != T_ZONE) push_terrain_undo();
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
            } else if (tool == T_SCATTER && terrain && asset_sel >= 0 &&
                       g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                // ---- SEMBRAR ----
                // Lo que quita el aspecto generado no es el terreno, es lo que
                // crece encima. Se siembra por REGLAS (pendiente, altura, agua)
                // para que la vegetacion respete el sitio en vez de salpicarlo:
                // hierba en el llano, nada colgando de un risco ni bajo el agua.
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const int NC = 40;
                ImVec2 prev; bool have_prev = false;
                ImU32 ring = sc_erase ? IM_COL32(255,120,90,230) : IM_COL32(120,230,120,230);
                for (int k = 0; k <= NC; k++) {
                    float a = 6.2831853f * k / NC;
                    float wx = hit[0] + cosf(a) * sc_radius, wz = hit[2] + sinf(a) * sc_radius;
                    float wy = g3d_editor_terrain_height(terrain, wx, wz);
                    float p2[2];
                    if (g3d_editor_world_to_screen(wx, wy, wz, (float)vp.w, (float)vp.h, p2)) {
                        ImVec2 pt(img_min.x + p2[0], img_min.y + p2[1]);
                        if (have_prev) dl->AddLine(prev, pt, ring, 2.0f);
                        prev = pt; have_prev = true;
                    } else have_prev = false;
                }

                // Un trazo entero es UN paso de deshacer, asi que la
                // instantanea se toma al pulsar, no mientras se arrastra.
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && sc_mode != 2)
                    push_scatter_undo();

                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    if (sc_mode == 2) {
                        // EDITAR: un clic elige el ejemplar mas cercano.
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            if (!g3d_scatter_pick(hit[0], hit[2], sc_radius, &sc_sel_k, &sc_sel_i))
                                { sc_sel_k = sc_sel_i = -1; }
                    } else if (sc_mode == 1) {
                        // Borrar: se reconstruye la siembra sin lo que cae dentro
                        // del pincel. Es O(todo), pero solo mientras borras.
                        std::vector<std::string> names;
                        std::vector<std::vector<std::array<float,5>>> keep;
                        for (int k = 0; k < g3d_scatter_kinds(); k++) {
                            const char* nm = g3d_scatter_kind_asset(k);
                            names.push_back(nm ? nm : "");
                            keep.push_back({});
                            int n = g3d_scatter_kind_count(k);
                            for (int i = 0; i < n; i++) {
                                float v[5];
                                if (!g3d_scatter_get(k, i, v)) continue;
                                float dx = v[0] - hit[0], dz = v[2] - hit[2];
                                if (dx*dx + dz*dz > sc_radius * sc_radius)
                                    keep.back().push_back({v[0],v[1],v[2],v[3],v[4]});
                            }
                        }
                        g3d_scatter_clear();
                        for (size_t k = 0; k < names.size(); k++)
                            for (auto& v : keep[k])
                                g3d_scatter_add(names[k].c_str(), v[0], v[1], v[2], v[3], v[4]);
                        g3d_scatter_build(1.0f);
                    } else {
                        std::string asset = "Assets/" + assets[asset_sel];
                        // El viento es de esta especie, no del sembrado entero.
                        g3d_scatter_set_kind_wind(asset.c_str(), sc_wind);
                        g3d_scatter_set_kind_distance(asset.c_str(), sc_dist);
                        g3d_scatter_set_kind_solid(asset.c_str(), sc_solid ? 1 : 0);
                        int placed = 0;
                        int tries = (int)(sc_density * 4.0f) + 4;
                        for (int t = 0; t < tries && placed < (int)sc_density; t++) {
                            float a = (rand() / (float)RAND_MAX) * 6.2831853f;
                            float r = sqrtf(rand() / (float)RAND_MAX) * sc_radius;
                            float wx = hit[0] + cosf(a) * r, wz = hit[2] + sinf(a) * r;
                            float wy = g3d_editor_terrain_height(terrain, wx, wz);
                            if (wy < sc_ymin || wy > sc_ymax) continue;
                            // Pendiente por diferencias: nada crece en una pared.
                            const float e = 1.5f;
                            float hx = g3d_editor_terrain_height(terrain, wx + e, wz)
                                     - g3d_editor_terrain_height(terrain, wx - e, wz);
                            float hz = g3d_editor_terrain_height(terrain, wx, wz + e)
                                     - g3d_editor_terrain_height(terrain, wx, wz - e);
                            float slope = sqrtf(hx*hx + hz*hz) / (2.0f * e);
                            if (slope > sc_slope_max) continue;
                            // Mojado = la superficie del agua esta por encima del
                            // suelo. Sembrar dentro de un lago es de las cosas que
                            // mas delatan un sembrado automatico.
                            if (sc_avoid_water && g3d_watersim_active() &&
                                g3d_water_level_at(wx, wz) > wy + 0.02f) continue;
                            float sc = sc_scale_min +
                                       (sc_scale_max - sc_scale_min) * (rand() / (float)RAND_MAX);
                            g3d_scatter_add(asset.c_str(), wx, wy, wz,
                                            (rand() / (float)RAND_MAX) * 360.0f, sc);
                            placed++;
                        }
                        if (placed) {
                            g3d_scatter_build(1.0f);
                            /* Sembrar y no ver nada era mudo: el modelo puede no
                               cargarse, o el fondo de grupos puede estar lleno. Si
                               esta especie se queda sin grupos, se dice. */
                            int gr = -1;
                            for (int k = 0; k < g3d_scatter_kinds(); k++) {
                                const char* na = g3d_scatter_kind_asset(k);
                                if (na && asset == na) { gr = g3d_scatter_kind_groups(k); break; }
                            }
                            if (gr == 0) {
                                status = "No se ve: " + asset + " no ha dado ninguna malla";
                                console_add("SIEMBRA: '" + asset + "' no ha dado ninguna malla.\n"
                                            "  Puede que el fichero no este en Assets de ESTE proyecto,\n"
                                            "  o que no queden huecos de instancias libres (quedan " +
                                            std::to_string(g3d_instances_free_slots()) + ").\n");
                            } else if (g3d_instances_free_slots() < 16) {
                                console_add("SIEMBRA: quedan solo " +
                                            std::to_string(g3d_instances_free_slots()) +
                                            " huecos de instancias; con menos, las especies nuevas\n"
                                            "  dejaran de aparecer. Quita alguna especie del sembrado.\n");
                            }
                        }
                    }
                }
            } else if (tool == T_VERTEX && terrain) {
                // ---- VERTICE A VERTICE: raton ----
                // El dibujo de la rejilla no esta aqui: se pinta siempre que la
                // herramienta este activa. Aqui solo se elige y se arrastra.
                ImGuiIO& io = ImGui::GetIO();
                int side = 0; float wsize = 0.0f;
                bool grid_ok = g3d_editor_terrain_grid(terrain, &side, &wsize) != 0;

                // La altura se ajusta al paso pedido: es lo que da mesetas y
                // escalones limpios en vez de bultos.
                auto snapped = [&](float y) {
                    return (vx_snap > 0.001f) ? roundf(y / vx_snap) * vx_snap : y;
                };

                if (grid_ok && !vx_drag && !vx_box &&
                    g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, hit)) {
                    float step = wsize / (float)(side - 1);
                    int ci = (int)floorf((hit[0] + wsize * 0.5f) / step + 0.5f);
                    int cj = (int)floorf((hit[2] + wsize * 0.5f) / step + 0.5f);
                    if (ci < 0) ci = 0; if (cj < 0) cj = 0;
                    if (ci > side - 1) ci = side - 1;
                    if (cj > side - 1) cj = side - 1;
                    vx_i = ci; vx_j = cj;
                }

                // --- SELECCION POR RECTANGULO ---
                // Modo explicito en vez de un modificador: de cerca el vertice mas
                // proximo siempre cae bajo el cursor, asi que "pulsar en hueco" no
                // se puede distinguir; y un Shift que no llegue te mueve el terreno
                // en vez de seleccionar, que es peor que no poder seleccionar.
                if (grid_ok && vx_mode == 1 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    vx_box = true; vx_box_a = io.MousePos;
                }
                // --- AGARRAR ---
                if (grid_ok && !vx_box && vx_mode == 0 && vx_i >= 0 &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    float pv[3];
                    if (g3d_editor_terrain_vertex(terrain, vx_i, vx_j, pv)) {
                        push_terrain_undo();
                        vx_y0 = pv[1];
                        vx_mouse_y0 = io.MousePos.y;
                        vx_drag = true;
                        // Para el arrastre lateral: donde toca el suelo el rayo del
                        // raton al empezar. El desplazamiento sale de la diferencia
                        // con el punto de cada frame, asi que la mano y el vertice
                        // van juntos sea cual sea el angulo de camara.
                        vx_grab_x = hit[0]; vx_grab_z = hit[2];
                        vx_sel_xz0.clear();
                        if (vx_sel.count(vx_j * side + vx_i))
                            for (int cell : vx_sel) {
                                float q[3];
                                if (g3d_editor_terrain_vertex(terrain, cell % side, cell / side, q))
                                    vx_sel_xz0.push_back({ cell, { q[0], q[2] } });
                            }
                        else vx_sel_xz0.push_back({ vx_j * side + vx_i, { pv[0], pv[2] } });
                        // Si el vertice agarrado esta en la seleccion, se mueve
                        // TODA ella; si no, solo el (con su caida suave).
                        vx_sel_y0.clear();
                        if (vx_sel.count(vx_j * side + vx_i)) {
                            for (int cell : vx_sel) {
                                float q[3];
                                if (g3d_editor_terrain_vertex(terrain, cell % side, cell / side, q))
                                    vx_sel_y0.push_back({ cell, q[1] });
                            }
                        } else {
                            int n = 2 * vx_soft + 1;
                            vx_y0_near.assign((size_t)n * n, 0.0f);
                            for (int j = 0; j < n; j++)
                                for (int i = 0; i < n; i++) {
                                    float q[3];
                                    if (g3d_editor_terrain_vertex(terrain, vx_i - vx_soft + i,
                                                                  vx_j - vx_soft + j, q))
                                        vx_y0_near[(size_t)j * n + i] = q[1];
                                }
                        }
                    }
                }

                // --- ARRASTRAR ---
                if (vx_drag && vx_axis == 1 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    // LATERAL: se sigue el punto del terreno bajo el raton.
                    float cur[3];
                    if (g3d_editor_terrain_pick(sx, sy, (float)vp.w, (float)vp.h, terrain, cur)) {
                        float ddx = cur[0] - vx_grab_x, ddz = cur[2] - vx_grab_z;
                        for (auto& sv : vx_sel_xz0)
                            g3d_editor_terrain_set_vertex_xz(terrain, sv.first % side,
                                                             sv.first / side,
                                                             sv.second.first + ddx,
                                                             sv.second.second + ddz,
                                                             vx_lat, 0);
                        g3d_editor_terrain_commit(terrain);
                    }
                } else if (vx_drag && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    float dy = (vx_mouse_y0 - io.MousePos.y) * vx_sens;
                    if (!vx_sel_y0.empty()) {
                        for (auto& sv : vx_sel_y0)
                            g3d_editor_terrain_set_vertex_y(terrain, sv.first % side,
                                                            sv.first / side,
                                                            snapped(sv.second + dy), 0);
                    } else {
                        g3d_editor_terrain_set_vertex_y(terrain, vx_i, vx_j,
                                                        snapped(vx_y0 + dy), 0);
                        if (vx_soft > 0) {
                            int n = 2 * vx_soft + 1;
                            for (int j = 0; j < n; j++)
                                for (int i = 0; i < n; i++) {
                                    int gi = vx_i - vx_soft + i, gj = vx_j - vx_soft + j;
                                    if (gi == vx_i && gj == vx_j) continue;
                                    float di = (float)(gi - vx_i), dj = (float)(gj - vx_j);
                                    float d = sqrtf(di*di + dj*dj);
                                    if (d > (float)vx_soft) continue;
                                    float w = 1.0f - d / (float)(vx_soft + 1);
                                    g3d_editor_terrain_set_vertex_y(terrain, gi, gj,
                                            snapped(vx_y0_near[(size_t)j * n + i] + dy * w), 0);
                                }
                        }
                    }
                    g3d_editor_terrain_commit(terrain);
                }
                if (vx_drag && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    vx_drag = false; vx_sel_y0.clear(); vx_sel_xz0.clear();
                    g3d_editor_terrain_commit(terrain);
                    if (!lakes.empty() || !rivers.empty()) rebuild_water();
                    if (g3d_watersim_active()) watersim_sync(true);
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
                if (wheel != 0.0f) {
                    /* Al pasar a manual, se parte del nivel que se esta VIENDO.
                       Antes se partia del ultimo valor manual -- de fabrica -1.0 --
                       asi que la primera vuelta de rueda mandaba el agua a una
                       altura sin relacion con lo que habia en pantalla: casi
                       siempre bajo tierra, donde ni se coloca ni se previsualiza.
                       Desde fuera parecia que la rueda no hacia nada. */
                    if (lake_auto) {
                        lake_level = g3d_lake_spill_level_r(hit[0], hit[2], lake_radius) - 0.3f;
                        lake_auto = false;
                    }
                    lake_level += wheel * 1.0f;
                }
                float lvl = lake_auto
                    ? g3d_lake_spill_level_r(hit[0], hit[2], lake_radius) - 0.3f   // borde LOCAL
                    : lake_level;
                if (lake_auto) lake_level = lvl;   // el slider sigue al automatico
                // Un nivel por debajo del suelo no se puede colocar. Antes eso
                // se rechazaba EN SILENCIO mientras la previsualizacion de agua
                // se seguia dibujando: parecia colocado, la escena se guardaba
                // sin el lago y en el juego no salia nada. Ahora ni se
                // previsualiza y se avisa sobre el cursor.
                bool lake_ok = (lvl > th + 0.05f);

                // previsualiza (solo rehace el agua cuando cambia la celda o el nivel)
                long key = ((long)(hit[0]*0.5f)*100003L + (long)(hit[2]*0.5f)) * 1000L + (long)(lvl*4.0f);
                lake_prev = { hit[0], hit[2], lvl, lake_depth, lake_radius, current_fx() };
                if (lake_ok) {
                    if (!lake_prev_on || key != lake_prev_key) {
                        lake_prev_on = true; lake_prev_key = key;
                        rebuild_water();
                    }
                } else if (lake_prev_on) {
                    lake_prev_on = false; lake_prev_key = -1;
                    rebuild_water();
                }

                if (!lake_ok) {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 c = ImGui::GetIO().MousePos;
                    const char* msg = "El nivel queda BAJO el suelo: sube con la rueda";
                    ImVec2 ts = ImGui::CalcTextSize(msg);
                    dl->AddRectFilled(ImVec2(c.x + 14, c.y - 10),
                                      ImVec2(c.x + 22 + ts.x, c.y + 12 + ts.y),
                                      IM_COL32(120, 20, 20, 220), 4.0f);
                    dl->AddText(ImVec2(c.x + 18, c.y - 6), IM_COL32(255, 230, 230, 255), msg);
                }
                status = lake_ok
                    ? "Rueda: nivel " + std::to_string((int)lvl) + "  |  clic para colocar el lago"
                    : "El nivel queda por debajo del suelo aqui (sube con la rueda).";
                // clic: confirma el lago de la previsualizacion
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && lake_ok) {
                    lakes.push_back({ hit[0], hit[2], lvl, lake_depth, lake_radius, current_fx() });
                    lake_prev_on = false; lake_prev_key = -1;
                    rebuild_water();
                    status = "Lago anadido (nivel " + std::to_string((int)lvl) +
                             ") -- van " + std::to_string(lakes.size());
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
                } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !wsources.empty()) {
                    // Quitar un manantial con el boton derecho, PERO el derecho es
                    // tambien la orbita de camara. Borrar en IsMouseClicked disparaba
                    // al PULSAR, o sea antes de saber si el usuario iba a arrastrar:
                    // cada giro de camara se llevaba un manantial por delante. Se
                    // decide al SOLTAR y solo si el raton apenas se movio.
                    ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                    bool was_drag = (drag.x * drag.x + drag.y * drag.y) > 16.0f;
                    if (!was_drag) {
                        // Y solo si hay uno CERCA de donde se ha hecho clic: sin radio
                        // se borraba el mas cercano aunque estuviera al otro lado del
                        // mapa, asi que un clic en vacio borraba algo invisible.
                        const float PICK_R = 14.0f;   // unidades de mundo
                        int bi = -1; float best = PICK_R * PICK_R;
                        for (int i = 0; i < (int)wsources.size(); i++) {
                            float dx = wsources[i].x - hit[0], dz = wsources[i].z - hit[2];
                            float d = dx*dx + dz*dz; if (d < best) { best = d; bi = i; }
                        }
                        if (bi >= 0) {
                            wsources.erase(wsources.begin() + bi);
                            watersim_sync(true);
                            status = "Manantial quitado";
                        } else {
                            status = "No hay ningun manantial cerca de ahi";
                        }
                    }
                }
            } else if (!terr_tool && tool != T_LAKE && tool != T_RIVER && tool != T_WATERFALL && tool != T_WATERSOURCE && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // punto: sobre agua->superficie, si no sobre el terreno/fondo
                int ok = place_point(sx, sy, hit);
                if (ok) {
                    if (tool == T_SPRITE) {                          // COLOCAR SPRITE 2D
                        if (sheet.image.empty() || sheet.frames.empty()) {
                            status = "Abre antes una hoja de sprites en el panel 'Sprites 3D'";
                        } else {
                            SprObj o;
                            o.sheet = sheet.image;
                            o.name  = "sprite_" + std::to_string((int)sprites.size() + 1);
                            if (!sheet.anims.empty()) o.anim = sheet.anims[0].name;
                            o.x = hit[0]; o.y = hit[1]; o.z = hit[2];
                            sprites.push_back(o);
                            spr_sel = (int)sprites.size() - 1; obj_sel = -1;
                            status = "Sprite colocado (" + o.sheet + ")";
                        }
                    } else if (tool == T_PLACE && asset_sel >= 0) {   // COLOCAR
                        void* m = load_model(assets[asset_sel]);
                        if (m) {
                            int e = g3d_model_spawn(scene, m, hit[0], hit[1], hit[2], 0.0f, 0.0f);
                            SObj o; o.asset = assets[asset_sel];
                            o.name = assets[asset_sel].substr(0, assets[asset_sel].find('.')) +
                                     "_" + std::to_string((int)objects.size());
                            o.entity = e; o.x = hit[0]; o.y = hit[1]; o.z = hit[2]; o.ry = 0; o.scale = 1;
                            o.csize = csize_del_modelo(o);   // la colision, del tamanio del modelo
                            objects.push_back(o); obj_sel = (int)objects.size() - 1; spr_sel = -1;
                        }
                    } else {                                         // SELECCIONAR
                        /* Se busca entre los modelos 3D Y los personajes 2D, y gana
                           el que caiga mas cerca del clic: un sprite es un objeto
                           mas de la escena. La seleccion es una sola, o objeto o
                           sprite, para que el Inspector y el gizmo no dude. */
                        float best = 1e12f; int bi = -1, bs = -1;
                        for (int i = 0; i < (int)objects.size(); i++) {
                            float dx = objects[i].x - hit[0], dz = objects[i].z - hit[2];
                            float d = dx*dx + dz*dz;
                            if (d < best) { best = d; bi = i; bs = -1; }
                        }
                        for (int i = 0; i < (int)sprites.size(); i++) {
                            float dx = sprites[i].x - hit[0], dz = sprites[i].z - hit[2];
                            float d = dx*dx + dz*dz;
                            if (d < best) { best = d; bs = i; bi = -1; }
                        }
                        if (best < 36.0f) {
                            if (bs >= 0) { spr_sel = bs; obj_sel = -1; }
                            else if (bi >= 0) { obj_sel = bi; spr_sel = -1; }
                        }
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
        ImGui::PushTextWrapPos(0.0f);   // que el texto se parta y no se corte
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
        ImGui::PopTextWrapPos();
        ImGui::End();

        // --- Panel: HUD 2D (graficos y textos de BennuGD2 sobre la escena) ---
        ImGui::Begin(ICON_FA_FONT "  HUD 2D");
        ImGui::PushTextWrapPos(0.0f);   // que el texto se parta y no se corte
        ImGui::TextDisabled("Graficos y textos de pantalla (1280x720).");
        ImGui::TextDisabled("Se generan como PROCESS con sus locales.");
        ImGui::Checkbox("Ver el HUD en el viewport", &hud_show);
        if (tool != T_HUD) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Editar")) tool = T_HUD;
        } else ImGui::TextColored(ImVec4(0.5f, 0.8f, 1, 1), "Arrastra los elementos en la escena");
        ImGui::Separator();

        auto hud_nombre_libre = [&](const char* base) {
            for (int n = 1; ; n++) {
                std::string cand = std::string(base) + "_" + std::to_string(n);
                bool usado = false;
                for (auto& h : hud) if (h.name == cand) { usado = true; break; }
                if (!usado) return cand;
            }
        };
        if (ImGui::Button(ICON_FA_IMAGE " Grafico")) {
            HudItem h; h.type = 0; h.name = hud_nombre_libre("hud_grafico");
            if (!hud_gfx_files.empty()) {
                h.asset = hud_gfx_files[0];
                if (hud_is_fpg(h.asset)) { H2Fpg* fp = hud_fpg(h.asset); if (fp) h.code = fp->g[0].code; }
            }
            h.x = HUD_W * 0.5f; h.y = HUD_H * 0.5f;
            hud.push_back(h); hud_sel = (int)hud.size() - 1; tool = T_HUD;
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_T " Texto")) {
            HudItem h; h.type = 1; h.name = hud_nombre_libre("hud_texto");
            if (!hud_font_files.empty()) h.font = hud_font_files[0];
            h.z = -200;   // los textos, por delante de los graficos
            h.x = 20.0f; h.y = 20.0f;
            hud.push_back(h); hud_sel = (int)hud.size() - 1; tool = T_HUD;
        }
        ImGui::SameLine();
        if (ImGui::Button("Refrescar")) {
            hud_gfx_files = hud_scan_gfx(); hud_font_files = hud_scan_fnt();
        }
        if (hud_sel >= 0 && hud_sel < (int)hud.size()) {
            ImGui::SameLine();
            if (ImGui::Button("Duplicar")) {
                HudItem h = hud[hud_sel];
                h.name = hud_nombre_libre(h.type ? "hud_texto" : "hud_grafico");
                h.x += 20.0f; h.y += 20.0f;
                hud.push_back(h); hud_sel = (int)hud.size() - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Borrar")) { hud.erase(hud.begin() + hud_sel); hud_sel = -1; }
        }
        ImGui::Separator();

        ImGui::BeginChild("lista_hud", ImVec2(0, 120), true);
        for (int i = 0; i < (int)hud.size(); i++) {
            std::string et = std::string(hud[i].type ? ICON_FA_T " " : ICON_FA_IMAGE " ") + hud[i].name;
            if (ImGui::Selectable(et.c_str(), hud_sel == i)) hud_sel = i;
        }
        if (hud.empty()) ImGui::TextDisabled("(vacio: anade un grafico o un texto)");
        ImGui::EndChild();

        if (hud_sel >= 0 && hud_sel < (int)hud.size()) {
            HudItem& h = hud[hud_sel];
            ImGui::SeparatorText("Elemento");
            {   char b[128]; strncpy(b, h.name.c_str(), 127); b[127] = 0;
                if (ImGui::InputText("Nombre (PROCESS)", b, sizeof(b))) h.name = b; }
            if (h.type == 0) {
                // ---- GRAFICO ----
                const char* actual = h.asset.empty() ? "(ninguno)" : h.asset.c_str();
                if (ImGui::BeginCombo("Grafico", actual)) {
                    for (auto& fgx : hud_gfx_files) {
                        bool sel = (fgx == h.asset);
                        if (ImGui::Selectable(fgx.c_str(), sel)) {
                            h.asset = fgx;
                            h.code = 0;
                            if (hud_is_fpg(fgx)) { H2Fpg* fp = hud_fpg(fgx); if (fp) h.code = fp->g[0].code; }
                        }
                    }
                    ImGui::EndCombo();
                }
                if (hud_is_fpg(h.asset)) {
                    H2Fpg* fp = hud_fpg(h.asset);
                    if (fp) {
                        // Elegir el grafico VIENDOLO: un FPG puede traer cientos y
                        // por el numero de codigo no hay quien sepa cual es cual.
                        H2Img* cur = hud_item_img(h);
                        if (cur) {
                            float k = 64.0f / (float)((cur->w > cur->h) ? cur->w : cur->h);
                            if (k > 4.0f) k = 4.0f;   // un icono de 8x8 tiene que verse
                            ImGui::Image((ImTextureID)(intptr_t)cur->tex,
                                         ImVec2(cur->w * k, cur->h * k));
                            ImGui::SameLine();
                        }
                        ImGui::BeginGroup();
                        ImGui::Text("graph = %d", h.code);
                        if (cur) ImGui::TextDisabled("%dx%d", cur->w, cur->h);
                        else ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1), "ese codigo no esta en el FPG");
                        if (ImGui::Button("Elegir grafico...")) {
                            hud_fpg_filter[0] = 0;
                            ImGui::OpenPopup("Graficos del FPG");
                        }
                        ImGui::EndGroup();
                        // ---- selector visual: rejilla de miniaturas ----
                        if (ImGui::BeginPopupModal("Graficos del FPG", nullptr,
                                                   ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("%s  -  %d graficos", h.asset.c_str(), fp->n);
                            ImGui::SetNextItemWidth(220);
                            ImGui::InputText("Filtrar (codigo o nombre)", hud_fpg_filter, sizeof(hud_fpg_filter));
                            ImGui::SameLine();
                            ImGui::TextDisabled("(clic en uno para elegirlo)");
                            ImGui::Separator();
                            // Compara sin distinguir mayusculas (nombres de FPG en mayusculas).
                            auto contiene = [](const char* txt, const char* pat) {
                                if (!pat || !*pat) return true;
                                for (const char* a = txt; *a; a++) {
                                    const char *x = a, *y = pat;
                                    while (*x && *y && tolower((unsigned char)*x) == tolower((unsigned char)*y)) { x++; y++; }
                                    if (!*y) return true;
                                }
                                return false;
                            };
                            const float cell = 92.0f, pad = 8.0f;
                            ImGui::BeginChild("rejilla_fpg", ImVec2(cell * 6 + pad * 7, 430.0f), true);
                            float ancho = ImGui::GetContentRegionAvail().x;
                            int cols = (int)(ancho / (cell + pad));
                            if (cols < 1) cols = 1;
                            int puestos = 0;
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            for (int i = 0; i < fp->n; i++) {
                                char cod[32]; snprintf(cod, sizeof(cod), "%d", fp->g[i].code);
                                if (hud_fpg_filter[0] && !contiene(cod, hud_fpg_filter) &&
                                    !contiene(fp->g[i].name, hud_fpg_filter)) continue;
                                if (puestos % cols) ImGui::SameLine(0.0f, pad);
                                puestos++;
                                ImGui::PushID(i);
                                ImVec2 p0 = ImGui::GetCursorScreenPos();
                                bool clic = ImGui::InvisibleButton("celda", ImVec2(cell, cell + 16.0f));
                                bool sobre = ImGui::IsItemHovered();
                                bool elegido = (fp->g[i].code == h.code);
                                ImVec2 p1(p0.x + cell, p0.y + cell);
                                dl->AddRectFilled(p0, p1, IM_COL32(28, 32, 40, 255));
                                // El grafico, encajado en la celda y sin deformar.
                                H2Img& im = fp->g[i].img;
                                float k2 = (cell - 8.0f) / (float)((im.w > im.h) ? im.w : im.h);
                                if (k2 > 4.0f) k2 = 4.0f;               // los muy pequenos, ampliados
                                float iw = im.w * k2, ih = im.h * k2;
                                ImVec2 q0(p0.x + (cell - iw) * 0.5f, p0.y + (cell - ih) * 0.5f);
                                dl->AddImage((ImTextureID)(intptr_t)im.tex, q0, ImVec2(q0.x + iw, q0.y + ih));
                                dl->AddRect(p0, p1, elegido ? IM_COL32(255, 200, 60, 255)
                                                    : sobre ? IM_COL32(120, 180, 255, 255)
                                                            : IM_COL32(70, 78, 92, 255));
                                dl->AddText(ImVec2(p0.x + 3, p1.y + 1),
                                            elegido ? IM_COL32(255, 200, 60, 255) : IM_COL32(190, 195, 205, 255), cod);
                                if (sobre)
                                    ImGui::SetTooltip("codigo %d  -  %dx%d\n%s",
                                                      fp->g[i].code, im.w, im.h, fp->g[i].name);
                                if (clic) { h.code = fp->g[i].code; ImGui::CloseCurrentPopup(); }
                                ImGui::PopID();
                            }
                            if (!puestos) ImGui::TextDisabled("Ningun grafico cuadra con el filtro.");
                            ImGui::EndChild();
                            if (ImGui::Button("Cerrar")) ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }
                        ImGui::TextDisabled("file = el FPG, graph = este codigo.");
                    } else ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1), "No pude leer el FPG.");
                } else if (!h.asset.empty() && !hud_img(h.asset)) {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1), "No pude leer la imagen.");
                } else if (H2Img* im1 = hud_item_img(h)) {
                    float k = 64.0f / (float)((im1->w > im1->h) ? im1->w : im1->h);
                    if (k > 4.0f) k = 4.0f;
                    ImGui::Image((ImTextureID)(intptr_t)im1->tex, ImVec2(im1->w * k, im1->h * k));
                    ImGui::SameLine();
                    ImGui::TextDisabled("%dx%d", im1->w, im1->h);
                }
                if (!h.asset.empty() && !fs::exists(assets_dir + "/" + h.asset))
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1),
                                       "'%s' no esta en Assets de este proyecto:\n"
                                       "el juego no podra cargarlo (copialo ahi).", h.asset.c_str());
                if (h.asset.empty()) {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1),
                                       "Sin grafico: este elemento NO se genera.");
                    if (hud_gfx_files.empty())
                        ImGui::TextWrapped("No hay ninguna imagen (.png/.jpg) ni ningun .fpg en la "
                                           "carpeta Assets del proyecto. Copia ahi el grafico y dale "
                                           "a Refrescar.");
                }
                ImGui::DragFloat("size (%)", &h.size, 1.0f, 1.0f, 1000.0f, "%.0f");
                ImGui::DragFloat("angle (grados)", &h.angle, 1.0f, -360.0f, 360.0f, "%.0f");
                bool fx = (h.flags & 1) != 0, fy = (h.flags & 2) != 0;
                if (ImGui::Checkbox("Espejo horizontal", &fx)) h.flags = (h.flags & ~1) | (fx ? 1 : 0);
                ImGui::SameLine();
                if (ImGui::Checkbox("Vertical", &fy)) h.flags = (h.flags & ~2) | (fy ? 2 : 0);
                ImGui::SliderInt("alpha", &h.alpha, 0, 255);
                ImGui::TextDisabled("x,y = CENTRO del grafico (como en BennuGD2).");
            } else {
                // ---- TEXTO ----
                const char* fnt = h.font.empty() ? "(fuente 0 del sistema)" : h.font.c_str();
                if (ImGui::BeginCombo("Fuente (.fnt)", fnt)) {
                    if (ImGui::Selectable("(fuente 0 del sistema)", h.font.empty())) h.font.clear();
                    for (auto& ff : hud_font_files)
                        if (ImGui::Selectable(ff.c_str(), ff == h.font)) h.font = ff;
                    ImGui::EndCombo();
                }
                if (!h.font.empty() && !hud_font(h.font))
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1), "No pude leer la fuente.");
                if (!h.font.empty() && !fs::exists(assets_dir + "/" + h.font))
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1),
                                       "'%s' no esta en Assets de este proyecto:\n"
                                       "el juego no podra cargarla (copiala ahi).", h.font.c_str());
                if (h.font.empty())
                    ImGui::TextDisabled("Con la fuente del sistema el previo es aproximado.");
                bool esvar = !h.var.empty();
                if (ImGui::Checkbox("Mostrar una variable (write_var)", &esvar)) {
                    if (esvar && h.var.empty()) h.var = "mi_variable";
                    if (!esvar) h.var.clear();
                }
                if (esvar) {
                    char b[128]; strncpy(b, h.var.c_str(), 127); b[127] = 0;
                    if (ImGui::InputText("Variable GLOBAL", b, sizeof(b))) h.var = b;
                    const char* tipos[] = { "int", "float", "string" };
                    ImGui::Combo("Tipo", &h.vartype, tipos, 3);
                    ImGui::TextDisabled("El editor la declara GLOBAL; dale valor desde tu codigo.");
                } else {
                    char b[256]; strncpy(b, h.text.c_str(), 255); b[255] = 0;
                    if (ImGui::InputText("Texto", b, sizeof(b))) h.text = b;
                }
                const char* alins[] = { "arriba-izquierda", "arriba-centro", "arriba-derecha",
                                        "medio-izquierda", "centro", "medio-derecha",
                                        "abajo-izquierda", "abajo-centro", "abajo-derecha" };
                ImGui::Combo("Alineacion", &h.align, alins, 9);
                float c[4] = { h.col[0] / 255.0f, h.col[1] / 255.0f, h.col[2] / 255.0f, h.col[3] / 255.0f };
                if (ImGui::ColorEdit4("Color", c)) {
                    for (int k = 0; k < 4; k++) h.col[k] = (int)(c[k] * 255.0f + 0.5f);
                }
                ImGui::TextDisabled("Se genera con write()/write_var() + write_set_rgba().");
            }
            ImGui::SeparatorText("Posicion en pantalla");
            ImGui::DragFloat("x", &h.x, 1.0f, -HUD_W, HUD_W * 2, "%.0f");
            ImGui::DragFloat("y", &h.y, 1.0f, -HUD_H, HUD_H * 2, "%.0f");
            ImGui::DragInt("z", &h.z, 1.0f, -1000, 1000);
            ImGui::TextDisabled("z menor = mas al frente. El 3D queda siempre detras.");
            if (ImGui::SmallButton("Centrar")) { h.x = HUD_W * 0.5f; h.y = HUD_H * 0.5f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Arriba izq.")) { h.x = 20; h.y = 20; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Abajo dcha.")) { h.x = HUD_W - 20; h.y = HUD_H - 20; }
        }
        ImGui::PopTextWrapPos();
        ImGui::End();

        // --- VENTANA de sprites 3D (hojas de sprites, estilo HD-2D) ---
        // Flotante y a dos columnas a proposito: metida en la columna acoplada de
        // la derecha, la hoja se veia por una rendija y no habia quien trabajara.
        // Izquierda: la hoja y su recorte. Derecha: animaciones y los colocados.
        if (show_spr_win) {
        /* En el modo Personajes esta ventana es un panel mas y va acoplada abajo.
           Fuera de ese modo se abre suelta y centrada -- pero OJO: fijarle la
           posicion la DESACOPLA, asi que solo se toca cuando va suelta. */
        if (modo != M_PERSONAJES) {
            ImGuiViewport* vp2 = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(vp2->WorkPos.x + vp2->WorkSize.x * 0.5f,
                                           vp2->WorkPos.y + vp2->WorkSize.y * 0.5f),
                                    ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImVec2 tam(vp2->WorkSize.x * 0.72f, vp2->WorkSize.y * 0.80f);
            if (tam.x < 680.0f) tam.x = 680.0f;
            if (tam.y < 440.0f) tam.y = 440.0f;
            ImGui::SetNextWindowSize(tam, ImGuiCond_Appearing);
        }
        // el tamano minimo solo manda cuando va suelta
        if (modo != M_PERSONAJES)
            ImGui::SetNextWindowSizeConstraints(ImVec2(680, 440), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin(ICON_FA_PERSON_RUNNING "  Sprites 3D", &show_spr_win);
        bool spr_dos_columnas = ImGui::BeginTable("spr_cols", 2,
                                    ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV);
        if (spr_dos_columnas) {
            ImGui::TableSetupColumn("hoja",  ImGuiTableColumnFlags_WidthStretch, 0.60f);
            ImGui::TableSetupColumn("resto", ImGuiTableColumnFlags_WidthStretch, 0.40f);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
        }
        if (sheet_refrescar) {          // hoja nueva sin fondo: que salga en las listas
            sheet_refrescar = false;
            hud_gfx_files = hud_scan_gfx();
            auto itc = hud_imgs.find(sheet.image);
            if (itc != hud_imgs.end()) { h2_free_image(&itc->second); hud_imgs.erase(itc); }
        }
        ImGui::TextDisabled("Hoja de sprites -> personajes 2D en el mundo 3D.");
        {
            // Solo imagenes (un FPG ya trae un grafico por fotograma).
            std::vector<std::string> imgs = hud_gfx_files;   // imagenes Y FPG
            const char* cur = sheet.image.empty() ? "(elige una hoja)" : sheet.image.c_str();
            if (ImGui::BeginCombo("Hoja", cur)) {
                for (auto& im : imgs)
                    if (ImGui::Selectable(im.c_str(), im == sheet.image)) {
                        if (!strcmp(sheet_open(im), "guardada"))
                            sheet_msg = std::to_string((int)sheet.frames.size()) + " fotogramas y " +
                                        std::to_string((int)sheet.anims.size()) +
                                        " animaciones (tal como los guardaste)";
                        sheet_sel.clear(); sheet_anim_sel = -1;
                    }
                ImGui::EndCombo();
            }
            if (imgs.empty()) ImGui::TextDisabled("No hay imagenes ni FPG en Assets.");
            else ImGui::TextDisabled("Vale un PNG o un FPG (se junta en una hoja al elegirlo).");
        }
        if (!sheet.image.empty()) {
            H2Img* im = hud_img(sheet.image);
            if (ImGui::Button(ICON_FA_TABLE_CELLS " Detectar fotogramas")) sheet_detect(sheet.image);
            ImGui::SameLine();
            if (ImGui::Checkbox("Limpiar", &sheet_limpiar)) sheet_detect(sheet.image);
            ImGui::SameLine();
            if (ImGui::Checkbox("Quitar fondo", &sheet_quitar_fondo)) sheet_detect(sheet.image);
            ImGui::SameLine();
            ImGui::Checkbox("Agrupar solo", &sheet_auto_anims);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Al detectar, propone animaciones agrupando por filas\n"
                                  "(fila1_1, fila1_2...). Apagado: las animaciones son\n"
                                  "solo las que montes tu.");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Los rips traen el fondo pintado; sin quitarlo el personaje\n"
                                  "sale en el juego con su recuadro. Se hace una COPIA\n"
                                  "'<nombre>_sinfondo.png' y el original no se toca.");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Quita el texto de creditos que traen los rips y junta\n"
                                  "los trozos sueltos (un arma separada del cuerpo).");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Guardar hoja")) sheet_save();
            if (!sheet_msg.empty()) ImGui::TextColored(ImVec4(0.6f,0.85f,1,1), "%s", sheet_msg.c_str());
            ImGui::TextDisabled("Hoja %dx%d", sheet.w, sheet.h);

            // ---- RECORTE A MANO ----
            // Dos formas: por columnas x filas (lo mas rapido si la hoja es
            // regular) o por tamano de celda con margen y separacion (lo que
            // hace falta cuando la hoja trae bordes o huecos fijos). La rejilla
            // se ve encima de la hoja antes de aplicarla.
            if (ImGui::CollapsingHeader("Recorte a mano", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SetNextItemWidth(70); ImGui::InputInt("cols", &sheet_grid_cols, 0);
                ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputInt("filas", &sheet_grid_rows, 0);
                ImGui::SameLine();
                if (ImGui::Button("Partir en rejilla")) {
                    sheet_make_grid(sheet_grid_cols, sheet_grid_rows);
                    if (sheet_grid_cols > 0 && sheet_grid_rows > 0) {
                        sheet_cell_w = sheet.w / sheet_grid_cols;
                        sheet_cell_h = sheet.h / sheet_grid_rows;
                        sheet_off_x = sheet_off_y = sheet_gap_x = sheet_gap_y = 0;
                    }
                }
                ImGui::Separator();
                ImGui::SetNextItemWidth(70); ImGui::InputInt("ancho celda", &sheet_cell_w, 0);
                ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputInt("alto celda", &sheet_cell_h, 0);
                ImGui::SetNextItemWidth(70); ImGui::InputInt("margen x", &sheet_off_x, 0);
                ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputInt("margen y", &sheet_off_y, 0);
                ImGui::SetNextItemWidth(70); ImGui::InputInt("separacion x", &sheet_gap_x, 0);
                ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::InputInt("separacion y", &sheet_gap_y, 0);
                ImGui::Checkbox("Ver la rejilla encima", &sheet_ver_rejilla);
                ImGui::SameLine();
                if (ImGui::Button("Aplicar el recorte")) {
                    if (sheet_cell_w > 0 && sheet_cell_h > 0) {
                        sheet.frames.clear(); sheet.anims.clear(); sheet_sel.clear();
                        int fila = 0;
                        for (int yy = sheet_off_y; yy + sheet_cell_h <= sheet.h;
                             yy += sheet_cell_h + sheet_gap_y, fila++) {
                            for (int xx = sheet_off_x; xx + sheet_cell_w <= sheet.w;
                                 xx += sheet_cell_w + sheet_gap_x) {
                                SprFrame f2;
                                f2.x = xx; f2.y = yy; f2.w = sheet_cell_w; f2.h = sheet_cell_h;
                                f2.ax = sheet_cell_w / 2; f2.ay = sheet_cell_h - 1;
                                f2.band = fila;
                                sheet.frames.push_back(f2);
                            }
                        }
                        int nf = 0;
                        for (auto& f2 : sheet.frames) if (f2.band + 1 > nf) nf = f2.band + 1;
                        if (sheet_auto_anims) {     // solo si las pides automaticas
                            for (int b = 0; b < nf; b++) {
                                SprAnim an; an.name = "fila" + std::to_string(b + 1); an.fps = 10;
                                for (int i = 0; i < (int)sheet.frames.size(); i++)
                                    if (sheet.frames[i].band == b) an.frames.push_back(i);
                                if (!an.frames.empty()) sheet.anims.push_back(an);
                            }
                            if (nf > 1) {
                                SprAnim an; an.name = "todas_las_direcciones"; an.fps = 10;
                                for (int i = 0; i < (int)sheet.frames.size(); i++) an.frames.push_back(i);
                                sheet.anims.push_back(an);
                            }
                        }
                        sheet.cols = sheet.frames.empty() ? 0 : (int)sheet.frames.size() / (nf ? nf : 1);
                        sheet.rows = nf;
                        sheet_msg = std::to_string((int)sheet.frames.size()) +
                                    " fotogramas recortados a mano (" +
                                    std::to_string(sheet_cell_w) + "x" + std::to_string(sheet_cell_h) +
                                    ") y " + std::to_string((int)sheet.anims.size()) + " animaciones";
                    }
                }
                ImGui::Checkbox("Dibujar fotogramas con el raton", &sheet_dibujar);
                if (sheet_dibujar)
                    ImGui::TextDisabled("Arrastra sobre la hoja para anadir un fotograma.");
            }

            ImGui::SetNextItemWidth(160);
            ImGui::SliderFloat("zoom", &sheet_zoom, 1.0f, 6.0f, "%.1fx");

            // ---- la hoja, con los fotogramas marcados ----
            // La hoja se lleva el alto que quede en su columna: es lo que se mira
            // el 90% del tiempo.
            float alto_hoja = ImGui::GetContentRegionAvail().y - 150.0f;
            if (alto_hoja < 220.0f) alto_hoja = 220.0f;
            ImGui::BeginChild("hoja_previo", ImVec2(0, alto_hoja), true,
                              ImGuiWindowFlags_HorizontalScrollbar);
            if (im) {
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImGui::Image((ImTextureID)(intptr_t)im->tex,
                             ImVec2(sheet.w * sheet_zoom, sheet.h * sheet_zoom));
                bool sobre_img = ImGui::IsItemHovered();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 mp = ImGui::GetIO().MousePos;
                int pinchado = -1;
                for (size_t i = 0; i < sheet.frames.size(); i++) {
                    const SprFrame& f2 = sheet.frames[i];
                    ImVec2 a(p0.x + f2.x * sheet_zoom, p0.y + f2.y * sheet_zoom);
                    ImVec2 b(a.x + f2.w * sheet_zoom, a.y + f2.h * sheet_zoom);
                    bool sel = std::find(sheet_sel.begin(), sheet_sel.end(), (int)i) != sheet_sel.end();
                    bool hov = sobre_img && mp.x >= a.x && mp.x <= b.x && mp.y >= a.y && mp.y <= b.y;
                    if (hov && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) pinchado = (int)i;
                    dl->AddRect(a, b, sel ? IM_COL32(255, 200, 60, 255)
                                     : hov ? IM_COL32(120, 200, 255, 220)
                                           : IM_COL32(90, 150, 220, 110));
                    if (sel || hov) {
                        char num[16]; snprintf(num, sizeof(num), "%d", (int)i);
                        dl->AddText(ImVec2(a.x + 2, a.y + 1), IM_COL32(255, 230, 120, 255), num);
                        // el ancla, que es lo que se planta en el suelo
                        dl->AddCircleFilled(ImVec2(a.x + f2.ax * sheet_zoom,
                                                   a.y + f2.ay * sheet_zoom),
                                            2.5f, IM_COL32(255, 90, 90, 255));
                    }
                }
                // ---- la rejilla del recorte a mano, encima de la hoja ----
                if (sheet_ver_rejilla && sheet_cell_w > 0 && sheet_cell_h > 0) {
                    for (int yy = sheet_off_y; yy + sheet_cell_h <= sheet.h;
                         yy += sheet_cell_h + sheet_gap_y)
                        for (int xx = sheet_off_x; xx + sheet_cell_w <= sheet.w;
                             xx += sheet_cell_w + sheet_gap_x)
                            dl->AddRect(ImVec2(p0.x + xx * sheet_zoom, p0.y + yy * sheet_zoom),
                                        ImVec2(p0.x + (xx + sheet_cell_w) * sheet_zoom,
                                               p0.y + (yy + sheet_cell_h) * sheet_zoom),
                                        IM_COL32(120, 255, 160, 150));
                }
                // ---- dibujar un fotograma arrastrando el raton ----
                if (sheet_dibujar && sobre_img) {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        sheet_drag_ini = mp; sheet_drag_on = true;
                    }
                    if (sheet_drag_on) {
                        ImVec2 a(sheet_drag_ini.x < mp.x ? sheet_drag_ini.x : mp.x,
                                 sheet_drag_ini.y < mp.y ? sheet_drag_ini.y : mp.y);
                        ImVec2 b(sheet_drag_ini.x > mp.x ? sheet_drag_ini.x : mp.x,
                                 sheet_drag_ini.y > mp.y ? sheet_drag_ini.y : mp.y);
                        dl->AddRect(a, b, IM_COL32(255, 255, 120, 255));
                        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                            sheet_drag_on = false;
                            SprFrame f2;
                            f2.x = (int)((a.x - p0.x) / sheet_zoom);
                            f2.y = (int)((a.y - p0.y) / sheet_zoom);
                            f2.w = (int)((b.x - a.x) / sheet_zoom);
                            f2.h = (int)((b.y - a.y) / sheet_zoom);
                            if (f2.w > 1 && f2.h > 1) {
                                f2.ax = f2.w / 2; f2.ay = f2.h - 1; f2.band = 0;
                                sheet.frames.push_back(f2);
                                sheet_sel.clear();
                                sheet_sel.push_back((int)sheet.frames.size() - 1);
                                sheet_msg = "Fotograma anadido a mano (" +
                                            std::to_string(f2.w) + "x" + std::to_string(f2.h) + ")";
                            }
                        }
                    }
                    pinchado = -1;   // mientras se dibuja no se selecciona
                }
                if (pinchado >= 0) {
                    bool ctrl = ImGui::GetIO().KeyCtrl, shift = ImGui::GetIO().KeyShift;
                    auto it = std::find(sheet_sel.begin(), sheet_sel.end(), pinchado);
                    if (shift && !sheet_sel.empty()) {          // rango desde el ultimo
                        int a2 = sheet_sel.back(), b2 = pinchado;
                        if (a2 > b2) std::swap(a2, b2);
                        for (int k = a2; k <= b2; k++)
                            if (std::find(sheet_sel.begin(), sheet_sel.end(), k) == sheet_sel.end())
                                sheet_sel.push_back(k);
                    } else if (ctrl) {
                        if (it != sheet_sel.end()) sheet_sel.erase(it);
                        else sheet_sel.push_back(pinchado);
                    } else {
                        sheet_sel.clear(); sheet_sel.push_back(pinchado);
                    }
                }
            } else ImGui::TextColored(ImVec4(1,0.5f,0.4f,1), "No pude leer la imagen.");
            ImGui::EndChild();
            // ---- ajustar a mano el fotograma elegido (se ve al momento) ----
            if (sheet_sel.size() == 1 && sheet_sel[0] < (int)sheet.frames.size()) {
                SprFrame& f2 = sheet.frames[sheet_sel[0]];
                ImGui::SeparatorText("Fotograma elegido");
                ImGui::SetNextItemWidth(180);
                ImGui::DragInt2("recorte x,y", &f2.x, 1.0f, 0, 8192);
                ImGui::SetNextItemWidth(180);
                ImGui::DragInt2("ancho,alto", &f2.w, 1.0f, 1, 8192);
                ImGui::SetNextItemWidth(180);
                ImGui::DragInt2("ancla x,y", &f2.ax, 1.0f, -4096, 8192);
                ImGui::TextDisabled("El ancla (punto rojo) es lo que se apoya en el suelo.");
                if (ImGui::SmallButton("Ancla a los pies")) { f2.ax = f2.w / 2; f2.ay = f2.h - 1; }
                ImGui::SameLine();
                if (ImGui::SmallButton("Borrar fotograma")) {
                    int pos = sheet_sel[0], viejos = (int)sheet.frames.size();
                    sheet.frames.erase(sheet.frames.begin() + pos);
                    {   std::vector<int> mapa(viejos);
                        for (int q = 0; q < viejos; q++)
                            mapa[q] = (q < pos) ? q : (q == pos ? -1 : q - 1);
                        sheet_remap(mapa);
                    }
                    sheet_sel.clear();
                    sheet_msg = "Fotograma borrado (las animaciones se han renumerado solas)";
                }
            }
            // Arreglo a mano de lo que la deteccion no puede saber: dos personajes
            // que se tocan salen en un fotograma, y un fotograma partido en dos.
            if (!sheet_sel.empty()) {
                if (sheet_sel.size() == 1) {
                    ImGui::SetNextItemWidth(70);
                    ImGui::InputInt("en##fr", &sheet_frame_split, 0);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Partir el fotograma") &&
                        sheet_frame_split >= 2 && sheet_sel[0] < (int)sheet.frames.size()) {
                        SprFrame f0 = sheet.frames[sheet_sel[0]];
                        int k = sheet_frame_split, anchoi = f0.w / k;
                        std::vector<SprFrame> nuevos;
                        for (int q = 0; q < k; q++) {
                            SprFrame f2 = f0;
                            f2.x = f0.x + q * anchoi;
                            f2.w = (q == k - 1) ? (f0.x + f0.w - f2.x) : anchoi;
                            f2.ax = f2.w / 2;
                            nuevos.push_back(f2);
                        }
                        int pos = sheet_sel[0], viejos = (int)sheet.frames.size();
                        sheet.frames.erase(sheet.frames.begin() + pos);
                        sheet.frames.insert(sheet.frames.begin() + pos,
                                            nuevos.begin(), nuevos.end());
                        {   // renumerar: quien usaba el partido se queda con el primer trozo
                            std::vector<int> mapa(viejos);
                            for (int q = 0; q < viejos; q++)
                                mapa[q] = (q < pos) ? q : (q == pos ? pos : q + k - 1);
                            sheet_remap(mapa);
                        }
                        sheet_sel.clear();
                        sheet_msg = "Fotograma partido en " + std::to_string(k) +
                                    " (las animaciones se han renumerado solas).";
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(si salieron dos juntos)");
                } else if (ImGui::SmallButton("Unir los fotogramas elegidos")) {
                    std::vector<int> sel = sheet_sel;
                    std::sort(sel.begin(), sel.end());
                    SprFrame u = sheet.frames[sel[0]];
                    int x2 = u.x + u.w, y2 = u.y + u.h, basey = u.y + u.ay;
                    for (size_t q = 1; q < sel.size(); q++) {
                        const SprFrame& f2 = sheet.frames[sel[q]];
                        if (f2.x < u.x) u.x = f2.x;
                        if (f2.y < u.y) u.y = f2.y;
                        if (f2.x + f2.w > x2) x2 = f2.x + f2.w;
                        if (f2.y + f2.h > y2) y2 = f2.y + f2.h;
                        if (f2.y + f2.ay > basey) basey = f2.y + f2.ay;
                    }
                    u.w = x2 - u.x; u.h = y2 - u.y;
                    u.ax = u.w / 2; u.ay = basey - u.y;
                    int viejos = (int)sheet.frames.size();
                    for (int q = (int)sel.size() - 1; q >= 0; q--)
                        sheet.frames.erase(sheet.frames.begin() + sel[q]);
                    sheet.frames.insert(sheet.frames.begin() + sel[0], u);
                    {   // los unidos pasan a ser uno; los de detras se corren
                        std::vector<int> mapa(viejos);
                        for (int q = 0; q < viejos; q++) {
                            bool era = std::find(sel.begin(), sel.end(), q) != sel.end();
                            int antes = 0;
                            for (int e : sel) if (e < q) antes++;
                            mapa[q] = era ? sel[0] : q - antes + (q > sel[0] ? 1 : 0);
                        }
                        sheet_remap(mapa);
                    }
                    sheet_sel.clear();
                    sheet_msg = "Fotogramas unidos (las animaciones se han renumerado solas).";
                }
            }
            ImGui::TextDisabled("Las animaciones salen solas al detectar; abajo se retocan.");
            ImGui::TextDisabled("clic = elegir | Ctrl+clic = anadir | May+clic = rango  (%d elegidos)",
                                (int)sheet_sel.size());

        }
        if (spr_dos_columnas) ImGui::TableSetColumnIndex(1);
        if (!sheet.image.empty()) {
            H2Img* im = hud_img(sheet.image);
            // ---- animaciones ----
            ImGui::SeparatorText("Animaciones");
            ImGui::SetNextItemWidth(140);
            ImGui::InputText("nombre", sheet_anim_name, sizeof(sheet_anim_name));
            ImGui::SameLine();
            ImGui::BeginDisabled(sheet_sel.empty());
            if (ImGui::Button("Crear con los elegidos")) {
                SprAnim an; an.name = sheet_anim_name; an.fps = 10;
                // EN EL ORDEN EN QUE LOS HAS IDO ELIGIENDO: ordenarlos por numero
                // impedia montar un vaiven (1-2-3-2) o cualquier orden a mano.
                an.frames = sheet_sel;
                sheet.anims.push_back(an);
                sheet_anim_sel = (int)sheet.anims.size() - 1;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("(en el orden que los elijas)");
            for (int i = 0; i < (int)sheet.anims.size(); i++) {
                ImGui::PushID(1000 + i);
                SprAnim& an = sheet.anims[i];
                char et[160];
                snprintf(et, sizeof(et), "%s  (%d fotogramas, %d fps)",
                         an.name.c_str(), (int)an.frames.size(), an.fps);
                if (ImGui::Selectable(et, sheet_anim_sel == i)) { sheet_anim_sel = i; sheet_anim_t = 0.0f; }
                if (sheet_anim_sel == i) {
                    {   char nb[128]; strncpy(nb, an.name.c_str(), 127); nb[127] = 0;
                        ImGui::SetNextItemWidth(160);
                        if (ImGui::InputText("nombre##an", nb, sizeof(nb))) an.name = nb; }
                    // ---- SUS fotogramas, para montarla a mano ----
                    // Se ven en miniatura y en su orden; se anaden los elegidos en
                    // la hoja, se quitan y se mueven de sitio.
                    ImGui::BeginDisabled(sheet_sel.empty());
                    if (ImGui::SmallButton("Anadir los elegidos"))
                        for (int fr2 : sheet_sel) an.frames.push_back(fr2);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Quitar los elegidos")) {
                        std::vector<int> q;
                        for (int fr2 : an.frames)
                            if (std::find(sheet_sel.begin(), sheet_sel.end(), fr2) == sheet_sel.end())
                                q.push_back(fr2);
                        an.frames.swap(q);
                    }
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Vaciar")) an.frames.clear();
                    if (im && !an.frames.empty()) {
                        ImGui::BeginChild("fot_de_la_anim", ImVec2(0, 88), true,
                                          ImGuiWindowFlags_HorizontalScrollbar);
                        ImDrawList* dl2 = ImGui::GetWindowDrawList();
                        int quitar = -1, mover = 0, movido = -1;
                        for (int q = 0; q < (int)an.frames.size(); q++) {
                            int fi = an.frames[q];
                            if (fi < 0 || fi >= (int)sheet.frames.size()) continue;
                            const SprFrame& f3 = sheet.frames[fi];
                            ImGui::PushID(5000 + q);
                            if (q) ImGui::SameLine(0.0f, 6.0f);
                            ImGui::BeginGroup();
                            ImVec2 c0 = ImGui::GetCursorScreenPos();
                            const float lado = 46.0f;
                            bool clic = ImGui::InvisibleButton("m", ImVec2(lado, lado));
                            bool sobre2 = ImGui::IsItemHovered();
                            dl2->AddRectFilled(c0, ImVec2(c0.x + lado, c0.y + lado),
                                               IM_COL32(28, 32, 40, 255));
                            float k2 = (lado - 6.0f) / (float)((f3.w > f3.h) ? f3.w : f3.h);
                            if (k2 > 4.0f) k2 = 4.0f;
                            float iw = f3.w * k2, ih = f3.h * k2;
                            ImVec2 q0(c0.x + (lado - iw) * 0.5f, c0.y + (lado - ih) * 0.5f);
                            dl2->AddImage((ImTextureID)(intptr_t)im->tex, q0,
                                          ImVec2(q0.x + iw, q0.y + ih),
                                          ImVec2(f3.x / (float)sheet.w, f3.y / (float)sheet.h),
                                          ImVec2((f3.x + f3.w) / (float)sheet.w,
                                                 (f3.y + f3.h) / (float)sheet.h));
                            dl2->AddRect(c0, ImVec2(c0.x + lado, c0.y + lado),
                                         sobre2 ? IM_COL32(120, 200, 255, 255) : IM_COL32(70, 78, 92, 255));
                            char num[16]; snprintf(num, sizeof(num), "%d", fi);
                            dl2->AddText(ImVec2(c0.x + 3, c0.y + lado - 14),
                                         IM_COL32(200, 205, 215, 255), num);
                            if (clic) { sheet_sel.clear(); sheet_sel.push_back(fi); }
                            if (ImGui::SmallButton("<")) { mover = -1; movido = q; }
                            ImGui::SameLine();
                            if (ImGui::SmallButton("x")) quitar = q;
                            ImGui::SameLine();
                            if (ImGui::SmallButton(">")) { mover = 1; movido = q; }
                            ImGui::EndGroup();
                            ImGui::PopID();
                        }
                        if (quitar >= 0) an.frames.erase(an.frames.begin() + quitar);
                        else if (mover && movido >= 0) {
                            int j2 = movido + mover;
                            if (j2 >= 0 && j2 < (int)an.frames.size())
                                std::swap(an.frames[movido], an.frames[j2]);
                        }
                        ImGui::EndChild();
                    } else if (an.frames.empty()) {
                        ImGui::TextDisabled("(vacia: elige fotogramas en la hoja y dale a Anadir)");
                    }
                    // Retoques rapidos: la deteccion deja el trabajo hecho, pero
                    // alguna banda junta dos animaciones o parte una de mas.
                    ImGui::SetNextItemWidth(80);
                    ImGui::InputInt("cada", &sheet_split_n, 0);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Partir") && sheet_split_n > 0 &&
                        (int)an.frames.size() > sheet_split_n) {
                        std::vector<SprAnim> nuevas;
                        for (size_t k = 0; k < an.frames.size(); k += sheet_split_n) {
                            SprAnim n2; n2.fps = an.fps;
                            n2.name = an.name + "_" + std::to_string((int)(k / sheet_split_n) + 1);
                            for (size_t q = k; q < an.frames.size() &&
                                 q < k + (size_t)sheet_split_n; q++)
                                n2.frames.push_back(an.frames[q]);
                            nuevas.push_back(n2);
                        }
                        sheet.anims.erase(sheet.anims.begin() + i);
                        sheet.anims.insert(sheet.anims.begin() + i, nuevas.begin(), nuevas.end());
                        sheet_anim_sel = i; ImGui::PopID(); break;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Unir con la siguiente") && i + 1 < (int)sheet.anims.size()) {
                        for (int fr2 : sheet.anims[i + 1].frames) an.frames.push_back(fr2);
                        sheet.anims.erase(sheet.anims.begin() + i + 1);
                        ImGui::PopID(); break;
                    }
                    ImGui::SetNextItemWidth(120);
                    ImGui::SliderInt("fps", &an.fps, 1, 30);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Rehacer con los elegidos") && !sheet_sel.empty()) {
                        an.frames = sheet_sel;
                        std::sort(an.frames.begin(), an.frames.end());
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Borrar")) {
                        sheet.anims.erase(sheet.anims.begin() + i);
                        sheet_anim_sel = -1; ImGui::PopID(); break;
                    }
                    // previo en marcha: se ve la animacion tal cual quedara
                    if (im && !an.frames.empty()) {
                        sheet_anim_t += ImGui::GetIO().DeltaTime * an.fps;
                        int k = an.frames[((int)sheet_anim_t) % (int)an.frames.size()];
                        if (k >= 0 && k < (int)sheet.frames.size()) {
                            const SprFrame& f2 = sheet.frames[k];
                            float z = 3.0f;
                            ImGui::Image((ImTextureID)(intptr_t)im->tex,
                                         ImVec2(f2.w * z, f2.h * z),
                                         ImVec2(f2.x / (float)sheet.w, f2.y / (float)sheet.h),
                                         ImVec2((f2.x + f2.w) / (float)sheet.w,
                                                (f2.y + f2.h) / (float)sheet.h));
                        }
                    }
                }
                ImGui::PopID();
            }
            if (sheet.anims.empty())
                ImGui::TextDisabled("(elige fotogramas arriba y crea una animacion)");
        }

        // ---- los sprites ya puestos en la escena ----
        ImGui::SeparatorText("Colocados en la escena");
        if (tool != T_SPRITE) {
            if (ImGui::Button(ICON_FA_PERSON_RUNNING " Colocar en la escena")) tool = T_SPRITE;
            ImGui::SameLine(); ImGui::TextDisabled("(usa la hoja abierta)");
        } else ImGui::TextColored(ImVec4(0.5f,0.8f,1,1), "Clic en la escena para colocar");
        ImGui::BeginChild("lista_spr", ImVec2(0, 90), true);
        for (int i = 0; i < (int)sprites.size(); i++) {
            std::string et = sprites[i].name + "   (" + sprites[i].sheet + ")";
            // La seleccion es UNA: al coger un personaje se suelta el objeto, o el
            // Inspector y el gizmo seguirian atendiendo al otro.
            if (ImGui::Selectable(et.c_str(), spr_sel == i && obj_sel < 0)) { spr_sel = i; obj_sel = -1; }
        }
        if (sprites.empty()) ImGui::TextDisabled("(ninguno)");
        ImGui::EndChild();

        if (spr_sel >= 0 && spr_sel < (int)sprites.size()) {
            SprObj& o = sprites[spr_sel];
            SheetDef* sh = sheet_of(o.sheet);
            {   char b[128]; strncpy(b, o.name.c_str(), 127); b[127] = 0;
                if (ImGui::InputText("Nombre (PROCESS)", b, sizeof(b))) o.name = b; }
            // hoja
            {
                std::vector<std::string> imgs;
                for (auto& fgx : hud_gfx_files) if (!hud_is_fpg(fgx)) imgs.push_back(fgx);
                if (ImGui::BeginCombo("Hoja", o.sheet.c_str())) {
                    for (auto& im2 : imgs)
                        if (ImGui::Selectable(im2.c_str(), im2 == o.sheet)) { o.sheet = im2; o.anim.clear(); }
                    ImGui::EndCombo();
                }
            }
            // animacion de esa hoja
            if (sh) {
                const char* cur = o.anim.empty() ? "(primer fotograma)" : o.anim.c_str();
                if (ImGui::BeginCombo("Animacion", cur)) {
                    if (ImGui::Selectable("(primer fotograma)", o.anim.empty())) o.anim.clear();
                    for (auto& an : sh->anims)
                        if (ImGui::Selectable(an.name.c_str(), an.name == o.anim)) o.anim = an.name;
                    ImGui::EndCombo();
                }
                if (o.dirs > 1) {
                    for (auto& an : sh->anims) if (an.name == o.anim) {
                        int pasos = (int)an.frames.size() / o.dirs;
                        if (pasos * o.dirs != (int)an.frames.size())
                            ImGui::TextColored(ImVec4(1,0.6f,0.4f,1),
                                "La animacion tiene %d fotogramas: no es multiplo de %d direcciones.",
                                (int)an.frames.size(), o.dirs);
                        else
                            ImGui::TextDisabled("%d pasos por direccion (la animacion va por filas: "
                                                "primero todos los de una direccion)", pasos);
                    }
                }
            } else ImGui::TextColored(ImVec4(1,0.5f,0.4f,1), "No pude leer esa hoja.");
            {   // fps de la animacion, aqui mismo: no hay que ir a la hoja para
                // ajustar como de rapido se mueve ESTE personaje.
                bool propio = (o.fps > 0);
                if (ImGui::Checkbox("fps propios", &propio)) o.fps = propio ? 10 : 0;
                if (o.fps > 0) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(140);
                    ImGui::SliderInt("fotogramas/seg", &o.fps, 1, 30);
                } else {
                    ImGui::SameLine();
                    int f0 = 10;
                    if (sh) for (auto& an : sh->anims)
                        if (an.name == (o.is_player ? (o.an_walk.empty() ? o.an_idle : o.an_walk) : o.anim))
                            { f0 = an.fps; break; }
                    ImGui::TextDisabled("(los de la animacion: %d)", f0);
                }
            }
            ImGui::DragFloat("Alto (unidades)", &o.height, 0.05f, 0.1f, 60.0f, "%.2f");
            const char* dirs_txt[] = { "1 (siempre el mismo)", "4 direcciones", "8 direcciones", "16 direcciones" };
            int di = (o.dirs == 16) ? 3 : (o.dirs == 8) ? 2 : (o.dirs == 4) ? 1 : 0;
            if (ImGui::Combo("Posturas", &di, dirs_txt, 4))
                o.dirs = (di == 3) ? 16 : (di == 2) ? 8 : (di == 1) ? 4 : 1;
            const char* bb[] = { "De pie (personajes)", "De cara del todo (items, efectos)" };
            ImGui::Combo("Encarado", &o.billboard, bb, 2);
            bool sh_on = o.shadow != 0, sm = o.smooth != 0, il = o.iluminado != 0;
            if (ImGui::Checkbox("Hace sombra", &sh_on)) o.shadow = sh_on;
            ImGui::SameLine();
            if (ImGui::Checkbox("Suavizado", &sm)) o.smooth = sm;
            if (ImGui::Checkbox("Se ilumina con la escena", &il)) o.iluminado = il;
            ImGui::SameLine();
            {   bool ap = o.ajuste_px != 0;
                if (ImGui::Checkbox("Ajuste a pixel", &ap)) o.ajuste_px = ap;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Redondea su posicion a pixeles enteros de pantalla:\n"
                                      "el pixel art deja de temblar al moverse.");
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Se apaga de noche y con el ciclo de dia, como el resto\n"
                                  "del mundo. Sin marcar, va siempre a pleno color.");
            ImGui::DragFloat("Recorte alfa", &o.cutout, 0.01f, 0.0f, 0.99f, "%.2f");
            ImGui::DragFloat3("Posicion", &o.x, 0.1f);

            // ---- lo mismo que un objeto 3D: fisica, jugador y zonas ----
            auto combo_tecla = [&](const char* et, std::string& tecla) { combo_tecla_ui(et, tecla); };
            // Animacion de la hoja y boton del mando: los usan tanto el jugador como
            // los NPC, asi que se definen antes de partir en dos la ficha.
            auto combo_anim_de = [&](const char* et, std::string& dest) {
                const char* cur = dest.empty() ? "(ninguna)" : dest.c_str();
                ImGui::SetNextItemWidth(150);
                if (ImGui::BeginCombo(et, cur)) {
                    if (ImGui::Selectable("(ninguna)", dest.empty())) dest.clear();
                    if (sh) for (auto& an : sh->anims)
                        if (ImGui::Selectable(an.name.c_str(), an.name == dest)) dest = an.name;
                    ImGui::EndCombo();
                }
            };
            auto combo_boton = [&](const char* et, std::string& b) { combo_boton_ui(et, b); };
            ImGui::SeparatorText("Que es y como se comporta");
            bool jug = o.is_player != 0;
            if (ImGui::Checkbox("Es el personaje que se controla", &jug)) {
                o.is_player = jug;
                if (jug) { o.phys = 0; spr_follow = spr_sel; }   // el jugador lleva char controller
                else if (spr_follow == spr_sel) spr_follow = -1;
            }
            if (o.is_player) {
                bool sigue = (spr_follow == spr_sel);
                if (ImGui::Checkbox("La camara le sigue", &sigue))
                    spr_follow = sigue ? spr_sel : -1;
                ImGui::DragFloat("Velocidad andando", &o.walk_speed, 0.1f, 0.5f, 80.0f, "%.1f");
                ImGui::DragFloat("Velocidad corriendo", &o.run_speed, 0.1f, 0.5f, 120.0f, "%.1f");
                ImGui::DragFloat("Fuerza del salto", &o.jump_force, 0.1f, 0.0f, 60.0f, "%.1f");
                ImGui::DragFloat("Radio de colision", &o.char_radius, 0.05f, 0.1f, 10.0f, "%.2f");
                ImGui::DragFloat("Altura de colision", &o.char_height, 0.05f, 0.2f, 20.0f, "%.2f");
                if (ImGui::TreeNode("Teclas y su animacion")) {
                    // Cada tecla con SU animacion al lado: mientras la tengas
                    // pulsada se reproduce esa. Es como se hace un juego 2D de
                    // siempre (una animacion por direccion).
                    auto fila_tecla = [&](const char* et, std::string& tecla,
                                          std::string* anim, int* espejo) {
                        ImGui::PushID(et);
                        ImGui::SetNextItemWidth(120);
                        combo_tecla("##k", tecla);
                        ImGui::SameLine();
                        if (anim) combo_anim_de("##a", *anim);
                        else { ImGui::SetNextItemWidth(150); ImGui::TextDisabled("(sin animacion propia)"); }
                        if (espejo) {
                            ImGui::SameLine();
                            bool e2 = (*espejo != 0);
                            if (ImGui::Checkbox("espejo", &e2)) *espejo = e2 ? 1 : 0;
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Dibuja la animacion volteada.\n"
                                                  "Con una sola de 'andar a la izquierda' ya tienes la derecha.");
                        }
                        ImGui::SameLine();
                        ImGui::TextUnformatted(et);
                        ImGui::PopID();
                    };
                    fila_tecla("Adelante", o.k_up, &o.an_up, &o.fx_up);
                    fila_tecla("Atras", o.k_down, &o.an_down, &o.fx_down);
                    fila_tecla("Izquierda", o.k_left, &o.an_left, &o.fx_left);
                    fila_tecla("Derecha", o.k_right, &o.an_right, &o.fx_right);
                    fila_tecla("Saltar", o.k_jump, &o.an_jump, nullptr);
                    fila_tecla("Correr", o.k_run, nullptr, nullptr);
                    ImGui::TextDisabled("Teclas: constantes de BennuGD2 (key(_W)...).\n"
                                        "Animacion vacia = se usa la de 'lo que hace' (abajo);\n"
                                        "'espejo' vale igual con animacion propia o sin ella.");
                    ImGui::SeparatorText("Mando");
                    bool um = o.usar_mando != 0;
                    if (ImGui::Checkbox("Moverse con el stick y la cruceta", &um)) o.usar_mando = um;
                    combo_boton("Boton de saltar", o.b_jump);
                    combo_boton("Boton de correr", o.b_run);
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Acciones (las que tu quieras)")) {
                    ImGui::TextDisabled("Cada accion se dispara con una tecla, con un boton del\n"
                                        "mando o con los dos, y hace sonar una animacion y/o\n"
                                        "llamar a un PROCESS o FUNCTION tuyo.");
                    if (ImGui::Button(ICON_FA_PLUS " Anadir accion")) {
                        SprAccion ac;
                        ac.nombre = "accion" + std::to_string((int)o.acciones.size() + 1);
                        o.acciones.push_back(ac);
                    }
                    int borrar_acc = -1;
                    for (int q = 0; q < (int)o.acciones.size(); q++) {
                        SprAccion& ac = o.acciones[q];
                        ImGui::PushID(9000 + q);
                        ImGui::Separator();
                        {   char nb[64]; strncpy(nb, ac.nombre.c_str(), 63); nb[63] = 0;
                            ImGui::SetNextItemWidth(120);
                            if (ImGui::InputText("##nom", nb, sizeof(nb))) ac.nombre = nb; }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("quitar")) borrar_acc = q;
                        {   bool con_tecla = !ac.tecla.empty();
                            if (ImGui::Checkbox("tecla", &con_tecla))
                                ac.tecla = con_tecla ? "_K" : "";
                            if (con_tecla) {
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(110);
                                combo_tecla("##t", ac.tecla);
                            }
                        }
                        combo_boton("boton del mando", ac.boton);
                        combo_anim_de("animacion", ac.anim);
                        bool e2 = ac.espejo != 0;
                        if (ImGui::Checkbox("espejo", &e2)) ac.espejo = e2 ? 1 : 0;
                        ImGui::SameLine();
                        bool uv = ac.una_vez != 0;
                        if (ImGui::Checkbox("suena entera al pulsar", &uv)) ac.una_vez = uv ? 1 : 0;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Marcado: se reproduce del tiron y no se corta (ataques).\n"
                                              "Sin marcar: suena mientras aguantes la tecla o el boton.");
                        {   // que suelte un dialogo, para lo de "hablar" sin escribir codigo
                            const char* cur = ac.dialogo.empty() ? "(ninguno)" : ac.dialogo.c_str();
                            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                            if (ImGui::BeginCombo("Dialogo que saca", cur)) {
                                if (ImGui::Selectable("(ninguno)", ac.dialogo.empty())) ac.dialogo.clear();
                                for (auto& dd : dialogos)
                                    if (ImGui::Selectable(dd.nombre.c_str(), dd.nombre == ac.dialogo))
                                        ac.dialogo = dd.nombre;
                                ImGui::EndCombo();
                            }
                        }
                        selector_codigo("codaccion", ac.archivo, ac.llama, ac.nombre);
                        ImGui::PopID();
                    }
                    if (borrar_acc >= 0) o.acciones.erase(o.acciones.begin() + borrar_acc);
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Animaciones segun lo que hace")) {
                    auto combo_anim = [&](const char* et, std::string& dest) {
                        const char* cur = dest.empty() ? "(ninguna)" : dest.c_str();
                        if (ImGui::BeginCombo(et, cur)) {
                            if (ImGui::Selectable("(ninguna)", dest.empty())) dest.clear();
                            if (sh) for (auto& an : sh->anims)
                                if (ImGui::Selectable(an.name.c_str(), an.name == dest)) dest = an.name;
                            ImGui::EndCombo();
                        }
                    };
                    combo_anim("Quieto", o.an_idle);
                    combo_anim("Andando", o.an_walk);
                    combo_anim("Corriendo", o.an_run);
                    combo_anim("Saltando", o.an_jump);
                    ImGui::TextDisabled("Sin animacion para un estado se usa la de 'Quieto'.");
                    ImGui::TreePop();
                }
            } else {
                const char* fis[] = { "Ninguna (decorativo)", "Caja", "Esfera", "Capsula",
                                      "Cilindro", "Muro invisible (no se mueve)" };
                ImGui::Combo("Colision", &o.phys, fis, 6);
                if (o.phys >= 1 && o.phys <= 4) {
                    ImGui::DragFloat("Masa (0 = fijo)", &o.mass, 0.1f, 0.0f, 500.0f, "%.2f");
                    ImGui::DragFloat("Rebote", &o.bounce, 0.01f, 0.0f, 1.0f, "%.2f");
                    ImGui::DragFloat("Friccion", &o.friction, 0.01f, 0.0f, 2.0f, "%.2f");
                    bool fl = o.buoyant != 0;
                    if (ImGui::Checkbox("Flota en el agua", &fl)) o.buoyant = fl;
                    if (o.buoyant)
                        ImGui::DragFloat("Densidad (0.5 = medio hundido)", &o.density, 0.01f, 0.05f, 1.0f, "%.2f");
                }
                if (o.phys >= 1)
                    ImGui::DragFloat("Tamano de la colision", &o.csize, 0.05f, 0.1f, 40.0f, "%.2f");

                // ---- NPC: que estorbe y que se pueda hablar con el ----
                ImGui::SeparatorText("Como NPC");
                bool sol = o.solido != 0;
                if (ImGui::Checkbox("Bloquea el paso (le sigue la colision)", &sol)) o.solido = sol;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Una caja de colision pegada al sprite, que se mueve con el.\n"
                                      "Los muros invisibles son fijos; esto vale para NPC que andan.");
                if (o.solido)
                    ImGui::DragFloat("Radio que ocupa", &o.sol_radio, 0.05f, 0.1f, 20.0f, "%.2f");

                {   const char* comps[] = { "Quieto", "Patrulla entre A y B",
                                            "Sigue al jugador", "Huye del jugador" };
                    ImGui::Combo("Comportamiento", &o.comport, comps, 4);
                    if (o.comport > 0) {
                        ImGui::DragFloat("Velocidad", &o.com_vel, 0.1f, 0.1f, 40.0f, "%.1f");
                        combo_anim_de("Animacion al moverse", o.an_walk);
                        if (o.comport == 1) {
                            ImGui::DragFloat2("Punto B (x,z)", &o.com_bx, 0.1f);
                            if (ImGui::SmallButton("Poner B donde mira la camara")) {
                                o.com_bx = vcam_target[0]; o.com_bz = vcam_target[2];
                            }
                            ImGui::TextDisabled("A es donde esta puesto ahora mismo.");
                        } else {
                            ImGui::DragFloat("Se entera a", &o.com_radio, 0.5f, 1.0f, 200.0f, "%.0f");
                        }
                    }
                }
                bool inter = o.inter_on != 0;
                if (ImGui::Checkbox("Se puede interactuar (acercarse y pulsar)", &inter)) o.inter_on = inter;
                if (o.inter_on) {
                    ImGui::DragFloat("Distancia para poder", &o.inter_radio, 0.1f, 0.5f, 40.0f, "%.1f");
                    ImGui::SetNextItemWidth(120);
                    combo_tecla("Tecla", o.inter_tecla);
                    combo_boton("Boton del mando", o.inter_boton);
                    combo_anim_de("Animacion al hacerlo", o.inter_anim);
                    /* Lo normal al hablar con alguien es que DIGA algo, con su
                       bocadillo. Antes esto solo sabia llamar a codigo tuyo, y el
                       texto pelado de las reglas sale sin caja. */
                    {   const char* cur = o.inter_dialogo.empty() ? "(ninguno)" : o.inter_dialogo.c_str();
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                        if (ImGui::BeginCombo("Dialogo que suelta", cur)) {
                            if (ImGui::Selectable("(ninguno)", o.inter_dialogo.empty())) o.inter_dialogo.clear();
                            for (auto& dd : dialogos)
                                if (ImGui::Selectable(dd.nombre.c_str(), dd.nombre == o.inter_dialogo))
                                    o.inter_dialogo = dd.nombre;
                            ImGui::EndCombo();
                        }
                        if (dialogos.empty())
                            ImGui::TextDisabled("  (aun no hay dialogos: Ventana > Dialogos)");
                    }
                    ImGui::TextDisabled("Y ademas, si quieres, tu propio codigo:");
                    selector_codigo("codinter", o.inter_arch, o.inter_llama, o.name + "_hablar");
                    bool mir = o.inter_mirar != 0;
                    if (ImGui::Checkbox("Se gira hacia el jugador al acercarse", &mir)) o.inter_mirar = mir;
                    ImGui::TextDisabled("Si el PROCESS no existe, el editor te crea el esqueleto.");
                }
            }
            {   // zonas de barrera pintadas (igual que en los objetos)
                int zl = o.zone_layer + 1;
                const char* zz[] = { "(ninguna)", "Capa 0", "Capa 1", "Capa 2", "Capa 3" };
                if (ImGui::Combo("No puede entrar en", &zl, zz, 5)) o.zone_layer = zl - 1;
            }

            if (ImGui::Button("Ir a el")) {
                vcam_target[0] = o.x; vcam_target[1] = o.y + o.height * 0.5f; vcam_target[2] = o.z;
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicar")) {
                // Con diez NPC iguales, configurar cada uno a mano es un castigo.
                SprObj c = o;
                c.entity = -1;                    // el suyo, no el del original
                c.is_player = 0;                  // solo puede haber un jugador
                c.name = o.name + "_2";
                for (int k = 2; ; k++) {
                    bool rep = false;
                    for (auto& q : sprites) if (q.name == c.name) { rep = true; break; }
                    if (!rep) break;
                    c.name = o.name + "_" + std::to_string(k + 1);
                }
                c.x += 1.5f;
                sprites.push_back(c);
                spr_sel = (int)sprites.size() - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Borrar")) {
                if (o.entity >= 0) g3d_sprite_destroy(o.entity);
                sprites.erase(sprites.begin() + spr_sel);
                spr_sel = -1;
            }
        }
        if (spr_dos_columnas) ImGui::EndTable();
        // ---- guardado automatico ----
        // En cuanto sueltas el raton y la hoja ha cambiado, se escribe su .sheet.
        // Antes habia que acordarse de "Guardar hoja", y si cerrabas sin darle
        // perdias las animaciones que habias montado.
        if (!sheet.image.empty() && !sheet.frames.empty() &&
            !ImGui::IsAnyItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            size_t fa = sheet_firma();
            if (fa != sheet_firma_guardada) {
                sheet_save();
                sheet_firma_guardada = fa;
                sheet_cache[sheet.image] = sheet;   // y los sprites colocados, al dia
            }
        }
        ImGui::End();
        }

        // --- Panel: Entorno (agua / mar / lago) ---
        ImGui::Begin("Entorno");
        ImGui::PushTextWrapPos(0.0f);   // que el texto se parta y no se corte
        ImGui::TextDisabled("Terreno y agua auto: menu 'Terreno' (arriba).");

        /* ---- SONIDO de la escena ----
           La musica de fondo (una por escena: SDL_mixer solo toca una a la vez) y
           los ambientes que suenan mientras el jugador esta dentro de una zona
           pintada -- lluvia, bosque, cueva. Los golpes y los sonidos de una
           cascada concreta van por otro lado: en las reglas y en el objeto. */
        if (ImGui::CollapsingHeader(ICON_FA_MUSIC "  Sonido")) {
            auto combo_fichero = [&](const char* et, std::string& dest,
                                     const std::vector<std::string>& lista) {
                const char* cur = dest.empty() ? "(ninguno)" : dest.c_str();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                if (ImGui::BeginCombo(et, cur)) {
                    if (ImGui::Selectable("(ninguno)", dest.empty())) dest.clear();
                    for (auto& m : lista)
                        if (ImGui::Selectable(m.c_str(), m == dest)) dest = m;
                    ImGui::EndCombo();
                }
            };
            ImGui::SeparatorText("Musica de esta escena");
            combo_fichero("musica", esc_musica, musicas);
            aviso_audio(esc_musica);
            if (musicas.empty())
                ImGui::TextDisabled("Pon los ficheros en Assets/Music (ogg, mp3, mod, xm...)");
            if (!esc_musica.empty()) {
                ImGui::SliderInt("Volumen", &esc_mus_vol, 0, 128);
                bool lp = esc_mus_loop != 0;
                if (ImGui::Checkbox("En bucle", &lp)) esc_mus_loop = lp ? 1 : 0;
                ImGui::SliderFloat("Entra (s)", &esc_mus_fade, 0.0f, 8.0f, "%.1f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("0 = arranca de golpe. Con un par de segundos\nentra sola y no pega un salto al empezar.");
            }
            ImGui::SeparatorText("Ambiente por zona");
            ImGui::TextWrapped("Suena mientras el jugador esta dentro de la zona pintada.");
            int quitar = -1;
            for (int i = 0; i < (int)zsonidos.size(); i++) {
                ImGui::PushID(3000 + i);
                const char* zz[] = { "Capa 0", "Capa 1", "Capa 2", "Capa 3" };
                ImGui::SetNextItemWidth(110);
                ImGui::Combo("zona", &zsonidos[i].zona, zz, 4);
                ImGui::SameLine();
                if (ImGui::SmallButton("Quitar")) quitar = i;
                combo_fichero("sonido", zsonidos[i].sonido, sonidos);
                ImGui::SliderInt("volumen", &zsonidos[i].vol, 0, 128);
                ImGui::Separator();
                ImGui::PopID();
            }
            if (quitar >= 0) zsonidos.erase(zsonidos.begin() + quitar);
            if (ImGui::Button(ICON_FA_PLUS "  Anadir ambiente de zona", ImVec2(-1, 0)))
                zsonidos.push_back(ZonaSonido());
            if (sonidos.empty())
                ImGui::TextDisabled("Pon los ficheros en Assets/Sounds (wav, ogg).");
        }
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
        if (ImGui::TreeNode("Espuma y salpicaduras")) {
            ImGui::SliderFloat("Espuma", &water_foam, 0.0f, 1.5f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Cuanta espuma lleva el agua. Pasado cierto punto tapa\n"
                                  "el color del agua y el mar se vuelve una sabana blanca.");
            ImGui::SliderFloat("Salpicaduras", &splash_amount, 0.0f, 3.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Gotas al pie de las cascadas y contra las rocas.\n"
                                  "A 0 no salpica, pero el agua sigue rodeando las rocas:\n"
                                  "el desvio es simulacion, las gotas son adorno.");
            ImGui::SliderFloat("Corriente minima", &splash_speed, 0.0f, 4.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lo rapido que tiene que ir el agua para salpicar en\n"
                                  "una roca. Subelo si una balsa quieta burbujea.");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Olas de playa", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Intensidad##surf", &surf_amount, 0.0f, 2.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0 = sin rompientes ni espuma en la orilla.");
            ImGui::SliderFloat("Separacion##surf", &surf_len, 2.0f, 30.0f, "%.1f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Distancia entre rompientes, medida en PROFUNDIDAD:\n"
                                  "una playa mas tendida las separa mas, como una de verdad.");
            ImGui::SliderFloat("Velocidad##surf", &surf_speed, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Subida por la arena", &surf_runup, 0.0f, 8.0f, "%.1f");
            ImGui::SliderFloat("Altura de cresta", &surf_height, 0.0f, 3.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Altura real de la ola: la cresta se levanta y se\n"
                                  "apunta al llegar al bajio, como una ola de verdad.");
            ImGui::SliderFloat("Direccion", &surf_dir, -180.0f, 180.0f, "%.0f grados");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("De donde viene la mar. Solo manda en aguas abiertas:\n"
                                  "al notar el fondo las olas refractan y se giran solas\n"
                                  "para llegar paralelas a la orilla, como las reales.");
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
                    /* Desde donde se mira al personaje. Estaba clavada a su
                       espalda, y eso deja fuera un plataformas 2.5D, que quiere
                       verlo de perfil. */
                    ImGui::SliderFloat("Angulo", &cam_orbit, 0.0f, 360.0f, "%.0f grados");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("0 = a la espalda (lo de siempre)\n"
                                          "90 / 270 = de PERFIL, para un 2.5D\n"
                                          "180 = de frente");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Perfil")) { cam_orbit = 90.0f; cam_25d = true; }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Espalda")) { cam_orbit = 0.0f; cam_25d = false; }
                    /* Girar la camara no basta: con los controles pegados a los ejes
                       del mundo, de perfil la D llevaba al personaje hacia el fondo.
                       Esto ata los controles a la pantalla y, ademas, bloquea la
                       profundidad, que es lo que hace que sea un plataformas. */
                    {   // girar la camara mientras se juega
                        bool gir = cam_girable != 0;
                        if (ImGui::Checkbox("El jugador puede girar la camara", &gir))
                            cam_girable = gir ? 1 : 0;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("El angulo de arriba pasa a ser el INICIAL, y en el juego\n"
                                              "se gira. Los controles giran con ella: la D siempre\n"
                                              "lleva a la derecha de la pantalla.");
                        if (cam_girable) {
                            const char* cc[] = { "con el raton", "con las teclas Q y E",
                                                 "con el stick derecho del mando" };
                            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
                            ImGui::Combo("Se gira", &cam_gira_con, cc, 3);
                            if (cam_gira_con == 0)
                                ImGui::SliderFloat("Sensibilidad", &cam_sens, 20.0f, 400.0f, "%.0f");
                            else
                                ImGui::SliderFloat("Grados por segundo", &cam_gira_vel, 20.0f, 540.0f, "%.0f");
                            ImGui::TextDisabled("Ojo: entra en la plantilla del jugador al crearla.\nSi su script ya existe, regeneralo.");
                        }
                    }
                    ImGui::Checkbox("Plataformas 2.5D (bloquear profundidad)", &cam_25d);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Solo izquierda/derecha y salto: el personaje no puede\n"
                                          "irse al fondo. Los controles se generan RESPECTO A LA\n"
                                          "CAMARA, asi que la D siempre lleva a la derecha de la\n"
                                          "pantalla, mires desde donde mires.\n"
                                          "Ojo: afecta a la plantilla del jugador al CREARLA. Si su\n"
                                          "script ya existe, borralo para regenerarlo.");
                }
            }
            if (cam_follow < 0)
                ImGui::TextColored(ImVec4(1,0.7f,0.2f,1), "Elige un objeto a seguir");
        }
        }   // fin CollapsingHeader Camara principal

        ImGui::PopTextWrapPos();
        ImGui::End();

        // --- Panel: Jerarquia (objetos de la escena) ---
        ImGui::Begin("Jerarquia");
        ImGui::PushTextWrapPos(0.0f);   // que el texto se parta y no se corte
        ImGui::TextDisabled("Objetos: %d", (int)objects.size());
        ImGui::Separator();
        // Con personajes en la escena, la lista de objetos se queda con la mitad:
        // antes se comia todo el alto y la de personajes caia fuera de la ventana.
        ImGui::BeginChild("lista_obj", ImVec2(0, sprites.empty() ? 0.0f
                                                 : ImGui::GetContentRegionAvail().y * 0.5f));
        int pedir_borrar = -1;                 // no se borra dentro del bucle: invalidaria el recorrido
        for (int i = 0; i < (int)objects.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::Selectable(objects[i].name.c_str(), obj_sel == i)) { obj_sel = i; spr_sel = -1; }
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

        /* ---- Los personajes 2D son objetos de la escena como los demas ----
           Estaban solo en su ventana flotante, asi que para tocar uno habia que
           abrirla y buscarlo en su lista. Aqui se eligen igual que un modelo 3D:
           la seleccion es una sola, o un objeto o un sprite, para que el
           Inspector y el gizmo sepan siempre a quien hacen caso. */
        if (!sprites.empty()) {
            ImGui::Separator();
            ImGui::TextDisabled("Personajes 2D: %d", (int)sprites.size());
            ImGui::BeginChild("lista_spr_jer");
            int borrar_spr = -1;
            for (int i = 0; i < (int)sprites.size(); i++) {
                ImGui::PushID(1000 + i);
                std::string et = std::string(ICON_FA_PERSON_RUNNING "  ") + sprites[i].name;
                if (ImGui::Selectable(et.c_str(), spr_sel == i && obj_sel < 0)) {
                    spr_sel = i; obj_sel = -1;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    spr_sel = i; obj_sel = -1;
                    show_spr_win = true;            // doble clic: su ficha entera
                }
                if (ImGui::IsItemHovered() && !sprites[i].sheet.empty())
                    ImGui::SetTooltip("%s", sprites[i].sheet.c_str());
                if (ImGui::BeginPopupContextItem("ctx_spr")) {
                    spr_sel = i; obj_sel = -1;
                    ImGui::TextDisabled("%s", sprites[i].name.c_str());
                    ImGui::Separator();
                    if (ImGui::MenuItem("Abrir su ficha (hojas y animaciones)")) show_spr_win = true;
                    if (ImGui::MenuItem("Duplicar")) {
                        SprObj c = sprites[i];
                        c.entity = -1;
                        c.name = sprites[i].name + "_2";
                        for (int k = 2; ; k++) {
                            bool rep = false;
                            for (auto& q : sprites) if (q.name == c.name) { rep = true; break; }
                            if (!rep) break;
                            c.name = sprites[i].name + "_" + std::to_string(k + 1);
                        }
                        c.x += 1.5f;
                        sprites.push_back(c);
                        spr_sel = (int)sprites.size() - 1;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Borrar")) borrar_spr = i;
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
            if (borrar_spr >= 0) {
                if (sprites[borrar_spr].entity >= 0) g3d_sprite_destroy(sprites[borrar_spr].entity);
                sprites.erase(sprites.begin() + borrar_spr);
                if (spr_sel >= (int)sprites.size()) spr_sel = (int)sprites.size() - 1;
                if (spr_follow >= (int)sprites.size()) spr_follow = -1;
            }
        }
        ImGui::PopTextWrapPos();
        ImGui::End();

        // --- Panel: Inspector (del objeto seleccionado / pincel de terreno) ---
        bool borrar_sel = false;   // se borra al cerrar el panel: dentro invalidaria `o`
        ImGui::Begin("Inspector");
        ImGui::PushTextWrapPos(0.0f);   // que el texto se parta y no se corte
        if (tool == T_HOLE) {
            ImGui::SeparatorText("Agujero de terreno");
            ImGui::SliderFloat("Radio", &brush_r, 3.0f, 60.0f, "%.0f");
            ImGui::Checkbox("Rellenar (quitar agujero)", &hole_fill);
            ImGui::TextWrapped("Perfora el terreno para ver la cueva de debajo. "
                               "Excava primero la cueva (modo cueva).");
            ImGui::Separator();
        }
        {
            ImGui::Checkbox("Mostrar FPS en el juego (depuracion)", &show_fps);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Escribe fps, llamadas de dibujo y triangulos en una\n"
                                  "esquina del juego. Es como se ve QUE cuesta, en vez de\n"
                                  "notar solo que va lento.");
            ImGui::Separator();
            ImGui::SeparatorText(ICON_FA_SUN "  Sol y ciclo dia/noche");
            ImGui::Checkbox("El sol se mueve", &sun_cycle);
            if (sun_cycle) {
                ImGui::SliderFloat("Dura un dia", &sun_day_sec, 10.0f, 900.0f, "%.0f s");
                ImGui::SliderFloat("Hora", &sun_hour, 0.0f, 360.0f, "%.0f grados");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("0 = amanecer, 90 = mediodia, 180 = atardecer,\n"
                                      "270 = medianoche. Es por donde ARRANCA el ciclo.");
            } else {
                ImGui::SliderFloat("Rumbo", &sun_azim, 0.0f, 360.0f, "%.0f grados");
                ImGui::SliderFloat("Altura", &sun_elev, -10.0f, 90.0f, "%.0f grados");
            }
            ImGui::SliderFloat("Intensidad", &sun_intensity, 0.0f, 4.0f, "%.2f");
            ImGui::TextDisabled("Se emite como PROCESS escena_sol() con sus locales.");
            ImGui::Separator();
        }
        if (tool == T_SCATTER) {
            ImGui::SeparatorText(ICON_FA_SEEDLING "  Sembrar");
            if (asset_sel < 0)
                ImGui::TextColored(ImVec4(1,0.7f,0.3f,1),
                                   "Elige antes un asset en el panel de la izquierda.");
            else
                ImGui::Text("Sembrando: %s", assets[asset_sel].c_str());
            /* Uno por linea: el Inspector es estrecho y en una sola fila el
               tercer modo queda pegado al borde y no se encuentra. */
            ImGui::SeparatorText("Modo");
            ImGui::RadioButton("Sembrar##scm", &sc_mode, 0);
            ImGui::RadioButton("Borrar##scm", &sc_mode, 1);
            ImGui::RadioButton("Editar uno (clic para elegir)##scm", &sc_mode, 2);
            ImGui::Separator();
            ImGui::SliderFloat("Radio##sc", &sc_radius, 3.0f, 60.0f, "%.0f");
            ImGui::SliderFloat("Densidad##sc", &sc_density, 1.0f, 40.0f, "%.0f por pincelada");
            ImGui::SliderFloat("Escala min", &sc_scale_min, 0.1f, 3.0f, "%.2f");
            ImGui::SliderFloat("Escala max", &sc_scale_max, 0.1f, 3.0f, "%.2f");
            if (sc_scale_max < sc_scale_min) sc_scale_max = sc_scale_min;
            ImGui::SliderFloat("Pendiente maxima", &sc_slope_max, 0.05f, 2.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("No siembra donde el suelo sea mas empinado que esto.\n"
                                  "Un arbol colgando de un risco delata el sembrado.");
            ImGui::SliderFloat("Altura minima", &sc_ymin, -100.0f, 100.0f, "%.0f");
            ImGui::SliderFloat("Altura maxima", &sc_ymax, -100.0f, 200.0f, "%.0f");
            ImGui::Checkbox("Evitar el agua", &sc_avoid_water);
            ImGui::SliderFloat("Distancia de dibujo", &sc_dist, 40.0f, 1200.0f, "%.0f u");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Mas alla no se dibuja. Es POR ESPECIE: la hierba no\n"
                                  "hace falta verla lejos y un arbol si.");
            if (ImGui::SliderFloat("Distancia de LOD", &sc_lod, 0.0f, 800.0f, "%.0f u"))
                g3d_instances_set_lod_distance(sc_lod);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("A partir de aqui se dibuja con una malla de bajo\n"
                                  "poligono generada sola. 0 = apagado (venia asi).\n"
                                  "Es lo que sostiene un bosque de miles de copias.");
            ImGui::SliderFloat("Viento de esta especie", &sc_wind, 0.0f, 1.5f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Balanceo en el shader: un bosque se mueve sin animar\n"
                                  "un solo arbol. Es POR ESPECIE -- deja 0 para rocas y\n"
                                  "troncos, o se balancearan como si fueran hierba.");
            if (g3d_scatter_kinds() > 0) {
                ImGui::TextDisabled("Sembrado:");
                for (int k = 0; k < g3d_scatter_kinds(); k++) {
                    const char* nm = g3d_scatter_kind_asset(k);
                    /* Editable EN SITIO: antes esto solo se leia, asi que para
                       cambiarle el viento a una especie ya sembrada habia que
                       volver a sembrarla. */
                    ImGui::PushID(k);
                    ImGui::Text("%s  x%d", nm ? nm : "?", g3d_scatter_kind_count(k));
                    float kw = g3d_scatter_get_kind_wind(k);
                    float kd = g3d_scatter_get_kind_distance(k);
                    bool  ks = g3d_scatter_get_kind_solid(k) != 0;
                    bool ch = false;
                    ImGui::SetNextItemWidth(110);
                    ch |= ImGui::SliderFloat("viento", &kw, 0.0f, 1.5f, "%.2f");
                    ImGui::SetNextItemWidth(110);
                    ch |= ImGui::SliderFloat("hasta", &kd, 40.0f, 1200.0f, "%.0f u");
                    ch |= ImGui::Checkbox("solido", &ks);
                    if (ch) {
                        if (ImGui::IsItemActivated()) push_scatter_undo();
                        g3d_scatter_kind_apply(k, kw, kd, ks ? 1 : 0);
                        if (ks != (g3d_scatter_get_kind_solid(k) != 0))
                            g3d_scatter_build(1.0f);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::Checkbox("Bloquea el paso (solido)", &sc_solid);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Pone un colisionador por ejemplar. El motor admite 512\n"
                                  "cajas en total, asi que esto es para troncos y rocas\n"
                                  "grandes -- no para hierba.");
            {
                int sp = g3d_scatter_solid_placed();
                if (sp > 0) ImGui::TextDisabled("%d colisionadores de 512", sp);
            }
            // --- edicion de un ejemplar ---
            if (sc_mode == 2) {
                ImGui::SeparatorText("Ejemplar elegido");
                float v[5];
                if (sc_sel_k >= 0 && g3d_scatter_get(sc_sel_k, sc_sel_i, v)) {
                    ImGui::Text("%s  #%d", g3d_scatter_kind_asset(sc_sel_k), sc_sel_i);
                    bool ch = false;
                    ch |= ImGui::SliderFloat("Tamano##one", &v[4], 0.05f, 6.0f, "%.2f");
                    ch |= ImGui::SliderFloat("Giro##one",   &v[3], 0.0f, 360.0f, "%.0f grados");
                    ch |= ImGui::DragFloat3("Posicion##one", v, 0.1f);
                    if (ch) {
                        /* Solo al EMPEZAR a arrastrar el deslizador: si no, cada
                           pixel de arrastre dejaria su propio paso. */
                        if (ImGui::IsItemActivated()) push_scatter_undo();
                        g3d_scatter_set(sc_sel_k, sc_sel_i, v[0], v[1], v[2], v[3], v[4]);
                        g3d_scatter_build(1.0f);
                    }
                    if (ImGui::Button("Quitar este")) {
                        push_scatter_undo();
                        g3d_scatter_remove(sc_sel_k, sc_sel_i);
                        sc_sel_k = sc_sel_i = -1;
                        g3d_scatter_build(1.0f);
                    }
                } else {
                    ImGui::TextDisabled("Haz clic sobre uno en la escena.");
                }
            }
            ImGui::Text("%d plantados (%d especies)", g3d_scatter_count(), g3d_scatter_kinds());
            if (ImGui::Button("Quitar toda la siembra")) { push_scatter_undo(); g3d_scatter_clear(); g3d_scatter_build(1.0f); }
            ImGui::Separator();
        }
        if (tool == T_VERTEX) {
            ImGui::SeparatorText(ICON_FA_BORDER_ALL "  Vertice a vertice");
            ImGui::TextWrapped("Arrastra un vertice arriba o abajo. El pincel redondea "
                               "todo; esto es lo que da aristas, escalones y crestas.");
            ImGui::RadioButton("Mover vertice", &vx_mode, 0); ImGui::SameLine();
            ImGui::RadioButton("Seleccionar zona", &vx_mode, 1);
            if (vx_mode == 0) {
                ImGui::RadioButton("Altura", &vx_axis, 0); ImGui::SameLine();
                ImGui::RadioButton("Lateral", &vx_axis, 1);
                if (vx_axis == 1) {
                    ImGui::SliderFloat("Tope lateral", &vx_lat, 0.05f, 0.45f, "%.2f de celda");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Un vertice no puede salirse de su celda: la fisica,\n"
                                          "el picado y el agua dan por hecho que la rejilla es\n"
                                          "regular y solo miran la altura. Dentro de ese tope,\n"
                                          "lo que ellos creen y lo que se ve no llega a media\n"
                                          "celda de diferencia.");
                }
            }
            ImGui::Separator();
            ImGui::SliderInt("Densidad", &vx_px, 6, 40, "%d px");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Separacion minima entre lineas. De lejos la rejilla se\n"
                                  "aclara sola; acercandote llega a la rejilla real,\n"
                                  "vertice a vertice.");
            ImGui::SliderInt("Arrastre suave", &vx_soft, 0, 6, "%d vecinos");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0 = solo ese vertice (aristas duras).\n"
                                  "Mas alto arrastra tambien alrededor, para una loma.");
            ImGui::SliderFloat("Sensibilidad", &vx_sens, 0.01f, 0.3f, "%.3f u/pixel");
            ImGui::SliderFloat("Paso de altura", &vx_snap, 0.0f, 4.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0 = libre. Con paso, las alturas caen en multiplos:\n"
                                  "mesetas y escalones limpios en vez de bultos.");

            ImGui::SeparatorText("Seleccion");
            ImGui::TextWrapped("En 'Seleccionar zona', arrastra un rectangulo sobre la "
                               "rejilla (Ctrl suma a lo ya elegido). Vuelve a 'Mover "
                               "vertice' y arrastra uno de los elegidos: se mueven todos.");
            ImGui::Text("%d vertices elegidos", (int)vx_sel.size());
            if (!vx_sel.empty()) {
                int lado = 0; float wsz = 0.0f;
                bool ok = terrain && g3d_editor_terrain_grid(terrain, &lado, &wsz);
                if (ImGui::Button("Nivelar a la altura del activo") && ok) {
                    float pv[3];
                    if (g3d_editor_terrain_vertex(terrain, vx_i, vx_j, pv)) {
                        push_terrain_undo();
                        for (int cell : vx_sel)
                            g3d_editor_terrain_set_vertex_y(terrain, cell % lado,
                                                            cell / lado, pv[1], 0);
                        g3d_editor_terrain_commit(terrain);
                        if (!lakes.empty() || !rivers.empty()) rebuild_water();
                        if (g3d_watersim_active()) watersim_sync(true);
                        status = "Nivelados " + std::to_string(vx_sel.size()) + " vertices";
                    }
                }
                if (ImGui::Button("Nivelar a la media") && ok) {
                    double sum = 0; int n = 0;
                    for (int cell : vx_sel) {
                        float q[3];
                        if (g3d_editor_terrain_vertex(terrain, cell % lado, cell / lado, q))
                            { sum += q[1]; n++; }
                    }
                    if (n) {
                        push_terrain_undo();
                        float avg = (float)(sum / n);
                        for (int cell : vx_sel)
                            g3d_editor_terrain_set_vertex_y(terrain, cell % lado,
                                                            cell / lado, avg, 0);
                        g3d_editor_terrain_commit(terrain);
                        if (!lakes.empty() || !rivers.empty()) rebuild_water();
                        if (g3d_watersim_active()) watersim_sync(true);
                        status = "Nivelados a la media";
                    }
                }
                if (ImGui::Button("Quitar seleccion")) vx_sel.clear();
            }
            if (vx_i >= 0)
                ImGui::Text("Vertice (%d, %d)", vx_i, vx_j);
            ImGui::TextDisabled("Ctrl+Z deshace tambien el relieve.");
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
                if (ImGui::SliderFloat("Nivel (altura)", &lake_level, -30.0f, 40.0f, "%.1f")) {
                    /* El deslizador esta en el panel, o sea que el raton NO esta
                       sobre la escena y la previsualizacion no se recompone sola.
                       Sin esto habia que mover el nivel y luego volver a pasar el
                       cursor por encima para ver el efecto. */
                    lake_prev_key = -1;
                }
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
            if (ImGui::SliderFloat("Evaporacion", &ws_evap, 0.0f, 0.12f, "%.3f")) { if (g3d_watersim_active()) watersim_sync(true); }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = el agua se QUEDA (llena y no se seca). Mas = rios finos / charcos que se secan.");
            if (ImGui::SliderFloat("Velocidad flujo", &ws_flow, 0.3f, 3.0f, "%.1f")) { if (g3d_watersim_active()) watersim_sync(false); }
            ImGui::Separator();
            /* Si o no, sin numero: nadie sabe cuantos segundos hacen falta -- eso
               depende del caudal y del terreno. Lo unico que se decide de verdad
               es si el agua ya esta corrida al empezar o si arranca seca. */
            bool prefill_on = (ws_prefill > 0.5f);
            if (ImGui::Checkbox("Empezar con el agua ya corrida", &prefill_on))
                ws_prefill = prefill_on ? 400.0f : 0.0f;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Marcado: el juego arranca con el rio ya lleno.\n"
                                  "Sin marcar: arranca seco y se va llenando -- para una\n"
                                  "presa que se abre o una inundacion que empieza cuando\n"
                                  "el jugador hace algo.");
            ImGui::Text("Manantiales: %d", (int)wsources.size());
            /* Un caudal constante sin salida NI evaporacion no se estabiliza
               nunca: medido sobre un terreno real, el volumen crece sin parar
               (60 s -> 360, 300 s -> 1591) y el agua acaba saliendose del cauce.
               No es un fallo, es que no tiene a donde ir. */
            if (!wsources.empty() && ws_evap < 0.005f) {
                ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
                    "Con evaporacion 0 el agua NO para de crecer:");
                ImGui::TextWrapped("un manantial vierte sin parar y no tiene salida, "
                                   "asi que antes o despues se sale del cauce. Sube la "
                                   "evaporacion, baja el caudal, o dale salida al mar.");
            }
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
            /* Las reglas tienen su propia seccion, y abierta: estaban metidas
               dentro de "Codigo" y ahi no las encuentra nadie -- que es lo que
               pasa cuando quieres que una roca diga algo al tocarla. */
            if (ImGui::CollapsingHeader(ICON_FA_CODE "  Reglas (si pasa esto, haz esto)",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextDisabled("Ejemplos: al tocarlo saca un dialogo, suma puntos,");
                ImGui::TextDisabled("abre otra escena o llama a tu propio codigo.");
                ui_reglas(o.reglas, o.name);
                if (!o.reglas.empty())
                    ImGui::TextDisabled("Salen en el script del objeto; si lo has editado a mano,\nregeneralo en 'Codigo' para que entren.");
            }
            if (ImGui::CollapsingHeader(ICON_FA_CODE "  Codigo (sonido y script)")) {
                /* Codigo TUYO enganchado al objeto: eliges el .prg y de el sale la
                   lista de PROCESS/FUNCTION. Lo mismo que ya se podia hacer con un
                   personaje, pero para cualquier objeto de la escena. */
                ImGui::SeparatorText("Su sonido");
                combo_sonido("sonido propio", o.amb_sonido);
                if (!o.amb_sonido.empty()) {
                    ImGui::DragFloat("Se oye a", &o.amb_radio, 0.5f, 1.0f, 300.0f, "%.0f");
                    ImGui::SliderInt("Volumen pegado a el", &o.amb_vol, 0, 128);
                    ImGui::TextDisabled("En bucle, mas fuerte cuanto mas cerca: una cascada,\nuna hoguera, una maquina.");
                } else
                    ImGui::TextDisabled("Suena en bucle mientras el jugador este cerca.");

                ImGui::SeparatorText("Script del objeto");
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
                                         "Cilindro", "Muro invisible (colision)",
                                         "Malla exacta del modelo (fijo)",
                                         "Forma del modelo (se puede mover)" };
                ImGui::Combo("Cuerpo", &o.phys, ptypes, IM_ARRAYSIZE(ptypes));
                /* Las dos ultimas sacan la colision del PROPIO modelo, asi que no
                   hay tamanio que ajustar: es el problema de las cajas y esferas,
                   que en cuanto el objeto no es cuadrado no hay numero que valga. */
                if (o.phys == 6) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1), "Exacta: la colision ES el modelo");
                    ImGui::TextWrapped("Triangulo a triangulo: paredes, suelos, escaleras, rocas, "
                                       "un nivel entero. Nada que ajustar. Como no se mueve (Jolt no "
                                       "admite malla movil), sirve para el decorado solido: el jugador "
                                       "y los objetos chocan con el, pero nada lo empuja.");
                } else if (o.phys == 7) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1), "La forma del modelo, y se mueve");
                    ImGui::TextWrapped("Una envolvente convexa hecha con los vertices del modelo: se "
                                       "ajusta sola. Es lo mas pegado que se puede tener en algo que se "
                                       "mueve. Ojo: rellena los huecos (una silla colisiona como un "
                                       "bloque con su silueta); si necesitas el hueco de verdad, hazlo "
                                       "fijo con 'Malla exacta'.");
                    bool fijo7 = (o.mass <= 0.0f);
                    ImGui::TextUnformatted("Se mueve:");
                    if (ImGui::RadioButton("No, es fijo (corta el paso)##m7", fijo7)) o.mass = 0.0f;
                    if (ImGui::RadioButton("Si, se puede empujar##m7", !fijo7) && fijo7) o.mass = 1.0f;
                    if (!fijo7)
                        ImGui::DragFloat("Masa / peso##m7", &o.mass, 0.1f, 0.01f, 1000.0f, "%.2f kg");
                    ImGui::DragFloat("Rebote##m7",   &o.bounce,   0.01f, 0.0f, 1.0f, "%.2f");
                    ImGui::DragFloat("Friccion##m7", &o.friction, 0.01f, 0.0f, 2.0f, "%.2f");
                }
                if (o.phys >= 1 && o.phys <= 4) {         // cuerpo dinamico
                    /* Que se mueva o no es la pregunta de verdad, y estaba escondida
                       en un 0 de la masa: una roca con la masa que viene puesta (1 kg)
                       el personaje la aparta de un empujon y parece que no choca. */
                    bool fijo = (o.mass <= 0.0f);
                    ImGui::TextUnformatted("Se mueve:");
                    // Una por linea: en el Inspector las dos juntas se cortan.
                    if (ImGui::RadioButton("No, es fijo (corta el paso)", fijo)) o.mass = 0.0f;
                    if (ImGui::RadioButton("Si, se puede empujar", !fijo) && fijo) o.mass = 1.0f;
                    if (!fijo)
                        ImGui::DragFloat("Masa / peso", &o.mass, 0.1f, 0.01f, 1000.0f, "%.2f kg");
                    ImGui::DragFloat("Tamano colision", &o.csize, 0.05f, 0.1f, 50.0f, "%.2f");
                    if (ImGui::Button("Ajustar al modelo", ImVec2(-1, 0)))
                        o.csize = csize_del_modelo(o);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Pone la colision del tamanio que tiene el modelo.\nMientras este objeto este elegido, la colision se\ndibuja en la escena para que se vea si cuadra.");
                    {   // Aviso cuando la colision se queda corta: es lo que hace
                        // que el personaje se meta dentro del dibujo "sin chocar".
                        float ideal = csize_del_modelo(o);
                        if (o.csize < ideal * 0.6f)
                            ImGui::TextColored(ImVec4(1, 0.55f, 0.45f, 1),
                                "La colision (%.2f) es mucho menor que el modelo (~%.2f):\nse puede meter dentro del dibujo.", o.csize, ideal);
                    }
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
        } else if (spr_sel >= 0 && spr_sel < (int)sprites.size()) {
            /* ---- Ficha de un PERSONAJE 2D en el Inspector ----
               Lo de todos los dias (donde esta, como de alto, que animacion hace,
               si es el que se controla) para no tener que abrir la ventana grande.
               Las hojas, los fotogramas y las acciones siguen en su ficha, que es
               donde se ve el dibujo: aqui esta el boton para ir. */
            SprObj& o = sprites[spr_sel];
            ImGui::Text(ICON_FA_PERSON_RUNNING "  %s", o.name.c_str());
            ImGui::TextDisabled("%s", o.sheet.empty() ? "(sin hoja)" : o.sheet.c_str());
            ImGui::Spacing();
            if (ImGui::Button("Abrir su ficha (hojas, animaciones y acciones)", ImVec2(-1, 0)))
                show_spr_win = true;
            ImGui::Spacing();
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Posicion", &o.x, 0.1f);
                if (ImGui::DragFloat("Altura (unidades)", &o.height, 0.05f, 0.1f, 60.0f, "%.2f"))
                    if (o.height < 0.1f) o.height = 0.1f;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Lo que mide en el mundo. De aqui salen los pixeles\npor unidad: es lo que le da su tamanio frente a los\nmodelos 3D.");
                if (ImGui::Button("Apoyar en el suelo", ImVec2(-1, 0)) && terrain)
                    o.y = g3d_editor_terrain_height(terrain, o.x, o.z);
            }
            if (ImGui::CollapsingHeader("Que hace", ImGuiTreeNodeFlags_DefaultOpen)) {
                SheetDef* sh = sheet_of(o.sheet);
                const char* cur = o.anim.empty() ? "(el primer fotograma)" : o.anim.c_str();
                if (ImGui::BeginCombo("Animacion", cur)) {
                    if (ImGui::Selectable("(el primer fotograma)", o.anim.empty())) o.anim.clear();
                    if (sh) for (auto& an : sh->anims)
                        if (ImGui::Selectable(an.name.c_str(), an.name == o.anim)) o.anim = an.name;
                    ImGui::EndCombo();
                }
                if (!sh) ImGui::TextDisabled("(su hoja no esta abierta: abre su ficha)");
                /* fps = 0 no es "parado": es "los que tenga su animacion". Aqui se
                   dice con palabras, que un 0 a secas parecia un valor roto. */
                bool propio = (o.fps > 0);
                if (ImGui::Checkbox("Fotogramas por segundo propios", &propio))
                    o.fps = propio ? 10 : 0;
                if (o.fps > 0) ImGui::SliderInt("fotogramas/seg", &o.fps, 1, 30);
                else           ImGui::TextDisabled("   (los de su animacion)");
                bool jug = o.is_player != 0;
                if (ImGui::Checkbox("Es el personaje que se controla", &jug)) {
                    o.is_player = jug;
                    if (jug) spr_follow = spr_sel;
                    else if (spr_follow == spr_sel) spr_follow = -1;
                }
                int nd = o.dirs;
                if (ImGui::SliderInt("Vistas (direcciones)", &nd, 1, 16)) o.dirs = nd;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Cuantas vistas tiene dibujadas: 1 = siempre la misma,\n4 = las cuatro de toda la vida, 8 o 16 = mas fino.");
                if (!o.acciones.empty())
                    ImGui::TextDisabled("%d accion(es) de tecla puestas; se editan en su ficha.",
                                        (int)o.acciones.size());
            }
            if (ImGui::CollapsingHeader(ICON_FA_CODE "  Reglas (si pasa esto, haz esto)")) {
                ui_reglas(o.reglas, o.name);
            }
            ImGui::Spacing();
            if (ImGui::Button("Duplicar##spr", ImVec2(-1, 0))) {
                SprObj c = o;
                c.entity = -1;
                c.name = o.name + "_2";
                for (int k = 2; ; k++) {
                    bool rep = false;
                    for (auto& q : sprites) if (q.name == c.name) { rep = true; break; }
                    if (!rep) break;
                    c.name = o.name + "_" + std::to_string(k + 1);
                }
                c.x += 1.5f;
                sprites.push_back(c);
                spr_sel = (int)sprites.size() - 1;
            }
            if (ImGui::Button("Borrar personaje", ImVec2(-1, 0))) {
                if (o.entity >= 0) g3d_sprite_destroy(o.entity);
                sprites.erase(sprites.begin() + spr_sel);
                spr_sel = -1;
                if (spr_follow >= (int)sprites.size()) spr_follow = -1;
            }
        } else {
            ImGui::TextDisabled("Nada seleccionado.");
            ImGui::TextWrapped("Elige un asset y haz clic en la escena para colocar. "
                               "Sin asset armado, clic selecciona el objeto o el personaje "
                               "mas cercano.");
        }
        ImGui::SeparatorText("Camara");
        ImGui::SliderFloat("Distancia", &cam_dist, 5.0f, 60.0f);
        ImGui::Checkbox("Ver lo que vera el jugador", &show_cam_view);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Abre una ventanita con la escena renderizada DESDE la\n"
                              "camara del juego, para colocarla mirando el encuadre\n"
                              "y no un frustum de alambre.\n"
                              "Cuesta un segundo pase de render: si van justas las\n"
                              "fps, apagala.");
        if (ImGui::Checkbox("Mover la camara con el gizmo", &cam_gizmo) && cam_gizmo) {
            /* La casilla tiene que bastarse sola. El gizmo pedia ADEMAS la
               herramienta Mover activa y ningun objeto elegido -- dos condiciones
               invisibles, asi que marcabas y no pasaba nada. */
            if (tool != T_MOVE && tool != T_ROTATE && tool != T_SCALE) tool = T_MOVE;
            obj_sel = -1;
            /* Y se lleva la vista HASTA la camara. Estaba en (0,45,-90) mientras
               tu mirabas a otro sitio: el gizmo se dibujaba fuera de pantalla, o
               sea que "no aparecia" aunque el codigo corriera perfectamente.
               Y ha de ser la camara DE VERDAD: siguiendo a un personaje, cam_pos
               es un valor muerto y la vista se iba a donde no habia nada. */
            float anchor[3];
            if (cam_anchor(anchor, nullptr, nullptr)) {
                vcam_target[0] = anchor[0];
                vcam_target[1] = anchor[1];
                vcam_target[2] = anchor[2];
                if (cam_dist < 12.0f) cam_dist = 12.0f;
                status = "Gizmo de camara: vista llevada hasta ella";
            } else {
                status = "Elige a que objeto sigue la camara para poder moverla";
            }
        }
        if (cam_gizmo && obj_sel >= 0)
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
                               "Deselecciona el objeto para mover la camara.");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Con Mover/Rotar activos y NINGUN objeto elegido, el gizmo\n"
                              "arrastra la camara del juego en vez de un objeto.\n"
                              "Mover lleva el ojo; Rotar gira la mira sin cambiar la\n"
                              "distancia al objetivo.");

        ImGui::SeparatorText("Sombras");
        // Calidad del shadow map. Se aplica al viewport al momento para verlo.
        int sres_idx = (shadow_res >= 4096) ? 2 : (shadow_res >= 2048) ? 1 : 0;
        const char* sres_lbl[] = { "Baja (1024)", "Media (2048)", "Alta (4096)" };
        if (ImGui::Combo("Calidad", &sres_idx, sres_lbl, 3)) {
            shadow_res = (sres_idx == 2) ? 4096 : (sres_idx == 1) ? 2048 : 1024;
            g3d_renderer_set_shadow_resolution((unsigned)shadow_res);
        }
        ImGui::TextDisabled("Mas alta = bordes mas nitidos, algo mas de video.");
        ImGui::PopTextWrapPos();
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
                        { int di = doc_de(scripts_dir + "/" + regen_obj + ".prg");
                          if (di >= 0) code_recargar(di); }
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

        /* ---- MENUS: principal, pausa, opciones ----
           Eliges la fuente, escribes las opciones y dices que hace cada una. Sale
           como un PROCESS de BennuGD2 con write() y write_set_rgba, que se maneja
           con teclado, mando o raton. */
        if (show_menus) {
            ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
            ImGui::Begin("Menus del juego", &show_menus);
            ImGui::BeginChild("lista_menus", ImVec2(150, 0), true);
            ImGui::TextDisabled("MENUS");
            for (int i = 0; i < (int)menus.size(); i++) {
                ImGui::PushID(i);
                if (ImGui::Selectable(menus[i].nombre.c_str(), menu_sel == i)) menu_sel = i;
                ImGui::PopID();
            }
            if (menus.empty()) ImGui::TextDisabled("(ninguno)");
            ImGui::Spacing();
            if (ImGui::Button(ICON_FA_PLUS "  Nuevo", ImVec2(-1, 0))) {
                Menu m;
                m.nombre = "menu" + std::to_string((int)menus.size() + 1);
                // uno recien hecho ya trae algo con lo que empezar
                /* "Jugar" TIENE que cerrar el menu. Sin accion no hacia nada: con un
                   menu de arranque que congela el juego, pulsarla no te dejaba
                   jugar y parecia que el juego estaba colgado. */
                MenuOpc j; j.texto = "Jugar";
                { Accion a; a.tipo = 6; j.acciones.push_back(a); }
                m.opciones.push_back(j);
                MenuOpc x; x.texto = "Salir";
                { Accion a; a.tipo = 7; x.acciones.push_back(a); }
                m.opciones.push_back(x);
                menus.push_back(m);
                menu_sel = (int)menus.size() - 1;
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("ficha_menu", ImVec2(0, 0), false);
            if (menu_sel >= 0 && menu_sel < (int)menus.size()) {
                Menu& m = menus[menu_sel];
                { char nb[80]; snprintf(nb, sizeof(nb), "%s", m.nombre.c_str());
                  ImGui::SetNextItemWidth(200);
                  if (ImGui::InputText("Nombre (PROCESS)", nb, sizeof(nb))) m.nombre = ident_bgd(nb, "menu"); }
                const char* cc[] = { "Al arrancar el juego", "Al pulsar una tecla o boton",
                                     "Solo cuando lo llame una regla" };
                ImGui::SetNextItemWidth(260);
                ImGui::Combo("Cuando sale", &m.cuando, cc, 3);
                if (m.cuando == 1) {
                    ImGui::SetNextItemWidth(110); combo_tecla_ui("tecla", m.tecla);
                    combo_boton_ui("boton", m.boton);
                }
                bool pa = m.pausa != 0;
                if (ImGui::Checkbox("Congela el juego mientras esta abierto", &pa)) m.pausa = pa ? 1 : 0;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Para el menu de pausa. El mundo se queda quieto y a la vista.");

                ImGui::SeparatorText("Como se ve");
                ImGui::DragInt2("Donde empieza (x, y)", &m.x, 1.0f, 0, 4000);
                ImGui::DragInt("Separacion entre opciones", &m.sep, 0.5f, 8, 200);
                {   // fuente .fnt de Assets (la lista ya la tiene el HUD 2D)
                    const char* cur = m.fuente.empty() ? "(la del sistema)" : m.fuente.c_str();
                    ImGui::SetNextItemWidth(240);
                    if (ImGui::BeginCombo("Fuente", cur)) {
                        if (ImGui::Selectable("(la del sistema)", m.fuente.empty())) m.fuente.clear();
                        for (auto& fn : hud_font_files)
                            if (ImGui::Selectable(fn.c_str(), fn == m.fuente)) m.fuente = fn;
                        ImGui::EndCombo();
                    }
                }
                {   const char* cur = m.fondo.empty() ? "(ninguno)" : m.fondo.c_str();
                    ImGui::SetNextItemWidth(240);
                    if (ImGui::BeginCombo("Fondo", cur)) {
                        if (ImGui::Selectable("(ninguno)", m.fondo.empty())) m.fondo.clear();
                        for (auto& g : hud_gfx_files)
                            if (ImGui::Selectable(g.c_str(), g == m.fondo)) m.fondo = g;
                        ImGui::EndCombo();
                    }
                }
                { float c1[3] = { m.col[0]/255.0f, m.col[1]/255.0f, m.col[2]/255.0f };
                  if (ImGui::ColorEdit3("Color normal", c1))
                      { m.col[0]=(int)(c1[0]*255); m.col[1]=(int)(c1[1]*255); m.col[2]=(int)(c1[2]*255); }
                  float c2[3] = { m.col_sel[0]/255.0f, m.col_sel[1]/255.0f, m.col_sel[2]/255.0f };
                  if (ImGui::ColorEdit3("Color de la elegida", c2))
                      { m.col_sel[0]=(int)(c2[0]*255); m.col_sel[1]=(int)(c2[1]*255); m.col_sel[2]=(int)(c2[2]*255); } }

                ImGui::SeparatorText("Como se maneja");
                { bool t = m.con_teclado != 0, g = m.con_mando != 0, r = m.con_raton != 0;
                  if (ImGui::Checkbox("Teclado", &t)) m.con_teclado = t;
                  ImGui::SameLine();
                  if (ImGui::Checkbox("Mando", &g)) m.con_mando = g;
                  ImGui::SameLine();
                  if (ImGui::Checkbox("Raton", &r)) m.con_raton = r; }
                combo_sonido("Sonido al moverse", m.snd_mover);
                combo_sonido("Sonido al elegir", m.snd_elegir);

                ImGui::SeparatorText("Opciones");
                int quitar_o = -1;
                for (int q = 0; q < (int)m.opciones.size(); q++) {
                    ImGui::PushID(500 + q);
                    MenuOpc& op = m.opciones[q];
                    char tb[128]; snprintf(tb, sizeof(tb), "%s", op.texto.c_str());
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
                    if (ImGui::InputText("texto", tb, sizeof(tb))) op.texto = tb;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Quitar")) quitar_o = q;
                    const char* cls[] = { "Hace algo al pulsarla", "Es un ajuste (izquierda/derecha)",
                                          "Es una ranura de partida" };
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                    ImGui::Combo("que es", &op.clase, cls, 3);
                    if (op.clase == 2) {
                        ImGui::SetNextItemWidth(120);
                        ImGui::DragInt("ranura", &op.ranura, 0.2f, 1, guardado.ranuras);
                        const char* mm2[] = { "Cargar esa partida", "Guardar en esa ranura" };
                        ImGui::SetNextItemWidth(220);
                        ImGui::Combo("al pulsarla", &op.ranura_modo, mm2, 2);
                        ImGui::TextDisabled("  Ensenia si esta vacia o en que escena la dejaste.");
                    }
                    if (op.clase == 1) {
                        const char* aj[] = { "Volumen de la musica", "Volumen de los sonidos",
                                             "Pantalla completa (si/no)", "Una variable del juego" };
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                        if (ImGui::Combo("ajusta", &op.ajuste, aj, 4)) {
                            if (op.ajuste <= 1) { op.vmin = 0; op.vmax = 128; op.paso = 8; }
                            else if (op.ajuste == 2) { op.vmin = 0; op.vmax = 1; op.paso = 1; }
                        }
                        if (op.ajuste == 3) {
                            combo_var("variable", op.var);
                            ImGui::DragInt3("min / max / paso", &op.vmin, 1.0f, -99999, 99999);
                            ImGui::TextDisabled("  Sirve para lo que quieras: dificultad, sensibilidad,");
                            ImGui::TextDisabled("  vidas, idioma... Es una variable del juego como otra.");
                        }
                        ImGui::TextDisabled("  El valor se ensenia al lado y se guarda al salir del menu.");
                    } else {
                        ui_acciones(op.acciones, m.nombre + "_" + std::to_string(q + 1), 1);
                    }
                    ImGui::Separator();
                    ImGui::PopID();
                }
                if (quitar_o >= 0) m.opciones.erase(m.opciones.begin() + quitar_o);
                if (ImGui::Button(ICON_FA_PLUS "  Anadir opcion", ImVec2(-1, 0)))
                    m.opciones.push_back(MenuOpc());
                ImGui::Spacing();
                if (ImGui::Button("Borrar este menu", ImVec2(-1, 0))) {
                    menus.erase(menus.begin() + menu_sel);
                    menu_sel = menus.empty() ? -1 : 0;
                }
            } else {
                ImGui::TextWrapped("Un menu es una lista de opciones y lo que hace cada una: "
                                   "empezar la partida (ir a una escena), abrir otro menu, "
                                   "cambiar una variable, llamar a tu codigo o salir del juego.");
            }
            ImGui::EndChild();
            ImGui::End();
        }

        /* ---- DIALOGOS: lo que dice la gente ----
           Paginas con su texto, quien habla y su retrato; y si una pagina tiene
           respuestas, es una pregunta que puede llevar a otra pagina. El bocadillo
           es un grafico tuyo (PNG suelto o un grafico de un FPG). */
        if (show_dialogos) {
            ImGui::SetNextWindowSize(ImVec2(680, 560), ImGuiCond_FirstUseEver);
            ImGui::Begin("Dialogos del juego", &show_dialogos);
            ImGui::BeginChild("lista_dlg", ImVec2(160, 0), true);
            ImGui::TextDisabled("DIALOGOS");
            for (int i = 0; i < (int)dialogos.size(); i++) {
                ImGui::PushID(i);
                if (ImGui::Selectable(dialogos[i].nombre.c_str(), dlg_sel == i)) dlg_sel = i;
                ImGui::PopID();
            }
            if (dialogos.empty()) ImGui::TextDisabled("(ninguno)");
            ImGui::Spacing();
            if (ImGui::Button(ICON_FA_PLUS "  Nuevo", ImVec2(-1, 0))) {
                Dialogo d;
                d.nombre = "dialogo" + std::to_string((int)dialogos.size() + 1);
                DlgPag p1; p1.texto = "Hola, viajero.";
                d.paginas.push_back(p1);
                dialogos.push_back(d);
                dlg_sel = (int)dialogos.size() - 1;
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("ficha_dlg", ImVec2(0, 0), false);
            if (dlg_sel >= 0 && dlg_sel < (int)dialogos.size()) {
                Dialogo& d = dialogos[dlg_sel];
                { char nb[80]; snprintf(nb, sizeof(nb), "%s", d.nombre.c_str());
                  ImGui::SetNextItemWidth(200);
                  if (ImGui::InputText("Nombre (PROCESS)", nb, sizeof(nb))) d.nombre = ident_bgd(nb, "dialogo"); }
                ImGui::TextDisabled("Para que salga: ponle a un objeto una regla con la accion");
                ImGui::TextDisabled("\"Sacar un dialogo\" (al tocarlo, al pulsar una tecla cerca...).");

                ImGui::SeparatorText("El bocadillo");
                {   const char* cur = d.caja.empty() ? "(sin caja: solo el texto)" : d.caja.c_str();
                    ImGui::SetNextItemWidth(300);
                    if (ImGui::BeginCombo("Grafico", cur)) {
                        if (ImGui::Selectable("(sin caja: solo el texto)", d.caja.empty())) d.caja.clear();
                        for (auto& g : hud_gfx_files)
                            if (ImGui::Selectable(g.c_str(), g == d.caja)) d.caja = g;
                        ImGui::EndCombo();
                    }
                    bool es_fpg = d.caja.size() > 4 &&
                                  (d.caja.substr(d.caja.size()-4) == ".fpg" ||
                                   d.caja.substr(d.caja.size()-4) == ".f16" ||
                                   d.caja.substr(d.caja.size()-4) == ".f32");
                    if (es_fpg) {
                        ImGui::DragInt("Que grafico del FPG", &d.caja_graf, 1.0f, 1, 999);
                        H2Fpg* fp = hud_fpg(d.caja);
                        if (fp) ImGui::TextDisabled("  el FPG trae %d graficos", fp->n);
                    }
                    ImGui::TextDisabled("Vale un PNG suelto o un FPG: el grafico se estira al tamanio de abajo.");
                }
                ImGui::DragInt2("Centro (x, y)", &d.cx, 1.0f, 0, 4000);
                ImGui::DragInt2("Tamanio (ancho, alto)", &d.cw, 1.0f, 40, 4000);

                ImGui::SeparatorText("El texto");
                {   const char* cur = d.fuente.empty() ? "(la del sistema)" : d.fuente.c_str();
                    ImGui::SetNextItemWidth(240);
                    if (ImGui::BeginCombo("Fuente", cur)) {
                        if (ImGui::Selectable("(la del sistema)", d.fuente.empty())) d.fuente.clear();
                        for (auto& fn : hud_font_files)
                            if (ImGui::Selectable(fn.c_str(), fn == d.fuente)) d.fuente = fn;
                        ImGui::EndCombo();
                    }
                }
                ImGui::DragInt2("Margenes (x, y)", &d.mx, 1.0f, 0, 300);
                ImGui::DragInt("Letras por segundo", &d.vel, 1.0f, 0, 200);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("El texto se escribe solo, como en los juegos.\n0 = sale entero de golpe.\nPulsando se acaba de escribir de una vez.");
                { float c1[3] = { d.col[0]/255.0f, d.col[1]/255.0f, d.col[2]/255.0f };
                  if (ImGui::ColorEdit3("Color del texto", c1))
                      { d.col[0]=(int)(c1[0]*255); d.col[1]=(int)(c1[1]*255); d.col[2]=(int)(c1[2]*255); }
                  float c2[3] = { d.col_nombre[0]/255.0f, d.col_nombre[1]/255.0f, d.col_nombre[2]/255.0f };
                  if (ImGui::ColorEdit3("Color del nombre y la respuesta elegida", c2))
                      { d.col_nombre[0]=(int)(c2[0]*255); d.col_nombre[1]=(int)(c2[1]*255); d.col_nombre[2]=(int)(c2[2]*255); } }
                combo_tecla_ui("Pasar con la tecla", d.tecla);
                combo_boton_ui("o con el boton", d.boton);
                combo_sonido("Sonido de las letras", d.snd_letra);
                combo_sonido("Sonido al pasar", d.snd_pasar);
                { bool pa = d.pausa != 0;
                  if (ImGui::Checkbox("Congela el juego mientras se habla", &pa)) d.pausa = pa ? 1 : 0; }

                ImGui::SeparatorText("Paginas");
                int quitar_p = -1;
                for (int q = 0; q < (int)d.paginas.size(); q++) {
                    DlgPag& pg = d.paginas[q];
                    ImGui::PushID(700 + q);
                    char cab[128];
                    snprintf(cab, sizeof(cab), "Pagina %d%s%s", q + 1,
                             pg.quien.empty() ? "" : " - ", pg.quien.c_str());
                    if (ImGui::CollapsingHeader(cab, ImGuiTreeNodeFlags_DefaultOpen)) {
                        { char qb[80]; snprintf(qb, sizeof(qb), "%s", pg.quien.c_str());
                          ImGui::SetNextItemWidth(200);
                          if (ImGui::InputText("Quien habla", qb, sizeof(qb))) pg.quien = qb; }
                        { char tb[1024]; snprintf(tb, sizeof(tb), "%s", pg.texto.c_str());
                          if (ImGui::InputTextMultiline("Lo que dice", tb, sizeof(tb),
                                                        ImVec2(-1, 60))) pg.texto = tb; }
                        ImGui::TextDisabled("Se parte solo en lineas para que quepa en la caja.");
                        {   const char* cur = pg.retrato.empty() ? "(sin retrato)" : pg.retrato.c_str();
                            ImGui::SetNextItemWidth(240);
                            if (ImGui::BeginCombo("Retrato", cur)) {
                                if (ImGui::Selectable("(sin retrato)", pg.retrato.empty())) pg.retrato.clear();
                                for (auto& g : hud_gfx_files)
                                    if (ImGui::Selectable(g.c_str(), g == pg.retrato))
                                        { pg.retrato = g; pg.retrato_cara = -1; }
                                ImGui::EndCombo();
                            }
                        }
                        /* ---- LA CARA ----
                           Un retrato suele venir en una HOJA con varias caras (seria,
                           riendo, enfadada). Si la imagen tiene fotogramas detectados,
                           se elige cual sale en esta pagina y se ve al momento. */
                        if (!pg.retrato.empty() && !hud_is_fpg(pg.retrato)) {
                            SheetDef* hj = sheet_of(pg.retrato);
                            int nf = hj ? (int)hj->frames.size() : 0;
                            if (nf > 1) {
                                if (pg.retrato_cara >= nf) pg.retrato_cara = nf - 1;
                                ImGui::Text("Cara (%d en la hoja):", nf);
                                ImGui::SameLine();
                                ImGui::TextDisabled(pg.retrato_cara < 0
                                    ? "la imagen entera" : "elegida la %d", pg.retrato_cara);
                                /* Una REJILLA de caras y se pincha la que quieras. Antes
                                   habia un control de arrastrar, y con 30 caras eso no hay
                                   quien lo use: al pulsarlo no pasaba nada y parecia que no
                                   se podian elegir. */
                                H2Img* im = hud_img(pg.retrato);
                                if (im && im->tex && im->w > 0 && im->h > 0) {
                                    ImGui::BeginChild("caras", ImVec2(0, 150), true,
                                                      ImGuiWindowFlags_HorizontalScrollbar);
                                    const float alto = 58.0f;
                                    float ancho_util = ImGui::GetContentRegionAvail().x;
                                    float usado = 0.0f;
                                    // la primera "cara" es la imagen entera
                                    {
                                        ImGui::PushID(-1);
                                        bool sel = (pg.retrato_cara < 0);
                                        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.949f, 0.686f, 0.259f, 1.0f));
                                        float w = alto * im->w / im->h;
                                        if (w > 90.0f) w = 90.0f;
                                        if (ImGui::ImageButton("##entera", (ImTextureID)(intptr_t)im->tex,
                                                               ImVec2(w, alto)))
                                            pg.retrato_cara = -1;
                                        if (sel) ImGui::PopStyleColor();
                                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("La imagen entera");
                                        usado += w + 16.0f;
                                        ImGui::PopID();
                                    }
                                    for (int c3 = 0; c3 < nf; c3++) {
                                        SprFrame& fr = hj->frames[c3];
                                        if (fr.w <= 0 || fr.h <= 0) continue;
                                        float w = alto * fr.w / fr.h;
                                        if (usado + w + 16.0f < ancho_util) ImGui::SameLine();
                                        else usado = 0.0f;
                                        usado += w + 16.0f;
                                        ImGui::PushID(c3);
                                        bool sel = (pg.retrato_cara == c3);
                                        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.949f, 0.686f, 0.259f, 1.0f));
                                        if (ImGui::ImageButton("##cara", (ImTextureID)(intptr_t)im->tex,
                                                               ImVec2(w, alto),
                                                               ImVec2((float)fr.x / im->w, (float)fr.y / im->h),
                                                               ImVec2((float)(fr.x + fr.w) / im->w,
                                                                      (float)(fr.y + fr.h) / im->h)))
                                            pg.retrato_cara = c3;
                                        if (sel) ImGui::PopStyleColor();
                                        if (ImGui::IsItemHovered())
                                            ImGui::SetTooltip("Cara %d  (%dx%d)", c3, fr.w, fr.h);
                                        ImGui::PopID();
                                    }
                                    ImGui::EndChild();
                                }
                            } else if (nf == 1) {
                                ImGui::TextDisabled("  (una sola cara: sale la imagen entera)");
                            } else {
                                ImGui::TextDisabled("  Si la hoja trae varias caras, abrela en Sprites 3D");
                                ImGui::TextDisabled("  y dale a Detectar: aqui podras elegir cual sale.");
                            }
                        }
                        ImGui::TextDisabled("RESPUESTAS (si no pones ninguna, se pasa de pagina)");
                        int quitar_o = -1;
                        for (int r = 0; r < (int)pg.opciones.size(); r++) {
                            DlgOpc& o = pg.opciones[r];
                            ImGui::PushID(800 + r);
                            { char tb[128]; snprintf(tb, sizeof(tb), "%s", o.texto.c_str());
                              ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                              if (ImGui::InputText("texto", tb, sizeof(tb))) o.texto = tb; }
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Quitar")) quitar_o = r;
                            {   // a donde lleva
                                std::string et = (o.salto < 0) ? "cerrar el dialogo"
                                                               : ("ir a la pagina " + std::to_string(o.salto + 1));
                                ImGui::SetNextItemWidth(220);
                                if (ImGui::BeginCombo("lleva a", et.c_str())) {
                                    if (ImGui::Selectable("cerrar el dialogo", o.salto < 0)) o.salto = -1;
                                    for (int k = 0; k < (int)d.paginas.size(); k++) {
                                        std::string n2 = "ir a la pagina " + std::to_string(k + 1);
                                        if (ImGui::Selectable(n2.c_str(), o.salto == k)) o.salto = k;
                                    }
                                    ImGui::EndCombo();
                                }
                            }
                            ui_acciones(o.acciones, d.nombre + "_" + std::to_string(q + 1) +
                                                    "_" + std::to_string(r + 1), 1);
                            ImGui::Separator();
                            ImGui::PopID();
                        }
                        if (quitar_o >= 0) pg.opciones.erase(pg.opciones.begin() + quitar_o);
                        if (ImGui::SmallButton("+ anadir respuesta")) pg.opciones.push_back(DlgOpc());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Quitar esta pagina")) quitar_p = q;
                    }
                    ImGui::PopID();
                }
                if (quitar_p >= 0) d.paginas.erase(d.paginas.begin() + quitar_p);
                if (ImGui::Button(ICON_FA_PLUS "  Anadir pagina", ImVec2(-1, 0)))
                    d.paginas.push_back(DlgPag());
                ImGui::Spacing();
                if (ImGui::Button("Borrar este dialogo", ImVec2(-1, 0))) {
                    dialogos.erase(dialogos.begin() + dlg_sel);
                    dlg_sel = dialogos.empty() ? -1 : 0;
                }
            } else {
                ImGui::TextWrapped("Un dialogo es lo que dice alguien: paginas de texto con quien "
                                   "habla y su retrato. Si una pagina lleva respuestas, es una "
                                   "pregunta, y cada respuesta puede llevar a otra pagina, cambiar "
                                   "una variable, sonar algo o llamar a tu codigo.");
            }
            ImGui::EndChild();
            ImGui::End();
        }

        /* ---- Las ESCENAS del proyecto ----
           Cada escena es su fichero en Scenes/. En el juego se montan una cada vez
           y se viaja de una a otra con la accion "Ir a otra escena" de una regla:
           tocar una puerta, entrar en una zona, cumplirse algo. */
        if (show_escenas) {
            ImGui::SetNextWindowSize(ImVec2(430, 340), ImGuiCond_FirstUseEver);
            ImGui::Begin("Escenas del proyecto", &show_escenas);
            ImGui::TextWrapped("La marcada con la estrella es por la que empieza el juego. "
                               "Para viajar de una a otra, ponle a un objeto una regla con "
                               "la accion \"Ir a otra escena\".");
            ImGui::Separator();
            auto lista = escenas_del_proyecto();
            static char nom_esc[80] = "";
            static std::string esc_menu;
            static bool pedir_nueva = false, pedir_ren_esc = false, pedir_borrar_esc = false;
            for (auto& e : lista) {
                std::string ruta = scenes_dir + "/" + e;
                bool abierta  = (ruta == scene_path);
                bool inicial  = (escena_inicial == "Scenes/" + e);
                ImGui::PushID(e.c_str());
                if (ImGui::SmallButton(inicial ? ICON_FA_STAR : ICON_FA_STAR_HALF_STROKE))
                    { escena_inicial = "Scenes/" + e; write_manifest(); }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(inicial ? "Es la escena inicial del juego"
                                              : "Hacer que el juego empiece por esta");
                ImGui::SameLine();
                if (ImGui::Selectable((e + (abierta ? "   (abierta)" : "")).c_str(), abierta)) {
                    if (!abierta) { save_scene(scene_path); load_scene(ruta); scene_path = ruta; }
                }
                if (ImGui::BeginPopupContextItem("ctx_esc")) {
                    esc_menu = e;
                    if (ImGui::MenuItem("Abrir")) { save_scene(scene_path); load_scene(ruta); scene_path = ruta; }
                    if (ImGui::MenuItem("Empezar el juego por esta")) { escena_inicial = "Scenes/" + e; write_manifest(); }
                    if (ImGui::MenuItem("Duplicar")) {
                        std::string base = fs::path(e).stem().string();
                        for (int k = 2; k < 100; k++) {
                            std::string cand = scenes_dir + "/" + base + "_" + std::to_string(k) + ".scene";
                            std::error_code ec;
                            if (fs::exists(cand, ec)) continue;
                            fs::copy_file(ruta, cand, ec);
                            // el relieve, el pintado, las zonas y la siembra van aparte
                            for (const char* suf : { ".terrain", ".paint.png", ".zones", ".scatter" }) {
                                std::error_code e2;
                                if (fs::exists(ruta + suf, e2)) fs::copy_file(ruta + suf, cand + suf, e2);
                            }
                            break;
                        }
                    }
                    if (ImGui::MenuItem("Renombrar...")) {
                        snprintf(nom_esc, sizeof(nom_esc), "%s", fs::path(e).stem().string().c_str());
                        pedir_ren_esc = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Borrar...")) pedir_borrar_esc = true;
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            if (lista.empty()) ImGui::TextDisabled("(ninguna)");
            ImGui::Spacing();
            if (ImGui::Button(ICON_FA_PLUS "  Escena nueva", ImVec2(-1, 0))) { nom_esc[0] = 0; pedir_nueva = true; }
            ImGui::TextDisabled("Al abrir otra se guarda antes la que tengas puesta.");

            if (pedir_nueva) { ImGui::OpenPopup("Escena nueva"); pedir_nueva = false; }
            if (ImGui::BeginPopupModal("Escena nueva", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("Nombre (sin .scene):");
                ImGui::SetNextItemWidth(260);
                bool ok = ImGui::InputText("##ne", nom_esc, sizeof(nom_esc), ImGuiInputTextFlags_EnterReturnsTrue);
                if (ImGui::Button("Crear", ImVec2(120, 0)) || ok) {
                    std::string nom = ident_bgd(nom_esc, "escena");
                    std::string ruta = scenes_dir + "/" + nom + ".scene";
                    std::error_code ec;
                    if (!fs::exists(ruta, ec)) {
                        save_scene(scene_path);      // no perder lo que tenias
                        objects.clear(); obj_sel = -1;
                        sprites.clear(); spr_sel = -1; spr_follow = -1;
                        hud.clear(); hud_sel = -1;
                        esc_musica.clear(); zsonidos.clear();
                        scene_path = ruta;
                        save_scene(scene_path);      // nace vacia, con el terreno de ahora
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (pedir_ren_esc) { ImGui::OpenPopup("Renombrar escena"); pedir_ren_esc = false; }
            if (ImGui::BeginPopupModal("Renombrar escena", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Renombrar %s", esc_menu.c_str());
                ImGui::SetNextItemWidth(260);
                bool ok = ImGui::InputText("##re", nom_esc, sizeof(nom_esc), ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1),
                    "Ojo: si alguna regla manda a esta escena, vuelve a elegirla.");
                if (ImGui::Button("Renombrar", ImVec2(120, 0)) || ok) {
                    std::string nom = ident_bgd(nom_esc, "escena");
                    std::string vieja = scenes_dir + "/" + esc_menu;
                    std::string nueva = scenes_dir + "/" + nom + ".scene";
                    std::error_code ec;
                    if (!fs::exists(nueva, ec)) {
                        fs::rename(vieja, nueva, ec);
                        for (const char* suf : { ".terrain", ".paint.png", ".zones", ".scatter" }) {
                            std::error_code e2;
                            if (fs::exists(vieja + suf, e2)) fs::rename(vieja + suf, nueva + suf, e2);
                        }
                        if (scene_path == vieja) scene_path = nueva;
                        if (escena_inicial == "Scenes/" + esc_menu) { escena_inicial = "Scenes/" + nom + ".scene"; }
                        write_manifest();
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (pedir_borrar_esc) { ImGui::OpenPopup("Borrar escena"); pedir_borrar_esc = false; }
            if (ImGui::BeginPopupModal("Borrar escena", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Se va a borrar Scenes/%s", esc_menu.c_str());
                ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1), "Con su relieve, su pintado y su siembra. No se puede deshacer.");
                bool es_abierta = (scene_path == scenes_dir + "/" + esc_menu);
                if (es_abierta) ImGui::TextDisabled("(es la que tienes abierta: abre otra antes)");
                ImGui::BeginDisabled(es_abierta);
                if (ImGui::Button("Borrar", ImVec2(120, 0))) {
                    std::string ruta = scenes_dir + "/" + esc_menu;
                    std::error_code ec;
                    fs::remove(ruta, ec);
                    for (const char* suf : { ".terrain", ".paint.png", ".zones", ".scatter" }) {
                        std::error_code e2; fs::remove(ruta + suf, e2);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::End();
        }

        /* Ordenar mueve ficheros del disco, asi que se pregunta antes. */
        if (pedir_ordenar) { ImGui::OpenPopup("Ordenar los Assets"); pedir_ordenar = false; }
        if (ImGui::BeginPopupModal("Ordenar los Assets", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Se van a repartir por carpetas los ficheros sueltos de Assets/:");
            ImGui::BulletText("Models    .glb .gltf .fbx .obj .md3");
            ImGui::BulletText("Textures  imagenes que NO son hoja de sprites");
            ImGui::BulletText("Sprites   las imagenes que usas como hoja");
            ImGui::BulletText("Fonts     .fnt .fnx");
            ImGui::BulletText("Music     .mp3 .mod .xm .it .flac...");
            ImGui::BulletText("Sounds    .wav .ogg");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1),
                "Tus escenas NO se rompen: el editor busca cada asset por su nombre.");
            ImGui::TextDisabled("Lo que no reconozca se queda donde esta. Si ya hay un fichero");
            ImGui::TextDisabled("con ese nombre en la carpeta destino, no se pisa.");
            ImGui::Spacing();
            if (ImGui::Button("Ordenar", ImVec2(140, 0))) { ordenar_assets(); ImGui::CloseCurrentPopup(); }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(140, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        /* ---- GUARDAR PARTIDA: que entra, cuantas ranuras y como se llama ----
           Nada de esto lo decide el editor: no sabe si tu juego es de mazmorras o
           de carreras, asi que el "progreso" lo defines tu marcando casillas. */
        if (show_guardado) {
            ImGui::SetNextWindowSize(ImVec2(440, 420), ImGuiCond_FirstUseEver);
            ImGui::Begin("Guardar partida", &show_guardado);
            ImGui::TextWrapped("Una partida guardada es lo que TU digas que es. Marca lo que "
                               "tiene que sobrevivir y el editor lo guarda todo junto.");
            ImGui::Separator();
            ImGui::DragInt("Cuantas ranuras", &guardado.ranuras, 0.1f, 1, 20);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Partida 1, 2, 3... Cada una en su fichero.");
            { char fb[80]; snprintf(fb, sizeof(fb), "%s", guardado.fichero.c_str());
              ImGui::SetNextItemWidth(200);
              if (ImGui::InputText("Nombre del fichero", fb, sizeof(fb))) guardado.fichero = ident_bgd(fb, "partida");
              ImGui::TextDisabled("  Saldran %s1.sav, %s2.sav...", guardado.fichero.c_str(), guardado.fichero.c_str()); }

            ImGui::SeparatorText("Que se guarda");
            ImGui::Checkbox("Las variables del juego (las que marques abajo)", &guardado.con_vars);
            ImGui::Checkbox("En que escena estabas", &guardado.con_escena);
            ImGui::Checkbox("Donde estaba el jugador y hacia donde miraba", &guardado.con_jugador);
            ImGui::Checkbox("Las reglas ya cumplidas (la puerta abierta sigue abierta)", &guardado.con_reglas);

            if (guardado.con_vars) {
                ImGui::SeparatorText("Cuales de tus variables");
                if (gvars.empty())
                    ImGui::TextDisabled("(todavia no tienes ninguna: Ventana > Variables del juego)");
                for (auto& v : gvars) {
                    ImGui::PushID(v.nombre.c_str());
                    ImGui::Checkbox(v.nombre.c_str(), &v.guardar);
                    ImGui::PopID();
                }
                ImGui::TextDisabled("Desmarca las que no son progreso (un contador temporal, un ajuste).");
            }

            ImGui::SeparatorText("Como se guarda y se carga");
            ImGui::TextWrapped("Con acciones, asi que sirve cualquier disparador: un punto de "
                               "guardado que se toca, un menu con sus ranuras, al pasar de escena, "
                               "cada N segundos para autoguardar...");
            ImGui::BulletText("En una regla: \"Guardar la partida\" / \"Cargar la partida\".");
            ImGui::BulletText("En un menu: una opcion de clase \"ranura de partida\",");
            ImGui::BulletText("   que ademas ensenia si esa ranura esta vacia.");
            ImGui::End();
        }

        /* ---- Las VARIABLES DEL JUEGO: puntos, vida, llaves... ----
           Son GLOBAL de BennuGD2, asi que valen para las reglas, para el HUD 2D
           (write_var) y para tu propio codigo, sin declararlas a mano. */
        if (show_gvars) {
            ImGui::SetNextWindowSize(ImVec2(430, 320), ImGuiCond_FirstUseEver);
            ImGui::Begin("Variables del juego", &show_gvars);
            ImGui::TextWrapped("Lo que cuenta el juego. Se generan como GLOBAL con su valor "
                               "inicial; las reglas las cambian, el HUD 2D las pinta y tu codigo "
                               "las ve sin declarar nada.");
            ImGui::Separator();
            int quitar = -1;
            for (int i = 0; i < (int)gvars.size(); i++) {
                ImGui::PushID(i);
                char nb[64]; snprintf(nb, sizeof(nb), "%s", gvars[i].nombre.c_str());
                ImGui::SetNextItemWidth(180);
                if (ImGui::InputText("##n", nb, sizeof(nb))) gvars[i].nombre = ident_bgd(nb, "var");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(110);
                ImGui::InputInt("empieza en", &gvars[i].valor);
                ImGui::SameLine();
                if (ImGui::SmallButton("Quitar")) quitar = i;
                ImGui::PopID();
            }
            if (quitar >= 0) gvars.erase(gvars.begin() + quitar);
            if (gvars.empty()) ImGui::TextDisabled("(ninguna todavia)");
            ImGui::Spacing();
            if (ImGui::Button(ICON_FA_PLUS "  Anadir variable")) {
                GameVar v; v.nombre = "var" + std::to_string((int)gvars.size() + 1); v.valor = 0;
                gvars.push_back(v);
            }
            ImGui::SameLine();
            if (ImGui::Button("Puntos y vida")) {   // las dos de siempre, de un clic
                bool hay_p = false, hay_v = false;
                for (auto& v : gvars) { if (v.nombre == "puntos") hay_p = true; if (v.nombre == "vida") hay_v = true; }
                if (!hay_p) { GameVar v; v.nombre = "puntos"; v.valor = 0;   gvars.push_back(v); }
                if (!hay_v) { GameVar v; v.nombre = "vida";   v.valor = 100; gvars.push_back(v); }
            }
            ImGui::TextDisabled("Se guardan con la escena.");
            ImGui::End();
        }

        /* ================= EDITOR DE CODIGO (pantalla completa) =================
           Varios ficheros a la vez en pestanias, el arbol de Scripts al lado, el
           esquema de PROCESS/FUNCTION del que estas viendo, buscar y reemplazar,
           autocompletado y compilar marcando la linea del error. Se abre desde el
           Inspector, desde el menu y con doble clic en un objeto. */
        static std::string compile_out;
        static std::vector<std::pair<std::string,int>> compile_err;  // fichero, linea
        static char  buscar_txt[128] = "", reemp_txt[128] = "";
        static bool  buscar_abierto = false, buscar_case = false, foco_buscar = false;
        static bool  pedir_nuevo = false, pedir_renombrar = false, pedir_borrar_prg = false, pedir_ir = false;
        static char  nombre_nuevo[80] = "";
        static std::string fich_menu;          // fichero sobre el que actua el menu del arbol
        static int   ir_linea = 1;
        static float code_zoom = 1.0f;
        static bool  ver_espacios = false;
        static int   tabulacion = 4;
        static int   cerrar_doc = -1;          // pestania que se pide cerrar
        static bool  ac_abierto = false;       // desplegable del autocompletado
        static int   ac_sel = 0;
        static int   ac_col = 0;               // columna donde empieza la palabra
        static std::vector<std::pair<std::string,std::string>> ac_lista;   // texto, ayuda
        static std::vector<std::string> fich_cache;
        static double fich_t = -1.0;

        if (show_script) {
            // ---- utilidades de coordenadas: la columna del editor cuenta las
            //      tabulaciones expandidas, y para tocar el texto hace falta el
            //      indice real dentro de la linea.
            auto col_a_idx = [&](const std::string& l, int col) -> size_t {
                int c = 0; size_t i = 0;
                while (i < l.size() && c < col) {
                    if (l[i] == '\t') c = (c / tabulacion + 1) * tabulacion; else c++;
                    i++;
                }
                return i;
            };
            auto idx_a_col = [&](const std::string& l, size_t idx) -> int {
                int c = 0;
                for (size_t i = 0; i < idx && i < l.size(); i++) {
                    if (l[i] == '\t') c = (c / tabulacion + 1) * tabulacion; else c++;
                }
                return c;
            };
            auto en_lineas = [](const std::string& t) {
                std::vector<std::string> v; size_t i = 0;
                for (;;) { size_t f = t.find('\n', i);
                          if (f == std::string::npos) { v.push_back(t.substr(i)); break; }
                          v.push_back(t.substr(i, f - i)); i = f + 1; }
                return v;
            };
            auto minus = [](std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; };
            // Buscar hacia delante o hacia atras desde el cursor, dando la vuelta
            // al llegar al final. Devuelve si ha encontrado algo, y si dio la vuelta.
            auto buscar_en = [&](CodeDoc* d, bool atras, bool* vuelta) -> bool {
                std::string pat = buscar_txt;
                if (!d || pat.empty()) return false;
                auto ls = en_lineas(d->ed.GetText());
                if (ls.empty()) return false;
                auto cur = d->ed.GetCursorPosition();
                int L = (int)ls.size();
                int l0 = cur.mLine < L ? cur.mLine : L - 1;
                size_t i0 = col_a_idx(ls[l0], cur.mColumn);
                std::string pm = buscar_case ? pat : minus(pat);
                if (vuelta) *vuelta = false;
                for (int k = 0; k <= L; k++) {
                    int ln = atras ? ((l0 - k) % L + L) % L : (l0 + k) % L;
                    if (vuelta && ((!atras && ln < l0) || (atras && ln > l0))) *vuelta = true;
                    const std::string& l = ls[ln];
                    std::string lm = buscar_case ? l : minus(l);
                    size_t pos;
                    if (!atras) {
                        size_t desde = (k == 0) ? i0 : 0;
                        pos = (desde > lm.size()) ? std::string::npos : lm.find(pm, desde);
                    } else {
                        if (k == 0) { if (i0 == 0) continue; pos = lm.rfind(pm, i0 - 1); }
                        else        pos = lm.rfind(pm);
                    }
                    if (pos != std::string::npos) {
                        int c0 = idx_a_col(l, pos), c1 = idx_a_col(l, pos + pat.size());
                        d->ed.SetSelection(TextEditor::Coordinates(ln, c0), TextEditor::Coordinates(ln, c1));
                        d->ed.SetCursorPosition(TextEditor::Coordinates(ln, atras ? c0 : c1));
                        return true;
                    }
                }
                return false;
            };

            ImGuiViewport* mv = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(mv->WorkPos);
            ImGui::SetNextWindowSize(mv->WorkSize);
            if (focus_script) { ImGui::SetNextWindowFocus(); focus_script = false; }
            ImGui::Begin("Editor de codigo", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar);

            CodeDoc* d = doc_activo();
            bool ped_guardar = false, ped_guardar_todo = false, ped_compilar = false;
            bool ped_buscar_sig = false, ped_buscar_ant = false;

            // ------------------------------ menu ------------------------------
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Archivo")) {
                    if (ImGui::MenuItem("Nuevo script...", "Ctrl+N")) { nombre_nuevo[0] = 0; pedir_nuevo = true; }
                    if (ImGui::MenuItem("Abrir main.prg"))            open_main_script();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Guardar", "Ctrl+S", false, d != nullptr))      ped_guardar = true;
                    if (ImGui::MenuItem("Guardar todo", "Ctrl+Shift+S", false, !docs.empty())) ped_guardar_todo = true;
                    if (ImGui::MenuItem("Cerrar pestania", "Ctrl+W", false, d != nullptr)) cerrar_doc = doc_sel;
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cerrar el editor", "Esc"))   show_script = false;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Editar", d != nullptr)) {
                    if (ImGui::MenuItem("Deshacer", "Ctrl+Z", false, d->ed.CanUndo())) d->ed.Undo();
                    if (ImGui::MenuItem("Rehacer", "Ctrl+Y", false, d->ed.CanRedo())) d->ed.Redo();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cortar", "Ctrl+X", false, d->ed.HasSelection()))  d->ed.Cut();
                    if (ImGui::MenuItem("Copiar", "Ctrl+C", false, d->ed.HasSelection())) d->ed.Copy();
                    if (ImGui::MenuItem("Pegar", "Ctrl+V"))  d->ed.Paste();
                    if (ImGui::MenuItem("Seleccionar todo", "Ctrl+A")) d->ed.SelectAll();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Buscar", d != nullptr)) {
                    if (ImGui::MenuItem("Buscar...", "Ctrl+F"))    { buscar_abierto = true; foco_buscar = true; }
                    if (ImGui::MenuItem("Siguiente", "F3"))        ped_buscar_sig = true;
                    if (ImGui::MenuItem("Anterior", "Shift+F3"))   ped_buscar_ant = true;
                    if (ImGui::MenuItem("Ir a la linea...", "Ctrl+G")) { pedir_ir = true; ir_linea = d->ed.GetCursorPosition().mLine + 1; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Ver")) {
                    ImGui::SetNextItemWidth(140);
                    ImGui::SliderFloat("Tamanio del texto", &code_zoom, 0.7f, 2.0f, "%.2f");
                    if (ImGui::MenuItem("Ver espacios y tabuladores", nullptr, ver_espacios)) ver_espacios = !ver_espacios;
                    ImGui::SetNextItemWidth(100);
                    ImGui::SliderInt("Tabulacion", &tabulacion, 2, 8);
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Compilar  (F5)", nullptr, false, d != nullptr)) ped_compilar = true;
                ImGui::EndMenuBar();
            }

            // ------------------------- atajos de teclado -------------------------
            {
                ImGuiIO& io = ImGui::GetIO();
                bool ctrl = io.KeyCtrl, shift = io.KeyShift;
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) { if (shift) ped_guardar_todo = true; else ped_guardar = true; }
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) { nombre_nuevo[0] = 0; pedir_nuevo = true; }
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_W, false)) cerrar_doc = doc_sel;
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_F, false)) { buscar_abierto = true; foco_buscar = true; }
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_G, false) && d) { pedir_ir = true; ir_linea = d->ed.GetCursorPosition().mLine + 1; }
                if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) { if (shift) ped_buscar_ant = true; else ped_buscar_sig = true; }
                if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) ped_compilar = true;
                if (!ac_abierto && !buscar_abierto && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) &&
                    ImGui::IsKeyPressed(ImGuiKey_Escape, false)) show_script = false;
            }

            // =============================== barra de buscar ===============================
            if (buscar_abierto) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.20f, 1.0f));
                ImGui::BeginChild("barra_buscar", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2 + 8), true);
                ImGui::SetNextItemWidth(260);
                if (foco_buscar) { ImGui::SetKeyboardFocusHere(); foco_buscar = false; }
                if (ImGui::InputText("Buscar", buscar_txt, sizeof(buscar_txt), ImGuiInputTextFlags_EnterReturnsTrue))
                    ped_buscar_sig = true;
                ImGui::SameLine(); if (ImGui::Button("< Ant")) ped_buscar_ant = true;
                ImGui::SameLine(); if (ImGui::Button("Sig >")) ped_buscar_sig = true;
                ImGui::SameLine(); ImGui::Checkbox("May/min", &buscar_case);
                ImGui::SameLine(ImGui::GetWindowWidth() - 90);
                if (ImGui::Button("Cerrar##buscar")) buscar_abierto = false;
                ImGui::SetNextItemWidth(260);
                ImGui::InputText("Reemplazar por", reemp_txt, sizeof(reemp_txt));
                ImGui::SameLine();
                if (ImGui::Button("Reemplazar") && d) {
                    // Si lo que hay seleccionado es la coincidencia, se cambia; si no,
                    // primero se salta a la siguiente.
                    std::string sel = d->ed.GetSelectedText();
                    bool igual = buscar_case ? (sel == buscar_txt) : (minus(sel) == minus(buscar_txt));
                    if (igual && !sel.empty()) { d->ed.Delete(); d->ed.InsertText(reemp_txt); }
                    ped_buscar_sig = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reemplazar todo") && d && buscar_txt[0]) {
                    d->ed.SetCursorPosition(TextEditor::Coordinates(0, 0));
                    d->ed.SetSelection(TextEditor::Coordinates(0, 0), TextEditor::Coordinates(0, 0));
                    int n = 0; bool vuelta = false;
                    // Se para al dar la vuelta: si lo nuevo contiene lo viejo
                    // (cambiar "a" por "aa") si no, no acabaria nunca.
                    while (n < 20000 && buscar_en(d, false, &vuelta) && !vuelta) {
                        d->ed.Delete(); d->ed.InsertText(reemp_txt); n++;
                    }
                    compile_out = "Reemplazadas " + std::to_string(n) + " coincidencias.";
                    compile_err.clear();
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            // =============================== cuerpo ===============================
            float alto_salida = compile_out.empty() ? 0.0f : 170.0f;
            ImGui::BeginChild("cuerpo_code", ImVec2(0, -alto_salida), false);

            // ---------------- panel izquierdo: ficheros + esquema ----------------
            ImGui::BeginChild("panel_code", ImVec2(230, 0), true);
            ImGui::TextDisabled("SCRIPTS DEL PROYECTO");
            ImGui::SameLine(ImGui::GetWindowWidth() - 32);
            if (ImGui::SmallButton("+")) { nombre_nuevo[0] = 0; pedir_nuevo = true; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Nuevo script (Ctrl+N)");
            ImGui::Separator();
            {
                double ahora = ImGui::GetTime();
                if (fich_t < 0.0 || ahora - fich_t > 0.5) { fich_cache = prg_de_scripts(); fich_t = ahora; }
                ImGui::BeginChild("lista_fich", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.55f), false);
                for (auto& n : fich_cache) {
                    std::string ruta = scripts_dir + "/" + n;
                    int di = doc_de(ruta);
                    bool abierto = (di >= 0);
                    bool sel = (di >= 0 && di == doc_sel);
                    std::string etiqueta = (abierto && docs[di]->sucio ? "* " : "  ") + n;
                    if (!abierto) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    if (ImGui::Selectable((etiqueta + "##f" + n).c_str(), sel)) code_abrir(ruta, n, "");
                    if (!abierto) ImGui::PopStyleColor();
                    if (ImGui::BeginPopupContextItem(("ctx" + n).c_str())) {
                        fich_menu = n;
                        if (ImGui::MenuItem("Abrir"))     code_abrir(ruta, n, "");
                        if (ImGui::MenuItem("Renombrar...")) {
                            snprintf(nombre_nuevo, sizeof(nombre_nuevo), "%s", fs::path(n).stem().string().c_str());
                            pedir_renombrar = true;
                        }
                        if (ImGui::MenuItem("Duplicar")) {
                            std::string base = fs::path(n).stem().string();
                            for (int k = 2; k < 100; k++) {
                                std::string cand = scripts_dir + "/" + base + "_" + std::to_string(k) + ".prg";
                                FILE* t = fopen(cand.c_str(), "r");
                                if (t) { fclose(t); continue; }
                                std::string txt; leer_todo(ruta, txt);
                                escribir_todo(cand, txt);
                                fich_t = -1.0;
                                code_abrir(cand, fs::path(cand).filename().string(), "");
                                break;
                            }
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Borrar...")) pedir_borrar_prg = true;
                        ImGui::EndPopup();
                    }
                }
                // main.prg no vive en Scripts, pero se edita igual
                ImGui::Separator();
                {
                    std::string mp = project_dir + "/main.prg";
                    int di = doc_de(mp);
                    if (ImGui::Selectable(((di >= 0 && docs[di]->sucio ? "* " : "  ") + std::string("main.prg")).c_str(),
                                          di >= 0 && di == doc_sel))
                        open_main_script();
                }
                ImGui::EndChild();
            }
            ImGui::Separator();
            ImGui::TextDisabled("PROCESOS Y FUNCIONES");
            ImGui::BeginChild("esquema", ImVec2(0, 0), false);
            if (d) {
                auto syms = prg_simbolos(d->ed.GetText());
                if (syms.empty()) ImGui::TextDisabled("(ninguno)");
                for (auto& sy : syms) {
                    ImGui::PushStyleColor(ImGuiCol_Text, sy.es_func ? ImVec4(0.55f, 0.85f, 1.0f, 1.0f)
                                                                    : ImVec4(0.75f, 1.0f, 0.65f, 1.0f));
                    bool clic = ImGui::Selectable((std::string(sy.es_func ? "f  " : "p  ") + sy.nombre +
                                                  "##sym" + std::to_string(sy.linea)).c_str());
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\nlinea %d", sy.firma.c_str(), sy.linea);
                    if (clic) code_ir_a(doc_sel, sy.linea);
                }
            }
            ImGui::EndChild();
            ImGui::EndChild();   // panel_code

            // ---------------- derecha: pestanias + editor ----------------
            ImGui::SameLine();
            ImGui::BeginChild("zona_editor", ImVec2(0, 0), false);
            if (docs.empty()) {
                ImGui::TextDisabled("No hay ningun fichero abierto.");
                ImGui::TextDisabled("Elige uno de la lista, o crea uno nuevo con el boton +.");
            } else if (ImGui::BeginTabBar("pestanias_code", ImGuiTabBarFlags_Reorderable |
                                                            ImGuiTabBarFlags_AutoSelectNewTabs |
                                                            ImGuiTabBarFlags_FittingPolicyScroll)) {
                for (int i = 0; i < (int)docs.size(); i++) {
                    bool abierta = true;
                    ImGuiTabItemFlags tf = docs[i]->sucio ? ImGuiTabItemFlags_UnsavedDocument : 0;
                    std::string et = docs[i]->titulo + "###doc" + std::to_string(i);
                    if (ImGui::BeginTabItem(et.c_str(), &abierta, tf)) {
                        doc_sel = i;
                        ImGui::EndTabItem();
                    }
                    if (!abierta) cerrar_doc = i;
                }
                ImGui::EndTabBar();
            }
            d = doc_activo();
            if (d) {
                // Aviso de que el fichero cambio por debajo (lo regenero el editor).
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 2));
                auto cur = d->ed.GetCursorPosition();
                ImGui::TextDisabled("%s", d->ruta.c_str());
                ImGui::SameLine(ImGui::GetWindowWidth() - 320);
                ImGui::Text("Ln %d, Col %d%s", cur.mLine + 1, cur.mColumn + 1, d->sucio ? "   *sin guardar" : "");
                ImGui::SameLine();
                if (ImGui::SmallButton("Guardar")) ped_guardar = true;
                ImGui::SameLine();
                if (ImGui::SmallButton("Compilar")) ped_compilar = true;
                ImGui::PopStyleVar();

                d->ed.SetShowWhitespaces(ver_espacios);
                d->ed.SetTabSize(tabulacion);

                // Con el desplegable del autocompletado abierto, las flechas y el
                // Enter son suyos: si no, el editor moveria el cursor a la vez.
                bool robar = false;
                if (ac_abierto && !ac_lista.empty()) {
                    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) { ac_sel = (ac_sel + 1) % (int)ac_lista.size(); robar = true; }
                    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))   { ac_sel = (ac_sel - 1 + (int)ac_lista.size()) % (int)ac_lista.size(); robar = true; }
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))   { ac_abierto = false; robar = true; }
                    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_Tab, false)) {
                        // se cambia la palabra a medias por la elegida
                        auto c = d->ed.GetCursorPosition();
                        d->ed.SetSelection(TextEditor::Coordinates(c.mLine, ac_col), c);
                        d->ed.Delete();
                        d->ed.InsertText(ac_lista[ac_sel].first);
                        ac_abierto = false; robar = true;
                    }
                }
                if (robar) d->ed.SetHandleKeyboardInputs(false);

                ImGui::BeginChild("zona_codigo", ImVec2(0, 0), false);
                ImGui::SetWindowFontScale(code_zoom);
                d->ed.Render("CodeEditor", ImVec2(0, 0));
                ImGui::EndChild();
                if (robar) d->ed.SetHandleKeyboardInputs(true);

                if (d->ed.IsTextChanged()) {
                    d->sucio = true;
                    // ---- autocompletado: la palabra que se esta escribiendo ----
                    auto c = d->ed.GetCursorPosition();
                    std::string linea = d->ed.GetCurrentLineText();
                    size_t idx = col_a_idx(linea, c.mColumn), b = idx;
                    while (b > 0 && (isalnum((unsigned char)linea[b-1]) || linea[b-1] == '_')) b--;
                    std::string pref = linea.substr(b, idx - b);
                    ac_col = idx_a_col(linea, b);
                    ac_lista.clear(); ac_sel = 0;
                    if (pref.size() >= 2) {
                        std::string pm = minus(pref);
                        auto cabe = [&](const std::string& cand) {
                            return cand.size() > pref.size() && minus(cand).rfind(pm, 0) == 0;
                        };
                        const auto& lang = d->ed.GetLanguageDefinition();
                        for (auto& k : lang.mKeywords) if (cabe(k)) ac_lista.push_back({ k, "palabra clave" });
                        for (auto& it : lang.mIdentifiers) if (cabe(it.first)) ac_lista.push_back({ it.first, it.second.mDeclaration });
                        // lo tuyo: los procesos y funciones de los ficheros abiertos
                        for (auto& dd : docs)
                            for (auto& sy : prg_simbolos(dd->ed.GetText()))
                                if (cabe(sy.nombre)) ac_lista.push_back({ sy.nombre, sy.firma + "   (" + dd->titulo + ")" });
                        std::sort(ac_lista.begin(), ac_lista.end());
                        ac_lista.erase(std::unique(ac_lista.begin(), ac_lista.end()), ac_lista.end());
                        if (ac_lista.size() > 40) ac_lista.resize(40);
                    }
                    ac_abierto = !ac_lista.empty();
                }
                if (ac_abierto && !ac_lista.empty()) {
                    // Se pinta a mano encima de todo: una ventana de ImGui le
                    // robaria el foco al editor y dejarias de escribir.
                    ImVec2 p = d->ed.GetCursorScreenPos();
                    float lh = ImGui::GetTextLineHeightWithSpacing();
                    int n = (int)ac_lista.size(); if (n > 8) n = 8;
                    int desde = ac_sel - n + 1; if (desde < 0) desde = 0;
                    float w = 340.0f, h = n * lh + 8.0f;
                    ImDrawList* dl = ImGui::GetForegroundDrawList();
                    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(30, 32, 40, 245), 4.0f);
                    dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(90, 130, 200, 255), 4.0f);
                    for (int k = 0; k < n; k++) {
                        int i = desde + k;
                        float y = p.y + 4.0f + k * lh;
                        if (i == ac_sel)
                            dl->AddRectFilled(ImVec2(p.x + 2, y), ImVec2(p.x + w - 2, y + lh), IM_COL32(60, 110, 190, 255), 3.0f);
                        dl->AddText(ImVec2(p.x + 8, y + 1), IM_COL32(235, 235, 235, 255), ac_lista[i].first.c_str());
                        if (!ac_lista[i].second.empty())
                            dl->AddText(ImVec2(p.x + 150, y + 1), IM_COL32(150, 150, 160, 255), ac_lista[i].second.c_str());
                    }
                }
                if (d->ed.IsCursorPositionChanged() && !d->ed.IsTextChanged()) ac_abierto = false;
            }
            ImGui::EndChild();   // zona_editor
            ImGui::EndChild();   // cuerpo_code

            // ---------------- salida del compilador ----------------
            if (alto_salida > 0.0f) {
                ImGui::Separator();
                ImGui::BeginChild("salida_code", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                if (!compile_err.empty()) {
                    ImGui::TextDisabled("Doble clic en un error para ir a su linea:");
                    for (auto& e : compile_err) {
                        std::string et = fs::path(e.first).filename().string() + ":" + std::to_string(e.second);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.5f, 1.0f));
                        if (ImGui::Selectable(et.c_str())) {
                            int di = doc_de(e.first);
                            if (di < 0) di = code_abrir(e.first, fs::path(e.first).filename().string(), "");
                            code_ir_a(di, e.second);
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::Separator();
                }
                ImGui::TextUnformatted(compile_out.c_str());
                ImGui::EndChild();
            }

            // =============================== acciones ===============================
            if (ped_guardar && d) {
                if (code_guardar(doc_sel)) { compile_out = "Guardado: " + d->ruta; compile_err.clear(); fich_t = -1.0; }
                else                       { compile_out = "ERROR: no puedo escribir " + d->ruta; }
            }
            if (ped_guardar_todo) {
                int n = 0;
                for (int i = 0; i < (int)docs.size(); i++) if (docs[i]->sucio && code_guardar(i)) n++;
                compile_out = "Guardados " + std::to_string(n) + " ficheros."; compile_err.clear(); fich_t = -1.0;
            }
            if ((ped_buscar_sig || ped_buscar_ant) && d) {
                bool v = false;
                if (!buscar_en(d, ped_buscar_ant, &v)) {
                    compile_out = std::string("No se encuentra: ") + buscar_txt; compile_err.clear();
                }
            }
            if (ped_compilar && d) {
                code_guardar(doc_sel);
                fich_t = -1.0;
                /* Se compila EL FICHERO, envuelto en un main de mentira que lo
                   incluye si el no trae uno. Asi bgdc da la linea del fichero de
                   verdad y el error se puede marcar donde esta. */
                std::string txt = d->ed.GetText(), may;
                for (char c : txt) may += (char)toupper((unsigned char)c);
                bool tiene_main = (may.find("PROCESS MAIN") != std::string::npos);
                std::string objetivo = d->ruta;
                std::string tmp = scripts_dir + "/__check.prg";
                if (!tiene_main) {
                    FILE* f2 = fopen(tmp.c_str(), "w");
                    if (f2) {
                        fputs("import \"libmod_gfx\";\nimport \"libmod_misc\";\n"
                              "import \"libmod_input\";\nimport \"libmod_sound\";\n"
                              "import \"libmod_3d\";\n", f2);
                        fprintf(f2, "#include \"%s\"\n", d->ruta.c_str());
                        fputs("PROCESS main()\nBEGIN\nEND\n", f2);
                        fclose(f2);
                        objetivo = tmp;
                    }
                }
                compile_out.clear(); compile_err.clear();
                std::string cmd = ruta_util("lib/bgdc", BGDC_PATH) + " \"" + objetivo + "\" 2>&1";
                FILE* p = popen(cmd.c_str(), "r");
                if (p) {
                    char buf[512]; size_t n;
                    while ((n = fread(buf, 1, sizeof(buf) - 1, p)) > 0) { buf[n] = 0; compile_out += buf; }
                    int rc = pclose(p);
                    // "<fichero>:<linea>: error: ..." -> marca en el margen
                    TextEditor::ErrorMarkers marcas;
                    { std::vector<std::string> ls = en_lineas(compile_out);
                      for (auto& l : ls) {
                          size_t p2 = l.rfind(": error");
                          if (p2 == std::string::npos) p2 = l.rfind(": warning");
                          if (p2 == std::string::npos) continue;
                          size_t p1 = l.rfind(':', p2 - 1);
                          if (p1 == std::string::npos) continue;
                          std::string sn = l.substr(p1 + 1, p2 - p1 - 1);
                          if (sn.empty() || sn.find_first_not_of("0123456789") != std::string::npos) continue;
                          std::string fich = l.substr(0, p1);
                          int ln = atoi(sn.c_str());
                          compile_err.push_back({ fich, ln });
                          if (fich == d->ruta) marcas[ln] = l.substr(p2 + 2);
                      }
                    }
                    d->ed.SetErrorMarkers(marcas);
                    compile_out += (rc == 0) ? "\n[OK] compila." : "\n[FALLO] revisa los errores.";
                    if (rc == 0) { compile_err.clear(); d->ed.SetErrorMarkers(TextEditor::ErrorMarkers()); }
                    else if (!compile_err.empty()) {
                        int di = doc_de(compile_err[0].first);
                        if (di >= 0) code_ir_a(di, compile_err[0].second);
                    }
                } else compile_out = "ERROR: no pude ejecutar bgdc";
                { std::error_code ec; fs::remove(tmp, ec); }
                /* El .dcb de la PRUEBA no pinta nada en el proyecto. El de main.prg
                   si: es el juego compilado, y borrarlo dejaria sin nada que
                   ejecutar a quien acaba de darle a compilar. */
                if (!tiene_main) { std::error_code ec;
                                   fs::remove(fs::path(objetivo).replace_extension(".dcb"), ec); }
            }

            // ---------------- cerrar pestania (con aviso si hay cambios) ----------------
            if (cerrar_doc >= 0 && cerrar_doc < (int)docs.size()) {
                if (docs[cerrar_doc]->sucio) ImGui::OpenPopup("Cambios sin guardar");
                else {
                    docs.erase(docs.begin() + cerrar_doc);
                    if (doc_sel >= (int)docs.size()) doc_sel = (int)docs.size() - 1;
                    cerrar_doc = -1;
                }
            } else cerrar_doc = -1;
            if (ImGui::BeginPopupModal("Cambios sin guardar", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (cerrar_doc >= 0 && cerrar_doc < (int)docs.size())
                    ImGui::Text("%s tiene cambios sin guardar.", docs[cerrar_doc]->titulo.c_str());
                if (ImGui::Button("Guardar y cerrar", ImVec2(160, 0))) {
                    code_guardar(cerrar_doc);
                    docs.erase(docs.begin() + cerrar_doc);
                    if (doc_sel >= (int)docs.size()) doc_sel = (int)docs.size() - 1;
                    cerrar_doc = -1; ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cerrar sin guardar", ImVec2(160, 0))) {
                    docs.erase(docs.begin() + cerrar_doc);
                    if (doc_sel >= (int)docs.size()) doc_sel = (int)docs.size() - 1;
                    cerrar_doc = -1; ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(110, 0))) { cerrar_doc = -1; ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            // ---------------- nuevo / renombrar / borrar / ir a la linea ----------------
            if (pedir_nuevo) { ImGui::OpenPopup("Nuevo script"); pedir_nuevo = false; }
            if (ImGui::BeginPopupModal("Nuevo script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("Nombre del fichero (sin .prg):");
                ImGui::SetNextItemWidth(280);
                bool ok = ImGui::InputText("##nn", nombre_nuevo, sizeof(nombre_nuevo),
                                           ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::TextDisabled("Se creara con un PROCESS del mismo nombre, listo para llamarlo\ndesde un objeto o un personaje.");
                if (ImGui::Button("Crear", ImVec2(120, 0)) || ok) {
                    std::string nom = ident_bgd(nombre_nuevo, "script");
                    std::string ruta = scripts_dir + "/" + nom + ".prg";
                    FILE* t = fopen(ruta.c_str(), "r");
                    if (t) { fclose(t); code_abrir(ruta, nom + ".prg", ""); }
                    else {
                        std::string plant =
                            "// " + nom + ": codigo tuyo. Es un PROCESS normal de BennuGD2, asi que\n"
                            "// puede durar varios FRAME o acabar enseguida con RETURN.\n"
                            "PROCESS " + nom + "()\n"
                            "BEGIN\n"
                            "    // ---- tu codigo aqui ----\n"
                            "    RETURN;\n"
                            "END\n";
                        escribir_todo(ruta, plant);
                        fich_t = -1.0;
                        code_abrir(ruta, nom + ".prg", "");
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (pedir_renombrar) { ImGui::OpenPopup("Renombrar script"); pedir_renombrar = false; }
            if (ImGui::BeginPopupModal("Renombrar script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Renombrar %s", fich_menu.c_str());
                ImGui::SetNextItemWidth(280);
                bool ok = ImGui::InputText("##rn", nombre_nuevo, sizeof(nombre_nuevo),
                                           ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1),
                    "Ojo: si algun objeto o personaje llama a este fichero,\nhay que volver a elegirlo en su Inspector.");
                if (ImGui::Button("Renombrar", ImVec2(120, 0)) || ok) {
                    std::string nom = ident_bgd(nombre_nuevo, "script");
                    std::string vieja = scripts_dir + "/" + fich_menu;
                    std::string nueva = scripts_dir + "/" + nom + ".prg";
                    std::error_code ec;
                    fs::rename(vieja, nueva, ec);
                    if (!ec) {
                        int di = doc_de(vieja);
                        if (di >= 0) { docs[di]->ruta = nueva; docs[di]->titulo = nom + ".prg"; }
                        fich_t = -1.0;
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (pedir_borrar_prg) { ImGui::OpenPopup("Borrar script"); pedir_borrar_prg = false; }
            if (ImGui::BeginPopupModal("Borrar script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Se va a borrar Scripts/%s", fich_menu.c_str());
                ImGui::TextColored(ImVec4(1, 0.5f, 0.4f, 1), "Esto no se puede deshacer.");
                if (ImGui::Button("Borrar", ImVec2(120, 0))) {
                    std::string ruta = scripts_dir + "/" + fich_menu;
                    std::error_code ec; fs::remove(ruta, ec);
                    int di = doc_de(ruta);
                    if (di >= 0) {
                        docs.erase(docs.begin() + di);
                        if (doc_sel >= (int)docs.size()) doc_sel = (int)docs.size() - 1;
                    }
                    fich_t = -1.0;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (pedir_ir) { ImGui::OpenPopup("Ir a la linea"); pedir_ir = false; }
            if (ImGui::BeginPopupModal("Ir a la linea", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::SetNextItemWidth(120);
                bool ok = ImGui::InputInt("Linea", &ir_linea, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue);
                if (ImGui::Button("Ir", ImVec2(120, 0)) || ok) { code_ir_a(doc_sel, ir_linea); ImGui::CloseCurrentPopup(); }
                ImGui::SameLine();
                if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        /* --- VENTANITA: lo que vera el jugador ---
           El frustum de alambre dice donde esta la camara, pero no como queda el
           encuadre. Esto es la escena renderizada de verdad desde ella, mientras
           se coloca. */
        if (show_cam_view) {
            ImGui::SetNextWindowSize(ImVec2(400, 260), ImGuiCond_FirstUseEver);
            ImGui::Begin("Vista de la camara", &show_cam_view);
            if (!camview_ok) {
                ImGui::TextColored(ImVec4(1,0.7f,0.2f,1), "La camara sigue a un objeto");
                ImGui::TextWrapped("Elige a cual en el panel de Camara y aqui saldra su vista.");
            } else {
                ImVec2 av = ImGui::GetContentRegionAvail();
                if (av.y < 40.0f) av.y = 40.0f;
                /* Se respeta la proporcion de la ventana del juego: si aqui se
                   viera mas ancho de lo que se vera jugando, el encuadre que
                   eliges no es el que sale. */
                float ar = 16.0f / 9.0f;
                float w = av.x, h = w / ar;
                if (h > av.y) { h = av.y; w = h * ar; }
                if (w < 16.0f) { w = 16.0f; h = 9.0f; }
                camFbo.resize((int)w, (int)h);
                ImGui::Image((ImTextureID)(intptr_t)camFbo.tex, ImVec2(w, h), ImVec2(0,0), ImVec2(1,1));
                const char *nm[4] = { "fija", "tercera persona", "primera persona", "cenital" };
                ImGui::TextDisabled("%s%s", nm[cam_mode & 3],
                                    (cam_mode == 1 && cam_25d) ? "  -  2.5D" : "");
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

        /* Ya creadas todas las ventanas del frame, se trae al frente la pestania
           que manda en cada modo. Antes de crearlas no se puede: SetWindowFocus
           necesita que la ventana exista. */
        if (poner_delante) {
            poner_delante = false;
            switch (modo) {
            case M_ESCENA:     ImGui::SetWindowFocus("Assets");
                               ImGui::SetWindowFocus("Jerarquia"); break;
            case M_TERRENO:    ImGui::SetWindowFocus("Assets");
                               ImGui::SetWindowFocus("Entorno");   break;
            case M_PERSONAJES: ImGui::SetWindowFocus("Assets");
                               ImGui::SetWindowFocus("Jerarquia");
                               ImGui::SetWindowFocus(ICON_FA_PERSON_RUNNING "  Sprites 3D"); break;
            case M_INTERFAZ:   ImGui::SetWindowFocus("Dialogos del juego");
                               ImGui::SetWindowFocus("Menus del juego");
                               ImGui::SetWindowFocus(ICON_FA_FONT "  HUD 2D"); break;
            case M_CODIGO:     ImGui::SetWindowFocus("Escenas del proyecto");
                               ImGui::SetWindowFocus("Variables del juego");
                               ImGui::SetWindowFocus("Editor de codigo"); break;
            }
            ImGui::SetWindowFocus("Escena");   // la vista 3D, siempre en su sitio
        }
        if (!enfocar_panel.empty()) {
            ImGui::SetWindowFocus(enfocar_panel.c_str());
            enfocar_panel.clear();
        }

        ImGui::Render();

        // aplicar transformaciones de los objetos colocados (en Play manda el emulador)
        if (!playing)
            for (auto& o : objects) {
                g3d_entity_impl_set_position(o.entity, o.x, o.y, o.z);
                g3d_entity_impl_set_rotation(o.entity, 0.0f, o.ry, 0.0f);
                g3d_entity_impl_set_scale(o.entity, o.scale, o.scale, o.scale);
            }

        // ---- sol ----
        // El mismo calculo que se emite al juego, para que lo que ves aqui sea
        // lo que luego corre: si el editor lo aproximara de otra forma, el ciclo
        // se veria distinto al jugar.
        {
            if (sun_cycle && sun_day_sec > 0.1f)
                sun_hour += 360.0f * ImGui::GetIO().DeltaTime / sun_day_sec;
            while (sun_hour >= 360.0f) sun_hour -= 360.0f;
            float ang = sun_cycle ? sun_hour : sun_elev;
            float el  = sun_cycle ? sinf(sun_hour * 3.14159265f / 180.0f)
                                  : sinf(sun_elev * 3.14159265f / 180.0f);
            float az  = sun_cycle ? sun_hour : sun_azim;
            float k = el < 0.0f ? 0.0f : el;
            float dx = -cosf(az * 3.14159265f / 180.0f) * 0.8f;
            float dz = sun_cycle ? -0.35f
                                 : -sinf(sun_azim * 3.14159265f / 180.0f) * 0.8f;
            g3d_light_set_direction(light, dx, -el, dz);
            g3d_light_set_intensity(light, 0.05f + sun_intensity * k);
            g3d_light_impl_set_color(light, 1.0f,
                                (150.0f + 105.0f * k) / 255.0f,
                                (90.0f + 165.0f * k) / 255.0f);
            g3d_sky_set_gradient(0.05f + 0.30f*k, 0.07f + 0.48f*k, 0.15f + 0.70f*k,
                                 0.10f + 0.72f*k, 0.09f + 0.79f*k, 0.18f + 0.78f*k);
            (void)ang;
        }

        // ---- agua (mar/lago global) ----
        g3d_water_render_set_surf(surf_amount, surf_len, surf_speed, surf_runup);
        g3d_water_render_set_surf_wave(surf_height, surf_dir);
        g3d_water_render_set_detail(water_foam, 1.0f);
        g3d_water_splash_set_amount(splash_amount);
        g3d_water_splash_set_threshold(splash_speed);
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

        /* ---- VENTANITA: la escena vista DESDE LA CAMARA DEL JUEGO ----
           Se reutiliza la camara del editor movida a su sitio, no una segunda:
           asi lo que se ve aqui es exactamente lo que dibuja el motor. La camara
           del editor se recalcula entera al principio del frame siguiente
           (vcam_target + yaw + pitch + distancia), asi que moverla aqui no deja
           rastro y no hace falta guardar nada. */
        if (show_cam_view && camview_ok && camFbo.w > 0) {
            g3d_camera_set_position(cam, camview_p[0], camview_p[1], camview_p[2]);
            g3d_camera_look_at(cam, camview_t[0], camview_t[1], camview_t[2], 0.0f, 1.0f, 0.0f);
            g3d_editor_set_aspect(camFbo.h > 0 ? (float)camFbo.w / (float)camFbo.h : 1.777f);
            g3d_renderer_set_target(camFbo.fbo);
            g3d_renderer_set_viewport_physical(0, 0, (unsigned)camFbo.w, (unsigned)camFbo.h);
            glBindFramebuffer(GL_FRAMEBUFFER, camFbo.fbo);
            glViewport(0, 0, camFbo.w, camFbo.h);
            g3d_renderer_render();
            /* Y se devuelve donde estaba: si no, el viewport de abajo se dibuja
               desde la camara del juego y parece que el editor ha dado un salto. */
            float rx = vcam_target[0] + sinf(cam_yaw) * cosf(cam_pitch) * cam_dist;
            float ry = vcam_target[1] + sinf(cam_pitch) * cam_dist + 1.0f;
            float rz = vcam_target[2] + cosf(cam_yaw) * cosf(cam_pitch) * cam_dist;
            g3d_camera_set_position(cam, rx, ry, rz);
            g3d_camera_look_at(cam, vcam_target[0], vcam_target[1], vcam_target[2], 0.0f, 1.0f, 0.0f);
        }

        // ---- sprites 2D del mundo: se refrescan y se animan tambien en el editor,
        //      con el mismo dibujante del juego (lo que ves es lo que corre) ----
        for (auto& so : sprites) spr_sync(so, ImGui::GetIO().DeltaTime);

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
        // Los paneles que hayas sacado fuera del editor son ventanas del sistema:
        // se pintan aqui, y despues hay que devolver el contexto GL a la principal.
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            SDL_Window*   back_w = SDL_GL_GetCurrentWindow();
            SDL_GLContext back_c = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(back_w, back_c);
        }

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
