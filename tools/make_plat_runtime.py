#!/usr/bin/env python3
"""Genera editor3d/runtime/plataformas.prg: la BIBLIOTECA de juegos de plataformas 2.5D (la usa el modo
"Plataformas" del editor; la escribe en Scripts/ del proyecto y la incluye en el main.prg generado).

Parte de un juego completo y funcionando (por defecto ~/Escritorio/MegaHD2D/megatest.prg), lo vuelve
CONFIGURABLE (hoja del heroe, celdas, animaciones, enemigos, rutas) y le pone prefijo pl_ a todo para que
no choque con los procesos y variables del proyecto en el que se use.

Uso:  make_plat_runtime.py [origen.prg] [salida.prg]
"""
import re, sys, os

SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/Escritorio/MegaHD2D/megatest.prg')
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'runtime', 'plataformas.prg')
s = open(SRC).read()

def rep(old, new, cnt=1):
    global s
    assert s.count(old) >= 1, 'no encuentro: ' + old[:80]
    s = s.replace(old, new, cnt)

def cut(start_marker, end_marker, new_text=''):
    """Sustituye desde start_marker (incluido) hasta end_marker (excluido)."""
    global s
    a = s.index(start_marker); b = s.index(end_marker, a)
    s = s[:a] + new_text + s[b:]

# ------------------------------------------------------------------ cabecera: sin imports ni main
a = s.index('CONST')
s = s[a:]
rep('CONST\n    TS = 2.0;           // tamano de bloque\n    MAXEN = 16;\nEND', 'CONST\n    PL_TS = 2.0;        // tamano de bloque\nEND')
s = re.sub(r'\bTS\b', 'PL_TS', s)

# ------------------------------------------------------------------ globales de configuracion
rep('GLOBAL\n    int scene; int camera; int light;\n', '''GLOBAL
    // ---- CONFIGURACION: la rellena el codigo que genera el editor antes de llamar a montar() ----
    string hero_sheet; int hero_cw = 160; int hero_ch = 160; float hero_ax = 0.42; float hero_ay = 0.9375; float hero_ppu = 52.0;
    float hero_hl = 0.5; float hero_hr = 0.8; float hero_hh = 2.55;           // caja de colision: de -hl a +hr alrededor del ancla (se invierte al mirar atras)
    float hero_run = 11.0; float hero_jump = 24.0; float hero_grav = 62.0; int hero_hp = 8;
    int ha_row[9]; int ha_col[9]; int ha_n[9]; int ha_fps[9];                  // animaciones del heroe: 0 quieto, 1 correr, 2 subir, 3 cima, 4 caer, 5 disparar, 6 correr+disparar, 7 dano, 8 muerte
    string en_sheet[3]; int en_cw[3]; float en_ax[3]; float en_ay[3]; float en_ppu[3]; int en_hp[3]; float en_w[3]; float en_h[3];   // tipos de enemigo: 0 tirador, 1 perseguidor
    int ea_row[27]; int ea_col[27]; int ea_n[27]; int ea_fps[27];              // 9 animaciones por tipo: 0 quieto/escondido, 1 abrirse, 2 andar, 3 atacar, 4 dano, 5 muerte, 6 subir, 7 cima, 8 caer
    string dirfondos = "backgrounds/"; int hud_on = 1;
    int g_hero; int g_en[3];
    int bull_mesh; int mat_bh; int mat_bhc; int mat_be;
''')
rep('    int g_hero; int g_ph; int g_met; int g_kaka;                  // graficos: hoja del heroe y hoja placeholder\n', '')
rep('    int hhp = 8; int hmaxhp = 8;', '    int hhp = 8; int hmaxhp = 8;')

