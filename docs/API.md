# Referencia de la API

Generada automaticamente desde los modulos del motor (1150 funciones, 491 documentadas).

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
#### `g3d_cloth_create(float, float, int, int, float, float, float) -> int`

#### `g3d_cloth_create_oriented(float, float, int, int, float, float, float, float, float) -> int`

#### `g3d_cloth_pin(int, int) -> int`

#### `g3d_cloth_set_wind(int, float, float, float, float) -> int`

#### `g3d_cloth_set_collider(int, float, float, float, float) -> int`

#### `g3d_cloth_clear_collider(int) -> int`

#### `g3d_cloth_set_texture(int, int) -> int`

#### `g3d_cloth_update(int, float) -> int`

#### `g3d_cloth_push(float, float, float, float) -> int`

#### `g3d_cloth_push_capsule(float, float, float, float, float, float, float) -> int`

#### `g3d_cloth_pin_move(int, int, int, float, float, float) -> int`

### collide
#### `g3d_collide_move(float, float, float, float, float, float, float) -> int`

#### `g3d_collide_x() -> float`

#### `g3d_collide_z() -> float`

#### `g3d_collide_floor(float, float, float, float) -> float`

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
#### `g3d_fbx_set_recenter(int) -> int`

### fire
#### `g3d_fire_add(float, float, float, float) -> int`

#### `g3d_fire_clear() -> int`

### flow
#### `g3d_flow_add(float, float, float, float, float, float, float, float, float) -> int`

#### `g3d_flow_add_river(int, float, float, float, float, float, float, float, float) -> int`

#### `g3d_flow_set_texture(int) -> int`

#### `g3d_flow_set_color(float, float, float) -> int`

#### `g3d_flow_set_foam(float) -> int`

#### `g3d_flow_set_speed(float) -> int`

#### `g3d_flow_clear() -> int`

#### `g3d_flow_set_clip(float) -> int`

### fluid
#### `g3d_fluid_add(float, float, float, float, float, float) -> int`

#### `g3d_fluid_clear() -> int`

#### `g3d_fluid_style(float, float, float, float, float, float, float, float, float, float) -> int`

#### `g3d_fluid_block_reset() -> int`

#### `g3d_fluid_set_texture(int) -> int`

### general
#### `g3d_raycast(float, float, float, float, float, float, float) -> float`

#### `g3d_triangles() -> int`

#### `g3d_culled() -> int`

#### `g3d_raycast(float, float, float, float, float, float, pointer, pointer, pointer) -> int`

### gltf
#### `g3d_gltf_set_recenter(int) -> int`

#### `g3d_gltf_set_chunking(float) -> int`

### ibl
#### `g3d_ibl_refresh() -> int`

### instances
#### `g3d_instances_create(int, int) -> int`

#### `g3d_instances_add(int, float, float, float, float, float) -> int`

#### `g3d_instances_set(int, int, float, float, float, float, float) -> int`

#### `g3d_instances_create_skinned(int, int, int) -> int`

#### `g3d_instances_set_wind(int, float) -> int`

#### `g3d_instances_set_alpha_cut(int, int) -> int`

#### `g3d_instances_set_distance(int, float) -> int`

#### `g3d_instances_clear(int) -> int`

#### `g3d_instances_count(int) -> int`

### lake
#### `g3d_lake_add(float, float, float, float) -> int`

#### `g3d_lake_add_r(float, float, float, float, float) -> int`

#### `g3d_lake_spill_level(float, float) -> float`

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
#### `g3d_load_gltf(string) -> int`

#### `g3d_load_tr(string) -> int`

#### `g3d_load_gltf_fractured(string) -> int`

#### `g3d_load_obj(string) -> int`

#### `g3d_load_fbx(string) -> int`

#### `g3d_load_md3(string) -> int`

#### `g3d_load_texture(string) -> int`

### lvl
#### `g3d_lvl_load(string) -> int`

#### `g3d_lvl_free(int) -> int`

#### `g3d_lvl_planes(int) -> int`

#### `g3d_lvl_w(int, int) -> int`

#### `g3d_lvl_h(int, int) -> int`

#### `g3d_lvl_cell(int, int, int, int, int) -> int`

#### `g3d_lvl_links(int) -> string`

### material
#### `g3d_material_create() -> int`

#### `g3d_material_set_color(int, float, float, float, float) -> int`

#### `g3d_material_set_metallic(int, float) -> int`

#### `g3d_material_set_roughness(int, float) -> int`

#### `g3d_material_set_texture(int, int, int) -> int`

#### `g3d_material_set_map(int, int, int) -> int`

#### `g3d_material_set_outline(int, int) -> int`

### mirror
#### `g3d_mirror_create(float, float, float, float, float, float, float, float) -> int`

#### `g3d_mirror_set_flip(int, int) -> int`

#### `g3d_mirror_set_tint(int, float, float, float) -> int`

#### `g3d_mirror_clear() -> int`

#### `g3d_mirror_set_distance(float) -> int`

