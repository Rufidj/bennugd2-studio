# Modelos y animación

Cargar un modelo da un **identificador de modelo**. Para verlo hay que **colocarlo** en una escena
(`g3d_model_spawn`), lo que crea una entidad raíz de la que cuelgan todas sus mallas.

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

GLOBAL
    int escena;
    int camara;

PROCESS camara_fija()
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    x = 0.0; y = 2.0; z = -5.0;
    target_x = 0.0; target_y = 1.0; target_z = 0.0;
    LOOP FRAME; END
END

PROCESS personaje()
PRIVATE
    int modelo;
    int cuenta;       // número de animaciones
    float t;          // tiempo de la animación
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;

    modelo = g3d_load_gltf("Assets/Models/personaje.glb");
    IF (modelo < 0) exit("No se pudo cargar el modelo", 1); END

    // Lo coloca en la escena con 1.8 de alto y sin giro.
    entity = g3d_model_spawn(escena, modelo, 0.0, 0.0, 0.0, 1.8, 0.0);

    g3d_model_lock_root(modelo, 1);         // la animación no desplaza al personaje
    cuenta = g3d_model_anim_count(modelo);

    LOOP
        t += 0.0166;
        IF (cuenta > 0)
            g3d_model_animate(modelo, 0, t, 1);   // animación 0, con bucle
        END
        angle_y += 500;
        FRAME;
    END
END

PROCESS main()
BEGIN
    escena = g3d_scene_create("modelos");
    g3d_scene_set_active(escena);
    camara = g3d_camera_create();
    g3d_camera_set_active(camara);

    g3d_sky_set_gradient(0.25, 0.45, 0.85, 0.75, 0.85, 0.95);   // sin cielo la pantalla sale NEGRA
    g3d_sky_enable(1);
    camara_fija();
    personaje();
    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

> Cambia `Assets/Models/personaje.glb` por un modelo tuyo. También se leen FBX (`g3d_load_fbx`), OBJ
> (`g3d_load_obj`) y MD3 (`g3d_load_md3`).

## Mezclar animaciones

Para pasar suavemente de quieto a andar, mezcla dos animaciones con un peso de 0 a 1:

```bennugd
peso = velocidad / 6.0;                  // 0 = quieto, 1 = corriendo
g3d_model_animate_blend(modelo, ANIM_QUIETO, t, ANIM_CORRER, t, peso, 1);
```

## Acoplar un objeto a un hueso

Busca el hueso una vez y lee su posición cada frame, **después** de animar:

```bennugd
mano = g3d_model_node_find(modelo, "hand_r");       // ignora mayúsculas, vale un trozo del nombre
...
g3d_model_animate(modelo, 0, t, 1);
arma.x = g3d_model_node_x(modelo, mano);
arma.y = g3d_model_node_y(modelo, mano);
arma.z = g3d_model_node_z(modelo, mano);
```

## Quitar un modelo

`g3d_model_despawn(raiz)` destruye la raíz, todas sus mallas y sus materiales privados. Úsalo siempre que
crees y quites modelos continuamente (proyectiles, enemigos) para no agotar los huecos del motor.
