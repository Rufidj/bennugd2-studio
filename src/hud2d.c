// ============================================================================
//  hud2d.c - ver en el editor lo mismo que dibuja BennuGD2 (ver hud2d.h).
//
//  Formatos (los mismos que libmod_gfx/file_div.c):
//    fpg/f16/f32 : cabecera de 8 bytes, paleta si es de 8 bits, y luego una
//                  ristra de graficos (codigo, tamano, puntos de control y
//                  pixeles).
//    fnt/fnx     : cabecera de 8 bytes (el 8o es el bpp), paleta si toca,
//                  charset, la tabla de los 256 caracteres y sus bitmaps.
//  Todo little-endian, como lo escribe DIV.
// ============================================================================
#include "hud2d.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/* Los graficos de BennuGD2 suelen venir comprimidos con gzip: su capa de
   ficheros (core/common/files.c) lo descomprime al vuelo, asi que aqui se hace
   igual. gzread lee tambien los que NO estan comprimidos, asi que sirve para
   los dos casos. */
typedef gzFile H2File;
#define h2_open(p)              gzopen((p), "rb")
#define h2_read(f, buf, n)      (gzread((f), (buf), (unsigned)(n)) == (int)(n))
#define h2_seek_cur(f, n)       (gzseek((f), (long)(n), SEEK_CUR) >= 0)
#define h2_seek_set(f, n)       (gzseek((f), (long)(n), SEEK_SET) >= 0)
#define h2_close(f)             gzclose(f)

/* La tabla de conversion la exporta el runtime de BennuGD2 (core/common/xctype.c):
   se usa la suya para que el editor elija EXACTAMENTE el mismo glifo que write(). */
extern unsigned char iso8859_1_to_cp850[256];

/* --------------------------------------------------------------------------- */
/*  Lectura little-endian                                                       */
/* --------------------------------------------------------------------------- */

static int rd_i32(H2File f, int *out) {
    unsigned char b[4];
    if (!h2_read(f, b, 4)) return 0;
    *out = (int)((unsigned)b[0] | ((unsigned)b[1] << 8) |
                 ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24));
    return 1;
}

/* --------------------------------------------------------------------------- */
/*  Paleta de los ficheros de 8 bits                                            */
/* --------------------------------------------------------------------------- */

/* 768 bytes de 0..63 (asi los guarda DIV: 6 bits por componente) y detras 576
   bytes de gamma que no se usan. */
static int read_palette(H2File f, unsigned char pal[256][3]) {
    unsigned char raw[768];
    int i;
    if (!h2_read(f, raw, sizeof(raw))) return 0;
    for (i = 0; i < 256; i++) {
        pal[i][0] = (unsigned char)(raw[i * 3 + 0] << 2);
        pal[i][1] = (unsigned char)(raw[i * 3 + 1] << 2);
        pal[i][2] = (unsigned char)(raw[i * 3 + 2] << 2);
    }
    return h2_seek_cur(f, 576);
}

/* --------------------------------------------------------------------------- */
/*  Un bitmap del fichero -> RGBA                                               */
/* --------------------------------------------------------------------------- */

/* Devuelve un buffer w*h*4 (hay que liberarlo) o NULL. En 8 y 16 bits el color
   0 es transparente, igual que el colorkey que pone file_div.c. */
static unsigned char *read_bitmap_rgba(H2File f, int w, int h, int bpp,
                                       unsigned char pal[256][3]) {
    unsigned char *rgba, *row;
    int x, y, pitch;

    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return NULL;
    if (bpp != 8 && bpp != 16 && bpp != 32) return NULL;   /* 1 bit: no se usa */

    pitch = (w * bpp + 7) / 8;
    rgba  = (unsigned char *)calloc((size_t)w * h, 4);
    row   = (unsigned char *)malloc((size_t)pitch);
    if (!rgba || !row) { free(rgba); free(row); return NULL; }

    for (y = 0; y < h; y++) {
        unsigned char *dst = rgba + (size_t)y * w * 4;
        if (!h2_read(f, row, pitch)) { free(rgba); free(row); return NULL; }
        for (x = 0; x < w; x++) {
            unsigned char *p = dst + x * 4;
            if (bpp == 8) {
                unsigned idx = row[x];
                if (!idx) continue;                       /* transparente */
                p[0] = pal[idx][0]; p[1] = pal[idx][1]; p[2] = pal[idx][2]; p[3] = 255;
            } else if (bpp == 16) {
                unsigned v = (unsigned)row[x * 2] | ((unsigned)row[x * 2 + 1] << 8);
                if (!v) continue;                         /* transparente */
                p[0] = (unsigned char)(((v >> 11) & 0x1F) * 255 / 31);
                p[1] = (unsigned char)(((v >>  5) & 0x3F) * 255 / 63);
                p[2] = (unsigned char)(( v        & 0x1F) * 255 / 31);
                p[3] = 255;
            } else {                                      /* 32: ARGB8888 */
                unsigned v = (unsigned)row[x * 4] | ((unsigned)row[x * 4 + 1] << 8) |
                             ((unsigned)row[x * 4 + 2] << 16) | ((unsigned)row[x * 4 + 3] << 24);
                p[0] = (unsigned char)((v >> 16) & 0xFF);
                p[1] = (unsigned char)((v >>  8) & 0xFF);
                p[2] = (unsigned char)( v        & 0xFF);
                p[3] = (unsigned char)((v >> 24) & 0xFF);
            }
        }
    }
    free(row);
    return rgba;
}