### model
#### `g3d_model_despawn(int raiz) -> int`
Destruye una entidad raiz creada con G3D_MODEL_SPAWN, todas sus mallas hijas y sus materiales privados. Usala al crear y quitar modelos constantemente para no agotar los huecos de entidades y materiales. Tambien vale para una sola entidad con material (p. ej. un trozo de fractura).

#### `g3d_model_mesh(int) -> int`

#### `g3d_model_texture(int) -> int`

#### `g3d_model_submesh_count(int) -> int`

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

#### `g3d_model_submesh(int, int) -> int`

#### `g3d_model_submesh_texture(int, int) -> int`

#### `g3d_model_submesh_is_water(int, int) -> int`

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

#### `g3d_model_submesh_cx(int, int) -> float`

#### `g3d_model_submesh_cy(int, int) -> float`

#### `g3d_model_submesh_cz(int, int) -> float`

#### `g3d_model_submesh_hx(int, int) -> float`

#### `g3d_model_submesh_hy(int, int) -> float`

#### `g3d_model_submesh_hz(int, int) -> float`

#### `g3d_model_spawn(int escena, int modelo, float x, float y, float z, float alto, float giro_y) -> int`
Crea en la escena un modelo completo (todas sus mallas) colgando de una entidad raiz vacia. Mover, rotar o escalar esa raiz mueve todo el modelo. El modelo se escala uniformemente para que mida 'alto' de alto (si alto <= 0 se queda 1:1). El giro va en MILESIMAS DE GRADO.

**Devuelve:** id de la entidad raiz, o -1 si el modelo no es valido

```
m = g3d_load_gltf("personaje.glb");
raiz = g3d_model_spawn(escena, m, 0, 0, 0, 1.8, 0);
```

#### `g3d_model_set_gpu_skin(int modelo, int activar) -> int`
Activa (1) o desactiva (0) el skinning por GPU del modelo.

#### `g3d_model_submesh_lod(int, int, int) -> int`

#### `g3d_model_submesh_map(int, int, int) -> int`

### mouse
#### `g3d_mouse_capture(int) -> int`

#### `g3d_mouse_update() -> int`

#### `g3d_mouse_dx() -> int`

#### `g3d_mouse_dy() -> int`

### paint
#### `g3d_paint_create(int, int) -> int`

#### `g3d_paint_fill(int, int, float) -> int`

#### `g3d_paint_get_texture(int) -> int`

#### `g3d_paint_save(int, string) -> int`

### particles
#### `g3d_particles_create(float, float, float, float, float, float, float, float, float, float, float, float, float, int) -> int`

#### `g3d_particles_set_color(int, float, float, float) -> int`

#### `g3d_particles_set_floor(int, float) -> int`

#### `g3d_particles_clear() -> int`

### physics
#### `g3d_physics_body_create(int, int, float, float) -> int`

#### `g3d_physics_body_set_velocity(int, float, float, float) -> int`

#### `g3d_physics_step(float) -> int`

#### `g3d_physics_set_gravity(float) -> int`

### pick
#### `g3d_pick_terrain(int, float, float, float, float, int) -> int`

#### `g3d_pick_entity(int, float, float, float, float) -> int`

#### `g3d_pick_x() -> float`

#### `g3d_pick_y() -> float`

#### `g3d_pick_z() -> float`

### prefab
#### `g3d_prefab_create(string) -> int`

#### `g3d_prefab_add_box(int, float, float, float, float, float, float, float, float, float, float, float, float, int) -> int`

#### `g3d_prefab_piece_texture(int, string) -> int`

#### `g3d_prefab_save(int, string) -> int`

#### `g3d_prefab_load(string) -> int`

#### `g3d_prefab_instantiate(int, int, float, float, float, float) -> int`

#### `g3d_prefab_count(int) -> int`

### primitive
#### `g3d_primitive_cube() -> int`

#### `g3d_primitive_column(int) -> int`

#### `g3d_primitive_wedge() -> int`

#### `g3d_primitive_sphere(int) -> int`

#### `g3d_primitive_plane() -> int`

#### `g3d_primitive_terrain(int, float, float, float, int) -> int`

#### `g3d_primitive_cliffs(int, float, float, float, int, float, float) -> int`

#### `g3d_primitive_mountain(int, float, float, float, int, float) -> int`

### ray
#### `g3d_ray_hit_x() -> float`

#### `g3d_ray_hit_y() -> float`

#### `g3d_ray_hit_z() -> float`

#### `g3d_ray_entity() -> int`

### render
#### `g3d_render_width() -> int`

#### `g3d_render_height() -> int`

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
#### `g3d_river_begin(float, float) -> int`

#### `g3d_river_point(float, float) -> int`

#### `g3d_river_block() -> int`

#### `g3d_river_falls() -> int`

#### `g3d_river_end() -> int`

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
#### `g3d_rope_create(float, float, float, float, float, float, int, float, float) -> int`

#### `g3d_rope_pin(int, int, int) -> int`

#### `g3d_rope_pin_move(int, int, float, float, float) -> int`

#### `g3d_rope_set_texture(int, int) -> int`

