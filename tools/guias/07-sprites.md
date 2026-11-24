# Sprites 2D en el mundo 3D (HD-2D)

Un `C3D_SPRITE` es un gráfico 2D normal de BennuGD2 plantado en la escena 3D. Siempre mira a la cámara, tapa
y es tapado por la geometría 3D, y se anima como en 2D: **cambiando `graph`**. El estilo de los juegos
"HD-2D": personajes dibujados sobre un mundo en 3D.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

GLOBAL
    int escena;
    int camara;

PROCESS camara_fija()
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    x = 0.0; y = 3.0; z = -8.0;
    target_x = 0.0; target_y = 1.0; target_z = 0.0;
    LOOP FRAME; END
END

// Un sprite de ejemplo dibujado por código (en un juego real, cargas un PNG o un FPG).
PROCESS heroe()
BEGIN
    ctype = C_3D; csubtype = C3D_SPRITE;

    graph = map_new(16, 32);                       // un gráfico de 16x32 píxeles
    map_clear(0, graph, rgb(220, 60, 60));         // lo rellena de rojo

    entity = g3d_sprite_create(escena, 0.0, 0.0, 0.0);
    g3d_sprite_set_pixels_per_unit(entity, 16.0);  // 16 píxeles = 1 unidad: mide 2 de alto
    g3d_sprite_set_anchor(entity, 0.5, 1.0);       // los pies, en x,y,z
    g3d_sprite_set_billboard(entity, G3D_SPRITE_UPRIGHT);

    LOOP
        x += 0.02;                                 // se mueve como cualquier proceso
        IF (x > 3.0) x = -3.0; END
        FRAME;
    END
END

PROCESS main()
BEGIN
    escena = g3d_scene_create("hd2d");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);
    camara_fija();
    heroe();
    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Piezas clave

- **El gráfico lo manda `graph` (y `file`)**, igual que en 2D. Para animar, cambia `graph` entre los
  fotogramas de un FPG, o usa una hoja de sprites (ver más abajo).
- **Tamaño:** `g3d_sprite_set_pixels_per_unit` (cómodo para pixel art) o `g3d_sprite_set_height`.
- **Ancla:** `(0.5, 1.0)` = centro-abajo, los pies del personaje sobre el suelo. Si el gráfico trae el
  punto de control 0, se usa ese.
- **Orientación:** `G3D_SPRITE_UPRIGHT` gira solo en horizontal y sigue de pie (personajes);
  `G3D_SPRITE_FACING` mira siempre de frente (objetos, efectos).
- **Pixel art nítido:** por defecto el filtrado es *nearest* y la posición se redondea a píxeles enteros
  (`g3d_sprite_set_smooth` y `g3d_sprite_set_snap` lo cambian).
- **Sombra y luz:** `g3d_sprite_set_shadow(sprite, 1)` proyecta sombra; por defecto la luz de la escena le
  afecta (`g3d_sprite_set_lit`).

## Hojas de sprites

Un solo gráfico partido en celdas. Con las variables de la estructura `sprite` del proceso:

```bennugd
sprite.cols = 4; sprite.rows = 2;     // la hoja tiene 4 columnas y 2 filas
LOOP
    contador++;
    sprite.frame = (contador / 6) % 8; // fotograma que se dibuja
    FRAME;
END
```

## Varias direcciones

`g3d_sprite_dir(sprite, angulo, ndirs)` dice qué postura dibujar según desde dónde se mira (0 = de
frente a la cámara). El ángulo va en milésimas de grado, como la variable `angle`.
