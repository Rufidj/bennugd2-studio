# Documentacion de libmod_gfx (lo esencial). Verificado contra libmod_gfx/m_*.c y libbggfx.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e

# ---- movimiento y geometria (solo dentro de un PROCESS) ----
D("ADVANCE","Mueve el proceso 'distancia' en la direccion de su variable angle. Los angulos de BennuGD van en MILESIMAS DE GRADO (90000 = 90 grados) y la Y de pantalla crece hacia abajo, asi que con angle = 90000 el proceso sube.",["distancia"],"",
  "angle = 45000;\nadvance(4);   // avanza 4 pixeles en diagonal hacia arriba a la derecha")
D("XADVANCE","Como ADVANCE pero con el angulo dado, sin tocar la variable angle.",["angulo","distancia"])
D("GET_ANGLE","Angulo (en milesimas de grado) desde este proceso hasta otro, indicado por su identificador.",["proceso"],"angulo",sig="I")
D("GET_ANGLE","Angulo (en milesimas de grado) desde el primer proceso hasta el segundo.",["proceso_a","proceso_b"],"angulo",sig="II")
D("GET_DIST","Distancia en pixeles desde este proceso hasta otro, indicado por su identificador.",["proceso"],"distancia",sig="I")
D("GET_DIST","Distancia en pixeles entre dos procesos.",["proceso_a","proceso_b"],"distancia",sig="II")
D("COLLISION","Comprueba si este proceso choca con otros. Se le pasa el tipo de proceso (TYPE nombre), el identificador de uno concreto, o -1 para el raton. La colision se calcula con la forma real del grafico de cada proceso.",["objetivo"],"identificador del proceso con el que choca, o 0",
  "IF (collision(TYPE bala)) vida--; END",sig="I")
D("COLLISION","Igual, pero para el proceso indicado en vez de para este.",["proceso","objetivo"],"identificador del proceso con el que choca, o 0",sig="II")

# ---- color ----
D("RGB","Crea un color con rojo, verde y azul (0 a 255 cada uno), opaco. El numero que devuelve se usa en DRAWING_COLOR, MAP_CLEAR, MAP_PUT_PIXEL...",["r","g","b"],"el color",
  "color = rgb(255, 128, 0);   // naranja",sig="BBB")
D("RGB","Alias que acepta un mapa (se ignora) y devuelve el mismo color.",["archivo","grafico","r","g","b"],"el color",sig="IIBBB")
D("RGBA","Crea un color con rojo, verde, azul y alfa (0 a 255 cada uno; alfa 255 = opaco).",["r","g","b","alfa"],"el color",sig="BBBB")
D("RGBA","Alias que acepta un mapa (se ignora) y devuelve el mismo color.",["archivo","grafico","r","g","b","alfa"],"el color",sig="IIBBBB")
D("RGB_GET","Separa un color en rojo, verde y azul, en tres variables que se pasan por puntero.",["color","&r","&g","&b"],"",sig="IPPP")
D("RGBA_GET","Separa un color en rojo, verde, azul y alfa, en cuatro variables que se pasan por puntero.",["color","&r","&g","&b","&alfa"],"",sig="IPPPP")

# ---- pantalla y ventana ----
D("SET_MODE","Cambia el tamano de la pantalla (o de la ventana) en pixeles.",["ancho","alto"],ret="",ejemplo="set_mode(1280, 720);",sig="II")
D("SET_MODE","Cambia el tamano de la pantalla con banderas de modo.",["ancho","alto","flags"],sig="III")
D("SET_FPS","Fotogramas por segundo del juego. El segundo valor es cuantos fotogramas se pueden saltar al dibujar si el juego va lento (0 = ninguno).",["fps","saltos"],ret="los fps fijados",ejemplo="set_fps(60, 0);")
D("WINDOW_SET_TITLE","Pone el titulo de la ventana.",["titulo"])
D("WINDOW_SET_ICON","Pone el icono de la ventana a partir de un grafico.",["archivo","grafico"])
D("WINDOW_MOVE","Mueve la ventana a una posicion del escritorio.",["x","y"])
D("WINDOW_SET_POS","Alias de WINDOW_MOVE.",["x","y"])
D("WINDOW_GET_POS","Lee la posicion de la ventana en dos variables que se pasan por puntero.",["&x","&y"])
D("DESKTOP_GET_SIZE","Lee el tamano del escritorio en dos variables que se pasan por puntero.",["&ancho","&alto"])
for n,t in [("HIDE","Oculta"),("SHOW","Muestra"),("MINIMIZE","Minimiza"),("MAXIMIZE","Maximiza"),("RAISE","Trae al frente"),("RESTORE","Restaura")]:
    D("WINDOW_"+n,t+" la ventana.")
