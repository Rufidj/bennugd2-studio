# Documentacion del agua (superficie, lagos, rios, cascadas, simulacion), fluidos y fuego.
# Comprobado contra libmod_3d_water*.h, libmod_3d_watersim.h, libmod_3d_flow.h, libmod_3d_scenefile.c.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- Superficie global ----
D("G3D_WATER_CREATE","Crea la superficie de agua global a la altura nivel, de tamano x tamano unidades y subdividida subdiv veces por lado, y la activa. Con olas animadas, reflejo Fresnel, brillo del sol y transparencia. Despues se afina con g3d_water_set_*.",["nivel","tamano","subdiv"],"id del agua",
  "g3d_water_create(0.0, 400.0, 64);\ng3d_water_set_color(0.0, 0.1, 0.25,  0.1, 0.5, 0.6);\ng3d_water_set_waves(0.3, 6.0, 1.0);")
D("G3D_WATER_SET_ENABLED","Muestra (1) u oculta (0) el agua.",["activo"])
D("G3D_WATER_SET_WAVES","Olas: amplitud maxima, longitud de onda base y velocidad de la animacion.",["amplitud","longitud","velocidad"])
D("G3D_WATER_SET_COLOR","Colores del agua (0 a 1): el de aguas profundas y el de orilla / poco fondo; el brillo Fresnel mezcla entre ambos.",["prof_r","prof_g","prof_b","orilla_r","orilla_g","orilla_b"])
D("G3D_WATER_SET_TEXTURE","Textura de la superficie (de g3d_load_texture, que se repita sin costuras): anade detalle de ondas que se desplazan y tinte. 0 la quita.",["textura"])
D("G3D_WATER_SET_REFLECTION","Reflejo plano: dibuja la escena en espejo y la mezcla en el agua segun Fresnel. activo 1/0; fuerza escala el efecto.",["activo","fuerza"])
D("G3D_WATER_SET_REFLECTION_FLIP","Invierte la coordenada V del reflejo (1) si en tu plataforma sale boca abajo.",["invertir"])
D("G3D_WATER_SET_SSR","Reflejos en espacio de pantalla sobre el agua. activo 1/0 y fuerza.",["activo","fuerza"])
D("G3D_WATER_SET_OCEAN","Oleaje de mar abierto: direccion (x, z) en la que avanzan las olas y amplitud del oleaje que rompe en la orilla (0 = desactivado).",["dir_x","dir_z","amplitud"])
D("G3D_WATER_SET_TESSELLATION","Activa (1) o desactiva (0) las olas con volumen real por teselado (requiere GL4; por defecto activo si se dispone de el).",["activo"])
D("G3D_WATER_SET_SURF","Oleaje que rompe en la playa: cantidad (0 = sin rompientes), longitud de onda medida en unidades de profundidad, velocidad con que llegan y alcance (cuanto sube la espuma por la arena, en unidades del mundo).",["cantidad","longitud","velocidad","alcance"])
D("G3D_WATER_SET_SURF_WAVE","Forma del oleaje de orilla: altura de la cresta y direccion de la que llega, en grados. En la orilla las olas giran solas para quedar paralelas a la costa.",["altura","direccion_grados"])
D("G3D_WATER_SET_FOAM","Espuma (0 = ninguna) y cuanto distorsiona el agua lo que hay detras (refraccion, 0 = nada).",["espuma","refraccion"])
D("G3D_WATER_SET_SPLASH","Salpicaduras: intensidad (0 = sin ellas, 1 por defecto) y velocidad minima del agua, en unidades por segundo, para que salpique al chocar con un obstaculo (0.8 por defecto; sube si un lago en calma burbujea alrededor de las rocas).",["cantidad","umbral"])
D("G3D_WATER_SET_CAUSTICS","Caustica: la red de luz que el agua proyecta en el fondo. 0 la apaga; 1 es la fuerza por defecto.",["fuerza"])
D("G3D_WATER_SET_UNDERWATER_LOOK","Aspecto bajo el agua cuando la camara esta sumergida: visibilidad (hasta donde se ve) y fuerza de los rayos de luz (0 = ninguno).",["visibilidad","rayos"])
D("G3D_WATER_RIPPLE","Lanza una onda circular en (x, z) de la superficie. fuerza 1 = chapoteo normal. Llamala por evento (una brazada) o repetida (un objeto en el agua).",["x","z","fuerza"])
D("G3D_WATER_SPLASH","Onda en la superficie mas una rafaga de gotas en (x, y, z).",["x","y","z","fuerza"],"","g3d_water_splash(px, 0.0, pz, 1.0);")
D("G3D_WATER_ADD_RIPPLE_SOURCE","Registra una fuente continua de ondas (desembocadura de un rio, etc.): el motor repite la onda solo, con una cadencia.",["x","z","fuerza"])
D("G3D_WATER_CLEAR_RIPPLE_SOURCES","Quita todas las fuentes continuas de ondas.")
D("G3D_WATER_LEVEL_AT","Altura de la superficie del agua (mar, lago o rio) en un punto.",["x","z"],"altura, o un valor muy negativo si no hay agua ahi",
  "if (g3d_water_level_at(px, pz) > py) nadando = 1; end")

