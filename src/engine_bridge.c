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
#include "libmod_3d_erosion.h"
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

/* Planos cercano y lejano de la camara del editor. La precision del buffer de profundidad cae con el CUADRADO de la
   distancia dividido por el plano cercano: con 0.1 de cercano y un circuito de cientos de metros a la vista, las
   superficies a pocos centimetros una de otra se pelean (z-fighting) y salen "triangulos rotos". Se ajusta a la distancia. */
void g3d_editor_set_planes(float n, float f) {
    if (g_editor_cam && n > 0.0f && f > n) { g_editor_cam->near_plane = n; g_editor_cam->far_plane = f; g_editor_cam->proj_dirty = 1; }
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

/* El rayo de esta pantalla (sx,sy en pixeles): origen y direccion en el mundo.
   Es la version generica de g3d_editor_ray_plane, para cuando lo que hay que
   probar no es un plano sino una malla (una pared, un objeto). */
int g3d_editor_screen_ray(float sx, float sy, float w, float h, float *out_org, float *out_dir) {
    if (!g_editor_cam || w < 1.0f || h < 1.0f) return 0;
    Mat4 inv = mat4_inverse(g_editor_cam->view_projection);
    float ndcx = 2.0f * sx / w - 1.0f;
    float ndcy = 1.0f - 2.0f * sy / h;
    Vec4 pn = mat4_mul_vec4(inv, vec4_make(ndcx, ndcy, -1.0f, 1.0f));
    Vec4 pf = mat4_mul_vec4(inv, vec4_make(ndcx, ndcy,  1.0f, 1.0f));
    if (fabsf(pn.w) < 1e-6f || fabsf(pf.w) < 1e-6f) return 0;
    Vec3 a = vec3_make(pn.x / pn.w, pn.y / pn.w, pn.z / pn.w);
    Vec3 b = vec3_make(pf.x / pf.w, pf.y / pf.w, pf.z / pf.w);
    Vec3 d = vec3_normalize(vec3_sub(b, a));
    out_org[0] = g_editor_cam->position.x;
    out_org[1] = g_editor_cam->position.y;
    out_org[2] = g_editor_cam->position.z;
    out_dir[0] = d.x; out_dir[1] = d.y; out_dir[2] = d.z;
    return 1;
}

/* Interseccion rayo-triangulo (Moller-Trumbore), en espacio de mundo. */
static int ray_hits_tri(Vec3 o, Vec3 d, Vec3 v0, Vec3 v1, Vec3 v2, float max_t, float *out_t) {
    Vec3 e1 = vec3_sub(v1, v0);
    Vec3 e2 = vec3_sub(v2, v0);
    Vec3 pv = vec3_cross(d, e2);
    float det = vec3_dot(e1, pv);
    if (fabsf(det) < 1e-8f) return 0;               /* rayo paralelo al triangulo */
    float invDet = 1.0f / det;
    Vec3 tv = vec3_sub(o, v0);
    float u = vec3_dot(tv, pv) * invDet;
    if (u < 0.0f || u > 1.0f) return 0;
    Vec3 qv = vec3_cross(tv, e1);
    float v = vec3_dot(d, qv) * invDet;
    if (v < 0.0f || u + v > 1.0f) return 0;
    float t = vec3_dot(e2, qv) * invDet;
    if (t < 1e-4f || t > max_t) return 0;            /* detras o mas lejos que lo ya hallado */
    *out_t = t;
    return 1;
}

/* Rayo contra un modelo cargado, colocado como en la escena (posicion + giro Y +
   escala uniforme -- la misma composicion que usa g3d_model_spawn con el objeto:
   world = Ry(yaw) * (v_local * escala) + (px,py,pz)). Sirve para clavar cosas en
   una pared: el editor no tiene mas colision que esta contra la geometria real.

   *inout_dist entra con la distancia maxima a batir (un valor grande al
   empezar) y SOLO se actualiza si este modelo golpea mas cerca; se puede pasar
   el mismo puntero por varios objetos seguidos y quedarse con el mas cercano de
   todos sin rehacer la cuenta. hit = el punto; normal = la cara del triangulo,
   ya orientada hacia el origen del rayo (para que "hacia fuera" sea siempre
   hacia fuera, sin depender de que lado se modelo la pared). */
int g3d_editor_ray_model(float ox, float oy, float oz, float dx, float dy, float dz,
                         void *model_ptr, float px, float py, float pz,
                         float yaw, float scale,
                         float *inout_dist, float *hit, float *normal) {
    G3DModel *model = (G3DModel *)model_ptr;
    if (!model || model->mesh_count == 0 || scale <= 0.0f) return 0;
    Vec3 o = vec3_make(ox, oy, oz);
    Vec3 d = vec3_normalize(vec3_make(dx, dy, dz));
    float cy = cosf(yaw), sy = sinf(yaw);
    int found = 0;

    for (uint32_t mi = 0; mi < model->mesh_count; mi++) {
        G3DMesh *mesh = &model->meshes[mi];
        if (!mesh->vertices || !mesh->indices || mesh->index_count < 3) continue;

        /* rechazo rapido: la caja del submesh, transformada, contra el rayo */
        Vec3 bmin = vec3_make(1e30f, 1e30f, 1e30f), bmax = vec3_make(-1e30f, -1e30f, -1e30f);
        for (int c = 0; c < 8; c++) {
            Vec3 lp = vec3_make(
                (c & 1) ? mesh->aabb_max[0] : mesh->aabb_min[0],
                (c & 2) ? mesh->aabb_max[1] : mesh->aabb_min[1],
                (c & 4) ? mesh->aabb_max[2] : mesh->aabb_min[2]);
            lp = vec3_scale(lp, scale);
            Vec3 wp = vec3_make(lp.x * cy + lp.z * sy, lp.y, -lp.x * sy + lp.z * cy);
            wp = vec3_make(wp.x + px, wp.y + py, wp.z + pz);
            if (wp.x < bmin.x) bmin.x = wp.x; if (wp.x > bmax.x) bmax.x = wp.x;
            if (wp.y < bmin.y) bmin.y = wp.y; if (wp.y > bmax.y) bmax.y = wp.y;
            if (wp.z < bmin.z) bmin.z = wp.z; if (wp.z > bmax.z) bmax.z = wp.z;
        }
        float tmin = 0.0f, tmax = *inout_dist;
        int reject = 0;
        float od[3] = { o.x, o.y, o.z }, dd[3] = { d.x, d.y, d.z };
        float bn[3] = { bmin.x, bmin.y, bmin.z }, bx[3] = { bmax.x, bmax.y, bmax.z };
        for (int ax = 0; ax < 3 && !reject; ax++) {
            if (fabsf(dd[ax]) < 1e-9f) {
                if (od[ax] < bn[ax] - 0.01f || od[ax] > bx[ax] + 0.01f) reject = 1;
            } else {
                float inv = 1.0f / dd[ax];
                float t1 = (bn[ax] - od[ax]) * inv, t2 = (bx[ax] - od[ax]) * inv;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) reject = 1;
            }
        }
        if (reject) continue;

        /* el triangulo a triangulo, ya con pocos submeshes filtrados */
        for (uint32_t ii = 0; ii + 2 < mesh->index_count; ii += 3) {
            uint32_t i0 = mesh->indices[ii], i1 = mesh->indices[ii+1], i2 = mesh->indices[ii+2];
            if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count) continue;
            Vec3 lv[3];
            lv[0] = vec3_make(mesh->vertices[i0].position[0], mesh->vertices[i0].position[1], mesh->vertices[i0].position[2]);
            lv[1] = vec3_make(mesh->vertices[i1].position[0], mesh->vertices[i1].position[1], mesh->vertices[i1].position[2]);
            lv[2] = vec3_make(mesh->vertices[i2].position[0], mesh->vertices[i2].position[1], mesh->vertices[i2].position[2]);
            Vec3 wv[3];
            for (int k = 0; k < 3; k++) {
                Vec3 lp = vec3_scale(lv[k], scale);
                Vec3 rp = vec3_make(lp.x * cy + lp.z * sy, lp.y, -lp.x * sy + lp.z * cy);
                wv[k] = vec3_make(rp.x + px, rp.y + py, rp.z + pz);
            }
            float t;
            if (!ray_hits_tri(o, d, wv[0], wv[1], wv[2], *inout_dist, &t)) continue;
            *inout_dist = t;
            found = 1;
            hit[0] = o.x + d.x * t; hit[1] = o.y + d.y * t; hit[2] = o.z + d.z * t;
            Vec3 n = vec3_normalize(vec3_cross(vec3_sub(wv[1], wv[0]), vec3_sub(wv[2], wv[0])));
            if (vec3_dot(n, d) > 0.0f) n = vec3_scale(n, -1.0f);   /* siempre hacia la camara */
            normal[0] = n.x; normal[1] = n.y; normal[2] = n.z;
        }
    }
    return found;
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
/* --- Edicion VERTICE A VERTICE ---------------------------------------------
   El pincel siempre deja formas blandas: sirve para el bulto general, pero no
   para una arista, un escalon o una cresta. Esto expone la rejilla tal cual es
   -- indices (i,j) con i hacia +x y j hacia +z, que es el mismo orden en que se
   guarda el relieve -- para poder tocar un vertice y solo ese. */