D("FADE","Funde la pantalla hacia un color. Los cuatro primeros valores son el color de destino (rojo, verde, azul y su alfa/intensidad) y 'velocidad' es lo rapido que se hace.",["r","g","b","alfa","velocidad"],"",sig="IIIII")
D("FADE_OFF","Funde la pantalla a negro con la velocidad dada (el alfa de destino es 255).",["velocidad"],sig="I")
D("FADE_ON","Quita el fundido: la pantalla vuelve a verse (alfa 0) con la velocidad dada.",["velocidad"],sig="I")
D("FADE_OFF","Funde a negro solo la region indicada.",["velocidad","region"],sig="II")
D("FADE_ON","Quita el fundido solo en la region indicada.",["velocidad","region"],sig="II")

# ---- texto ----
D("WRITE","Escribe un texto en pantalla y devuelve su identificador. El texto se queda hasta que lo borras con WRITE_DELETE (o hasta que acaba el programa), no hay que volver a escribirlo cada frame. La alineacion es una de ALIGN_TOP_LEFT, ALIGN_TOP, ALIGN_TOP_RIGHT, ALIGN_CENTER_LEFT, ALIGN_CENTER, ALIGN_CENTER_RIGHT, ALIGN_BOTTOM_LEFT, ALIGN_BOTTOM, ALIGN_BOTTOM_RIGHT. La fuente es un identificador de FNT_LOAD o TTF_LOAD (0 = la fuente del sistema).",["fuente","x","y","alineacion","texto"],"identificador del texto",
  "txt = write(0, 10, 10, ALIGN_TOP_LEFT, \"Hola\");\n...\nwrite_delete(txt);",sig="IIIIS")
D("WRITE","Igual, indicando tambien la profundidad (z): menor z se dibuja delante.",["fuente","x","y","z","alineacion","texto"],"identificador del texto",sig="IIIIIS")
D("WRITE_VAR","Escribe el valor de una variable y lo mantiene actualizado solo: si la variable cambia, el texto cambia. Se pasa la propia variable.",["fuente","x","y","alineacion","variable"],"identificador del texto",
  "write_var(0, 10, 30, ALIGN_TOP_LEFT, puntos);",sig="IIIIV++")
D("WRITE_VAR","Igual, indicando tambien la profundidad (z).",["fuente","x","y","z","alineacion","variable"],"identificador del texto",sig="IIIIIV++")
D("WRITE_MOVE","Mueve un texto ya escrito.",["texto","x","y"],sig="III")
D("WRITE_MOVE","Mueve un texto ya escrito, cambiando tambien su profundidad.",["texto","x","y","z"],sig="IIII")
D("WRITE_DELETE","Borra un texto escrito con WRITE o WRITE_VAR (0 borra todos).",["texto"])
D("WRITE_IN_MAP","Dibuja un texto dentro de un grafico nuevo y devuelve su codigo, en vez de ponerlo en pantalla.",["fuente","texto","alineacion"],"codigo del grafico",sig="ISI")
D("TEXT_WIDTH","Ancho en pixeles que ocupa un texto con una fuente.",["fuente","texto"],"ancho")
D("TEXT_HEIGHT","Alto en pixeles que ocupa un texto con una fuente.",["fuente","texto"],"alto")
D("FNT_LOAD","Carga una fuente de mapa de bits (.fnt) desde un fichero y devuelve su identificador (0 si falla). Los ficheros .fnt de BennuGD pueden venir comprimidos con gzip.",["fichero"],"identificador de la fuente",sig="S")
D("FNT_LOAD","Carga una fuente .fnt EN SEGUNDO PLANO: el identificador se guarda en la variable que se pasa por puntero cuando termina.",["fichero","&identificador"],"",sig="SP")
D("FNT_UNLOAD","Libera una fuente.",["fuente"])
D("TTF_LOAD","Carga una fuente TrueType (.ttf) y devuelve su identificador. Se usa igual que una fuente .fnt en WRITE. Ajusta su tamano con TTF_SET_SIZE.",["fichero"],"identificador de la fuente")
D("TTF_SET_SIZE","Tamano de una fuente TrueType, en puntos.",["fuente","tamano"])

