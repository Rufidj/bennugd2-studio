# Documentacion de telas (CLOTH) y cuerdas (ROPE). Comprobado contra libmod_3d_cloth/rope.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

SIM = " Es una simulacion de Verlet: llama a la funcion update cada frame para que se mueva."
D("G3D_CLOTH_CREATE","Crea una tela (bandera, lona, cortina): una rejilla de nx x ny particulas unidas por distancias, de ancho x alto unidades, con la esquina superior izquierda en (x, y, z). Nace plana en el plano XY y cuelga hacia -Y. Crea su propia entidad en la escena activa, con textura, luz y sombra como cualquier objeto." + SIM + " Mas particulas = mas realista pero mas lento.",
  ["ancho","alto","nx","ny","x","y","z"],"id de la tela, o -1",
  "tela = g3d_cloth_create(4.0, 3.0, 20, 15, 0, 6, 0);\ng3d_cloth_pin(tela, 0);          // sujeta el borde de arriba\ng3d_cloth_set_wind(tela, 1, 0, 0.3, 4.0);\n// cada frame:\ng3d_cloth_update(tela, 1.0 / 60);")
D("G3D_CLOTH_CREATE_ORIENTED","Igual que g3d_cloth_create, pero el ancho de la tela sigue la direccion (dirx, dirz) en el plano XZ en vez de ir siempre por +X. Sirve para colgar un estandarte pegado a una pared que no mira al eje X.",
  ["ancho","alto","nx","ny","x","y","z","dirx","dirz"],"id de la tela, o -1")
D("G3D_CLOTH_PIN","Elige como se sujeta la tela: 0 = todo el borde superior (cortina, estandarte), 1 = solo las dos esquinas de arriba (lona), 2 = el borde izquierdo (bandera en un mastil).",["tela","modo"])
D("G3D_CLOTH_PIN_MOVE","Sujeta y coloca UNA particula de la tela (columna i, fila j de la rejilla, desde 0) en un punto del mundo. Llamala cada frame para colgar la tela de algo que se mueve, por ejemplo un punto de una cuerda (g3d_rope_x/y/z).",["tela","i","j","x","y","z"])
D("G3D_CLOTH_SET_WIND","Viento sobre la tela: direccion (x, y, z) y fuerza.",["tela","x","y","z","fuerza"])
D("G3D_CLOTH_SET_COLLIDER","Esfera que la tela no puede atravesar (centro y radio). Muevela cada frame para que siga a un personaje o un objeto.",["tela","x","y","z","radio"])
D("G3D_CLOTH_CLEAR_COLLIDER","Quita la esfera de colision de la tela.",["tela"])
D("G3D_CLOTH_SET_TEXTURE","Pone una textura (de g3d_load_texture) a la tela.",["tela","textura"],"","g3d_cloth_set_texture(tela, g3d_load_texture(\"Assets/bandera.png\"));")
D("G3D_CLOTH_UPDATE","Avanza la simulacion de la tela dt segundos y actualiza su malla. Una vez por frame.",["tela","dt"])
D("G3D_CLOTH_PUSH","Empujon global: aparta TODAS las telas de una esfera. Lo llama lo que deba apartarlas (el jugador, un barril, un coche) una vez por frame, con su posicion y su grosor; si deja de llamarse, la tela se recupera sola. Devuelve cuantas particulas de tela esta apartando, para poder frenar al atravesar una lona pesada.",
  ["x","y","z","radio"],"particulas apartadas (0 = aire libre)","tocando = g3d_cloth_push(px, py + 1, pz, 0.5);\nif (tocando > 0) vel = vel * 0.5; end")
D("G3D_CLOTH_PUSH_CAPSULE","Como g3d_cloth_push pero con una capsula (del punto a al b, con radio): la forma real de una persona, un barril o un coche, para que lo que sobresale no atraviese la tela.",["ax","ay","az","bx","by","bz","radio"],"particulas apartadas (0 = aire libre)")

D("G3D_ROPE_CREATE","Crea una cuerda entre dos puntos: una fila de segmentos particulas dibujada como un tubo (se ve desde cualquier angulo) de radio grosor. holgura es cuanto mas larga es que la distancia entre los extremos: 0 = tirante, 0.3 = cuelga bastante. Nace con los dos extremos sujetos. Sirve para cables, lianas, puentes y para colgar telas." + SIM,
  ["ax","ay","az","bx","by","bz","segmentos","grosor","holgura"],"id de la cuerda, o -1",
  "cuerda = g3d_rope_create(-3, 4, 0,  3, 4, 0,  16, 0.05, 0.25);\n// cada frame:\ng3d_rope_update(cuerda, 1.0 / 60);")
D("G3D_ROPE_PIN","Fija (1) o suelta (0) un extremo de la cuerda: extremo 0 = el primero, 1 = el ultimo.",["cuerda","extremo","fijo"],"","g3d_rope_pin(cuerda, 1, 0);   // suelta el extremo final")
D("G3D_ROPE_PIN_MOVE","Clava un punto cualquiera de la cuerda (0 = el primero) en una posicion: un gancho que se mueve. Llamala cada frame.",["cuerda","punto","x","y","z"])
D("G3D_ROPE_SET_TEXTURE","Textura del tubo de la cuerda. OJO: este parametro es el identificador de textura de OpenGL, no el manejador de g3d_load_texture.",["cuerda","textura_gl"])
D("G3D_ROPE_SET_WIND","Viento sobre la cuerda: direccion (x, y, z) y fuerza.",["cuerda","x","y","z","fuerza"])
D("G3D_ROPE_LOAD","Cuelga peso de un punto de la cuerda: se comba ahi. Hay que repetirlo CADA FRAME mientras siga colgado; si se deja de llamar, la cuerda se recupera sola.",["cuerda","punto","peso"])
D("G3D_ROPE_UPDATE","Avanza la simulacion de la cuerda dt segundos. Una vez por frame.",["cuerda","dt"])
D("G3D_ROPE_POINTS","Numero de puntos de la cuerda (los validos para los demas comandos van de 0 a ese numero menos 1).",["cuerda"],"numero de puntos")
D("G3D_ROPE_X","Coordenada X actual del punto n de la cuerda, para colgar cosas de el.",["cuerda","punto"],"X","")
D("G3D_ROPE_Y","Coordenada Y actual del punto n de la cuerda.",["cuerda","punto"],"Y")
D("G3D_ROPE_Z","Coordenada Z actual del punto n de la cuerda.",["cuerda","punto"],"Z",
  "mitad = g3d_rope_points(cuerda) / 2;\ng3d_cloth_pin_move(tela, 0, 0, g3d_rope_x(cuerda, mitad), g3d_rope_y(cuerda, mitad), g3d_rope_z(cuerda, mitad));")
D("G3D_ROPE_DESTROY","Elimina la cuerda.",["cuerda"])

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
