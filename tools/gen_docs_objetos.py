# Anade a tools/api_docs.json la documentacion de model, rigidbody, vehicle y sprite.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---------------- modelos ----------------
D("G3D_MODEL_SPAWN","Crea en la escena un modelo completo (todas sus mallas) colgando de una entidad raiz vacia. Mover, rotar o escalar esa raiz mueve todo el modelo. El modelo se escala uniformemente para que mida 'alto' de alto (si alto <= 0 se queda 1:1). El giro va en MILESIMAS DE GRADO.",
  ["escena","modelo","x","y","z","alto","giro_y"],"id de la entidad raiz, o -1 si el modelo no es valido",
  "m = g3d_load_gltf(\"personaje.glb\");\nraiz = g3d_model_spawn(escena, m, 0, 0, 0, 1.8, 0);")
D("G3D_MODEL_DESPAWN","Destruye una entidad raiz creada con G3D_MODEL_SPAWN, todas sus mallas hijas y sus materiales privados. Usala al crear y quitar modelos constantemente para no agotar los huecos de entidades y materiales. Tambien vale para una sola entidad con material (p. ej. un trozo de fractura).",["raiz"])
D("G3D_MODEL_HEIGHT","Altura total del modelo: la extension maxima en Y entre todas sus mallas.",["modelo"],"altura")
D("G3D_MODEL_SIZE","Mayor dimension del modelo (ancho, alto o fondo) entre todas sus mallas. Util para encuadrar un modelo con una camara libre sea cual sea su escala.",["modelo"],"extension maxima")
D("G3D_MODEL_ORIENT","Corrige la orientacion del modelo: aplica una rotacion a sus vertices y vuelve a apoyarlo en el suelo. Sirve para modelos cuya pose original viene inclinada. OJO: los angulos van en RADIANES (a diferencia de las rotaciones de entidad, que van en milesimas de grado).",["modelo","rx","ry","rz"])
D("G3D_MODEL_ANIM_COUNT","Numero de animaciones que trae el modelo.",["modelo"],"cantidad")
D("G3D_MODEL_ANIM_DURATION","Duracion de una animacion.",["modelo","animacion"],"duracion, en la misma unidad de tiempo que usa G3D_MODEL_ANIMATE")
D("G3D_MODEL_ANIMATE","Pone al modelo (con esqueleto) en la pose de una animacion en el instante dado. Con loop=1 el tiempo se repite al pasar de la duracion. Llamala cada frame con un tiempo creciente. No hace nada si el modelo no tiene esqueleto.",["modelo","animacion","tiempo","bucle"],
  ejemplo="t = t + dt;\ng3d_model_animate(m, ANIM_CAMINAR, t, 1);")
D("G3D_MODEL_ANIMATE_BLEND","Mezcla dos animaciones (p. ej. quieto <-> andar <-> correr). Peso 0 = solo la primera, 1 = solo la segunda; entre medias interpola traslacion y escala y hace slerp de la rotacion. Cada animacion lleva su propio tiempo.",
  ["modelo","anim_a","tiempo_a","anim_b","tiempo_b","peso","bucle"])
D("G3D_MODEL_ANIMATE_ALL","Reproduce a la vez TODAS las animaciones del modelo. Funciona tambien sin esqueleto (toldos, ventiladores, puertas), moviendo los nodos, y si hay esqueleto tambien lo deforma.",["modelo","tiempo","bucle"])
D("G3D_MODEL_REST_POSE","Devuelve el modelo a su pose de reposo.",["modelo"])
D("G3D_MODEL_LOCK_ROOT","Con 1, quita el movimiento de raiz de las animaciones: el personaje anima en el sitio en vez de desplazarse por la pantalla. Afecta a G3D_MODEL_ANIMATE y a ANIMATE_BLEND.",["modelo","activar"])
D("G3D_MODEL_NODE_FIND","Busca un nodo (hueso) del esqueleto por nombre. La busqueda ignora mayusculas y acepta un trozo del nombre. Buscalo una vez y luego lee su posicion cada frame, p. ej. para acoplar un arma a una mano.",["modelo","nombre"],"indice del nodo, o -1 si no existe",
  "mano = g3d_model_node_find(m, \"hand_r\");\n// cada frame, despues de animar:\nx = g3d_model_node_x(m, mano);")
