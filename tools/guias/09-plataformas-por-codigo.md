# Un juego de plataformas por código

No hay un "modo plataformas": un nivel de plataformas es **un nivel dibujado en la escena** más **código
que lo juega**. Dibuja el escenario con las herramientas de mundo (terreno, primitivas, prefabs,
modelos) y programa el personaje, las monedas y las reglas.

Este ejemplo lo monta todo por código para mostrar las piezas; en un juego real las plataformas las
pondrías en el editor y asignarías un `PROCESS` a cada objeto que deba hacer algo.

Las piezas son tres:

1. **Colisión del escenario.** Cada plataforma necesita una caja de colisión (`g3d_collider_add_box`) o
   una malla (`g3d_collider_add_model`) para que el personaje pise sobre ella.
2. **El personaje.** Un controlador `g3d_char_*` con movimiento solo en un eje (desplazamiento lateral)
   y salto.
3. **Las reglas.** Recoger cosas, caer al vacío, llegar a la meta: son comprobaciones de distancia y
   de altura en un proceso.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

CONST
    DT        = 0.0166;
    VELOCIDAD = 7.0;
    SALTO     = 12.0;
END

GLOBAL
    int escena;
    int camara;
    int monedas;        // monedas recogidas
    int pj;             // controlador del personaje

PROCESS sol()
BEGIN
    ctype = C_3D; csubtype = C3D_LIGHT;
    entity = g3d_light_create(G3D_LIGHT_DIRECTIONAL);
    target_x = 0.3; target_y = -0.8; target_z = -0.5;   // la luz viaja hacia -Z: ilumina las caras que ve la cámara (en z positiva)
    intensity = 0.6;                  // por defecto es 1.0 y los objetos blancos salen quemados
    LOOP FRAME; END
END

// Una plataforma: un cubo escalado y su caja de colisión. El cubo mide 1x1x1 y está centrado,
// así que escalarlo a (ancho, alto, fondo) da exactamente esa caja.
PROCESS plataforma(float px, float py, float pz, float ancho, float alto, float fondo)
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, px, py, pz);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());
    size_x = ancho * 100; size_y = alto * 100; size_z = fondo * 100;   // escala en %: dentro de un proceso la manda 'size'
    g3d_collider_add_box(px - ancho / 2, py - alto / 2, pz - fondo / 2,
                         px + ancho / 2, py + alto / 2, pz + fondo / 2);
    x = px; y = py; z = pz;
    LOOP FRAME; END
END

// Una moneda: gira y desaparece cuando el personaje se acerca.
PROCESS moneda(float px, float py)
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, px, py, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());
    size_x = 0.4 * 100; size_y = 0.4 * 100; size_z = 0.4 * 100;   // escala en %: dentro de un proceso la manda 'size'
    x = px; y = py; z = 0.0;
    LOOP
        angle_y += 3000;
        // distancia al personaje (en el plano del juego, X e Y)
        IF (abs(g3d_char_x(pj) - px) < 0.8 AND abs(g3d_char_y(pj) + 0.9 - py) < 1.2)
            monedas++;
            say("Monedas: " + itoa(monedas));
            g3d_entity_destroy(entity);
            BREAK;
        END
        FRAME;
    END
END

PROCESS camara_lateral()
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    LOOP
        target_x = g3d_char_x(pj); target_y = g3d_char_y(pj) + 1.0; target_z = 0.0;
        // La cámara va en z POSITIVA mirando hacia -Z: así +X queda a la derecha de la pantalla.
        x = target_x; y = target_y + 1.0; z = 14.0;
        FRAME;
    END
END

PROCESS heroe()
PRIVATE float vx;
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, 0.0, 0.0, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());
    size_x = 0.8 * 100; size_y = 1.8 * 100; size_z = 0.8 * 100;   // escala en %: dentro de un proceso la manda 'size'

    pj = g3d_char_create(0.0, 3.0, 0.0, 0.4, 1.8);

    LOOP
        vx = 0.0;
        IF (key(_RIGHT)) vx =  VELOCIDAD; END
        IF (key(_LEFT))  vx = -VELOCIDAD; END
        g3d_char_move(pj, vx, 0.0);                     // solo en X: es un juego lateral
        IF (key(_SPACE) AND g3d_char_grounded(pj)) g3d_char_jump(pj, SALTO); END
        g3d_char_update(pj, DT);

        // el cubo mide 1 de alto y está centrado: se sube media altura para apoyarlo en los pies
        x = g3d_char_x(pj); y = g3d_char_y(pj) + 0.9; z = 0.0;

        // caer al vacío: vuelve al principio
        IF (g3d_char_y(pj) < -15.0)
            g3d_char_set_position(pj, 0.0, 3.0, 0.0);
        END
        FRAME;
    END
END

PROCESS main()
BEGIN
    escena = g3d_scene_create("plataformas");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);

    sol();
    // x, y, z (centro), ancho, alto, fondo
    plataforma( 0.0, -0.5, 0.0, 10.0, 1.0, 4.0);
    plataforma(11.0,  0.5, 0.0,  4.0, 1.0, 4.0);
    plataforma(18.0,  2.0, 0.0,  4.0, 1.0, 4.0);
    plataforma(26.0,  1.0, 0.0,  8.0, 1.0, 4.0);

    moneda(11.0, 2.2);
    moneda(18.0, 3.7);
    moneda(26.0, 2.7);

    heroe();
    camara_lateral();

    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Cómo crecer desde aquí

| Quieres... | Usa |
|---|---|
| Plataformas que se mueven | un proceso que cambie `x`, `y` y llame a `g3d_collider_set_box(caja, ...)` cada frame |
| Enemigos | un proceso con su propio controlador `g3d_char_*` y la misma comprobación de distancia |
| Saltar con más control | `g3d_char_set_tuning` para escalón y pendiente; `g3d_char_set_water` para nadar |
| Un nivel hecho en el editor | `g3d_scene_load("nivel.scene")` y colisión con `g3d_collider_add_model` sobre sus modelos |
| Empujar cajas | `g3d_char_set_push` y [cuerpos rígidos](#guia/05-fisica) |
| Saber dónde empieza el jugador | `g3d_scene_has_spawn()` y `g3d_scene_spawn_x/y/z()` |
| Sprites 2D en vez de cubos | [sprites HD-2D](#guia/07-sprites) |

> `g3d_collider_add_box` devuelve el identificador de la caja; guárdalo si vas a moverla o quitarla
> (`g3d_collider_set_box`, `g3d_collider_remove_box`).