int g3d_editor_terrain_grid(void *mesh, int *side, float *size) {
    G3DMesh *m = (G3DMesh *)mesh;
    if (!m || m->vertex_count < 4) return 0;
    /* La malla es cuadrada: el lado sale de la raiz del numero de vertices. */
    int s = (int)(sqrtf((float)m->vertex_count) + 0.5f);
    if (s < 2 || s * s != (int)m->vertex_count) return 0;
    if (side) *side = s;
    if (size) *size = m->aabb_max[0] - m->aabb_min[0];
    return 1;
}

/* Posicion de un vertice de la rejilla. Devuelve 0 si (i,j) se sale. */
int g3d_editor_terrain_vertex(void *mesh, int i, int j, float *out_xyz) {
    G3DMesh *m = (G3DMesh *)mesh;
    int s = 0;
    if (!g3d_editor_terrain_grid(mesh, &s, NULL)) return 0;
    if (i < 0 || j < 0 || i >= s || j >= s) return 0;
    const float *p = m->vertices[(size_t)j * s + i].position;
    if (out_xyz) { out_xyz[0] = p[0]; out_xyz[1] = p[1]; out_xyz[2] = p[2]; }
    return 1;
}

/* Mueve un vertice en Y. `commit` sube el resultado a la GPU y recalcula
   normales; durante un arrastre se pasa 0 en los intermedios y 1 al final, que
   es lo caro. */
void g3d_editor_terrain_set_vertex_y(void *mesh, int i, int j, float y, int commit) {
    G3DMesh *m = (G3DMesh *)mesh;
    int s = 0;
    if (!g3d_editor_terrain_grid(mesh, &s, NULL)) return;
    if (i < 0 || j < 0 || i >= s || j >= s) return;
    m->vertices[(size_t)j * s + i].position[1] = y;
    if (commit) g3d_terrain_update(m);
}

