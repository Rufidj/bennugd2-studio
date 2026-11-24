# Genera/actualiza tools/api_docs.json con la documentacion verificada del nucleo.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8")) if os.path.exists(R) else {}
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- escena ----
D("G3D_SCENE_CREATE","Crea una escena vacia.",["nombre"],"id de la escena")
D("G3D_SCENE_DESTROY","Destruye una escena.",["escena"])
D("G3D_SCENE_SET_ACTIVE","Hace activa una escena: es la que se renderiza.",["escena"])
D("G3D_SCENE_LOAD","Carga un mapa .lvl (terreno, luces, entidades, camara), crea la escena y la deja activa.",["fichero"],"id de la escena, o -1 si el fichero no existe",
  'escena = g3d_scene_load("mapa.lvl");')
D("G3D_SCENE_TERRAIN_HEIGHT","Altura del terreno de la escena cargada en el punto (x, z). Sobre terreno de voxeles usa la superficie real esculpida.",["x","z"],"altura en Y; 0 si no hay terreno")
D("G3D_SCENE_WATER_LEVEL","Altura Y del agua de la escena cargada.",ret="nivel del agua")
D("G3D_SCENE_HAS_SPAWN","Indica si el mapa cargado define un punto de inicio del jugador.",ret="1 si existe")
D("G3D_SCENE_SPAWN_X","Coordenada X del punto de inicio del jugador.",ret="X")
D("G3D_SCENE_SPAWN_Y","Coordenada Y del punto de inicio del jugador.",ret="Y")
D("G3D_SCENE_SPAWN_Z","Coordenada Z del punto de inicio del jugador.",ret="Z")
D("G3D_SCENE_PLAYER_RADIUS","Radio de colision del jugador definido en el mapa.",ret="radio")
D("G3D_SCENE_PLAYER_HEIGHT","Altura del jugador definida en el mapa.",ret="altura")
D("G3D_SCENE_PLAYER_CLIMB","Altura maxima de escalon que puede subir el jugador, definida en el mapa.",ret="altura de escalon")

# ---- camara ----
D("G3D_CAMERA_CREATE","Crea una camara.",ret="id de la camara")
D("G3D_CAMERA_SET_ACTIVE","Hace activa una camara: es la que se usa para renderizar.",["camara"])
D("G3D_CAMERA_SET_POSITION","Coloca la camara en el mundo.",["camara","x","y","z"])
D("G3D_CAMERA_LOOK_AT","Orienta la camara hacia un punto. (ux,uy,uz) es el vector que apunta hacia arriba, normalmente (0,1,0).",
  ["camara","objetivo_x","objetivo_y","objetivo_z","arriba_x","arriba_y","arriba_z"],
  ejemplo="g3d_camera_set_position(cam, 0, 5, -10);\ng3d_camera_look_at(cam, 0, 1, 0, 0, 1, 0);")
D("G3D_CAMERA_SET_FOV","Campo de vision vertical de la camara, en GRADOS (por defecto 75). Fija aspecto 16:9, near 0.1 y far 1000.",["camara","fov_grados"])
D("G3D_CAMERA_SET_PROJECTION","OBSOLETA: no hace nada. Usa G3D_CAMERA_SET_FOV.",["camara","tipo"])
D("G3D_CAMERA_SAFE_DISTANCE","Para camaras en tercera persona: dado el punto objetivo (t), la direccion hacia donde esta la camara (d), la distancia deseada y un radio, devuelve la distancia a la que se puede poner la camara sin atravesar cajas de colision ni terreno.",
  ["objetivo_x","objetivo_y","objetivo_z","dir_x","dir_y","dir_z","distancia","radio"],"distancia segura (<= distancia)")

# ---- luces ----
D("G3D_LIGHT_CREATE","Crea una luz. Tipo: G3D_LIGHT_DIRECTIONAL (0), G3D_LIGHT_POINT (1) o G3D_LIGHT_SPOT (2). Maximo 32 luces. La variante de 1 parametro crea la luz en blanco.",
  ["tipo","r","g","b"],"id de la luz, o -1 si se llego al maximo",
  "sol = g3d_light_create(G3D_LIGHT_DIRECTIONAL, 1.0, 0.95, 0.8);\ng3d_light_set_direction(sol, -0.4, -1.0, -0.3);")
D("G3D_LIGHT_SET_POSITION","Posicion de una luz puntual o foco.",["luz","x","y","z"])
D("G3D_LIGHT_SET_DIRECTION","Direccion hacia la que apunta una luz direccional o un foco.",["luz","x","y","z"])
D("G3D_LIGHT_SET_INTENSITY","Intensidad de la luz. Los valores negativos se tratan como 0.",["luz","intensidad"])
D("G3D_LIGHT_SET_COLOR","Color de la luz, componentes de 0.0 a 1.0.",["luz","r","g","b"])
D("G3D_LIGHT_SET_RANGE","Alcance de una luz puntual o foco.",["luz","alcance"])
D("G3D_LIGHT_SET_CONE","Angulo del cono de un foco, en grados.",["luz","grados"])
D("G3D_LIGHT_ENABLE_SHADOW","Activa (1) o desactiva (0) las sombras de una luz.",["luz","activar"])
D("G3D_LIGHT_SET_SHADOW_QUALITY","Resolucion del mapa de sombras. Afecta tambien a la del renderer (equivale a G3D_SET_SHADOW_RESOLUTION).",["luz","resolucion"])