D("G3D_MODEL_NODE_X","Posicion X en el mundo de un nodo del esqueleto, tras la ultima animacion.",["modelo","nodo"],"X")
D("G3D_MODEL_NODE_Y","Posicion Y en el mundo de un nodo del esqueleto, tras la ultima animacion.",["modelo","nodo"],"Y")
D("G3D_MODEL_NODE_Z","Posicion Z en el mundo de un nodo del esqueleto, tras la ultima animacion.",["modelo","nodo"],"Z")
D("G3D_MODEL_SET_GPU_SKIN","Activa (1) o desactiva (0) el skinning por GPU del modelo.",["modelo","activar"])

# ---------------- rigidbody ----------------
D("G3D_RIGIDBODY_CREATE","Crea un cuerpo rigido en forma de caja. hx, hy, hz son las MEDIAS dimensiones (la mitad del ancho, alto y fondo); si valen 0.01 o menos se usa 0.5. Masa 0 o negativa = cuerpo inmovil. Rebote por defecto 0.12 y rozamiento 0.55.",
  ["x","y","z","hx","hy","hz","masa"],"id del cuerpo, o -1 si no quedan huecos",
  "caja = g3d_rigidbody_create(0, 5, 0, 0.5, 0.5, 0.5, 10);\n// cada frame:\ng3d_rigidbody_step(dt);")
D("G3D_RIGIDBODY_CREATE_SPHERE","Crea un cuerpo rigido esferico.",["x","y","z","radio","masa"],"id del cuerpo")
D("G3D_RIGIDBODY_CREATE_CAPSULE","Crea un cuerpo rigido en capsula. half_height es la mitad del alto.",["x","y","z","radio","medio_alto","masa"],"id del cuerpo")
D("G3D_RIGIDBODY_CREATE_CYLINDER","Crea un cuerpo rigido cilindrico. half_height es la mitad del alto.",["x","y","z","radio","medio_alto","masa"],"id del cuerpo")
D("G3D_RIGIDBODY_DESTROY","Destruye un cuerpo rigido.",["cuerpo"])
D("G3D_RIGIDBODY_CLEAR","Destruye todos los cuerpos rigidos.")
D("G3D_RIGIDBODY_STEP","Avanza la simulacion de fisica. Llamala una vez por frame. dt se limita a 0.05 s.",["dt"])
D("G3D_RIGIDBODY_APPLY_IMPULSE","Aplica un impulso lineal: cambia la velocidad en impulso / masa. Un impulso hacia arriba lo despega del suelo.",["cuerpo","ix","iy","iz"])
D("G3D_RIGIDBODY_APPLY_ANGULAR_IMPULSE","Aplica un impulso de giro (cambia la velocidad angular).",["cuerpo","ax","ay","az"])
D("G3D_RIGIDBODY_SET_VELOCITY","Fija la velocidad lineal del cuerpo.",["cuerpo","vx","vy","vz"])
D("G3D_RIGIDBODY_SET_ANGULAR_VELOCITY","Fija la velocidad angular del cuerpo. Lo marca como no apoyado en el suelo.",["cuerpo","wx","wy","wz"])
D("G3D_RIGIDBODY_SET_DAMPING","Amortiguacion lineal y angular. Un valor negativo deja el actual sin cambiar.",["cuerpo","lineal","angular"])
D("G3D_RIGIDBODY_SET_UPRIGHT","Fuerza con la que el cuerpo se endereza solo (0 = no se endereza). Los negativos cuentan como 0.",["cuerpo","fuerza"])
D("G3D_RIGIDBODY_SET_BUOYANCY","Flotacion: nivel Y del agua y densidad relativa del cuerpo respecto al agua (0 o menos = no flota). El empuje se reparte por el volumen sumergido, asi que el cuerpo se endereza o vuelca solo segun donde empuje el agua.",["cuerpo","agua_y","densidad_relativa"])
D("G3D_RIGIDBODY_SET_BOUNCE","Rebote (restitucion) y rozamiento, ambos de 0 a 1. Un valor negativo deja el actual sin cambiar.",["cuerpo","rebote","rozamiento"])
D("G3D_RIGIDBODY_X","Posicion X del centro del cuerpo.",["cuerpo"],"X")
D("G3D_RIGIDBODY_Y","Posicion Y del centro del cuerpo.",["cuerpo"],"Y")
D("G3D_RIGIDBODY_Z","Posicion Z del centro del cuerpo.",["cuerpo"],"Z")
D("G3D_RIGIDBODY_GROUNDED","Indica si el cuerpo esta apoyado en el suelo.",["cuerpo"],"1 si esta apoyado")
D("G3D_RIGIDBODY_ANGLE_X","Orientacion del cuerpo en X (pitch), ya en MILESIMAS DE GRADO para pasarsela directamente a G3D_ENTITY_SET_ROTATION.",["cuerpo"],"angulo")
D("G3D_RIGIDBODY_ANGLE_Y","Orientacion del cuerpo en Y (yaw), en MILESIMAS DE GRADO.",["cuerpo"],"angulo")
D("G3D_RIGIDBODY_ANGLE_Z","Orientacion del cuerpo en Z (roll), en MILESIMAS DE GRADO.",["cuerpo"],"angulo")
D("G3D_RIGIDBODY_RENDER_X","X donde dibujar un modelo con el origen en los pies para que se apoye bien en el suelo y ruede alrededor de su centro. Es la posicion del cuerpo menos el desplazamiento de modelo girado.",["cuerpo"],"X")
D("G3D_RIGIDBODY_RENDER_Y","Y donde dibujar el modelo (ver G3D_RIGIDBODY_RENDER_X).",["cuerpo"],"Y")
D("G3D_RIGIDBODY_RENDER_Z","Z donde dibujar el modelo (ver G3D_RIGIDBODY_RENDER_X).",["cuerpo"],"Z")
D("G3D_RIGIDBODY_SET_MODEL_OFFSET","Desplazamiento entre el centro del cuerpo y el origen del modelo, que usan G3D_RIGIDBODY_RENDER_X/Y/Z. Por defecto es (0, medio_alto, 0), pensado para modelos con el origen en los pies.",["cuerpo","ox","oy","oz"],
  ejemplo="g3d_entity_set_position(raiz, g3d_rigidbody_render_x(c), g3d_rigidbody_render_y(c), g3d_rigidbody_render_z(c));\ng3d_entity_set_rotation(raiz, g3d_rigidbody_angle_x(c), g3d_rigidbody_angle_y(c), g3d_rigidbody_angle_z(c));")