/* Desplaza un vertice EN HORIZONTAL, acotado a una fraccion de la celda.
   Nada del motor lee la X/Z de un vertice de terreno: la altura, el picado, la
   fisica y el campo de agua dan por hecho que estan clavados en la rejilla. Por
   eso el limite no es un capricho -- mientras el desplazamiento se quede dentro
   de la celda, lo que esos sistemas creen y lo que se ve difieren menos de media
   celda, que es invisible. Soltarlo del todo obligaria a rehacerlos. */
void g3d_editor_terrain_set_vertex_xz(void *mesh, int i, int j,
                                      float x, float z, float max_frac, int commit) {
    G3DMesh *m = (G3DMesh *)mesh;
    int s = 0; float size = 0.0f;
    if (!g3d_editor_terrain_grid(mesh, &s, &size)) return;
    if (i < 0 || j < 0 || i >= s || j >= s) return;
    float step = size / (float)(s - 1);
    float lim  = step * (max_frac > 0.0f ? max_frac : 0.45f);
    /* Posicion "de rejilla" de este vertice: de ahi se mide el desvio. */
    float bx = -size * 0.5f + (float)i * step;
    float bz = -size * 0.5f + (float)j * step;
    float dx = x - bx, dz = z - bz;
    if (dx >  lim) dx =  lim;  if (dx < -lim) dx = -lim;
    if (dz >  lim) dz =  lim;  if (dz < -lim) dz = -lim;
    /* El borde no se mueve: dejaria un diente en el limite del mapa. */
    if (i == 0 || j == 0 || i == s - 1 || j == s - 1) { dx = 0.0f; dz = 0.0f; }
    m->vertices[(size_t)j * s + i].position[0] = bx + dx;
    m->vertices[(size_t)j * s + i].position[2] = bz + dz;
    if (commit) g3d_terrain_update(m);
}

/* Los desvios horizontales van en un fichero APARTE. El .terrain guarda una Y
   por vertice y lo leen escenas ya hechas; ampliarlo las romperia. Sin este
   fichero, el terreno es simplemente uno sin desvios. */
void g3d_editor_terrain_save_xz(void *mesh, const char *path) {
    G3DMesh *m = (G3DMesh *)mesh;
    int s = 0; float size = 0.0f;
    if (!g3d_editor_terrain_grid(mesh, &s, &size)) return;
    float step = size / (float)(s - 1);
    /* Si no hay ni un desvio, no se escribe fichero. */
    int any = 0;
    for (int j = 0; j < s && !any; j++)
        for (int i = 0; i < s; i++) {
            const float *p = m->vertices[(size_t)j * s + i].position;
            float bx = -size * 0.5f + i * step, bz = -size * 0.5f + j * step;
            if (fabsf(p[0] - bx) > 1e-4f || fabsf(p[2] - bz) > 1e-4f) { any = 1; break; }
        }
    if (!any) { remove(path); return; }
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t n = (uint32_t)(s * s);
    fwrite(&n, sizeof(uint32_t), 1, f);
    for (int j = 0; j < s; j++)
        for (int i = 0; i < s; i++) {
            const float *p = m->vertices[(size_t)j * s + i].position;
            float d[2] = { p[0] - (-size * 0.5f + i * step),
                           p[2] - (-size * 0.5f + j * step) };
            fwrite(d, sizeof(float), 2, f);
        }
    fclose(f);
}

int g3d_editor_terrain_load_xz(void *mesh, const char *path) {
    G3DMesh *m = (G3DMesh *)mesh;
    int s = 0; float size = 0.0f;
    if (!g3d_editor_terrain_grid(mesh, &s, &size)) return 0;
    float step = size / (float)(s - 1);
    FILE *f = fopen(path, "rb");
    if (!f) {   /* sin fichero: rejilla limpia */
        for (int j = 0; j < s; j++)
            for (int i = 0; i < s; i++) {
                m->vertices[(size_t)j * s + i].position[0] = -size * 0.5f + i * step;
                m->vertices[(size_t)j * s + i].position[2] = -size * 0.5f + j * step;
            }
        g3d_terrain_update(m);
        return 0;
    }
    uint32_t n = 0;
    if (fread(&n, sizeof(uint32_t), 1, f) != 1 || n != (uint32_t)(s * s)) { fclose(f); return 0; }
    for (int j = 0; j < s; j++)
        for (int i = 0; i < s; i++) {
            float d[2] = { 0.0f, 0.0f };
            if (fread(d, sizeof(float), 2, f) != 2) break;
            m->vertices[(size_t)j * s + i].position[0] = -size * 0.5f + i * step + d[0];
            m->vertices[(size_t)j * s + i].position[2] = -size * 0.5f + j * step + d[1];
        }
    fclose(f);
    g3d_terrain_update(m);
    return 1;
}

/* Erosiona el relieve de la malla. Los parametros van sueltos y no en la
   estructura para no tener que exponerla al C++ del editor. */
