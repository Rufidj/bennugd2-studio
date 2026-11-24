# Un juego de carreras por código

No hay un "modo carreras". En el editor solo marcas **por dónde se puede circular** (la carretera) y todo lo
demás es código: dónde salen los coches, la meta, las vueltas, los rivales, los turbos...

**La carretera no se dibuja ni cambia el suelo.** El aspecto lo pones tú (pintando el terreno con las
herramientas de terreno, o poniendo un modelo `.glb`) y la herramienta **Carretera** solo marca *por dónde se
puede circular*. Esa información se consulta con las funciones `g3d_road_*`. Son pocas y bastan para hacer una
carrera completa:

| Pregunta | Función |
|---|---|
| ¿Cuánto mide la carretera? | `g3d_road_length(carretera)` |
| ¿Dónde está el punto a *d* metros del principio? | `g3d_road_x/y/z(carretera, d)` |
| ¿Hacia dónde mira la carretera ahí? | `g3d_road_yaw(carretera, d)` |
| ¿Qué ancho tiene? ¿Qué superficie es? | `g3d_road_width(carretera, d)`, `g3d_road_surface(carretera, d)` |
| ¿Dónde estoy respecto a ella? | `g3d_road_nearest(x, z)` y luego `g3d_road_near_dist()`, `g3d_road_near_offset()`, `g3d_road_near_on()` |

`g3d_road_near_dist()` es la **distancia recorrida** del punto más cercano de la carretera, y eso es todo lo
que hace falta para saber en qué vuelta va un coche, quién va primero o hacia dónde apuntar un rival.

## La carretera

Una carretera es una lista de nodos (posición, ancho, peralte) unidos por curvas suaves. En un proyecto real
la marcas en el editor; aquí la inventamos por código para ver todas las piezas, y como no hay terreno ni modelo
que la muestre, la dibujamos con `g3d_road_show` y la hacemos suelo con `g3d_road_set_ground` (en tu juego no
harían falta).

