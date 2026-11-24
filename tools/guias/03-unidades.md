# Unidades y convenciones

Es la fuente más habitual de errores, así que conviene tenerlo a mano.

## Ángulos

| Dónde | Unidad |
|---|---|
| `angle_x/y/z` de un proceso, `g3d_entity_set_rotation`, `g3d_model_spawn` (giro) | **milésimas de grado** (90000 = 90°) |
| `g3d_rigidbody_angle_x/y/z`, `g3d_vehicle_yaw/pitch/roll` | milésimas de grado |
| `g3d_vehicle_create` (rumbo), `g3d_sprite_dir` (ángulo) | milésimas de grado |
| `fov` de la cámara, `g3d_camera_set_fov` | **grados** |
| `cone_angle` de un foco, `g3d_light_set_cone` | grados |
| `g3d_char_set_tuning` (pendiente), `g3d_vehicle_set_tuning` (giro máximo) | grados |
| `g3d_collider_add_model_yaw` (giro) | grados |
| `g3d_model_orient` | **radianes** |

Un truco: las rotaciones que salen del motor (`g3d_rigidbody_angle_*`) ya vienen en milésimas de grado
**para que se las pases tal cual** a `g3d_entity_set_rotation`.

## Escala, color, opacidad

| Qué | Unidad |
|---|---|
| `size`, `size_x/y/z` (proceso) | porcentaje (100 = tamaño normal) |
| `g3d_entity_set_scale` (entidad sin proceso) | factor (1.0 = tamaño normal) |
| `alpha`, `g3d_entity_set_alpha` | 0 a 255 |
| `color_r/g/b`, `g3d_entity_set_color` | 0 a 255 |
| `g3d_light_set_color` | 0.0 a 1.0 |

## Ejes

Y es hacia **arriba**. El suelo es el plano X-Z. La gravedad tira hacia -Y.

**Izquierda y derecha dependen de hacia dónde mira la cámara.** La proyección es la estándar de OpenGL:

| La cámara mira hacia... | +X queda a la... |
|---|---|
| **+Z** (cámara detrás, en z negativa) | **izquierda** |
| **-Z** (cámara delante, en z positiva) | **derecha** |

Esto se comprobó con una imagen: con la cámara en `z = -8` mirando al origen, un cubo en `x = +3` se ve a la
**izquierda**. Por eso, en un juego visto desde atrás (mirando a +Z), "ir a la derecha" es `-X`; y en uno
lateral conviene poner la cámara en z positiva para que +X sea la derecha de la pantalla.

El **rumbo** (vehículo, carretera) vale 0 mirando a +Z y crece hacia +X, es decir, hacia la izquierda de la
pantalla si la cámara está detrás. Un rumbo creciente es un giro **a la izquierda** visto desde atrás.

## Identificadores

Casi todas las funciones `*_create` devuelven un número que identifica lo creado
(escena, cámara, luz, entidad, personaje, cuerpo...). Cada tipo numera por separado y empieza en 0,
así que **0 es un identificador válido**. Cuando falla devuelven `-1`.

Los **modelos** y **mallas** (`g3d_load_gltf`, `g3d_primitive_cube`...) devuelven un identificador opaco
que solo sirve para pasárselo a otras funciones (`g3d_model_spawn`, `g3d_entity_set_mesh`...).

## Tiempo

Las funciones de simulación (`g3d_char_update`, `g3d_rigidbody_step`, `g3d_vehicle_update`) reciben
`dt` en **segundos**. Si pasas un `dt` demasiado grande se recorta (0.05 s en el personaje y en los
cuerpos rígidos, 0.1 s en el vehículo), para que un tirón no rompa la simulación.
