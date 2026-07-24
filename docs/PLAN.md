# Plan de trabajo — bennugd2-studio + libmod_3d

Estamos mezclando frentes y por eso conviene parar y ordenar. Este documento separa
los hilos, dice qué es cada uno y en qué orden atacarlos. **Regla de oro para todo:**
cada cambio del motor debe ser *aditivo y opt-in* — el objetivo es fusionar
`libmod_3d` con el BennuGD2 oficial, y nada puede romper el comportamiento actual.

---

## Hilo A — El código generado debe ser 100% idiomático BennuGD2  ⬅ PRIORITARIO

### El problema
Hoy genero procesos con estilo *handle*, que no es como se escribe BennuGD2:

```bennu
PROCESS barrel_2(int id)
BEGIN
    LOOP
        g3d_entity_set_position(id, g3d_rigidbody_render_x(cuerpo), ...);
        g3d_entity_set_rotation(id, g3d_rigidbody_angle_x(cuerpo), ...);
        FRAME;
    END
END
```

### El objetivo (ya soportado por el motor, no hay que inventarlo)
`libmod_3d` declara variables **nativas** de proceso (`x, y, z, angle_x/y/z, size,
size_x/y/z, entity, target_x/y/z, intensity, color_r/g/b, fov, range, cone_angle,
alpha`) y un *hook* (`g3d_process_instance_hook`) las vuelca a la entidad cada
`FRAME`. El idioma correcto es:

```bennu
PROCESS Barrel()
PRIVATE
    int cuerpo;
END
BEGIN
    ctype = C_3D;
    csubtype = C3D_ENTITY;
    x = 72.6; y = 14.1; z = 24.6;          // variables nativas
    entity = g3d_model_spawn(scene, modelo, x, y, z, 0.0, 0.0);
    cuerpo = g3d_rigidbody_create_cylinder(x, y, z, ...);
    LOOP
        // el cuerpo fisico manda: se leen sus coords y se ESCRIBEN en x,y,z;
        // el hook las envia a la entidad sola al hacer FRAME. Sin set_position.
        x = g3d_rigidbody_render_x(cuerpo);
        y = g3d_rigidbody_render_y(cuerpo);
        z = g3d_rigidbody_render_z(cuerpo);
        angle_x = g3d_rigidbody_angle_x(cuerpo);
        angle_y = g3d_rigidbody_angle_y(cuerpo);
        angle_z = g3d_rigidbody_angle_z(cuerpo);
        FRAME;
    END
END
```
csubtypes: `C3D_ENTITY=1`, `C3D_LIGHT=2`, `C3D_CAMERA=3`.

### Estructura de ficheros (DECIDIDO, ✅ hecho)
Ya no se genera `__escena.prg` (que además duplicaba los scripts). En su lugar,
`main.prg` tiene un **bloque marcado** que el editor regenera:
```bennu
import ...
// >>> EDITOR: no toques este bloque, lo regenera el editor >>>
#include "Scripts/barrel_2.prg"      // un include por objeto, SIN duplicar
...
GLOBAL ... END
FUNCTION escena_iniciar() ... END
PROCESS escena_motor() ... END
// <<< EDITOR: fin del bloque >>>
PROCESS main()                        // esto es TUYO, no se pisa
BEGIN ... END
```
Verificado: se regenera el bloque respetando el código del usuario de fuera (una
función propia + llamada en `main` sobrevivieron a un regenerado que además quitó
un objeto); compila y corre.

### Alcance (todo lo que genera el editor hay que reescribirlo a este estilo)
- [x] Objeto con física → proceso con `ctype/csubtype`, `entity` local, x/y/z nativos.
      ✅ hecho y verificado en pantalla (barriles cayendo por la ladera dibujados
      por el hook desde sus vars nativas; barco flotando). `escena_iniciar` lanza
      estos objetos con `Nombre(modelo)` y el proceso se crea su propia entidad.
      Aún en estilo *handle* (se migran en su turno): jugador, cámara-objetivo y
      objetos enganchados a un hueso.
- [x] Objeto decorativo → proceso nativo self-spawn (con posado si el modelo lo
      necesita). Muro invisible → ya no spawnea entidad ni proceso: solo registra
      el colisionador. ✅ verificado en pantalla (roca y antorcha aparecen; escena
      completa 1552 frames).
- [x] Jugador (char controller) → x/y/z/angle_y nativos, controles dentro.
      ✅ hecho y verificado en pantalla (3ª persona: el personaje se dibuja a su
      escala junto al agua y la cámara lo sigue). La entidad la crea escena_iniciar
      (para que `follow_ent` siga válido sin depender del orden de procesos) y el
      proceso la ata a `entity`. Pendiente: cuando haya arma enganchada, migrar
      junto con el enganche a hueso.
- [x] Luz del escenario → proceso `escena_sol()` como el ejemplo `Sun()`
      (target/intensity/color nativos). ✅
