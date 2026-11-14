// =============================================================================
//  MODULO: la IA de serie (kr_ia_defecto)
//  Carriles, trazada de curva, reaccion a los demas, rutas alternativas y equilibrio de carrera. La usan los pilotos que NO tienen
//  "IA propia". Puedes copiarla a Scripts/carreras_usuario/kr_ia_defecto.prg para cambiarla para todos, o leerla como ejemplo.
// =============================================================================
FUNCTION kr_ia_defecto(int kid)
PRIVATE
    float px; float pz; float yaw; float vf; int cp_i; int lap; int fin;
    float skill; float agr; float vari; float st_vel;
    float a_acel; float a_giro;
    float off; float off_obj; int off_t; int ruta; int ruta_j; int rt_hecho; float bhw;
    int fr_i; float fr_d; float fr_s; int at_i; float at_d; float at_s;
    float tx; float tz; float usable; float lx; float lz; int look; int ci; int ci3; int ci11;
    float dif; float dif2; float dyaw; float curva; float off_ap; float peso_ap; float gom;
    int lanes; int lane_cur; int lane_new;
    int tj; float tcx; float tcz; float tdx; float tdz; float talong; float tside; float tbest; int tsel; int der; int salto;
    float wl; float wr; float wf; float wlen; float wx; float wz; float py;
    int rr; int jj; int rj; int rj3; int rj11; int rj14; int pasa; float pa; float dj0; float dj1; float bx; float bz; float byaw;
