# Referencia de la API

Generada automaticamente desde los modulos del motor (1150 funciones, 1084 documentadas).

## Modulo 3d
### camera
#### `g3d_camera_create() -> int`
Crea una camara.

**Devuelve:** id de la camara

#### `g3d_camera_set_active(int camara) -> int`
Hace activa una camara: es la que se usa para renderizar.

#### `g3d_camera_set_position(int camara, float x, float y, float z) -> int`
Coloca la camara en el mundo.

#### `g3d_camera_look_at(int camara, float objetivo_x, float objetivo_y, float objetivo_z, float arriba_x, float arriba_y, float arriba_z) -> int`
Orienta la camara hacia un punto. (ux,uy,uz) es el vector que apunta hacia arriba, normalmente (0,1,0).

```
g3d_camera_set_position(cam, 0, 5, -10);
g3d_camera_look_at(cam, 0, 1, 0, 0, 1, 0);
```

#### `g3d_camera_set_projection(int camara, int tipo) -> int`
OBSOLETA: no hace nada. Usa G3D_CAMERA_SET_FOV.

#### `g3d_camera_set_fov(int camara, float fov_grados) -> int`
Campo de vision vertical de la camara, en GRADOS (por defecto 75). Fija aspecto 16:9, near 0.1 y far 1000.

#### `g3d_camera_safe_distance(float objetivo_x, float objetivo_y, float objetivo_z, float dir_x, float dir_y, float dir_z, float distancia, float radio) -> float`
Para camaras en tercera persona: dado el punto objetivo (t), la direccion hacia donde esta la camara (d), la distancia deseada y un radio, devuelve la distancia a la que se puede poner la camara sin atravesar cajas de colision ni terreno.

**Devuelve:** distancia segura (<= distancia)

### char
#### `g3d_char_create(float x, float y, float z, float radio, float altura) -> int`
Crea un controlador de personaje (capsula) que camina por el terreno y las cajas de colision, con gravedad.

**Devuelve:** id del personaje

```
pj = g3d_char_create(0, 2, 0, 0.4, 1.8);
// cada frame:
g3d_char_move(pj, vel_x, vel_z);
g3d_char_update(pj, dt);
```

#### `g3d_char_destroy(int personaje) -> int`
Destruye un personaje.

#### `g3d_char_move(int personaje, float vx, float vz) -> int`
Velocidad horizontal deseada para el siguiente G3D_CHAR_UPDATE (en el plano X,Z).

#### `g3d_char_jump(int personaje, float velocidad) -> int`
Salta con la velocidad vertical indicada, solo si esta en el suelo. Dentro del agua da una brazada hacia arriba.

#### `g3d_char_set_water(int personaje, int en_agua, float superficie_y) -> int`
Marca al personaje dentro (1) o fuera (0) del agua y la altura Y de la superficie. Hay que llamarla cada frame: activa la flotacion y el nado libre.

#### `g3d_char_update(int personaje, float dt) -> int`
Avanza la simulacion del personaje (movimiento, gravedad, colisiones). Llamar una vez por frame.

#### `g3d_char_set_position(int personaje, float x, float y, float z) -> int`
Coloca al personaje y le anula la velocidad.

#### `g3d_char_set_push(int personaje, float fuerza) -> int`
Fuerza maxima con la que empuja cuerpos rigidos. 0 = no mueve nada, pero sigue chocando.

#### `g3d_char_set_tuning(int personaje, float escalon, float pendiente_grados) -> int`
Ajusta la altura maxima de escalon y la pendiente maxima transitable en grados (se ignora fuera de 1..89). Un valor de escalon negativo no cambia el actual.

#### `g3d_char_x(int personaje) -> float`
Posicion X del personaje.

**Devuelve:** X

#### `g3d_char_y(int personaje) -> float`
Posicion Y del personaje.

**Devuelve:** Y

#### `g3d_char_z(int personaje) -> float`
Posicion Z del personaje.

**Devuelve:** Z

#### `g3d_char_grounded(int personaje) -> int`
Indica si el personaje esta apoyado en el suelo.

**Devuelve:** 1 si esta en el suelo

#### `g3d_char_clear_all() -> int`
Destruye todos los personajes.

### cloth
#### `g3d_cloth_create(float ancho, float alto, int nx, int ny, float x, float y, float z) -> int`
Crea una tela (bandera, lona, cortina): una rejilla de nx x ny particulas unidas por distancias, de ancho x alto unidades, con la esquina superior izquierda en (x, y, z). Nace plana en el plano XY y cuelga hacia -Y. Crea su propia entidad en la escena activa, con textura, luz y sombra como cualquier objeto. Es una simulacion de Verlet: llama a la funcion update cada frame para que se mueva. Mas particulas = mas realista pero mas lento.

**Devuelve:** id de la tela, o -1

```
tela = g3d_cloth_create(4.0, 3.0, 20, 15, 0, 6, 0);
g3d_cloth_pin(tela, 0);          // sujeta el borde de arriba
g3d_cloth_set_wind(tela, 1, 0, 0.3, 4.0);
// cada frame:
g3d_cloth_update(tela, 1.0 / 60);
```

#### `g3d_cloth_create_oriented(float ancho, float alto, int nx, int ny, float x, float y, float z, float dirx, float dirz) -> int`
Igual que g3d_cloth_create, pero el ancho de la tela sigue la direccion (dirx, dirz) en el plano XZ en vez de ir siempre por +X. Sirve para colgar un estandarte pegado a una pared que no mira al eje X.

**Devuelve:** id de la tela, o -1

#### `g3d_cloth_pin(int tela, int modo) -> int`
Elige como se sujeta la tela: 0 = todo el borde superior (cortina, estandarte), 1 = solo las dos esquinas de arriba (lona), 2 = el borde izquierdo (bandera en un mastil).

#### `g3d_cloth_set_wind(int tela, float x, float y, float z, float fuerza) -> int`
Viento sobre la tela: direccion (x, y, z) y fuerza.

#### `g3d_cloth_set_collider(int tela, float x, float y, float z, float radio) -> int`
Esfera que la tela no puede atravesar (centro y radio). Muevela cada frame para que siga a un personaje o un objeto.

#### `g3d_cloth_clear_collider(int tela) -> int`
Quita la esfera de colision de la tela.

#### `g3d_cloth_set_texture(int tela, int textura) -> int`
Pone una textura (de g3d_load_texture) a la tela.

```
g3d_cloth_set_texture(tela, g3d_load_texture("Assets/bandera.png"));
```

#### `g3d_cloth_update(int tela, float dt) -> int`
Avanza la simulacion de la tela dt segundos y actualiza su malla. Una vez por frame.

#### `g3d_cloth_push(float x, float y, float z, float radio) -> int`
Empujon global: aparta TODAS las telas de una esfera. Lo llama lo que deba apartarlas (el jugador, un barril, un coche) una vez por frame, con su posicion y su grosor; si deja de llamarse, la tela se recupera sola. Devuelve cuantas particulas de tela esta apartando, para poder frenar al atravesar una lona pesada.

**Devuelve:** particulas apartadas (0 = aire libre)

```
tocando = g3d_cloth_push(px, py + 1, pz, 0.5);
if (tocando > 0) vel = vel * 0.5; end
```

#### `g3d_cloth_push_capsule(float ax, float ay, float az, float bx, float by, float bz, float radio) -> int`
Como g3d_cloth_push pero con una capsula (del punto a al b, con radio): la forma real de una persona, un barril o un coche, para que lo que sobresale no atraviese la tela.

**Devuelve:** particulas apartadas (0 = aire libre)

#### `g3d_cloth_pin_move(int tela, int i, int j, float x, float y, float z) -> int`
Sujeta y coloca UNA particula de la tela (columna i, fila j de la rejilla, desde 0) en un punto del mundo. Llamala cada frame para colgar la tela de algo que se mueve, por ejemplo un punto de una cuerda (g3d_rope_x/y/z).

### collide
#### `g3d_collide_move(float x, float y, float z, float radio, float alto, float dx, float dz) -> int`
Mueve a un personaje (cilindro de radio y altura dados, con los pies en y) desde (x, z) sumando (dx, dz), deslizandolo contra las entidades solidas: se resuelve primero X y luego Z, asi resbala por las paredes en vez de quedarse parado. El resultado no se devuelve: se lee con g3d_collide_x() y g3d_collide_z(). Ignora lo que esta por encima de la cabeza o por debajo de los pies (maximo 0.2 bajo la cima). Solo cuentan las entidades marcadas como solidas con g3d_entity_set_collider; se tratan como cajas alineadas con los ejes (AABB) en coordenadas del mundo.

**Devuelve:** siempre 1

```
g3d_collide_move(px, py, pz, 0.4, 1.8, vx, vz);
px = g3d_collide_x();
pz = g3d_collide_z();
```

#### `g3d_collide_x() -> float`
Coordenada X resultante de la ultima llamada a g3d_collide_move.

**Devuelve:** X final

#### `g3d_collide_z() -> float`
Coordenada Z resultante de la ultima llamada a g3d_collide_move.

**Devuelve:** Z final

#### `g3d_collide_floor(float x, float z, float radio, float pies_y) -> float`
Altura del suelo bajo un punto: la cima mas alta de las entidades solidas que hay bajo (x, z) y que no estan mas de 1 unidad por encima de los pies (permite subir escalones). Solo cuentan las entidades marcadas como solidas con g3d_entity_set_collider; se tratan como cajas alineadas con los ejes (AABB) en coordenadas del mundo.

**Devuelve:** altura del suelo, o un valor muy negativo (-1e9) si no hay ninguna entidad debajo; en ese caso usa la altura del terreno

```
suelo = g3d_collide_floor(px, pz, 0.4, py);
if (suelo > -1000000) py = suelo; end
```

### collider
#### `g3d_collider_add_box(float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) -> int`
Anade una caja de colision (AABB) en coordenadas del mundo. Los extremos se ordenan solos.

**Devuelve:** id de la caja

#### `g3d_collider_clear() -> int`
Elimina todas las cajas de colision.

#### `g3d_collider_set_box(int caja, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) -> int`
Cambia las dimensiones de una caja ya creada.

#### `g3d_collider_remove_box(int caja) -> int`
Elimina una caja de colision.

#### `g3d_collider_add_mesh(int modelo, int submalla, float x, float y, float z, float escala) -> int`
Anade la malla EXACTA (triangulo a triangulo) de una submalla de un modelo como colision estatica: paredes, suelos, escaleras. No se mueve nunca (Jolt no admite malla movil). Para una malla de varias submallas usa G3D_COLLIDER_ADD_MODEL.

#### `g3d_collider_add_model(int modelo, float x, float y, float z, float escala) -> int`
Anade el modelo ENTERO (todas sus submallas juntas) como una sola colision estatica exacta. Es la forma de dar colision a un nivel o un objeto completo con una sola llamada en vez de un bucle por submallas. Devuelve -1 si el modelo no tiene submallas.

**Devuelve:** -1 si falla

```
g3d_collider_add_model(m_nivel, 0, 0, 0, 1.0);
```

#### `g3d_collider_add_model_yaw(int modelo, float x, float y, float z, float escala, float giro_grados) -> int`
Igual que G3D_COLLIDER_ADD_MODEL pero con la malla girada alrededor de Y. OJO: el giro va en GRADOS normales (no en milesimas de grado). Util para una pista importada que hay que orientar. Solo existe con Jolt.

**Devuelve:** -1 si falla

### draw
#### `g3d_draw_calls() -> int`
Numero de llamadas de dibujo del ultimo frame. Si es alto, agrupa objetos o usa instancias.

**Devuelve:** numero de llamadas

### entity
#### `g3d_entity_spawn(int escena, int modelo, float x, float y, float z) -> int`
Crea una entidad con un modelo en una escena.

**Devuelve:** id de la entidad

#### `g3d_entity_destroy(int entidad) -> int`
Destruye una entidad.

#### `g3d_entity_set_position(int entidad, float x, float y, float z) -> int`
Posicion de la entidad en el mundo. OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables (x, y, z); usala solo en entidades sin proceso.

#### `g3d_entity_set_rotation(int entidad, float pitch, float yaw, float roll) -> int`
Rotacion de la entidad. Los angulos van en MILESIMAS DE GRADO, como los de BennuGD (90000 = 90 grados). OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables (angle_x/y/z); usala solo en entidades sin proceso.

```
g3d_entity_set_rotation(e, 0, 90000, 0);   // girada 90 grados sobre Y
```

#### `g3d_entity_set_scale(int entidad, float sx, float sy, float sz) -> int`
Escala de la entidad en cada eje (1.0 = tamano original). OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables (size, size_x/y/z); usala solo en entidades sin proceso.

#### `g3d_entity_get_position(int entidad, pointer &x, pointer &y, pointer &z) -> int`
Lee la posicion de la entidad en tres variables float que se pasan por puntero.

#### `g3d_entity_get_rotation(int entidad, pointer &pitch, pointer &yaw, pointer &roll) -> int`
Lee la rotacion de la entidad en tres variables que se pasan por puntero.

#### `g3d_entity_set_parent(int entidad, int padre) -> int`
Hace que una entidad cuelgue de otra (hereda su transformacion).

#### `g3d_entity_set_collider(int entidad, int solido) -> int`
Marca (1) o desmarca (0) la entidad como solida para la colision.

#### `g3d_entity_set_mesh(int entidad, int malla) -> int`
Cambia la malla de la entidad.

#### `g3d_entity_set_material(int entidad, int material) -> int`
Asigna un material a la entidad.

#### `g3d_entity_set_alpha(int entidad, int alfa) -> int`
Opacidad con la convencion de BennuGD: 0 (invisible) a 255 (opaco). OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables (alpha); usala solo en entidades sin proceso.

#### `g3d_entity_set_color(int entidad, int r, int g, int b) -> int`
Tinte de color, de 0 a 255 por canal (255,255,255 = sin tinte). OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables (color_r/g/b); usala solo en entidades sin proceso.

#### `g3d_entity_set_xray(int entidad, int marcar) -> int`
Marca (1) una entidad, p. ej. un muro, para que G3D_XRAY_UPDATE la desvanezca cuando tapa al objetivo.

#### `g3d_entity_set_blend(int entidad, int modo) -> int`
Modo de mezcla con las constantes de BennuGD (BLEND_NORMAL, BLEND_ADD, BLEND_MULTIPLY, BLEND_SUBTRACT). Se aplica a todo el arbol del modelo. OJO: si la entidad pertenece a un proceso C_3D, el proceso la pisa en cada frame con sus variables (blendmode); usala solo en entidades sin proceso.

### fbx
#### `g3d_fbx_set_recenter(int activo) -> int`
Activa (1) o desactiva (0) el recentrado de los modelos FBX al cargarlos. Afecta solo a las cargas posteriores. En personajes animados se recentraba con la pose inicial (tumbada) y los desplazaba de su capsula: usa 0.

```
g3d_fbx_set_recenter(0);
pj = g3d_load_fbx("Assets/heroe.fbx");
```

### fire
#### `g3d_fire_add(float x, float y, float z, float tamano) -> int`
Pone un fuego (antorcha, hoguera) con la base de la llama en (x, y, z). Dibuja una llama procedural brillante (hace bloom) y crea una luz puntual calida que parpadea en la escena activa, ideal para escenas nocturnas.

**Devuelve:** id del fuego, o -1

```
g3d_fire_add(2, 0, 3, 1.2);
```

#### `g3d_fire_clear() -> int`
Quita todos los fuegos.

### flow
#### `g3d_flow_add(float tx, float ty, float tz, float bx, float by, float bz, float ancho, float velocidad, float repeticion) -> int`
Lamina de agua que fluye entre el centro de un borde superior (tx, ty, tz) y el del inferior (bx, by, bz), de ancho dado (a lo largo de X): la textura se desplaza a lo largo. Vertical da una cascada; inclinada, un rio.

**Devuelve:** id del flujo, o -1

#### `g3d_flow_add_river(int terreno, float x0, float z0, float x1, float z1, float ancho, float desplazamiento_y, float velocidad, float repeticion) -> int`
Rio en linea recta de (x0, z0) a (x1, z1) cuyos vertices siguen la altura del terreno (mas desplazamiento_y). Reacciona al relieve.

**Devuelve:** id del flujo, o -1

#### `g3d_flow_set_texture(int textura) -> int`
Textura comun de todos los flujos (de g3d_load_texture).

#### `g3d_flow_set_color(float r, float g, float b) -> int`
Color del agua de todos los flujos (r, g, b de 0 a 1).

#### `g3d_flow_set_foam(float espuma) -> int`
Intensidad de espuma de la PROXIMA cascada que crees.

#### `g3d_flow_set_speed(float multiplicador) -> int`
Multiplicador de velocidad de la PROXIMA cascada que crees.

#### `g3d_flow_clear() -> int`
Quita todos los flujos.

#### `g3d_flow_set_clip(float y) -> int`
Recorta los fragmentos de los flujos por debajo de la altura y (por ejemplo el nivel del agua, para que la cascada desaparezca en la superficie y no se vea bajo el agua).

### fluid
#### `g3d_fluid_add(float cx, float cz, float tam_x, float tam_z, float nivel, float profundidad) -> int`
Coloca una zona de fluido rectangular: un charco, estanque o sala inundada centrado en (cx, cz), de tam_x por tam_z, con la superficie a la altura nivel y profundidad unidades de agua debajo (profundiza el color). Pueden convivir varias, todas con el mismo estilo (g3d_fluid_style).

**Devuelve:** id de la zona, o -1

```
g3d_fluid_add(10, -5, 8, 6, 2.0, 3.0);
```

#### `g3d_fluid_clear() -> int`
Quita todas las zonas de fluido.

#### `g3d_fluid_style(float amplitud, float longitud, float velocidad, float prof_r, float prof_g, float prof_b, float orilla_r, float orilla_g, float orilla_b, float opacidad) -> int`
Estilo comun de todas las zonas de fluido: olas (amplitud, longitud, velocidad), color profundo (r, g, b), color de orilla (r, g, b) y opacidad (0 transparente a 1 opaco). Sin textura.

#### `g3d_fluid_block_reset() -> int`
Reinicia la mascara de celdas que bloquean el relleno de lagos y la cobertura de lagos. Llamala antes de preparar rios y lagos nuevos.

#### `g3d_fluid_set_texture(int textura) -> int`
Textura comun de las zonas de fluido (de g3d_load_texture). 0 la quita.

### general
#### `g3d_raycast(float ox, float oy, float oz, float dx, float dy, float dz, float maxd) -> float`
Lanza un rayo desde un origen en una direccion (no hace falta que este normalizada) y busca la primera entidad solida que corta, hasta maxd unidades. Si acierta devuelve la distancia y guarda el punto y la entidad: se leen con g3d_ray_hit_x/y/z() y g3d_ray_entity(). Si no acierta devuelve -1. Solo cuentan las entidades marcadas como solidas con g3d_entity_set_collider; se tratan como cajas alineadas con los ejes (AABB) en coordenadas del mundo.

**Devuelve:** distancia al impacto, o -1 si no toca nada

```
if (g3d_raycast(px, py + 1, pz, 0, 0, 1, 50.0) >= 0)
    say("entidad " + g3d_ray_entity());
end
```

#### `g3d_triangles() -> int`
Numero de triangulos dibujados en el ultimo frame.

**Devuelve:** numero de triangulos

#### `g3d_culled() -> int`
Numero de entidades descartadas por no verse en el ultimo frame.

**Devuelve:** numero de entidades

#### `g3d_raycast(float, float, float, float, float, float, pointer, pointer, pointer) -> int`

### gltf
#### `g3d_gltf_set_recenter(int activo) -> int`
Activa (1) o desactiva (0) el recentrado de los modelos glTF al cargarlos. Afecta solo a las cargas posteriores.

#### `g3d_gltf_set_chunking(float celda) -> int`
Divide los modelos glTF muy grandes (mapas) en celdas espaciales para poder ocultar las que no se ven. -1 = automatico, 0 = desactivado, mayor que 0 = tamano de celda en unidades. Afecta a las cargas posteriores.

### ibl
#### `g3d_ibl_refresh() -> int`
Obliga a recapturar el cielo para el IBL. Llamala cuando el cielo o el sol cambian (ciclo dia/noche); no hace falta cada frame, basta cada pocos segundos.

```
g3d_ibl_refresh();
```

### instances
#### `g3d_instances_create(int malla, int textura) -> int`
Crea un grupo de instancias: una malla dibujada muchas veces en UNA sola llamada de dibujo, con una transformacion por copia. Es lo que hay que usar para miles de arboles, rocas o hierba. La textura puede ser 0. Anade copias con g3d_instances_add.

**Devuelve:** id del grupo, o -1

```
g = g3d_instances_create(m_pino, tex_pino);
for (i = 0; i < 500; i++)
    g3d_instances_add(g, rand(-90, 90), 0, rand(-90, 90), rand(0, 360), 1.0);
end
```

#### `g3d_instances_add(int grupo, float x, float y, float z, float giro, float escala) -> int`
Anade una copia al grupo en (x, y, z), con giro en grados alrededor de Y y escala uniforme.

**Devuelve:** indice de la copia

#### `g3d_instances_set(int grupo, int indice, float x, float y, float z, float giro, float escala) -> int`
Cambia en el sitio la copia indice del grupo (posicion, giro en grados y escala). Sirve para mover una copia concreta cada frame.

#### `g3d_instances_create_skinned(int malla, int textura, int modelo) -> int`
Como g3d_instances_create pero con animacion de esqueleto por GPU: malla es una submalla con huesos y modelo aporta las matrices de los huesos. Llama antes a g3d_model_set_gpu_skin(modelo, 1).

**Devuelve:** id del grupo, o -1

#### `g3d_instances_set_wind(int grupo, float fuerza) -> int`
Balanceo por viento del grupo (en el vertex shader, sin animar esqueletos). 0 = inmovil: ponlo a 0 en rocas.

#### `g3d_instances_set_alpha_cut(int grupo, int activo) -> int`
1 = descarta los pixeles transparentes (follaje, hojas); 0 = solido (criaturas). Por defecto 1.

#### `g3d_instances_set_distance(int grupo, float distancia) -> int`
Distancia maxima a la que se dibuja el grupo; mas alla se descarta. Un bosque grande necesita esto y g3d_set_lod.

#### `g3d_instances_clear(int grupo) -> int`
Vacia el grupo (quita todas sus copias) pero conserva el grupo.

#### `g3d_instances_count(int grupo) -> int`
Numero de copias del grupo.

**Devuelve:** numero de copias

### lake
#### `g3d_lake_add(float semilla_x, float semilla_z, float superficie, float profundidad) -> int`
Llena de agua el hueco del terreno que contiene el punto (semilla_x, semilla_z), hasta la altura superficie, con la forma exacta del hueco. Necesita que el terreno este registrado como colisionador (g3d_set_terrain_collider). Con el agua unificada del motor, el lago se une a rios, mar y cascadas por si solo.

**Devuelve:** -1 si no pudo (sin terreno registrado); si no, el hueco queda lleno

```
g3d_lake_add(30, 12, g3d_lake_spill_level(30, 12) - 0.2, 4.0);
```

#### `g3d_lake_add_r(float semilla_x, float semilla_z, float superficie, float profundidad, float radio_max) -> int`
Como g3d_lake_add pero el agua solo llena un disco de radio_max alrededor del punto (0 = sin limite). Sirve en hoyos abiertos, junto al borde de una montana, para que no se desborde y llene todo el escenario.

**Devuelve:** -1 si no pudo (sin terreno registrado); si no, el hueco queda lleno

#### `g3d_lake_spill_level(float semilla_x, float semilla_z) -> float`
Altura a la que se desbordaria el hueco del terreno donde esta el punto. Pon el agua justo por debajo para que no rebose por el borde.

**Devuelve:** altura de desborde

### light
#### `g3d_light_create(int tipo, float r, float g, float b) -> int`
Crea una luz. Tipo: G3D_LIGHT_DIRECTIONAL (0), G3D_LIGHT_POINT (1) o G3D_LIGHT_SPOT (2). Maximo 32 luces. La variante de 1 parametro crea la luz en blanco.

**Devuelve:** id de la luz, o -1 si se llego al maximo

```
sol = g3d_light_create(G3D_LIGHT_DIRECTIONAL, 1.0, 0.95, 0.8);
g3d_light_set_direction(sol, -0.4, -1.0, -0.3);
```

#### `g3d_light_create(int tipo) -> int`
Crea una luz. Tipo: G3D_LIGHT_DIRECTIONAL (0), G3D_LIGHT_POINT (1) o G3D_LIGHT_SPOT (2). Maximo 32 luces. La variante de 1 parametro crea la luz en blanco.

**Devuelve:** id de la luz, o -1 si se llego al maximo

```
sol = g3d_light_create(G3D_LIGHT_DIRECTIONAL, 1.0, 0.95, 0.8);
g3d_light_set_direction(sol, -0.4, -1.0, -0.3);
```

#### `g3d_light_set_position(int luz, float x, float y, float z) -> int`
Posicion de una luz puntual o foco.

#### `g3d_light_set_direction(int luz, float x, float y, float z) -> int`
Direccion hacia la que apunta una luz direccional o un foco.

#### `g3d_light_set_intensity(int luz, float intensidad) -> int`
Intensidad de la luz. Los valores negativos se tratan como 0.

#### `g3d_light_set_color(int luz, float r, float g, float b) -> int`
Color de la luz, componentes de 0.0 a 1.0.

#### `g3d_light_set_range(int luz, float alcance) -> int`
Alcance de una luz puntual o foco.

#### `g3d_light_set_cone(int luz, float grados) -> int`
Angulo del cono de un foco, en grados.

#### `g3d_light_enable_shadow(int luz, int activar) -> int`
Activa (1) o desactiva (0) las sombras de una luz.

#### `g3d_light_set_shadow_quality(int luz, int resolucion) -> int`
Resolucion del mapa de sombras. Afecta tambien a la del renderer (equivale a G3D_SET_SHADOW_RESOLUTION).

### load
#### `g3d_load_gltf(string ruta) -> int`
Carga un modelo .gltf o .glb y devuelve su manejador. Por defecto lo recentra en el origen; con g3d_gltf_set_recenter(0) (antes de cargar) conserva las coordenadas originales, necesario para niveles donde cada pieza ya esta en su sitio.

**Devuelve:** manejador del modelo, o -1 si falla

```
m = g3d_load_gltf("Assets/casa.glb");
if (m == -1) say("no se pudo cargar"); end
```

#### `g3d_load_tr(string ruta) -> int`
Carga directamente un nivel original de Tomb Raider (.PHD, .TR2, .TR4, .TRC) sin conversion previa.

**Devuelve:** manejador del modelo, o -1 si falla

```
nivel = g3d_load_tr("DATA/LEVEL1.PHD");
```

#### `g3d_load_gltf_fractured(string ruta) -> int`
Carga un glTF prefracturado (p. ej. de Blender) creando UNA submalla por nodo, en vez de una por material: cada trozo se puede tratar aparte (un cuerpo rigido por pedazo). Ver g3d_model_submesh_cx/hx y g3d_rigidbody_create_convex.

**Devuelve:** manejador del modelo, o -1

#### `g3d_load_obj(string ruta) -> int`
Carga un modelo Wavefront .obj.

**Devuelve:** manejador del modelo, o -1 si falla

```
m = g3d_load_obj("Assets/roca.obj");
```

#### `g3d_load_fbx(string ruta) -> int`
Carga un modelo .fbx, con esqueleto y animaciones si las tiene. Por defecto lo recentra con g3d_fbx_set_recenter; en personajes animados conviene desactivarlo (valor 0) para que no se desplace respecto a su capsula de colision.

**Devuelve:** manejador del modelo, o -1 si falla

```
m = g3d_load_fbx("Assets/lara.fbx");
```

#### `g3d_load_md3(string ruta) -> int`
Reservada: el formato MD3 no esta soportado y siempre devuelve -1.

**Devuelve:** -1

#### `g3d_load_texture(string ruta) -> int`
Carga una imagen (PNG, JPG...) como textura de la GPU, para usarla en materiales o como cielo.

**Devuelve:** manejador de la textura, o -1 si falla

```
tex = g3d_load_texture("Assets/ladrillo.png");
```

### lvl
#### `g3d_lvl_load(string ruta) -> int`
Carga un nivel binario .lvl (planos con capas de celdas y los enlaces entre objetos). Lo leen a la vez el editor y el juego.

**Devuelve:** manejador del nivel, o 0 si no existe o no es valido

```
niv = g3d_lvl_load("Levels/nivel1.lvl");
if (niv == 0) say("no hay nivel"); end
```

#### `g3d_lvl_free(int nivel) -> int`
Libera un nivel cargado con g3d_lvl_load.

#### `g3d_lvl_planes(int nivel) -> int`
Numero de planos del nivel (los validos van de 0 a ese numero menos 1).

**Devuelve:** numero de planos

#### `g3d_lvl_w(int nivel, int plano) -> int`
Ancho en celdas del plano.

**Devuelve:** celdas

#### `g3d_lvl_h(int nivel, int plano) -> int`
Alto en celdas del plano.

**Devuelve:** celdas

#### `g3d_lvl_cell(int nivel, int plano, int capa, int x, int y) -> int`
Valor de la celda (x, y) de una capa de un plano; la fila 0 es la de ARRIBA del mapa. Devuelve 0 si la celda esta fuera.

**Devuelve:** valor 0..255

#### `g3d_lvl_links(int nivel) -> string`
Enlaces entre objetos del nivel: texto con una linea por objeto (plano|x|y|nombre|destinos, con destinos separados por comas). Puertas, botones y portales se enlazan por su nombre.

**Devuelve:** texto de los enlaces (vacio si no hay)

### material
#### `g3d_material_create() -> int`
Crea un material PBR (metalico 0, rugosidad 0.5). Se asigna a una entidad con g3d_entity_set_material.

**Devuelve:** id del material

