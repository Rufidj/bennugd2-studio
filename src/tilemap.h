// ============================================================================
//  tilemap.h - MODO "PLATAFORMAS 2.5D" del editor: pintar niveles por tiles con textura.
//
//  Formato (el mismo que lee el juego, ver MegaHD2D/levels/LEEME.md):
//    tiles/tileset.def   un tile por linea: textura, tapa, solido, dano, unavia, resbala,
//                        rebota, cinta, mueve=eje:bloques:vel, forma, marcador
//    tiles/*.png         una textura por tile
//    levels/*.png        el mapa: 1 pixel = 1 bloque; R = plano jugable, G = fondo,
//                        B = primer plano (numero de tile; 0 = vacio). La fila 0 es la de ARRIBA.
//
//  Todo el estado vive en TileEditor; main.cpp solo llama a draw_*() y le da las rutas.
//  Los indices de tile NO se reordenan nunca (los mapas guardan el numero), por eso no hay
//  "borrar tile": se edita o se reutiliza.
// ============================================================================
#ifndef TILEMAP_H
#define TILEMAP_H

#include "imgui.h"
#include "imfilebrowser.h"
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <filesystem>
extern "C" {      // niveles binarios (.lvl) de libmod_3d (libmod_3d_lvl.h)
    struct G3DLvl;
    G3DLvl* g3d_lvl_new(void); void g3d_lvl_free(G3DLvl* l); G3DLvl* g3d_lvl_load(const char* path); int g3d_lvl_save(const G3DLvl* l, const char* path);
    int g3d_lvl_plane_count(const G3DLvl* l); int g3d_lvl_plane_exists(const G3DLvl* l, int plane); int g3d_lvl_width(const G3DLvl* l, int plane); int g3d_lvl_height(const G3DLvl* l, int plane);
    int g3d_lvl_plane_alloc(G3DLvl* l, int plane, int w, int h); void g3d_lvl_plane_remove(G3DLvl* l, int plane); uint8_t* g3d_lvl_layer(G3DLvl* l, int plane, int layer);
    const char* g3d_lvl_links(const G3DLvl* l); void g3d_lvl_set_links(G3DLvl* l, const char* text);
}
#include <functional>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "hud2d.h"

namespace tm_fs = std::filesystem;

#define TM_WIN_MAP   "Mapa de tiles"
#define TM_WIN_SET   "Tileset"
#define TM_WIN_PROPS "Propiedades del tile"
#define TM_WIN_BG    "Fondo y ambiente"
#define TM_WIN_CHAR  "Personajes (plataformas)"

// animacion de la hoja de sprites: fila, primera columna, numero de fotogramas y fotogramas por segundo
struct PlatAnim { int row = 0, col = 0, n = 1, fps = 1; };
// un personaje de plataformas: hoja de sprites, tamano de celda, ancla, escala y sus 9 animaciones
struct PlatChar {
    std::string sheet; int cw = 160; float ax = 0.5f, ay = 0.9375f, ppu = 52.0f; PlatAnim an[9];
};
// todo lo que el juego necesita saber de los personajes (se guarda en <proyecto>/plataformas.cfg)
struct PlatCfg {
    PlatChar hero; std::string hero_model; int hero_anim[7] = { 0, 1, 2, 3, 4, 5, -1 }; float hero_yaw = 0.0f; std::string hero_depth;   // hero_anim: numero de animacion del .glb para quieto, correr, andar, salto, dano, muerte   // hoja del heroe de espaldas y de frente (portales y tramos en profundidad); vacia = usa 'correr' de perfil
    float hl = 0.5f, hr = 0.8f, hh = 2.55f, run = 11.0f, jump = 24.0f, grav = 62.0f; int hp = 8;
    PlatChar en[2]; std::string en_model[2]; int en_hp[2] = { 3, 5 }; float en_w[2] = { 0.9f, 1.0f }, en_h[2] = { 1.5f, 1.7f };
    int hud = 1;
    static const char* hero_an(int i) { static const char* n[] = { "quieto", "correr", "subir", "cima", "caer", "disparar", "correr_disparando", "dano", "muerte" }; return n[i]; }
    static const char* en_an(int i)   { static const char* n[] = { "quieto", "abrirse", "andar", "atacar", "dano", "muerte", "subir", "cima", "caer" }; return n[i]; }

    // el heroe y los dos enemigos del juego de ejemplo (Roboman, el Met y la kaka)
    void defaults() {
        *this = PlatCfg();
        hero.sheet = "Assets/Sprites/roboman_sheet.png"; hero.cw = 160; hero.ax = 0.42f; hero.ay = 0.9375f; hero.ppu = 52.0f;
        const int h[9][4] = { {0,1,2,2},{1,0,6,12},{2,1,1,1},{2,2,1,1},{2,3,1,1},{3,1,2,10},{5,0,6,12},{4,0,1,1},{4,4,2,5} };
        for (int i = 0; i < 9; i++) hero.an[i] = { h[i][0], h[i][1], h[i][2], h[i][3] };
        hero_depth = "Assets/Sprites/roboman_depth_sheet.png";
        en[0].sheet = "Assets/Sprites/met_sheet.png"; en[0].cw = 128; en[0].ax = 0.42f; en[0].ay = 0.9375f; en[0].ppu = 55.0f;
        const int e0[9][4] = { {0,0,3,3},{0,3,3,1},{1,0,6,8},{2,0,4,1},{4,0,1,1},{4,2,3,1},{0,0,1,1},{0,0,1,1},{0,0,1,1} };
        for (int i = 0; i < 9; i++) en[0].an[i] = { e0[i][0], e0[i][1], e0[i][2], e0[i][3] };
        en[1].sheet = "Assets/Sprites/kaka_sheet.png"; en[1].cw = 160; en[1].ax = 0.5f; en[1].ay = 0.9375f; en[1].ppu = 44.0f;
        const int e1[9][4] = { {0,0,6,4},{0,0,1,1},{1,0,6,10},{3,0,6,1},{4,1,1,1},{5,0,6,1},{2,1,1,1},{2,2,1,1},{2,3,1,1} };
        for (int i = 0; i < 9; i++) en[1].an[i] = { e1[i][0], e1[i][1], e1[i][2], e1[i][3] };
    }
    bool load(const std::string& path) {
        std::ifstream f(path); if (!f) return false;
        std::string line;
        auto A = [](PlatAnim& a, const std::string& v) { sscanf(v.c_str(), "%d,%d,%d,%d", &a.row, &a.col, &a.n, &a.fps); };
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t p = line.find('='); if (p == std::string::npos || line[0] == '#') continue;
            std::string k = line.substr(0, p), v = line.substr(p + 1);
            auto ch = [&](const std::string& pre, PlatChar& c) -> bool {
                if (k.compare(0, pre.size(), pre) != 0) return false; std::string r = k.substr(pre.size());
                if (r == "hoja") c.sheet = v; else if (r == "celda") c.cw = atoi(v.c_str());
                else if (r == "ancla") sscanf(v.c_str(), "%f,%f", &c.ax, &c.ay); else if (r == "escala") c.ppu = (float)atof(v.c_str());
                else if (r.compare(0, 5, "anim.") == 0) { std::string n = r.substr(5);
                    for (int i = 0; i < 9; i++) if (n == (&c == &hero ? hero_an(i) : en_an(i))) A(c.an[i], v); }
                return true;
            };
            if (ch("heroe.", hero)) {
                if (k == "heroe.profundidad") hero_depth = v;
                if (k == "heroe.modelo") hero_model = v;
                if (k == "heroe.anims") sscanf(v.c_str(), "%d,%d,%d,%d,%d,%d,%d", &hero_anim[0], &hero_anim[1], &hero_anim[2], &hero_anim[3], &hero_anim[4], &hero_anim[5], &hero_anim[6]);
                if (k == "heroe.giro") hero_yaw = (float)atof(v.c_str());
                else if (k == "heroe.caja") sscanf(v.c_str(), "%f,%f,%f", &hl, &hr, &hh);
                else if (k == "heroe.mov") sscanf(v.c_str(), "%f,%f,%f,%d", &run, &jump, &grav, &hp);
            } else if (ch("enemigo1.", en[0])) {
                if (k == "enemigo1.modelo") en_model[0] = v; else if (k == "enemigo1.vida") en_hp[0] = atoi(v.c_str()); else if (k == "enemigo1.caja") sscanf(v.c_str(), "%f,%f", &en_w[0], &en_h[0]);
            } else if (ch("enemigo2.", en[1])) {
                if (k == "enemigo2.modelo") en_model[1] = v; else if (k == "enemigo2.vida") en_hp[1] = atoi(v.c_str()); else if (k == "enemigo2.caja") sscanf(v.c_str(), "%f,%f", &en_w[1], &en_h[1]);
            } else if (k == "hud") hud = atoi(v.c_str());
        }
        return true;
    }
    bool save(const std::string& path) const {
        std::ofstream f(path); if (!f) return false; char b[200];
        f << "# Personajes de los juegos de plataformas (panel 'Personajes (plataformas)' del editor).\n"
             "# animaciones: fila,primera_columna,fotogramas,fps\n";
        auto C = [&](const std::string& pre, const PlatChar& c, bool heroe) {
            f << pre << "hoja=" << c.sheet << "\n" << pre << "celda=" << c.cw << "\n";
            snprintf(b, sizeof b, "%s" "ancla=%g,%g\n", pre.c_str(), c.ax, c.ay); f << b;
            snprintf(b, sizeof b, "%s" "escala=%g\n", pre.c_str(), c.ppu); f << b;
            for (int i = 0; i < 9; i++) { snprintf(b, sizeof b, "%sanim.%s=%d,%d,%d,%d\n", pre.c_str(), heroe ? hero_an(i) : en_an(i), c.an[i].row, c.an[i].col, c.an[i].n, c.an[i].fps); f << b; }
        };
        C("heroe.", hero, true); f << "heroe.profundidad=" << hero_depth << "\n" << "heroe.modelo=" << hero_model << "\n" << "heroe.anims=" << hero_anim[0] << "," << hero_anim[1] << "," << hero_anim[2] << "," << hero_anim[3] << "," << hero_anim[4] << "," << hero_anim[5] << "," << hero_anim[6] << "\n" << "heroe.giro=" << hero_yaw << "\n";
        snprintf(b, sizeof b, "heroe.caja=%g,%g,%g\nheroe.mov=%g,%g,%g,%d\n", hl, hr, hh, run, jump, grav, hp); f << b;
        for (int e = 0; e < 2; e++) {
            std::string pre = e ? "enemigo2." : "enemigo1."; C(pre, en[e], false);
            snprintf(b, sizeof b, "%svida=%d\n%scaja=%g,%g\n", pre.c_str(), en_hp[e], pre.c_str(), en_w[e], en_h[e]); f << b; f << pre << "modelo=" << en_model[e] << "\n";
        }
        f << "hud=" << hud << "\n"; return true;
    }
    // el codigo BennuGD2 que pone esa configuracion en la biblioteca (plataformas.prg)
    std::string emit() const {
        std::string o; char b[512];
        snprintf(b, sizeof b, "    pl_hero_sheet = \"%s\"; pl_hero_cw = %d; pl_hero_ch = %d; pl_hero_ax = %g; pl_hero_ay = %g; pl_hero_ppu = %g;\n", hero.sheet.c_str(), hero.cw, hero.cw, hero.ax, hero.ay, hero.ppu); o += b;
        snprintf(b, sizeof b, "    pl_hero_hl = %g; pl_hero_hr = %g; pl_hero_hh = %g; pl_hero_run = %g; pl_hero_jump = %g; pl_hero_grav = %g; pl_hero_hp = %d; pl_hud_on = %d;\n", hl, hr, hh, run, jump, grav, hp, hud); o += b;
        o += "    pl_hero_dsheet = \"" + hero_depth + "\";\n";
        o += "    pl_hero_model = \"" + hero_model + "\";\n";
        for (int i = 0; i < 7; i++) o += "    pl_hm_map[" + std::to_string(i) + "] = " + std::to_string(hero_anim[i]) + ";\n";
        { char gb[64]; snprintf(gb, sizeof gb, "    pl_hm_yawoff = %g;\n", hero_yaw); o += gb; }
        for (int i = 0; i < 9; i++) { snprintf(b, sizeof b, "    pl_ha_row[%d] = %d; pl_ha_col[%d] = %d; pl_ha_n[%d] = %d; pl_ha_fps[%d] = %d;   // %s\n", i, hero.an[i].row, i, hero.an[i].col, i, hero.an[i].n, i, hero.an[i].fps, hero_an(i)); o += b; }
        for (int e = 0; e < 2; e++) {
            const PlatChar& c = en[e];
            snprintf(b, sizeof b, "    pl_en_sheet[%d] = \"%s\"; pl_en_cw[%d] = %d; pl_en_ax[%d] = %g; pl_en_ay[%d] = %g; pl_en_ppu[%d] = %g; pl_en_hp[%d] = %d; pl_en_w[%d] = %g; pl_en_h[%d] = %g;\n",
                     e, c.sheet.c_str(), e, c.cw, e, c.ax, e, c.ay, e, c.ppu, e, en_hp[e], e, en_w[e], e, en_h[e]); o += b;
            o += "    pl_en_model[" + std::to_string(e) + "] = \"" + en_model[e] + "\";\n";
            for (int i = 0; i < 9; i++) { snprintf(b, sizeof b, "    pl_ea_row[%d] = %d; pl_ea_col[%d] = %d; pl_ea_n[%d] = %d; pl_ea_fps[%d] = %d;\n", e * 9 + i, c.an[i].row, e * 9 + i, c.an[i].col, e * 9 + i, c.an[i].n, e * 9 + i, c.an[i].fps); o += b; }
        }
        return o;
    }
};

// Enlace de una casilla (bloque LNKS del .lvl): nombre del objeto y a donde apunta. Puerta: nombre. Boton/diana: nombres de las puertas que abre
// (separados por comas). Portal de nivel: escena/puerta de destino.
struct TmLink { int plane = 0, x = 0, y = 0; std::string name, tgt; };

