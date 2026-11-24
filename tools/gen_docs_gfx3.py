# Documentacion de gfx: media (video/audio Theora), pathfinding, scroll, fundidos, dibujo, texto, colores, ventana.
# Comprobado contra libmod_gfx/m_*.c y libbggfx/g_media.c, g_pathfind.c, g_fade.c.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- MEDIA ----
D("MEDIA_LOAD(SP)","Abre un video (Theora) para reproducirlo. La imagen del video se va dibujando en un grafico de la biblioteca del sistema, cuyo codigo se escribe en la variable que pases por referencia (usa & delante). Todavia no suena: llama a MEDIA_PLAY.",["archivo","&grafico"],"manejador del video, o 0 si no se pudo abrir",
  "video = media_load(\"intro.ogv\", &g);\nif (video) media_play(video); put(0, g, 320, 240); end")
D("MEDIA_LOAD(SPI)","Igual, con un tiempo maximo de espera (timeout) para operaciones de red.",["archivo","&grafico","timeout"],"manejador del video, o 0")
D("MEDIA_UNLOAD","Cierra el video y libera sus recursos.",["video"])
D("MEDIA_PLAY","Empieza la reproduccion. Si estaba parado o terminado, lo reabre desde el principio.",["video"],"0 si va bien, -1 si hay error")
D("MEDIA_PAUSE(P)","Pausa la reproduccion.",["video"])
D("MEDIA_PAUSE(PI)","Pausa (valor distinto de 0) o reanuda (0) la reproduccion.",["video","pausar"])
D("MEDIA_RESUME","Reanuda un video en pausa.",["video"])
D("MEDIA_STOP","Detiene la reproduccion y libera lo que usa para decodificar.",["video"])
D("MEDIA_GET_TIME","Tiempo de reproduccion actual en milisegundos.",["video"],"milisegundos, o -1 si el video no es valido")
D("MEDIA_GET_DURATION","Duracion total del video en milisegundos. En esta version aun no esta implementada y siempre devuelve -1.",["video"],"-1")
D("MEDIA_GET_STATUS","Estado de la reproduccion: -1 = error, 0 = parado, 1 = reproduciendo, 2 = en pausa, 3 = terminado.",["video"],"codigo de estado")
D("MEDIA_GET_MUTE","Indica si el audio del video esta silenciado.",["video"],"1 si esta silenciado, 0 si no")
D("MEDIA_SET_MUTE","Silencia (1) o activa (0) el audio del video.",["video","silenciar"])
D("MEDIA_GET_VOLUME","Volumen actual del video.",["video"],"volumen de 0 a 128")
D("MEDIA_SET_VOLUME","Cambia el volumen del video, de 0 a 128 (fuera de ese rango no hace nada y devuelve -1).",["video","volumen"],"el volumen anterior, o -1")

# ---- PATH: busqueda de caminos ----
D("PATH_NEW","Crea una cuadricula de busqueda de caminos a partir de un grafico: cada pixel NEGRO (color 0 / transparente) es transitable y cualquier otro color es un obstaculo. Cada pixel es una celda.",["archivo","grafico"],"manejador de la cuadricula, o 0 si no pudo",
  "mapa = path_new(0, g_mapa);")
D("PATH_FIND(PIIIII)","Busca el camino mas corto de (x0, y0) a (x1, y1) con el algoritmo A*, en 4 direcciones o en 8 si opciones incluye 1 (PF_DIAG). Devuelve una lista de pares x, y que acaba en el par -1, -1.",["cuadricula","x0","y0","x1","y1","opciones"],"puntero a la lista de puntos (hay que liberarla con PATH_FREE_RESULTS), o 0 si no hay camino",
  "ruta = path_find(mapa, 5, 5, 40, 30, 1);\nif (ruta)\n    i = 0;\n    while (ruta[i] != -1) say(\"paso \" + ruta[i] + \",\" + ruta[i+1]); i += 2; end\n    path_free_results(ruta);\nend")
