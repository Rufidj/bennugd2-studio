# Cuerpos rígidos (física)

Un **cuerpo rígido** es un objeto que obedece a la física: cae, rebota, se apila y se deja empujar.
Se crea con `g3d_rigidbody_create*`, se avanza con `g3d_rigidbody_step` y se **lee su posición** para
mover el modelo que lo representa.

> En PC la física la hace [Jolt](https://github.com/jrouwe/JoltPhysics). En Vita y PS5 se usa un motor
> propio más simple; algunas funciones (mallas exactas, envolventes convexas) solo existen con Jolt.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

GLOBAL
    int escena;
    int camara;

PROCESS sol()
BEGIN
    ctype = C_3D; csubtype = C3D_LIGHT;
    entity = g3d_light_create(G3D_LIGHT_DIRECTIONAL);
    target_x = 0.4; target_y = -0.8; target_z = 0.3;
    intensity = 0.6;                  // por defecto es 1.0 y los objetos blancos salen quemados
    LOOP FRAME; END
END

PROCESS camara_fija()
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    x = 0.0; y = 6.0; z = -14.0;
    target_x = 0.0; target_y = 2.0; target_z = 0.0;
    LOOP FRAME; END
END

// Una caja que cae. El proceso copia la posición y el giro del cuerpo a su modelo.
PROCESS caja(float px, float py, float pz)
PRIVATE int cuerpo;
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, px, py, pz);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());

    // posición, MEDIAS dimensiones (la mitad de cada lado) y masa
    cuerpo = g3d_rigidbody_create(px, py, pz, 0.5, 0.5, 0.5, 10.0);
    g3d_rigidbody_set_bounce(cuerpo, 0.3, 0.6);   // rebote y rozamiento

    LOOP
        x = g3d_rigidbody_render_x(cuerpo);
        y = g3d_rigidbody_render_y(cuerpo);
        z = g3d_rigidbody_render_z(cuerpo);
        // los ángulos ya salen en milésimas de grado, listos para angle_*
        angle_x = g3d_rigidbody_angle_x(cuerpo);
        angle_y = g3d_rigidbody_angle_y(cuerpo);
        angle_z = g3d_rigidbody_angle_z(cuerpo);
        FRAME;
    END
END

PROCESS main()
PRIVATE int i;
BEGIN
    escena = g3d_scene_create("fisica");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);

    // Un suelo: masa 0 = cuerpo FIJO (choca con todo, no lo mueve nada).
    g3d_rigidbody_create(0.0, -0.5, 0.0, 50.0, 0.5, 50.0, 0.0);

    sol();
    camara_fija();

    // Una torre de cajas
    FOR (i = 0; i < 6; i++)
        caja(0.0, 1.0 + i * 1.2, 0.0);
    END

    LOOP
        g3d_rigidbody_step(0.0166);                   // una vez por frame
        IF (key(_SPACE))                              // empujón a la torre
            g3d_rigidbody_apply_impulse(0, 0.0, 60.0, 20.0);
        END
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Piezas clave

- **Medias dimensiones.** `g3d_rigidbody_create(x, y, z, hx, hy, hz, masa)` toma la *mitad* de cada
  lado: una caja de 1×1×1 son `0.5, 0.5, 0.5`.
- **Masa 0 = fijo.** Sirve para suelos, paredes y decorado sólido.
- **`g3d_rigidbody_render_*` en vez de `g3d_rigidbody_x/y/z`.** Los primeros dan dónde dibujar un modelo
  con el origen en los pies; los segundos son el centro del cuerpo. Si tu modelo tiene el origen en
  otro sitio, ajústalo con `g3d_rigidbody_set_model_offset`.
- **Un cuerpo dormido ignora lo que no le despierta.** Los impulsos y `set_velocity` ya lo despiertan solos.
- **Formas:** caja, esfera, cápsula y cilindro; y `g3d_rigidbody_create_convex_model` para ajustarse a
  un modelo (barriles, rocas).

## Flotar

`g3d_rigidbody_set_buoyancy(cuerpo, nivel_agua, densidad)` con densidad relativa al agua: menor que 1
flota, 1 queda a flor, mayor que 1 se hunde. El empuje se reparte por el volumen sumergido, así que un
cuerpo ancho y bajo aguanta y uno alto y estrecho vuelca.

```bennugd
g3d_rigidbody_set_buoyancy(barril, g3d_scene_water_level(), 0.6);   // flota
```

## Nivel con colisión exacta

Para dar colisión a un nivel importado con una sola llamada:

```bennugd
nivel = g3d_load_gltf("Assets/Models/nivel.glb");
g3d_collider_add_model(nivel, 0.0, 0.0, 0.0, 1.0);   // malla exacta, estática
```
