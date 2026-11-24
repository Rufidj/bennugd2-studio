# Documentacion de gfx: MAP_PUT/BLOCK_COPY, paletas, fuentes (FNT_NEW), cajas de colision y shaders.
# Comprobado contra libmod_gfx/m_map.c, m_fnt.c, m_mathgfx.c, m_shaders.c y libbggfx/g_shaders.h.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

PUT = "dibuja un grafico sobre otro grafico (no en pantalla)"
D("MAP_PUT(IIIIIIIIIIBBBBIIIIIII)","Como MAP_PUT con color, pero con una mezcla (blend) personalizada: factor origen y destino de color, factor origen y destino de alfa y ecuaciones de color y de alfa (constantes de OpenGL).",
  ["archivo_destino","grafico_destino","archivo_origen","grafico_origen","x","y","angulo","escala_x","escala_y","banderas","r","g","b","alfa","modo","src_rgb","dst_rgb","src_alfa","dst_alfa","ec_rgb","ec_alfa"],"0 si algun grafico no existe")
BC = "Copia un rectangulo (x, y, ancho, alto) de un grafico origen sobre otro grafico destino en (x_dest, y_dest), aplicando las banderas (espejo, etc.)."
D("MAP_BLOCK_COPY(IIIIIIIIIII)",BC,["archivo_dest","grafico_dest","x_dest","y_dest","archivo_orig","grafico_orig","x","y","ancho","alto","banderas"],"0 si algun grafico no existe",
  "map_block_copy(0, destino, 10, 10, 0, hoja, 32, 0, 32, 32, 0);")
D("MAP_BLOCK_COPY(IIIIIIIIII)","Igual, pero origen y destino estan en el mismo archivo (el primero): archivo, grafico_dest, x_dest, y_dest, grafico_orig, x, y, ancho, alto, banderas.",["archivo","grafico_dest","x_dest","y_dest","grafico_orig","x","y","ancho","alto","banderas"],"0 si algun grafico no existe")
D("MAP_BLOCK_COPY(IIIIIIIIIIIBBBB)",BC + " Con tinte r, g, b y alfa (0 a 255).",["archivo_dest","grafico_dest","x_dest","y_dest","archivo_orig","grafico_orig","x","y","ancho","alto","banderas","r","g","b","alfa"],"0 si algun grafico no existe")
D("MAP_BLOCK_COPY(IIIIIIIIIIIBBBBI)",BC + " Con tinte y modo de mezcla.",["archivo_dest","grafico_dest","x_dest","y_dest","archivo_orig","grafico_orig","x","y","ancho","alto","banderas","r","g","b","alfa","modo"],"0 si algun grafico no existe")
D("MAP_BLOCK_COPY(IIIIIIIIIIIBBBBIIIIIII)",BC + " Con tinte y mezcla personalizada (src_rgb, dst_rgb, src_alfa, dst_alfa, ec_rgb, ec_alfa).",["archivo_dest","grafico_dest","x_dest","y_dest","archivo_orig","grafico_orig","x","y","ancho","alto","banderas","r","g","b","alfa","modo","src_rgb","dst_rgb","src_alfa","dst_alfa","ec_rgb","ec_alfa"],"0 si algun grafico no existe")
D("MAP_SET_PALETTE","Cambia colores de la paleta de un grafico de 8 bits: desde el color primero, n colores tomados de un array de estructuras con r, g, b, a (4 bytes por color). Solo vale en graficos de 8 bits.",["archivo","grafico","primero","n","&colores"],"0 si va bien, -1 si no hay superficie, -2 si no es de 8 bits")
D("MAP_GET_PALETTE","Copia en un array n colores de la paleta de un grafico de 8 bits, desde el color primero (4 bytes por color: r, g, b, a).",["archivo","grafico","primero","n","&colores"],"0 si va bien, -1 si no hay superficie, -2 si no es de 8 bits")

D("GET_REAL_POINT(IPP)","Posicion en pantalla del punto de control numero n del grafico de este proceso, teniendo en cuenta su giro, tamano y espejo. Se escribe en dos variables de tipo double (por referencia). Si el punto no esta definido devuelve la posicion del proceso.",["punto","&x","&y"],"1 si va bien, 0 si el punto no existe")
D("GET_REAL_POINT(IIPP)","Igual, para el proceso con ese identificador.",["proceso","punto","&x","&y"],"1 si va bien, 0 si no")
D("GET_REAL_POINT(IDDPP)","Convierte un punto (x, y) en coordenadas del grafico del proceso dado a su posicion real en pantalla, con su giro, tamano y espejo.",["proceso","x","y","&x_real","&y_real"],"1 si va bien, 0 si no")

