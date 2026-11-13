// =============================================================================
//  carreras.prg - biblioteca de CARRERAS DE KARTS de editor3d (runtime).
//
//  Viaja con el proyecto (Scripts/carreras.prg) y se reescribe cada vez que se
//  genera el juego: no se edita a mano. Una escena de carreras (.kart) se
//  genera como <escena>_montar(), que rellena la configuracion kr_* y llama a
//  kr_montar(). Al cambiar de escena se llama a kr_descargar().
//
//  REGLA: nunca say()/printf. Toda salida -> send_notifications().
//
//  Fisica arcade (la misma que KartMVP, fases 1-3): el kart es un G3D_CHAR
//  (gravedad, rayo al suelo contra la malla de Jolt, deslizar en muros) y aqui
//  se decide la velocidad: yaw separado de vx,vz, agarre lateral = inercia y
//  deriva, saltito, derrape con carga y turbo, trampolines, control aereo.
//
//  Trampas del motor que esto ya tiene en cuenta:
//    - mirando a +Z, +X queda a la IZQUIERDA: girar a la derecha = bajar el yaw;
//    - g3d_model_spawn: el 4o parametro es la ALTURA deseada (0.0 = 1:1);
//    - sin heightmap hay un suelo invisible en y = 0: fuera del circuito no se
//      cae, por eso el respawn usa limites (kr_lim_*);
//    - rotaciones R = Ry*Rx*Rz: pitch + = morro abajo, roll + = techo a la
//      derecha del piloto;
//    - una rampa de malla no lanza (el char se pega a la cresta): lanzan los
//      trampolines (zonas).
// =============================================================================

CONST
    // notificaciones
    N_DEBUG = 0;
    N_INFO  = 1;
    N_WARN  = 2;
    N_ERROR = 3;

    KR_DT        = 0.0166667;   // tick fijo 60 Hz
    KR_GRAVEDAD  = 38.0;
    KR_Y_CAIDA   = -30.0;
    KR_ACEL      = 28.0;
    KR_FRENO     = 50.0;
    KR_PUNTA     = 40.0;
    KR_ATRAS     = 12.0;
    KR_ROCE      = 0.7;
    KR_GIRO      = 140.0;
    KR_AGARRE    = 8.0;
    KR_AIRE      = 0.3;
    KR_RADIO     = 0.8;
    KR_ALTO      = 1.0;
    KR_MEDIO     = 0.3;
    KR_HOP       = 7.0;
    KR_DER_MIN   = 15.0;
    KR_DER_BASE  = 70.0;
    KR_DER_MOD   = 45.0;
    KR_DER_AGARRE = 2.5;
    KR_DER_VIS   = 22.0;
    KR_CARGA1    = 0.7;
    KR_CARGA2    = 1.5;
    KR_TURBO1    = 36;
    KR_TURBO2    = 66;
    KR_TURBO_PUNTA = 1.35;
    KR_TURBO_ACEL  = 60.0;
    KR_ZONA_MUERTA = 0.15;
    KR_MAX_TRAMP = 64;
    KR_MAX_OBJ   = 128;
    KR_MAX_CP    = 400;
    KR_MAX_TURBO = 64;
    KR_MAX_LAGO  = 16;
    KR_MAX_LUZ   = 24;
    KR_AGUA_Y    = -0.35;       // nivel del agua de los lagos
    // superficies (kr_cp_sup y la que hay bajo el kart)
    KR_S_ASFALTO = 0;
    KR_S_TIERRA  = 1;
    KR_S_HIELO   = 2;
    KR_S_BARRO   = 3;
    KR_S_FUERA   = 4;           // cesped / fuera de la carretera
    KR_S_AGUA    = 5;
    // camara
    KR_CAM_DIST   = 6.5;
    KR_CAM_ALTO   = 2.4;
    KR_CAM_MIRA   = 4.0;
    KR_CAM_GIRO   = 0.10;
    KR_CAM_Y      = 0.18;
    KR_CAM_FOV    = 68.0;
    KR_CAM_FOV_V  = 14.0;
    KR_CAM_FOV_T  = 8.0;
END

GLOBAL
    // ---- configuracion: la rellena <escena>_montar() ----
    string kr_pista = "";             // .glb del circuito (dibujo + colision)
    // colocacion de una pista propia (importada): posicion, escala y giro (grados)
    string kr_extra_vis = ""; string kr_extra_col = "";   // con pista propia: turbos y meta (solo se ven) / rampas (se ven y se pisan), generados por el editor
    float  kr_pista_x = 0.0; float kr_pista_y = 0.0; float kr_pista_z = 0.0; float kr_pista_esc = 1.0; float kr_pista_yaw = 0.0;
    string kr_kart_modelo = "";       // .glb del kart ("" = cubo)
    float  kr_kart_tam = 1.3;         // alto del kart en metros
    float  kr_salida_x; float kr_salida_y; float kr_salida_z; float kr_salida_yaw;
    float  kr_lim_x0 = -100.0; float kr_lim_x1 = 100.0;
    float  kr_lim_z0 = -100.0; float kr_lim_z1 = 100.0;

    // ---- notificaciones ----
    int    notify_fd   = 0;
    int    notify_min  = N_DEBUG;
    int    notify_fila = 0;

    // ---- estado de la biblioteca ----
    int    kr_scene = -1;
    int    kr_camera;
    int    kr_txt_ayuda; int kr_txt_hud;
    int    kr_auto = 0;               // piloto de prueba: bgdi main.dcb auto|recto|derrape
    int    kr_frame = 0;

    // input (lo lee el kart; manana tambien la IA)
    float  kr_in_acel; float kr_in_giro; int kr_in_salto;

    // trampolines
    int    kr_n_tramp = 0;
    float  kr_tr_x0[63]; float kr_tr_y0[63]; float kr_tr_z0[63];
    float  kr_tr_x1[63]; float kr_tr_y1[63]; float kr_tr_z1[63];
    float  kr_tr_fuerza[63];

    // objetos del circuito (modelos colocados en el editor)
    int    kr_n_obj = 0;
    string kr_ob_mod[127];
    float  kr_ob_x[127]; float kr_ob_y[127]; float kr_ob_z[127];
    float  kr_ob_yaw[127]; float kr_ob_alto[127]; int kr_ob_col[127];

    // carrera: vueltas, checkpoints (la linea central muestreada cada kr_paso m)
    int    kr_vueltas = 3;
    // efectos: de las 4 superficies de la carretera (asfalto, tierra, hielo, barro) y de las 4 zonas pintadas en el terreno
    // (agarre, velocidad maxima y aceleracion, como multiplicadores); la escena los rellena y pone kr_ef_ok = 1
    int    kr_auto_dur = 600;          // pruebas (auto/recto/derrape/lago): fotogramas que dura (segundo argumento)
    int    kr_ef_ok = 0;
    float  kr_ef_ag[3]; float kr_ef_vel[3]; float kr_ef_acel[3];
    int    kr_zn_on[3]; float kr_zn_ag[3]; float kr_zn_vel[3]; float kr_zn_acel[3];
    float  kr_paso = 6.0;
    int    kr_n_cp = 0;
    float  kr_cp_x[399]; float kr_cp_y[399]; float kr_cp_z[399];
    float  kr_cp_yaw[399]; float kr_cp_hw[399]; int kr_cp_sup[399];
    int    kr_cp_cur = 0;                 // checkpoint mas cercano al kart (lo lee el bot)
    int    kr_cuenta = 0;                 // ticks de cuenta atras (0 = en carrera)
    int    kr_fin = 0;                    // 1 = carrera terminada
    int    kr_lap = 1; int kr_prog = 0; int kr_d0 = 0;
    int    kr_t_race = 0; int kr_t_lap = 0; int kr_best = 0;
    int    kr_in_reset = 0;               // R: volver al ultimo checkpoint
    string kr_msg = ""; int kr_msg_t = 0;
    // ambiente: 0 como la escena vinculada, 1 dia, 2 noche, 3 ciclo; luces puntuales (farolas) y faros del kart
    int    kr_amb_modo = 1; float kr_amb_hora = 9.0; float kr_amb_dur = 240.0; int kr_faros = 1;
    float  kr_hora = 10.0;                // hora actual (0..24)
    float  kr_noche = 0.0;                // 0 pleno dia .. 1 noche cerrada
    int    kr_sol = -1; int kr_ext_light = -1; int kr_faro_id = -1;
    int    kr_n_luz = 0;
    float  kr_lz_x[23]; float kr_lz_y[23]; float kr_lz_z[23];
    float  kr_lz_r[23]; float kr_lz_g[23]; float kr_lz_b[23]; float kr_lz_inten[23]; float kr_lz_radio[23];
    int    kr_lz_noche[23]; int kr_lz_poste[23]; int kr_lz_id[23];
    int    kr_externa = 0;                // 1 = la escena 3D (terreno, objetos) la monta el editor antes de kr_montar
    int    kr_ext_scene = -1; int kr_ext_camera = -1;
    string kr_hud2 = "";
    int    kr_sem_map = 0;                // grafico del semaforo de salida
    int    kr_txt_hud2; int kr_txt_msg;

    // los demas caminos (atajos): puntos de carretera cada ~6 m, solo para saber la superficie
    int    kr_n_ar = 0;
    float  kr_ar_x[399]; float kr_ar_y[399]; float kr_ar_z[399];
    float  kr_ar_yaw[399]; float kr_ar_hw[399]; int kr_ar_sup[399];

    // placas de turbo
    int    kr_n_turbo = 0;
    float  kr_tb_x0[63]; float kr_tb_y0[63]; float kr_tb_z0[63];
    float  kr_tb_x1[63]; float kr_tb_y1[63]; float kr_tb_z1[63];

    // lagos (rectangulos: centro y tamano)
    int    kr_n_lago = 0;
    float  kr_lg_x[15]; float kr_lg_z[15]; float kr_lg_w[15]; float kr_lg_h[15];
    float  kr_lg_prof[15]; int kr_lg_mortal[15];

    // estado publico del kart
    float  kr_kart_x; float kr_kart_y; float kr_kart_z;
    float  kr_kart_yaw; float kr_kart_vel; int kr_kart_turbo;
    string kr_hud;
