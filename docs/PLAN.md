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
- [ ] Objeto con física → proceso con `ctype/csubtype`, `entity` local, x/y/z nativos.
- [ ] Objeto decorativo y muro invisible.
- [ ] Jugador (char controller) → x/y/z/angle_y nativos, controles dentro.
- [ ] Luz del escenario → como el ejemplo `Sun()` (target/intensity/color nativos).
- [ ] Cámara → `csubtype = C3D_CAMERA`, target_x/y/z nativos.
- [ ] `escena_iniciar` / `escena_motor`: revisar si parte del bucle desaparece
      (mucho de lo que hace el motor ahora a mano lo hará el hook).
- [ ] Enganche de armas a hueso: rehacer en este estilo (además valida el fix de
      `quat_to_euler`, aún sin probar con un arma puesta).
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

### Candidatas ya detectadas
- **Sombras:** `G3D_SET_SHADOWS`, `G3D_LIGHT_ENABLE_SHADOW`,
  `G3D_LIGHT_SET_SHADOW_QUALITY`, `G3D_SET_SHADOW_RESOLUTION`.
  `SET_SHADOW_QUALITY` es un mando **muerto** (toca un campo por-luz que la pasada
  direccional ignora) → candidato claro a deprecar o a hacer que funcione.
- **Rotación/posición:** revisar solapes entre `ENTITY_SET_*`, `CHAR_SET_*`,
  `RIGIDBODY_*` y las variables nativas nuevas (mucho `set_position` sobra si el
  proceso usa x/y/z).
- (Rellenar con el resto al hacer la pasada.)

### Decisión de política (por el merge)
No **borrar** funciones aún: romper la API molesta a quien ya la use y complica el
merge. Preferir: (1) marcar como obsoletas en un comentario, (2) que las que estén
rotas funcionen o (3) que las redundantes reenvíen a la buena. Borrado real, solo
cuando el merge esté hecho y de acuerdo con SplinterGU.

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