- [x] Cámara → proceso `escena_camara()`, `csubtype = C3D_CAMERA`, x/y/z (posición)
      y target_x/y/z (mira) nativos; el follow (3ª/FPS/cenital) va dentro. ✅
      **Destapó un bug del motor:** el hook usaba `entity==0` como "sin atar", pero
      cámaras/luces/entidades numeran desde 0, así que la PRIMERA (id 0) no se
      dibujaba (cámara-proceso → pantalla negra). Arreglado: el default de la local
      `entity` pasa a -1 y el hook salta con `entity_id < 0`. Verificado en 3ª
      persona (cámara sigue, luz cálida) y FPS (1401 frames).
- [ ] `escena_iniciar` / `escena_motor`: revisar si parte del bucle desaparece
      (mucho de lo que hace el motor ahora a mano lo hará el hook).
- [x] Enganche de armas a hueso → el arma es un proceso nativo que cada frame lee
      la posición del hueso del personaje y se coloca ahí con x/y/z/angle_y. ✅
      Verificado en pantalla (la antorcha cuelga de mixamorig:RightHand del
      personaje). Valida además el fix de `quat_to_euler` (la orientación del arma
      sale del yaw del personaje via get_rotation). El bloque de enganche
      desaparece de escena_motor, que ahora SOLO avanza la física.
- [ ] Variables locales bien: `PRIVATE`/`PUBLIC` según haga falta, nombres claros.

### Cómo verificar
Regenerar el proyecto de pruebas, compilar y ejecutar; comparar el .prg generado
con un juego de `BennuGD2/games/` y con el ejemplo `Sun()`. Debe leerse como algo
que escribiría un programador de BennuGD2 a mano.

---

## Hilo B — Auditar el motor: funciones duplicadas o solapadas

### Método
Recorrer los 305 `FUNC(...)` de `libmod_3d_exports.h` agrupando por familia y
marcar las que hacen lo mismo o casi.

### Resultado de la pasada (304 funciones) — ✅ hecho
La API está **más limpia de lo esperado**: los loaders son uno por formato, los
`destroy`/`clear` uno por familia, los `SET_*` globales cada uno su efecto,
`MODEL_HEIGHT` (extensión Y) y `MODEL_SIZE` (mayor de las 3) son distintas. No hay
duplicados de verdad (dos funciones que hagan lo mismo). Lo que sí hay:

**Stubs muertos exportados (no hacen nada):**
- `G3D_PHYSICS_BODY_CREATE` → `return -1`
- `G3D_PHYSICS_BODY_SET_VELOCITY` → no-op
- `G3D_PHYSICS_STEP` → no-op
  Los tres son una API de física vieja, sustituida por `G3D_RIGIDBODY_*`. Peor que
  duplicados: quien las use tiene física que no funciona en silencio.
- `G3D_CAMERA_SET_PROJECTION` → no-op. Se usa `G3D_CAMERA_SET_FOV`.
- (`G3D_CAMERA_FREE`: hay wrapper stub pero NO está exportado, así que no molesta.)

**Mando muerto:**
- `G3D_LIGHT_SET_SHADOW_QUALITY`: guardaba una resolución por-luz que la pasada
  direccional ignora → subías la calidad y no cambiaba nada. Solapa con
  `G3D_SET_SHADOW_RESOLUTION`.

### Qué se hizo (política del merge: no borrar aún, arreglar/deprecar)
- **`LIGHT_SET_SHADOW_QUALITY` ahora FUNCIONA:** reenvía a la resolución del
  renderer (además de conservar el campo por-luz por compatibilidad). Deja de ser
  un mando muerto.
- **Los 4 stubs marcados como OBSOLETOS** en el código, con comentario que apunta a
  la función buena y nota de "borrado real cuando se cierre el merge". No se
  cambia su comportamiento (siguen siendo no-op) para no romper a nadie.

Borrado real de los stubs: cuando la fusión con BennuGD2 esté hecha y de acuerdo
con SplinterGU.

---

## Hilo C — Pendientes aparcados (no tocar hasta cerrar A y B)
- Verificar enganche de armas en FPS con un arma real (depende del Hilo A).
- Editor 2D visual (sprites, fuentes, menús). Grande; esperar a tener un menú
  hecho a mano para saber qué hace falta.
- Múltiples escenas 3D (cambio de nivel con descarga limpia). Necesita piezas de
  motor (destruir entidades/cuerpos/colliders/zonas).
- Empaquetado/AppImage: el `.tar.gz` portable ya funciona; pulir si hace falta.
- Preparar la propuesta de merge para SplinterGU.

---

## Orden propuesto
1. **Hilo A** (idioma BennuGD2) — es lo que más se nota y lo que pediste. Se hace
   una plantilla a la vez, verificando que compila y corre.
2. **Hilo B** (auditoría de duplicados) — encaja bien después de A, porque A ya
   deja claro qué `set_*` sobran.
3. **Hilo C** según prioridad tuya.

Un hilo cada vez. Nada de mezclar A con B en el mismo commit.
