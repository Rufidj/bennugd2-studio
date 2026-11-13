// ============================================================================
//  plataformas.prg - juegos de PLATAFORMAS 2.5D (sprites sobre un escenario 3D de bloques)
//  Biblioteca del editor (modo Plataformas). OJO: desde 2026-10-31 este fichero ES la fuente (ya no se regenera
//  con tools/make_plat_runtime.py, que pisaria los cambios); el editor lo copia a Scripts/ cada vez que generas el juego. Todo lo de aqui lleva el prefijo pl_ para
//  no chocar con tus procesos y variables.
//
//  Uso desde tu codigo:  rellena pl_hero_sheet, pl_levelfile, pl_tilesetfile... y llama a pl_montar().
// ============================================================================
CONST
    PL_TS = 2.0;        // tamano de bloque
END

GLOBAL
    // ---- CONFIGURACION: la rellena el codigo que genera el editor antes de llamar a pl_montar() ----
    string pl_hero_sheet; int pl_hero_cw = 160; int pl_hero_ch = 160; float pl_hero_ax = 0.42; float pl_hero_ay = 0.9375; float pl_hero_ppu = 52.0;
    float pl_hero_hl = 0.5; float pl_hero_hr = 0.8; float pl_hero_hh = 2.55;           // caja de colision: de -hl a +hr alrededor del ancla (se invierte al mirar atras)
    float pl_hero_run = 11.0; float pl_hero_jump = 24.0; float pl_hero_grav = 62.0; int pl_hero_hp = 8;
    int pl_ha_row[9]; int pl_ha_col[9]; int pl_ha_n[9]; int pl_ha_fps[9];                  // animaciones del heroe: 0 quieto, 1 correr, 2 subir, 3 cima, 4 caer, 5 disparar, 6 correr+disparar, 7 dano, 8 muerte
    string pl_en_sheet[3]; int pl_en_cw[3]; float pl_en_ax[3]; float pl_en_ay[3]; float pl_en_ppu[3]; int pl_en_hp[3]; float pl_en_w[3]; float pl_en_h[3];   // tipos de enemigo: 0 tirador, 1 perseguidor
    int pl_ea_row[27]; int pl_ea_col[27]; int pl_ea_n[27]; int pl_ea_fps[27];              // 9 animaciones por tipo: 0 quieto/escondido, 1 abrirse, 2 andar, 3 atacar, 4 dano, 5 muerte, 6 subir, 7 cima, 8 caer
    string pl_dirfondos = "backgrounds/"; int pl_hud_on = 1;
    int pl_g_hero; int pl_g_en[3];
    // mapa 2.5D colocado DENTRO de un mundo 3D (pl_montar_mundo): se construye con sus coordenadas y cuelga de una entidad raiz (pl_root)
    int pl_fz_puerta[8]; int pl_fz_est[8]; int pl_fz_ent[8]; int pl_fz_sw[8]; int pl_fz_box[24]; float pl_fz_off[8]; float pl_fz_sx[8]; float pl_fz_sz[8]; float pl_fz_dx[8]; float pl_fz_dy[8];   // puerta e interruptor de cada fachada: estado 0 cerrada, 1 abriendose, 2 abierta
    int pl_ev_pulsar[8]; int pl_ev_abrir[8]; int pl_ev_entrar[8]; int pl_fz_eprev;                                 // eventos (los atiende el proceso fachada_eventos que genera el editor, que llama a tus PROCESS)
    float pl_fz_x[8]; float pl_fz_y[8]; float pl_fz_z[8]; int pl_fz_e[8]; int pl_fz_n;      // fachadas con puerta (modo teletransporte): donde esta la puerta en el mundo y a que escena lleva
    int pl_fct[79999];                                  // tile de la fachada en cada celda (para el cuerpo del edificio)
    int pl_fcm[79999];                                  // celdas de la fachada (capa Exterior), para ponerle colision
    float pl_cor_hx; float pl_cor_hy;                    // donde estaba el heroe del mapa al entrar al pasillo (el heroe del pasillo empieza ahi y entra andando)
    int pl_mundo; int pl_root; int pl_ext_root; int ps_ext_root; float pl_wx; float pl_wy; float pl_wz; float pl_wyaw;      // pl_dentro = 1: el jugador esta dentro del mapa y lo controla el heroe 2D
    int pl_por_n; float pl_por_x; float pl_por_y; int pl_ent_prev;
    int pl_lvk[8]; int pl_lvi[8]; int pl_lvp[8]; int pl_lvs[8];       // niveles abiertos (pl_lv_open): tipo (1 .lvl, 2 PNG), id, plano y subcapa
    string pl_en_model[3]; int pl_em_n; int pl_em_m[96]; int pl_em_np[96]; int pl_em_p[384]; float pl_em_sc[96]; float pl_em_t[96]; int pl_em_a[96]; int pl_em_pa[96]; float pl_em_pt[96]; float pl_em_w[96]; float pl_em_yaw[96]; int pl_em_a3;   // enemigos como modelo 3D: una copia del modelo por enemigo (animaciones 0 quieto 1 andar 2 salto 3 ataque 4 dano 5 muerte)
    float pl_hm_yawoff;                                  // giro de correccion del modelo del heroe (grados)
    int pl_hm_map[7] = 0, 1, 2, 3, 4, 5, -1;                 // numero de animacion del .glb del heroe para: quieto, correr, andar, salto, dano, muerte
    string pl_hero_model; int pl_hm_on; int pl_hm_model; int pl_hm_part[7]; int pl_hm_n; float pl_hm_scale;   // heroe como modelo 3D (glb con esqueleto): animaciones 0 quieto 1 correr 2 andar 3 salto 4 dano 5 muerte
    float pl_hm_x; float pl_hm_y; float pl_hm_z; float pl_hm_yaw; int pl_hm_anim; int pl_hm_show; int pl_hm_blink; int pl_hm_ps;
    string pl_hero_dsheet; int pl_g_hero_d; int pl_trans_dir;   // hoja del heroe de espaldas y de frente (portales): fila 0 correr de espaldas, fila 1 correr de frente
    int pl_bull_mesh; int pl_mat_bh; int pl_mat_bhc; int pl_mat_be;
    int pl_box_mesh; int pl_wedge_mesh; int pl_mat_metal; int pl_mat_top; int pl_mat_bg;
    int pl_lw; int pl_lh;                      // tamano del nivel en bloques (sale de la imagen)
    // capas de decorado: 1 fondo cercano, 2 primer plano, 3 fondo medio, 4 fondo lejano, 5 primer plano cercano. z, grosor y escala de cada una (.level: capaN=z,grosor,escala)
    // ---- planos jugables apilados en z (portales): <nivel>.png = plano 0, <nivel>.p1.png = el de detras, <nivel>.p2.png ----
    int pl_plane; int pl_nplanes = 1; float pl_pd = 5.0;       // plano activo, cuantos hay, separacion en z
    float pl_zoff; int pl_vis;                                 // al construir: desplazamiento en z y modo 'solo decorado' (planos que no son el activo)
    float pl_tz; int pl_trans; int pl_cambio; int pl_portal_cd; // transicion entre planos: z extra del heroe y la camara, bloqueo de control, reconstruccion en curso
    float pl_bgz = -14.0; float pl_bgk = 1.0;                  // fondo: z y escala (se aleja si hay planos detras)
    float pl_t_oh[79];                    // tiles de decorado 3D (objeto=arbol|pino|...; alto=)
    int pl_t_portal[79]; string pl_t_dest[79];                  // portal: 1 al fondo, -1 hacia delante, 2 = a otro NIVEL (pl_t_dest)
    // pasillo embebido (ver pasillo.prg): el nivel declara pasillo=<nivel>; pl_mode 1 = se juega el pasillo (camara de detras), 0 = el nivel de lado
    string pl_cor_file; float pl_cor_px; float pl_cor_py; int pl_cor_found; int pl_cor_req; int pl_mode; int pl_camblend; float pl_cam_p[6];
    float ps_hx; float ps_hy; float ps_hz;                      // posicion del heroe del pasillo (coordenadas del pasillo) y desplazamiento del pasillo en el mundo
    float ps_ox; float ps_oy; float ps_oz;
    int pl_cor_fin; int pl_cor_pass; int pl_cor_h; float pl_cam_keep[6]; int pl_cam_keep_on; float pl_cam_last[6];   // pasillo_fin=plano: el final del pasillo desemboca en el plano 2D de detras (plano 1) en vez de volver
    string pl_arr_from; float pl_arr_x; float pl_arr_y; int pl_arr_ok;   // llegada por una puerta lateral: de que nivel vienes y donde esta, en este, la puerta que lleva de vuelta a el
    string pl_salto; string pl_alfinal;                        // peticion de cambio de nivel (la atiende el codigo que genera el editor) y a donde lleva el final de un pasillo
    float pl_ret_x; float pl_ret_y; string pl_ret_file; string pl_ret_level; int pl_ret_on;   // de donde se salio por un portal de nivel (para volver al mismo sitio)
    float pl_dz = -1.0; float pl_lz[8]; float pl_lsz[8]; float pl_ls[8];
    int pl_lvl[79999]; int pl_lvlb[79999]; int pl_lvlf[79999];   // capas: plano jugable, fondo, primer plano (indice de tile; 0 = vacio)
    string pl_levelfile = "levels/level1.png";
    float pl_spx[255]; float pl_spy[255]; int pl_spt[255]; int pl_nsp;   // marcadores: 2 Met, 3 pl_perseguidor
    float pl_cpx[7]; float pl_cpy[7]; int pl_cpent[7]; int pl_cpon[7]; int pl_ncp;   // puntos de control
    int pl_mat_flag; int pl_mat_flagon;
    // ---- tileset: propiedades de cada tile (1..pl_nt) leidas de tiles/tileset.def ----
    string pl_tilesetfile = "tiles/tileset.def"; string pl_tdir;
    int pl_nt; string pl_t_name[79]; string pl_t_tex[79]; string pl_t_top[79];
    int pl_t_solid[79]; int pl_t_dmg[79]; int pl_t_oneway[79]; int pl_t_mark[79]; int pl_t_shape[79];
    float pl_t_slip[79]; float pl_t_bounce[79]; float pl_t_conv[79]; int pl_t_axis[79]; float pl_t_dist[79]; float pl_t_spd[79];
    int pl_t_mat[79]; int pl_t_matop[79];
    int pl_t_liq[79]; int pl_t_swim[79]; float pl_t_slow[79]; int pl_t_alpha[79];   // pl_liquidos: tipo (1 agua 2 lava 3 acido 4 cieno), se nada, lentitud, transparencia
    int pl_t_dir[79];                                                          // puerta: hacia donde se abre (1 arriba, 2 abajo, 3 izquierda, 4 derecha)
    int pl_t_ramp[79]; int pl_t_door[79]; int pl_t_btn[79]; int pl_t_tgt[79]; float pl_t_wait[79]; int pl_t_mode[79];   // rampa (1 der, 2 izq), puerta/boton/diana (canal 1..15), espera y modo de las que se mueven
    int pl_chan[160]; int pl_pressed[160]; int pl_toggled[160]; int pl_hold[160]; float pl_t_hold[79];        // pl_hold: fotogramas que siguen activos tras soltar el boton                      // estado de los canales (puertas, botones, dianas)
    string pl_lk_name[139]; string pl_lk_tgt[139]; int pl_lk_x[139]; int pl_lk_y[139]; int pl_lk_lv[139]; int pl_lk_n;    // enlaces entre objetos (bloque LNKS del .lvl): puertas, botones y portales con nombre
    int pl_lk_s[8]; int pl_lk_ns; string pl_arr_door;
    int pl_pl_ex[48]; int pl_pl_ns[48]; int pl_pl_sc[384];
    int pl_dr_ex[63]; int pl_dr_ns[63]; int pl_dr_sc[504]; int pl_bt_ch[63]; int pl_tg_ch[63]; int pl_tg_c[63]; int pl_tg_r[63];
    int pl_dr_ent[63]; int pl_dr_c[63]; int pl_dr_r[63]; int pl_dr_t[63]; float pl_dr_pr[63]; int pl_ndr; int pl_dr_len[63]; int pl_dr_pl[63];     // puertas
    int pl_bt_ent[63]; int pl_bt_c[63]; int pl_bt_r[63]; int pl_bt_t[63]; int pl_nbt;                    // botones de pisar
    int pl_tg_ent[63]; int pl_tg_t[63]; int pl_ntg; int pl_last_ent;                                 // dianas (se activan disparandolas)
    float pl_hcx; float pl_hhalf = 0.65;                                          // centro y semiancho del cuerpo del heroe (para golpes y botones)
    int pl_hride = -1;                                                       // plataforma sobre la que va el heroe
    float pl_bg_alto = 36.0; int pl_bg_rep;                                     // fondo: altura y si se repite en mosaico
    int pl_liqmark[79999];                                                     // celdas de liquido ya unidas en un cuerpo
    int pl_liq_ent[2047]; float pl_liq_x[2047]; float pl_liq_y[2047]; int pl_liq_n;    // superficies de liquido que ondulan
    // ---- ambiente del nivel (levels/<nivel>.level; ver LEEME) ----
    float pl_sk_r = 0.04; float pl_sk_g = 0.06; float pl_sk_b = 0.16;         // cielo, arriba
    float pl_hz_r = 0.10; float pl_hz_g = 0.14; float pl_hz_b = 0.30;         // cielo, horizonte
    float pl_fg_r = 0.04; float pl_fg_g = 0.05; float pl_fg_b = 0.12; float pl_fg_n = 40.0; float pl_fg_f = 110.0;   // niebla
    float pl_am_r = 0.42; float pl_am_g = 0.46; float pl_am_b = 0.58;         // luz ambiente
    float pl_li_r = 1.0; float pl_li_g = 0.95; float pl_li_b = 0.85; float pl_li_i = 1.5;   // luz del sol
    string pl_tl_file[5]; float pl_tl_d[5]; float pl_tl_h[5]; float pl_tl_y[5]; float pl_tl_mov[5]; int pl_tl_n; int pl_sil = 1; string pl_deco; string pl_deco_model; int pl_deco_n = 70;   // telones del pasillo (telonN=archivo,distancia,alto,y,mov) y siluetas automaticas
    string pl_bgfile; float pl_bgmove = 0.85; int pl_columnas = 1;           // imagen de fondo (en backgrounds/) y cuanto sigue a la pl_camara
    float pl_camx; float pl_camy; float pl_camdist = 16.0;                    // distancia de la pl_camara al plano del heroe (menos = mas zoom)
    float pl_camx2;                                          // posicion de la pl_camara (la publica el proceso pl_camara)
    // ---- plataformas que se mueven (solo se pisan desde arriba) ----
    float pl_plx[111]; float pl_ply[111]; float pl_pldx[111]; float pl_pldy[111]; int pl_npl;       // solidos que se mueven: plataformas (0..47) y puertas
    float pl_plw[111]; float pl_plh[111];                                                   // semiancho y semialto de cada uno
    float pl_hx; float pl_hy; int pl_hface = 1;     // posicion del heroe (para pl_camara/enemigos)
    int pl_hhp = 8; int pl_hmaxhp = 8;
    int pl_ehit;                              // un disparo enemigo ha dado al heroe
    int pl_nbul;                              // balas del heroe en pantalla
    int pl_nen;                               // enemigos creados
    int pl_enalive[255]; int pl_enopen[255]; int pl_enhp[255]; float pl_enx[255]; float pl_eny[255];
    float pl_enw[255]; float pl_enh[255]; float pl_enreach[255];   // semiancho, alto y alcance extra (garras)
    int pl_score;
    float pl_startx = 7.0; float pl_starty = 4.0;
END

// ---------------------------------------------------------------------------
//  Rejilla de colision
// ---------------------------------------------------------------------------
FUNCTION int pl_is_solid(int c, int r)
PRIVATE int t;
BEGIN
    IF (c < 0 OR c >= pl_lw) RETURN 1; END         // paredes laterales del mundo
    IF (r < 0)                                  // bajo el mapa: abierto, salvo bajo un liquido (tiene fondo)
        IF (pl_lvl[c] > 0)
            IF (pl_t_liq[pl_lvl[c]] > 0) RETURN 1; END
        END
        RETURN 0;
    END
    IF (r >= pl_lh) RETURN 0; END                  // el cielo esta abierto
    t = pl_lvl[r * pl_lw + c];
    IF (t > 0)
        IF (pl_t_solid[t] AND NOT pl_t_oneway[t]) RETURN 1; END
    END
    RETURN 0;
END

// tile del plano jugable en un punto del mundo (0 = vacio o fuera)
// nombre de un nivel a partir de su fichero: Assets/Levels/level1.png -> level1
FUNCTION string pl_stem(string f)
PRIVATE int i; int a;
BEGIN
    a = 0;
    FOR (i = 0; i < strlen(f); i = i + 1) IF (substr(f, i, 1) == "/") a = i + 1; END END
    RETURN substr(f, a, strlen(f) - a - 4);
END

// ---- Entrada: teclado Y mando (stick izquierdo / cruceta y botones) con las mismas acciones ----
// A (cruz) salta, X (cuadrado) dispara, B (circulo) = accion (puertas, interruptores), R1 = ataque giratorio.
FUNCTION int pl_in_left()
BEGIN RETURN key(_LEFT) OR key(_A) OR joy_getbutton(JOY_BUTTON_DPAD_LEFT) OR joy_getaxis(JOY_AXIS_LEFTX) < -14000; END
FUNCTION int pl_in_right()
BEGIN RETURN key(_RIGHT) OR key(_D) OR joy_getbutton(JOY_BUTTON_DPAD_RIGHT) OR joy_getaxis(JOY_AXIS_LEFTX) > 14000; END
FUNCTION int pl_in_up()
BEGIN RETURN key(_UP) OR key(_W) OR joy_getbutton(JOY_BUTTON_DPAD_UP) OR joy_getaxis(JOY_AXIS_LEFTY) < -14000; END
FUNCTION int pl_in_down()
BEGIN RETURN key(_DOWN) OR key(_S) OR joy_getbutton(JOY_BUTTON_DPAD_DOWN) OR joy_getaxis(JOY_AXIS_LEFTY) > 14000; END
FUNCTION int pl_in_jump()
BEGIN RETURN key(_SPACE) OR key(_K) OR joy_getbutton(JOY_BUTTON_A); END
FUNCTION int pl_in_shot()
BEGIN RETURN key(_J) OR key(_X) OR key(_Z) OR joy_getbutton(JOY_BUTTON_X); END
FUNCTION int pl_in_spin()
BEGIN RETURN key(_L) OR key(_C) OR joy_getbutton(JOY_BUTTON_RIGHTSHOULDER); END
FUNCTION int pl_in_act()
BEGIN RETURN key(_E) OR joy_getbutton(JOY_BUTTON_B); END

FUNCTION int pl_tile_at(float px, float py)
PRIVATE int c; int r;
BEGIN
    IF (px < 0.0 OR py < 0.0) RETURN 0; END
    c = px / PL_TS; r = py / PL_TS;
    IF (c >= pl_lw OR r >= pl_lh) RETURN 0; END
    RETURN pl_lvl[r * pl_lw + c];
END

// altura de la superficie de una rampa bajo el punto (px, py); -1000 si no hay rampa. Mira la celda del pie y la de debajo.
FUNCTION float pl_slope_y(float px, float py)
PRIVATE int c; int r; int rb; int t; int k; float fx; float hs; float best;
BEGIN
    best = -1000.0;
    IF (px < 0.0 OR py < 0.0) RETURN best; END
    c = px / PL_TS; rb = py / PL_TS;
    IF (c >= pl_lw) RETURN best; END
    FOR (k = 0; k <= 1; k = k + 1)
        r = rb - k;
        IF (r >= 0 AND r < pl_lh)
            t = pl_lvl[r * pl_lw + c];
            IF (t > 0)
                IF (pl_t_ramp[t] > 0)
                    fx = px - c * PL_TS;
                    IF (pl_t_ramp[t] == 1) hs = r * PL_TS + fx; ELSE hs = r * PL_TS + (PL_TS - fx); END
                    IF (hs > best) best = hs; END
                END
            END
        END
    END
    RETURN best;
END