END

// =========================== notificaciones ==================================
PROCESS kr_notify_toast(string linea)
PRIVATE
    int txt; int t;
BEGIN
    txt = write(0, 1270, 10 + notify_fila * 18, 2, linea);
    notify_fila = (notify_fila + 1) % 6;
    FOR (t = 0; t < 240; t = t + 1) FRAME; END
    write_delete(txt);
END

FUNCTION send_notifications(int nivel, string canal, string texto)
PRIVATE
    string tag; string linea;
BEGIN
    IF (nivel < notify_min) RETURN; END
    SWITCH (nivel)
        CASE N_DEBUG: tag = "DBG"; END
        CASE N_INFO:  tag = "INF"; END
        CASE N_WARN:  tag = "AVS"; END
        DEFAULT:      tag = "ERR"; END
    END
    linea = "[" + get_timer() + "] " + tag + " " + canal + ": " + texto;
    IF (!notify_fd) notify_fd = fopen("carreras.log", O_WRITE); END
    IF (notify_fd) fputs(notify_fd, linea); fflush(notify_fd); END
    IF (nivel >= N_WARN) kr_notify_toast(linea); END
END

// =========================== trampolines =====================================
FUNCTION kr_trampolin_add(float x0, float y0, float z0, float x1, float y1, float z1, float fuerza)
BEGIN
    IF (kr_n_tramp >= KR_MAX_TRAMP)
        send_notifications(N_WARN, "carreras", "demasiados trampolines (max " + KR_MAX_TRAMP + ")");
        RETURN;
    END
    kr_tr_x0[kr_n_tramp] = x0; kr_tr_y0[kr_n_tramp] = y0; kr_tr_z0[kr_n_tramp] = z0;
    kr_tr_x1[kr_n_tramp] = x1; kr_tr_y1[kr_n_tramp] = y1; kr_tr_z1[kr_n_tramp] = z1;
    kr_tr_fuerza[kr_n_tramp] = fuerza;
    kr_n_tramp = kr_n_tramp + 1;
END

FUNCTION int kr_trampolin_en(float qx, float qy, float qz)
PRIVATE
    int i;
BEGIN
    FOR (i = 0; i < kr_n_tramp; i = i + 1)
        IF (qx >= kr_tr_x0[i] AND qx <= kr_tr_x1[i] AND qy >= kr_tr_y0[i] AND qy <= kr_tr_y1[i]
            AND qz >= kr_tr_z0[i] AND qz <= kr_tr_z1[i])
            RETURN i;
        END
    END
    RETURN -1;
END

// =========================== checkpoints, turbo, lagos ========================
FUNCTION kr_cp_add(float cx, float cy, float cz, float cyaw, float chw, int csup)
BEGIN
    IF (kr_n_cp >= KR_MAX_CP) RETURN; END
    kr_cp_x[kr_n_cp] = cx; kr_cp_y[kr_n_cp] = cy; kr_cp_z[kr_n_cp] = cz;
    kr_cp_yaw[kr_n_cp] = cyaw; kr_cp_hw[kr_n_cp] = chw; kr_cp_sup[kr_n_cp] = csup;
    kr_n_cp = kr_n_cp + 1;
END

FUNCTION kr_ar_add(float cx, float cy, float cz, float cyaw, float chw, int csup)
BEGIN
    IF (kr_n_ar >= KR_MAX_CP) RETURN; END
    kr_ar_x[kr_n_ar] = cx; kr_ar_y[kr_n_ar] = cy; kr_ar_z[kr_n_ar] = cz;
    kr_ar_yaw[kr_n_ar] = cyaw; kr_ar_hw[kr_n_ar] = chw; kr_ar_sup[kr_n_ar] = csup;
    kr_n_ar = kr_n_ar + 1;
END

FUNCTION kr_turbo_add(float x0, float y0, float z0, float x1, float y1, float z1)
BEGIN
    IF (kr_n_turbo >= KR_MAX_TURBO) RETURN; END
    kr_tb_x0[kr_n_turbo] = x0; kr_tb_y0[kr_n_turbo] = y0; kr_tb_z0[kr_n_turbo] = z0;
    kr_tb_x1[kr_n_turbo] = x1; kr_tb_y1[kr_n_turbo] = y1; kr_tb_z1[kr_n_turbo] = z1;
    kr_n_turbo = kr_n_turbo + 1;
END

FUNCTION int kr_turbo_en(float qx, float qy, float qz)
PRIVATE
    int i;
BEGIN
    FOR (i = 0; i < kr_n_turbo; i = i + 1)
        IF (qx >= kr_tb_x0[i] AND qx <= kr_tb_x1[i] AND qy >= kr_tb_y0[i] AND qy <= kr_tb_y1[i]
            AND qz >= kr_tb_z0[i] AND qz <= kr_tb_z1[i])
            RETURN 1;
        END
    END
    RETURN 0;
END

FUNCTION kr_lago_add(float cx, float cz, float w, float h, float prof, int mortal)
BEGIN
    IF (kr_n_lago >= KR_MAX_LAGO) RETURN; END
    kr_lg_x[kr_n_lago] = cx; kr_lg_z[kr_n_lago] = cz; kr_lg_w[kr_n_lago] = w; kr_lg_h[kr_n_lago] = h;
    kr_lg_prof[kr_n_lago] = prof; kr_lg_mortal[kr_n_lago] = mortal;
    kr_n_lago = kr_n_lago + 1;
END

// Indice del lago en cuyo agua esta el punto (bajo la superficie), o -1.
FUNCTION int kr_lago_en(float qx, float qy, float qz)
PRIVATE
    int i;
BEGIN
    IF (qy > KR_AGUA_Y + 0.8) RETURN -1; END
    FOR (i = 0; i < kr_n_lago; i = i + 1)
        IF (abs(qx - kr_lg_x[i]) <= kr_lg_w[i] * 0.5 AND abs(qz - kr_lg_z[i]) <= kr_lg_h[i] * 0.5) RETURN i; END
    END
    RETURN -1;
END

FUNCTION kr_luz_add(float lx, float lz, float ly, float lr, float lg, float lb, float inten, float radio, int noche, int poste)
BEGIN
    IF (kr_n_luz >= KR_MAX_LUZ) RETURN; END
    kr_lz_x[kr_n_luz] = lx; kr_lz_y[kr_n_luz] = ly; kr_lz_z[kr_n_luz] = lz;
    kr_lz_r[kr_n_luz] = lr; kr_lz_g[kr_n_luz] = lg; kr_lz_b[kr_n_luz] = lb;
    kr_lz_inten[kr_n_luz] = inten; kr_lz_radio[kr_n_luz] = radio; kr_lz_noche[kr_n_luz] = noche; kr_lz_poste[kr_n_luz] = poste;
    kr_n_luz = kr_n_luz + 1;