```bennugd compila
import "libmod_gfx"; import "libmod_misc"; import "libmod_input"; import "libmod_3d";

CONST
    DT        = 0.0166;
    N_VUELTAS = 3;
END

GLOBAL
    int escena;
    int camara;
    int carretera;       // id de la carretera
    float largo;         // longitud de la carretera, en metros
    int jugador_id;      // id del vehículo del jugador

PROCESS sol()
BEGIN
    ctype = C_3D; csubtype = C3D_LIGHT;
    entity = g3d_light_create(G3D_LIGHT_DIRECTIONAL);
    target_x = 0.3; target_y = -0.8; target_z = 0.5;
    intensity = 0.6;                  // por defecto es 1.0 y los objetos blancos salen quemados
    LOOP FRAME; END
END

PROCESS camara_detras()
BEGIN
    ctype = C_3D; csubtype = C3D_CAMERA;
    entity = camara;
    LOOP
        target_x = g3d_vehicle_x(jugador_id);
        target_y = g3d_vehicle_y(jugador_id);
        target_z = g3d_vehicle_z(jugador_id);
        x = target_x; y = target_y + 12.0; z = target_z - 18.0;
        FRAME;
    END
END

// El dibujo de un coche: sigue a su vehículo.
PROCESS kart(int vid, int r, int g, int b)
BEGIN
    ctype = C_3D; csubtype = C3D_ENTITY;
    entity = g3d_entity_spawn(escena, 0, 0.0, 0.0, 0.0);
    g3d_entity_set_mesh(entity, g3d_primitive_cube());
    size_x = 2.0 * 100; size_y = 1.0 * 100; size_z = 4.0 * 100;   // escala en %: dentro de un proceso la manda 'size'
    color_r = r; color_g = g; color_b = b;
    LOOP
        x = g3d_vehicle_x(vid); y = g3d_vehicle_y(vid) + 0.5; z = g3d_vehicle_z(vid);
        angle_y = g3d_vehicle_yaw(vid);
        FRAME;
    END
END

// Cuenta las vueltas de un vehículo. Sirve para el jugador y para los rivales.
PROCESS vueltas(int vid, string nombre)
PRIVATE float d; float previo; int n;
BEGIN
    LOOP
        g3d_road_nearest(g3d_vehicle_x(vid), g3d_vehicle_z(vid));
        d = g3d_road_near_dist();
        // al cruzar la meta la distancia pasa de casi "largo" a casi 0
        IF (previo > largo * 0.75 AND d < largo * 0.25 AND n < N_VUELTAS)
            n++;
            say(nombre + ": vuelta " + itoa(n) + " de " + itoa(N_VUELTAS));
            IF (n == N_VUELTAS) say("GANA " + nombre); END
        END
        previo = d;
        FRAME;
    END
END

// El jugador conduce con el teclado.
PROCESS jugador()
PRIVATE float acel; float giro; float freno;
BEGIN
    jugador_id = g3d_vehicle_create(g3d_road_x(carretera, 4.0), 1.0, g3d_road_z(carretera, 4.0),
                                    g3d_road_yaw(carretera, 4.0));
    g3d_vehicle_set_tuning(jugador_id, 30.0, 60.0, 32.0, 40.0, 0.8);
    kart(jugador_id, 230, 60, 60);
    vueltas(jugador_id, "Jugador");
    LOOP
        acel = 0.0; giro = 0.0; freno = 0.0;
        IF (key(_UP))    acel =  1.0; END
        IF (key(_DOWN))  acel = -1.0; END
        IF (key(_LEFT))  giro =  1.0; END       // dirección positiva = hacia +X = izquierda de la pantalla
        IF (key(_RIGHT)) giro = -1.0; END
        IF (key(_SPACE)) freno = 1.0; END
        g3d_vehicle_update(jugador_id, DT, acel, giro, freno);
        FRAME;
    END
END

// Un rival: mira un punto de la carretera 18 m por delante de él y gira hacia allí.
PROCESS rival(float dist_inicio, int r, int g, int b)
PRIVATE
    int v; float d;
    float fx; float fz;              // hacia dónde mira el coche
    float tx; float tz; float tl;    // hacia dónde está el punto objetivo
    float giro; float acel;
BEGIN
    v = g3d_vehicle_create(g3d_road_x(carretera, dist_inicio), 1.0, g3d_road_z(carretera, dist_inicio),
                           g3d_road_yaw(carretera, dist_inicio));
    g3d_vehicle_set_tuning(v, 26.0, 50.0, 32.0, 40.0, 0.8);
    kart(v, r, g, b);
    vueltas(v, "Rival");
    LOOP
        g3d_road_nearest(g3d_vehicle_x(v), g3d_vehicle_z(v));
        d = g3d_road_near_dist() + 18.0;
        tx = g3d_road_x(carretera, d) - g3d_vehicle_x(v);
        tz = g3d_road_z(carretera, d) - g3d_vehicle_z(v);
        tl = sqrt(tx * tx + tz * tz); IF (tl < 0.001) tl = 1.0; END
        fx = sin(g3d_vehicle_yaw(v)); fz = cos(g3d_vehicle_yaw(v));
        // producto cruzado: positivo si el objetivo queda del lado en que crece el rumbo
        giro = (fz * tx - fx * tz) / tl * 3.0;
        IF (giro > 1.0) giro = 1.0; END
        IF (giro < -1.0) giro = -1.0; END
        acel = 1.0 - abs(giro) * 0.5;       // frena un poco en las curvas
        g3d_vehicle_update(v, DT, acel, giro, 0.0);
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

    // La carretera: un circuito cerrado. Nodo: x, z, y, ancho, peralte, sigue el terreno, dy, superficie.
    // (con 'sigue el terreno' a 0 la altura es la 'y' del nodo; con 1 la carretera se pega al relieve + dy)
    carretera = g3d_road_create(1);
    g3d_road_add_node(carretera, -40.0, -30.0, 0.0, 14.0, 0.0, 0, 0.0, 0);
    g3d_road_add_node(carretera,  50.0, -35.0, 0.0, 14.0, 0.0, 0, 0.0, 0);
    g3d_road_add_node(carretera,  70.0,  20.0, 0.0, 14.0, 0.0, 0, 0.0, 0);
    g3d_road_add_node(carretera,  10.0,  60.0, 8.0, 14.0, 0.0, 0, 0.0, 1);   // sube 8 m, y su tramo es de tierra
    g3d_road_add_node(carretera, -50.0,  30.0, 0.0, 14.0, 0.0, 0, 0.0, 0);
    g3d_road_build(carretera);
    g3d_road_show(carretera, escena);        // SOLO para este ejemplo: en tu juego la carretera ya está pintada en tu mapa
    g3d_road_set_ground(carretera, 1);       // y aquí tiene que ser suelo (normalmente lo es tu terreno o tu modelo)
    largo = g3d_road_length(carretera);

    sol();
    jugador();
    rival(20.0, 60, 90, 240);
    camara_detras();

    LOOP
        IF (key(_ESC)) exit(); END
        FRAME;
    END
END
```