// mayor dano de los tiles que toca el rectangulo
FUNCTION int pl_box_dmg(float x0, float y0, float x1, float y1)
PRIVATE int c0; int c1; int r0; int r1; int c; int r; int t; int best;
BEGIN
    IF (x0 < 0.0 OR y0 < 0.0) RETURN 0; END
    c0 = x0 / PL_TS; c1 = x1 / PL_TS; r0 = y0 / PL_TS; r1 = y1 / PL_TS;
    FOR (r = r0; r <= r1; r = r + 1)
        FOR (c = c0; c <= c1; c = c + 1)
            IF (c >= 0 AND c < pl_lw AND r >= 0 AND r < pl_lh)
                t = pl_lvl[r * pl_lw + c];
                IF (t > 0 AND pl_t_dmg[t] > best) best = pl_t_dmg[t]; END
            END
        END
    END
    RETURN best;
END

// 1 si el rectangulo (x0,y0)-(x1,y1) toca algun bloque
FUNCTION int pl_box_hits(float x0, float y0, float x1, float y1)
PRIVATE
    int c0; int c1; int r0; int r1; int c; int r;
BEGIN
    IF (x0 < 0.0) c0 = -1; ELSE c0 = x0 / PL_TS; END
    IF (x1 < 0.0) c1 = -1; ELSE c1 = x1 / PL_TS; END
    IF (y0 < 0.0) r0 = -1; ELSE r0 = y0 / PL_TS; END
    IF (y1 < 0.0) r1 = -1; ELSE r1 = y1 / PL_TS; END
    FOR (r = r0; r <= r1; r = r + 1)
        FOR (c = c0; c <= c1; c = c + 1)
            IF (pl_is_solid(c, r)) RETURN 1; END
        END
    END
    RETURN 0;
END

FUNCTION int pl_make_mat(string tex, float rough, float metal)
PRIVATE int m;
BEGIN
    m = g3d_material_create();
    g3d_material_set_color(m, 1.0, 1.0, 1.0, 1.0);
    g3d_material_set_roughness(m, rough);
    g3d_material_set_metallic(m, metal);
    IF (tex != "") g3d_material_set_texture(m, 0, g3d_load_texture(tex)); END
    RETURN m;
END

// ---------------------------------------------------------------------------
//  Tileset: un tile por linea en tiles/tileset.def (ver las propiedades en ese fichero)
// ---------------------------------------------------------------------------
FUNCTION int pl_load_tileset(string fn)
PRIVATE
    int h; string line; string tok[15]; string sub[5]; int n; int k; int p; int i; int dl; string key; string val;
BEGIN
    IF (NOT fexists(fn)) say("ERROR: no se encuentra el tileset " + fn); RETURN 0; END
    dl = 0;
    FOR (i = 0; i < strlen(fn); i = i + 1) IF (substr(fn, i, 1) == "/") dl = i + 1; END END
    pl_tdir = substr(fn, 0, dl);
    h = fopen(fn, O_READ);
    WHILE (NOT feof(h))
        line = trim(fgets(h));
        IF (strlen(line) > 1 AND substr(line, 0, 1) != "#")
            n = split(" +", line, &tok[0], 15);
            IF (tok[0] == "tile" AND pl_nt < 78 AND n >= 2)
                pl_nt = pl_nt + 1;
                pl_t_name[pl_nt] = tok[1]; pl_t_oh[pl_nt] = 0.0; pl_t_tex[pl_nt] = ""; pl_t_top[pl_nt] = ""; pl_t_solid[pl_nt] = 1; pl_t_slow[pl_nt] = 1.0; pl_t_alpha[pl_nt] = 255;
                FOR (k = 2; k < n; k = k + 1)
                    p = find(tok[k], "=");
                    IF (p > 0)
                        key = substr(tok[k], 0, p); val = substr(tok[k], p + 1);
                        IF (key == "textura") pl_t_tex[pl_nt] = val; END
                        IF (key == "alto") pl_t_oh[pl_nt] = atof(val); END
                        IF (key == "tapa") pl_t_top[pl_nt] = val; END
                        IF (key == "solido") pl_t_solid[pl_nt] = atoi(val); END
                        IF (key == "dano") pl_t_dmg[pl_nt] = atoi(val); END
                        IF (key == "unavia") pl_t_oneway[pl_nt] = atoi(val); END
                        IF (key == "resbala") pl_t_slip[pl_nt] = atof(val); END
                        IF (key == "rebota") pl_t_bounce[pl_nt] = atof(val); END
                        IF (key == "cinta") pl_t_conv[pl_nt] = atof(val); END
                        IF (key == "liquido")
                            IF (val == "agua") pl_t_liq[pl_nt] = 1; END
                            IF (val == "lava") pl_t_liq[pl_nt] = 2; END
                            IF (val == "acido") pl_t_liq[pl_nt] = 3; END
                            IF (val == "cieno") pl_t_liq[pl_nt] = 4; END
                            pl_t_solid[pl_nt] = 0;
                            IF (pl_t_alpha[pl_nt] == 255) pl_t_alpha[pl_nt] = 150; END
                            IF (pl_t_slow[pl_nt] == 1.0) pl_t_slow[pl_nt] = 0.6; END
                        END
                        IF (key == "nado") pl_t_swim[pl_nt] = atoi(val); END
                        IF (key == "rampa")
                            IF (val == "der") pl_t_ramp[pl_nt] = 1; END
                            IF (val == "izq") pl_t_ramp[pl_nt] = 2; END
                            pl_t_solid[pl_nt] = 0;
                        END
                        IF (key == "portal")
                            IF (val == "pasillo") pl_t_portal[pl_nt] = 3; END
                            IF (val == "exterior") pl_t_portal[pl_nt] = 4; END
                            IF (substr(val, 0, 6) == "nivel:") pl_t_portal[pl_nt] = 2; pl_t_dest[pl_nt] = substr(val, 6); END
                            IF (val == "fondo") pl_t_portal[pl_nt] = 1; END
                            IF (val == "frente") pl_t_portal[pl_nt] = -1; END
                            pl_t_solid[pl_nt] = 0;
                        END
                        IF (key == "puerta") pl_t_door[pl_nt] = atoi(val); IF (pl_t_dir[pl_nt] == 0) pl_t_dir[pl_nt] = 1; END END
                        IF (key == "abre")
                            IF (val == "arriba") pl_t_dir[pl_nt] = 1; END
                            IF (val == "abajo") pl_t_dir[pl_nt] = 2; END
                            IF (val == "izq") pl_t_dir[pl_nt] = 3; END
                            IF (val == "der") pl_t_dir[pl_nt] = 4; END
                        END
                        IF (key == "boton") pl_t_btn[pl_nt] = atoi(val); pl_t_solid[pl_nt] = 0; IF (pl_t_hold[pl_nt] == 0.0) pl_t_hold[pl_nt] = 4.0; END END
                        IF (key == "mantener") pl_t_hold[pl_nt] = atof(val); END      // segundos que sigue activo tras soltarlo; -1 = para siempre
                        IF (key == "diana") pl_t_tgt[pl_nt] = atoi(val); END
                        IF (key == "espera") pl_t_wait[pl_nt] = atof(val); END
                        IF (key == "activa" AND val == "pisar") pl_t_mode[pl_nt] = 1; END
                        IF (key == "lento") pl_t_slow[pl_nt] = atof(val); END
                        IF (key == "alfa") pl_t_alpha[pl_nt] = atoi(val); END
                        IF (key == "forma" AND val == "pinchos") pl_t_shape[pl_nt] = 1; END
                        IF (key == "marcador")
                            pl_t_solid[pl_nt] = 0;
                            IF (val == "inicio") pl_t_mark[pl_nt] = 1; END
                            IF (val == "met" OR val == "enemigo1") pl_t_mark[pl_nt] = 2; END
                            IF (val == "kaka" OR val == "enemigo2") pl_t_mark[pl_nt] = 3; END
                            IF (val == "check") pl_t_mark[pl_nt] = 4; END
                        END
                        IF (key == "mueve")
                            split(":", val, &sub[0], 5);
                            IF (sub[0] == "x") pl_t_axis[pl_nt] = 1; ELSE pl_t_axis[pl_nt] = 2; END
                            pl_t_dist[pl_nt] = atof(sub[1]); pl_t_spd[pl_nt] = atof(sub[2]);
                        END
                    END
                END
            END
        END
    END
    fclose(h);
    FOR (i = 1; i <= pl_nt; i = i + 1)
        IF (pl_t_tex[i] != "")
            IF (pl_t_slip[i] > 0.0) pl_t_mat[i] = pl_make_mat(pl_tdir + pl_t_tex[i], 0.12, 0.0);
            ELSE pl_t_mat[i] = pl_make_mat(pl_tdir + pl_t_tex[i], 0.6, 0.0); END
        END
        IF (pl_t_top[i] != "") pl_t_matop[i] = pl_make_mat(pl_tdir + pl_t_top[i], 0.6, 0.0); END
    END
    say("tileset " + fn + ": " + itoa(pl_nt) + " tiles");
    RETURN 1;
END

// ---------------------------------------------------------------------------
//  Ajustes del nivel: levels/<nivel>.level (opcional). Una linea  clave=valor  (ver levels/LEEME.md)
// ---------------------------------------------------------------------------
FUNCTION pl_load_settings()
PRIVATE
    int h; int i; string line; string sub[7]; int n; int p; string key; string val; string fn;
BEGIN
    fn = substr(pl_levelfile, 0, strlen(pl_levelfile) - 4) + ".level";
    IF (NOT fexists(fn)) RETURN; END
    h = fopen(fn, O_READ);
    WHILE (NOT feof(h))
        line = trim(fgets(h));
        p = find(line, "=");
        IF (strlen(line) > 2 AND substr(line, 0, 1) != "#" AND p > 0)
            key = trim(substr(line, 0, p)); val = trim(substr(line, p + 1));
            n = split(",", val, &sub[0], 7);
            IF (key == "cielo" AND n >= 3) pl_sk_r = atof(sub[0]); pl_sk_g = atof(sub[1]); pl_sk_b = atof(sub[2]); END
            IF (key == "horizonte" AND n >= 3) pl_hz_r = atof(sub[0]); pl_hz_g = atof(sub[1]); pl_hz_b = atof(sub[2]); END
            IF (key == "niebla" AND n >= 5) pl_fg_r = atof(sub[0]); pl_fg_g = atof(sub[1]); pl_fg_b = atof(sub[2]); pl_fg_n = atof(sub[3]); pl_fg_f = atof(sub[4]); END
            IF (key == "planos" AND n >= 1) pl_pd = atof(sub[0]); END
            IF (key == "al_final") pl_alfinal = trim(val); END
            IF (key == "pasillo_fin" AND val == "plano") pl_cor_fin = 1; END
            IF (key == "pasillo") pl_cor_file = "Assets/Levels/" + trim(val) + ".png"; END
            IF (key == "ambiente" AND n >= 3) pl_am_r = atof(sub[0]); pl_am_g = atof(sub[1]); pl_am_b = atof(sub[2]); END
            IF (substr(key, 0, 4) == "capa" AND n >= 3 AND strlen(key) == 5 AND atoi(substr(key, 4, 1)) >= 1 AND atoi(substr(key, 4, 1)) <= 6)
                pl_lz[atoi(substr(key, 4, 1))] = atof(sub[0]); pl_lsz[atoi(substr(key, 4, 1))] = atof(sub[1]); pl_ls[atoi(substr(key, 4, 1))] = atof(sub[2]);
            END
            IF (key == "luz" AND n >= 4) pl_li_r = atof(sub[0]); pl_li_g = atof(sub[1]); pl_li_b = atof(sub[2]); pl_li_i = atof(sub[3]); END
            IF (key == "fondo") pl_bgfile = val; pl_columnas = 0; END
            IF (key == "fondo_mov") pl_bgmove = atof(val); END
            IF (key == "fondo_modo" AND val == "repetir") pl_bg_rep = 1; END
            IF (key == "fondo_alto") pl_bg_alto = atof(val); END
            IF (key == "columnas") pl_columnas = atoi(val); END
            IF (key == "capa7" AND n >= 1) pl_dz = atof(sub[0]); END
            IF (key == "siluetas") pl_sil = atoi(val); END
            IF (key == "decorado") pl_deco = trim(val); END
            IF (key == "decorado_modelo") pl_deco_model = trim(val); END
            IF (key == "decorado_n") pl_deco_n = atoi(val); END
            IF (substr(key, 0, 5) == "telon" AND strlen(key) == 6 AND atoi(substr(key, 5, 1)) >= 1 AND atoi(substr(key, 5, 1)) <= 5 AND n >= 1)
                i = atoi(substr(key, 5, 1)) - 1; pl_tl_file[i] = trim(sub[0]); pl_tl_d[i] = 40.0; pl_tl_h[i] = 40.0; pl_tl_y[i] = 12.0; pl_tl_mov[i] = 0.0;
                IF (n >= 2) pl_tl_d[i] = atof(sub[1]); END
                IF (n >= 3) pl_tl_h[i] = atof(sub[2]); END
                IF (n >= 4) pl_tl_y[i] = atof(sub[3]); END
                IF (n >= 5) pl_tl_mov[i] = atof(sub[4]); END
                IF (i + 1 > pl_tl_n) pl_tl_n = i + 1; END
                pl_columnas = 0;
            END
        END
    END
    fclose(h);
    say("ajustes del nivel: " + fn);
END

// Fondo con imagen: una pared lejana que sigue a la pl_camara a una fraccion de su velocidad (parallax).
// En modo 'repetir' la imagen se coloca en mosaico, sin limite de ancho, y se desliza sin costuras.
PROCESS pl_fondo(int mat, float aspecto)
PRIVATE int e; float c0; float ww; float a; float b; int seg[15]; int n; int i; float sw; float scroll; float m; int k;
BEGIN
    IF (pl_bg_rep)
        sw = pl_bg_alto * pl_bgk * aspecto;                         // ancho de cada copia, respetando la proporcion
        IF (sw < 8.0) sw = 8.0; END
        n = (2.0 * 30.0) / sw + 3; IF (n > 15) n = 15; END
        FOR (i = 0; i < n; i = i + 1)
            seg[i] = g3d_entity_spawn(scene, 0, 0, 0, 0);
            g3d_entity_set_mesh(seg[i], pl_box_mesh); g3d_entity_set_material(seg[i], mat);
            g3d_entity_set_scale(seg[i], sw, pl_bg_alto * pl_bgk, 1.0);
        END
        LOOP
            scroll = pl_camx * (1.0 - pl_bgmove); IF (scroll < 0.0) scroll = 0.0; END
            k = scroll / sw; m = scroll - sw * k;       // desplazamiento dentro de una copia
            FOR (i = 0; i < n; i = i + 1)
                g3d_entity_set_position(seg[i], pl_camx + (i - n / 2) * sw - m, pl_camy + 1.0, pl_bgz);
            END
            FRAME;
        END
    ELSE
        a = 20.0; b = pl_lw * PL_TS - 20.0; IF (b < a) b = a; END
        c0 = (a + b) / 2.0;
        ww = (2.0 * 28.0 + (b - a) * (1.0 - pl_bgmove)) * pl_bgk;
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, mat);
        g3d_entity_set_scale(e, ww, 36.0 * pl_bgk, 1.0);
        LOOP
            g3d_entity_set_position(e, c0 + (pl_camx - c0) * pl_bgmove, pl_camy + 1.0, pl_bgz);
            FRAME;
        END
    END
END

// Botones de pisar, dianas y puertas: comparten CANALES (1..15). Una puerta del canal N se abre mientras el canal N este activo:
// un boton de ese canal pisado, o una diana de ese canal disparada (alterna cada vez que le das).
PROCESS pl_circuitos()
PRIVATE int q; int i; int k; int c; int r; int t; int ch; int want; int over; float pr; float dx; float dy; float dist;
BEGIN
    LOOP
        FOR (k = 1; k < 160; k = k + 1) pl_pressed[k] = 0; END
        FOR (i = 0; i < pl_nbt; i = i + 1)
            c = pl_bt_c[i]; r = pl_bt_r[i]; t = pl_bt_t[i];
            IF (abs(pl_hcx - (c * PL_TS + 1.0)) < 1.2 AND pl_hy < r * PL_TS + 0.9 AND pl_hy + 0.2 > r * PL_TS - 0.3)
                q = pl_bt_ch[i];
                pl_pressed[q] = 1;
                IF (pl_t_hold[t] < 0.0) pl_toggled[q] = 1; ELSE IF (pl_t_hold[t] * 60.0 > pl_hold[q]) pl_hold[q] = pl_t_hold[t] * 60.0; END END
                g3d_entity_set_color(pl_bt_ent[i], 120, 255, 140);
            ELSE
                g3d_entity_set_color(pl_bt_ent[i], 255, 170, 150);
            END
        END
        FOR (k = 1; k < 160; k = k + 1)
            IF (pl_hold[k] > 0 AND NOT pl_pressed[k]) pl_hold[k] = pl_hold[k] - 1; END
            IF (pl_pressed[k] OR pl_toggled[k] OR pl_hold[k] > 0) pl_chan[k] = 1; ELSE pl_chan[k] = 0; END
        END
        FOR (i = 0; i < pl_ntg; i = i + 1)
            IF (pl_chan[pl_tg_ch[i]]) g3d_entity_set_color(pl_tg_ent[i], 120, 255, 140); ELSE g3d_entity_set_color(pl_tg_ent[i], 255, 170, 150); END
        END
        FOR (i = 0; i < pl_ndr; i = i + 1)
            c = pl_dr_c[i]; r = pl_dr_r[i]; t = pl_dr_t[i]; ch = pl_t_door[t]; want = pl_chan[ch]; pr = pl_dr_pr[i];
            IF (pl_dr_ex[i]) want = 0; FOR (q = 0; q < pl_dr_ns[i]; q = q + 1) IF (pl_chan[pl_dr_sc[i * 8 + q]]) want = 1; END END END
            dist = pl_dr_len[i] * PL_TS - 0.1;                                     // se aparta todo lo que mide el grupo: deja el paso libre
            over = 0;
            IF (abs(pl_hcx - (c * PL_TS + 1.0)) < 1.55 + pl_hhalf - 0.65 AND pl_hy < (r + 1) * PL_TS AND pl_hy + 2.55 > r * PL_TS) over = 1; END
            IF (want)
                pr = pr + 6.0 * 0.0166667 / dist; IF (pr > 1.0) pr = 1.0; END
            ELSE
                IF (NOT over) pr = pr - 6.0 * 0.0166667 / dist; IF (pr < 0.0) pr = 0.0; END END      // no se cierra encima del heroe
            END
            pl_dr_pr[i] = pr;
            dx = 0.0; dy = 0.0;
            IF (pl_t_dir[t] == 2) dy = -pr * dist; ELSE IF (pl_t_dir[t] == 3) dx = -pr * dist; ELSE IF (pl_t_dir[t] == 4) dx = pr * dist; ELSE dy = pr * dist; END END END
            k = pl_dr_pl[i];                                                    // el solido dinamico: te lleva encima y no se atraviesa
            pl_pldx[k] = (c * PL_TS + 1.0 + dx) - pl_plx[k]; pl_pldy[k] = (r * PL_TS + 1.0 + dy) - pl_ply[k];
            pl_plx[k] = c * PL_TS + 1.0 + dx; pl_ply[k] = r * PL_TS + 1.0 + dy;
            IF (pr > 0.0) pl_lvl[r * pl_lw + c] = 0; ELSE pl_lvl[r * pl_lw + c] = t; END      // cerrada del todo: tambien cuenta para los enemigos
            g3d_entity_set_position(pl_dr_ent[i], pl_plx[k], pl_ply[k], -1.35);
        END
        FRAME;
    END
END

