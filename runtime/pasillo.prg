// ============================================================================
//  pasillo.prg - tramos en PROFUNDIDAD estilo Crash Bandicoot (camara detras del heroe, se corre hacia la pantalla).
//  Biblioteca del editor (modo Plataformas). Necesita plataformas.prg incluido ANTES: reutiliza el tileset (pl_load_tileset),
//  la configuracion del heroe (pl_hero_*) y su hoja de espaldas/frente. Todo lo de aqui lleva el prefijo ps_.
//
//  El mapa es una imagen vista DESDE ARRIBA: una columna = un carril (x), una fila = un paso de profundidad; la fila de abajo
//  de la imagen es el principio del pasillo y la de arriba el final.
//     R = tile del suelo de esa celda (0 = HUECO: se cae)         -> propiedades del tileset (dano, hielo...)
//     G = altura de la columna en bloques (0 = suelo a ras, 1 = un bloque mas alto...)
//     B = marcador (tile con marcador=inicio | check): donde empieza el heroe y los puntos de control
//  Uso:  pl_levelfile = "Assets/Levels/pasillo1.png"; pl_tilesetfile = "...tileset.def";  (config del heroe)  ps_montar();
// ============================================================================
CONST
    PS_TS = 2.0;        // tamano de cada celda (igual que un bloque de plataformas)
END

GLOBAL
    int ps_w; int ps_h;                       // tamano del pasillo en celdas
    float ps_top[39999];                      // altura de la superficie de cada celda (-999 = hueco)
    int ps_tile[39999];                       // tile de cada celda (para el dano, el hielo...)
    int ps_tk[10]; int ps_cube_mesh; int ps_wedge_mesh; int ps_cmesh[10]; int ps_pdn; float ps_pdx[15]; float ps_pdz[15]; string ps_pdd[15]; int ps_pcd; string ps_file; int ps_arr; string ps_arrn; float ps_arrx; float ps_arrz; int ps_dcm[41]; int ps_ndeco;       // mallas de columna por altura (ps_cmesh[n]: n+1 bloques)
    float ps_startx = 1.0; float ps_startz = -1.0;
    float ps_cpx[15]; float ps_cpz[15]; int ps_cpon[15]; int ps_ncp;
    int ps_done;                              // 1 al llegar al final
    float ps_camdist = 11.0; float ps_camalt = 5.5;      // distancia detras del heroe y altura de la camara
    int ps_embed;      // el pasillo va DENTRO de un nivel de plataformas: desplazamiento (en el mundo) del origen del pasillo
    float ps_arm_s = 1.0; float ps_arm_l; float ps_cam_x; float ps_cam_y; float ps_cam_z;   // brazo de camara: fraccion libre y posicion resultante
    int ps_blon[16]; float ps_blx[16]; float ps_bly[16]; float ps_blz[16];     // disparos del heroe en el pasillo (para que los enemigos los vean)
    int ps_paso;                                              // ps_paso: 1 = el heroe ENTRA al pasillo andando desde el mapa, -1 = SALE andando hacia el mapa (sin desaparecer)
    int ps_busy; int ps_back;                 // ps_back: se salio del pasillo hacia ATRAS (por donde se entro), no por el final
    int ps_ramp[39999];                       // 1 rampa que sube hacia +x, 2 hacia -x (la altura de ps_top es la del lado bajo)
    // mecanismos del pasillo: solidos que se mueven (ascensores, puertas), botones y dianas, y sus canales
    int ps_dn; float ps_dx[63]; float ps_dz[63]; float ps_db[63]; float ps_dh[63]; float ps_dpy[63]; float ps_dpx[63]; int ps_dk[63]; int ps_dt[63]; int ps_de[63]; float ps_dpos[63]; int ps_ddir[63]; int ps_dwait[63]; float ps_dpr[63];
    int ps_nb; float ps_bx[31]; float ps_bz[31]; float ps_bb[31]; int ps_be[31]; int ps_bt[31];
    int ps_ng; float ps_gx[31]; float ps_gz[31]; int ps_ge[31]; int ps_gt[31];
    int ps_chan[160]; int ps_pressed[160]; int ps_hold[160]; int ps_tog[160]; int ps_bch[31]; int ps_gch[31]; int ps_dex[63]; int ps_dns[63]; int ps_dsc[504]; int ps_pdl[15]; float ps_cx; float ps_cy; int ps_fmat;
    float ps_spx[63]; float ps_spz[63]; int ps_spt[63]; int ps_nsp;      // marcadores de enemigo (2 Met, 3 el que persigue)
    float ps_hvy; int ps_dmg; int ps_bounce; int ps_spin;               // el heroe y los enemigos del pasillo se hablan por aqui: velocidad vertical, dano pedido, rebote tras pisar, giro de ataque
    int ps_liqmark[39999];                    // celdas de liquido ya unidas en un cuerpo
    int ps_hid;                               // proceso del heroe del pasillo (modo embebido)
    int ps_vista;                             // 1 = el final se ve en la camara (para el fundido)
END

FUNCTION float ps_cell_top(float px, float pz, int modo, float sx)
PRIVATE int c; int rb; float f;
BEGIN
    // modo 0 (choques): fuera del pasillo es pared; modo 1 (suelo): fuera del pasillo es vacio
    IF (px < 0.0 OR pz > 0.0 OR px >= ps_w * PS_TS OR -pz >= ps_h * PS_TS)
        IF (modo == 0) RETURN 99.0; ELSE RETURN -999.0; END
    END
    c = px / PS_TS; rb = (0.0 - pz) / PS_TS;
    IF (ps_ramp[rb * ps_w + c] > 0)                                   // rampa: la altura sube por el lado x segun donde esta el centro de la huella (sx)
        f = (sx - c * PS_TS) / PS_TS; IF (f < 0.0) f = 0.0; END IF (f > 1.0) f = 1.0; END
        IF (ps_ramp[rb * ps_w + c] == 2) f = 1.0 - f; END
        RETURN ps_top[rb * ps_w + c] + f * PS_TS;
    END
    RETURN ps_top[rb * ps_w + c];
END

// altura maxima bajo la huella del heroe (un cuadrado de radio r)
FUNCTION float ps_ftop(float px, float pz, float r, int modo)
PRIVATE float a; float b; float t;
BEGIN
    t = ps_cell_top(px - r, pz - r, modo, px);
    a = ps_cell_top(px + r, pz - r, modo, px); IF (a > t) t = a; END
    a = ps_cell_top(px - r, pz + r, modo, px); IF (a > t) t = a; END
    a = ps_cell_top(px + r, pz + r, modo, px); IF (a > t) t = a; END
    RETURN t;
END

// Lo mas alto que pisa o choca con un solido que se mueve (ascensor, puerta) bajo la huella (px, pz, r) de alguien con los pies en ly.
// modo 1 (suelo): solo cuenta lo que esta a los pies o debajo; modo 0 (choques): cuenta lo que se interpone con el cuerpo (2.5 de alto)
FUNCTION float ps_dyn_top(float px, float pz, float r, int modo, float ly)
PRIVATE int i; float t; float b; float best;
BEGIN
    best = -999.0;
    FOR (i = 0; i < ps_dn; i = i + 1)
        IF (abs(px - (ps_dx[i] + ps_dpx[i])) < 1.0 + r AND abs(pz - ps_dz[i]) < 1.0 + r)
            b = ps_db[i] + ps_dpy[i]; t = b + ps_dh[i];
            IF (modo == 1)
                IF (t <= ly + 0.6 AND t > best) best = t; END
            ELSE
                IF (b < ly + 2.5 AND t > best) best = t; END
            END
        END
    END
    RETURN best;
END

FUNCTION float ps_ftop2(float px, float pz, float r, int modo, float ly)
PRIVATE float a; float b;
BEGIN
    a = ps_ftop(px, pz, r, modo); b = ps_dyn_top(px, pz, r, modo, ly);
    IF (b > a) a = b; END
    RETURN a;
END

FUNCTION int ps_tile_at(float px, float pz)
PRIVATE int c; int rb;
BEGIN
    IF (px < 0.0 OR pz > 0.0 OR px >= ps_w * PS_TS OR -pz >= ps_h * PS_TS) RETURN 0; END
    c = px / PS_TS; rb = (0.0 - pz) / PS_TS;
    RETURN ps_tile[rb * ps_w + c];
END

// ---------------------------------------------------------------------------
//  Construccion del pasillo: una pila de cubos por celda
// ---------------------------------------------------------------------------
// Liquidos: las celdas contiguas con el mismo liquido y la misma altura se funden en un solo cuerpo (sin rejillas ni juntas)
FUNCTION int ps_build_liquids()
PRIVATE int r; int c; int t; int w; int d; int ok; int i; int j; int e; float tp; float cx; float cz;
BEGIN
    FOR (i = 0; i < ps_w * ps_h; i = i + 1) ps_liqmark[i] = 0; END
    FOR (r = 0; r < ps_h; r = r + 1)
        FOR (c = 0; c < ps_w; c = c + 1)
            t = ps_tile[r * ps_w + c];
            IF (t > 0 AND ps_liqmark[r * ps_w + c] == 0)
                IF (pl_t_liq[t] > 0 AND pl_t_tex[t] != "")
                    tp = ps_top[r * ps_w + c];
                    w = 1;
                    WHILE (c + w < ps_w AND ps_tile[r * ps_w + c + w] == t AND ps_top[r * ps_w + c + w] == tp AND ps_liqmark[r * ps_w + c + w] == 0) w = w + 1; END
                    d = 1; ok = 1;
                    WHILE (ok AND r + d < ps_h)
                        FOR (i = 0; i < w; i = i + 1)
                            IF (ps_tile[(r + d) * ps_w + c + i] != t OR ps_top[(r + d) * ps_w + c + i] != tp OR ps_liqmark[(r + d) * ps_w + c + i] != 0) ok = 0; END
                        END
                        IF (ok) d = d + 1; END
                    END
                    FOR (j = 0; j < d; j = j + 1) FOR (i = 0; i < w; i = i + 1) ps_liqmark[(r + j) * ps_w + c + i] = 1; END END
                    cx = (c + w / 2.0) * PS_TS + ps_ox; cz = -((r + d / 2.0) * PS_TS) + ps_oz;
                    e = g3d_entity_spawn(scene, 0, 0, 0, 0);                              // el lecho, opaco
                    g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
                    g3d_entity_set_scale(e, w * PS_TS, PS_TS, d * PS_TS);
                    g3d_entity_set_position(e, cx, tp - 1.0 + ps_oy, cz);
                    e = g3d_entity_spawn(scene, 0, 0, 0, 0);                              // el liquido, translucido, hasta casi el borde
                    g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[t]);
                    g3d_entity_set_scale(e, w * PS_TS, 1.4, d * PS_TS);
                    g3d_entity_set_position(e, cx, tp + 0.7 + ps_oy, cz);
                    g3d_entity_set_blend(e, 1); g3d_entity_set_alpha(e, pl_t_alpha[t]);
                END
            END
        END
    END
    RETURN 1;
