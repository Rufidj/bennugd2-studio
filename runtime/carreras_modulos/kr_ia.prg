// =============================================================================
//  MODULO: conductor de los rivales (kr_ia)
//  kr_ia(kid) se llama una vez por fotograma por cada kart con IA (kid 1..7), ANTES de moverlo. Tiene que dejar escrito lo
//  que "pulsaria" ese piloto:
//      kr_ia_acel[kid]   -1..1   (+ acelera, - frena / marcha atras)
//      kr_ia_giro[kid]   -1..1   (+ gira a la derecha)
//      kr_ia_salto[kid]  0 / 1   (saltito / derrape)
//  Puede leer todo el estado de la carrera: los karts (kr_k_x/y/z/yaw/vel/cp/lap/prog/pos..., sus vecinos kr_k_fr_*/kr_k_at_*),
//  el trazado (kr_cp_*, kr_n_cp), las rutas alternativas (kr_rt_*, kr_ar_*) y los datos de su piloto (kr_p_*).
//
//  Dos formas de ponerle tu propio conductor:
//   1) A UN PILOTO: en el editor elige "IA propia" para ese piloto y escribe tu FUNCTION kr_ia_usuario(int kid)
//      (Scripts/carreras_usuario/kr_ia_usuario.prg). Los demas siguen con la IA de serie.
//   2) A TODOS: copia este fichero a Scripts/carreras_usuario/kr_ia.prg y cambialo; tiene que definir FUNCTION kr_ia(int kid).
// =============================================================================
// Despacho: el piloto con "IA propia" usa kr_ia_usuario; los demas, la de serie.
FUNCTION kr_ia(int kid)
BEGIN
    IF (kr_p_ia[kid] == 1) kr_ia_usuario(kid); ELSE kr_ia_defecto(kid); END
END
