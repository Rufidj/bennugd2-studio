# Corrige/amplia la documentacion de fisica tras revisar la ruta Jolt (la de PC).
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

NOTA = " En PC la fisica de cuerpos rigidos la hace Jolt; en Vita y PS5 se usa un motor propio mas simple."
D("G3D_RIGIDBODY_CREATE","Crea un cuerpo rigido en forma de caja. hx, hy, hz son las MEDIAS dimensiones (la mitad del ancho, alto y fondo); si valen menos de 0.01 se usa 0.5. Masa 0 o negativa = cuerpo FIJO: choca con todo pero no lo mueve nada (decorado solido). Rebote por defecto 0.12 y rozamiento 0.55." + NOTA,
  ["x","y","z","hx","hy","hz","masa"],"id del cuerpo, o -1 si no quedan huecos",
  "caja = g3d_rigidbody_create(0, 5, 0, 0.5, 0.5, 0.5, 10);\n// cada frame:\ng3d_rigidbody_step(dt);")
D("G3D_RIGIDBODY_CREATE_CAPSULE","Crea un cuerpo rigido en capsula. medio_alto es la mitad del alto de la parte CILINDRICA: la altura total de la capsula es 2 * (medio_alto + radio). Valores menores de 0.01 se cambian por 0.5. Masa 0 = fijo.",["x","y","z","radio","medio_alto","masa"],"id del cuerpo")
D("G3D_RIGIDBODY_CREATE_CYLINDER","Crea un cuerpo rigido cilindrico. medio_alto es la mitad del alto total. Valores menores de 0.01 se cambian por 0.5. Masa 0 = fijo.",["x","y","z","radio","medio_alto","masa"],"id del cuerpo")
D("G3D_RIGIDBODY_CREATE_SPHERE","Crea un cuerpo rigido esferico. Un radio menor de 0.01 se cambia por 0.5. Masa 0 = fijo.",["x","y","z","radio","masa"],"id del cuerpo")
D("G3D_RIGIDBODY_CREATE_CONVEX","Crea un cuerpo rigido con la envolvente convexa de UNA submalla de un modelo (p. ej. un trozo de una fractura). El cuerpo se coloca en el centroide de la malla y gira sobre el. Falla (devuelve -1) si la submalla tiene menos de 4 vertices.",["x","y","z","modelo","submalla","escala","masa"],"id del cuerpo, o -1 si falla")
D("G3D_RIGIDBODY_CREATE_CONVEX_MODEL","Crea un cuerpo rigido con la envolvente convexa de TODAS las submallas del modelo: se ajusta a su forma sin tener que dar tamanos. Es lo que hay que usar para lo que se mueve (un barril, una caja, una roca que rueda). Solo existe con Jolt: sin el, devuelve -1.",["x","y","z","modelo","escala","masa"],"id del cuerpo, o -1 si falla",
  "barril = g3d_rigidbody_create_convex_model(0, 5, 0, m_barril, 1.0, 20);")
D("G3D_RIGIDBODY_SET_CCD","Activa (1) la deteccion continua de colisiones del cuerpo, para que objetos pequenos y rapidos (una bala) no atraviesen paredes finas. Con 0 vuelve a la deteccion normal. Solo tiene efecto con Jolt.",["cuerpo","activar"])
D("G3D_RIGIDBODY_STEP","Avanza la simulacion de fisica. Llamala una vez por frame. dt se limita a 0.05 s. Antes de simular se incorpora el terreno de la escena como colision, asi que los cuerpos chocan con el suelo sin hacer nada mas.",["dt"])
D("G3D_RIGIDBODY_APPLY_IMPULSE","Aplica un impulso lineal: cambia la velocidad en impulso / masa. Despierta al cuerpo si estaba dormido (un cuerpo quieto durante un rato se duerme y, sin despertarlo, ignoraria el impulso).",["cuerpo","ix","iy","iz"])
D("G3D_RIGIDBODY_APPLY_ANGULAR_IMPULSE","Aplica un impulso de giro: hace girar al cuerpo sin moverlo de sitio. Despierta al cuerpo si dormia.",["cuerpo","ax","ay","az"])
D("G3D_RIGIDBODY_SET_VELOCITY","Fija la velocidad lineal del cuerpo. Despierta al cuerpo si dormia.",["cuerpo","vx","vy","vz"])
D("G3D_RIGIDBODY_SET_ANGULAR_VELOCITY","Fija la velocidad angular del cuerpo. Despierta al cuerpo si dormia.",["cuerpo","wx","wy","wz"])
D("G3D_RIGIDBODY_SET_UPRIGHT","Fuerza con la que el cuerpo se endereza solo, llevando su 'arriba' hacia el 'arriba' del mundo (0 = no se endereza; los negativos cuentan como 0). Funciona con cualquier orientacion.",["cuerpo","fuerza"])
D("G3D_RIGIDBODY_SET_BUOYANCY","Flotacion. Indica el nivel Y del agua y la densidad del cuerpo RELATIVA al agua: menor que 1 flota (0.5 = flota a media agua), 1 queda a flor y mayor que 1 se hunde. 0 o menos desactiva la flotacion. El empuje se reparte por el volumen sumergido, no en el centro, asi que un cuerpo ancho y bajo aguanta, uno alto y estrecho vuelca, y un golpe fuerte tumba a cualquiera. Hay que repetir la llamada si el nivel del agua cambia.",["cuerpo","agua_y","densidad_relativa"],
  ejemplo="g3d_rigidbody_set_buoyancy(barril, g3d_scene_water_level(), 0.6);   // flota")
D("G3D_RIGIDBODY_GROUNDED","CON JOLT (PC) devuelve 1 si el cuerpo esta EN REPOSO (dormido), 0 si se esta moviendo; no distingue 'apoyado en el suelo' de 'quieto en el aire'. Sin Jolt (Vita/PS5) indica apoyo en el suelo.",["cuerpo"],"1 si esta en reposo")
D("G3D_COLLIDER_ADD_MESH","Anade la malla EXACTA (triangulo a triangulo) de una submalla de un modelo como colision estatica: paredes, suelos, escaleras. No se mueve nunca (Jolt no admite malla movil). Para una malla de varias submallas usa G3D_COLLIDER_ADD_MODEL.",["modelo","submalla","x","y","z","escala"])
D("G3D_COLLIDER_ADD_MODEL","Anade el modelo ENTERO (todas sus submallas juntas) como una sola colision estatica exacta. Es la forma de dar colision a un nivel o un objeto completo con una sola llamada en vez de un bucle por submallas. Devuelve -1 si el modelo no tiene submallas.",["modelo","x","y","z","escala"],"-1 si falla",
  "g3d_collider_add_model(m_nivel, 0, 0, 0, 1.0);")
D("G3D_COLLIDER_ADD_MODEL_YAW","Igual que G3D_COLLIDER_ADD_MODEL pero con la malla girada alrededor de Y. OJO: el giro va en GRADOS normales (no en milesimas de grado). Util para una pista importada que hay que orientar. Solo existe con Jolt.",["modelo","x","y","z","escala","giro_grados"],"-1 si falla")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