# ------------------------------------------------------------------ rutas fijas -> configurables
rep('mat_bg = make_mat("backgrounds/" + bgfile, 0.9, 0.0);', 'mat_bg = make_mat(dirfondos + bgfile, 0.9, 0.0);')
rep('g = map_load("backgrounds/" + bgfile);', 'g = map_load(dirfondos + bgfile);')
rep('    mat_metal = make_mat("assets/tile_metal.png", 0.55, 0.25);\n    mat_bg = make_mat("assets/tile_bg.png", 0.9, 0.0);\n    g3d_material_set_color(mat_bg, 0.9, 0.95, 1.0, 1.0);',
    '''    IF (nt >= 1 AND t_tex[1] != "") mat_metal = make_mat(tdir + t_tex[1], 0.55, 0.25); ELSE mat_metal = make_mat("", 0.55, 0.25); END    // la textura del primer tile sirve para mastiles y columnas
    mat_bg = make_mat("", 0.9, 0.0);
    g3d_material_set_color(mat_bg, 0.10, 0.12, 0.20, 1.0);''')
rep('    mat_top = make_mat', '    mat_top = make_mat') if 'mat_top = make_mat' in s else None

# ------------------------------------------------------------------ marcadores: enemigo1 / enemigo2 (y los nombres de siempre)
rep('                            IF (val == "met") t_mark[nt] = 2; END\n                            IF (val == "kaka") t_mark[nt] = 3; END',
    '                            IF (val == "met" OR val == "enemigo1") t_mark[nt] = 2; END\n                            IF (val == "kaka" OR val == "enemigo2") t_mark[nt] = 3; END')

# ------------------------------------------------------------------ ayudantes de celdas (heroe y enemigos)
rep('FUNCTION cell(int sp, int col, int row, int cw, int ch, float ax, float ay)\nBEGIN\n    g3d_sprite_set_cell(sp, col * cw, row * ch, cw, ch);\n    g3d_sprite_set_anchor(sp, ax, ay);\nEND',
    '''FUNCTION cell(int sp, int col, int row, int cw, int ch, float ax, float ay)
BEGIN
    g3d_sprite_set_cell(sp, col * cw, row * ch, cw, ch);
    g3d_sprite_set_anchor(sp, ax, ay);
END

// fotograma f de la animacion a del heroe (se repite sola)
FUNCTION cell_h(int sp, int a, int f)
PRIVATE int n;
BEGIN
    n = ha_n[a]; IF (n < 1) n = 1; END
    cell(sp, ha_col[a] + f % n, ha_row[a], hero_cw, hero_ch, hero_ax, hero_ay);
END

// fotograma f de la animacion a del tipo de enemigo tp
FUNCTION cell_en(int sp, int tp, int a, int f)
PRIVATE int k; int n;
BEGIN
    k = tp * 9 + a; n = ea_n[k]; IF (n < 1) n = 1; END
    cell(sp, ea_col[k] + f % n, ea_row[k], en_cw[tp], en_cw[tp], en_ax[tp], en_ay[tp]);
END''')

