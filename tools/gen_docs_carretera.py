# Documentacion de las carreteras (libmod_3d_road.c) y del signo de la direccion del vehiculo.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

D("G3D_ROAD_CREATE","Crea una carretera vacia. Una carretera es SOLO informacion de por donde se puede circular: una lista de nodos unidos por curvas suaves. NO se dibuja ni cambia el suelo: el aspecto es el de tu terreno pintado o tu modelo (.glb), y el suelo el de tu terreno o tu colision. Todo lo demas (salida, meta, vueltas, rivales) lo hace el juego. Maximo 8 carreteras. Las que dibujas en el editor con la herramienta Carretera ya vienen creadas (id 0, 1...) al empezar tu codigo.",["cerrada"],"id de la carretera, o -1 si no quedan huecos",
  "carretera = g3d_road_create(1);   // 1 = circuito cerrado, 0 = abierta\ng3d_road_add_node(carretera, 0, 0, 0, 12, 0, 0, 0, 0);\ng3d_road_add_node(carretera, 60, 0, 0, 12, 0, 0, 0, 0);\ng3d_road_add_node(carretera, 60, 60, 0, 12, 0, 0, 0, 0);\ng3d_road_build(carretera);")
D("G3D_ROAD_DESTROY","Destruye una carretera.",["carretera"])
D("G3D_ROAD_CLEAR","Destruye todas las carreteras. g3d_scene_load tambien las borra antes de cargar las de la escena.")
D("G3D_ROAD_ADD_NODE","Anade un nodo a la carretera. Las curvas entre nodos son suaves (tangentes Catmull-Rom). Ancho, peralte, altura y dy se interpolan hasta el nodo siguiente. Con sigue=1 la carretera va pegada al terreno (altura = terreno + dy) y 'y' se ignora; con sigue=0 va a la altura fija 'y'. La superficie es una etiqueta del tramo que SALE de este nodo: 0 asfalto, 1 tierra, 2 hielo, 3 barro, o lo que decida tu juego. Un peralte positivo sube el borde derecho (curvas a la izquierda). Maximo 512 nodos por carretera.",
  ["carretera","x","z","y","ancho","peralte_grados","sigue_terreno","dy","superficie"],"indice del nodo, o -1")
D("G3D_ROAD_SET_TANGENT","Tangente propia de un nodo: la direccion y la 'fuerza' con la que la curva pasa por el. (0, 0) = automatica (la normal: la curva sale de los nodos vecinos). Sirve para curvas muy cerradas, o para reproducir un trazado dibujado en el editor, que guarda la tangente de los nodos que el usuario ha arrastrado. Hay que llamarla antes de G3D_ROAD_BUILD.",["carretera","nodo","tx","tz"])
D("G3D_ROAD_BUILD","Calcula la linea central de la carretera. Hay que llamarla despues de anadir los nodos y, si algun nodo sigue el terreno, DESPUES de cargar el terreno. g3d_scene_load ya la llama por ti para las carreteras de la escena. Una carretera cerrada necesita al menos 3 nodos; una abierta, 2.",["carretera"],"numero de muestras de la linea central (unas cada 2 m), 0 si no hay trazado valido")
D("G3D_ROAD_COUNT","Numero de carreteras ya calculadas.",ret="cantidad")
D("G3D_ROAD_NODES","Numero de nodos de una carretera.",["carretera"],"cantidad")
D("G3D_ROAD_CLOSED","Indica si la carretera es un circuito cerrado.",["carretera"],"1 si es cerrada")
D("G3D_ROAD_LENGTH","Longitud de la linea central de la carretera, en metros (unidades de mundo), contando la vuelta entera si es cerrada.",["carretera"],"longitud")
D("G3D_ROAD_X","Posicion X del punto de la linea central situado a 'distancia' metros del principio. En una carretera cerrada la distancia da la vuelta (y admite negativos); en una abierta se recorta a los extremos.",["carretera","distancia"],"X")
D("G3D_ROAD_Y","Altura del punto de la carretera a 'distancia' metros del principio (ver G3D_ROAD_X).",["carretera","distancia"],"Y")
D("G3D_ROAD_Z","Posicion Z del punto de la carretera a 'distancia' metros del principio (ver G3D_ROAD_X).",["carretera","distancia"],"Z")
D("G3D_ROAD_YAW","Hacia donde mira la carretera en ese punto, como rumbo en MILESIMAS DE GRADO: 0 mira hacia +Z y crece hacia +X (adelante = (sin rumbo, cos rumbo)), igual que el rumbo de un vehiculo. Vista desde una camara detras del coche (mirando a +Z), un rumbo creciente es un giro a la izquierda.",["carretera","distancia"],"rumbo")
D("G3D_ROAD_WIDTH","Ancho de la carretera en ese punto, en metros.",["carretera","distancia"],"ancho")
D("G3D_ROAD_SURFACE","Etiqueta de superficie del tramo en ese punto (la que se puso en G3D_ROAD_ADD_NODE).",["carretera","distancia"],"superficie")
D("G3D_ROAD_NEAREST","Busca el punto de carretera mas cercano a (x, z), de entre todas las carreteras, y guarda el resultado para las funciones G3D_ROAD_NEAR_*. Es la base de casi todo: en que vuelta va un coche, quien va primero, hacia donde apunta un rival o si se ha salido de la pista.",["x","z"],"id de la carretera mas cercana, o -1 si no hay ninguna",
  "g3d_road_nearest(g3d_vehicle_x(v), g3d_vehicle_z(v));\nIF (NOT g3d_road_near_on())\n    say(\"Fuera de la pista\");\nEND")