#### `g3d_rope_points(int) -> int`

#### `g3d_rope_x(int, int) -> float`

#### `g3d_rope_y(int, int) -> float`

#### `g3d_rope_z(int, int) -> float`

#### `g3d_rope_update(int, float) -> int`

#### `g3d_rope_load(int, int, float) -> int`

#### `g3d_rope_set_wind(int, float, float, float, float) -> int`

#### `g3d_rope_destroy(int) -> int`

### scatter
#### `g3d_scatter_clear() -> int`

#### `g3d_scatter_load(string, float) -> int`

#### `g3d_scatter_group(string) -> int`

#### `g3d_scatter_model(int, int, int, float, float, float, float, int) -> int`

#### `g3d_scatter_mesh(int, int, int, int, float, float, float, float, int) -> int`

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
#### `g3d_set_sun(float) -> int`

#### `g3d_set_clear_color(float, float, float, float) -> int`

#### `g3d_set_ambient_light(float, float, float, float) -> int`

#### `g3d_set_fog(int, float, float, float, float, float) -> int`

#### `g3d_set_wireframe_mode(int) -> int`

#### `g3d_set_shadows(int) -> int`

#### `g3d_set_shadow_resolution(int) -> int`

#### `g3d_set_hdr(int) -> int`

#### `g3d_set_ibl(int, float) -> int`

#### `g3d_set_occlusion(int) -> int`

#### `g3d_set_smaa(int) -> int`

#### `g3d_set_fsr(int, float, float) -> int`

#### `g3d_set_fsr_height(int, int, float) -> int`

#### `g3d_set_exposure(float) -> int`

#### `g3d_set_bloom(int, float, float) -> int`

#### `g3d_set_tonemap(int) -> int`

#### `g3d_set_ssao(int, float, float) -> int`

#### `g3d_set_underwater(int, float, float, float, float) -> int`

#### `g3d_set_lod(float) -> int`

#### `g3d_set_culling(int) -> int`

#### `g3d_set_backface_cull(int) -> int`

#### `g3d_set_spawn_parent(int) -> int`

#### `g3d_set_terrain_collider(int) -> int`

### sky
#### `g3d_sky_set_gradient(float, float, float, float, float, float) -> int`

#### `g3d_sky_set_clouds(float, float) -> int`

#### `g3d_sky_set_low_clouds(float, float, float, float) -> int`

#### `g3d_sky_set_texture(int) -> int`

#### `g3d_sky_enable(int) -> int`

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
#### `g3d_stream_init(float, int) -> int`

#### `g3d_stream_update(float, float) -> int`

#### `g3d_stream_load_count() -> int`

#### `g3d_stream_load_x(int) -> int`

#### `g3d_stream_load_z(int) -> int`

#### `g3d_stream_unload_count() -> int`

#### `g3d_stream_unload_x(int) -> int`

#### `g3d_stream_unload_z(int) -> int`

#### `g3d_stream_loaded_count() -> int`

### terrain
#### `g3d_terrain_paint(int, int, int, float, float, float, float, float) -> int`

#### `g3d_terrain_get_height(int, float, float) -> float`

#### `g3d_terrain_raise(int, float, float, float, float) -> int`

#### `g3d_terrain_hole(int, float, float, float, int) -> int`

#### `g3d_terrain_smooth(int, float, float, float, float) -> int`

#### `g3d_terrain_flatten(int, float, float, float, float, float) -> int`

#### `g3d_terrain_load(int, string) -> int`

### tr
#### `g3d_tr_probe(string) -> int`

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
#### `g3d_voxterrain_floor(float, float, float) -> float`

#### `g3d_voxterrain_solid(float, float, float) -> int`

#### `g3d_voxterrain_blocked(float, float, float, float, float) -> int`

#### `g3d_voxterrain_worldsize() -> float`

#### `g3d_voxterrain_walk(float, float, float, float, float, float, float, float) -> int`

#### `g3d_voxterrain_walkx() -> float`

#### `g3d_voxterrain_walky() -> float`

#### `g3d_voxterrain_walkz() -> float`

### wall
#### `g3d_wall_ray(float, float, float, float, float, float, float) -> float`

### water
#### `g3d_water_set_ssr(int, float) -> int`

#### `g3d_water_set_ocean(float, float, float) -> int`

#### `g3d_water_set_tessellation(int) -> int`

#### `g3d_water_set_surf(float, float, float, float) -> int`

#### `g3d_water_set_surf_wave(float, float) -> int`

#### `g3d_water_set_foam(float, float) -> int`

#### `g3d_water_set_splash(float, float) -> int`

#### `g3d_water_set_caustics(float) -> int`

#### `g3d_water_set_underwater_look(float, float) -> int`

#### `g3d_water_create(float, float, int) -> int`

#### `g3d_water_level_at(float, float) -> float`

#### `g3d_water_set_waves(float, float, float) -> int`

#### `g3d_water_set_color(float, float, float, float, float, float) -> int`

#### `g3d_water_set_enabled(int) -> int`