// Enemigos en 3D: cada enemigo carga su copia del modelo (cada uno con su animacion) y la anima; el sprite se deja diminuto.
FUNCTION int pl_em_new(int tp)
PRIVATE int s; int i; int sm; int st; int mat; int e; float h; int m;
BEGIN
    IF (pl_en_model[tp] == "" OR pl_em_n >= 96) RETURN -1; END
    IF (NOT fexists(pl_en_model[tp])) RETURN -1; END
    g3d_gltf_set_recenter(0);
    m = g3d_load_gltf(pl_en_model[tp]);
    g3d_gltf_set_recenter(1);
    IF (m < 0) RETURN -1; END
    s = pl_em_n; pl_em_n = pl_em_n + 1;
    h = g3d_model_height(m); IF (h <= 0.0) h = 1.0; END
    pl_em_sc[s] = pl_en_h[tp] * 1.1 / h;
    pl_em_np[s] = g3d_model_submesh_count(m); IF (pl_em_np[s] > 4) pl_em_np[s] = 4; END
    FOR (i = 0; i < pl_em_np[s]; i = i + 1)
        sm = g3d_model_submesh(m, i); st = g3d_model_submesh_texture(m, i);
        mat = g3d_material_create(); g3d_material_set_color(mat, 1.0, 1.0, 1.0, 1.0);
        g3d_material_set_metallic(mat, 0.0); g3d_material_set_roughness(mat, 0.85);
        IF (st >= 0) g3d_material_set_texture(mat, 0, st); END
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, sm); g3d_entity_set_material(e, mat); g3d_entity_set_scale(e, 0.0, 0.0, 0.0);
        pl_em_p[s * 4 + i] = e;
    END
    pl_em_m[s] = m; pl_em_a[s] = -1; pl_em_yaw[s] = 0.0;
    RETURN s;
END

FUNCTION pl_em_set(int s, float px, float py, float pz, float yaw, int anim, int show)
PRIVATE int i; int lp; float d; float dt = 0.0166667; float sc;
BEGIN
    IF (s < 0) RETURN; END
    IF (NOT show)
        FOR (i = 0; i < pl_em_np[s]; i = i + 1) g3d_entity_set_scale(pl_em_p[s * 4 + i], 0.0, 0.0, 0.0); END
        RETURN;
    END
    d = yaw - pl_em_yaw[s];
    WHILE (d > 180.0) d = d - 360.0; END
    WHILE (d < -180.0) d = d + 360.0; END
    IF (pl_em_a[s] < 0) d = d + 0.0; pl_em_yaw[s] = yaw; ELSE pl_em_yaw[s] = pl_em_yaw[s] + d * 0.25; END
    IF (anim != pl_em_a[s]) pl_em_pa[s] = pl_em_a[s]; pl_em_pt[s] = pl_em_t[s]; pl_em_a[s] = anim; pl_em_t[s] = 0.0; pl_em_w[s] = 0.0; IF (pl_em_pa[s] < 0) pl_em_w[s] = 1.0; END END
    lp = 1; IF (anim >= 2) lp = 0; END
    pl_em_t[s] = pl_em_t[s] + dt;
    IF (pl_em_w[s] < 1.0)
        pl_em_w[s] = pl_em_w[s] + 0.18; IF (pl_em_w[s] > 1.0) pl_em_w[s] = 1.0; END
        g3d_model_animate_blend(pl_em_m[s], pl_em_pa[s], pl_em_pt[s], 1, pl_em_t[s], pl_em_w[s], lp);
    ELSE
        g3d_model_animate(pl_em_m[s], anim, pl_em_t[s], lp);
    END
    sc = pl_em_sc[s];
    FOR (i = 0; i < pl_em_np[s]; i = i + 1)
        g3d_entity_set_position(pl_em_p[s * 4 + i], px, py, pz);
        g3d_entity_set_rotation(pl_em_p[s * 4 + i], 0.0, pl_em_yaw[s] * 1000.0, 0.0);
        g3d_entity_set_scale(pl_em_p[s * 4 + i], sc, sc, sc);
    END
END

// El heroe como modelo 3D: los procesos del heroe (nivel y pasillo) solo dejan su estado en pl_hm_*; este proceso lo dibuja y anima
// numero de animacion del modelo para un estado (0 quieto ... 5 muerte); si el modelo no la tiene (-1) se usa la de quieto
FUNCTION int pl_hm_anim_of(int st)
BEGIN
    IF (st < 0 OR st > 6) RETURN pl_hm_map[0]; END
    IF (pl_hm_map[st] < 0) RETURN pl_hm_map[0]; END
    RETURN pl_hm_map[st];
END

FUNCTION pl_hm_put(float px, float py, float pz, float yaw, int anim, int blink)
BEGIN
    pl_hm_x = px; pl_hm_y = py; pl_hm_z = pz; pl_hm_yaw = yaw; pl_hm_anim = anim; pl_hm_blink = blink; pl_hm_show = 1;
END

PROCESS pl_hmodelo()
PRIVATE int i; int sm; int st; int mat; int e; float h; float t; float pt; float w; float cur; float d; int an; int pan; int lp; float dt = 0.0166667; int blink_on;
BEGIN
    g3d_gltf_set_recenter(0);                       // el origen del modelo son sus pies
    pl_hm_model = g3d_load_gltf(pl_hero_model);
    g3d_gltf_set_recenter(1);
    IF (pl_hm_model < 0) say("ERROR: no se carga el modelo del heroe " + pl_hero_model); pl_hm_on = 0; RETURN; END
    h = g3d_model_height(pl_hm_model); IF (h <= 0.0) h = 1.0; END
    pl_hm_scale = pl_hero_hh / h;
    pl_hm_n = g3d_model_submesh_count(pl_hm_model); IF (pl_hm_n > 7) pl_hm_n = 7; END
    FOR (i = 0; i < pl_hm_n; i = i + 1)
        sm = g3d_model_submesh(pl_hm_model, i); st = g3d_model_submesh_texture(pl_hm_model, i);
        mat = g3d_material_create(); g3d_material_set_color(mat, 1.0, 1.0, 1.0, 1.0);
        g3d_material_set_metallic(mat, 0.0); g3d_material_set_roughness(mat, 0.85);
        IF (st >= 0) g3d_material_set_texture(mat, 0, st); END
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, sm); g3d_entity_set_material(e, mat);
        g3d_entity_set_scale(e, 0.0, 0.0, 0.0);
        pl_hm_part[i] = e;
    END
    pl_hm_on = 1; an = -1; cur = pl_hm_yaw;
    LOOP
        IF (pl_hm_show)
            d = pl_hm_yaw - cur;
            WHILE (d > 180.0) d = d - 360.0; END
            WHILE (d < -180.0) d = d + 360.0; END
            cur = cur + d * 0.3;
            IF (pl_hm_anim != an) pan = an; pt = t; an = pl_hm_anim; t = 0.0; w = 0.0; IF (pan < 0) w = 1.0; END END
            lp = 1; IF (an == 3 OR an == 5 OR an == 6) lp = 0; END
            t = t + dt;
            IF (w < 1.0)
                w = w + 0.18; IF (w > 1.0) w = 1.0; END
                g3d_model_animate_blend(pl_hm_model, pl_hm_anim_of(pan), pt, 1, t, w, lp);
            ELSE
                g3d_model_animate(pl_hm_model, pl_hm_anim_of(an), t, lp);
            END
            blink_on = (pl_hm_blink AND (pl_hm_blink % 2) == 1);
            FOR (i = 0; i < pl_hm_n; i = i + 1)
                g3d_entity_set_position(pl_hm_part[i], pl_hm_x, pl_hm_y, pl_hm_z);
                g3d_entity_set_rotation(pl_hm_part[i], 0.0, (cur + pl_hm_yawoff) * 1000.0, 0.0);
                IF (blink_on) g3d_entity_set_scale(pl_hm_part[i], 0.0, 0.0, 0.0); ELSE g3d_entity_set_scale(pl_hm_part[i], pl_hm_scale, pl_hm_scale, pl_hm_scale); END
            END
        ELSE
            FOR (i = 0; i < pl_hm_n; i = i + 1) g3d_entity_set_scale(pl_hm_part[i], 0.0, 0.0, 0.0); END
        END
        FRAME;
    END
END

// Las superficies de agua, lava... ondulan un poco
PROCESS pl_liquidos()
PRIVATE int i; int tic; float w;
BEGIN
    LOOP
        tic = tic + 1;
        FOR (i = 0; i < pl_liq_n; i = i + 1)
            w = sin(tic * 3000 + pl_liq_x[i] * 9000.0);
            g3d_entity_set_position(pl_liq_ent[i], pl_liq_x[i], pl_liq_y[i] + w * 0.06, -1.35);
        END
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  Plataforma que se mueve: un proceso por celda. Solo se pisa desde arriba.
// ---------------------------------------------------------------------------
PROCESS pl_plataforma(int pn, int tt, float ox, float oy)
PRIVATE
    int q; int want; int e; float p; int d = 1; float lim; float nx; float ny; float dt = 0.0166667; int esp;
BEGIN
    lim = pl_t_dist[tt] * PL_TS;
    e = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_mat[tt]);
    g3d_entity_set_scale(e, PL_TS, 0.6, 3.3);
    pl_plx[pn] = ox; pl_ply[pn] = oy; pl_plw[pn] = 1.0; pl_plh[pn] = 0.3;
    LOOP
        IF (pl_pl_ex[pn])                                      // con enlace: va al extremo mientras algun boton/diana que la nombra este activo, y vuelve cuando no
            want = 0; FOR (q = 0; q < pl_pl_ns[pn]; q = q + 1) IF (pl_chan[pl_pl_sc[pn * 8 + q]]) want = 1; END END
            IF (want) d = 1; ELSE d = -1; END
            p = p + d * pl_t_spd[tt] * dt;
            IF (p > lim) p = lim; END
            IF (p < 0.0) p = 0.0; END
        ELSE IF (pl_t_mode[tt] == 1)                           // al pisar: sube mientras vas encima, y vuelve cuando bajas
            IF (pl_hride == pn) d = 1; ELSE d = -1; END
            p = p + d * pl_t_spd[tt] * dt;
            IF (p > lim) p = lim; END
            IF (p < 0.0) p = 0.0; END
        ELSE
            IF (esp > 0)
                esp = esp - 1;                              // pausa en cada extremo
            ELSE
                p = p + d * pl_t_spd[tt] * dt;
                IF (p > lim) p = lim; d = -1; esp = pl_t_wait[tt] * 60.0; END
                IF (p < 0.0) p = 0.0; d = 1; esp = pl_t_wait[tt] * 60.0; END
            END
        END END
        IF (pl_t_axis[tt] == 1) nx = ox + p; ny = oy; ELSE nx = ox; ny = oy + p; END
        pl_pldx[pn] = nx - pl_plx[pn]; pl_pldy[pn] = ny - pl_ply[pn];
        pl_plx[pn] = nx; pl_ply[pn] = ny;
        g3d_entity_set_position(e, pl_plx[pn], pl_ply[pn], -1.35);
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  Liquidos: las celdas contiguas del mismo liquido se funden en rectangulos grandes (sin juntas),
//  con una sola lamina de superficie por tramo que ondula.
// ---------------------------------------------------------------------------
FUNCTION pl_build_liquids()
PRIVATE int r; int c; int t; int w; int h; int ok; int i; int j; int e; int c0; int cs;
BEGIN
    FOR (r = 0; r < pl_lh; r = r + 1)
        FOR (c = 0; c < pl_lw; c = c + 1)
            t = pl_lvl[r * pl_lw + c];
            IF (t > 0 AND pl_liqmark[r * pl_lw + c] == 0)
                IF (pl_t_liq[t] > 0 AND pl_t_tex[t] != "")
                    // ancho: hasta donde siga el mismo liquido
                    w = 1;
                    WHILE (c + w < pl_lw AND pl_lvl[r * pl_lw + c + w] == t AND pl_liqmark[r * pl_lw + c + w] == 0) w = w + 1; END
                    // alto: mientras la fila de encima sea igual de ancha y del mismo liquido
                    h = 1; ok = 1;
                    WHILE (ok AND r + h < pl_lh)
                        FOR (i = 0; i < w; i = i + 1)
                            IF (pl_lvl[(r + h) * pl_lw + c + i] != t OR pl_liqmark[(r + h) * pl_lw + c + i] != 0) ok = 0; END
                        END
                        IF (ok) h = h + 1; END
                    END
                    FOR (j = 0; j < h; j = j + 1) FOR (i = 0; i < w; i = i + 1) pl_liqmark[(r + j) * pl_lw + c + i] = 1; END END
                    // el cuerpo: un solo prisma translucido
                    e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                    g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
                    g3d_entity_set_scale(e, w * PL_TS, h * PL_TS, 3.3);
                    g3d_entity_set_position(e, (c + w / 2.0) * PL_TS, (r + h / 2.0) * PL_TS, -1.35);
                    g3d_entity_set_blend(e, 1); g3d_entity_set_alpha(e, pl_t_alpha[t]);
                    // la superficie: una lamina por tramo continuo sin liquido encima
                    cs = -1;
                    FOR (i = 0; i <= w; i = i + 1)
                        IF (i < w AND pl_tile_at((c + i) * PL_TS + 1.0, (r + h) * PL_TS + 0.1) != t)
                            IF (cs < 0) cs = i; END
                        ELSE
                            IF (cs >= 0 AND pl_liq_n < 2047)
                                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                                g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
                                g3d_entity_set_scale(e, (i - cs) * PL_TS, 0.2, 3.3);
                                g3d_entity_set_blend(e, 1); g3d_entity_set_alpha(e, pl_t_alpha[t] + 60);
                                pl_liq_ent[pl_liq_n] = e; pl_liq_x[pl_liq_n] = (c + (cs + i) / 2.0) * PL_TS; pl_liq_y[pl_liq_n] = (r + h) * PL_TS - 0.1; pl_liq_n = pl_liq_n + 1;
                            END
                            cs = -1;
                        END
                    END
                END
            END
        END
    END
END

// un bloque 3D en la capa indicada: 0 plano jugable, 1 fondo (detras), 2 primer plano (delante)
FUNCTION pl_tile_cell(int t, int c, int r, int layer)
PRIVATE int e; int i; float zz; float sz; float sc; int lay0;
BEGIN
    IF (pl_t_tex[t] == "") RETURN; END
    sc = 1.0; lay0 = (layer == 0 AND NOT pl_vis);   // en un plano que solo se ve (pl_vis) no hay puertas, botones, liquidos ni plataformas: todo es decorado
    zz = -1.35; sz = 3.3;                            // bloques de z -3 a +0.3: casi a ras del plano del heroe (menos solape por la perspectiva)
    IF (layer > 0) zz = pl_lz[layer]; sz = pl_lsz[layer]; sc = pl_ls[layer]; END   // capas de decorado: z, grosor y escala propios
    zz = zz + pl_zoff;
    IF (layer == 0 AND pl_t_portal[t] != 0) zz = -2.8 + pl_zoff; sz = 0.4; END   // el portal es una lamina al fondo: el heroe se ve delante
    IF (pl_t_door[t] > 0 AND lay0) RETURN; END        // las puertas se crean aparte (se mueven)
    IF (pl_t_ramp[t] > 0 AND lay0)                    // rampa: una cuna maciza (45 grados), con los laterales texturizados
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_wedge_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
        g3d_entity_set_scale(e, PL_TS, PL_TS, sz);
        IF (pl_t_ramp[t] == 2) g3d_entity_set_rotation(e, 0.0, 180000.0, 0.0); END
        g3d_entity_set_position(e, c * PL_TS + 1.0, r * PL_TS + 1.0, zz);
        RETURN;
    END
    IF (pl_t_btn[t] > 0 AND lay0)                      // boton: una placa baja en el suelo
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
        g3d_entity_set_scale(e, PL_TS * 0.8, 0.3, 1.6);
        g3d_entity_set_position(e, c * PL_TS + 1.0, r * PL_TS + 0.15, 0.0);
        IF (pl_nbt < 63) pl_bt_ent[pl_nbt] = e; pl_bt_c[pl_nbt] = c; pl_bt_r[pl_nbt] = r; pl_bt_t[pl_nbt] = t; pl_bt_ch[pl_nbt] = pl_lk_ch(0, c, pl_lh - 1 - r, pl_t_btn[t]); pl_nbt = pl_nbt + 1; END
        RETURN;
    END
    IF (pl_t_liq[t] > 0 AND lay0) RETURN; END      // los pl_liquidos se construyen aparte, como cuerpos continuos (pl_build_liquids)
    IF (pl_t_shape[t] == 1)                            // pinchos: tres diamantes
        FOR (i = 0; i < 3; i = i + 1)
            e = g3d_entity_spawn(scene, 0, 0, 0, 0);
            g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
            g3d_entity_set_scale(e, 0.75, 0.75, 0.75);
            g3d_entity_set_position(e, c * PL_TS + 0.35 + i * 0.65, r * PL_TS + 0.55, pl_zoff);
            g3d_entity_set_rotation(e, 45000.0, 0.0, 45000.0);
        END
        RETURN;
    END
    e = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
    g3d_entity_set_scale(e, PL_TS * sc, PL_TS * sc, sz);
    g3d_entity_set_position(e, (c * PL_TS + 1.0) * sc, (r * PL_TS + 1.0) * sc, zz);
    IF (pl_mundo AND (layer == 6 OR layer == 1 OR (lay0 AND pl_t_solid[t]))) g3d_entity_set_collider(e, 1); END      // en un mundo 3D, las paredes del mapa se atraviesan por la puerta, no por la pared
    IF (layer == 2 OR layer == 5) g3d_entity_set_xray(e, 1); END      // los primeros planos se vuelven translucidos cuando tapan al heroe
    IF (lay0 AND pl_t_tgt[t] > 0 AND pl_ntg < 63) pl_tg_ent[pl_ntg] = e; pl_tg_t[pl_ntg] = t; pl_tg_ch[pl_ntg] = pl_lk_ch(0, c, pl_lh - 1 - r, pl_t_tgt[t]); pl_tg_c[pl_ntg] = c; pl_tg_r[pl_ntg] = r; pl_ntg = pl_ntg + 1; END      // diana
    IF (lay0 AND pl_t_matop[t] > 0 AND NOT pl_is_solid(c, r + 1))   // tapa superior con otra textura
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_t_matop[t]);
        g3d_entity_set_scale(e, PL_TS, 0.25, sz);
        g3d_entity_set_position(e, c * PL_TS + 1.0, r * PL_TS + PL_TS, zz);
    END
END

// ---------------------------------------------------------------------------
//  Escenario: lee el mapa (PNG; cada pixel es un bloque; R = plano jugable, G = fondo, B = primer plano,
//  el valor es el numero de tile del tileset, 0 = vacio) y crea los cubos con la textura de cada tile
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  Lectura de niveles: <nivel>.lvl (binario, todos los planos) o, si no existe, los PNG antiguos (<nivel>.png, .p1.png, .capas.png).
//  Se identifica el nivel por la ruta de su PNG (la de siempre); asi no cambia nada en las escenas ni en el .level.
// ---------------------------------------------------------------------------
FUNCTION int pl_lv_exists(string png, int plane)
PRIVATE string st; int h; int r;
BEGIN
    st = substr(png, 0, strlen(png) - 4);
    IF (fexists(st + ".lvl"))
        r = 0; h = g3d_lvl_load(st + ".lvl");
        IF (h != 0) IF (g3d_lvl_w(h, plane) > 0) r = 1; END g3d_lvl_free(h); END
        RETURN r;
    END
    IF (plane > 0) st = st + ".p" + itoa(plane); END
    RETURN fexists(st + ".png");
END

// abre un plano del nivel; sub 0 = capas 0..2 (R, G, B), sub 1 = capas 3..5 (decorado extra). Devuelve un manejador > 0, o 0
FUNCTION int pl_lv_open(string png, int plane, int sub)
PRIVATE string st; int s; int h; int g;
BEGIN
    s = 1; WHILE (s < 8 AND pl_lvk[s] != 0) s = s + 1; END
    IF (s >= 8) RETURN 0; END
    st = substr(png, 0, strlen(png) - 4);
    IF (fexists(st + ".lvl"))
        h = g3d_lvl_load(st + ".lvl");
        IF (h == 0) RETURN 0; END
        IF (g3d_lvl_w(h, plane) <= 0) g3d_lvl_free(h); RETURN 0; END
        pl_lvk[s] = 1; pl_lvi[s] = h;
    ELSE
        IF (sub == 2) RETURN 0; END                   // la capa Exterior solo existe en los .lvl
        IF (plane > 0) st = st + ".p" + itoa(plane); END
        IF (sub == 1) st = st + ".capas"; END
        IF (NOT fexists(st + ".png")) RETURN 0; END
        g = map_load(st + ".png");
        IF (g <= 0) RETURN 0; END
        pl_lvk[s] = 2; pl_lvi[s] = g;
    END
    pl_lvp[s] = plane; pl_lvs[s] = sub;
    RETURN s;