END

FUNCTION float kr_mez(float a, float b, float t)
BEGIN
    RETURN a + (b - a) * t;
END

// m:ss.cc
FUNCTION string kr_tiempo(int ticks)
PRIVATE
    int cs; int mm; int ss; int cc; string r;
BEGIN
    cs = ticks * 100 / 60;
    mm = cs / 6000; ss = (cs / 100) % 60; cc = cs % 100;
    r = "" + mm + ":";
    IF (ss < 10) r = r + "0"; END
    r = r + ss + ".";
    IF (cc < 10) r = r + "0"; END
    r = r + cc;
    RETURN r;
END

// =========================== objetos =========================================
// Los apunta <escena>_montar(); se colocan en kr_montar() con la escena creada.
//   colision: 0 ninguna, 1 caja (la del objeto girado), 2 malla exacta (sin giro).
FUNCTION kr_objeto_add(string modelo, float ox, float oy, float oz, float giro, float alto, int colision)
BEGIN
    IF (kr_n_obj >= KR_MAX_OBJ)
        send_notifications(N_WARN, "carreras", "demasiados objetos (max " + KR_MAX_OBJ + ")");
        RETURN;
    END
    kr_ob_mod[kr_n_obj] = modelo;
    kr_ob_x[kr_n_obj] = ox; kr_ob_y[kr_n_obj] = oy; kr_ob_z[kr_n_obj] = oz;
    kr_ob_yaw[kr_n_obj] = giro; kr_ob_alto[kr_n_obj] = alto; kr_ob_col[kr_n_obj] = colision;
    kr_n_obj = kr_n_obj + 1;
END

FUNCTION kr_objetos_montar()
PRIVATE
    int   i; int j; int m; int ns; int col;
    float s; float h; float cs; float sn;
    float mnx; float mny; float mnz; float mxx; float mxy; float mxz;
    float cx; float cy; float cz; float hx; float hy; float hz;
    float lx; float lz; float wx; float wz;
    float bx0; float bz0; float bx1; float bz1;
    int   k;
BEGIN
    FOR (i = 0; i < kr_n_obj; i = i + 1)
        IF (!fexists(kr_ob_mod[i]))
            send_notifications(N_WARN, "carreras", "no existe el objeto " + kr_ob_mod[i]);
            CONTINUE;
        END
        IF (find(lcase(kr_ob_mod[i]), ".fbx") >= 0)
            g3d_fbx_set_recenter(1);
            m = g3d_load_fbx(kr_ob_mod[i]);
        ELSE
            g3d_gltf_set_recenter(1);                 // centrado en XZ y apoyado en y = 0
            m = g3d_load_gltf(kr_ob_mod[i]);
            g3d_gltf_set_recenter(0);
        END
        IF (m == -1)
            send_notifications(N_WARN, "carreras", "no se pudo cargar " + kr_ob_mod[i]);
            CONTINUE;
        END
        g3d_model_spawn(kr_scene, m, kr_ob_x[i], kr_ob_y[i], kr_ob_z[i], kr_ob_alto[i], kr_ob_yaw[i] * 1000);

        col = kr_ob_col[i];
        IF (col == 0) CONTINUE; END
        h = g3d_model_height(m); s = 1.0;
        IF (kr_ob_alto[i] > 0.0 AND h > 0.0001) s = kr_ob_alto[i] / h; END
        IF (col == 2 AND abs(kr_ob_yaw[i]) < 0.5)
            // malla exacta: solo vale sin giro (el colisionador de malla no gira)
            IF (g3d_collider_add_model(m, kr_ob_x[i], kr_ob_y[i], kr_ob_z[i], s) >= 0) CONTINUE; END
        END
        // caja: la del modelo (sus submallas), escalada y girada como el dibujo (Ry)
        ns = g3d_model_submesh_count(m);
        mnx = 1.0e9; mny = 1.0e9; mnz = 1.0e9; mxx = -1.0e9; mxy = -1.0e9; mxz = -1.0e9;
        FOR (j = 0; j < ns; j = j + 1)
            cx = g3d_model_submesh_cx(m, j); cy = g3d_model_submesh_cy(m, j); cz = g3d_model_submesh_cz(m, j);
            hx = g3d_model_submesh_hx(m, j); hy = g3d_model_submesh_hy(m, j); hz = g3d_model_submesh_hz(m, j);
            IF (cx - hx < mnx) mnx = cx - hx; END  IF (cx + hx > mxx) mxx = cx + hx; END
            IF (cy - hy < mny) mny = cy - hy; END  IF (cy + hy > mxy) mxy = cy + hy; END
            IF (cz - hz < mnz) mnz = cz - hz; END  IF (cz + hz > mxz) mxz = cz + hz; END
        END
        IF (ns <= 0) CONTINUE; END
        cs = cos(kr_ob_yaw[i] * 1000); sn = sin(kr_ob_yaw[i] * 1000);
        bx0 = 1.0e9; bz0 = 1.0e9; bx1 = -1.0e9; bz1 = -1.0e9;
        FOR (k = 0; k < 4; k = k + 1)
            IF (k == 0 OR k == 1) lx = mnx * s; ELSE lx = mxx * s; END
            IF (k == 0 OR k == 2) lz = mnz * s; ELSE lz = mxz * s; END
            wx = lx * cs + lz * sn;  wz = -lx * sn + lz * cs;
            IF (wx < bx0) bx0 = wx; END  IF (wx > bx1) bx1 = wx; END
            IF (wz < bz0) bz0 = wz; END  IF (wz > bz1) bz1 = wz; END
        END
        g3d_collider_add_box(kr_ob_x[i] + bx0, kr_ob_y[i] + mny * s, kr_ob_z[i] + bz0,
                             kr_ob_x[i] + bx1, kr_ob_y[i] + mxy * s, kr_ob_z[i] + bz1);
    END
    IF (kr_n_obj > 0) send_notifications(N_INFO, "carreras", "" + kr_n_obj + " objetos colocados"); END
END

// =========================== input ===========================================
FUNCTION kr_leer_input()
PRIVATE
    float ax; int ci; float ddx; float ddz; float ang; float dif; float dif2; int look;
BEGIN
    kr_in_acel = 0.0; kr_in_giro = 0.0; kr_in_salto = 0; kr_in_reset = 0;
    kr_frame = kr_frame + 1;

    IF (kr_auto == 4 AND kr_n_cp > 0)
        // bot: persigue el checkpoint que va unos metros por delante
        look = 2 + kr_kart_vel / 15.0;
        ci = (kr_cp_cur + look) % kr_n_cp;
        ddx = kr_cp_x[ci] - kr_kart_x; ddz = kr_cp_z[ci] - kr_kart_z;
        dif = atan2(ddx, ddz) / 1000.0 - kr_kart_yaw;
        WHILE (dif > 180.0)  dif = dif - 360.0; END
        WHILE (dif < -180.0) dif = dif + 360.0; END
        kr_in_giro = clamp(-dif / 15.0, -1.0, 1.0);     // yaw sube = girar a la izquierda
        // curva mas adelante: soltar el gas o frenar
        ci = (kr_cp_cur + 14) % kr_n_cp;
        ddx = kr_cp_x[ci] - kr_kart_x; ddz = kr_cp_z[ci] - kr_kart_z;
        dif2 = atan2(ddx, ddz) / 1000.0 - kr_kart_yaw;
        WHILE (dif2 > 180.0)  dif2 = dif2 - 360.0; END
        WHILE (dif2 < -180.0) dif2 = dif2 + 360.0; END
        kr_in_acel = 1.0;
        IF (abs(dif2) > 30.0 AND kr_kart_vel > 24.0) kr_in_acel = 0.0; END
        IF (abs(dif2) > 55.0 AND kr_kart_vel > 20.0) kr_in_acel = -0.6; END
        RETURN;
    END

    IF (kr_auto)
        kr_in_acel = 1.0;
        IF (kr_auto == 1 AND kr_frame > 120) kr_in_giro = 0.6; END              // auto
        IF (kr_auto == 5) kr_in_acel = 0.7; END                                  // lago
        IF (kr_auto == 3 AND kr_frame >= 90 AND kr_frame < 240)                  // derrape
            kr_in_salto = 1; kr_in_giro = -0.5;
        END
        RETURN;
    END

    IF (key(_UP)    OR key(_W)) kr_in_acel = kr_in_acel + 1.0; END
    IF (key(_DOWN)  OR key(_S)) kr_in_acel = kr_in_acel - 1.0; END
    IF (key(_RIGHT) OR key(_D)) kr_in_giro = kr_in_giro + 1.0; END
    IF (key(_LEFT)  OR key(_A)) kr_in_giro = kr_in_giro - 1.0; END
    IF (key(_SPACE)) kr_in_salto = 1; END
    IF (key(_R)) kr_in_reset = 1; END
    IF (joy_number() > 0)
        ax = joy_getaxis(0, JOY_AXIS_LEFTX) / 32767.0;
        IF (ax > KR_ZONA_MUERTA OR ax < -KR_ZONA_MUERTA) kr_in_giro = kr_in_giro + ax; END
        kr_in_acel = kr_in_acel + (joy_getaxis(0, JOY_AXIS_TRIGGERRIGHT)
                                 - joy_getaxis(0, JOY_AXIS_TRIGGERLEFT)) / 32767.0;
        IF (joy_getbutton(0, JOY_BUTTON_A)) kr_in_salto = 1; END
    END
    kr_in_acel = clamp(kr_in_acel, -1.0, 1.0);
    kr_in_giro = clamp(kr_in_giro, -1.0, 1.0);
