# Documentacion del resto de 3D: carga de modelos, colision y rayos, picking, cielo, materiales,
# primitivas, raton relativo y estadisticas. Cada dato esta comprobado contra el C de libmod_3d.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- Carga de modelos y texturas ----
D("G3D_LOAD_GLTF","Carga un modelo .gltf o .glb y devuelve su manejador. Por defecto lo recentra en el origen; con g3d_gltf_set_recenter(0) (antes de cargar) conserva las coordenadas originales, necesario para niveles donde cada pieza ya esta en su sitio.",["ruta"],"manejador del modelo, o -1 si falla",
  "m = g3d_load_gltf(\"Assets/casa.glb\");\nif (m == -1) say(\"no se pudo cargar\"); end")
D("G3D_LOAD_OBJ","Carga un modelo Wavefront .obj.",["ruta"],"manejador del modelo, o -1 si falla","m = g3d_load_obj(\"Assets/roca.obj\");")
D("G3D_LOAD_FBX","Carga un modelo .fbx, con esqueleto y animaciones si las tiene. Por defecto lo recentra con g3d_fbx_set_recenter; en personajes animados conviene desactivarlo (valor 0) para que no se desplace respecto a su capsula de colision.",["ruta"],"manejador del modelo, o -1 si falla","m = g3d_load_fbx(\"Assets/lara.fbx\");")
D("G3D_LOAD_MD3","Reservada: el formato MD3 no esta soportado y siempre devuelve -1.",["ruta"],"-1")
D("G3D_LOAD_TR","Carga directamente un nivel original de Tomb Raider (.PHD, .TR2, .TR4, .TRC) sin conversion previa.",["ruta"],"manejador del modelo, o -1 si falla","nivel = g3d_load_tr(\"DATA/LEVEL1.PHD\");")
D("G3D_TR_PROBE","Indica de que juego de Tomb Raider es un archivo (1 a 5) sin cargarlo.",["ruta"],"1..5, o 0 si no es un nivel de Tomb Raider")
D("G3D_GLTF_SET_RECENTER","Activa (1) o desactiva (0) el recentrado de los modelos glTF al cargarlos. Afecta solo a las cargas posteriores.",["activo"])
D("G3D_FBX_SET_RECENTER","Activa (1) o desactiva (0) el recentrado de los modelos FBX al cargarlos. Afecta solo a las cargas posteriores. En personajes animados se recentraba con la pose inicial (tumbada) y los desplazaba de su capsula: usa 0.",["activo"],"","g3d_fbx_set_recenter(0);\npj = g3d_load_fbx(\"Assets/heroe.fbx\");")
D("G3D_GLTF_SET_CHUNKING","Divide los modelos glTF muy grandes (mapas) en celdas espaciales para poder ocultar las que no se ven. -1 = automatico, 0 = desactivado, mayor que 0 = tamano de celda en unidades. Afecta a las cargas posteriores.",["celda"])
D("G3D_LOAD_TEXTURE","Carga una imagen (PNG, JPG...) como textura de la GPU, para usarla en materiales o como cielo.",["ruta"],"manejador de la textura, o -1 si falla","tex = g3d_load_texture(\"Assets/ladrillo.png\");")

# ---- Colision con entidades solidas ----
NOTA_SOL = " Solo cuentan las entidades marcadas como solidas con g3d_entity_set_collider; se tratan como cajas alineadas con los ejes (AABB) en coordenadas del mundo."
D("G3D_COLLIDE_MOVE","Mueve a un personaje (cilindro de radio y altura dados, con los pies en y) desde (x, z) sumando (dx, dz), deslizandolo contra las entidades solidas: se resuelve primero X y luego Z, asi resbala por las paredes en vez de quedarse parado. El resultado no se devuelve: se lee con g3d_collide_x() y g3d_collide_z(). Ignora lo que esta por encima de la cabeza o por debajo de los pies (maximo 0.2 bajo la cima)." + NOTA_SOL,
  ["x","y","z","radio","alto","dx","dz"],"siempre 1",
  "g3d_collide_move(px, py, pz, 0.4, 1.8, vx, vz);\npx = g3d_collide_x();\npz = g3d_collide_z();")
