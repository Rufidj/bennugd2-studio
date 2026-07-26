// ============================================================================
//  engine_bridge.c - puente entre el editor y el CORE de libmod_3d.
//
//  El core expone la capa _impl (registros con handles int + camara por puntero).
//  La API publica de handles (g3d_scene_create, g3d_model_spawn, ...) vive en
//  libmod_3d.c, que NO compilamos (lleva la capa BennuGD2). Aqui reimplementamos
//  solo lo que el editor necesita, delegando en las funciones _impl del core.
//  g3d_model_spawn es una copia de la de libmod_3d.c SIN el LOD lejano.
// ============================================================================
#include "libmod_3d_scene.h"
#include "libmod_3d_camera.h"
#include "libmod_3d_light.h"
#include "libmod_3d_entity.h"
#include "libmod_3d_material.h"
#include "libmod_3d_renderer.h"
#include "libmod_3d_mesh.h"
#include "libmod_3d_gltf.h"
#include "libmod_3d_math.h"
#include "libmod_3d_primitives.h"
#include "libmod_3d_texture.h"
#include "libmod_3d_terrain.h"
#include "libmod_3d_pick.h"
#include "libmod_3d_paint.h"
#include "libmod_3d_water.h"
#include "libmod_3d_flow.h"
#include "libmod_3d_cave.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static G3DCamera *g_editor_cam = NULL;

/* Matrices de la camara (column-major, para ImGuizmo) y aspect del viewport. */
void g3d_editor_get_view(float *m16) {
    if (g_editor_cam) memcpy(m16, g_editor_cam->view_matrix.m, 16 * sizeof(float));
}
void g3d_editor_get_proj(float *m16) {
    if (g_editor_cam) memcpy(m16, g_editor_cam->projection_matrix.m, 16 * sizeof(float));
}
void g3d_editor_set_aspect(float a) {
    if (g_editor_cam && a > 0.0f) { g_editor_cam->aspect_ratio = a; g_editor_cam->proj_dirty = 1; }
}

/* Rayo de pantalla (sx,sy en [0,w]x[0,h]) intersecado con el plano y=planeY.
   Devuelve 1 y el punto en out[3]. Base para colocar objetos con el raton. */
int g3d_editor_ray_plane(float sx, float sy, float w, float h, float planeY, float *out) {
    if (!g_editor_cam || w < 1.0f || h < 1.0f) return 0;
    Mat4 inv = mat4_inverse(g_editor_cam->view_projection);
    float ndcx = 2.0f * sx / w - 1.0f;
    float ndcy = 1.0f - 2.0f * sy / h;
    Vec4 pn = mat4_mul_vec4(inv, vec4_make(ndcx, ndcy, -1.0f, 1.0f));
    Vec4 pf = mat4_mul_vec4(inv, vec4_make(ndcx, ndcy,  1.0f, 1.0f));
    if (fabsf(pn.w) < 1e-6f || fabsf(pf.w) < 1e-6f) return 0;
    Vec3 a = vec3_make(pn.x / pn.w, pn.y / pn.w, pn.z / pn.w);
    Vec3 b = vec3_make(pf.x / pf.w, pf.y / pf.w, pf.z / pf.w);
    Vec3 o = g_editor_cam->position;
    Vec3 d = vec3_normalize(vec3_sub(b, a));
    if (fabsf(d.y) < 1e-6f) return 0;
    float t = (planeY - o.y) / d.y;
    if (t < 0.0f) return 0;
    out[0] = o.x + d.x * t; out[1] = planeY; out[2] = o.z + d.z * t;
    return 1;
}

/* DBG: imprime la camara real y a que NDC proyecta un punto del mundo */
void g3d_editor_dbg_camera(void) {
    if (!g_editor_cam) { printf("DBG cam = NULL\n"); return; }
    G3DCamera *c = g_editor_cam;
    printf("DBG cam pos=(%.2f,%.2f,%.2f) tgt=(%.2f,%.2f,%.2f) fov=%.1f aspect=%.3f\n",
           c->position.x, c->position.y, c->position.z,
           c->target.x, c->target.y, c->target.z, c->fov, c->aspect_ratio);
    float *m = c->view_projection.m;
    float px = 0, py = 2, pz = 0;
    float X = m[0]*px + m[4]*py + m[8]*pz + m[12];
    float Y = m[1]*px + m[5]*py + m[9]*pz + m[13];
    float Z = m[2]*px + m[6]*py + m[10]*pz + m[14];
    float W = m[3]*px + m[7]*py + m[11]*pz + m[15];
    printf("DBG origen(0,2,0) -> NDC (%.3f, %.3f, %.3f) w=%.3f  [~0,0=centro]\n",
           W != 0 ? X/W : 0, W != 0 ? Y/W : 0, W != 0 ? Z/W : 0, W);
}