END

FUNCTION int pl_lv_w(int h)
BEGIN
    IF (pl_lvk[h] == 1) RETURN g3d_lvl_w(pl_lvi[h], pl_lvp[h]); END
    RETURN map_info(0, pl_lvi[h], G_WIDTH);
END

FUNCTION int pl_lv_h(int h)
BEGIN
    IF (pl_lvk[h] == 1) RETURN g3d_lvl_h(pl_lvi[h], pl_lvp[h]); END
    RETURN map_info(0, pl_lvi[h], G_HEIGHT);
END

// valor (0..255) de la capa ch (0..2) de la celda (x, y); y = 0 es la fila de ARRIBA del mapa
// ---- Enlaces entre objetos ---------------------------------------------------------------------------------------------------
// El bloque LNKS del .lvl guarda, por casilla, un NOMBRE y unos DESTINOS:  plano;x;y;nombre;destino1,destino2   (y = fila contando desde arriba)
//   puerta / ascensor: nombre.   boton / diana: destinos = nombres de las puertas que abre.   portal a otro nivel: destino = escena/puerta.
// Un boton o una diana con enlace abre SOLO las puertas que nombra; sin enlace usa el canal del tile (como antes).
FUNCTION int pl_lk_load(int g, int plane, int lv)
PRIVATE string txt; string ln[200]; string f[6]; int n; int i; int m;
BEGIN
    IF (pl_lvk[g] != 1) RETURN 0; END
    txt = g3d_lvl_links(pl_lvi[g]);
    IF (strlen(txt) < 3) RETURN 0; END
    n = split("\n", txt, &ln[0], 200);
    FOR (i = 0; i < n; i = i + 1)
        m = split(";", ln[i], &f[0], 6);
        IF (m >= 4 AND atoi(f[0]) == plane AND pl_lk_n < 139)
            pl_lk_x[pl_lk_n] = atoi(f[1]); pl_lk_y[pl_lk_n] = atoi(f[2]); pl_lk_name[pl_lk_n] = trim(f[3]); pl_lk_tgt[pl_lk_n] = ""; pl_lk_lv[pl_lk_n] = lv;
            IF (m >= 5) pl_lk_tgt[pl_lk_n] = trim(f[4]); END
            pl_lk_n = pl_lk_n + 1;
        END
    END
    RETURN pl_lk_n;
END

FUNCTION int pl_lk_find(int lv, int px, int py)
PRIVATE int i;
BEGIN
    FOR (i = 0; i < pl_lk_n; i = i + 1) IF (pl_lk_lv[i] == lv AND pl_lk_x[i] == px AND pl_lk_y[i] == py) RETURN i; END END
    RETURN -1;
END

FUNCTION int pl_lk_in(string list, string name)
PRIVATE string t[15]; int n; int i;
BEGIN
    IF (name == "" OR list == "") RETURN 0; END
    n = split(",", list, &t[0], 15);
    FOR (i = 0; i < n; i = i + 1) IF (trim(t[i]) == name) RETURN 1; END END
    RETURN 0;
END

// canal de un boton o una diana: el suyo propio si tiene enlace (16 + su numero), o el del tile
FUNCTION int pl_lk_ch(int lv, int px, int py, int legacy)
PRIVATE int e;
BEGIN
    e = pl_lk_find(lv, px, py);
    IF (e >= 0 AND (pl_lk_name[e] != "" OR pl_lk_tgt[e] != "")) RETURN 16 + e; END
    RETURN legacy;
END

// puerta con nombre (en cualquiera de las casillas del rectangulo): los canales de los botones/dianas que la nombran. Devuelve 1 si va por enlaces.
FUNCTION int pl_lk_door(int lv, int c0, int c1, int y0, int y1)
PRIVATE int px; int py; int e; int e2; string nm;
BEGIN
    pl_lk_ns = 0; nm = "";
    FOR (py = y0; py <= y1; py = py + 1)
        FOR (px = c0; px <= c1; px = px + 1)
            e = pl_lk_find(lv, px, py);
            IF (e >= 0 AND nm == "" AND pl_lk_name[e] != "") nm = pl_lk_name[e]; END
        END
    END
    IF (nm == "") RETURN 0; END
    FOR (e2 = 0; e2 < pl_lk_n; e2 = e2 + 1)
        IF (pl_lk_lv[e2] == lv AND pl_lk_in(pl_lk_tgt[e2], nm) AND pl_lk_ns < 8) pl_lk_s[pl_lk_ns] = 16 + e2; pl_lk_ns = pl_lk_ns + 1; END
    END
    RETURN 1;
END

FUNCTION int pl_lv_get(int h, int x, int y, int ch)
PRIVATE int r; int g; int b; int a;
BEGIN
    IF (pl_lvk[h] == 1) RETURN g3d_lvl_cell(pl_lvi[h], pl_lvp[h], pl_lvs[h] * 3 + ch, x, y); END
    rgba_get(map_get_pixel(0, pl_lvi[h], x, y), &r, &g, &b, &a);
    IF (ch == 0) RETURN r; END
    IF (ch == 1) RETURN g; END
    RETURN b;
END

FUNCTION pl_lv_close(int h)
BEGIN
    IF (h <= 0 OR h >= 8 OR pl_lvk[h] == 0) RETURN; END
    IF (pl_lvk[h] == 1) g3d_lvl_free(pl_lvi[h]); ELSE map_unload(0, pl_lvi[h]); END
    pl_lvk[h] = 0;
END

// nombre del fichero de un plano: <nivel>.png (plano 0), <nivel>.p1.png... y con otro sufijo (".capas.png")
FUNCTION string pl_planefile(int p, string suf)
PRIVATE string b;
BEGIN
    b = substr(pl_levelfile, 0, strlen(pl_levelfile) - 4);
    IF (p > 0) b = b + ".p" + itoa(p); END
    RETURN b + suf;
END

// planos que NO son el activo: solo se ven (sin colision ni mecanismos), cada uno mas al fondo (pl_pd por plano)
FUNCTION pl_build_visual(int p)
PRIVATE int g; int c; int r; int py; int cr; int cg; int cb; int ca; int w; int h;
BEGIN
    g = pl_lv_open(pl_levelfile, p, 0);
    IF (g <= 0) RETURN; END
    pl_vis = 1; pl_zoff = -(p - pl_plane) * pl_pd;
    w = pl_lv_w(g); h = pl_lv_h(g);
    FOR (py = 0; py < h; py = py + 1)
        r = h - 1 - py;
        FOR (c = 0; c < w; c = c + 1)
            cr = pl_lv_get(g, c, py, 0); cg = pl_lv_get(g, c, py, 1); cb = pl_lv_get(g, c, py, 2);
            IF (cg > 0 AND cg <= pl_nt) pl_tile_cell(cg, c, r, 1); END
            IF (cr > 0 AND cr <= pl_nt AND pl_t_mark[cr] == 0) pl_tile_cell(cr, c, r, 0); END
            IF (cb > 0 AND cb <= pl_nt) pl_tile_cell(cb, c, r, 2); END
        END
    END
    pl_lv_close(g);
    pl_vis = 0; pl_zoff = 0.0;
END

// del mapa al mundo (x, z del mapa -> x, z del mundo), con el giro y la posicion de su raiz
FUNCTION float pl_wxof(float lx, float lz)
BEGIN
    RETURN pl_wx + lx * cos(pl_wyaw * 1000.0) + lz * sin(pl_wyaw * 1000.0);
END

FUNCTION float pl_wzof(float lx, float lz)
BEGIN
    RETURN pl_wz - lx * sin(pl_wyaw * 1000.0) + lz * cos(pl_wyaw * 1000.0);
END

// Colision del mapa para el jugador 3D del mundo: cajas (en coordenadas del mundo) que cubren las paredes, el fondo y la fachada. Se juntan las
// celdas contiguas de una fila en UNA caja (el motor admite pocas cajas).
FUNCTION int pl_caja_run(int c0, int c1, int r, float z0, float z1)
PRIVATE float xs[4]; float zs[4]; float x0; float x1; float zz0; float zz1; int i;
BEGIN
    xs[0] = pl_wxof(c0 * PL_TS, z0); zs[0] = pl_wzof(c0 * PL_TS, z0);
    xs[1] = pl_wxof((c1 + 1) * PL_TS, z0); zs[1] = pl_wzof((c1 + 1) * PL_TS, z0);
    xs[2] = pl_wxof(c0 * PL_TS, z1); zs[2] = pl_wzof(c0 * PL_TS, z1);
    xs[3] = pl_wxof((c1 + 1) * PL_TS, z1); zs[3] = pl_wzof((c1 + 1) * PL_TS, z1);
    x0 = xs[0]; x1 = xs[0]; zz0 = zs[0]; zz1 = zs[0];
    FOR (i = 1; i < 4; i = i + 1)
        IF (xs[i] < x0) x0 = xs[i]; END
        IF (xs[i] > x1) x1 = xs[i]; END
        IF (zs[i] < zz0) zz0 = zs[i]; END
        IF (zs[i] > zz1) zz1 = zs[i]; END
    END
    RETURN g3d_collider_add_box(x0, pl_wy + r * PL_TS, zz0, x1, pl_wy + (r + 1) * PL_TS, zz1);
END

FUNCTION pl_mundo_cajas()
PRIVATE int r; int c; int c0; int k;
BEGIN
    FOR (k = 0; k < 3; k = k + 1)                     // 0 plano jugable (solidos), 1 fondo, 2 fachada
        FOR (r = 0; r < pl_lh; r = r + 1)
            c = 0;
            WHILE (c < pl_lw)
                IF ((k == 0 AND pl_lvl[r * pl_lw + c] > 0 AND pl_t_solid[pl_lvl[r * pl_lw + c]]) OR (k == 1 AND pl_lvlb[r * pl_lw + c] > 0) OR (k == 2 AND pl_fcm[r * pl_lw + c] > 0))
                    c0 = c;
                    WHILE (c < pl_lw AND ((k == 0 AND pl_lvl[r * pl_lw + c] > 0 AND pl_t_solid[pl_lvl[r * pl_lw + c]]) OR (k == 1 AND pl_lvlb[r * pl_lw + c] > 0) OR (k == 2 AND pl_fcm[r * pl_lw + c] > 0))) c = c + 1; END
                    IF (k == 0) pl_caja_run(c0, c - 1, r, -3.0, 0.3); END
                    IF (k == 1) pl_caja_run(c0, c - 1, r, -5.0, -3.4); END
                    IF (k == 2) pl_caja_run(c0, c - 1, r, 1.2, 3.2); END
                ELSE
                    c = c + 1;
                END
            END
        END
    END
END

FUNCTION int pl_build_level()
PRIVATE
    string fn; int c; int r; int e; int i; int g; int col; int cr; int cg; int cb; int ca; int py; int t; int k; int j; int q;
BEGIN
    IF (NOT pl_load_tileset(pl_tilesetfile)) RETURN 0; END
    g = pl_lv_open(pl_levelfile, pl_plane, 0);
    IF (g <= 0) say("ERROR: no se encuentra el nivel " + pl_levelfile); RETURN 0; END
    pl_lk_load(g, pl_plane, 0);
    pl_lw = pl_lv_w(g); pl_lh = pl_lv_h(g);
    IF (pl_lw * pl_lh > 79999) say("ERROR: el nivel es demasiado grande (max 79999 bloques)"); RETURN 0; END
    say("nivel " + pl_levelfile + ": " + itoa(pl_lw) + " x " + itoa(pl_lh) + " bloques");

    pl_box_mesh = g3d_primitive_cube(); pl_wedge_mesh = g3d_primitive_wedge();
    IF (pl_nt >= 1 AND pl_t_tex[1] != "") pl_mat_metal = pl_make_mat(pl_tdir + pl_t_tex[1], 0.55, 0.25); ELSE pl_mat_metal = pl_make_mat("", 0.55, 0.25); END    // la textura del primer tile sirve para mastiles y pl_columnas
    pl_mat_bg = pl_make_mat("", 0.9, 0.0);
    g3d_material_set_color(pl_mat_bg, 0.10, 0.12, 0.20, 1.0);
    pl_mat_flag = pl_make_mat("", 0.6, 0.0);   g3d_material_set_color(pl_mat_flag, 0.9, 0.15, 0.15, 1.0);
    pl_mat_flagon = pl_make_mat("", 0.6, 0.0); g3d_material_set_color(pl_mat_flagon, 0.2, 0.95, 0.3, 1.0);

    // --- leer la imagen: la fila de arriba de la imagen es la de arriba del nivel ---
    pl_arr_ok = 0;
    IF (pl_arr_door != "")                                                  // llegas a una puerta con nombre: apareces en ella
        FOR (q = 0; q < pl_lk_n; q = q + 1)
            IF (pl_lk_lv[q] == 0 AND pl_lk_name[q] == pl_arr_door AND NOT pl_arr_ok) pl_arr_x = pl_lk_x[q] * PL_TS + 1.0; pl_arr_y = (pl_lh - 1 - pl_lk_y[q]) * PL_TS; pl_arr_ok = 1; END
        END
        pl_arr_door = "";
    END
    FOR (py = 0; py < pl_lh; py = py + 1)
        r = pl_lh - 1 - py;
        FOR (c = 0; c < pl_lw; c = c + 1)
            cr = pl_lv_get(g, c, py, 0); cg = pl_lv_get(g, c, py, 1); cb = pl_lv_get(g, c, py, 2);
            IF (cr > 0 AND cr <= pl_nt AND pl_t_portal[cr] == 2 AND pl_arr_from != "" AND pl_t_dest[cr] == pl_arr_from AND NOT pl_arr_ok) pl_arr_x = c * PL_TS + 1.0; pl_arr_y = r * PL_TS; pl_arr_ok = 1; END
            IF (cr > pl_nt) cr = 0; END
            IF (cg > pl_nt) cg = 0; END
            IF (cb > pl_nt) cb = 0; END
            pl_lvlb[r * pl_lw + c] = cg; pl_lvlf[r * pl_lw + c] = cb;
            t = cr;
            IF (t > 0)
                IF (pl_t_mark[t] == 1) IF (NOT pl_cambio AND NOT (pl_ret_on AND pl_ret_file == pl_levelfile)) pl_startx = c * PL_TS + 1.0; pl_starty = r * PL_TS; END t = 0;
                ELSE IF (pl_t_mark[t] == 2 OR pl_t_mark[t] == 3)
                    IF (pl_nsp < 255) pl_spx[pl_nsp] = c * PL_TS + 1.0; pl_spy[pl_nsp] = r * PL_TS; pl_spt[pl_nsp] = pl_t_mark[t]; pl_nsp = pl_nsp + 1; END
                    t = 0;
                ELSE IF (pl_t_mark[t] == 4)
                    IF (pl_ncp < 7) pl_cpx[pl_ncp] = c * PL_TS + 1.0; pl_cpy[pl_ncp] = r * PL_TS; pl_ncp = pl_ncp + 1; END
                    t = 0;
                ELSE IF (pl_t_axis[t] > 0)                      // plataforma que se mueve
                    IF (pl_npl < 47)
                        pl_pl_ex[pl_npl] = pl_lk_door(0, c, c, pl_lh - 1 - r, pl_lh - 1 - r); pl_pl_ns[pl_npl] = pl_lk_ns;
                        FOR (q = 0; q < pl_lk_ns; q = q + 1) pl_pl_sc[pl_npl * 8 + q] = pl_lk_s[q]; END
                        pl_plataforma(pl_npl, t, c * PL_TS + 1.0, r * PL_TS + PL_TS - 0.3); pl_npl = pl_npl + 1;
                    END
                    t = 0;
                END END END END
            END
            pl_lvl[r * pl_lw + c] = t;
            IF (t > 0 AND pl_t_portal[t] == 4 AND pl_por_n == 0) pl_por_x = c * PL_TS + 1.0; pl_por_y = r * PL_TS; pl_por_n = 1; END      // la puerta del mapa al mundo (portal 'Salir al mundo 3D')
            IF (t > 0 AND pl_t_portal[t] == 3 AND (NOT pl_cor_found OR r * PL_TS < pl_cor_py)) pl_cor_px = c * PL_TS + 1.0; pl_cor_py = r * PL_TS; pl_cor_found = 1; END   // el pasillo se coloca detras del portal mas bajo
            IF (t > 0)
                IF (pl_t_door[t] > 0 AND pl_ndr < 63) pl_dr_t[pl_ndr] = t; pl_dr_c[pl_ndr] = c; pl_dr_r[pl_ndr] = r; pl_ndr = pl_ndr + 1; END
            END
        END
    END
    pl_lv_close(g);

    // --- capas de decorado extra (R = fondo medio, G = fondo lejano, B = primer plano cercano) ---
    i = 0;
    g = pl_lv_open(pl_levelfile, pl_plane, 1);
    IF (g > 0)
        FOR (py = 0; py < pl_lv_h(g) AND py < pl_lh; py = py + 1)
            r = pl_lh - 1 - py;
            FOR (c = 0; c < pl_lv_w(g) AND c < pl_lw; c = c + 1)
                cr = pl_lv_get(g, c, py, 0); cg = pl_lv_get(g, c, py, 1); cb = pl_lv_get(g, c, py, 2);
                IF (cr > 0 AND cr <= pl_nt) pl_tile_cell(cr, c, r, 3); END
                IF (cg > 0 AND cg <= pl_nt) pl_tile_cell(cg, c, r, 4); END
                IF (cb > 0 AND cb <= pl_nt) pl_tile_cell(cb, c, r, 5); END
            END
        END
        pl_lv_close(g);
    END

    // --- capa 'Decorado': columnas de tiles normales (ps_deco_2d, en pasillo.prg) ---
    g = pl_lv_open(pl_levelfile, pl_plane, 2);
    IF (g > 0) ps_deco_2d(g, pl_lw, pl_lh); pl_lv_close(g); END

    // --- EXTERIOR (fachada): solo cuando el mapa esta colocado en un mundo 3D; se ve desde fuera y se esconde al entrar (pl_mundo_entrar / pl_mundo_salir) ---
    IF (pl_mundo)
        g = pl_lv_open(pl_levelfile, pl_plane, 2);
        IF (g > 0)
            pl_ext_root = g3d_entity_spawn(scene, 0, 0, 0, 0);          // cuelga del mapa: moviendola se esconde toda la fachada de golpe
            g3d_set_spawn_parent(pl_ext_root);
            FOR (py = 0; py < pl_lv_h(g) AND py < pl_lh; py = py + 1)
                r = pl_lh - 1 - py;
                FOR (c = 0; c < pl_lv_w(g) AND c < pl_lw; c = c + 1)
                    cr = pl_lv_get(g, c, py, 0);
                    IF (cr > 0 AND cr <= pl_nt) pl_tile_cell(cr, c, r, 6); pl_fcm[r * pl_lw + c] = 1; END
                END
            END
            g3d_set_spawn_parent(pl_root);
            g3d_entity_set_parent(pl_ext_root, pl_root);
            pl_lv_close(g);
        END
    END

    // --- cubos 3D: z de -3 a 1 (el heroe camina en z = 0; la pl_camara mira hacia -z, asi +x queda a la derecha) ---
    FOR (r = 0; r < pl_lh; r = r + 1)
        FOR (c = 0; c < pl_lw; c = c + 1)
            // el fondo se abre en la puerta del pasillo (3 de ancho y 3 de alto sobre el portal): el pasillo esta pegado detras y se ve por el hueco
            IF (pl_lvlb[r * pl_lw + c] > 0 AND NOT (pl_cor_found AND pl_cor_file != "" AND abs(c - (pl_cor_px - 1.0) / PL_TS) < 1.6 AND r >= pl_cor_py / PL_TS - 0.1 AND r <= pl_cor_py / PL_TS + 2.1)) pl_tile_cell(pl_lvlb[r * pl_lw + c], c, r, 1); END
            IF (pl_lvl[r * pl_lw + c] > 0)  pl_tile_cell(pl_lvl[r * pl_lw + c], c, r, 0); END
            IF (pl_lvlf[r * pl_lw + c] > 0) pl_tile_cell(pl_lvlf[r * pl_lw + c], c, r, 2); END
        END
    END

    pl_build_liquids();
    FOR (i = pl_plane + 1; i < pl_nplanes; i = i + 1) pl_build_visual(i); END      // los planos de detras, solo para verlos
    FOR (i = 0; i < pl_ndr; i = i + 1)                                    // puertas: bloques que suben y bajan (el grupo entero se aparta su propia longitud)
        k = 1; c = pl_dr_c[i]; r = pl_dr_r[i];
        IF (pl_t_dir[pl_dr_t[i]] == 3 OR pl_t_dir[pl_dr_t[i]] == 4)                // lateral: celdas contiguas del mismo tile en la fila
            WHILE (c - k >= 0 AND pl_lvl[r * pl_lw + c - k] == pl_dr_t[i]) k = k + 1; END
            j = 1; WHILE (c + j < pl_lw AND pl_lvl[r * pl_lw + c + j] == pl_dr_t[i]) j = j + 1; END
        ELSE                                                           // vertical: celdas contiguas en la columna
            WHILE (r - k >= 0 AND pl_lvl[(r - k) * pl_lw + c] == pl_dr_t[i]) k = k + 1; END
            j = 1; WHILE (r + j < pl_lh AND pl_lvl[(r + j) * pl_lw + c] == pl_dr_t[i]) j = j + 1; END
        END
        pl_dr_len[i] = k + j - 1;
        IF (pl_t_dir[pl_dr_t[i]] == 3 OR pl_t_dir[pl_dr_t[i]] == 4) pl_dr_ex[i] = pl_lk_door(0, c - k + 1, c + j - 1, pl_lh - 1 - r, pl_lh - 1 - r);
        ELSE pl_dr_ex[i] = pl_lk_door(0, c, c, pl_lh - 1 - (r + j - 1), pl_lh - 1 - (r - k + 1)); END
        pl_dr_ns[i] = pl_lk_ns; FOR (q = 0; q < pl_lk_ns; q = q + 1) pl_dr_sc[i * 8 + q] = pl_lk_s[q]; END
        pl_dr_pl[i] = pl_npl; pl_plx[pl_npl] = c * PL_TS + 1.0; pl_ply[pl_npl] = r * PL_TS + 1.0; pl_plw[pl_npl] = 1.0; pl_plh[pl_npl] = 1.0; pl_pldx[pl_npl] = 0.0; pl_pldy[pl_npl] = 0.0;
        IF (pl_npl < 110) pl_npl = pl_npl + 1; END
        pl_dr_ent[i] = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(pl_dr_ent[i], pl_box_mesh); g3d_entity_set_material(pl_dr_ent[i], pl_t_mat[pl_dr_t[i]]);
        g3d_entity_set_scale(pl_dr_ent[i], PL_TS, PL_TS, 3.3);
        g3d_entity_set_position(pl_dr_ent[i], pl_dr_c[i] * PL_TS + 1.0, pl_dr_r[i] * PL_TS + 1.0, -1.35);
    END
    pl_circuitos();

    // --- puntos de control: un mastil y una bandera roja (verde al activarse) ---
    FOR (i = 0; i < pl_ncp; i = i + 1)
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_mat_metal);
        g3d_entity_set_scale(e, 0.14, 2.8, 0.14); g3d_entity_set_position(e, pl_cpx[i], pl_cpy[i] + 1.4, -0.4);
        pl_cpent[i] = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(pl_cpent[i], pl_box_mesh); g3d_entity_set_material(pl_cpent[i], pl_mat_flag);
        g3d_entity_set_scale(pl_cpent[i], 0.9, 0.6, 0.08); g3d_entity_set_position(pl_cpent[i], pl_cpx[i] + 0.5, pl_cpy[i] + 2.3, -0.4);
    END

    // --- fondo: la imagen del nivel (con parallax) o una pared lejana, y pl_columnas para dar profundidad ---
    pl_liquidos();
    pl_hm_on = 0; pl_hm_show = 0; pl_hm_ps = 0;
    IF (pl_hero_model != "" AND fexists(pl_hero_model)) pl_hm_on = 1; pl_hmodelo(); END
    IF (pl_mundo)
        pl_columnas = 0;                                  // en un mundo 3D no hay pared de fondo: se ve el mundo
    ELSE IF (pl_bgfile != "")
        pl_mat_bg = pl_make_mat(pl_dirfondos + pl_bgfile, 0.9, 0.0);
        g3d_material_set_color(pl_mat_bg, 1.0, 1.0, 1.0, 1.0);
        g = map_load(pl_dirfondos + pl_bgfile);
        IF (g > 0) pl_fondo(pl_mat_bg, 1.0 * map_info(0, g, G_WIDTH) / map_info(0, g, G_HEIGHT)); ELSE pl_fondo(pl_mat_bg, 1.78); END
    ELSE
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_mat_bg);
        g3d_entity_set_scale(e, (pl_lw * PL_TS + 80.0) * pl_bgk, (pl_lh * PL_TS + 40.0) * pl_bgk, 1.0);
        g3d_entity_set_position(e, pl_lw * PL_TS / 2.0, pl_lh * PL_TS / 2.0, pl_bgz);
    END END
    FOR (i = 0; i < (pl_lw * PL_TS / 11.0) AND pl_columnas; i = i + 1)
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_mat_metal);
        g3d_entity_set_scale(e, 3.0, 40.0, 3.0);
        g3d_entity_set_position(e, 6.0 + i * 11.0, 14.0, -(9.0 + (i % 3) * 1.5));
    END
    IF (pl_mundo) pl_mundo_cajas(); END
    RETURN 1;