D("G3D_COLLIDE_X","Coordenada X resultante de la ultima llamada a g3d_collide_move.",[],"X final")
D("G3D_COLLIDE_Z","Coordenada Z resultante de la ultima llamada a g3d_collide_move.",[],"Z final")
D("G3D_COLLIDE_FLOOR","Altura del suelo bajo un punto: la cima mas alta de las entidades solidas que hay bajo (x, z) y que no estan mas de 1 unidad por encima de los pies (permite subir escalones). " + NOTA_SOL.strip(),
  ["x","z","radio","pies_y"],"altura del suelo, o un valor muy negativo (-1e9) si no hay ninguna entidad debajo; en ese caso usa la altura del terreno",
  "suelo = g3d_collide_floor(px, pz, 0.4, py);\nif (suelo > -1000000) py = suelo; end")
D("G3D_RAYCAST(FFFFFFF)","Lanza un rayo desde un origen en una direccion (no hace falta que este normalizada) y busca la primera entidad solida que corta, hasta maxd unidades. Si acierta devuelve la distancia y guarda el punto y la entidad: se leen con g3d_ray_hit_x/y/z() y g3d_ray_entity(). Si no acierta devuelve -1." + NOTA_SOL,
  ["ox","oy","oz","dx","dy","dz","maxd"],"distancia al impacto, o -1 si no toca nada",
  "if (g3d_raycast(px, py + 1, pz, 0, 0, 1, 50.0) >= 0)\n    say(\"entidad \" + g3d_ray_entity());\nend")
D("G3D_RAY_HIT_X","X del punto de impacto del ultimo g3d_raycast que acerto.",[],"X del impacto")
D("G3D_RAY_HIT_Y","Y del punto de impacto del ultimo g3d_raycast que acerto.",[],"Y del impacto")
D("G3D_RAY_HIT_Z","Z del punto de impacto del ultimo g3d_raycast que acerto.",[],"Z del impacto")
D("G3D_RAY_ENTITY","Entidad alcanzada por el ultimo g3d_raycast.",[],"id de la entidad, o -1 si fallo")
D("G3D_WALL_RAY","Rayo contra TODA la geometria colisionable (la de g3d_collider_add_model y similares), no solo contra cajas. Util para camaras que no deben atravesar paredes. Solo funciona con Jolt (PC); en las plataformas sin Jolt devuelve siempre el ultimo parametro (maxd), es decir, 'no choca'.",
  ["ox","oy","oz","dx","dy","dz","maxd"],"distancia al primer impacto, o maxd si no hay ninguno")

# ---- Picking con el raton ----
D("G3D_PICK_ENTITY","Entidad solida que hay bajo un punto de la pantalla (sx, sy en pixeles, con w y h el tamano de la pantalla). Lanza un rayo desde la camara de hasta 5000 unidades. El punto de impacto se lee con g3d_pick_x/y/z().",
  ["camara","sx","sy","w","h"],"id de la entidad, o -1 si no hay ninguna",
  "e = g3d_pick_entity(cam, mouse.x, mouse.y, 960, 540);\nif (e != -1) say(\"has pulsado la entidad \" + e); end")
D("G3D_PICK_TERRAIN","Punto del terreno bajo un punto de la pantalla. Avanza el rayo y afina el impacto por biseccion. Con terreno = 0 usa un suelo plano a altura 0.",
  ["camara","sx","sy","w","h","terreno"],"1 si toca el terreno (punto en g3d_pick_x/y/z), 0 si no",
  "if (g3d_pick_terrain(cam, mouse.x, mouse.y, 960, 540, 0))\n    objetivo_x = g3d_pick_x();\n    objetivo_z = g3d_pick_z();\nend")
D("G3D_PICK_X","X del ultimo punto encontrado por g3d_pick_entity o g3d_pick_terrain.",[],"X del punto")
D("G3D_PICK_Y","Y del ultimo punto encontrado por g3d_pick_entity o g3d_pick_terrain.",[],"Y del punto")
D("G3D_PICK_Z","Z del ultimo punto encontrado por g3d_pick_entity o g3d_pick_terrain.",[],"Z del punto")