#### `g3d_water_set_texture(int) -> int`

#### `g3d_water_set_reflection(int, float) -> int`

#### `g3d_water_set_reflection_flip(int) -> int`

#### `g3d_water_ripple(float, float, float) -> int`

#### `g3d_water_add_ripple_source(float, float, float) -> int`

#### `g3d_water_clear_ripple_sources() -> int`

#### `g3d_water_splash(float, float, float, float) -> int`

### waterfall
#### `g3d_waterfall_add(float, float, float, float, float, float, float) -> int`

### watersim
#### `g3d_watersim_clear_sources() -> int`

#### `g3d_watersim_settle(float) -> int`

#### `g3d_watersim_add_source(float, float, float) -> int`

#### `g3d_watersim_set_viscosity(float) -> int`

#### `g3d_watersim_set_evaporation(float) -> int`

### world
#### `g3d_world_rebase(float, float, float) -> int`

### worldgen
#### `g3d_worldgen_set(int, float, float, float) -> int`

#### `g3d_worldgen_height(float, float) -> float`

#### `g3d_worldgen_tile(int, int, int, float, int, float, float, int) -> int`

#### `g3d_worldgen_tile_free(int) -> int`

#### `g3d_worldgen_set_water_depth(float) -> int`

#### `g3d_worldgen_set_biome_textures(int, int, int, int, float) -> int`

### xray
#### `g3d_xray_update(float, float, float, float, float, float, float, float, float) -> int`

### zone
#### `g3d_zone_clear() -> int`

#### `g3d_zone_init(int, float) -> int`

#### `g3d_zone_load(string) -> int`

#### `g3d_zone_blocked(float, float, int) -> int`

## Modulo ads
### general
#### `ads_initialize() -> int`

#### `ads_load_interstitial(string) -> int`

#### `ads_show_interstitial() -> int`

#### `ads_is_interstitial_loaded() -> int`

#### `ads_load_rewarded(string) -> int`

#### `ads_show_rewarded() -> int`

#### `ads_is_rewarded_loaded() -> int`

#### `ads_show_banner(string, int) -> int`

#### `ads_hide_banner() -> int`

#### `ads_poll_event() -> int`

#### `ads_check_consent() -> int`

#### `ads_show_consent_form() -> int`

#### `ads_set_child_directed(int) -> int`

## Modulo debug
### general
#### `trace(int) -> int`

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

#### `get_real_point(int, double, double, pointer, pointer) -> int`

#### `get_real_point(int, int, pointer, pointer) -> int`

#### `get_real_point(int, pointer, pointer) -> int`

#### `collision(int proceso, int objetivo) -> int`
Igual, pero para el proceso indicado en vez de para este.

**Devuelve:** identificador del proceso con el que choca, o 0

#### `collision(int objetivo) -> int`
Comprueba si este proceso choca con otros. Se le pasa el tipo de proceso (TYPE nombre), el identificador de uno concreto, o -1 para el raton. La colision se calcula con la forma real del grafico de cada proceso.

**Devuelve:** identificador del proceso con el que choca, o 0

```
IF (collision(TYPE bala)) vida--; END
```

#### `scroll_start(int, int, int, int, int, int, int, int, int) -> int`

#### `scroll_start(int, int, int, int, int, int, int) -> int`

#### `scroll_start(int, int, int, int, int, int) -> int`

#### `scroll_stop(int) -> int`

#### `scroll_move(int) -> int`

#### `region_define(int, int, int, int, int) -> int`

#### `region_out(int, int) -> int`

#### `screen_get() -> int`

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

#### `rgb_get(int, int, int, pointer, pointer, pointer) -> int`

#### `rgba_get(int color, pointer &r, pointer &g, pointer &b, pointer &alfa) -> int`
Separa un color en rojo, verde, azul y alfa, en cuatro variables que se pasan por puntero.

#### `rgba_get(int, int, int, pointer, pointer, pointer, pointer) -> int`

#### `fade(int r, int g, int b, int alfa, int velocidad) -> int`
Funde la pantalla hacia un color. Los cuatro primeros valores son el color de destino (rojo, verde, azul y su alfa/intensidad) y 'velocidad' es lo rapido que se hace.

#### `fade(int, int, int, int, int, int) -> int`

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

#### `window_get_size(pointer, pointer, pointer, pointer, pointer, pointer) -> int`

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

#### `window_set_dpi_aware() -> int`

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

#### `write_in_map(int, int, int, int, int, string, int) -> int`

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

#### `write_value(int, int, int, int, int, pointer) -> int`

#### `write_value(int, int, int, int, int, int, pointer) -> int`

#### `write_get_rgba(int, pointer, pointer, pointer, pointer) -> int`

#### `write_set_rgba(int, byte, byte, byte, byte) -> int`

#### `map_block_copy(int, int, int, int, int, int, int, int, int, int, int) -> int`

#### `map_block_copy(int, int, int, int, int, int, int, int, int, int, int, byte, byte, byte, byte) -> int`