END

// Lee el input antes que el kart, y en las pruebas deja la posicion en el log.
PROCESS kr_control()
PRIVATE
    int t;
BEGIN
    priority = 10;
    LOOP
        kr_leer_input();
        IF (kr_auto)
            t = t + 1;
            IF (t % 15 == 0)
                send_notifications(N_DEBUG, "auto", "t=" + t + " pos=" + kr_kart_x + "," + kr_kart_y + ","
                                   + kr_kart_z + " yaw=" + kr_kart_yaw + " vel=" + kr_kart_vel);
            END
            IF (kr_auto == 4)
                IF (kr_fin) send_notifications(N_INFO, "auto", "bot: carrera terminada en " + kr_tiempo(kr_t_race)); exit(); END
                IF (t >= 5400) send_notifications(N_WARN, "auto", "bot: tiempo agotado (prog " + kr_prog + " de " + (kr_d0 + kr_vueltas * kr_n_cp) + ")"); exit(); END
            ELSEIF (t >= kr_auto_dur)
                send_notifications(N_INFO, "auto", "fin de la prueba"); exit();
            END
        END
        FRAME;
    END
END

// =========================== kart ============================================
// Un solo proceso lleva fisica, carrera (checkpoints, vueltas, tiempos), superficies,
// agua, placas de turbo y reaparicion.
PROCESS kr_kart(float sx, float sy, float sz, float syaw)
PRIVATE
    int   ch; int ent; int mat; int modelo;
    float yaw; float vx; float vz;
    float fx; float fz; float rx; float rz;
    float vf; float vl; float gv; float k; float punta;
    float px; float py; float pz; float ox; float oy; float oz;
    int   suelo; int suelo_ant; int salto_ant;
    int   derrape; int dir; float carga;
    int   turbo_t; int tramp_cd; int tr;
    float vis_yaw; float vis_pitch; float vis_roll; float dip; float dh; float obj;
    int   kmh;
    // entradas efectivas (la cuenta atras y la meta las anulan)
    float a_acel; float a_giro; int a_salto;
    // superficie
    int   surf; float sgrip; float spunta; float sacel; int on_road; int lg; int agua_t; int caida_t; int on_alt; int alt_sup; int zi; int fuera_t; float y_min; float d_fuera; int atasco_t;
    // carrera
    int   cp_i; int cp_start; int ok_i; int ok_prog; int lap_on; int best; int j; int ci; int dlt; int hay;
    float dx2; float dz2; float dd; float bd; float fx2; float fz2; float lon; float lat;
    int   lap_t; int reinicio; int hh; int mm;
    // respaldo sin checkpoints
    float okx; float oky; float okz; float okyaw; int ok_t;
