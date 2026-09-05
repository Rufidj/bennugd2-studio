# Editor de BennuGD2

Editor visual estilo Unity/Unreal **para BennuGD2**. El objetivo es un editor único
y fácil que cubra **todo** BennuGD2 —escenas, objetos, scripts, recursos— y genere
juegos listos para ejecutar. Empieza por el **3D** (sobre el módulo
**[libmod_3d](https://github.com/Rufidj/libmod_3d)**) y está pensado para crecer hacia
el resto del motor (2D, UI, sonido…).

Es una aplicación C++ independiente con **Dear ImGui** que enlaza el *core* del motor y
renderiza contra el mismo contexto OpenGL: lo que ves en el editor es lo que corre en el juego.

## Los ficheros del proyecto

```
MiJuego/
  MiJuego.bgd2      el proyecto: escena inicial, variables, guardado
  Scenes/           las escenas (.scene) con su relieve, pintado y zonas
  Scripts/          tu código (.prg) y el de cada objeto
  Assets/
    Models/         .glb .gltf .fbx .obj .md3
    Textures/       imágenes de terreno y materiales
    Sprites/        hojas de sprites (y su .sheet con los fotogramas)
    Fonts/          .fnt .fnx
    Music/          la música
    Sounds/         los efectos
  menus.def  dialogos.def
```

El editor **busca en Assets y en todas sus subcarpetas**, así que da igual cómo lo
tengas: los proyectos viejos con todo suelto siguen funcionando. Y si quieres ordenarlos,
**Proyecto → Ordenar los Assets** reparte lo suelto por esas carpetas. No rompe nada:
las escenas guardan el *nombre* del fichero y el editor lo encuentra esté donde esté.

## Cómo está repartido

El editor no enseña todo a la vez. Arriba hay **dos filas y cada una es una cosa**: los
menús (órdenes: Proyecto, Archivo, Ventana, Editar, Generar, Juego) y, debajo, el **modo
de trabajo** —*Escena, Terreno, Personajes, Interfaz, Código*— que cambia las herramientas
del **rail** de la izquierda y los paneles que salen. Esculpiendo un monte no te estorban
los del HUD.

- **Rail de herramientas**: en vertical, agrupadas por para qué sirven (ESCULPIR,
  PINTAR Y SEMBRAR, AGUA...) y con su nombre al lado.
- **Paneles acoplados** por defecto, pero **cualquiera se puede arrancar** y dejar suelto,
  incluso fuera de la ventana del editor (otro monitor, por ejemplo).
- **Ventana → Restablecer la disposición** deja el modo como venía de fábrica.
- La tipografía es **Noto Sans** (licencia OFL, va en `fonts/`), no la de mapa de bits
  que trae ImGui de serie: era lo que le daba cara de ventana de depuración.

## Qué hace hoy (3D)

- **Viewport 3D** acoplable con navegación estilo Unreal (botón derecho + WASD, rueda = zoom).
- **Editor de terreno**: esculpir (subir/bajar/suavizar/nivelar), pintar texturas, agujeros.
- **Colocación de objetos** (`.glb`, `.gltf`, `.fbx`) con **gizmos** reales (mover/rotar/escalar).
- **Agua** (mar/lago): nivel, oleaje, color y textura.
- **Física (Jolt)** por objeto: caja/esfera/cápsula/cilindro, masa, rebote, fricción, flotación.
  Elige si el cuerpo es **fijo** (corta el paso: rocas, muros, barcos) o **se puede
  empujar** — un cuerpo ligero el personaje lo aparta, y eso parece "no choca".
  La colisión **se dibuja en la escena** con el objeto elegido, nace del tamaño del
  modelo y tiene botón **Ajustar al modelo**: un collider más pequeño que el dibujo
  es la otra razón de "lo atraviesa".
- **Muros invisibles** y **zonas de barrera pintables por capas** (qué objeto pasa y cuál no).
- **Jugador** controlable (char controller, WASD/salto/nado) con **animaciones** automáticas,
  y **cámara girable** en tercera persona (ratón, teclas Q/E o stick derecho): los
  controles giran con ella, así que la D siempre lleva a la derecha de la pantalla.
- **Enganchar objetos a huesos** (arma en la mano).
- **Visor de animaciones** (doble clic en un modelo): previsualiza cada clip y su índice.
- **Editor de código** completo: varios ficheros en pestañas, árbol de `Scripts/`,
  esquema de `PROCESS`/`FUNCTION`, buscar y reemplazar, ir a la línea,
  autocompletado y compilar marcando la línea del error.
- **Personajes 2D en la Jerarquía y el Inspector**: se eligen y se mueven como un
  objeto más (clic en la escena, lista, gizmo).
- **Reglas** en cualquier objeto o personaje: *si pasa esto, haz esto* — tocar, entrar
  en una zona, una variable, cada N segundos… y suma puntos, quita vida, llama a tu
  `.prg`, enseña un texto o quita la cosa de la escena.
- **Variables del juego** (puntos, vida, llaves) como `GLOBAL`: las cambian las reglas,
  las pinta el HUD 2D y las ve tu código.
- **Música y sonido**: música por escena (con entrada suave), ambiente por zona pintada,
  sonido propio de un objeto (más fuerte de cerca) y sonidos como acción de una regla.
- **Varias escenas** por proyecto y **viajar de una a otra** en el juego (una regla lo pide:
  tocar una puerta, entrar en una zona, cumplirse algo).
- **Menús** (principal, pausa, opciones): eliges la fuente, escribes las opciones y dices
  qué hace cada una; se manejan con teclado, mando y ratón.
- **Sistema de proyectos** `.bgd2` (carpeta con Assets/Scenes/Scripts) y **generación del juego**
  (`main.prg`) que compila y ejecuta con BennuGD2.

## Música y sonido

Los ficheros van en **`Assets/Music`** y **`Assets/Sounds`** (las crea el editor).
También se leen los que ya tengas sueltos en `Assets/`, así que no hay que mover
nada. Música: `ogg`, `mp3`, `mod`, `xm`, `it`, `s3m`, `mid`, `flac`, `wav`.
Sonidos: `wav`, `ogg`, `flac`.

- **Música de la escena** (panel *Entorno → Sonido*): fichero, volumen, bucle y
  segundos de entrada. Una por escena, porque SDL_mixer sólo toca una a la vez.
- **Ambiente por zona**: un sonido en bucle mientras el jugador está dentro de una
  zona pintada — lluvia, bosque, cueva. Se abre y se cierra su canal al entrar y salir.
- **Sonido propio de un objeto** (Inspector → *Su sonido*): en bucle y **más fuerte
  cuanto más cerca estás**, con su radio. Una cascada, una hoguera, una máquina.
- **Sonar un sonido** como acción de cualquier regla: el golpe, la moneda, el daño.

Cada fichero se carga una sola vez en su `GLOBAL` (`snd_coin`, `musica`), aunque lo
usen diez reglas, y `main.prg` gana `import "libmod_sound"` si le faltaba.

> **El nombre del fichero tiene que ser ASCII.** Medido con `bgdc`: una ruta con un
> emoji, una tilde o una eñe no se abre en el juego (`music_load` devuelve 0), aunque
> en el editor se oiga. Espacios, guiones y corchetes sí valen. El editor lo marca en
> rojo y ofrece **renombrar** el fichero, arreglando de paso todas las referencias de
> la escena.

## Menús

**Escena → Menús del juego**. Un menú es una lista de opciones y lo que hace cada una,
con las mismas acciones que una regla: empezar la partida (*ir a otra escena*), llamar a
tu `PROCESS`, cambiar una variable, sonar algo, abrir otro menú, cerrarse o salir del
juego.

- **Cuándo sale**: al arrancar el juego, al pulsar una tecla o botón (lleva su vigilante
  y no se abre dos veces), o sólo cuando lo llame una regla.
- **Cómo se ve**: fuente `.fnt` de `Assets` (o la del sistema), gráfico de fondo,
  posición, separación y los dos colores — el normal y el de la opción elegida.
- **Cómo se maneja**: teclado, mando y ratón, los tres a la vez si quieres, con sonido
  al moverse y al elegir.
- **Opciones que opcionan**: una opción puede ser *un ajuste* en vez de un botón — se
  cambia con izquierda/derecha y enseña su valor. De fábrica: volumen de la música,
  volumen de los sonidos y pantalla completa. Y **cualquier variable del juego**, que es
  lo que lo hace valer para el juego que sea: dificultad, sensibilidad, vidas, idioma…
  Se guardan solos en `opciones.cfg` y se cargan al arrancar.
- **Congela el juego** mientras está abierto (`signal(ALL_PROCESS, S_FREEZE)`): eso es el
  menú de pausa.

Sale como un `PROCESS` de BennuGD2 normal: los textos se crean una vez con `write()` y
mover la selección es cambiar dos colores con `write_set_rgba`, no rehacer la pantalla.

## Guardar partida

**Ventana → Guardar partida**. Una partida guardada es **lo que tú digas que es**: el
editor no sabe si haces un RPG, un plataformas o un juego de coches, así que no decide
por ti qué es "el progreso". Marcas lo que tiene que sobrevivir:

- **Las variables del juego**, una por una (los puntos sí, un contador temporal no).
- **En qué escena** estabas.
- **Dónde estaba el jugador** y hacia dónde miraba.
- **Las reglas ya cumplidas**, para que la puerta que abriste siga abierta.

Eliges también **cuántas ranuras** y el **nombre del fichero** (`partida1.sav`,
`partida2.sav`…). Todo se vuelca de una vez con el `save()` de BennuGD2.

Y se guarda **desde donde quieras**, porque son acciones: una regla con *"Guardar la
partida"* (un punto de guardado que se toca, al pasar de escena, cada N segundos para
autoguardar) o un menú con opciones de tipo **ranura**, que además enseñan si están
vacías. El aspecto del menú de guardado es el del menú: tu fuente, tu fondo, tus colores.


## Diálogos

**Escena → Diálogos** (o el rail, en el modo Interfaz). Un diálogo es lo que dice
alguien: **páginas** con su texto, quién habla y su retrato. Si una página lleva
**respuestas**, es una pregunta: cada respuesta puede llevar a otra página, cambiar una
variable, sonar algo, abrir un menú, ir a otra escena o llamar a tu `PROCESS`.

El **retrato puede venir en una hoja con varias caras** (seria, riendo, enfadada): si la
imagen tiene fotogramas detectados —ábrela en Sprites 3D y dale a *Detectar*—, en cada
página eliges **qué cara sale**, con vista previa en el editor. En el juego se recorta esa
cara a su propio gráfico (`map_new` + `map_block_copy`), respetando la transparencia.

El **bocadillo es un gráfico tuyo** —un PNG suelto o un gráfico de un FPG— estirado al
tamaño que le des con `size_x`/`size_y`. Encima, el texto con tu fuente `.fnt` (o la del
sistema), con el nombre de quien habla en su propio color y las respuestas resaltadas.

- El texto **se escribe solo**, letra a letra, a las letras por segundo que digas
  (pulsando, sale entero de golpe). 0 = sin efecto.
- Las líneas **se parten solas** para caber en la caja: el editor las mide con la misma
  fuente que usará el juego, porque `write()` no parte por sí mismo.
- **Congela el juego** mientras se habla, si quieres.
- Para que salga, **desde donde quieras**: una regla con la acción *"Sacar un diálogo"*
  (al tocar algo, al pulsar cerca, al entrar en una zona), la **interacción de un
  personaje** (acércate y pulsa: eliges directamente el diálogo que suelta), una **acción
  de tecla** suya, una **opción de menú**, o la **respuesta de otro diálogo** — así se
  encadenan.
- Es la manera de que un texto salga **con su bocadillo**: la acción "Enseñar un texto"
  escribe una línea pelada arriba de la pantalla, sin caja.


## Varias escenas

Cada escena es su fichero en `Scenes/`. En **Escena → Escenas del proyecto** las tienes
todas: crear, abrir, duplicar (con su relieve, su pintado y su siembra), renombrar,
borrar, y marcar con la estrella **por cuál empieza el juego**.

En el `main.prg` generado, cada escena trae sus propios procesos con su nombre delante
(`pueblo_sol`, `pueblo_camara`, `pueblo_montar`) y se monta una cada vez. Viajar de una
a otra es la acción **"Ir a otra escena"** de cualquier regla: el objeto que la pide no
cambia nada él mismo — deja la petición y la atiende `escena_gestor`, porque quien pide
el viaje es de la escena que se va y se moriría a media faena. Al llegar, se matan los
procesos de la anterior y se sueltan sus cuerpos, muros, vegetación, zonas, manantiales
y personajes.

Dos cosas que conviene saber:

- Los **nombres no se pueden repetir entre escenas** (cada objeto es un `PROCESS`). El
  editor avisa por la consola al generar si encuentra dos iguales.
- Las **variables del juego viven en el proyecto** (`.bgd2`), no en la escena: los
  puntos y la vida sobreviven al cambio de mapa.

## Telas: banderas, cortinas, toldos

Herramienta **Tela** (modo Escena): clic en la escena y ahí cuelga. Se ve **ondear
mientras la editas**, que es la única forma de saber si una bandera queda bien.

- **Cómo cuelga**: del borde de arriba (cortina), de las dos esquinas (guirnalda) o del
  borde izquierdo (bandera). Su tamaño, sus trozos (más = cae mejor, cuesta más) y **su
  propia textura**.
- **El viento**: fuerza y dirección.
- **La aparta lo que pase**: el jugador, y cualquier objeto con *"Aparta las telas al
  pasar"* marcado. El empujón es una **cápsula** de la altura del objeto, no una esfera,
  para que lo que sobresale no atraviese la tela. Y con *"…y la tela lo frena"* una lona
  pesada cuesta de atravesar.

En el juego sale como un `PROCESS` con sus locales, igual que la vegetación:

```prg
PROCESS pueblo_bandera()
BEGIN
    ctype = C_3D; csubtype = C3D_CLOTH;
    x = 3.0;  y = 5.5;  z = 0.0;            // de donde cuelga
    entity = g3d_cloth_create(3.0, 2.0, 16, 12, x, y, z);
    g3d_cloth_pin(entity, 2);               // sujeta por la izquierda
    target_x = 1.0; target_z = 0.3;         // hacia donde sopla
    wind = 1.2;                             // y con qué fuerza
    LOOP
        FRAME;
    END
END
```

Cambia `wind` desde tu código y tienes una racha o una tormenta, sin llamar a nada.


## Cuerdas

Herramienta **Cuerda** (modo Escena): **clic donde empieza y clic donde acaba** — mientras
mueves el ratón ves por dónde iría. Cuelga con su propia física: tendederos, cables,
lianas, puentes.

- **Cuánto cuelga**: de fábrica **0, o sea tirante**, que es como se tiende una cuerda.
  Súbelo para un tendedero con panza. Ojo: con holgura cae **mucho** más de lo que parece
  — con 40 de vano y 15%, la panza baja 9 unidades. El editor lo calcula por ti.
- Tirante se queda **recta**, y **se comba donde le cuelgues algo** (una sábana la vence
  un poco); si lo quitas, vuelve a tensarse.
- Mientras la dibujas ves **la curva que va a quedar**, en verde si cabe y en **rojo si
  tocaría el suelo** (y entonces no te deja ponerla). Los extremos suben solos lo justo
  para que quepa.
- **Cada extremo se puede soltar**: entonces cae y se balancea.
- La aparta lo que pase, igual que las telas.
- **Se le cuelgan telas**: si pones una tela justo debajo de una cuerda, **se cuelga de
  ella sola**; y en la ficha de la tela puedes elegir de qué cuerda y desde qué punto. Su borde de arriba sigue la cuerda, así que si la cuerda se mueve, la tela va con
  ella.

Se dibuja como un tubo, no como una línea, para que se vea desde cualquier ángulo y le
dé la luz. En el juego es un `PROCESS` con `wind` y `target_x/y/z`, como la tela.


## Colisión: la forma del propio modelo

Además de caja, esfera, cápsula y cilindro —donde la colisión es **un solo número** y
nada que no sea cuadrado encaja— hay dos formas que salen del modelo y **no hay que
ajustar**:

- **Malla exacta del modelo (fijo)**: la colisión *es* la geometría, triángulo a
  triángulo. Paredes, suelos, escaleras, rocas, un nivel entero. No se mueve (Jolt no
  admite malla móvil), así que es el decorado sólido: chocas con él, pero nada lo empuja.
- **Forma del modelo (se puede mover)**: una envolvente convexa hecha con los vértices
  del modelo. Es lo más pegado que se puede tener en algo que se mueve. Rellena los
  huecos (una silla colisiona como un bloque con su silueta); si necesitas el hueco de
  verdad, hazlo fijo con la malla exacta.


## Reglas: si pasa esto, haz esto

*Juego → Variables del juego* declara lo que el juego cuenta (`puntos`, `vida`,
llaves…): salen como `GLOBAL` de BennuGD2 con su valor inicial, así que las ve tu
código y el HUD 2D las pinta con `write_var` sin declarar nada.

Cualquier objeto 3D y cualquier personaje 2D tiene una lista de **reglas** en su
Inspector. Cada regla es un **disparador** y las **cosas que pasan**:

| Cuándo | Qué pasa |
|---|---|
| al empezar (una vez) | llamar a tu `PROCESS` (fichero + proceso, de dos listas) |
| cada frame | cambiar una variable (poner / sumar / restar) |
| al acercarse y pulsar tecla o botón | quitar esto de la escena |
| cuando el jugador lo toca | enseñar un texto N segundos |
| cuando el jugador entra en una zona pintada | |
| cuando una variable cumple (`>=`, `<`, …) | |
| cada N segundos | |

Dos casillas cambian el carácter de la regla: **solo la primera vez** (un cofre, un
checkpoint) y **mientras se cumpla** — sin marcar salta *al* cumplirse (tocar una
moneda suma una vez), marcado pasa todo el rato (perder vida dentro de la lava).

Se genera como código BennuGD2 normal dentro del `PROCESS` del objeto:

```
        // ---- regla: cada 1.00 s ----
        regla_t[0] = regla_t[0] + escena_dt;
        IF (regla_t[0] >= 1.000)
            regla_t[0] = 0.0;
            puntos = puntos + 10;
            aviso_txt = "+10 puntos!"; aviso_t = 1.50;
        END
```

## Editor de código y código propio en los objetos

Pulsa `Editar script` en un objeto (o **Archivo → Abrir main.prg**) y se abre el
**editor de código** a pantalla completa:

- **Pestañas**: varios ficheros abiertos a la vez, con aviso de los que tienen
  cambios sin guardar.
- **Árbol de `Scripts/`** con nuevo, renombrar, duplicar y borrar; y `main.prg`.
- **Esquema** del fichero: sus `PROCESS` y `FUNCTION`; clic y salta a la línea.
- **Buscar y reemplazar** (`Ctrl+F`, `Ctrl+H`, `F3`), **ir a la línea** (`Ctrl+G`),
  deshacer/rehacer, tamaño del texto, tabulación y ver espacios.
- **Autocompletado** (mientras escribes) con las palabras clave de BennuGD2, la API
  del motor y los procesos de tus propios ficheros.
- **Compilar** (`F5`): pasa `bgdc` por el fichero y **marca la línea del error** en el
  margen, con la lista de errores debajo para saltar a cada uno.

### Asignar código a un objeto o a un personaje

En el Inspector de un objeto 3D (sección **Código**) y en las acciones de un
personaje 2D, el código propio se elige **de dos listas**: el fichero `.prg` de
`Scripts/` y, de él, el `PROCESS` o la `FUNCTION`. Nada de teclear el nombre a
ciegas. `Nuevo` crea el esqueleto (en ese fichero, o en uno nuevo) y lo abre;
`Editar` salta a su línea.

Un objeto puede llamar a tu código **al empezar** (una vez), **cada frame**, o
**al acercarse y pulsar** una tecla o un botón del mando, con la distancia que le
pongas — un cofre, una puerta, una palanca. Sale en su `PROCESS`, como código
BennuGD2 normal:

```
    IF ((key(_E) OR joy_getbutton(JOY_BUTTON_A)) AND obj_acc[0] == 0)
        IF ((jug_x - x) * (jug_x - x) + (jug_z - z) * (jug_z - z) < 6.250)
            abrir_cofre();
        END
    END
    obj_acc[0] = (key(_E) OR joy_getbutton(JOY_BUTTON_A));
```

## HUD 2D (gráficos y textos de BennuGD2)

Panel **HUD 2D** + herramienta de la barra: coloca sobre la escena los gráficos y
los textos de pantalla del juego (1280x720) y los ves tal cual saldrán, porque el
editor lee los formatos **nativos**: `PNG/JPG`, **FPG** y las **fuentes
`.fnt`/`.fnx`**, con sus mismas métricas y su charset (los ficheros comprimidos con
gzip, como los guarda BennuGD2, también). El gráfico de un FPG se elige **viéndolo**:
`Elegir gráfico...` abre una rejilla con las miniaturas de todos sus gráficos, con
su código debajo y un filtro por código o nombre.

Cada elemento se genera como **código BennuGD2 normal**, con sus locales:

- Gráfico → un `PROCESS` con `file` / `graph` / `x` / `y` / `z` / `size` / `angle` /
  `flags` / `alpha` (`x,y` es el centro del gráfico, como en BennuGD2).
- Texto → `write()` (o `write_var()` sobre una GLOBAL que declara el editor) con su
  alineación `ALIGN_*` y `write_set_rgba()` para el color.
- Los recursos se cargan una vez en `escena_iniciar` con `map_load` / `fpg_load` /
  `fnt_load` y quedan en GLOBALs.

Se arrastran con el ratón en el viewport y se guardan en la escena.

## Sprites 2D en el mundo 3D (HD-2D)

**Ventana Sprites 3D** (flotante; se abre y se cierra con el boton de rejilla de la
barra -- que se queda marcado mientras esta abierta --, con el boton de la
herramienta de sprite, o desde el menu *Escena*; a dos columnas: la hoja a la izquierda, animaciones y
personajes colocados a la derecha). Eliges una **hoja de sprites** (PNG) de `Assets` y el editor
**detecta solo los fotogramas Y las animaciones**, por los huecos transparentes — funciona igual con
hojas en rejilla que con las irregulares (un *rip* tipico de Octopath: 82 fotogramas
de anchos distintos). Cada fotograma guarda su **ancla** en la linea del suelo de su
banda, que es lo que evita que el personaje baile al animar.

Las animaciones **las montas tu**: al detectar solo salen los fotogramas, y la lista
de animaciones esta vacia hasta que crees las tuyas. Si quieres un punto de partida,
la casilla **Agrupar solo** propone una agrupacion (parte cada banda donde el tamano
de los fotogramas pega un salto -- un ataque con lanza mide el doble que andar -- y
junta las poses sueltas seguidas); con la hoja de Cyrus de Octopath salen 18 grupos
utiles, pero va apagada porque llenaba la lista de `fila1_1`, `fila1_2`... y tapaba
las tuyas.

Lo que hagas **se guarda solo** en el `.sheet` en cuanto sueltas el raton, y al
volver a abrir la hoja se cargan **tus** fotogramas y animaciones tal cual (tambien
si la abres por el nombre del original y el trabajo esta en la copia `_sinfondo`).

Y se montan **a mano** igual de facil: eliges fotogramas en la hoja (clic, Ctrl+clic,
May+clic) y **Crear con los elegidos** los mete **en el orden en que los elegiste**
(vale un vaiven 1-2-3-2). La animacion elegida ensena **sus fotogramas en miniatura**,
en orden, y de cada uno se puede mover de sitio (`<` `>`) o quitarlo (`x`); ademas
**Anadir / Quitar los elegidos** y **Vaciar**. Retocar la hoja (partir, unir o borrar
un fotograma) ya **no** vacia las animaciones: se renumeran solas.

Tambien: renombrar, **partir cada N fotogramas**, **unir con la
siguiente**, cambiar los fps y verla en marcha. Todo se guarda en un `.sheet` junto a
la imagen.

Que traga la deteccion (medido con una bateria de hojas):

| caso | resultado |
|---|---|
| rejilla con fondo transparente | bien |
| rejilla con los sprites pegados al borde de la celda | bien |
| **fondo opaco** (blanco, magenta, negro, panel de color) | bien |
| **lineas de rejilla dibujadas** | bien |
| trozos sueltos (un arma separada del cuerpo) | se juntan solos |
| **creditos del rip** ("Ripped by ...") | se descartan |
| dos personajes que se TOCAN | salen en un fotograma: hay que partirlo |
| varias animaciones del mismo tamano en una fila | salen como una: hay que partirla |

**Un FPG tambien vale como hoja**: al elegirlo en la lista, sus graficos se juntan
una vez en `<nombre>_hoja.png` con transparencia, y los fotogramas y sus anclas
salen del propio FPG (su punto de control 0, que en un FPG de personajes suele
estar en los pies). A partir de ahi se trabaja como con cualquier hoja.

**Fondo transparente**: los rips traen el fondo pintado (blanco, magenta, un panel
de color), y asi el personaje saldria en el juego con su recuadro. Al detectar se
escribe una copia `<nombre>_sinfondo.png` con el fondo en transparente -- quitando
solo lo que conecta con el borde de la imagen, para no comerse la cara del
personaje si es del mismo color que el fondo -- y se trabaja con ella; el original
no se toca. Se puede desactivar con la casilla *Quitar fondo*.

**Recorte a mano**, para lo que la deteccion no puede adivinar: columnas x filas, o
tamano de celda con margen y separacion, **viendo la rejilla encima de la hoja**
antes de aplicarla; dibujar un fotograma arrastrando el raton; y ajustar el elegido
(recorte y ancla) con el resultado a la vista. Tambien partir un fotograma en N (dos
personajes pegados) o unir varios en uno.

Los personajes se **colocan en la escena** con la herramienta de sprite (clic en el
viewport), se mueven con el **gizmo** (y se escala su alto con el de escala), se
**duplican** ya configurados desde su ficha, y se ven ahi mismo animandose, dibujados por el **mismo renderizador del
juego**: tapan y son tapados por el 3D y proyectan su sombra. Por cada uno se genera
un `PROCESS` con `csubtype = C3D_SPRITE`, mas las tablas de recortes de su hoja:

```prg
PROCESS spr_cyrus()
BEGIN
    ctype = C_3D;  csubtype = C3D_SPRITE;
    entity = g3d_sprite_create(scene, x, y, z);
    g3d_sprite_set_pixels_per_unit(entity, 14.8);   // el fotograma mas alto = 2.5 unidades
    file = 0;  graph = hoja_cyrus;                  // la hoja entera
    LOOP
        tic++;
        IF (tic >= 7)  tic = 0;  paso = (paso + 1) % 6;  END
        dir = g3d_sprite_dir(entity, angle, 8);     // postura segun la camara
        fot = hoja_cyrus_andar[dir * 6 + paso];
        g3d_sprite_set_cell(entity, hoja_cyrus_x[fot], hoja_cyrus_y[fot], ...);
        FRAME;
    END
END
```

Tu codigo solo tiene que mover `x/y/z` y `angle`: la postura y el fotograma se
resuelven solos.

Y un sprite se configura **igual que un objeto 3D**, en su misma ficha:

- **Colision**: ninguna (decorativo), caja, esfera, capsula, cilindro o muro
  invisible; con masa, rebote, friccion, tamano y flotacion. Se genera con los
  mismos cuerpos rigidos de Jolt que los modelos.
- **Es el personaje que se controla**: capsula de personaje (char controller) con
  velocidad andando/corriendo, salto, radio y altura, nado en el agua que haya
  debajo, y **las teclas se eligen en el editor** (adelante/atras/izquierda/
  derecha/saltar/correr). Los controles salen respecto a la camara, como en los
  objetos.
- **Acciones, las que quieras**: una lista abierta donde cada accion se dispara con
  una **tecla** (las 70 y pico de BennuGD2, con filtro), con un **boton del mando**
  (`JOY_BUTTON_*`) o con los dos, y hace sonar una animacion, **llamar a codigo
  tuyo**, o ambas. Cada una elige si suena **entera al pulsar** (ataques: no se
  corta y no se repite mientras aguantas) o **mientras la aguantes** (cubrirse), y
  si va espejada. Si le pones un `PROCESS` tuyo y no existe, el editor **crea el
  esqueleto** en `Scripts/` y lo incluye, para que el juego compile desde el primer
  momento.
- **Mando**: moverse con el stick izquierdo (con zona muerta) y la cruceta, y un
  boton para saltar y otro para correr.
- **Velocidad de la animacion por personaje**: casilla *fps propios* en su ficha.
  Sin marcar usa los fps de la animacion en la hoja; marcada, este personaje va al
  ritmo que le pongas (y se ve al momento en el viewport).
- **Una animacion por tecla de movimiento, y espejarla**: al lado de cada tecla se elige que
  animacion suena mientras esta pulsada (W una, S otra...), que es como se hace un
  juego 2D de siempre, y una casilla **espejo** la dibuja volteada -- con una sola
  animacion de "andar a la izquierda" ya tienes la derecha. El espejo vale con
  animacion propia o sin ella (voltea la del estado). Si dejas la animacion vacia
  se usa la de 'lo que hace'; el salto manda sobre todo.
- **Animaciones por estado**: quieto, andando, corriendo y saltando; el proceso
  cambia sola la que toca segun lo que este haciendo.
- **Zonas de barrera** pintadas: la capa que le corta el paso.
- **Como NPC**: *bloquea el paso* le pone una caja de colision que **se mueve con
  el** (los muros invisibles son fijos), y *se puede interactuar* le anade el
  clasico "acercate y pulsa": distancia, tecla y/o boton del mando, animacion al
  hacerlo, un `PROCESS` tuyo (con esqueleto creado si no existe) y girarse hacia
  el jugador al acercarse.
- **Comportamiento** (para los que no son el jugador): quieto, **patrulla** entre
  donde esta y un punto B, **sigue al jugador** o **huye** de el, con su velocidad
  y su radio de deteccion; se apoya en el terreno al andar y usa su *animacion al
  moverse* mientras se mueve.
- **Ajuste a pixel**: la posicion del sprite se redondea a pixeles enteros de
  pantalla, para que el pixel art no tiemble al moverse.
- **Se ilumina con la escena**: el sprite recibe la luz ambiental y la del sol, asi
  que se apaga de noche con el ciclo de dia en vez de quedarse a pleno brillo como
  una pegatina.
- **La camara le sigue**: como un sprite no es una entidad, se le pega un marcador
  invisible y la camara mira a ese, asi que el modo de camara del editor
  (tercera persona, cenital, 2.5D) funciona igual con sprites que con modelos.

## Hoja de ruta

- Editor 2D completo (sprites en el mundo, scroll, mapas y colisiones) sobre
  `libmod_gfx`; el HUD de pantalla ya está hecho.
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