```
mat = g3d_material_create();
g3d_material_set_color(mat, 1.0, 0.2, 0.2, 1.0);
g3d_entity_set_material(ent, mat);
```

#### `g3d_material_set_color(int material, float r, float g, float b, float alpha) -> int`
Color base del material: r, g, b y alpha, entre 0 y 1.

#### `g3d_material_set_metallic(int material, float valor) -> int`
Metalicidad del material, de 0 (no metal) a 1 (metal). Se limita a ese rango.

#### `g3d_material_set_roughness(int material, float valor) -> int`
Rugosidad del material, de 0 (espejo) a 1 (mate). Se limita a ese rango.

#### `g3d_material_set_texture(int material, int ranura, int textura) -> int`
Asigna una textura (de g3d_load_texture) a una ranura del material: 0 = color (albedo), 1 = normales, 2 = metalico, 3 = rugosidad.

```
g3d_material_set_texture(mat, 0, g3d_load_texture("Assets/muro.png"));
g3d_material_set_texture(mat, 1, g3d_load_texture("Assets/muro_n.png"));
```

#### `g3d_material_set_map(int material, int tipo, int textura) -> int`
Asigna un mapa de detalle a un material: 1 = normales, 2 = metalico, 3 = rugosidad. Equivale a g3d_material_set_texture con esas ranuras (el color, ranura 0, solo va por g3d_material_set_texture).

#### `g3d_material_set_outline(int material, int activo) -> int`
Activa (1) o desactiva (0) el contorno tipo dibujo animado en los objetos con este material.

### mirror
#### `g3d_mirror_create(float x, float y, float z, float nx, float ny, float nz, float ancho, float alto) -> int`
Crea un espejo plano centrado en (x, y, z), orientado segun la normal (nx, ny, nz), de ancho x alto en su plano. Tiene su propia textura de reflejo y no necesita ninguna imagen. Pueden convivir varios (y el agua).

**Devuelve:** indice del espejo, o -1 si no caben mas

```
esp = g3d_mirror_create(0, 1.5, 5,  0, 0, -1,  3.0, 2.0);
```

#### `g3d_mirror_set_flip(int espejo, int invertir) -> int`
Invierte (1) la coordenada V del reflejo de un espejo si sale boca abajo.

#### `g3d_mirror_set_tint(int espejo, float r, float g, float b) -> int`
Tinte del espejo (0.9 da un cristal ligeramente oscuro).

#### `g3d_mirror_clear() -> int`
Quita todos los espejos.

#### `g3d_mirror_set_distance(float distancia) -> int`
Distancia maxima a la camara a la que se vuelve a dibujar el reflejo de un espejo; mas alla conserva su ultimo fotograma. Permite muchos espejos baratos. Por defecto 80.

### model
#### `g3d_model_despawn(int raiz) -> int`
Destruye una entidad raiz creada con G3D_MODEL_SPAWN, todas sus mallas hijas y sus materiales privados. Usala al crear y quitar modelos constantemente para no agotar los huecos de entidades y materiales. Tambien vale para una sola entidad con material (p. ej. un trozo de fractura).

#### `g3d_model_mesh(int modelo) -> int`
Primera malla (submalla 0) de un modelo, para usarla en g3d_instances_create o g3d_scatter_mesh.

**Devuelve:** manejador de malla, o -1

#### `g3d_model_texture(int modelo) -> int`
Textura de color (albedo) general del modelo.

**Devuelve:** manejador de textura, o -1 si no tiene

#### `g3d_model_submesh_count(int modelo) -> int`
Numero de submallas del modelo.

**Devuelve:** numero de submallas

#### `g3d_model_height(int modelo) -> float`
Altura total del modelo: la extension maxima en Y entre todas sus mallas.

**Devuelve:** altura

#### `g3d_model_size(int modelo) -> float`
Mayor dimension del modelo (ancho, alto o fondo) entre todas sus mallas. Util para encuadrar un modelo con una camara libre sea cual sea su escala.

**Devuelve:** extension maxima

#### `g3d_model_orient(int modelo, float rx, float ry, float rz) -> int`
Corrige la orientacion del modelo: aplica una rotacion a sus vertices y vuelve a apoyarlo en el suelo. Sirve para modelos cuya pose original viene inclinada. OJO: los angulos van en RADIANES (a diferencia de las rotaciones de entidad, que van en milesimas de grado).

#### `g3d_model_anim_count(int modelo) -> int`
Numero de animaciones que trae el modelo.

**Devuelve:** cantidad

#### `g3d_model_anim_duration(int modelo, int animacion) -> float`
Duracion de una animacion.

**Devuelve:** duracion, en la misma unidad de tiempo que usa G3D_MODEL_ANIMATE

#### `g3d_model_animate(int modelo, int animacion, float tiempo, int bucle) -> int`
Pone al modelo (con esqueleto) en la pose de una animacion en el instante dado. Con loop=1 el tiempo se repite al pasar de la duracion. Llamala cada frame con un tiempo creciente. No hace nada si el modelo no tiene esqueleto.

```
t = t + dt;
g3d_model_animate(m, ANIM_CAMINAR, t, 1);
```

#### `g3d_model_animate_blend(int modelo, int anim_a, float tiempo_a, int anim_b, float tiempo_b, float peso, int bucle) -> int`
Mezcla dos animaciones (p. ej. quieto <-> andar <-> correr). Peso 0 = solo la primera, 1 = solo la segunda; entre medias interpola traslacion y escala y hace slerp de la rotacion. Cada animacion lleva su propio tiempo.

#### `g3d_model_animate_all(int modelo, float tiempo, int bucle) -> int`
Reproduce a la vez TODAS las animaciones del modelo. Funciona tambien sin esqueleto (toldos, ventiladores, puertas), moviendo los nodos, y si hay esqueleto tambien lo deforma.

#### `g3d_model_rest_pose(int modelo) -> int`
Devuelve el modelo a su pose de reposo.

#### `g3d_model_lock_root(int modelo, int activar) -> int`
Con 1, quita el movimiento de raiz de las animaciones: el personaje anima en el sitio en vez de desplazarse por la pantalla. Afecta a G3D_MODEL_ANIMATE y a ANIMATE_BLEND.

#### `g3d_model_submesh(int modelo, int indice) -> int`
Submalla numero i (desde 0) del modelo.

**Devuelve:** manejador de malla, o -1 si el indice no existe

#### `g3d_model_submesh_texture(int modelo, int indice) -> int`
Textura de color de la submalla i.

**Devuelve:** manejador de textura, o -1

#### `g3d_model_submesh_is_water(int modelo, int indice) -> int`
1 si la submalla i es una sala de agua de un nivel de Tomb Raider (marcada por el cargador g3d_load_tr), 0 si no.

**Devuelve:** 1 o 0

#### `g3d_model_node_find(int modelo, string nombre) -> int`
Busca un nodo (hueso) del esqueleto por nombre. La busqueda ignora mayusculas y acepta un trozo del nombre. Buscalo una vez y luego lee su posicion cada frame, p. ej. para acoplar un arma a una mano.

**Devuelve:** indice del nodo, o -1 si no existe

```
mano = g3d_model_node_find(m, "hand_r");
// cada frame, despues de animar:
x = g3d_model_node_x(m, mano);
```

#### `g3d_model_node_x(int modelo, int nodo) -> float`
Posicion X en el mundo de un nodo del esqueleto, tras la ultima animacion.

**Devuelve:** X

#### `g3d_model_node_y(int modelo, int nodo) -> float`
Posicion Y en el mundo de un nodo del esqueleto, tras la ultima animacion.

**Devuelve:** Y

#### `g3d_model_node_z(int modelo, int nodo) -> float`
Posicion Z en el mundo de un nodo del esqueleto, tras la ultima animacion.

**Devuelve:** Z

#### `g3d_model_submesh_cx(int modelo, int indice) -> float`
X del centro de la caja de la submalla i, en el espacio del modelo.

**Devuelve:** coordenada

#### `g3d_model_submesh_cy(int modelo, int indice) -> float`
Y del centro de la caja de la submalla i, en el espacio del modelo.

**Devuelve:** coordenada

#### `g3d_model_submesh_cz(int modelo, int indice) -> float`
Z del centro de la caja de la submalla i, en el espacio del modelo.

**Devuelve:** coordenada

#### `g3d_model_submesh_hx(int modelo, int indice) -> float`
Mitad del ancho (en X) de la caja de la submalla i. Con CX/CY/CZ y HY/HZ describe la caja que envuelve a un trozo.

**Devuelve:** semi-ancho

#### `g3d_model_submesh_hy(int modelo, int indice) -> float`
Mitad del alto (en Y) de la caja de la submalla i.

**Devuelve:** semi-alto

#### `g3d_model_submesh_hz(int modelo, int indice) -> float`
Mitad del fondo (en Z) de la caja de la submalla i.

**Devuelve:** semi-fondo

#### `g3d_model_spawn(int escena, int modelo, float x, float y, float z, float alto, float giro_y) -> int`
Crea en la escena un modelo completo (todas sus mallas) colgando de una entidad raiz vacia. Mover, rotar o escalar esa raiz mueve todo el modelo. El modelo se escala uniformemente para que mida 'alto' de alto (si alto <= 0 se queda 1:1). El giro va en MILESIMAS DE GRADO.

**Devuelve:** id de la entidad raiz, o -1 si el modelo no es valido

```
m = g3d_load_gltf("personaje.glb");
raiz = g3d_model_spawn(escena, m, 0, 0, 0, 1.8, 0);
```

#### `g3d_model_set_gpu_skin(int modelo, int activar) -> int`
Activa (1) o desactiva (0) el skinning por GPU del modelo.

#### `g3d_model_submesh_lod(int modelo, int indice, int rejilla) -> int`
Genera una copia simplificada de la submalla i para usarla como nivel de detalle lejano en un grupo de instancias. rejilla son las celdas a lo largo del eje mas largo (10 agresivo, 24 suave).

**Devuelve:** manejador de malla, o -1

#### `g3d_model_submesh_map(int modelo, int indice, int tipo) -> int`
Mapa de la submalla i: tipo 1 = normales, 2 = metalico, 3 = rugosidad.

**Devuelve:** manejador de textura, o -1

### mouse
#### `g3d_mouse_capture(int activo) -> int`
Activa (1) o desactiva (0) el raton relativo: oculta y bloquea el cursor y da movimientos limpios sin tope en el borde. Es lo que se usa para girar la camara en primera persona. Reinicia los incrementos.

```
g3d_mouse_capture(1);
```

#### `g3d_mouse_update() -> int`
Lee el movimiento del raton desde la ultima llamada. Llamala una vez por frame y luego usa g3d_mouse_dx() y g3d_mouse_dy().

```
g3d_mouse_update();
mirar_x += g3d_mouse_dx() * 100;
```

#### `g3d_mouse_dx() -> int`
Movimiento horizontal del raton en pixeles (positivo a la derecha) en la ultima g3d_mouse_update.

**Devuelve:** incremento en X

#### `g3d_mouse_dy() -> int`
Movimiento vertical del raton en pixeles (positivo hacia abajo) en la ultima g3d_mouse_update.

**Devuelve:** incremento en Y

### paint
#### `g3d_paint_create(int ancho, int alto) -> int`
Crea un lienzo (ancho x alto pixeles, empieza negro opaco) para pintar la textura del terreno.

**Devuelve:** manejador del lienzo

#### `g3d_paint_fill(int lienzo, int textura, float repeticion) -> int`
Rellena todo el lienzo repitiendo una textura repeticion veces (capa base) y lo sube a la GPU.

#### `g3d_paint_get_texture(int lienzo) -> int`
Textura del lienzo, para usarla como color (albedo) de un material con g3d_material_set_texture.

**Devuelve:** manejador de textura

#### `g3d_paint_save(int lienzo, string ruta) -> int`
Guarda el lienzo en un archivo de imagen.

**Devuelve:** resultado de la escritura

### particles
#### `g3d_particles_create(float x, float y, float z, float ex, float ey, float ez, float vx, float vy, float vz, float dispersion, float gravedad, float tamano, float vida, int cantidad) -> int`
Crea un emisor de particulas (puntos suaves en GPU): niebla, espuma, chispas, fuentes. Nacen dentro de la caja centrada en (x, y, z) de semi-lados (ex, ey, ez), con velocidad base (vx, vy, vz) mas una dispersion aleatoria, caen con la gravedad, viven vida segundos y se dibujan con un tamano; cantidad particulas se reciclan sin parar.

**Devuelve:** id del emisor, o -1

```
fuente = g3d_particles_create(0, 1, 0,  0.2, 0, 0.2,  0, 6, 0, 1.5,  9.8, 0.15, 1.5, 200);
g3d_particles_set_color(fuente, 0.7, 0.85, 1.0);
```

#### `g3d_particles_set_color(int emisor, float r, float g, float b) -> int`
Color de las particulas de un emisor (r, g, b de 0 a 1).

#### `g3d_particles_set_floor(int emisor, float y) -> int`
Las particulas que caen por debajo de y reaparecen (para que un chorro desaparezca al llegar al agua).

#### `g3d_particles_clear() -> int`
Quita todos los emisores.

### physics
#### `g3d_physics_body_create(int a, int b, float c, float d) -> int`
OBSOLETA: no hace nada y devuelve -1. Usa g3d_rigidbody_create y sus variantes.

**Devuelve:** -1

#### `g3d_physics_body_set_velocity(int cuerpo, float vx, float vy, float vz) -> int`
OBSOLETA: no hace nada. Usa g3d_rigidbody_set_velocity.

#### `g3d_physics_step(float dt) -> int`
OBSOLETA: no hace nada. Usa g3d_rigidbody_step.

#### `g3d_physics_set_gravity(float gravedad) -> int`
Gravedad global en unidades/s^2, positiva hacia abajo (24 por defecto). Afecta al controlador de personaje y al vehiculo y, en PC, a los cuerpos rigidos de Jolt.

```
g3d_physics_set_gravity(9.8);
```

### pick
#### `g3d_pick_terrain(int camara, float sx, float sy, float w, float h, int terreno) -> int`
Punto del terreno bajo un punto de la pantalla. Avanza el rayo y afina el impacto por biseccion. Con terreno = 0 usa un suelo plano a altura 0.

**Devuelve:** 1 si toca el terreno (punto en g3d_pick_x/y/z), 0 si no

```
if (g3d_pick_terrain(cam, mouse.x, mouse.y, 960, 540, 0))
    objetivo_x = g3d_pick_x();
    objetivo_z = g3d_pick_z();
end
```

#### `g3d_pick_entity(int camara, float sx, float sy, float w, float h) -> int`
Entidad solida que hay bajo un punto de la pantalla (sx, sy en pixeles, con w y h el tamano de la pantalla). Lanza un rayo desde la camara de hasta 5000 unidades. El punto de impacto se lee con g3d_pick_x/y/z().

**Devuelve:** id de la entidad, o -1 si no hay ninguna

```
e = g3d_pick_entity(cam, mouse.x, mouse.y, 960, 540);
if (e != -1) say("has pulsado la entidad " + e); end
```

#### `g3d_pick_x() -> float`
X del ultimo punto encontrado por g3d_pick_entity o g3d_pick_terrain.

**Devuelve:** X del punto

#### `g3d_pick_y() -> float`
Y del ultimo punto encontrado por g3d_pick_entity o g3d_pick_terrain.

**Devuelve:** Y del punto

#### `g3d_pick_z() -> float`
Z del ultimo punto encontrado por g3d_pick_entity o g3d_pick_terrain.

**Devuelve:** Z del punto

### prefab
#### `g3d_prefab_create(string nombre) -> int`
Crea un prefab vacio con nombre: un grupo reutilizable de piezas (paredes, salas) con posicion local, tamano, color, textura opcional e indicador de colision.

**Devuelve:** id del prefab, o -1

```
p = g3d_prefab_create("sala");
g3d_prefab_add_box(p, 0, 1, 0,  4, 2, 0.2,  0, 0, 0,  0.8, 0.8, 0.8, 1);
g3d_prefab_instantiate(p, escena, 10, 0, 5, 90);
```

#### `g3d_prefab_add_box(int prefab, float x, float y, float z, float tx, float ty, float tz, float rx, float ry, float rz, float r, float g, float b, int solido) -> int`
Anade una caja al prefab: posicion local (x, y, z), tamano (x, y, z), giro (grados en x, y, z), color (r, g, b) y si es solida (1) o no (0).

**Devuelve:** indice de la pieza

#### `g3d_prefab_piece_texture(int prefab, string ruta) -> int`
Da textura (una ruta de imagen repetible) a la ULTIMA pieza anadida.

#### `g3d_prefab_save(int prefab, string ruta) -> int`
Guarda el prefab en un archivo de texto.

**Devuelve:** resultado de la escritura

#### `g3d_prefab_load(string ruta) -> int`
Carga un prefab de un archivo de texto y lo crea nuevo.

**Devuelve:** id del prefab nuevo, o -1

#### `g3d_prefab_instantiate(int prefab, int escena, float x, float y, float z, float giro) -> int`
Coloca el prefab en una escena en (x, y, z) girado yaw grados: crea una entidad por pieza (con la colision marcada).

**Devuelve:** numero de piezas creadas

#### `g3d_prefab_count(int prefab) -> int`
Numero de piezas del prefab.

**Devuelve:** numero de piezas

### primitive
#### `g3d_primitive_cube() -> int`
Cubo de 1 x 1 x 1 centrado en el origen. Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del modelo, o -1

```
caja = g3d_primitive_cube();
e = g3d_entity_spawn(escena, caja, 0, 0.5, 0);
```

#### `g3d_primitive_column(int bloques) -> int`
Columna de 1 x N x 1 centrada en el origen, con la textura repetida una vez por bloque de alto (blocks menor que 1 se toma como 1). Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del modelo, o -1

#### `g3d_primitive_wedge() -> int`
Cuna (rampa) de 1 x 1 x 1 centrada en el origen: sube hacia +X (altura -0.5 en x=-0.5 y +0.5 en x=+0.5). Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del modelo, o -1

#### `g3d_primitive_sphere(int segmentos) -> int`
Esfera de radio 0.5. segmentos es la resolucion (minimo 3; 16 a 32 se ve redonda). Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del modelo, o -1

#### `g3d_primitive_plane() -> int`
Plano horizontal de 1 x 1 centrado en el origen, con la cara visible hacia arriba. Se agranda con size_x / size_z del proceso. Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del modelo, o -1

#### `g3d_primitive_terrain(int rejilla, float tamano, float alto, float tiling, int semilla) -> int`
Terreno procedural con ruido fractal: malla de (rejilla+1) x (rejilla+1) vertices centrada en el origen, en el plano XZ. Con alto da la altura maxima, tiling cuantas veces se repite la textura y semilla cambia el relieve. Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del terreno, o -1

```
t = g3d_primitive_terrain(128, 200.0, 12.0, 20.0, 7);
```

#### `g3d_primitive_cliffs(int rejilla, float tamano, float alto, float tiling, int semilla, float pendiente, float fondo_agua) -> int`
Terreno de acantilados: como g3d_primitive_terrain pero con el ruido aplastado en cuencas y mesetas. pendiente (1 suave, 4 a 6 dramatico) marca lo abrupto de los acantilados; fondo_agua (0 a 1) hunde las cuencas por debajo de 0 para que el agua las llene. Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del terreno, o -1

#### `g3d_primitive_mountain(int rejilla, float tamano, float pico, float tiling, int semilla, float canal) -> int`
Una montana: un pico radial con ruido de detalle, con base hundida bajo 0 para que el agua forme un lago alrededor. canal (0 a 1) talla una garganta en la cara -Z (para una cascada o rio). Devuelve un modelo listo para g3d_entity_spawn y ya subido a la GPU.

**Devuelve:** manejador del terreno, o -1

### ray
#### `g3d_ray_hit_x() -> float`
X del punto de impacto del ultimo g3d_raycast que acerto.

**Devuelve:** X del impacto

#### `g3d_ray_hit_y() -> float`
Y del punto de impacto del ultimo g3d_raycast que acerto.

**Devuelve:** Y del impacto

#### `g3d_ray_hit_z() -> float`
Z del punto de impacto del ultimo g3d_raycast que acerto.

**Devuelve:** Z del impacto

#### `g3d_ray_entity() -> int`
Entidad alcanzada por el ultimo g3d_raycast.

**Devuelve:** id de la entidad, o -1 si fallo

### render
#### `g3d_render_width() -> int`
Anchura real, en pixeles, a la que se dibuja la escena (distinta de la ventana si hay FSR).

**Devuelve:** pixeles

#### `g3d_render_height() -> int`
Altura real, en pixeles, a la que se dibuja la escena (distinta de la ventana si hay FSR).

**Devuelve:** pixeles

### rigidbody
#### `g3d_rigidbody_create(float x, float y, float z, float hx, float hy, float hz, float masa) -> int`
Crea un cuerpo rigido en forma de caja. hx, hy, hz son las MEDIAS dimensiones (la mitad del ancho, alto y fondo); si valen menos de 0.01 se usa 0.5. Masa 0 o negativa = cuerpo FIJO: choca con todo pero no lo mueve nada (decorado solido). Rebote por defecto 0.12 y rozamiento 0.55. En PC la fisica de cuerpos rigidos la hace Jolt; en Vita y PS5 se usa un motor propio mas simple.

**Devuelve:** id del cuerpo, o -1 si no quedan huecos

```
caja = g3d_rigidbody_create(0, 5, 0, 0.5, 0.5, 0.5, 10);
// cada frame:
g3d_rigidbody_step(dt);
```

#### `g3d_rigidbody_create_sphere(float x, float y, float z, float radio, float masa) -> int`
Crea un cuerpo rigido esferico. Un radio menor de 0.01 se cambia por 0.5. Masa 0 = fijo.

**Devuelve:** id del cuerpo

#### `g3d_rigidbody_create_capsule(float x, float y, float z, float radio, float medio_alto, float masa) -> int`
Crea un cuerpo rigido en capsula. medio_alto es la mitad del alto de la parte CILINDRICA: la altura total de la capsula es 2 * (medio_alto + radio). Valores menores de 0.01 se cambian por 0.5. Masa 0 = fijo.

**Devuelve:** id del cuerpo

#### `g3d_rigidbody_create_cylinder(float x, float y, float z, float radio, float medio_alto, float masa) -> int`
Crea un cuerpo rigido cilindrico. medio_alto es la mitad del alto total. Valores menores de 0.01 se cambian por 0.5. Masa 0 = fijo.

**Devuelve:** id del cuerpo

#### `g3d_rigidbody_create_convex(float x, float y, float z, int modelo, int submalla, float escala, float masa) -> int`
Crea un cuerpo rigido con la envolvente convexa de UNA submalla de un modelo (p. ej. un trozo de una fractura). El cuerpo se coloca en el centroide de la malla y gira sobre el. Falla (devuelve -1) si la submalla tiene menos de 4 vertices.

**Devuelve:** id del cuerpo, o -1 si falla

#### `g3d_rigidbody_set_ccd(int cuerpo, int activar) -> int`
Activa (1) la deteccion continua de colisiones del cuerpo, para que objetos pequenos y rapidos (una bala) no atraviesen paredes finas. Con 0 vuelve a la deteccion normal. Solo tiene efecto con Jolt.

#### `g3d_rigidbody_create_convex_model(float x, float y, float z, int modelo, float escala, float masa) -> int`
Crea un cuerpo rigido con la envolvente convexa de TODAS las submallas del modelo: se ajusta a su forma sin tener que dar tamanos. Es lo que hay que usar para lo que se mueve (un barril, una caja, una roca que rueda). Solo existe con Jolt: sin el, devuelve -1.

**Devuelve:** id del cuerpo, o -1 si falla

```
barril = g3d_rigidbody_create_convex_model(0, 5, 0, m_barril, 1.0, 20);
```

#### `g3d_rigidbody_destroy(int cuerpo) -> int`
Destruye un cuerpo rigido.

#### `g3d_rigidbody_clear() -> int`
Destruye todos los cuerpos rigidos.

#### `g3d_rigidbody_step(float dt) -> int`
Avanza la simulacion de fisica. Llamala una vez por frame. dt se limita a 0.05 s. Antes de simular se incorpora el terreno de la escena como colision, asi que los cuerpos chocan con el suelo sin hacer nada mas.

#### `g3d_rigidbody_apply_impulse(int cuerpo, float ix, float iy, float iz) -> int`
Aplica un impulso lineal: cambia la velocidad en impulso / masa. Despierta al cuerpo si estaba dormido (un cuerpo quieto durante un rato se duerme y, sin despertarlo, ignoraria el impulso).

#### `g3d_rigidbody_set_velocity(int cuerpo, float vx, float vy, float vz) -> int`
Fija la velocidad lineal del cuerpo. Despierta al cuerpo si dormia.

#### `g3d_rigidbody_set_damping(int cuerpo, float lineal, float angular) -> int`
Amortiguacion lineal y angular. Un valor negativo deja el actual sin cambiar.

#### `g3d_rigidbody_apply_angular_impulse(int cuerpo, float ax, float ay, float az) -> int`
Aplica un impulso de giro: hace girar al cuerpo sin moverlo de sitio. Despierta al cuerpo si dormia.

#### `g3d_rigidbody_set_upright(int cuerpo, float fuerza) -> int`
Fuerza con la que el cuerpo se endereza solo, llevando su 'arriba' hacia el 'arriba' del mundo (0 = no se endereza; los negativos cuentan como 0). Funciona con cualquier orientacion.

#### `g3d_rigidbody_set_buoyancy(int cuerpo, float agua_y, float densidad_relativa) -> int`
Flotacion. Indica el nivel Y del agua y la densidad del cuerpo RELATIVA al agua: menor que 1 flota (0.5 = flota a media agua), 1 queda a flor y mayor que 1 se hunde. 0 o menos desactiva la flotacion. El empuje se reparte por el volumen sumergido, no en el centro, asi que un cuerpo ancho y bajo aguanta, uno alto y estrecho vuelca, y un golpe fuerte tumba a cualquiera. Hay que repetir la llamada si el nivel del agua cambia.

```
g3d_rigidbody_set_buoyancy(barril, g3d_scene_water_level(), 0.6);   // flota
```

#### `g3d_rigidbody_set_bounce(int cuerpo, float rebote, float rozamiento) -> int`
Rebote (restitucion) y rozamiento, ambos de 0 a 1. Un valor negativo deja el actual sin cambiar.

#### `g3d_rigidbody_x(int cuerpo) -> float`
Posicion X del centro del cuerpo.

**Devuelve:** X

#### `g3d_rigidbody_y(int cuerpo) -> float`
Posicion Y del centro del cuerpo.

**Devuelve:** Y

#### `g3d_rigidbody_z(int cuerpo) -> float`
Posicion Z del centro del cuerpo.

**Devuelve:** Z

#### `g3d_rigidbody_grounded(int cuerpo) -> int`
CON JOLT (PC) devuelve 1 si el cuerpo esta EN REPOSO (dormido), 0 si se esta moviendo; no distingue 'apoyado en el suelo' de 'quieto en el aire'. Sin Jolt (Vita/PS5) indica apoyo en el suelo.

**Devuelve:** 1 si esta en reposo

#### `g3d_rigidbody_set_angular_velocity(int cuerpo, float wx, float wy, float wz) -> int`
Fija la velocidad angular del cuerpo. Despierta al cuerpo si dormia.

#### `g3d_rigidbody_angle_x(int cuerpo) -> float`
Orientacion del cuerpo en X (pitch), ya en MILESIMAS DE GRADO para pasarsela directamente a G3D_ENTITY_SET_ROTATION.

**Devuelve:** angulo

#### `g3d_rigidbody_angle_y(int cuerpo) -> float`
Orientacion del cuerpo en Y (yaw), en MILESIMAS DE GRADO.

**Devuelve:** angulo

#### `g3d_rigidbody_angle_z(int cuerpo) -> float`
Orientacion del cuerpo en Z (roll), en MILESIMAS DE GRADO.

**Devuelve:** angulo

#### `g3d_rigidbody_render_x(int cuerpo) -> float`
X donde dibujar un modelo con el origen en los pies para que se apoye bien en el suelo y ruede alrededor de su centro. Es la posicion del cuerpo menos el desplazamiento de modelo girado.

**Devuelve:** X

#### `g3d_rigidbody_render_y(int cuerpo) -> float`
Y donde dibujar el modelo (ver G3D_RIGIDBODY_RENDER_X).

**Devuelve:** Y

#### `g3d_rigidbody_render_z(int cuerpo) -> float`
Z donde dibujar el modelo (ver G3D_RIGIDBODY_RENDER_X).

**Devuelve:** Z

#### `g3d_rigidbody_set_model_offset(int cuerpo, float ox, float oy, float oz) -> int`
Desplazamiento entre el centro del cuerpo y el origen del modelo, que usan G3D_RIGIDBODY_RENDER_X/Y/Z. Por defecto es (0, medio_alto, 0), pensado para modelos con el origen en los pies.

```
g3d_entity_set_position(raiz, g3d_rigidbody_render_x(c), g3d_rigidbody_render_y(c), g3d_rigidbody_render_z(c));
g3d_entity_set_rotation(raiz, g3d_rigidbody_angle_x(c), g3d_rigidbody_angle_y(c), g3d_rigidbody_angle_z(c));
```

### river
#### `g3d_river_begin(float ancho, float elevacion) -> int`
Empieza a definir un rio: ancho del cauce (si es 0.1 o menos se usa 3) y cuanto sube el agua sobre el lecho. A continuacion da los puntos con g3d_river_point y termina con g3d_river_end.

```
g3d_river_begin(4.0, 0.3);
g3d_river_point(-80, -60);
g3d_river_point(-20, -10);
g3d_river_point(40, 30);
g3d_river_end();
```

#### `g3d_river_point(float x, float z) -> int`
Anade un punto (x, z) al rio que se esta definiendo; la altura sale del terreno. El agua corre en el orden en que das los puntos.