END

FUNCTION int ps_fmat_or(int tile)
BEGIN
    IF (ps_fmat > 0) RETURN ps_fmat; END
    RETURN pl_t_mat[tile];
END

FUNCTION int ps_build_file(string fich)
PRIVATE
    int g; int c; int py; int rb; int cr; int cg; int cb; int ca; int k; int n; int e; int t; int m; float yy;
    int g2; int r2; int gg2; int b2; int a2; int b; int s0; int cnt; int tt; string fn2;
BEGIN
    IF (pl_nt == 0 AND NOT pl_load_tileset(pl_tilesetfile)) RETURN 0; END
    g = pl_lv_open(fich, 0, 0);
    IF (g <= 0) say("ERROR: no se encuentra el pasillo " + fich); RETURN 0; END
    ps_w = pl_lv_w(g); ps_h = pl_lv_h(g);
    pl_lk_load(g, 0, 1);
    g2 = pl_lv_open(fich, 0, 1);                                    // capas Piso 2 (R), Piso 3 (G) y Piso 4+ (B) del muro
    IF (g2 > 0 AND (pl_lv_w(g2) != ps_w OR pl_lv_h(g2) != ps_h)) pl_lv_close(g2); g2 = 0; END
    IF (ps_w * ps_h > 39999) say("ERROR: el pasillo es demasiado grande (max 39999 celdas)"); RETURN 0; END
    say("pasillo " + fich + ": " + itoa(ps_w) + " carriles x " + itoa(ps_h) + " pasos");
    ps_dn = 0; ps_nb = 0; ps_ng = 0; ps_fmat = 0; ps_cx = 0.0; ps_cy = 0.0;
    FOR (k = 0; k < 160; k = k + 1) ps_chan[k] = 0; ps_pressed[k] = 0; ps_hold[k] = 0; ps_tog[k] = 0; END
    ps_pdn = 0; ps_pcd = 60; ps_arr = 0; ps_file = fich; ps_arrn = "";
    ps_ncp = 0; ps_nsp = 0; ps_startx = ps_w * PS_TS / 2.0; ps_startz = -1.0;
    ps_cube_mesh = g3d_primitive_cube(); ps_wedge_mesh = g3d_primitive_wedge();
    FOR (k = 0; k < 10; k = k + 1) ps_cmesh[k] = 0; END
    IF (ps_embed)                                           // primero se busca el inicio: el pasillo se coloca detras del portal, con su inicio alineado a el
        FOR (py = 0; py < ps_h; py = py + 1)
            FOR (c = 0; c < ps_w; c = c + 1)
                cb = pl_lv_get(g, c, py, 2);
                IF (cb > 0 AND cb <= pl_nt AND pl_t_mark[cb] == 1) ps_startx = c * PS_TS + 1.0; END
            END
        END
        ps_ox = pl_cor_px - ps_startx; ps_oy = pl_cor_py; ps_oz = -5.4;       // pegado detras del nivel: empieza justo tras su pared del fondo, en el hueco de la puerta (el portal)
    END
    FOR (py = 0; py < ps_h; py = py + 1)
        rb = ps_h - 1 - py;
        FOR (c = 0; c < ps_w; c = c + 1)
            cr = pl_lv_get(g, c, py, 0); cg = pl_lv_get(g, c, py, 1); cb = pl_lv_get(g, c, py, 2);
            IF (cr > pl_nt) cr = 0; END
            IF (cb > pl_nt) cb = 0; END
            r2 = 0; gg2 = 0; b2 = 0; IF (g2 > 0) r2 = pl_lv_get(g2, c, py, 0); gg2 = pl_lv_get(g2, c, py, 1); b2 = pl_lv_get(g2, c, py, 2); END
            ps_tile[rb * ps_w + c] = cr; ps_top[rb * ps_w + c] = -999.0; ps_ramp[rb * ps_w + c] = 0;
            IF (cb > 0 AND pl_t_portal[cb] == 2 AND pl_t_dest[cb] != "" AND ps_pdn < 15)       // puerta lateral a otra sala: un panel con la textura del tile, que sobresale del muro
                ps_pdx[ps_pdn] = c * PS_TS + 1.0; ps_pdz[ps_pdn] = -(rb * PS_TS + 1.0); ps_pdd[ps_pdn] = pl_t_dest[cb]; ps_pdl[ps_pdn] = pl_lk_find(1, c, py); ps_pdn = ps_pdn + 1;
                e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[cb]);
                g3d_entity_set_scale(e, 1.0, PS_TS * 1.7, PS_TS * 0.9);                      // panel fino pegado a la cara del muro que da al pasillo, sobresaliendo 0.4
                IF (c * PS_TS + 1.0 < ps_w * PS_TS / 2.0) g3d_entity_set_position(e, (c + 1) * PS_TS - 0.1 + ps_ox, PS_TS * 0.85 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                ELSE g3d_entity_set_position(e, c * PS_TS + 0.1 + ps_ox, PS_TS * 0.85 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz); END
                IF (NOT ps_arr AND ((pl_arr_door != "" AND ps_pdl[ps_pdn - 1] >= 0 AND pl_lk_name[ps_pdl[ps_pdn - 1]] == pl_arr_door) OR (pl_arr_door == "" AND pl_arr_from != "" AND pl_t_dest[cb] == pl_arr_from)))             // llegas por ella: apareces junto a la puerta, hacia dentro del pasillo
                    ps_arr = 1; ps_arrz = -(rb * PS_TS + 1.0);
                    IF (c * PS_TS + 1.0 < ps_w * PS_TS / 2.0) ps_arrx = (c + 1) * PS_TS + 1.0; ELSE ps_arrx = (c - 1) * PS_TS + 1.0; END
                END
            END
            IF (cb > 0)                                         // marcador: inicio o punto de control
                IF (pl_t_mark[cb] == 1) ps_startx = c * PS_TS + 1.0; ps_startz = -(rb * PS_TS + 1.0); END
                IF ((pl_t_mark[cb] == 2 OR pl_t_mark[cb] == 3) AND ps_nsp < 63) ps_spx[ps_nsp] = c * PS_TS + 1.0; ps_spz[ps_nsp] = -(rb * PS_TS + 1.0); ps_spt[ps_nsp] = pl_t_mark[cb] - 2; ps_nsp = ps_nsp + 1; END
                IF (pl_t_mark[cb] == 4 AND ps_ncp < 15) ps_cpx[ps_ncp] = c * PS_TS + 1.0; ps_cpz[ps_ncp] = -(rb * PS_TS + 1.0); ps_cpon[ps_ncp] = 0; ps_ncp = ps_ncp + 1; END
            END
            IF (cr > 0 AND pl_t_door[cr] > 0 AND ps_dn < 63)
                // puerta: una columna que se aparta (hacia arriba, o hacia abajo con abre=abajo) mientras su canal esta activo
                n = cg; IF (n < 1) n = 1; END IF (n > 9) n = 9; END
                ps_top[rb * ps_w + c] = 0.0;
                IF (ps_cmesh[n] == 0) ps_cmesh[n] = g3d_primitive_column(n + 1); END
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[cr]);
                g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, -1.0 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cmesh[n]); g3d_entity_set_material(e, pl_t_mat[cr]);
                g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                ps_dex[ps_dn] = pl_lk_door(1, c, c, py, py); ps_dns[ps_dn] = pl_lk_ns; FOR (k = 0; k < pl_lk_ns; k = k + 1) ps_dsc[ps_dn * 8 + k] = pl_lk_s[k]; END
                ps_de[ps_dn] = e; ps_dk[ps_dn] = 2; ps_dt[ps_dn] = cr; ps_dx[ps_dn] = c * PS_TS + 1.0; ps_dz[ps_dn] = -(rb * PS_TS + 1.0);
                ps_db[ps_dn] = 0.0; ps_dh[ps_dn] = (n + 1) * PS_TS; ps_dpy[ps_dn] = 0.0; ps_dpx[ps_dn] = 0.0; ps_dpr[ps_dn] = 0.0; ps_dn = ps_dn + 1;
            ELSE IF (cr > 0 AND pl_t_axis[cr] > 0 AND ps_dn < 63)
                // ascensor / plataforma que se mueve (mueve=x:... lateral, y:... arriba y abajo; activa=pisar: solo sube mientras vas encima)
                n = cg; IF (n > 9) n = 9; END
                ps_top[rb * ps_w + c] = -999.0;
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[cr]);
                g3d_entity_set_scale(e, PS_TS, 0.6, PS_TS);
                ps_dex[ps_dn] = pl_lk_door(1, c, c, py, py); ps_dns[ps_dn] = pl_lk_ns; FOR (k = 0; k < pl_lk_ns; k = k + 1) ps_dsc[ps_dn * 8 + k] = pl_lk_s[k]; END
                ps_de[ps_dn] = e; ps_dk[ps_dn] = 1; ps_dt[ps_dn] = cr; ps_dx[ps_dn] = c * PS_TS + 1.0; ps_dz[ps_dn] = -(rb * PS_TS + 1.0);
                ps_db[ps_dn] = n * PS_TS - 0.6; ps_dh[ps_dn] = 0.6; ps_dpy[ps_dn] = 0.0; ps_dpx[ps_dn] = 0.0; ps_dpos[ps_dn] = 0.0; ps_ddir[ps_dn] = 1; ps_dwait[ps_dn] = 0; ps_dn = ps_dn + 1;
            ELSE IF (cr > 0 AND pl_t_btn[cr] > 0 AND ps_nb < 31)
                // boton: una placa a ras de suelo; se activa al pisarla
                n = cg; IF (n > 9) n = 9; END
                ps_top[rb * ps_w + c] = n * PS_TS;
                IF (n < 1) n = 1; END
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, ps_fmat_or(cr));
                g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, ps_top[rb * ps_w + c] - 1.0 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[cr]);
                g3d_entity_set_scale(e, PS_TS * 0.8, 0.3, PS_TS * 0.8);
                g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, ps_top[rb * ps_w + c] + 0.15 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                ps_be[ps_nb] = e; ps_bch[ps_nb] = pl_lk_ch(1, c, py, pl_t_btn[cr]); ps_bt[ps_nb] = cr; ps_bx[ps_nb] = c * PS_TS + 1.0; ps_bz[ps_nb] = -(rb * PS_TS + 1.0); ps_bb[ps_nb] = ps_top[rb * ps_w + c]; ps_nb = ps_nb + 1;
            ELSE IF (cr > 0 AND pl_t_tgt[cr] > 0 AND ps_ng < 31)
                // diana: un pilar que se golpea con el giro (J / X / Z) y alterna su canal
                n = cg; IF (n < 1) n = 1; END IF (n > 9) n = 9; END
                ps_top[rb * ps_w + c] = n * PS_TS;
                IF (ps_cmesh[n] == 0) ps_cmesh[n] = g3d_primitive_column(n + 1); END
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cmesh[n]); g3d_entity_set_material(e, pl_t_mat[cr]);
                g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, (n - 1) * PS_TS / 2.0 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                ps_gch[ps_ng] = pl_lk_ch(1, c, py, pl_t_tgt[cr]); ps_ge[ps_ng] = e; ps_gt[ps_ng] = cr; ps_gx[ps_ng] = c * PS_TS + 1.0; ps_gz[ps_ng] = -(rb * PS_TS + 1.0); ps_ng = ps_ng + 1;
            ELSE IF (cr > 0 AND pl_t_ramp[cr] > 0)
                // rampa: una losa a 45 grados que sube hacia +x (der) o -x (izq); sobre un suelo
                n = cg; IF (n > 9) n = 9; END
                ps_top[rb * ps_w + c] = n * PS_TS; ps_ramp[rb * ps_w + c] = pl_t_ramp[cr];
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, ps_fmat_or(cr));
                g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, n * PS_TS - 1.0 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);                  // la cuna: maciza, con los laterales texturizados
                g3d_entity_set_mesh(e, ps_wedge_mesh); g3d_entity_set_material(e, pl_t_mat[cr]);
                g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                IF (pl_t_ramp[cr] == 2) g3d_entity_set_rotation(e, 0.0, 180000.0, 0.0); END
                g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, n * PS_TS + 1.0 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
            ELSE IF (cr > 0 AND pl_t_liq[cr] > 0 AND pl_t_tex[cr] != "")
                // liquido: una cubeta con el lecho 1.5 por debajo de la altura de la celda, llena hasta casi el borde (translucida)
                n = cg; IF (n > 9) n = 9; END
                ps_top[rb * ps_w + c] = n * PS_TS - 1.5;
                // (los cuerpos de liquido se construyen luego, fundiendo las celdas contiguas: ps_build_liquids)
            ELSE IF (cr > 0)
                n = cg;                                        // altura en bloques
                ps_top[rb * ps_w + c] = n * PS_TS;
                IF (pl_t_axis[cr] == 0 AND pl_t_door[cr] == 0 AND pl_t_btn[cr] == 0 AND pl_t_tgt[cr] == 0 AND n == 0) ps_fmat = pl_t_mat[cr]; END
                // la celda es UNA entidad: un cubo a ras (n = 0) o una columna de n+1 bloques con la textura repetida por bloque.
                // (una sola malla: asi, al volverse translucida por tapar al heroe, no se acumulan capas, y se gastan menos entidades)
                IF (n > 9) n = 9; END
                // los bloques, de abajo arriba: el tile de Suelo en los dos primeros; Piso 2 / Piso 3 / Piso 4+ (capas.png) en los siguientes (un piso vacio sigue con el de debajo)
                ps_tk[0] = cr; ps_tk[1] = cr;
                FOR (b = 2; b <= n; b = b + 1)
                    ps_tk[b] = ps_tk[b - 1];
                    IF (b == 2 AND r2 > 0 AND r2 <= pl_nt) ps_tk[b] = r2; END
                    IF (b == 3 AND gg2 > 0 AND gg2 <= pl_nt) ps_tk[b] = gg2; END
                    IF (b >= 4 AND b2 > 0 AND b2 <= pl_nt) ps_tk[b] = b2; END
                END
                s0 = 0;
                WHILE (s0 <= n)                                      // un tramo de bloques con el mismo tile = UNA entidad (columna con la textura repetida por bloque)
                    b = s0; WHILE (b + 1 <= n AND ps_tk[b + 1] == ps_tk[s0]) b = b + 1; END
                    cnt = b - s0 + 1;
                    IF (cnt >= 2 AND ps_cmesh[cnt - 1] == 0) ps_cmesh[cnt - 1] = g3d_primitive_column(cnt); END
                    e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                    IF (cnt >= 2) g3d_entity_set_mesh(e, ps_cmesh[cnt - 1]); ELSE g3d_entity_set_mesh(e, ps_cube_mesh); END
                    g3d_entity_set_material(e, pl_t_mat[ps_tk[s0]]);
                    g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
                    g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, (s0 - 1 + b) * PS_TS / 2.0 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                    IF (n >= 1) g3d_entity_set_xray(e, 1); END       // los muros se vuelven translucidos cuando tapan al heroe
                    s0 = b + 1;
                END
                tt = ps_tk[n];
                IF (pl_t_matop[tt] > 0)                              // tapa superior con otra textura
                    e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                    g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_matop[tt]);
                    g3d_entity_set_scale(e, PS_TS, 0.25, PS_TS);
                    g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, n * PS_TS + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                    IF (n >= 1) g3d_entity_set_xray(e, 1); END
                END
                IF (pl_t_shape[cr] == 1)                        // pinchos sobre el suelo: tres diamantes en fila
                    FOR (k = 0; k < 3; k = k + 1)
                        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                        g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[cr]);
                        g3d_entity_set_scale(e, 0.7, 0.7, 0.7);
                        g3d_entity_set_position(e, c * PS_TS + 0.4 + k * 0.6 + ps_ox, n * PS_TS + 0.4 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                        g3d_entity_set_rotation(e, 45000.0, 0.0, 45000.0);
                    END
                END
            END END END END END END END
        END
    END
    pl_lv_close(g); IF (g2 > 0) pl_lv_close(g2); END
    IF (ps_arr) ps_startx = ps_arrx; ps_startz = ps_arrz; pl_arr_from = ""; pl_ret_on = 0; END
    pl_arr_door = "";      // llegada por una puerta lateral
    ps_build_liquids();
    ps_ndeco = 0; ps_build_deco(fich);
    IF (pl_mundo)                                          // capa EXTERIOR (techo): lo que se ve desde fuera del mapa; se aparta al entrar
        g = pl_lv_open(fich, 0, 2);
        IF (g > 0)
            ps_ext_root = g3d_entity_spawn(scene, 0, 0, 0, 0);
            g3d_set_spawn_parent(ps_ext_root);
            FOR (py = 0; py < ps_h; py = py + 1)
                rb = ps_h - 1 - py;
                FOR (c = 0; c < ps_w; c = c + 1)
                    cr = pl_lv_get(g, c, py, 0);
                    IF (cr > 0 AND cr <= pl_nt)
                        n = ps_top[rb * ps_w + c] / PS_TS; IF (n < 0) n = 0; END
                        e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                        g3d_entity_set_mesh(e, ps_cube_mesh); g3d_entity_set_material(e, pl_t_mat[cr]);
                        g3d_entity_set_scale(e, PS_TS, 1.0, PS_TS);
                        g3d_entity_set_position(e, c * PS_TS + 1.0 + ps_ox, n * PS_TS + 4.5 + ps_oy, -(rb * PS_TS + 1.0) + ps_oz);
                    END
                END
            END
            g3d_set_spawn_parent(pl_root);
            pl_lv_close(g);
        END
    END
    RETURN 1;
END

FUNCTION int ps_build()
BEGIN
    RETURN ps_build_file(pl_levelfile);
END

// ---------------------------------------------------------------------------
//  Mecanismos del pasillo: ascensores, puertas, botones y dianas (los mismos tiles que en el nivel 2D).
//  Botones (se pisan) y dianas (se golpean con el giro) activan CANALES; una puerta del canal N se aparta mientras el canal este activo.
// ---------------------------------------------------------------------------
PROCESS ps_mecanismos()
PRIVATE int i; int k; int t; float pr; float lim; float H; int riding; int over; float ox0; float oy0; int d; float dt = 0.0166667;
END
BEGIN
    LOOP
        IF (ps_embed AND pl_mode != 1) FRAME; CONTINUE; END
        FOR (k = 1; k < 160; k = k + 1) ps_pressed[k] = 0; END
        FOR (i = 0; i < ps_nb; i = i + 1)
            t = ps_bt[i];
            IF (abs(ps_hx - ps_bx[i]) < 1.1 AND abs(ps_hz - ps_bz[i]) < 1.1 AND ps_hy < ps_bb[i] + 0.9 AND ps_hy > ps_bb[i] - 0.5)
                ps_pressed[ps_bch[i]] = 1;
                IF (pl_t_hold[t] < 0.0) ps_tog[ps_bch[i]] = 1; ELSE IF (pl_t_hold[t] * 60.0 > ps_hold[ps_bch[i]]) ps_hold[ps_bch[i]] = pl_t_hold[t] * 60.0; END END
                g3d_entity_set_color(ps_be[i], 120, 255, 140);
            ELSE
                g3d_entity_set_color(ps_be[i], 255, 170, 150);
            END
        END
        FOR (i = 0; i < ps_ng; i = i + 1)                      // diana: cada giro cerca la alterna
            t = ps_gch[i];
            IF (ps_spin == 19 AND abs(ps_hx - ps_gx[i]) < 3.2 AND abs(ps_hz - ps_gz[i]) < 3.2) ps_tog[t] = 1 - ps_tog[t]; END
        END
        FOR (k = 1; k < 160; k = k + 1)
            IF (ps_hold[k] > 0 AND NOT ps_pressed[k]) ps_hold[k] = ps_hold[k] - 1; END
            IF (ps_pressed[k] OR ps_tog[k] OR ps_hold[k] > 0) ps_chan[k] = 1; ELSE ps_chan[k] = 0; END
        END
        FOR (i = 0; i < ps_ng; i = i + 1)
            IF (ps_chan[ps_gch[i]]) g3d_entity_set_color(ps_ge[i], 120, 255, 140); ELSE g3d_entity_set_color(ps_ge[i], 255, 170, 150); END
        END
        FOR (i = 0; i < ps_dn; i = i + 1)
            t = ps_dt[i]; H = ps_dh[i];
            IF (ps_dk[i] == 2)                                 // puerta
                pr = ps_dpr[i];
                over = ps_chan[pl_t_door[t]];
                IF (ps_dex[i]) over = 0; FOR (k = 0; k < ps_dns[i]; k = k + 1) IF (ps_chan[ps_dsc[i * 8 + k]]) over = 1; END END END
                IF (over)
                    pr = pr + 6.0 * dt / H; IF (pr > 1.0) pr = 1.0; END
                ELSE
                    over = (abs(ps_hx - ps_dx[i]) < 1.7 AND abs(ps_hz - ps_dz[i]) < 1.7 AND ps_hy < ps_db[i] + ps_dpy[i] + H AND ps_hy + 2.5 > ps_db[i] + ps_dpy[i]);
                    IF (NOT over) pr = pr - 6.0 * dt / H; IF (pr < 0.0) pr = 0.0; END END      // no se cierra encima del heroe
                END
                ps_dpr[i] = pr;
                IF (pl_t_dir[t] == 2) ps_dpy[i] = -pr * H; ELSE ps_dpy[i] = pr * H; END
                g3d_entity_set_position(ps_de[i], ps_dx[i] + ps_ox, ps_db[i] + ps_dpy[i] + H / 2.0 + ps_oy, ps_dz[i] + ps_oz);
            ELSE                                               // ascensor / plataforma
                lim = pl_t_dist[t] * PS_TS;
                riding = (abs(ps_hx - (ps_dx[i] + ps_dpx[i])) < 1.3 AND abs(ps_hz - ps_dz[i]) < 1.3 AND abs(ps_hy - (ps_db[i] + ps_dpy[i] + H)) < 0.35);
                IF (ps_dex[i])                                 // con enlace: va al extremo mientras un boton/diana que la nombra este activo
                    over = 0; FOR (d = 0; d < ps_dns[i]; d = d + 1) IF (ps_chan[ps_dsc[i * 8 + d]]) over = 1; END END
                    d = -1; IF (over) d = 1; END
                    ps_dpos[i] = ps_dpos[i] + d * pl_t_spd[t] * dt;
                    IF (ps_dpos[i] > lim) ps_dpos[i] = lim; END
                    IF (ps_dpos[i] < 0.0) ps_dpos[i] = 0.0; END
                ELSE IF (pl_t_mode[t] == 1)                    // al pisar: sube mientras vas encima, y vuelve cuando bajas
                    d = -1; IF (riding) d = 1; END
                    ps_dpos[i] = ps_dpos[i] + d * pl_t_spd[t] * dt;
                    IF (ps_dpos[i] > lim) ps_dpos[i] = lim; END
                    IF (ps_dpos[i] < 0.0) ps_dpos[i] = 0.0; END
                ELSE
                    IF (ps_dwait[i] > 0)
                        ps_dwait[i] = ps_dwait[i] - 1;
                    ELSE
                        ps_dpos[i] = ps_dpos[i] + ps_ddir[i] * pl_t_spd[t] * dt;
                        IF (ps_dpos[i] > lim) ps_dpos[i] = lim; ps_ddir[i] = -1; ps_dwait[i] = pl_t_wait[t] * 60.0; END
                        IF (ps_dpos[i] < 0.0) ps_dpos[i] = 0.0; ps_ddir[i] = 1; ps_dwait[i] = pl_t_wait[t] * 60.0; END
                    END
                END END
                ox0 = ps_dpx[i]; oy0 = ps_dpy[i];
                IF (pl_t_axis[t] == 1) ps_dpx[i] = ps_dpos[i]; ELSE ps_dpy[i] = ps_dpos[i]; END
                IF (riding) ps_cx = ps_cx + ps_dpx[i] - ox0; ps_cy = ps_cy + ps_dpy[i] - oy0; END
                g3d_entity_set_position(ps_de[i], ps_dx[i] + ps_dpx[i] + ps_ox, ps_db[i] + ps_dpy[i] + H / 2.0 + ps_oy, ps_dz[i] + ps_oz);
            END
        END
        FRAME;
    END
END

// Disparo del heroe en el pasillo: sale hacia donde mira (al fondo, hacia la camara o de lado), se para en las paredes y quita vida a los enemigos
PROCESS ps_bala(float bx, float by, float bz, float ddx, float ddz)
PRIVATE int e; int s; int vida = 80;
BEGIN
    s = 0; WHILE (s < 16 AND ps_blon[s]) s = s + 1; END
    IF (s >= 16) RETURN; END
    e = g3d_entity_spawn(scene, 0, bx + ps_ox, by + ps_oy, bz + ps_oz);
    g3d_entity_set_mesh(e, pl_bull_mesh); g3d_entity_set_material(e, pl_mat_bh); g3d_entity_set_scale(e, 0.45, 0.45, 0.45);
    g3d_entity_set_blend(e, 2);                                           // brilla (como los disparos del nivel 2D)
    ps_blon[s] = 1;
    WHILE (vida > 0 AND ps_blon[s])
        bx = bx + ddx * 0.45; bz = bz + ddz * 0.45;
        IF (ps_ftop(bx, bz, 0.15, 0) > by - 0.4) ps_blon[s] = 0; END         // choca con un muro
        ps_blx[s] = bx; ps_bly[s] = by; ps_blz[s] = bz;
        g3d_entity_set_position(e, bx + ps_ox, by + ps_oy, bz + ps_oz);
        vida = vida - 1;
        FRAME;
    END
    ps_blon[s] = 0;
    g3d_entity_destroy(e);
END

// ---------------------------------------------------------------------------
//  Enemigos del pasillo (los marcadores enemigo1 / enemigo2 de la capa Marcadores): usan las hojas y vidas de los enemigos del nivel.
//  Persiguen al heroe cuando se acerca, hacen dano al tocarlo y mueren si los pisas (cayendo encima) o con el giro (J / X / Z).
// ---------------------------------------------------------------------------
PROCESS ps_enemigo(int tp, float sx, float sz)
PRIVATE
    float lx; float ly; float lz; float vy; float dx; float dz; float dist; float sp; float nx; float nz; float gh; float dt = 0.0166667;
    int hp; int tic; int hurtf; int moving; int dir = 1; int spun; int dead_t; float rad = 0.5; int em; float yaw; int a3; int kb;
END
BEGIN
    IF (pl_g_en[tp] <= 0) RETURN; END
    ctype = C_3D; csubtype = C3D_SPRITE;
    lx = sx; lz = sz; ly = ps_ftop(lx, lz, 0.0, 1); IF (ly < -50.0) ly = 0.0; END
    x = lx + ps_ox; y = ly + ps_oy; z = lz + ps_oz;
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, pl_en_ppu[tp]);
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);
    g3d_sprite_set_cutout(entity, 0.4);
    g3d_sprite_set_shadow(entity, 1);
    g3d_sprite_set_smooth(entity, 1);
    g3d_sprite_set_lit(entity, 1);
    file = 0; graph = pl_g_en[tp];
    em = pl_em_new(tp); IF (em >= 0) g3d_sprite_set_pixels_per_unit(entity, 100000.0); END
    hp = pl_en_hp[tp]; IF (hp < 1) hp = 1; END
    WHILE (hp > 0)
        tic = tic + 1;
        IF (hurtf > 0) hurtf = hurtf - 1; END
        dx = ps_hx - lx; dz = ps_hz - lz; dist = sqrt(dx * dx + dz * dz); IF (dist < 0.01) dist = 0.01; END
        moving = 0;
        IF (hurtf == 0 AND dist < 14.0 AND dist > 1.4)             // persigue al heroe (cada eje por separado, sin subir muros)
            sp = 3.2; IF (tp == 1) sp = 4.6; END
            nx = lx + dx / dist * sp * dt; nz = lz + dz / dist * sp * dt;
            IF (ps_ftop2(nx, lz, rad, 0, ly) <= ly + 0.45) lx = nx; END
            IF (ps_ftop2(lx, nz, rad, 0, ly) <= ly + 0.45) lz = nz; END
            moving = 1; IF (dx > 0.0) dir = 1; ELSE dir = -1; END
        END
        gh = ps_ftop2(lx, lz, rad, 1, ly);                         // gravedad y suelo
        IF (ly > gh + 0.01 OR vy > 0.0)
            vy = vy - 58.0 * dt; ly = ly + vy * dt;
            IF (ly <= gh) ly = gh; vy = 0.0; END
        ELSE
            ly = gh; vy = 0.0;
        END
        IF (ly < -14.0) hp = 0; END
        // contacto con el heroe
        IF (hurtf == 0 AND dist < 1.3 AND abs(ps_hy - ly) < 1.8)
            IF (ps_hvy < -2.0 AND ps_hy > ly + 0.7)                // pisado desde arriba
                hp = 0; ps_bounce = 1;
            ELSE
                ps_dmg = 1;
            END
        END
        FOR (kb = 0; kb < 16; kb = kb + 1)                          // un disparo del heroe lo alcanza
            IF (ps_blon[kb] AND abs(ps_blx[kb] - lx) < 1.0 AND abs(ps_blz[kb] - lz) < 1.0 AND abs(ps_bly[kb] - (ly + 1.0)) < 1.8)
                hp = hp - 1; hurtf = 10; ps_blon[kb] = 0;
            END
        END
        IF (ps_spin > 0 AND dist < 2.8 AND NOT spun)               // el giro del heroe
            hp = hp - 2; spun = 1; hurtf = 14;
        END
        IF (ps_spin == 0) spun = 0; END
        IF (hurtf > 0) pl_cell_en(entity, tp, 4, 0);
        ELSE IF (moving) pl_cell_en(entity, tp, 2, tic / 6);
        ELSE pl_cell_en(entity, tp, 0, tic / 14); END END
        IF (dir > 0) flags = 0; ELSE flags = 1; END
        x = lx + ps_ox; y = ly + ps_oy; z = lz + ps_oz;
        IF (em >= 0)
            IF (dist > 0.05) yaw = atan2(dx, dz) / 1000.0; END
            a3 = 0; IF (hurtf > 0) a3 = 4; ELSE IF (moving) a3 = 1; END END
            pl_em_set(em, x, y, z, yaw, a3, dist < 40.0);
        END
        FRAME;
    END
    pl_score = pl_score + 300;
    dead_t = 0;
    WHILE (dead_t < 36)
        pl_cell_en(entity, tp, 5, dead_t / 6);
        dead_t = dead_t + 1;
        IF (em >= 0) pl_em_set(em, x, y, z, yaw, 5, 1); END
        FRAME;
    END
    IF (em >= 0) pl_em_set(em, x, y, z, yaw, 5, 0); END
    g3d_sprite_destroy(entity);
