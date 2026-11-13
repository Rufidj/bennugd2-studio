# Circuitos de karts (modo Carreras)

Un circuito es la escena `Scenes/<nombre>.kart` (texto). Al **Guardar** el editor escribe tambien
`Assets/Models/<nombre>_pista.glb` (malla y colision de la carretera) y copia las texturas de serie a
`Assets/Textures/carreras/` (carpeta interna: no sale como textura de terreno). El juego lo monta
`runtime/carreras.prg` (`kr_montar()`).

Convenciones del motor: Y arriba, metros, yaw 0 mira a +Z y **+X queda a la izquierda** del piloto.
Ninguna superficie conducible por debajo de y = 0 (el controlador tiene un suelo invisible ahi).

## Caminos, nodos y tramos
Un **camino** es una lista de **nodos** unidos por **tramos**. El tramo `i` va del nodo `i` al `i+1` (el ultimo
vuelve al primero si el camino es cerrado). Hay un camino **principal** cerrado (el que lleva las vueltas y los
checkpoints) y los que quieras mas (atajos, desvios), que se reconocen como carretera pero no cuentan para las vueltas.

* **Nodo**: posicion, altura, ancho, peralte y tangente (derivada del Bezier; 0,0 = automatica). Altura, ancho y
  peralte se interpolan a lo largo de cada tramo (suave o lineal).
* **Tramo**: `tipo` (0 recta, 1 curva Bezier con las tangentes de sus nodos, 2 **salto**: la carretera se corta y el kart
  sale lanzado al final del tramo anterior), superficie (0 asfalto, 1 tierra, 2 hielo), muro izquierdo/derecho,
  fuerza del salto y perfil suave/lineal.

## Lineas del .kart (`clave=valores`, `#` = comentario)
| Linea | Significado |
|---|---|
| `kart=ruta` / `kart_tam=m` | modelo del kart y su alto |
| `vueltas=N` · `margen=m` · `salida=x z` | vueltas, cesped alrededor / limites del respawn, donde arranca (altura y rumbo salen de la carretera) |
| `escena=Scenes/x.scene` | escena 3D vinculada (terreno, vegetacion, agua, objetos); sin ella, cesped plano |
| `camino=nombre cerrado principal` | empieza un camino (cerrado y principal: 0/1) |
| `nodo=x z y ancho peralte tx tz tipo sup muroI muroD fuerza suave` | un nodo y el tramo que sale de el |
| `rampa=x z ancho largo alto fuerza` | rampa de malla sobre la carretera + trampolin en la cresta |
| `turbo=x z largo ancho` | placa de turbo pegada a la carretera |
| `ambiente=modo hora dur faros` | momento del dia: modo 0 como la escena vinculada, 1 dia, 2 noche, 3 ciclo; `hora` de salida del ciclo (0-24), `dur` = segundos que dura un dia entero, `faros` = faros del kart de noche (0/1) |
| `luz=x z alt r g b inten radio noche poste` | luz puntual (farola, foco); `noche` = solo de noche; `poste` = dibuja poste y bombilla. Maximo 24 (el motor admite 32 en total) |
| `cruce=x z tipo` | lo que se eligio en un cruce de carreteras: 0 puente, 1 tunel (se rehace solo al editar el trazado) |
| `lago=cx cz ancho largo prof mortal` | lago rectangular con cuenca excavada (**solo sin escena vinculada**) |
| `objeto=x z alturaExtra giro alto colision modelo` | modelo del proyecto (con escena vinculada es mas comodo colocarlos en el visor 3D) |
| `textura=parte metros ruta` | textura de una parte (`-` = la de serie): asfalto tierra hielo cesped muro base meta rampa turbo fondo |

El peralte es + en las curvas a la izquierda (sube el borde derecho). Los ficheros del formato anterior (`punto=`,
una sola linea cerrada) se leen y se convierten solos en un camino principal.

## Escena 3D vinculada
Con `escena=` el circuito se monta **sobre** esa escena: el terreno es el suelo (no se genera cesped) y su sol, cielo,
vegetacion, agua y objetos se editan con los modos Terreno y Escena. En el visor 3D se ve la carretera como vista previa
y la barra "Circuito (3D)" ofrece *Adaptar el terreno a la carretera*: nivela el terreno bajo cada tramo (terraplen o
desmonte, mas ancho cuanto mas haya que cortar; los saltos no se tocan) para que nunca tape la carretera.
No pongas un jugador en esa escena: el kart es el jugador, y su camara fija se elimina al montar.