BEGIN
    priority = 0;
    ch = g3d_char_create(sx, sy, sz, KR_RADIO, KR_ALTO);
    IF (ch < 0) send_notifications(N_ERROR, "carreras", "g3d_char_create fallo"); RETURN; END
    g3d_char_set_tuning(ch, 0.35, 50.0);

    ent = -1; modelo = -1;
    IF (kr_kart_modelo != "" AND fexists(kr_kart_modelo))
        g3d_gltf_set_recenter(1);
        modelo = g3d_load_gltf(kr_kart_modelo);
        g3d_gltf_set_recenter(0);
        IF (modelo != -1) ent = g3d_model_spawn(kr_scene, modelo, sx, sy, sz, kr_kart_tam, 0.0); END
    END
    IF (ent < 0)
        IF (kr_kart_modelo != "")
            send_notifications(N_WARN, "carreras", "sin modelo de kart " + kr_kart_modelo + ": uso un cubo");
        END
        mat = g3d_material_create();
        g3d_material_set_color(mat, 0.90, 0.20, 0.15, 1.0);
        ent = g3d_entity_spawn(kr_scene, 0, sx, sy, sz);
        g3d_entity_set_mesh(ent, g3d_primitive_cube());
        g3d_entity_set_material(ent, mat);
        g3d_entity_set_scale(ent, 1.4, KR_MEDIO * 2.0, 2.0);
        modelo = -1;
    END

    yaw = syaw; px = sx; py = sy; pz = sz;
    okx = sx; oky = sy; okz = sz; okyaw = syaw;
    kr_kart_x = px; kr_kart_y = py; kr_kart_z = pz; kr_kart_yaw = yaw;
    surf = KR_S_ASFALTO;

    // checkpoint de salida = el mas cercano al kart
    cp_i = 0; bd = 1.0e18;
    FOR (j = 0; j < kr_n_cp; j = j + 1)
        dx2 = sx - kr_cp_x[j]; dz2 = sz - kr_cp_z[j]; dd = dx2 * dx2 + dz2 * dz2;
        IF (dd < bd) bd = dd; cp_i = j; END
    END
    cp_start = cp_i; ok_i = cp_i; ok_prog = 0; kr_cp_cur = cp_i;
    kr_prog = 0; kr_lap = 1; lap_on = 0; kr_fin = 0; kr_t_race = 0; kr_t_lap = 0; kr_best = 0;
    IF (kr_n_cp > 0) kr_d0 = (kr_n_cp - cp_start) % kr_n_cp; ELSE kr_d0 = 0; END
    // el punto mas bajo del trazado: por debajo de el (con margen) el kart se ha caido de la pista al vacio
    y_min = 0.0;
    IF (kr_n_cp > 0)
        y_min = kr_cp_y[0];
        FOR (j = 1; j < kr_n_cp; j = j + 1) IF (kr_cp_y[j] < y_min) y_min = kr_cp_y[j]; END END
    END
    fuera_t = 0; atasco_t = 0;

    LOOP
        // ---------- entradas efectivas ----------
        a_acel = kr_in_acel; a_giro = kr_in_giro; a_salto = kr_in_salto;
        IF (kr_cuenta > 0 OR kr_fin) a_acel = 0.0; a_giro = 0.0; a_salto = 0; END
        IF (kr_cuenta > 0)
            kr_cuenta = kr_cuenta - 1;
            IF (kr_cuenta > 120) kr_msg = "3"; ELSEIF (kr_cuenta > 60) kr_msg = "2"; ELSEIF (kr_cuenta > 0) kr_msg = "1";
            ELSE kr_msg = "YA!"; kr_msg_t = 70; END
        ELSEIF (kr_msg_t > 0)
            kr_msg_t = kr_msg_t - 1;
            IF (kr_msg_t == 0 AND !kr_fin) kr_msg = ""; END
        END

        // ---------- superficie: efectos ----------
        sgrip = 1.0; spunta = 1.0; sacel = 1.0;
        IF (surf <= KR_S_BARRO)      sgrip = kr_ef_ag[surf]; spunta = kr_ef_vel[surf]; sacel = kr_ef_acel[surf];
        ELSEIF (surf == KR_S_FUERA)  sgrip = 0.80; spunta = 0.55; sacel = 0.70;
        ELSEIF (surf == KR_S_AGUA)   sgrip = 0.70; spunta = 0.40; sacel = 0.80; END
        FOR (zi = 0; zi <= 3; zi++)                       // zonas pintadas en el terreno (barro, turbo, hielo...)
            IF (kr_zn_on[zi] AND g3d_zone_blocked(px, pz, zi))
                sgrip = sgrip * kr_zn_ag[zi]; spunta = spunta * kr_zn_vel[zi]; sacel = sacel * kr_zn_acel[zi];
            END
        END

        suelo = g3d_char_grounded(ch);
        IF (a_salto AND !salto_ant AND suelo AND !derrape)
            g3d_char_jump(ch, KR_HOP); suelo = 0;
        END

        fx = sin(yaw * 1000); fz = cos(yaw * 1000);
        rx = cos(yaw * 1000); rz = -sin(yaw * 1000);
        vf = vx * fx + vz * fz;
        vl = vx * rx + vz * rz;

        punta = KR_PUNTA * spunta;
        IF (turbo_t > 0)
            punta = KR_PUNTA * KR_TURBO_PUNTA;
            IF (vf < punta) vf = vf + KR_TURBO_ACEL * KR_DT; END
            turbo_t = turbo_t - 1;
        END
        IF (a_acel > 0.0)
            IF (vf < punta) vf = vf + KR_ACEL * sacel * a_acel * KR_DT; END
        ELSEIF (a_acel < 0.0)
            IF (vf > 0.0) vf = vf + KR_FRENO * a_acel * KR_DT;
            ELSE          vf = vf + KR_ACEL * 0.5 * a_acel * KR_DT; END
        ELSE
            vf = vf - vf * KR_ROCE * KR_DT;
        END
        IF (vf > punta) vf = vf - (vf - punta) * 3.0 * KR_DT; END
        IF (vf < -KR_ATRAS) vf = -KR_ATRAS; END

        IF (derrape)
            IF (!a_salto OR vf < KR_DER_MIN * 0.6)
                IF (carga >= KR_CARGA2)     turbo_t = KR_TURBO2;
                ELSEIF (carga >= KR_CARGA1) turbo_t = KR_TURBO1; END
                send_notifications(N_DEBUG, "carreras", "fin derrape (carga " + carga + " s, turbo " + turbo_t + ")");
                derrape = 0;
            ELSE
                gv = dir * (KR_DER_BASE + KR_DER_MOD * a_giro * dir);
                IF (suelo) yaw = yaw - gv * KR_DT; carga = carga + KR_DT; END
            END
        END
        IF (!derrape)
            gv = KR_GIRO * a_giro * clamp(abs(vf) / 10.0, 0.0, 1.0);
            IF (vf < 0.0) gv = -gv; END
            IF (suelo) yaw = yaw - gv * KR_DT; ELSE yaw = yaw - gv * KR_AIRE * KR_DT; END
        END

        IF (suelo)
            IF (derrape) k = KR_DER_AGARRE * KR_DT; ELSE k = KR_AGARRE * sgrip * KR_DT; END
            vl = vl - vl * clamp(k, 0.0, 1.0);
        END
        vx = fx * vf + rx * vl;
        vz = fz * vf + rz * vl;

        ox = px; oy = py; oz = pz;
        g3d_char_move(ch, vx, vz);
        g3d_char_update(ch, KR_DT);
        px = g3d_char_x(ch); py = g3d_char_y(ch); pz = g3d_char_z(ch);
        suelo = g3d_char_grounded(ch);
        vx = (px - ox) / KR_DT;
        vz = (pz - oz) / KR_DT;

        IF (suelo AND !suelo_ant)
            dip = -0.18;
            IF (a_salto AND !derrape AND vf > KR_DER_MIN AND (a_giro > 0.3 OR a_giro < -0.3))
                derrape = 1; carga = 0.0;
                IF (a_giro > 0.0) dir = 1; ELSE dir = -1; END
                send_notifications(N_DEBUG, "carreras", "derrape dir=" + dir);
            END
        END

        // ---------- trampolines y placas de turbo ----------
        IF (tramp_cd > 0) tramp_cd = tramp_cd - 1; END
        IF (suelo AND tramp_cd == 0)
            tr = kr_trampolin_en(px, py, pz);
            IF (tr >= 0)
                g3d_char_jump(ch, kr_tr_fuerza[tr]);
                IF (turbo_t < 30) turbo_t = 30; END
                tramp_cd = 40;
                send_notifications(N_DEBUG, "carreras", "trampolin " + tr);
            END
        END
        IF (suelo AND kr_turbo_en(px, py, pz))
            IF (turbo_t < 50) turbo_t = 50; END
        END

        // ---------- progreso por checkpoints ----------
        on_road = 0;
        IF (kr_n_cp > 4)
            best = -1; bd = 1.0e18;
            FOR (j = -4; j <= 25; j = j + 1)
                ci = (cp_i + j + kr_n_cp * 2) % kr_n_cp;
                dx2 = px - kr_cp_x[ci]; dz2 = pz - kr_cp_z[ci]; dd = dx2 * dx2 + dz2 * dz2;
                IF (abs(py - kr_cp_y[ci]) < 15.0 AND dd < bd) bd = dd; best = ci; END
            END
            IF (best >= 0)
                dlt = best - cp_i;
                IF (dlt > kr_n_cp / 2) dlt = dlt - kr_n_cp; ELSEIF (dlt < -kr_n_cp / 2) dlt = dlt + kr_n_cp; END
                kr_prog = kr_prog + dlt; cp_i = best; kr_cp_cur = best;
                fx2 = sin(kr_cp_yaw[best] * 1000); fz2 = cos(kr_cp_yaw[best] * 1000);
                dx2 = px - kr_cp_x[best]; dz2 = pz - kr_cp_z[best];
                lon = dx2 * fx2 + dz2 * fz2; lat = dx2 * fz2 - dz2 * fx2;
                IF (kr_cp_sup[best] >= 0 AND abs(lat) <= kr_cp_hw[best] + 0.4 AND abs(lon) <= kr_paso * 0.8
                    AND abs(py - kr_cp_y[best]) < 5.0)
                    on_road = 1;
                END
            END
        END

        // ---------- atajos: carretera de otro camino (sin checkpoints) ----------
        on_alt = 0;
        IF (!on_road AND kr_n_ar > 0 AND suelo)
            FOR (j = 0; j < kr_n_ar; j = j + 1)
                dx2 = px - kr_ar_x[j]; dz2 = pz - kr_ar_z[j];
                IF (dx2 * dx2 + dz2 * dz2 < 144.0 AND abs(py - kr_ar_y[j]) < 5.0 AND kr_ar_sup[j] >= 0)
                    fx2 = sin(kr_ar_yaw[j] * 1000); fz2 = cos(kr_ar_yaw[j] * 1000);
                    lon = dx2 * fx2 + dz2 * fz2; lat = dx2 * fz2 - dz2 * fx2;
                    IF (abs(lat) <= kr_ar_hw[j] + 0.4 AND abs(lon) <= kr_paso * 0.8) on_alt = 1; alt_sup = kr_ar_sup[j]; BREAK; END
                END
            END
        END

        // ---------- superficie bajo el kart (en el aire se conserva la ultima) ----------
        lg = kr_lago_en(px, py, pz);
        IF (suelo)
            IF (lg >= 0)       surf = KR_S_AGUA;
            ELSEIF (on_road)   surf = kr_cp_sup[cp_i];
            ELSEIF (on_alt)    surf = alt_sup;
            ELSEIF (kr_n_cp > 4) surf = KR_S_FUERA;
            ELSE               surf = KR_S_ASFALTO; END
        END

        // ---------- vueltas ----------
        IF (kr_cuenta == 0 AND !kr_fin AND kr_n_cp > 4)
            kr_t_race = kr_t_race + 1; kr_t_lap = kr_t_lap + 1;
            IF (!lap_on AND kr_prog >= kr_d0) lap_on = 1; kr_t_lap = 0; END
            IF (lap_on AND kr_prog >= kr_d0 + kr_lap * kr_n_cp)
                lap_t = kr_t_lap;
                IF (kr_best == 0 OR lap_t < kr_best) kr_best = lap_t; END
                send_notifications(N_INFO, "carreras", "vuelta " + kr_lap + "/" + kr_vueltas + ": " + kr_tiempo(lap_t));
                kr_t_lap = 0;
                IF (kr_lap >= kr_vueltas)
                    kr_fin = 1;
                    kr_msg = "META!  " + kr_tiempo(kr_t_race) + "   (ENTER = repetir)";
                    send_notifications(N_INFO, "carreras", "carrera terminada: " + kr_tiempo(kr_t_race));
                ELSE
                    kr_lap = kr_lap + 1;
                END
            END
        END

        // ---------- reaparicion ----------
        reinicio = 0;
        IF (kr_n_cp > 4)
            IF (on_road AND suelo) ok_i = cp_i; ok_prog = kr_prog; END
            IF (suelo AND !on_road AND !on_alt AND lg < 0 AND py < kr_cp_y[cp_i] - 5.0) caida_t = caida_t + 1; ELSE caida_t = 0; END
            IF (lg >= 0 AND kr_lg_mortal[lg] AND suelo) agua_t = agua_t + 1; ELSE agua_t = 0; END
            // fuera de la pista: al vacio (mucho mas abajo que todo el trazado), encima de un techo o de un muro (otro nivel
            // que el del trazado) o lejisimos de el. Medio segundo largo de gracia y vuelve al ultimo checkpoint bueno.
            d_fuera = (px - kr_cp_x[cp_i]) * (px - kr_cp_x[cp_i]) + (pz - kr_cp_z[cp_i]) * (pz - kr_cp_z[cp_i]);
            IF (!on_road AND !on_alt AND lg < 0
                AND (py < y_min - 25.0 OR (suelo AND (abs(py - kr_cp_y[cp_i]) > 6.0 OR d_fuera > 6400.0))))
                fuera_t = fuera_t + 1;
            ELSE fuera_t = 0; END
            IF (py < y_min - 60.0) fuera_t = 999; END        // cayendo al vacio: no se espera
            // atascado: acelerando y sin avanzar (clavado contra algo, encajado entre un saliente y un techo): ~3,5 s y vuelve
            IF (a_acel > 0.5 AND vf < 1.5 AND suelo AND kr_cuenta == 0 AND !kr_fin) atasco_t = atasco_t + 1; ELSE atasco_t = 0; END
            IF (caida_t > 25 OR agua_t > 70 OR kr_in_reset OR fuera_t > 90 OR atasco_t > 210)
                px = kr_cp_x[ok_i]; py = kr_cp_y[ok_i] + 1.2; pz = kr_cp_z[ok_i]; yaw = kr_cp_yaw[ok_i];
                cp_i = ok_i; kr_prog = ok_prog; reinicio = 1;
                send_notifications(N_INFO, "carreras", "reaparece en el checkpoint " + ok_i);
            END
        ELSE
            IF (suelo) ok_t = ok_t + 1; IF (ok_t >= 30) okx = px; oky = py; okz = pz; okyaw = yaw; ok_t = 0; END END
            IF (py < KR_Y_CAIDA)
                px = okx; py = oky + 1.0; pz = okz; yaw = okyaw; reinicio = 1;
                send_notifications(N_INFO, "carreras", "respawn");
            END
        END
        IF (px < kr_lim_x0 OR px > kr_lim_x1 OR pz < kr_lim_z0 OR pz > kr_lim_z1)
            IF (kr_n_cp > 4) px = kr_cp_x[ok_i]; py = kr_cp_y[ok_i] + 1.2; pz = kr_cp_z[ok_i]; yaw = kr_cp_yaw[ok_i]; cp_i = ok_i; kr_prog = ok_prog;
            ELSE px = okx; py = oky + 1.0; pz = okz; yaw = okyaw; END
            reinicio = 1;
        END
        // repetir la carrera al acabar
        IF (kr_fin AND key(_ENTER))
            px = sx; py = sy; pz = sz; yaw = syaw; cp_i = cp_start; ok_i = cp_start; ok_prog = 0; reinicio = 1;
            kr_prog = 0; kr_lap = 1; lap_on = 0; kr_fin = 0; kr_t_race = 0; kr_t_lap = 0;
            kr_cuenta = 180; kr_msg = "";
        END
        IF (reinicio)
            g3d_char_set_position(ch, px, py, pz);
            vx = 0.0; vz = 0.0; derrape = 0; turbo_t = 0; caida_t = 0; agua_t = 0; fuera_t = 0; atasco_t = 0; surf = KR_S_ASFALTO;
            ox = px; oy = py; oz = pz;
        END
        suelo_ant = suelo;
        salto_ant = a_salto;

        // ---------- dibujo ----------
        dh = sqrt((px - ox) * (px - ox) + (pz - oz) * (pz - oz));
        IF (dh > 0.01)
            obj = clamp(-atan2(py - oy, dh), -35000.0, 35000.0);
            IF (!suelo) obj = obj * 0.5; END
            vis_pitch = vis_pitch + (obj - vis_pitch) * 0.25;
        END
        IF (derrape) obj = -dir * 8000.0;
        ELSE obj = -a_giro * clamp(vf / KR_PUNTA, 0.0, 1.0) * 5000.0; END
        vis_roll = vis_roll + (obj - vis_roll) * 0.15;
        IF (derrape) obj = -dir * KR_DER_VIS; ELSE obj = 0.0; END
        vis_yaw = vis_yaw + (obj - vis_yaw) * 0.15;
        dip = dip + (0.0 - dip) * 0.2;
        IF (modelo != -1) g3d_entity_set_position(ent, px, py + dip, pz);
        ELSE g3d_entity_set_position(ent, px, py + KR_MEDIO + dip, pz); END
        g3d_entity_set_rotation(ent, vis_pitch, (yaw + vis_yaw) * 1000, vis_roll);

        kr_kart_x = px; kr_kart_y = py; kr_kart_z = pz; kr_kart_yaw = yaw; kr_kart_vel = vf;
        kr_kart_turbo = turbo_t > 0;

        // ---------- HUD ----------
        kmh = vf * 3.6;
        kr_hud = "" + kmh + " km/h";
        IF (derrape)
            IF (carga >= KR_CARGA2)     kr_hud = kr_hud + "   DERRAPE **";
            ELSEIF (carga >= KR_CARGA1) kr_hud = kr_hud + "   DERRAPE *";
            ELSE                         kr_hud = kr_hud + "   DERRAPE"; END
        END
        IF (turbo_t > 0) kr_hud = kr_hud + "   TURBO!"; END
        IF (surf == KR_S_TIERRA) kr_hud = kr_hud + "   [tierra]"; END
        IF (surf == KR_S_HIELO)  kr_hud = kr_hud + "   [hielo]"; END
        IF (surf == KR_S_BARRO)  kr_hud = kr_hud + "   [barro]"; END
        IF (surf == KR_S_FUERA)  kr_hud = kr_hud + "   [fuera de pista]"; END
        IF (surf == KR_S_AGUA)   kr_hud = kr_hud + "   [agua]"; END
        IF (kr_n_cp > 4)
            kr_hud2 = "Vuelta " + kr_lap + "/" + kr_vueltas + "   " + kr_tiempo(kr_t_race);
            IF (kr_best > 0) kr_hud2 = kr_hud2 + "   Mejor vuelta " + kr_tiempo(kr_best); END
            IF (kr_amb_modo == 3)
                hh = kr_hora; mm = (kr_hora - hh) * 60.0;
                kr_hud2 = kr_hud2 + "   ";
                IF (hh < 10) kr_hud2 = kr_hud2 + "0"; END
                kr_hud2 = kr_hud2 + hh + ":";
                IF (mm < 10) kr_hud2 = kr_hud2 + "0"; END
                kr_hud2 = kr_hud2 + mm;
            END
        ELSE
            kr_hud2 = "";
        END
        FRAME;
    END