## Una carretera marcada en el editor

En el editor no se dibuja la carretera: **la pintas tú** en el terreno con las herramientas de terreno (la textura
que quieras) o pones un modelo `.glb` de pista como un objeto más de la escena. Después, en el modo **Terreno**,
eliges la herramienta **Carretera** y haces clic a lo largo de ella para decir *por dónde se puede circular*.

- Puedes hacer clic sobre el terreno o sobre el modelo; se toma lo que ves más cerca. Sobre un modelo, el punto
  se queda a la altura de su superficie.
- Ajustas el **ancho** y el **peralte** de cada punto y la **superficie** de cada tramo.
- En el visor se ve una guía (la línea central y los bordes del ancho). **No existe en el juego.**
- La carretera **se guarda dentro de la escena** (junto al terreno y los objetos), así que viaja con ella.

Al generar el juego, esa carretera ya está creada y calculada cuando empieza tu código. Su id es 0 (la siguiente,
1...), así que basta con usarla:

```bennugd
carretera = 0;                         // la primera carretera de la escena
largo = g3d_road_length(carretera);
jugador_id = g3d_vehicle_create(g3d_road_x(carretera, 4.0), 1.0, g3d_road_z(carretera, 4.0),
                                g3d_road_yaw(carretera, 4.0));
```

Todo lo demás (dónde salen los coches, la meta, las vueltas, los rivales) es código tuyo, como en el ejemplo.

## Cómo funciona

- **Dónde salen los coches:** en `g3d_road_x/z/yaw(carretera, 4.0)`, a 4 m del principio y mirando como la carretera.
- **Vueltas:** `vueltas()` mira a cada vehículo con `g3d_road_nearest` y detecta cuando su distancia recorrida
  salta de casi `largo` a casi 0. Un solo proceso sirve para cualquier número de coches.
- **Rival:** cada frame busca su sitio en la carretera, mira el punto 18 m más adelante y gira hacia él.
  Con `g3d_road_near_offset()` (separación lateral, positiva a la izquierda) puedes hacerle seguir una línea
  distinta del centro.
- **Superficie:** `g3d_road_near_surface()` te dice si el coche pisa asfalto, tierra, hielo o barro (las
  etiquetas las pones al dibujar). Qué le pasa al coche en cada una lo decides tú, por ejemplo con
  `g3d_vehicle_set_tuning`.
- **Fuera de la carretera:** `g3d_road_near_on()` vale 0 si el coche se sale del ancho.

## Cómo crecer desde aquí

| Quieres... | Usa |
|---|---|
| Posición en carrera (1º, 2º...) | compara la vuelta y `g3d_road_near_dist()` de cada coche |
| Volver a la pista tras salirse | `g3d_road_nearest` y colocar el coche en `g3d_road_x/z(carretera, g3d_road_near_dist())` |
| Un circuito dibujado en el editor | herramienta Carretera del modo Terreno (ver arriba) |
| Turbos, rampas, obstáculos | procesos con una distancia a lo largo de la carretera: `d` y `g3d_road_near_offset()` |
| Velocímetro | `g3d_vehicle_speed(id)` |
| Cámara con colisión | `g3d_camera_safe_distance` |

> **Suelo y aspecto:** la carretera es solo información: ni se dibuja ni cambia el suelo. El suelo es el de tu
> terreno o tu modelo, y lo que ves es lo que pintaste. Para una carretera inventada por código, `g3d_road_show`
> la dibuja y `g3d_road_set_ground` la hace suelo (como en este ejemplo); los cuerpos rígidos de Jolt no la notan.