# ---- Cielo ----
D("G3D_SKY_SET_GRADIENT","Pone un cielo degradado: color del cenit (arriba) y del horizonte, cada uno con r, g, b entre 0 y 1, y lo activa. Sin cielo la pantalla sale NEGRA. El cielo tambien es la luz ambiente (reflejos), asi que cambiarlo recalcula la iluminacion.",
  ["cenit_r","cenit_g","cenit_b","horizonte_r","horizonte_g","horizonte_b"],"",
  "g3d_sky_set_gradient(0.25, 0.45, 0.85,  0.75, 0.85, 0.95);\ng3d_sky_enable(1);")
D("G3D_SKY_SET_TEXTURE","Usa una textura (de g3d_load_texture) como cielo en lugar del degradado. 0 quita la textura.",["textura"])
D("G3D_SKY_ENABLE","Activa (1) o desactiva (0) el dibujado del cielo.",["activo"],"","g3d_sky_enable(1);")
D("G3D_SKY_SET_CLOUDS","Nubes altas del cielo: cantidad entre 0 (despejado) y 1 (cubierto) y velocidad (si es 0 o menos se usa 1).",["cantidad","velocidad"],"","g3d_sky_set_clouds(0.4, 1.0);")
D("G3D_SKY_SET_LOW_CLOUDS","Nubes bajas volumetricas: cobertura entre 0 y 1 (0 = sin nubes), altura de la base en unidades del mundo, grosor (minimo 1) y velocidad (si es 0 o menos se usa 1). Tambien proyectan sombra sobre la escena.",["cobertura","base","grosor","velocidad"])

# ---- Materiales ----
D("G3D_MATERIAL_CREATE","Crea un material PBR (metalico 0, rugosidad 0.5). Se asigna a una entidad con g3d_entity_set_material.",[],"id del material",
  "mat = g3d_material_create();\ng3d_material_set_color(mat, 1.0, 0.2, 0.2, 1.0);\ng3d_entity_set_material(ent, mat);")
D("G3D_MATERIAL_SET_COLOR","Color base del material: r, g, b y alpha, entre 0 y 1.",["material","r","g","b","alpha"])
D("G3D_MATERIAL_SET_METALLIC","Metalicidad del material, de 0 (no metal) a 1 (metal). Se limita a ese rango.",["material","valor"])
D("G3D_MATERIAL_SET_ROUGHNESS","Rugosidad del material, de 0 (espejo) a 1 (mate). Se limita a ese rango.",["material","valor"])
D("G3D_MATERIAL_SET_TEXTURE","Asigna una textura (de g3d_load_texture) a una ranura del material: 0 = color (albedo), 1 = normales, 2 = metalico, 3 = rugosidad.",["material","ranura","textura"],"",
  "g3d_material_set_texture(mat, 0, g3d_load_texture(\"Assets/muro.png\"));\ng3d_material_set_texture(mat, 1, g3d_load_texture(\"Assets/muro_n.png\"));")
D("G3D_MATERIAL_SET_MAP","Asigna un mapa de detalle a un material: 1 = normales, 2 = metalico, 3 = rugosidad. Equivale a g3d_material_set_texture con esas ranuras (el color, ranura 0, solo va por g3d_material_set_texture).",["material","tipo","textura"])
D("G3D_MATERIAL_SET_OUTLINE","Activa (1) o desactiva (0) el contorno tipo dibujo animado en los objetos con este material.",["material","activo"])

# ---- Primitivas ----
NOTA_PRIM = " Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU."
D("G3D_PRIMITIVE_CUBE","Cubo de 1 x 1 x 1 centrado en el origen." + NOTA_PRIM,[],"manejador del modelo, o -1",
  "caja = g3d_primitive_cube();\ne = g3d_entity_spawn(escena, caja, 0, 0.5, 0);")
