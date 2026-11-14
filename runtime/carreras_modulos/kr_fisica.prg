// =============================================================================
//  MODULO: fisica / manejo del kart (kr_fisica)
//  Un paso de conduccion de UN kart (kid): aceleracion, freno, giro, derrape, turbo, agarre segun la superficie y empujones entre
//  karts. Lee lo que el piloto "pulsa" (kr_f_acel/giro/salto) y el estado (kr_f_*), y deja la velocidad nueva (kr_f_vx/vz) y el
//  rumbo (kr_f_yaw). NO mueve al kart: despues el motor lo desplaza con la colision del mundo. Estadisticas del kart: kr_p_*[kid]
//  (vel, acel, giro, agarre, peso); constantes de ajuste: KR_*.
//  Copialo a Scripts/carreras_usuario/kr_fisica.prg para cambiar el manejo (coches, motos, naves...). Conserva el nombre.
// =============================================================================
FUNCTION kr_fisica(int kid)
PRIVATE
    int   ch; float yaw; float vx; float vz; float px; float py; float pz;
    float fx; float fz; float rx; float rz; float vf; float vl; float gv; float k; float punta;
    int   suelo; int salto_ant; int derrape; int dir; float carga; int turbo_t; int surf; int slip;
    float a_acel; float a_giro; int a_salto;
    float sgrip; float spunta; float sacel; int zi;
    float st_vel; float st_acel; float st_giro; float st_agarre; float st_peso;
    int k2; float ddx; float ddz; float dist2; float dd; float otro;
BEGIN
    ch = kr_f_ch[kid]; yaw = kr_f_yaw[kid]; vx = kr_f_vx[kid]; vz = kr_f_vz[kid]; px = kr_f_px[kid]; py = kr_f_py[kid]; pz = kr_f_pz[kid];
    suelo = kr_f_suelo[kid]; salto_ant = kr_f_salto_ant[kid]; derrape = kr_f_der[kid]; dir = kr_f_dir[kid]; carga = kr_f_carga[kid];
    turbo_t = kr_f_turbo[kid]; surf = kr_f_surf[kid]; slip = kr_f_slip[kid];
    a_acel = kr_f_acel[kid]; a_giro = kr_f_giro[kid]; a_salto = kr_f_salto[kid];
    st_vel = kr_p_vel[kid]; st_acel = kr_p_acel[kid]; st_giro = kr_p_giro[kid]; st_agarre = kr_p_agarre[kid]; st_peso = kr_p_peso[kid];

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

    punta = KR_PUNTA * spunta * st_vel;
    IF (slip) punta = punta * 1.06; END
    IF (turbo_t > 0)
        punta = KR_PUNTA * st_vel * KR_TURBO_PUNTA;
        IF (vf < punta) vf = vf + KR_TURBO_ACEL * KR_DT; END
        turbo_t = turbo_t - 1;
    END
    IF (a_acel > 0.0)
        IF (vf < punta) vf = vf + KR_ACEL * sacel * st_acel * a_acel * KR_DT; END
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
        gv = KR_GIRO * st_giro * a_giro * clamp(abs(vf) / 10.0, 0.0, 1.0);
        IF (vf < 0.0) gv = -gv; END
        IF (suelo) yaw = yaw - gv * KR_DT; ELSE yaw = yaw - gv * KR_AIRE * KR_DT; END
    END

    IF (suelo)
        IF (derrape) k = KR_DER_AGARRE * KR_DT; ELSE k = KR_AGARRE * sgrip * st_agarre * KR_DT; END
        vl = vl - vl * clamp(k, 0.0, 1.0);
    END
    vx = fx * vf + rx * vl;
    vz = fz * vf + rz * vl;
    FOR (k2 = 0; k2 < kr_n_k; k2 = k2 + 1)                  // choques entre karts: se apartan (empujon suave, sin atravesarse)
        IF (k2 != kid)
            ddx = px - kr_k_x[k2]; ddz = pz - kr_k_z[k2]; dist2 = ddx * ddx + ddz * ddz;
            IF (dist2 < 3.6 AND abs(py - kr_k_y[k2]) < 1.6)
                dd = sqrt(dist2); IF (dd < 0.05) dd = 0.05; END
                otro = kr_p_peso[k2] / (kr_p_peso[k2] + st_peso) * 2.0;           // el mas pesado empuja mas y se deja empujar menos
                vx = vx + ddx / dd * (1.9 - dd) * 14.0 * otro;
                vz = vz + ddz / dd * (1.9 - dd) * 14.0 * otro;
            END
        END
    END


    kr_f_yaw[kid] = yaw; kr_f_vx[kid] = vx; kr_f_vz[kid] = vz; kr_f_vf[kid] = vf; kr_f_suelo[kid] = suelo;
    kr_f_der[kid] = derrape; kr_f_carga[kid] = carga; kr_f_turbo[kid] = turbo_t;
END