END

FUNCTION ps_spawn_enemies()
PRIVATE int i;
BEGIN
    FOR (i = 0; i < ps_nsp; i = i + 1) ps_enemigo(ps_spt[i], ps_spx[i], ps_spz[i]); END
END

// ---------------------------------------------------------------------------
//  El heroe: se corre hacia delante (-z), atras y a los lados; se salta
// ---------------------------------------------------------------------------
PROCESS ps_heroe()
PRIVATE
    float vx; float vz; float vy; int grounded; int jprev; int tic; int cara; int moving; int invul; int hurt_t;
    float nx; float nz; float gh; float ny; float spd = 10.0; float rad = 0.45; int ix; int iz; int jump_k; int t; int a; int f;
    int dead_t; float lx; float ly; float lz; int inliq; int swim; float geff; float hyaw = 180.0; int ma; int spin_t; int sprev; int shoot_t; int fprev;
END
BEGIN
    ctype = C_3D; csubtype = C3D_SPRITE;
    lx = ps_startx; ly = 0.0; lz = ps_startz; cara = 0;
    IF (ps_embed AND ps_paso > 0) lx = pl_cor_hx - ps_ox; ly = pl_cor_hy - ps_oy; lz = 0.0 - ps_oz; END      // empieza donde estaba el heroe del mapa: entra andando
    x = lx + ps_ox; y = ly + ps_oy; z = lz + ps_oz;
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, pl_hero_ppu);
    IF (pl_hm_on) g3d_sprite_set_pixels_per_unit(entity, 100000.0); END   // con el modelo 3D el sprite no se ve (diminuto)
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);
    g3d_sprite_set_cutout(entity, 0.4);
    g3d_sprite_set_shadow(entity, 1);
    g3d_sprite_set_smooth(entity, 1);
    g3d_sprite_set_lit(entity, 1);
    file = 0; graph = pl_g_hero;
    LOOP
        tic = tic + 1;
        IF (ps_paso != 0)                                  // pasando entre el mapa 2D y el pasillo: anda solo por el hueco de la puerta (el mismo personaje, sin desaparecer)
            IF (ps_paso > 0) nx = ps_startx; nz = ps_startz; ELSE nx = pl_cor_hx - ps_ox; nz = 0.0 - ps_oz; END
            gh = sqrt((nx - lx) * (nx - lx) + (nz - lz) * (nz - lz));
            IF (gh < 0.2)
                IF (ps_paso < 0) ps_done = 1; ps_back = 1; END
                ps_paso = 0; vy = 0.0; grounded = 1;
            ELSE
                lx = lx + (nx - lx) / gh * 7.5 * 0.0166667; lz = lz + (nz - lz) / gh * 7.5 * 0.0166667;
                ly = ly + (0.0 - ly) * 0.15;
                x = lx + ps_ox; y = ly + ps_oy; z = lz + ps_oz;
                ps_hx = lx; ps_hy = ly; ps_hz = lz; ps_hvy = 0.0;
                IF (ps_paso > 0) hyaw = 180.0; cara = 0; ELSE hyaw = 0.0; cara = 1; END
                IF (pl_hm_on)
                    alpha = 0; pl_hm_ps = 1; pl_hm_put(x, y, z, hyaw, 1, 0);
                ELSE
                    IF (pl_g_hero_d > 0) graph = pl_g_hero_d; pl_cell(entity, (tic / 5) % 6, cara, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay);
                    ELSE graph = pl_g_hero; pl_cell_h(entity, 1, tic * pl_ha_fps[1] / 60); END
                    alpha = 255;
                END
                FRAME;
                CONTINUE;
            END
        END
        // ----- entrada -----
        ix = 0; iz = 0;
        IF (pl_in_right()) ix = ix + 1; END
        IF (pl_in_left()) ix = ix - 1; END
        IF (pl_in_up()) iz = iz + 1; END          // hacia delante (al fondo de la pantalla)
        IF (pl_in_down()) iz = iz - 1; END
        jump_k = pl_in_jump();
        // dentro de un liquido (agua, lava, acido, cieno): mas lento, y se nada si el liquido lo permite
        t = ps_tile_at(lx, lz); inliq = 0; swim = 0;
        IF (t > 0)
            IF (pl_t_liq[t] > 0 AND ly < ps_cell_top(lx, lz, 1, lx) + 1.4) inliq = 1; swim = pl_t_swim[t]; END
        END
        vx = ix * spd; vz = iz * spd;
        IF (inliq) vx = vx * pl_t_slow[t]; vz = vz * pl_t_slow[t]; END
        IF (ix != 0 AND iz != 0) vx = vx * 0.7071; vz = vz * 0.7071; END
        IF (hurt_t > 0) hurt_t = hurt_t - 1; END
        IF (invul > 0) invul = invul - 1; END
        // ataque giratorio (J / X / Z) y rebote al pisar a un enemigo; dano pedido por los enemigos al tocarte
        IF (ix != 0 OR iz != 0) hyaw = atan2(ix * 1.0, 0.0 - iz) / 1000.0; END      // hacia donde mira: al fondo, a la camara o de lado
        IF (shoot_t > 0) shoot_t = shoot_t - 1; END
        IF ((pl_in_shot()) AND NOT fprev AND shoot_t == 0)       // J / X / Z: dispara hacia donde mira
            shoot_t = 12; ps_bala(lx, ly + 1.3, lz, sin(hyaw * 1000.0), cos(hyaw * 1000.0));
        END
        fprev = pl_in_shot();
        IF ((pl_in_spin()) AND NOT sprev AND spin_t == 0) spin_t = 20; END          // L / C: ataque giratorio (tambien golpea las dianas)
        sprev = pl_in_spin();
        IF (spin_t > 0) spin_t = spin_t - 1; END
        ps_spin = spin_t;
        IF (ps_bounce) vy = 17.0; grounded = 0; ps_bounce = 0; invul = 12; END
        IF (ps_dmg > 0)
            IF (invul == 0 AND spin_t == 0) pl_hhp = pl_hhp - ps_dmg; invul = 80; hurt_t = 14; vy = 14.0; grounded = 0; END
            ps_dmg = 0;
        END

        // ----- movimiento horizontal, cada eje por separado (se sube un escalon de hasta 0.45) -----
        lx = lx + ps_cx; ly = ly + ps_cy; ps_cx = 0.0; ps_cy = 0.0;                  // lo que te lleva (ascensor)
        nx = lx + vx * 0.0166667;
        IF (ps_ftop2(nx, lz, rad, 0, ly) <= ly + 0.45) lx = nx; END
        nz = lz - vz * 0.0166667;
        IF (ps_ftop2(lx, nz, rad, 0, ly) <= ly + 0.45) lz = nz; END

        // ----- vertical -----
        IF (jump_k AND NOT jprev AND (grounded OR (swim AND inliq)))
            vy = 22.0; IF (inliq) vy = 17.0; END
            grounded = 0;
        END
        jprev = jump_k;
        gh = ps_ftop2(lx, lz, rad, 1, ly);
        IF (grounded AND gh > ly + 0.005 AND gh <= ly + 0.7) ly = gh; END          // sube con el ascensor o por la rampa
        IF (grounded AND gh < ly - 0.01 AND gh >= ly - 0.5) ly = gh; END          // bajar un escalon sin caer
        IF (grounded AND gh < ly - 0.5) grounded = 0; END                      // se acabo el suelo
        geff = 60.0;
        IF (inliq) IF (swim) geff = 11.0; ELSE geff = 33.0; END END
        IF (NOT grounded) vy = vy - geff * 0.0166667; END
        IF (inliq AND swim AND vy < -4.0) vy = -4.0; END
        IF (NOT grounded)
            ny = ly + vy * 0.0166667;
            IF (vy <= 0.0 AND ny <= gh AND ly >= gh - 0.6) ly = gh; vy = 0.0; grounded = 1; ELSE ly = ny; END
        END
        IF (grounded) vy = 0.0; END

        // ----- dano del suelo, hueco ly puntos de control -----
        t = ps_tile_at(lx, lz);
        IF ((grounded OR inliq) AND invul == 0 AND t > 0 AND pl_t_dmg[t] > 0)
            pl_hhp = pl_hhp - pl_t_dmg[t]; invul = 80; hurt_t = 14; vy = 14.0; grounded = 0;
        END
        FOR (a = 0; a < ps_ncp; a = a + 1)
            IF (NOT ps_cpon[a] AND abs(lx - ps_cpx[a]) < 2.5 AND abs(lz - ps_cpz[a]) < 2.5) ps_cpon[a] = 1; ps_startx = ps_cpx[a]; ps_startz = ps_cpz[a]; END
        END
        IF (ps_pcd > 0) ps_pcd = ps_pcd - 1; END
        IF (ps_pcd == 0 AND dead_t == 0 AND ps_pdn > 0 AND pl_in_act() AND NOT ps_embed)          // puerta lateral: E junto a ella pasa a la otra sala
            FOR (f = 0; f < ps_pdn; f = f + 1)
                IF (abs(lx - ps_pdx[f]) < 3.2 AND abs(lz - ps_pdz[f]) < 2.2)
                    pl_arr_from = pl_stem(ps_file); pl_ret_on = 0; pl_salto = ps_pdd[f]; ps_pcd = 600;
                    IF (ps_pdl[f] >= 0 AND pl_lk_tgt[ps_pdl[f]] != "")                      // portal con enlace: escena/puerta de destino
                        a = find(pl_lk_tgt[ps_pdl[f]], "/");
                        IF (a > 0) pl_salto = substr(pl_lk_tgt[ps_pdl[f]], 0, a); pl_arr_door = substr(pl_lk_tgt[ps_pdl[f]], a + 1); pl_arr_from = ""; ELSE pl_salto = pl_lk_tgt[ps_pdl[f]]; END
                    END
                END
            END
        END
        IF (ly < -14.0 OR pl_hhp <= 0)                                           // se cayo o se acabo la vida: vuelve al ultimo punto
            IF (pl_hhp <= 0) pl_hhp = pl_hmaxhp; ELSE pl_hhp = pl_hhp - 2; IF (pl_hhp <= 0) pl_hhp = pl_hmaxhp; END END
            lx = ps_startx; lz = ps_startz; ly = ps_ftop(lx, lz, 0.0, 1) + 0.5; vy = 0.0; invul = 90;
        END
        IF (ps_embed AND iz < 0 AND lz > -0.6 AND NOT ps_done)                 // retroceder por la entrada: se vuelve al mapa 2D, por su portal (el pasillo esta unido al mapa)
            ps_paso = -1;                                                       // sale andando hacia el mapa; al llegar a su plano se da por terminado
        END
        IF (-lz >= (ps_h - 1) * PS_TS AND NOT ps_done)                         // llegaste al final del pasillo: se vuelve al nivel de donde saliste (o al de al_final=)
            ps_done = 1;
            IF (NOT ps_embed)
                IF (pl_ret_on) pl_salto = pl_ret_level; ELSE IF (pl_alfinal != "") pl_salto = pl_alfinal; END END
            END
        END

        // ----- animacion: de espaldas al avanzar, de frente al retroceder, de perfil al ir de lado -----
        moving = (ix != 0 OR iz != 0);
        IF (iz > 0) cara = 0; ELSE IF (iz < 0) cara = 1; ELSE IF (ix != 0) cara = 2; END END END
        IF (ix > 0 AND iz == 0) flags = 0; END
        IF (ix < 0 AND iz == 0) flags = 1; END
        IF (cara != 2) flags = 0; END
        IF (invul > 0 AND (tic / 3) % 2 == 0) alpha = 120; ELSE alpha = 255; END
        IF (pl_g_hero_d > 0 AND cara != 2)
            graph = pl_g_hero_d;
            IF (NOT grounded)
                IF (vy > 0.0) pl_cell(entity, 4 + cara, 2, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay);
                ELSE pl_cell(entity, cara, 3, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay); END
            ELSE
                IF (moving) pl_cell(entity, (tic / 5) % 6, cara, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay);
                ELSE pl_cell(entity, cara * 2 + (tic / 30) % 2, 2, pl_hero_cw, pl_hero_ch, pl_hero_ax, pl_hero_ay); END
            END
        ELSE
            graph = pl_g_hero;
            IF (hurt_t > 0) a = 7; f = 0;
            ELSE IF (NOT grounded)
                IF (vy > 6.0) a = 2; ELSE IF (vy > -6.0) a = 3; ELSE a = 4; END END
                f = 0;
            ELSE IF (moving) a = 1; f = tic * pl_ha_fps[1] / 60; ELSE a = 0; f = tic * pl_ha_fps[0] / 60; END END END
            pl_cell_h(entity, a, f);
        END
        IF (pl_hm_on)                                        // heroe en 3D: el modelo mira hacia donde se mueve (hacia delante = de espaldas a la camara)
            alpha = 0;
            ma = 0; IF (hurt_t > 0) ma = 4; ELSE IF (NOT grounded) ma = 3; ELSE IF (ix != 0 OR iz != 0) ma = 1; ELSE IF (shoot_t > 0 AND pl_hm_map[6] >= 0) ma = 6; END END END END
            pl_hm_ps = 1; pl_hm_put(lx + ps_ox, ly + ps_oy, lz + ps_oz, hyaw + spin_t * 36.0, ma, 0);
            IF (invul > 0 AND (tic / 3) % 2 == 0) pl_hm_blink = 1; END
        END
        ps_hx = lx; ps_hy = ly; ps_hz = lz; ps_hvy = vy;
        x = lx + ps_ox; y = ly + ps_oy; z = lz + ps_oz;      // la posicion del mundo (el sprite); la fisica va en coordenadas del pasillo
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  Brazo de camara: la camara quiere estar detras y por encima del heroe; si un muro se interpone en esa linea se acerca al heroe
//  (se acerca rapido, se aleja despacio) para no meterse dentro de la pared.
// ---------------------------------------------------------------------------
FUNCTION float ps_arm(float hx, float hy, float hz, float dx, float dy, float dz)
PRIVATE int k; float t; float px; float py; float pz; float top;
BEGIN
    FOR (k = 1; k <= 14; k = k + 1)
        t = k / 14.0;
        px = hx + (dx - hx) * t; py = hy + (dy - hy) * t; pz = hz + (dz - hz) * t;
        top = ps_ftop(px, pz, 0.5, 1);                         // lo mas alto bajo la camara (y sus lados)
        IF (py < top + 0.7)
            IF (t < 0.3) RETURN 0.3; END
            RETURN t - 1.0 / 14.0;
        END
    END
    RETURN 1.0;
