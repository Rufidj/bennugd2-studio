# Documentacion de libmod_gfx, segunda tanda: figuras, regiones, puntos de control, cajas, info de graficos, ventana.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e
OBJ = " Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP)."
COL = "Igual, con un color concreto en vez del de DRAWING_COLOR."

# ---- mas figuras (todas con la forma 'DRAW_X(...)' y la forma con color al final) ----
D("DRAW_ARC","Dibuja el borde de un arco de circulo: centro, radio y angulos de inicio y fin en MILESIMAS DE GRADO."+OBJ,["x","y","radio","angulo_inicio","angulo_fin"],"identificador del dibujo",sig="IIIII")
D("DRAW_ARC",COL,["x","y","radio","angulo_inicio","angulo_fin","color"],"identificador del dibujo",sig="IIIIII")
D("DRAW_ARC_FILLED","Dibuja un arco relleno (un sector de circulo): centro, radio y angulos de inicio y fin en milesimas de grado."+OBJ,["x","y","radio","angulo_inicio","angulo_fin"],"identificador del dibujo",sig="IIIII")
D("DRAW_ARC_FILLED",COL,["x","y","radio","angulo_inicio","angulo_fin","color"],"identificador del dibujo",sig="IIIIII")
D("DRAW_ELLIPSE","Dibuja el borde de una elipse: centro, radios horizontal y vertical, y giro en GRADOS."+OBJ,["x","y","radio_x","radio_y","grados"],"identificador del dibujo",sig="IIIII")
D("DRAW_ELLIPSE",COL,["x","y","radio_x","radio_y","grados","color"],"identificador del dibujo",sig="IIIIII")
D("DRAW_ELLIPSE_FILLED","Dibuja una elipse rellena: centro, radios horizontal y vertical, y giro en grados."+OBJ,["x","y","radio_x","radio_y","grados"],"identificador del dibujo",sig="IIIII")
D("DRAW_ELLIPSE_FILLED",COL,["x","y","radio_x","radio_y","grados","color"],"identificador del dibujo",sig="IIIIII")
D("DRAW_SECTOR","Dibuja el borde de un sector de corona circular: centro, radio interior y exterior, y angulos de inicio y fin en milesimas de grado."+OBJ,["x","y","radio_interior","radio_exterior","angulo_inicio","angulo_fin"],"identificador del dibujo",sig="IIIIII")
D("DRAW_SECTOR",COL,["x","y","radio_interior","radio_exterior","angulo_inicio","angulo_fin","color"],"identificador del dibujo",sig="IIIIIII")
D("DRAW_SECTOR_FILLED","Dibuja un sector de corona circular relleno."+OBJ,["x","y","radio_interior","radio_exterior","angulo_inicio","angulo_fin"],"identificador del dibujo",sig="IIIIII")
D("DRAW_SECTOR_FILLED",COL,["x","y","radio_interior","radio_exterior","angulo_inicio","angulo_fin","color"],"identificador del dibujo",sig="IIIIIII")
D("DRAW_TRIANGLE","Dibuja el borde de un triangulo dado por sus tres vertices (coordenadas absolutas)."+OBJ,["x1","y1","x2","y2","x3","y3"],"identificador del dibujo",sig="IIIIII")
D("DRAW_TRIANGLE",COL,["x1","y1","x2","y2","x3","y3","color"],"identificador del dibujo",sig="IIIIIII")
D("DRAW_TRIANGLE_FILLED","Dibuja un triangulo relleno dado por sus tres vertices (coordenadas absolutas)."+OBJ,["x1","y1","x2","y2","x3","y3"],"identificador del dibujo",sig="IIIIII")
D("DRAW_TRIANGLE_FILLED",COL,["x1","y1","x2","y2","x3","y3","color"],"identificador del dibujo",sig="IIIIIII")
D("DRAW_RECTANGLE_ROUND","Dibuja el borde de un rectangulo con las esquinas redondeadas: posicion, ancho, alto y radio de las esquinas."+OBJ,["x","y","ancho","alto","radio"],"identificador del dibujo",sig="IIIII")
D("DRAW_RECTANGLE_ROUND",COL,["x","y","ancho","alto","radio","color"],"identificador del dibujo",sig="IIIIII")
D("DRAW_RECTANGLE_ROUND_FILLED","Dibuja un rectangulo relleno con las esquinas redondeadas."+OBJ,["x","y","ancho","alto","radio"],"identificador del dibujo",sig="IIIII")
D("DRAW_RECTANGLE_ROUND_FILLED",COL,["x","y","ancho","alto","radio","color"],"identificador del dibujo",sig="IIIIII")
# figuras con listas de puntos: array de decimales (float) con pares x,y
LST = " Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos."
D("DRAW_POINTS","Dibuja varios puntos a la vez."+LST,["cantidad","&puntos"],"identificador del dibujo",sig="IP")
D("DRAW_POINTS",COL,["cantidad","&puntos","color"],"identificador del dibujo",sig="IPI")
D("DRAW_LINES","Dibuja varias lineas a la vez, cada una de un par de puntos."+LST,["cantidad","&puntos"],"identificador del dibujo",sig="IP")
D("DRAW_LINES",COL,["cantidad","&puntos","color"],"identificador del dibujo",sig="IPI")
D("DRAW_RECTANGLES","Dibuja varios rectangulos (borde) a la vez."+LST,["cantidad","&rectangulos"],"identificador del dibujo",sig="IP")
D("DRAW_RECTANGLES",COL,["cantidad","&rectangulos","color"],"identificador del dibujo",sig="IPI")
D("DRAW_RECTANGLES_FILLED","Dibuja varios rectangulos rellenos a la vez."+LST,["cantidad","&rectangulos"],"identificador del dibujo",sig="IP")
D("DRAW_RECTANGLES_FILLED",COL,["cantidad","&rectangulos","color"],"identificador del dibujo",sig="IPI")
D("DRAW_POLYGON","Dibuja el borde de un poligono cerrado."+LST,["cantidad","&puntos"],"identificador del dibujo",sig="IP")
D("DRAW_POLYGON",COL,["cantidad","&puntos","color"],"identificador del dibujo",sig="IPI")
D("DRAW_POLYGON_FILLED","Dibuja un poligono relleno."+LST,["cantidad","&puntos"],"identificador del dibujo",sig="IP")
D("DRAW_POLYGON_FILLED",COL,["cantidad","&puntos","color"],"identificador del dibujo",sig="IPI")
D("DRAW_POLYLINE","Dibuja una linea quebrada que une los puntos. Con 'cerrar' en 1 une tambien el ultimo con el primero."+LST,["cantidad","&puntos","cerrar"],"identificador del dibujo",sig="IPI")
D("DRAW_POLYLINE",COL,["cantidad","&puntos","cerrar","color"],"identificador del dibujo",sig="IPII")
D("DRAW_SET_THICKNESS","Grosor de linea de los siguientes dibujos, en pixeles (decimal).",["grosor"],sig="F")
D("DRAW_SET_THICKNESS","Cambia el grosor de linea de un dibujo ya creado.",["dibujo","grosor"],sig="IF")
D("DRAW_GET_THICKNESS","Grosor de linea que se usa para los siguientes dibujos.",ret="grosor",sig="")
D("DRAW_GET_THICKNESS","Grosor de linea de un dibujo ya creado.",["dibujo"],"grosor",sig="I")