/* -------- escena -------- */
int g3d_scene_create(const char *name)   { return g3d_scene_impl_create(name); }
int g3d_scene_set_active(int scene_id)    { return g3d_scene_impl_set_active(scene_id); }

/* -------- camara (una sola, por puntero, enchufada al renderer) -------- */
int g3d_camera_create(void) {
    g_editor_cam = g3d_camera_impl_create(G3D_CAMERA_PERSPECTIVE);
    g3d_renderer_set_camera(g_editor_cam);
    return 1;
}
int g3d_camera_set_active(int camera_id) {
    (void)camera_id;
    if (g_editor_cam) g3d_renderer_set_camera(g_editor_cam);
    return 1;
}
int g3d_camera_set_position(int camera_id, float x, float y, float z) {
    (void)camera_id;
    if (g_editor_cam) g3d_camera_set_position_impl(g_editor_cam, vec3_make(x, y, z));
    return 1;
}
int g3d_camera_look_at(int camera_id, float tx, float ty, float tz,
                       float ux, float uy, float uz) {
    (void)camera_id;
    if (g_editor_cam)
        g3d_camera_look_at_impl(g_editor_cam, vec3_make(tx, ty, tz), vec3_make(ux, uy, uz));
    return 1;
}

/* -------- luz (se anade a la escena activa) -------- */
int g3d_light_create(int type, float r, float g, float b) {
    int id = g3d_light_impl_create(type, r, g, b);
    if (id >= 0) g3d_scene_impl_add_light(g3d_scene_impl_get_active(), id);
    return id;
}
int g3d_light_set_direction(int light_id, float dx, float dy, float dz) {
    return g3d_light_impl_set_direction(light_id, dx, dy, dz);
}
int g3d_light_set_intensity(int light_id, float intensity) {
    return g3d_light_impl_set_intensity(light_id, intensity);
}
int g3d_light_enable_shadow(int light_id, int enabled) {
    return g3d_light_impl_enable_shadow(light_id, enabled);
}

/* -------- TERRENO: crear malla esculpible + esculpir + pintar + picking -------- */
static G3DPaintCanvas *g_terrain_canvas = NULL;   /* lienzo de albedo del terreno */
static int g_terrain_entity = -1;                 /* entidad del terreno (para ocultar en modo cueva) */

/* Crea un terreno plano con un LIENZO de pintura como albedo (relleno con la
   textura base). El terreno mapea el lienzo 1:1 (UV 0..1); el lienzo tilea la
   textura internamente. Devuelve el puntero a la malla. */