D("G3D_PRIMITIVE_COLUMN","Columna de 1 x N x 1 centrada en el origen, con la textura repetida una vez por bloque de alto (blocks menor que 1 se toma como 1)." + NOTA_PRIM,["bloques"],"manejador del modelo, o -1")
D("G3D_PRIMITIVE_WEDGE","Cuna (rampa) de 1 x 1 x 1 centrada en el origen: sube hacia +X (altura -0.5 en x=-0.5 y +0.5 en x=+0.5)." + NOTA_PRIM,[],"manejador del modelo, o -1")
D("G3D_PRIMITIVE_SPHERE","Esfera de radio 0.5. segmentos es la resolucion (minimo 3; 16 a 32 se ve redonda)." + NOTA_PRIM,["segmentos"],"manejador del modelo, o -1")
D("G3D_PRIMITIVE_PLANE","Plano horizontal de 1 x 1 centrado en el origen, con la cara visible hacia arriba. Se agranda con size_x / size_z del proceso." + NOTA_PRIM,[],"manejador del modelo, o -1")
D("G3D_PRIMITIVE_TERRAIN","Terreno procedural con ruido fractal: malla de (rejilla+1) x (rejilla+1) vertices centrada en el origen, en el plano XZ. Con alto da la altura maxima, tiling cuantas veces se repite la textura y semilla cambia el relieve." + NOTA_PRIM,
  ["rejilla","tamano","alto","tiling","semilla"],"manejador del terreno, o -1","t = g3d_primitive_terrain(128, 200.0, 12.0, 20.0, 7);")
D("G3D_PRIMITIVE_CLIFFS","Terreno de acantilados: como g3d_primitive_terrain pero con el ruido aplastado en cuencas y mesetas. pendiente (1 suave, 4 a 6 dramatico) marca lo abrupto de los acantilados; fondo_agua (0 a 1) hunde las cuencas por debajo de 0 para que el agua las llene." + NOTA_PRIM,
  ["rejilla","tamano","alto","tiling","semilla","pendiente","fondo_agua"],"manejador del terreno, o -1")
D("G3D_PRIMITIVE_MOUNTAIN","Una montana: un pico radial con ruido de detalle, con base hundida bajo 0 para que el agua forme un lago alrededor. canal (0 a 1) talla una garganta en la cara -Z (para una cascada o rio)." + NOTA_PRIM,
  ["rejilla","tamano","pico","tiling","semilla","canal"],"manejador del terreno, o -1")

# ---- Raton relativo (camara en primera persona) ----
D("G3D_MOUSE_CAPTURE","Activa (1) o desactiva (0) el raton relativo: oculta y bloquea el cursor y da movimientos limpios sin tope en el borde. Es lo que se usa para girar la camara en primera persona. Reinicia los incrementos.",["activo"],"","g3d_mouse_capture(1);")
D("G3D_MOUSE_UPDATE","Lee el movimiento del raton desde la ultima llamada. Llamala una vez por frame y luego usa g3d_mouse_dx() y g3d_mouse_dy().",[],"","g3d_mouse_update();\nmirar_x += g3d_mouse_dx() * 100;")
D("G3D_MOUSE_DX","Movimiento horizontal del raton en pixeles (positivo a la derecha) en la ultima g3d_mouse_update.",[],"incremento en X")
D("G3D_MOUSE_DY","Movimiento vertical del raton en pixeles (positivo hacia abajo) en la ultima g3d_mouse_update.",[],"incremento en Y")

# ---- Estadisticas y mundo grande ----
D("G3D_DRAW_CALLS","Numero de llamadas de dibujo del ultimo frame. Si es alto, agrupa objetos o usa instancias.",[],"numero de llamadas")
D("G3D_TRIANGLES","Numero de triangulos dibujados en el ultimo frame.",[],"numero de triangulos")
D("G3D_CULLED","Numero de entidades descartadas por no verse en el ultimo frame.",[],"numero de entidades")
D("G3D_WORLD_REBASE","Origen flotante: desplaza TODO el mundo (-dx, -dy, -dz) para mantener las coordenadas pequenas lejos del origen, donde los decimales pierden precision. Llamala cuando la camara se aleje mucho y ajusta tu propia camara y tus variables con el mismo desplazamiento.",["dx","dy","dz"],"",
  "if (abs(cam_x) > 5000)\n    g3d_world_rebase(cam_x, 0, 0);\n    cam_x = 0;\nend")

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
