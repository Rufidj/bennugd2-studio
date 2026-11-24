# Un vehículo

El vehículo (`g3d_vehicle_*`) es un coche **arcade**: dirección tipo bicicleta y las cuatro ruedas
siguiendo el terreno. No es un cuerpo rígido de Jolt: es un controlador propio, rápido y fácil de
conducir.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

GLOBAL
    int escena;
    int camara;

PROCESS camara_sigue(int coche)
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    LOOP
        target_x = g3d_vehicle_x(coche);
        target_y = g3d_vehicle_y(coche);
        target_z = g3d_vehicle_z(coche);
        x = target_x; y = target_y + 8.0; z = target_z - 14.0;
        FRAME;
    END
END

PROCESS coche()
PRIVATE
    int v;                 // id del vehículo
    float acel; float giro; float freno;
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, 0.0, 0.0, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());

    // x, y, z y rumbo (en milésimas de grado)
    v = g3d_vehicle_create(0.0, 1.0, 0.0, 0);

    // motor, velocidad máxima, giro máximo (grados), freno y resistencia
    g3d_vehicle_set_tuning(v, 30.0, 60.0, 32.0, 40.0, 0.8);

    camara_sigue(v);

    LOOP
        acel = 0.0; giro = 0.0; freno = 0.0;
        IF (key(_UP))    acel =  1.0; END
        IF (key(_DOWN))  acel = -1.0; END
        // dirección positiva = el rumbo crece hacia +X; con la cámara detrás eso es girar a la IZQUIERDA
        IF (key(_LEFT))  giro =  1.0; END
        IF (key(_RIGHT)) giro = -1.0; END
        IF (key(_SPACE)) freno = 1.0; END

        g3d_vehicle_update(v, 0.0166, acel, giro, freno);

        x = g3d_vehicle_x(v); y = g3d_vehicle_y(v); z = g3d_vehicle_z(v);
        angle_y = g3d_vehicle_yaw(v);       // ya viene en milésimas de grado
        angle_x = g3d_vehicle_pitch(v);
        angle_z = g3d_vehicle_roll(v);
        FRAME;
    END
END

PROCESS main()
BEGIN
    escena = g3d_scene_create("carrera");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);
    coche();
    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Piezas clave

- **`g3d_vehicle_update(v, dt, acelerador, direccion, freno)`**: acelerador de -1 (marcha atrás) a 1,
  dirección de -1 a 1 (**positiva = hacia +X**, a la izquierda de quien mira hacia +Z) y freno de 0 a 1. Los valores fuera de rango se recortan.
- **Cuanta más velocidad, menos giro:** la dirección pierde hasta un 40 % de autoridad a máxima velocidad.
- **El vehículo solo acelera con las ruedas en el suelo.**
- **Dimensiones:** `g3d_vehicle_set_geometry(v, batalla, via, altura)`. Valores por defecto: batalla 4.0,
  vía 2.4, altura 1.0.
- **Velocidad:** `g3d_vehicle_speed(v)`, con signo (negativa = marcha atrás). Útil para un velocímetro.
- Para varios vehículos, cada uno tiene su id (hay un máximo de 8).
