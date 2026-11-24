# Primeros pasos

Un programa 3D en BennuGD2 son **procesos normales** que el motor 3D reconoce. No hay un
bucle de render que escribir: el motor dibuja la escena activa cada frame, igual que BennuGD2
dibuja los gráficos 2D.

## Lo mínimo: escena, cámara, luz y un objeto

Cada cosa de la escena es un `PROCESS` con `ctype = C_3D` y un `csubtype` que dice qué es
(`C3D_CAMERA`, `C3D_LIGHT`, `C3D_ENTITY`...). El motor lee las variables del proceso (`x`, `y`, `z`,
`angle_y`, `target_x`...) y las aplica a la escena en cada frame, así que moverlo es cambiar una variable.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

GLOBAL
    int escena;
    int camara;

// Una luz de sol. En una luz, la dirección es (target - posición).
PROCESS sol()
BEGIN
    ctype = C_3D; csubtype = C3D_LIGHT;
    entity = g3d_light_create(G3D_LIGHT_DIRECTIONAL);
    target_x = 0.4; target_y = -0.8; target_z = 0.3;
    intensity = 0.6;
    LOOP FRAME; END
END

// La cámara: dónde está y a dónde mira.
PROCESS camara_fija()
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    x = 0.0; y = 3.0; z = -8.0;
    target_x = 0.0; target_y = 1.0; target_z = 0.0;
    fov = 60.0;
    LOOP FRAME; END
END

// Un cubo que gira. angle_y va en MILÉSIMAS DE GRADO: 1000 = 1 grado por frame.
PROCESS cubo()
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, 0.0, 1.0, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());
    x = 0.0; y = 1.0; z = 0.0;      // la posición la manda el PROCESO, no el spawn
    LOOP
        angle_y += 1000;
        FRAME;
    END
END

PROCESS main()
BEGIN
    escena = g3d_scene_create("demo");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);

    sol();
    camara_fija();
    cubo();

    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Qué has aprendido

- La **escena** es el contenedor; solo se dibuja la activa (`g3d_scene_set_active`).
- La **cámara** y la **luz** se crean con `g3d_camera_create` / `g3d_light_create` y se atan al
  proceso con la variable `entity`.
- **El cielo es obligatorio.** Sin `g3d_sky_set_gradient` y `g3d_sky_enable(1)` la pantalla sale negra.
- **La posición la manda el proceso.** Cada frame el motor pone la entidad en las `x`, `y`, `z` del proceso
  (que valen 0 al empezar), así que la posición que pasas a `g3d_entity_spawn` solo sirve hasta el primer
  frame: asigna también `x`, `y`, `z`.
- Hasta que `entity` no vale algo distinto de `-1`, el motor ignora el proceso.
- Si prefieres no usar procesos, todo lo anterior también se puede hacer llamando a las funciones
  (`g3d_camera_set_position`, `g3d_entity_set_position`...). **Ojo:** si hay un proceso atado a esa
  cámara o entidad, el proceso pisa lo que hagas a mano en el siguiente frame.

## Siguiente paso

[Procesos 3D](#guia/02-procesos-3d) explica todas las variables que el motor lee de un proceso.
