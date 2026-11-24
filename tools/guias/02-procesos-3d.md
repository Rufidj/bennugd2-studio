# Procesos 3D (componentes)

Un proceso con `ctype = C_3D` es un **componente** de la escena. Su `csubtype` decide de qué tipo.
Cada frame, antes de dibujar, el motor copia las variables del proceso a la escena. El proceso solo
empieza a contar cuando `entity` deja de valer `-1`.
**Eso incluye la posición**: la entidad se coloca en las `x`, `y`, `z` del proceso (0 por defecto), no donde
la creaste con `g3d_entity_spawn`.

> **Ojo con las funciones sueltas.** Si una entidad pertenece a un proceso, el proceso **pisa cada frame**
> lo que hagas con `g3d_entity_set_position`, `set_rotation`, `set_scale`, `set_color`, `set_alpha` y
> `set_blend`. Dentro de un proceso usa sus variables (`x`, `angle_y`, `size_x`, `color_r`, `alpha`...).
> Las funciones sueltas son para entidades que **no** tienen proceso.

## Subtipos

| `csubtype` | Qué es | `entity` se obtiene con |
|---|---|---|
| `C3D_ENTITY` | Un objeto (modelo o malla) | `g3d_entity_spawn`, `g3d_model_spawn` |
| `C3D_LIGHT` | Una luz | `g3d_light_create` |
| `C3D_CAMERA` | Una cámara | `g3d_camera_create` |
| `C3D_SPRITE` | Un gráfico 2D plantado en el mundo | `g3d_sprite_create` |
| `C3D_WATER` | El agua entera del mundo | — |
| `C3D_CLOTH` | Una tela (bandera, cortina) | — |
| `C3D_ROPE` | Una cuerda | — |
| `C3D_SCATTER` | Una especie de vegetación sembrada | — |

## Variables comunes (todas las que tienen posición)

| Variable | Unidad | Efecto |
|---|---|---|
| `x`, `y`, `z` | unidades de mundo | posición |
| `angle_x`, `angle_y`, `angle_z` | **milésimas de grado** (90000 = 90°) | rotación |
| `size` | porcentaje (100 = tamaño normal) | escala uniforme |
| `size_x`, `size_y`, `size_z` | porcentaje | escala por eje; si valen 100 manda `size` |
| `alpha` | 0 a 255 | opacidad |
| `color_r`, `color_g`, `color_b` | 0 a 255 | tinte (255,255,255 = sin tinte) |

## Entidad

```bennugd
PROCESS caja()
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, 0.0, 0.0, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());
    size = 150;                      // un 50 % más grande
    color_r = 255; color_g = 120; color_b = 120;   // tinte rojizo
    LOOP
        angle_y += 500;              // medio grado por frame
        FRAME;
    END
END
```

## Luz

La dirección de una luz es **`target` menos la posición**. Para el sol basta con dejar `x,y,z` en 0
y poner `target` como la dirección en la que viaja la luz.

| Variable | Efecto |
|---|---|
| `target_x/y/z` | hacia dónde apunta (luz direccional o foco) |
| `intensity` | intensidad (por debajo de 0 se trata como 0) |
| `range` | alcance (luces puntuales y focos) |
| `cone_angle` | ángulo del cono de un foco, en grados |
| `color_r/g/b` | color, de 0 a 255 |

```bennugd
PROCESS linterna()
BEGIN
    ctype = C_3D; csubtype = C3D_LIGHT;
    entity = g3d_light_create(G3D_LIGHT_SPOT);
    x = 0.0; y = 4.0; z = 0.0;
    target_x = 0.0; target_y = 0.0; target_z = 0.0;   // mira hacia el suelo
    cone_angle = 35.0;
    range = 12.0;
    intensity = 2.0;
    LOOP FRAME; END
END
```

## Cámara

| Variable | Efecto |
|---|---|
| `x,y,z` | posición de la cámara |
| `target_x/y/z` | punto al que mira |
| `fov` | campo de visión vertical, **en grados** (por defecto 60) |

Para una cámara que sigue algo, cambia `x,y,z` y `target` en el `LOOP`:

```bennugd
PROCESS camara_sigue(int objetivo_entity)
PRIVATE float ox; float oy; float oz;
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    LOOP
        g3d_entity_get_position(objetivo_entity, &ox, &oy, &oz);
        target_x = ox; target_y = oy + 1.0; target_z = oz;
        x = ox; y = oy + 4.0; z = oz - 8.0;
        FRAME;
    END
END
```

> Con la cámara hecha proceso, `fov` (por defecto 60) se vuelve a aplicar cada frame. Si quieres
> otro ángulo, ponlo en la variable `fov` del proceso en vez de llamar a `g3d_camera_set_fov`.

## Agua, tela, cuerda y vegetación

Estos procesos **son** el elemento entero, no una instancia:

- `C3D_WATER`: parámetros en la estructura `water` (`water.waves`, `water.wave_len`, `water.wave_speed`,
  `water.foam`, `water.surf`, `water.splash`, `water.evaporation`, `water.flow`).
- `C3D_CLOTH` y `C3D_ROPE`: `wind` es la fuerza del viento y `target_x/y/z` hacia dónde sopla.
- `C3D_SCATTER`: `wind` (balanceo), `draw_dist` (distancia a la que deja de dibujarse) y `solid`
  (si bloquea el paso).