# ---------------- vehiculo ----------------
D("G3D_VEHICLE_CREATE","Crea un vehiculo arcade (direccion tipo bicicleta y seguimiento del terreno con 4 ruedas). Valores por defecto: batalla 4.0, via 2.4, motor 26, velocidad maxima 55, giro maximo 32 grados, freno 40, resistencia 0.8. El rumbo va en MILESIMAS DE GRADO. Maximo 8 vehiculos.",
  ["x","y","z","rumbo"],"id del vehiculo, o -1 si no quedan huecos")
D("G3D_VEHICLE_DESTROY","Destruye un vehiculo.",["vehiculo"])
D("G3D_VEHICLE_UPDATE","Avanza el vehiculo. acelerador de -1 a 1 (negativo = marcha atras), direccion de -1 a 1 y freno de 0 a 1 (los valores fuera de rango se recortan). Con la velocidad la direccion pierde hasta un 40 % de autoridad. Solo acelera con las ruedas en el suelo. dt se limita a 0.1 s.",
  ["vehiculo","dt","acelerador","direccion","freno"],
  ejemplo="g3d_vehicle_update(coche, dt, acel, giro, freno);")
D("G3D_VEHICLE_SET_GEOMETRY","Dimensiones: distancia entre ejes (batalla), distancia entre ruedas de un mismo eje (via) y altura de la suspension. Los valores demasiado pequenos se ignoran (batalla y via deben ser mayores que 0.1; la altura, 0 o mas).",["vehiculo","batalla","via","altura"])
D("G3D_VEHICLE_SET_TUNING","Ajuste de conduccion: fuerza del motor, velocidad maxima, giro maximo en GRADOS, fuerza de frenado y resistencia. Un valor 0 o negativo deja el actual sin cambiar.",["vehiculo","motor","vel_maxima","giro_max_grados","freno","resistencia"])
D("G3D_VEHICLE_X","Posicion X del centro del chasis.",["vehiculo"],"X")
D("G3D_VEHICLE_Y","Posicion Y del centro del chasis.",["vehiculo"],"Y")
D("G3D_VEHICLE_Z","Posicion Z del centro del chasis.",["vehiculo"],"Z")
D("G3D_VEHICLE_YAW","Rumbo del vehiculo, en MILESIMAS DE GRADO.",["vehiculo"],"angulo")
D("G3D_VEHICLE_PITCH","Inclinacion adelante/atras segun el terreno, en MILESIMAS DE GRADO.",["vehiculo"],"angulo")
D("G3D_VEHICLE_ROLL","Inclinacion lateral segun el terreno, en MILESIMAS DE GRADO.",["vehiculo"],"angulo")
D("G3D_VEHICLE_SPEED","Velocidad hacia delante, con signo (negativa = marcha atras).",["vehiculo"],"velocidad")

