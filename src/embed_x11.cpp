// ============================================================================
//  embed_x11.cpp - Embeber la ventana del juego (bgdi) DENTRO del editor.
//
//  Solo Linux/X11: lanza el proceso, espera a que cree su ventana (la localiza
//  por _NET_WM_PID), y la reparenta dentro de la ventana del editor con
//  XReparentWindow, colocada sobre el viewport "Escena". Cero cambios en el
//  motor: es un truco de ventanas del gestor X11.
// ============================================================================
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <spawn.h>
#include <ctime>
#include <cstdio>
#include <cstring>

extern char** environ;
#define GAME_TITLE "EDITOR_PLAY"

static Display* g_dpy   = nullptr;
static Window   g_child = 0;
static pid_t    g_pid   = 0;

// El manejador de errores de Xlib por defecto llama a exit() (cerraria el editor)
// cuando una ventana desaparece a mitad del recorrido -> lo ignoramos.
static int x_ignore(Display*, XErrorEvent*) { return 0; }

// Busca recursivamente una ventana cuyo titulo (_NET_WM_NAME o WM_NAME) == target.
static int name_matches(Display* d, Window w, const char* target) {
    Atom netName = XInternAtom(d, "_NET_WM_NAME", False);
    Atom utf8    = XInternAtom(d, "UTF8_STRING", False);
    Atom type; int fmt; unsigned long n, after; unsigned char* prop = nullptr;
    if (XGetWindowProperty(d, w, netName, 0, 64, False, utf8, &type, &fmt, &n, &after, &prop) == Success && prop) {
        int m = (strcmp((char*)prop, target) == 0); XFree(prop); if (m) return 1;
    }
    char* nm = nullptr;
    if (XFetchName(d, w, &nm) && nm) { int m = (strcmp(nm, target) == 0); XFree(nm); if (m) return 1; }
    return 0;
}
static Window find_by_name(Display* d, Window w, const char* target) {
    if (name_matches(d, w, target)) return w;
    Window root, parent, *children = nullptr; unsigned int nc = 0;
    Window result = 0;
    if (XQueryTree(d, w, &root, &parent, &children, &nc)) {
        for (unsigned i = 0; i < nc && !result; i++)
            result = find_by_name(d, children[i], target);
        if (children) XFree(children);
    }
    return result;
}

extern "C" int game_embed_start(const char* shell_cmd, unsigned long parent,
                                int x, int y, int w, int h) {
    if (g_child) return 1;                       // ya hay uno
    XSetErrorHandler(x_ignore);                  // NO cerrar el editor ante un X error
    if (!g_dpy) g_dpy = XOpenDisplay(nullptr);
    if (!g_dpy) return 0;

    // posix_spawn en vez de fork(): fork() con un contexto OpenGL de NVIDIA vivo
    // puede tumbar el proceso padre (el editor). posix_spawn evita ese problema.
    char* argv[] = { (char*)"/bin/sh", (char*)"-c", (char*)shell_cmd, nullptr };
    if (posix_spawn(&g_pid, "/bin/sh", nullptr, nullptr, argv, environ) != 0) {
        g_pid = 0; return 0;
    }

    Window root = DefaultRootWindow(g_dpy);
    Window found = 0;
    for (int t = 0; t < 250 && !found; t++) {    // ~5s esperando su ventana (por titulo)
        XSync(g_dpy, False);
        found = find_by_name(g_dpy, root, GAME_TITLE);
        if (!found) { struct timespec ts = {0, 20 * 1000 * 1000}; nanosleep(&ts, nullptr); }
    }
    if (!found) { return 0; }   // g_pid sigue vivo -> Stop podra matarlo

    g_child = found;
    XReparentWindow(g_dpy, g_child, (Window)parent, x, y);
    XMoveResizeWindow(g_dpy, g_child, x, y, w > 0 ? w : 640, h > 0 ? h : 480);
    XMapWindow(g_dpy, g_child);
    XSync(g_dpy, False);
    return 1;
}

extern "C" void game_embed_move(int x, int y, int w, int h) {
    if (g_dpy && g_child) {
        XMoveResizeWindow(g_dpy, g_child, x, y, w > 0 ? w : 1, h > 0 ? h : 1);
        XSync(g_dpy, False);
    }
}

extern "C" void game_embed_stop(void) {
    if (g_pid > 0) { kill(g_pid, SIGTERM); g_pid = 0; }
    g_child = 0;
    if (g_dpy) XSync(g_dpy, False);
}

extern "C" int game_embed_active(void) { return (g_child != 0 || g_pid != 0) ? 1 : 0; }