void *g3d_editor_make_terrain(int scene_id, int grid, float worldsize,
                              float paint_tiling, const char *texpath) {
    G3DMesh *m = g3d_primitive_create_terrain(grid, worldsize, 0.0f, 1.0f, 1);
    if (!m) return NULL;
    g3d_mesh_upload_gpu(m);
    int mat = g3d_material_impl_create();
    G3DMaterial *mm = g3d_material_impl_get(mat);
    G3DTexture *base = texpath ? g3d_texture_load_impl(texpath) : NULL;
    if (base) g3d_texture_upload_gpu(base);
    g_terrain_canvas = g3d_paint_create(1024, 1024);
    if (base && g_terrain_canvas) g3d_paint_fill(g_terrain_canvas, base, paint_tiling);
    if (mm) {
        mm->roughness = 0.95f; mm->metallic = 0.0f;
        mm->albedo_texture = g_terrain_canvas ? g3d_paint_get_texture(g_terrain_canvas) : base;
        mm->albedo_texture_id = 0;
    }
    int e = g3d_entity_impl_spawn(scene_id, 0, 0.0f, 0.0f, 0.0f);
    G3DEntity *ent = g3d_entity_impl_get(e);
    if (ent) ent->mesh = m;
    g3d_entity_impl_set_material(e, mat);
    g_terrain_entity = e;
    return m;
}
/* Carga una textura (para el pincel de pintar). Devuelve G3DTexture* (void*). */
void *g3d_editor_load_texture(const char *path) {
    G3DTexture *t = g3d_texture_load_impl(path);
    if (t) g3d_texture_upload_gpu(t);
    return t;
}
/* Pinta la textura tex en el lienzo del terreno bajo (x,z). */
void g3d_editor_terrain_paint(void *mesh, void *tex, float tiling,
                              float x, float z, float r, float opacity) {
    if (mesh && tex && g_terrain_canvas)
        g3d_terrain_paint((G3DMesh *)mesh, g_terrain_canvas, (G3DTexture *)tex,
                          tiling, x, z, r, opacity);
}
/* Guardar/cargar el PINTADO del terreno (imagen del lienzo). */
void g3d_editor_paint_save(const char *path) {
    if (g_terrain_canvas) g3d_paint_save(g_terrain_canvas, path);
}
int g3d_editor_paint_load(const char *path) {
    if (!g_terrain_canvas) return 0;
    G3DTexture *t = g3d_texture_load_impl(path);
    if (!t) return 0;
    g3d_paint_fill(g_terrain_canvas, t, 1.0f);   /* restaura el lienzo 1:1 */
    return 1;
}
/* Punto del terreno bajo el raton (out[3]); 1 si acierta. */
int g3d_editor_terrain_pick(float sx, float sy, float w, float h, void *mesh, float *out) {
    if (!g_editor_cam || !mesh) return 0;
    if (!g3d_pick_terrain(g_editor_cam, sx, sy, w, h, (G3DMesh *)mesh)) return 0;
    out[0] = g3d_pick_x(); out[1] = g3d_pick_y(); out[2] = g3d_pick_z();
    return 1;
}
void g3d_editor_terrain_raise(void *mesh, float x, float z, float r, float amt) {
    if (mesh) g3d_terrain_raise((G3DMesh *)mesh, x, z, r, amt);
}
void g3d_editor_terrain_smooth(void *mesh, float x, float z, float r, float amt) {
    if (mesh) g3d_terrain_smooth((G3DMesh *)mesh, x, z, r, amt);
}
void g3d_editor_terrain_flatten(void *mesh, float x, float z, float r, float amt) {
    if (!mesh) return;
    float th = g3d_terrain_get_height((G3DMesh *)mesh, x, z);
    g3d_terrain_flatten((G3DMesh *)mesh, x, z, r, th, amt);
}
/* Nivelar hacia una altura DADA (para excavar un cauce de rio a un lecho). */
void g3d_editor_terrain_flatten_to(void *mesh, float x, float z, float r, float target_h, float amt) {
    if (mesh) g3d_terrain_flatten((G3DMesh *)mesh, x, z, r, target_h, amt);
}
/* Agujero de terreno (on=1 perfora, on=0 rellena) -> ver la cueva por debajo. */
void g3d_editor_terrain_hole(void *mesh, float x, float z, float r, int on) {
    if (mesh) g3d_terrain_set_hole((G3DMesh *)mesh, x, z, r, on);
}
float g3d_editor_terrain_height(void *mesh, float x, float z) {
    return mesh ? g3d_terrain_get_height((G3DMesh *)mesh, x, z) : 0.0f;
}
/* Snapshot/restore de las alturas del terreno (para deshacer el cauce de un rio
   al borrarlo). vcount da el tamano del buffer necesario. */
int g3d_editor_terrain_vcount(void *mesh) {
    G3DMesh *m = (G3DMesh *)mesh;
    return m ? (int)m->vertex_count : 0;
}
void g3d_editor_terrain_snapshot(void *mesh, float *out) {
    G3DMesh *m = (G3DMesh *)mesh;
    if (!m || !out) return;
    for (uint32_t i = 0; i < m->vertex_count; i++) out[i] = m->vertices[i].position[1];
}
void g3d_editor_terrain_restore(void *mesh, const float *in) {
    G3DMesh *m = (G3DMesh *)mesh;
    if (!m || !in) return;
    for (uint32_t i = 0; i < m->vertex_count; i++) m->vertices[i].position[1] = in[i];
    g3d_terrain_update(m);   /* recomputa normales + sube a GPU */
}