struct TmTile {
    float oh = 0.0f;                                    // alto en bloques cuando el tile va en la capa 'Decorado' (0 = 1 bloque)
    std::string name, tex, top, marker, shape, extra;   // marker: inicio|met|kaka|check; shape: ""|pinchos
    int   solid = 1, dmg = 0, oneway = 0;
    float slip = 0.0f, bounce = 0.0f, conv = 0.0f;
    int   axis = 0;                                     // 0 quieto, 1 mueve en x, 2 en y
    float dist = 0.0f, spd = 0.0f;
    int   liq = 0;                                      // 0 no, 1 agua, 2 lava, 3 acido, 4 cieno
    int   swim = 0;                                     // se nada (gravedad baja, el salto sube)
    float slow = 1.0f;                                  // 1 = velocidad normal; menos = mas lento
    int   alpha = 255;                                  // transparencia 0..255
    int   ramp = 0;                                     // 0 no, 1 sube hacia la derecha, 2 sube hacia la izquierda
    int   dir = 0;                                      // puerta: hacia donde se abre (0/1 arriba, 2 abajo, 3 izquierda, 4 derecha)
    std::string dest;                                   // portal 2: nivel al que lleva (p. ej. un pasillo)
    int   portal = 0;                                   // 1 = pasa al plano de detras (hacia el fondo), -1 = al de delante
    int   door = 0, btn = 0, tgt = 0;                   // canal (1..15) de puerta, boton de pisar y diana (disparar)
    float wait = 0.0f;                                  // pausa (s) en cada extremo de lo que se mueve
    int   on_step = 0;                                  // se mueve solo mientras vas encima (ascensor)
    float hold = 0.0f;                                  // boton: segundos activo tras soltarlo (0 = por defecto, -1 = siempre)
};

// ajustes de ambiente de un nivel (levels/<nivel>.level)
static constexpr int TM_NL = 8;   // capas de tiles por nivel (la 6 es la EXTERIOR: fachada / techo, solo se ve desde fuera cuando el mapa esta dentro de un mundo 3D)

struct TmCfg {
    float sky[3] = { 0.04f, 0.06f, 0.16f }, hor[3] = { 0.10f, 0.14f, 0.30f }, fog[3] = { 0.04f, 0.05f, 0.12f };
    float fog_n = 40.0f, fog_f = 110.0f;
    float amb[3] = { 0.42f, 0.46f, 0.58f }, sun[3] = { 1.0f, 0.95f, 0.85f }; float sun_i = 1.5f;
    std::string bg; float bg_mov = 0.85f; int columnas = 1;
    // capas de decorado 1..5: z, grosor y escala (la escala mantiene el tamano aparente de lo lejano: (16+|z|)/16)
    float lz[TM_NL] = { 0, -4.2f, 2.4f, -8.0f, -14.0f, 5.0f, 2.2f, -1.0f }, lsz[TM_NL] = { 0, 1.6f, 0.5f, 1.6f, 1.6f, 0.5f, 2.0f, 1.0f }, ls[TM_NL] = { 1, 1.0f, 1.0f, 1.5f, 1.875f, 0.69f, 1.0f, 1.0f };
    std::string cor_dest;                               // nivel de plataformas: pasillo (otro nivel con modo=pasillo) que va DENTRO de este, detras del portal
    int cor_fin = 0;                                    // 0 = al terminar el pasillo se vuelve al portal; 1 = desemboca en el plano 1 (otro 2D al fondo)
    int corridor = 0;                                   // 1 = nivel de pasillo estilo Crash (vista desde arriba: R suelo, G altura, B marcadores)
    int bg_rep = 0; float bg_alto = 36.0f;              // fondo en mosaico (repetido) y su alto
};

struct TileEditor {
    // ---- rutas ----
    std::string game_dir;                 // carpeta del juego de plataformas (con tiles/ y levels/)
    std::string bgdc, bgdi;               // las pone main.cpp
    std::string starter_dir;              // runtime/starter: tiles, fondos, sprites y un nivel de ejemplo
    std::function<void(const std::string&)> on_created;   // se acaba de crear el ejemplo: main.cpp lo pone como escena inicial
    std::function<void(const std::string&)> on_play;   // "Probar nivel": main.cpp compila y ejecuta el proyecto empezando por esta escena
    PlatCfg pcfg;                         // personajes
    bool    pcfg_dirty = false;
    std::string level = "level1";         // nivel abierto (sin .png)
    std::string msg;                      // ultimo aviso
    bool  inited = false;

    // ---- tileset ----
    std::vector<TmTile> tiles;            // el tile i+1 es tiles[i]
    std::string header;                   // comentarios del .def, se conservan al guardar
    int   sel_tile = 1;
    int   brush_h = 1;                    // pasillo: altura (en bloques) con la que se pinta la capa 'Altura'
    bool  set_dirty = false;
    std::map<std::string, H2Img> imgs;    // cache de texturas

    // ---- mapa ----
    int W = 0, H = 0;
    std::vector<uint8_t> L[TM_NL];       // capas: 0 plano jugable, 1 fondo cercano, 2 primer plano, 3 fondo medio, 4 fondo lejano, 5 primer plano cercano
    int   cur_layer = 0;
    bool  vis[TM_NL] = { true, true, true, true, true, true, true, true };
    bool  map_dirty = false;

    // ---- vista y herramientas ----
    float zoom = 44.0f;
    ImVec2 pan = ImVec2(20, 20);
    std::vector<TmLink> links; int sel_c = -1, sel_r = -1;      // enlaces de todos los planos y la casilla elegida con la herramienta Enlazar
    int   tool = 0;                       // 0 lapiz, 1 goma, 2 rectangulo, 3 relleno, 4 cuentagotas
    bool  stroke = false; int lc = -1, lr = -1;
    int   rc0 = 0, rr0 = 0;               // inicio del rectangulo
    bool  rect_erase = false;
    struct Snap { int W, H; std::vector<uint8_t> L[TM_NL]; };
    std::vector<Snap> undo, redo;
    int   new_w = 80, new_h = 20; char new_name[64] = "level2"; bool new_floor = true;
    int   rs_w = 0, rs_h = 0;
    char  dir_buf[512] = "";
    bool  show_new = false, new_corr = false;
    TmCfg cfg; bool cfg_dirty = false;      // ambiente y fondo del nivel abierto
    ImGui::FileBrowser fb_tex, fb_bg; bool fb_ready = false;
    char  tex_name[64] = "";
    bool  fit_pending = true;             // ajustar la vista al nivel (se espera unos fotogramas a que la ventana tenga su tamano)
    int   fit_wait = 0;

    // ------------------------------------------------------------------ rutas
    // el juego es el PROYECTO del editor: tiles, niveles y fondos viven en su Assets/
    std::string tiles_dir()  const { return game_dir + "/Assets/Tiles"; }
    std::string levels_dir() const { return game_dir + "/Assets/Levels"; }
    std::string scenes_dir() const { return game_dir + "/Scenes"; }
    std::string sprites_dir() const { return game_dir + "/Assets/Sprites"; }
    std::string cfg_file()   const { return game_dir + "/plataformas.cfg"; }
    int plane = 0;                                                  // plano jugable que se edita: 0 = el de delante, 1 y 2 = los de detras
    std::string stem() const { return level + (plane ? ".p" + std::to_string(plane) : ""); }
    std::string capas_path() const { return levels_dir() + "/" + stem() + ".capas.png"; }
    std::string level_path() const { return levels_dir() + "/" + stem() + ".png"; }
    bool plane_exists(int p) const {
        if (p < 0 || p > 7) return false;
        if (planes_mask < 0) { planes_mask = 0; if (G3DLvl* l = g3d_lvl_load(lvl_path(level).c_str())) { for (int i = 0; i < 8; i++) if (g3d_lvl_plane_exists(l, i)) planes_mask |= 1 << i; g3d_lvl_free(l); }
                               else { std::error_code ec; for (int i = 0; i < 8; i++) if (tm_fs::exists(levels_dir() + "/" + level + (i ? ".p" + std::to_string(i) : "") + ".png", ec)) planes_mask |= 1 << i; } }
        return (planes_mask >> p) & 1;
    }
    std::string cfg_path()   const { return levels_dir() + "/" + level + ".level"; }
    std::string bg_dir()     const { return game_dir + "/Assets/Backgrounds"; }
    bool dir_ok() const { return !game_dir.empty() && tm_fs::exists(tiles_dir()) && tm_fs::exists(levels_dir()); }

    static std::vector<std::string> split_ws(const std::string& s) {
        std::vector<std::string> o; std::istringstream is(s); std::string t; while (is >> t) o.push_back(t); return o;
    }

    // ---------------------------------------------------------------- texturas
    H2Img* img_of(const std::string& file) {
        if (file.empty()) return nullptr;
        auto it = imgs.find(file);
        if (it == imgs.end()) {
            H2Img im = {}; h2_load_image((tiles_dir() + "/" + file).c_str(), &im);
            it = imgs.emplace(file, im).first;
        }
        return it->second.tex ? &it->second : nullptr;
    }
    void free_imgs() { for (auto& kv : imgs) if (kv.second.tex) h2_free_image(&kv.second); imgs.clear(); }

