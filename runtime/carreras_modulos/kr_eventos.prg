// =============================================================================
//  MODULO: eventos de la carrera (kr_eventos)
//  El motor llama a estas funciones cuando pasa algo. Por defecto no hacen nada: copia este fichero a
//  Scripts/carreras_usuario/kr_eventos.prg y rellena las que quieras (sonidos, puntos, mensajes, logros...). Conservalas TODAS
//  con el mismo nombre y parametros.
//  kid = numero de kart (0 = el jugador, 1..7 = rivales); los motivos de reaparicion son:
//  1 caida, 2 agua, 3 el jugador pulso R, 4 fuera de pista, 5 atascado, 6 fuera del mapa.
// =============================================================================
FUNCTION kr_ev_inicio()                          BEGIN END                         // el circuito ya esta montado, antes de la cuenta atras
FUNCTION kr_ev_vuelta(int kid, int vuelta, int ticks)   BEGIN END                  // un kart acaba una vuelta (ticks = duracion, 60 por segundo)
FUNCTION kr_ev_meta(int kid, int puesto, int ticks)     BEGIN END                  // un kart cruza la meta de la ultima vuelta
FUNCTION kr_ev_reaparece(int kid, int motivo)           BEGIN END                  // un kart vuelve al ultimo checkpoint
FUNCTION kr_ev_turbo(int kid)                           BEGIN END                  // un kart pisa una placa de turbo
FUNCTION kr_ev_trampolin(int kid)                       BEGIN END                  // un kart sale lanzado de un trampolin
FUNCTION kr_ev_derrape(int kid)                         BEGIN END                  // un kart empieza a derrapar
