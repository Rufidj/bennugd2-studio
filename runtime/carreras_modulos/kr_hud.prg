// =============================================================================
//  MODULO: HUD del jugador (kr_hud_actualizar)
//  Se llama cada fotograma. Tiene que dejar escrito el texto de las dos lineas de arriba: kr_hud (velocidad, derrape, turbo,
//  superficie) y kr_hud2 (vuelta, tiempo, puesto, hora). Para hacer el tuyo, crea Scripts/carreras_usuario/kr_hud.prg con tu
//  FUNCTION kr_hud_actualizar() (sin parametros). Lee: kr_kart_vel, kr_k_derrape/carga/turbo/surf[0], kr_lap, kr_vueltas,
//  kr_t_race, kr_best, kr_k_pos[0], kr_n_k, kr_hora, kr_amb_modo.
// =============================================================================
FUNCTION kr_hud_actualizar()
PRIVATE
    int kmh; int hh; int mm;
BEGIN
        kmh = kr_kart_vel * 3.6;
        kr_hud = "" + kmh + " km/h";
        IF (kr_k_derrape[0])
            IF (kr_k_carga[0] >= KR_CARGA2)     kr_hud = kr_hud + "   DERRAPE **";
            ELSEIF (kr_k_carga[0] >= KR_CARGA1) kr_hud = kr_hud + "   DERRAPE *";
            ELSE                         kr_hud = kr_hud + "   DERRAPE"; END
        END
        IF (kr_k_turbo[0] > 0) kr_hud = kr_hud + "   TURBO!"; END
        IF (kr_k_surf[0] == KR_S_TIERRA) kr_hud = kr_hud + "   [tierra]"; END
        IF (kr_k_surf[0] == KR_S_HIELO)  kr_hud = kr_hud + "   [hielo]"; END
        IF (kr_k_surf[0] == KR_S_BARRO)  kr_hud = kr_hud + "   [barro]"; END
        IF (kr_k_surf[0] == KR_S_FUERA)  kr_hud = kr_hud + "   [fuera de pista]"; END
        IF (kr_k_surf[0] == KR_S_AGUA)   kr_hud = kr_hud + "   [agua]"; END
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
        IF (kr_n_k > 1 AND kr_n_cp > 4) kr_hud2 = kr_hud2 + "   Puesto " + (kr_k_pos[0] + 1) + "/" + kr_n_k; END
END