END

// calcula la posicion final de la camara (ps_cam_x/y/z, en coordenadas del pasillo) para el seguimiento (cx, cy, cz).
// Primero prueba a subir un poco la camara; si aun asi se interpone algo, se acerca al heroe (y el x-ray hace translucido lo que quede delante).
FUNCTION int ps_arm_set(float cx, float cy, float cz)
PRIVATE float dx; float dy; float dz; float hx; float hy; float hz; float want; float best; float bl; float lift; int k;
BEGIN
    dx = cx; dy = cy + ps_camalt; dz = cz + ps_camdist;
    hx = ps_hx; hy = ps_hy + 1.3; hz = ps_hz;
    best = -1.0; bl = 0.0;
    FOR (k = 0; k <= 3; k = k + 1)                       // sube poco (max 4.5): mas que eso acaba en vista cenital; es mejor acercarse y dejar que el x-ray despeje
        lift = k * 1.5;
        want = ps_arm(hx, hy, hz, dx, dy + lift, dz);
        IF (want > best + 0.001) best = want; bl = lift; END
        IF (want >= 0.99) BREAK; END
    END
    IF (best < ps_arm_s) ps_arm_s = ps_arm_s + (best - ps_arm_s) * 0.5; ELSE ps_arm_s = ps_arm_s + (best - ps_arm_s) * 0.05; END
    IF (bl > ps_arm_l) ps_arm_l = ps_arm_l + (bl - ps_arm_l) * 0.3; ELSE ps_arm_l = ps_arm_l + (bl - ps_arm_l) * 0.04; END
    ps_cam_x = hx + (dx - hx) * ps_arm_s; ps_cam_y = hy + (dy + ps_arm_l - hy) * ps_arm_s; ps_cam_z = hz + (dz - hz) * ps_arm_s;
    RETURN 1;
