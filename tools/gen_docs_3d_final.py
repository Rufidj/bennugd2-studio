# Documentacion del resto del motor 3D: submallas, streaming, mundos procedurales, zonas,
# prefabs, terreno de voxels y niveles .lvl. Comprobado contra las cabeceras de libmod_3d.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- Modelos y submallas ----
D("G3D_LOAD_GLTF_FRACTURED","Carga un glTF prefracturado (p. ej. de Blender) creando UNA submalla por nodo, en vez de una por material: cada trozo se puede tratar aparte (un cuerpo rigido por pedazo). Ver g3d_model_submesh_cx/hx y g3d_rigidbody_create_convex.",["ruta"],"manejador del modelo, o -1")
D("G3D_MODEL_MESH","Primera malla (submalla 0) de un modelo, para usarla en g3d_instances_create o g3d_scatter_mesh.",["modelo"],"manejador de malla, o -1")
D("G3D_MODEL_TEXTURE","Textura de color (albedo) general del modelo.",["modelo"],"manejador de textura, o -1 si no tiene")
D("G3D_MODEL_SUBMESH_COUNT","Numero de submallas del modelo.",["modelo"],"numero de submallas")
D("G3D_MODEL_SUBMESH","Submalla numero i (desde 0) del modelo.",["modelo","indice"],"manejador de malla, o -1 si el indice no existe")
D("G3D_MODEL_SUBMESH_TEXTURE","Textura de color de la submalla i.",["modelo","indice"],"manejador de textura, o -1")
D("G3D_MODEL_SUBMESH_MAP","Mapa de la submalla i: tipo 1 = normales, 2 = metalico, 3 = rugosidad.",["modelo","indice","tipo"],"manejador de textura, o -1")
D("G3D_MODEL_SUBMESH_IS_WATER","1 si la submalla i es una sala de agua de un nivel de Tomb Raider (marcada por el cargador g3d_load_tr), 0 si no.",["modelo","indice"],"1 o 0")
D("G3D_MODEL_SUBMESH_CX","X del centro de la caja de la submalla i, en el espacio del modelo.",["modelo","indice"],"coordenada")
D("G3D_MODEL_SUBMESH_CY","Y del centro de la caja de la submalla i, en el espacio del modelo.",["modelo","indice"],"coordenada")
D("G3D_MODEL_SUBMESH_CZ","Z del centro de la caja de la submalla i, en el espacio del modelo.",["modelo","indice"],"coordenada")
D("G3D_MODEL_SUBMESH_HX","Mitad del ancho (en X) de la caja de la submalla i. Con CX/CY/CZ y HY/HZ describe la caja que envuelve a un trozo.",["modelo","indice"],"semi-ancho")
D("G3D_MODEL_SUBMESH_HY","Mitad del alto (en Y) de la caja de la submalla i.",["modelo","indice"],"semi-alto")
D("G3D_MODEL_SUBMESH_HZ","Mitad del fondo (en Z) de la caja de la submalla i.",["modelo","indice"],"semi-fondo")
D("G3D_MODEL_SUBMESH_LOD","Genera una copia simplificada de la submalla i para usarla como nivel de detalle lejano en un grupo de instancias. rejilla son las celdas a lo largo del eje mas largo (10 agresivo, 24 suave).",["modelo","indice","rejilla"],"manejador de malla, o -1")

# ---- Streaming de mundos grandes ----
D("G3D_STREAM_INIT","Prepara el streaming de un mundo grande: el mundo se divide en teselas cuadradas de tam_tesela unidades y se mantienen las que estan a menos de radio teselas de la camara (un cuadrado de (2*radio+1) por lado). El motor solo lleva la logica espacial; tu juego decide que contiene cada tesela y la crea o destruye.",["tam_tesela","radio"],"",
  "g3d_stream_init(64.0, 3);")
D("G3D_STREAM_UPDATE","Recalcula las teselas cargadas para la camara en (camx, camz). Llamala cada frame (es barata: las listas solo cambian al cruzar el limite de una tesela). Deja en las listas de carga y descarga las teselas que han cambiado en esta llamada.",["camx","camz"],"",
  "g3d_stream_update(cam_x, cam_z);\nfor (i = 0; i < g3d_stream_load_count(); i++)\n    crear_tesela(g3d_stream_load_x(i), g3d_stream_load_z(i));\nend\nfor (i = 0; i < g3d_stream_unload_count(); i++)\n    borrar_tesela(g3d_stream_unload_x(i), g3d_stream_unload_z(i));\nend")
