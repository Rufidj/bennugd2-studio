# Propuesta para BennuGD2: `instance_go_frame()`

**Permitir que una aplicación anfitriona ejecute BennuGD2 frame a frame**

---

## 1. Resumen

Se propone añadir a `bgdrtm` **una única función nueva**, `instance_go_frame()`, que ejecuta
**un solo frame** del programa BennuGD2 y devuelve el control a quien la llamó.

- Son **74 líneas añadidas y 0 líneas modificadas o borradas**.
- `instance_go_all()` **no se toca**, y `bgdi` sigue usándola exactamente igual.
- Para quien use BennuGD2 de la forma habitual, **el comportamiento es idéntico**: la función
  nueva es código inerte mientras nadie la llame.

El objetivo es que otras aplicaciones (en nuestro caso, un editor visual) puedan **embeber el
intérprete real de BennuGD2** en lugar de reimplementar el lenguaje.

---

## 2. Motivación: qué no se puede hacer hoy

Hoy el ciclo de vida de un programa BennuGD2 es (`core/bgdi/main.c`):

```c
string_init();
init_c_type();
dcb_load( dcbname );
sysproc_init();
bgdrtm_entry( argc, argv );

if ( mainproc ) {
    instance_new( mainproc, NULL );
    ret = instance_go_all();      /* <-- aquí se cede TODO el control */
}

bgdrtm_exit();
```

El problema está en `instance_go_all()`: **contiene el bucle completo del juego**
(`while ( first_instance ) { ... }`) y no retorna hasta que el programa termina.

Eso significa que **una aplicación anfitriona no puede intercalar su propio trabajo** entre
frames de BennuGD2. Si un programa externo quiere ejecutar código BennuGD2 y además pintar su
propia interfaz, hoy no tiene forma de hacerlo: o cede el bucle entero, o no lo usa.

No existe ninguna API pública para "avanza un frame y devuélveme el control".

---

## 3. El caso de uso real