/* Guardar/cargar el relieve del terreno (dump binario de la Y de cada vertice). */
void g3d_editor_terrain_save(void *mesh, const char *path) {
    G3DMesh *m = (G3DMesh *)mesh;
    if (!m) return;
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t n = m->vertex_count;
    fwrite(&n, sizeof(uint32_t), 1, f);
    for (uint32_t i = 0; i < n; i++)
        fwrite(&m->vertices[i].position[1], sizeof(float), 1, f);
    fclose(f);
}
int g3d_editor_terrain_load(void *mesh, const char *path) {
    G3DMesh *m = (G3DMesh *)mesh;
    if (!m) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint32_t n = 0;
    if (fread(&n, sizeof(uint32_t), 1, f) != 1 || n != m->vertex_count) { fclose(f); return 0; }
    for (uint32_t i = 0; i < n; i++) {
        float y;
        if (fread(&y, sizeof(float), 1, f) == 1) m->vertices[i].position[1] = y;
    }
    fclose(f);
    g3d_terrain_update(m);   /* recomputa normales + sube a GPU */
    return 1;
}

/* Proyecta un punto del mundo a pixel del viewport (out2). 1 si esta delante. */
int g3d_editor_world_to_screen(float wx, float wy, float wz, float w, float h, float *out2) {
    if (!g_editor_cam) return 0;
    float *m = g_editor_cam->view_projection.m;
    float X = m[0]*wx + m[4]*wy + m[8]*wz  + m[12];
    float Y = m[1]*wx + m[5]*wy + m[9]*wz  + m[13];
    float W = m[3]*wx + m[7]*wy + m[11]*wz + m[15];
    if (W <= 1e-5f) return 0;                       /* detras de la camara */
    out2[0] = (X / W * 0.5f + 0.5f) * w;
    out2[1] = (1.0f - (Y / W * 0.5f + 0.5f)) * h;   /* pantalla y hacia abajo */
    return 1;
}

/* -------- AGUA: plano global (mar/lago) a un nivel Y -------- */
static int   g_water_created = 0;
static float g_water_level = -1e30f;
/* (Re)crea/activa el agua y aplica oleaje/color en vivo. Solo recrea si cambia
   el nivel; los setters (olas/color/swell) son baratos por frame. */
void g3d_editor_water_update(int enabled, float level, float size,
                             float amp, float wavelen, float speed, float swell,
                             float dr, float dg, float db,
                             float sr, float sg, float sb) {
    if (!enabled) { if (g_water_created) g3d_water_set_enabled(0); return; }
    if (!g_water_created || fabsf(level - g_water_level) > 1e-4f) {
        g3d_water_create(level, size, 200);
        g3d_water_set_ssr(1, 0.6f);
        g_water_created = 1; g_water_level = level;
    }
    g3d_water_set_waves(amp, wavelen, speed);
    g3d_water_set_ocean(1.0f, 0.3f, swell);
    g3d_water_set_color(dr, dg, db, sr, sg, sb);
    g3d_water_set_enabled(1);
}
/* Textura de la superficie del agua (o 0 para quitarla). */
void g3d_editor_water_set_texture(void *tex) {
    g3d_water_set_texture(tex ? ((G3DTexture *)tex)->gl_handle : 0);
}
void g3d_editor_fluid_set_texture(void *tex) {
    g3d_fluid_set_texture(tex ? ((G3DTexture *)tex)->gl_handle : 0);
}
void g3d_editor_flow_set_texture(void *tex) {
    g3d_flow_set_texture(tex ? ((G3DTexture *)tex)->gl_handle : 0);
}

/* -------- CUEVAS: volumen de voxels carvable con el raton (3D) -------- */
static int g_cave_id = -1, g_cave_entity = -1;

static int screen_ray_world(float sx, float sy, float w, float h, Vec3 *o, Vec3 *d) {
    if (!g_editor_cam || w < 1.0f || h < 1.0f) return 0;
    Mat4 inv = mat4_inverse(g_editor_cam->view_projection);
    float ndcx = 2.0f * sx / w - 1.0f, ndcy = 1.0f - 2.0f * sy / h;
    Vec4 pn = mat4_mul_vec4(inv, vec4_make(ndcx, ndcy, -1.0f, 1.0f));
    Vec4 pf = mat4_mul_vec4(inv, vec4_make(ndcx, ndcy,  1.0f, 1.0f));
    if (fabsf(pn.w) < 1e-6f || fabsf(pf.w) < 1e-6f) return 0;
    Vec3 a = vec3_make(pn.x/pn.w, pn.y/pn.w, pn.z/pn.w);
    Vec3 b = vec3_make(pf.x/pf.w, pf.y/pf.w, pf.z/pf.w);
    *o = g_editor_cam->position; *d = vec3_normalize(vec3_sub(b, a));
    return 1;
}