BX = "Calcula las 4 esquinas reales en pantalla de una caja de colision (con el giro, tamano y espejo del proceso): se escriben 8 valores double (x e y de cada esquina, en orden: izquierda-arriba, derecha-arriba, derecha-abajo, izquierda-abajo del grafico sin girar) en el array que se pasa por referencia, y el radio en el ultimo parametro (para cajas circulares)."
D("BOX_GET_REAL_VERTEX(IPP)",BX + " Usa el grafico del proceso actual; la caja es el codigo dado (-1 = la caja propia del proceso).",["caja","&vertices","&radio"],"1 si va bien, 0 si no")
D("BOX_GET_REAL_VERTEX(IIIPP)",BX + " Usa el grafico indicado (archivo y grafico), con la posicion y giro del proceso actual.",["archivo","grafico","caja","&vertices","&radio"],"1 si va bien, 0 si no")
D("BOX_GET_REAL_VERTEX(IIIIPP)",BX + " Como la anterior, para otro proceso (el primer parametro).",["proceso","archivo","grafico","caja","&vertices","&radio"],"1 si va bien, 0 si no")
D("BOX_GET_BY_POS(IIIPPPPPP)","Lee los datos de la caja de colision numero n (por orden de posicion) de un grafico: codigo, forma, x, y, ancho y alto, escritos en las variables por referencia (cada una puede ser 0 si no te interesa).",["archivo","grafico","posicion","&codigo","&forma","&x","&y","&ancho","&alto"],"1 si existe, 0 si no")
D("BOX_GET_BY_POS(IPPPPPP)","Igual, para el grafico del proceso actual.",["posicion","&codigo","&forma","&x","&y","&ancho","&alto"],"1 si existe, 0 si no")

FN = "Crea una fuente a partir de un grafico con todas las letras en una rejilla de celdas del mismo tamano."
D("FNT_NEW()","Crea una fuente vacia (juego de caracteres ISO 8859). Despues se rellena letra a letra con GLYPH_SET.",[],"codigo de la fuente")
D("FNT_NEW(I)","Crea una fuente vacia con el juego de caracteres indicado (0 = ISO 8859, 1 = CP850, 2 = UTF-8).",["charset"],"codigo de la fuente")
D("FNT_NEW(IIIIIIII)",FN + " Indica el grafico, el juego de caracteres, el ancho y alto de cada celda, el primer y ultimo caracter que contiene y las opciones.",["archivo","grafico","charset","ancho","alto","primero","ultimo","opciones"],"codigo de la fuente, o -1 si el grafico no existe",
  "fuente = fnt_new(0, g_letras, 0, 8, 8, 32, 127, 0);")
D("FNT_NEW(IIIIIIIIS)",FN + " Con una cadena que da el orden real de los caracteres de la rejilla.",["archivo","grafico","charset","ancho","alto","primero","ultimo","opciones","orden"],"codigo de la fuente, o -1")
D("FNT_NEW(IIIIIIIIIIII)",FN + " Solo se lee la region (x, y, ancho, alto) del grafico; un ancho o alto de 0 llega hasta el borde.",["archivo","grafico","reg_x","reg_y","reg_ancho","reg_alto","charset","ancho","alto","primero","ultimo","opciones"],"codigo de la fuente, o -1")
D("FNT_NEW(IIIIIIIIIIIIS)","Como la anterior, con una cadena que da el orden de los caracteres.",["archivo","grafico","reg_x","reg_y","reg_ancho","reg_alto","charset","ancho","alto","primero","ultimo","opciones","orden"],"codigo de la fuente, o -1")
D("FNT_NEW(IIIIIIIIIIIII)","Como FNT_NEW(IIIIIIII), pudiendo fijar el tamano real de la celda, los margenes (izquierdo y superior) dentro del grafico y el espaciado entre letras.",["archivo","grafico","charset","ancho","alto","primero","ultimo","opciones","ancho_celda","alto_celda","margen_izq","margen_sup","espaciado"],"codigo de la fuente, o -1")
D("FNT_NEW(IIIIIIIISIIIII)","Rejilla con orden de caracteres, tamano de celda, margenes y espaciado.",["archivo","grafico","charset","ancho","alto","primero","ultimo","opciones","orden","ancho_celda","alto_celda","margen_izq","margen_sup","espaciado"],"codigo de la fuente, o -1")
D("FNT_NEW(IIIIIIIIIIIIIIIII)","Rejilla leida solo dentro de una region del grafico (x, y, ancho, alto; 0 = hasta el borde), con tamano de celda, margenes y espaciado.",["archivo","grafico","reg_x","reg_y","reg_ancho","reg_alto","charset","ancho","alto","primero","ultimo","opciones","ancho_celda","alto_celda","margen_izq","margen_sup","espaciado"],"codigo de la fuente, o -1")
D("FNT_NEW(IIIIIIIIIIIISIIIII)","Igual, con la cadena que da el orden de los caracteres.",["archivo","grafico","reg_x","reg_y","reg_ancho","reg_alto","charset","ancho","alto","primero","ultimo","opciones","orden","ancho_celda","alto_celda","margen_izq","margen_sup","espaciado"],"codigo de la fuente, o -1")