D("PATH_FIND(PIIIIIII)","Igual, con peso (1 = A* normal; mas alto da prioridad a la heuristica y es mas rapido pero menos exacto) y heuristica: 0 = Manhattan, 1 = euclidea, 2 = octil, 3 = Chebyshev.",["cuadricula","x0","y0","x1","y1","opciones","peso","heuristica"],"puntero a la lista de puntos, o 0 si no hay camino")
D("PATH_FREE_RESULTS","Libera la lista de puntos que devolvio PATH_FIND. Hay que llamarla siempre.",["lista"])
D("PATH_DESTROY","Libera la cuadricula creada con PATH_NEW.",["cuadricula"])

# ---- SCROLL ----
D("SCROLL_START(IIIIIII)","Crea el scroll numero n: un fondo de mapa grande que se desplaza con una camara (n debe ser menor que el maximo de scrolls del motor; fuera de rango no hace nada). Indica el archivo y grafico del fondo, el archivo y grafico del fondo lejano (para el efecto parallax), la region donde se dibuja y las banderas (repeticion horizontal y vertical, etc.). Despues se mueve con las variables de scroll[n] (x0, y0...) y SCROLL_MOVE.",
  ["n","archivo","grafico","archivo_lejano","grafico_lejano","region","banderas"],"",
  "scroll_start(0, fpg, 1, fpg, 2, 0, 3);")
D("SCROLL_START(IIIIII)","Igual, usando el mismo archivo para el fondo y el fondo lejano.",["n","archivo","grafico","grafico_lejano","region","banderas"])
D("SCROLL_START(IIIIIIIII)","Igual que la primera forma, y ademas indica el archivo y el grafico de destino donde se dibuja el scroll (en vez de la pantalla).",["n","archivo","grafico","archivo_lejano","grafico_lejano","region","banderas","archivo_destino","grafico_destino"])
D("SCROLL_STOP","Detiene el scroll numero n y lo quita de la pantalla.",["n"])
D("SCROLL_MOVE","Actualiza el scroll n siguiendo a su proceso camara (ctype = c_scroll).",["n"])

# ---- FADE ----
D("FADE(IIIII)","Funde la pantalla hacia un color: r, g, b y alfa de 0 a 255 (alfa 0 = transparente, 255 = totalmente cubierto) durante los milisegundos indicados. FADE_OFF y FADE_ON son atajos para fundir a negro y volver.",["r","g","b","alfa","milisegundos"],"","fade(0, 0, 0, 255, 1000);   // a negro en 1 s")
D("FADE(IIIIII)","Igual, limitado a una region de la pantalla.",["r","g","b","alfa","milisegundos","region"])

# ---- DIBUJO ----
D("DRAWING_RGBA(BBBB)","Fija el color de los siguientes DRAW_* con sus componentes r, g, b y alfa, de 0 a 255.",["r","g","b","alfa"],"","drawing_rgba(255, 128, 0, 200);")
D("DRAWING_RGBA(IBBBB)","Cambia el color de un dibujo ya creado (el que devolvio un DRAW_*).",["dibujo","r","g","b","alfa"])
D("DRAWING_BLENDMODE(I)","Modo de mezcla de los siguientes DRAW_* (normal, aditivo, etc., con las constantes B_*).",["modo"])
D("DRAWING_BLENDMODE(II)","Cambia el modo de mezcla de un dibujo ya creado.",["dibujo","modo"])
D("DRAWING_BLENDMODE(IIIIII)","Mezcla personalizada para los siguientes DRAW_*: factor origen y destino de color, factor origen y destino de alfa, y ecuaciones de color y de alfa (constantes de OpenGL).",["src_rgb","dst_rgb","src_alfa","dst_alfa","ec_rgb","ec_alfa"])
D("DRAWING_BLENDMODE(IIIIIII)","Igual que la anterior, aplicada a un dibujo ya creado.",["dibujo","src_rgb","dst_rgb","src_alfa","dst_alfa","ec_rgb","ec_alfa"])
D("DRAWING_Z()","Profundidad (z) que usan los siguientes DRAW_*.",[],"z actual")
D("DRAWING_Z(II)","Cambia la profundidad de un dibujo ya creado: menor z se dibuja delante.",["dibujo","z"])
D("DRAW_COORDS","Lee las coordenadas de un dibujo creado: x, y y z se escriben en las variables que pases por referencia (cada una puede ser 0 si no la quieres). OJO: en esta version los valores se recortan a 8 bits (0 a 255).",["dibujo","&x","&y","&z"],"1, o -1 si el dibujo no existe")
D("DRAW_CURVE(IIIIIIIII)","Dibuja una curva de Bezier entre dos extremos con dos puntos de control: (x0, y0) inicio, (x1, y1) y (x2, y2) control, (x3, y3) fin, y nivel de detalle (cuantos segmentos usa).",["x0","y0","x1","y1","x2","y2","x3","y3","nivel"],"identificador del dibujo si no hay DRAWING_MAP activo","draw_curve(20, 200, 80, 40, 160, 40, 220, 200, 30);")
D("DRAW_CURVE(IIIIIIIIII)","Igual, con un color concreto.",["x0","y0","x1","y1","x2","y2","x3","y3","nivel","color"])