**Devuelve:** 1 si lo anadio, 0 si ya no caben mas puntos

#### `g3d_river_block() -> int`
Marca el rio que se esta definiendo (begin/point) en la mascara que impide que los lagos lo inunden, SIN construirlo. Llamala antes de crear los lagos.

#### `g3d_river_falls() -> int`
Anade solo las cascadas del rio definido (begin/point), para el camino COMPLETO sin recortar, despues de haber creado su superficie con otra secuencia.

#### `g3d_river_end() -> int`
Construye el rio con los puntos dados, siguiendo el terreno, con flujo animado y cascadas donde el cauce cae un desnivel fuerte.

**Devuelve:** resultado de la construccion

### road
#### `g3d_road_create(int cerrada) -> int`
Crea una carretera vacia. Una carretera es SOLO informacion de por donde se puede circular: una lista de nodos unidos por curvas suaves. NO se dibuja ni cambia el suelo: el aspecto es el de tu terreno pintado o tu modelo (.glb), y el suelo el de tu terreno o tu colision. Todo lo demas (salida, meta, vueltas, rivales) lo hace el juego. Maximo 8 carreteras. Las que dibujas en el editor con la herramienta Carretera ya vienen creadas (id 0, 1...) al empezar tu codigo.

**Devuelve:** id de la carretera, o -1 si no quedan huecos

```
carretera = g3d_road_create(1);   // 1 = circuito cerrado, 0 = abierta
g3d_road_add_node(carretera, 0, 0, 0, 12, 0, 0, 0, 0);
g3d_road_add_node(carretera, 60, 0, 0, 12, 0, 0, 0, 0);
g3d_road_add_node(carretera, 60, 60, 0, 12, 0, 0, 0, 0);
g3d_road_build(carretera);
```

#### `g3d_road_destroy(int carretera) -> int`
Destruye una carretera.

#### `g3d_road_clear() -> int`
Destruye todas las carreteras. g3d_scene_load tambien las borra antes de cargar las de la escena.

#### `g3d_road_add_node(int carretera, float x, float z, float y, float ancho, float peralte_grados, int sigue_terreno, float dy, int superficie) -> int`
Anade un nodo a la carretera. Las curvas entre nodos son suaves (tangentes Catmull-Rom). Ancho, peralte, altura y dy se interpolan hasta el nodo siguiente. Con sigue=1 la carretera va pegada al terreno (altura = terreno + dy) y 'y' se ignora; con sigue=0 va a la altura fija 'y'. La superficie es una etiqueta del tramo que SALE de este nodo: 0 asfalto, 1 tierra, 2 hielo, 3 barro, o lo que decida tu juego. Un peralte positivo sube el borde derecho (curvas a la izquierda). Maximo 512 nodos por carretera.

**Devuelve:** indice del nodo, o -1

#### `g3d_road_set_tangent(int carretera, int nodo, float tx, float tz) -> int`
Tangente propia de un nodo: la direccion y la 'fuerza' con la que la curva pasa por el. (0, 0) = automatica (la normal: la curva sale de los nodos vecinos). Sirve para curvas muy cerradas, o para reproducir un trazado dibujado en el editor, que guarda la tangente de los nodos que el usuario ha arrastrado. Hay que llamarla antes de G3D_ROAD_BUILD.

#### `g3d_road_build(int carretera) -> int`
Calcula la linea central de la carretera. Hay que llamarla despues de anadir los nodos y, si algun nodo sigue el terreno, DESPUES de cargar el terreno. g3d_scene_load ya la llama por ti para las carreteras de la escena. Una carretera cerrada necesita al menos 3 nodos; una abierta, 2.

**Devuelve:** numero de muestras de la linea central (unas cada 2 m), 0 si no hay trazado valido

#### `g3d_road_count() -> int`
Numero de carreteras ya calculadas.

**Devuelve:** cantidad

#### `g3d_road_nodes(int carretera) -> int`
Numero de nodos de una carretera.

**Devuelve:** cantidad

#### `g3d_road_closed(int carretera) -> int`
Indica si la carretera es un circuito cerrado.

**Devuelve:** 1 si es cerrada

#### `g3d_road_length(int carretera) -> float`
Longitud de la linea central de la carretera, en metros (unidades de mundo), contando la vuelta entera si es cerrada.

**Devuelve:** longitud

#### `g3d_road_x(int carretera, float distancia) -> float`
Posicion X del punto de la linea central situado a 'distancia' metros del principio. En una carretera cerrada la distancia da la vuelta (y admite negativos); en una abierta se recorta a los extremos.

**Devuelve:** X

#### `g3d_road_y(int carretera, float distancia) -> float`
Altura del punto de la carretera a 'distancia' metros del principio (ver G3D_ROAD_X).

**Devuelve:** Y

#### `g3d_road_z(int carretera, float distancia) -> float`
Posicion Z del punto de la carretera a 'distancia' metros del principio (ver G3D_ROAD_X).

**Devuelve:** Z

#### `g3d_road_yaw(int carretera, float distancia) -> float`
Hacia donde mira la carretera en ese punto, como rumbo en MILESIMAS DE GRADO: 0 mira hacia +Z y crece hacia +X (adelante = (sin rumbo, cos rumbo)), igual que el rumbo de un vehiculo. Vista desde una camara detras del coche (mirando a +Z), un rumbo creciente es un giro a la izquierda.

**Devuelve:** rumbo

#### `g3d_road_width(int carretera, float distancia) -> float`
Ancho de la carretera en ese punto, en metros.

**Devuelve:** ancho

#### `g3d_road_surface(int carretera, float distancia) -> int`
Etiqueta de superficie del tramo en ese punto (la que se puso en G3D_ROAD_ADD_NODE).

**Devuelve:** superficie

#### `g3d_road_nearest(float x, float z) -> int`
Busca el punto de carretera mas cercano a (x, z), de entre todas las carreteras, y guarda el resultado para las funciones G3D_ROAD_NEAR_*. Es la base de casi todo: en que vuelta va un coche, quien va primero, hacia donde apunta un rival o si se ha salido de la pista.

**Devuelve:** id de la carretera mas cercana, o -1 si no hay ninguna

```
g3d_road_nearest(g3d_vehicle_x(v), g3d_vehicle_z(v));
IF (NOT g3d_road_near_on())
    say("Fuera de la pista");
END
```

#### `g3d_road_near_dist() -> float`
De la ultima G3D_ROAD_NEAREST: distancia recorrida a lo largo de la carretera del punto mas cercano. Con ella se sabe en que vuelta va un coche (salta de casi 'largo' a casi 0 al cruzar la meta) o quien va por delante.

**Devuelve:** distancia en metros

#### `g3d_road_near_offset() -> float`
De la ultima G3D_ROAD_NEAREST: separacion lateral al centro de la carretera. Positiva hacia la izquierda de quien avanza por ella (hacia +X cuando el rumbo es 0), negativa hacia la derecha.

**Devuelve:** metros

#### `g3d_road_near_y() -> float`
De la ultima G3D_ROAD_NEAREST: altura de la carretera en ese punto.

**Devuelve:** altura

#### `g3d_road_near_width() -> float`
De la ultima G3D_ROAD_NEAREST: ancho de la carretera en ese punto.

**Devuelve:** ancho en metros

#### `g3d_road_near_surface() -> int`
De la ultima G3D_ROAD_NEAREST: etiqueta de superficie del tramo.

**Devuelve:** superficie

#### `g3d_road_near_on() -> int`
De la ultima G3D_ROAD_NEAREST: indica si el punto esta dentro del ancho de la carretera (separacion lateral menor o igual que la mitad del ancho).

**Devuelve:** 1 si esta en la carretera

#### `g3d_road_near_yaw() -> float`
De la ultima G3D_ROAD_NEAREST: rumbo de la carretera en ese punto, en MILESIMAS DE GRADO (ver G3D_ROAD_YAW).

**Devuelve:** rumbo

#### `g3d_road_show(int carretera, int escena) -> int`
OPCIONAL: dibuja la carretera, creando su malla (una por superficie) como entidades de la escena. Normalmente NO se usa: tu carretera ya esta pintada en el terreno o es un modelo. Sirve para ver una carretera inventada por codigo (como en las guias) o para depurar. No la vuelve suelo: para eso G3D_ROAD_SET_GROUND. Hay que llamarla despues de G3D_ROAD_BUILD. Cada superficie tiene un color de serie (asfalto gris oscuro, tierra marron, hielo azul claro, barro marron oscuro).

**Devuelve:** numero de entidades creadas (una por superficie usada)

```
g3d_road_build(carretera);
g3d_road_show(carretera, escena);
```

#### `g3d_road_set_ground(int carretera, int activar) -> int`
Hace que la carretera cuente como SUELO (para el vehiculo, el personaje y las colisiones, via G3D_SCENE_TERRAIN_HEIGHT): dentro de su ancho, la altura es la de la carretera, con su peralte. Por defecto NO lo es (0): una carretera marcada en el editor solo dice por donde se puede circular y el suelo es el de tu terreno o tu modelo. Util para carreteras inventadas por codigo junto a G3D_ROAD_SHOW. Los cuerpos rigidos de Jolt no la notan.

#### `g3d_road_hide(int carretera) -> int`
Quita el dibujo de la carretera (sigue existiendo y siendo suelo).

#### `g3d_road_entity(int carretera, int superficie) -> int`
Entidad con la que se dibuja una superficie de la carretera. Sirve para cambiar su aspecto con las funciones de siempre: g3d_entity_set_material, g3d_material_set_texture, g3d_entity_set_color... La textura se repite cada 8 m a lo largo de la carretera.

**Devuelve:** id de la entidad, o -1 si la carretera no tiene esa superficie

```
// una textura de asfalto en la superficie 0
m = g3d_material_create();
g3d_material_set_texture(m, 0, g3d_load_texture("asfalto.png"));
g3d_entity_set_material(g3d_road_entity(carretera, 0), m);
```

#### `g3d_road_set_color(int superficie, float r, float g, float b) -> int`
Cambia el color de serie de una superficie (de 0 a 7), con componentes de 0.0 a 1.0. Afecta tambien a las carreteras ya dibujadas.

### rope
#### `g3d_rope_create(float ax, float ay, float az, float bx, float by, float bz, int segmentos, float grosor, float holgura) -> int`
Crea una cuerda entre dos puntos: una fila de segmentos particulas dibujada como un tubo (se ve desde cualquier angulo) de radio grosor. holgura es cuanto mas larga es que la distancia entre los extremos: 0 = tirante, 0.3 = cuelga bastante. Nace con los dos extremos sujetos. Sirve para cables, lianas, puentes y para colgar telas. Es una simulacion de Verlet: llama a la funcion update cada frame para que se mueva.

**Devuelve:** id de la cuerda, o -1

```
cuerda = g3d_rope_create(-3, 4, 0,  3, 4, 0,  16, 0.05, 0.25);
// cada frame:
g3d_rope_update(cuerda, 1.0 / 60);
```

#### `g3d_rope_pin(int cuerda, int extremo, int fijo) -> int`
Fija (1) o suelta (0) un extremo de la cuerda: extremo 0 = el primero, 1 = el ultimo.

```
g3d_rope_pin(cuerda, 1, 0);   // suelta el extremo final
```

#### `g3d_rope_pin_move(int cuerda, int punto, float x, float y, float z) -> int`
Clava un punto cualquiera de la cuerda (0 = el primero) en una posicion: un gancho que se mueve. Llamala cada frame.

#### `g3d_rope_set_texture(int cuerda, int textura_gl) -> int`
Textura del tubo de la cuerda. OJO: este parametro es el identificador de textura de OpenGL, no el manejador de g3d_load_texture.

#### `g3d_rope_points(int cuerda) -> int`
Numero de puntos de la cuerda (los validos para los demas comandos van de 0 a ese numero menos 1).

**Devuelve:** numero de puntos

#### `g3d_rope_x(int cuerda, int punto) -> float`
Coordenada X actual del punto n de la cuerda, para colgar cosas de el.

**Devuelve:** X

#### `g3d_rope_y(int cuerda, int punto) -> float`
Coordenada Y actual del punto n de la cuerda.

**Devuelve:** Y

#### `g3d_rope_z(int cuerda, int punto) -> float`
Coordenada Z actual del punto n de la cuerda.

**Devuelve:** Z

```
mitad = g3d_rope_points(cuerda) / 2;
g3d_cloth_pin_move(tela, 0, 0, g3d_rope_x(cuerda, mitad), g3d_rope_y(cuerda, mitad), g3d_rope_z(cuerda, mitad));
```

#### `g3d_rope_update(int cuerda, float dt) -> int`
Avanza la simulacion de la cuerda dt segundos. Una vez por frame.

#### `g3d_rope_load(int cuerda, int punto, float peso) -> int`
Cuelga peso de un punto de la cuerda: se comba ahi. Hay que repetirlo CADA FRAME mientras siga colgado; si se deja de llamar, la cuerda se recupera sola.

#### `g3d_rope_set_wind(int cuerda, float x, float y, float z, float fuerza) -> int`
Viento sobre la cuerda: direccion (x, y, z) y fuerza.

#### `g3d_rope_destroy(int cuerda) -> int`
Elimina la cuerda.

### scatter
#### `g3d_scatter_clear() -> int`
Quita toda la vegetacion sembrada y sus grupos de instancias.

#### `g3d_scatter_load(string ruta, float viento) -> int`
Carga de un archivo de siembra (el que guarda el editor) todas las colocaciones y monta los grupos de instancias. Es lo que llama un juego generado. Si el archivo no existe no es un error: la escena simplemente no tiene vegetacion. viento multiplica el balanceo de cada especie (1 = como se diseno).

**Devuelve:** numero de colocaciones cargadas, o 0

```
g3d_scatter_load("Scenes/bosque.scatter", 1.0);
```

#### `g3d_scatter_group(string recurso) -> int`
Indice de una especie de la siembra a partir de su recurso (por ejemplo "Assets/pino.glb"); la crea si no existe. Es lo que une un proceso a su bosque (se guarda en entity).

**Devuelve:** indice de la especie

#### `g3d_scatter_model(int modelo, int terreno, int cantidad, float area, float alto, float variacion, float viento, int semilla) -> int`
Siembra copias de un modelo entero sobre el terreno: cantidad, cuadrado de semi-lado area, altura objetivo a la que se escala cada copia, variacion de escala (escala base por 1 +/- variacion), viento y semilla.

**Devuelve:** id del primer grupo

#### `g3d_scatter_mesh(int malla, int textura, int terreno, int cantidad, float area, float esc_min, float esc_max, float viento, int semilla) -> int`
Siembra copias de una malla al azar sobre el terreno: cuantas, en un cuadrado de semi-lado area, con giro aleatorio y escala entre esc_min y esc_max, con balanceo por viento y una semilla para repetir el mismo resultado.

**Devuelve:** id del primer grupo

### scene
#### `g3d_scene_create(string nombre) -> int`
Crea una escena vacia.

**Devuelve:** id de la escena

#### `g3d_scene_destroy(int escena) -> int`
Destruye una escena.

#### `g3d_scene_set_active(int escena) -> int`
Hace activa una escena: es la que se renderiza.

#### `g3d_scene_load(string fichero) -> int`
Carga un mapa .lvl (terreno, luces, entidades, camara), crea la escena y la deja activa.

**Devuelve:** id de la escena, o -1 si el fichero no existe

```
escena = g3d_scene_load("mapa.lvl");
```

#### `g3d_scene_terrain_height(float x, float z) -> float`
Altura del terreno de la escena cargada en el punto (x, z). Sobre terreno de voxeles usa la superficie real esculpida.

**Devuelve:** altura en Y; 0 si no hay terreno

#### `g3d_scene_water_level() -> float`
Altura Y del agua de la escena cargada.

**Devuelve:** nivel del agua

#### `g3d_scene_has_spawn() -> int`
Indica si el mapa cargado define un punto de inicio del jugador.

**Devuelve:** 1 si existe

#### `g3d_scene_spawn_x() -> float`
Coordenada X del punto de inicio del jugador.

**Devuelve:** X

#### `g3d_scene_spawn_y() -> float`
Coordenada Y del punto de inicio del jugador.

**Devuelve:** Y

#### `g3d_scene_spawn_z() -> float`
Coordenada Z del punto de inicio del jugador.

**Devuelve:** Z

#### `g3d_scene_player_radius() -> float`
Radio de colision del jugador definido en el mapa.

**Devuelve:** radio

#### `g3d_scene_player_height() -> float`
Altura del jugador definida en el mapa.

**Devuelve:** altura

#### `g3d_scene_player_climb() -> float`
Altura maxima de escalon que puede subir el jugador, definida en el mapa.

**Devuelve:** altura de escalon

### set
#### `g3d_set_sun(float t) -> int`
Ciclo dia/noche en un solo numero: t da una vuelta completa cada 1.0 (0 = amanecer, 0.25 = mediodia, 0.5 = atardecer, 0.75 = medianoche). Mueve la luz direccional principal de la escena (direccion, color calido cerca del horizonte, intensidad de 0 de noche a 1.6 al mediodia) y ajusta la luz ambiente. Necesita que la escena tenga una luz direccional. Para que los reflejos sigan al sol llama despues a g3d_ibl_refresh.

```
hora += 0.0005;
g3d_set_sun(hora);
g3d_ibl_refresh();
```

#### `g3d_set_clear_color(float r, float g, float b, float alpha) -> int`
Color de fondo que se pinta antes de dibujar la escena (r, g, b, alpha entre 0 y 1). Solo se ve donde no hay cielo ni objetos; con un cielo activo apenas importa.

#### `g3d_set_ambient_light(float r, float g, float b, float intensidad) -> int`
Luz ambiente: la que llega a todas partes por igual, incluso a las sombras. Color r, g, b entre 0 y 1 e intensidad.

```
g3d_set_ambient_light(0.4, 0.42, 0.46, 1.0);
```

#### `g3d_set_fog(int activo, float r, float g, float b, float inicio, float fin) -> int`
Niebla lineal. Los objetos a distancia menor que inicio no se tocan y a distancia fin quedan totalmente del color de la niebla. Pon el mismo color que el horizonte del cielo para que se fundan. activo: 1 o 0.

```
g3d_set_fog(1, 0.75, 0.85, 0.95, 40.0, 220.0);
```

#### `g3d_set_wireframe_mode(int activo) -> int`
Dibuja solo las aristas de los triangulos (1) o normal (0). Sirve para depurar mallas.

#### `g3d_set_shadows(int activo) -> int`
Activa (1) o desactiva (0) las sombras proyectadas de la luz direccional. Desactivarlas ahorra mucho rendimiento.

```
g3d_set_shadows(1);
```

#### `g3d_set_shadow_resolution(int resolucion) -> int`
Tamano del mapa de sombras en pixeles por lado. Se limita entre 512 y 8192. Mayor tamano = sombras mas nitidas pero mas lentas y con mas memoria; 2048 o 4096 es un buen equilibrio.

```
g3d_set_shadow_resolution(4096);
```

#### `g3d_set_hdr(int activo) -> int`
Activa (1) o desactiva (0) la tuberia HDR de postproceso (exposicion, bloom, SSAO y tonemap dependen de ella). Viene activada.

#### `g3d_set_ibl(int activo, float intensidad) -> int`
Iluminacion basada en imagen: el cielo se captura y se usa como luz ambiente y reflejos reales sobre materiales metalicos y brillantes. activo: 1 o 0; intensidad escala el efecto (1.0 = la luz del propio cielo). Si cambias el cielo en tiempo de ejecucion usa g3d_ibl_refresh.

```
g3d_set_ibl(1, 1.0);
```

#### `g3d_set_occlusion(int activo) -> int`
Activa (1) o desactiva (0) el descarte por oclusion: no se dibuja lo que queda totalmente oculto tras otra geometria. Ayuda en interiores y ciudades.

#### `g3d_set_smaa(int activo) -> int`
Activa (1) o desactiva (0) el antialiasing SMAA de 1 pasada (suaviza los bordes dentados). Es tambien requisito del escalador FSR.

```
g3d_set_smaa(1);
```

#### `g3d_set_fsr(int activo, float escala, float nitidez) -> int`
Escalado FSR 1: dibuja la escena a una fraccion del tamano de la pantalla y la amplia con nitidez. escala 1.0 = nativo, 0.667 = 'calidad' (1080p desde 720p), 0.5 = 'rendimiento'. nitidez regula el afilado. Con activo = 0 vuelve al tamano nativo. Conviene tener SMAA activado.

```
g3d_set_smaa(1);
g3d_set_fsr(1, 0.667, 0.2);
```

#### `g3d_set_fsr_height(int activo, int alto, float nitidez) -> int`
Como g3d_set_fsr pero indicando la altura interna de render en pixeles (por ejemplo 720), en vez de una fraccion; asi sigue valiendo si cambias el tamano de la ventana. Nunca renderiza por encima de la pantalla.

```
g3d_set_smaa(1);
g3d_set_fsr_height(1, 720, 0.2);
```

#### `g3d_set_exposure(float exposicion) -> int`
Exposicion del HDR: multiplica el brillo final (1.0 por defecto; los negativos se toman como 0). Sube para escenas oscuras, baja para quemadas.

```
g3d_set_exposure(0.8);
```

#### `g3d_set_bloom(int activo, float cantidad, float umbral) -> int`
Resplandor de las zonas muy brillantes. activo 1/0; cantidad es cuanto se suma (0.5 por defecto); umbral es el brillo a partir del cual brilla (0.8 por defecto). Con valores negativos conserva el valor anterior.

```
g3d_set_bloom(1, 0.5, 0.8);
```

#### `g3d_set_tonemap(int modo) -> int`
Curva de color del HDR: 0 = simple (recorta a 1.0), 1 = ACES (cinematografico, conserva el detalle de las luces fuertes).

```
g3d_set_tonemap(1);
```

#### `g3d_set_ssao(int activo, float radio, float fuerza) -> int`
Oclusion ambiental en espacio de pantalla: oscurece las esquinas y rincones. activo 1/0; radio (0.5 por defecto) y fuerza (1.2 por defecto) solo cambian si das un valor mayor que 0.

```
g3d_set_ssao(1, 0.5, 1.2);
```

#### `g3d_set_underwater(int activo, float r, float g, float b, float fuerza) -> int`
Filtro de bajo el agua en toda la pantalla: tinte r, g, b y fuerza (si es 0 o menos se usa 1). Activalo cuando la camara este por debajo de la superficie del agua.

```
g3d_set_underwater(cam_y < nivel_agua, 0.1, 0.35, 0.5, 1.0);
```

#### `g3d_set_lod(float distancia) -> int`
Distancia a partir de la cual los objetos instanciados (g3d_instances_*) pasan a su version simplificada generada automaticamente. 0 = sin LOD.

#### `g3d_set_culling(int activo) -> int`
Activa (1) o desactiva (0) el descarte de objetos fuera del campo de vision de la camara. Viene activado.

#### `g3d_set_backface_cull(int activo) -> int`
Activa (1) o desactiva (0) el descarte de las caras traseras de los triangulos. Desactivarlo se ve util en hojas o telas de una sola cara.

#### `g3d_set_spawn_parent(int entidad) -> int`
Padre de creacion: lo que crees a continuacion (entidades, sprites...) queda colgado de esta entidad y se mueve con ella. Pasa -1 para volver a crear sin padre.

```
g3d_set_spawn_parent(coche);
rueda = g3d_entity_spawn(escena, m_rueda, 1, 0, 1.5);
g3d_set_spawn_parent(-1);
```

#### `g3d_set_terrain_collider(int terreno) -> int`
Registra una malla de terreno (de g3d_primitive_terrain, ya modelada con g3d_terrain_load) como suelo de colision, para que cuerpos rigidos y personajes se apoyen en el relieve. Sin esto solo tenian colision los terrenos cargados con una escena de disco.

**Devuelve:** 1 si lo registro, 0 si no es un terreno valido

```
t = g3d_primitive_terrain(128, 200.0, 12.0, 20.0, 7);
g3d_set_terrain_collider(t);
```

### sky
#### `g3d_sky_set_gradient(float cenit_r, float cenit_g, float cenit_b, float horizonte_r, float horizonte_g, float horizonte_b) -> int`
Pone un cielo degradado: color del cenit (arriba) y del horizonte, cada uno con r, g, b entre 0 y 1, y lo activa. Sin cielo la pantalla sale NEGRA. El cielo tambien es la luz ambiente (reflejos), asi que cambiarlo recalcula la iluminacion.

```
g3d_sky_set_gradient(0.25, 0.45, 0.85,  0.75, 0.85, 0.95);
g3d_sky_enable(1);
```

#### `g3d_sky_set_clouds(float cantidad, float velocidad) -> int`
Nubes altas del cielo: cantidad entre 0 (despejado) y 1 (cubierto) y velocidad (si es 0 o menos se usa 1).

```
g3d_sky_set_clouds(0.4, 1.0);
```

#### `g3d_sky_set_low_clouds(float cobertura, float base, float grosor, float velocidad) -> int`
Nubes bajas volumetricas: cobertura entre 0 y 1 (0 = sin nubes), altura de la base en unidades del mundo, grosor (minimo 1) y velocidad (si es 0 o menos se usa 1). Tambien proyectan sombra sobre la escena.

#### `g3d_sky_set_texture(int textura) -> int`
Usa una textura (de g3d_load_texture) como cielo en lugar del degradado. 0 quita la textura.

#### `g3d_sky_enable(int activo) -> int`
Activa (1) o desactiva (0) el dibujado del cielo.

```
g3d_sky_enable(1);
```

### sprite
#### `g3d_sprite_create(int escena, float x, float y, float z) -> int`
Crea un sprite 2D dentro del mundo 3D (estilo HD-2D). Es un quad que se dibuja en el pase opaco con recorte alfa, asi que tapa y es tapado por la geometria 3D sin ordenar nada. La textura sale de un grafico normal de BennuGD2, por lo que se anima como en 2D, cambiando el grafico.

**Devuelve:** id del sprite

#### `g3d_sprite_destroy(int sprite) -> int`
Destruye un sprite.

#### `g3d_sprite_set_height(int sprite, float alto) -> int`
Fija el alto del sprite en unidades de mundo.

#### `g3d_sprite_set_pixels_per_unit(int sprite, float pixeles_por_unidad) -> int`
Cuantos pixeles del grafico entran en una unidad de mundo. Es lo comodo para pixel art: el tamano sale del propio grafico.

#### `g3d_sprite_set_anchor(int sprite, float ax, float ay) -> int`
Punto del grafico que se coloca en (x, y, z), de 0 a 1. (0.5, 1.0) = centro-abajo: los pies del personaje sobre el suelo.

#### `g3d_sprite_set_billboard(int sprite, int modo) -> int`
Como se encara a la camara: G3D_SPRITE_UPRIGHT (0) gira solo en horizontal y sigue de pie (personajes); G3D_SPRITE_FACING (1) mira siempre de frente (objetos, efectos).

#### `g3d_sprite_set_cutout(int sprite, float umbral) -> int`
Umbral de alfa: los pixeles con alfa por debajo se descartan y dejan un agujero.

#### `g3d_sprite_set_shadow(int sprite, int activar) -> int`
Activa (1) o desactiva (0) que el sprite proyecte sombra: el mismo quad con el mismo recorte alfa entra en el mapa de sombras.

#### `g3d_sprite_dir(int sprite, float angulo, int ndirs) -> int`
Que postura dibujar segun desde donde se mira: 0 = de frente a la camara, y va girando hasta ndirs-1. Con esto el .prg elige el grafico de entre sus N posturas. El angulo va en MILESIMAS DE GRADO, como el local 'angle'.

**Devuelve:** indice de la postura (0 .. ndirs-1)

#### `g3d_sprite_set_smooth(int sprite, int activar) -> int`
Filtrado de la textura. Por defecto es nearest (pixel art); con 1 es bilineal suavizado.

#### `g3d_sprite_set_lit(int sprite, int activar) -> int`
Con 1 (por defecto) le afecta la luz de la escena, de modo que se apaga de noche y con el ciclo de dia en vez de ir siempre a pleno brillo.

#### `g3d_sprite_set_snap(int sprite, int activar) -> int`
Con 1 (por defecto) la posicion en pantalla se redondea a pixeles enteros, para que el pixel art no tiemble al moverse.

#### `g3d_sprite_set_cell(int sprite, int x, int y, int ancho, int alto) -> int`
Recorte en pixeles dentro del grafico, para hojas de sprites irregulares (el editor saca estas medidas al detectar los fotogramas).

### stream
#### `g3d_stream_init(float tam_tesela, int radio) -> int`
Prepara el streaming de un mundo grande: el mundo se divide en teselas cuadradas de tam_tesela unidades y se mantienen las que estan a menos de radio teselas de la camara (un cuadrado de (2*radio+1) por lado). El motor solo lleva la logica espacial; tu juego decide que contiene cada tesela y la crea o destruye.

```
g3d_stream_init(64.0, 3);
```

#### `g3d_stream_update(float camx, float camz) -> int`
Recalcula las teselas cargadas para la camara en (camx, camz). Llamala cada frame (es barata: las listas solo cambian al cruzar el limite de una tesela). Deja en las listas de carga y descarga las teselas que han cambiado en esta llamada.

```
g3d_stream_update(cam_x, cam_z);
for (i = 0; i < g3d_stream_load_count(); i++)
    crear_tesela(g3d_stream_load_x(i), g3d_stream_load_z(i));
end
for (i = 0; i < g3d_stream_unload_count(); i++)
    borrar_tesela(g3d_stream_unload_x(i), g3d_stream_unload_z(i));
end
```

#### `g3d_stream_load_count() -> int`
Numero de teselas que ENTRARON en el anillo en la ultima g3d_stream_update (hay que crearlas).

**Devuelve:** numero de teselas

#### `g3d_stream_load_x(int i) -> int`
Coordenada X (de tesela, no del mundo) de la tesela i de la lista de carga.