/* Entra en modo cueva: crea (una vez) un volumen de roca que copia el relieve
   del terreno, lo mesha, oculta el terreno-heightmap y muestra la cueva. */
int g3d_editor_cave_enter(int scene, void *terrain, float world_size) {
    if (g_cave_id < 0) {
        const int S = 97;
        float *h = (float *)malloc(sizeof(float) * S * S);
        if (!h) return -1;
        for (int j = 0; j < S; j++)
            for (int i = 0; i < S; i++) {
                float wx = ((float)i/(S-1) - 0.5f) * world_size;
                float wz = ((float)j/(S-1) - 0.5f) * world_size;
                h[j*S + i] = g3d_terrain_get_height((G3DMesh *)terrain, wx, wz);
            }
        g_cave_id = g3d_cave_create(0.0f, 0.0f, 0.0f, world_size, 96);
        if (g_cave_id >= 0) g3d_cave_fill_from_heightfield(g_cave_id, h, S, world_size);
        free(h);
        if (g_cave_id < 0) return -1;
        G3DMesh *m = g3d_cave_build_mesh(g_cave_id);
        int mat = g3d_material_impl_create();
        G3DMaterial *mm = g3d_material_impl_get(mat);
        if (mm) {
            mm->roughness = 0.95f; mm->metallic = 0.0f;
            mm->color[0] = 0.45f; mm->color[1] = 0.42f; mm->color[2] = 0.40f; mm->color[3] = 1.0f;
            mm->albedo_texture = NULL; mm->albedo_texture_id = -1;
        }
        g_cave_entity = g3d_entity_impl_spawn(scene, 0, 0, 0, 0);
        G3DEntity *e = g3d_entity_impl_get(g_cave_entity);
        if (e) e->mesh = m;
        g3d_entity_impl_set_material(g_cave_entity, mat);
    }
    /* El terreno-heightfield NUNCA se oculta: sigue pintable/colocable/con agua.
       La cueva convive; un pequeno offset hacia abajo evita el z-fighting con la
       superficie del terreno en las zonas sin carvar. */
    if (g_cave_entity >= 0) g3d_entity_impl_set_position(g_cave_entity, 0, -0.15f, 0);
    return g_cave_id;
}
/* Salir del modo cueva: el terreno siempre estuvo visible; la cueva permanece.
   El modo solo activa/desactiva las herramientas de excavar. */
void g3d_editor_cave_exit(void) {
    /* no-op: terreno y cueva conviven siempre */
}
/* Descarta las cuevas (opcional). */
void g3d_editor_cave_reset(void) {
    if (g_cave_entity >= 0) g3d_entity_impl_set_position(g_cave_entity, 0, -1e6f, 0);
    g3d_cave_clear();
    g_cave_id = -1; g_cave_entity = -1;
}
/* Carva la roca en un punto del MUNDO (p.ej. el punto de terreno bajo el cursor)
   y remesha en el sitio. mode 0 = quitar roca (cueva), 1 = anadir. Tiempo real. */
void g3d_editor_cave_dig(float wx, float wy, float wz, float radius, int mode, float strength) {
    if (g_cave_id < 0) return;
    g3d_cave_edit(g_cave_id, wx, wy, wz, radius, mode, strength);
    G3DMesh *m = g3d_cave_build_mesh(g_cave_id);
    G3DEntity *e = g3d_entity_impl_get(g_cave_entity);
    if (e) e->mesh = m;
}

/* Cursor 3D: raycast a la roca. strength>0 => carva/rellena y remesha.
   out[3] = punto de la roca apuntado (para el anillo). Devuelve 1 si acierta. */