# ---- Zonas de fluido ----
D("G3D_FLUID_ADD","Coloca una zona de fluido rectangular: un charco, estanque o sala inundada centrado en (cx, cz), de tam_x por tam_z, con la superficie a la altura nivel y profundidad unidades de agua debajo (profundiza el color). Pueden convivir varias, todas con el mismo estilo (g3d_fluid_style).",
  ["cx","cz","tam_x","tam_z","nivel","profundidad"],"id de la zona, o -1","g3d_fluid_add(10, -5, 8, 6, 2.0, 3.0);")
D("G3D_FLUID_CLEAR","Quita todas las zonas de fluido.")
D("G3D_FLUID_STYLE","Estilo comun de todas las zonas de fluido: olas (amplitud, longitud, velocidad), color profundo (r, g, b), color de orilla (r, g, b) y opacidad (0 transparente a 1 opaco). Sin textura.",
  ["amplitud","longitud","velocidad","prof_r","prof_g","prof_b","orilla_r","orilla_g","orilla_b","opacidad"])
D("G3D_FLUID_SET_TEXTURE","Textura comun de las zonas de fluido (de g3d_load_texture). 0 la quita.",["textura"])
D("G3D_FLUID_BLOCK_RESET","Reinicia la mascara de celdas que bloquean el relleno de lagos y la cobertura de lagos. Llamala antes de preparar rios y lagos nuevos.")

# ---- Lagos y rios sobre el terreno ----
D("G3D_LAKE_ADD","Llena de agua el hueco del terreno que contiene el punto (semilla_x, semilla_z), hasta la altura superficie, con la forma exacta del hueco. Necesita que el terreno este registrado como colisionador (g3d_set_terrain_collider). Con el agua unificada del motor, el lago se une a rios, mar y cascadas por si solo.",
  ["semilla_x","semilla_z","superficie","profundidad"],"-1 si no pudo (sin terreno registrado); si no, el hueco queda lleno","g3d_lake_add(30, 12, g3d_lake_spill_level(30, 12) - 0.2, 4.0);")
D("G3D_LAKE_ADD_R","Como g3d_lake_add pero el agua solo llena un disco de radio_max alrededor del punto (0 = sin limite). Sirve en hoyos abiertos, junto al borde de una montana, para que no se desborde y llene todo el escenario.",
  ["semilla_x","semilla_z","superficie","profundidad","radio_max"],"-1 si no pudo (sin terreno registrado); si no, el hueco queda lleno")
D("G3D_LAKE_SPILL_LEVEL","Altura a la que se desbordaria el hueco del terreno donde esta el punto. Pon el agua justo por debajo para que no rebose por el borde.",["semilla_x","semilla_z"],"altura de desborde")
D("G3D_RIVER_BEGIN","Empieza a definir un rio: ancho del cauce (si es 0.1 o menos se usa 3) y cuanto sube el agua sobre el lecho. A continuacion da los puntos con g3d_river_point y termina con g3d_river_end.",["ancho","elevacion"],"","g3d_river_begin(4.0, 0.3);\ng3d_river_point(-80, -60);\ng3d_river_point(-20, -10);\ng3d_river_point(40, 30);\ng3d_river_end();")
D("G3D_RIVER_POINT","Anade un punto (x, z) al rio que se esta definiendo; la altura sale del terreno. El agua corre en el orden en que das los puntos.",["x","z"],"1 si lo anadio, 0 si ya no caben mas puntos")
D("G3D_RIVER_END","Construye el rio con los puntos dados, siguiendo el terreno, con flujo animado y cascadas donde el cauce cae un desnivel fuerte.",[],"resultado de la construccion")
D("G3D_RIVER_BLOCK","Marca el rio que se esta definiendo (begin/point) en la mascara que impide que los lagos lo inunden, SIN construirlo. Llamala antes de crear los lagos.")
D("G3D_RIVER_FALLS","Anade solo las cascadas del rio definido (begin/point), para el camino COMPLETO sin recortar, despues de haber creado su superficie con otra secuencia.")