BEGIN
    px = kr_k_x[kid]; pz = kr_k_z[kid]; yaw = kr_k_yaw[kid]; vf = kr_k_vel[kid]; cp_i = kr_k_cp[kid]; lap = kr_k_lap[kid]; fin = kr_k_fin[kid];
    skill = kr_p_skill[kid]; agr = kr_p_agr[kid]; vari = kr_p_var[kid]; st_vel = kr_p_vel[kid];
    fr_i = kr_k_fr_i[kid]; fr_d = kr_k_fr_d[kid]; fr_s = kr_k_fr_s[kid]; at_i = kr_k_at_i[kid]; at_d = kr_k_at_d[kid]; at_s = kr_k_at_s[kid];
    off = kr_ia_off[kid]; off_obj = kr_ia_off_obj[kid]; off_t = kr_ia_off_t[kid]; ruta = kr_ia_ruta[kid]; ruta_j = kr_ia_ruta_j[kid]; rt_hecho = kr_ia_rt_hecho[kid]; bhw = kr_ia_bhw[kid];
    a_acel = 0.0; a_giro = 0.0; salto = 0; der = kr_ia_der[kid]; py = kr_k_y[kid];
        IF (kr_n_cp > 4 AND !fin)
        off_t = off_t - 1;
        // carriles: la carretera se reparte en carriles de ~3,4 m y cada piloto se mueve de uno a otro (no en una linea unica)
        usable = bhw * kr_ia_ancho - 2.2; IF (usable < 0.0) usable = 0.0; END
        lanes = (usable * 2.0) / 3.4 + 1; IF (lanes < 1) lanes = 1; END IF (lanes > 6) lanes = 6; END
        IF (off_t <= 0)                                              // de vez en cuando cambia de carril (mas o menos segun su variacion)
            lane_cur = ((off_obj + 1.0) * lanes - 1.0) / 2.0 + 0.5; IF (lane_cur < 0) lane_cur = 0; END IF (lane_cur >= lanes) lane_cur = lanes - 1; END
            lane_new = lane_cur;
            IF (rand(0, 100) < 35 + 60 * vari)
                IF (rand(0, 1)) lane_new = lane_cur + 1; ELSE lane_new = lane_cur - 1; END
                IF (vari > 0.7 AND rand(0, 100) < 30) lane_new = lane_new + (lane_new - lane_cur); END
            END
            IF (lane_new < 0) lane_new = 0; END IF (lane_new >= lanes) lane_new = lanes - 1; END
            off_obj = -1.0 + (2.0 * lane_new + 1.0) / lanes;
            off_t = (260 - 180 * vari) + rand(0, 160);
        END
        // reaccion a los demas: el de delante en mi carril se pasa por el lado con mas sitio; si el de atras aprieta y soy agresivo, le cierro
        IF (fr_i >= 0 AND fr_d < 13.0 AND off_t > 25)
            IF (fr_s >= 0.0) off_obj = off - 0.55 - agr * 0.25; ELSE off_obj = off + 0.55 + agr * 0.25; END
            IF (off_obj > 0.95) off_obj = off - 0.6; END
            IF (off_obj < -0.95) off_obj = off + 0.6; END
            off_t = 40 + rand(0, 40);
        ELSEIF (at_i >= 0 AND agr > 0.55 AND at_d < 9.0 AND off_t > 40)
            IF (at_s >= 0.0) off_obj = off + 0.3 * agr; ELSE off_obj = off - 0.3 * agr; END
            off_t = 55;
        END
        off_obj = clamp(off_obj, -1.0, 1.0);
        off = off + (off_obj - off) * (0.02 + 0.03 * (fr_i >= 0));
        look = 2 + vf / 13.0;
        // ---------- rutas alternativas: antes de una bifurcacion elige por cual ir (los dos tuneles, las cabinas del peaje...) ----------
        IF (ruta < 0)
            FOR (rr = 0; rr < kr_n_rt; rr = rr + 1)
                pasa = (kr_rt_e[rr] - cp_i + kr_n_cp * 2) % kr_n_cp;                       // checkpoints que faltan para la bifurcacion
                IF (pasa >= 6 AND pasa <= 16 AND rt_hecho != rr + 1000 * lap)
                    rt_hecho = rr + 1000 * lap;
                    pa = 0.5 + (vari - 0.5) * 0.2;                                          // por defecto, mitad y mitad; algo mas aventureros los variables
                    IF (fr_i >= 0 AND fr_d < 16.0) pa = pa + 0.2; END                        // si hay alguien delante, prueba el otro camino
                    IF (rand(0, 100) < pa * 100) ruta = rr; ruta_j = 0; END
                    send_notifications(N_DEBUG, "ia", "kart " + kid + " vuelta " + lap + " bifurcacion " + rr + ": " + (ruta == rr));
                END
            END
        END
        IF (ruta >= 0)
            // avanza por la ruta: el punto mas cercano hacia delante
            FOR (jj = 0; jj < 4; jj = jj + 1)
                IF (ruta_j < kr_rt_n[ruta] - 1)
                    rj = kr_rt_ini[ruta] + ruta_j;
                    dj0 = (kr_ar_x[rj] - px) * (kr_ar_x[rj] - px) + (kr_ar_z[rj] - pz) * (kr_ar_z[rj] - pz);
                    dj1 = (kr_ar_x[rj + 1] - px) * (kr_ar_x[rj + 1] - px) + (kr_ar_z[rj + 1] - pz) * (kr_ar_z[rj + 1] - pz);
                    IF (dj1 < dj0) ruta_j = ruta_j + 1; END
                END
            END
            pasa = (kr_rt_x[ruta] - cp_i + kr_n_cp * 2) % kr_n_cp;
            IF (ruta_j >= kr_rt_n[ruta] - 1 OR pasa < 2 OR pasa > kr_n_cp / 2) ruta = -1; END          // llego a la union con el principal
        END
        IF (ruta >= 0)
            rj = kr_rt_ini[ruta] + clamp(ruta_j + look, 0, kr_rt_n[ruta] - 1);
            bx = kr_ar_x[rj]; bz = kr_ar_z[rj]; byaw = kr_ar_yaw[rj]; bhw = kr_ar_hw[rj];
            rj3 = kr_rt_ini[ruta] + clamp(ruta_j + 3, 0, kr_rt_n[ruta] - 1); rj11 = kr_rt_ini[ruta] + clamp(ruta_j + 11, 0, kr_rt_n[ruta] - 1);
            rj14 = kr_rt_ini[ruta] + clamp(ruta_j + 14, 0, kr_rt_n[ruta] - 1);
        ELSE
            ci = (cp_i + look) % kr_n_cp;
            bx = kr_cp_x[ci]; bz = kr_cp_z[ci]; byaw = kr_cp_yaw[ci]; bhw = kr_cp_hw[ci];
        END
        kr_k_rt[kid] = ruta;
        usable = bhw * kr_ia_ancho - 2.2; IF (usable < 0.0) usable = 0.0; END
        // trazada de carrera: en una curva entra por fuera, corta por dentro y sale abierto (los habilidosos mas que los demas)
        IF (ruta >= 0) dyaw = kr_ar_yaw[rj11] - kr_ar_yaw[rj3];
        ELSE
            ci3 = (cp_i + 3) % kr_n_cp; ci11 = (cp_i + 11 + vf / 6.0) % kr_n_cp;
            dyaw = kr_cp_yaw[ci11] - kr_cp_yaw[ci3];
        END
        WHILE (dyaw > 180.0)  dyaw = dyaw - 360.0; END
        WHILE (dyaw < -180.0) dyaw = dyaw + 360.0; END
        curva = clamp(dyaw / 35.0, -1.0, 1.0);
        off_ap = -curva * 0.85;
        peso_ap = abs(curva) * (0.35 + 0.5 * skill);
        lx = cos(byaw * 1000); lz = -sin(byaw * 1000);
        tx = bx + lx * kr_mez(off, off_ap, peso_ap) * usable; tz = bz + lz * kr_mez(off, off_ap, peso_ap) * usable;
        // placas de turbo: los que apuntan bien (y los agresivos) se desvian a pisarlas si estan cerca y delante
        tsel = -1; tbest = 1.0e9;
        FOR (tj = 0; tj < kr_n_turbo; tj = tj + 1)
            tcx = (kr_tb_x0[tj] + kr_tb_x1[tj]) * 0.5; tcz = (kr_tb_z0[tj] + kr_tb_z1[tj]) * 0.5;
            tdx = tcx - px; tdz = tcz - pz;
            talong = tdx * sin(yaw * 1000) + tdz * cos(yaw * 1000); tside = tdx * cos(yaw * 1000) - tdz * sin(yaw * 1000);
            IF (talong > 5.0 AND talong < 10.0 + vf * 1.4 AND abs(tside) < 2.0 + talong * 0.35 AND abs(py - kr_tb_y0[tj]) < 4.0 AND talong < tbest
                AND (skill > 0.9 OR agr > 0.6 OR (kid + tj) % 3 != 0))
                tbest = talong; tsel = tj; tx = tcx; tz = tcz;
            END
        END
        dif = atan2(tx - px, tz - pz) / 1000.0 - yaw;
        WHILE (dif > 180.0)  dif = dif - 360.0; END
        WHILE (dif < -180.0) dif = dif + 360.0; END
        a_giro = clamp(-dif / 15.0, -1.0, 1.0);
        // paredes: tres rayos (izquierda, frente, derecha) contra la colision del nivel; se aparta del lado que se acerca
        wlen = 5.0 + vf * 0.2;
        wf = g3d_wall_ray(px, py + 0.7, pz, sin(yaw * 1000), 0.0, cos(yaw * 1000), wlen);
        wl = g3d_wall_ray(px, py + 0.7, pz, sin(yaw * 1000) * 0.87 + cos(yaw * 1000) * 0.5, 0.0, cos(yaw * 1000) * 0.87 - sin(yaw * 1000) * 0.5, wlen);
        wr = g3d_wall_ray(px, py + 0.7, pz, sin(yaw * 1000) * 0.87 - cos(yaw * 1000) * 0.5, 0.0, cos(yaw * 1000) * 0.87 + sin(yaw * 1000) * 0.5, wlen);
        a_giro = clamp(a_giro + (clamp(1.0 - wl / 3.5, 0.0, 1.0) - clamp(1.0 - wr / 3.5, 0.0, 1.0)) * 0.8, -1.0, 1.0);
        IF (ruta >= 0) dif2 = atan2(kr_ar_x[rj14] - px, kr_ar_z[rj14] - pz) / 1000.0 - yaw;           // la curva que viene: soltar el gas o frenar
        ELSE
            ci = (cp_i + 14) % kr_n_cp;
            dif2 = atan2(kr_cp_x[ci] - px, kr_cp_z[ci] - pz) / 1000.0 - yaw;
        END
        WHILE (dif2 > 180.0)  dif2 = dif2 - 360.0; END
        WHILE (dif2 < -180.0) dif2 = dif2 + 360.0; END
        a_acel = 1.0;
        gom = 0.0;                                                    // equilibrio: el rezagado aprieta, el que va muy por delante afloja
        IF (kr_equilibrio > 0.0) gom = clamp((kr_k_prog[0] - kr_k_prog[kid]) / 40.0, -1.0, 1.0) * 0.07 * kr_equilibrio; END
        IF (vf > KR_PUNTA * st_vel * (skill + gom)) a_acel = 0.0; END
        IF (abs(dif2) > 30.0 AND vf > 24.0) a_acel = 0.0; END
        IF (abs(dif2) > 55.0 AND vf > 20.0) a_acel = -0.6; END
        IF (wf < 4.5 AND vf > 14.0) a_acel = clamp(a_acel, -0.7, 0.0); END       // muro de frente: frena
        // derrape con salto: los buenos pisan el salto en las curvas cerradas, derrapan y sueltan al salir (mini-turbo)
        IF (kr_ia_derrapa AND (skill > 0.88 OR agr > 0.65))      // apagado por defecto: en el circuito de prueba los hace mas lentos; kr_ia_derrapa = 1 lo activa
            IF (der AND abs(curva) < 0.4) der = 0; END
            IF (!der AND abs(curva) > 0.85 AND abs(a_giro) > 0.5 AND vf > KR_DER_MIN + 3.0 AND wf > 12.0) der = 1; END
            IF (der) salto = 1; END
        ELSE der = 0; END
        IF (fr_i >= 0 AND agr < 0.6)                                  // prudente: no embiste al de delante
            IF (fr_d < 5.5 AND vf > kr_k_vel[fr_i] + 1.0) a_acel = clamp(a_acel, -0.3, 0.25); END
            IF (fr_d < 3.2) a_acel = clamp(a_acel, -0.5, 0.0); END
        END
    END
    kr_ia_off[kid] = off; kr_ia_off_obj[kid] = off_obj; kr_ia_off_t[kid] = off_t; kr_ia_ruta[kid] = ruta; kr_ia_ruta_j[kid] = ruta_j; kr_ia_rt_hecho[kid] = rt_hecho; kr_ia_bhw[kid] = bhw;
    kr_k_off[kid] = off;
    kr_ia_acel[kid] = a_acel; kr_ia_giro[kid] = a_giro; kr_ia_salto[kid] = salto; kr_ia_der[kid] = der;
END

