# Documentacion de instancias, vegetacion, terreno, pintura, particulas, espejos y utilidades 3D.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- Instancias ----
D("G3D_INSTANCES_CREATE","Crea un grupo de instancias: una malla dibujada muchas veces en UNA sola llamada de dibujo, con una transformacion por copia. Es lo que hay que usar para miles de arboles, rocas o hierba. La textura puede ser 0. Anade copias con g3d_instances_add.",["malla","textura"],"id del grupo, o -1",
  "g = g3d_instances_create(m_pino, tex_pino);\nfor (i = 0; i < 500; i++)\n    g3d_instances_add(g, rand(-90, 90), 0, rand(-90, 90), rand(0, 360), 1.0);\nend")
D("G3D_INSTANCES_ADD","Anade una copia al grupo en (x, y, z), con giro en grados alrededor de Y y escala uniforme.",["grupo","x","y","z","giro","escala"],"indice de la copia")
D("G3D_INSTANCES_SET","Cambia en el sitio la copia indice del grupo (posicion, giro en grados y escala). Sirve para mover una copia concreta cada frame.",["grupo","indice","x","y","z","giro","escala"])
D("G3D_INSTANCES_CREATE_SKINNED","Como g3d_instances_create pero con animacion de esqueleto por GPU: malla es una submalla con huesos y modelo aporta las matrices de los huesos. Llama antes a g3d_model_set_gpu_skin(modelo, 1).",["malla","textura","modelo"],"id del grupo, o -1")
D("G3D_INSTANCES_SET_WIND","Balanceo por viento del grupo (en el vertex shader, sin animar esqueletos). 0 = inmovil: ponlo a 0 en rocas.",["grupo","fuerza"])
D("G3D_INSTANCES_SET_ALPHA_CUT","1 = descarta los pixeles transparentes (follaje, hojas); 0 = solido (criaturas). Por defecto 1.",["grupo","activo"])
D("G3D_INSTANCES_SET_DISTANCE","Distancia maxima a la que se dibuja el grupo; mas alla se descarta. Un bosque grande necesita esto y g3d_set_lod.",["grupo","distancia"])
D("G3D_INSTANCES_CLEAR","Vacia el grupo (quita todas sus copias) pero conserva el grupo.",["grupo"])
D("G3D_INSTANCES_COUNT","Numero de copias del grupo.",["grupo"],"numero de copias")
D("G3D_SCATTER_MESH","Siembra copias de una malla al azar sobre el terreno: cuantas, en un cuadrado de semi-lado area, con giro aleatorio y escala entre esc_min y esc_max, con balanceo por viento y una semilla para repetir el mismo resultado.",["malla","textura","terreno","cantidad","area","esc_min","esc_max","viento","semilla"],"id del primer grupo")
D("G3D_SCATTER_MODEL","Siembra copias de un modelo entero sobre el terreno: cantidad, cuadrado de semi-lado area, altura objetivo a la que se escala cada copia, variacion de escala (escala base por 1 +/- variacion), viento y semilla.",["modelo","terreno","cantidad","area","alto","variacion","viento","semilla"],"id del primer grupo")
D("G3D_SCATTER_LOAD","Carga de un archivo de siembra (el que guarda el editor) todas las colocaciones y monta los grupos de instancias. Es lo que llama un juego generado. Si el archivo no existe no es un error: la escena simplemente no tiene vegetacion. viento multiplica el balanceo de cada especie (1 = como se diseno).",["ruta","viento"],"numero de colocaciones cargadas, o 0",
  "g3d_scatter_load(\"Scenes/bosque.scatter\", 1.0);")
D("G3D_SCATTER_GROUP","Indice de una especie de la siembra a partir de su recurso (por ejemplo \"Assets/pino.glb\"); la crea si no existe. Es lo que une un proceso a su bosque (se guarda en entity).",["recurso"],"indice de la especie")
D("G3D_SCATTER_CLEAR","Quita toda la vegetacion sembrada y sus grupos de instancias.")

# ---- Terreno ----
BR = " El terreno debe haberse creado con g3d_primitive_terrain (o ser compatible). Recalcula normales y actualiza la GPU, asi que se ve al instante."
D("G3D_TERRAIN_GET_HEIGHT","Altura del terreno en (x, z) interpolando entre vertices. Util para colocar objetos y la camara sobre el suelo.",["terreno","x","z"],"altura, o 0 si no es un terreno o esta fuera",
  "py = g3d_terrain_get_height(t, px, pz);")
D("G3D_TERRAIN_RAISE","Pincel de escultura: sube (fuerza positiva) o baja (negativa) el terreno dentro de radio unidades de (x, z), con caida suave hacia el borde." + BR,["terreno","x","z","radio","fuerza"],"","g3d_terrain_raise(t, 10, 10, 8.0, 0.5);")
D("G3D_TERRAIN_SMOOTH","Alisa el terreno acercando cada altura a la media de sus vecinas dentro del radio. cantidad (0 a 1) es la mezcla en el centro." + BR,["terreno","x","z","radio","cantidad"])
D("G3D_TERRAIN_FLATTEN","Aplana el terreno hacia una altura objetivo dentro del radio. cantidad (0 a 1) es la mezcla." + BR,["terreno","x","z","radio","altura","cantidad"],"","g3d_terrain_flatten(t, 0, 0, 12.0, 2.0, 1.0);   // plataforma a altura 2")
D("G3D_TERRAIN_HOLE","Quita (1) o restaura (0) las celdas del terreno dentro de un disco, para ver a traves del suelo (una cueva de voxels debajo, una entrada).",["terreno","x","z","radio","activo"],"1 si cambio algo")
D("G3D_TERRAIN_LOAD","Aplica al terreno el relieve guardado por el editor (un archivo con el numero de vertices y la altura Y de cada uno). Permite que un juego reproduzca el relieve esculpido.",["terreno","ruta"],"1 si lo cargo; 0 si falta el archivo o el numero de vertices no coincide",
  "t = g3d_primitive_terrain(128, 200.0, 12.0, 20.0, 7);\ng3d_terrain_load(t, \"Scenes/mundo.terrain\");\ng3d_set_terrain_collider(t);")
