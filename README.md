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

Requiere el repo de **libmod_3d** como hermano (`../libmod_3d`), SDL2, SDL2_image y OpenGL.

```sh
cmake -S . -B build
cmake --build build -j4
./bin/editor3d
```

El editor compila el *core* de libmod_3d directamente (todos los `libmod_3d_*.c`
excepto `libmod_3d.c`, que es la capa de enlace con BennuGD2).

## Dependencias vendorizadas

En `imgui/`: Dear ImGui (docking) + backends SDL2/OpenGL3, ImGuizmo,
ImGuiColorTextEdit y imfilebrowser. En `fonts/`: Font Awesome.