# ------------------------------------------------------------------ disparos: bolas brillantes (sin hoja de sprites)
cut('PROCESS hbullet(float bx, float by, int dir, int big)', '// ---------------------------------------------------------------------------\n//  Disparo enemigo', '''PROCESS hbullet(float bx, float by, int dir, int big)
PRIVATE
    int i; int alive = 1; float dmg; int t; int be;
BEGIN
    x = bx; y = by; z = 0.0;
    be = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(be, bull_mesh);
    IF (big) g3d_entity_set_material(be, mat_bhc); g3d_entity_set_scale(be, 0.9, 0.9, 0.9); dmg = 3.0;
    ELSE g3d_entity_set_material(be, mat_bh); g3d_entity_set_scale(be, 0.45, 0.45, 0.45); dmg = 1.0; END
    g3d_entity_set_blend(be, 2);
    nbul = nbul + 1;
    WHILE (alive)
        x = x + dir * 30.0 / 60.0;
        IF (x < hx - 24.0 OR x > hx + 24.0) alive = 0; END
        t = tile_at(x + dir * 0.3, y);                    // dianas: alternan su canal
        IF (t > 0)
            IF (t_tgt[t] > 0) toggled[t_tgt[t]] = 1 - toggled[t_tgt[t]]; alive = 0; END
        END
        IF (box_hits(x - 0.2, y - 0.2, x + 0.2, y + 0.2)) alive = 0; END
        FOR (i = 0; i < nen; i = i + 1)
            IF (alive AND enalive[i])
                IF (x > enx[i] - enw[i] AND x < enx[i] + enw[i] AND y > eny[i] AND y < eny[i] + enh[i] + 0.4)
                    IF (enopen[i]) enhp[i] = enhp[i] - dmg; END
                    alive = 0;
                END
            END
        END
        g3d_entity_set_position(be, x, y, 0.0);
        FRAME;
    END
    nbul = nbul - 1;
    g3d_entity_destroy(be);
END

''')
cut('PROCESS ebullet(float bx, float by, int dir)', '// ---------------------------------------------------------------------------\n//  Enemigo "Met"', '''PROCESS ebullet(float bx, float by, int dir)
PRIVATE
    int alive = 1; int be;
BEGIN
    x = bx; y = by; z = 0.0;
    be = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(be, bull_mesh); g3d_entity_set_material(be, mat_be); g3d_entity_set_scale(be, 0.5, 0.5, 0.5);
    g3d_entity_set_blend(be, 2);
    WHILE (alive)
        x = x + dir * 12.0 / 60.0;
        IF (x < hx - 24.0 OR x > hx + 24.0) alive = 0; END
        IF (box_hits(x - 0.2, y - 0.2, x + 0.2, y + 0.2)) alive = 0; END
        IF (x > hcx - hhalf AND x < hcx + hhalf AND y > hy AND y < hy + 2.5)
            ehit = 1; alive = 0;
        END
        g3d_entity_set_position(be, x, y, 0.0);
        FRAME;
    END
    g3d_entity_destroy(be);
END

''')

# ------------------------------------------------------------------ enemigo tirador (tipo 0)
cut('PROCESS met(float sx, float sy)', '// ---------------------------------------------------------------------------\n//  Kaka con garras', '''PROCESS met(float sx, float sy)
PRIVATE
    int eid; int dir = -1; int tic; int st; int stt; int fr; int lasthp; int hurtf; int dead_t; int tp;
    float nx;
BEGIN
    IF (nen >= 255) say("AVISO: demasiados enemigos (max 255), se ignora uno"); RETURN; END
    ctype = C_3D; csubtype = C3D_SPRITE;
    tp = 0;
    eid = nen; nen = nen + 1;
    x = sx; y = sy; z = 0.0;
    enalive[eid] = 1; enhp[eid] = en_hp[tp]; enx[eid] = x; eny[eid] = y; enopen[eid] = 0; enw[eid] = en_w[tp]; enh[eid] = en_h[tp]; enreach[eid] = 0.0;
    lasthp = enhp[eid];
    stt = 40 + eid * 17;                         // cada uno arranca desfasado
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, en_ppu[tp]);
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);
    g3d_sprite_set_cutout(entity, 0.4);
    g3d_sprite_set_shadow(entity, 1);
    g3d_sprite_set_smooth(entity, 1);
    file = 0; graph = g_en[tp];
    // st: 0 escondido, 1 abriendose, 2 disparando, 3 andando
    WHILE (enhp[eid] > 0)
        tic = tic + 1;
        stt = stt - 1;
        IF (enhp[eid] < lasthp) hurtf = 8; END
        lasthp = enhp[eid];
        IF (hurtf > 0) hurtf = hurtf - 1; END

        IF (st == 0)                                // escondido (invulnerable)
            enopen[eid] = 0;
            IF (stt <= 0 AND abs(hx - x) < 18.0)
                st = 1; stt = 18;
                IF (hx > x) dir = 1; ELSE dir = -1; END
            ELSE IF (stt <= 0) stt = 30; END END
            cell_en(entity, tp, 0, tic / 20);
        ELSE IF (st == 1)                           // abriendose
            enopen[eid] = 1;
            cell_en(entity, tp, 1, (18 - stt) / 6);
            IF (stt <= 0) st = 2; stt = 36; END
        ELSE IF (st == 2)                           // disparando
            enopen[eid] = 1;
            IF (stt == 24) ebullet(x + dir * 1.4, y + 1.0, dir); END
            fr = (36 - stt) / 9; IF (fr > 3) fr = 3; END
            cell_en(entity, tp, 3, fr);
            IF (stt <= 0) st = 3; stt = 80; END
        ELSE                                        // andando hacia el heroe
            enopen[eid] = 1;
            IF (hx > x) dir = 1; ELSE dir = -1; END
            nx = x + dir * 2.5 / 60.0;
            IF (box_hits(nx + dir * 0.9 - 0.1, y + 0.2, nx + dir * 0.9 + 0.1, y + 1.2)
                OR NOT box_hits(nx + dir * 0.9 - 0.1, y - 0.4, nx + dir * 0.9 + 0.1, y - 0.1))
                // borde o pared: no avanza
            ELSE
                x = nx;
            END
            cell_en(entity, tp, 2, tic / 8);
            IF (stt <= 0) st = 0; stt = 70; END
        END END END
        IF (hurtf > 0) cell_en(entity, tp, 4, 0); END
        IF (dir > 0) flags = 0; ELSE flags = 1; END
        enx[eid] = x;
        FRAME;
    END
    // muerte
    enalive[eid] = 0;
    score = score + 100;
    dead_t = 0;
    WHILE (dead_t < 24)
        cell_en(entity, tp, 5, dead_t / 8);
        dead_t = dead_t + 1;
        FRAME;
    END
    g3d_sprite_destroy(entity);
END

''')