int g3d_editor_cave_carve(float sx, float sy, float w, float h, float radius,
                          int mode, float strength, float *out) {
    if (g_cave_id < 0) return 0;
    Vec3 o, d;
    if (!screen_ray_world(sx, sy, w, h, &o, &d)) return 0;
    float hx, hy, hz;
    if (!g3d_cave_raycast(g_cave_id, o.x, o.y, o.z, d.x, d.y, d.z, 1.0e6f, &hx, &hy, &hz))
        return 0;
    out[0] = hx; out[1] = hy; out[2] = hz;
    if (strength > 0.0f) {
        g3d_cave_edit(g_cave_id, hx, hy, hz, radius, mode, strength);
        G3DMesh *m = g3d_cave_build_mesh(g_cave_id);
        G3DEntity *e = g3d_entity_impl_get(g_cave_entity);
        if (e) e->mesh = m;
    }
    return 1;
}

/* -------- CUEVA procedural: cuenco de roca bajo un agujero del terreno -------- */
/* Perfora el terreno en (wx,wz) y genera una cavidad (cuenco) de roca con las
   paredes hacia dentro, de radio `radius` y profundidad `depth`, apoyada en la
   superficie. Un clic -> cueva real en la que mirar/entrar. Devuelve la entidad. */
int g3d_editor_cave_place(int scene_id, void *terrain, float wx, float wz,
                          float radius, float depth, const char *texpath) {
    if (terrain) g3d_terrain_set_hole((G3DMesh *)terrain, wx, wz, radius, 1);
    float cy = terrain ? g3d_terrain_get_height((G3DMesh *)terrain, wx, wz) : 0.0f;

    const int RINGS = 14, SEGS = 28;
    int vcount = (RINGS + 1) * (SEGS + 1);
    int icount = RINGS * SEGS * 6;
    G3DVertex *v = (G3DVertex *)calloc((size_t)vcount, sizeof(G3DVertex));
    uint32_t *idx = (uint32_t *)malloc((size_t)icount * sizeof(uint32_t));
    if (!v || !idx) { free(v); free(idx); return -1; }

    int vi = 0;
    for (int i = 0; i <= RINGS; i++) {
        float phi = (float)i / RINGS * 1.5707963f;      /* 0 (borde) .. pi/2 (fondo) */
        float rr = radius * cosf(phi);
        float yy = -depth * sinf(phi);
        for (int j = 0; j <= SEGS; j++) {
            float th = (float)j / SEGS * 6.2831853f;
            float px = rr * cosf(th), pz = rr * sinf(th);
            v[vi].position[0] = wx + px;
            v[vi].position[1] = cy + yy;
            v[vi].position[2] = wz + pz;
            /* normal hacia DENTRO/arriba (se ve el interior del cuenco) */
            float nx = -px, ny = (-yy) + radius * 0.25f, nz = -pz;
            float nl = sqrtf(nx*nx + ny*ny + nz*nz); if (nl < 1e-5f) nl = 1.0f;
            v[vi].normal[0] = nx/nl; v[vi].normal[1] = ny/nl; v[vi].normal[2] = nz/nl;
            v[vi].texcoord[0] = (float)j / SEGS * 3.0f;
            v[vi].texcoord[1] = (float)i / RINGS * 3.0f;
            vi++;
        }
    }
    int k = 0;
    for (int i = 0; i < RINGS; i++)
        for (int j = 0; j < SEGS; j++) {
            uint32_t a = (uint32_t)(i*(SEGS+1) + j), b = a + 1;
            uint32_t c = (uint32_t)((i+1)*(SEGS+1) + j), d = c + 1;
            idx[k++] = a; idx[k++] = c; idx[k++] = b;
            idx[k++] = b; idx[k++] = c; idx[k++] = d;
        }
    G3DMesh *m = g3d_mesh_create("cave", v, (uint32_t)vcount, idx, (uint32_t)icount);
    free(v); free(idx);
    if (!m) return -1;
    g3d_mesh_upload_gpu(m);

    int mat = g3d_material_impl_create();
    G3DMaterial *mm = g3d_material_impl_get(mat);
    if (mm) {
        mm->roughness = 0.95f; mm->metallic = 0.0f;
        if (texpath) {
            G3DTexture *t = g3d_texture_load_impl(texpath);
            if (t) { g3d_texture_upload_gpu(t); mm->albedo_texture = t; mm->albedo_texture_id = 0; }
        } else {
            mm->color[0] = 0.34f; mm->color[1] = 0.31f; mm->color[2] = 0.29f; mm->color[3] = 1.0f;
            mm->albedo_texture = NULL; mm->albedo_texture_id = -1;
        }
    }
    int e = g3d_entity_impl_spawn(scene_id, 0, 0, 0, 0);
    G3DEntity *ent = g3d_entity_impl_get(e);
    if (ent) ent->mesh = m;
    g3d_entity_impl_set_material(e, mat);
    return e;
}