#### `map_block_copy(int, int, int, int, int, int, int, int, int, int, int, byte, byte, byte, byte, int) -> int`

#### `map_block_copy(int, int, int, int, int, int, int, int, int, int, int, byte, byte, byte, byte, int, int, int, int, int, int, int) -> int`

#### `map_block_copy(int, int, int, int, int, int, int, int, int, int) -> int`

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y) -> int`
Copia un grafico (origen) sobre otro (destino) en la posicion (x, y).

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y, int angulo, int escala_x, int escala_y, int flags, byte r, byte g, byte b, byte alfa) -> int`
Copia un grafico sobre otro con todas las opciones: angulo (milesimas de grado), escala en porcentaje (100 = tamano normal), banderas (B_HMIRROR, B_VMIRROR, B_TRANSLUCENT...), tinte (r, g, b) y alfa (0 a 255).

#### `map_put(int archivo_destino, int grafico_destino, int archivo_origen, int grafico_origen, int x, int y, int angulo, int escala_x, int escala_y, int flags, byte r, byte g, byte b, byte alfa, int mezcla) -> int`
Igual, con modo de mezcla (BLEND_NORMAL, BLEND_ADD, BLEND_MULTIPLY, BLEND_SUBTRACT...).

#### `map_put(int, int, int, int, int, int, int, int, int, int, byte, byte, byte, byte, int, int, int, int, int, int, int) -> int`

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

#### `map_clear(int, int, int, int, int, int, int) -> int`

#### `map_clone(int archivo, int grafico) -> int`
Crea una copia de un grafico y devuelve el codigo de la copia.

**Devuelve:** codigo de la copia

#### `map_name(int, int) -> string`

#### `map_set_name(int, int, string) -> int`

#### `map_exists(int archivo, int grafico) -> int`
Indica si existe un grafico.

**Devuelve:** 1 si existe

#### `map_del(int, int) -> int`

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

#### `fpg_del(int) -> int`

#### `fpg_unload(int archivo) -> int`
Libera una coleccion de graficos.

#### `map_info_set(int, int, int, int) -> int`

#### `map_info(int, int, int, int) -> int`

#### `map_info(int, int, int) -> int`

#### `graphic_set(int, int, int, int) -> int`

#### `graphic_info(int, int, int) -> int`

#### `point_get(int, int, int, pointer, pointer) -> int`

#### `point_set(int, int, int, int, int) -> int`

#### `center_set(int, int, int, int) -> int`

#### `point_get_total(int, int) -> int`

#### `box_set(int, int, int, int, int, int, int, int) -> int`

#### `box_set(int, int, int, int, int, int) -> int`

#### `box_get(int, int, int, pointer, pointer, pointer, pointer, pointer) -> int`

#### `box_get(int, pointer, pointer, pointer, pointer, pointer) -> int`

#### `box_get_by_pos(int, int, int, pointer, pointer, pointer, pointer, pointer, pointer) -> int`

#### `box_get_by_pos(int, pointer, pointer, pointer, pointer, pointer, pointer) -> int`

#### `box_remove(int, int, int) -> int`

#### `box_remove(int) -> int`

#### `box_get_real_vertex(int, int, int, int, pointer, pointer) -> int`

#### `box_get_real_vertex(int, int, int, pointer, pointer) -> int`

#### `box_get_real_vertex(int, pointer, pointer) -> int`

#### `fnt_new() -> int`

#### `fnt_new(int) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, string) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, int, int, int, int) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, int, int, int, int, string) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, int, int, int, int, int) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, string, int, int, int, int, int) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int) -> int`

#### `fnt_new(int, int, int, int, int, int, int, int, int, int, int, int, string, int, int, int, int, int) -> int`

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

#### `ttf_get_family(int) -> string`

#### `ttf_get_style(int) -> string`

#### `glyph_get(int, int) -> int`

#### `glyph_set(int, int, int, int) -> int`

#### `drawing_color(int color) -> int`
Color con el que se dibujaran los siguientes DRAW_*.

#### `drawing_color(int dibujo, int color) -> int`
Cambia el color de un dibujo ya creado.

#### `drawing_rgba(byte, byte, byte, byte) -> int`

#### `drawing_rgba(int, byte, byte, byte, byte) -> int`

#### `drawing_blendmode(int) -> int`

#### `drawing_blendmode(int, int) -> int`

#### `drawing_blendmode(int, int, int, int, int, int) -> int`

#### `drawing_blendmode(int, int, int, int, int, int, int) -> int`

#### `drawing_z() -> int`

#### `drawing_z(int z) -> int`
Profundidad (z) de los siguientes dibujos: menor z se dibuja delante.

#### `drawing_z(int, int) -> int`

#### `drawing_map(int archivo, int grafico) -> int`
Hace que los siguientes DRAW_* dibujen dentro de un grafico en vez de en pantalla. Con (0, 0), que no es un grafico, se vuelve a dibujar en pantalla.

#### `draw_delete(int dibujo) -> int`
Borra un dibujo creado con un DRAW_* (0 borra todos).