/* --------------------------------------------------------------------------- */
/*  Subir a OpenGL                                                              */
/* --------------------------------------------------------------------------- */

static unsigned int upload_rgba(const unsigned char *rgba, int w, int h) {
    GLuint tex = 0;
    GLint prev = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    /* NEAREST: un HUD pixel-art tiene que verse como en el juego, sin difuminar. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev);
    return (unsigned int)tex;
}

/* --------------------------------------------------------------------------- */
/*  Imagenes sueltas (PNG/JPG/...)                                              */
/* --------------------------------------------------------------------------- */

int h2_load_image(const char *path, H2Img *out) {
    SDL_Surface *s, *conv;
    if (!path || !out) return 0;
    out->tex = 0; out->w = out->h = 0;

    s = IMG_Load(path);
    if (!s) return 0;
    conv = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);   /* = RGBA en memoria */
    SDL_FreeSurface(s);
    if (!conv) return 0;

    /* SDL puede dejar relleno al final de cada linea; GL lo lee seguido. */
    {
        int y;
        unsigned char *tight = (unsigned char *)malloc((size_t)conv->w * conv->h * 4);
        if (!tight) { SDL_FreeSurface(conv); return 0; }
        for (y = 0; y < conv->h; y++)
            memcpy(tight + (size_t)y * conv->w * 4,
                   (unsigned char *)conv->pixels + (size_t)y * conv->pitch,
                   (size_t)conv->w * 4);
        out->tex = upload_rgba(tight, conv->w, conv->h);
        out->w = conv->w; out->h = conv->h;
        free(tight);
    }
    SDL_FreeSurface(conv);
    return out->tex != 0;
}

void h2_free_image(H2Img *i) {
    if (!i || !i->tex) return;
    GLuint t = (GLuint)i->tex;
    glDeleteTextures(1, &t);
    i->tex = 0; i->w = i->h = 0;
}

/* --------------------------------------------------------------------------- */
/*  FPG                                                                         */
/* --------------------------------------------------------------------------- */

int h2_load_fpg(const char *path, H2Fpg *out) {
    H2File f;
    char header[8];
    int bpp, cap = 0;
    unsigned char pal[256][3];

    if (!path || !out) return 0;
    out->g = NULL; out->n = 0;
    memset(pal, 0, sizeof(pal));

    f = h2_open(path);
    if (!f) return 0;
    if (!h2_read(f, header, 8)) { h2_close(f); return 0; }

    if      (!memcmp(header, "f32\x1A\x0D\x0A\x00", 7)) bpp = 32;
    else if (!memcmp(header, "f16\x1A\x0D\x0A\x00", 7)) bpp = 16;
    else if (!memcmp(header, "fpg\x1A\x0D\x0A\x00", 7)) bpp = 8;
    else { h2_close(f); return 0; }

    if (bpp == 8 && !read_palette(f, pal)) { h2_close(f); return 0; }

    for (;;) {
        int code, regsize, w, h, flags;
        char name[32], fpname[12];
        unsigned char *rgba;

        if (!rd_i32(f, &code)) break;
        if (!rd_i32(f, &regsize)) break;
        if (!h2_read(f, name, sizeof(name))) break;
        if (!h2_read(f, fpname, sizeof(fpname))) break;
        if (!rd_i32(f, &w) || !rd_i32(f, &h) || !rd_i32(f, &flags)) break;
        if (code < 0 || code > 999) break;

        /* flags = numero de puntos de control; cada uno son dos int16 */
        if (flags > 0 && !h2_seek_cur(f, (long)flags * 4)) break;

        rgba = read_bitmap_rgba(f, w, h, bpp, pal);
        if (!rgba) break;

        if (out->n == cap) {
            H2FpgGraph *ng;
            cap = cap ? cap * 2 : 16;
            ng = (H2FpgGraph *)realloc(out->g, (size_t)cap * sizeof(H2FpgGraph));
            if (!ng) { free(rgba); break; }
            out->g = ng;
        }
        memcpy(out->g[out->n].name, name, sizeof(name));
        out->g[out->n].name[sizeof(name)] = 0;
        out->g[out->n].code = code;
        out->g[out->n].img.tex = upload_rgba(rgba, w, h);
        out->g[out->n].img.w = w;
        out->g[out->n].img.h = h;
        out->n++;
        free(rgba);
    }

    h2_close(f);
    return out->n > 0;
}