D("G3D_STREAM_LOAD_COUNT","Numero de teselas que ENTRARON en el anillo en la ultima g3d_stream_update (hay que crearlas).",[],"numero de teselas")
D("G3D_STREAM_LOAD_X","Coordenada X (de tesela, no del mundo) de la tesela i de la lista de carga.",["i"],"X de tesela")
D("G3D_STREAM_LOAD_Z","Coordenada Z (de tesela, no del mundo) de la tesela i de la lista de carga.",["i"],"Z de tesela")
D("G3D_STREAM_UNLOAD_COUNT","Numero de teselas que SALIERON del anillo en la ultima g3d_stream_update (hay que liberarlas).",[],"numero de teselas")
D("G3D_STREAM_UNLOAD_X","Coordenada X de tesela de la tesela i de la lista de descarga.",["i"],"X de tesela")
D("G3D_STREAM_UNLOAD_Z","Coordenada Z de tesela de la tesela i de la lista de descarga.",["i"],"Z de tesela")
D("G3D_STREAM_LOADED_COUNT","Numero de teselas que estan ahora dentro del anillo.",[],"numero de teselas")

# ---- Mundo procedural ----
D("G3D_WORLDGEN_SET","Configura el mundo procedural infinito: semilla, amplitud vertical, frecuencia horizontal (menor = relieve mas grande) y nivel del mar (lo que queda por debajo esta bajo el agua). El mismo mundo sale siempre con la misma semilla y las teselas vecinas encajan en las uniones.",["semilla","amplitud","frecuencia","nivel_mar"],"","g3d_worldgen_set(42, 30.0, 0.01, 2.0);")
D("G3D_WORLDGEN_HEIGHT","Altura del mundo procedural en (x, z). Es mar donde sea menor que el nivel del mar. Sirve para colocar arboles o rocas sobre el suelo.",["x","z"],"altura")
D("G3D_WORLDGEN_TILE","Construye la tesela de terreno (tx, tz) de tam unidades con una rejilla de res x res, colocada en el origen de render (rox, 0, roz) de la escena indicada y con una textura (o 0). Pensada para usarla con g3d_stream_*.",["escena","tx","tz","tam","res","rox","roz","textura"],"id de la entidad, o -1",
  "ent = g3d_worldgen_tile(escena, tx, tz, 64.0, 33, tx * 64.0, tz * 64.0, tex);")
D("G3D_WORLDGEN_TILE_FREE","Destruye una tesela creada con g3d_worldgen_tile y libera su malla.",["entidad"])
D("G3D_WORLDGEN_SET_WATER_DEPTH","Profundidad del fondo marino respecto al nivel del mar (1 por defecto; mayor = mares mas profundos). Solo afecta al terreno ya sumergido.",["profundidad"])
D("G3D_WORLDGEN_SET_BIOME_TEXTURES","Texturas reales por franja de bioma (arena, hierba, roca, nieve), mezcladas segun altura y pendiente. escala es la repeticion en el mundo (por ejemplo 0.03). Con 0 en todas vuelve a los colores planos.",["arena","hierba","roca","nieve","escala"])

# ---- Zonas de movimiento pintadas ----
D("G3D_ZONE_INIT","Crea la mascara de zonas: una rejilla de lado x lado celdas sobre un mundo de tam_mundo unidades, centrada en el origen, con capas de la 0 a la 7 por celda. lado = rejilla del terreno + 1.",["lado","tam_mundo"])
D("G3D_ZONE_LOAD","Carga la mascara de zonas pintada en el editor desde un archivo.",["ruta"],"1 si lo cargo")
D("G3D_ZONE_BLOCKED","1 si la celda mas cercana a (x, z) tiene marcada la capa (0 a 7); con capa negativa, si tiene cualquiera. Sirve para confinar a quien respete esa capa (un barco no sale del agua aunque el personaje si).",["x","z","capa"],"1 o 0",
  "if (g3d_zone_blocked(nx, nz, 0)) nx = x; nz = z; end")
D("G3D_ZONE_CLEAR","Borra todas las capas de la mascara (conserva su tamano).")

# ---- Prefabs ----
D("G3D_PREFAB_CREATE","Crea un prefab vacio con nombre: un grupo reutilizable de piezas (paredes, salas) con posicion local, tamano, color, textura opcional e indicador de colision.",["nombre"],"id del prefab, o -1",
  "p = g3d_prefab_create(\"sala\");\ng3d_prefab_add_box(p, 0, 1, 0,  4, 2, 0.2,  0, 0, 0,  0.8, 0.8, 0.8, 1);\ng3d_prefab_instantiate(p, escena, 10, 0, 5, 90);")
