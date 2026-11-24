# Documentacion de los ajustes globales de render del motor 3D (G3D_SET_*, IBL, tamano de render).
# Cada dato comprobado contra libmod_3d.c / libmod_3d_renderer.c.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

D("G3D_SET_SUN","Ciclo dia/noche en un solo numero: t da una vuelta completa cada 1.0 (0 = amanecer, 0.25 = mediodia, 0.5 = atardecer, 0.75 = medianoche). Mueve la luz direccional principal de la escena (direccion, color calido cerca del horizonte, intensidad de 0 de noche a 1.6 al mediodia) y ajusta la luz ambiente. Necesita que la escena tenga una luz direccional. Para que los reflejos sigan al sol llama despues a g3d_ibl_refresh.",
  ["t"],"","hora += 0.0005;\ng3d_set_sun(hora);\ng3d_ibl_refresh();")
D("G3D_SET_CLEAR_COLOR","Color de fondo que se pinta antes de dibujar la escena (r, g, b, alpha entre 0 y 1). Solo se ve donde no hay cielo ni objetos; con un cielo activo apenas importa.",["r","g","b","alpha"])
D("G3D_SET_AMBIENT_LIGHT","Luz ambiente: la que llega a todas partes por igual, incluso a las sombras. Color r, g, b entre 0 y 1 e intensidad.",["r","g","b","intensidad"],"","g3d_set_ambient_light(0.4, 0.42, 0.46, 1.0);")
D("G3D_SET_FOG","Niebla lineal. Los objetos a distancia menor que inicio no se tocan y a distancia fin quedan totalmente del color de la niebla. Pon el mismo color que el horizonte del cielo para que se fundan. activo: 1 o 0.",["activo","r","g","b","inicio","fin"],"","g3d_set_fog(1, 0.75, 0.85, 0.95, 40.0, 220.0);")
D("G3D_SET_WIREFRAME_MODE","Dibuja solo las aristas de los triangulos (1) o normal (0). Sirve para depurar mallas.",["activo"])
D("G3D_SET_SHADOWS","Activa (1) o desactiva (0) las sombras proyectadas de la luz direccional. Desactivarlas ahorra mucho rendimiento.",["activo"],"","g3d_set_shadows(1);")
D("G3D_SET_SHADOW_RESOLUTION","Tamano del mapa de sombras en pixeles por lado. Se limita entre 512 y 8192. Mayor tamano = sombras mas nitidas pero mas lentas y con mas memoria; 2048 o 4096 es un buen equilibrio.",["resolucion"],"","g3d_set_shadow_resolution(4096);")
D("G3D_SET_HDR","Activa (1) o desactiva (0) la tuberia HDR de postproceso (exposicion, bloom, SSAO y tonemap dependen de ella). Viene activada.",["activo"])
D("G3D_SET_IBL","Iluminacion basada en imagen: el cielo se captura y se usa como luz ambiente y reflejos reales sobre materiales metalicos y brillantes. activo: 1 o 0; intensidad escala el efecto (1.0 = la luz del propio cielo). Si cambias el cielo en tiempo de ejecucion usa g3d_ibl_refresh.",["activo","intensidad"],"","g3d_set_ibl(1, 1.0);")
D("G3D_IBL_REFRESH","Obliga a recapturar el cielo para el IBL. Llamala cuando el cielo o el sol cambian (ciclo dia/noche); no hace falta cada frame, basta cada pocos segundos.",[],"","g3d_ibl_refresh();")
D("G3D_SET_OCCLUSION","Activa (1) o desactiva (0) el descarte por oclusion: no se dibuja lo que queda totalmente oculto tras otra geometria. Ayuda en interiores y ciudades.",["activo"])
D("G3D_SET_SMAA","Activa (1) o desactiva (0) el antialiasing SMAA de 1 pasada (suaviza los bordes dentados). Es tambien requisito del escalador FSR.",["activo"],"","g3d_set_smaa(1);")
D("G3D_SET_FSR","Escalado FSR 1: dibuja la escena a una fraccion del tamano de la pantalla y la amplia con nitidez. escala 1.0 = nativo, 0.667 = 'calidad' (1080p desde 720p), 0.5 = 'rendimiento'. nitidez regula el afilado. Con activo = 0 vuelve al tamano nativo. Conviene tener SMAA activado.",["activo","escala","nitidez"],"","g3d_set_smaa(1);\ng3d_set_fsr(1, 0.667, 0.2);")
D("G3D_SET_FSR_HEIGHT","Como g3d_set_fsr pero indicando la altura interna de render en pixeles (por ejemplo 720), en vez de una fraccion; asi sigue valiendo si cambias el tamano de la ventana. Nunca renderiza por encima de la pantalla.",["activo","alto","nitidez"],"","g3d_set_smaa(1);\ng3d_set_fsr_height(1, 720, 0.2);")
D("G3D_RENDER_WIDTH","Anchura real, en pixeles, a la que se dibuja la escena (distinta de la ventana si hay FSR).",[],"pixeles")
D("G3D_RENDER_HEIGHT","Altura real, en pixeles, a la que se dibuja la escena (distinta de la ventana si hay FSR).",[],"pixeles")
D("G3D_SET_EXPOSURE","Exposicion del HDR: multiplica el brillo final (1.0 por defecto; los negativos se toman como 0). Sube para escenas oscuras, baja para quemadas.",["exposicion"],"","g3d_set_exposure(0.8);")
D("G3D_SET_BLOOM","Resplandor de las zonas muy brillantes. activo 1/0; cantidad es cuanto se suma (0.5 por defecto); umbral es el brillo a partir del cual brilla (0.8 por defecto). Con valores negativos conserva el valor anterior.",["activo","cantidad","umbral"],"","g3d_set_bloom(1, 0.5, 0.8);")
D("G3D_SET_TONEMAP","Curva de color del HDR: 0 = simple (recorta a 1.0), 1 = ACES (cinematografico, conserva el detalle de las luces fuertes).",["modo"],"","g3d_set_tonemap(1);")
D("G3D_SET_SSAO","Oclusion ambiental en espacio de pantalla: oscurece las esquinas y rincones. activo 1/0; radio (0.5 por defecto) y fuerza (1.2 por defecto) solo cambian si das un valor mayor que 0.",["activo","radio","fuerza"],"","g3d_set_ssao(1, 0.5, 1.2);")
D("G3D_SET_UNDERWATER","Filtro de bajo el agua en toda la pantalla: tinte r, g, b y fuerza (si es 0 o menos se usa 1). Activalo cuando la camara este por debajo de la superficie del agua.",["activo","r","g","b","fuerza"],"","g3d_set_underwater(cam_y < nivel_agua, 0.1, 0.35, 0.5, 1.0);")
D("G3D_SET_LOD","Distancia a partir de la cual los objetos instanciados (g3d_instances_*) pasan a su version simplificada generada automaticamente. 0 = sin LOD.",["distancia"])
D("G3D_SET_CULLING","Activa (1) o desactiva (0) el descarte de objetos fuera del campo de vision de la camara. Viene activado.",["activo"])
D("G3D_SET_BACKFACE_CULL","Activa (1) o desactiva (0) el descarte de las caras traseras de los triangulos. Desactivarlo se ve util en hojas o telas de una sola cara.",["activo"])
D("G3D_SET_SPAWN_PARENT","Padre de creacion: lo que crees a continuacion (entidades, sprites...) queda colgado de esta entidad y se mueve con ella. Pasa -1 para volver a crear sin padre.",["entidad"],"","g3d_set_spawn_parent(coche);\nrueda = g3d_entity_spawn(escena, m_rueda, 1, 0, 1.5);\ng3d_set_spawn_parent(-1);")
D("G3D_SET_TERRAIN_COLLIDER","Registra una malla de terreno (de g3d_primitive_terrain, ya modelada con g3d_terrain_load) como suelo de colision, para que cuerpos rigidos y personajes se apoyen en el relieve. Sin esto solo tenian colision los terrenos cargados con una escena de disco.",["terreno"],"1 si lo registro, 0 si no es un terreno valido",
  "t = g3d_primitive_terrain(128, 200.0, 12.0, 20.0, 7);\ng3d_set_terrain_collider(t);")

json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