END

// ---------------------------------------------------------------------------
//  Camara: detras del heroe, algo elevada, mirando al frente
// ---------------------------------------------------------------------------
PROCESS ps_camara(int cam)
PRIVATE float cx; float cy; float cz; float ty; float t; int k;
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = cam;
    cx = ps_startx; cy = 0.0; cz = ps_startz;
    LOOP
        cx = cx + (ps_hx - cx) * 0.10;
        cy = cy + (ps_hy - cy) * 0.10;
        cz = cz + (ps_hz - cz) * 0.25;
        k = ps_arm_set(cx, cy, cz);                // la camara se acerca al heroe si un muro se interpone (brazo de camara)
        x = ps_cam_x; y = ps_cam_y; z = ps_cam_z;
        target_x = cx; target_y = cy + 1.2; target_z = cz - 6.0;
        g3d_xray_update(x, y, z, ps_hx, ps_hy + 1.3, ps_hz, 0.35, 0.05, 0.18);       // y los muros que aun tapan, translucidos
        FRAME;
    END
END

// ---------------------------------------------------------------------------
//  Montar / descargar
// Una columna de decorado hecha con un tile normal (su textura repetida por bloque): n bloques hacia arriba desde y0 y nd hacia abajo (que se hunden en el vacio).
FUNCTION ps_deco_col(int tl, float qx, float qz, float y0, int n, int nd)
PRIVATE int e; int k;
BEGIN
    k = n + nd; IF (k < 1) k = 1; END IF (k > 40) k = 40; END
    IF (ps_dcm[k] == 0) ps_dcm[k] = g3d_primitive_column(k); END
    e = g3d_entity_spawn(scene, 0, 0, 0, 0);
    g3d_entity_set_mesh(e, ps_dcm[k]); g3d_entity_set_material(e, pl_t_mat[tl]);
    g3d_entity_set_scale(e, PS_TS, PS_TS, PS_TS);
    g3d_entity_set_position(e, qx + ps_ox, y0 + (n - nd) * PS_TS / 2.0 + ps_oy, qz + ps_oz);