# ---- regiones, pantalla y ventana ----
D("REGION_DEFINE","Define una region rectangular de la pantalla (una zona a la que se puede limitar el dibujo de los procesos con su variable region). La region 0 es la pantalla entera.",["region","x","y","ancho","alto"],"el numero de region",
  "region_define(1, 0, 0, 320, 200);   // los procesos con region = 1 solo se ven en ese rectangulo")
D("REGION_OUT","Indica si un proceso (su grafico entero) esta fuera de una region.",["proceso","region"],"1 si esta fuera")
D("SCREEN_GET","Captura la pantalla en un grafico nuevo y devuelve su codigo (0 si falla). Sirve para guardar una captura con MAP_SAVE.",ret="codigo del grafico",ejemplo="g = screen_get();\nmap_save(0, g, \"captura.png\");")
D("WINDOW_GET_SIZE","Lee el tamano actual de la ventana, su tamano maximo y su tamano minimo en seis variables que se pasan por puntero (cada pareja ancho, alto).",["&ancho","&alto","&ancho_max","&alto_max","&ancho_min","&alto_min"])

# ---- informacion y propiedades de un grafico ----
D("MAP_DEL","Libera un grafico de memoria (igual que MAP_UNLOAD).",["archivo","grafico"])
D("FPG_DEL","Libera una coleccion de graficos (igual que FPG_UNLOAD).",["archivo"])
D("MAP_NAME","Nombre de un grafico (el que traia el fichero o el que se le puso con MAP_SET_NAME).",["archivo","grafico"],"el nombre")
D("MAP_SET_NAME","Pone nombre a un grafico.",["archivo","grafico","nombre"])
D("MAP_INFO","Pide una propiedad de un grafico con una constante: G_WIDTH (ancho), G_HEIGHT (alto), G_DEPTH (profundidad de color), G_CENTER_X y G_CENTER_Y. Con archivo 0 y grafico 0 pregunta por la pantalla.",["archivo","grafico","propiedad"],"el valor",
  "ancho = map_info(0, graph, G_WIDTH);",sig="III")