# ---- Varios ----
D("TEXTURE_SET_QUALITY","Calidad de filtrado de las texturas al escalar: Q_NEAREST (0, pixelado, por defecto) o Q_LINEAR (suavizado). Afecta a los graficos creados despues.",["calidad"],"","texture_set_quality(Q_LINEAR);")
D("DISPLAY_GET_ORIENTATION","Orientacion de la pantalla donde esta la ventana: 0 = desconocida, 1 = horizontal, 2 = horizontal invertida, 3 = vertical, 4 = vertical invertida (valores de SDL).",[],"codigo de orientacion")
D("WINDOW_SET_DPI_AWARE","En Windows, avisa de que el programa gestiona pantallas de alta densidad para que el sistema no escale la ventana. En otros sistemas no hace nada.")
D("RGB_GET(IIIPPP)","Descompone un color de un grafico en rojo, verde y azul (0 a 255), escritos en las variables por referencia.",["archivo","grafico","color","&r","&g","&b"])
D("RGBA_GET(IIIPPPP)","Descompone un color de un grafico en rojo, verde, azul y alfa (0 a 255), escritos en las variables por referencia.",["archivo","grafico","color","&r","&g","&b","&a"])
D("MAP_CLEAR(IIIIIII)","Rellena de un color solo un rectangulo (x0, y0, x1, y1) de un grafico. Las esquinas se ordenan solas.",["archivo","grafico","x0","y0","x1","y1","color"])
D("TTF_GET_FAMILY","Nombre de la familia de una fuente TTF (por ejemplo \"DejaVu Sans\").",["fuente"],"texto")
D("TTF_GET_STYLE","Estilo de una fuente TTF (por ejemplo \"Bold\").",["fuente"],"texto")
D("GLYPH_GET","Copia el grafico de un caracter de una fuente en un grafico nuevo y devuelve su codigo, para poder modificarlo.",["fuente","caracter"],"codigo del grafico nuevo, o 0 si no existe")
D("GLYPH_SET","Sustituye el grafico de un caracter de una fuente por otro grafico. Si el grafico tiene 3 o mas puntos de control, el segundo marca el desplazamiento del caracter.",["fuente","caracter","archivo","grafico"],"1 si lo cambia, 0 si no")
D("WRITE_IN_MAP(IIIIISI)","Escribe un texto dentro de un grafico (no en pantalla): archivo y grafico de destino, fuente, posicion, texto y alineacion.",["archivo","grafico","fuente","x","y","texto","alineacion"],"1 si lo escribe, 0 si no (texto vacio o grafico invalido)")
D("WRITE_VALUE(IIIIIP)","Escribe en pantalla el valor de una variable y lo mantiene actualizado cada frame: fuente, x, y, alineacion, el codigo de tipo de la variable (el de BennuGD: entero, cadena...) y un puntero a la variable. Para el caso normal es mas comodo WRITE_INT o WRITE_VAR.",["fuente","x","y","alineacion","tipo","&variable"],"identificador del texto")
D("WRITE_VALUE(IIIIIIP)","Igual, indicando tambien la profundidad (z).",["fuente","x","y","z","alineacion","tipo","&variable"],"identificador del texto")
D("WRITE_GET_RGBA","Lee el color de un texto en pantalla (r, g, b y alfa en las variables por referencia).",["texto","&r","&g","&b","&a"])
D("WRITE_SET_RGBA","Cambia el color de un texto en pantalla (r, g, b y alfa de 0 a 255).",["texto","r","g","b","alfa"])
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