void h2_free_fpg(H2Fpg *f) {
    int i;
    if (!f) return;
    for (i = 0; i < f->n; i++) h2_free_image(&f->g[i].img);
    free(f->g);
    f->g = NULL; f->n = 0;
}

/* --------------------------------------------------------------------------- */
/*  FNT / FNX                                                                   */
/* --------------------------------------------------------------------------- */

static int next_pow2(int v) { int p = 1; while (p < v) p <<= 1; return p; }

int h2_load_fnt(const char *path, H2Font *out) {
    H2File f;
    char header[8];
    int bpp, charset, types = 0, i, nuevo;
    unsigned char pal[256][3];
    unsigned char *pix[256];
    int gw[256], gh[256];
    int aw = 64, ah, cx = 0, cy = 0, rowh = 0, maxw = 1;
    unsigned char *atlas;

    struct { int width, height, xadvance, yadvance, xoffset, yoffset, fileoffset; } cd[256];

    if (!path || !out) return 0;
    memset(out, 0, sizeof(*out));
    memset(pix, 0, sizeof(pix));
    memset(pal, 0, sizeof(pal));

    f = h2_open(path);
    if (!f) return 0;
    if (!h2_read(f, header, 8)) { h2_close(f); return 0; }

    nuevo = !memcmp(header, "fnx\x1A\x0D\x0A\x00", 7);
    if (!nuevo && memcmp(header, "fnt\x1A\x0D\x0A\x00", 7)) { h2_close(f); return 0; }

    bpp = (unsigned char)header[7];
    if (bpp == 0) bpp = 8;
    if (bpp == 8 && !read_palette(f, pal)) { h2_close(f); return 0; }

    if (!rd_i32(f, &types)) { h2_close(f); return 0; }

    if (nuevo) {
        for (i = 0; i < 256; i++) {
            if (!rd_i32(f, &cd[i].width)    || !rd_i32(f, &cd[i].height) ||
                !rd_i32(f, &cd[i].xadvance) || !rd_i32(f, &cd[i].yadvance) ||
                !rd_i32(f, &cd[i].xoffset)  || !rd_i32(f, &cd[i].yoffset) ||
                !rd_i32(f, &cd[i].fileoffset)) { h2_close(f); return 0; }
        }
        charset = types;
    } else {
        /* Formato viejo: solo ancho, alto, yoffset y offset en el fichero. */
        for (i = 0; i < 256; i++) {
            int w, h, yo, fo;
            if (!rd_i32(f, &w) || !rd_i32(f, &h) || !rd_i32(f, &yo) || !rd_i32(f, &fo)) {
                h2_close(f); return 0;
            }
            cd[i].width = w; cd[i].height = h;
            cd[i].xoffset = 0; cd[i].yoffset = yo;
            cd[i].xadvance = w; cd[i].yadvance = h + yo;
            cd[i].fileoffset = fo;
        }
        charset = H2_CHARSET_CP850;
    }

    /* Bitmaps de cada caracter */
    for (i = 0; i < 256; i++) {
        gw[i] = gh[i] = 0;
        if (!cd[i].fileoffset || cd[i].width <= 0 || cd[i].height <= 0) continue;
        if (!h2_seek_set(f, cd[i].fileoffset)) continue;
        pix[i] = read_bitmap_rgba(f, cd[i].width, cd[i].height, bpp, pal);
        if (!pix[i]) continue;
        gw[i] = cd[i].width; gh[i] = cd[i].height;
        if (gw[i] > maxw) maxw = gw[i];
    }
    h2_close(f);

    /* Atlas: los glifos en estantes, con un pixel de separacion. */
    while (aw < maxw + 2) aw <<= 1;
    if (aw < 128) aw = 128;
    for (i = 0; i < 256; i++) {
        if (!pix[i]) continue;
        if (cx + gw[i] + 1 > aw) { cx = 0; cy += rowh + 1; rowh = 0; }
        out->glyph[i].u = cx; out->glyph[i].v = cy;
        out->glyph[i].w = gw[i]; out->glyph[i].h = gh[i];
        cx += gw[i] + 1;
        if (gh[i] > rowh) rowh = gh[i];
    }
    ah = next_pow2(cy + rowh + 1);
    if (ah < 8) ah = 8;

    atlas = (unsigned char *)calloc((size_t)aw * ah, 4);
    if (!atlas) { for (i = 0; i < 256; i++) free(pix[i]); return 0; }
    for (i = 0; i < 256; i++) {
        int y;
        if (!pix[i]) continue;
        for (y = 0; y < gh[i]; y++)
            memcpy(atlas + ((size_t)(out->glyph[i].v + y) * aw + out->glyph[i].u) * 4,
                   pix[i] + (size_t)y * gw[i] * 4, (size_t)gw[i] * 4);
        free(pix[i]);
    }

    out->tex = upload_rgba(atlas, aw, ah);
    free(atlas);
    out->aw = aw; out->ah = ah;
    out->charset = charset;

    for (i = 0; i < 256; i++) {
        out->glyph[i].xoffset  = cd[i].xoffset;
        out->glyph[i].yoffset  = cd[i].yoffset;
        out->glyph[i].xadvance = cd[i].xadvance;
        /* Alto de la fuente: el mismo criterio que libbggfx (g_text.c). */
        if (out->glyph[i].h && out->maxheight < out->glyph[i].h + cd[i].yoffset)
            out->maxheight = out->glyph[i].h + cd[i].yoffset;
    }
    return out->tex != 0;
}