END

// ---------------------------------------------------------------------------
//  Fotograma de una hoja: celda (col,fila) de tamano cw x ch, ancla en pies
// ---------------------------------------------------------------------------
FUNCTION pl_cell(int sp, int col, int row, int cw, int ch, float ax, float ay)
BEGIN
    g3d_sprite_set_cell(sp, col * cw, row * ch, cw, ch);
    g3d_sprite_set_anchor(sp, ax, ay);
END

// fotograma f de la animacion a del heroe (se repite sola)
FUNCTION pl_cell_h(int sp, int a, int f)
PRIVATE int n;
BEGIN
    n = pl_ha_n[a]; IF (n < 1) n = 1; END
    pl_cell(sp, pl_ha_col[a] + f % n, pl_ha_row[a], pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay);
END

// fotograma f de la animacion a del tipo de enemigo tp
FUNCTION pl_cell_en(int sp, int tp, int a, int f)
PRIVATE int k; int n;
BEGIN
    k = tp * 9 + a; n = pl_ea_n[k]; IF (n < 1) n = 1; END
    pl_em_a3 = 0; IF (a == 1 OR a == 3) pl_em_a3 = 3; ELSE IF (a == 2) pl_em_a3 = 1; ELSE IF (a == 4) pl_em_a3 = 4; ELSE IF (a == 5) pl_em_a3 = 5; ELSE IF (a >= 6) pl_em_a3 = 2; END END END END END
    pl_cell(sp, pl_ea_col[k] + f % n, pl_ea_row[k], pl_en_cw[tp], pl_en_cw[tp], pl_en_ax[tp], pl_en_ay[tp]);
END

// ---------------------------------------------------------------------------
//  Disparo del heroe
// ---------------------------------------------------------------------------
PROCESS pl_bala_heroe(float bx, float by, int dir, int big)
PRIVATE
    int i; int alive = 1; float dmg; int t; int be; int q;
BEGIN
    x = bx; y = by; z = 0.0;
    be = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(be, pl_bull_mesh);
    IF (big) g3d_entity_set_material(be, pl_mat_bhc); g3d_entity_set_scale(be, 0.9, 0.9, 0.9); dmg = 3.0;
    ELSE g3d_entity_set_material(be, pl_mat_bh); g3d_entity_set_scale(be, 0.45, 0.45, 0.45); dmg = 1.0; END
    g3d_entity_set_blend(be, 2);
    pl_nbul = pl_nbul + 1;
    WHILE (alive)
        x = x + dir * 30.0 / 60.0;
        IF (x < pl_hx - 24.0 OR x > pl_hx + 24.0) alive = 0; END
        t = pl_tile_at(x + dir * 0.3, y);                    // dianas: alternan su canal
        IF (t > 0)
            IF (pl_t_tgt[t] > 0)
                i = y / PL_TS;
                q = pl_lk_ch(0, (x + dir * 0.3) / PL_TS, pl_lh - 1 - i, pl_t_tgt[t]);
                pl_toggled[q] = 1 - pl_toggled[q]; alive = 0;
            END
        END
        IF (pl_box_hits(x - 0.2, y - 0.2, x + 0.2, y + 0.2)) alive = 0; END
        FOR (i = 0; i < pl_nen; i = i + 1)
            IF (alive AND pl_enalive[i])
                IF (x > pl_enx[i] - pl_enw[i] AND x < pl_enx[i] + pl_enw[i] AND y > pl_eny[i] AND y < pl_eny[i] + pl_enh[i] + 0.4)
                    IF (pl_enopen[i]) pl_enhp[i] = pl_enhp[i] - dmg; END
                    alive = 0;
                END
            END
        END
        g3d_entity_set_position(be, x, y, 0.0);
        FRAME;
    END
    pl_nbul = pl_nbul - 1;
    g3d_entity_destroy(be);
END

// ---------------------------------------------------------------------------
//  Disparo enemigo
// ---------------------------------------------------------------------------
PROCESS pl_bala_enemigo(float bx, float by, int dir)
PRIVATE
    int alive = 1; int be;
BEGIN
    x = bx; y = by; z = 0.0;
    be = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(be, pl_bull_mesh); g3d_entity_set_material(be, pl_mat_be); g3d_entity_set_scale(be, 0.5, 0.5, 0.5);
    g3d_entity_set_blend(be, 2);
    WHILE (alive)
        x = x + dir * 12.0 / 60.0;
        IF (x < pl_hx - 24.0 OR x > pl_hx + 24.0) alive = 0; END
        IF (pl_box_hits(x - 0.2, y - 0.2, x + 0.2, y + 0.2)) alive = 0; END
        IF (x > pl_hcx - pl_hhalf AND x < pl_hcx + pl_hhalf AND y > pl_hy AND y < pl_hy + 2.5)
            pl_ehit = 1; alive = 0;
        END
        g3d_entity_set_position(be, x, y, 0.0);
        FRAME;
    END
    g3d_entity_destroy(be);
END

// ---------------------------------------------------------------------------
//  Enemigo "Met": escondido bajo el casco (invulnerable), se abre, dispara y anda
// ---------------------------------------------------------------------------
PROCESS pl_tirador(float sx, float sy)
PRIVATE
    int eid; int dir = -1; int tic; int st; int stt; int fr; int lasthp; int hurtf; int dead_t; int tp; int em;
    float nx;
BEGIN
    IF (pl_nen >= 255) say("AVISO: demasiados enemigos (max 255), se ignora uno"); RETURN; END
    ctype = C_3D; csubtype = C3D_SPRITE;
    tp = 0;
    eid = pl_nen; pl_nen = pl_nen + 1;
    x = sx; y = sy; z = 0.0;
    pl_enalive[eid] = 1; pl_enhp[eid] = pl_en_hp[tp]; pl_enx[eid] = x; pl_eny[eid] = y; pl_enopen[eid] = 0; pl_enw[eid] = pl_en_w[tp]; pl_enh[eid] = pl_en_h[tp]; pl_enreach[eid] = 0.0;
    lasthp = pl_enhp[eid];
    stt = 40 + eid * 17;                         // cada uno arranca desfasado
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, pl_en_ppu[tp]);
    em = pl_em_new(tp); IF (em >= 0) g3d_sprite_set_pixels_per_unit(entity, 100000.0); END
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);
    g3d_sprite_set_cutout(entity, 0.4);
    g3d_sprite_set_shadow(entity, 1);
    g3d_sprite_set_smooth(entity, 1);
    file = 0; graph = pl_g_en[tp];
    // st: 0 escondido, 1 abriendose, 2 disparando, 3 andando
    WHILE (pl_enhp[eid] > 0)
        tic = tic + 1;
        stt = stt - 1;
        IF (pl_enhp[eid] < lasthp) hurtf = 8; END
        lasthp = pl_enhp[eid];
        IF (hurtf > 0) hurtf = hurtf - 1; END

        IF (st == 0)                                // escondido (invulnerable)
            pl_enopen[eid] = 0;
            IF (stt <= 0 AND abs(pl_hx - x) < 18.0)
                st = 1; stt = 18;
                IF (pl_hx > x) dir = 1; ELSE dir = -1; END
            ELSE IF (stt <= 0) stt = 30; END END
            pl_cell_en(entity, tp, 0, tic / 20);
        ELSE IF (st == 1)                           // abriendose
            pl_enopen[eid] = 1;
            pl_cell_en(entity, tp, 1, (18 - stt) / 6);
            IF (stt <= 0) st = 2; stt = 36; END
        ELSE IF (st == 2)                           // disparando
            pl_enopen[eid] = 1;
            IF (stt == 24) pl_bala_enemigo(x + dir * 1.4, y + 1.0, dir); END
            fr = (36 - stt) / 9; IF (fr > 3) fr = 3; END
            pl_cell_en(entity, tp, 3, fr);
            IF (stt <= 0) st = 3; stt = 80; END
        ELSE                                        // andando hacia el heroe
            pl_enopen[eid] = 1;
            IF (pl_hx > x) dir = 1; ELSE dir = -1; END
            nx = x + dir * 2.5 / 60.0;
            IF (pl_box_hits(nx + dir * 0.9 - 0.1, y + 0.2, nx + dir * 0.9 + 0.1, y + 1.2)
                OR NOT pl_box_hits(nx + dir * 0.9 - 0.1, y - 0.4, nx + dir * 0.9 + 0.1, y - 0.1))
                // borde o pared: no avanza
            ELSE
                x = nx;
            END
            pl_cell_en(entity, tp, 2, tic / 8);
            IF (stt <= 0) st = 0; stt = 70; END
        END END END
        IF (hurtf > 0) pl_cell_en(entity, tp, 4, 0); END
        IF (dir > 0) flags = 0; ELSE flags = 1; END
        pl_enx[eid] = x;
        IF (em >= 0) pl_em_set(em, x, y, z, 90.0 * dir, pl_em_a3, abs(pl_hx - x) < 40.0); END
        FRAME;
    END
    // muerte
    pl_enalive[eid] = 0;
    pl_score = pl_score + 100;
    dead_t = 0;
    WHILE (dead_t < 24)
        pl_cell_en(entity, tp, 5, dead_t / 8);
        dead_t = dead_t + 1;
        IF (em >= 0) pl_em_set(em, x, y, z, 90.0 * dir, 5, 1); END
        FRAME;
    END
    IF (em >= 0) pl_em_set(em, x, y, z, 0.0, 5, 0); END
    g3d_sprite_destroy(entity);
END

// ---------------------------------------------------------------------------
//  Kaka con garras (modelo 3D prerenderizado): persigue, salta y ataca con las garras
//  hoja 6x6 de 160 px: filas 0 quieta, 1 andar, 2 salto, 3 ataque, 4 daño, 5 muerte
// ---------------------------------------------------------------------------
PROCESS pl_perseguidor(float sx, float sy)
PRIVATE
    int eid; int dir = -1; int tic; int st; int stt; int fr; int lasthp; int hurtf; int dead_t; int grounded; int em;
    float vx; float vy; float nx; float ny; float c; int i; float dist; int moving;
    float hw; float hh; float dt = 0.0166667; int tp;
BEGIN
    IF (pl_nen >= 255) say("AVISO: demasiados enemigos (max 255), se ignora uno"); RETURN; END
    tp = 1; hw = pl_en_w[tp] * 0.8; hh = pl_en_h[tp];
    ctype = C_3D; csubtype = C3D_SPRITE;
    eid = pl_nen; pl_nen = pl_nen + 1;
    x = sx; y = sy; z = 0.0;
    pl_enalive[eid] = 1; pl_enhp[eid] = pl_en_hp[tp]; pl_enx[eid] = x; pl_eny[eid] = y; pl_enopen[eid] = 1;
    pl_enw[eid] = pl_en_w[tp]; pl_enh[eid] = pl_en_h[tp]; pl_enreach[eid] = 0.0; lasthp = pl_enhp[eid];
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, pl_en_ppu[tp]);
    em = pl_em_new(tp); IF (em >= 0) g3d_sprite_set_pixels_per_unit(entity, 100000.0); END
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);
    g3d_sprite_set_cutout(entity, 0.4);
    g3d_sprite_set_shadow(entity, 1);
    g3d_sprite_set_smooth(entity, 1);
    file = 0; graph = pl_g_en[tp];
    // st: 0 andar/quieta, 1 atacar, 2 saltar
    WHILE (pl_enhp[eid] > 0)
        tic = tic + 1;
        IF (pl_enhp[eid] < lasthp) hurtf = 8; END
        lasthp = pl_enhp[eid];
        IF (hurtf > 0) hurtf = hurtf - 1; END
        dist = pl_hx - x;
        moving = 0;
        IF (st == 0)
            IF (abs(dist) < 18.0)
                IF (dist > 0.0) dir = 1; ELSE dir = -1; END
                IF (abs(dist) > 3.4) vx = dir * 4.5; moving = 1;
                ELSE
                    vx = 0.0; st = 1; stt = 36;                 // al alcance: ataca
                END
            ELSE
                vx = 0.0;
            END
            // salta si hay pared delante o el heroe esta mas arriba
            IF (grounded AND moving AND (pl_box_hits(x + dir * (hw + 0.3), y + 0.3, x + dir * (hw + 0.5), y + 1.0) OR (pl_hy > y + 2.6 AND abs(dist) < 9.0)))
                vy = 20.0; grounded = 0; st = 2;
            END
        ELSE IF (st == 1)
            vx = 0.0; stt = stt - 1;
            fr = (36 - stt) / 6; IF (fr > 5) fr = 5; END
            IF (fr >= 1 AND fr <= 3) pl_enreach[eid] = 2.4; ELSE pl_enreach[eid] = 0.0; END
            IF (stt <= 0) st = 0; pl_enreach[eid] = 0.0; END
        ELSE                                                   // en el aire
            IF (grounded) st = 0; END
        END END
        // fisica propia (misma que el heroe)
        vy = vy - 58.0 * dt;
        IF (vy < -34.0) vy = -34.0; END
        nx = x + vx * dt;
        IF (NOT pl_box_hits(nx - hw, y + 0.05, nx + hw, y + hh)) x = nx; END
        ny = y + vy * dt; grounded = 0;
        IF (pl_box_hits(x - hw + 0.05, ny, x + hw - 0.05, ny + hh))
            IF (vy <= 0.0)
                IF (ny < 0.0) c = -1.0; ELSE c = ny / PL_TS; END
                i = c; ny = (i + 1) * PL_TS; grounded = 1;
            ELSE
                c = (ny + hh) / PL_TS; i = c; ny = i * PL_TS - hh - 0.001;
            END
            vy = 0.0;
        END
        y = ny;
        IF (y < -10.0) pl_enhp[eid] = 0; END
        // fotograma
        IF (hurtf > 0)
            pl_cell_en(entity, tp, 4, 0);
        ELSE IF (st == 1)
            pl_cell_en(entity, tp, 3, fr);
        ELSE IF (NOT grounded)
            IF (vy > 6.0) pl_cell_en(entity, tp, 6, 0); ELSE IF (vy > -6.0) pl_cell_en(entity, tp, 7, 0); ELSE pl_cell_en(entity, tp, 8, 0); END END
        ELSE IF (moving)
            pl_cell_en(entity, tp, 2, tic / 6);
        ELSE
            pl_cell_en(entity, tp, 0, tic / 14);
        END END END END
        IF (dir > 0) flags = 0; ELSE flags = 1; END
        pl_enx[eid] = x; pl_eny[eid] = y;
        IF (em >= 0) pl_em_set(em, x, y, z, 90.0 * dir, pl_em_a3, abs(pl_hx - x) < 40.0); END
        FRAME;
    END
    // muerte
    pl_enalive[eid] = 0; pl_enreach[eid] = 0.0;
    pl_score = pl_score + 300;
    dead_t = 0;
    WHILE (dead_t < 36)
        pl_cell_en(entity, tp, 5, dead_t / 6);
        dead_t = dead_t + 1;
        IF (em >= 0) pl_em_set(em, x, y, z, 90.0 * dir, 5, 1); END
        FRAME;
    END
    IF (em >= 0) pl_em_set(em, x, y, z, 0.0, 5, 0); END
    g3d_sprite_destroy(entity);