END

// =========================== ambiente: sol, luna, cielo, farolas y faros ==========
// La hora lleva el sol por el cielo (0 amanecer a las 6, mediodia a las 12, ocaso a las 18); de noche
// queda la luna, el cielo oscuro, y se encienden las luces "de noche" y los faros del kart.
PROCESS kr_ambiente()
PRIVATE
    float h; float ang; float e; float d; float t; float k;
    float cr; float cg; float cb; float hr; float hg; float hb;
    float fx; float fz; int i;
BEGIN
    priority = -20;
    h = kr_amb_hora;
    IF (kr_amb_modo == 1) h = 10.0; END
    IF (kr_amb_modo == 2) h = 0.0; END
    LOOP
        IF (kr_amb_modo == 3)
            h = h + 24.0 / (kr_amb_dur * 60.0);
            IF (h >= 24.0) h = h - 24.0; END
        END
        kr_hora = h;
        IF (kr_amb_modo == 0)
            kr_noche = 0.0;                                   // la escena manda: no se toca el cielo
        ELSE
            ang = (h - 6.0) * 15.0;
            e = sin(ang * 1000);                              // altura del sol: 1 mediodia, 0 horizonte, -1 medianoche
            d = clamp((e + 0.10) / 0.40, 0.0, 1.0);           // 0 noche .. 1 dia
            t = clamp(1.0 - abs(e) / 0.28, 0.0, 1.0);         // crepusculo
            kr_noche = 1.0 - d;
            IF (kr_sol >= 0)
                IF (e > -0.05) g3d_light_set_direction(kr_sol, -cos(ang * 1000) * 0.9, -(0.12 + 0.88 * clamp(e, 0.0, 1.0)), -0.3);
                ELSE g3d_light_set_direction(kr_sol, 0.35, -0.8, 0.3); END                // la luna
                g3d_light_set_intensity(kr_sol, kr_mez(0.22, 1.5, d));
                cr = kr_mez(0.45, 1.0, d); cg = kr_mez(0.55, 0.95, d); cb = kr_mez(1.0, 0.85, d);
                k = t * 0.75;
                cr = kr_mez(cr, 1.0, k); cg = kr_mez(cg, 0.55, k); cb = kr_mez(cb, 0.30, k);
                g3d_light_set_color(kr_sol, cr, cg, cb);
            END
            g3d_set_ambient_light(kr_mez(0.07, 0.35, d), kr_mez(0.09, 0.38, d), kr_mez(0.18, 0.45, d), 1.0);
            hr = kr_mez(0.04, 0.75, d); hg = kr_mez(0.06, 0.80, d); hb = kr_mez(0.14, 0.85, d);
            k = t * 0.8;
            hr = kr_mez(hr, 0.95, k); hg = kr_mez(hg, 0.50, k); hb = kr_mez(hb, 0.30, k);
            g3d_sky_set_gradient(kr_mez(0.01, 0.35, d), kr_mez(0.02, 0.55, d), kr_mez(0.08, 0.85, d), hr, hg, hb);
            g3d_set_exposure(kr_mez(1.0, 1.35, kr_noche));
        END
        // farolas: las "de noche" suben con la oscuridad
        FOR (i = 0; i < kr_n_luz; i = i + 1)
            IF (kr_lz_id[i] >= 0)
                IF (kr_lz_noche[i]) g3d_light_set_intensity(kr_lz_id[i], kr_lz_inten[i] * kr_noche);
                ELSE g3d_light_set_intensity(kr_lz_id[i], kr_lz_inten[i]); END
            END
        END
        // faros del kart: un foco hacia delante, de noche
        IF (kr_faro_id >= 0)
            fx = sin(kr_kart_yaw * 1000); fz = cos(kr_kart_yaw * 1000);
            g3d_light_set_position(kr_faro_id, kr_kart_x + fx * 1.4, kr_kart_y + 1.0, kr_kart_z + fz * 1.4);
            g3d_light_set_direction(kr_faro_id, fx, -0.16, fz);
            g3d_light_set_intensity(kr_faro_id, 4.0 * kr_noche);
        END
        FRAME;
    END
