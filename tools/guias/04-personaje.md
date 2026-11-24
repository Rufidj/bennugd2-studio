# Un personaje que camina

El controlador de personaje (`g3d_char_*`) es una **cápsula** que camina por el terreno y por las cajas de
colisión, con gravedad, saltos y escalones. No dibuja nada: tú le pones un modelo encima.

El ciclo es siempre el mismo: **pedir movimiento → actualizar → leer la posición**.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

GLOBAL
    int escena;
    int camara;

CONST
    VELOCIDAD = 6.0;     // unidades por segundo
    SALTO     = 9.0;     // velocidad inicial del salto
    DT        = 0.0166;  // un frame a 60 fps, en segundos
END

PROCESS sol()
BEGIN
    ctype = C_3D; csubtype = C3D_LIGHT;
    entity = g3d_light_create(G3D_LIGHT_DIRECTIONAL);
    target_x = 0.4; target_y = -0.8; target_z = 0.3;
    intensity = 0.6;                  // por defecto es 1.0 y los objetos blancos salen quemados
    LOOP FRAME; END
END

PROCESS camara_tercera(int objetivo)
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    LOOP
        target_x = g3d_char_x(objetivo);
        target_y = g3d_char_y(objetivo) + 1.0;
        target_z = g3d_char_z(objetivo);
        x = target_x;
        y = target_y + 4.0;
        z = target_z - 8.0;
        FRAME;
    END
END

// El personaje: una cápsula invisible con un cubo encima para verla.
PROCESS jugador()
PRIVATE
    int pj;          // id del controlador de personaje
    float vx; float vz;
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, 0.0, 0.0, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());

    // x, y, z (pies), radio y altura de la cápsula
    pj = g3d_char_create(0.0, 2.0, 0.0, 0.4, 1.8);

    // Camara que lo sigue (el id del personaje es lo que necesita)
    camara_tercera(pj);

    LOOP
        vx = 0.0; vz = 0.0;
        // La cámara va detrás mirando a +Z, y desde ahí +X queda a la IZQUIERDA: ir a la derecha es -X.
        IF (key(_D)) vx = -VELOCIDAD; END
        IF (key(_A)) vx =  VELOCIDAD; END
        IF (key(_W)) vz =  VELOCIDAD; END
        IF (key(_S)) vz = -VELOCIDAD; END

        g3d_char_move(pj, vx, vz);                    // velocidad deseada en el plano X,Z
        IF (key(_SPACE) AND g3d_char_grounded(pj)) g3d_char_jump(pj, SALTO); END
        g3d_char_update(pj, DT);                      // aplica gravedad y colisiones

        x = g3d_char_x(pj); y = g3d_char_y(pj); z = g3d_char_z(pj);
        FRAME;
    END
END

PROCESS main()
BEGIN
    escena = g3d_scene_create("mundo");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);

    // Un suelo de colisión: una caja de -50 a 50 en X y Z, de -1 a 0 en Y.
    g3d_collider_add_box(-50.0, -1.0, -50.0, 50.0, 0.0, 50.0);

    sol();
    jugador();

    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Piezas clave

- **`g3d_char_create(x, y, z, radio, alto)`** crea la cápsula y devuelve su id.
- **`g3d_char_move(pj, vx, vz)`** no mueve nada por sí sola: guarda la velocidad deseada para el
  siguiente `g3d_char_update`.
- **`g3d_char_update(pj, dt)`** hace la simulación. Llámala una vez por frame.
- **`g3d_char_grounded(pj)`** vale 1 si está en el suelo; úsala para permitir el salto.
- El personaje camina por el **terreno** de la escena cargada y por las **cajas** de
  `g3d_collider_add_box`. Para un nivel con mallas, usa [g3d_collider_add_model](#api/g3d_collider_add_model).

## Ajustes útiles

| Quieres... | Usa |
|---|---|
| Subir escalones más altos, o pendientes más fuertes | `g3d_char_set_tuning(pj, escalon, pendiente_grados)` |
| Que empuje cajas y barriles | `g3d_char_set_push(pj, fuerza)` |
| Teletransportarlo | `g3d_char_set_position(pj, x, y, z)` |
| Nadar | `g3d_char_set_water(pj, 1, altura_del_agua)` cada frame |