# ---------------- sprites ----------------
D("G3D_SPRITE_CREATE","Crea un sprite 2D dentro del mundo 3D (estilo HD-2D). Es un quad que se dibuja en el pase opaco con recorte alfa, asi que tapa y es tapado por la geometria 3D sin ordenar nada. La textura sale de un grafico normal de BennuGD2, por lo que se anima como en 2D, cambiando el grafico.",
  ["escena","x","y","z"],"id del sprite")
D("G3D_SPRITE_DESTROY","Destruye un sprite.",["sprite"])
D("G3D_SPRITE_SET_HEIGHT","Fija el alto del sprite en unidades de mundo.",["sprite","alto"])
D("G3D_SPRITE_SET_PIXELS_PER_UNIT","Cuantos pixeles del grafico entran en una unidad de mundo. Es lo comodo para pixel art: el tamano sale del propio grafico.",["sprite","pixeles_por_unidad"])
D("G3D_SPRITE_SET_ANCHOR","Punto del grafico que se coloca en (x, y, z), de 0 a 1. (0.5, 1.0) = centro-abajo: los pies del personaje sobre el suelo.",["sprite","ax","ay"])
D("G3D_SPRITE_SET_BILLBOARD","Como se encara a la camara: G3D_SPRITE_UPRIGHT (0) gira solo en horizontal y sigue de pie (personajes); G3D_SPRITE_FACING (1) mira siempre de frente (objetos, efectos).",["sprite","modo"])
D("G3D_SPRITE_SET_CUTOUT","Umbral de alfa: los pixeles con alfa por debajo se descartan y dejan un agujero.",["sprite","umbral"])
D("G3D_SPRITE_SET_SHADOW","Activa (1) o desactiva (0) que el sprite proyecte sombra: el mismo quad con el mismo recorte alfa entra en el mapa de sombras.",["sprite","activar"])
D("G3D_SPRITE_SET_SMOOTH","Filtrado de la textura. Por defecto es nearest (pixel art); con 1 es bilineal suavizado.",["sprite","activar"])
D("G3D_SPRITE_SET_LIT","Con 1 (por defecto) le afecta la luz de la escena, de modo que se apaga de noche y con el ciclo de dia en vez de ir siempre a pleno brillo.",["sprite","activar"])
D("G3D_SPRITE_SET_SNAP","Con 1 (por defecto) la posicion en pantalla se redondea a pixeles enteros, para que el pixel art no tiemble al moverse.",["sprite","activar"])
D("G3D_SPRITE_SET_CELL","Recorte en pixeles dentro del grafico, para hojas de sprites irregulares (el editor saca estas medidas al detectar los fotogramas).",["sprite","x","y","ancho","alto"])
D("G3D_SPRITE_DIR","Que postura dibujar segun desde donde se mira: 0 = de frente a la camara, y va girando hasta ndirs-1. Con esto el .prg elige el grafico de entre sus N posturas. El angulo va en MILESIMAS DE GRADO, como el local 'angle'.",["sprite","angulo","ndirs"],"indice de la postura (0 .. ndirs-1)")

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
