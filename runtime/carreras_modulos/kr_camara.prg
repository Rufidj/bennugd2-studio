// =============================================================================
//  MODULO: camara del juego de carreras (kr_camara)
//  Un PROCESS que sigue al kart del jugador cada fotograma. Para sustituirla por la tuya, crea
//  Scripts/carreras_usuario/kr_camara.prg con tu PROCESS kr_camara() (mismo nombre, sin parametros).
//  Lee: kr_kart_x/y/z, kr_kart_yaw, kr_kart_vel, kr_kart_turbo (el jugador), kr_camera (la camara del motor), KR_CAM_*.
// =============================================================================
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