# ---- graficos (mapas) ----
# Concepto: un grafico se identifica por (archivo, grafico). El archivo 0 es la biblioteca del sistema (donde van MAP_NEW y MAP_LOAD).
D("MAP_NEW","Crea un grafico vacio del tamano indicado en la biblioteca del sistema (archivo 0) y devuelve su codigo (0 si falla).",["ancho","alto"],"codigo del grafico",
  "g = map_new(32, 32);\nmap_clear(0, g, rgb(255, 0, 0));   // lo rellena de rojo",sig="II")
D("MAP_NEW","Igual, y con la bandera B_CLEAR ademas lo deja limpio.",["ancho","alto","flags"],"codigo del grafico",sig="III")
D("MAP_LOAD","Carga una imagen (PNG, JPG...) como grafico en la biblioteca del sistema y devuelve su codigo (0 si falla).",["fichero"],"codigo del grafico",
  "graph = map_load(\"Assets/Sprites/heroe.png\");",sig="S")
D("MAP_LOAD","Carga una imagen EN SEGUNDO PLANO: el codigo se guarda en la variable que se pasa por puntero cuando termina.",["fichero","&codigo"],"",sig="SP")
D("MAP_SAVE","Guarda un grafico en un fichero de imagen. Devuelve -1 si el grafico no existe.",["archivo","grafico","fichero"],"0 si va bien, -1 si no")
D("MAP_CLONE","Crea una copia de un grafico y devuelve el codigo de la copia.",["archivo","grafico"],"codigo de la copia")
D("MAP_EXISTS","Indica si existe un grafico.",["archivo","grafico"],"1 si existe")
D("MAP_UNLOAD","Libera un grafico de memoria.",["archivo","grafico"])
D("MAP_CLEAR","Limpia un grafico (lo deja transparente).",["archivo","grafico"],sig="II")
D("MAP_CLEAR","Rellena un grafico entero de un color.",["archivo","grafico","color"],sig="III")
D("MAP_CLEAR","Rellena de un color solo un rectangulo (x0, y0, x1, y1).",["archivo","grafico","x0","y0","x1","y1"],sig="IIIIII")
D("MAP_GET_PIXEL","Color de un pixel de un grafico (-1 si el grafico no existe).",["archivo","grafico","x","y"],"el color")
D("MAP_PUT_PIXEL","Pone un pixel de un color en un grafico.",["archivo","grafico","x","y","color"])
D("MAP_PUT","Copia un grafico (origen) sobre otro (destino) en la posicion (x, y).",["archivo_destino","grafico_destino","archivo_origen","grafico_origen","x","y"],"",sig="IIIIII")
D("MAP_PUT","Copia un grafico sobre otro con todas las opciones: angulo (milesimas de grado), escala en porcentaje (100 = tamano normal), banderas (B_HMIRROR, B_VMIRROR, B_TRANSLUCENT...), tinte (r, g, b) y alfa (0 a 255).",
  ["archivo_destino","grafico_destino","archivo_origen","grafico_origen","x","y","angulo","escala_x","escala_y","flags","r","g","b","alfa"],"",sig="IIIIIIIIIIBBBB")
D("MAP_PUT","Igual, con modo de mezcla (BLEND_NORMAL, BLEND_ADD, BLEND_MULTIPLY, BLEND_SUBTRACT...).",
  ["archivo_destino","grafico_destino","archivo_origen","grafico_origen","x","y","angulo","escala_x","escala_y","flags","r","g","b","alfa","mezcla"],"",sig="IIIIIIIIIIBBBBI")
D("FPG_LOAD","Carga una coleccion de graficos (.fpg) y devuelve su identificador de archivo (0 si falla). Cada grafico se pide luego con (archivo, codigo): las variables file y graph de un proceso. Los .fpg de BennuGD pueden venir comprimidos con gzip.",["fichero"],"identificador del archivo",
  "file = fpg_load(\"Assets/heroe.fpg\");\ngraph = 1;",sig="S")