# ---- entidades ----
D("G3D_ENTITY_SPAWN","Crea una entidad con un modelo en una escena.",["escena","modelo","x","y","z"],"id de la entidad")
D("G3D_ENTITY_DESTROY","Destruye una entidad.",["entidad"])
D("G3D_ENTITY_SET_POSITION","Posicion de la entidad en el mundo.",["entidad","x","y","z"])
D("G3D_ENTITY_SET_ROTATION","Rotacion de la entidad. Los angulos van en MILESIMAS DE GRADO, como los de BennuGD (90000 = 90 grados).",["entidad","pitch","yaw","roll"],
  ejemplo="g3d_entity_set_rotation(e, 0, 90000, 0);   // girada 90 grados sobre Y")
D("G3D_ENTITY_SET_SCALE","Escala de la entidad en cada eje (1.0 = tamano original).",["entidad","sx","sy","sz"])
D("G3D_ENTITY_GET_POSITION","Lee la posicion de la entidad en tres variables float que se pasan por puntero.",["entidad","&x","&y","&z"])
D("G3D_ENTITY_GET_ROTATION","Lee la rotacion de la entidad en tres variables que se pasan por puntero.",["entidad","&pitch","&yaw","&roll"])
D("G3D_ENTITY_SET_PARENT","Hace que una entidad cuelgue de otra (hereda su transformacion).",["entidad","padre"])
D("G3D_ENTITY_SET_COLLIDER","Marca (1) o desmarca (0) la entidad como solida para la colision.",["entidad","solido"])
D("G3D_ENTITY_SET_MESH","Cambia la malla de la entidad.",["entidad","malla"])
D("G3D_ENTITY_SET_MATERIAL","Asigna un material a la entidad.",["entidad","material"])
D("G3D_ENTITY_SET_ALPHA","Opacidad con la convencion de BennuGD: 0 (invisible) a 255 (opaco).",["entidad","alfa"])
D("G3D_ENTITY_SET_COLOR","Tinte de color, de 0 a 255 por canal (255,255,255 = sin tinte).",["entidad","r","g","b"])
D("G3D_ENTITY_SET_BLEND","Modo de mezcla con las constantes de BennuGD (BLEND_NORMAL, BLEND_ADD, BLEND_MULTIPLY, BLEND_SUBTRACT). Se aplica a todo el arbol del modelo.",["entidad","modo"])
D("G3D_ENTITY_SET_XRAY","Marca (1) una entidad, p. ej. un muro, para que G3D_XRAY_UPDATE la desvanezca cuando tapa al objetivo.",["entidad","marcar"])

# ---- personaje (controlador propio) ----
D("G3D_CHAR_CREATE","Crea un controlador de personaje (capsula) que camina por el terreno y las cajas de colision, con gravedad.",["x","y","z","radio","altura"],"id del personaje",
  "pj = g3d_char_create(0, 2, 0, 0.4, 1.8);\n// cada frame:\ng3d_char_move(pj, vel_x, vel_z);\ng3d_char_update(pj, dt);")
D("G3D_CHAR_DESTROY","Destruye un personaje.",["personaje"])
D("G3D_CHAR_MOVE","Velocidad horizontal deseada para el siguiente G3D_CHAR_UPDATE (en el plano X,Z).",["personaje","vx","vz"])
D("G3D_CHAR_JUMP","Salta con la velocidad vertical indicada, solo si esta en el suelo. Dentro del agua da una brazada hacia arriba.",["personaje","velocidad"])
D("G3D_CHAR_SET_WATER","Marca al personaje dentro (1) o fuera (0) del agua y la altura Y de la superficie. Hay que llamarla cada frame: activa la flotacion y el nado libre.",["personaje","en_agua","superficie_y"])
D("G3D_CHAR_UPDATE","Avanza la simulacion del personaje (movimiento, gravedad, colisiones). Llamar una vez por frame.",["personaje","dt"])
D("G3D_CHAR_SET_POSITION","Coloca al personaje y le anula la velocidad.",["personaje","x","y","z"])
D("G3D_CHAR_SET_PUSH","Fuerza maxima con la que empuja cuerpos rigidos. 0 = no mueve nada, pero sigue chocando.",["personaje","fuerza"])
D("G3D_CHAR_SET_TUNING","Ajusta la altura maxima de escalon y la pendiente maxima transitable en grados (se ignora fuera de 1..89). Un valor de escalon negativo no cambia el actual.",["personaje","escalon","pendiente_grados"])
D("G3D_CHAR_X","Posicion X del personaje.",["personaje"],"X")
D("G3D_CHAR_Y","Posicion Y del personaje.",["personaje"],"Y")
D("G3D_CHAR_Z","Posicion Z del personaje.",["personaje"],"Z")
D("G3D_CHAR_GROUNDED","Indica si el personaje esta apoyado en el suelo.",["personaje"],"1 si esta en el suelo")
D("G3D_CHAR_CLEAR_ALL","Destruye todos los personajes.")

# ---- colisiones ----
D("G3D_COLLIDER_ADD_BOX","Anade una caja de colision (AABB) en coordenadas del mundo. Los extremos se ordenan solos.",["min_x","min_y","min_z","max_x","max_y","max_z"],"id de la caja")
D("G3D_COLLIDER_CLEAR","Elimina todas las cajas de colision.")
D("G3D_COLLIDER_SET_BOX","Cambia las dimensiones de una caja ya creada.",["caja","min_x","min_y","min_z","max_x","max_y","max_z"])
D("G3D_COLLIDER_REMOVE_BOX","Elimina una caja de colision.",["caja"])
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