# ------------------------------------------------------------------ enemigo perseguidor (tipo 1): se reescribe el principio y los fotogramas
rep('    float hw = 0.8; float hh = 1.7; float dt = 0.0166667;\nBEGIN\n    IF (nen >= 255) say("AVISO: demasiados enemigos (max 255), se ignora uno"); RETURN; END',
    '    float hw; float hh; float dt = 0.0166667; int tp;\nBEGIN\n    IF (nen >= 255) say("AVISO: demasiados enemigos (max 255), se ignora uno"); RETURN; END\n    tp = 1; hw = en_w[tp] * 0.8; hh = en_h[tp];')
rep('    enalive[eid] = 1; enhp[eid] = 5; enx[eid] = x; eny[eid] = y; enopen[eid] = 1;\n    enw[eid] = 1.0; enh[eid] = 1.7; enreach[eid] = 0.0; lasthp = enhp[eid];',
    '    enalive[eid] = 1; enhp[eid] = en_hp[tp]; enx[eid] = x; eny[eid] = y; enopen[eid] = 1;\n    enw[eid] = en_w[tp]; enh[eid] = en_h[tp]; enreach[eid] = 0.0; lasthp = enhp[eid];')
rep('    g3d_sprite_set_pixels_per_unit(entity, 44.0);       // la kaka mide ~1.9 unidades', '    g3d_sprite_set_pixels_per_unit(entity, en_ppu[tp]);')
rep('    file = 0; graph = g_kaka;', '    file = 0; graph = g_en[tp];')
rep('            cell(entity, 1, 4, 160, 160, 0.5, 0.9375);', '            cell_en(entity, tp, 4, 0);')
rep('            cell(entity, fr, 3, 160, 160, 0.5, 0.9375);', '            cell_en(entity, tp, 3, fr);')
rep('            IF (vy > 6.0) fr = 1; ELSE IF (vy > -6.0) fr = 2; ELSE fr = 3; END END\n            cell(entity, fr, 2, 160, 160, 0.5, 0.9375);',
    '            IF (vy > 6.0) cell_en(entity, tp, 6, 0); ELSE IF (vy > -6.0) cell_en(entity, tp, 7, 0); ELSE cell_en(entity, tp, 8, 0); END END')