int g3d_editor_terrain_erode(void *mesh, int iterations, float rain, float evap,
                             float capacity, float dissolve, float deposit,
                             float min_slope, float talus) {
    G3DMesh *m = (G3DMesh *)mesh;
    int side = 0; float size = 0.0f;
    if (!g3d_editor_terrain_grid(mesh, &side, &size)) return 0;
    int n = side * side;
    float *h = (float *)malloc((size_t)n * sizeof(float));
    if (!h) return 0;
    for (int i = 0; i < n; i++) h[i] = m->vertices[i].position[1];

    G3DErosionParams p;
    g3d_erosion_defaults(&p);
    p.rain = rain; p.evaporation = evap; p.capacity = capacity;
    p.dissolve = dissolve; p.deposit = deposit; p.min_slope = min_slope;
    p.talus = talus;
    int done = g3d_erosion_run(h, side, size, iterations, &p);

    for (int i = 0; i < n; i++) m->vertices[i].position[1] = h[i];
    free(h);
    g3d_terrain_update(m);
    return done;
}

void g3d_editor_terrain_commit(void *mesh) {
    if (mesh) g3d_terrain_update((G3DMesh *)mesh);
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

/* ---- vista previa de los mapas 2.5D colocados en un mundo (editor): cajas con la textura de cada tile, colgando de una entidad raiz ---- */
static G3DMesh *g_prev_cube = NULL;
static char g_prev_mat_path[96][512]; static int g_prev_mat_id[96]; static int g_prev_mat_n = 0;

int g3d_editor_prev_group(int scene_id, float x, float y, float z, float yaw_rad) {
    int e = g3d_entity_impl_spawn(scene_id, 0, x, y, z);
    if (e >= 0) g3d_entity_impl_set_rotation(e, 0.0f, yaw_rad, 0.0f);
    return e;
}

/* material con la textura del fichero (se reutiliza por ruta); ruta vacia = gris liso */
int g3d_editor_prev_material(const char *path) {
    for (int i = 0; i < g_prev_mat_n; i++) if (!strcmp(g_prev_mat_path[i], path)) return g_prev_mat_id[i];
    int m = g3d_material_impl_create();
    if (m < 0) return -1;
    g3d_material_impl_set_color(m, 1.0f, 1.0f, 1.0f, 1.0f);
    g3d_material_impl_set_roughness(m, 0.9f); g3d_material_impl_set_metallic(m, 0.0f);
    if (path && path[0]) {
        G3DTexture *tex = g3d_texture_load_impl(path);
        if (tex) { g3d_texture_upload_gpu(tex); G3DMaterial *mm = g3d_material_impl_get(m); if (mm) { mm->albedo_texture = (void *)tex; mm->albedo_texture_id = 0; } }
    } else g3d_material_impl_set_color(m, 0.6f, 0.6f, 0.65f, 1.0f);
    if (g_prev_mat_n < 96) { snprintf(g_prev_mat_path[g_prev_mat_n], 512, "%s", path ? path : ""); g_prev_mat_id[g_prev_mat_n++] = m; }
    return m;
}

int g3d_editor_prev_box(int scene_id, int parent, float cx, float cy, float cz, float sx, float sy, float sz, int mat) {
    if (!g_prev_cube) { g_prev_cube = g3d_primitive_create_cube(); if (g_prev_cube) g3d_mesh_upload_gpu(g_prev_cube); }
    int e = g3d_entity_impl_spawn(scene_id, 0, cx, cy, cz);
    if (e < 0) return -1;
    G3DEntity *en = g3d_entity_impl_get(e);
    if (en) { en->mesh = (void *)g_prev_cube; en->model_id = 0; }
    g3d_entity_impl_set_scale(e, sx, sy, sz);
    if (mat >= 0) g3d_entity_impl_set_material(e, mat);
    if (parent >= 0) g3d_entity_impl_set_parent(e, parent);
    return e;
}

/* Depuracion del editor: cuantas entidades vivas hay (para detectar restos de vistas previas mal borradas) */
int g3d_editor_count_entities(void) {
    int n = 0;
    for (int i = 0; i < 200000; i++) {
        extern G3DEntity *g3d_entity_impl_get(int);
        G3DEntity *e = g3d_entity_impl_get(i);
        if (e) n++;
        else if (i > 0 && !g3d_entity_impl_get(i + 1) && !g3d_entity_impl_get(i + 2) && i > 20000) break;
    }
    return n;
}

/* Punto de un MODELO bajo el raton (para trazar sobre una pista importada). El modelo esta colocado en (tx,ty,tz) con
   escala 'scale' y girado yaw_deg grados alrededor de Y. Devuelve 1 si acierta y deja el punto del MUNDO en out[3]. */
int g3d_editor_model_pick(float sx, float sy, float w, float h, void *model_ptr,
                          float tx, float ty, float tz, float scale, float yaw_deg, float *out) {
    G3DModel *model = (G3DModel *)model_ptr;
    Vec3 o, d;
    if (!model || !screen_ray_world(sx, sy, w, h, &o, &d)) return 0;
    if (scale <= 1e-6f) scale = 1.0f;
    /* el rayo al espacio del modelo: se deshace la traslacion, el giro y la escala */
    float a = -yaw_deg * 0.017453292f, ca = cosf(a), sa = sinf(a);
    float ox = o.x - tx, oy = o.y - ty, oz = o.z - tz;
    float lox = (ox * ca + oz * sa) / scale, loy = oy / scale, loz = (-ox * sa + oz * ca) / scale;
    float ldx = (d.x * ca + d.z * sa) / scale, ldy = d.y / scale, ldz = (-d.x * sa + d.z * ca) / scale;
    float best = 1e30f; int hit = 0;
    for (uint32_t mi = 0; mi < model->mesh_count; mi++) {
        const G3DMesh *m = &model->meshes[mi];
        if (!m->vertices || !m->indices) continue;
        /* caja del mesh: si el rayo no la toca, no se miran sus triangulos */
        {
            float tmin = 0.0f, tmax = 1e30f; int ok = 1;
            const float lo[3] = { m->aabb_min[0], m->aabb_min[1], m->aabb_min[2] }, hi[3] = { m->aabb_max[0], m->aabb_max[1], m->aabb_max[2] };
            const float ro[3] = { lox, loy, loz }, rd[3] = { ldx, ldy, ldz };
            for (int k = 0; k < 3 && ok; k++) {
                if (fabsf(rd[k]) < 1e-9f) { if (ro[k] < lo[k] || ro[k] > hi[k]) ok = 0; }
                else {
                    float t1 = (lo[k] - ro[k]) / rd[k], t2 = (hi[k] - ro[k]) / rd[k];
                    if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
                    if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2;
                    if (tmin > tmax) ok = 0;
                }
            }
            if (!ok) continue;
        }
        for (uint32_t i = 0; i + 2 < m->index_count; i += 3) {
            const float *p0 = m->vertices[m->indices[i]].position, *p1 = m->vertices[m->indices[i + 1]].position, *p2 = m->vertices[m->indices[i + 2]].position;
            float e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
            float e2x = p2[0] - p0[0], e2y = p2[1] - p0[1], e2z = p2[2] - p0[2];
            float px = ldy * e2z - ldz * e2y, py = ldz * e2x - ldx * e2z, pz = ldx * e2y - ldy * e2x;
            float det = e1x * px + e1y * py + e1z * pz;
            if (fabsf(det) < 1e-12f) continue;
            float inv = 1.0f / det;
            float tx_ = lox - p0[0], ty_ = loy - p0[1], tz_ = loz - p0[2];
            float u = (tx_ * px + ty_ * py + tz_ * pz) * inv; if (u < 0.0f || u > 1.0f) continue;
            float qx = ty_ * e1z - tz_ * e1y, qy = tz_ * e1x - tx_ * e1z, qz = tx_ * e1y - ty_ * e1x;
            float v = (ldx * qx + ldy * qy + ldz * qz) * inv; if (v < 0.0f || u + v > 1.0f) continue;
            float t = (e2x * qx + e2y * qy + e2z * qz) * inv;
            if (t > 1e-4f && t < best) { best = t; hit = 1; }
        }
    }
    if (!hit) return 0;
    /* punto en el mundo: el del rayo original (la t es la misma en ambos espacios) */
    out[0] = o.x + d.x * best; out[1] = o.y + d.y * best; out[2] = o.z + d.z * best;
    return 1;
}

/* Quita el terreno del editor (para rehacerlo con otro tamano): su entidad deja de dibujarse. */
void g3d_editor_terrain_destroy(void) {
    if (g_terrain_entity >= 0) { g3d_entity_impl_destroy(g_terrain_entity); g_terrain_entity = -1; }
}

/* Caja de un modelo (min x,y,z, max x,y,z) calculada con sus vertices; 0 si no hay. */
int g3d_editor_model_bounds(void *model_ptr, float *out6) {
    G3DModel *model = (G3DModel *)model_ptr; if (!model) return 0;
    float mn[3] = { 1e30f, 1e30f, 1e30f }, mx[3] = { -1e30f, -1e30f, -1e30f }; int any = 0;
    for (uint32_t mi = 0; mi < model->mesh_count; mi++) {
        const G3DMesh *m = &model->meshes[mi];
        for (uint32_t i = 0; m->vertices && i < m->vertex_count; i++)
            for (int k = 0; k < 3; k++) { float v = m->vertices[i].position[k]; if (v < mn[k]) mn[k] = v; if (v > mx[k]) mx[k] = v; any = 1; }
    }
    if (!any) return 0;
    for (int k = 0; k < 3; k++) { out6[k] = mn[k]; out6[3 + k] = mx[k]; }
    return 1;
}

/* Todas las superficies que atraviesa el rayo del raton sobre un MODELO, de la mas cercana a la mas lejana (un tunel tiene
   techo Y suelo: el primer punto es el techo, y el que se quiere trazar es el de abajo). Devuelve cuantas hay (max) y deja
   los puntos del MUNDO en out[3*i]. Misma colocacion que g3d_editor_model_pick. */
static int model_ray_all_core(void *model_ptr, Vec3 o, Vec3 d, float tx, float ty, float tz, float scale, float yaw_deg, float *out, int max);
int g3d_editor_model_pick_all(float sx, float sy, float w, float h, void *model_ptr,
                              float tx, float ty, float tz, float scale, float yaw_deg, float *out, int max) {
    Vec3 o, d;
    if (!model_ptr || max <= 0 || !screen_ray_world(sx, sy, w, h, &o, &d)) return 0;
    return model_ray_all_core(model_ptr, o, d, tx, ty, tz, scale, yaw_deg, out, max);
}
/* Igual con un rayo cualquiera del mundo (origen y direccion): para mirar que hay bajo un punto (rayo vertical). */
int g3d_editor_model_ray_all(void *model_ptr, float ox_, float oy_, float oz_, float dx_, float dy_, float dz_,
                             float tx, float ty, float tz, float scale, float yaw_deg, float *out, int max) {
    Vec3 o = vec3_make(ox_, oy_, oz_), d = vec3_make(dx_, dy_, dz_);
    return model_ray_all_core(model_ptr, o, d, tx, ty, tz, scale, yaw_deg, out, max);
}
static int model_ray_all_core(void *model_ptr, Vec3 o, Vec3 d, float tx, float ty, float tz, float scale, float yaw_deg, float *out, int max) {
    G3DModel *model = (G3DModel *)model_ptr;
    if (!model || max <= 0) return 0;
    if (scale <= 1e-6f) scale = 1.0f;
    float a = -yaw_deg * 0.017453292f, ca = cosf(a), sa = sinf(a);
    float ox = o.x - tx, oy = o.y - ty, oz = o.z - tz;
    float lox = (ox * ca + oz * sa) / scale, loy = oy / scale, loz = (-ox * sa + oz * ca) / scale;
    float ldx = (d.x * ca + d.z * sa) / scale, ldy = d.y / scale, ldz = (-d.x * sa + d.z * ca) / scale;
    enum { MAXH = 256 };
    float ts[MAXH]; int n = 0;
    for (uint32_t mi = 0; mi < model->mesh_count; mi++) {
        const G3DMesh *m = &model->meshes[mi];
        if (!m->vertices || !m->indices) continue;
        {
            float tmin = 0.0f, tmax = 1e30f; int ok = 1;
            const float lo[3] = { m->aabb_min[0], m->aabb_min[1], m->aabb_min[2] }, hi[3] = { m->aabb_max[0], m->aabb_max[1], m->aabb_max[2] };
            const float ro[3] = { lox, loy, loz }, rd[3] = { ldx, ldy, ldz };
            for (int k = 0; k < 3 && ok; k++) {
                if (fabsf(rd[k]) < 1e-9f) { if (ro[k] < lo[k] || ro[k] > hi[k]) ok = 0; }
                else {
                    float t1 = (lo[k] - ro[k]) / rd[k], t2 = (hi[k] - ro[k]) / rd[k];
                    if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
                    if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2;
                    if (tmin > tmax) ok = 0;
                }
            }
            if (!ok) continue;
        }
        for (uint32_t i = 0; i + 2 < m->index_count; i += 3) {
            const float *p0 = m->vertices[m->indices[i]].position, *p1 = m->vertices[m->indices[i + 1]].position, *p2 = m->vertices[m->indices[i + 2]].position;
            float e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
            float e2x = p2[0] - p0[0], e2y = p2[1] - p0[1], e2z = p2[2] - p0[2];
            float px = ldy * e2z - ldz * e2y, py = ldz * e2x - ldx * e2z, pz = ldx * e2y - ldy * e2x;
            float det = e1x * px + e1y * py + e1z * pz;
            if (fabsf(det) < 1e-12f) continue;
            float inv = 1.0f / det;
            float tx_ = lox - p0[0], ty_ = loy - p0[1], tz_ = loz - p0[2];
            float u = (tx_ * px + ty_ * py + tz_ * pz) * inv; if (u < 0.0f || u > 1.0f) continue;
            float qx = ty_ * e1z - tz_ * e1y, qy = tz_ * e1x - tx_ * e1z, qz = tx_ * e1y - ty_ * e1x;
            float v = (ldx * qx + ldy * qy + ldz * qz) * inv; if (v < 0.0f || u + v > 1.0f) continue;
            float t = (e2x * qx + e2y * qy + e2z * qz) * inv;
            if (t > 1e-4f && n < MAXH) ts[n++] = t;
        }
    }
    /* ordenar y quitar los casi repetidos (la cara de arriba y la de abajo de una chapa fina son una sola superficie) */
    for (int i = 1; i < n; i++) { float k = ts[i]; int j = i - 1; while (j >= 0 && ts[j] > k) { ts[j + 1] = ts[j]; j--; } ts[j + 1] = k; }
    int c = 0;
    for (int i = 0; i < n && c < max; i++) {
        if (c > 0) { float prev = out[3 * (c - 1) + 1]; float wy = o.y + d.y * ts[i]; if (fabsf(wy - prev) < 0.15f) continue; }
        out[3 * c] = o.x + d.x * ts[i]; out[3 * c + 1] = o.y + d.y * ts[i]; out[3 * c + 2] = o.z + d.z * ts[i]; c++;
    }
    return c;
}

/* ======================= PIEZAS DE UN MODELO (colision de pistas importadas) =======================
   Un .glb de circuito junta en una sola malla todo lo que comparte material, asi que "la columna" o "el puente de debajo"
   no existen como objetos sueltos. Pero cada pieza SI es una superficie conexa: se agrupan los triangulos por vertices
   compartidos (o coincidentes) y cada grupo es una pieza que se puede quitar de la colision. */
typedef struct { float c[3], mn[3], mx[3]; int ntri; } CompInfo;
static void *g_cm_model = NULL;
static int g_cm_n = 0, g_cm_total = 0;
static int *g_cm_tricomp = NULL;          /* por triangulo (global): pieza */
static uint32_t *g_cm_off = NULL;          /* por malla: primer triangulo global */
static CompInfo *g_cm_info = NULL;

static int cm_find(int *p, int x) { while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; } return x; }