D("GRAPHIC_INFO","Alias de MAP_INFO de tres parametros: propiedad de un grafico (G_WIDTH, G_HEIGHT, G_CENTER_X, G_CENTER_Y...).",["archivo","grafico","propiedad"],"el valor")
D("MAP_INFO_SET","Cambia una propiedad de un grafico: G_CENTER_X o G_CENTER_Y (el punto del grafico que se coloca en la posicion del proceso).",["archivo","grafico","propiedad","valor"],sig="IIII")
D("MAP_INFO","Cambia una propiedad de un grafico (G_CENTER_X o G_CENTER_Y).",["archivo","grafico","propiedad","valor"],sig="IIII")
D("GRAPHIC_SET","Alias de MAP_INFO_SET: cambia G_CENTER_X o G_CENTER_Y de un grafico.",["archivo","grafico","propiedad","valor"])

# ---- puntos de control ----
D("CENTER_SET","Fija el centro de un grafico: el punto del grafico que queda en la posicion del proceso al dibujarlo (el punto de control 0). Por defecto es el centro; con los pies del personaje pegados al suelo, ponlo abajo.",["archivo","grafico","x","y"],"1 si va bien, -1 si el grafico no existe",
  "center_set(0, graph, ancho / 2, alto);   // ancla en los pies")
D("POINT_SET","Fija un punto de control de un grafico (de 0 a 999), por ejemplo donde sale un disparo o donde se agarra un arma.",["archivo","grafico","punto","x","y"],"1 si va bien, -1 si no")
D("POINT_GET","Lee un punto de control de un grafico en dos variables que se pasan por puntero. El punto 0 sin definir da el centro del grafico. Devuelve 0 si el punto no existe.",["archivo","grafico","punto","&x","&y"],"1 si existe, 0 si no")
D("POINT_GET_TOTAL","Cuantos puntos de control tiene un grafico.",["archivo","grafico"],"cantidad")

# ---- cajas de colision ----
D("BOX_SET","Define una caja de colision de un grafico (la forma que usa COLLISION): un numero de caja, el tipo de forma, y su posicion (x, y) y tamano (ancho, alto) relativos al grafico. Si la caja ya existe, la sustituye.",["archivo","grafico","caja","forma","x","y","ancho","alto"],"1 si va bien, 0 si el grafico no existe",sig="IIIIIIII")
D("BOX_SET","Igual, para el grafico del proceso que la llama.",["caja","forma","x","y","ancho","alto"],"1 si va bien, 0 si no tiene grafico",sig="IIIIII")
D("BOX_GET","Lee una caja de colision de un grafico: su forma, posicion y tamano, en variables que se pasan por puntero.",["archivo","grafico","caja","&forma","&x","&y","&ancho","&alto"],"1 si existe",sig="IIIPPPPP")
D("BOX_GET","Igual, para el grafico del proceso que la llama.",["caja","&forma","&x","&y","&ancho","&alto"],"1 si existe",sig="IPPPPP")
D("BOX_REMOVE","Quita una caja de colision de un grafico.",["archivo","grafico","caja"],sig="III")
D("BOX_REMOVE","Quita una caja de colision del grafico del proceso que la llama.",["caja"],sig="I")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