END

// Decorado pintado con tiles normales: la capa 'Decorado' (capa 7 del .lvl) pone una columna del tile en cada casilla marcada; su alto es el
// 'alto' del tile (bloques). Sin colision; se ve alrededor del pasillo y en el fondo.
FUNCTION ps_build_deco(string fich)
PRIVATE int g; int py; int c; int rb; int v; int n;
BEGIN
    g = pl_lv_open(fich, 0, 2);
    IF (g <= 0) RETURN 0; END
    FOR (py = 0; py < ps_h; py = py + 1)
        rb = ps_h - 1 - py;
        FOR (c = 0; c < ps_w; c = c + 1)
            v = pl_lv_get(g, c, py, 1);
            IF (v > 0 AND v <= pl_nt AND pl_t_tex[v] != "")
                n = pl_t_oh[v]; IF (n < 1) n = 1; END
                ps_deco_col(v, c * PS_TS + 1.0, -(rb * PS_TS + 1.0), 0.0 - PS_TS / 2.0 - 0.0, n, 12);
                ps_ndeco = ps_ndeco + 1;
            END
        END
    END
    pl_lv_close(g);
    RETURN ps_ndeco;
END

// Lo mismo en un nivel 2D: la columna en el z de la capa (capa7=z), con sus pies en la casilla.
FUNCTION ps_deco_2d(int g, int lw, int lh)
PRIVATE int py; int c; int r; int v; int n; float sox; float soy; float soz;
BEGIN
    sox = ps_ox; soy = ps_oy; soz = ps_oz; ps_ox = 0.0; ps_oy = 0.0; ps_oz = 0.0;
    FOR (py = 0; py < pl_lv_h(g) AND py < lh; py = py + 1)
        r = lh - 1 - py;
        FOR (c = 0; c < pl_lv_w(g) AND c < lw; c = c + 1)
            v = pl_lv_get(g, c, py, 1);
            IF (v > 0 AND v <= pl_nt AND pl_t_tex[v] != "")
                n = pl_t_oh[v]; IF (n < 1) n = 1; END
                ps_deco_col(v, c * PL_TS + 1.0, pl_dz, r * PL_TS, n, 0);
            END
        END
    END
    ps_ox = sox; ps_oy = soy; ps_oz = soz;