#### `draw_move(int dibujo, int x, int y) -> int`
Mueve un dibujo ya creado.

#### `draw_coords(int, pointer, pointer, pointer) -> int`

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

#### `draw_points(int, pointer) -> int`

#### `draw_points(int, pointer, int) -> int`

#### `draw_lines(int, pointer) -> int`

#### `draw_lines(int, pointer, int) -> int`

#### `draw_rectangles(int, pointer) -> int`

#### `draw_rectangles(int, pointer, int) -> int`

#### `draw_rectangles_filled(int, pointer) -> int`

#### `draw_rectangles_filled(int, pointer, int) -> int`

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

#### `draw_curve(int, int, int, int, int, int, int, int, int) -> int`

#### `draw_curve(int, int, int, int, int, int, int, int, int, int) -> int`

#### `draw_arc(int, int, int, int, int) -> int`

#### `draw_arc(int, int, int, int, int, int) -> int`

#### `draw_arc_filled(int, int, int, int, int) -> int`

#### `draw_arc_filled(int, int, int, int, int, int) -> int`

#### `draw_ellipse(int, int, int, int, int) -> int`

#### `draw_ellipse(int, int, int, int, int, int) -> int`

#### `draw_ellipse_filled(int, int, int, int, int) -> int`

#### `draw_ellipse_filled(int, int, int, int, int, int) -> int`

#### `draw_sector(int, int, int, int, int, int) -> int`

#### `draw_sector(int, int, int, int, int, int, int) -> int`

#### `draw_sector_filled(int, int, int, int, int, int) -> int`

#### `draw_sector_filled(int, int, int, int, int, int, int) -> int`

#### `draw_triangle(int, int, int, int, int, int) -> int`

#### `draw_triangle(int, int, int, int, int, int, int) -> int`

#### `draw_triangle_filled(int, int, int, int, int, int) -> int`

#### `draw_triangle_filled(int, int, int, int, int, int, int) -> int`

#### `draw_rectangle_round(int, int, int, int, int) -> int`

#### `draw_rectangle_round(int, int, int, int, int, int) -> int`

#### `draw_rectangle_round_filled(int, int, int, int, int) -> int`

#### `draw_rectangle_round_filled(int, int, int, int, int, int) -> int`

#### `draw_polygon(int, pointer) -> int`

#### `draw_polygon(int, pointer, int) -> int`

#### `draw_polygon_filled(int, pointer) -> int`

#### `draw_polygon_filled(int, pointer, int) -> int`

#### `draw_polyline(int, pointer, int) -> int`

#### `draw_polyline(int, pointer, int, int) -> int`

#### `draw_set_thickness(float) -> float`

#### `draw_set_thickness(int, float) -> float`

#### `draw_get_thickness() -> float`

#### `draw_get_thickness(int) -> float`

#### `path_new(int, int) -> pointer`

#### `path_destroy(pointer) -> int`

#### `path_find(pointer, int, int, int, int, int) -> pointer`

#### `path_find(pointer, int, int, int, int, int, int, int) -> pointer`

#### `path_free_results(pointer) -> int`

#### `texture_set_quality(int) -> int`

#### `map_set_palette(int, int, int, int, pointer) -> int`

#### `map_get_palette(int, int, int, int, pointer) -> int`

#### `shader_get_language() -> int`

#### `shader_get_min_version() -> int`

#### `shader_get_max_version() -> int`

#### `shader_create(string, string) -> pointer`

#### `shader_free(pointer) -> int`

#### `shader_getattributelocation(pointer, string) -> int`

#### `shader_get_param_location(pointer, string) -> int`

#### `shader_create_params(int) -> pointer`

#### `shader_free_params(pointer) -> int`

#### `shader_set_param(pointer, int, int, int) -> int`

#### `shader_set_param(pointer, int, int, float) -> int`

#### `shader_set_param(pointer, int, int, int, int, int) -> int`

#### `shader_set_param(pointer, int, int, int, pointer) -> int`

#### `shader_set_param(pointer, int, int, pointer, int, int, int, int) -> int`

#### `shader_get_param(pointer, int, pointer, pointer) -> int`

#### `shader_get_param(pointer, int, pointer, pointer, pointer) -> int`

#### `shader_get_param(pointer, int, pointer, pointer, pointer, pointer, pointer, pointer) -> int`

#### `media_load(string, pointer) -> pointer`

#### `media_load(string, pointer, int) -> pointer`

#### `media_unload(pointer) -> int`

#### `media_play(pointer) -> int`

#### `media_pause(pointer) -> int`

#### `media_pause(pointer, int) -> int`

#### `media_resume(pointer) -> int`

#### `media_stop(pointer) -> int`

#### `media_get_time(pointer) -> int`

#### `media_get_duration(pointer) -> int`

#### `media_get_status(pointer) -> int`

#### `media_get_mute(pointer) -> int`

#### `media_set_mute(pointer, int) -> int`

#### `media_get_volume(pointer) -> int`

#### `media_set_volume(pointer, int) -> int`

## Modulo iap
### general
#### `iap_initialize() -> int`

