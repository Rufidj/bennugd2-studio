# Carreras: sustituir piezas del sistema

El modo Carreras es una biblioteca BennuGD2 (`runtime/carreras.prg`) más **módulos sustituibles**. Al generar el juego, el editor
copia cada módulo a `Scripts/carreras/`. Si tu proyecto tiene un fichero con el mismo nombre en `Scripts/carreras_usuario/`, se usa
ese en lugar del de serie (el editor lo avisa en la consola con "modulo propio: ...").

| Módulo | Qué hace | Qué debes conservar |
|---|---|---|
| `kr_eventos.prg` | Avisos: inicio, vuelta, meta, reaparición, turbo, trampolín, derrape | Todas las funciones `kr_ev_*` con su firma |
| `kr_ia.prg` | Reparte el conductor de cada piloto (IA de serie o propia) | Escribir `kr_ia_acel/giro/salto[kid]` |
| `kr_ia_defecto.prg` | La IA de serie (carriles, trazada, rutas, equilibrio) | `kr_ia_defecto(kid)` |
| `kr_ia_usuario.prg` | Tu conductor, para pilotos con la casilla "IA propia" (`kr_p_ia[kid] = 1`) | `kr_ia_usuario(kid)` |
| `kr_fisica.prg` | El manejo: aceleración, giro, derrape, turbo, agarre, empujones. Lee `kr_f_*[kid]` y deja `kr_f_vx/vz/yaw/vf/der/carga/turbo` | `kr_fisica(kid)` |
| `kr_hud.prg` | Textos/estado en pantalla cada fotograma | `kr_hud_actualizar()` |
| `kr_camara.prg` | La cámara | el proceso de cámara que lance el motor |

Estado legible por los módulos: `kr_k_yaw/cp/lap/vel/pos/fin`, vecinos `kr_k_fr_*` (delante) y `kr_k_at_*` (detrás),
`kr_k_derrape/carga/turbo/surf`, ruta `kr_k_rt`, y las tablas `kr_p_*` de pilotos.

Cada módulo de serie lleva comentarios; cópialo y cámbialo. La prueba `tools/test_carreras.sh` comprueba la sustitución.

En el editor: pestaña Carreras → Pilotos → "Modulos del sistema" (botón *Personalizar* copia el módulo de serie a
`Scripts/carreras_usuario/`) y la casilla "IA propia" en cada rival (se guarda en `.kart` como `|1` tras el modelo).