SH = " Los shaders se escriben en GLSL; solo hay soporte cuando el motor grafico lo permite (con SDL_gpu)."
D("SHADER_GET_LANGUAGE","Lenguaje de shaders que soporta el motor grafico (constantes SHADER_LANGUAGE_*: GLSL, GLSL ES, HLSL...). -1 si no hay soporte.",[],"codigo del lenguaje")
D("SHADER_GET_MIN_VERSION","Version minima de shader soportada, o -1 si no hay soporte.",[],"version")
D("SHADER_GET_MAX_VERSION","Version maxima de shader soportada, o -1 si no hay soporte.",[],"version")
D("SHADER_CREATE","Compila un shader a partir del texto del vertex shader y del fragment shader (cadenas con el codigo, no nombres de archivo)." + SH,["vertice","fragmento"],"manejador del shader, o 0 si falla la compilacion")
D("SHADER_FREE","Libera un shader.",["shader"])
D("SHADER_GETATTRIBUTELOCATION","Reservada: ahora mismo no esta disponible (la implementacion esta desactivada). Para los uniformes usa SHADER_GET_PARAM_LOCATION.",["shader","nombre"],"")
D("SHADER_GET_PARAM_LOCATION","Posicion de una variable uniform del shader por su nombre; hace falta para fijarle valor con SHADER_SET_PARAM.",["shader","nombre"],"posicion, o -1 si no existe")
D("SHADER_CREATE_PARAMS","Crea un conjunto de valores para los uniforms con espacio para n parametros. Se asocia a un grafico o proceso para que el shader los reciba.",["n"],"manejador del conjunto de parametros")
D("SHADER_FREE_PARAMS","Libera un conjunto de parametros.",["parametros"])
D("SHADER_SET_PARAM(PIII)","Fija un uniform entero: el tipo debe ser UNIFORM_INT o UNIFORM_UINT.",["parametros","tipo","posicion","valor"],"0 si va bien, -1 si no")
D("SHADER_SET_PARAM(PIIF)","Fija un uniform decimal (tipo UNIFORM_FLOAT).",["parametros","tipo","posicion","valor"],"0 si va bien, -1 si no")
D("SHADER_SET_PARAM(PIIIII)","Pasa una imagen al shader (tipo SHADER_IMAGE): el grafico (archivo y grafico) y la unidad de textura que usa.",["parametros","tipo","posicion","archivo","grafico","unidad"],"0 si va bien, -1 si no")
D("SHADER_SET_PARAM(PIIIP)","Fija un uniform de tipo array o vector (UNIFORM_INT_ARRAY, UNIFORM_FLOAT2_ARRAY, UNIFORM_FLOAT3_ARRAY, UNIFORM_FLOAT4_ARRAY, etc.): n valores tomados de la memoria apuntada.",["parametros","tipo","posicion","n","&valores"],"0 si va bien, -1 si no")
D("SHADER_SET_PARAM(PIIPIIII)","Fija un uniform de matriz (tipo UNIFORM_MATRIX): puntero a los valores, numero de matrices, filas y columnas, y si hay que transponerlas (1) o no (0).",["parametros","tipo","posicion","&valores","n_matrices","filas","columnas","transponer"],"0 si va bien, -1 si no")
D("SHADER_GET_PARAM(PIPP)","Lee un uniform entero o decimal del conjunto de parametros: escribe su tipo y un puntero al valor.",["parametros","posicion","&tipo","&valor"],"0 si va bien, -1 si no")
D("SHADER_GET_PARAM(PIPPP)","Lee un uniform de tipo vector o array: escribe el tipo, el numero de valores y un puntero a ellos.",["parametros","posicion","&tipo","&n","&valores"],"0 si va bien, -1 si no")
D("SHADER_GET_PARAM(PIPPPPPP)","Lee un uniform de matriz: escribe el tipo, un puntero a los valores, el numero de matrices, las filas, las columnas y si estan transpuestas.",["parametros","posicion","&tipo","&valores","&n_matrices","&filas","&columnas","&transponer"],"0 si va bien, -1 si no")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