rep('            cell(entity, (tic / 6) % 6, 1, 160, 160, 0.5, 0.9375);', '            cell_en(entity, tp, 2, tic / 6);')
rep('            cell(entity, (tic / 14) % 6, 0, 160, 160, 0.5, 0.9375);', '            cell_en(entity, tp, 0, tic / 14);')
rep('        cell(entity, dead_t / 6, 5, 160, 160, 0.5, 0.9375);', '        cell_en(entity, tp, 5, dead_t / 6);')

# ------------------------------------------------------------------ heroe: todo configurable
rep('    float gravity = 62.0; float runspd = 11.0; float jumpv = 24.0;', '    float gravity; float runspd; float jumpv; int a; int f;')
rep('    ctype = C_3D; csubtype = C3D_SPRITE;\n    x = startx; y = starty; z = 0.0;\n    entity = g3d_sprite_create(scene, x, y, z);\n    g3d_sprite_set_pixels_per_unit(entity, 52.0);',
    '    ctype = C_3D; csubtype = C3D_SPRITE;\n    gravity = hero_grav; runspd = hero_run; jumpv = hero_jump; hh = hero_hh;\n    x = startx; y = starty; z = 0.0;\n    entity = g3d_sprite_create(scene, x, y, z);\n    g3d_sprite_set_pixels_per_unit(entity, hero_ppu);')
rep('IF (hface > 0) hl = 0.5; hr = 0.8; ELSE hl = 0.8; hr = 0.5; END', 'IF (hface > 0) hl = hero_hl; hr = hero_hr; ELSE hl = hero_hr; hr = hero_hl; END')
rep('            col = 4 + (tic / 12) % 2; row = 4;\n            cell(entity, col, row, 160, 160, 0.42, 0.9375);', '            cell_h(entity, 8, tic * ha_fps[8] / 60);')
cut('        // ----- animacion -----', '        IF (hface > 0) flags = 0; ELSE flags = 1; END', '''        // ----- animacion: cada estado usa su fila de la hoja (configurable) -----
        moving = (dirin != 0 AND hurt_t == 0);
        IF (moving) anim = anim + 1; ELSE anim = 0; END
        a = 0;
        IF (hurt_t > 0)
            a = 7;
        ELSE
            IF (NOT grounded)
                IF (shoot_t > 0) a = 5; ELSE IF (vy > 6.0) a = 2; ELSE IF (vy > -6.0) a = 3; ELSE a = 4; END END END
            ELSE
                IF (shoot_t > 0)
                    IF (moving) a = 6; ELSE a = 5; END
                ELSE
                    IF (moving) a = 1; ELSE a = 0; END
                END
            END
        END
        IF (a == 1 OR a == 6) f = anim * ha_fps[a] / 60; ELSE f = tic * ha_fps[a] / 60; END
        IF (a == 5) f = (16 - shoot_t) * ha_n[5] / 16; END                  // el fogonazo, y luego el retroceso
        cell_h(entity, a, f);
''')
rep('        IF (charge > 55 AND (tic / 4) % 2 == 0) color_r = 190; color_g = 235; color_b = 255;\n        ELSE color_r = 205; color_g = 205; color_b = 205; END   // tono base: baja el brillo del heroe',
    '        IF (charge > 55 AND (tic / 4) % 2 == 0) color_r = 190; color_g = 235; color_b = 255;\n        ELSE color_r = 215; color_g = 215; color_b = 215; END   // tono base: baja un poco el brillo del heroe')