END

// ---------------------------------------------------------------------------
//  El heroe
// ---------------------------------------------------------------------------
// (se declaran aqui porque se lanzan antes de definirse)
DECLARE PROCESS pl_cambio_plano(int sentido, float destx, float desty)
END
DECLARE PROCESS ps_gestor()
END
DECLARE PROCESS ps_heroe()
END
DECLARE PROCESS ps_enemigo(int tp, float sx, float sz)
END
DECLARE PROCESS ps_mecanismos()
END
DECLARE PROCESS ps_bala(float bx, float by, float bz, float ddx, float ddz)
END
DECLARE PROCESS pl_mundo_salir(float lx, float ly)
END

PROCESS pl_heroe()
PRIVATE
    int i;
    float vx; float vy; int grounded;
    int jprev; int sprev;
    int tic; int anim; int shoot_t; int hurt_t; int invul; int charge; int dead_t;
    int col; int row; int moving; int dirin; int q; int j;
    float nx; float ny; float c;
    float hw = 0.65; float hh = 2.55; float hl = 0.5; float hr = 0.8; int kk; float fxc; float stepy; int bump; float sh;   // caja: de -hl a +hr alrededor del ancla (invertida al mirar atras)
    float dt = 0.0166667;
    float gravity; float runspd; float jumpv; int a; int f;
    int jump_k; int shot_k;
    int dmg; int ride = -1; int fidx; float tvx; float landy; float ptop; int landed;
    int oc; int oc0; int oc1; int orow; int oidx; int wasg;
    int inliq; int swim; int liqi; int liqup; float geff; float sy; float pl_l; float pl_r; float pl_u; float pl_d; int hid;
END
BEGIN
    ctype = C_3D; csubtype = C3D_SPRITE;
    gravity = pl_hero_grav; runspd = pl_hero_run; jumpv = pl_hero_jump; hh = pl_hero_hh;
    x = pl_startx; y = pl_starty; z = pl_tz;
    IF (pl_arr_ok AND NOT pl_cambio) x = pl_arr_x; y = pl_arr_y; pl_startx = x; pl_starty = y; pl_arr_ok = 0; pl_arr_from = ""; pl_ret_on = 0; pl_portal_cd = 90; END   // llegas por una puerta lateral: apareces en la puerta que vuelve de donde vienes
    IF (pl_ret_on AND pl_ret_file == pl_levelfile AND NOT pl_cambio) x = pl_ret_x; y = pl_ret_y; pl_startx = x; pl_starty = y; pl_ret_on = 0; pl_portal_cd = 90; END   // vuelve de otro nivel: donde saliste
    IF (pl_cambio)                                         // llega por un portal: se busca un hueco libre justo encima del sitio
        FOR (i = 0; i < 30 AND pl_box_hits(x - hl, y, x + hr, y + hh); i = i + 1) y = y + 0.5; END
    END
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, pl_hero_ppu);      // 1 unidad = 52 px de la hoja: el heroe mide ~2.7 unidades (lo mismo que su caja de colision)
    IF (pl_hm_on) g3d_sprite_set_pixels_per_unit(entity, 100000.0); END   // con el modelo 3D el sprite no se ve (diminuto)
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);
    g3d_sprite_set_cutout(entity, 0.4);
    g3d_sprite_set_shadow(entity, 1);
    g3d_sprite_set_smooth(entity, 1);
    g3d_sprite_set_lit(entity, 1);
    file = 0; graph = pl_g_hero;
    LOOP
        tic = tic + 1;
        IF (pl_trans == 0 AND graph != pl_g_hero) graph = pl_g_hero; END     // vuelve a la hoja de perfil

        IF (pl_mundo AND NOT pl_dentro)                    // el jugador esta fuera, en el mundo 3D: el heroe del mapa espera oculto
            IF (alpha != 0) alpha = 0; g3d_sprite_set_shadow(entity, 0); END
            IF (pl_hm_on) pl_hm_show = 0; END
            hid = 1; pl_hx = x; pl_hy = y;
            FRAME;
            CONTINUE;
        END
        IF (pl_mode == 1)                                  // se esta jugando el pasillo: este heroe espera oculto
            IF (alpha != 0) alpha = 0; g3d_sprite_set_shadow(entity, 0); END
            hid = 1;
            IF (pl_hm_on AND NOT pl_hm_ps) pl_hm_show = 0; END
            pl_hx = x; pl_hy = y;
            FRAME;
            CONTINUE;
        END
        IF (hid AND dead_t == 0) hid = 0; IF (pl_mundo) x = pl_startx; y = pl_starty; END alpha = 255; g3d_sprite_set_shadow(entity, 1); vx = 0.0; vy = 0.0; pl_portal_cd = 90; invul = 60; END   // vuelve del pasillo

        IF (pl_trans != 0)                                 // pasando de un plano a otro: sin control, solo se mueve en z
            z = pl_tz; pl_hx = x; pl_hy = y;
            IF (pl_g_hero_d > 0)
                graph = pl_g_hero_d;
                IF (pl_trans_dir > 0) pl_cell(entity, (tic / 5) % 6, 0, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay);   // hacia el fondo: de espaldas
                ELSE pl_cell(entity, (tic / 5) % 6, 1, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay); END                // hacia la camara: de frente
            ELSE
                pl_cell_h(entity, 1, tic * pl_ha_fps[1] / 60);
            END
            IF (pl_hm_on) alpha = 0; pl_hm_put(x, y, z, 180.0 * (pl_trans_dir > 0), 1, 0); END
            FRAME;
            CONTINUE;
        END

        IF (dead_t > 0)
            // animacion de muerte y reaparicion
            dead_t = dead_t - 1;
            pl_cell_h(entity, 8, tic * pl_ha_fps[8] / 60);
            IF (pl_hm_on) alpha = 0; pl_hm_put(x, y, z, 90.0 * pl_hface, 5, 0); END
            IF (dead_t == 0)
                pl_hhp = pl_hmaxhp; x = pl_startx; y = pl_starty; vx = 0.0; vy = 0.0;
                invul = 90; hurt_t = 0; alpha = 255;
            END
            pl_hx = x; pl_hy = y;
            FRAME;
            CONTINUE;
        END

        // ----- entrada -----
        dirin = 0;
        IF (pl_in_right()) dirin = dirin + 1; END
        IF (pl_in_left()) dirin = dirin - 1; END
        jump_k = pl_in_jump() OR pl_in_up();
        kk = pl_tile_at(x, y + 1.0);                       // en una puerta al fondo / al pasillo, Arriba (o W) entra en ella en vez de saltar
        IF (kk > 0) IF (pl_t_portal[kk] == 1 OR pl_t_portal[kk] == 3) IF (pl_in_up()) jump_k = pl_in_jump(); END END END
        shot_k = pl_in_shot();

        // ----- movimiento horizontal -----
        // ----- pl_liquidos: agua, lava, acido, cieno -----
        liqi = pl_tile_at(x, y + 1.0); inliq = 0; swim = 0;
        IF (liqi > 0)
            IF (pl_t_liq[liqi] > 0) inliq = 1; IF (pl_t_swim[liqi]) swim = 1; END END
        END
        tvx = dirin * runspd;
        IF (inliq) tvx = tvx * pl_t_slow[liqi]; END
        IF (hurt_t > 0)
            hurt_t = hurt_t - 1;          // retroceso: sin control
        ELSE
            IF (grounded AND fidx > 0 AND pl_t_slip[fidx] > 0.0)      // hielo: patina
                vx = vx + (tvx - vx) * (0.05 * (1.0 - pl_t_slip[fidx]) + 0.008);
            ELSE
                vx = tvx;
            END
            IF (dirin != 0) pl_hface = dirin; END
        END
        IF (pl_hface > 0) hl = pl_hero_hl; hr = pl_hero_hr; ELSE hl = pl_hero_hr; hr = pl_hero_hl; END         // el cuerpo es mas largo por delante (canon)
        IF (pl_box_hits(x - hl + 0.02, y + 0.05, x + hr - 0.02, y + hh - 0.05))   // al girarse junto a una pared el canon puede quedar dentro: se aparta
            FOR (kk = 1; kk <= 14; kk = kk + 1)
                IF (NOT pl_box_hits(x - kk * 0.05 - hl + 0.02, y + 0.05, x - kk * 0.05 + hr - 0.02, y + hh - 0.05)) x = x - kk * 0.05; BREAK; END
                IF (NOT pl_box_hits(x + kk * 0.05 - hl + 0.02, y + 0.05, x + kk * 0.05 + hr - 0.02, y + hh - 0.05)) x = x + kk * 0.05; BREAK; END
            END
            IF (pl_box_hits(x - hl + 0.02, y + 0.05, x + hr - 0.02, y + hh - 0.05))                         // si sigue dentro, hacia arriba hasta salir
                FOR (kk = 1; kk <= 14; kk = kk + 1)
                    IF (NOT pl_box_hits(x - hl + 0.02, y + 0.05 + kk * 0.1, x + hr - 0.02, y + hh - 0.05 + kk * 0.1)) y = y + kk * 0.1; BREAK; END
                END
            END
        END
        IF (ride >= 0 AND grounded) x = x + pl_pldx[ride]; y = y + pl_pldy[ride]; END    // lo lleva la plataforma

        // ----- salto (altura variable: soltar corta el impulso) -----
        IF (jump_k AND NOT jprev AND grounded AND hurt_t == 0)
            vy = jumpv; IF (inliq AND NOT swim) vy = jumpv * 0.7; END
            grounded = 0;
        END
        IF (swim AND jump_k AND NOT jprev AND NOT grounded AND hurt_t == 0)     // con la cabeza fuera, un salto saca del agua
            liqup = pl_tile_at(x, y + hh + 0.4);
            IF (liqup == 0) vy = jumpv * 0.8; ELSE IF (pl_t_liq[liqup] == 0) vy = jumpv * 0.8; END END
        END
        IF (NOT jump_k AND vy > 8.0 AND NOT swim) vy = 8.0; END
        jprev = jump_k;
        geff = gravity;
        IF (inliq)
            IF (swim) geff = gravity * 0.18; ELSE geff = gravity * 0.55; END
        END
        vy = vy - geff * dt;
        IF (swim)
            IF (jump_k AND hurt_t == 0) vy = vy + 70.0 * dt; END     // mantener salto = nadar hacia arriba
            IF (vy < -5.0) vy = -5.0; END
            IF (jump_k AND vy > 9.0 AND vy < 15.0) vy = 9.0; END
        END
        IF (vy < -34.0) vy = -34.0; END

        // ----- mover en X y resolver contra la rejilla -----
        nx = x + vx * dt;
        IF (grounded AND fidx > 0) nx = nx + pl_t_conv[fidx] * dt; END      // cinta transportadora
        FOR (i = 0; i < pl_npl; i = i + 1)                                      // las plataformas que se mueven tambien cortan el paso por el lado
            IF (nx + hr > pl_plx[i] - pl_plw[i] AND nx - hl < pl_plx[i] + pl_plw[i] AND y < pl_ply[i] + pl_plh[i] - 0.12 AND y + hh > pl_ply[i] - pl_plh[i] + 0.1)
                IF (nx < pl_plx[i]) nx = pl_plx[i] - pl_plw[i] - hr - 0.001; ELSE nx = pl_plx[i] + pl_plw[i] + hl + 0.001; END
            END
        END
        IF (grounded AND pl_box_hits(nx - hl, y + 0.05, nx + hr, y + hh))              // sobre una rampa: si el lateral de un escalon estorba, se sube a el
            IF (pl_slope_y(x - pl_hface * 0.3, y + 0.05) > -500.0 OR pl_slope_y(x, y + 0.05) > -500.0)
                FOR (kk = 1; kk <= 6; kk = kk + 1)
                    IF (NOT pl_box_hits(nx - hl, y + 0.05 + kk * 0.2, nx + hr, y + hh + kk * 0.2)) y = y + kk * 0.2; BREAK; END
                END
            END
        END
        IF (vx != 0.0 AND pl_box_hits(nx - hl, y + 0.05, nx + hr, y + hh))              // un borde o escalon de hasta 0.5 se sube sin atascarse
            FOR (kk = 1; kk <= 5; kk = kk + 1)
                IF (NOT pl_box_hits(nx - hl, y + 0.05 + kk * 0.1, nx + hr, y + hh + kk * 0.1)) y = y + kk * 0.1; BREAK; END
            END
        END
        IF (pl_box_hits(nx - hl, y + 0.05, nx + hr, y + hh))
            IF (vx > 0.0)
                c = (nx + hr) / PL_TS; i = c; nx = i * PL_TS - hr - 0.001;
            ELSE
                c = (nx - hl) / PL_TS; i = c; nx = (i + 1) * PL_TS + hl + 0.001;
            END
        END
        x = nx;

        // ----- mover en Y -----
        ny = y + vy * dt;
        grounded = 0;
        IF (pl_box_hits(x - hl + 0.02, ny, x + hr - 0.02, ny + hh))
            IF (vy <= 0.0)
                IF (ny < 0.0) c = -1.0; ELSE c = ny / PL_TS; END
                i = c; ny = (i + 1) * PL_TS;
                grounded = 1;
            ELSE
                bump = 0;                                                      // esquina del techo: se esquiva hacia el lado y se sigue subiendo
                FOR (kk = 1; kk <= 6 AND NOT bump; kk = kk + 1)
                    sh = kk * 0.07;
                    IF (NOT pl_box_hits(x + sh - hl + 0.02, ny, x + sh + hr - 0.02, ny + hh) AND NOT pl_box_hits(x + sh - hl, y + 0.05, x + sh + hr, y + hh)) x = x + sh; bump = 1; END
                    IF (NOT bump AND NOT pl_box_hits(x - sh - hl + 0.02, ny, x - sh + hr - 0.02, ny + hh) AND NOT pl_box_hits(x - sh - hl, y + 0.05, x - sh + hr, y + hh)) x = x - sh; bump = 1; END
                END
                IF (NOT bump) c = (ny + hh) / PL_TS; i = c; ny = i * PL_TS - hh - 0.001; END
            END
            IF (NOT bump OR vy <= 0.0) vy = 0.0; END
            bump = 0;
        END
        // tiles de una via y plataformas que se mueven: solo se pisan cayendo desde arriba
        ride = -1;
        IF (NOT grounded AND vy <= 0.0)
            landed = 0; landy = -1000.0;
            IF (ny >= 0.0)
                orow = ny / PL_TS;
                oc0 = (x - hl + 0.02) / PL_TS; oc1 = (x + hr - 0.02) / PL_TS;
                FOR (oc = oc0; oc <= oc1; oc = oc + 1)
                    IF (oc >= 0 AND oc < pl_lw AND orow < pl_lh)
                        oidx = pl_lvl[orow * pl_lw + oc];
                        IF (oidx > 0)
                            IF (pl_t_oneway[oidx] AND y >= (orow + 1) * PL_TS - 0.06 AND ny < (orow + 1) * PL_TS)
                                landed = 1; IF ((orow + 1) * PL_TS > landy) landy = (orow + 1) * PL_TS; END
                            END
                        END
                    END
                END
            END
            FOR (i = 0; i < pl_npl; i = i + 1)
                ptop = pl_ply[i] + pl_plh[i];
                IF (abs(x + (hr - hl) / 2.0 - pl_plx[i]) < pl_plw[i] + hw * 0.7 AND y >= ptop - 0.35 - abs(pl_pldy[i]) AND ny <= ptop)
                    landed = 1; IF (ptop > landy) landy = ptop; ride = i; END
                END
            END
            IF (landed) ny = landy; vy = 0.0; grounded = 1; END
        END
        y = ny;
        // rampas: la superficie se sigue pegada al suelo (se sube y se baja andando)
        fxc = x + (hr - hl) / 2.0;                                                    // centro del cuerpo
        sy = pl_slope_y(fxc, y + 0.05);
        IF (sy > -500.0 AND vy <= 0.0 AND y - sy < 0.7 AND y - sy > -1.0 AND (NOT grounded OR y < sy) AND NOT pl_box_hits(x - hl + 0.04, sy + 0.08, x + hr - 0.04, sy + hh - 0.05))      // no bajar si ya esta apoyado sobre algo mas alto, ni a un sitio donde el cuerpo quedaria dentro de un bloque
            y = sy; vy = 0.0; grounded = 1; ride = -1;
        END
        FOR (i = 0; i < pl_npl; i = i + 1)                                      // plataformas macizas: lo que quede dentro se saca por donde menos se mete
            IF (x + hr > pl_plx[i] - pl_plw[i] AND x - hl < pl_plx[i] + pl_plw[i] AND y < pl_ply[i] + pl_plh[i] AND y + hh > pl_ply[i] - pl_plh[i])
                pl_l = x + hr - (pl_plx[i] - pl_plw[i]); pl_r = pl_plx[i] + pl_plw[i] - (x - hl);
                pl_u = pl_ply[i] + pl_plh[i] - y;        pl_d = y + hh - (pl_ply[i] - pl_plh[i]);
                IF (pl_u <= pl_l AND pl_u <= pl_r AND pl_u <= pl_d)
                    y = pl_ply[i] + pl_plh[i]; IF (vy < 0.0) vy = 0.0; END grounded = 1; ride = i;
                ELSE IF (pl_d <= pl_l AND pl_d <= pl_r)
                    y = pl_ply[i] - pl_plh[i] - hh; IF (vy > 0.0) vy = 0.0; END
                ELSE IF (pl_l <= pl_r)
                    x = pl_plx[i] - pl_plw[i] - hr - 0.001;
                ELSE
                    x = pl_plx[i] + pl_plw[i] + hl + 0.001;
                END END END
                IF (invul == 0 AND pl_box_hits(x - hl + 0.05, y + 0.05, x + hr - 0.05, y + hh - 0.05))      // aplastado contra una pared
                    pl_hhp = pl_hhp - 3; invul = 80; hurt_t = 14; vy = 10.0; grounded = 0;
                END
            END
        END
        pl_hride = ride;
        // que tile pisa (para hielo, cinta y trampolin)
        fidx = 0;
        IF (grounded AND ride < 0) fidx = pl_tile_at(fxc, y - 0.2); END
        IF (grounded AND fidx > 0)
            IF (pl_t_bounce[fidx] > 0.0) vy = pl_t_bounce[fidx]; grounded = 0; END
        END

        // ----- caer al foso -----
        IF (y < -10.0) pl_hhp = 0; END

        // ----- disparo (carga si se mantiene) -----
        IF (shot_k)
            IF (charge < 120) charge = charge + 1; END
        END
        IF (NOT shot_k AND sprev AND pl_nbul < 3 AND hurt_t == 0)
            IF (charge > 55)
                pl_bala_heroe(x + pl_hface * 1.1, y + 1.35, pl_hface, 1);
            ELSE
                pl_bala_heroe(x + pl_hface * 1.1, y + 1.35, pl_hface, 0);
            END
            shoot_t = 16;
            charge = 0;
        END
        IF (NOT shot_k) charge = 0; END
        sprev = shot_k;
        IF (shoot_t > 0) shoot_t = shoot_t - 1; END

        // ----- dano: balas enemigas y contacto con enemigos -----
        IF (invul > 0) invul = invul - 1; END
        IF (pl_ehit)
            IF (invul == 0) pl_hhp = pl_hhp - 2; invul = 80; hurt_t = 14; vx = -pl_hface * 6.0; vy = 8.0; grounded = 0; END
            pl_ehit = 0;
        END
        IF (invul == 0)
            FOR (i = 0; i < pl_nen; i = i + 1)
                IF (pl_enalive[i] AND abs(pl_enx[i] - fxc) < pl_enw[i] + pl_hhalf * 0.75 + pl_enreach[i] AND pl_eny[i] < y + hh AND pl_eny[i] + pl_enh[i] > y)
                    pl_hhp = pl_hhp - 3; invul = 80; hurt_t = 14; vx = -pl_hface * 6.0; vy = 9.0; grounded = 0;
                END
            END
        END
        // tiles que hacen dano (pinchos, lava...)
        dmg = pl_box_dmg(fxc - 0.45, y + 0.05, fxc + 0.45, y + 1.0);
        IF (invul == 0 AND dmg > 0)
            pl_hhp = pl_hhp - dmg; invul = 80; hurt_t = 14; vx = -pl_hface * 6.0; vy = 12.0; grounded = 0;
        END
        // puntos de control: al pasar cerca, se guarda como lugar de reaparicion
        FOR (i = 0; i < pl_ncp; i = i + 1)
            IF (NOT pl_cpon[i] AND abs(x - pl_cpx[i]) < 1.6 AND abs((y + 1.0) - (pl_cpy[i] + 1.4)) < 3.0)
                pl_cpon[i] = 1; pl_startx = pl_cpx[i]; pl_starty = pl_cpy[i];
                g3d_entity_set_material(pl_cpent[i], pl_mat_flagon);
            END
        END
        IF (pl_hhp <= 0) dead_t = 100; END

        // portales: E, S o Abajo en un tile de portal pasa al plano de detras (o de delante)
        IF (pl_portal_cd > 0) pl_portal_cd = pl_portal_cd - 1; END
        kk = pl_tile_at(x, y + 1.0);                       // en una puerta al fondo / al pasillo, Arriba (o W) entra en ella en vez de saltar
        IF (pl_portal_cd == 0 AND grounded AND dead_t == 0 AND kk > 0 AND ((pl_t_portal[kk] == 1 OR pl_t_portal[kk] == 2 OR pl_t_portal[kk] == 3) AND (pl_in_act() OR pl_in_up()) OR (pl_t_portal[kk] != 1 AND pl_t_portal[kk] != 2 AND pl_t_portal[kk] != 3 AND (pl_in_act() OR pl_in_down()))))
            kk = pl_tile_at(x, y + 1.0);
            IF (kk > 0 AND pl_t_portal[kk] == 3 AND pl_cor_found AND pl_cor_file != "")      // portal al pasillo de este mismo nivel
                pl_portal_cd = 60; pl_mode = 1; pl_cor_req = 1; pl_cor_hx = x; pl_cor_hy = y;
            END
            IF (kk > 0 AND pl_t_portal[kk] == 4)                                   // salir al mundo 3D: escena_gestor te devuelve a donde estabas
                pl_portal_cd = 600;
                IF (pl_mundo) pl_mundo_salir(x, y); ELSE pl_salto = "@exterior"; END
            END
            IF (kk > 0 AND pl_t_portal[kk] == 2 AND pl_t_dest[kk] != "")          // portal a otro nivel (p. ej. un pasillo): se apunta de donde sales y se pide el cambio
                pl_ret_x = x; pl_ret_y = y; pl_ret_file = pl_levelfile; pl_ret_level = pl_stem(pl_levelfile); pl_ret_on = 1; pl_arr_from = pl_stem(pl_levelfile);
                pl_salto = pl_t_dest[kk]; pl_portal_cd = 600; pl_trans = 1;
                j = (y + 1.0) / PL_TS; q = pl_lk_find(0, x / PL_TS, pl_lh - 1 - j);          // portal con enlace: escena/puerta de destino
                IF (q >= 0 AND pl_lk_tgt[q] != "")
                    j = find(pl_lk_tgt[q], "/");
                    IF (j > 0) pl_salto = substr(pl_lk_tgt[q], 0, j); pl_arr_door = substr(pl_lk_tgt[q], j + 1); pl_arr_from = ""; ELSE pl_salto = pl_lk_tgt[q]; END
                END
            END
            IF (kk > 0)
                IF ((pl_t_portal[kk] == 1 AND pl_plane + 1 < pl_nplanes) OR (pl_t_portal[kk] == -1 AND pl_plane > 0))
                    pl_portal_cd = 60; pl_trans = 1; pl_cambio_plano(pl_t_portal[kk], x, y);
                END
            END
        END

        // ----- animacion: cada estado usa su fila de la hoja (configurable) -----
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
        IF (a == 1 OR a == 6) f = anim * pl_ha_fps[a] / 60; ELSE f = tic * pl_ha_fps[a] / 60; END
        IF (a == 5) f = (16 - shoot_t) * pl_ha_n[5] / 16; END                  // el fogonazo, y luego el retroceso
        pl_cell_h(entity, a, f);
        IF (pl_hface > 0) flags = 0; ELSE flags = 1; END

        // parpadeo si es invulnerable; destello mientras se carga el disparo
        IF (invul > 0 AND (tic / 3) % 2 == 0) alpha = 90; ELSE alpha = 255; END
        IF (charge > 55 AND (tic / 4) % 2 == 0) color_r = 190; color_g = 235; color_b = 255;
        ELSE color_r = 215; color_g = 215; color_b = 215; END   // tono base: baja un poco el brillo del heroe

        pl_hx = x; pl_hy = y; pl_hcx = x + (hr - hl) / 2.0; pl_hhalf = (hl + hr) / 2.0;
        IF (pl_hm_on)                                      // en 3D: el sprite se oculta y el modelo toma su sitio, su sentido y su animacion
            alpha = 0;
            a = 0; IF (hurt_t > 0) a = 4; ELSE IF (NOT grounded) a = 3; ELSE IF (moving) a = 1; ELSE IF (shoot_t > 0 AND pl_hm_map[6] >= 0) a = 6; END END END END
            pl_hm_put(x, y, z, 90.0 * pl_hface, a, 0);
            IF (invul > 0 AND (tic / 3) % 2 == 0) pl_hm_blink = 1; END
        END
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  Camara lateral: proceso C3D_CAMERA (el motor lee x/y/z y target_x/y/z cada FRAME)
// ---------------------------------------------------------------------------
PROCESS pl_camara(int cam)
PRIVATE
    float cx = 14.0; float cy = 8.0; float tx; float ty; float vw;
    float d[6]; float q[6]; int i; int ini; int fovs;