**Devuelve:** X de tesela

#### `g3d_stream_load_z(int i) -> int`
Coordenada Z (de tesela, no del mundo) de la tesela i de la lista de carga.

**Devuelve:** Z de tesela

#### `g3d_stream_unload_count() -> int`
Numero de teselas que SALIERON del anillo en la ultima g3d_stream_update (hay que liberarlas).

**Devuelve:** numero de teselas

#### `g3d_stream_unload_x(int i) -> int`
Coordenada X de tesela de la tesela i de la lista de descarga.

**Devuelve:** X de tesela

#### `g3d_stream_unload_z(int i) -> int`
Coordenada Z de tesela de la tesela i de la lista de descarga.

**Devuelve:** Z de tesela

#### `g3d_stream_loaded_count() -> int`
Numero de teselas que estan ahora dentro del anillo.

**Devuelve:** numero de teselas

### terrain
#### `g3d_terrain_paint(int terreno, int lienzo, int textura, float repeticion, float x, float z, float radio, float opacidad) -> int`
Pinta una textura sobre el lienzo del terreno con un pincel: dentro de radio unidades de (x, z), con caida suave y opacidad de 0 a 1. La textura se repite src_tiling veces por todo el terreno.

```
lienzo = g3d_paint_create(1024, 1024);
g3d_paint_fill(lienzo, tex_hierba, 40.0);
g3d_terrain_paint(t, lienzo, tex_roca, 40.0, 20, 5, 6.0, 0.8);
```

#### `g3d_terrain_get_height(int terreno, float x, float z) -> float`
Altura del terreno en (x, z) interpolando entre vertices. Util para colocar objetos y la camara sobre el suelo.

**Devuelve:** altura, o 0 si no es un terreno o esta fuera

```
py = g3d_terrain_get_height(t, px, pz);
```

#### `g3d_terrain_raise(int terreno, float x, float z, float radio, float fuerza) -> int`
Pincel de escultura: sube (fuerza positiva) o baja (negativa) el terreno dentro de radio unidades de (x, z), con caida suave hacia el borde. El terreno debe haberse creado con g3d_primitive_terrain (o ser compatible). Recalcula normales y actualiza la GPU, asi que se ve al instante.

```
g3d_terrain_raise(t, 10, 10, 8.0, 0.5);
```

#### `g3d_terrain_hole(int terreno, float x, float z, float radio, int activo) -> int`
Quita (1) o restaura (0) las celdas del terreno dentro de un disco, para ver a traves del suelo (una cueva de voxels debajo, una entrada).

**Devuelve:** 1 si cambio algo

#### `g3d_terrain_smooth(int terreno, float x, float z, float radio, float cantidad) -> int`
Alisa el terreno acercando cada altura a la media de sus vecinas dentro del radio. cantidad (0 a 1) es la mezcla en el centro. El terreno debe haberse creado con g3d_primitive_terrain (o ser compatible). Recalcula normales y actualiza la GPU, asi que se ve al instante.

#### `g3d_terrain_flatten(int terreno, float x, float z, float radio, float altura, float cantidad) -> int`
Aplana el terreno hacia una altura objetivo dentro del radio. cantidad (0 a 1) es la mezcla. El terreno debe haberse creado con g3d_primitive_terrain (o ser compatible). Recalcula normales y actualiza la GPU, asi que se ve al instante.

```
g3d_terrain_flatten(t, 0, 0, 12.0, 2.0, 1.0);   // plataforma a altura 2
```

#### `g3d_terrain_load(int terreno, string ruta) -> int`
Aplica al terreno el relieve guardado por el editor (un archivo con el numero de vertices y la altura Y de cada uno). Permite que un juego reproduzca el relieve esculpido.

**Devuelve:** 1 si lo cargo; 0 si falta el archivo o el numero de vertices no coincide

```
t = g3d_primitive_terrain(128, 200.0, 12.0, 20.0, 7);
g3d_terrain_load(t, "Scenes/mundo.terrain");
g3d_set_terrain_collider(t);
```

### tr
#### `g3d_tr_probe(string ruta) -> int`
Indica de que juego de Tomb Raider es un archivo (1 a 5) sin cargarlo.

**Devuelve:** 1..5, o 0 si no es un nivel de Tomb Raider

### vehicle
#### `g3d_vehicle_create(float x, float y, float z, float rumbo) -> int`
Crea un vehiculo arcade (direccion tipo bicicleta y seguimiento del terreno con 4 ruedas). Valores por defecto: batalla 4.0, via 2.4, motor 26, velocidad maxima 55, giro maximo 32 grados, freno 40, resistencia 0.8. El rumbo va en MILESIMAS DE GRADO. Maximo 8 vehiculos.

**Devuelve:** id del vehiculo, o -1 si no quedan huecos

#### `g3d_vehicle_destroy(int vehiculo) -> int`
Destruye un vehiculo.

#### `g3d_vehicle_update(int vehiculo, float dt, float acelerador, float direccion, float freno) -> int`
Avanza el vehiculo. acelerador de -1 a 1 (negativo = marcha atras), direccion de -1 a 1 y freno de 0 a 1 (los valores fuera de rango se recortan). Una direccion POSITIVA hace crecer el rumbo, es decir, gira hacia +X: a la izquierda de la pantalla si la camara va detras mirando a +Z. Con la velocidad la direccion pierde hasta un 40 % de autoridad. Solo acelera con las ruedas en el suelo. dt se limita a 0.1 s.

```
g3d_vehicle_update(coche, dt, acel, giro, freno);
```

#### `g3d_vehicle_set_geometry(int vehiculo, float batalla, float via, float altura) -> int`
Dimensiones: distancia entre ejes (batalla), distancia entre ruedas de un mismo eje (via) y altura de la suspension. Los valores demasiado pequenos se ignoran (batalla y via deben ser mayores que 0.1; la altura, 0 o mas).

#### `g3d_vehicle_set_tuning(int vehiculo, float motor, float vel_maxima, float giro_max_grados, float freno, float resistencia) -> int`
Ajuste de conduccion: fuerza del motor, velocidad maxima, giro maximo en GRADOS, fuerza de frenado y resistencia. Un valor 0 o negativo deja el actual sin cambiar.

#### `g3d_vehicle_x(int vehiculo) -> float`
Posicion X del centro del chasis.

**Devuelve:** X

#### `g3d_vehicle_y(int vehiculo) -> float`
Posicion Y del centro del chasis.

**Devuelve:** Y

#### `g3d_vehicle_z(int vehiculo) -> float`
Posicion Z del centro del chasis.

**Devuelve:** Z

#### `g3d_vehicle_yaw(int vehiculo) -> float`
Rumbo del vehiculo, en MILESIMAS DE GRADO.

**Devuelve:** angulo

#### `g3d_vehicle_pitch(int vehiculo) -> float`
Inclinacion adelante/atras segun el terreno, en MILESIMAS DE GRADO.

**Devuelve:** angulo

#### `g3d_vehicle_roll(int vehiculo) -> float`
Inclinacion lateral segun el terreno, en MILESIMAS DE GRADO.

**Devuelve:** angulo

#### `g3d_vehicle_speed(int vehiculo) -> float`
Velocidad hacia delante, con signo (negativa = marcha atras).

**Devuelve:** velocidad

### voxterrain
#### `g3d_voxterrain_floor(float x, float y, float z) -> float`
Altura del suelo sobre el que se apoya alguien en (x, y, z): la superficie solida justo debajo (el suelo de la cueva si esta dentro de una). Si el punto esta dentro de roca, devuelve la superficie justo encima (para sacarlo).

**Devuelve:** altura, o un valor muy negativo si no hay

#### `g3d_voxterrain_solid(float x, float y, float z) -> int`
1 si el punto (x, y, z) esta dentro de roca del terreno de voxels (cuevas, voladizos). Sirve para colisionar con paredes y techos.

**Devuelve:** 1 o 0

#### `g3d_voxterrain_blocked(float x, float y, float z, float radio, float alto) -> int`
1 si una capsula (pies en y, de radio y alto dados) chocaria con roca en (x, z): colision robusta con paredes y techos.

**Devuelve:** 1 o 0

#### `g3d_voxterrain_worldsize() -> float`
Tamano del mapa de voxels en X/Z, para mantener al personaje dentro del mundo.

**Devuelve:** tamano

#### `g3d_voxterrain_walk(float x, float y, float z, float radio, float alto, float paso_max, float dx, float dz) -> int`
Mueve una capsula (radio y alto) desde (x, y, z) sumando (dx, dz): desliza por X y luego por Z, sube cuestas de hasta paso_max, y se detiene ante paredes, precipicios y techos bajos. Lee el resultado con g3d_voxterrain_walkx/y/z().

```
g3d_voxterrain_walk(px, py, pz, 0.4, 1.8, 0.5, vx, vz);
px = g3d_voxterrain_walkx();
py = g3d_voxterrain_walky();
pz = g3d_voxterrain_walkz();
```

#### `g3d_voxterrain_walkx() -> float`
X resuelta por la ultima g3d_voxterrain_walk.

**Devuelve:** X

#### `g3d_voxterrain_walky() -> float`
Y resuelta por la ultima g3d_voxterrain_walk.

**Devuelve:** Y

#### `g3d_voxterrain_walkz() -> float`
Z resuelta por la ultima g3d_voxterrain_walk.

**Devuelve:** Z

### wall
#### `g3d_wall_ray(float ox, float oy, float oz, float dx, float dy, float dz, float maxd) -> float`
Rayo contra TODA la geometria colisionable (la de g3d_collider_add_model y similares), no solo contra cajas. Util para camaras que no deben atravesar paredes. Solo funciona con Jolt (PC); en las plataformas sin Jolt devuelve siempre el ultimo parametro (maxd), es decir, 'no choca'.

**Devuelve:** distancia al primer impacto, o maxd si no hay ninguno

### water
#### `g3d_water_set_ssr(int activo, float fuerza) -> int`
Reflejos en espacio de pantalla sobre el agua. activo 1/0 y fuerza.

#### `g3d_water_set_ocean(float dir_x, float dir_z, float amplitud) -> int`
Oleaje de mar abierto: direccion (x, z) en la que avanzan las olas y amplitud del oleaje que rompe en la orilla (0 = desactivado).

#### `g3d_water_set_tessellation(int activo) -> int`
Activa (1) o desactiva (0) las olas con volumen real por teselado (requiere GL4; por defecto activo si se dispone de el).

#### `g3d_water_set_surf(float cantidad, float longitud, float velocidad, float alcance) -> int`
Oleaje que rompe en la playa: cantidad (0 = sin rompientes), longitud de onda medida en unidades de profundidad, velocidad con que llegan y alcance (cuanto sube la espuma por la arena, en unidades del mundo).

#### `g3d_water_set_surf_wave(float altura, float direccion_grados) -> int`
Forma del oleaje de orilla: altura de la cresta y direccion de la que llega, en grados. En la orilla las olas giran solas para quedar paralelas a la costa.

#### `g3d_water_set_foam(float espuma, float refraccion) -> int`
Espuma (0 = ninguna) y cuanto distorsiona el agua lo que hay detras (refraccion, 0 = nada).

#### `g3d_water_set_splash(float cantidad, float umbral) -> int`
Salpicaduras: intensidad (0 = sin ellas, 1 por defecto) y velocidad minima del agua, en unidades por segundo, para que salpique al chocar con un obstaculo (0.8 por defecto; sube si un lago en calma burbujea alrededor de las rocas).

#### `g3d_water_set_caustics(float fuerza) -> int`
Caustica: la red de luz que el agua proyecta en el fondo. 0 la apaga; 1 es la fuerza por defecto.

#### `g3d_water_set_underwater_look(float visibilidad, float rayos) -> int`
Aspecto bajo el agua cuando la camara esta sumergida: visibilidad (hasta donde se ve) y fuerza de los rayos de luz (0 = ninguno).

#### `g3d_water_create(float nivel, float tamano, int subdiv) -> int`
Crea la superficie de agua global a la altura nivel, de tamano x tamano unidades y subdividida subdiv veces por lado, y la activa. Con olas animadas, reflejo Fresnel, brillo del sol y transparencia. Despues se afina con g3d_water_set_*.

**Devuelve:** id del agua

```
g3d_water_create(0.0, 400.0, 64);
g3d_water_set_color(0.0, 0.1, 0.25,  0.1, 0.5, 0.6);
g3d_water_set_waves(0.3, 6.0, 1.0);
```

#### `g3d_water_level_at(float x, float z) -> float`
Altura de la superficie del agua (mar, lago o rio) en un punto.

**Devuelve:** altura, o un valor muy negativo si no hay agua ahi

```
if (g3d_water_level_at(px, pz) > py) nadando = 1; end
```

#### `g3d_water_set_waves(float amplitud, float longitud, float velocidad) -> int`
Olas: amplitud maxima, longitud de onda base y velocidad de la animacion.

#### `g3d_water_set_color(float prof_r, float prof_g, float prof_b, float orilla_r, float orilla_g, float orilla_b) -> int`
Colores del agua (0 a 1): el de aguas profundas y el de orilla / poco fondo; el brillo Fresnel mezcla entre ambos.

#### `g3d_water_set_enabled(int activo) -> int`
Muestra (1) u oculta (0) el agua.

#### `g3d_water_set_texture(int textura) -> int`
Textura de la superficie (de g3d_load_texture, que se repita sin costuras): anade detalle de ondas que se desplazan y tinte. 0 la quita.

#### `g3d_water_set_reflection(int activo, float fuerza) -> int`
Reflejo plano: dibuja la escena en espejo y la mezcla en el agua segun Fresnel. activo 1/0; fuerza escala el efecto.

#### `g3d_water_set_reflection_flip(int invertir) -> int`
Invierte la coordenada V del reflejo (1) si en tu plataforma sale boca abajo.

#### `g3d_water_ripple(float x, float z, float fuerza) -> int`
Lanza una onda circular en (x, z) de la superficie. fuerza 1 = chapoteo normal. Llamala por evento (una brazada) o repetida (un objeto en el agua).

#### `g3d_water_add_ripple_source(float x, float z, float fuerza) -> int`
Registra una fuente continua de ondas (desembocadura de un rio, etc.): el motor repite la onda solo, con una cadencia.

#### `g3d_water_clear_ripple_sources() -> int`
Quita todas las fuentes continuas de ondas.

#### `g3d_water_splash(float x, float y, float z, float fuerza) -> int`
Onda en la superficie mas una rafaga de gotas en (x, y, z).

```
g3d_water_splash(px, 0.0, pz, 1.0);
```

### waterfall
#### `g3d_waterfall_add(float tx, float ty, float tz, float bx, float bz, float ancho, float arco) -> int`
Cascada puesta a mano: del borde (tx, ty, tz) a la base (bx, bz), de ancho dado y con un arco (comba hacia delante, 0 = recta). La base aterriza en el agua si la hay. Usa antes g3d_flow_set_foam / g3d_flow_set_speed para su aspecto.

**Devuelve:** id de la cascada, o -1

### watersim
#### `g3d_watersim_clear_sources() -> int`
Quita todos los manantiales.

#### `g3d_watersim_settle(float segundos) -> int`
Avanza la simulacion segundos de golpe, para que el agua alcance su estado estable (un manantial con su lago y su rio ya llenos) en vez de llenarse poco a poco. Llamala al colocar un manantial o al cargar la escena.

#### `g3d_watersim_add_source(float x, float z, float tasa) -> int`
Manantial en (x, z): vierte tasa unidades de profundidad por segundo en esa celda mientras exista. Una tasa negativa es un sumidero. El agua baja sola por las pendientes formando lagos y rios.

**Devuelve:** id del manantial, o -1

```
g3d_watersim_add_source(0, 0, 0.5);
g3d_watersim_settle(60.0);   // que ya aparezca lleno
```

#### `g3d_watersim_set_viscosity(float factor) -> int`
Multiplicador de la velocidad de flujo (1 = normal). Valores bajos dan un liquido lento y espeso, como lava.

#### `g3d_watersim_set_evaporation(float tasa) -> int`
Agua que se evapora o se absorbe por segundo en todas partes. Mantiene los rios finos y seca los charcos sueltos. 0 = el agua solo sale por los bordes del mapa. Ponla a 0 si quieres que un rio alimentado por un manantial no se seque.

### world
#### `g3d_world_rebase(float dx, float dy, float dz) -> int`
Origen flotante: desplaza TODO el mundo (-dx, -dy, -dz) para mantener las coordenadas pequenas lejos del origen, donde los decimales pierden precision. Llamala cuando la camara se aleje mucho y ajusta tu propia camara y tus variables con el mismo desplazamiento.

```
if (abs(cam_x) > 5000)
    g3d_world_rebase(cam_x, 0, 0);
    cam_x = 0;
end
```

### worldgen
#### `g3d_worldgen_set(int semilla, float amplitud, float frecuencia, float nivel_mar) -> int`
Configura el mundo procedural infinito: semilla, amplitud vertical, frecuencia horizontal (menor = relieve mas grande) y nivel del mar (lo que queda por debajo esta bajo el agua). El mismo mundo sale siempre con la misma semilla y las teselas vecinas encajan en las uniones.

```
g3d_worldgen_set(42, 30.0, 0.01, 2.0);
```

#### `g3d_worldgen_height(float x, float z) -> float`
Altura del mundo procedural en (x, z). Es mar donde sea menor que el nivel del mar. Sirve para colocar arboles o rocas sobre el suelo.

**Devuelve:** altura

#### `g3d_worldgen_tile(int escena, int tx, int tz, float tam, int res, float rox, float roz, int textura) -> int`
Construye la tesela de terreno (tx, tz) de tam unidades con una rejilla de res x res, colocada en el origen de render (rox, 0, roz) de la escena indicada y con una textura (o 0). Pensada para usarla con g3d_stream_*.

**Devuelve:** id de la entidad, o -1

```
ent = g3d_worldgen_tile(escena, tx, tz, 64.0, 33, tx * 64.0, tz * 64.0, tex);
```

#### `g3d_worldgen_tile_free(int entidad) -> int`
Destruye una tesela creada con g3d_worldgen_tile y libera su malla.

#### `g3d_worldgen_set_water_depth(float profundidad) -> int`
Profundidad del fondo marino respecto al nivel del mar (1 por defecto; mayor = mares mas profundos). Solo afecta al terreno ya sumergido.

#### `g3d_worldgen_set_biome_textures(int arena, int hierba, int roca, int nieve, float escala) -> int`
Texturas reales por franja de bioma (arena, hierba, roca, nieve), mezcladas segun altura y pendiente. escala es la repeticion en el mundo (por ejemplo 0.03). Con 0 en todas vuelve a los colores planos.

### xray
#### `g3d_xray_update(float cam_x, float cam_y, float cam_z, float obj_x, float obj_y, float obj_z, float radio, float alfa_min, float velocidad) -> int`
Rayos X para camaras en tercera persona: una vez por frame, las entidades marcadas con g3d_entity_set_xray cuya caja corta el segmento camara-objetivo (engordado por radio) se vuelven translucidas hacia alfa_min (0 a 1) y recuperan su opacidad al dejar de tapar. velocidad (0 a 1) es la fraccion que avanza por frame.

```
g3d_xray_update(cx, cy, cz, px, py + 1, pz, 0.5, 0.2, 0.15);
```

### zone
#### `g3d_zone_clear() -> int`
Borra todas las capas de la mascara (conserva su tamano).

#### `g3d_zone_init(int lado, float tam_mundo) -> int`
Crea la mascara de zonas: una rejilla de lado x lado celdas sobre un mundo de tam_mundo unidades, centrada en el origen, con capas de la 0 a la 7 por celda. lado = rejilla del terreno + 1.

#### `g3d_zone_load(string ruta) -> int`
Carga la mascara de zonas pintada en el editor desde un archivo.

**Devuelve:** 1 si lo cargo

#### `g3d_zone_blocked(float x, float z, int capa) -> int`
1 si la celda mas cercana a (x, z) tiene marcada la capa (0 a 7); con capa negativa, si tiene cualquiera. Sirve para confinar a quien respete esa capa (un barco no sale del agua aunque el personaje si).

**Devuelve:** 1 o 0

```
if (g3d_zone_blocked(nx, nz, 0)) nx = x; nz = z; end
```

## Modulo ads
### general
#### `ads_initialize() -> int`
Inicia los anuncios. Hay que llamarla antes que las demas ADS_*. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

**Devuelve:** 1 si va bien

#### `ads_load_interstitial(string id_unidad) -> int`
Pide un anuncio a pantalla completa (intersticial) con el identificador de unidad de AdMob. Conviene hacerlo con tiempo (al empezar el nivel) porque tarda en cargar. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_show_interstitial() -> int`
Muestra el anuncio intersticial cargado. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_is_interstitial_loaded() -> int`
Indica si el intersticial ya esta cargado y listo para mostrarse. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

**Devuelve:** 1 si esta listo

#### `ads_load_rewarded(string id_unidad) -> int`
Pide un anuncio con recompensa (el usuario lo ve a cambio de algo) con el identificador de unidad de AdMob. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_show_rewarded() -> int`
Muestra el anuncio con recompensa. El resultado se consulta con ADS_POLL_EVENT. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_is_rewarded_loaded() -> int`
Indica si el anuncio con recompensa esta cargado. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

**Devuelve:** 1 si esta listo

#### `ads_show_banner(string id_unidad, int posicion) -> int`
Muestra un banner fijo con el identificador de unidad de AdMob, arriba (ADS_BANNER_POS_TOP) o abajo (ADS_BANNER_POS_BOTTOM) de la pantalla. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_hide_banner() -> int`
Oculta el banner. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_poll_event() -> int`
Devuelve el codigo de un evento de anuncios ocurrido (por ejemplo ADS_EVENT_CONSENT_FORM_DISMISSED, que vale 8, al cerrarse el formulario de consentimiento), o 0 si no hay ninguno. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

**Devuelve:** codigo del evento, 0 si no hay

