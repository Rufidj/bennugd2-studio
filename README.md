# Editor de BennuGD2

Editor visual estilo Unity/Unreal **para BennuGD2**. El objetivo es un editor único
y fácil que cubra **todo** BennuGD2 —escenas, objetos, scripts, recursos— y genere
juegos listos para ejecutar. Empieza por el **3D** (sobre el módulo
**[libmod_3d](https://github.com/Rufidj/libmod_3d)**) y está pensado para crecer hacia
el resto del motor (2D, UI, sonido…).

Es una aplicación C++ independiente con **Dear ImGui** que enlaza el *core* del motor y
renderiza contra el mismo contexto OpenGL: lo que ves en el editor es lo que corre en el juego.

## Qué hace hoy (3D)

- **Viewport 3D** acoplable con navegación estilo Unreal (botón derecho + WASD, rueda = zoom).
- **Editor de terreno**: esculpir (subir/bajar/suavizar/nivelar), pintar texturas, agujeros.
- **Colocación de objetos** (`.glb`, `.gltf`, `.fbx`) con **gizmos** reales (mover/rotar/escalar).
- **Agua** (mar/lago): nivel, oleaje, color y textura.
- **Física (Jolt)** por objeto: caja/esfera/cápsula/cilindro, masa, rebote, fricción, flotación.
- **Muros invisibles** y **zonas de barrera pintables por capas** (qué objeto pasa y cuál no).
- **Jugador** controlable (char controller, WASD/salto/nado) con **animaciones** automáticas.
- **Enganchar objetos a huesos** (arma en la mano).
- **Visor de animaciones** (doble clic en un modelo): previsualiza cada clip y su índice.
- **Editor de scripts** por objeto con resaltado de sintaxis.
- **Sistema de proyectos** `.bgd2` (carpeta con Assets/Scenes/Scripts) y **generación del juego**
  (`main.prg`) que compila y ejecuta con BennuGD2.

## Hoja de ruta

- Editor 2D (sprites, mapas, colisiones) sobre `libmod_gfx`.
- Editor de UI, sonido y recursos.
- Un único proyecto que englobe 2D + 3D en el mismo juego.

## Compilar

**Requisitos:** SDL2, SDL2_image, OpenGL, CMake ≥ 3.10 y un **BennuGD2 ya compilado**,
porque el editor enlaza contra sus librerías.

> ### ⚠️ Necesita ESTE `libmod_3d`
> El editor usa una API de edición (zonas pintadas, carga de relieve, colisionador de
> terreno, consultas de modelo…) que **no está en el `libmod_3d` original de BennuGD2**.
> Hay que usar **[Rufidj/libmod_3d](https://github.com/Rufidj/libmod_3d)**:
>
> ```sh
> cd BennuGD2/modules
> git clone https://github.com/Rufidj/libmod_3d.git   # sustituye al modulo original
> # recompila BennuGD2 para regenerar libmod_3d
> ```
>
> Si no, el editor compilaría pero moriría al arrancar con
> `undefined symbol: g3d_zone_load`. Para evitarlo, **CMake lo comprueba al configurar**
> y te dice exactamente qué funciones faltan y qué hacer.

La disposición esperada es la normal del repo de BennuGD2:

```
BennuGD2/
├── core/                     cabeceras y runtime (bgdrtm)
├── build/<plataforma>/bin/   binarios ya compilados (libmod_3d, libbgdrtm, bgdc…)
└── modules/
    ├── libmod_3d/            el módulo 3D
    └── editor3d/             ESTE proyecto
```

Con esa estructura, **no hay que configurar nada**:

```sh
cmake -S . -B build
cmake --build build -j4
./bin/editor3d
```

CMake localiza BennuGD2 solo (dos niveles por encima) y detecta la carpeta de
binarios de tu plataforma. Si lo tienes en otro sitio, indícalo:

```sh
cmake -S . -B build -DBGD_ROOT=/ruta/a/BennuGD2
# o, si hace falta, por partes:
cmake -S . -B build -DBGD_BIN=/ruta/a/BennuGD2/build/linux-gnu/bin \
                    -DLIBMOD3D=/ruta/a/libmod_3d
```

Si falta algo, CMake avisa con un mensaje claro indicando dónde ha buscado.

> **Nota:** el editor **enlaza** `libmod_3d` en vez de compilar su *core*, y añade el
> directorio de binarios al `RPATH` (con `--disable-new-dtags`). Esto es
> imprescindible: el intérprete de BennuGD2 carga los módulos con
> `dlopen("libmod_xxx.so")` sin ruta, así ambos comparten **una sola instancia** del
> motor y funciona sin depender de `LD_LIBRARY_PATH` (por ejemplo, al abrir el
> editor con doble clic desde el escritorio).

Por ahora el build está probado en **Linux**.

## Dependencias vendorizadas

En `imgui/`: Dear ImGui (docking) + backends SDL2/OpenGL3, ImGuizmo,
ImGuiColorTextEdit y imfilebrowser. En `fonts/`: Font Awesome.