void h2_free_font(H2Font *f) {
    if (!f || !f->tex) return;
    GLuint t = (GLuint)f->tex;
    glDeleteTextures(1, &t);
    memset(f, 0, sizeof(*f));
}

/* --------------------------------------------------------------------------- */
/*  Metricas y texto                                                            */
/* --------------------------------------------------------------------------- */

int h2_glyph_index(const H2Font *f, unsigned char ch) {
    if (f && f->charset == H2_CHARSET_CP850) return iso8859_1_to_cp850[ch];
    return ch;
}

int h2_text_width(const H2Font *f, const char *txt) {
    int l = 0;
    const unsigned char *p = (const unsigned char *)txt;
    if (!f || !txt) return 0;
    while (*p) l += f->glyph[h2_glyph_index(f, *p++)].xadvance;
    return l;
}

/* UTF-8 (lo que teclea el usuario en ImGui) -> latin-1 (lo que escribe
   BennuGD2). Lo que no cabe en latin-1 se sustituye por '?'. */
void h2_utf8_to_latin1(const char *in, char *out, int outsz) {
    int o = 0;
    const unsigned char *p = (const unsigned char *)in;
    if (!out || outsz <= 0) return;
    if (!in) { out[0] = 0; return; }
    while (*p && o < outsz - 1) {
        unsigned c = *p;
        if (c < 0x80) { out[o++] = (char)c; p++; }
        else if ((c & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
            unsigned u = ((c & 0x1F) << 6) | (p[1] & 0x3F);
            out[o++] = (char)(u < 256 ? u : '?');
            p += 2;
        } else {
            /* 3 o 4 bytes: fuera de latin-1 */
            out[o++] = '?';
            p++;
            while ((*p & 0xC0) == 0x80) p++;
        }
    }
    out[o] = 0;
}

/* ===========================================================================
   Hojas de sprites: detectar los fotogramas
   =========================================================================== */

/* Carga la imagen en RGBA (hay que liberar con free). */
static unsigned char *load_rgba(const char *path, int *w, int *h) {
    SDL_Surface *s = IMG_Load(path), *conv;
    unsigned char *px;
    int y;
    if (!s) return NULL;
    conv = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(s);
    if (!conv) return NULL;
    px = (unsigned char *)malloc((size_t)conv->w * conv->h * 4);
    if (!px) { SDL_FreeSurface(conv); return NULL; }
    for (y = 0; y < conv->h; y++)
        memcpy(px + (size_t)y * conv->w * 4,
               (unsigned char *)conv->pixels + (size_t)y * conv->pitch,
               (size_t)conv->w * 4);
    *w = conv->w; *h = conv->h;
    SDL_FreeSurface(conv);
    return px;
}

/* Dos colores parecidos (por canal). */
static int color_cerca(const unsigned char *a, const unsigned char *b, int tol) {
    int i;
    for (i = 0; i < 3; i++) {
        int d = (int)a[i] - (int)b[i];
        if (d < 0) d = -d;
        if (d > tol) return 0;
    }
    return 1;
}

/* Marca como VACIO lo que es fondo, que no siempre es transparencia:
   1. alfa por debajo del umbral;
   2. el fondo OPACO de los rips (blanco, magenta, negro...): se rellena desde
      los bordes de la imagen mientras el color se parezca, asi que se come tanto
      la pagina blanca como el panel de color donde estan los sprites;
   3. las lineas de rejilla dibujadas: una fila o columna entera de un solo color
      no separa nada visualmente, pero si pega todos los fotogramas en uno. */
static unsigned char *mascara_vacio(const unsigned char *px, int w, int h, int con_lineas) {
    const int ALFA = 8, TOL = 16;
    unsigned char *vacio = (unsigned char *)calloc((size_t)w * h, 1);
    unsigned char *cand;      /* pixeles que PODRIAN ser fondo (por color) */
    int *cola;
    int x, y, cab = 0, fin = 0;
    size_t transp = 0, total = (size_t)w * h;

    if (!vacio) return NULL;
    cand = (unsigned char *)calloc(total, 1);
    if (!cand) return vacio;

    /* 1. transparencia: eso es fondo esta donde este */
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (px[((size_t)y * w + x) * 4 + 3] < ALFA) {
                vacio[(size_t)y * w + x] = 1;
                cand[(size_t)y * w + x] = 1;
                transp++;
            }

    /* 2. candidatos por color: las esquinas (el fondo casi siempre las toca) y,
          si la hoja no trae transparencia, los colores que ocupan media hoja. */
    {
        unsigned char refs[8][3];
        int nrefs = 0, i;
        const int esq[4][2] = { {0,0}, {w-1,0}, {0,h-1}, {w-1,h-1} };
        for (i = 0; i < 4 && nrefs < 8; i++) {
            const unsigned char *c = px + (((size_t)esq[i][1] * w) + esq[i][0]) * 4;
            int rep = 0, k;
            if (c[3] < ALFA) continue;
            for (k = 0; k < nrefs; k++) if (color_cerca(c, refs[k], TOL)) { rep = 1; break; }
            if (!rep) { refs[nrefs][0]=c[0]; refs[nrefs][1]=c[1]; refs[nrefs][2]=c[2]; nrefs++; }
        }
        if (transp * 20 < total) {
            /* histograma en rejilla de 32 niveles por canal (tolerante al ruido) */
            static unsigned int hist[32768];
            unsigned int k;
            memset(hist, 0, sizeof(hist));
            for (y = 0; y < h; y++)
                for (x = 0; x < w; x++) {
                    const unsigned char *c = px + ((size_t)y * w + x) * 4;
                    hist[((c[0] >> 3) << 10) | ((c[1] >> 3) << 5) | (c[2] >> 3)]++;
                }
            for (i = 0; i < 3 && nrefs < 8; i++) {
                unsigned int mejor = 0, idx = 0;
                for (k = 0; k < 32768; k++) if (hist[k] > mejor) { mejor = hist[k]; idx = k; }
                if ((size_t)mejor * 8 < total) break;      /* menos del 12%: ya no es fondo */
                hist[idx] = 0;
                refs[nrefs][0] = (unsigned char)(((idx >> 10) & 31) << 3);
                refs[nrefs][1] = (unsigned char)(((idx >>  5) & 31) << 3);
                refs[nrefs][2] = (unsigned char)(( idx        & 31) << 3);
                nrefs++;
            }
        }
        for (y = 0; y < h; y++)
            for (x = 0; x < w; x++) {
                size_t p = (size_t)y * w + x;
                int k;
                if (cand[p]) continue;
                for (k = 0; k < nrefs; k++)
                    if (color_cerca(px + p * 4, refs[k], TOL)) { cand[p] = 1; break; }
            }
    }

    /* 3. Solo es fondo lo que se llega a tocar DESDE EL BORDE. Asi la cara del
          personaje no se borra por ser del mismo blanco que la pagina: esta
          rodeada de dibujo y no conecta con fuera. */
    cola = (int *)malloc(sizeof(int) * total);
    if (!cola) { free(cand); return vacio; }
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x += (y == 0 || y == h - 1) ? 1 : (w > 1 ? w - 1 : 1)) {
            size_t p = (size_t)y * w + x;
            if (!cand[p] || vacio[p]) continue;
            vacio[p] = 1;
            cola[fin++] = (int)p;
        }
    }
    while (cab < fin) {
        int q = cola[cab++], qx = q % w, qy = q / w, k;
        int vx[4] = { qx - 1, qx + 1, qx, qx };
        int vy[4] = { qy, qy, qy - 1, qy + 1 };
        for (k = 0; k < 4; k++) {
            size_t r;
            if (vx[k] < 0 || vx[k] >= w || vy[k] < 0 || vy[k] >= h) continue;
            r = (size_t)vy[k] * w + vx[k];
            if (vacio[r] || !cand[r]) continue;
            vacio[r] = 1;
            cola[fin++] = (int)r;
        }
    }
    free(cola);
    free(cand);

    /* 4. lineas de rejilla dibujadas: pegan todos los fotogramas en uno solo.
          Solo estorban al MEDIR, no al quitar el fondo. */
    if (!con_lineas) return vacio;
    for (y = 0; y < h; y++) {
        int llenos = 0, iguales = 0;
        const unsigned char *ref = NULL;
        for (x = 0; x < w; x++) {
            size_t p = (size_t)y * w + x;
            if (vacio[p]) continue;
            llenos++;
            if (!ref) { ref = px + p * 4; iguales++; }
            else if (color_cerca(px + p * 4, ref, TOL)) iguales++;
        }
        if (llenos >= w * 9 / 10 && iguales >= llenos * 95 / 100)
            for (x = 0; x < w; x++) vacio[(size_t)y * w + x] = 1;
    }
    for (x = 0; x < w; x++) {
        int llenos = 0, iguales = 0;
        const unsigned char *ref = NULL;
        for (y = 0; y < h; y++) {
            size_t p = (size_t)y * w + x;
            if (vacio[p]) continue;
            llenos++;
            if (!ref) { ref = px + p * 4; iguales++; }
            else if (color_cerca(px + p * 4, ref, TOL)) iguales++;
        }
        if (llenos >= h * 9 / 10 && iguales >= llenos * 95 / 100)
            for (y = 0; y < h; y++) vacio[(size_t)y * w + x] = 1;
    }
    return vacio;
}