/* -------- spawn de modelo (copia de libmod_3d.c, sin el LOD lejano) -------- */
int g3d_model_spawn(int scene_id, void *model_ptr, float x, float y, float z,
                    float height, float roty) {
    G3DModel *model = (G3DModel *)model_ptr;
    if (!model || model->mesh_count == 0) return -1;

    float s = 1.0f;
    if (height > 0.0f) {
        float ymin = model->meshes[0].aabb_min[1], ymax = model->meshes[0].aabb_max[1];
        for (uint32_t i = 1; i < model->mesh_count; i++) {
            if (model->meshes[i].aabb_min[1] < ymin) ymin = model->meshes[i].aabb_min[1];
            if (model->meshes[i].aabb_max[1] > ymax) ymax = model->meshes[i].aabb_max[1];
        }
        float mh = ymax - ymin;
        if (mh > 1e-6f) s = height / mh;
    }

    int root = g3d_entity_impl_spawn(scene_id, 0, x, y, z);
    if (root < 0) return -1;
    g3d_entity_impl_set_rotation(root, 0.0f, roty, 0.0f);

    void **key_alb = (void **)calloc(model->mesh_count, sizeof(void *));
    void **key_nrm = (void **)calloc(model->mesh_count, sizeof(void *));
    void **key_met = (void **)calloc(model->mesh_count, sizeof(void *));
    void **key_rgh = (void **)calloc(model->mesh_count, sizeof(void *));
    int *key_mat = (int *)calloc(model->mesh_count, sizeof(int));
    unsigned char *key_out = (unsigned char *)calloc(model->mesh_count, 1);
    int key_count = 0;

    for (uint32_t j = 0; j < model->mesh_count; j++) {
        void *alb = model->mesh_textures ? model->mesh_textures[j] : NULL;
        void *nrm = model->mesh_normal ? model->mesh_normal[j] : NULL;
        void *met = model->mesh_metallic ? model->mesh_metallic[j] : NULL;
        void *rgh = model->mesh_roughness ? model->mesh_roughness[j] : NULL;
        /* contorno toon: entra en la clave para no fusionarse con otros
           materiales sin textura (ver libmod_3d.c) */
        unsigned char outl = model->mesh_outline ? model->mesh_outline[j] : 0;

        int mat = -1;
        for (int k = 0; k < key_count; k++) {
            if (key_alb[k] == alb && key_nrm[k] == nrm &&
                key_met[k] == met && key_rgh[k] == rgh &&
                (!key_out || key_out[k] == outl)) { mat = key_mat[k]; break; }
        }
        if (mat < 0) {
            mat = g3d_material_impl_create();
            G3DMaterial *m = g3d_material_impl_get(mat);
            if (m) {
                m->albedo_texture = alb;
                m->albedo_texture_id = alb ? 0 : -1;
                m->roughness = 0.9f;
                m->metallic = 0.0f;
                m->outline = outl;
                if (nrm) g3d_material_impl_set_map(mat, 1, nrm);
                if (met) g3d_material_impl_set_map(mat, 2, met);
                if (rgh) g3d_material_impl_set_map(mat, 3, rgh);
            }
            if (mat >= 0) {
                if (key_out) key_out[key_count] = outl;
                key_alb[key_count] = alb; key_nrm[key_count] = nrm;
                key_met[key_count] = met; key_rgh[key_count] = rgh;
                key_mat[key_count] = mat; key_count++;
            }
        }
        int ent = g3d_entity_impl_spawn(scene_id, 0, 0.0f, 0.0f, 0.0f);
        if (ent < 0) continue;
        G3DEntity *e = g3d_entity_impl_get(ent);
        if (e) {
            e->mesh = &model->meshes[j];
            e->anim_model = (model->meshes[j].anim_node >= 0) ? (void *)model : NULL;
        }
        g3d_entity_impl_set_material(ent, mat);
        g3d_entity_impl_set_scale(ent, s, s, s);
        g3d_entity_impl_set_parent(ent, root);
    }
    free(key_alb); free(key_nrm); free(key_met); free(key_rgh); free(key_mat); free(key_out);
    return root;
}
