# Formato binario de niveles (.lvl)

Un `.lvl` guarda **todo un nivel** (todos sus planos y capas) en un solo fichero. Lo escriben y leen el editor y el juego con la
misma biblioteca C (`libmod_3d_lvl.c`, funciones `g3d_lvl_*`). En BennuGD2 se usa con `G3D_LVL_LOAD / FREE / PLANES / W / H / CELL`.

El nivel se sigue identificando por la ruta de su PNG (`Assets/Levels/level1.png`): si existe `level1.lvl` junto a ella, **manda el .lvl**;
si no, se leen los PNG antiguos (`level1.png`, `level1.p1.png`, `level1.capas.png`). El editor convierte los PNG a `.lvl` la primera vez que abre
un nivel (sin tocar los PNG) y a partir de ahi solo guarda `.lvl`.

## Estructura (little endian)

```
cabecera : "BLVL" | u16 version (=1) | u16 reservado
bloques  : { char tag[4] | u32 tamano | datos[tamano] } ... hasta el final
```

Un lector **ignora los bloques que no conoce**, asi se pueden anadir tipos (objetos, luces, zonas, disparadores) sin romper ficheros viejos.

### Bloque `PLAN` (un plano)
```
u16 indice del plano (0 = el jugable, 1.. = los de detras) | u16 ancho | u16 alto | u8 numero de capas (6)
capas * (ancho*alto) bytes   (fila 0 = fila de ARRIBA del mapa)
```
Capas 0, 1, 2 = lo que antes eran los canales R, G, B del PNG; 3, 4, 5 = los de `<nivel>.capas.png`.
El significado depende del tipo de nivel (2D: plano jugable / fondo / primer plano; pasillo: suelo / altura / marcadores).

### Bloque `LNKS` (enlaces entre objetos)
Texto, una linea por objeto con nombre o enlace: `plano;x;y;nombre;destinos` (y = fila contando desde ARRIBA; destinos separados por comas).
- puerta: solo nombre. Un boton o una diana con enlace abre SOLO las puertas que nombra en `destinos` (sin enlace usan el canal del tile, como antes).
- portal a otro nivel: `destinos` = `escena/puerta` (la puerta con nombre de la sala de destino donde apareces) o solo `escena`.
El editor los pone con la herramienta Enlazar (K).

## Siguientes bloques previstos
`OBJS` (objetos y personajes con parametros y acciones), `LGHT` (luces: color, parpadeo, canal), `ZONE` (zonas y disparadores), `CONF` (el contenido del `.level`).