    // ----------------------------------------------------------------- tileset
    bool load_tileset() {
        tiles.clear(); header.clear(); free_imgs();
        std::ifstream f(tiles_dir() + "/tileset.def");
        if (!f) { msg = "No encuentro tiles/tileset.def en " + game_dir; return false; }
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto tok = split_ws(line);
            if (tok.empty()) continue;
            if (tok[0][0] == '#') { header += line + "\n"; continue; }
            if (tok[0] != "tile" || tok.size() < 2) continue;
            TmTile t; t.name = tok[1]; bool has_a = false, has_s = false;
            for (size_t k = 2; k < tok.size(); k++) {
                size_t p = tok[k].find('=');
                if (p == std::string::npos) { t.extra += " " + tok[k]; continue; }
                std::string key = tok[k].substr(0, p), val = tok[k].substr(p + 1);
                if (key == "textura") t.tex = val;
                else if (key == "tapa") t.top = val;
                else if (key == "solido") t.solid = atoi(val.c_str());
                else if (key == "dano") t.dmg = atoi(val.c_str());
                else if (key == "unavia") t.oneway = atoi(val.c_str());
                else if (key == "resbala") t.slip = (float)atof(val.c_str());
                else if (key == "rebota") t.bounce = (float)atof(val.c_str());
                else if (key == "cinta") t.conv = (float)atof(val.c_str());
                else if (key == "forma") t.shape = (val == "pinchos") ? val : "";
                else if (key == "marcador") { t.marker = val; t.solid = 0; }
                else if (key == "alto") t.oh = (float)atof(val.c_str());
                else if (key == "liquido") { t.liq = (val == "agua") ? 1 : (val == "lava") ? 2 : (val == "acido") ? 3 : (val == "cieno") ? 4 : 0; t.solid = 0; }
                else if (key == "nado") t.swim = atoi(val.c_str());
                else if (key == "rampa") { t.ramp = (val == "der") ? 1 : (val == "izq") ? 2 : 0; t.solid = 0; }
                else if (key == "portal") { if (val.compare(0, 6, "nivel:") == 0) { t.portal = 2; t.dest = val.substr(6); } else if (val == "pasillo") t.portal = 3; else if (val == "exterior") t.portal = 4; else t.portal = (val == "fondo") ? 1 : (val == "frente") ? -1 : 0; t.solid = 0; }
                else if (key == "puerta") t.door = atoi(val.c_str());
                else if (key == "abre") t.dir = (val == "abajo") ? 2 : (val == "izq") ? 3 : (val == "der") ? 4 : 1;
                else if (key == "boton") { t.btn = atoi(val.c_str()); t.solid = 0; }
                else if (key == "diana") t.tgt = atoi(val.c_str());
                else if (key == "espera") t.wait = (float)atof(val.c_str());
                else if (key == "activa") t.on_step = (val == "pisar");
                else if (key == "mantener") t.hold = (float)atof(val.c_str());
                else if (key == "lento") { t.slow = (float)atof(val.c_str()); has_s = true; }
                else if (key == "alfa") { t.alpha = atoi(val.c_str()); has_a = true; }
                else if (key == "mueve") {
                    size_t a = val.find(':'), b = val.find(':', a == std::string::npos ? 0 : a + 1);
                    if (a != std::string::npos && b != std::string::npos) {
                        t.axis = (val[0] == 'x') ? 1 : 2; t.dist = (float)atof(val.substr(a + 1, b - a - 1).c_str());
                        t.spd = (float)atof(val.substr(b + 1).c_str());
                    }
                } else t.extra += " " + tok[k];
            }
            if (t.liq && !has_a) t.alpha = 150;
            if (t.liq && !has_s) t.slow = 0.6f;
            tiles.push_back(t);
        }
        if (sel_tile < 1 || sel_tile > (int)tiles.size()) sel_tile = tiles.empty() ? 0 : 1;
        set_dirty = false; return true;
    }
    bool save_tileset() {
        std::ofstream f(tiles_dir() + "/tileset.def");
        if (!f) { msg = "No puedo escribir tileset.def"; return false; }
        f << (header.empty() ? "# Tileset: un tile por linea. El indice de cada tile es su posicion (1, 2, 3...). 0 = vacio.\n" : header);
        char b[64];
        for (const auto& t : tiles) {
            f << "tile " << t.name;
            if (!t.tex.empty()) f << " textura=" << t.tex;
            if (!t.top.empty()) f << " tapa=" << t.top;
            if (t.marker.empty()) f << " solido=" << t.solid;
            if (t.oh > 0.0f) { snprintf(b, sizeof b, "%g", t.oh); f << " alto=" << b; }
            if (t.dmg > 0) f << " dano=" << t.dmg;
            if (t.oneway) f << " unavia=1";
            if (t.slip > 0.0f)   { snprintf(b, sizeof b, "%g", t.slip);   f << " resbala=" << b; }
            if (t.bounce > 0.0f) { snprintf(b, sizeof b, "%g", t.bounce); f << " rebota=" << b; }
            if (t.conv != 0.0f)  { snprintf(b, sizeof b, "%g", t.conv);   f << " cinta=" << b; }
            if (t.liq) { static const char* ln[] = { "", "agua", "lava", "acido", "cieno" }; f << " liquido=" << ln[t.liq];
                         snprintf(b, sizeof b, "%g", t.slow); f << " lento=" << b; f << " alfa=" << t.alpha; if (t.swim) f << " nado=1"; }
            if (t.ramp) f << " rampa=" << (t.ramp == 1 ? "der" : "izq");
            if (t.portal == 2) f << " portal=nivel:" << t.dest; else if (t.portal == 3) f << " portal=pasillo"; else if (t.portal == 4) f << " portal=exterior"; else if (t.portal) f << " portal=" << (t.portal > 0 ? "fondo" : "frente");
            if (t.door) { f << " puerta=" << t.door; if (t.dir > 1) f << " abre=" << (t.dir == 2 ? "abajo" : t.dir == 3 ? "izq" : "der"); }
            if (t.btn)  { f << " boton=" << t.btn; if (t.hold != 0.0f) { snprintf(b, sizeof b, "%g", t.hold); f << " mantener=" << b; } }
            if (t.tgt)  f << " diana=" << t.tgt;
            if (t.axis && t.on_step) f << " activa=pisar";
            if (t.axis && t.wait > 0.0f) { snprintf(b, sizeof b, "%g", t.wait); f << " espera=" << b; }
            if (t.axis) { snprintf(b, sizeof b, "%c:%g:%g", t.axis == 1 ? 'x' : 'y', t.dist, t.spd); f << " mueve=" << b; }
            if (!t.shape.empty()) f << " forma=" << t.shape;
            if (!t.marker.empty()) f << " marcador=" << t.marker;
            f << t.extra << "\n";
        }
        set_dirty = false; return true;
    }

    // -------------------------------------------------------------------- mapa
    int  at(int l, int c, int r) const { return (c < 0 || r < 0 || c >= W || r >= H) ? 0 : L[l][r * W + c]; }
    void put(int l, int c, int r, int v) { if (c >= 0 && r >= 0 && c < W && r < H && L[l][r * W + c] != v) { L[l][r * W + c] = (uint8_t)v; map_dirty = true; } }
    void alloc(int w, int h) { W = w; H = h; for (auto& l : L) l.assign((size_t)w * h, 0); }

    // ---- niveles binarios (.lvl): un fichero por nivel con todos sus planos y capas (ver libmod_3d_lvl.h) ----
    std::string lvl_path(const std::string& name) const { return levels_dir() + "/" + name + ".lvl"; }
    mutable int planes_mask = -1;                                  // cache de que planos existen (bit p); -1 = por recalcular
    // importa los PNG antiguos de un nivel (planos .p1.., capas .capas.png) a un nivel binario en memoria; nullptr si no hay plano 0
    G3DLvl* import_png_level(const std::string& name) const {
        G3DLvl* l = g3d_lvl_new(); if (!l) return nullptr;
        auto rd = [](const std::string& path) -> SDL_Surface* {
            SDL_Surface* s = IMG_Load(path.c_str()); if (!s) return nullptr;
            SDL_Surface* c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGBA32, 0); SDL_FreeSurface(s); return c; };
        for (int p = 0; p < 8; p++) {
            std::string base = levels_dir() + "/" + name + (p ? ".p" + std::to_string(p) : "");
            SDL_Surface* c = rd(base + ".png"); if (!c) { if (p == 0) { g3d_lvl_free(l); return nullptr; } continue; }
            if (!g3d_lvl_plane_alloc(l, p, c->w, c->h)) { SDL_FreeSurface(c); continue; }
            for (int k = 0; k < 3; k++) { uint8_t* d = g3d_lvl_layer(l, p, k);
                for (int y = 0; y < c->h; y++) { const uint8_t* row = (const uint8_t*)c->pixels + (size_t)y * c->pitch; for (int x = 0; x < c->w; x++) d[y * c->w + x] = row[x * 4 + k]; } }
            if (SDL_Surface* c2 = rd(base + ".capas.png")) {
                for (int k = 0; k < 3; k++) { uint8_t* d = g3d_lvl_layer(l, p, 3 + k);
                    for (int y = 0; y < c2->h && y < c->h; y++) { const uint8_t* row = (const uint8_t*)c2->pixels + (size_t)y * c2->pitch; for (int x = 0; x < c2->w && x < c->w; x++) d[y * c->w + x] = row[x * 4 + k]; } }
                SDL_FreeSurface(c2);
            }
            SDL_FreeSurface(c);
        }
        return l;
    }
    // abre el nivel binario; si solo existe en PNG lo convierte (sin tocar los PNG) y lo deja guardado como .lvl
    G3DLvl* open_lvl(const std::string& name) const {
        if (G3DLvl* l = g3d_lvl_load(lvl_path(name).c_str())) return l;
        G3DLvl* l = import_png_level(name);
        if (l) g3d_lvl_save(l, lvl_path(name).c_str());
        return l;
    }
    bool fill_from(G3DLvl* l, int p) {
        if (!g3d_lvl_plane_exists(l, p)) return false;
        alloc(g3d_lvl_width(l, p), g3d_lvl_height(l, p));
        for (int k = 0; k < TM_NL; k++) if (uint8_t* d = g3d_lvl_layer(l, p, k)) memcpy(L[k].data(), d, (size_t)W * H);
        links = parse_links(g3d_lvl_links(l)); sel_c = sel_r = -1;
        return true;
    }
    bool load_level(const std::string& name) {
        plane = 0; planes_mask = -1;
        G3DLvl* l = open_lvl(name);
        if (!l) { msg = "No puedo abrir el nivel " + name; return false; }
        bool ok = fill_from(l, 0); g3d_lvl_free(l);
        if (!ok) { msg = "Nivel no valido: " + name; return false; }
        level = name; map_dirty = false; undo.clear(); redo.clear(); load_cfg();
        fit_pending = true; rs_w = W; rs_h = H; msg = "Nivel " + name + " abierto (" + std::to_string(W) + "x" + std::to_string(H) + ")"; return true;
    }
    // lee el plano 'plane' (capas 0..5) del disco; false si no existe
    bool read_plane() {
        G3DLvl* l = open_lvl(level); if (!l) return false;
        bool ok = fill_from(l, plane); g3d_lvl_free(l); return ok;
    }
    // cambia de plano jugable (guarda el actual); si el de destino no existe se crea vacio con el mismo tamano
    void set_plane(int p) {
        if (map_dirty) save_level();
        int w = W, h = H;
        if (p == 0) { std::string n = level; load_level(n); return; }
        plane = p;
        if (!read_plane()) { alloc(w, h); if (!save_level()) { plane = 0; return; } }   // plano nuevo: vacio
        undo.clear(); redo.clear(); map_dirty = false; fit_pending = true;
        msg = "Plano " + std::to_string(plane) + " (detras del " + std::to_string(plane - 1) + ")";
    }
    static std::vector<TmLink> parse_links(const char* txt) {
        std::vector<TmLink> r; std::istringstream in(txt ? txt : ""); std::string ln;
        while (std::getline(in, ln)) {
            std::vector<std::string> f; size_t a = 0;
            for (;;) { size_t b = ln.find(';', a); if (b == std::string::npos) { f.push_back(ln.substr(a)); break; } f.push_back(ln.substr(a, b - a)); a = b + 1; }
            if (f.size() < 4) continue;
            TmLink k; k.plane = atoi(f[0].c_str()); k.x = atoi(f[1].c_str()); k.y = atoi(f[2].c_str()); k.name = f[3]; if (f.size() > 4) k.tgt = f[4]; r.push_back(k);
        }
        return r;
    }
    std::string links_text() const {
        std::string s;
        for (auto& k : links) if (!k.name.empty() || !k.tgt.empty()) s += std::to_string(k.plane) + ";" + std::to_string(k.x) + ";" + std::to_string(k.y) + ";" + k.name + ";" + k.tgt + "\n";
        return s;
    }
    TmLink* link_at(int c, int r, bool make) {
        for (auto& k : links) if (k.plane == plane && k.x == c && k.y == r) return &k;
        if (!make) return nullptr;
        TmLink k; k.plane = plane; k.x = c; k.y = r; links.push_back(k); return &links.back();
    }
    // que objeto enlazable hay en la casilla: 1 puerta, 2 boton, 3 diana, 4 portal a otro nivel (0 = ninguno). Se mira el suelo y los marcadores (pasillos).
    int link_kind(int c, int r) const {
        for (int l : { 0, 2 }) {
            int v = (c >= 0 && r >= 0 && c < W && r < H) ? L[l][r * W + c] : 0; if (v < 1 || v > (int)tiles.size()) continue;
            const TmTile& t = tiles[v - 1];
            if (t.door > 0) return 1; if (t.btn > 0) return 2; if (t.tgt > 0) return 3; if (t.portal == 2) return 4; if (t.axis > 0) return 5;
        }
        return 0;
    }
    std::string auto_name(int kind, int c, int r) const {
        static const char* b[] = { "", "puerta", "boton", "diana", "salida", "ascensor" }; int n = 1;
        for (;;) { std::string s = std::string(b[kind]) + std::to_string(n++); bool used = false; for (auto& k : links) if (k.plane == plane && k.name == s) used = true; if (!used) return s; (void)c; (void)r; }
    }
    bool save_level() {
        if (W <= 0) return false;
        G3DLvl* l = open_lvl(level); if (!l) l = g3d_lvl_new(); if (!l) return false;
        g3d_lvl_set_links(l, links_text().c_str());
        bool ok = g3d_lvl_plane_alloc(l, plane, W, H);
        if (ok) { for (int k = 0; k < TM_NL; k++) if (uint8_t* d = g3d_lvl_layer(l, plane, k)) memcpy(d, L[k].data(), (size_t)W * H);
                  ok = g3d_lvl_save(l, lvl_path(level).c_str()) != 0; }
        g3d_lvl_free(l); planes_mask = -1;
        if (ok) { ensure_plat_scene(level); map_dirty = false; if (cfg_dirty) save_cfg(); msg = "Guardado " + level + ".lvl"; } else msg = "No puedo guardar " + lvl_path(level);
        return ok;
    }
    int find_tile(const std::string& name) const { for (size_t i = 0; i < tiles.size(); i++) if (tiles[i].name == name) return (int)i + 1; return 0; }
    void new_level(const std::string& name, int w, int h, bool with_floor, bool corridor = false) {
        w = (w + 3) / 4 * 4;
        alloc(w, h); level = name; undo.clear(); redo.clear(); cfg = TmCfg(); cfg_dirty = false;
        if (with_floor) {
            int ground = find_tile("metal"); if (!ground) ground = 1;
            for (int c = 0; c < w; c++) { L[0][(h - 1) * W + c] = (uint8_t)ground; L[0][(h - 2) * W + c] = (uint8_t)ground; }
            for (int r = 0; r < h; r++) { L[0][r * W] = (uint8_t)ground; L[0][r * W + w - 1] = (uint8_t)ground; }
            int st = find_tile("inicio"); if (st) L[0][(h - 3) * W + 3] = (uint8_t)st;
        }
        if (corridor) {                                  // pasillo: suelo en todas las celdas y el inicio al principio (fila de abajo)
            cfg.corridor = 1; cfg_dirty = true; plane = 0;
            int ground = find_tile("metal"); if (!ground) ground = 1;
            for (int i = 0; i < W * H; i++) { L[0][i] = (uint8_t)ground; L[1][i] = 0; L[2][i] = 0; }
            int st = find_tile("inicio"); if (st) L[2][(h - 1) * W + w / 2] = (uint8_t)st;
        }
        fit_pending = true; map_dirty = true; rs_w = W; rs_h = H; save_level(); ensure_plat_scene(name); msg = "Nivel nuevo " + name;
    }
    void resize_level(int nw, int nh) {
        nw = (nw + 3) / 4 * 4;                          // el ancho va en multiplos de 4: BennuGD2 (SDL_gpu) revienta leyendo imagenes de otros anchos                 // el suelo (abajo) y la izquierda se quedan donde estan
        if (nw < 8 || nh < 6 || nw * nh > 79999) { msg = "Tamano no valido (min 8x6, max 79999 bloques en total)"; return; }
        push_undo();
        std::vector<uint8_t> n[TM_NL]; for (auto& v : n) v.assign((size_t)nw * nh, 0);
        for (int l = 0; l < TM_NL; l++)
            for (int r = 0; r < H; r++)
                for (int c = 0; c < W; c++) {
                    int nr = r + (nh - H), nc = c;
                    if (nr >= 0 && nr < nh && nc < nw) n[l][nr * nw + nc] = L[l][r * W + c];
                }
        { std::vector<TmLink> nl; for (auto k : links) { if (k.plane == plane) k.y += nh - H; if (k.x < nw && k.y >= 0 && k.y < nh) nl.push_back(k); } links = nl; sel_c = sel_r = -1; }
        for (int l = 0; l < TM_NL; l++) L[l] = n[l]; W = nw; H = nh; map_dirty = true; fit_pending = true; rs_w = W; rs_h = H;
        msg = "Nivel redimensionado a " + std::to_string(W) + "x" + std::to_string(H);
    }
    // Un nivel ES una escena del proyecto: Scenes/<nivel>.plat guarda a que nivel apunta (el editor la incluye al generar el juego)
    bool level_is_corridor(const std::string& name) const {
        std::ifstream f(levels_dir() + "/" + name + ".level"); std::string ln;
        while (std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (ln == "modo=pasillo") return true; }
        return false;
    }
    void ensure_plat_scene(const std::string& name) {
        std::error_code ec; tm_fs::create_directories(scenes_dir(), ec);
        std::string f = scenes_dir() + "/" + name + ".plat";
        bool pas = (name == level) ? cfg.corridor != 0 : level_is_corridor(name);
        std::string want = std::string("# Escena de plataformas 2.5D: la crea el modo Plataformas del editor.\nnivel=Assets/Levels/") + name + ".png\n" + (pas ? "tipo=pasillo\n" : "");
        std::string have; { std::ifstream in(f); std::ostringstream ss; ss << in.rdbuf(); have = ss.str(); }
        if (have != want) { std::ofstream o(f); o << want; }
    }
    // Copia el ejemplo (tiles, fondos, hojas de sprites y un nivel) al proyecto y crea su escena y su configuracion
    bool crear_ejemplo() {
        if (starter_dir.empty() || !tm_fs::exists(starter_dir)) { msg = "No encuentro runtime/starter junto al editor"; return false; }
        std::error_code ec;
        const char* pares[][2] = { {"Tiles", "Assets/Tiles"}, {"Backgrounds", "Assets/Backgrounds"}, {"Sprites", "Assets/Sprites"}, {"Levels", "Assets/Levels"} };
        for (auto& pr : pares) {
            std::string src = starter_dir + "/" + pr[0], dst = game_dir + "/" + pr[1];
            if (!tm_fs::exists(src)) continue;
            tm_fs::create_directories(dst, ec);
            for (auto& e : tm_fs::directory_iterator(src)) {
                std::string d = dst + "/" + e.path().filename().string();
                if (!tm_fs::exists(d, ec)) tm_fs::copy_file(e.path(), d, ec);      // no pisa nada tuyo
            }
        }
        if (!tm_fs::exists(cfg_file(), ec)) { pcfg.defaults(); pcfg.save(cfg_file()); }
        for (auto& nm : level_names()) ensure_plat_scene(nm);
        if (on_created) on_created("level1");
        reload_all(); msg = "Ejemplo de plataformas creado en el proyecto"; return true;
    }
    void load_pcfg() { if (!pcfg.load(cfg_file())) pcfg.defaults(); pcfg_dirty = false; }
    std::vector<std::string> level_names() const {
        std::vector<std::string> v;
        if (tm_fs::exists(levels_dir()))
            for (auto& e : tm_fs::directory_iterator(levels_dir()))
                if ((e.path().extension() == ".png" || e.path().extension() == ".lvl") && e.path().stem().string()[0] != '_' && e.path().stem().string().find('.') == std::string::npos) v.push_back(e.path().stem().string());
        std::sort(v.begin(), v.end()); v.erase(std::unique(v.begin(), v.end()), v.end()); return v;
    }
    std::vector<std::string> tile_pngs() const {
        std::vector<std::string> v;
        if (tm_fs::exists(tiles_dir()))
            for (auto& e : tm_fs::directory_iterator(tiles_dir()))
                if (e.path().extension() == ".png" && e.path().stem().string()[0] != '_') v.push_back(e.path().filename().string());
        std::sort(v.begin(), v.end()); return v;
    }

    // ------------------------------------------------------- ambiente del nivel (.level)
    void load_cfg() {
        cfg = TmCfg(); cfg_dirty = false;
        std::ifstream f(cfg_path()); if (!f) return;
        std::string line;
        auto nums = [](const std::string& v, float* o, int n) { std::istringstream is(v); std::string t; int i = 0; while (i < n && std::getline(is, t, ',')) o[i++] = (float)atof(t.c_str()); return i; };
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t p = line.find('='); if (p == std::string::npos || line[0] == '#') continue;
            std::string k = line.substr(0, p), v = line.substr(p + 1); float a[5] = { 0, 0, 0, 0, 0 };
            if (k == "cielo") nums(v, cfg.sky, 3); else if (k == "horizonte") nums(v, cfg.hor, 3);
            else if (k == "niebla") { if (nums(v, a, 5) >= 5) { for (int i = 0; i < 3; i++) cfg.fog[i] = a[i]; cfg.fog_n = a[3]; cfg.fog_f = a[4]; } }
            else if (k == "ambiente") nums(v, cfg.amb, 3);
            else if (k == "luz") { if (nums(v, a, 4) >= 4) { for (int i = 0; i < 3; i++) cfg.sun[i] = a[i]; cfg.sun_i = a[3]; } }
            else if (k == "fondo") { cfg.bg = v; cfg.columnas = 0; }
            else if (k == "fondo_mov") cfg.bg_mov = (float)atof(v.c_str());
            else if (k == "fondo_modo") cfg.bg_rep = (v == "repetir");
            else if (k == "fondo_alto") cfg.bg_alto = (float)atof(v.c_str());
            else if (k == "columnas") cfg.columnas = atoi(v.c_str());
            else if (k == "modo") cfg.corridor = (v == "pasillo");
            else if (k == "pasillo") cfg.cor_dest = v;
            else if (k == "pasillo_fin") cfg.cor_fin = (v == "plano");
            else if (k.size() == 5 && k.compare(0, 4, "capa") == 0 && k[4] >= '1' && k[4] <= '6') { int n = k[4] - '0'; if (nums(v, a, 3) >= 3) { cfg.lz[n] = a[0]; cfg.lsz[n] = a[1]; cfg.ls[n] = a[2]; } }
        }
    }
    bool save_cfg() {
        std::ofstream f(cfg_path()); if (!f) { msg = "No puedo guardar " + cfg_path(); return false; }
        char b[160];
        f << "# Ajustes del nivel (colores 0..1). Todo es opcional. Los escribe el editor (panel Fondo y ambiente).\n";
        snprintf(b, sizeof b, "cielo=%g,%g,%g\nhorizonte=%g,%g,%g\n", cfg.sky[0], cfg.sky[1], cfg.sky[2], cfg.hor[0], cfg.hor[1], cfg.hor[2]); f << b;
        snprintf(b, sizeof b, "niebla=%g,%g,%g,%g,%g\n", cfg.fog[0], cfg.fog[1], cfg.fog[2], cfg.fog_n, cfg.fog_f); f << b;
        snprintf(b, sizeof b, "ambiente=%g,%g,%g\n", cfg.amb[0], cfg.amb[1], cfg.amb[2]); f << b;
        snprintf(b, sizeof b, "luz=%g,%g,%g,%g\n", cfg.sun[0], cfg.sun[1], cfg.sun[2], cfg.sun_i); f << b;
        if (!cfg.bg.empty()) { f << "fondo=" << cfg.bg << "\n"; snprintf(b, sizeof b, "fondo_mov=%g\n", cfg.bg_mov); f << b;
                               if (cfg.bg_rep) { f << "fondo_modo=repetir\n"; snprintf(b, sizeof b, "fondo_alto=%g\n", cfg.bg_alto); f << b; } }
        f << "columnas=" << cfg.columnas << "\n";
        if (cfg.corridor) f << "modo=pasillo\n";
        if (!cfg.cor_dest.empty()) { f << "pasillo=" << cfg.cor_dest << "\n"; if (cfg.cor_fin) f << "pasillo_fin=plano\n"; }
        for (int n = 1; n < TM_NL; n++) { snprintf(b, sizeof b, "capa%d=%g,%g,%g\n", n, cfg.lz[n], cfg.lsz[n], cfg.ls[n]); f << b; }
        cfg_dirty = false; ensure_plat_scene(level); return true;
    }
    // Importar una imagen cualquiera: se convierte a PNG, se reduce si es enorme y se copia a la carpeta del juego.
    // Devuelve el nombre de fichero (vacio si falla).
    std::string import_image(const std::string& src, const std::string& destdir, int maxdim) {
        SDL_Surface* s0 = IMG_Load(src.c_str()); if (!s0) { msg = "No puedo leer la imagen"; return ""; }
        SDL_Surface* c = SDL_ConvertSurfaceFormat(s0, SDL_PIXELFORMAT_RGBA32, 0); SDL_FreeSurface(s0); if (!c) return "";
        int w = c->w, h = c->h; float k = std::min(1.0f, (float)maxdim / (float)std::max(w, h));
        if (k < 1.0f) { SDL_Surface* d = SDL_CreateRGBSurfaceWithFormat(0, std::max(1, (int)(w * k)), std::max(1, (int)(h * k)), 32, SDL_PIXELFORMAT_RGBA32);
                        SDL_BlitScaled(c, nullptr, d, nullptr); SDL_FreeSurface(c); c = d; }
        std::string stem = tm_fs::path(src).stem().string();
        for (auto& ch : stem) if (!(isalnum((unsigned char)ch) || ch == '_' || ch == '-')) ch = '_';
        if (stem.empty()) stem = "textura";
        std::string name = stem + ".png"; int n = 2;
        while (tm_fs::exists(destdir + "/" + name)) name = stem + "_" + std::to_string(n++) + ".png";
        tm_fs::create_directories(destdir);
        bool ok = IMG_SavePNG(c, (destdir + "/" + name).c_str()) == 0; SDL_FreeSurface(c);
        return ok ? name : "";
    }
    std::vector<std::string> bg_pngs() const {
        std::vector<std::string> v;
        if (tm_fs::exists(bg_dir())) for (auto& e : tm_fs::directory_iterator(bg_dir())) if (e.path().extension() == ".png") v.push_back(e.path().filename().string());
        std::sort(v.begin(), v.end()); return v;
    }
    std::string unique_tile_name(const std::string& base) const {
        std::string n = base; int k = 2; while (find_tile(n)) n = base + "_" + std::to_string(k++); return n;
    }

    // ------------------------------------------------------------------- deshacer
    void push_undo() {
        Snap s; s.W = W; s.H = H; for (int l = 0; l < TM_NL; l++) s.L[l] = L[l];
        undo.push_back(std::move(s)); if (undo.size() > 80) undo.erase(undo.begin()); redo.clear();
    }
    void do_undo() {
        if (undo.empty()) return;
        Snap c; c.W = W; c.H = H; for (int l = 0; l < TM_NL; l++) c.L[l] = L[l]; redo.push_back(std::move(c));
        Snap& s = undo.back(); W = s.W; H = s.H; for (int l = 0; l < TM_NL; l++) L[l] = s.L[l]; undo.pop_back(); map_dirty = true;
    }
    void do_redo() {
        if (redo.empty()) return;
        Snap c; c.W = W; c.H = H; for (int l = 0; l < TM_NL; l++) c.L[l] = L[l]; undo.push_back(std::move(c));
        Snap& s = redo.back(); W = s.W; H = s.H; for (int l = 0; l < TM_NL; l++) L[l] = s.L[l]; redo.pop_back(); map_dirty = true;
    }

    // ------------------------------------------------------------------ iniciar
    void init(const std::string& default_dir) {
        if (inited) return; inited = true;
        const char* env = getenv("PLATAFORMAS_DIR");
        game_dir = env ? env : default_dir;
        snprintf(dir_buf, sizeof dir_buf, "%s", game_dir.c_str());
        reload_all(); load_pcfg();
    }
    void reload_all() {
        if (!dir_ok()) { msg = "Este proyecto aun no tiene plataformas (Assets/Tiles y Assets/Levels)"; tiles.clear(); alloc(0, 0); return; }
        load_tileset();
        auto names = level_names();
        if (!names.empty()) { std::string n = (std::find(names.begin(), names.end(), level) != names.end()) ? level : names[0]; load_level(n); }
        else alloc(0, 0);
    }

    // -------------------------------------------------------------------- probar
    void probar() {
        if (!dir_ok()) return;
        if (set_dirty) save_tileset();
        if (map_dirty) save_level();
        if (cfg_dirty) save_cfg();
        if (pcfg_dirty) { pcfg.save(cfg_file()); pcfg_dirty = false; }
        ensure_plat_scene(level);
        if (on_play) { on_play(level); msg = "Compilando y ejecutando " + level + "... (mira la Consola)"; }
        else msg = "Esta version del editor no puede ejecutar el juego";
    }

    // =========================================================================
    //  VENTANA: el mapa
    // =========================================================================
    static ImU32 marker_color(const std::string& m) {
        if (m == "inicio") return IM_COL32(250, 220, 40, 255);
        if (m == "met" || m == "enemigo1") return IM_COL32(70, 220, 90, 255);
        if (m == "kaka" || m == "enemigo2") return IM_COL32(150, 90, 30, 255);
        return IM_COL32(240, 240, 245, 255);
    }
    static const char* marker_letter(const std::string& m) {
        if (m == "inicio") return "I"; if (m == "met" || m == "enemigo1") return "1"; if (m == "kaka" || m == "enemigo2") return "2"; return "C";
    }
    int paint_val() const { return (cfg.corridor && cur_layer == 1) ? brush_h : sel_tile; }
    void draw_height(ImDrawList* dl, int v, ImVec2 a, ImVec2 b) {      // pasillo: la altura de una columna, como numero sobre un tono mas claro cuanto mas alta
        int g = std::min(255, 70 + v * 28); dl->AddRectFilled(a, b, IM_COL32(g, g * 0.7f, 40, 140));
        if (b.x - a.x >= 12) { char t[8]; snprintf(t, sizeof t, "%d", v); dl->AddText(ImVec2(a.x + 3, a.y + 1), IM_COL32(255, 255, 255, 240), t); }
    }
    void draw_cell(ImDrawList* dl, int v, ImVec2 a, ImVec2 b, ImU32 tint) {
        if (v < 1 || v > (int)tiles.size()) { dl->AddRectFilled(a, b, IM_COL32(200, 0, 200, 160)); return; }
        const TmTile& t = tiles[v - 1];
        if (!t.marker.empty()) {
            dl->AddRectFilled(a, b, (marker_color(t.marker) & 0x00FFFFFF) | 0x80000000);
            dl->AddRect(a, b, marker_color(t.marker), 0, 0, 2.0f);
            float fs = ImGui::GetFontSize(); ImVec2 c((a.x + b.x) * 0.5f - fs * 0.25f, (a.y + b.y) * 0.5f - fs * 0.5f);
            dl->AddText(c, IM_COL32(20, 20, 30, 255), marker_letter(t.marker)); return;
        }
        H2Img* im = img_of(t.tex);
        if (t.liq) {                                                       // liquido: se pinta translucido con una ola arriba
            ImU32 al = (ImU32)std::min(255, std::max(60, t.alpha + 40)); ImU32 tt = (tint & 0x00FFFFFF) | (((tint >> 24) * al / 255) << 24);
            if (im) dl->AddImage((ImTextureID)(intptr_t)im->tex, a, b, ImVec2(0, 0), ImVec2(1, 1), tt);
            else dl->AddRectFilled(a, b, IM_COL32(60, 120, 200, 140));
            dl->AddLine(ImVec2(a.x, a.y + 2), ImVec2(b.x, a.y + 2), IM_COL32(255, 255, 255, 150), 2.0f);
        } else if (im) dl->AddImage((ImTextureID)(intptr_t)im->tex, a, b, ImVec2(0, 0), ImVec2(1, 1), tint);
        else if (!t.liq) dl->AddRectFilled(a, b, IM_COL32(120, 120, 130, 255));
        float s = b.x - a.x;
        if (!t.top.empty() && s >= 20) {                                   // tapa: franja arriba
            H2Img* ti = img_of(t.top);
            if (ti) dl->AddImage((ImTextureID)(intptr_t)ti->tex, a, ImVec2(b.x, a.y + s * 0.18f), ImVec2(0, 0), ImVec2(1, 1), tint);
        }
        if (t.dmg > 0) dl->AddRect(a, b, IM_COL32(255, 60, 50, 230), 0, 0, 2.0f);
        if (t.oneway && s >= 20) dl->AddLine(ImVec2(a.x, a.y + 2), ImVec2(b.x, a.y + 2), IM_COL32(120, 255, 255, 255), 3.0f);
        if (t.axis && s >= 20) {                                           // flechas: se mueve
            ImVec2 m((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f); float d = s * 0.28f;
            if (t.axis == 1) { dl->AddTriangleFilled(ImVec2(m.x - d - 4, m.y), ImVec2(m.x - d + 6, m.y - 6), ImVec2(m.x - d + 6, m.y + 6), IM_COL32(255, 200, 40, 255));
                               dl->AddTriangleFilled(ImVec2(m.x + d + 4, m.y), ImVec2(m.x + d - 6, m.y - 6), ImVec2(m.x + d - 6, m.y + 6), IM_COL32(255, 200, 40, 255)); }
            else { dl->AddTriangleFilled(ImVec2(m.x, m.y - d - 4), ImVec2(m.x - 6, m.y - d + 6), ImVec2(m.x + 6, m.y - d + 6), IM_COL32(255, 200, 40, 255));
                   dl->AddTriangleFilled(ImVec2(m.x, m.y + d + 4), ImVec2(m.x - 6, m.y + d - 6), ImVec2(m.x + 6, m.y + d - 6), IM_COL32(255, 200, 40, 255)); }
        }
        if (t.ramp && s >= 12) {                                            // rampa: se tapa la mitad de arriba
            ImU32 bgc = IM_COL32(40, 46, 62, 255);
            if (t.ramp == 1) dl->AddTriangleFilled(a, ImVec2(b.x, a.y), ImVec2(a.x, b.y), bgc);
            else             dl->AddTriangleFilled(ImVec2(b.x, a.y), a, ImVec2(b.x, b.y), bgc);
            if (t.ramp == 1) dl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y), IM_COL32(255, 255, 255, 200), 2.0f);
            else             dl->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), IM_COL32(255, 255, 255, 200), 2.0f);
        }
        if ((t.door || t.btn || t.tgt) && s >= 20) {                       // puerta, boton, diana: letra y canal
            char lb[8]; snprintf(lb, sizeof lb, "%c%d", t.door ? 'P' : t.btn ? 'B' : 'D', t.door ? t.door : t.btn ? t.btn : t.tgt);
            dl->AddRectFilled(ImVec2(b.x - 24, b.y - 16), b, IM_COL32(0, 0, 0, 170)); dl->AddText(ImVec2(b.x - 22, b.y - 15), IM_COL32(255, 230, 120, 255), lb);
        }
        if (t.slip > 0.0f && s >= 20) dl->AddRectFilled(a, ImVec2(b.x, a.y + 4), IM_COL32(170, 230, 255, 255));
        if (t.conv != 0.0f && s >= 20) dl->AddText(ImVec2(a.x + 3, a.y + 1), IM_COL32(255, 220, 60, 255), t.conv > 0 ? ">>" : "<<");
        if (t.bounce > 0.0f && s >= 20) dl->AddText(ImVec2(a.x + 3, a.y + 1), IM_COL32(120, 255, 120, 255), "^");
    }

    void flood(int l, int c, int r, int to) {
        int from = at(l, c, r); if (from == to || c < 0 || r < 0 || c >= W || r >= H) return;
        std::vector<std::pair<int, int>> st; st.push_back({ c, r });
        while (!st.empty()) {
            auto [x, y] = st.back(); st.pop_back();
            if (x < 0 || y < 0 || x >= W || y >= H || L[l][y * W + x] != from) continue;
            L[l][y * W + x] = (uint8_t)to; map_dirty = true;
            st.push_back({ x + 1, y }); st.push_back({ x - 1, y }); st.push_back({ x, y + 1 }); st.push_back({ x, y - 1 });
        }
    }
    void line_paint(int c0, int r0, int c1, int r1, int v) {                 // sin huecos al arrastrar rapido
        int dx = std::abs(c1 - c0), dy = std::abs(r1 - r0), sx = c0 < c1 ? 1 : -1, sy = r0 < r1 ? 1 : -1, err = dx - dy;
        for (;;) {
            put(cur_layer, c0, r0, v); if (c0 == c1 && r0 == r1) break;
            int e2 = 2 * err; if (e2 > -dy) { err -= dy; c0 += sx; } if (e2 < dx) { err += dx; r0 += sy; }
        }
    }

    void draw_map() {
        ImGui::Begin(TM_WIN_MAP);
        if (!dir_ok()) {
            ImGui::TextWrapped("Este proyecto todavia no tiene nada de plataformas (Assets/Tiles y Assets/Levels).");
            ImGui::Spacing();
            if (ImGui::Button("Crear el juego de ejemplo (heroe, enemigos, tiles y un nivel)")) crear_ejemplo();
            if (ImGui::Button("Solo crear las carpetas")) {
                std::error_code ec; tm_fs::create_directories(tiles_dir(), ec); tm_fs::create_directories(levels_dir(), ec); tm_fs::create_directories(bg_dir(), ec);
                if (!tm_fs::exists(tiles_dir() + "/tileset.def")) { std::ofstream o(tiles_dir() + "/tileset.def"); o << "# Tileset: un tile por linea. El indice de cada tile es su posicion (1, 2, 3...). 0 = vacio.\n"; }
                reload_all();
            }
            ImGui::TextDisabled("%s", msg.c_str()); ImGui::End(); return;
        }
        // ---- barra de arriba: nivel, guardar, probar ----
        auto names = level_names();
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##nivel", (level + (map_dirty ? " *" : "")).c_str())) {
            for (auto& n : names) if (ImGui::Selectable(n.c_str(), n == level)) { if (map_dirty) save_level(); load_level(n); }
            ImGui::EndCombo();
        }
        ImGui::SameLine(0, 8); ImGui::TextDisabled("Plano");
        for (int pl = 0; pl < 3; pl++) {
            ImGui::SameLine(0, 2);
            bool ex = plane_exists(pl) || pl == plane, on = (plane == pl);
            if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.55f, 0.95f, 1));
            char lb[16]; snprintf(lb, sizeof lb, ex ? "%d##pl" : "+%d##pl", pl);
            if (ImGui::Button(lb) && !on) set_plane(pl);
            if (on) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(pl == 0 ? "Plano 0: el de delante (donde empieza el heroe)" : ex ? "Plano %d: detras del anterior" : "Crear el plano %d (vacio, del mismo tamano) detras del anterior", pl);
        }
        ImGui::SameLine(); if (ImGui::Button("Nuevo")) show_new = true;
        ImGui::SameLine(); if (ImGui::Button("Guardar")) { save_level(); if (set_dirty) save_tileset(); }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.25f, 1.0f));
        if (ImGui::Button("Probar nivel")) probar();
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Guarda y ejecuta el nivel en el juego (compila el .prg)");
        ImGui::SameLine(0, 18); ImGui::TextDisabled("%dx%d", W, H);
        ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::DragInt("##rw", &rs_w, 0.3f, 8, 500);
        ImGui::SameLine(); ImGui::SetNextItemWidth(70); ImGui::DragInt("##rh", &rs_h, 0.3f, 6, 200);
        ImGui::SameLine(); if (ImGui::Button("Redimensionar")) resize_level(rs_w, rs_h);

        // ---- herramientas y capas ----
        const char* tn[] = { "Lapiz", "Goma", "Rectangulo", "Relleno", "Cuentagotas", "Enlazar" };
        const char* tk[] = { "B", "E", "R", "F", "I", "K" };
        for (int i = 0; i < 6; i++) {
            bool on = (tool == i);
            if (on) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.949f, 0.686f, 0.259f, 1)); ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.09f, 0.1f, 0.12f, 1)); }
            if (ImGui::Button(tn[i])) tool = i;
            if (on) ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(i == 5 ? "Enlazar  (tecla K)\nClic en una puerta, boton, diana o portal para ponerle nombre y elegir con que se enlaza." : "%s  (tecla %s)\nBoton derecho = borrar. Alt+clic = cuentagotas.", tn[i], tk[i]);
            ImGui::SameLine();
        }
        ImGui::SameLine(0, 16);
        // de la mas lejana a la mas cercana; el numero (1..6) es la tecla
        static const char* ln_n[TM_NL] = { "Plano jugable", "Fondo", "Primer plano", "Fondo 2", "Fondo 3", "Primer plano 2", "Exterior (fachada)", "Decorado" };
        static const char* ln_p[TM_NL] = { "Suelo", "Altura", "Marcadores", "Piso 2", "Piso 3", "Piso 4+", "Exterior (techo)", "Decorado" };            // pasillo estilo Crash
        const char* const* ln = cfg.corridor ? ln_p : ln_n;
        static const int  ord[TM_NL] = { 4, 3, 1, 0, 2, 5, 6, 7 };
        for (int oi = 0; oi < TM_NL; oi++) {
            int i = ord[oi]; 
            bool on = (cur_layer == i);
            if (on) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.55f, 0.95f, 1)); }
            if (ImGui::Button(ln[i])) cur_layer = i;
            if (on) ImGui::PopStyleColor();
            ImGui::SameLine(0, 2);
            ImGui::Checkbox((std::string("##v") + std::to_string(i)).c_str(), &vis[i]);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ver / ocultar la capa");
            ImGui::SameLine(0, 10);
        }
        {   bool cc = cfg.corridor != 0;
            if (ImGui::Checkbox("Pasillo", &cc)) { cfg.corridor = cc; cfg_dirty = true; cur_layer = 0; ensure_plat_scene(level); }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Nivel en profundidad estilo Crash Bandicoot: el mapa se ve desde arriba.\nSuelo = tile de cada celda (vacio = hueco). Altura = bloques que sube la columna. Marcadores = inicio y puntos de control.\nPiso 2, 3 y 4+ = tile del segundo, tercer y cuarto bloque (y siguientes) del muro; si esta vacio sigue el de abajo.\nEl final del pasillo esta en la fila de arriba.");
            ImGui::SameLine(0, 6);
            if (cfg.corridor && cur_layer == 1) { ImGui::SetNextItemWidth(90); ImGui::SliderInt("##alt", &brush_h, 1, 9, "alto %d"); ImGui::SameLine(0, 6);
                if (ImGui::Button("Muros laterales")) {          // las dos columnas de los bordes suben a la altura elegida (y se les pone suelo si no tenian)
                    push_undo();
                    for (int r = 0; r < H; r++) for (int c : { 0, W - 1 }) { put(1, c, r, brush_h); if (!at(0, c, r)) put(0, c, r, sel_tile > 0 ? sel_tile : 1); }
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Levanta un muro del alto elegido en los dos lados del pasillo.\nPara muros en otros sitios: pinta la capa Altura (rectangulo con R, o relleno con F)."); ImGui::SameLine(0, 6); } }
        ImGui::SetNextItemWidth(110); ImGui::SliderFloat("##zoom", &zoom, 10.0f, 110.0f, "zoom %.0f");
        ImGui::SameLine(); if (ImGui::Button("Ajustar")) fit_pending = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Encuadrar el nivel entero en la ventana");

        // ---- el lienzo ----
        ImVec2 avail = ImGui::GetContentRegionAvail(); avail.y -= ImGui::GetTextLineHeightWithSpacing() + 2;
        if (avail.y < 60) avail.y = 60;
        if (fit_pending && W > 0 && H > 0 && ++fit_wait >= 4) {                                      // encuadrar el nivel entero
            zoom = std::max(10.0f, std::min(110.0f, std::min((avail.x - 16) / W, (avail.y - 16) / H)));
            pan = ImVec2((avail.x - W * zoom) * 0.5f, (avail.y - H * zoom) * 0.5f); fit_pending = false; fit_wait = 0;
        }
        ImGui::InvisibleButton("##cv", avail, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        ImVec2 p0 = ImGui::GetItemRectMin(), p1 = ImGui::GetItemRectMax();
        bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
        ImGuiIO& io = ImGui::GetIO(); ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(p0, p1, true);
        dl->AddRectFilled(p0, p1, IM_COL32(24, 27, 34, 255));

        if (hov && io.MouseWheel != 0.0f) {                                        // zoom hacia el cursor
            float old = zoom; zoom = std::min(110.0f, std::max(10.0f, zoom * (io.MouseWheel > 0 ? 1.12f : 1.0f / 1.12f)));
            ImVec2 m(io.MousePos.x - p0.x, io.MousePos.y - p0.y);
            pan.x = m.x - (m.x - pan.x) * zoom / old; pan.y = m.y - (m.y - pan.y) * zoom / old;
        }
        if ((hov || act) && (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || (ImGui::IsKeyDown(ImGuiKey_Space) && ImGui::IsMouseDown(ImGuiMouseButton_Left)))) {
            pan.x += io.MouseDelta.x; pan.y += io.MouseDelta.y;
        }
        auto cell_a = [&](int c, int r) { return ImVec2(p0.x + pan.x + c * zoom, p0.y + pan.y + r * zoom); };
        int c_min = std::max(0, (int)std::floor((-pan.x) / zoom)), c_max = std::min(W - 1, (int)std::floor((p1.x - p0.x - pan.x) / zoom));
        int r_min = std::max(0, (int)std::floor((-pan.y) / zoom)), r_max = std::min(H - 1, (int)std::floor((p1.y - p0.y - pan.y) / zoom));
        // marco del nivel
        dl->AddRectFilled(cell_a(0, 0), cell_a(W, H), IM_COL32(40, 46, 62, 255));
        static const int orden[TM_NL] = { 7, 4, 3, 1, 0, 2, 5, 6 };                     // de lo mas lejano a lo mas cercano
        for (int oi = 0; oi < TM_NL; oi++) {
            int l = orden[oi]; if (!vis[l]) continue;
            ImU32 tint = (l == cur_layer) ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 105);
            for (int r = r_min; r <= r_max; r++)
                for (int c = c_min; c <= c_max; c++) {
                    int v = L[l][r * W + c]; if (!v) continue;
                    if (cfg.corridor && l == 1) draw_height(dl, v, cell_a(c, r), cell_a(c + 1, r + 1)); else draw_cell(dl, v, cell_a(c, r), cell_a(c + 1, r + 1), tint);
                }
        }
        {   // enlaces: nombre de cada objeto, una linea del boton/diana a las puertas que abre, y la casilla elegida
            auto ctr = [&](int c, int r) { return ImVec2(p0.x + pan.x + (c + 0.5f) * zoom, p0.y + pan.y + (r + 0.5f) * zoom); };
            for (auto& k : links) {
                if (k.plane != plane || (k.name.empty() && k.tgt.empty())) continue;
                int kd = link_kind(k.x, k.y); if (kd == 0) continue;
                if (kd == 2 || kd == 3) {
                    size_t a = 0; std::string t = k.tgt + ",";
                    for (size_t b; (b = t.find(',', a)) != std::string::npos; a = b + 1) {
                        std::string nm = t.substr(a, b - a); if (nm.empty()) continue;
                        for (auto& d : links) if (d.plane == plane && d.name == nm && (link_kind(d.x, d.y) == 1 || link_kind(d.x, d.y) == 5)) dl->AddLine(ctr(k.x, k.y), ctr(d.x, d.y), IM_COL32(120, 255, 160, 200), 2.0f);
                    }
                }
                ImU32 col = (kd == 1 || kd == 5) ? IM_COL32(255, 200, 80, 255) : kd == 4 ? IM_COL32(190, 140, 255, 255) : IM_COL32(120, 255, 160, 255);
                ImVec2 a = cell_a(k.x, k.y); dl->AddRect(a, cell_a(k.x + 1, k.y + 1), col, 0, 0, 2.0f);
                if (zoom >= 22.0f && !k.name.empty()) { dl->AddRectFilled(ImVec2(a.x, a.y), ImVec2(a.x + ImGui::CalcTextSize(k.name.c_str()).x + 4, a.y + 14), IM_COL32(10, 12, 20, 200)); dl->AddText(ImVec2(a.x + 2, a.y), col, k.name.c_str()); }
            }
            if (tool == 5 && sel_c >= 0) dl->AddRect(cell_a(sel_c, sel_r), cell_a(sel_c + 1, sel_r + 1), IM_COL32(255, 255, 255, 255), 0, 0, 3.0f);
        }
        if (zoom >= 14.0f) {                                                       // rejilla
            for (int c = c_min; c <= c_max + 1; c++) dl->AddLine(cell_a(c, r_min), cell_a(c, r_max + 1), IM_COL32(255, 255, 255, 22));
            for (int r = r_min; r <= r_max + 1; r++) dl->AddLine(cell_a(c_min, r), cell_a(c_max + 1, r), IM_COL32(255, 255, 255, 22));
        }
        dl->AddRect(cell_a(0, 0), cell_a(W, H), IM_COL32(240, 175, 66, 255), 0, 0, 2.0f);

        // celda bajo el raton
        int mc = (int)std::floor((io.MousePos.x - p0.x - pan.x) / zoom), mr = (int)std::floor((io.MousePos.y - p0.y - pan.y) / zoom);
        bool in_map = hov && mc >= 0 && mr >= 0 && mc < W && mr < H;
        bool alt = io.KeyAlt, panning = ImGui::IsKeyDown(ImGuiKey_Space) || ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        int brush = (tool == 1) ? 0 : paint_val();
        if (in_map && !panning) {
            if (tool < 4) { if (cfg.corridor && cur_layer == 1) draw_height(dl, std::max(brush, 0), cell_a(mc, mr), cell_a(mc + 1, mr + 1)); else draw_cell(dl, std::max(brush, 0), cell_a(mc, mr), cell_a(mc + 1, mr + 1), IM_COL32(255, 255, 255, 120)); }
            dl->AddRect(cell_a(mc, mr), cell_a(mc + 1, mr + 1), IM_COL32(255, 255, 255, 230), 0, 0, 2.0f);
        }
        // ---- pintar ----
        bool lmb = ImGui::IsMouseDown(ImGuiMouseButton_Left), rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        bool lclick = ImGui::IsMouseClicked(ImGuiMouseButton_Left), rclick = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        if (in_map && !panning && tool == 5) { if (lclick) { sel_c = mc; sel_r = mr; } }
        else if (in_map && !panning && (lclick || rclick)) {
            if ((lclick && (alt || tool == 4))) {                                   // cuentagotas
                int v = at(cur_layer, mc, mr); if (v > 0) { if (cfg.corridor && cur_layer == 1) brush_h = v; else sel_tile = v; } if (tool == 4) tool = 0;
            } else if (tool == 2) { rc0 = mc; rr0 = mr; stroke = true; rect_erase = rclick; push_undo(); }
            else if (tool == 3) { push_undo(); flood(cur_layer, mc, mr, rclick ? 0 : paint_val()); }
            else { push_undo(); stroke = true; lc = mc; lr = mr; rect_erase = rclick;
                   line_paint(mc, mr, mc, mr, (rclick || tool == 1) ? 0 : paint_val()); }
        }
        if (stroke && tool != 2 && (lmb || rmb) && act && mc >= -1000) {
            if (mc != lc || mr != lr) { line_paint(lc, lr, mc, mr, (rect_erase || tool == 1) ? 0 : paint_val()); lc = mc; lr = mr; }
        }
        if (stroke && tool == 2) {
            int a0 = std::min(rc0, mc), a1 = std::max(rc0, mc), b0 = std::min(rr0, mr), b1 = std::max(rr0, mr);
            dl->AddRect(cell_a(a0, b0), cell_a(a1 + 1, b1 + 1), IM_COL32(255, 220, 80, 255), 0, 0, 2.0f);
            if (!lmb && !rmb) {
                for (int r = b0; r <= b1; r++) for (int c = a0; c <= a1; c++) put(cur_layer, c, r, rect_erase ? 0 : paint_val());
                stroke = false;
            }
        }
        if (!lmb && !rmb) stroke = false;
        dl->PopClipRect();

        // ---- atajos (con la ventana enfocada) ----
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput) {
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) do_undo();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) do_redo();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) { save_level(); if (set_dirty) save_tileset(); }
            if (!io.KeyCtrl) {
                if (ImGui::IsKeyPressed(ImGuiKey_B)) tool = 0; if (ImGui::IsKeyPressed(ImGuiKey_E)) tool = 1;
                if (ImGui::IsKeyPressed(ImGuiKey_R)) tool = 2; if (ImGui::IsKeyPressed(ImGuiKey_F)) tool = 3;
                if (ImGui::IsKeyPressed(ImGuiKey_I)) tool = 4; if (ImGui::IsKeyPressed(ImGuiKey_K)) tool = 5;
                if (ImGui::IsKeyPressed(ImGuiKey_7)) cur_layer = 6; if (ImGui::IsKeyPressed(ImGuiKey_1)) cur_layer = 0; if (ImGui::IsKeyPressed(ImGuiKey_2)) cur_layer = 1; if (ImGui::IsKeyPressed(ImGuiKey_3)) cur_layer = 2;
                if (ImGui::IsKeyPressed(ImGuiKey_4)) cur_layer = 3; if (ImGui::IsKeyPressed(ImGuiKey_5)) cur_layer = 4; if (ImGui::IsKeyPressed(ImGuiKey_6)) cur_layer = 5;
            }
        }
        // ---- pie: que hay bajo el cursor ----
        if (in_map) {
            int v = at(cur_layer, mc, mr);
            ImGui::Text("(%d, %d)  %s   |  %s", mc, H - 1 - mr, v > 0 && v <= (int)tiles.size() ? tiles[v - 1].name.c_str() : "vacio", msg.c_str());
        } else ImGui::TextDisabled("%s", msg.c_str());

        // ---- ventana "nivel nuevo" ----
        if (show_new) { ImGui::OpenPopup("Nivel nuevo"); show_new = false; }
        if (ImGui::BeginPopupModal("Nivel nuevo", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Nombre", new_name, sizeof new_name);
            ImGui::DragInt("Ancho (bloques)", &new_w, 0.5f, 8, 500); ImGui::DragInt("Alto (bloques)", &new_h, 0.5f, 6, 200);
            ImGui::Checkbox("Con suelo, paredes e inicio del heroe", &new_floor);
            ImGui::Checkbox("Pasillo en profundidad (estilo Crash)", &new_corr);
            if (ImGui::Button("Crear")) {
                std::string nm = new_name; for (auto& ch : nm) if (!(isalnum((unsigned char)ch) || ch == '_' || ch == '-')) ch = '_';
                if (!nm.empty() && new_w * new_h <= 79999) { if (map_dirty) save_level(); new_level(nm, new_w, new_h, new_floor, new_corr); ImGui::CloseCurrentPopup(); }
                else msg = "Nombre o tamano no validos";
            }
            ImGui::SameLine(); if (ImGui::Button("Cancelar")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::End();
    }

    // =========================================================================
    //  VENTANA: la paleta de tiles
    // =========================================================================
    // nombres de las puertas (casillas con puerta que tienen nombre) de un nivel: para elegir destino de un portal
    std::vector<std::string> door_names_of(const std::string& lvl_name) const {
        std::vector<std::string> r; G3DLvl* l = g3d_lvl_load(lvl_path(lvl_name).c_str()); if (!l) return r;
        for (auto& k : parse_links(g3d_lvl_links(l))) if (k.plane == 0 && !k.name.empty() && link_kind_in(l, k.x, k.y) == 4) r.push_back(k.name);
        g3d_lvl_free(l); return r;
    }
    int link_kind_in(G3DLvl* l, int c, int r) const {                  // lo mismo que link_kind pero leyendo otro nivel del disco
        for (int ly : { 0, 2 }) {
            uint8_t* d = g3d_lvl_layer(l, 0, ly); if (!d) continue; int w = g3d_lvl_width(l, 0), h = g3d_lvl_height(l, 0); if (c < 0 || r < 0 || c >= w || r >= h) continue;
            int v = d[r * w + c]; if (v < 1 || v > (int)tiles.size()) continue; const TmTile& t = tiles[v - 1];
            if (t.door > 0) return 1; if (t.btn > 0) return 2; if (t.tgt > 0) return 3; if (t.portal == 2) return 4; if (t.axis > 0) return 5;
        }
        return 0;
    }
    static std::string clean_name(std::string s) { std::string o; for (char ch : s) if (ch != ';' && ch != ',' && ch != '/' && ch != '\n' && ch != '\r') o += ch; return o; }
    // Ventana del objeto de la casilla elegida con la herramienta Enlazar: nombre y a que se enlaza (todo se elige de listas)
    void draw_links() {
        if (!ImGui::Begin("Enlaces de la casilla")) { ImGui::End(); return; }
        if (tool != 5) ImGui::TextDisabled("Elige la herramienta 'Enlazar' (K) y haz clic en una puerta, boton, diana o portal.");
        int kd = (sel_c >= 0) ? link_kind(sel_c, sel_r) : 0;
        if (sel_c < 0) ImGui::TextWrapped("Ninguna casilla elegida.");
        else if (kd == 0) ImGui::TextWrapped("La casilla (%d, %d) no tiene nada enlazable. Se enlazan las puertas, los botones, las dianas y los portales a otro nivel.", sel_c, sel_r);
        else {
            static const char* kn[] = { "", "Puerta", "Boton", "Diana", "Portal a otro nivel", "Ascensor / plataforma" };
            ImGui::Text("%s en (%d, %d)", kn[kd], sel_c, sel_r);
            TmLink* k = link_at(sel_c, sel_r, true); bool ch = false;
            char nb[64]; snprintf(nb, sizeof nb, "%s", k->name.c_str());
            if (ImGui::InputText("Nombre", nb, sizeof nb)) { k->name = clean_name(nb); ch = true; }
            ImGui::SameLine(); if (ImGui::SmallButton("Auto")) { k->name = auto_name(kd, sel_c, sel_r); ch = true; }
            if (kd == 1 || kd == 5) {
                ImGui::SeparatorText(kd == 5 ? "Lo activan (sube mientras estan activos)" : "La abren");
                bool any = false;
                for (auto& o : links) if (o.plane == plane && !o.name.empty()) {
                    int ok = link_kind(o.x, o.y); if (ok != 2 && ok != 3) continue;
                    std::string t = "," + o.tgt + ","; if (!k->name.empty() && t.find("," + k->name + ",") != std::string::npos) { ImGui::BulletText("%s (%d, %d)", o.name.c_str(), o.x, o.y); any = true; }
                }
                if (k->name.empty()) ImGui::TextDisabled("Ponle un nombre para poder enlazarla.");
                else if (!any) ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "Ningun boton ni diana la abre todavia.");
            } else if (kd == 2 || kd == 3) {
                ImGui::SeparatorText("Abre / activa");
                std::string cur = "," + k->tgt + ","; bool any = false;
                for (auto& o : links) if (o.plane == plane && !o.name.empty() && (link_kind(o.x, o.y) == 1 || link_kind(o.x, o.y) == 5)) {
                    any = true; bool on = cur.find("," + o.name + ",") != std::string::npos;
                    std::string lab = o.name + "  (" + std::to_string(o.x) + ", " + std::to_string(o.y) + ")";
                    if (ImGui::Checkbox(lab.c_str(), &on)) {
                        std::string out; std::string t = k->tgt + ","; size_t a = 0;
                        for (size_t b; (b = t.find(',', a)) != std::string::npos; a = b + 1) { std::string nm = t.substr(a, b - a); if (!nm.empty() && nm != o.name) out += (out.empty() ? "" : ",") + nm; }
                        if (on) out += (out.empty() ? "" : ",") + o.name;
                        k->tgt = out; ch = true;
                    }
                }
                if (!any) ImGui::TextDisabled("No hay puertas ni ascensores con nombre. Elige uno y ponle nombre.");
                else if (k->tgt.empty()) ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "No abre ninguna puerta: marca las que quieras.");
                if (k->name.empty() && !k->tgt.empty()) { k->name = auto_name(kd, sel_c, sel_r); ch = true; }
            } else if (kd == 4) {
                ImGui::SeparatorText("Destino");
                std::string sc = k->tgt, dr; size_t sl = sc.find('/'); if (sl != std::string::npos) { dr = sc.substr(sl + 1); sc = sc.substr(0, sl); }
                if (ImGui::BeginCombo("Nivel", sc.empty() ? "(el del tile)" : sc.c_str())) {
                    if (ImGui::Selectable("(el del tile)", sc.empty())) { k->tgt.clear(); ch = true; }
                    for (auto& n : level_names()) if (ImGui::Selectable(n.c_str(), n == sc)) { k->tgt = n; ch = true; }
                    ImGui::EndCombo();
                }
                if (!sc.empty() && ImGui::BeginCombo("Puerta de llegada", dr.empty() ? "(inicio del nivel)" : dr.c_str())) {
                    if (ImGui::Selectable("(inicio del nivel)", dr.empty())) { k->tgt = sc; ch = true; }
                    for (auto& n : door_names_of(sc)) if (ImGui::Selectable(n.c_str(), n == dr)) { k->tgt = sc + "/" + n; ch = true; }
                    ImGui::EndCombo();
                }
                if (!sc.empty() && door_names_of(sc).empty()) ImGui::TextDisabled("Ese nivel no tiene puertas con nombre: abrelo y nombra la de llegada.");
            }
            if (ch) map_dirty = true;
            if (ImGui::Button("Quitar nombre y enlaces")) { k->name.clear(); k->tgt.clear(); map_dirty = true; }
        }
        // avisos: cosas a medias en el nivel
        std::vector<std::string> warn;
        for (auto& o : links) if (o.plane == plane) {
            int ok = link_kind(o.x, o.y);
            if (ok == 0 && (!o.name.empty() || !o.tgt.empty())) warn.push_back("'" + o.name + "' en (" + std::to_string(o.x) + ", " + std::to_string(o.y) + ") ya no esta sobre ningun objeto");
            if ((ok == 2 || ok == 3) && o.tgt.empty() && !o.name.empty()) warn.push_back("'" + o.name + "' no abre ninguna puerta");
            if (ok == 2 || ok == 3) { std::string t = o.tgt + ","; size_t a = 0; for (size_t b; (b = t.find(',', a)) != std::string::npos; a = b + 1) { std::string nm = t.substr(a, b - a); bool f = nm.empty(); for (auto& d : links) if (d.plane == plane && d.name == nm && (link_kind(d.x, d.y) == 1 || link_kind(d.x, d.y) == 5)) f = true; if (!f) warn.push_back("'" + o.name + "' apunta a la puerta '" + nm + "', que no existe"); } }
        }
        if (!warn.empty()) { ImGui::SeparatorText("Avisos"); for (auto& w : warn) ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "%s", w.c_str()); }
        ImGui::End();
    }
    void draw_tileset() {
        ImGui::Begin(TM_WIN_SET);
        if (!dir_ok()) { ImGui::TextDisabled("Sin carpeta de juego."); ImGui::End(); return; }
        float cell = 56.0f; float w = ImGui::GetContentRegionAvail().x;
        int per = std::max(1, (int)(w / (cell + 6)));
        for (size_t i = 0; i < tiles.size(); i++) {
            const TmTile& t = tiles[i]; int id = (int)i + 1;
            ImGui::PushID((int)i);
            ImVec2 a = ImGui::GetCursorScreenPos(); ImVec2 b(a.x + cell, a.y + cell);
            if (ImGui::InvisibleButton("##t", ImVec2(cell, cell))) sel_tile = id;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            draw_cell(dl, id, a, b, IM_COL32(255, 255, 255, 255));
            if (sel_tile == id) dl->AddRect(ImVec2(a.x - 2, a.y - 2), ImVec2(b.x + 2, b.y + 2), IM_COL32(240, 175, 66, 255), 0, 0, 3.0f);
            else if (ImGui::IsItemHovered()) dl->AddRect(a, b, IM_COL32(255, 255, 255, 180), 0, 0, 2.0f);
            dl->AddText(ImVec2(a.x + 3, b.y - ImGui::GetFontSize() - 1), IM_COL32(255, 255, 255, 255), std::to_string(id).c_str());
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%d  %s", id, t.name.c_str());
            ImGui::PopID();
            if ((int)((i + 1) % per) != 0 && i + 1 < tiles.size()) ImGui::SameLine(0, 6);
        }
        ImGui::Separator();
        // anadir un tile con una textura nueva de la carpeta tiles/
        static int add_sel = 0; auto pngs = tile_pngs();
        std::vector<std::string> libres;
        for (auto& p : pngs) { bool usada = false; for (auto& t : tiles) if (t.tex == p || t.top == p) usada = true; if (!usada) libres.push_back(p); }
        ImGui::TextDisabled("Texturas sin usar en tiles/ (%zu)", libres.size());
        if (!libres.empty()) {
            if (add_sel >= (int)libres.size()) add_sel = 0;
            ImGui::SetNextItemWidth(-90);
            if (ImGui::BeginCombo("##libre", libres[add_sel].c_str())) {
                for (int i = 0; i < (int)libres.size(); i++) if (ImGui::Selectable(libres[i].c_str(), i == add_sel)) add_sel = i;
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Tile")) {
                TmTile t; t.tex = libres[add_sel]; t.name = tm_fs::path(t.tex).stem().string(); tiles.push_back(t);
                sel_tile = (int)tiles.size(); set_dirty = true; save_tileset();
            }
        }
        if (!fb_ready) {
            fb_tex.SetTitle("Importar textura (imagen cualquiera)"); fb_tex.SetTypeFilters({ ".png", ".jpg", ".jpeg", ".bmp", ".tga" });
            fb_bg.SetTitle("Importar fondo (imagen cualquiera)");    fb_bg.SetTypeFilters({ ".png", ".jpg", ".jpeg", ".bmp", ".tga" }); fb_ready = true;
        }
        if (ImGui::Button("Importar textura propia...")) { const char* hm = getenv("HOME"); fb_tex.SetPwd(hm ? hm : "/"); fb_tex.Open(); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Elige cualquier imagen (png, jpg...) de tu ordenador:\nse copia a tiles/ y se crea un tile nuevo con ella.");
        fb_tex.Display();
        if (fb_tex.HasSelected()) {
            std::string nm = import_image(fb_tex.GetSelected().string(), tiles_dir(), 512); fb_tex.ClearSelected();
            if (!nm.empty()) {
                TmTile t; t.tex = nm; t.name = unique_tile_name(tm_fs::path(nm).stem().string()); tiles.push_back(t);
                sel_tile = (int)tiles.size(); set_dirty = true; save_tileset(); msg = "Tile nuevo: " + t.name;
            }
        }
        if (ImGui::Button("Marcador...")) ImGui::OpenPopup("nuevo_marcador");
        if (ImGui::BeginPopup("nuevo_marcador")) {
            const char* ms[] = { "inicio", "enemigo1", "enemigo2", "check" };
            for (auto m : ms) if (ImGui::Selectable(m)) { if (!find_tile(m == std::string("check") ? "checkpoint" : m)) { TmTile t; t.name = (m == std::string("check")) ? "checkpoint" : m; t.marker = m; t.solid = 0; tiles.push_back(t); sel_tile = (int)tiles.size(); set_dirty = true; save_tileset(); } }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::SameLine(); ImGui::TextDisabled("(los tiles no se borran: los mapas guardan su numero)");
        ImGui::End();
    }

    // =========================================================================
    //  VENTANA: propiedades del tile elegido
    // =========================================================================
    void draw_props() {
        ImGui::Begin(TM_WIN_PROPS);
        if (!dir_ok() || sel_tile < 1 || sel_tile > (int)tiles.size()) { ImGui::TextDisabled("Elige un tile en la paleta."); ImGui::End(); return; }
        TmTile& t = tiles[sel_tile - 1]; bool ch = false;
        ImGui::Text("Tile %d", sel_tile); ImGui::Separator();
        char nb[64]; snprintf(nb, sizeof nb, "%s", t.name.c_str());
        if (ImGui::InputText("Nombre", nb, sizeof nb)) { t.name = nb; ch = true; }
        auto pngs = tile_pngs();
        auto combo_tex = [&](const char* label, std::string& val) {
            if (ImGui::BeginCombo(label, val.empty() ? "(ninguna)" : val.c_str())) {
                if (ImGui::Selectable("(ninguna)", val.empty())) { val.clear(); ch = true; }
                for (auto& p : pngs) if (ImGui::Selectable(p.c_str(), p == val)) { val = p; ch = true; }
                ImGui::EndCombo();
            }
        };
        if (t.marker.empty()) {
            combo_tex("Textura", t.tex); combo_tex("Tapa (arriba)", t.top);
            if (H2Img* im = img_of(t.tex)) ImGui::Image((ImTextureID)(intptr_t)im->tex, ImVec2(96, 96));
            ImGui::SeparatorText("Decorado");
            ch |= ImGui::DragFloat("Alto en Decorado (bloques, 0 = 1)", &t.oh, 0.1f, 0.0f, 30.0f, "%.1f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Si pintas este tile en la capa 'Decorado' de un pasillo, sale como una columna de tantos bloques de alto (arboles, torres, edificios...).");
            ImGui::SeparatorText("Comportamiento");
            bool so = t.solid != 0; if (ImGui::Checkbox("Solido", &so)) { t.solid = so; ch = true; }
            bool ow = t.oneway != 0; if (ImGui::Checkbox("Una via (se pisa desde arriba)", &ow)) { t.oneway = ow; ch = true; }
            ch |= ImGui::SliderInt("Dano", &t.dmg, 0, 8);
            ch |= ImGui::SliderFloat("Resbala (hielo)", &t.slip, 0.0f, 1.0f);
            ch |= ImGui::SliderFloat("Rebota (trampolin)", &t.bounce, 0.0f, 50.0f);
            ch |= ImGui::SliderFloat("Cinta (+ der / - izq)", &t.conv, -12.0f, 12.0f);
            ImGui::SeparatorText("Liquido");
            const char* liqs[] = { "No es liquido", "Agua", "Lava", "Acido", "Cieno" };
            int lq = t.liq; if (ImGui::Combo("Tipo", &lq, liqs, 5)) { if (lq && !t.liq) { t.alpha = 150; t.slow = 0.6f; t.solid = 0; t.swim = (lq == 1); } t.liq = lq; ch = true; }
            if (t.liq) {
                bool sw = t.swim != 0; if (ImGui::Checkbox("Se nada (mantener salto = subir)", &sw)) { t.swim = sw; ch = true; }
                ch |= ImGui::SliderFloat("Lentitud (1 = normal)", &t.slow, 0.1f, 1.0f);
                ch |= ImGui::SliderInt("Transparencia", &t.alpha, 40, 255);
                ImGui::TextDisabled("El dano de arriba tambien vale: lava y acido queman.");
            }
            ImGui::SeparatorText("Rampa");
            { const char* rs[] = { "No es rampa", "Sube hacia la derecha", "Sube hacia la izquierda" };
              ch |= ImGui::Combo("Inclinacion", &t.ramp, rs, 3); if (t.ramp) t.solid = 0; }
            ImGui::SeparatorText("Portal a otro plano");
            { const char* ps[] = { "No", "Hacia el fondo", "Hacia delante", "A otro nivel", "Al pasillo de este nivel", "Salir al mundo 3D" }; int pi = t.portal == 4 ? 5 : t.portal == 3 ? 4 : t.portal == 2 ? 3 : t.portal > 0 ? 1 : t.portal < 0 ? 2 : 0;
              if (ImGui::Combo("Portal", &pi, ps, 6)) { t.portal = pi == 5 ? 4 : pi == 4 ? 3 : pi == 3 ? 2 : pi == 1 ? 1 : pi == 2 ? -1 : 0; if (t.portal) t.solid = 0; ch = true; }
              if (ImGui::IsItemHovered()) ImGui::SetTooltip("Se entra con E, S o Abajo.\nHacia el fondo / delante: pasa al plano de detras o de delante (se pintan con el selector 'Plano').\nAl pasillo de este nivel: entra en el pasillo estilo Crash que se pone detras del nivel (panel 'Fondo y ambiente' > Pasillo dentro del nivel); al llegar al final se vuelve aqui.\nA otro nivel: salta a ese nivel (p. ej. un pasillo estilo Crash); al terminar un pasillo se vuelve donde saliste.\nSalir al mundo 3D: si llegaste a este mapa desde una escena 3D con la accion 'Ir a una escena' + 'recordar donde estoy', vuelves a esa escena, justo donde estabas.");
              if (t.portal == 2) {
                  if (ImGui::BeginCombo("  Nivel de destino", t.dest.empty() ? "(elige)" : t.dest.c_str())) {
                      for (auto& n : level_names()) if (ImGui::Selectable(n.c_str(), n == t.dest)) { t.dest = n; ch = true; }
                      ImGui::EndCombo();
                  }
              } }
            ImGui::SeparatorText("Puertas, botones y dianas (canal 0 = no)");
            ch |= ImGui::SliderInt("Puerta del canal", &t.door, 0, 15);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Se abre (sube) cuando su canal esta activo.");
            if (t.door) { const char* ds[] = { "Arriba", "Arriba", "Abajo", "Izquierda", "Derecha" }; int d = t.dir < 1 ? 1 : t.dir;
                          if (ImGui::BeginCombo("  Se abre hacia", ds[d])) { for (int k = 1; k <= 4; k++) if (ImGui::Selectable(ds[k], d == k)) { t.dir = k; ch = true; } ImGui::EndCombo(); } }
            ch |= ImGui::SliderInt("Boton del canal", &t.btn, 0, 15);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Se pisa. Activa su canal.");
            if (t.btn) { ch |= ImGui::SliderFloat("  Sigue activo (s)", &t.hold, -1.0f, 20.0f, t.hold < 0 ? "para siempre" : (t.hold == 0 ? "por defecto (4 s)" : "%.1f"));
                         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tiempo que sigue activo despues de soltarlo.\nTodo a la izquierda = para siempre."); }
            ch |= ImGui::SliderInt("Diana del canal", &t.tgt, 0, 15);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Se dispara con el cañon: alterna su canal.");
            ImGui::SeparatorText("Se mueve");
            const char* ejes[] = { "No se mueve", "Horizontal (x)", "Vertical (y)" };
            ch |= ImGui::Combo("Eje", &t.axis, ejes, 3);
            if (t.axis) { ch |= ImGui::SliderFloat("Recorrido (bloques)", &t.dist, 1.0f, 20.0f); ch |= ImGui::SliderFloat("Velocidad", &t.spd, 0.5f, 12.0f);
                          bool os = t.on_step != 0; if (ImGui::Checkbox("Solo se mueve mientras vas encima (ascensor)", &os)) { t.on_step = os; ch = true; }
                          if (!t.on_step) ch |= ImGui::SliderFloat("Pausa en cada extremo (s)", &t.wait, 0.0f, 6.0f); }
            ImGui::SeparatorText("Forma");
            int sh = t.shape == "pinchos" ? 1 : 0; const char* fs[] = { "Bloque", "Pinchos" };
            if (ImGui::Combo("Dibujo", &sh, fs, 2)) { t.shape = sh ? "pinchos" : ""; ch = true; }
        } else {
            ImGui::TextWrapped("Marcador de tipo '%s': no se dibuja en el juego, coloca el inicio del heroe, un enemigo o un punto de control.", t.marker.c_str());
        }
        if (ch) set_dirty = true;
        ImGui::Separator();
        if (ImGui::Button("Guardar tileset")) { save_tileset(); msg = "tileset.def guardado"; }
        ImGui::SameLine(); if (set_dirty) ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1), "cambios sin guardar");
        ImGui::End();
    }

    // =========================================================================
    //  VENTANA: personajes del juego de plataformas (heroe y dos tipos de enemigo)
    // =========================================================================
    void draw_chars() {
        ImGui::Begin(TM_WIN_CHAR);
        if (!dir_ok()) { ImGui::TextDisabled("Crea primero el juego de plataformas."); ImGui::End(); return; }
        bool ch = false;
        auto sheets = [&]() { std::vector<std::string> v; std::error_code ec;
            if (tm_fs::exists(sprites_dir(), ec)) for (auto& e : tm_fs::directory_iterator(sprites_dir())) if (e.path().extension() == ".png") v.push_back("Assets/Sprites/" + e.path().filename().string());
            std::sort(v.begin(), v.end()); return v; }();
        auto personaje = [&](const char* titulo, PlatChar& c, bool heroe, int idx) {
            ImGui::PushID(titulo);
            if (ImGui::CollapsingHeader(titulo, idx == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
                if (ImGui::BeginCombo("Hoja de sprites", c.sheet.empty() ? "(elige)" : c.sheet.c_str())) {
                    for (auto& sh : sheets) if (ImGui::Selectable(sh.c_str(), sh == c.sheet)) { c.sheet = sh; ch = true; }
                    ImGui::EndCombo();
                }
                if (heroe) {
                    if (ImGui::BeginCombo("Modelo 3D (en vez de sprites)", pcfg.hero_model.empty() ? "(ninguno: usa sprites)" : pcfg.hero_model.c_str())) {
                        if (ImGui::Selectable("(ninguno: usa sprites)", pcfg.hero_model.empty())) { pcfg.hero_model.clear(); ch = true; }
                        std::error_code ec2; std::vector<std::string> ms;
                        for (const char* sub : { "Models", "Sprites" }) { std::string d = game_dir + "/Assets/" + sub;
                            if (tm_fs::exists(d, ec2)) for (auto& e : tm_fs::directory_iterator(d, ec2)) if (e.path().extension() == ".glb") ms.push_back(std::string("Assets/") + sub + "/" + e.path().filename().string()); }
                        std::sort(ms.begin(), ms.end());
                        for (auto& m : ms) if (ImGui::Selectable(m.c_str(), m == pcfg.hero_model)) { pcfg.hero_model = m; ch = true; }
                        ImGui::EndCombo();
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Un .glb con esqueleto y animaciones, en este orden: 0 quieto, 1 correr, 2 andar, 3 salto, 4 dano, 5 muerte.\nSi lo eliges, el heroe se dibuja en 3D (en el nivel y en el pasillo) y los sprites no se usan.\nMide lo mismo que la caja de colision.");
                    if (!pcfg.hero_model.empty()) {
                        ImGui::TextDisabled("Animaciones del modelo (numero en el .glb; -1 = ninguna):");
                        static const char* an_n[7] = { "quieto", "correr", "andar", "salto", "dano", "muerte", "disparo" };
                        for (int i = 0; i < 7; i++) { ImGui::SetNextItemWidth(70); ch |= ImGui::InputInt(an_n[i], &pcfg.hero_anim[i], 0, 0); if (i % 3 != 2 && i != 6) ImGui::SameLine(0, 10); }
                        ImGui::SetNextItemWidth(120); ch |= ImGui::DragFloat("Giro del modelo (grados)", &pcfg.hero_yaw, 0.5f, -180.0f, 180.0f, "%.1f");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Si el modelo no mira recto (de frente o de espaldas se ve un poco en diagonal), corrigelo aqui.\nEn el mundo 3D el mismo giro va en la rotacion Y del jugador.");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pon aqui el numero de animacion de TU modelo para cada situacion.\nAsi el heroe del mapa 2.5D puede ser el MISMO personaje que el del mundo 3D (mismo .glb).");
                    }
                    if (ImGui::BeginCombo("De espaldas y de frente", pcfg.hero_depth.empty() ? "(ninguna: usa correr de perfil)" : pcfg.hero_depth.c_str())) {
                        if (ImGui::Selectable("(ninguna: usa correr de perfil)", pcfg.hero_depth.empty())) { pcfg.hero_depth.clear(); ch = true; }
                        for (auto& sh : sheets) if (ImGui::Selectable(sh.c_str(), sh == pcfg.hero_depth)) { pcfg.hero_depth = sh; ch = true; }
                        ImGui::EndCombo();
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hoja que se usa al pasar por un portal (y en los tramos en profundidad).\nCeldas del mismo tamano. Fila 0: correr de espaldas (6). Fila 1: correr de frente (6).\nFila 2: quieto espaldas (0-1), quieto frente (2-3), salto subiendo espaldas (4) y frente (5). Fila 3: cayendo espaldas (0) y frente (1).");
                }
                ch |= ImGui::InputInt("Tamano de celda (px)", &c.cw);
                ch |= ImGui::SliderFloat("Pixeles por unidad", &c.ppu, 8.0f, 160.0f, "%.0f");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tamano en el mundo = pixeles / este valor.\nUn bloque mide 2 unidades; un heroe de 2.7 de alto mide ~1.3 bloques.");
                float an[2] = { c.ax, c.ay }; if (ImGui::SliderFloat2("Ancla (pies)", an, 0.0f, 1.0f)) { c.ax = an[0]; c.ay = an[1]; ch = true; }
                if (heroe) {
                    float cj[3] = { pcfg.hl, pcfg.hr, pcfg.hh };
                    if (ImGui::DragFloat3("Caja: atras, delante, alto", cj, 0.02f, 0.1f, 6.0f)) { pcfg.hl = cj[0]; pcfg.hr = cj[1]; pcfg.hh = cj[2]; ch = true; }
                    ch |= ImGui::DragFloat("Velocidad", &pcfg.run, 0.1f, 1.0f, 40.0f); ch |= ImGui::DragFloat("Fuerza de salto", &pcfg.jump, 0.1f, 5.0f, 60.0f);
                    ch |= ImGui::DragFloat("Gravedad", &pcfg.grav, 0.5f, 10.0f, 200.0f); ch |= ImGui::SliderInt("Vida", &pcfg.hp, 1, 99);
                } else {
                    std::string& em = pcfg.en_model[idx - 1];
                    if (ImGui::BeginCombo("Modelo 3D (en vez de sprites)", em.empty() ? "(ninguno: usa sprites)" : em.c_str())) {
                        if (ImGui::Selectable("(ninguno: usa sprites)", em.empty())) { em.clear(); ch = true; }
                        std::error_code ec2; std::vector<std::string> ms;
                        for (const char* sub : { "Models", "Sprites" }) { std::string d = game_dir + "/Assets/" + sub;
                            if (tm_fs::exists(d, ec2)) for (auto& e : tm_fs::directory_iterator(d, ec2)) if (e.path().extension() == ".glb") ms.push_back(std::string("Assets/") + sub + "/" + e.path().filename().string()); }
                        std::sort(ms.begin(), ms.end());
                        for (auto& m : ms) if (ImGui::Selectable(m.c_str(), m == em)) { em = m; ch = true; }
                        ImGui::EndCombo();
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Un .glb con esqueleto y animaciones, en este orden: 0 quieto, 1 andar, 2 salto, 3 ataque, 4 dano, 5 muerte.\nSi lo eliges, el enemigo se dibuja en 3D (en el nivel y en el pasillo). Mide lo mismo que su caja de colision.");
                    ch |= ImGui::SliderInt("Vida", &pcfg.en_hp[idx - 1], 1, 50);
                    float cj[2] = { pcfg.en_w[idx - 1], pcfg.en_h[idx - 1] };
                    if (ImGui::DragFloat2("Caja: ancho, alto", cj, 0.02f, 0.2f, 6.0f)) { pcfg.en_w[idx - 1] = cj[0]; pcfg.en_h[idx - 1] = cj[1]; ch = true; }
                }
                ImGui::TextDisabled("Animaciones: fila, primera columna, fotogramas, fps");
                for (int i = 0; i < 9; i++) {
                    int v[4] = { c.an[i].row, c.an[i].col, c.an[i].n, c.an[i].fps };
                    ImGui::SetNextItemWidth(-130);
                    if (ImGui::InputInt4(heroe ? PlatCfg::hero_an(i) : PlatCfg::en_an(i), v)) { c.an[i] = { v[0], std::max(0, v[1]), std::max(1, v[2]), std::max(1, v[3]) }; ch = true; }
                }
            }
            ImGui::PopID();
        };
        personaje("Heroe", pcfg.hero, true, 0);
        personaje("Enemigo 1: tirador (se esconde, se abre y dispara)", pcfg.en[0], false, 1);
        personaje("Enemigo 2: perseguidor (corre, salta y ataca de cerca)", pcfg.en[1], false, 2);
        bool hud = pcfg.hud != 0; if (ImGui::Checkbox("Marcador de vida y puntos", &hud)) { pcfg.hud = hud; ch = true; }
        if (ch) pcfg_dirty = true;
        ImGui::Separator();
        if (ImGui::Button("Guardar personajes")) { pcfg.save(cfg_file()); pcfg_dirty = false; msg = "plataformas.cfg guardado"; }
        ImGui::SameLine(); if (ImGui::Button("Valores del ejemplo")) { pcfg.defaults(); pcfg_dirty = true; }
        if (pcfg_dirty) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1), "cambios sin guardar"); }
        ImGui::TextDisabled("Los marcadores 'enemigo1' y 'enemigo2' del mapa usan estos dos tipos.");
        ImGui::End();
    }

    // =========================================================================
    //  VENTANA: fondo y ambiente del nivel
    // =========================================================================
    void draw_bg() {
        ImGui::Begin(TM_WIN_BG);
        if (!dir_ok() || W <= 0) { ImGui::TextDisabled("Abre un nivel."); ImGui::End(); return; }
        bool ch = false;
        ImGui::TextDisabled("Nivel: %s", level.c_str());
        if (!cfg.corridor) {
            ImGui::SeparatorText("Pasillo dentro del nivel");
            auto nm = level_names(); std::vector<std::string> pas; for (auto& n : nm) if (n != level && level_is_corridor(n)) pas.push_back(n);
            if (ImGui::BeginCombo("Pasillo", cfg.cor_dest.empty() ? "(ninguno)" : cfg.cor_dest.c_str())) {
                if (ImGui::Selectable("(ninguno)", cfg.cor_dest.empty())) { cfg.cor_dest.clear(); ch = true; }
                for (auto& n : pas) if (ImGui::Selectable(n.c_str(), n == cfg.cor_dest)) { cfg.cor_dest = n; ch = true; }
                ImGui::EndCombo();
            }
            if (!cfg.cor_dest.empty()) {
                const char* fs[] = { "Volver al portal", "Pasar al 2D del fondo (plano 1)" };
                if (ImGui::Combo("Al terminar el pasillo", &cfg.cor_fin, fs, 2)) ch = true;
                if (cfg.cor_fin && !plane_exists(1)) ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "Falta el plano 1: pulsa '+1' junto a Plano en la barra y dibuja el 2D del fondo.");
            }
            ImGui::TextWrapped("El pasillo (un nivel con la casilla 'Pasillo') se construye detras de este nivel, empezando tras el portal mas bajo con 'Al pasillo de este nivel'. Se ve desde el lado y se entra por el portal, sin cambiar de escena.");
        }
        ImGui::SeparatorText("Capas de decorado (tiles)");
        ImGui::TextWrapped("Las capas 'Fondo 2', 'Fondo 3', 'Fondo' (detras) y 'Primer plano', 'Primer plano 2' (delante) se pintan con tiles como el plano jugable, pero no tienen colision. z negativo = mas al fondo.\n'Exterior' es la FACHADA del mapa (en un pasillo, su techo): se ve desde fuera cuando el mapa esta colocado en un mundo 3D, y desaparece al entrar. Dibuja ahi las paredes de fuera.");
        {   static const char* nm[TM_NL] = { "", "Fondo", "Primer plano", "Fondo 2", "Fondo 3", "Primer plano 2", "Exterior (fachada)", "Decorado" };
            for (int n = 1; n < TM_NL; n++) {
                ImGui::PushID(n); ImGui::TextUnformatted(nm[n]);
                ImGui::SetNextItemWidth(70); ch |= ImGui::DragFloat("z", &cfg.lz[n], 0.1f, -60.0f, 30.0f, "%.1f"); ImGui::SameLine();
                ImGui::SetNextItemWidth(70); ch |= ImGui::DragFloat("grosor", &cfg.lsz[n], 0.05f, 0.1f, 20.0f, "%.2f"); ImGui::SameLine();
                ImGui::SetNextItemWidth(70); ch |= ImGui::DragFloat("escala", &cfg.ls[n], 0.02f, 0.2f, 6.0f, "%.2f");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tamano de los bloques de la capa. Lo lejano con escala (16+|z|)/16 se ve del mismo tamano que el plano jugable y se mueve mas lento (parallax).");
                ImGui::PopID();
            } }
        ImGui::SeparatorText("Imagen de fondo");
        auto bgs = bg_pngs();
        if (ImGui::BeginCombo("Fondo", cfg.bg.empty() ? "(pared lisa y columnas)" : cfg.bg.c_str())) {
            if (ImGui::Selectable("(pared lisa y columnas)", cfg.bg.empty())) { cfg.bg.clear(); cfg.columnas = 1; ch = true; }
            for (auto& b : bgs) if (ImGui::Selectable(b.c_str(), b == cfg.bg)) { cfg.bg = b; cfg.columnas = 0; ch = true; }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Importar fondo...")) { const char* hm = getenv("HOME"); fb_bg.SetPwd(hm ? hm : "/"); fb_bg.Open(); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cualquier imagen de tu ordenador (mejor apaisada, 16:9).\nSe copia a backgrounds/.");
        fb_bg.Display();
        if (fb_bg.HasSelected()) {
            std::string nm = import_image(fb_bg.GetSelected().string(), bg_dir(), 2048); fb_bg.ClearSelected();
            if (!nm.empty()) { cfg.bg = nm; cfg.columnas = 0; ch = true; msg = "Fondo importado: " + nm; }
        }
        if (!cfg.bg.empty()) {
            static std::map<std::string, H2Img> bgimg; auto it = bgimg.find(cfg.bg);
            if (it == bgimg.end()) { H2Img im = {}; h2_load_image((bg_dir() + "/" + cfg.bg).c_str(), &im); it = bgimg.emplace(cfg.bg, im).first; }
            if (it->second.tex) { float w = ImGui::GetContentRegionAvail().x; ImGui::Image((ImTextureID)(intptr_t)it->second.tex, ImVec2(w, w * it->second.h / std::max(1, it->second.w))); }
            { int m = cfg.bg_rep; const char* ms[] = { "Estirar (una sola imagen)", "Repetir en mosaico" };
              if (ImGui::Combo("Colocacion", &m, ms, 2)) { cfg.bg_rep = m; ch = true; }
              if (cfg.bg_rep) { ch |= ImGui::SliderFloat("Alto del patron", &cfg.bg_alto, 30.0f, 70.0f, "%.0f");
                                if (ImGui::IsItemHovered()) ImGui::SetTooltip("La imagen se repite a lo ancho sin limite,\nconservando su proporcion. Mejor si encaja sin costura."); } }
            ch |= ImGui::SliderFloat("Movimiento", &cfg.bg_mov, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("1 = pegado a la pantalla (cielo infinito)\n0 = fijo en el mundo (se mueve al andar)\n0.85 = lejano, con parallax suave");
            bool co = cfg.columnas != 0; if (ImGui::Checkbox("Columnas del fondo", &co)) { cfg.columnas = co; ch = true; }
        } else {
            bool co = cfg.columnas != 0; if (ImGui::Checkbox("Columnas del fondo", &co)) { cfg.columnas = co; ch = true; }
        }
        ImGui::SeparatorText("Cielo y niebla");
        ch |= ImGui::ColorEdit3("Cielo (arriba)", cfg.sky);
        ch |= ImGui::ColorEdit3("Horizonte", cfg.hor);
        ch |= ImGui::ColorEdit3("Niebla", cfg.fog);
        ch |= ImGui::SliderFloat("Niebla desde", &cfg.fog_n, 0.0f, 200.0f, "%.0f");
        ch |= ImGui::SliderFloat("Niebla hasta", &cfg.fog_f, 20.0f, 400.0f, "%.0f");
        ImGui::SeparatorText("Luz");
        ch |= ImGui::ColorEdit3("Ambiente", cfg.amb);
        ch |= ImGui::ColorEdit3("Sol", cfg.sun);
        ch |= ImGui::SliderFloat("Intensidad del sol", &cfg.sun_i, 0.0f, 4.0f);
        if (ch) cfg_dirty = true;
        ImGui::Separator();
        if (ImGui::Button("Guardar ambiente")) { save_cfg(); msg = "Guardado " + level + ".level"; }
        ImGui::SameLine(); if (cfg_dirty) ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1), "cambios sin guardar");
        ImGui::TextDisabled("Se aplica al pulsar Probar nivel.");
        ImGui::End();
    }
};

#endif