# ---- Simulacion de agua en vivo ----
D("G3D_WATERSIM_ADD_SOURCE","Manantial en (x, z): vierte tasa unidades de profundidad por segundo en esa celda mientras exista. Una tasa negativa es un sumidero. El agua baja sola por las pendientes formando lagos y rios.",["x","z","tasa"],"id del manantial, o -1",
  "g3d_watersim_add_source(0, 0, 0.5);\ng3d_watersim_settle(60.0);   // que ya aparezca lleno")
D("G3D_WATERSIM_CLEAR_SOURCES","Quita todos los manantiales.")
D("G3D_WATERSIM_SETTLE","Avanza la simulacion segundos de golpe, para que el agua alcance su estado estable (un manantial con su lago y su rio ya llenos) en vez de llenarse poco a poco. Llamala al colocar un manantial o al cargar la escena.",["segundos"])
D("G3D_WATERSIM_SET_VISCOSITY","Multiplicador de la velocidad de flujo (1 = normal). Valores bajos dan un liquido lento y espeso, como lava.",["factor"])
D("G3D_WATERSIM_SET_EVAPORATION","Agua que se evapora o se absorbe por segundo en todas partes. Mantiene los rios finos y seca los charcos sueltos. 0 = el agua solo sale por los bordes del mapa. Ponla a 0 si quieres que un rio alimentado por un manantial no se seque.",["tasa"])

# ---- Cascadas y flujos ----
D("G3D_WATERFALL_ADD","Cascada puesta a mano: del borde (tx, ty, tz) a la base (bx, bz), de ancho dado y con un arco (comba hacia delante, 0 = recta). La base aterriza en el agua si la hay. Usa antes g3d_flow_set_foam / g3d_flow_set_speed para su aspecto.",
  ["tx","ty","tz","bx","bz","ancho","arco"],"id de la cascada, o -1")
D("G3D_FLOW_ADD","Lamina de agua que fluye entre el centro de un borde superior (tx, ty, tz) y el del inferior (bx, by, bz), de ancho dado (a lo largo de X): la textura se desplaza a lo largo. Vertical da una cascada; inclinada, un rio.",
  ["tx","ty","tz","bx","by","bz","ancho","velocidad","repeticion"],"id del flujo, o -1")
D("G3D_FLOW_ADD_RIVER","Rio en linea recta de (x0, z0) a (x1, z1) cuyos vertices siguen la altura del terreno (mas desplazamiento_y). Reacciona al relieve.",["terreno","x0","z0","x1","z1","ancho","desplazamiento_y","velocidad","repeticion"],"id del flujo, o -1")
D("G3D_FLOW_SET_TEXTURE","Textura comun de todos los flujos (de g3d_load_texture).",["textura"])
D("G3D_FLOW_SET_COLOR","Color del agua de todos los flujos (r, g, b de 0 a 1).",["r","g","b"])
D("G3D_FLOW_SET_FOAM","Intensidad de espuma de la PROXIMA cascada que crees.",["espuma"])
D("G3D_FLOW_SET_SPEED","Multiplicador de velocidad de la PROXIMA cascada que crees.",["multiplicador"])
D("G3D_FLOW_SET_CLIP","Recorta los fragmentos de los flujos por debajo de la altura y (por ejemplo el nivel del agua, para que la cascada desaparezca en la superficie y no se vea bajo el agua).",["y"])
D("G3D_FLOW_CLEAR","Quita todos los flujos.")

# ---- Fuego ----
D("G3D_FIRE_ADD","Pone un fuego (antorcha, hoguera) con la base de la llama en (x, y, z). Dibuja una llama procedural brillante (hace bloom) y crea una luz puntual calida que parpadea en la escena activa, ideal para escenas nocturnas.",["x","y","z","tamano"],"id del fuego, o -1","g3d_fire_add(2, 0, 3, 1.2);")
D("G3D_FIRE_CLEAR","Quita todos los fuegos.")

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
