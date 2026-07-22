/*
 * script_host.c - Embebe el interprete REAL de BennuGD2 (bgdrtm) dentro del
 * editor, para que el Play en vivo pueda ejecutar los scripts propios del
 * usuario con fidelidad total (en vez de reimplementar el lenguaje).
 *
 * Por que funciona el estado compartido:
 *   bgdrtm carga los modulos con dlopen(..., RTLD_NOW | RTLD_GLOBAL). El editor
 *   ya enlaza libmod_3d.so, asi que ese dlopen devuelve LA MISMA instancia ya
 *   cargada -> interprete y editor comparten escenas/entidades/registros.
 *
 * Secuencia (copiada de core/bgdi/main.c, que es el runtime oficial):
 *   string_init -> init_c_type -> dcb_load -> sysproc_init -> bgdrtm_entry
 *   -> instance_new(mainproc) -> [cada frame] instance_go_frame -> bgdrtm_exit
 *
 * La diferencia con bgdi: en vez de ceder el bucle entero con instance_go_all(),
 * avanzamos UN frame por frame del editor con instance_go_frame() (la API que
 * anadimos a bgdrtm), para poder intercalar nuestro render.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "bgdrtm.h"
#include "instance.h"
#include "i_procdef.h"

/* Anadida por nosotros en core/bgdrtm/interpreter.c: corre UN frame completo.
   Devuelve !=0 mientras el programa sigue vivo. */
extern int64_t instance_go_frame( int run_handler_hooks );

/* Entradas del runtime que no declaran las cabeceras publicas. */
extern void string_init( void );
extern void init_c_type( void );
extern void sysproc_init( void );

static int  g_running = 0;
static int  g_inited  = 0;     /* el runtime solo se inicializa UNA vez por proceso */
static char g_prev_cwd[4096];

/* Arranca el interprete con un .dcb ya compilado. `workdir` = carpeta del
   proyecto (los scripts usan rutas relativas a ella). 1 = ok. */
int script_host_start( const char * dcb_path, const char * workdir ) {
    if ( g_running ) return 0;

    g_prev_cwd[0] = 0;
    if ( workdir && *workdir ) {
        if ( !getcwd( g_prev_cwd, sizeof( g_prev_cwd ) ) ) g_prev_cwd[0] = 0;
        if ( chdir( workdir ) != 0 )
            fprintf( stderr, "script_host: aviso, no pude entrar en %s\n", workdir );
    }

    /* Inicializacion del runtime: SOLO la primera vez. Repetirla (y sobre todo
       repetir bgdrtm_exit) hace que los finalizadores de los modulos liberen dos
       veces la misma memoria -> "munmap_chunk(): invalid pointer" en key_exit(). */
    if ( !g_inited ) {
        string_init();
        init_c_type();
    }

    if ( !dcb_load( (char *) dcb_path ) ) {
        fprintf( stderr, "script_host: no pude cargar el DCB '%s'\n", dcb_path );
        if ( g_prev_cwd[0] ) { if ( chdir( g_prev_cwd ) != 0 ) {} }
        return 0;
    }

    if ( !g_inited ) {
        sysproc_init();
        char * argv[2];
        argv[0] = (char *) dcb_path;
        argv[1] = NULL;
        bgdrtm_entry( 1, argv );
        g_inited = 1;
    }

    if ( !mainproc ) {
        fprintf( stderr, "script_host: el DCB no tiene un PROCESS main\n" );
        if ( g_prev_cwd[0] ) { if ( chdir( g_prev_cwd ) != 0 ) {} }
        return 0;
    }

    instance_new( mainproc, NULL );
    g_running = 1;
    return 1;
}

/* Avanza UN frame de los scripts. Devuelve !=0 mientras siguen vivos. */
int script_host_frame( void ) {
    if ( !g_running ) return 0;
    /* 0 = NO ejecutar los handler hooks de los modulos: el editor ya posee la
       ventana y bombea los eventos SDL; si los modulos tambien los consumen,
       la aplicacion se queda sin input y el escritorio se congela. */
    int alive = ( instance_go_frame( 0 ) != 0 );
    if ( !alive ) g_running = 0;
    return alive;
}

/* Para los scripts y vuelve al directorio de trabajo anterior.
   NO llamamos a bgdrtm_exit(): ejecuta los finalizadores de TODOS los modulos
   (p.ej. key_exit() de libbginput), que liberan memoria global del proceso. Como
   el editor arranca y para el Play muchas veces, la segunda pasada hacia doble
   free y abortaba. En su lugar destruimos las instancias vivas: los scripts se
   detienen, el runtime queda listo para el siguiente Play. */
void script_host_stop( void ) {
    if ( !g_running ) return;

    int guard = 0;
    while ( first_instance && guard++ < 100000 ) {
        INSTANCE * i = first_instance;
        instance_destroy( i );
        if ( first_instance == i ) break;   /* no avanza -> no bucle infinito */
    }

    g_running = 0;
    if ( g_prev_cwd[0] ) { if ( chdir( g_prev_cwd ) != 0 ) {} g_prev_cwd[0] = 0; }
}

int script_host_running( void ) { return g_running; }

/* Numero de instancias (procesos) vivas. Diagnostico: si crece cada frame,
   algo esta creando procesos sin parar y el editor se degrada hasta colgarse. */
int script_host_instance_count( void ) {
    int n = 0;
    INSTANCE * i = first_instance;
    while ( i && n < 1000000 ) { n++; i = i->next; }
    return n;
}