END

// Decorado en 3D real alrededor del pasillo: suelo con relieve lejos abajo, montanas al fondo y objetos repartidos a los lados y mas alla
// (temas: bosque, montanas, ruinas, ciudad; o un modelo propio con decorado_modelo=). Todo sin colision, solo para ver.
FUNCTION ps_decorado()
PRIVATE int i; int e; int gm; int mt; int mm; int mp; int mf; float zf; float cw; float qx; float qz; float s; float hh; int lado; int md; float gr; float gg; float gb; float mr; float mg; float mb; float pr; float pg; float pb; int ntp;
BEGIN
    zf = 0.0 - ps_h * PS_TS; cw = ps_w * PS_TS;
    gr = 0.10; gg = 0.16; gb = 0.10;  mr = 0.22; mg = 0.28; mb = 0.38;  pr = 0.14; pg = 0.30; pb = 0.12;
    IF (pl_deco == "montanas") gr = 0.30; gg = 0.28; gb = 0.25; mr = 0.45; mg = 0.45; mb = 0.55; pr = 0.35; pg = 0.33; pb = 0.30; END
    IF (pl_deco == "ruinas")   gr = 0.28; gg = 0.22; gb = 0.14; mr = 0.40; mg = 0.30; mb = 0.22; pr = 0.55; pg = 0.48; pb = 0.36; END
    IF (pl_deco == "ciudad")   gr = 0.08; gg = 0.08; gb = 0.12; mr = 0.12; mg = 0.14; mb = 0.24; pr = 0.18; pg = 0.20; pb = 0.30; END
    mm = g3d_material_create(); g3d_material_set_color(mm, gr, gg, gb, 1.0); g3d_material_set_roughness(mm, 0.95);
    gm = g3d_primitive_terrain(80, 700.0, 32.0, 24.0, 7);
    e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, gm); g3d_entity_set_material(e, mm);
    g3d_entity_set_position(e, cw / 2.0, -48.0, zf / 2.0 - 60.0);
    mm = g3d_material_create(); g3d_material_set_color(mm, mr, mg, mb, 1.0); g3d_material_set_roughness(mm, 0.95);
    FOR (i = 0; i < 4; i = i + 1)                               // montanas lejanas, a ambos lados y al fondo
        mt = g3d_primitive_mountain(48, 260.0, 70.0 + i * 12.0, 8.0, 11 + i, 0.0);
        e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, mt); g3d_entity_set_material(e, mm);
        g3d_entity_set_position(e, cw / 2.0 + (i - 1.5) * 190.0, -50.0, zf - 170.0 - (i % 2) * 70.0);
    END
    md = 0;
    IF (pl_deco_model != "" AND fexists(pl_deco_model))
        g3d_gltf_set_recenter(0); md = g3d_load_gltf(pl_deco_model); g3d_gltf_set_recenter(1);
    END
    mp = g3d_material_create(); g3d_material_set_color(mp, pr, pg, pb, 1.0); g3d_material_set_roughness(mp, 0.9);
    mf = g3d_material_create(); g3d_material_set_color(mf, 0.30, 0.20, 0.10, 1.0); g3d_material_set_roughness(mf, 0.9);
    pl_box_mesh = g3d_primitive_cube(); ntp = g3d_primitive_sphere(12);
    FOR (i = 0; i < pl_deco_n; i = i + 1)
        lado = rand(0, 1); s = 22.0 + rand(0, 100) / 100.0 * 90.0;       // distancia al pasillo
        IF (lado) qx = cw + s; ELSE qx = 0.0 - s; END
        qz = 20.0 - rand(0, 100) / 100.0 * (ps_h * PS_TS + 130.0);
        hh = 10.0 + rand(0, 100) / 100.0 * 26.0;
        IF (md > 0)
            gm = g3d_model_submesh_count(md);
            FOR (mt = 0; mt < gm AND mt < 4; mt = mt + 1)
                e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, g3d_model_submesh(md, mt));
                IF (g3d_model_submesh_texture(md, mt) >= 0) ntp = g3d_material_create(); g3d_material_set_texture(ntp, 0, g3d_model_submesh_texture(md, mt)); g3d_entity_set_material(e, ntp); ELSE g3d_entity_set_material(e, mp); END
                g3d_entity_set_scale(e, hh / 6.0, hh / 6.0, hh / 6.0); g3d_entity_set_position(e, qx, -20.0 + rand(0, 100) / 10.0, qz);
                g3d_entity_set_rotation(e, 0.0, rand(0, 359) * 1000.0, 0.0);
            END
        ELSIF (pl_deco == "ciudad")                                // torres altas
            e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, mp);
            g3d_entity_set_scale(e, 8.0 + rand(0, 10), hh * 2.4 + 30.0, 8.0 + rand(0, 10));
            g3d_entity_set_position(e, qx, -50.0 + (hh * 2.4 + 30.0) / 2.0, qz);
        ELSIF (pl_deco == "ruinas")                                // columnas rotas y bloques
            e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, mp);
            g3d_entity_set_scale(e, 3.0 + rand(0, 3), hh * 1.4, 3.0 + rand(0, 3));
            g3d_entity_set_position(e, qx, -30.0 + hh * 0.7, qz);
            g3d_entity_set_rotation(e, rand(0, 8) * 1000.0 - 4000.0, rand(0, 90) * 1000.0, rand(0, 8) * 1000.0 - 4000.0);
        ELSIF (pl_deco == "montanas")                              // rocas
            e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, ntp); g3d_entity_set_material(e, mp);
            g3d_entity_set_scale(e, hh * 0.6, hh * 0.45, hh * 0.5);
            g3d_entity_set_position(e, qx, -26.0 + hh * 0.2, qz);
        ELSE                                                       // bosque: tronco + copa
            e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, mf);
            g3d_entity_set_scale(e, 2.2, hh + 30.0, 2.2); g3d_entity_set_position(e, qx, -50.0 + (hh + 30.0) / 2.0, qz);
            e = g3d_entity_spawn(scene, 0, 0, 0, 0); g3d_entity_set_mesh(e, ntp); g3d_entity_set_material(e, mp);
            g3d_entity_set_scale(e, hh * 0.55, hh * 0.45, hh * 0.55); g3d_entity_set_position(e, qx, -20.0 + hh, qz);
        END
    END