D("G3D_ROAD_NEAR_DIST","De la ultima G3D_ROAD_NEAREST: distancia recorrida a lo largo de la carretera del punto mas cercano. Con ella se sabe en que vuelta va un coche (salta de casi 'largo' a casi 0 al cruzar la meta) o quien va por delante.",ret="distancia en metros")
D("G3D_ROAD_NEAR_OFFSET","De la ultima G3D_ROAD_NEAREST: separacion lateral al centro de la carretera. Positiva hacia la izquierda de quien avanza por ella (hacia +X cuando el rumbo es 0), negativa hacia la derecha.",ret="metros")
D("G3D_ROAD_NEAR_Y","De la ultima G3D_ROAD_NEAREST: altura de la carretera en ese punto.",ret="altura")
D("G3D_ROAD_NEAR_WIDTH","De la ultima G3D_ROAD_NEAREST: ancho de la carretera en ese punto.",ret="ancho en metros")
D("G3D_ROAD_NEAR_SURFACE","De la ultima G3D_ROAD_NEAREST: etiqueta de superficie del tramo.",ret="superficie")
D("G3D_ROAD_NEAR_ON","De la ultima G3D_ROAD_NEAREST: indica si el punto esta dentro del ancho de la carretera (separacion lateral menor o igual que la mitad del ancho).",ret="1 si esta en la carretera")
D("G3D_ROAD_NEAR_YAW","De la ultima G3D_ROAD_NEAREST: rumbo de la carretera en ese punto, en MILESIMAS DE GRADO (ver G3D_ROAD_YAW).",ret="rumbo")

D("G3D_ROAD_SHOW","OPCIONAL: dibuja la carretera, creando su malla (una por superficie) como entidades de la escena. Normalmente NO se usa: tu carretera ya esta pintada en el terreno o es un modelo. Sirve para ver una carretera inventada por codigo (como en las guias) o para depurar. No la vuelve suelo: para eso G3D_ROAD_SET_GROUND. Hay que llamarla despues de G3D_ROAD_BUILD. Cada superficie tiene un color de serie (asfalto gris oscuro, tierra marron, hielo azul claro, barro marron oscuro).",["carretera","escena"],"numero de entidades creadas (una por superficie usada)",
  "g3d_road_build(carretera);\ng3d_road_show(carretera, escena);")
D("G3D_ROAD_SET_GROUND","Hace que la carretera cuente como SUELO (para el vehiculo, el personaje y las colisiones, via G3D_SCENE_TERRAIN_HEIGHT): dentro de su ancho, la altura es la de la carretera, con su peralte. Por defecto NO lo es (0): una carretera marcada en el editor solo dice por donde se puede circular y el suelo es el de tu terreno o tu modelo. Util para carreteras inventadas por codigo junto a G3D_ROAD_SHOW. Los cuerpos rigidos de Jolt no la notan.",["carretera","activar"])
D("G3D_ROAD_HIDE","Quita el dibujo de la carretera (sigue existiendo y siendo suelo).",["carretera"])
D("G3D_ROAD_ENTITY","Entidad con la que se dibuja una superficie de la carretera. Sirve para cambiar su aspecto con las funciones de siempre: g3d_entity_set_material, g3d_material_set_texture, g3d_entity_set_color... La textura se repite cada 8 m a lo largo de la carretera.",["carretera","superficie"],"id de la entidad, o -1 si la carretera no tiene esa superficie",
  "// una textura de asfalto en la superficie 0\nm = g3d_material_create();\ng3d_material_set_texture(m, 0, g3d_load_texture(\"asfalto.png\"));\ng3d_entity_set_material(g3d_road_entity(carretera, 0), m);")
D("G3D_ROAD_SET_COLOR","Cambia el color de serie de una superficie (de 0 a 7), con componentes de 0.0 a 1.0. Afecta tambien a las carreteras ya dibujadas.",["superficie","r","g","b"])

# signo de la direccion del vehiculo (comprobado: con direccion +1 el coche se va hacia +X)
d["G3D_VEHICLE_UPDATE"]["desc"] = ("Avanza el vehiculo. acelerador de -1 a 1 (negativo = marcha atras), direccion de -1 a 1 y freno de 0 a 1 (los valores fuera de rango se recortan). "
  "Una direccion POSITIVA hace crecer el rumbo, es decir, gira hacia +X: a la izquierda de la pantalla si la camara va detras mirando a +Z. "
  "Con la velocidad la direccion pierde hasta un 40 % de autoridad. Solo acelera con las ruedas en el suelo. dt se limita a 0.1 s.")
# aviso en las funciones de entidad: un proceso C_3D las pisa cada frame (comprobado con la escena en pantalla)
AV = " OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables ({v}); usala solo en entidades sin proceso."
for n, v in [("SET_POSITION","x, y, z"),("SET_ROTATION","angle_x/y/z"),("SET_SCALE","size, size_x/y/z"),("SET_COLOR","color_r/g/b"),("SET_ALPHA","alpha"),("SET_BLEND","blendmode")]:
    k = "G3D_ENTITY_" + n
    if k in d and "OJO" not in d[k]["desc"]: d[k]["desc"] += AV.format(v=v)
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