static int cmp_int(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

int h2_detect_frames(const char *path, H2Rect *out, int max,
                     int *sheet_w, int *sheet_h, int limpiar) {
    unsigned char *px, *vacio, *fila;
    int w, h, x, y, n = 0, banda = 0;

    if (!path || !out || max <= 0) return 0;
    px = load_rgba(path, &w, &h);
    if (!px) return 0;
    if (sheet_w) *sheet_w = w;
    if (sheet_h) *sheet_h = h;

    vacio = mascara_vacio(px, w, h, 1);
    if (!vacio) { free(px); return 0; }

    fila = (unsigned char *)calloc((size_t)h, 1);
    if (!fila) { free(px); free(vacio); return 0; }
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (!vacio[(size_t)y * w + x]) { fila[y] = 1; break; }

    for (y = 0; y < h; ) {
        int y0, y1, cx, prim = n;
        if (!fila[y]) { y++; continue; }
        y0 = y;
        while (y < h && fila[y]) y++;
        y1 = y;                                  /* banda [y0, y1) */

        for (cx = 0; cx < w && n < max; ) {
            int x0, x1, yy, xx, top, bot, hay = 0;
            for (yy = y0; yy < y1; yy++)
                if (!vacio[(size_t)yy * w + cx]) { hay = 1; break; }
            if (!hay) { cx++; continue; }
            x0 = cx;
            while (cx < w) {
                hay = 0;
                for (yy = y0; yy < y1; yy++)
                    if (!vacio[(size_t)yy * w + cx]) { hay = 1; break; }
                if (!hay) break;
                cx++;
            }
            x1 = cx;
            top = y1; bot = y0;
            for (yy = y0; yy < y1; yy++)
                for (xx = x0; xx < x1; xx++)
                    if (!vacio[(size_t)yy * w + xx]) {
                        if (yy < top) top = yy;
                        if (yy > bot) bot = yy;
                        break;
                    }
            if (top > bot) { top = y0; bot = y1 - 1; }
            out[n].x = x0; out[n].y = top;
            out[n].w = x1 - x0; out[n].h = bot - top + 1;
            out[n].ax = out[n].w / 2;
            out[n].ay = (y1 - 1) - top;
            out[n].band = banda;
            n++;
        }

        if (limpiar && n > prim + 1) {
            /* Trozos SUELTOS de un mismo fotograma (un arma, un efecto): en una
               banda de personajes destacan porque son mucho mas bajos que el
               resto. Se pegan al fotograma vecino mas cercano. */
            int cnt = n - prim, i, med;
            int *alt = (int *)malloc(sizeof(int) * (size_t)cnt);
            if (alt) {
                for (i = 0; i < cnt; i++) alt[i] = out[prim + i].h;
                qsort(alt, cnt, sizeof(int), cmp_int);
                med = alt[cnt / 2];
                free(alt);
                for (i = 0; i < cnt; ) {
                    H2Rect *f = &out[prim + i];
                    if (f->h * 5 >= med * 2 || cnt <= 1) { i++; continue; }   /* >= 40% del alto: es un fotograma */
                    {
                        int izq = (i > 0) ? (f->x - (out[prim+i-1].x + out[prim+i-1].w)) : 1 << 30;
                        int der = (i < cnt - 1) ? (out[prim+i+1].x - (f->x + f->w)) : 1 << 30;
                        int j = (izq <= der) ? i - 1 : i + 1;
                        H2Rect *o2 = &out[prim + j];
                        int nx = f->x < o2->x ? f->x : o2->x;
                        int ny = f->y < o2->y ? f->y : o2->y;
                        int nx2 = (f->x + f->w > o2->x + o2->w) ? f->x + f->w : o2->x + o2->w;
                        int ny2 = (f->y + f->h > o2->y + o2->h) ? f->y + f->h : o2->y + o2->h;
                        o2->ay += o2->y - ny;      /* el ancla sigue en la linea del suelo */
                        o2->x = nx; o2->y = ny; o2->w = nx2 - nx; o2->h = ny2 - ny;
                        o2->ax = o2->w / 2;
                        memmove(f, f + 1, sizeof(H2Rect) * (size_t)(n - prim - i - 1));
                        n--; cnt--;
                        if (j < i) i = j; /* volver a mirar el que ha crecido */
                    }
                }
            }
        }
        if (n > prim) banda++;
    }

    /* Los rips traen los creditos escritos ("Keifer / Ripped by Fret / ..."): unas
       cuantas bandas AL FINAL, de muchos trozos diminutos al lado de los
       personajes. Se quitan solo las de la cola y mientras sigan cumpliendolo, que
       una banda suelta de objetos pequenos en medio de la hoja si es buena. */
    if (limpiar && n > 0) {
        int i, nb = 0, ref, *alt = (int *)malloc(sizeof(int) * (size_t)n);
        for (i = 0; i < n; i++) if (out[i].band + 1 > nb) nb = out[i].band + 1;
        if (alt && nb > 1) {
            for (i = 0; i < n; i++) alt[i] = out[i].h;
            qsort(alt, n, sizeof(int), cmp_int);
            ref = alt[n * 9 / 10];          /* lo alto que es un personaje de esta hoja */
            for (int b = nb - 1; b >= 1; b--) {
                int cnt = 0, altos = 0, k;
                for (i = 0; i < n; i++)
                    if (out[i].band == b) { cnt++; if (out[i].h * 5 >= ref * 2) altos++; }
                if (cnt < 5 || altos > 0) break;        /* esta banda ya es buena: parar */
                k = 0;
                for (i = 0; i < n; i++)
                    if (out[i].band != b) out[k++] = out[i];
                n = k;
            }
        }
        free(alt);
    }

    free(fila);
    free(vacio);
    free(px);
    return n;
}

/* Escribe una copia de la hoja con el FONDO EN TRANSPARENTE. El fondo se busca
   igual que al recortar (alfa, relleno desde los bordes y colores que ocupan
   media hoja), pero SIN tocar las lineas de rejilla: esas quedan fuera de los
   recortes de todos modos, y borrarlas podria comerse parte de un dibujo.
   Devuelve 1 si habia fondo opaco que quitar, 0 si la hoja ya venia con alfa
   (no hace nada) y -1 si no se pudo. */
int h2_make_transparent(const char *src, const char *dst) {
    unsigned char *px, *vacio;
    int w, h, x, y;
    size_t transp = 0, quitados = 0, total;
    SDL_Surface *sup;

    if (!src || !dst) return -1;
    px = load_rgba(src, &w, &h);
    if (!px) return -1;
    total = (size_t)w * h;

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (px[((size_t)y * w + x) * 4 + 3] < 8) transp++;
    if (transp * 20 >= total) { free(px); return 0; }   /* ya tiene transparencia */

    vacio = mascara_vacio(px, w, h, 0);
    if (!vacio) { free(px); return -1; }
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            size_t p = (size_t)y * w + x;
            if (!vacio[p]) continue;
            px[p * 4 + 3] = 0;            /* fondo -> transparente */
            quitados++;
        }
    free(vacio);
    if (!quitados) { free(px); return 0; }

    sup = SDL_CreateRGBSurfaceFrom(px, w, h, 32, w * 4,
                                   0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (!sup) { free(px); return -1; }
    if (IMG_SavePNG(sup, dst) != 0) { SDL_FreeSurface(sup); free(px); return -1; }
    SDL_FreeSurface(sup);
    free(px);
    return 1;
}

int h2_guess_grid(const H2Rect *frames, int n, int sheet_w, int sheet_h,
                  int *cols, int *rows) {
    int i, bandas = 0, maxfila = 0, encolumna = 0, ultimo_y = -1000000;
    if (!frames || n <= 0 || sheet_w <= 0 || sheet_h <= 0) return 0;

    /* Los fotogramas vienen por bandas (el detector recorre banda a banda, y
       dentro de cada una de izquierda a derecha). Las columnas NO se pueden
       sacar del ancho recortado -- un muneco de 13 px en una celda de 32 daria
       el doble de columnas --, asi que se cuentan los fotogramas de cada banda. */
    for (i = 0; i < n; i++) {
        int nueva = (i == 0) || (frames[i].y > ultimo_y + frames[i].h / 2) ||
                    (frames[i].x <= frames[i - 1].x);
        if (nueva) {
            if (encolumna > maxfila) maxfila = encolumna;
            encolumna = 0;
            bandas++;
            ultimo_y = frames[i].y;
        }
        encolumna++;
    }
    if (encolumna > maxfila) maxfila = encolumna;
    if (bandas < 1 || maxfila < 1) return 0;
    /* Rejilla de verdad = todas las bandas con el mismo numero de fotogramas. */
    if (bandas * maxfila != n) return 0;
    if (cols) *cols = maxfila;
    if (rows) *rows = bandas;
    return 1;
}

/* ===========================================================================
   Un FPG como hoja de sprites
   =========================================================================== */

int h2_fpg_to_sheet(const char *fpg_path, const char *png_out,
                    H2Rect *out, int max, int *sheet_w, int *sheet_h) {
    H2File f;
    char header[8];
    int bpp, n = 0, i;
    unsigned char pal[256][3];
    /* Cada grafico: sus pixeles, su tamano y su punto de control 0. */
    struct { unsigned char *px; int w, h, cx, cy; } *g = NULL;
    int cap = 0, aw = 0, ah = 0, x = 0, y = 0, rowh = 0, maxw = 1;
    unsigned char *atlas;
    SDL_Surface *sup;

    if (!fpg_path || !png_out || !out || max <= 0) return 0;
    memset(pal, 0, sizeof(pal));

    f = h2_open(fpg_path);
    if (!f) return 0;
    if (!h2_read(f, header, 8)) { h2_close(f); return 0; }
    if      (!memcmp(header, "f32\x1A\x0D\x0A\x00", 7)) bpp = 32;
    else if (!memcmp(header, "f16\x1A\x0D\x0A\x00", 7)) bpp = 16;
    else if (!memcmp(header, "fpg\x1A\x0D\x0A\x00", 7)) bpp = 8;
    else { h2_close(f); return 0; }
    if (bpp == 8 && !read_palette(f, pal)) { h2_close(f); return 0; }

    for (;;) {
        int code, regsize, w, h, flags, k;
        char name[32], fpname[12];
        unsigned char *rgba;
        int cx = -1, cy = -1;

        if (!rd_i32(f, &code) || !rd_i32(f, &regsize)) break;
        if (!h2_read(f, name, sizeof(name)) || !h2_read(f, fpname, sizeof(fpname))) break;
        if (!rd_i32(f, &w) || !rd_i32(f, &h) || !rd_i32(f, &flags)) break;
        if (code < 0 || code > 999) break;

        /* Puntos de control: el 0 es el centro del grafico (en un FPG de
           personajes suele estar en los pies, que es justo el ancla que quiere
           un sprite plantado en el suelo). */
        for (k = 0; k < flags; k++) {
            unsigned char b[4];
            int px2, py2;
            if (!h2_read(f, b, 4)) { flags = k; break; }
            px2 = (short)((unsigned)b[0] | ((unsigned)b[1] << 8));
            py2 = (short)((unsigned)b[2] | ((unsigned)b[3] << 8));
            if (k == 0 && px2 != -1 && py2 != -1) { cx = px2; cy = py2; }
        }

        rgba = read_bitmap_rgba(f, w, h, bpp, pal);
        if (!rgba) break;
        if (n == cap) {
            void *ng;
            cap = cap ? cap * 2 : 32;
            ng = realloc(g, (size_t)cap * sizeof(*g));
            if (!ng) { free(rgba); break; }
            g = ng;
        }
        g[n].px = rgba; g[n].w = w; g[n].h = h; g[n].cx = cx; g[n].cy = cy;
        if (w > maxw) maxw = w;
        n++;
        if (n >= max) break;
    }
    h2_close(f);
    if (!n) { free(g); return 0; }

    /* Colocacion en estantes, con un pixel de margen para que no se pisen. El
       ancho sale del area total: con uno fijo, un FPG grande daba una hoja
       estrechisima y larguisima (1024 x 9000), que es tirar memoria de video. */
    {
        double area = 0.0;
        for (i = 0; i < n; i++) area += (double)(g[i].w + 1) * (g[i].h + 1);
        aw = 64;
        while ((double)aw * aw < area * 1.15 && aw < 4096) aw <<= 1;
        while (aw < maxw + 2 && aw < 8192) aw <<= 1;
    }
    for (i = 0; i < n; i++) {
        if (x + g[i].w + 1 > aw) { x = 0; y += rowh + 1; rowh = 0; }
        out[i].x = x; out[i].y = y; out[i].w = g[i].w; out[i].h = g[i].h;
        out[i].ax = (g[i].cx >= 0) ? g[i].cx : g[i].w / 2;
        out[i].ay = (g[i].cy >= 0) ? g[i].cy : g[i].h - 1;
        out[i].band = 0;
        x += g[i].w + 1;
        if (g[i].h > rowh) rowh = g[i].h;
    }
    ah = y + rowh + 1;
    atlas = (unsigned char *)calloc((size_t)aw * ah, 4);
    if (!atlas) { for (i = 0; i < n; i++) free(g[i].px); free(g); return 0; }
    for (i = 0; i < n; i++) {
        int yy;
        for (yy = 0; yy < g[i].h; yy++)
            memcpy(atlas + ((size_t)(out[i].y + yy) * aw + out[i].x) * 4,
                   g[i].px + (size_t)yy * g[i].w * 4, (size_t)g[i].w * 4);
        free(g[i].px);
    }
    free(g);

    sup = SDL_CreateRGBSurfaceFrom(atlas, aw, ah, 32, aw * 4,
                                   0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (!sup) { free(atlas); return 0; }
    if (IMG_SavePNG(sup, png_out) != 0) { SDL_FreeSurface(sup); free(atlas); return 0; }
    SDL_FreeSurface(sup);
    free(atlas);

    if (sheet_w) *sheet_w = aw;
    if (sheet_h) *sheet_h = ah;
    return n;
}