END

// =========================== semaforo de salida ===============================
// Tres luces: se encienden en rojo una a una (3, 2, 1) y al final todas en verde.
PROCESS kr_semaforo()
PRIVATE
    int st; int ult; int i; int col;
BEGIN
    kr_sem_map = map_new(230, 80, 32);
    x = 640; y = 120; z = -900;
    ult = -1;
    LOOP
        st = 0;
        IF (kr_cuenta > 120) st = 1;
        ELSEIF (kr_cuenta > 60) st = 2;
        ELSEIF (kr_cuenta > 0) st = 3;
        ELSEIF (kr_msg_t > 0 AND !kr_fin) st = 4; END
        IF (st != ult)
            ult = st;
            IF (st > 0)
                map_clear(0, kr_sem_map, rgba(10, 10, 14, 170));
                drawing_map(0, kr_sem_map);
                FOR (i = 0; i < 3; i = i + 1)
                    IF (st == 4)         col = rgba(40, 230, 80, 255);
                    ELSEIF (i < st)      col = rgba(240, 50, 40, 255);
                    ELSE                 col = rgba(55, 30, 30, 255); END
                    drawing_color(col);
                    draw_circle_filled(40 + i * 75, 40, 29);
                END
                graph = kr_sem_map;
            ELSE
                graph = 0;
            END
        END
        FRAME;
    END
END

// =========================== camara ==========================================
PROCESS kr_camara()
PRIVATE
    float cyaw; float d; float dist; float cy; float fx; float fz;
    float fov_act; float fov_meta;
BEGIN
    priority = -10;
    cyaw = kr_kart_yaw; cy = kr_kart_y + KR_CAM_ALTO; fov_act = KR_CAM_FOV;
    LOOP
        d = kr_kart_yaw - cyaw;
        WHILE (d > 180.0)  d = d - 360.0; END
        WHILE (d < -180.0) d = d + 360.0; END
        cyaw = cyaw + d * KR_CAM_GIRO;
        fx = sin(cyaw * 1000); fz = cos(cyaw * 1000);
        dist = g3d_camera_safe_distance(kr_kart_x, kr_kart_y + 1.0, kr_kart_z, -fx, 0.3, -fz, KR_CAM_DIST, 0.4);
        cy = cy + (kr_kart_y + KR_CAM_ALTO - cy) * KR_CAM_Y;
        IF (cy < kr_kart_y + 1.2) cy = kr_kart_y + 1.2; END
        IF (kr_n_lago > 0 AND cy < KR_AGUA_Y + 1.6) cy = KR_AGUA_Y + 1.6; END   // nunca bajo la superficie del agua
        g3d_camera_set_position(kr_camera, kr_kart_x - fx * dist, cy, kr_kart_z - fz * dist);
        g3d_camera_look_at(kr_camera, kr_kart_x + fx * KR_CAM_MIRA, kr_kart_y + 1.0,
                           kr_kart_z + fz * KR_CAM_MIRA, 0.0, 1.0, 0.0);
        fov_meta = KR_CAM_FOV + KR_CAM_FOV_V * clamp(kr_kart_vel / KR_PUNTA, 0.0, 1.0);
        IF (kr_kart_turbo) fov_meta = fov_meta + KR_CAM_FOV_T; END
        fov_act = fov_act + (fov_meta - fov_act) * 0.1;
        g3d_camera_set_fov(kr_camera, fov_act);
        FRAME;
    END
END

// Luces puntuales de las farolas (con su poste) y el foco de los faros del kart.
FUNCTION kr_luces_montar()
PRIVATE
    int i; int ent; int mat;
BEGIN
    FOR (i = 0; i < kr_n_luz; i = i + 1)
        kr_lz_id[i] = g3d_light_create(1, kr_lz_r[i], kr_lz_g[i], kr_lz_b[i]);
        IF (kr_lz_id[i] >= 0)
            g3d_light_set_position(kr_lz_id[i], kr_lz_x[i], kr_lz_y[i], kr_lz_z[i]);
            g3d_light_set_range(kr_lz_id[i], kr_lz_radio[i]);
            g3d_light_set_intensity(kr_lz_id[i], 0.0);
        END
        IF (kr_lz_poste[i])
            mat = g3d_material_create(); g3d_material_set_color(mat, 0.18, 0.18, 0.2, 1.0);       // el poste
            ent = g3d_entity_spawn(kr_scene, 0, kr_lz_x[i], kr_lz_y[i] * 0.5 - 0.05, kr_lz_z[i]);
            g3d_entity_set_mesh(ent, g3d_primitive_cube()); g3d_entity_set_material(ent, mat);
            g3d_entity_set_scale(ent, 0.22, kr_lz_y[i], 0.22);
            mat = g3d_material_create(); g3d_material_set_color(mat, kr_lz_r[i] * 4.0, kr_lz_g[i] * 4.0, kr_lz_b[i] * 4.0, 1.0);   // la bombilla (brilla con el bloom)
            ent = g3d_entity_spawn(kr_scene, 0, kr_lz_x[i], kr_lz_y[i] + 0.1, kr_lz_z[i]);
            g3d_entity_set_mesh(ent, g3d_primitive_sphere(12)); g3d_entity_set_material(ent, mat);
            g3d_entity_set_scale(ent, 0.55, 0.55, 0.55);
        END
    END
    IF (kr_faros)
        kr_faro_id = g3d_light_create(2, 1.0, 0.95, 0.80);
        IF (kr_faro_id >= 0) g3d_light_set_range(kr_faro_id, 48.0); g3d_light_set_cone(kr_faro_id, 38.0); g3d_light_set_intensity(kr_faro_id, 0.0); END
    END
    IF (kr_n_luz > 0 OR kr_faros) g3d_set_bloom(1, 0.35, 1.0); END
END

// =========================== montar / descargar ==============================
// Monta el circuito con la configuracion kr_* ya rellena.
FUNCTION kr_montar()
PRIVATE
    int luz; int m; int r; int i;