#### `ads_check_consent() -> int`
Comprueba si hace falta pedir al usuario el consentimiento de privacidad (obligatorio en la Union Europea). Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_show_consent_form() -> int`
Muestra el formulario de consentimiento de privacidad. Al cerrarse lo avisa el evento ADS_EVENT_CONSENT_FORM_DISMISSED. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

#### `ads_set_child_directed(int dirigida_a_ninos) -> int`
Marca la aplicacion como dirigida a ninos (1) o no (0), para que los anuncios cumplan la normativa de proteccion de menores. Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes.

## Modulo debug
### general
#### `trace(int activar) -> int`
Activa (1) o desactiva (0) el modo de seguimiento del depurador integrado.

## Modulo gfx
### general
#### `advance(double distancia) -> int`
Mueve el proceso 'distancia' en la direccion de su variable angle. Los angulos de BennuGD van en MILESIMAS DE GRADO (90000 = 90 grados) y la Y de pantalla crece hacia abajo, asi que con angle = 90000 el proceso sube.

```
angle = 45000;
advance(4);   // avanza 4 pixeles en diagonal hacia arriba a la derecha
```

#### `xadvance(int angulo, double distancia) -> int`
Como ADVANCE pero con el angulo dado, sin tocar la variable angle.

#### `get_angle(int proceso) -> int`
Angulo (en milesimas de grado) desde este proceso hasta otro, indicado por su identificador.

**Devuelve:** angulo

#### `get_angle(int proceso_a, int proceso_b) -> int`
Angulo (en milesimas de grado) desde el primer proceso hasta el segundo.

**Devuelve:** angulo

#### `get_dist(int proceso) -> double`
Distancia en pixeles desde este proceso hasta otro, indicado por su identificador.

**Devuelve:** distancia

#### `get_dist(int proceso_a, int proceso_b) -> double`
Distancia en pixeles entre dos procesos.

**Devuelve:** distancia

#### `get_real_point(int proceso, double x, double y, pointer &x_real, pointer &y_real) -> int`
Convierte un punto (x, y) en coordenadas del grafico del proceso dado a su posicion real en pantalla, con su giro, tamano y espejo.

**Devuelve:** 1 si va bien, 0 si no

#### `get_real_point(int proceso, int punto, pointer &x, pointer &y) -> int`
Igual, para el proceso con ese identificador.

**Devuelve:** 1 si va bien, 0 si no

#### `get_real_point(int punto, pointer &x, pointer &y) -> int`
Posicion en pantalla del punto de control numero n del grafico de este proceso, teniendo en cuenta su giro, tamano y espejo. Se escribe en dos variables de tipo double (por referencia). Si el punto no esta definido devuelve la posicion del proceso.

**Devuelve:** 1 si va bien, 0 si el punto no existe

#### `collision(int proceso, int objetivo) -> int`
Igual, pero para el proceso indicado en vez de para este.

**Devuelve:** identificador del proceso con el que choca, o 0

#### `collision(int objetivo) -> int`
Comprueba si este proceso choca con otros. Se le pasa el tipo de proceso (TYPE nombre), el identificador de uno concreto, o -1 para el raton. La colision se calcula con la forma real del grafico de cada proceso.

**Devuelve:** identificador del proceso con el que choca, o 0

```
IF (collision(TYPE bala)) vida--; END
```

#### `scroll_start(int n, int archivo, int grafico, int archivo_lejano, int grafico_lejano, int region, int banderas, int archivo_destino, int grafico_destino) -> int`
Igual que la primera forma, y ademas indica el archivo y el grafico de destino donde se dibuja el scroll (en vez de la pantalla).

#### `scroll_start(int n, int archivo, int grafico, int archivo_lejano, int grafico_lejano, int region, int banderas) -> int`
Crea el scroll numero n: un fondo de mapa grande que se desplaza con una camara (n debe ser menor que el maximo de scrolls del motor; fuera de rango no hace nada). Indica el archivo y grafico del fondo, el archivo y grafico del fondo lejano (para el efecto parallax), la region donde se dibuja y las banderas (repeticion horizontal y vertical, etc.). Despues se mueve con las variables de scroll[n] (x0, y0...) y SCROLL_MOVE.

```
scroll_start(0, fpg, 1, fpg, 2, 0, 3);
```

#### `scroll_start(int n, int archivo, int grafico, int grafico_lejano, int region, int banderas) -> int`
Igual, usando el mismo archivo para el fondo y el fondo lejano.

#### `scroll_stop(int n) -> int`
Detiene el scroll numero n y lo quita de la pantalla.

#### `scroll_move(int n) -> int`
Actualiza el scroll n siguiendo a su proceso camara (ctype = c_scroll).

#### `region_define(int region, int x, int y, int ancho, int alto) -> int`
Define una region rectangular de la pantalla (una zona a la que se puede limitar el dibujo de los procesos con su variable region). La region 0 es la pantalla entera.

**Devuelve:** el numero de region

```
region_define(1, 0, 0, 320, 200);   // los procesos con region = 1 solo se ven en ese rectangulo
```

#### `region_out(int proceso, int region) -> int`
Indica si un proceso (su grafico entero) esta fuera de una region.

**Devuelve:** 1 si esta fuera

#### `screen_get() -> int`
Captura la pantalla en un grafico nuevo y devuelve su codigo (0 si falla). Sirve para guardar una captura con MAP_SAVE.

**Devuelve:** codigo del grafico

```
g = screen_get();
map_save(0, g, "captura.png");
```

#### `rgb(byte r, byte g, byte b) -> int`
Crea un color con rojo, verde y azul (0 a 255 cada uno), opaco. El numero que devuelve se usa en DRAWING_COLOR, MAP_CLEAR, MAP_PUT_PIXEL...

**Devuelve:** el color

```
color = rgb(255, 128, 0);   // naranja
```

#### `rgb(int archivo, int grafico, byte r, byte g, byte b) -> int`
Alias que acepta un mapa (se ignora) y devuelve el mismo color.

**Devuelve:** el color

#### `rgba(byte r, byte g, byte b, byte alfa) -> int`
Crea un color con rojo, verde, azul y alfa (0 a 255 cada uno; alfa 255 = opaco).

**Devuelve:** el color

#### `rgba(int archivo, int grafico, byte r, byte g, byte b, byte alfa) -> int`
Alias que acepta un mapa (se ignora) y devuelve el mismo color.

**Devuelve:** el color

#### `rgb_get(int color, pointer &r, pointer &g, pointer &b) -> int`
Separa un color en rojo, verde y azul, en tres variables que se pasan por puntero.

#### `rgb_get(int archivo, int grafico, int color, pointer &r, pointer &g, pointer &b) -> int`
Descompone un color de un grafico en rojo, verde y azul (0 a 255), escritos en las variables por referencia.

#### `rgba_get(int color, pointer &r, pointer &g, pointer &b, pointer &alfa) -> int`
Separa un color en rojo, verde, azul y alfa, en cuatro variables que se pasan por puntero.

#### `rgba_get(int archivo, int grafico, int color, pointer &r, pointer &g, pointer &b, pointer &a) -> int`
Descompone un color de un grafico en rojo, verde, azul y alfa (0 a 255), escritos en las variables por referencia.

#### `fade(int r, int g, int b, int alfa, int milisegundos) -> int`
Funde la pantalla hacia un color: r, g, b y alfa de 0 a 255 (alfa 0 = transparente, 255 = totalmente cubierto) durante los milisegundos indicados. FADE_OFF y FADE_ON son atajos para fundir a negro y volver.

```
fade(0, 0, 0, 255, 1000);   // a negro en 1 s
```

#### `fade(int r, int g, int b, int alfa, int milisegundos, int region) -> int`
Igual, limitado a una region de la pantalla.

#### `fade_on(int velocidad) -> int`
Quita el fundido: la pantalla vuelve a verse (alfa 0) con la velocidad dada.

#### `fade_on(int velocidad, int region) -> int`
Quita el fundido solo en la region indicada.

#### `fade_off(int velocidad) -> int`
Funde la pantalla a negro con la velocidad dada (el alfa de destino es 255).

#### `fade_off(int velocidad, int region) -> int`
Funde a negro solo la region indicada.

#### `set_mode(int ancho, int alto) -> int`
Cambia el tamano de la pantalla (o de la ventana) en pixeles.

```
set_mode(1280, 720);
```

#### `set_mode(int ancho, int alto, int flags) -> int`
Cambia el tamano de la pantalla con banderas de modo.

#### `set_fps(int fps, int saltos) -> int`
Fotogramas por segundo del juego. El segundo valor es cuantos fotogramas se pueden saltar al dibujar si el juego va lento (0 = ninguno).

**Devuelve:** los fps fijados

```
set_fps(60, 0);
```

#### `window_set_title(string titulo) -> int`
Pone el titulo de la ventana.

#### `window_set_icon(int archivo, int grafico) -> int`
Pone el icono de la ventana a partir de un grafico.

#### `window_move(int x, int y) -> int`
Mueve la ventana a una posicion del escritorio.

#### `window_set_pos(int x, int y) -> int`
Alias de WINDOW_MOVE.

#### `window_get_pos(pointer &x, pointer &y) -> int`
Lee la posicion de la ventana en dos variables que se pasan por puntero.

#### `window_get_size(pointer &ancho, pointer &alto, pointer &ancho_max, pointer &alto_max, pointer &ancho_min, pointer &alto_min) -> int`
Lee el tamano actual de la ventana, su tamano maximo y su tamano minimo en seis variables que se pasan por puntero (cada pareja ancho, alto).

#### `desktop_get_size(pointer &ancho, pointer &alto) -> int`
Lee el tamano del escritorio en dos variables que se pasan por puntero.

#### `window_hide() -> int`
Oculta la ventana.

#### `window_show() -> int`
Muestra la ventana.

#### `window_minimize() -> int`
Minimiza la ventana.

#### `window_maximize() -> int`
Maximiza la ventana.

#### `window_raise() -> int`
Trae al frente la ventana.

#### `window_restore() -> int`
Restaura la ventana.

#### `display_get_orientation() -> int`
Orientacion de la pantalla donde esta la ventana: 0 = desconocida, 1 = horizontal, 2 = horizontal invertida, 3 = vertical, 4 = vertical invertida (valores de SDL).

**Devuelve:** codigo de orientacion

#### `window_set_dpi_aware() -> int`
En Windows, avisa de que el programa gestiona pantallas de alta densidad para que el sistema no escale la ventana. En otros sistemas no hace nada.

#### `write(int fuente, int x, int y, int alineacion, string texto) -> int`
Escribe un texto en pantalla y devuelve su identificador. El texto se queda hasta que lo borras con WRITE_DELETE (o hasta que acaba el programa), no hay que volver a escribirlo cada frame. La alineacion es una de ALIGN_TOP_LEFT, ALIGN_TOP, ALIGN_TOP_RIGHT, ALIGN_CENTER_LEFT, ALIGN_CENTER, ALIGN_CENTER_RIGHT, ALIGN_BOTTOM_LEFT, ALIGN_BOTTOM, ALIGN_BOTTOM_RIGHT. La fuente es un identificador de FNT_LOAD o TTF_LOAD (0 = la fuente del sistema).

**Devuelve:** identificador del texto

```
txt = write(0, 10, 10, ALIGN_TOP_LEFT, "Hola");
...
write_delete(txt);
```

#### `write(int fuente, int x, int y, int z, int alineacion, string texto) -> int`
Igual, indicando tambien la profundidad (z): menor z se dibuja delante.

**Devuelve:** identificador del texto

#### `write_move(int texto, int x, int y) -> int`
Mueve un texto ya escrito.

#### `write_move(int texto, int x, int y, int z) -> int`
Mueve un texto ya escrito, cambiando tambien su profundidad.

#### `write_delete(int texto) -> int`
Borra un texto escrito con WRITE o WRITE_VAR (0 borra todos).

#### `write_in_map(int fuente, string texto, int alineacion) -> int`
Dibuja un texto dentro de un grafico nuevo y devuelve su codigo, en vez de ponerlo en pantalla.

**Devuelve:** codigo del grafico

#### `write_in_map(int archivo, int grafico, int fuente, int x, int y, string texto, int alineacion) -> int`
Escribe un texto dentro de un grafico (no en pantalla): archivo y grafico de destino, fuente, posicion, texto y alineacion.

**Devuelve:** 1 si lo escribe, 0 si no (texto vacio o grafico invalido)

#### `text_width(int fuente, string texto) -> int`
Ancho en pixeles que ocupa un texto con una fuente.

**Devuelve:** ancho

#### `text_height(int fuente, string texto) -> int`
Alto en pixeles que ocupa un texto con una fuente.

**Devuelve:** alto

#### `write_var(int fuente, int x, int y, int alineacion, ... variable, ?, ?) -> int`
Escribe el valor de una variable y lo mantiene actualizado solo: si la variable cambia, el texto cambia. Se pasa la propia variable.

**Devuelve:** identificador del texto

```
write_var(0, 10, 30, ALIGN_TOP_LEFT, puntos);
```

#### `write_var(int fuente, int x, int y, int z, int alineacion, ... variable, ?, ?) -> int`
Igual, indicando tambien la profundidad (z).

**Devuelve:** identificador del texto

#### `write_value(int fuente, int x, int y, int alineacion, int tipo, pointer &variable) -> int`
Escribe en pantalla el valor de una variable y lo mantiene actualizado cada frame: fuente, x, y, alineacion, el codigo de tipo de la variable (el de BennuGD: entero, cadena...) y un puntero a la variable. Para el caso normal es mas comodo WRITE_INT o WRITE_VAR.

**Devuelve:** identificador del texto

#### `write_value(int fuente, int x, int y, int z, int alineacion, int tipo, pointer &variable) -> int`
Igual, indicando tambien la profundidad (z).

**Devuelve:** identificador del texto

#### `write_get_rgba(int texto, pointer &r, pointer &g, pointer &b, pointer &a) -> int`
Lee el color de un texto en pantalla (r, g, b y alfa en las variables por referencia).

#### `write_set_rgba(int texto, byte r, byte g, byte b, byte alfa) -> int`
Cambia el color de un texto en pantalla (r, g, b y alfa de 0 a 255).

#### `map_block_copy(int archivo_dest, int grafico_dest, int x_dest, int y_dest, int archivo_orig, int grafico_orig, int x, int y, int ancho, int alto, int banderas) -> int`
Copia un rectangulo (x, y, ancho, alto) de un grafico origen sobre otro grafico destino en (x_dest, y_dest), aplicando las banderas (espejo, etc.).

**Devuelve:** 0 si algun grafico no existe

```
map_block_copy(0, destino, 10, 10, 0, hoja, 32, 0, 32, 32, 0);
```

#### `map_block_copy(int archivo_dest, int grafico_dest, int x_dest, int y_dest, int archivo_orig, int grafico_orig, int x, int y, int ancho, int alto, int banderas, byte r, byte g, byte b, byte alfa) -> int`
Copia un rectangulo (x, y, ancho, alto) de un grafico origen sobre otro grafico destino en (x_dest, y_dest), aplicando las banderas (espejo, etc.). Con tinte r, g, b y alfa (0 a 255).

**Devuelve:** 0 si algun grafico no existe

#### `map_block_copy(int archivo_dest, int grafico_dest, int x_dest, int y_dest, int archivo_orig, int grafico_orig, int x, int y, int ancho, int alto, int banderas, byte r, byte g, byte b, byte alfa, int modo) -> int`
Copia un rectangulo (x, y, ancho, alto) de un grafico origen sobre otro grafico destino en (x_dest, y_dest), aplicando las banderas (espejo, etc.). Con tinte y modo de mezcla.

**Devuelve:** 0 si algun grafico no existe

#### `map_block_copy(int archivo_dest, int grafico_dest, int x_dest, int y_dest, int archivo_orig, int grafico_orig, int x, int y, int ancho, int alto, int banderas, byte r, byte g, byte b, byte alfa, int modo, int src_rgb, int dst_rgb, int src_alfa, int dst_alfa, int ec_rgb, int ec_alfa) -> int`
Copia un rectangulo (x, y, ancho, alto) de un grafico origen sobre otro grafico destino en (x_dest, y_dest), aplicando las banderas (espejo, etc.). Con tinte y mezcla personalizada (src_rgb, dst_rgb, src_alfa, dst_alfa, ec_rgb, ec_alfa).

**Devuelve:** 0 si algun grafico no existe

#### `map_block_copy(int archivo, int grafico_dest, int x_dest, int y_dest, int grafico_orig, int x, int y, int ancho, int alto, int banderas) -> int`
Igual, pero origen y destino estan en el mismo archivo (el primero): archivo, grafico_dest, x_dest, y_dest, grafico_orig, x, y, ancho, alto, banderas.

**Devuelve:** 0 si algun grafico no existe

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y) -> int`
Copia un grafico (origen) sobre otro (destino) en la posicion (x, y).

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y, int angulo, int escala_x, int escala_y, int flags, byte r, byte g, byte b, byte alfa) -> int`
Copia un grafico sobre otro con todas las opciones: angulo (milesimas de grado), escala en porcentaje (100 = tamano normal), banderas (B_HMIRROR, B_VMIRROR, B_TRANSLUCENT...), tinte (r, g, b) y alfa (0 a 255).

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y, int angulo, int escala_x, int escala_y, int flags, byte r, byte g, byte b, byte alfa, int mezcla) -> int`
Igual, con modo de mezcla (BLEND_NORMAL, BLEND_ADD, BLEND_MULTIPLY, BLEND_SUBTRACT...).

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y, int angulo, int escala_x, int escala_y, int banderas, byte r, byte g, byte b, byte alfa, int modo, int src_rgb, int dst_rgb, int src_alfa, int dst_alfa, int ec_rgb, int ec_alfa) -> int`
Como MAP_PUT con color, pero con una mezcla (blend) personalizada: factor origen y destino de color, factor origen y destino de alfa y ecuaciones de color y de alfa (constantes de OpenGL).

**Devuelve:** 0 si algun grafico no existe

#### `map_new(int ancho, int alto) -> int`
Crea un grafico vacio del tamano indicado en la biblioteca del sistema (archivo 0) y devuelve su codigo (0 si falla).

**Devuelve:** codigo del grafico

```
g = map_new(32, 32);
map_clear(0, g, rgb(255, 0, 0));   // lo rellena de rojo
```

#### `map_new(int ancho, int alto, int flags) -> int`
Igual, y con la bandera B_CLEAR ademas lo deja limpio.

**Devuelve:** codigo del grafico

#### `map_clear(int archivo, int grafico) -> int`
Limpia un grafico (lo deja transparente).

#### `map_clear(int archivo, int grafico, int color) -> int`
Rellena un grafico entero de un color.

#### `map_clear(int archivo, int grafico, int x0, int y0, int x1, int y1) -> int`
Rellena de un color solo un rectangulo (x0, y0, x1, y1).

#### `map_clear(int archivo, int grafico, int x0, int y0, int x1, int y1, int color) -> int`
Rellena de un color solo un rectangulo (x0, y0, x1, y1) de un grafico. Las esquinas se ordenan solas.

#### `map_clone(int archivo, int grafico) -> int`
Crea una copia de un grafico y devuelve el codigo de la copia.

**Devuelve:** codigo de la copia

#### `map_name(int archivo, int grafico) -> string`
Nombre de un grafico (el que traia el fichero o el que se le puso con MAP_SET_NAME).

**Devuelve:** el nombre

#### `map_set_name(int archivo, int grafico, string nombre) -> int`
Pone nombre a un grafico.

#### `map_exists(int archivo, int grafico) -> int`
Indica si existe un grafico.

**Devuelve:** 1 si existe

#### `map_del(int archivo, int grafico) -> int`
Libera un grafico de memoria (igual que MAP_UNLOAD).

#### `map_unload(int archivo, int grafico) -> int`
Libera un grafico de memoria.

#### `map_load(string fichero) -> int`
Carga una imagen (PNG, JPG...) como grafico en la biblioteca del sistema y devuelve su codigo (0 si falla).

**Devuelve:** codigo del grafico

```
graph = map_load("Assets/Sprites/heroe.png");
```

#### `map_load(string fichero, pointer &codigo) -> int`
Carga una imagen EN SEGUNDO PLANO: el codigo se guarda en la variable que se pasa por puntero cuando termina.

#### `map_save(int archivo, int grafico, string fichero) -> int`
Guarda un grafico en un fichero de imagen. Devuelve -1 si el grafico no existe.

**Devuelve:** 0 si va bien, -1 si no

#### `map_get_pixel(int archivo, int grafico, int x, int y) -> int`
Color de un pixel de un grafico (-1 si el grafico no existe).

**Devuelve:** el color

#### `map_put_pixel(int archivo, int grafico, int x, int y, int color) -> int`
Pone un pixel de un color en un grafico.

#### `fpg_add(int archivo, int codigo, int archivo_origen, int grafico_origen) -> int`
Anade a una coleccion una copia de un grafico, con el codigo indicado.

#### `fpg_new() -> int`
Crea una coleccion de graficos vacia y devuelve su identificador.

**Devuelve:** identificador del archivo

#### `fpg_exists(int archivo) -> int`
Indica si existe una coleccion de graficos.

**Devuelve:** 1 si existe

#### `fpg_load(string fichero) -> int`
Carga una coleccion de graficos (.fpg) y devuelve su identificador de archivo (0 si falla). Cada grafico se pide luego con (archivo, codigo): las variables file y graph de un proceso. Los .fpg de BennuGD pueden venir comprimidos con gzip.

**Devuelve:** identificador del archivo

```
file = fpg_load("Assets/heroe.fpg");
graph = 1;
```

#### `fpg_load(string fichero, pointer &archivo) -> int`
Carga un .fpg EN SEGUNDO PLANO: el identificador se guarda en la variable que se pasa por puntero cuando termina.

#### `fpg_del(int archivo) -> int`
Libera una coleccion de graficos (igual que FPG_UNLOAD).

#### `fpg_unload(int archivo) -> int`
Libera una coleccion de graficos.

#### `map_info_set(int archivo, int grafico, int propiedad, int valor) -> int`
Cambia una propiedad de un grafico: G_CENTER_X o G_CENTER_Y (el punto del grafico que se coloca en la posicion del proceso).

#### `map_info(int archivo, int grafico, int propiedad, int valor) -> int`
Cambia una propiedad de un grafico (G_CENTER_X o G_CENTER_Y).

#### `map_info(int archivo, int grafico, int propiedad) -> int`
Pide una propiedad de un grafico con una constante: G_WIDTH (ancho), G_HEIGHT (alto), G_DEPTH (profundidad de color), G_CENTER_X y G_CENTER_Y. Con archivo 0 y grafico 0 pregunta por la pantalla.

**Devuelve:** el valor

```
ancho = map_info(0, graph, G_WIDTH);
```

#### `graphic_set(int archivo, int grafico, int propiedad, int valor) -> int`
Alias de MAP_INFO_SET: cambia G_CENTER_X o G_CENTER_Y de un grafico.

#### `graphic_info(int archivo, int grafico, int propiedad) -> int`
Alias de MAP_INFO de tres parametros: propiedad de un grafico (G_WIDTH, G_HEIGHT, G_CENTER_X, G_CENTER_Y...).

**Devuelve:** el valor

#### `point_get(int archivo, int grafico, int punto, pointer &x, pointer &y) -> int`
Lee un punto de control de un grafico en dos variables que se pasan por puntero. El punto 0 sin definir da el centro del grafico. Devuelve 0 si el punto no existe.

**Devuelve:** 1 si existe, 0 si no

#### `point_set(int archivo, int grafico, int punto, int x, int y) -> int`
Fija un punto de control de un grafico (de 0 a 999), por ejemplo donde sale un disparo o donde se agarra un arma.

**Devuelve:** 1 si va bien, -1 si no

#### `center_set(int archivo, int grafico, int x, int y) -> int`
Fija el centro de un grafico: el punto del grafico que queda en la posicion del proceso al dibujarlo (el punto de control 0). Por defecto es el centro; con los pies del personaje pegados al suelo, ponlo abajo.

**Devuelve:** 1 si va bien, -1 si el grafico no existe

```
center_set(0, graph, ancho / 2, alto);   // ancla en los pies
```

#### `point_get_total(int archivo, int grafico) -> int`
Cuantos puntos de control tiene un grafico.

**Devuelve:** cantidad

#### `box_set(int archivo, int grafico, int caja, int forma, int x, int y, int ancho, int alto) -> int`
Define una caja de colision de un grafico (la forma que usa COLLISION): un numero de caja, el tipo de forma, y su posicion (x, y) y tamano (ancho, alto) relativos al grafico. Si la caja ya existe, la sustituye.

**Devuelve:** 1 si va bien, 0 si el grafico no existe

#### `box_set(int caja, int forma, int x, int y, int ancho, int alto) -> int`
Igual, para el grafico del proceso que la llama.

**Devuelve:** 1 si va bien, 0 si no tiene grafico

#### `box_get(int archivo, int grafico, int caja, pointer &forma, pointer &x, pointer &y, pointer &ancho, pointer &alto) -> int`
Lee una caja de colision de un grafico: su forma, posicion y tamano, en variables que se pasan por puntero.

**Devuelve:** 1 si existe

#### `box_get(int caja, pointer &forma, pointer &x, pointer &y, pointer &ancho, pointer &alto) -> int`
Igual, para el grafico del proceso que la llama.

**Devuelve:** 1 si existe

#### `box_get_by_pos(int archivo, int grafico, int posicion, pointer &codigo, pointer &forma, pointer &x, pointer &y, pointer &ancho, pointer &alto) -> int`
Lee los datos de la caja de colision numero n (por orden de posicion) de un grafico: codigo, forma, x, y, ancho y alto, escritos en las variables por referencia (cada una puede ser 0 si no te interesa).

**Devuelve:** 1 si existe, 0 si no

#### `box_get_by_pos(int posicion, pointer &codigo, pointer &forma, pointer &x, pointer &y, pointer &ancho, pointer &alto) -> int`
Igual, para el grafico del proceso actual.

**Devuelve:** 1 si existe, 0 si no

#### `box_remove(int archivo, int grafico, int caja) -> int`
Quita una caja de colision de un grafico.

#### `box_remove(int caja) -> int`
Quita una caja de colision del grafico del proceso que la llama.

#### `box_get_real_vertex(int proceso, int archivo, int grafico, int caja, pointer &vertices, pointer &radio) -> int`
Calcula las 4 esquinas reales en pantalla de una caja de colision (con el giro, tamano y espejo del proceso): se escriben 8 valores double (x e y de cada esquina, en orden: izquierda-arriba, derecha-arriba, derecha-abajo, izquierda-abajo del grafico sin girar) en el array que se pasa por referencia, y el radio en el ultimo parametro (para cajas circulares). Como la anterior, para otro proceso (el primer parametro).

**Devuelve:** 1 si va bien, 0 si no

#### `box_get_real_vertex(int archivo, int grafico, int caja, pointer &vertices, pointer &radio) -> int`
Calcula las 4 esquinas reales en pantalla de una caja de colision (con el giro, tamano y espejo del proceso): se escriben 8 valores double (x e y de cada esquina, en orden: izquierda-arriba, derecha-arriba, derecha-abajo, izquierda-abajo del grafico sin girar) en el array que se pasa por referencia, y el radio en el ultimo parametro (para cajas circulares). Usa el grafico indicado (archivo y grafico), con la posicion y giro del proceso actual.

**Devuelve:** 1 si va bien, 0 si no

#### `box_get_real_vertex(int caja, pointer &vertices, pointer &radio) -> int`
Calcula las 4 esquinas reales en pantalla de una caja de colision (con el giro, tamano y espejo del proceso): se escriben 8 valores double (x e y de cada esquina, en orden: izquierda-arriba, derecha-arriba, derecha-abajo, izquierda-abajo del grafico sin girar) en el array que se pasa por referencia, y el radio en el ultimo parametro (para cajas circulares). Usa el grafico del proceso actual; la caja es el codigo dado (-1 = la caja propia del proceso).

**Devuelve:** 1 si va bien, 0 si no

#### `fnt_new() -> int`
Crea una fuente vacia (juego de caracteres ISO 8859). Despues se rellena letra a letra con GLYPH_SET.

**Devuelve:** codigo de la fuente

#### `fnt_new(int charset) -> int`
Crea una fuente vacia con el juego de caracteres indicado (0 = ISO 8859, 1 = CP850, 2 = UTF-8).

**Devuelve:** codigo de la fuente

#### `fnt_new(int archivo, int grafico, int charset, int ancho, int alto, int primero, int ultimo, int opciones) -> int`
Crea una fuente a partir de un grafico con todas las letras en una rejilla de celdas del mismo tamano. Indica el grafico, el juego de caracteres, el ancho y alto de cada celda, el primer y ultimo caracter que contiene y las opciones.

**Devuelve:** codigo de la fuente, o -1 si el grafico no existe

```
fuente = fnt_new(0, g_letras, 0, 8, 8, 32, 127, 0);
```

#### `fnt_new(int archivo, int grafico, int charset, int ancho, int alto, int primero, int ultimo, int opciones, string orden) -> int`
Crea una fuente a partir de un grafico con todas las letras en una rejilla de celdas del mismo tamano. Con una cadena que da el orden real de los caracteres de la rejilla.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_new(int archivo, int grafico, int reg_x, int reg_y, int reg_ancho, int reg_alto, int charset, int ancho, int alto, int primero, int ultimo, int opciones) -> int`
Crea una fuente a partir de un grafico con todas las letras en una rejilla de celdas del mismo tamano. Solo se lee la region (x, y, ancho, alto) del grafico; un ancho o alto de 0 llega hasta el borde.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_new(int archivo, int grafico, int reg_x, int reg_y, int reg_ancho, int reg_alto, int charset, int ancho, int alto, int primero, int ultimo, int opciones, string orden) -> int`
Como la anterior, con una cadena que da el orden de los caracteres.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_new(int archivo, int grafico, int charset, int ancho, int alto, int primero, int ultimo, int opciones, int ancho_celda, int alto_celda, int margen_izq, int margen_sup, int espaciado) -> int`
Como FNT_NEW(IIIIIIII), pudiendo fijar el tamano real de la celda, los margenes (izquierdo y superior) dentro del grafico y el espaciado entre letras.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_new(int archivo, int grafico, int charset, int ancho, int alto, int primero, int ultimo, int opciones, string orden, int ancho_celda, int alto_celda, int margen_izq, int margen_sup, int espaciado) -> int`
Rejilla con orden de caracteres, tamano de celda, margenes y espaciado.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_new(int archivo, int grafico, int reg_x, int reg_y, int reg_ancho, int reg_alto, int charset, int ancho, int alto, int primero, int ultimo, int opciones, int ancho_celda, int alto_celda, int margen_izq, int margen_sup, int espaciado) -> int`
Rejilla leida solo dentro de una region del grafico (x, y, ancho, alto; 0 = hasta el borde), con tamano de celda, margenes y espaciado.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_new(int archivo, int grafico, int reg_x, int reg_y, int reg_ancho, int reg_alto, int charset, int ancho, int alto, int primero, int ultimo, int opciones, string orden, int ancho_celda, int alto_celda, int margen_izq, int margen_sup, int espaciado) -> int`
Igual, con la cadena que da el orden de los caracteres.

**Devuelve:** codigo de la fuente, o -1

#### `fnt_load(string fichero) -> int`
Carga una fuente de mapa de bits (.fnt) desde un fichero y devuelve su identificador (0 si falla). Los ficheros .fnt de BennuGD pueden venir comprimidos con gzip.

**Devuelve:** identificador de la fuente

#### `fnt_load(string fichero, pointer &identificador) -> int`
Carga una fuente .fnt EN SEGUNDO PLANO: el identificador se guarda en la variable que se pasa por puntero cuando termina.

#### `fnt_unload(int fuente) -> int`
Libera una fuente.

#### `ttf_load(string fichero) -> int`
Carga una fuente TrueType (.ttf) y devuelve su identificador. Se usa igual que una fuente .fnt en WRITE. Ajusta su tamano con TTF_SET_SIZE.

**Devuelve:** identificador de la fuente

#### `ttf_set_size(int fuente, int tamano) -> int`
Tamano de una fuente TrueType, en puntos.

#### `ttf_get_family(int fuente) -> string`
Nombre de la familia de una fuente TTF (por ejemplo "DejaVu Sans").

**Devuelve:** texto

#### `ttf_get_style(int fuente) -> string`
Estilo de una fuente TTF (por ejemplo "Bold").

**Devuelve:** texto

#### `glyph_get(int fuente, int caracter) -> int`
Copia el grafico de un caracter de una fuente en un grafico nuevo y devuelve su codigo, para poder modificarlo.

**Devuelve:** codigo del grafico nuevo, o 0 si no existe

#### `glyph_set(int fuente, int caracter, int archivo, int grafico) -> int`
Sustituye el grafico de un caracter de una fuente por otro grafico. Si el grafico tiene 3 o mas puntos de control, el segundo marca el desplazamiento del caracter.

**Devuelve:** 1 si lo cambia, 0 si no

#### `drawing_color(int color) -> int`
Color con el que se dibujaran los siguientes DRAW_*.

#### `drawing_color(int dibujo, int color) -> int`
Cambia el color de un dibujo ya creado.

#### `drawing_rgba(byte r, byte g, byte b, byte alfa) -> int`
Fija el color de los siguientes DRAW_* con sus componentes r, g, b y alfa, de 0 a 255.

```
drawing_rgba(255, 128, 0, 200);
```

#### `drawing_rgba(int dibujo, byte r, byte g, byte b, byte alfa) -> int`
Cambia el color de un dibujo ya creado (el que devolvio un DRAW_*).

#### `drawing_blendmode(int modo) -> int`
Modo de mezcla de los siguientes DRAW_* (normal, aditivo, etc., con las constantes B_*).

#### `drawing_blendmode(int dibujo, int modo) -> int`
Cambia el modo de mezcla de un dibujo ya creado.

#### `drawing_blendmode(int src_rgb, int dst_rgb, int src_alfa, int dst_alfa, int ec_rgb, int ec_alfa) -> int`
Mezcla personalizada para los siguientes DRAW_*: factor origen y destino de color, factor origen y destino de alfa, y ecuaciones de color y de alfa (constantes de OpenGL).

#### `drawing_blendmode(int dibujo, int src_rgb, int dst_rgb, int src_alfa, int dst_alfa, int ec_rgb, int ec_alfa) -> int`
Igual que la anterior, aplicada a un dibujo ya creado.

#### `drawing_z() -> int`
Profundidad (z) que usan los siguientes DRAW_*.

**Devuelve:** z actual

#### `drawing_z(int z) -> int`
Profundidad (z) de los siguientes dibujos: menor z se dibuja delante.

#### `drawing_z(int dibujo, int z) -> int`
Cambia la profundidad de un dibujo ya creado: menor z se dibuja delante.

#### `drawing_map(int archivo, int grafico) -> int`
Hace que los siguientes DRAW_* dibujen dentro de un grafico en vez de en pantalla. Con (0, 0), que no es un grafico, se vuelve a dibujar en pantalla.

#### `draw_delete(int dibujo) -> int`
Borra un dibujo creado con un DRAW_* (0 borra todos).

#### `draw_move(int dibujo, int x, int y) -> int`
Mueve un dibujo ya creado.

#### `draw_coords(int dibujo, pointer &x, pointer &y, pointer &z) -> int`
Lee las coordenadas de un dibujo creado: x, y y z se escriben en las variables que pases por referencia (cada una puede ser 0 si no la quieres). OJO: en esta version los valores se recortan a 8 bits (0 a 255).

**Devuelve:** 1, o -1 si el dibujo no existe

#### `draw_point(int x, int y) -> int`
Dibuja un punto con el color de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_point(int x, int y, int color) -> int`
Dibuja un punto de un color concreto.

**Devuelve:** identificador del dibujo

#### `draw_line(int x0, int y0, int x1, int y1) -> int`
Dibuja una linea con el color de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

```
drawing_color(rgb(255, 255, 0));
lin = draw_line(10, 10, 200, 100);
```

#### `draw_line(int x0, int y0, int x1, int y1, int color) -> int`
Dibuja una linea de un color concreto.

**Devuelve:** identificador del dibujo

#### `draw_rectangle(int x, int y, int ancho, int alto) -> int`
Dibuja el borde de un rectangulo (x, y, ancho, alto) con el color de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_rectangle(int x, int y, int ancho, int alto, int color) -> int`
Dibuja el borde de un rectangulo de un color concreto.

**Devuelve:** identificador del dibujo

#### `draw_rectangle_filled(int x, int y, int ancho, int alto) -> int`
Dibuja un rectangulo relleno (x, y, ancho, alto) con el color de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_rectangle_filled(int x, int y, int ancho, int alto, int color) -> int`
Dibuja un rectangulo relleno de un color concreto.

**Devuelve:** identificador del dibujo