D("G3D_TERRAIN_PAINT","Pinta una textura sobre el lienzo del terreno con un pincel: dentro de radio unidades de (x, z), con caida suave y opacidad de 0 a 1. La textura se repite src_tiling veces por todo el terreno.",["terreno","lienzo","textura","repeticion","x","z","radio","opacidad"],"",
  "lienzo = g3d_paint_create(1024, 1024);\ng3d_paint_fill(lienzo, tex_hierba, 40.0);\ng3d_terrain_paint(t, lienzo, tex_roca, 40.0, 20, 5, 6.0, 0.8);")
D("G3D_PAINT_CREATE","Crea un lienzo (ancho x alto pixeles, empieza negro opaco) para pintar la textura del terreno.",["ancho","alto"],"manejador del lienzo")
D("G3D_PAINT_FILL","Rellena todo el lienzo repitiendo una textura repeticion veces (capa base) y lo sube a la GPU.",["lienzo","textura","repeticion"])
D("G3D_PAINT_GET_TEXTURE","Textura del lienzo, para usarla como color (albedo) de un material con g3d_material_set_texture.",["lienzo"],"manejador de textura")
D("G3D_PAINT_SAVE","Guarda el lienzo en un archivo de imagen.",["lienzo","ruta"],"resultado de la escritura")

# ---- Particulas ----
D("G3D_PARTICLES_CREATE","Crea un emisor de particulas (puntos suaves en GPU): niebla, espuma, chispas, fuentes. Nacen dentro de la caja centrada en (x, y, z) de semi-lados (ex, ey, ez), con velocidad base (vx, vy, vz) mas una dispersion aleatoria, caen con la gravedad, viven vida segundos y se dibujan con un tamano; cantidad particulas se reciclan sin parar.",
  ["x","y","z","ex","ey","ez","vx","vy","vz","dispersion","gravedad","tamano","vida","cantidad"],"id del emisor, o -1",
  "fuente = g3d_particles_create(0, 1, 0,  0.2, 0, 0.2,  0, 6, 0, 1.5,  9.8, 0.15, 1.5, 200);\ng3d_particles_set_color(fuente, 0.7, 0.85, 1.0);")
D("G3D_PARTICLES_SET_COLOR","Color de las particulas de un emisor (r, g, b de 0 a 1).",["emisor","r","g","b"])
D("G3D_PARTICLES_SET_FLOOR","Las particulas que caen por debajo de y reaparecen (para que un chorro desaparezca al llegar al agua).",["emisor","y"])
D("G3D_PARTICLES_CLEAR","Quita todos los emisores.")

# ---- Espejos ----
D("G3D_MIRROR_CREATE","Crea un espejo plano centrado en (x, y, z), orientado segun la normal (nx, ny, nz), de ancho x alto en su plano. Tiene su propia textura de reflejo y no necesita ninguna imagen. Pueden convivir varios (y el agua).",["x","y","z","nx","ny","nz","ancho","alto"],"indice del espejo, o -1 si no caben mas",
  "esp = g3d_mirror_create(0, 1.5, 5,  0, 0, -1,  3.0, 2.0);")
D("G3D_MIRROR_SET_TINT","Tinte del espejo (0.9 da un cristal ligeramente oscuro).",["espejo","r","g","b"])
D("G3D_MIRROR_SET_FLIP","Invierte (1) la coordenada V del reflejo de un espejo si sale boca abajo.",["espejo","invertir"])
D("G3D_MIRROR_SET_DISTANCE","Distancia maxima a la camara a la que se vuelve a dibujar el reflejo de un espejo; mas alla conserva su ultimo fotograma. Permite muchos espejos baratos. Por defecto 80.",["distancia"])
D("G3D_MIRROR_CLEAR","Quita todos los espejos.")

# ---- Varios ----
D("G3D_XRAY_UPDATE","Rayos X para camaras en tercera persona: una vez por frame, las entidades marcadas con g3d_entity_set_xray cuya caja corta el segmento camara-objetivo (engordado por radio) se vuelven translucidas hacia alfa_min (0 a 1) y recuperan su opacidad al dejar de tapar. velocidad (0 a 1) es la fraccion que avanza por frame.",
  ["cam_x","cam_y","cam_z","obj_x","obj_y","obj_z","radio","alfa_min","velocidad"],"","g3d_xray_update(cx, cy, cz, px, py + 1, pz, 0.5, 0.2, 0.15);")
D("G3D_PHYSICS_SET_GRAVITY","Gravedad global en unidades/s^2, positiva hacia abajo (24 por defecto). Afecta al controlador de personaje y al vehiculo y, en PC, a los cuerpos rigidos de Jolt.",["gravedad"],"","g3d_physics_set_gravity(9.8);")
D("G3D_PHYSICS_BODY_CREATE","OBSOLETA: no hace nada y devuelve -1. Usa g3d_rigidbody_create y sus variantes.",["a","b","c","d"],"-1")
D("G3D_PHYSICS_BODY_SET_VELOCITY","OBSOLETA: no hace nada. Usa g3d_rigidbody_set_velocity.",["cuerpo","vx","vy","vz"])
D("G3D_PHYSICS_STEP","OBSOLETA: no hace nada. Usa g3d_rigidbody_step.",["dt"])

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