#### `iap_purchase(string) -> int`

#### `iap_consume(string) -> int`

#### `iap_get_price(string) -> string`

#### `iap_poll_event() -> int`

#### `iap_get_last_purchase_sku() -> string`

#### `iap_get_last_purchase_token() -> string`

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
#### `crypt_new(int, pointer) -> pointer`

#### `crypt_del(pointer) -> int`

#### `crypt_encrypt(pointer, pointer, pointer, int) -> int`

#### `crypt_decrypt(pointer, pointer, pointer, int) -> int`

#### `crypt_encrypt(int, pointer, pointer, pointer, int) -> int`

#### `crypt_decrypt(int, pointer, pointer, pointer, int) -> int`

#### `cd() -> string`

#### `cd(string) -> int`

#### `chdir(string) -> int`

#### `mkdir(string) -> int`

#### `rmdir(string) -> int`

#### `glob(string) -> string`

#### `rm(string) -> int`

#### `diropen(string) -> int`

#### `dirclose(int) -> int`

#### `dirread(int) -> string`

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

#### `file(string) -> string`

#### `fopen(string fichero, int modo) -> int`
Abre un fichero y devuelve su manejador (0 si falla). El modo es O_READ (0, leer), O_READWRITE (1), O_WRITE (2, escribir), O_ZREAD (3, leer comprimido) u O_ZWRITE (4, escribir comprimido).

**Devuelve:** manejador del fichero

```
f = fopen("datos.txt", O_READ);
IF (f) linea = fgets(f); fclose(f); END
```

#### `fclose(int fichero) -> int`
Cierra un fichero abierto con FOPEN.

#### `fread(int, ..., ?, ?) -> int`

#### `fread(pointer, int, int) -> int`

#### `fwrite(int, ..., ?, ?) -> int`

#### `fwrite(pointer, int, int) -> int`

#### `fseek(int, int, int) -> int`

#### `frewind(int fichero)`
Vuelve al principio de un fichero abierto.

#### `ftell(int fichero) -> int`
Posicion actual de lectura o escritura de un fichero abierto.

**Devuelve:** posicion

#### `fflush(int) -> int`

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

#### `decimal(double) -> double`

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

#### `mmod(double, double) -> double`

#### `modulus(double, double) -> double`

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

#### `isinf(double) -> int`

#### `isnan(double) -> int`

#### `finite(double) -> int`

#### `intersect(double, double, double, double, double, double, double, double, pointer, pointer) -> int`

#### `intersect_line_circle(double, double, double, double, double, double, double, pointer, pointer, pointer, pointer) -> int`

#### `intersect_circle(double, double, double, double, double, double, pointer, pointer, pointer, pointer) -> int`

#### `ortho(double, double, double, double, double, double, pointer, pointer) -> double`

#### `project(double, double, double, double, double, double, pointer, pointer, pointer, pointer) -> double`

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

#### `get_distx(int, double) -> double`

#### `get_disty(int, double) -> double`

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

#### `rangechk(double, double, double) -> int`

#### `rangechk(int, int, int) -> int`

#### `remap(double valor, double desde_min, double desde_max, double hasta_min, double hasta_max) -> double`
Convierte un valor de un rango a otro: remap(5, 0, 10, 0, 100) = 50.

**Devuelve:** el valor convertido

#### `remap(int valor, int desde_min, int desde_max, int hasta_min, int hasta_max) -> int`
Lo mismo con enteros.

**Devuelve:** el valor convertido

#### `normalize(double valor, double bajo, double alto) -> double`
Convierte un valor del rango [bajo, alto] a un numero entre 0 y 1.

**Devuelve:** la fraccion

#### `normalize(int, int, int) -> double`

#### `mem_calloc(int, int) -> pointer`

#### `mem_alloc(int) -> pointer`

#### `mem_free(pointer) -> int`

#### `mem_realloc(pointer, int) -> pointer`

#### `mem_cmp(pointer, pointer, int) -> int`

#### `mem_set(pointer, int, int) -> int`

#### `mem_setw(pointer, int, int) -> int`

#### `mem_setd(pointer, int, int) -> int`

#### `mem_seti(pointer, int, int) -> int`

#### `mem_copy(pointer, pointer, int) -> int`

#### `mem_move(pointer, pointer, int) -> int`

#### `mem_available() -> int`

#### `mem_total() -> int`

#### `calloc(int, int) -> pointer`

#### `alloc(int) -> pointer`

#### `free(pointer) -> int`

#### `realloc(pointer, int) -> pointer`

#### `memcmp(pointer, pointer, int) -> int`

#### `memset(pointer, int, int) -> int`

#### `memsetw(pointer, int, int) -> int`

#### `memsetd(pointer, int, int) -> int`

#### `memseti(pointer, int, int) -> int`

#### `memcopy(pointer, pointer, int) -> int`

#### `memmove(pointer, pointer, int) -> int`

#### `memory_free() -> int`

#### `memory_total() -> int`

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

#### `signal_action(int, int) -> int`

#### `signal_action(int, int, int) -> int`

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