D("G3D_PREFAB_ADD_BOX","Anade una caja al prefab: posicion local (x, y, z), tamano (x, y, z), giro (grados en x, y, z), color (r, g, b) y si es solida (1) o no (0).",["prefab","x","y","z","tx","ty","tz","rx","ry","rz","r","g","b","solido"],"indice de la pieza")
D("G3D_PREFAB_PIECE_TEXTURE","Da textura (una ruta de imagen repetible) a la ULTIMA pieza anadida.",["prefab","ruta"])
D("G3D_PREFAB_COUNT","Numero de piezas del prefab.",["prefab"],"numero de piezas")
D("G3D_PREFAB_SAVE","Guarda el prefab en un archivo de texto.",["prefab","ruta"],"resultado de la escritura")
D("G3D_PREFAB_LOAD","Carga un prefab de un archivo de texto y lo crea nuevo.",["ruta"],"id del prefab nuevo, o -1")
D("G3D_PREFAB_INSTANTIATE","Coloca el prefab en una escena en (x, y, z) girado yaw grados: crea una entidad por pieza (con la colision marcada).",["prefab","escena","x","y","z","giro"],"numero de piezas creadas")

# ---- Terreno de voxels ----
D("G3D_VOXTERRAIN_SOLID","1 si el punto (x, y, z) esta dentro de roca del terreno de voxels (cuevas, voladizos). Sirve para colisionar con paredes y techos.",["x","y","z"],"1 o 0")
D("G3D_VOXTERRAIN_FLOOR","Altura del suelo sobre el que se apoya alguien en (x, y, z): la superficie solida justo debajo (el suelo de la cueva si esta dentro de una). Si el punto esta dentro de roca, devuelve la superficie justo encima (para sacarlo).",["x","y","z"],"altura, o un valor muy negativo si no hay")
D("G3D_VOXTERRAIN_BLOCKED","1 si una capsula (pies en y, de radio y alto dados) chocaria con roca en (x, z): colision robusta con paredes y techos.",["x","y","z","radio","alto"],"1 o 0")
D("G3D_VOXTERRAIN_WORLDSIZE","Tamano del mapa de voxels en X/Z, para mantener al personaje dentro del mundo.",[],"tamano")
D("G3D_VOXTERRAIN_WALK","Mueve una capsula (radio y alto) desde (x, y, z) sumando (dx, dz): desliza por X y luego por Z, sube cuestas de hasta paso_max, y se detiene ante paredes, precipicios y techos bajos. Lee el resultado con g3d_voxterrain_walkx/y/z().",
  ["x","y","z","radio","alto","paso_max","dx","dz"],"",
  "g3d_voxterrain_walk(px, py, pz, 0.4, 1.8, 0.5, vx, vz);\npx = g3d_voxterrain_walkx();\npy = g3d_voxterrain_walky();\npz = g3d_voxterrain_walkz();")
D("G3D_VOXTERRAIN_WALKX","X resuelta por la ultima g3d_voxterrain_walk.",[],"X")
D("G3D_VOXTERRAIN_WALKY","Y resuelta por la ultima g3d_voxterrain_walk.",[],"Y")
D("G3D_VOXTERRAIN_WALKZ","Z resuelta por la ultima g3d_voxterrain_walk.",[],"Z")

# ---- Niveles .lvl ----
D("G3D_LVL_LOAD","Carga un nivel binario .lvl (planos con capas de celdas y los enlaces entre objetos). Lo leen a la vez el editor y el juego.",["ruta"],"manejador del nivel, o 0 si no existe o no es valido","niv = g3d_lvl_load(\"Levels/nivel1.lvl\");\nif (niv == 0) say(\"no hay nivel\"); end")
D("G3D_LVL_FREE","Libera un nivel cargado con g3d_lvl_load.",["nivel"])
D("G3D_LVL_PLANES","Numero de planos del nivel (los validos van de 0 a ese numero menos 1).",["nivel"],"numero de planos")
D("G3D_LVL_W","Ancho en celdas del plano.",["nivel","plano"],"celdas")
D("G3D_LVL_H","Alto en celdas del plano.",["nivel","plano"],"celdas")
D("G3D_LVL_CELL","Valor de la celda (x, y) de una capa de un plano; la fila 0 es la de ARRIBA del mapa. Devuelve 0 si la celda esta fuera.",["nivel","plano","capa","x","y"],"valor 0..255")
D("G3D_LVL_LINKS","Enlaces entre objetos del nivel: texto con una linea por objeto (plano|x|y|nombre|destinos, con destinos separados por comas). Puertas, botones y portales se enlazan por su nombre.",["nivel"],"texto de los enlaces (vacio si no hay)")

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)

d = json.load(open(R, encoding="utf-8"))
D("G3D_RAYCAST(FFFFFFPPP)","NO USAR: variante con tres punteros declarada en el motor, pero su implementacion es la misma que la de 7 parametros y lee el ultimo parametro como distancia maxima, no como puntero. Usa G3D_RAYCAST con 7 parametros y lee el impacto con g3d_ray_hit_x/y/z().",["ox","oy","oz","dx","dy","dz","&salida"],"distancia al impacto, o -1")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
