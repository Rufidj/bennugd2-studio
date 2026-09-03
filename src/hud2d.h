// ============================================================================
//  hud2d.h - lectura de los formatos graficos NATIVOS de BennuGD2 (FPG, FNT)
//  y de imagenes sueltas (PNG/JPG), para el previo del HUD dentro del editor.
//
//  Por que se leen aqui y no se reutiliza libbggfx: el modulo grafico de
//  BennuGD2 dibuja con SDL_gpu sobre SU ventana y SU estado, y el editor pinta
//  con OpenGL crudo dentro de ImGui. Lo unico que hace falta son los pixeles y
//  las metricas, asi que se leen los ficheros (mismo formato DIV/Fenix que
//  carga libmod_gfx/file_div.c) y se suben a una textura de OpenGL.
// ============================================================================
#ifndef HUD2D_H
#define HUD2D_H

#ifdef __cplusplus
extern "C" {
#endif

/* Una imagen ya subida a OpenGL. */
typedef struct {
    unsigned int tex;      /* 0 = no cargada */
    int w, h;
} H2Img;

/* Un grafico dentro de un FPG (el "codigo" es el que se pone en graph). */
typedef struct {
    int   code;
    char  name[36];
    H2Img img;
} H2FpgGraph;

typedef struct {
    H2FpgGraph *g;
    int         n;
} H2Fpg;

/* Charsets, iguales que los de libbggfx (g_font.h). */
#define H2_CHARSET_ISO8859  0
#define H2_CHARSET_CP850    1
#define H2_CHARSET_UTF8     2

/* Una fuente .fnt/.fnx: los 256 glifos en un solo atlas, con las mismas
   metricas que usa write() (xoffset/yoffset/xadvance). */
typedef struct {
    unsigned int tex;          /* atlas */
    int aw, ah;
    struct {
        int u, v, w, h;        /* recorte dentro del atlas */
        int xoffset, yoffset;  /* donde se planta respecto al cursor */
        int xadvance;          /* cuanto avanza el cursor despues */
    } glyph[256];
    int charset;
    int maxheight;             /* alto de la fuente, como lo calcula libbggfx */
} H2Font;

int  h2_load_image(const char *path, H2Img  *out);   /* png/jpg/bmp/tga... */
int  h2_load_fpg  (const char *path, H2Fpg  *out);   /* fpg / f16 / f32 */
int  h2_load_fnt  (const char *path, H2Font *out);   /* fnt / fnx */

void h2_free_image(H2Img  *i);
void h2_free_fpg  (H2Fpg  *f);
void h2_free_font (H2Font *f);

/* Glifo que le toca a un caracter (latin-1) segun el charset de la fuente. */
int  h2_glyph_index(const H2Font *f, unsigned char ch);
/* Metricas identicas a las de libbggfx, para colocar el texto igual que write(). */
int  h2_text_width (const H2Font *f, const char *txt);   /* suma de xadvance */

/* El editor maneja UTF-8 (ImGui) pero BennuGD2 escribe bytes latin-1: la
   conversion hace falta tanto para el previo como para el .prg generado. */
void h2_utf8_to_latin1(const char *in, char *out, int outsz);

/* ---------------------------------------------------------------------------
   Hojas de sprites: sacar los fotogramas solos.
   Un rectangulo por fotograma, en pixeles dentro de la hoja.
   --------------------------------------------------------------------------- */
typedef struct {
    int x, y, w, h;     /* recorte del fotograma dentro de la hoja */
    int ax, ay;         /* ancla, en pixeles DENTRO del recorte: centro-abajo de
                           la banda (la linea del suelo). Con hojas irregulares
                           es lo que evita que el personaje baile al animar. */
    int band;           /* banda (fila) de la hoja en la que esta, empezando en 0 */
} H2Rect;

/* Detecta los fotogramas de una hoja mirando los huecos TRANSPARENTES: primero
   parte la imagen en bandas horizontales (filas sin nada), y cada banda en
   columnas. Es lo que funciona con las hojas normales, esten en rejilla o no.
   Devuelve cuantos ha encontrado (hasta 'max') y el tamano de la hoja. */
/* 'limpiar' (1 = recomendado) descarta las bandas que parecen texto (los
   creditos que traen los rips: "Ripped by ...") y junta los trozos sueltos de un
   mismo fotograma (un arma separada del cuerpo). Con 0 devuelve todo tal cual. */
int h2_detect_frames(const char *path, H2Rect *out, int max,
                     int *sheet_w, int *sheet_h, int limpiar);

/* Copia la hoja con el fondo puesto en transparente (los rips vienen con el
   fondo pintado, y si no se quita el sprite sale en el juego con su recuadro).
   1 = habia fondo y se ha escrito 'dst'; 0 = la hoja ya traia alfa; -1 = error. */
int h2_make_transparent(const char *src, const char *dst);

/* Compone una HOJA (PNG con transparencia) juntando todos los graficos de un FPG,
   y devuelve el recorte de cada uno. El ancla sale del punto de control 0 del
   grafico si lo trae (en un FPG suele estar en los pies), y si no, del centro
   abajo. Asi un FPG de toda la vida se puede usar como hoja de sprites sin
   cambiar nada de lo demas. Devuelve cuantos graficos ha metido. */
int h2_fpg_to_sheet(const char *fpg_path, const char *png_out,
                    H2Rect *out, int max, int *sheet_w, int *sheet_h);

/* Mira si esos fotogramas encajan en una rejilla regular de columnas x filas
   (lo normal). Devuelve 1 y la rejilla; 0 si la hoja es irregular. */
int h2_guess_grid(const H2Rect *frames, int n, int sheet_w, int sheet_h,
                  int *cols, int *rows);

#ifdef __cplusplus
}
#endif

#endif