# ------------------------------------------------------------------ main -> montar / descargar
a = s.index('// ---------------------------------------------------------------------------\nPROCESS main()')
s = s[:a] + '''// ---------------------------------------------------------------------------
//  MONTAR / DESCARGAR: lo que llama el codigo que genera el editor (o tu propio main.prg)
// ---------------------------------------------------------------------------

// pone a cero todo lo que deja un nivel anterior
FUNCTION reiniciar()
PRIVATE int i;
BEGIN
    nt = 0; nsp = 0; ncp = 0; npl = 0; nbt = 0; ntg = 0; ndr = 0; liq_n = 0; nen = 0; nbul = 0; score = 0;
    ehit = 0; hride = -1; bg_rep = 0; bgfile = ""; bgmove = 0.85; columnas = 1; bg_alto = 36.0;
    sk_r = 0.04; sk_g = 0.06; sk_b = 0.16; hz_r = 0.10; hz_g = 0.14; hz_b = 0.30;
    fg_r = 0.04; fg_g = 0.05; fg_b = 0.12; fg_n = 40.0; fg_f = 110.0;
    am_r = 0.42; am_g = 0.46; am_b = 0.58; li_r = 1.0; li_g = 0.95; li_b = 0.85; li_i = 1.5;
    FOR (i = 0; i < 16; i = i + 1) chan[i] = 0; pressed[i] = 0; toggled[i] = 0; hold[i] = 0; END
    FOR (i = 0; i < 39; i = i + 1)
        t_tex[i] = ""; t_top[i] = ""; t_solid[i] = 0; t_dmg[i] = 0; t_oneway[i] = 0; t_mark[i] = 0; t_shape[i] = 0; t_slip[i] = 0.0; t_bounce[i] = 0.0; t_conv[i] = 0.0;
        t_axis[i] = 0; t_dist[i] = 0.0; t_spd[i] = 0.0; t_mat[i] = 0; t_matop[i] = 0; t_liq[i] = 0; t_swim[i] = 0; t_slow[i] = 1.0; t_alpha[i] = 255;
        t_ramp[i] = 0; t_door[i] = 0; t_btn[i] = 0; t_tgt[i] = 0; t_wait[i] = 0.0; t_mode[i] = 0; t_hold[i] = 0.0; t_dir[i] = 0;
    END
    FOR (i = 0; i < 79999; i = i + 1) lvl[i] = 0; lvlb[i] = 0; lvlf[i] = 0; liqmark[i] = 0; END
END

FUNCTION montar()
PRIVATE int i;
BEGIN
    reiniciar();
    scene = g3d_scene_create("plataformas");
    g3d_scene_set_active(scene);
    camera = g3d_camera_create();
    g3d_camera_set_active(camera);
    g3d_camera_set_fov(camera, 38.0);

    load_settings();                                    // cielo, niebla, luz y fondo de este nivel
    g3d_sky_set_gradient(sk_r, sk_g, sk_b,  hz_r, hz_g, hz_b);
    g3d_sky_enable(1);
    g3d_set_ambient_light(am_r, am_g, am_b, 1.0);
    light = g3d_light_create(0, li_r, li_g, li_b);
    g3d_light_set_direction(light, 0.35, -0.75, -0.55);
    g3d_light_set_intensity(light, li_i);
    g3d_light_enable_shadow(light, 1);
    g3d_set_shadows(1);
    g3d_set_hdr(1);
    g3d_set_tonemap(1);
    g3d_set_bloom(1, 0.35, 1.0);
    g3d_set_fog(1, fg_r, fg_g, fg_b, fg_n, fg_f);

    g_hero = map_load(hero_sheet);
    FOR (i = 0; i < 2; i = i + 1) IF (en_sheet[i] != "") g_en[i] = map_load(en_sheet[i]); END END
    IF (g_hero <= 0) say("ERROR: no se carga la hoja del heroe " + hero_sheet); RETURN; END
    hmaxhp = hero_hp; hhp = hero_hp;

    // los disparos son bolas brillantes (no necesitan hoja de sprites)
    bull_mesh = g3d_primitive_sphere(12);
    mat_bh = g3d_material_create();  g3d_material_set_color(mat_bh, 1.0, 0.9, 0.3, 1.0);
    mat_bhc = g3d_material_create(); g3d_material_set_color(mat_bhc, 0.4, 0.9, 1.0, 1.0);
    mat_be = g3d_material_create();  g3d_material_set_color(mat_be, 1.0, 0.45, 0.2, 1.0);

    IF (NOT build_level()) RETURN; END
    hero();
    FOR (i = 0; i < nsp; i = i + 1)
        IF (spt[i] == 2) met(spx[i], spy[i]); END
        IF (spt[i] == 3) kaka(spx[i], spy[i]); END
    END
    IF (hud_on)
        write(0, 20, 20, 0, "VIDA:");     write_var(0, 80, 20, 0, hhp);
        write(0, 20, 44, 0, "PUNTOS:");   write_var(0, 100, 44, 0, score);
    END
    camara(camera);
END

// Quita lo que monto montar(): los procesos los mata el editor por su nombre; esto suelta la escena del motor y los textos
FUNCTION descargar()
BEGIN
    write_delete(all_text);
    g3d_scene_destroy(scene);
END
'''
# ------------------------------------------------------------------ quitar globales sueltas ya declaradas por el editor
rep('    string levelfile = "levels/level1.png";', '    string levelfile = "levels/level1.png";')