BEGIN
    IF (pl_cam_keep_on)                                  // llega del pasillo: arranca en la pose que tenia y se desliza a la vista de lado
        FOR (i = 0; i < 6; i = i + 1) q[i] = pl_cam_keep[i]; END
        ini = 1; pl_camblend = 90; pl_cam_keep_on = 0;
    END
    IF (NOT pl_mundo) ctype = C_3D; csubtype = C3D_CAMERA; entity = cam; END      // en un mundo 3D no es la camara: solo calcula la pose (pl_cwp) y la copia la camara del mundo
    vw = pl_camdist * 0.62 + 0.5;                       // media anchura que se ve (fov 38, 16:9)
    IF (pl_cambio) cx = pl_camx; cy = pl_camy; END         // tras un cambio de plano sigue donde estaba
    LOOP
        IF (pl_mundo AND NOT pl_dentro) ini = 0; FRAME; CONTINUE; END               // el jugador esta fuera del mapa: la camara es la del mundo (se crea despues y es la que manda)
        tx = pl_hx + pl_hface * 3.0; ty = pl_hy + 2.5;
        IF (tx < vw) tx = vw; END
        IF (tx > pl_lw * PL_TS - vw) tx = pl_lw * PL_TS - vw; END
        IF (pl_lw * PL_TS < 2.0 * vw) tx = pl_lw * PL_TS / 2.0; END
        IF (ty < pl_camdist * 0.34 + 0.2) ty = pl_camdist * 0.34 + 0.2; END      // que se vea el suelo
        cx = cx + (tx - cx) * 0.12;
        cy = cy + (ty - cy) * 0.08;
        pl_camx = cx; pl_camy = cy;
        IF (pl_mode == 1)                                // jugando el pasillo: la pose la calcula ps_gestor
            FOR (i = 0; i < 6; i = i + 1) d[i] = pl_cam_p[i]; END
        ELSE                                             // de lado
            d[0] = cx; d[1] = cy + 1.2; d[2] = pl_camdist + pl_tz; d[3] = cx; d[4] = cy; d[5] = pl_tz;
        END
        IF (NOT ini OR pl_camblend <= 0)
            FOR (i = 0; i < 6; i = i + 1) q[i] = d[i]; END
            ini = 1;
        ELSE                                             // al pasar de una vista a otra la camara se desliza hasta la nueva
            pl_camblend = pl_camblend - 1;
            FOR (i = 0; i < 6; i = i + 1) q[i] = q[i] + (d[i] - q[i]) * 0.07; END
        END
        FOR (i = 0; i < 6; i = i + 1) pl_cam_last[i] = q[i]; END
        IF (pl_mundo)                                    // colocado en un mundo: las coordenadas del mapa se pasan al mundo (giro y posicion de su raiz)
            g3d_xray_update(pl_wxof(q[0], q[2]), q[1] + pl_wy, pl_wzof(q[0], q[2]), pl_wxof(pl_hx, pl_tz), pl_hy + pl_wy + 1.3, pl_wzof(pl_hx, pl_tz), 0.35, 0.1, 0.18);
            x = pl_wxof(q[0], q[2]); y = q[1] + pl_wy; z = pl_wzof(q[0], q[2]);
            target_x = pl_wxof(q[3], q[5]); target_y = q[4] + pl_wy; target_z = pl_wzof(q[3], q[5]);
            pl_cwp[0] = x; pl_cwp[1] = y; pl_cwp[2] = z; pl_cwp[3] = target_x; pl_cwp[4] = target_y; pl_cwp[5] = target_z;   // la camara del mundo copia esto mientras se esta dentro
            FRAME; CONTINUE;
        END
        IF (pl_mode == 1) g3d_xray_update(q[0], q[1], q[2], ps_hx + ps_ox, ps_hy + ps_oy + 1.3, ps_hz + ps_oz, 0.35, 0.05, 0.18);   // lo que tapa al heroe (muros del pasillo) se vuelve translucido
        ELSE g3d_xray_update(q[0], q[1], q[2], pl_hx, pl_hy + 1.3, pl_tz, 0.35, 0.1, 0.18); END                                     // y en el 2D, los primeros planos
        x = q[0]; y = q[1]; z = q[2];
        target_x = q[3]; target_y = q[4]; target_z = q[5];
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  MONTAR / DESCARGAR: lo que llama el codigo que genera el editor (o tu propio main.prg)
// ---------------------------------------------------------------------------

// pone a cero todo lo que deja un nivel anterior
FUNCTION pl_reiniciar()
PRIVATE int i;
BEGIN
    g3d_set_spawn_parent(-1);                      // lo que cuelga del mapa de una escena anterior ya no existe
    pl_lk_n = 0;
    pl_tl_n = 0; pl_sil = 1; pl_deco = ""; pl_deco_model = ""; pl_deco_n = 70;
    pl_por_n = 0; pl_nt = 0; pl_nsp = 0; pl_ncp = 0; pl_npl = 0; pl_nbt = 0; pl_ntg = 0; pl_ndr = 0; pl_liq_n = 0; pl_nen = 0; pl_nbul = 0; pl_em_n = 0;
    IF (NOT pl_cambio) pl_score = 0; pl_plane = 0; END
    pl_tz = 0.0; pl_zoff = 0.0; pl_vis = 0; pl_alfinal = ""; pl_salto = ""; pl_cor_file = ""; pl_cor_fin = 0; pl_cor_h = 0; pl_cor_found = 0; pl_cor_req = 0; pl_mode = 0; pl_camblend = 0;
    IF (NOT pl_cambio) pl_trans = 0; END
    pl_ehit = 0; pl_hride = -1; pl_bg_rep = 0; pl_bgfile = ""; pl_bgmove = 0.85; pl_columnas = 1; pl_bg_alto = 36.0;
    pl_sk_r = 0.04; pl_sk_g = 0.06; pl_sk_b = 0.16; pl_hz_r = 0.10; pl_hz_g = 0.14; pl_hz_b = 0.30;
    pl_fg_r = 0.04; pl_fg_g = 0.05; pl_fg_b = 0.12; pl_fg_n = 40.0; pl_fg_f = 110.0;
    pl_am_r = 0.42; pl_am_g = 0.46; pl_am_b = 0.58; pl_li_r = 1.0; pl_li_g = 0.95; pl_li_b = 0.85; pl_li_i = 1.5;
    FOR (i = 0; i < 160; i = i + 1) pl_chan[i] = 0; pl_pressed[i] = 0; pl_toggled[i] = 0; pl_hold[i] = 0; END
    FOR (i = 0; i < 39; i = i + 1)
        pl_t_tex[i] = ""; pl_t_top[i] = ""; pl_t_solid[i] = 0; pl_t_dmg[i] = 0; pl_t_oneway[i] = 0; pl_t_mark[i] = 0; pl_t_shape[i] = 0; pl_t_slip[i] = 0.0; pl_t_bounce[i] = 0.0; pl_t_conv[i] = 0.0;
        pl_t_axis[i] = 0; pl_t_dist[i] = 0.0; pl_t_spd[i] = 0.0; pl_t_mat[i] = 0; pl_t_matop[i] = 0; pl_t_liq[i] = 0; pl_t_swim[i] = 0; pl_t_slow[i] = 1.0; pl_t_alpha[i] = 255;
        pl_t_ramp[i] = 0; pl_t_door[i] = 0; pl_t_btn[i] = 0; pl_t_tgt[i] = 0; pl_t_wait[i] = 0.0; pl_t_mode[i] = 0; pl_t_hold[i] = 0.0; pl_t_dir[i] = 0; pl_t_portal[i] = 0; pl_t_dest[i] = "";
    END
    pl_lz[1] = -4.2; pl_lsz[1] = 1.6; pl_ls[1] = 1.0;   pl_lz[2] = 2.4; pl_lsz[2] = 0.5; pl_ls[2] = 1.0;
    pl_lz[3] = -8.0; pl_lsz[3] = 1.6; pl_ls[3] = 1.5;   pl_lz[4] = -14.0; pl_lsz[4] = 1.6; pl_ls[4] = 1.875;   pl_lz[5] = 5.0; pl_lsz[5] = 0.5; pl_ls[5] = 0.69;   pl_lz[6] = 2.2; pl_lsz[6] = 2.0; pl_ls[6] = 1.0;
    FOR (i = 0; i < 79999; i = i + 1) pl_fcm[i] = 0; pl_lvl[i] = 0; pl_lvlb[i] = 0; pl_lvlf[i] = 0; pl_liqmark[i] = 0; END
END

// ---------------------------------------------------------------------------
//  Mapa 2.5D colocado en un MUNDO 3D: se construye en la escena del mundo (no crea escena, camara ni luz) con sus coordenadas, colgando de
//  una entidad raiz que se coloca (y gira) en el mundo. Fuera del mapa lo controla el jugador 3D; al entrar por su portal 'Salir al mundo 3D'
//  (E, S o Abajo, desde fuera) pasa a controlarlo el heroe 2D, y al volver a cruzar el portal se sale delante de el.
// ---------------------------------------------------------------------------
FUNCTION pl_montar_mundo(float wx, float wy, float wz, float yaw)
PRIVATE int i;
BEGIN
    pl_reiniciar();
    pl_mundo = 1; pl_dentro = 0; pl_ext_root = 0; ps_ext_root = 0; pl_wx = wx; pl_wy = wy; pl_wz = wz; pl_wyaw = yaw; pl_sal_ok = 0;
    pl_nplanes = 1; pl_plane = 0; pl_cambio = 0;
    pl_load_settings();
    pl_root = g3d_entity_spawn(scene, 0, wx, wy, wz);
    g3d_entity_set_rotation(pl_root, 0.0, yaw * 1000.0, 0.0);
    g3d_set_spawn_parent(pl_root);
    pl_g_hero = map_load(pl_hero_sheet);
    pl_g_hero_d = 0;
    FOR (i = 0; i < 2; i = i + 1) IF (pl_en_sheet[i] != "") pl_g_en[i] = map_load(pl_en_sheet[i]); END END
    IF (pl_g_hero <= 0) say("ERROR: no se carga la hoja del heroe " + pl_hero_sheet); g3d_set_spawn_parent(-1); RETURN; END
    pl_hmaxhp = pl_hero_hp; pl_hhp = pl_hero_hp;
    pl_bull_mesh = g3d_primitive_sphere(12);
    pl_mat_bh = g3d_material_create();  g3d_material_set_color(pl_mat_bh, 1.0, 0.9, 0.3, 1.0);
    pl_mat_bhc = g3d_material_create(); g3d_material_set_color(pl_mat_bhc, 0.4, 0.9, 1.0, 1.0);
    pl_mat_be = g3d_material_create();  g3d_material_set_color(pl_mat_be, 1.0, 0.45, 0.2, 1.0);
    IF (NOT pl_build_level()) g3d_set_spawn_parent(-1); RETURN; END
    pl_heroe();
    IF (pl_cor_found AND pl_cor_file != "" AND pl_lv_exists(pl_cor_file, 0)) ps_gestor(); END      // el pasillo estilo Crash de este mapa, detras de su portal
    FOR (i = 0; i < pl_nsp; i = i + 1)
        IF (pl_spt[i] == 2) pl_tirador(pl_spx[i], pl_spy[i]); END
        IF (pl_spt[i] == 3) pl_perseguidor(pl_spx[i], pl_spy[i]); END
    END
    pl_camara(camera);
    pl_mundo_vigila();
    g3d_set_spawn_parent(-1);                          // lo que se cree despues ya no es del mapa (hasta que se entre)
END

// el jugador del mundo entra en el mapa: el heroe 2D aparece en su portal y la camara pasa a ser la del mapa
FUNCTION pl_mundo_entrar()
BEGIN
    IF (pl_dentro) RETURN; END
    pl_startx = pl_por_x; pl_starty = pl_por_y;
    pl_cwp[0] = pl_wxof(pl_por_x, pl_camdist); pl_cwp[1] = pl_wy + pl_por_y + 3.7; pl_cwp[2] = pl_wzof(pl_por_x, pl_camdist);   // primera pose de la camara del mapa (hasta que pl_camara la calcule)
    pl_cwp[3] = pl_wxof(pl_por_x, 0.0); pl_cwp[4] = pl_wy + pl_por_y + 2.5; pl_cwp[5] = pl_wzof(pl_por_x, 0.0);
    pl_dentro = 1; pl_portal_cd = 60;
    IF (pl_ext_root > 0) g3d_entity_set_position(pl_ext_root, 0.0, 0.0 - 100000.0, 0.0); END      // la fachada se aparta: dentro se ve el interior
    IF (ps_ext_root > 0) g3d_entity_set_position(ps_ext_root, 0.0, 0.0 - 100000.0, 0.0); END     // y el techo del pasillo
    g3d_set_spawn_parent(pl_root);                     // lo que nazca dentro (disparos...) cuelga del mapa
END

// el heroe cruza el portal: se sale al mundo, delante del mapa; el jugador 3D reaparece alli
PROCESS pl_mundo_salir(float lx, float ly)
BEGIN
    IF (NOT pl_dentro) RETURN; END
    pl_sal_x = pl_wxof(lx, 4.5); pl_sal_y = pl_wy + ly + 0.2; pl_sal_z = pl_wzof(lx, 4.5); pl_sal_ok = 1;
    pl_dentro = 0; pl_portal_cd = 90;
    IF (pl_ext_root > 0) g3d_entity_set_position(pl_ext_root, 0.0, 0.0, 0.0); END                   // y vuelve a verse desde fuera
    IF (ps_ext_root > 0) g3d_entity_set_position(ps_ext_root, 0.0, 0.0, 0.0); END
    g3d_set_spawn_parent(-1);
END

// desde fuera: ponerse delante de la puerta del mapa y pulsar E (o S, Abajo) para entrar
PROCESS pl_mundo_vigila()
BEGIN
    LOOP
        IF (NOT pl_dentro AND pl_por_n > 0)
            IF ((pl_in_act() OR pl_in_down() OR pl_in_up()) AND pl_sal_ok == 0 AND pl_portal_cd == 0)      // cualquiera de esas teclas (Arriba = seguir andando hacia la puerta)
                IF (abs(jug_x - pl_wxof(pl_por_x, 2.0)) < 3.0 AND abs(jug_z - pl_wzof(pl_por_x, 2.0)) < 6.5 AND abs(jug_y - (pl_wy + pl_por_y)) < 4.0) pl_mundo_entrar(); END
            END
        END
        IF (pl_portal_cd > 0 AND NOT pl_dentro) pl_portal_cd = pl_portal_cd - 1; END
        pl_ent_prev = pl_in_act() OR pl_in_down();
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  Mapa 2.5D como FACHADA (modo teletransporte): en el mundo 3D solo se ve y se toca la capa Exterior del mapa (si no hay, sus paredes solidas); la
//  puerta (portal 'Salir al mundo 3D') lleva a la escena PROPIA del mapa, completa, con sus caidas y su fondo. Al salir por ese portal se vuelve aqui.
// ---------------------------------------------------------------------------
// tile con el que se hace el cuerpo del edificio en la celda (c, r): el de la fachada
FUNCTION int pl_cuerpo_tile(int c, int r)
BEGIN
    RETURN pl_fct[r * pl_lw + c];