int g3d_editor_comp_build(void *model_ptr) {
    G3DModel *model = (G3DModel *)model_ptr;
    if (!model) return 0;
    if (model == g_cm_model && g_cm_tricomp) return g_cm_n;
    free(g_cm_tricomp); free(g_cm_off); free(g_cm_info); g_cm_tricomp = NULL; g_cm_off = NULL; g_cm_info = NULL; g_cm_n = 0; g_cm_total = 0;
    g_cm_model = model;
    uint32_t nm = model->mesh_count, totv = 0; size_t tott = 0;
    g_cm_off = (uint32_t *)calloc(nm + 1, sizeof(uint32_t));
    for (uint32_t i = 0; i < nm; i++) { g_cm_off[i] = (uint32_t)tott; const G3DMesh *m = &model->meshes[i]; if (m->vertices && m->indices) { totv += m->vertex_count; tott += m->index_count / 3; } }
    g_cm_off[nm] = (uint32_t)tott; g_cm_total = (int)tott;
    if (tott == 0 || totv == 0) return 0;
    /* ids de vertice globales, unificando los de la misma posicion (las costuras de UV duplican vertices) */
    uint32_t cap = 1; while (cap < totv * 2 + 16) cap <<= 1;
    int32_t *hk = (int32_t *)malloc((size_t)cap * 3 * sizeof(int32_t)); int *hv = (int *)malloc((size_t)cap * sizeof(int));
    for (uint32_t i = 0; i < cap; i++) hv[i] = -1;
    int *vid = (int *)malloc((size_t)totv * sizeof(int)); uint32_t base = 0; int nid = 0;
    for (uint32_t mi = 0; mi < nm; mi++) {
        const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t v = 0; v < m->vertex_count; v++) {
            int32_t q[3]; for (int k = 0; k < 3; k++) q[k] = (int32_t)lrintf(m->vertices[v].position[k] * 50.0f);
            uint32_t h = ((uint32_t)q[0] * 73856093u) ^ ((uint32_t)q[1] * 19349663u) ^ ((uint32_t)q[2] * 83492791u);
            uint32_t s = h & (cap - 1); int got = -1;
            while (hv[s] >= 0) { if (hk[s*3] == q[0] && hk[s*3+1] == q[1] && hk[s*3+2] == q[2]) { got = hv[s]; break; } s = (s + 1) & (cap - 1); }
            if (got < 0) { hk[s*3] = q[0]; hk[s*3+1] = q[1]; hk[s*3+2] = q[2]; hv[s] = nid; got = nid++; }
            vid[base + v] = got;
        }
        base += m->vertex_count;
    }
    int *par = (int *)malloc((size_t)nid * sizeof(int)); for (int i = 0; i < nid; i++) par[i] = i;
    base = 0;
    for (uint32_t mi = 0; mi < nm; mi++) {
        const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t t = 0; t + 2 < m->index_count; t += 3) {
            int a = cm_find(par, vid[base + m->indices[t]]), b = cm_find(par, vid[base + m->indices[t + 1]]), c = cm_find(par, vid[base + m->indices[t + 2]]);
            if (a != b) par[b] = a; b = cm_find(par, vid[base + m->indices[t + 1]]); if (a != c) par[cm_find(par, c)] = a; (void)b;
        }
        base += m->vertex_count;
    }
    int *root2comp = (int *)malloc((size_t)nid * sizeof(int)); for (int i = 0; i < nid; i++) root2comp[i] = -1;
    g_cm_tricomp = (int *)malloc(tott * sizeof(int));
    int ncomp = 0; base = 0;
    for (uint32_t mi = 0; mi < nm; mi++) {
        const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t t = 0; t + 2 < m->index_count; t += 3) {
            int r = cm_find(par, vid[base + m->indices[t]]);
            if (root2comp[r] < 0) root2comp[r] = ncomp++;
            g_cm_tricomp[g_cm_off[mi] + t / 3] = root2comp[r];
        }
        base += m->vertex_count;
    }
    g_cm_info = (CompInfo *)calloc((size_t)ncomp, sizeof(CompInfo));
    for (int i = 0; i < ncomp; i++) for (int k = 0; k < 3; k++) { g_cm_info[i].mn[k] = 1e30f; g_cm_info[i].mx[k] = -1e30f; }
    for (uint32_t mi = 0; mi < nm; mi++) {
        const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t t = 0; t + 2 < m->index_count; t += 3) {
            CompInfo *ci = &g_cm_info[g_cm_tricomp[g_cm_off[mi] + t / 3]]; ci->ntri++;
            for (int e = 0; e < 3; e++) { const float *p = m->vertices[m->indices[t + e]].position;
                for (int k = 0; k < 3; k++) { ci->c[k] += p[k] / 3.0f; if (p[k] < ci->mn[k]) ci->mn[k] = p[k]; if (p[k] > ci->mx[k]) ci->mx[k] = p[k]; } }
        }
    }
    for (int i = 0; i < ncomp; i++) for (int k = 0; k < 3; k++) g_cm_info[i].c[k] /= (float)(g_cm_info[i].ntri > 0 ? g_cm_info[i].ntri : 1);
    free(hk); free(hv); free(vid); free(par); free(root2comp);
    g_cm_n = ncomp;
    return ncomp;
}
/* c(3), mn(3), mx(3), ntri en out[10] */
int g3d_editor_comp_info(int i, float *out10) {
    if (!g_cm_info || i < 0 || i >= g_cm_n) return 0;
    const CompInfo *c = &g_cm_info[i];
    for (int k = 0; k < 3; k++) { out10[k] = c->c[k]; out10[3 + k] = c->mn[k]; out10[6 + k] = c->mx[k]; }
    out10[9] = (float)c->ntri; return 1;
}
/* La pieza bajo el raton: la del triangulo mas cercano (modelo colocado como en g3d_editor_model_pick). -1 si no hay. */
int g3d_editor_comp_pick(float sx, float sy, float w, float h, void *model_ptr, float tx, float ty, float tz, float scale, float yaw_deg) {
    G3DModel *model = (G3DModel *)model_ptr; Vec3 o, d;
    if (!model || !screen_ray_world(sx, sy, w, h, &o, &d)) return -1;
    if (g3d_editor_comp_build(model) <= 0) return -1;
    if (scale <= 1e-6f) scale = 1.0f;
    float a = -yaw_deg * 0.017453292f, ca = cosf(a), sa = sinf(a);
    float ox = o.x - tx, oy = o.y - ty, oz = o.z - tz;
    float lox = (ox * ca + oz * sa) / scale, loy = oy / scale, loz = (-ox * sa + oz * ca) / scale;
    float ldx = (d.x * ca + d.z * sa) / scale, ldy = d.y / scale, ldz = (-d.x * sa + d.z * ca) / scale;
    float best = 1e30f; int comp = -1;
    for (uint32_t mi = 0; mi < model->mesh_count; mi++) {
        const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t i = 0; i + 2 < m->index_count; i += 3) {
            const float *p0 = m->vertices[m->indices[i]].position, *p1 = m->vertices[m->indices[i + 1]].position, *p2 = m->vertices[m->indices[i + 2]].position;
            float e1x = p1[0]-p0[0], e1y = p1[1]-p0[1], e1z = p1[2]-p0[2], e2x = p2[0]-p0[0], e2y = p2[1]-p0[1], e2z = p2[2]-p0[2];
            float px = ldy*e2z - ldz*e2y, py = ldz*e2x - ldx*e2z, pz = ldx*e2y - ldy*e2x;
            float det = e1x*px + e1y*py + e1z*pz; if (fabsf(det) < 1e-12f) continue;
            float inv = 1.0f / det, tx_ = lox-p0[0], ty_ = loy-p0[1], tz_ = loz-p0[2];
            float u = (tx_*px + ty_*py + tz_*pz) * inv; if (u < 0.0f || u > 1.0f) continue;
            float qx = ty_*e1z - tz_*e1y, qy = tz_*e1x - tx_*e1z, qz = tx_*e1y - ty_*e1x;
            float v = (ldx*qx + ldy*qy + ldz*qz) * inv; if (v < 0.0f || u + v > 1.0f) continue;
            float t = (e2x*qx + e2y*qy + e2z*qz) * inv;
            if (t > 1e-4f && t < best) { best = t; comp = g_cm_tricomp[g_cm_off[mi] + i / 3]; }
        }
    }
    return comp;
}
/* Los triangulos que se quedan (las piezas con excl[i] == 0), sin compartir vertices; el que llama los libera con free(). */
static int cm_tri_ok(const G3DMesh *m, uint32_t t, float min_ny, float min_alto) {
    if (min_ny <= -1.5f && min_alto <= 0.0f) return 1;
    const float *p0 = m->vertices[m->indices[t]].position, *p1 = m->vertices[m->indices[t + 1]].position, *p2 = m->vertices[m->indices[t + 2]].position;
    float e1x = p1[0]-p0[0], e1y = p1[1]-p0[1], e1z = p1[2]-p0[2], e2x = p2[0]-p0[0], e2y = p2[1]-p0[1], e2z = p2[2]-p0[2];
    float nx = e1y*e2z - e1z*e2y, ny = e1z*e2x - e1x*e2z, nz = e1x*e2y - e1y*e2x;
    float l = sqrtf(nx*nx + ny*ny + nz*nz); if (l < 1e-12f) return 0;
    float nyn = ny / l;
    if (nyn < min_ny) return 0;
    /* una cara empinada de menos de min_alto de alto es un bordillo, un escalon o el canto de una pegatina: el kart se encaja ahi */
    if (min_alto > 0.0f && fabsf(nyn) < 0.6f) { float ymin = p0[1], ymax = p0[1]; if (p1[1] < ymin) ymin = p1[1]; if (p2[1] < ymin) ymin = p2[1]; if (p1[1] > ymax) ymax = p1[1]; if (p2[1] > ymax) ymax = p2[1]; if (ymax - ymin < min_alto) return 0; }
    return 1;   /* los triangulos que miran hacia abajo (techos, bajos de puentes) se pueden descartar */
}
int g3d_editor_comp_kept(void *model_ptr, const unsigned char *excl, float min_ny, float min_alto, float **pos, uint32_t **idx, uint32_t *nv, uint32_t *ni) {
    G3DModel *model = (G3DModel *)model_ptr; if (!model || g3d_editor_comp_build(model) <= 0) return 0;
    size_t cnt = 0;
    for (uint32_t mi = 0; mi < model->mesh_count; mi++) { const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t t = 0; t + 2 < m->index_count; t += 3) if (!excl[g_cm_tricomp[g_cm_off[mi] + t / 3]] && cm_tri_ok(m, t, min_ny, min_alto)) cnt++; }
    if (cnt == 0) return 0;
    float *P = (float *)malloc(cnt * 9 * sizeof(float)); uint32_t *I = (uint32_t *)malloc(cnt * 3 * sizeof(uint32_t)); size_t k = 0;
    for (uint32_t mi = 0; mi < model->mesh_count; mi++) { const G3DMesh *m = &model->meshes[mi]; if (!m->vertices || !m->indices) continue;
        for (uint32_t t = 0; t + 2 < m->index_count; t += 3) if (!excl[g_cm_tricomp[g_cm_off[mi] + t / 3]] && cm_tri_ok(m, t, min_ny, min_alto)) {
            for (int e = 0; e < 3; e++) { const float *p = m->vertices[m->indices[t + e]].position; P[k*9 + e*3] = p[0]; P[k*9 + e*3 + 1] = p[1]; P[k*9 + e*3 + 2] = p[2]; I[k*3 + e] = (uint32_t)(k*3 + e); }
            k++; } }
    *pos = P; *idx = I; *nv = (uint32_t)(cnt * 3); *ni = (uint32_t)(cnt * 3); return 1;
}