D("FPG_LOAD","Carga un .fpg EN SEGUNDO PLANO: el identificador se guarda en la variable que se pasa por puntero cuando termina.",["fichero","&archivo"],"",sig="SP")
D("FPG_NEW","Crea una coleccion de graficos vacia y devuelve su identificador.",ret="identificador del archivo")
D("FPG_ADD","Anade a una coleccion una copia de un grafico, con el codigo indicado.",["archivo","codigo","archivo_origen","grafico_origen"])
D("FPG_EXISTS","Indica si existe una coleccion de graficos.",["archivo"],"1 si existe")
D("FPG_UNLOAD","Libera una coleccion de graficos.",["archivo"])

# ---- dibujo ----
# Concepto: los DRAW_* crean OBJETOS de dibujo que se quedan en pantalla hasta que se borran con DRAW_DELETE (no hay que repintarlos
# cada frame). Si se ha elegido un grafico con DRAWING_MAP, dibujan directamente en ese grafico (y ya no son objetos).
D("DRAWING_COLOR","Color con el que se dibujaran los siguientes DRAW_*.",["color"],sig="I")
D("DRAWING_COLOR","Cambia el color de un dibujo ya creado.",["dibujo","color"],sig="II")
D("DRAWING_Z","Profundidad (z) de los siguientes dibujos: menor z se dibuja delante.",["z"],sig="I")
D("DRAWING_MAP","Hace que los siguientes DRAW_* dibujen dentro de un grafico en vez de en pantalla. Con (0, 0), que no es un grafico, se vuelve a dibujar en pantalla.",["archivo","grafico"])
D("DRAW_DELETE","Borra un dibujo creado con un DRAW_* (0 borra todos).",["dibujo"])
D("DRAW_MOVE","Mueve un dibujo ya creado.",["dibujo","x","y"])
D("DRAW_POINT","Dibuja un punto con el color de DRAWING_COLOR.",["x","y"],"identificador del dibujo",sig="II")
D("DRAW_POINT","Dibuja un punto de un color concreto.",["x","y","color"],"identificador del dibujo",sig="III")
D("DRAW_LINE","Dibuja una linea con el color de DRAWING_COLOR.",["x0","y0","x1","y1"],"identificador del dibujo",
  "drawing_color(rgb(255, 255, 0));\nlin = draw_line(10, 10, 200, 100);",sig="IIII")
D("DRAW_LINE","Dibuja una linea de un color concreto.",["x0","y0","x1","y1","color"],"identificador del dibujo",sig="IIIII")
D("DRAW_RECTANGLE","Dibuja el borde de un rectangulo (x, y, ancho, alto) con el color de DRAWING_COLOR.",["x","y","ancho","alto"],"identificador del dibujo",sig="IIII")
D("DRAW_RECTANGLE","Dibuja el borde de un rectangulo de un color concreto.",["x","y","ancho","alto","color"],"identificador del dibujo",sig="IIIII")
D("DRAW_RECTANGLE_FILLED","Dibuja un rectangulo relleno (x, y, ancho, alto) con el color de DRAWING_COLOR.",["x","y","ancho","alto"],"identificador del dibujo",sig="IIII")
D("DRAW_RECTANGLE_FILLED","Dibuja un rectangulo relleno de un color concreto.",["x","y","ancho","alto","color"],"identificador del dibujo",sig="IIIII")
D("DRAW_CIRCLE","Dibuja el borde de un circulo (centro y radio) con el color de DRAWING_COLOR.",["x","y","radio"],"identificador del dibujo",sig="III")
D("DRAW_CIRCLE","Dibuja el borde de un circulo de un color concreto.",["x","y","radio","color"],"identificador del dibujo",sig="IIII")
D("DRAW_CIRCLE_FILLED","Dibuja un circulo relleno (centro y radio) con el color de DRAWING_COLOR.",["x","y","radio"],"identificador del dibujo",sig="III")
D("DRAW_CIRCLE_FILLED","Dibuja un circulo relleno de un color concreto.",["x","y","radio","color"],"identificador del dibujo",sig="IIII")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