#### `draw_points(int cantidad, pointer &puntos) -> int`
Dibuja varios puntos a la vez. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_points(int cantidad, pointer &puntos, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_lines(int cantidad, pointer &puntos) -> int`
Dibuja varias lineas a la vez, cada una de un par de puntos. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_lines(int cantidad, pointer &puntos, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_rectangles(int cantidad, pointer &rectangulos) -> int`
Dibuja varios rectangulos (borde) a la vez. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_rectangles(int cantidad, pointer &rectangulos, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_rectangles_filled(int cantidad, pointer &rectangulos) -> int`
Dibuja varios rectangulos rellenos a la vez. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_rectangles_filled(int cantidad, pointer &rectangulos, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_circle(int x, int y, int radio) -> int`
Dibuja el borde de un circulo (centro y radio) con el color de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_circle(int x, int y, int radio, int color) -> int`
Dibuja el borde de un circulo de un color concreto.

**Devuelve:** identificador del dibujo

#### `draw_circle_filled(int x, int y, int radio) -> int`
Dibuja un circulo relleno (centro y radio) con el color de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_circle_filled(int x, int y, int radio, int color) -> int`
Dibuja un circulo relleno de un color concreto.

**Devuelve:** identificador del dibujo

#### `draw_curve(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int nivel) -> int`
Dibuja una curva de Bezier entre dos extremos con dos puntos de control: (x0, y0) inicio, (x1, y1) y (x2, y2) control, (x3, y3) fin, y nivel de detalle (cuantos segmentos usa).

**Devuelve:** identificador del dibujo si no hay DRAWING_MAP activo

```
draw_curve(20, 200, 80, 40, 160, 40, 220, 200, 30);
```

#### `draw_curve(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int nivel, int color) -> int`
Igual, con un color concreto.

#### `draw_arc(int x, int y, int radio, int angulo_inicio, int angulo_fin) -> int`
Dibuja el borde de un arco de circulo: centro, radio y angulos de inicio y fin en MILESIMAS DE GRADO. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_arc(int x, int y, int radio, int angulo_inicio, int angulo_fin, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_arc_filled(int x, int y, int radio, int angulo_inicio, int angulo_fin) -> int`
Dibuja un arco relleno (un sector de circulo): centro, radio y angulos de inicio y fin en milesimas de grado. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_arc_filled(int x, int y, int radio, int angulo_inicio, int angulo_fin, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_ellipse(int x, int y, int radio_x, int radio_y, int grados) -> int`
Dibuja el borde de una elipse: centro, radios horizontal y vertical, y giro en GRADOS. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_ellipse(int x, int y, int radio_x, int radio_y, int grados, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_ellipse_filled(int x, int y, int radio_x, int radio_y, int grados) -> int`
Dibuja una elipse rellena: centro, radios horizontal y vertical, y giro en grados. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_ellipse_filled(int x, int y, int radio_x, int radio_y, int grados, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_sector(int x, int y, int radio_interior, int radio_exterior, int angulo_inicio, int angulo_fin) -> int`
Dibuja el borde de un sector de corona circular: centro, radio interior y exterior, y angulos de inicio y fin en milesimas de grado. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_sector(int x, int y, int radio_interior, int radio_exterior, int angulo_inicio, int angulo_fin, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_sector_filled(int x, int y, int radio_interior, int radio_exterior, int angulo_inicio, int angulo_fin) -> int`
Dibuja un sector de corona circular relleno. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_sector_filled(int x, int y, int radio_interior, int radio_exterior, int angulo_inicio, int angulo_fin, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3) -> int`
Dibuja el borde de un triangulo dado por sus tres vertices (coordenadas absolutas). Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_triangle_filled(int x1, int y1, int x2, int y2, int x3, int y3) -> int`
Dibuja un triangulo relleno dado por sus tres vertices (coordenadas absolutas). Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_triangle_filled(int x1, int y1, int x2, int y2, int x3, int y3, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_rectangle_round(int x, int y, int ancho, int alto, int radio) -> int`
Dibuja el borde de un rectangulo con las esquinas redondeadas: posicion, ancho, alto y radio de las esquinas. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_rectangle_round(int x, int y, int ancho, int alto, int radio, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_rectangle_round_filled(int x, int y, int ancho, int alto, int radio) -> int`
Dibuja un rectangulo relleno con las esquinas redondeadas. Crea un objeto de dibujo que se queda en pantalla hasta DRAW_DELETE (o dibuja directamente en el grafico elegido con DRAWING_MAP).

**Devuelve:** identificador del dibujo

#### `draw_rectangle_round_filled(int x, int y, int ancho, int alto, int radio, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_polygon(int cantidad, pointer &puntos) -> int`
Dibuja el borde de un poligono cerrado. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_polygon(int cantidad, pointer &puntos, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_polygon_filled(int cantidad, pointer &puntos) -> int`
Dibuja un poligono relleno. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_polygon_filled(int cantidad, pointer &puntos, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_polyline(int cantidad, pointer &puntos, int cerrar) -> int`
Dibuja una linea quebrada que une los puntos. Con 'cerrar' en 1 une tambien el ultimo con el primero. Los puntos van en un array de numeros decimales (float) con pares x, y seguidos; 'cantidad' es cuantos puntos.

**Devuelve:** identificador del dibujo

#### `draw_polyline(int cantidad, pointer &puntos, int cerrar, int color) -> int`
Igual, con un color concreto en vez del de DRAWING_COLOR.

**Devuelve:** identificador del dibujo

#### `draw_set_thickness(float grosor) -> float`
Grosor de linea de los siguientes dibujos, en pixeles (decimal).

#### `draw_set_thickness(int dibujo, float grosor) -> float`
Cambia el grosor de linea de un dibujo ya creado.

#### `draw_get_thickness() -> float`
Grosor de linea que se usa para los siguientes dibujos.

**Devuelve:** grosor

#### `draw_get_thickness(int dibujo) -> float`
Grosor de linea de un dibujo ya creado.

**Devuelve:** grosor

#### `path_new(int archivo, int grafico) -> pointer`
Crea una cuadricula de busqueda de caminos a partir de un grafico: cada pixel NEGRO (color 0 / transparente) es transitable y cualquier otro color es un obstaculo. Cada pixel es una celda.

**Devuelve:** manejador de la cuadricula, o 0 si no pudo

```
mapa = path_new(0, g_mapa);
```

#### `path_destroy(pointer cuadricula) -> int`
Libera la cuadricula creada con PATH_NEW.

#### `path_find(pointer cuadricula, int x0, int y0, int x1, int y1, int opciones) -> pointer`
Busca el camino mas corto de (x0, y0) a (x1, y1) con el algoritmo A*, en 4 direcciones o en 8 si opciones incluye 1 (PF_DIAG). Devuelve una lista de pares x, y que acaba en el par -1, -1.

**Devuelve:** puntero a la lista de puntos (hay que liberarla con PATH_FREE_RESULTS), o 0 si no hay camino

```
ruta = path_find(mapa, 5, 5, 40, 30, 1);
if (ruta)
    i = 0;
    while (ruta[i] != -1) say("paso " + ruta[i] + "," + ruta[i+1]); i += 2; end
    path_free_results(ruta);
end
```

#### `path_find(pointer cuadricula, int x0, int y0, int x1, int y1, int opciones, int peso, int heuristica) -> pointer`
Igual, con peso (1 = A* normal; mas alto da prioridad a la heuristica y es mas rapido pero menos exacto) y heuristica: 0 = Manhattan, 1 = euclidea, 2 = octil, 3 = Chebyshev.

**Devuelve:** puntero a la lista de puntos, o 0 si no hay camino

#### `path_free_results(pointer lista) -> int`
Libera la lista de puntos que devolvio PATH_FIND. Hay que llamarla siempre.

#### `texture_set_quality(int calidad) -> int`
Calidad de filtrado de las texturas al escalar: Q_NEAREST (0, pixelado, por defecto) o Q_LINEAR (suavizado). Afecta a los graficos creados despues.

```
texture_set_quality(Q_LINEAR);
```

#### `map_set_palette(int archivo, int grafico, int primero, int n, pointer &colores) -> int`
Cambia colores de la paleta de un grafico de 8 bits: desde el color primero, n colores tomados de un array de estructuras con r, g, b, a (4 bytes por color). Solo vale en graficos de 8 bits.

**Devuelve:** 0 si va bien, -1 si no hay superficie, -2 si no es de 8 bits

#### `map_get_palette(int archivo, int grafico, int primero, int n, pointer &colores) -> int`
Copia en un array n colores de la paleta de un grafico de 8 bits, desde el color primero (4 bytes por color: r, g, b, a).

**Devuelve:** 0 si va bien, -1 si no hay superficie, -2 si no es de 8 bits

#### `shader_get_language() -> int`
Lenguaje de shaders que soporta el motor grafico (constantes SHADER_LANGUAGE_*: GLSL, GLSL ES, HLSL...). -1 si no hay soporte.

**Devuelve:** codigo del lenguaje

#### `shader_get_min_version() -> int`
Version minima de shader soportada, o -1 si no hay soporte.

**Devuelve:** version

#### `shader_get_max_version() -> int`
Version maxima de shader soportada, o -1 si no hay soporte.

**Devuelve:** version

#### `shader_create(string vertice, string fragmento) -> pointer`
Compila un shader a partir del texto del vertex shader y del fragment shader (cadenas con el codigo, no nombres de archivo). Los shaders se escriben en GLSL; solo hay soporte cuando el motor grafico lo permite (con SDL_gpu).

**Devuelve:** manejador del shader, o 0 si falla la compilacion

#### `shader_free(pointer shader) -> int`
Libera un shader.

#### `shader_getattributelocation(pointer shader, string nombre) -> int`
Reservada: ahora mismo no esta disponible (la implementacion esta desactivada). Para los uniformes usa SHADER_GET_PARAM_LOCATION.

#### `shader_get_param_location(pointer shader, string nombre) -> int`
Posicion de una variable uniform del shader por su nombre; hace falta para fijarle valor con SHADER_SET_PARAM.

**Devuelve:** posicion, o -1 si no existe

#### `shader_create_params(int n) -> pointer`
Crea un conjunto de valores para los uniforms con espacio para n parametros. Se asocia a un grafico o proceso para que el shader los reciba.

**Devuelve:** manejador del conjunto de parametros

#### `shader_free_params(pointer parametros) -> int`
Libera un conjunto de parametros.

#### `shader_set_param(pointer parametros, int tipo, int posicion, int valor) -> int`
Fija un uniform entero: el tipo debe ser UNIFORM_INT o UNIFORM_UINT.

**Devuelve:** 0 si va bien, -1 si no

#### `shader_set_param(pointer parametros, int tipo, int posicion, float valor) -> int`
Fija un uniform decimal (tipo UNIFORM_FLOAT).

**Devuelve:** 0 si va bien, -1 si no

#### `shader_set_param(pointer parametros, int tipo, int posicion, int archivo, int grafico, int unidad) -> int`
Pasa una imagen al shader (tipo SHADER_IMAGE): el grafico (archivo y grafico) y la unidad de textura que usa.

**Devuelve:** 0 si va bien, -1 si no

#### `shader_set_param(pointer parametros, int tipo, int posicion, int n, pointer &valores) -> int`
Fija un uniform de tipo array o vector (UNIFORM_INT_ARRAY, UNIFORM_FLOAT2_ARRAY, UNIFORM_FLOAT3_ARRAY, UNIFORM_FLOAT4_ARRAY, etc.): n valores tomados de la memoria apuntada.

**Devuelve:** 0 si va bien, -1 si no

#### `shader_set_param(pointer parametros, int tipo, int posicion, pointer &valores, int n_matrices, int filas, int columnas, int transponer) -> int`
Fija un uniform de matriz (tipo UNIFORM_MATRIX): puntero a los valores, numero de matrices, filas y columnas, y si hay que transponerlas (1) o no (0).

**Devuelve:** 0 si va bien, -1 si no

#### `shader_get_param(pointer parametros, int posicion, pointer &tipo, pointer &valor) -> int`
Lee un uniform entero o decimal del conjunto de parametros: escribe su tipo y un puntero al valor.

**Devuelve:** 0 si va bien, -1 si no

#### `shader_get_param(pointer parametros, int posicion, pointer &tipo, pointer &n, pointer &valores) -> int`
Lee un uniform de tipo vector o array: escribe el tipo, el numero de valores y un puntero a ellos.

**Devuelve:** 0 si va bien, -1 si no

#### `shader_get_param(pointer parametros, int posicion, pointer &tipo, pointer &valores, pointer &n_matrices, pointer &filas, pointer &columnas, pointer &transponer) -> int`
Lee un uniform de matriz: escribe el tipo, un puntero a los valores, el numero de matrices, las filas, las columnas y si estan transpuestas.

**Devuelve:** 0 si va bien, -1 si no

#### `media_load(string archivo, pointer &grafico) -> pointer`
Abre un video (Theora) para reproducirlo. La imagen del video se va dibujando en un grafico de la biblioteca del sistema, cuyo codigo se escribe en la variable que pases por referencia (usa & delante). Todavia no suena: llama a MEDIA_PLAY.

**Devuelve:** manejador del video, o 0 si no se pudo abrir

```
video = media_load("intro.ogv", &g);
if (video) media_play(video); put(0, g, 320, 240); end
```

#### `media_load(string archivo, pointer &grafico, int timeout) -> pointer`
Igual, con un tiempo maximo de espera (timeout) para operaciones de red.

**Devuelve:** manejador del video, o 0

#### `media_unload(pointer video) -> int`
Cierra el video y libera sus recursos.

#### `media_play(pointer video) -> int`
Empieza la reproduccion. Si estaba parado o terminado, lo reabre desde el principio.

**Devuelve:** 0 si va bien, -1 si hay error

#### `media_pause(pointer video) -> int`
Pausa la reproduccion.

#### `media_pause(pointer video, int pausar) -> int`
Pausa (valor distinto de 0) o reanuda (0) la reproduccion.

#### `media_resume(pointer video) -> int`
Reanuda un video en pausa.

#### `media_stop(pointer video) -> int`
Detiene la reproduccion y libera lo que usa para decodificar.

#### `media_get_time(pointer video) -> int`
Tiempo de reproduccion actual en milisegundos.

**Devuelve:** milisegundos, o -1 si el video no es valido

#### `media_get_duration(pointer video) -> int`
Duracion total del video en milisegundos. En esta version aun no esta implementada y siempre devuelve -1.

**Devuelve:** -1

#### `media_get_status(pointer video) -> int`
Estado de la reproduccion: -1 = error, 0 = parado, 1 = reproduciendo, 2 = en pausa, 3 = terminado.

**Devuelve:** codigo de estado

#### `media_get_mute(pointer video) -> int`
Indica si el audio del video esta silenciado.

**Devuelve:** 1 si esta silenciado, 0 si no

#### `media_set_mute(pointer video, int silenciar) -> int`
Silencia (1) o activa (0) el audio del video.

#### `media_get_volume(pointer video) -> int`
Volumen actual del video.

**Devuelve:** volumen de 0 a 128

#### `media_set_volume(pointer video, int volumen) -> int`
Cambia el volumen del video, de 0 a 128 (fuera de ese rango no hace nada y devuelve -1).

**Devuelve:** el volumen anterior, o -1

## Modulo iap
### general
#### `iap_initialize() -> int`
Inicia el sistema de compras dentro de la aplicacion. Hay que llamarla antes que las demas IAP_*. Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

**Devuelve:** 1 si va bien

#### `iap_purchase(string sku) -> int`
Inicia la compra de un producto por su identificador (SKU). El resultado llega despues como un evento de IAP_POLL_EVENT. Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

#### `iap_consume(string sku) -> int`
Consume un producto ya comprado (para poder comprarlo otra vez, como las monedas). Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

#### `iap_get_price(string sku) -> string`
Precio de un producto, con la moneda y el formato de la tienda. Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

**Devuelve:** el precio como texto

#### `iap_poll_event() -> int`
Devuelve el codigo de un evento de compras ocurrido, o 0 si no hay ninguno. Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

**Devuelve:** codigo del evento, 0 si no hay

#### `iap_get_last_purchase_sku() -> string`
Identificador (SKU) de la ultima compra realizada. Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

**Devuelve:** el SKU

#### `iap_get_last_purchase_token() -> string`
Token de la ultima compra realizada, para validarla en un servidor. Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada.

**Devuelve:** el token

## Modulo input
### general
#### `key(int tecla) -> int`
Indica si una tecla esta pulsada AHORA. Se le pasa una constante de tecla (_A, _SPACE, _ESC, _UP, _LEFT...). Una tecla pulsada y soltada dentro del mismo fotograma tambien cuenta como pulsada, para que el juego no se pierda pulsaciones rapidas.

**Devuelve:** 1 si esta pulsada, 0 si no

```
IF (key(_SPACE)) saltar(); END
IF (key(_LEFT)) x -= 4; END
```

#### `keyup(int tecla) -> int`
Alias de KEY_UP.

**Devuelve:** 1 solo en el fotograma de soltarla

#### `key_up(int tecla) -> int`
Indica si una tecla se ACABA de soltar: vale 1 solo en el fotograma en que pasa de pulsada a suelta.

**Devuelve:** 1 solo en el fotograma de soltarla

#### `keydown(int tecla) -> int`
Alias de KEY_DOWN.

**Devuelve:** 1 solo en el fotograma de la pulsacion

#### `key_down(int tecla) -> int`
Indica si una tecla se ACABA de pulsar: vale 1 solo en el fotograma en que pasa de suelta a pulsada. Util para acciones de un solo disparo (saltar, disparar, abrir un menu).

**Devuelve:** 1 solo en el fotograma de la pulsacion

```
IF (key_down(_SPACE)) disparar(); END   // un disparo por pulsacion, aunque mantengas la tecla
```

#### `joy_name() -> string`
Nombre del mando elegido.

**Devuelve:** el nombre

#### `joy_name(int mando) -> string`
Nombre del mando. Sin argumento, el del mando elegido; con un numero, el de ese mando.

**Devuelve:** el nombre

#### `joy_select(int mando) -> int`
Elige el mando con el que trabajan las funciones JOY_* que no llevan numero de mando. BennuGD2 arranca SIN ningun mando elegido: hasta que se elige, JOY_GETBUTTON y JOY_GETAXIS devuelven siempre 0. Normalmente se hace una vez al empezar.

**Devuelve:** el mando elegido

```
IF (joy_numjoysticks() > 0) joy_select(0); END
```

#### `joy_number() -> int`
Alias antiguo de JOY_NUMJOYSTICKS.

**Devuelve:** cantidad de mandos

#### `joy_numjoysticks() -> int`
Numero de mandos conectados.

**Devuelve:** cantidad de mandos

#### `joy_buttons() -> int`
Numero de botones (nombre antiguo) del mando elegido.

**Devuelve:** cantidad

#### `joy_buttons(int mando) -> int`
Numero de botones (nombre antiguo) del mando indicado.

**Devuelve:** cantidad

#### `joy_numbuttons() -> int`
Numero de botones del mando elegido.

**Devuelve:** cantidad

#### `joy_numbuttons(int mando) -> int`
Numero de botones del mando indicado.

**Devuelve:** cantidad

#### `joy_axes() -> int`
Numero de ejes (nombre antiguo) del mando elegido.

**Devuelve:** cantidad

#### `joy_axes(int mando) -> int`
Numero de ejes (nombre antiguo) del mando indicado.

**Devuelve:** cantidad

#### `joy_numaxes() -> int`
Numero de ejes del mando elegido.

**Devuelve:** cantidad

#### `joy_numaxes(int mando) -> int`
Numero de ejes del mando indicado.

**Devuelve:** cantidad

#### `joy_numhats() -> int`
Numero de cruces (sombreros / POV hats) del mando elegido.

**Devuelve:** cantidad

#### `joy_numhats(int mando) -> int`
Numero de cruces (sombreros / POV hats) del mando indicado.

**Devuelve:** cantidad

#### `joy_numballs() -> int`
Numero de bolas (trackballs) del mando elegido.

**Devuelve:** cantidad

#### `joy_numballs(int mando) -> int`
Numero de bolas (trackballs) del mando indicado.

**Devuelve:** cantidad

#### `joy_getaxis(int eje) -> int`
Posicion de un eje del mando elegido con JOY_SELECT (de -32768 a 32767).

**Devuelve:** posicion del eje

#### `joy_getaxis(int mando, int eje) -> int`
Posicion de un eje del mando indicado. Los ejes se piden con JOY_AXIS_LEFTX, JOY_AXIS_LEFTY, JOY_AXIS_RIGHTX, JOY_AXIS_RIGHTY (sticks) y JOY_AXIS_TRIGGERLEFT, JOY_AXIS_TRIGGERRIGHT (gatillos). El valor va de -32768 a 32767 y es 0 en reposo; para tener -1.0 .. 1.0, dividelo entre 32767.0. Los sticks nunca vuelven a 0 exacto: ignora un margen pequeno (zona muerta).

**Devuelve:** posicion del eje

```
ax = joy_getaxis(0, JOY_AXIS_LEFTX) / 32767.0;
IF (abs(ax) < 0.15) ax = 0.0; END   // zona muerta
```

#### `joy_getposition(int eje) -> int`
Alias antiguo de JOY_GETAXIS para el mando elegido.

**Devuelve:** posicion del eje

#### `joy_getposition(int mando, int eje) -> int`
Alias antiguo de JOY_GETAXIS.

**Devuelve:** posicion del eje

#### `joy_getbutton(int boton) -> int`
Estado de un boton del mando elegido con JOY_SELECT.

**Devuelve:** 1 si esta pulsado

#### `joy_getbutton(int mando, int boton) -> int`
Estado de un boton del mando indicado. Los botones se piden con las constantes JOY_BUTTON_A, _B, _X, _Y, _START, _BACK, _LEFTSHOULDER, _RIGHTSHOULDER, _DPAD_UP/DOWN/LEFT/RIGHT, _LEFTSTICK, _RIGHTSTICK, _GUIDE y _TOUCHPAD.

**Devuelve:** 1 si esta pulsado

```
IF (joy_getbutton(0, JOY_BUTTON_A)) saltar(); END
```

#### `joy_gethat(int cruz) -> int`
Estado de una cruz del mando elegido.

**Devuelve:** direccion de la cruz

#### `joy_gethat(int mando, int cruz) -> int`
Estado de una cruz (POV hat) del mando indicado: JOY_HAT_CENTERED, _UP, _DOWN, _LEFT, _RIGHT, _LEFTUP, _LEFTDOWN, _RIGHTUP o _RIGHTDOWN.

**Devuelve:** direccion de la cruz

#### `joy_getball(int bola, pointer &dx, pointer &dy) -> int`
Lee cuanto se ha movido una bola del mando elegido, en dos variables que se pasan por puntero.

#### `joy_getball(int mando, int bola, pointer &dx, pointer &dy) -> int`
Lee cuanto se ha movido una bola (trackball) del mando indicado, en dos variables que se pasan por puntero.

#### `joy_getaccel(pointer &x, pointer &y, pointer &z) -> int`
Lee el acelerometro del mando elegido en tres variables que se pasan por puntero.

#### `joy_getaccel(int mando, pointer &x, pointer &y, pointer &z) -> int`
Lee el acelerometro del mando indicado en tres variables que se pasan por puntero.

#### `joy_is_attached() -> int`
Indica si el mando elegido esta conectado.

**Devuelve:** 1 si esta conectado

#### `joy_is_attached(int mando) -> int`
Indica si un mando esta conectado. Sin argumento, el elegido.

**Devuelve:** 1 si esta conectado

#### `joy_query(int elemento) -> int`
Informacion general de los mandos: se pide con un 'elemento'.

**Devuelve:** el valor pedido

#### `joy_query(int mando, int elemento) -> int`
Informacion especifica de un mando: se pide con un 'elemento'.

**Devuelve:** el valor pedido

#### `joy_query(int mando, int elemento, int arg) -> int`
Informacion especifica de un mando: se pide con un 'elemento' y, segun cual sea, un argumento extra.

**Devuelve:** el valor pedido

#### `joy_set(int mando, int elemento, int arg1, int arg2) -> int`
Cambia un ajuste de un mando: se indica el 'elemento' y hasta dos argumentos.

#### `joy_set(int mando, int elemento, int arg1, int arg2, int arg3) -> int`
Cambia un ajuste de un mando: se indica el 'elemento' y hasta tres argumentos.

#### `number_joy() -> int`
Alias antiguo de JOY_NUMJOYSTICKS.

**Devuelve:** cantidad de mandos

#### `select_joy(int mando) -> int`
Alias antiguo de JOY_SELECT.

#### `get_joy_button(int boton) -> int`
Alias antiguo de JOY_GETBUTTON para el mando elegido.

**Devuelve:** 1 si esta pulsado

#### `get_joy_button(int mando, int boton) -> int`
Alias antiguo de JOY_GETBUTTON.

**Devuelve:** 1 si esta pulsado

#### `get_joy_position(int eje) -> int`
Alias antiguo de JOY_GETAXIS para el mando elegido.

**Devuelve:** posicion del eje

#### `get_joy_position(int mando, int eje) -> int`
Alias antiguo de JOY_GETAXIS.

**Devuelve:** posicion del eje

## Modulo misc
### general
#### `crypt_new(int metodo, pointer &clave) -> pointer`
Crea un cifrador con un metodo (CRYPT_NONE, CRYPT_DES o CRYPT_3DES) y una clave: un puntero a 8 bytes para DES o a 24 bytes para 3DES. Se usa con CRYPT_ENCRYPT y CRYPT_DECRYPT y se libera con CRYPT_DEL.

**Devuelve:** manejador del cifrador, o 0 si la clave no es valida

#### `crypt_del(pointer cifrador) -> int`
Libera un cifrador creado con CRYPT_NEW.

#### `crypt_encrypt(pointer cifrador, pointer &origen, pointer &destino, int bloques) -> int`
Cifra n bloques de 8 bytes de un buffer a otro con un cifrador ya creado. Cifra bloques de 8 bytes (DES o 3DES, modo ECB: sin encadenar bloques). El tamano de los datos debe ser multiplo de 8.

**Devuelve:** 0 si va bien, -1 si hay error

```
c = crypt_new(CRYPT_DES, &clave);
crypt_encrypt(c, &texto, &cifrado, 4);
crypt_del(c);
```

#### `crypt_decrypt(pointer cifrador, pointer &origen, pointer &destino, int bloques) -> int`
Descifra n bloques de 8 bytes de un buffer a otro con un cifrador ya creado. Cifra bloques de 8 bytes (DES o 3DES, modo ECB: sin encadenar bloques). El tamano de los datos debe ser multiplo de 8.

**Devuelve:** 0 si va bien, -1 si hay error

#### `crypt_encrypt(int metodo, pointer &clave, pointer &origen, pointer &destino, int bloques) -> int`
Cifra sin crear antes un cifrador: indicas directamente el metodo y la clave. Cifra bloques de 8 bytes (DES o 3DES, modo ECB: sin encadenar bloques). El tamano de los datos debe ser multiplo de 8.

**Devuelve:** 0 si va bien, -1 si hay error

#### `crypt_decrypt(int metodo, pointer &clave, pointer &origen, pointer &destino, int bloques) -> int`
Descifra sin crear antes un cifrador: indicas directamente el metodo y la clave. Cifra bloques de 8 bytes (DES o 3DES, modo ECB: sin encadenar bloques). El tamano de los datos debe ser multiplo de 8.

**Devuelve:** 0 si va bien, -1 si hay error

#### `cd() -> string`
Carpeta de trabajo actual.

**Devuelve:** la ruta

#### `cd(string carpeta) -> int`
Cambia la carpeta de trabajo.

**Devuelve:** 0 si va bien

#### `chdir(string carpeta) -> int`
Cambia la carpeta de trabajo.

**Devuelve:** 0 si va bien

#### `mkdir(string carpeta) -> int`
Crea una carpeta.

**Devuelve:** 0 si va bien

#### `rmdir(string carpeta) -> int`
Borra una carpeta vacia.

**Devuelve:** 0 si va bien

#### `glob(string ruta_con_comodines) -> string`
Busca ficheros con comodines ('*' y '?'): la primera llamada con la ruta devuelve el primero que coincide y cada llamada siguiente devuelve el siguiente, hasta que no quedan y devuelve una cadena vacia.

**Devuelve:** nombre del fichero, vacio al terminar

```
f = glob("Assets/*.png");
WHILE (f != "")
    say(f);
    f = glob("Assets/*.png");
END
```

#### `rm(string fichero) -> int`
Borra un fichero.

**Devuelve:** 0 si va bien

#### `diropen(string carpeta) -> int`
Abre una carpeta para leer su contenido y devuelve un manejador.

**Devuelve:** manejador

#### `dirclose(int manejador) -> int`
Cierra una carpeta abierta con DIROPEN.

#### `dirread(int manejador) -> string`
Lee la entrada siguiente de una carpeta abierta (vacia al terminar).

**Devuelve:** nombre

#### `get_base_path() -> string`
Carpeta desde la que se ejecuta el programa.

**Devuelve:** la ruta

#### `get_pref_path(string organizacion, string aplicacion) -> string`
Carpeta en la que guardar datos del jugador (partidas, opciones), propia del programa y del sistema. Hay que darle el nombre de la organizacion y de la aplicacion.

**Devuelve:** la ruta

#### `save(string fichero, ... variable, ?, ?) -> int`
Guarda una variable (o una estructura) en un fichero. Es la forma mas simple de guardar partidas.

**Devuelve:** bytes guardados

#### `load(string fichero, ... variable, ?, ?) -> int`
Lee un fichero escrito con SAVE y lo deja en una variable (o estructura).

**Devuelve:** bytes leidos

#### `file(string fichero) -> string`
Lee un fichero de texto entero y devuelve su contenido en una cadena (vacia si no existe).

**Devuelve:** el contenido

```
datos = file("nivel.txt");
```

#### `fopen(string fichero, int modo) -> int`
Abre un fichero y devuelve su manejador (0 si falla). El modo es O_READ (0, leer), O_READWRITE (1), O_WRITE (2, escribir), O_ZREAD (3, leer comprimido) u O_ZWRITE (4, escribir comprimido).

**Devuelve:** manejador del fichero

```
f = fopen("datos.txt", O_READ);
IF (f) linea = fgets(f); fclose(f); END
```

#### `fclose(int fichero) -> int`
Cierra un fichero abierto con FOPEN.

#### `fread(int fichero, ... variable, ?, ?) -> int`
Lee del fichero una variable (o estructura) escrita con FWRITE.

**Devuelve:** bytes leidos

#### `fread(pointer puntero, int bytes, int fichero) -> int`
Lee 'bytes' bytes de un fichero en un bloque de memoria.

**Devuelve:** bytes leidos

#### `fwrite(int fichero, ... variable, ?, ?) -> int`
Escribe en el fichero una variable (o estructura).

**Devuelve:** bytes escritos

#### `fwrite(pointer puntero, int bytes, int fichero) -> int`
Escribe 'bytes' bytes de un bloque de memoria en un fichero.

**Devuelve:** bytes escritos

#### `fseek(int fichero, int desplazamiento, int origen) -> int`
Mueve la posicion de lectura o escritura de un fichero. 'origen' es el punto de referencia (principio, posicion actual o final).

**Devuelve:** la posicion nueva

#### `frewind(int fichero)`
Vuelve al principio de un fichero abierto.

#### `ftell(int fichero) -> int`
Posicion actual de lectura o escritura de un fichero abierto.

**Devuelve:** posicion

#### `fflush(int fichero) -> int`
Fuerza a escribir en disco lo que quede pendiente de un fichero abierto.

#### `fputs(int fichero, string texto) -> int`
Escribe una cadena en un fichero.

#### `fgets(int fichero) -> string`
Lee una linea de texto de un fichero (sin el salto de linea final). Devuelve una cadena vacia al llegar al final.

**Devuelve:** la linea

#### `feof(int fichero) -> int`
Indica si se ha llegado al final de un fichero.

**Devuelve:** 1 si es el final

#### `flength(int fichero) -> int`
Tamano de un fichero abierto, en bytes.

**Devuelve:** bytes

#### `fexists(string fichero) -> int`
Indica si existe un fichero.

**Devuelve:** 1 si existe

#### `fremove(string fichero) -> int`
Borra un fichero.

#### `fmove(string origen, string destino) -> int`
Mueve o renombra un fichero.

#### `max(double a, double b) -> double`
El mayor de dos numeros.

**Devuelve:** el mayor

#### `min(double a, double b) -> double`
El menor de dos numeros.

**Devuelve:** el menor

#### `sgn(double valor) -> double`
Signo de un numero: -1, 0 o 1.

**Devuelve:** el signo

#### `sgn(int valor) -> int`
Signo de un numero: -1, 0 o 1.

**Devuelve:** el signo

#### `round(double valor) -> double`
Redondea al entero mas cercano (devuelve un decimal con valor entero).

**Devuelve:** el redondeo

#### `floor(double valor) -> double`
Redondea hacia abajo.

**Devuelve:** el redondeo

#### `ceil(double valor) -> double`
Redondea hacia arriba.

**Devuelve:** el redondeo

#### `trunc(double valor) -> double`
Quita la parte decimal.

**Devuelve:** el entero

#### `frac(double valor) -> double`
Parte decimal de un numero.

**Devuelve:** la parte decimal

#### `decimal(double valor) -> double`
Parte decimal de un numero conservando el signo: decimal(-2.75) = -0.75.

**Devuelve:** la parte decimal

#### `rad(int angulo) -> double`
Convierte milesimas de grado a radianes.

**Devuelve:** radianes

#### `deg(double radianes) -> int`
Convierte radianes a milesimas de grado.

**Devuelve:** angulo

#### `abs(double valor) -> double`
Valor absoluto.

**Devuelve:** valor sin signo

#### `exp(double valor) -> double`
e elevado a un valor.

**Devuelve:** resultado

#### `log(double valor) -> double`
Logaritmo natural.

**Devuelve:** resultado

#### `log10(double valor) -> double`
Logaritmo en base 10.

**Devuelve:** resultado

#### `pow(double base, double exponente) -> double`
Potencia: base elevada al exponente.

**Devuelve:** el resultado

#### `sqrt(double valor) -> double`
Raiz cuadrada.

**Devuelve:** la raiz

#### `fmod(double a, double b) -> double`
Resto de dividir a entre b (con decimales).

**Devuelve:** el resto

#### `mmod(double a, double b) -> double`
Alias de MODULUS.

**Devuelve:** el resto

#### `modulus(double a, double b) -> double`
Resto de dividir a entre b, siempre positivo para un b positivo: modulus(-1, 5) = 4 (el operador % da -1).

**Devuelve:** el resto

#### `cos(int angulo) -> double`
Coseno de un angulo. Los angulos de BennuGD van en MILESIMAS DE GRADO (90000 = 90 grados, 360000 = una vuelta). Devuelve un decimal entre -1 y 1.

**Devuelve:** el coseno

#### `sin(int angulo) -> double`
Seno de un angulo. Los angulos de BennuGD van en MILESIMAS DE GRADO (90000 = 90 grados, 360000 = una vuelta). Devuelve un decimal entre -1 y 1.

**Devuelve:** el seno

```
y = sin(angle) * 100;   // angle esta en milesimas de grado
```

#### `tan(int angulo) -> double`
Tangente de un angulo. Los angulos de BennuGD van en MILESIMAS DE GRADO (90000 = 90 grados, 360000 = una vuelta).

**Devuelve:** la tangente

#### `acos(double valor) -> double`
Arcocoseno: devuelve el angulo en milesimas de grado.

**Devuelve:** angulo

#### `asin(double valor) -> double`
Arcoseno: devuelve el angulo en milesimas de grado.

**Devuelve:** angulo

#### `atan(double valor) -> double`
Arcotangente: devuelve el angulo en milesimas de grado.

**Devuelve:** angulo

#### `atan2(double y, double x) -> double`
Arcotangente de y entre x, resolviendo el cuadrante: devuelve el angulo en milesimas de grado.

**Devuelve:** angulo

#### `isinf(double valor) -> int`
Indica si un decimal es infinito.

**Devuelve:** 1 si es infinito

#### `isnan(double valor) -> int`
Indica si un decimal no es un numero (NaN).

**Devuelve:** 1 si no es un numero

#### `finite(double valor) -> int`
Indica si un decimal es un numero normal (ni infinito ni NaN).

**Devuelve:** 1 si es finito

#### `intersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, pointer &x, pointer &y) -> int`
Calcula donde se cortan dos SEGMENTOS: (x1, y1)-(x2, y2) y (x3, y3)-(x4, y4). Si se cortan escribe el punto en las dos variables double por referencia.

**Devuelve:** 1 si se cortan, 0 si no

```
double ix, iy;
if (intersect(0, 0, 10, 10, 0, 10, 10, 0, &ix, &iy))
    say("se cruzan en " + ix + "," + iy);
end
```

#### `intersect_line_circle(double x1, double y1, double x2, double y2, double cx, double cy, double r, pointer &xa, pointer &ya, pointer &xb, pointer &yb) -> int`
Calcula donde corta un SEGMENTO (x1, y1)-(x2, y2) a una circunferencia de centro (cx, cy) y radio r. Escribe hasta dos puntos en las variables double por referencia.

**Devuelve:** numero de puntos de corte (0, 1 o 2); solo se escriben los que existen

#### `intersect_circle(double cx1, double cy1, double cx2, double cy2, double r1, double r2, pointer &xa, pointer &ya, pointer &xb, pointer &yb) -> int`
Calcula donde se cortan dos circunferencias: centro (cx1, cy1) y (cx2, cy2) con radios r1 y r2. Escribe hasta dos puntos en las variables double por referencia.

**Devuelve:** numero de puntos de corte (0, 1 o 2); solo se escriben los que existen

#### `ortho(double x1, double y1, double x2, double y2, double px, double py, pointer &x, pointer &y) -> double`
Proyeccion ortogonal de un punto sobre un segmento: el punto del segmento (x1, y1)-(x2, y2) mas cercano a (px, py). Lo escribe en dos variables double.

**Devuelve:** distancia (double) del punto al segmento

#### `project(double x1, double y1, double x2, double y2, double px, double py, pointer &xa, pointer &ya, pointer &xb, pointer &yb) -> double`
Recta paralela a un segmento que pasa por un punto: devuelve el segmento (x1, y1)-(x2, y2) desplazado en perpendicular hasta (px, py). Escribe sus dos extremos en variables double.

**Devuelve:** distancia (double) entre las dos rectas paralelas

#### `fget_angle(double x0, double y0, double x1, double y1) -> int`
Angulo en milesimas de grado desde el punto (x0, y0) hasta (x1, y1), con la Y de pantalla creciendo hacia abajo: 0 = hacia la derecha, 90000 = hacia arriba.

**Devuelve:** angulo

#### `fget_dist(double x0, double y0, double x1, double y1) -> double`
Distancia entre dos puntos (x0, y0) y (x1, y1).

**Devuelve:** distancia

#### `distance(double x0, double y0, double x1, double y1) -> double`
Alias de FGET_DIST: distancia entre dos puntos.

**Devuelve:** distancia

#### `dist(double x0, double y0, double x1, double y1) -> double`
Alias de FGET_DIST: distancia entre dos puntos.

**Devuelve:** distancia

#### `near_angle(int angulo, int destino, int paso) -> int`
Acerca un angulo a otro como mucho 'paso' por el camino mas corto (para girar suavemente hacia un objetivo). Angulos en milesimas de grado.

**Devuelve:** el angulo nuevo

#### `get_distx(int angulo, double distancia) -> double`
Desplazamiento horizontal de avanzar una distancia en un angulo: distancia * cos(angulo). Angulo en milesimas de grado.

**Devuelve:** desplazamiento en X

#### `get_disty(int angulo, double distancia) -> double`
Desplazamiento vertical de avanzar una distancia en un angulo: -distancia * sin(angulo) (la Y de pantalla crece hacia abajo). Angulo en milesimas de grado.

**Devuelve:** desplazamiento en Y

```
x += get_distx(angle, 5);
y += get_disty(angle, 5);   // lo mismo que advance(5)
```

#### `mag(double dx, double dy) -> double`
Modulo de un vector: raiz de dx^2 + dy^2.

**Devuelve:** longitud

#### `clamp(double valor, double minimo, double maximo) -> double`
Limita un valor decimal al rango [minimo, maximo].

**Devuelve:** el valor limitado

#### `clamp(int valor, int minimo, int maximo) -> int`
Limita un valor al rango [minimo, maximo].

**Devuelve:** el valor limitado

```
vida = clamp(vida, 0, 100);
```

#### `between(double valor, double limite_a, double limite_b) -> int`
Lo mismo con decimales.

**Devuelve:** 1 si esta dentro

#### `between(int valor, int limite_a, int limite_b) -> int`
Indica si un valor esta entre dos limites, ambos incluidos (el orden de los limites da igual).

**Devuelve:** 1 si esta dentro

#### `towards(double valor, double destino, double paso) -> double`
Lo mismo con decimales.

**Devuelve:** el valor nuevo

#### `towards(int valor, int destino, int paso) -> int`
Acerca un valor a un destino como mucho 'paso', sin pasarse.

**Devuelve:** el valor nuevo

```
x = towards(x, 200, 3);   // avanza 3 pixeles por frame hasta 200
```

#### `wrap(int valor, int limite_a, int limite_b) -> int`
Mantiene un valor dentro de un rango dando la vuelta (ambos limites incluidos): wrap(11, 0, 9) = 1.

**Devuelve:** el valor dentro del rango

#### `lerp(double a, double b, double t) -> double`
Interpolacion lineal: a + t * (b - a). Con t = 0 da a; con t = 1 da b.

**Devuelve:** el valor interpolado

#### `invlerp(double a, double b, double valor) -> double`
Inversa de LERP: en que punto (0 a 1) entre a y b esta un valor.

**Devuelve:** la fraccion

#### `rangechk(double valor, double limite_a, double limite_b) -> int`
Lo mismo con decimales.

**Devuelve:** -1, 0 o 1

#### `rangechk(int valor, int limite_a, int limite_b) -> int`
Compara un valor con un rango: -1 si esta por debajo, 1 si esta por encima y 0 si esta dentro (limites incluidos; el orden de los limites da igual).

**Devuelve:** -1, 0 o 1

#### `remap(double valor, double desde_min, double desde_max, double hasta_min, double hasta_max) -> double`
Convierte un valor de un rango a otro: remap(5, 0, 10, 0, 100) = 50.

**Devuelve:** el valor convertido

#### `remap(int valor, int desde_min, int desde_max, int hasta_min, int hasta_max) -> int`
Lo mismo con enteros.

**Devuelve:** el valor convertido

#### `normalize(double valor, double bajo, double alto) -> double`
Convierte un valor del rango [bajo, alto] a un numero entre 0 y 1.

**Devuelve:** la fraccion

#### `normalize(int valor, int bajo, int alto) -> double`
Convierte un valor entero del rango [bajo, alto] a un numero entre 0 y 1.

**Devuelve:** la fraccion

#### `mem_calloc(int, int) -> pointer`
Alias de CALLOC.

#### `mem_alloc(int) -> pointer`
Alias de ALLOC.

#### `mem_free(pointer) -> int`
Alias de FREE.

#### `mem_realloc(pointer, int) -> pointer`
Alias de REALLOC.

#### `mem_cmp(pointer, pointer, int) -> int`
Alias de MEMCMP.

#### `mem_set(pointer, int, int) -> int`
Alias de MEMSET.

#### `mem_setw(pointer &destino, int valor, int n) -> int`
Rellena memoria con valores de 16 bits (word). Escribe el mismo valor n veces seguidas en memoria desde el puntero, con el ancho indicado.

**Devuelve:** 1

#### `mem_setd(pointer &destino, int valor, int n) -> int`
Rellena memoria con valores de 32 bits (dword). Escribe el mismo valor n veces seguidas en memoria desde el puntero, con el ancho indicado.

**Devuelve:** 1

#### `mem_seti(pointer &destino, int valor, int n) -> int`
Rellena memoria con valores de 64 bits (int). Escribe el mismo valor n veces seguidas en memoria desde el puntero, con el ancho indicado.

**Devuelve:** 1

#### `mem_copy(pointer, pointer, int) -> int`
Alias de MEMCOPY.

#### `mem_move(pointer, pointer, int) -> int`
Alias de MEMMOVE.

#### `mem_available() -> int`
Alias de MEMORY_FREE.

**Devuelve:** bytes libres

#### `mem_total() -> int`
Alias de MEMORY_TOTAL.

#### `calloc(int cantidad, int tamano) -> pointer`
Reserva memoria para 'cantidad' elementos de 'tamano' bytes cada uno, puesta a cero.

**Devuelve:** puntero al bloque

#### `alloc(int bytes) -> pointer`
Reserva un bloque de memoria de 'bytes' bytes y devuelve su puntero (0 si no hay memoria). Hay que liberarlo con FREE.

**Devuelve:** puntero al bloque

#### `free(pointer puntero) -> int`
Libera un bloque reservado con ALLOC, CALLOC o REALLOC.

#### `realloc(pointer puntero, int bytes) -> pointer`
Cambia el tamano de un bloque reservado, conservando su contenido.

**Devuelve:** el puntero (puede cambiar)

#### `memcmp(pointer a, pointer b, int bytes) -> int`
Compara dos bloques de memoria byte a byte: 0 si son iguales.

**Devuelve:** 0 si son iguales

#### `memset(pointer puntero, int valor, int bytes) -> int`
Rellena 'bytes' bytes de memoria con el valor de un byte.

#### `memsetw(pointer &destino, int valor, int n) -> int`
Igual que MEM_SETW (nombre alternativo).

**Devuelve:** 1

#### `memsetd(pointer &destino, int valor, int n) -> int`
Igual que MEM_SETD (nombre alternativo).

**Devuelve:** 1

#### `memseti(pointer &destino, int valor, int n) -> int`
Igual que MEM_SETI (nombre alternativo).

**Devuelve:** 1

#### `memcopy(pointer destino, pointer origen, int bytes) -> int`
Copia 'bytes' bytes de memoria de un sitio a otro (no deben solaparse; para eso, MEMMOVE).

#### `memmove(pointer destino, pointer origen, int bytes) -> int`
Copia 'bytes' bytes de memoria de un sitio a otro, aunque se solapen.

#### `memory_free() -> int`
Memoria fisica libre, en bytes (solo informativo; puede ser aproximado).

**Devuelve:** bytes libres

#### `memory_total() -> int`
Memoria fisica total del sistema, en bytes (el calculo depende de la plataforma).

**Devuelve:** bytes

#### `get_id(int tipo) -> int`
Recorre los procesos de un tipo (TYPE nombre): cada llamada devuelve el identificador del siguiente y 0 cuando no quedan. Con 0 recorre todos los procesos.

**Devuelve:** identificador, o 0 al terminar

```
WHILE (id = get_id(TYPE enemigo))
    // id es un enemigo
END
```

#### `get_type(int proceso) -> int`
Tipo de un proceso a partir de su identificador.

**Devuelve:** el tipo

#### `get_status(int proceso) -> int64`
Estado de un proceso a partir de su identificador (en marcha, dormido, congelado o muerto).

**Devuelve:** el estado

#### `signal(int proceso, int senal) -> int`
Envia una senal a un proceso (por su identificador o por su tipo con TYPE nombre), o a todos con ALL_PROCESS. Las senales habituales son S_KILL (matarlo), S_SLEEP, S_FREEZE y S_WAKEUP; sumando S_TREE se aplica tambien a sus hijos.

```
signal(TYPE enemigo, S_KILL);   // quita todos los enemigos
```

#### `signal_action(int senal, int accion) -> int`
Cambia como reacciona ESTE proceso a una senal: la senal (S_KILL, S_WAKEUP, S_SLEEP o S_FREEZE) y la accion (S_IGN = ignorarla, S_DFL = comportamiento normal).

```
signal_action(S_KILL, S_IGN);   // este proceso no muere con signal(0, S_KILL)
```

#### `signal_action(int proceso, int senal, int accion) -> int`
Igual para otro proceso, para todos los de un tipo (pasando el tipo de proceso) o para todos (ALL_PROCESS).

#### `let_me_alone() -> int`
Mata a todos los procesos menos al que la llama.

#### `exit(string mensaje, int codigo) -> int`
Termina el programa. Se puede indicar un mensaje, que se escribe en la consola, y un codigo de salida.

#### `exit(string mensaje) -> int`
Termina el programa con un mensaje.

#### `exit() -> int`
Termina el programa.

#### `exists(int proceso) -> int`
Indica si un proceso esta vivo, por su identificador o por su tipo (TYPE nombre).

**Devuelve:** 1 si existe

#### `pause() -> int`
Pausa TODOS los procesos excepto el que la llama. Reanuda con RESUME. Es lo habitual para un menu de pausa.

**Devuelve:** 0

```
pause();
// ... menu de pausa ...
resume();
```

#### `resume() -> int`
Reanuda todos los procesos pausados con PAUSE.

**Devuelve:** 0

#### `pause(int proceso) -> int`
No disponible: la version con un proceso concreto esta desactivada en esta version del motor.

#### `resume(int proceso) -> int`
No disponible: la version con un proceso concreto esta desactivada en esta version del motor.

#### `rand_seed(int semilla) -> int`
Fija la semilla del generador de numeros al azar: con la misma semilla salen siempre los mismos numeros.

#### `rand(int minimo, int maximo) -> int`
Numero entero al azar entre dos valores, ambos incluidos (el orden da igual).

**Devuelve:** un entero al azar

```
dado = rand(1, 6);
```

#### `regex(string patron, string texto) -> int`
Busca una expresion regular (sintaxis POSIX extendida, sin distinguir mayusculas) en una cadena. Si hay coincidencia devuelve su posicion y rellena REGEX_REG[0] con el texto que coincide y REGEX_REG[1..15] con los grupos entre parentesis.

**Devuelve:** posicion de la coincidencia, o -1 si no hay

```
if (regex("([0-9]+)-([0-9]+)", "rango 10-20") != -1)
    say(regex_reg[1] + " a " + regex_reg[2]);
end
```

#### `regex_replace(string patron, string reemplazo, string texto) -> string`
Sustituye en un texto cada coincidencia de una expresion regular por otro texto (en el reemplazo se aceptan \0 a \9 para insertar los grupos). OJO al orden: patron, REEMPLAZO y luego el texto. REGEX_REG queda rellenado con la primera coincidencia.

**Devuelve:** el texto resultante

```
say(regex_replace("[0-9]+", "#", "a1b22c333"));   // a#b#c#
```

#### `split(string separador, string texto, pointer &trozos, int max) -> int`
Parte una cadena por un separador (expresion regular) y guarda los trozos en un array de cadenas. Devuelve cuantos trozos salieron.

**Devuelve:** cantidad de trozos

#### `join(string separador, pointer &trozos, int cantidad) -> string`
Une los elementos de un array de cadenas con un separador.

**Devuelve:** la cadena unida

#### `say(string texto)`
Escribe una linea de texto en la consola (salida estandar) para depurar. Es la forma de ver mensajes mientras el juego corre.

```
say("vidas: " + itoa(vidas));
```

#### `say_fast(string texto)`
Como SAY pero sin esperar a volcar la salida: mas rapida, para mensajes muy frecuentes.

#### `isort(pointer &array, int tam_elemento, int n, int desplazamiento, byte tam_clave, byte tipo) -> int`
Ordena un array de estructuras en memoria (quicksort) por una clave: puntero al array, tamano de cada elemento en bytes, numero de elementos, desplazamiento de la clave dentro del elemento, tamano de la clave en bytes y tipo de la clave (0 = entero, 1 = decimal).

**Devuelve:** 1

```
isort(&jugadores, sizeof(jugadores[0]), 10, 0, 8, 0);
```

#### `ksort(... array, ? clave, ?, ..., ?, ?) -> int`
Ordena un array de estructuras usando como clave la variable que se indica.

#### `ksort(... array, ? clave, ? cantidad, ..., ?, ?, int) -> int`
Igual, indicando cuantos elementos ordenar.

#### `sort(... array, ? cantidad, ?, int) -> int`
Igual, indicando cuantos elementos ordenar.

#### `sort(... array, ?, ?) -> int`
Ordena un array de numeros o de estructuras (en estas, usando su primera variable como clave).

#### `strlen(string texto) -> int`
Longitud de una cadena en caracteres.

**Devuelve:** la longitud

#### `len(string texto) -> int`
Alias de STRLEN.

**Devuelve:** la longitud

#### `ucase(string texto) -> string`
Pasa una cadena a mayusculas.

**Devuelve:** la cadena

#### `lcase(string texto) -> string`
Pasa una cadena a minusculas.

**Devuelve:** la cadena

#### `strcasecmp(string a, string b) -> int`
Compara dos cadenas sin distinguir mayusculas de minusculas: 0 si son iguales.

**Devuelve:** 0 si son iguales

#### `substr(string texto, int desde, int longitud) -> string`
Trozo de una cadena desde un caracter (empezando en 0) y con una longitud. Una longitud negativa deja fuera ese numero de caracteres del final.

**Devuelve:** el trozo

#### `substr(string texto, int desde) -> string`
Trozo de una cadena desde un caracter (empezando en 0) hasta el final.

**Devuelve:** el trozo

#### `find(string texto, string buscar) -> int`
Posicion de la primera aparicion de un trozo en una cadena (empezando en 0), o -1 si no esta.

**Devuelve:** posicion o -1

#### `find(string texto, string buscar, int desde) -> int`
Igual, empezando a buscar en una posicion.

**Devuelve:** posicion o -1

#### `rfind(string texto, string buscar) -> int`
Posicion de la ULTIMA aparicion de un trozo en una cadena, o -1 si no esta.

**Devuelve:** posicion o -1

#### `rfind(string texto, string buscar, int desde) -> int`
Igual, buscando hacia atras desde una posicion.

**Devuelve:** posicion o -1

#### `strtok(string texto, string delimitadores) -> string`
Parte una cadena en trozos separados por caracteres delimitadores: la primera llamada con la cadena y los delimitadores devuelve el primer trozo.

**Devuelve:** el trozo

#### `lpad(string texto, int longitud) -> string`
Alarga una cadena hasta una longitud anadiendo espacios por la IZQUIERDA (para alinear numeros).

**Devuelve:** la cadena

#### `rpad(string texto, int longitud) -> string`
Alarga una cadena hasta una longitud anadiendo espacios por la DERECHA.

**Devuelve:** la cadena

#### `itoa(int numero) -> string`
Convierte un entero en cadena.

**Devuelve:** la cadena

```
write(0, 10, 10, ALIGN_TOP_LEFT, "Puntos: " + itoa(puntos));
```

#### `ftoa(double numero) -> string`
Convierte un decimal en cadena.

**Devuelve:** la cadena

#### `atoi(string texto) -> int`
Convierte una cadena en entero.

**Devuelve:** el entero

#### `atof(string texto) -> double`
Convierte una cadena en decimal.

**Devuelve:** el decimal

#### `asc(string texto) -> byte`
Codigo del primer caracter de una cadena.

**Devuelve:** el codigo

#### `chr(int codigo) -> string`
Cadena de un solo caracter a partir de su codigo.

**Devuelve:** la cadena

#### `trim(string texto) -> string`
Quita los espacios del principio y del final de una cadena.

**Devuelve:** la cadena

#### `strrev(string texto) -> string`
Da la vuelta a una cadena.

**Devuelve:** la cadena al reves

#### `format(int numero) -> string`
Da formato a un entero con separador de miles.

**Devuelve:** la cadena

#### `format(double numero) -> string`
Da formato a un decimal con separador de miles.

**Devuelve:** la cadena

#### `format(double numero, int decimales) -> string`
Da formato a un decimal con un numero de decimales y separador de miles.

**Devuelve:** la cadena

#### `getenv(string nombre) -> string`
Valor de una variable de entorno del sistema (cadena vacia si no existe).

**Devuelve:** el valor

#### `exec(int modo, string programa, int n_args, pointer &argumentos) -> int`
Ejecuta un programa externo: modo (_P_WAIT espera a que termine y devuelve su codigo de salida; _P_NOWAIT devuelve su identificador de proceso del sistema), nombre del programa, numero de argumentos y un array de cadenas con los argumentos. No esta disponible en Switch, PS3 ni PS5 (devuelve -1).

**Devuelve:** codigo de salida o identificador, -1 si falla

#### `system_keyboard_open(string titulo, string texto_inicial, int maximo, int banderas) -> int`
Abre el teclado en pantalla del sistema: titulo, texto inicial, maximo de caracteres (hasta 2048) y banderas (1 = contrasena; 2 = tipo de teclado alternativo, pensado para numeros). Es un dialogo del sistema: solo existe de verdad en PS5 y Android; en PC devuelve -100 (el programa debe usar su propio teclado).

**Devuelve:** 0 si se abrio, -2 si ya hay uno abierto, -3 si no pudo iniciarse, -100 si no hay teclado del sistema

#### `system_keyboard_status() -> int`
Estado del teclado del sistema: 0 = cerrado, 1 = abierto (el usuario escribe), 2 = terminado. Llamala cada frame hasta que sea 2. Es un dialogo del sistema: solo existe de verdad en PS5 y Android; en PC devuelve -100 (el programa debe usar su propio teclado).

**Devuelve:** estado

#### `system_keyboard_ok() -> int`
Una vez terminado, indica si el usuario acepto (1) o cancelo (0).

**Devuelve:** 1 o 0

#### `system_keyboard_text() -> string`
Texto escrito por el usuario (vacio si cancelo o aun no ha terminado).

**Devuelve:** texto

#### `system_keyboard_close() -> int`
Cierra el teclado del sistema (lo aborta si seguia abierto).

#### `system_progress_open(string titulo, int banderas) -> int`
Abre una barra de progreso del sistema con un titulo; banderas = 1 anade boton de cancelar. Es una barra del sistema: en PS5 no esta disponible aun en las consolas probadas y en PC solo simula (si existe la variable de entorno MP_FAKE_PROGRESS escribe en la consola); el programa debe dibujar su propio progreso.

**Devuelve:** 0 si se abrio, -2 si ya hay una, -6 si no esta disponible, -100 en PC

#### `system_progress_set(int porcentaje) -> int`
Fija el porcentaje de la barra de progreso (0 a 100).

**Devuelve:** resultado, o -1 si no hay barra abierta

#### `system_progress_message(string mensaje) -> int`
Cambia el mensaje que muestra la barra de progreso.

**Devuelve:** resultado, o -1 si no hay barra abierta

#### `system_progress_update() -> int`
Estado de la barra de progreso (3 = el usuario pulso cancelar).

**Devuelve:** estado

#### `system_progress_close() -> int`
Cierra la barra de progreso.

#### `get_pref_language() -> string`
Idioma preferido del usuario segun el sistema.

**Devuelve:** el codigo del idioma

#### `uptime() -> int64`
Alias de GET_TIMER: milisegundos desde que arranco el programa.

**Devuelve:** milisegundos

#### `get_timer() -> int64`
Milisegundos desde que arranco el programa. Sirve para medir tiempos.

**Devuelve:** milisegundos

```
t0 = get_timer();
...
transcurrido = get_timer() - t0;
```

#### `time() -> int64`
Segundos desde el 1 de enero de 1970 (la hora del sistema).

**Devuelve:** segundos

#### `ftime(string formato, int hora) -> string`
Da formato a una hora con los codigos de strftime (%Y, %m, %d, %H, %M, %S...).

**Devuelve:** la cadena

```
say(ftime("%H:%M:%S", time()));
```

#### `string_new_array() -> pointer`
Crea un array de cadenas vacio (de tipo STRING *). Se amplia con STRING_RESIZE_ARRAY.

**Devuelve:** puntero al array

#### `string_new_array(int n) -> pointer`
Crea un array dinamico de n cadenas (todas vacias).

**Devuelve:** puntero al array

#### `string_resize_array(pointer &array, int nuevo_n) -> pointer`
Cambia el tamano de un array de cadenas. Hay que pasar la DIRECCION de la variable puntero (con &) porque puede moverse; las cadenas sobrantes se liberan.

**Devuelve:** nuevo tamano

#### `string_del_array(pointer &array) -> int`
Libera un array de cadenas y todas sus cadenas. Se pasa la direccion de la variable puntero (con &), que queda a 0.

**Devuelve:** 1

#### `string_size_array(pointer array) -> int`
Numero de elementos de un array de cadenas (se pasa el puntero tal cual, sin &).

**Devuelve:** numero de elementos

#### `string_newa() -> pointer`
Igual que STRING_NEW_ARRAY() (nombre corto).

**Devuelve:** puntero al array

#### `string_newa(int n) -> pointer`
Igual que STRING_NEW_ARRAY(n) (nombre corto).

**Devuelve:** puntero al array

#### `string_resizea(pointer &array, int nuevo_n) -> pointer`
Igual que STRING_RESIZE_ARRAY (nombre corto).

**Devuelve:** nuevo tamano

#### `string_dela(pointer &array) -> int`
Igual que STRING_DEL_ARRAY (nombre corto).

**Devuelve:** 1

#### `string_sizea(pointer array) -> int`
Igual que STRING_SIZE_ARRAY (nombre corto).

**Devuelve:** numero de elementos

#### `string_dump() -> int`
Escribe en la consola todas las cadenas que tiene el programa en memoria. Util para detectar fugas.

**Devuelve:** 1

#### `isalpha(byte caracter) -> int`
Indica si un caracter (su codigo) es una letra.

**Devuelve:** 1 si lo es

#### `isspace(byte caracter) -> int`
Indica si un caracter (su codigo) es un espacio, tabulador o salto de linea.

**Devuelve:** 1 si lo es

#### `isnum(byte caracter) -> int`
Indica si un caracter (su codigo) es una cifra (0-9).

**Devuelve:** 1 si lo es

#### `isxnum(byte caracter) -> int`
Indica si un caracter (su codigo) es una cifra hexadecimal (0-9, A-F).

**Devuelve:** 1 si lo es

#### `isalnum(byte caracter) -> int`
Indica si un caracter (su codigo) es una letra o una cifra.

**Devuelve:** 1 si lo es

#### `iswordchar(byte caracter) -> int`
Indica si un caracter (su codigo) puede formar parte de un identificador (letra, cifra o _).

**Devuelve:** 1 si lo es

#### `iswordfirst(byte caracter) -> int`
Indica si un caracter (su codigo) puede empezar un identificador (letra o _).

**Devuelve:** 1 si lo es

#### `toupper(byte caracter) -> int`
Pasa un caracter (su codigo) a mayuscula.

**Devuelve:** el codigo en mayuscula

#### `tolower(byte caracter) -> int`
Pasa un caracter (su codigo) a minuscula.

**Devuelve:** el codigo en minuscula

#### `list_create() -> pointer`
Crea una lista vacia. La lista es una lista enlazada de punteros (valores sueltos de 64 bits): los nuevos elementos se anaden AL PRINCIPIO.

**Devuelve:** manejador de la lista

```
lista = list_create();
list_insertitem(lista, &miobjeto);
list_free(lista);
```

#### `list_create(int reserva) -> pointer`
Crea una lista indicando cuantos nodos se guardan como reserva para reutilizarlos (-1 = el valor por defecto). La lista es una lista enlazada de punteros (valores sueltos de 64 bits): los nuevos elementos se anaden AL PRINCIPIO.

**Devuelve:** manejador de la lista

#### `list_free(pointer lista) -> int`
Libera la lista (no libera lo que apuntan sus elementos).

#### `list_empty(pointer lista) -> int`
Quita todos los elementos de la lista, pero conserva la lista.

#### `list_insertitem(pointer lista, pointer elemento) -> int`
Anade un elemento (un puntero o un valor) al principio de la lista.

#### `list_removeitem(pointer lista, pointer elemento) -> int`
Quita de la lista el elemento con ese valor (el primero que coincida).

#### `list_walk(pointer lista, pointer &contexto) -> pointer`
Recorre la lista elemento a elemento. Pasa una variable de contexto por referencia puesta a 0 al empezar; cada llamada devuelve el siguiente elemento y avanza el contexto, y devuelve 0 al llegar al final.

**Devuelve:** el siguiente elemento, o 0 al acabar

```
ctx = 0;
while ((item = list_walk(lista, &ctx)) != 0)
    say(item);
end
```

#### `list_size(pointer lista) -> int`
Numero de elementos de la lista.

**Devuelve:** numero de elementos

## Modulo net
### general
#### `net_open(int modo, int protocolo, string direccion, int puerto) -> pointer`
Abre una conexion de red y devuelve un manejador (0 si falla). El modo es 0 para SERVIDOR (espera conexiones y escucha en un puerto) o 1 para CLIENTE (se conecta a una direccion). El protocolo es 0 para UDP o 1 para TCP. Admite direcciones IPv4 e IPv6.

**Devuelve:** manejador de la conexion, 0 si falla

```
srv = net_open(0, 1, "", 5000);                // servidor TCP en el puerto 5000
cli = net_open(1, 1, "192.168.1.20", 5000);   // cliente TCP
```

#### `net_wait(pointer conexiones, int espera, pointer eventos) -> int`
Espera a que haya actividad en un grupo de conexiones (una lista creada con LIST_CREATE) hasta 'espera' milisegundos (-1 = sin limite) y deja en otra lista las que tienen novedades.

**Devuelve:** cuantas conexiones tienen novedades, o -1 si hay error

#### `net_send(pointer conexion, pointer datos, int bytes) -> int`
Envia datos por una conexion: un bloque de memoria y su longitud en bytes.

**Devuelve:** bytes enviados, o -1 si hay error

#### `net_recv(pointer conexion, pointer datos, int bytes) -> int`
Recibe datos de una conexion en un bloque de memoria de como mucho 'bytes' bytes.

**Devuelve:** bytes recibidos, o -1 si hay error

#### `net_close(pointer conexion) -> int`
Cierra una conexion.

#### `net_getavailablebytes(pointer conexion) -> int`
Bytes que hay disponibles para leer en una conexion.

**Devuelve:** bytes

#### `net_getreceivebuffersize(pointer conexion) -> int`
Tamano del buffer de recepcion de una conexion.

**Devuelve:** bytes

#### `net_getremoteaddr(pointer conexion) -> string`
Direccion IP del otro extremo de una conexion.

**Devuelve:** la direccion

#### `net_is_new_connection(pointer conexion) -> int`
Indica, para una conexion que ha salido de NET_WAIT, si es un cliente nuevo que se ha conectado a un servidor.

**Devuelve:** 1 si es una conexion nueva

#### `net_is_message_incoming(pointer conexion) -> int`
Indica, para una conexion que ha salido de NET_WAIT, si trae un mensaje para leer.

**Devuelve:** 1 si hay un mensaje

## Modulo sound
### general
#### `soundsys_init() -> int`
Inicia el sistema de sonido. No hace falta llamarla: se inicia sola al cargar el primer sonido o musica. Antes de iniciarlo se pueden fijar sound.freq, sound.mode y sound.channels.

**Devuelve:** 0 si va bien, -1 si falla

#### `soundsys_quit() -> int`
Cierra el sistema de sonido y libera sus recursos.

#### `music_load(string fichero) -> int`
Carga una musica (MP3, OGG, WAV, MOD...) desde un fichero. Devuelve un identificador para el resto de funciones MUSIC_*; 0 si no se pudo cargar. Inicia el sonido si hacia falta.

**Devuelve:** identificador de la musica, 0 si falla

```
m = music_load("Assets/Music/tema.ogg");
music_play(m);   // sin fin
```

#### `music_load(string fichero, pointer &identificador) -> int`
Carga una musica EN SEGUNDO PLANO, sin parar el juego: el identificador se guarda en la variable que se pasa por puntero cuando termina.

#### `music_unload(int musica) -> int`
Libera una musica cargada.

#### `music_unload(pointer &musica) -> int`
Libera una musica cargada con carga en segundo plano, y deja su variable a 0.

#### `music_play(int musica, int bucles) -> int`
Reproduce una musica repitiendola 'bucles' veces (0 = una vez, -1 = sin fin). Solo suena una musica a la vez: empezar otra corta la anterior.

**Devuelve:** 0 si va bien, -1 si hay error

#### `music_play(int musica) -> int`
Reproduce una musica en bucle sin fin.

**Devuelve:** 0 si va bien, -1 si hay error

#### `music_stop() -> int`
Para la musica que suena.

#### `music_pause() -> int`
Pausa la musica.

#### `music_resume() -> int`
Reanuda una musica pausada.

#### `music_rewind() -> int`
Vuelve al principio de la musica.

#### `music_fade_in(int musica, int bucles, int ms) -> int`
Empieza una musica subiendo el volumen poco a poco durante 'ms' milisegundos.

**Devuelve:** 0 si va bien, -1 si hay error

#### `music_fade_in(int musica, int bucles, int ms, double posicion) -> int`
Igual que la anterior, empezando en un punto de la musica ('posicion' en segundos).

**Devuelve:** 0 si va bien, -1 si hay error

#### `music_fade_off(int ms) -> int`
Baja el volumen de la musica poco a poco durante 'ms' milisegundos y la para.

#### `music_is_playing() -> int`
Indica si hay una musica sonando.

**Devuelve:** 1 si suena

#### `music_is_paused() -> int`
Indica si la musica esta en pausa.

**Devuelve:** 1 si esta en pausa

#### `music_is_fading() -> int`
Indica si la musica esta subiendo o bajando de volumen.

**Devuelve:** distinto de 0 si esta en un fundido

#### `music_set_volume(int volumen) -> int`
Volumen de la musica, de 0 a 128 (los valores fuera de rango se recortan).

**Devuelve:** 0 si va bien, -1 si el sonido no esta iniciado

```
music_set_volume(64);   // la mitad
```

#### `music_get_volume(int musica) -> int`
Volumen de una musica (necesita SDL_mixer 2.6 o mas reciente; con una anterior devuelve -1).

**Devuelve:** volumen

#### `music_set_playback_position(double segundos) -> int`
Salta a un punto de la musica, en segundos.

#### `music_set_time_position(double segundos) -> int`
Alias de MUSIC_SET_PLAYBACK_POSITION.

#### `music_get_playback_position(int musica) -> double`
Posicion actual de la musica, en segundos.

**Devuelve:** segundos

#### `music_get_time_position(int musica) -> double`
Alias de MUSIC_GET_PLAYBACK_POSITION.

**Devuelve:** segundos

#### `music_get_duration(int musica) -> double`
Duracion de una musica, en segundos.

**Devuelve:** segundos

#### `music_get_duration() -> double`
Duracion de la musica que suena, en segundos.

**Devuelve:** segundos

#### `music_get_loop_start(int musica) -> double`
Segundo en que esta el comienzo del bucle propio de la musica, si el fichero lo define.

**Devuelve:** segundos

#### `music_get_loop_start() -> double`
Lo mismo para la musica que suena.

**Devuelve:** segundos

#### `music_get_loop_end(int musica) -> double`
Segundo en que esta el final del bucle propio de la musica, si el fichero lo define.

**Devuelve:** segundos

#### `music_get_loop_end() -> double`
Lo mismo para la musica que suena.

**Devuelve:** segundos

#### `music_get_loop_len(int musica) -> double`
Duracion del bucle propio de la musica, en segundos, si el fichero lo define.

**Devuelve:** segundos

#### `music_get_loop_len() -> double`
Lo mismo para la musica que suena.

**Devuelve:** segundos

#### `sound_load(string fichero) -> int`
Carga un efecto de sonido (WAV, OGG...) desde un fichero. Devuelve un identificador para SOUND_PLAY; 0 si no se pudo cargar.

**Devuelve:** identificador del sonido, 0 si falla

```
s = sound_load("Assets/Sounds/salto.wav");
sound_play(s);
```

#### `sound_load(string fichero, pointer &identificador) -> int`
Carga un efecto de sonido EN SEGUNDO PLANO: el identificador se guarda en la variable que se pasa por puntero cuando termina.

#### `sound_unload(int sonido) -> int`
Libera un sonido cargado.

#### `sound_unload(pointer &sonido) -> int`
Libera un sonido cargado con carga en segundo plano y deja su variable a 0.

#### `sound_play(int sonido, int bucles) -> int`
Reproduce un sonido en el primer canal libre, repitiendolo 'bucles' veces (0 = una vez, -1 = sin fin). Se pueden oir varios a la vez, cada uno en su canal.

**Devuelve:** el canal usado, o -1 si no hay ninguno libre

#### `sound_play(int sonido) -> int`
Reproduce un sonido una vez en el primer canal libre.

**Devuelve:** el canal usado, o -1 si no hay ninguno libre

#### `sound_play(int sonido, int bucles, int canal) -> int`
Reproduce un sonido en un canal concreto (-1 = el primero libre).

**Devuelve:** el canal usado, o -1

#### `sound_play(int sonido, int bucles, int canal, int ms) -> int`
Reproduce un sonido en un canal y lo corta a los 'ms' milisegundos aunque no haya acabado.

**Devuelve:** el canal usado, o -1

#### `sound_stop(int canal) -> int`
Para el sonido de un canal (-1 = todos).

#### `sound_pause(int canal) -> int`
Pausa el sonido de un canal (-1 = todos).

#### `sound_resume(int canal) -> int`
Reanuda el sonido de un canal pausado (-1 = todos).

#### `sound_fade_in(int sonido, int bucles, int ms, int canal) -> int`
Reproduce un sonido en un canal subiendo el volumen durante 'ms' milisegundos.

**Devuelve:** el canal usado, o -1

#### `sound_fade_in(int sonido, int bucles, int ms, int canal, int tiempo) -> int`
Igual, y ademas lo corta a los 'tiempo' milisegundos.

**Devuelve:** el canal usado, o -1

#### `sound_fade_off(int canal, int ms) -> int`
Baja el volumen del sonido de un canal durante 'ms' milisegundos y lo para.

#### `sound_is_playing(int canal) -> int`
Indica si suena algo en un canal (-1 = cuenta los canales que suenan).

**Devuelve:** 1 si suena (o cuantos, con -1)

#### `sound_is_paused(int canal) -> int`
Indica si el sonido de un canal esta en pausa.

**Devuelve:** 1 si esta en pausa

#### `sound_is_fading(int canal) -> int`
Indica si un canal esta en un fundido.

**Devuelve:** distinto de 0 si esta en un fundido

#### `sound_set_volume(int sonido, int volumen) -> int`
Volumen propio de un sonido, de 0 a 128. Se aplica cada vez que suena.

#### `sound_set_volume(int volumen) -> int`
Volumen de TODOS los canales, de 0 a 128.

#### `sound_set_location(int canal, int angulo, int distancia) -> int`
Sonido posicional: angulo (0 a 360, 0 = delante, 90 = derecha) y distancia (0 a 255) de un canal respecto al oyente. Solo funciona mientras el canal esta sonando: si no suena nada en ese canal devuelve -1 y no hace nada.

**Devuelve:** 0 si va bien, -1 si no

#### `sound_set_spatial_position(int canal, int angulo, int distancia) -> int`
Alias de SOUND_SET_LOCATION.

**Devuelve:** 0 si va bien, -1 si no

#### `set_position(int canal, int angulo, int distancia) -> int`
Alias de SOUND_SET_LOCATION.

**Devuelve:** 0 si va bien, -1 si no

#### `channel_set_expire(int canal, int ms) -> int`
Hace que un canal se pare solo pasados 'ms' milisegundos (-1 = sin limite).

#### `channel_set_expire(int ms) -> int`
Hace que todos los canales se paren solos pasados 'ms' milisegundos.

#### `channel_set_volume(int canal, int volumen) -> int`
Volumen de un canal, de 0 a 128.

#### `channel_set_volume(int volumen) -> int`
Volumen de todos los canales, de 0 a 128.

#### `set_master_volume(int volumen) -> int`
Volumen general de todo el sonido, de 0 a 128.

**Devuelve:** 0 si va bien, -1 si el sonido no esta iniciado

#### `channel_set_panning(int canal, int izquierda, int derecha) -> int`
Reparto izquierda/derecha de un canal: volumen de cada lado, de 0 a 255 (255 y 255 = centrado, 255 y 0 = solo izquierda). Solo funciona mientras el canal esta sonando: si no suena nada en ese canal devuelve -1 y no hace nada.

**Devuelve:** 0 si va bien, -1 si no

#### `set_panning(int canal, int izquierda, int derecha) -> int`
Alias de CHANNEL_SET_PANNING.

**Devuelve:** 0 si va bien, -1 si no

#### `channel_set_distance(int canal, int distancia) -> int`
Distancia de un canal, de 0 (cerca, a todo volumen) a 255 (lejos, casi inaudible). Solo funciona mientras el canal esta sonando: si no suena nada en ese canal devuelve -1 y no hace nada.

**Devuelve:** 0 si va bien, -1 si no

#### `set_distance(int canal, int distancia) -> int`
Alias de CHANNEL_SET_DISTANCE.

**Devuelve:** 0 si va bien, -1 si no

#### `reserve_channels(int cantidad) -> int`
Reserva los 'cantidad' primeros canales para que no se usen como libres (para sonidos que tu eliges el canal). Devuelve cuantos se reservaron.

**Devuelve:** canales reservados

#### `reverse_stereo(int canal, int invertir) -> int`
Invierte (1) o no (0) los canales izquierdo y derecho de un canal. Solo funciona mientras el canal esta sonando: si no suena nada en ese canal devuelve -1 y no hace nada.

**Devuelve:** 0 si va bien, -1 si no

#### `get_num_music_decoders() -> int`
Numero de decodificadores de musica disponibles (formatos que se pueden leer).

**Devuelve:** cantidad

#### `get_music_decoder(int indice) -> string`
Nombre de un decodificador de musica, por su indice.

**Devuelve:** el nombre

#### `has_music_decoder(string nombre) -> int`
Indica si hay un decodificador con ese nombre.

**Devuelve:** 1 si existe

#### `music_get_type(int musica) -> int`
Devuelve el tipo de formato de una musica, segun lo que traiga el fichero.

**Devuelve:** el valor

#### `music_get_title(int musica) -> string`
Devuelve el titulo de una musica, segun lo que traiga el fichero.

**Devuelve:** el valor

#### `music_get_title_tag(int musica) -> string`
Devuelve el etiqueta de titulo de una musica, segun lo que traiga el fichero.

**Devuelve:** el valor

#### `music_get_artist_tag(int musica) -> string`
Devuelve el etiqueta de artista de una musica, segun lo que traiga el fichero.

**Devuelve:** el valor

#### `music_get_album_tag(int musica) -> string`
Devuelve el etiqueta de album de una musica, segun lo que traiga el fichero.

**Devuelve:** el valor

#### `music_get_copyright_tag(int musica) -> string`
Devuelve el etiqueta de copyright de una musica, segun lo que traiga el fichero.

**Devuelve:** el valor

#### `music_get_type() -> int`
Devuelve el tipo de formato de la musica que suena.

**Devuelve:** el valor

#### `music_get_title() -> string`
Devuelve el titulo de la musica que suena.

**Devuelve:** el valor

#### `music_get_title_tag() -> string`
Devuelve el etiqueta de titulo de la musica que suena.

**Devuelve:** el valor

#### `music_get_artist_tag() -> string`
Devuelve el etiqueta de artista de la musica que suena.

**Devuelve:** el valor

#### `music_get_album_tag() -> string`
Devuelve el etiqueta de album de la musica que suena.

**Devuelve:** el valor

#### `music_get_copyright_tag() -> string`
Devuelve el etiqueta de copyright de la musica que suena.

**Devuelve:** el valor

#### `sound_group_channel(int canal, int grupo) -> int`
Mete un canal en un grupo (una etiqueta numerica), para tratar varios canales a la vez.

#### `sound_group_channels(int desde, int hasta, int grupo) -> int`
Mete un rango de canales en un grupo.

**Devuelve:** cuantos canales se agruparon

#### `sound_group_available(int grupo) -> int`
Primer canal libre de un grupo, o -1 si no hay.

**Devuelve:** canal o -1

#### `sound_group_count(int grupo) -> int`
Cuantos canales tiene un grupo.

**Devuelve:** cantidad

#### `sound_group_oldest(int grupo) -> int`
El canal del grupo que lleva mas tiempo sonando.

**Devuelve:** canal o -1

#### `sound_group_newer(int grupo) -> int`
El canal del grupo que empezo a sonar hace menos.

**Devuelve:** canal o -1

#### `sound_group_halt(int grupo) -> int`
Para todos los sonidos de un grupo.

#### `sound_group_fade_out(int grupo, int ms) -> int`
Baja el volumen de todos los sonidos de un grupo durante 'ms' milisegundos y los para.

## Modulo video
### general
#### `video_open(string fichero) -> int`
Abre un video o un audio (MP4/H.264, WebM/VP9, MKV, AVI, OGV, MP3... lo que FFmpeg sepa decodificar; tambien direcciones de internet) y devuelve su manejador (0 si falla). Para verlo hace falta VIDEO_GRAPH.

**Devuelve:** manejador del video

```
v = video_open("intro.mp4");
g = video_graph(v);
video_play(v, 0);
graph = g; x = 640; y = 360;   // un proceso que muestra el video
WHILE (!video_is_finished(v)) FRAME; END
video_close(v);
```

#### `video_open_h(string direccion, string cabeceras) -> int`
Abre un video enviando cabeceras HTTP extra ("Cabecera: valor\r\n"): hace falta con servidores que piden cuenta (Authorization, Cookie...).

**Devuelve:** manejador del video

#### `video_graph(int video) -> int`
Grafico que muestra siempre el fotograma actual del video. Se usa como cualquier grafico (la variable graph de un proceso). Al cerrar el video, el grafico se libera unos 30 fotogramas despues.

**Devuelve:** codigo del grafico

#### `video_play(int video, int repetir) -> int`
Empieza a reproducir el video; con repetir = 1 lo hace en bucle, con 0 una sola vez.

```
video_play(v, 1);   // en bucle
```

#### `video_pause(int video, int pausa) -> int`
Pausa (1) o reanuda (0) el video.

#### `video_stop(int video) -> int`
Para el video.

#### `video_seek(int video, int ms) -> int`
Salta a un punto del video, en milisegundos.

#### `video_close(int video) -> int`
Cierra un video y libera sus recursos.

#### `video_width(int video) -> int`
Ancho del video en pixeles.

**Devuelve:** ancho

#### `video_height(int video) -> int`
Alto del video en pixeles.

**Devuelve:** alto

#### `video_duration(int video) -> int`
Duracion del video en milisegundos.

**Devuelve:** milisegundos

#### `video_position(int video) -> int`
Posicion actual del video en milisegundos. Es el reloj de la reproduccion: sigue avanzando aunque la decodificacion vaya con retraso.

**Devuelve:** milisegundos

#### `video_is_playing(int video) -> int`
Indica si el video se esta reproduciendo.

**Devuelve:** 1 si se reproduce

#### `video_is_finished(int video) -> int`
Indica si el video ha llegado al final.

**Devuelve:** 1 si ha terminado

#### `video_volume(int video, int volumen) -> int`
Volumen del video, de 0 a 128. El audio sale por el mismo dispositivo que la musica de BennuGD, y mientras un video tiene el audio sustituye a MUSIC_PLAY (solo un video a la vez lo tiene).

#### `video_use_gpu(int activar) -> int`
Conversion del color en la GPU para los videos que se abran despues (1 por defecto). Con 0 se hace por software. Solo vale para video yuv420p de 8 bits; con otros formatos, o si el motor grafico no puede, se usa el software solo.

**Devuelve:** el valor fijado

#### `video_gpu(int video) -> int`
Indica si un video se esta convirtiendo en la GPU.

**Devuelve:** 1 si usa la GPU

#### `video_frames(int video) -> int`
Fotogramas realmente mostrados hasta ahora (los que llegan tarde se descartan para no perder el reloj). Es el numero con el que comparar rendimientos.

**Devuelve:** fotogramas

#### `video_use_hw(int activar) -> int`
Decodificador de video de la consola (PS5) para los videos que se abran despues (1 por defecto). Si no puede con un video, o falla, se pasa solo a libavcodec desde la posicion actual.

**Devuelve:** el valor fijado

#### `video_use_plane(int activar) -> int`
Ajuste interno de decodificacion para los videos que se abran despues.

**Devuelve:** el valor fijado

#### `video_hw(int video) -> int`
Indica que decodificador usa un video: 1 el de la consola, 2 el simulado (en PC con LIBMOD_VIDEO_FAKE_HW=1), 0 software.

**Devuelve:** 0, 1 o 2

#### `video_subtitle_tracks(int video) -> int`
Cuantas pistas de subtitulos de texto hay (las del fichero y las cargadas aparte). Se ignoran las que son imagenes (PGS, DVB).

**Devuelve:** cantidad

#### `video_subtitle_lang(int video, int pista) -> string`
Nombre y idioma de una pista de subtitulos (empezando en 0).

**Devuelve:** el nombre, por ejemplo "spa - Castellano"

#### `video_subtitle_select(int video, int pista) -> int`
Elige la pista de subtitulos a mostrar; -1 los quita (es lo que hay por defecto).

#### `video_subtitle_load(int video, string fichero) -> int`
Carga un fichero de subtitulos externo (.srt, .ass, .ssa, .vtt...) como una pista mas, que queda elegida.

#### `video_subtitle_text(int video) -> string`
Texto del subtitulo que toca ahora (vacio si no hay ninguno). El modulo NO lo dibuja: se escribe con WRITE y la fuente que quieras.

**Devuelve:** el texto

```
write_var(0, 640, 680, ALIGN_BOTTOM, texto);   // texto = video_subtitle_text(v) cada frame
```

#### `video_audio_tracks(int video) -> int`
Cuantas pistas de audio tiene el video.

**Devuelve:** cantidad

#### `video_audio_lang(int video, int pista) -> string`
Nombre y idioma de una pista de audio (empezando en 0).

**Devuelve:** el nombre

#### `video_audio_select(int video, int pista) -> int`
Cambia a otra pista de audio (empezando en 0). Puede oirse un pequeno corte.

#### `video_is_audio_only(int video) -> int`
Indica si el fichero no tiene imagen (es solo musica).

**Devuelve:** 1 si es solo audio

#### `video_spectrum(int video, int banda) -> int`
Nivel, de 0 a 255, de una de las 32 bandas de frecuencia (de 40 Hz a 16 kHz, repartidas en escala logaritmica) de lo que suena ahora; 0 si esta en pausa. Sirve para hacer un ecualizador.

**Devuelve:** nivel de 0 a 255

#### `video_wave(int video, int indice) -> int`
Muestra de la onda que suena ahora, con signo (-128 a 127), para un osciloscopio. El indice va de 0 a 255.

**Devuelve:** valor de -128 a 127

#### `http_fetch(string, string, string, string) -> int`

#### `http_fetch_begin(string, string, string, string) -> int`

#### `http_fetch_poll() -> int`

#### `http_progress() -> int`

#### `http_total() -> int`

#### `video_download_begin(string, string, string) -> int`

#### `video_download_convert_begin(string, string, string, string) -> int`

#### `video_download_audio_begin(string, string, string) -> int`

#### `video_download_path() -> string`

#### `video_download_poll() -> int`

#### `video_download_percent() -> int`

#### `video_download_cancel() -> int`

#### `video_download_error() -> string`

#### `http_length() -> int`

#### `http_read(pointer, int) -> int`

#### `http_save(string) -> int`

#### `http_write(string, string) -> int`

#### `http_trace(string) -> int`

#### `http_error() -> string`

#### `torrent_cache(string) -> int`

#### `torrent_add(string) -> int`

#### `torrent_remove(int, int) -> int`

#### `torrent_state(int) -> int`

#### `torrent_error(int) -> string`

#### `torrent_name(int) -> string`

#### `torrent_files(int) -> int`

#### `torrent_file_name(int, int) -> string`

#### `torrent_file_size(int, int) -> int`

#### `torrent_main(int) -> int`

#### `torrent_select(int, int) -> int`

#### `torrent_url(int, int) -> string`

#### `torrent_peers(int) -> int`

#### `torrent_rate(int) -> int`

#### `torrent_percent(int) -> int`

#### `torrent_buffered(int) -> int`

#### `torrent_ready(int) -> int`

#### `torrent_downloaded(int) -> int`

#### `torrent_options(int, int) -> int`

#### `torrent_ahead(int) -> int`

#### `torrent_upload_limit(int) -> int`

#### `upload_start(string, int, string) -> int`

#### `upload_stop() -> int`

#### `upload_files() -> int`

#### `upload_bytes() -> int`

#### `upload_percent() -> int`

#### `upload_current() -> string`

#### `upload_url() -> string`

#### `dlna_scan() -> int`

#### `dlna_add(string) -> int`

#### `dlna_find(string) -> int`

#### `webdav_parse(string) -> string`

#### `ftp_list_begin(string) -> int`

#### `ftp_list_poll() -> int`

#### `ftp_list_result() -> string`

#### `ftp_list_error() -> string`

#### `dlna_scan_state() -> int`

#### `dlna_count() -> int`

#### `dlna_name(int) -> string`

#### `dlna_browse(int, string, int, int) -> int`

#### `dlna_browse_state() -> int`

#### `dlna_result() -> string`

#### `dlna_total() -> int`

#### `dlna_error() -> string`

#### `video_speed(int video, int porcentaje) -> int`
Velocidad de reproduccion en porcentaje, de 25 a 300 (100 = normal). Mantiene la posicion; el sonido se reinicia desde ella.

#### `video_chapters(int video) -> int`
Cuantos capitulos tiene el video.

**Devuelve:** cantidad

#### `video_chapter_start(int video, int capitulo) -> int`
Donde empieza un capitulo (empezando en 0).

**Devuelve:** posicion

#### `video_chapter_title(int video, int capitulo) -> string`
Titulo de un capitulo (empezando en 0).

**Devuelve:** el titulo

#### `video_get_speed(int video) -> int`
Velocidad de reproduccion actual en porcentaje.

**Devuelve:** porcentaje

#### `video_sub_delay(int video, int ms) -> int`
Retrasa los subtitulos tantos milisegundos (negativo: los adelanta).

#### `video_av_offset(int video, int ms) -> int`
Muestra la imagen tantos milisegundos mas tarde (negativo: mas pronto), para corregir un sonido desincronizado.

#### `upload_ip() -> string`

#### `upload_error() -> string`