# ------------------------------------------------------------------ prefijos: nada de lo de aqui puede chocar con tu proyecto
head = s[s.index('GLOBAL'):]
glob = head[:head.index('END\n')]
globals_ = set(re.findall(r'\b(?:int|float|string)\s+([a-z_][a-z0-9_]*)', glob))
procs = set(re.findall(r'^(?:FUNCTION(?:\s+\w+)?|PROCESS)\s+(\w+)\s*\(', s, re.M))
# LOS DEL EDITOR (ya existen en el main.prg generado): no se tocan
keep = {'scene', 'camera', 'light'}
nombres = {'hero': 'pl_heroe', 'camara': 'pl_camara', 'fondo_par': 'pl_fondo', 'circuitos': 'pl_circuitos', 'liquidos': 'pl_liquidos',
           'plat': 'pl_plataforma', 'hbullet': 'pl_bala_heroe', 'ebullet': 'pl_bala_enemigo', 'met': 'pl_tirador', 'kaka': 'pl_perseguidor',
           'montar': 'pl_montar', 'descargar': 'pl_descargar'}
ren = {}
for n in (globals_ | procs) - keep:
    ren[n] = nombres.get(n, 'pl_' + n)
# los nombres locales que coinciden con globales (p. ej. parametros) NO se renombran si son private: se detectan por declaracion local
def rename_code(text):
    out = []
    parts = re.split(r'("[^"\n]*")', text)       # no se toca dentro de las cadenas
    pat = re.compile(r'\b(' + '|'.join(sorted(map(re.escape, ren), key=len, reverse=True)) + r')\b')
    for p in parts:
        out.append(p if p.startswith('"') else pat.sub(lambda m: ren[m.group(1)], p))
    return ''.join(out)
# los locales (PRIVATE) con el mismo nombre que una global la ocultan: se renombran tambien (da igual, es coherente en todo el fichero)
s = rename_code(s)
# los nombres que ya empezaban por pl_ se quedarian como pl_pl_
s = s.replace('pl_pl_', 'pl_')
# lo que ya era "pl_x" local del heroe (pl_l, pl_r...) sigue igual
header = '''// ============================================================================
//  plataformas.prg - juegos de PLATAFORMAS 2.5D (sprites sobre un escenario 3D de bloques)
//  GENERADO por el editor (modo Plataformas) desde tools/make_plat_runtime.py: no se edita a mano,
//  se vuelve a escribir cada vez que generas el juego. Todo lo de aqui lleva el prefijo pl_ para
//  no chocar con tus procesos y variables.
//
//  Uso desde tu codigo:  rellena pl_hero_sheet, pl_levelfile, pl_tilesetfile... y llama a pl_montar().
// ============================================================================
'''
open(OUT, 'w').write(header + s)
print('escrito', OUT, len(s.splitlines()), 'lineas;', len(ren), 'nombres con prefijo')