## Lo que genera el juego
`<escena>_montar()` monta la escena vinculada (si hay), rellena la configuracion `kr_*` y llama a `kr_montar()`:
trampolines (rampas y saltos), placas de turbo, lagos, objetos, los **checkpoints** (`kr_cp_add`: la linea central del
camino principal cada ~6 m desde la meta, con rumbo, semiancho y superficie; en un salto es -1) y los puntos de los
demas caminos (`kr_ar_add`). Con ellos el runtime lleva vueltas, tiempos, semaforo de salida, superficie bajo el kart
(asfalto/tierra/hielo/fuera/agua), reaparicion en el ultimo checkpoint (tecla R) y pantalla de meta (ENTER repite).

## Pruebas sin GUI
- `EDITOR_PROYECTO=<carpeta> EDITOR_MODO=6 EDITOR_GENERAR=1 bin/editor3d`: abre/guarda el circuito, genera y compila.
- `EDITOR_KR_NUEVAESCENA=<nombre>` crea y vincula una escena; `EDITOR_KR_ADAPTAR=1` adapta el terreno (con
  `EDITOR_KR_COLINA=1` levanta antes una colina de 25 m bajo la carretera) e imprime cuantos puntos de carretera
  quedaban tapados antes y despues.
- `bgdi main.dcb [auto|recto|derrape|bot|lago]`: pilotos de prueba. `bot` da la vuelta siguiendo los checkpoints y sale
  al terminar. Todo va al log `carreras.log`.
- `EDITOR_KR_SEL=<tipo>:<indice>` y `EDITOR_KR_TAB=<0..3>` eligen un elemento o la pestana; `EDITOR_KR_3D=1` abre
  directamente el visor 3D con la escena vinculada.

## Borrar y deshacer (editor)
- **Supr** borra lo elegido (nodo, tramo, rampa, objeto, turbo, lago).
- **Borrar (goma)**: arrastra sobre el mapa y borra lo que toque; el tamano es en pixeles y se elige que borra
  (carretera, objetos, rampas y turbos, lagos). Un nodo se quita y sus tramos se unen; un tramo se quita y el camino
  cerrado se abre ahi o el abierto se parte en dos. Un trazo es un solo deshacer.
- **Borrar todo...**: la carretera, todo lo demas, o todo.
- **Deshacer / Rehacer**: Ctrl+Z, Ctrl+Shift+Z o Ctrl+Y (hasta 60 pasos), tambien para mover, crear y los cambios de los
  controles del panel. Prueba sin GUI: `EDITOR_KR_TESTBORRAR=1` (en memoria, no guarda); `EDITOR_KR_HERR=<n>` elige la herramienta.

## Cruces: puente o tunel
Donde dos carreteras de alturas distintas se superponen (mas de 4 m libres) se detecta un **cruce**: el editor lo marca en el mapa
(naranja = puente, morado = tunel), lo lista en la pestana Trazado y deja elegir como cruza. Con menos de 4 m de altura libre avisa
y no genera nada (sube una de las dos al menos 5 m).
* **Puente**: la carretera de arriba lleva una losa de 1 m de grosor y queda **abierta por debajo** (sin laterales), con dos pilares
  a los lados de la carretera de abajo.
* **Tunel**: los laterales de la de arriba siguen cerrados (terraplen) salvo la **boca** por donde pasa la de abajo, que queda forrada
  con dos paredes **macizas** (1 m de grueso, caras de dentro y de fuera, techo y tapas) de la misma altura a los dos lados, hasta la
  losa, y recortadas exactamente al ancho de la carretera de arriba, de modo que no sobresalen del terraplen.
Funciona entre dos caminos o entre dos partes del mismo (un ocho). Los saltos (huecos) no cuentan para los cruces.

## Ambiente y luces
`kr_ambiente()` lleva la hora: el sol recorre el cielo (amanecer a las 6, mediodia a las 12, ocaso a las 18) y de noche queda la luna,
el cielo y el ambiente se oscurecen, y se encienden las luces "de noche" y los faros del kart (un foco). En el ciclo el HUD muestra el
reloj. Con una escena vinculada y modo distinto de 0, el circuito mata el proceso del sol de esa escena y lleva el su `light`.
En modo 0 el cielo y el sol los lleva la escena y las luces solo de noche no se encienden (el circuito no sabe que hora es).
Se prueba sin GUI con un `.kart` que lleve `ambiente=3 16.6 60 1` y `luz=...`, y `bgdi main.dcb bot`.