#### `resume() -> int`

#### `pause(int) -> int`

#### `resume(int) -> int`

#### `rand_seed(int semilla) -> int`
Fija la semilla del generador de numeros al azar: con la misma semilla salen siempre los mismos numeros.

#### `rand(int minimo, int maximo) -> int`
Numero entero al azar entre dos valores, ambos incluidos (el orden da igual).

**Devuelve:** un entero al azar

```
dado = rand(1, 6);
```

#### `regex(string, string) -> int`

#### `regex_replace(string, string, string) -> string`

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

#### `isort(pointer, int, int, int, byte, byte) -> int`

#### `ksort(..., ?, ?, ..., ?, ?) -> int`

#### `ksort(..., ?, ?, ..., ?, ?, int) -> int`

#### `sort(..., ?, ?, int) -> int`

#### `sort(..., ?, ?) -> int`

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

#### `strtok(string, string) -> string`

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

#### `getenv(string) -> string`

#### `exec(int, string, int, pointer) -> int`

#### `system_keyboard_open(string, string, int, int) -> int`

#### `system_keyboard_status() -> int`

#### `system_keyboard_ok() -> int`

#### `system_keyboard_text() -> string`

#### `system_keyboard_close() -> int`

#### `system_progress_open(string, int) -> int`

#### `system_progress_set(int) -> int`

#### `system_progress_message(string) -> int`

#### `system_progress_update() -> int`

#### `system_progress_close() -> int`

#### `get_pref_language() -> string`

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

#### `string_new_array(int) -> pointer`

#### `string_resize_array(pointer, int) -> pointer`

#### `string_del_array(pointer) -> int`

#### `string_size_array(pointer) -> int`

#### `string_newa() -> pointer`

#### `string_newa(int) -> pointer`

#### `string_resizea(pointer, int) -> pointer`

#### `string_dela(pointer) -> int`

#### `string_sizea(pointer) -> int`

#### `string_dump() -> int`

#### `isalpha(byte) -> int`

#### `isspace(byte) -> int`

#### `isnum(byte) -> int`

#### `isxnum(byte) -> int`

#### `isalnum(byte) -> int`

#### `iswordchar(byte) -> int`

#### `iswordfirst(byte) -> int`

#### `toupper(byte) -> int`

#### `tolower(byte) -> int`

#### `list_create() -> pointer`

#### `list_create(int) -> pointer`

#### `list_free(pointer) -> int`

#### `list_empty(pointer) -> int`

#### `list_insertitem(pointer, pointer) -> int`

#### `list_removeitem(pointer, pointer) -> int`

#### `list_walk(pointer, pointer) -> pointer`

#### `list_size(pointer) -> int`

## Modulo net
### general
#### `net_open(int, int, string, int) -> pointer`

#### `net_wait(pointer, int, pointer) -> int`

#### `net_send(pointer, pointer, int) -> int`

#### `net_recv(pointer, pointer, int) -> int`

#### `net_close(pointer) -> int`

#### `net_getavailablebytes(pointer) -> int`

#### `net_getreceivebuffersize(pointer) -> int`

#### `net_getremoteaddr(pointer) -> string`

#### `net_is_new_connection(pointer) -> int`

#### `net_is_message_incoming(pointer) -> int`

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
#### `video_open(string) -> int`

#### `video_open_h(string, string) -> int`

#### `video_graph(int) -> int`

#### `video_play(int, int) -> int`

#### `video_pause(int, int) -> int`

#### `video_stop(int) -> int`

#### `video_seek(int, int) -> int`

#### `video_close(int) -> int`

#### `video_width(int) -> int`

#### `video_height(int) -> int`

#### `video_duration(int) -> int`

#### `video_position(int) -> int`

#### `video_is_playing(int) -> int`

#### `video_is_finished(int) -> int`

#### `video_volume(int, int) -> int`

#### `video_use_gpu(int) -> int`

#### `video_gpu(int) -> int`

#### `video_frames(int) -> int`

#### `video_use_hw(int) -> int`

#### `video_use_plane(int) -> int`

#### `video_hw(int) -> int`

#### `video_subtitle_tracks(int) -> int`

#### `video_subtitle_lang(int, int) -> string`

#### `video_subtitle_select(int, int) -> int`

#### `video_subtitle_load(int, string) -> int`

#### `video_subtitle_text(int) -> string`

#### `video_audio_tracks(int) -> int`

#### `video_audio_lang(int, int) -> string`

#### `video_audio_select(int, int) -> int`

#### `video_is_audio_only(int) -> int`

#### `video_spectrum(int, int) -> int`

#### `video_wave(int, int) -> int`

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

#### `video_speed(int, int) -> int`

#### `video_chapters(int) -> int`

#### `video_chapter_start(int, int) -> int`

#### `video_chapter_title(int, int) -> string`

#### `video_get_speed(int) -> int`

#### `video_sub_delay(int, int) -> int`

#### `video_av_offset(int, int) -> int`

#### `upload_ip() -> string`

#### `upload_error() -> string`