END

// Fondo del pasillo por planos, como el parallax del 2D: telones con imagen (telonN= del .level) a distintas distancias del final,
// y si no hay ninguno, siluetas automaticas (colinas/torres) que se funden con la niebla. Lo cercano se mueve mas al avanzar.
FUNCTION ps_fondos()
PRIVATE int i; int j; int e; int m; float zf; float w; float hh; float t; float cr; float cg; float cb; int cnt; int mm; float dmax;
BEGIN
    zf = 0.0 - ps_h * PS_TS;                                   // z del final del pasillo
    IF (pl_deco != "" OR pl_deco_model != "") ps_decorado(); END
    pl_box_mesh = g3d_primitive_cube();
    IF (pl_bgfile != "" AND pl_tl_n == 0) pl_tl_file[0] = pl_bgfile; pl_tl_d[0] = 90.0; pl_tl_h[0] = 120.0; pl_tl_y[0] = 35.0; pl_tl_mov[0] = 0.0; pl_tl_n = 1; END
    cnt = 0;
    FOR (i = pl_tl_n - 1; i >= 0; i = i - 1)                    // del mas lejano al mas cercano
        IF (pl_tl_file[i] != "" AND fexists(pl_dirfondos + pl_tl_file[i]))
            m = pl_make_mat(pl_dirfondos + pl_tl_file[i], 0.9, 0.0);
            g3d_material_set_color(m, 1.0, 1.0, 1.0, 1.0);
            e = g3d_entity_spawn(scene, 0, 0, 0, 0);
            g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, m);
            g3d_entity_set_blend(e, 1); g3d_entity_set_alpha(e, 255);
            w = (pl_tl_d[i] + ps_h * PS_TS + 40.0) * 2.4;      // ancho suficiente para llenar la vista a esa distancia
            g3d_entity_set_scale(e, w, pl_tl_h[i], 0.2);
            g3d_entity_set_position(e, ps_w * PS_TS / 2.0, pl_tl_y[i], zf - pl_tl_d[i] - i * 0.5);
            cnt = cnt + 1;
            IF (pl_tl_d[i] + ps_h * PS_TS > dmax) dmax = pl_tl_d[i] + ps_h * PS_TS; END
        END
    END
    IF (cnt > 0 AND dmax * 2.0 > pl_fg_f * 0.9)                // la niebla no debe borrar los telones: se alarga hasta cubrir el mas lejano
        g3d_set_fog(1, pl_fg_r, pl_fg_g, pl_fg_b, pl_fg_n * 0.6, dmax * 2.0);
    END
    IF (cnt == 0 AND pl_sil)                                    // siluetas: 3 filas de colinas/torres; cuanto mas lejos, mas del color del horizonte
        FOR (j = 0; j < 3; j = j + 1)
            t = (3 - j) / 3.0;                                  // 1 = lejos
            cr = pl_hz_r * (0.55 + 0.4 * t) + pl_fg_r * 0.05; cg = pl_hz_g * (0.55 + 0.4 * t) + pl_fg_g * 0.05; cb = pl_hz_b * (0.55 + 0.4 * t) + pl_fg_b * 0.05;
            mm = g3d_material_create(); g3d_material_set_color(mm, cr, cg, cb, 1.0);
            FOR (i = 0; i < 26; i = i + 1)
                hh = 8.0 + rand(0, 100) / 100.0 * (22.0 - j * 4.0);
                w = 6.0 + rand(0, 100) / 100.0 * 12.0;
                e = g3d_entity_spawn(scene, 0, 0, 0, 0);
                g3d_entity_set_mesh(e, pl_box_mesh); g3d_entity_set_material(e, mm);
                g3d_entity_set_scale(e, w, hh, 5.0);
                g3d_entity_set_position(e, ps_w * PS_TS / 2.0 + (i - 13) * (10.0 + j * 2.0) + rand(-30, 30) / 10.0, hh / 2.0 - 4.0 + j * 1.5, zf - 45.0 - j * 40.0 - rand(0, 100) / 10.0);
            END
        END
    END
END

// ---------------------------------------------------------------------------
FUNCTION ps_montar()
PRIVATE int i; int e_bg;
BEGIN
    pl_reiniciar();
    ps_embed = 0; ps_ox = 0.0; ps_oy = 0.0; ps_oz = 0.0; ps_paso = 0; ps_back = 0; ps_busy = 0;      // pasillo suelto (otra escena): nada que ver con el que se pego a un mapa
    scene = g3d_scene_create("pasillo");
    g3d_scene_set_active(scene);
    camera = g3d_camera_create();
    g3d_camera_set_active(camera);
    g3d_camera_set_fov(camera, 52.0);
    pl_load_settings();
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
    g3d_set_fog(1, pl_fg_r, pl_fg_g, pl_fg_b, pl_fg_n * 0.6, pl_fg_f * 0.9);
    pl_g_hero = map_load(pl_hero_sheet);
    FOR (i = 0; i < 2; i = i + 1) IF (pl_en_sheet[i] != "" AND fexists(pl_en_sheet[i])) pl_g_en[i] = map_load(pl_en_sheet[i]); END END
    pl_g_hero_d = 0; IF (pl_hero_dsheet != "" AND fexists(pl_hero_dsheet)) pl_g_hero_d = map_load(pl_hero_dsheet); END
    IF (pl_g_hero <= 0) say("ERROR: no se carga la hoja del heroe " + pl_hero_sheet); RETURN; END
    pl_hmaxhp = pl_hero_hp; pl_hhp = pl_hero_hp; ps_done = 0; ps_arm_s = 1.0; ps_arm_l = 0.0;
    IF (NOT ps_build()) RETURN; END
    ps_fondos();
    pl_hm_on = 0; pl_hm_show = 0; pl_hm_ps = 0;
    IF (pl_hero_model != "" AND fexists(pl_hero_model)) pl_hm_on = 1; pl_hmodelo(); END
    pl_bull_mesh = g3d_primitive_sphere(12); pl_mat_bh = g3d_material_create(); g3d_material_set_color(pl_mat_bh, 1.0, 0.9, 0.3, 1.0);   // los disparos
    ps_heroe(); ps_spawn_enemies(); ps_mecanismos();
    IF (pl_hud_on)
        write(0, 20, 20, 0, "VIDA:");     write_var(0, 80, 20, 0, pl_hhp);
    END
    ps_camara(camera);
END

// ---------------------------------------------------------------------------
//  Pasillo DENTRO de un nivel de plataformas (el nivel declara pasillo=<nivel> en su .level y un tile con portal=pasillo lo abre).
//  El pasillo se construye detras del nivel, en la misma escena 3D, y se ve desde el lado. Al entrar por el portal el heroe del
//  nivel se oculta, nace el del pasillo y la camara se pasa a la vista de detras (la mueve pl_camara con pl_cam_p); al llegar al final
//  se vuelve al portal.
// ---------------------------------------------------------------------------
PROCESS ps_gestor()
PRIVATE float cx; float cy; float cz; float ty; float t; int k;
BEGIN
    ps_embed = 1; ps_busy = 0;
    IF (NOT ps_build_file(pl_cor_file)) RETURN; END
    ps_mecanismos();
    cx = ps_startx; cy = 0.0; cz = ps_startz;
    LOOP
        IF (pl_cor_req)                                        // el heroe entro por el portal
            pl_cor_req = 0; ps_done = 0; ps_back = 0; ps_paso = 1; pl_camblend = 80; ps_arm_s = 1.0; ps_arm_l = 0.0;
            FOR (k = 0; k < ps_ncp; k = k + 1) ps_cpon[k] = 0; END
            ps_hid = ps_heroe(); ps_spawn_enemies();
            cx = ps_startx; cy = 0.0; cz = ps_startz;
        END
        IF (pl_mode == 1)
            cx = cx + (ps_hx - cx) * 0.10;
            cy = cy + (ps_hy - cy) * 0.10;
            cz = cz + (ps_hz - cz) * 0.25;
            k = ps_arm_set(cx, cy, cz);
            pl_cam_p[0] = ps_cam_x + ps_ox; pl_cam_p[1] = ps_cam_y + ps_oy; pl_cam_p[2] = ps_cam_z + ps_oz;
            pl_cam_p[3] = cx + ps_ox; pl_cam_p[4] = cy + 1.2 + ps_oy;       pl_cam_p[5] = cz - 6.0 + ps_oz;
            IF (ps_done AND NOT ps_busy)
                IF (pl_cor_fin AND pl_nplanes > 1 AND NOT ps_back)             // el pasillo desemboca en el 2D del fondo (plano 1): se pasa a el con la camara en su sitio
                    ps_busy = 1; pl_cor_pass = 1;
                    k = ps_hx + ps_ox; IF (k < 4) k = 4; END
                    pl_cambio_plano(1, k, pl_cor_py + 2.0);
                ELSE                                           // fin del pasillo: se vuelve al nivel
                    signal(ps_hid, S_KILL); signal(TYPE ps_enemigo, S_KILL); pl_hm_ps = 0; ps_spin = 0;
                    pl_mode = 0; pl_camblend = 80;
                END
            END
        END
        FRAME;
    END
END

FUNCTION ps_descargar()
BEGIN
    signal(TYPE ps_heroe, S_KILL); signal(TYPE ps_camara, S_KILL); signal(TYPE ps_gestor, S_KILL); signal(TYPE pl_hmodelo, S_KILL); signal(TYPE ps_enemigo, S_KILL); signal(TYPE ps_mecanismos, S_KILL); signal(TYPE ps_bala, S_KILL);
    FRAME;                                     // los procesos muertos sueltan sus entidades antes de destruir la escena
    write_delete(all_text);
    g3d_scene_destroy(scene);
END
