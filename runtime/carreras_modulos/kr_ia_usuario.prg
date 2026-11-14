// =============================================================================
//  MODULO: conductor propio (kr_ia_usuario) - para los pilotos con "IA propia"
//  Por defecto hace lo mismo que la IA de serie. Copia este fichero a Scripts/carreras_usuario/kr_ia_usuario.prg y escribe tu
//  conducta; mira kr_ia.prg para ver que puedes leer y que tienes que dejar escrito (kr_ia_acel/giro/salto[kid]).
// =============================================================================
FUNCTION kr_ia_usuario(int kid)
BEGIN
    kr_ia_defecto(kid);
END