Estamos desarrollando **bennugd2-studio**, un editor visual estilo Unity/Unreal para BennuGD2
(https://github.com/Rufidj/bennugd2-studio), construido sobre el módulo 3D `libmod_3d`.

El editor tiene un botón **Play** que previsualiza el juego **dentro de su propio viewport**,
sin abrir una segunda ventana. Para que esa previsualización sea fiel, los **scripts que
escribe el usuario** (PROCESSes de BennuGD2) tienen que ejecutarse de verdad.

Se valoraron dos caminos:

| Camino | Valoración |
|---|---|
| **Reimplementar** el lenguaje BennuGD2 dentro del editor | Descartado. Es un lenguaje completo (procesos, tipos, `FRAME`, cientos de funciones nativas). Además **divergiría** del runtime real: un script podría comportarse distinto en el editor y en el juego, que es peor que no previsualizarlo. |
| **Embeber el intérprete real** (`bgdrtm`) | Elegido. Fidelidad del 100 % porque *es* el motor de verdad, y muchísimo menos trabajo. |

Para el segundo camino solo falta una pieza: **poder avanzar un frame cada vez**. De ahí esta
propuesta.

Cabe destacar que `bgdrtm` **ya se compila como librería** (`add_library(bgdrtm ...)`), así que
enlazarla desde otra aplicación ya es posible hoy. Solo falta el punto de entrada.

---

## 4. La propuesta

Una función que ejecuta **exactamente un frame completo**: hace correr todos los procesos hasta
que todos hayan llamado a `FRAME`, ejecuta el trabajo de fin de frame, y **retorna**.

Devuelve un valor distinto de cero mientras el programa siga vivo (quedan instancias), y `0`
cuando ha terminado. El anfitrión simplemente la llama una vez por cada uno de sus propios
frames:

```c
while ( editor_sigue_abierto ) {
    procesar_eventos();
    if ( scripts_activos ) instance_go_frame( 0 );   /* un frame de BennuGD2 */
    renderizar_mi_interfaz();
    presentar();
}
```

La lógica interna es **la misma que la de `instance_go_all()`**; la única diferencia es que en
lugar de continuar con el siguiente frame, retorna.

---

## 5. Código

### `core/include/instance.h` (2 líneas)

```c
extern int64_t instance_go_all() ;
extern int64_t instance_go_frame( int run_handler_hooks ) ;   /* run ONE frame; !=0 while still running (host embedding).
                                                                 run_handler_hooks=0 si el host ya posee ventana/eventos. */
```

### `core/bgdrtm/interpreter.c` (72 líneas, justo debajo de `instance_go_all()`)

```c
/* Advance the program by ONE completed frame: run every process until they all
 * FRAME, then run the end-of-frame handler hooks once. Returns non-zero while
 * the program is still alive (instances remain), 0 when it has ended. Lets a
 * host application (e.g. the bennugd2-studio editor) embed the interpreter and
 * interleave BennuGD2 script execution with its own rendering, instead of
 * handing the whole loop to instance_go_all(). Same semantics per frame. */
int64_t instance_go_frame( int run_handler_hooks ) {

    if ( !first_instance ) return 0;

    must_exit = 0;

    while ( first_instance ) {
        frame_completed = 0;

        instance_reset_iterator_by_priority();
        INSTANCE* i = instance_next_by_priority();

        int64_t i_count = 0;
        while ( i ) {
            if ( LOCINT64( i, FRAME_PERCENT ) < 100 ) {
                int64_t status = LOCQWORD( i, STATUS );
                if ( status == STATUS_RUNNING ) {
                    if ( process_exec_hook_count )
                        for ( int n = 0; n < process_exec_hook_count; n++ )
                            process_exec_hook_list[n]( i );
                } else if ( status & ~( STATUS_KILLED | STATUS_DEAD ) ) {
                    i = instance_next_by_priority();
                    continue;
                }

                instance_go( i );
                i_count++;
                if ( must_exit ) return 0;
            }
            i = instance_next_by_priority();
        }

        if ( !i_count ) {
            frame_completed = 1;
            i = first_instance;
            while ( i ) {
                int64_t status = LOCQWORD( i, SAVED_STATUS ) = LOCQWORD( i, STATUS );
                if ( status == STATUS_DEAD || status == STATUS_KILLED || status == STATUS_RUNNING ) LOCINT64( i, FRAME_PERCENT ) -= 100;
                if ( i->last_priority != LOCINT64( i, PRIORITY ) ) {
                    instance_dirty( i );
                    LOCINT64( i, SAVED_PRIORITY ) = LOCINT64( i, PRIORITY );
                }
                i = i->next;
            }

            if ( !first_instance ) return 0;

            /* Los handler hooks de los modulos hacen cosas de "duenio de la
             * aplicacion": bombear/consumir la cola de eventos SDL (libsdlhandler
             * dump_new_events), volcar el frame grafico, etc. Cuando el interprete
             * va EMBEBIDO en un host (el editor) que ya posee la ventana y bombea
             * los eventos, ejecutarlos hace que ambos se peleen por la cola de
             * eventos y la aplicacion se queda sin input. Por eso el host puede
             * pedir que NO se ejecuten. */
            if ( run_handler_hooks && handler_hook_count )
                for ( int n = 0; n < handler_hook_count; n++ )
                    handler_hook_list[n].hook();

            return 1;   /* one frame done; still running */
        }
    }

    return 0;
}
```

> **Nota:** el comentario largo está en español porque salió del desarrollo. Si se integra,
> **conviene traducirlo al inglés** para mantener la coherencia con el resto del código base.
> Lo dejamos tal cual para no maquillar lo que realmente se probó.

---

## 6. El parámetro `run_handler_hooks` (y el fallo real que lo motivó)

Es la única decisión de diseño no evidente, y viene de un problema encontrado **en la práctica**.

Los módulos registran *handler hooks* que se ejecutan al final de cada frame. Varios de ellos
hacen tareas propias de **dueño de la aplicación**:

| Módulo | Hook | Qué hace |
|---|---|---|
| `libsdlhandler` | `dump_new_events` | `SDL_PumpEvents()` y **vacía la cola de eventos SDL** |
| `libbginput` | `process_key/mouse/touch/joy/sensors_events` | Procesa esos eventos |
| `libbggfx` | — | Vuelca el frame gráfico a pantalla |

Cuando el intérprete va **embebido**, la aplicación anfitriona **ya posee la ventana y ya bombea
los eventos SDL**. Si además se ejecutan esos hooks, **ambos se pelean por la misma cola de
eventos**: el anfitrión se queda sin input.

Nos pasó literalmente. Síntoma observado por el usuario: *"el personaje solo conseguí moverlo un
frame, después de eso el ordenador ya no respondía"*. El editor dejaba de recibir eventos y el
escritorio se quedaba esperando a una ventana OpenGL que no respondía.

Con `run_handler_hooks = 0` el problema desaparece. Y es seguro para los scripts: `key()` de
`libbginput` lee el array que devuelve `SDL_GetKeyboardState()` (`static const Uint8 * keystate`
en `i_key.c`), que SDL mantiene actualizado con el bombeo de eventos que ya hace el anfitrión.

**Un ejecutable normal como `bgdi` pasaría `1`** y tendría el comportamiento de siempre.

---

## 7. Compatibilidad

| Comprobación | Resultado |
|---|---|
| Líneas modificadas o borradas | **0** |
| Líneas añadidas | 74 (72 + 2) |
| `instance_go_all()` | **Intacta**, byte por byte |
| `bgdi` | Sigue llamando a `instance_go_all()`; sin cambios |
| Formato del `.dcb` | Sin cambios |
| ABI | Solo se añade un símbolo nuevo |
| Comportamiento sin anfitrión | **Idéntico** (la función no se llama nunca) |

Verificado con `git diff`: no hay ni una sola línea que empiece por `-`.

---

## 8. Cómo lo usa el anfitrión

Secuencia completa del editor (copiada de `core/bgdi/main.c`, salvo el bucle):

```c
/* Al arrancar (una sola vez por proceso) */
string_init();
init_c_type();
dcb_load( ruta_dcb );
sysproc_init();
bgdrtm_entry( 1, argv );
instance_new( mainproc, NULL );

/* Una vez por cada frame del editor */
int vivo = instance_go_frame( 0 );

/* Al parar: destruir las instancias vivas */
while ( first_instance ) instance_destroy( first_instance );
```

Detalle relevante: como los módulos se cargan con `dlopen(..., RTLD_NOW | RTLD_GLOBAL)` y el
editor **ya enlaza `libmod_3d.so`**, ese `dlopen` devuelve **la misma instancia ya cargada**.
Editor e intérprete comparten así **un solo motor** (mismas escenas, mismas entidades), que es
justo lo que hace útil la previsualización.

---

## 9. Qué se ha probado

- **Ejecución frame a frame verificada.** Un script de 3 frames, ejecutado desde un anfitrión
  externo, produce un entrelazado exacto: una llamada del anfitrión = un frame del script
  (`vivo=1, 1, 0`), y termina y se detiene limpiamente.
- **Con módulos reales cargados** (`libmod_misc`, `libmod_input`, `libmod_3d`), en un proceso con
  contexto OpenGL propio.
- **Arranque y parada repetidos** (Play → Stop → Play → Stop) sin fugas ni caídas, con el número
  de instancias estable y la memoria plana.
- **En el editor real**, con un proyecto de 16 objetos y un personaje FBX animado.

---

## 10. Notas para la integración

Somos conscientes de que la decisión es del mantenedor. Algunas alternativas por si se prefieren:

1. **Refactorizar en lugar de duplicar.** `instance_go_frame()` repite el cuerpo del bucle de
   `instance_go_all()`. Se podría reescribir `instance_go_all()` como:

   ```c
   int64_t instance_go_all() {
       must_exit = 0;
       while ( instance_go_frame( 1 ) ) ;
       return exit_value;
   }
   ```

   Es más limpio, pero **modifica** una función crítica existente. Optamos por la vía puramente
   aditiva precisamente para no arriesgar el comportamiento actual. Si se prefiere el refactor,
   encantados.

2. **Nombre.** `instance_go_frame` sigue la nomenclatura de `instance_go` / `instance_go_all`,
   pero cualquier otro nombre nos vale.

3. **Firma.** Si se prefiere no añadir el parámetro, puede dejarse `instance_go_frame(void)`
   (ejecutando siempre los hooks) y controlar los hooks con una variable global aparte, o
   separar en dos funciones. La necesidad real es poder **desactivarlos al embeber**.

4. Si resulta útil, podemos preparar un **Pull Request** ya listo, y/o un ejemplo mínimo de
   aplicación anfitriona para el repositorio.

---

## 11. Por qué creemos que aporta a BennuGD2

Más allá de nuestro editor, esta función abre la puerta a que BennuGD2 se pueda **embeber en
otras aplicaciones**: editores y herramientas visuales, previsualizaciones en vivo, tests
automatizados que avancen frame a frame, o integrarlo como motor de scripting dentro de otro
programa.

Y todo ello **sin cambiar en absoluto** el funcionamiento actual para quien no lo use.

Gracias por BennuGD2 y por considerarlo.