END

FUNCTION pl_montar_fachada(float wx, float wy, float wz, float yaw, int escena, int puerta)
PRIVATE int g; int e; int pc; int mt; int ii; int slot; int g2; int py; int r; int c; int c0; int cr; int t; int hay; int root; int k; float zz; float D;
BEGIN
    IF (pl_nt == 0 AND NOT pl_load_tileset(pl_tilesetfile)) RETURN; END
    IF (pl_box_mesh == 0) pl_box_mesh = g3d_primitive_cube(); END
    IF (pl_wedge_mesh == 0) pl_wedge_mesh = g3d_primitive_wedge(); END
    IF (pl_mat_flag == 0) pl_mat_flag = pl_make_mat("", 0.6, 0.0); g3d_material_set_color(pl_mat_flag, 0.9, 0.15, 0.15, 1.0); END
    pl_lz[6] = 2.2; pl_lsz[6] = 2.0; pl_ls[6] = 1.0;
    pl_wx = wx; pl_wy = wy; pl_wz = wz; pl_wyaw = yaw; pl_vis = 0; pl_zoff = 0.0; pl_mundo = 1;
    g = pl_lv_open(pl_levelfile, 0, 0);
    IF (g <= 0) say("ERROR: no se encuentra el mapa " + pl_levelfile); RETURN; END
    pl_lw = pl_lv_w(g); pl_lh = pl_lv_h(g);
    FOR (r = 0; r < pl_lh * pl_lw; r = r + 1) pl_fcm[r] = 0; pl_fct[r] = 0; END
    root = g3d_entity_spawn(scene, 0, wx, wy, wz);
    g3d_entity_set_rotation(root, 0.0, yaw * 1000.0, 0.0);
    g3d_set_spawn_parent(root);
    // la puerta: el portal 'Salir al mundo 3D' del plano jugable
    pl_por_n = 0;
    FOR (py = 0; py < pl_lh; py = py + 1)
        FOR (c = 0; c < pl_lw; c = c + 1)
            t = pl_lv_get(g, c, py, 0);
            IF (t > 0 AND t <= pl_nt AND pl_t_portal[t] == 4 AND pl_por_n == 0) pl_por_x = c * PL_TS + 1.0; pl_por_y = (pl_lh - 1 - py) * PL_TS; pl_por_n = 1; END
        END
    END
    // lo que se ve: la capa Exterior; si no hay, las paredes solidas del plano jugable
    hay = 0;
    g2 = pl_lv_open(pl_levelfile, 0, 2);
    IF (g2 > 0)
        FOR (py = 0; py < pl_lh; py = py + 1)
            r = pl_lh - 1 - py;
            FOR (c = 0; c < pl_lw; c = c + 1)
                cr = pl_lv_get(g2, c, py, 0);
                IF (cr > 0 AND cr <= pl_nt) pl_tile_cell(cr, c, r, 6); pl_fcm[r * pl_lw + c] = 1; pl_fct[r * pl_lw + c] = cr; hay = 1; END
            END
        END
        pl_lv_close(g2);
    END
    IF (NOT hay)
        FOR (py = 0; py < pl_lh; py = py + 1)
            r = pl_lh - 1 - py;
            FOR (c = 0; c < pl_lw; c = c + 1)
                cr = pl_lv_get(g, c, py, 0);
                IF (cr > 0 AND cr <= pl_nt AND pl_t_solid[cr] AND pl_t_mark[cr] == 0 AND pl_t_portal[cr] == 0) pl_tile_cell(cr, c, r, 6); pl_fcm[r * pl_lw + c] = 1; pl_fct[r * pl_lw + c] = cr; END
            END
        END
    END
    // el CUERPO del edificio: por detras de la pared del fondo se alarga la silueta de la fachada (pared trasera + las dos paredes de los lados de cada tramo),
    // para que al rodearlo sea un edificio con fondo y no una lamina. No se ve desde delante (queda tras el interior).
    D = 20.0;
    FOR (r = 0; r < pl_lh; r = r + 1)
        c = 0;
        WHILE (c < pl_lw)
            IF (pl_fcm[r * pl_lw + c] > 0)
                c0 = c; WHILE (c < pl_lw AND pl_fcm[r * pl_lw + c] > 0) c = c + 1; END
                FOR (k = c0; k < c; k = k + 1)
                    cr = pl_cuerpo_tile(k, r);
                    IF (cr > 0)
                        pl_lz[7] = -3.4 - D + 1.0; pl_lsz[7] = 2.0; pl_ls[7] = 1.0; pl_tile_cell(cr, k, r, 7);              // pared trasera
                        IF (k == c0 OR k == c - 1)
                            FOR (zz = 0; zz < D - 2.0; zz = zz + 2.0) pl_lz[7] = -3.4 - 1.0 - zz; pl_tile_cell(cr, k, r, 7); END   // las paredes de los lados
                        END
                    END
                END
            ELSE
                c = c + 1;
            END
        END
    END
    // lo que hay DENTRO, solo para verlo desde fuera (a traves de la puerta y de los huecos de la fachada): el plano jugable y el fondo, sin enemigos ni mecanismos
    pl_lz[1] = -4.2; pl_lsz[1] = 1.6; pl_ls[1] = 1.0; pl_lz[2] = 2.4; pl_lsz[2] = 0.5; pl_ls[2] = 1.0;
    pl_build_visual(0);
    pl_lv_close(g);
    slot = pl_fz_n;
    pl_fz_puerta[slot] = 0; pl_fz_est[slot] = 0; pl_fz_off[slot] = 0.0;
    IF (puerta AND pl_por_n > 0 AND slot < 8)                 // la puerta (3x3 casillas, tapa el hueco) y su interruptor a un lado
        mt = pl_t_mat[1]; FOR (ii = 1; ii <= pl_nt; ii = ii + 1) IF (pl_t_door[ii] > 0) mt = pl_t_mat[ii]; BREAK; END END
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, mt);
        g3d_entity_set_scale(e, 3.0 * PL_TS, 3.0 * PL_TS, 1.0);
        g3d_entity_set_position(e, pl_por_x, pl_por_y + 1.5 * PL_TS, 2.2);
        pl_fz_ent[slot] = e; pl_fz_puerta[slot] = 1;
        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
        g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, pl_mat_flag);
        g3d_entity_set_scale(e, 0.9, 1.3, 0.5);
        g3d_entity_set_position(e, pl_por_x + 5.2, pl_por_y + 1.8, 3.5);
        g3d_entity_set_color(e, 255, 120, 100);
        pl_fz_sw[slot] = e;
    END
    g3d_set_spawn_parent(-1);
    // colision: una caja por cada tramo seguido de una fila
    FOR (r = 0; r < pl_lh; r = r + 1)
        c = 0;
        WHILE (c < pl_lw)
            IF (pl_fcm[r * pl_lw + c] > 0)
                c0 = c; WHILE (c < pl_lw AND pl_fcm[r * pl_lw + c] > 0) c = c + 1; END
                pl_caja_run(c0, c - 1, r, 1.2, 3.2);
                pl_caja_run(c0, c - 1, r, -3.4 - 20.0, -3.4);                  // el cuerpo del edificio
            ELSE
                c = c + 1;
            END
        END
    END
    IF (pl_fz_puerta[slot])                                   // la puerta cerrada tambien es solida
        pc = (pl_por_x - 1.0) / PL_TS;
        FOR (ii = 0; ii < 3; ii = ii + 1) pl_fz_box[slot * 3 + ii] = pl_caja_run(pc - 1, pc + 1, pl_por_y / PL_TS + ii, 1.2, 3.2); END
        pl_fz_sx[slot] = pl_wxof(pl_por_x + 5.2, 3.5); pl_fz_sz[slot] = pl_wzof(pl_por_x + 5.2, 3.5);
        pl_fz_dx[slot] = pl_por_x; pl_fz_dy[slot] = pl_por_y + 1.5 * PL_TS;
    END
    // y la pared del fondo, para no poder colarse por el hueco de la puerta y andar por dentro
    g = pl_lv_open(pl_levelfile, 0, 0);
    IF (g > 0)
        FOR (r = 0; r < pl_lh * pl_lw; r = r + 1) pl_fcm[r] = 0; END
        FOR (py = 0; py < pl_lh; py = py + 1) FOR (c = 0; c < pl_lw; c = c + 1) IF (pl_lv_get(g, c, py, 1) > 0) pl_fcm[(pl_lh - 1 - py) * pl_lw + c] = 1; END END END
        FOR (r = 0; r < pl_lh; r = r + 1)
            c = 0;
            WHILE (c < pl_lw)
                IF (pl_fcm[r * pl_lw + c] > 0)
                    c0 = c; WHILE (c < pl_lw AND pl_fcm[r * pl_lw + c] > 0) c = c + 1; END
                    pl_caja_run(c0, c - 1, r, -5.0, -3.4);
                ELSE
                    c = c + 1;
                END
            END
        END
        pl_lv_close(g);
    END
    IF (pl_por_n > 0 AND pl_fz_n < 8)
        pl_fz_x[pl_fz_n] = pl_wxof(pl_por_x, 2.0); pl_fz_z[pl_fz_n] = pl_wzof(pl_por_x, 2.0); pl_fz_y[pl_fz_n] = wy + pl_por_y; pl_fz_e[pl_fz_n] = escena;
        pl_fz_n = pl_fz_n + 1;
    END
    pl_mundo = 0;
END

// Fachadas con puerta: el interruptor (E, de cerca) abre la puerta poco a poco; cuando esta abierta del todo, entrar por ella (andar hacia dentro, E, Arriba o Abajo) te
// lleva a la escena del mapa, y se apunta donde estabas para volver. Sin puerta se entra directamente. Los eventos (pulsar, abrirse, entrar) los atiende fachada_eventos.
PROCESS pl_fachada_vigila()
PRIVATE int i; int k; int tecla; int epul;
BEGIN
    LOOP
        epul = pl_in_act() AND NOT pl_fz_eprev;
        tecla = pl_in_act() OR pl_in_down() OR pl_in_up();
        FOR (i = 0; i < pl_fz_n; i = i + 1)
            IF (pl_fz_puerta[i] AND pl_fz_est[i] == 0 AND epul AND abs(jug_x - pl_fz_sx[i]) < 3.2 AND abs(jug_z - pl_fz_sz[i]) < 4.5 AND abs(jug_y - pl_fz_y[i]) < 4.0)
                pl_fz_est[i] = 1; pl_ev_pulsar[i] = 1; g3d_entity_set_color(pl_fz_sw[i], 120, 255, 140);
            END
            IF (pl_fz_est[i] == 1)
                pl_fz_off[i] = pl_fz_off[i] + 0.07;
                g3d_entity_set_position(pl_fz_ent[i], pl_fz_dx[i], pl_fz_dy[i] + pl_fz_off[i], 2.2);
                IF (pl_fz_off[i] >= 6.6)
                    pl_fz_est[i] = 2; pl_ev_abrir[i] = 1;
                    FOR (k = 0; k < 3; k = k + 1) g3d_collider_remove_box(pl_fz_box[i * 3 + k]); END
                END
            END
            IF (tecla AND (NOT pl_fz_puerta[i] OR pl_fz_est[i] == 2) AND vuelta_cd == 0 AND abs(jug_x - pl_fz_x[i]) < 3.0 AND abs(jug_z - pl_fz_z[i]) < 6.5 AND abs(jug_y - pl_fz_y[i]) < 4.0)
                vuelta_escena = escena_actual; vuelta_x = jug_x; vuelta_y = jug_y; vuelta_z = jug_z; vuelta_ok = 1;
                pl_ev_entrar[i] = 1; escena_pedida = pl_fz_e[i];
            END
        END
        pl_fz_eprev = pl_in_act();
        FRAME;
    END
END

FUNCTION pl_montar()
PRIVATE int i;
BEGIN
    pl_reiniciar();
    scene = g3d_scene_create("plataformas");
    g3d_scene_set_active(scene);
    camera = g3d_camera_create();
    g3d_camera_set_active(camera);
    g3d_camera_set_fov(camera, 38.0);

    IF (NOT pl_cambio)                                     // cuantos planos tiene el nivel (<nivel>.p1.png, .p2.png)
        pl_nplanes = 1;
        WHILE (pl_nplanes < 3 AND pl_lv_exists(pl_levelfile, pl_nplanes)) pl_nplanes = pl_nplanes + 1; END
    END
    pl_load_settings();                                    // cielo, niebla, luz y fondo de este nivel
    IF (pl_cor_file != "" AND pl_lv_exists(pl_cor_file, 0))        // alto del pasillo (en celdas)
        i = pl_lv_open(pl_cor_file, 0, 0);
        IF (i > 0) pl_cor_h = pl_lv_h(i); pl_lv_close(i); END
    END
    IF (pl_cor_fin AND pl_nplanes > 1 AND pl_cor_h > 0) pl_pd = 17.0 + pl_cor_h * 2.0 + 4.0; END      // el plano 1 queda justo al final del pasillo
    pl_bgz = -14.0 - pl_pd * (pl_nplanes - 1 - pl_plane); pl_bgk = (30.0 + pl_pd * (pl_nplanes - 1 - pl_plane)) / 30.0;   // el fondo queda detras del ultimo plano
    IF (pl_cor_h > 0 AND pl_plane == 0 AND NOT (pl_cor_fin AND pl_nplanes > 1))   // con pasillo dentro del nivel, el fondo se aleja hasta el final del pasillo
        pl_bgz = pl_bgz - (pl_cor_h * 2.0 + 19.0); pl_bgk = (30.0 - (pl_bgz + 14.0)) / 30.0;
    END
    g3d_sky_set_gradient(pl_sk_r, pl_sk_g, pl_sk_b,  pl_hz_r, pl_hz_g, pl_hz_b);
    g3d_sky_enable(1);
    g3d_set_ambient_light(pl_am_r, pl_am_g, pl_am_b, 1.0);
    light = g3d_light_create(0, pl_li_r, pl_li_g, pl_li_b);
    g3d_light_set_direction(light, 0.35, -0.75, -0.55);
    g3d_light_set_intensity(light, pl_li_i);
    g3d_light_enable_shadow(light, 1);
    g3d_set_shadows(1);
    g3d_set_hdr(1);
    g3d_set_tonemap(1);
    g3d_set_bloom(1, 0.35, 1.0);
    g3d_set_fog(1, pl_fg_r, pl_fg_g, pl_fg_b, pl_fg_n, pl_fg_f);

    pl_g_hero = map_load(pl_hero_sheet);
    pl_g_hero_d = 0; IF (pl_hero_dsheet != "" AND fexists(pl_hero_dsheet)) pl_g_hero_d = map_load(pl_hero_dsheet); END
    FOR (i = 0; i < 2; i = i + 1) IF (pl_en_sheet[i] != "") pl_g_en[i] = map_load(pl_en_sheet[i]); END END
    IF (pl_g_hero <= 0) say("ERROR: no se carga la hoja del heroe " + pl_hero_sheet); RETURN; END
    pl_hmaxhp = pl_hero_hp; IF (NOT pl_cambio) pl_hhp = pl_hero_hp; END

    // los disparos son bolas brillantes (no necesitan hoja de sprites)
    pl_bull_mesh = g3d_primitive_sphere(12);
    pl_mat_bh = g3d_material_create();  g3d_material_set_color(pl_mat_bh, 1.0, 0.9, 0.3, 1.0);
    pl_mat_bhc = g3d_material_create(); g3d_material_set_color(pl_mat_bhc, 0.4, 0.9, 1.0, 1.0);
    pl_mat_be = g3d_material_create();  g3d_material_set_color(pl_mat_be, 1.0, 0.45, 0.2, 1.0);

    IF (NOT pl_build_level()) RETURN; END
    pl_heroe();
    IF (pl_cor_found AND pl_plane == 0 AND pl_cor_file != "" AND pl_lv_exists(pl_cor_file, 0)) ps_gestor(); END      // el pasillo de este nivel, detras del portal
    FOR (i = 0; i < pl_nsp; i = i + 1)
        IF (pl_spt[i] == 2) pl_tirador(pl_spx[i], pl_spy[i]); END
        IF (pl_spt[i] == 3) pl_perseguidor(pl_spx[i], pl_spy[i]); END
    END
    IF (pl_hud_on)
        write(0, 20, 20, 0, "VIDA:");     write_var(0, 80, 20, 0, pl_hhp);
        write(0, 20, 44, 0, "PUNTOS:");   write_var(0, 100, 44, 0, pl_score);
    END
    pl_camara(camera);
END

// Pasa al plano de detras (dir > 0) o al de delante (dir < 0) con la camara avanzando en z. Se reconstruye la escena
// con el plano nuevo en z = 0, asi que el cambio no se nota: hacia el fondo se camina primero y se reconstruye al llegar;
// hacia delante se reconstruye primero (con el heroe aun en el fondo) y luego se camina hacia la camara.
PROCESS pl_cambio_plano(int sentido, float destx, float desty)
PRIVATE int i; float cx; float cy;
BEGIN
    pl_trans = 1; pl_trans_dir = sentido;
    IF (pl_cor_pass)                                   // viene del final del pasillo: la camara conserva su pose (en las coordenadas del plano nuevo)
        FOR (i = 0; i < 6; i = i + 1) pl_cam_keep[i] = pl_cam_last[i]; END
        pl_cam_keep[2] = pl_cam_keep[2] + pl_pd; pl_cam_keep[5] = pl_cam_keep[5] + pl_pd; pl_cam_keep_on = 1;
    END
    IF (sentido > 0 AND NOT pl_cor_pass)
        FOR (i = 1; i <= 36; i = i + 1) pl_tz = -pl_pd * (1.0 - cos(i * 5000.0)) / 2.0; FRAME; END   // 0 .. -pd con suavizado
    END
    // se quita el plano viejo (procesos por nombre y escena del motor) y se monta el nuevo
    signal(TYPE pl_heroe, S_KILL); signal(TYPE pl_camara, S_KILL);
    signal(TYPE pl_tirador, S_KILL); signal(TYPE pl_perseguidor, S_KILL);
    signal(TYPE pl_bala_heroe, S_KILL); signal(TYPE pl_bala_enemigo, S_KILL);
    signal(TYPE ps_gestor, S_KILL); signal(TYPE ps_heroe, S_KILL); signal(TYPE ps_enemigo, S_KILL); signal(TYPE ps_mecanismos, S_KILL); signal(TYPE ps_bala, S_KILL);
    signal(TYPE pl_fondo, S_KILL); signal(TYPE pl_circuitos, S_KILL); signal(TYPE pl_liquidos, S_KILL); signal(TYPE pl_plataforma, S_KILL); signal(TYPE pl_hmodelo, S_KILL);
    FRAME;                                             // los procesos muertos sueltan sus entidades antes de destruir la escena
    pl_descargar();
    pl_plane = pl_plane + sentido;
    pl_startx = destx; pl_starty = desty;
    pl_cambio = 1;
    IF (sentido < 0) pl_tz = -pl_pd; ELSE pl_tz = 0.0; END
    pl_trans = 1;
    pl_montar();
    pl_cambio = 0;
    IF (sentido < 0)
        FOR (i = 1; i <= 36; i = i + 1) pl_tz = -pl_pd * (1.0 + cos(i * 5000.0)) / 2.0; FRAME; END
    END
    pl_tz = 0.0; pl_trans = 0; pl_cor_pass = 0;
END

// Quita lo que monto pl_montar(): los procesos los mata el editor por su nombre; esto suelta la escena del motor y los textos
FUNCTION pl_descargar()
BEGIN
    FRAME;                                     // los procesos muertos sueltan sus entidades antes de destruir la escena
    write_delete(all_text);
    g3d_scene_destroy(scene);
END