BEGIN
    IF (argc > 1)
        IF (argv[1] == "auto")    kr_auto = 1; END
        IF (argv[1] == "recto")   kr_auto = 2; END
        IF (argv[1] == "derrape") kr_auto = 3; END
        IF (argv[1] == "bot")     kr_auto = 4; END
        IF (argv[1] == "lago")    kr_auto = 5; END
    END
    IF (kr_auto == 5 AND kr_n_lago > 0)        // prueba: el kart arranca de cara al primer lago
        kr_salida_x = kr_lg_x[0] - kr_lg_w[0] * 0.5 - 22.0; kr_salida_z = kr_lg_z[0];
        kr_salida_y = 1.0; kr_salida_yaw = 90.0;       // de cara al lago (mirando hacia +X)
    END
    IF (!kr_ef_ok)                          // sin efectos de la escena: los de serie
        kr_ef_ag[0] = 1.0;  kr_ef_vel[0] = 1.0; kr_ef_acel[0] = 1.0;
        kr_ef_ag[1] = 0.65; kr_ef_vel[1] = 0.9; kr_ef_acel[1] = 1.0;
        kr_ef_ag[2] = 0.12; kr_ef_vel[2] = 1.0; kr_ef_acel[2] = 0.6;
        kr_ef_ag[3] = 0.5;  kr_ef_vel[3] = 0.6; kr_ef_acel[3] = 0.7;
    END
    kr_frame = 0;
    kr_fin = 0; kr_msg = ""; kr_msg_t = 0; kr_cuenta = 180;
    IF (kr_auto) kr_cuenta = 0; END          // las pruebas empiezan ya

    IF (kr_amb_modo == 0 AND !kr_externa) kr_amb_modo = 1; END       // sin escena vinculada no hay "la de la escena"
    IF (kr_externa)
        // el terreno, el cielo, las luces y los objetos ya los ha montado la escena vinculada
        kr_scene = kr_ext_scene; kr_camera = kr_ext_camera; kr_sol = kr_ext_light;
        g3d_camera_set_fov(kr_camera, KR_CAM_FOV);
    ELSE
        kr_scene = g3d_scene_create("carreras");
        g3d_scene_set_active(kr_scene);
        kr_camera = g3d_camera_create();
        g3d_camera_set_active(kr_camera);
        g3d_camera_set_fov(kr_camera, KR_CAM_FOV);

        luz = g3d_light_create(0, 1.0, 0.95, 0.85);
        g3d_light_set_direction(luz, -0.4, -0.8, -0.3);
        g3d_light_set_intensity(luz, 1.4);
        g3d_light_enable_shadow(luz, 1);
        g3d_set_shadows(1);
        g3d_set_ambient_light(0.35, 0.38, 0.45, 1.0);
        g3d_sky_set_gradient(0.35, 0.55, 0.85,  0.75, 0.80, 0.85);
        g3d_sky_enable(1);
        g3d_set_hdr(1); g3d_set_tonemap(1); g3d_set_exposure(1.0);
        kr_sol = luz;
    END
    g3d_physics_set_gravity(KR_GRAVEDAD);
    g3d_set_culling(1);                      // las caras traseras no se dibujan (si no se veria el reverso del asfalto desde abajo)

    IF (kr_pista == "" OR !fexists(kr_pista))
        send_notifications(N_ERROR, "carreras", "no existe la pista '" + kr_pista + "' (guarda el circuito en el editor)");
    ELSE
        g3d_gltf_set_recenter(0);                 // coordenadas tal cual: dibujo == colision
        m = g3d_load_gltf(kr_pista);
        IF (m == -1)
            send_notifications(N_ERROR, "carreras", "no se pudo cargar " + kr_pista);
        ELSE
            r = g3d_model_spawn(kr_scene, m, kr_pista_x, kr_pista_y, kr_pista_z, 0.0, 0.0);   // 0.0 = tamano original
            IF (r >= 0 AND (kr_pista_esc != 1.0 OR kr_pista_yaw != 0.0))
                g3d_entity_set_scale(r, kr_pista_esc, kr_pista_esc, kr_pista_esc);
                g3d_entity_set_rotation(r, 0.0, kr_pista_yaw * 1000, 0.0);
            END
            r = g3d_collider_add_model_yaw(m, kr_pista_x, kr_pista_y, kr_pista_z, kr_pista_esc, kr_pista_yaw);
            IF (r < 0) send_notifications(N_ERROR, "carreras", "sin colision de malla (Jolt) para " + kr_pista); END
            IF (kr_extra_vis != "" AND fexists(kr_extra_vis))
                m = g3d_load_gltf(kr_extra_vis);
                IF (m != -1) g3d_model_spawn(kr_scene, m, 0.0, 0.0, 0.0, 0.0, 0.0); END
            END
            IF (kr_extra_col != "" AND fexists(kr_extra_col))
                m = g3d_load_gltf(kr_extra_col);
                IF (m != -1)
                    g3d_model_spawn(kr_scene, m, 0.0, 0.0, 0.0, 0.0, 0.0);
                    g3d_collider_add_model(m, 0.0, 0.0, 0.0, 1.0);
                END
            END
            send_notifications(N_INFO, "carreras", "pista " + kr_pista + ": " + kr_n_cp + " checkpoints, "
                               + kr_n_tramp + " trampolines, " + kr_n_turbo + " turbos, " + kr_n_lago + " lagos, " + kr_n_ar + " puntos de atajos");
        END
    END

    // agua: una zona por lago (la cuenca ya esta excavada en la malla de la pista)
    IF (kr_n_lago > 0)
        g3d_fluid_clear();
        g3d_fluid_style(0.05, 4.0, 0.6,  0.05, 0.25, 0.40,  0.20, 0.50, 0.60,  0.55);
        FOR (i = 0; i < kr_n_lago; i = i + 1)
            g3d_fluid_add(kr_lg_x[i], kr_lg_z[i], kr_lg_w[i], kr_lg_h[i], KR_AGUA_Y, kr_lg_prof[i]);
        END
    END

    kr_objetos_montar();
    kr_luces_montar();
    kr_ambiente();
    IF (!kr_auto) kr_semaforo(); END
    kr_control();
    IF (kr_auto > 0 AND kr_auto != 4 AND argc > 2) kr_auto_dur = atoi(argv[2]); END
    IF (kr_auto == 4 AND argc > 2 AND kr_n_cp > 0)         // prueba: empezar en el checkpoint argv[2]
        kr_cp_cur = atoi(argv[2]) % kr_n_cp;
        kr_salida_x = kr_cp_x[kr_cp_cur]; kr_salida_y = kr_cp_y[kr_cp_cur] + 0.6;
        kr_salida_z = kr_cp_z[kr_cp_cur]; kr_salida_yaw = kr_cp_yaw[kr_cp_cur];
    END
    kr_kart(kr_salida_x, kr_salida_y, kr_salida_z, kr_salida_yaw);
    kr_camara();
    kr_txt_ayuda = write(0, 10, 10, 0, "Flechas/WASD conducir - ESPACIO saltito (mantener girando = derrape) - R volver a pista");
    kr_txt_hud   = write_var(0, 10, 30, 0, kr_hud);
    kr_txt_hud2  = write_var(0, 10, 50, 0, kr_hud2);
    kr_txt_msg   = write_var(0, 640, 280, 4, kr_msg);
END

// Quita el circuito (la escena generada mata ademas los procesos kr_*).
FUNCTION kr_descargar()
PRIVATE
    int i;
BEGIN
    write_delete(kr_txt_ayuda);
    write_delete(kr_txt_hud);
    write_delete(kr_txt_hud2);
    write_delete(kr_txt_msg);
    g3d_fluid_clear();
    FOR (i = 0; i < kr_n_luz; i = i + 1) IF (kr_lz_id[i] >= 0) g3d_light_set_intensity(kr_lz_id[i], 0.0); END END
    IF (kr_faro_id >= 0) g3d_light_set_intensity(kr_faro_id, 0.0); kr_faro_id = -1; END
    IF (kr_sem_map) map_unload(0, kr_sem_map); kr_sem_map = 0; END
    IF (kr_scene >= 0 AND !kr_externa) g3d_scene_destroy(kr_scene); END
    kr_scene = -1; kr_externa = 0;
    kr_n_tramp = 0; kr_n_obj = 0; kr_n_cp = 0; kr_n_ar = 0; kr_n_turbo = 0; kr_n_lago = 0; kr_n_luz = 0;
END
