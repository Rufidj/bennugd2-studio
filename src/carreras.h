/*
 * carreras.h - modo CARRERAS de editor3d: circuitos de karts.
 *
 * Un circuito es una escena mas del proyecto (Scenes/<nombre>.kart, texto):
 *   - la LINEA CENTRAL cerrada (puntos de control: altura, ancho, peralte y
 *     muros a cada lado), que se suaviza con Catmull-Rom;
 *   - la SALIDA (el rumbo y la altura salen de la pista);
 *   - RAMPAS sobre la carretera (cada una lleva su trampolin en la cresta);
 *   - las TEXTURAS de cada parte (carretera, cesped, muros...) y su escala;
 *   - OBJETOS (modelos del proyecto) con giro, tamano y colision;
 *   - el kart (modelo y tamano).
 *
 * Al guardar se escribe tambien Assets/Models/<nombre>_pista.glb. Las texturas
 * van como ficheros externos (las de serie, de runtime/carreras, se copian a
 * Assets/Textures/carreras). El juego lo genera main.cpp como
 * <escena>_montar() -> kr_montar() de runtime/carreras.prg.
 *
 * Convenciones (las del motor): Y arriba, metros; yaw 0 mira a +Z; mirando a
 * +Z, +X queda a la IZQUIERDA. En la vista cenital +Z va hacia arriba y +X hacia
 * la izquierda, asi que izquierda/derecha se ven como las ve el piloto.
 * Ninguna superficie por debajo de y = 0 (el char controller tiene un suelo
 * invisible ahi).
 */
#pragma once

#include "imgui.h"
#include "hud2d.h"             // h2_load_thumb: miniaturas de las texturas
#include <map>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace kr {
namespace fs = std::filesystem;

// --------------------------------------------------------------------------- vectores
struct V3 { float x = 0, y = 0, z = 0; };
static inline V3 v3(float x, float y, float z) { V3 v; v.x = x; v.y = y; v.z = z; return v; }
static inline V3 operator+(V3 a, V3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V3 operator-(V3 a, V3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V3 operator*(V3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static inline float len(V3 a) { return std::sqrt(dot(a, a)); }
static inline V3 norm(V3 a) { float l = len(a); return l > 1e-9f ? a * (1.0f / l) : a; }
static const float KPI = 3.14159265f;

// --------------------------------------------------------------------------- materiales
// asfalto, tierra e hielo van seguidos: la superficie de un punto indexa directamente (MT_ASFALTO + sup)
enum { MT_ASFALTO, MT_TIERRA, MT_HIELO, MT_BARRO, MT_CESPED, MT_MURO, MT_BASE, MT_META, MT_RAMPA, MT_TURBO, MT_FONDO, MT_N };
static const char* MT_NOMBRE[MT_N] = {"asfalto", "tierra", "hielo", "barro", "cesped", "muro", "base", "meta", "rampa", "turbo", "fondo"};
static const char* MT_ETIQ[MT_N]   = {"Asfalto", "Tierra", "Hielo", "Barro", "Cesped", "Muros", "Laterales", "Meta", "Rampas", "Turbo", "Lagos"};
static const float MT_ESC[MT_N]    = {8, 8, 8, 8, 20, 4, 4, 1, 4, 4, 20};   // metros por repeticion de la textura
static const int N_SUP = 4;                // asfalto, tierra, hielo, barro: la superficie de un tramo
static const char* SUP_NOMBRE[N_SUP] = {"Asfalto", "Tierra", "Hielo", "Barro"};
// Efecto de una superficie o de una zona pintada en el terreno: multiplicadores de agarre, velocidad maxima y aceleracion
struct Efecto { float agarre = 1, vel = 1, acel = 1; };
static const Efecto EF_SUP_DEF[N_SUP] = {{1, 1, 1}, {0.65f, 0.9f, 1}, {0.12f, 1, 0.6f}, {0.5f, 0.6f, 0.7f}};
static const char* ZONA_NOMBRE[4]  = {"Zona 1 (roja)", "Zona 2 (verde)", "Zona 3 (azul)", "Zona 4 (amarilla)"};
static const Efecto EF_ZONA_DEF[4] = {{0.5f, 0.6f, 0.7f}, {1, 1.35f, 1.6f}, {0.12f, 1, 0.6f}, {0.8f, 0.5f, 0.7f}};
static const char* ZONA_DEF_NOMBRE[4] = {"barro", "turbo", "hielo", "arena lenta"};
static const int SPS = 40;   // muestras de la linea central por tramo entre dos puntos de control

// (uri_codificar ya no se usa: el motor no decodifica las rutas, ver exportar)
static inline std::string uri_codificar(const std::string& s) {
    std::string o; char b[4];
    for (unsigned char c : s) {
        if (isalnum(c) || strchr("/._-~", c)) o += (char)c;
        else { snprintf(b, sizeof b, "%%%02X", c); o += b; }
    }
    return o;
}

// --------------------------------------------------------------------------- datos
// Un CAMINO es una lista de NODOS unidos por TRAMOS. El tramo i va del nodo i al i+1 (el ultimo
// vuelve al primero si el camino es cerrado). Altura, ancho y peralte viven en los nodos y se
// interpolan a lo largo de cada tramo; el tipo, la superficie y los muros son de cada tramo.
struct Nodo {
    float x = 0, z = 0, y = 0, ancho = 14, peralte = 0;
    float tx = 0, tz = 0;            // tangente (derivada del Bezier); 0,0 = automatica
    int   sigue = 0;                 // 1 = la carretera sigue al terreno (altura = terreno + dy); 0 = altura fija (y)
    float dy = 0.3f;                 // metros sobre el terreno cuando sigue
};
// Relieve de una escena leido de su .terrain (la Y de cada vertice, rejilla cuadrada centrada en el origen)
struct TerrenoH {
    std::vector<float> h; int lado = 0; float tam = 400.0f;
    bool cargar(const std::string& ruta, float tamano = 400.0f) {
        FILE* f = fopen(ruta.c_str(), "rb"); if (!f) return false;
        uint32_t n = 0;
        if (fread(&n, 4, 1, f) != 1 || n < 4 || n > 4000000) { fclose(f); return false; }
        h.resize(n); size_t got = fread(h.data(), 4, n, f); fclose(f);
        if (got != n) return false;
        lado = (int)(std::sqrt((float)n) + 0.5f); if ((uint32_t)lado * lado != n) return false;
        tam = tamano; return true;
    }
    float en(float x, float z) const {
        if (lado < 2) return 0;
        float gx = (x / tam + 0.5f) * (lado - 1), gz = (z / tam + 0.5f) * (lado - 1);
        gx = std::clamp(gx, 0.0f, (float)(lado - 1)); gz = std::clamp(gz, 0.0f, (float)(lado - 1));
        int ix = (int)gx, iz = (int)gz, ix1 = std::min(ix + 1, lado - 1), iz1 = std::min(iz + 1, lado - 1);
        float fx = gx - ix, fz = gz - iz;
        float a = h[iz * lado + ix], b = h[iz * lado + ix1], c = h[iz1 * lado + ix], d = h[iz1 * lado + ix1];
        return (a + (b - a) * fx) * (1 - fz) + (c + (d - c) * fx) * fz;
    }
};
enum { T_RECTO, T_CURVA, T_SALTO };
static const char* TIPO_NOMBRE[3] = {"Recta", "Curva", "Salto (la carretera se corta)"};
struct Tramo {
    int   tipo = T_CURVA, sup = 0, muro_izq = 0, muro_der = 1, suave = 1;   // suave: perfil de altura/peralte/ancho suave o lineal
    float fuerza = 20;               // salto: fuerza del lanzamiento
};
struct Camino {
    std::string nombre = "camino";
    std::vector<Nodo> nodos;
    std::vector<Tramo> tramos;       // siempre tantos como nodos (en un camino abierto el ultimo no se usa)
    bool cerrado = true, principal = false;   // principal: el que lleva las vueltas
    int  ntr() const { int n = (int)nodos.size(); return cerrado ? (n >= 3 ? n : 0) : std::max(0, n - 1); }
    void sincronizar() { tramos.resize(nodos.size()); }
};
struct Rampa { float x = 0, z = 0, ancho = 7, largo = 22, alto = 2.4f, fuerza = 15; };
struct Objeto {
    std::string modelo;               // ruta dentro del proyecto (Assets/...)
    float x = 0, z = 0, y = 0;        // y = altura EXTRA sobre el suelo o la carretera
    float yaw = 0, alto = 0;          // giro en grados; alto en metros (0 = tamano original)
    int   colision = 1;               // 0 ninguna, 1 caja, 2 malla exacta (sin giro)
    float base = 0;                   // derivado: suelo debajo (carretera o 0)
};
struct Turbo { float x = 0, z = 0, largo = 12, ancho = 6; };
// luz puntual (farola, foco): y = altura; noche = solo se enciende de noche; poste = se dibuja un poste con su bombilla
struct Luz { float x = 0, z = 0, y = 6, r = 1.0f, g = 0.85f, b = 0.6f, inten = 2.5f, radio = 28; int noche = 1, poste = 1; };
static const char* AMB_NOMBRE[4] = {"Como la escena vinculada", "Dia", "Noche", "Ciclo dia / noche"};
struct Lago  { float cx = 0, cz = 0, w = 40, h = 30, prof = 3; int mortal = 0; };
struct Tramp { float x0, y0, z0, x1, y1, z1, fuerza; };
// Un CRUCE: dos carreteras de distinta altura que se superponen. El usuario elige puente (0) o tunel (1).
struct Cruce { float x = 0, z = 0; int tipo = 0; };
static const char* CRUCE_NOMBRE[2] = {"Puente (abierto por debajo, con pilares)", "Tunel (laterales cerrados, boca forrada)"};
struct CruceDet {                                   // derivado: el cruce tal como esta ahora
    float x = 0, z = 0, dy = 0; int tipo = 0;
    std::vector<int> sup, inf;                      // muestras de la carretera de arriba y de la de abajo
};
struct Cp    { float x, y, z, yaw, hw; int sup; };
// una RUTA alternativa para la IA: un camino abierto que sale del principal en el checkpoint 'e' y vuelve a el en el 'x'
struct Ruta  { int ini = 0, n = 0, e = 0, x = 0; };
// Un PILOTO: quien conduce y con que kart. Cada rival del circuito tiene el suyo (tambien el jugador tiene sus estadisticas).
// Las estadisticas son multiplicadores sobre el kart de serie (1 = igual); la personalidad decide como conduce la IA.
struct Piloto {
    std::string nombre = "Piloto";
    std::string modelo;                // "" = el kart del jugador
    float tam = 0;                     // alto del kart en metros (0 = el del jugador)
    float skill = 0.92f;               // 0.6..1.1: que fraccion de la velocidad maxima aprovecha (habilidad)
    float agr = 0.4f;                  // 0..1: agresividad (adelanta con decision, bloquea, no frena por el de delante)
    float var = 0.5f;                  // 0..1: cuanto cambia de carril y de trazada
    bool ia_propia = false;            // true = lo conduce kr_ia_usuario (modulo del proyecto) en vez de la IA de serie
    float vel = 1, acel = 1, giro = 1, agarre = 1, peso = 1;   // kart: velocidad maxima, aceleracion, giro, agarre, peso
};
// una pieza de la pista propia que NO colisiona (centro de la pieza en el espacio del modelo y su numero de triangulos: la identifican)
struct SinCol { float cx = 0, cy = 0, cz = 0; int ntri = 0; };

struct Muestra {
    V3 c, izq_borde, der_borde, t, izq;   // centro, bordes, tangente, lateral (+X = izquierda)
    float dist = 0, ancho = 14, yb = 0, peralte = 0, fuerza = 0;
    int muro_izq = 0, muro_der = 1, sup = 0, hueco = 0;
    int camino = 0, tramo = 0, tipo = T_CURVA;
};
struct Strip { int ini = 0, n = 0; bool cerrado = true; float largo = 0; };   // las muestras de un camino

struct Circuito {
    std::vector<Camino> caminos;
    std::vector<Rampa> rampas;
    std::vector<Objeto> objetos;
    std::vector<Turbo> turbos;
    std::vector<Lago> lagos;
    std::vector<Cruce> cruces;                 // lo elegido (se guarda); se rehace con los cruces detectados
    std::vector<Luz> luces;
    int   amb_modo = 1;                        // 0 como la escena vinculada, 1 dia, 2 noche, 3 ciclo
    float amb_hora = 9.0f;                     // hora de salida del ciclo (0..24)
    float amb_dur = 240.0f;                    // segundos que dura un dia entero en el ciclo
    int   faros = 1;                           // faros del kart (se encienden de noche)
    Efecto sup_ef[N_SUP];                      // agarre/velocidad/aceleracion de cada superficie
    Efecto zona_ef[4]; int zona_on[4] = {0, 0, 0, 0};   // zonas pintadas en el terreno (capas 1-4) y su efecto
    std::string zona_nom[4];
    int vueltas = 3;
    std::string escena;                        // escena 3D con el terreno y los objetos (ruta relativa: Scenes/x.scene); "" = cesped plano
    std::string tex[MT_N];                     // "" = la de serie
    float esc[MT_N];
    float sx = 0, sz = -40;                    // salida (y y rumbo los da la pista)
    std::string kart = "Assets/Models/kart.glb";
    std::string pista_glb;                     // pista propia (.glb de Blender...): se ve y colisiona ella, no se genera carretera. "" = carretera generada
    float sy_manual = 1.0f, syaw_manual = 0;   // salida a mano (altura y rumbo) cuando no hay trazado
    bool sin_terreno = false;                  // con pista propia: no se ve el terreno de la escena vinculada en el juego
    std::vector<SinCol> sincol;                // piezas de la pista propia que se quitan de la colision
    std::string ex_pcol;                       // modelo de colision generado (sin esas piezas); "" = se usa la pista entera
    // lo pone el editor: escribe el modelo de colision sin las piezas quitadas
    float col_alto = 0;                        // colision de la pista propia: se descartan las caras empinadas de menos de estos metros de alto (bordillos, cantos)
    bool  col_sin_techos = false;              // ... y las que miran hacia abajo (techos, bajos de puentes)
    std::function<bool(const std::string&, const std::string&, const std::vector<SinCol>&, float, float, const std::string&, std::string&)> hook_col;
    std::string ex_vis, ex_col;                // con pista propia: modelos extra generados (turbos y meta / rampas con colision); "" = ninguno
    float pista_x = 0, pista_y = 0, pista_z = 0, pista_esc = 1.0f, pista_yaw = 0;   // colocacion de la pista propia: posicion, escala y giro (grados)
    float kart_tam = 1.3f;
    std::vector<Piloto> pilotos;               // los rivales con IA (0..7)
    Piloto jugador;                            // el piloto del jugador (solo cuentan nombre y estadisticas)
    float ia_ancho = 1.0f;                     // cuanto del ancho del trazado usan los rivales (>1 = mas que el trazado)
    float equilibrio = 0.25f;                  // 0..1: los rivales rezagados aprietan y los que van muy por delante aflojan
    float margen = 60;                         // cesped alrededor (y limites del respawn)

    Circuito() { for (int k = 0; k < MT_N; k++) esc[k] = MT_ESC[k]; efectos_defecto(); jugador.nombre = "Tu"; }
    void efectos_defecto() {
        for (int i = 0; i < N_SUP; i++) sup_ef[i] = EF_SUP_DEF[i];
        for (int i = 0; i < 4; i++) { zona_ef[i] = EF_ZONA_DEF[i]; zona_on[i] = 0; zona_nom[i] = ZONA_DEF_NOMBRE[i]; }
    }

    // Altura del terreno en (x,z): si esta puesta, los nodos con 'sigue' la usan (no se guarda)
    std::function<float(float, float)> terreno_h;
    float pend_max = 0.40f;                     // pendiente maxima (subida/bajada) de la carretera que sigue al terreno
    void usar_terreno_fichero(const std::string& ruta, float tamano = 400.0f) {
        auto t = std::make_shared<TerrenoH>();
        if (t->cargar(ruta, tamano)) terreno_h = [t](float x, float z) { return t->en(x, z); }; else terreno_h = nullptr;
    }

    // ---- derivados (calcular) ----
    std::vector<Muestra> m;
    std::vector<Strip> strips;                 // uno por camino (n = 0 si no tiene muestras)
    int   ps = -1;                             // camino principal (el de las vueltas); -1 = ninguno valido
    float vuelta = 0;
    float gx0 = 0, gx1 = 0, gz0 = 0, gz1 = 0;  // cesped = limites
    float salida_y = 1, salida_yaw = 0;
    int   salida_i = 0, meta_i = 0;
    float paso = 6;
    std::vector<Tramp> tramps;      // rampas + lanzadores de los saltos
    std::vector<Tramp> turbo_z;     // zonas de las placas de turbo (fuerza sin uso)
    std::vector<Cp> cps;            // checkpoints del camino principal cada 'paso' m, desde la meta
    std::vector<Cp> alt;            // puntos de los demas caminos (atajos), para saber que se esta sobre carretera
    std::vector<Ruta> rutas;        // los caminos alternativos que enlazan con el principal (la IA elige entre ellos)
    std::vector<std::string> avisos_ruta;
    std::vector<CruceDet> cruces_det;
    std::vector<int> m_cdet;                    // por muestra: indice del cruce en el que esta la carretera de ARRIBA (-1 = ninguno)
    std::vector<std::string> avisos_cruce;      // cruces con poca altura libre

    // Mueve TODO el circuito (nodos, objetos, luces, turbos, lagos, rampas, cruces, salida)
    void desplazar(float dx, float dz) {
        for (auto& c : caminos) for (auto& n : c.nodos) { n.x += dx; n.z += dz; }
        for (auto& o : rampas) { o.x += dx; o.z += dz; }
        for (auto& o : objetos) { o.x += dx; o.z += dz; }
        for (auto& o : turbos) { o.x += dx; o.z += dz; }
        for (auto& o : lagos) { o.cx += dx; o.cz += dz; }
        for (auto& o : cruces) { o.x += dx; o.z += dz; }
        for (auto& o : luces) { o.x += dx; o.z += dz; }
        sx += dx; sz += dz;
    }
    // Limites de los nodos; false si no hay
    bool limites_nodos(float& x0, float& x1, float& z0, float& z1) const {
        bool hay = false;
        for (auto& c : caminos) for (auto& n : c.nodos) {
            if (!hay) { x0 = x1 = n.x; z0 = z1 = n.z; hay = true; }
            x0 = std::min(x0, n.x); x1 = std::max(x1, n.x); z0 = std::min(z0, n.z); z1 = std::max(z1, n.z);
        }
        return hay;
    }

    void ejemplo() {
        // ovalo con curva peraltada, loma y S estrecha
        caminos.clear(); rampas.clear(); objetos.clear(); turbos.clear(); lagos.clear();
        Camino c; c.nombre = "principal"; c.principal = true; c.cerrado = true;
        const float P[][7] = {
            {0, 0, -80, 0, 14, 0, 1}, {0, 0, 0, 0, 14, 0, 1}, {0, 0, 80, 0, 14, 0, 1},
            {30, 0, 130, 12, 14, 0, 1}, {90, 0, 148, 16, 14, 0, 1}, {140, 0, 110, 12, 14, 0, 1},
            {160, 6, 50, 0, 13, 0, 1}, {160, 10, -10, 0, 13, 0, 1}, {150, 2, -70, 0, 12, 1, 1},
            {110, 0, -115, -5, 9, 1, 1}, {65, 0, -105, 5, 9, 1, 1}, {35, 0, -145, -5, 9, 1, 1},
            {5, 0, -125, 0, 14, 0, 1}};
        for (auto& p : P) {
            Nodo n; n.x = p[0]; n.y = p[1]; n.z = p[2]; n.peralte = p[3]; n.ancho = p[4]; c.nodos.push_back(n);
            Tramo t; t.muro_izq = (int)p[5]; t.muro_der = (int)p[6]; c.tramos.push_back(t);
        }
        caminos.push_back(c);
        Rampa r; r.x = 0; r.z = 21; rampas.push_back(r);
        Turbo t; t.x = 0; t.z = -10; turbos.push_back(t);
        sx = 0; sz = -40;
    }

    // ------------------------------------------------------------- .kart
    bool load(const std::string& ruta) {
        std::ifstream in(ruta); if (!in) return false;
        caminos.clear(); rampas.clear(); objetos.clear(); turbos.clear(); lagos.clear(); cruces.clear(); luces.clear(); vueltas = 3; efectos_defecto(); pilotos.clear(); jugador = Piloto(); jugador.nombre = "Tu"; equilibrio = 0.25f; ia_ancho = 1.0f; pista_glb.clear(); sincol.clear(); col_alto = 0; col_sin_techos = false; sin_terreno = false; sy_manual = 1.0f; syaw_manual = 0; pista_x = pista_y = pista_z = 0; pista_esc = 1.0f; pista_yaw = 0;
        bool hubo_amb = false; amb_modo = 1; amb_hora = 9.0f; amb_dur = 240.0f; faros = 1;
        for (int k = 0; k < MT_N; k++) { tex[k].clear(); esc[k] = MT_ESC[k]; }
        std::vector<std::pair<Nodo, Tramo>> viejos;       // formato antiguo (punto=): una sola linea cerrada
        std::string ln;
        auto resto = [](std::istringstream& s) { std::string r; std::getline(s >> std::ws, r); return r; };
        while (std::getline(in, ln)) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            if (ln.empty() || ln[0] == '#') continue;
            size_t eq = ln.find('='); if (eq == std::string::npos) continue;
            std::string k = ln.substr(0, eq), v = ln.substr(eq + 1);
            std::istringstream s(v);
            if (k == "camino") { Camino c; int ce = 1, pr = 0; s >> c.nombre >> ce >> pr; c.cerrado = ce != 0; c.principal = pr != 0; caminos.push_back(c); }
            else if (k == "nodo") {
                if (caminos.empty()) caminos.push_back(Camino());
                Nodo n; Tramo t;
                s >> n.x >> n.z >> n.y >> n.ancho >> n.peralte >> n.tx >> n.tz >> t.tipo >> t.sup >> t.muro_izq >> t.muro_der >> t.fuerza >> t.suave;
                { int sg = 0; float dd = 0.15f; if (s >> sg >> dd) { n.sigue = sg != 0; n.dy = dd; } }
                t.tipo = std::clamp(t.tipo, 0, 2); t.sup = std::clamp(t.sup, 0, N_SUP - 1);
                caminos.back().nodos.push_back(n); caminos.back().tramos.push_back(t);
            }
            else if (k == "punto") {
                Nodo n; Tramo t; float salto = 0;
                s >> n.x >> n.y >> n.z >> n.peralte >> n.ancho >> t.muro_izq >> t.muro_der >> t.sup >> salto;
                t.sup = std::clamp(t.sup, 0, N_SUP - 1);
                if (salto > 0) { t.tipo = T_SALTO; t.fuerza = salto; }
                viejos.push_back({n, t});
            }
            else if (k == "rampa") { Rampa r; s >> r.x >> r.z >> r.ancho >> r.largo >> r.alto >> r.fuerza; rampas.push_back(r); }
            else if (k == "turbo") { Turbo t; s >> t.x >> t.z >> t.largo >> t.ancho; turbos.push_back(t); }
            else if (k == "lago") { Lago l; s >> l.cx >> l.cz >> l.w >> l.h >> l.prof >> l.mortal; lagos.push_back(l); }
            else if (k == "vueltas") s >> vueltas;
            else if (k == "efecto") { int i = 0; Efecto e; if (s >> i >> e.agarre >> e.vel >> e.acel && i >= 0 && i < N_SUP) sup_ef[i] = e; }
            else if (k == "zona") { int i = 0, on = 0; Efecto e; if (s >> i >> on >> e.agarre >> e.vel >> e.acel && i >= 0 && i < 4) { zona_ef[i] = e; zona_on[i] = on != 0; zona_nom[i] = resto(s); } }
            else if (k == "ambiente") { s >> amb_modo >> amb_hora >> amb_dur >> faros; amb_modo = std::clamp(amb_modo, 0, 3); hubo_amb = true; }
            else if (k == "luz") { Luz l; s >> l.x >> l.z >> l.y >> l.r >> l.g >> l.b >> l.inten >> l.radio >> l.noche >> l.poste; luces.push_back(l); }
            else if (k == "cruce") { Cruce cr; s >> cr.x >> cr.z >> cr.tipo; cr.tipo = std::clamp(cr.tipo, 0, 1); cruces.push_back(cr); }
            else if (k == "escena") escena = v;
            else if (k == "objeto") { Objeto o; s >> o.x >> o.z >> o.y >> o.yaw >> o.alto >> o.colision; o.modelo = resto(s); if (!o.modelo.empty()) objetos.push_back(o); }
            else if (k == "textura") {
                std::string slot; float e = 0; s >> slot >> e; std::string r = resto(s);
                for (int i = 0; i < MT_N; i++) if (slot == MT_NOMBRE[i]) { if (e > 0) esc[i] = e; tex[i] = (r == "-") ? "" : r; }
            }
            else if (k == "salida") s >> sx >> sz;
            else if (k == "kart") kart = v;
            else if (k == "pista") pista_glb = v;
            else if (k == "salida3d") s >> sy_manual >> syaw_manual;
            else if (k == "sin_terreno") { int v = 0; s >> v; sin_terreno = v != 0; }
            else if (k == "colision") { int t = 0; s >> col_alto >> t; col_sin_techos = t != 0; }
            else if (k == "sincol") { SinCol sc; if (s >> sc.cx >> sc.cy >> sc.cz >> sc.ntri) sincol.push_back(sc); }
            else if (k == "pista3d") s >> pista_x >> pista_y >> pista_z >> pista_esc >> pista_yaw;
            else if (k == "kart_tam") s >> kart_tam;
            else if (k == "rivales") {            // formato antiguo: n dificultad modelo -> se convierte en pilotos
                int n = 0, d = 1; s >> n >> d; std::string md = resto(s); n = std::clamp(n, 0, 7);
                if (pilotos.empty()) for (int q = 0; q < n; q++) { Piloto p; p.nombre = "Rival " + std::to_string(q + 1); p.modelo = md; p.skill = d <= 0 ? 0.82f : d == 1 ? 0.93f : 1.0f; pilotos.push_back(p); }
            }
            else if (k == "piloto") {
                Piloto p; s >> p.skill >> p.agr >> p.var >> p.vel >> p.acel >> p.giro >> p.agarre >> p.peso >> p.tam;
                std::string rs = resto(s); size_t b = rs.find('|'); if (b != std::string::npos) { p.nombre = rs.substr(0, b); p.modelo = rs.substr(b + 1); size_t b2 = p.modelo.find('|'); if (b2 != std::string::npos) { p.ia_propia = p.modelo.substr(b2 + 1) == "1"; p.modelo.resize(b2); } } else p.nombre = rs;
                while (!p.nombre.empty() && p.nombre.back() == ' ') p.nombre.pop_back(); while (!p.nombre.empty() && p.nombre.front() == ' ') p.nombre.erase(0, 1);
                if (pilotos.size() < 7) pilotos.push_back(p);
            }
            else if (k == "jugador") { s >> jugador.vel >> jugador.acel >> jugador.giro >> jugador.agarre >> jugador.peso; std::string rs = resto(s); size_t b = rs.find('|'); jugador.nombre = b == std::string::npos ? rs : rs.substr(b + 1); if (jugador.nombre.empty()) jugador.nombre = "Tu"; }
            else if (k == "equilibrio") s >> equilibrio;
            else if (k == "ia_ancho") s >> ia_ancho;
            else if (k == "margen") s >> margen;
        }
        if (caminos.empty() && !viejos.empty()) {
            Camino c; c.nombre = "principal"; c.principal = true; c.cerrado = true;
            for (auto& pr : viejos) { c.nodos.push_back(pr.first); c.tramos.push_back(pr.second); }
            caminos.push_back(c);
        }
        if (!hubo_amb) amb_modo = escena.empty() ? 1 : 0;                  // sin ambiente guardado: el de la escena si la hay
        bool hay_pr = false;
        for (auto& c : caminos) { c.sincronizar(); if (c.principal) { if (hay_pr) c.principal = false; hay_pr = true; } }
        if (!hay_pr) for (auto& c : caminos) if (c.cerrado && c.nodos.size() >= 3) { c.principal = true; break; }
        size_t tot = 0; for (auto& c : caminos) tot += c.nodos.size();
        return tot >= 2 || !pista_glb.empty();      // una pista propia puede no tener aun ningun punto
    }
    bool save(const std::string& ruta) const {
        FILE* f = fopen(ruta.c_str(), "w"); if (!f) return false;
        fputs("# circuito de karts (editor3d, modo Carreras)\n", f);
        if (!pista_glb.empty()) fprintf(f, "# pista propia: este .glb se ve y colisiona, no se genera carretera\npista=%s\nsalida3d=%.3f %.3f\npista3d=%.3f %.3f %.3f %.4f %.2f\nsin_terreno=%d\n", pista_glb.c_str(), sy_manual, syaw_manual, pista_x, pista_y, pista_z, pista_esc, pista_yaw, sin_terreno ? 1 : 0);
        if (!pista_glb.empty()) fprintf(f, "colision=%.3f %d\n", col_alto, col_sin_techos ? 1 : 0);
        for (auto& sc : sincol) fprintf(f, "sincol=%.4f %.4f %.4f %d\n", sc.cx, sc.cy, sc.cz, sc.ntri);
        fputs("# piloto=habilidad agresividad variacion vel acel giro agarre peso tamano | nombre | modelo   (un rival; las estadisticas son multiplicadores)\n", f);
        for (auto& p : pilotos) fprintf(f, "piloto=%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.2f %s|%s%s\n", p.skill, p.agr, p.var, p.vel, p.acel, p.giro, p.agarre, p.peso, p.tam, p.nombre.c_str(), p.modelo.c_str(), p.ia_propia ? "|1" : "");
        fprintf(f, "jugador=%.3f %.3f %.3f %.3f %.3f |%s\nequilibrio=%.3f\n", jugador.vel, jugador.acel, jugador.giro, jugador.agarre, jugador.peso, jugador.nombre.c_str(), equilibrio);
        fprintf(f, "ia_ancho=%.3f\n", ia_ancho);
        fprintf(f, "kart=%s\nkart_tam=%.3f\nmargen=%.1f\nsalida=%.3f %.3f\nvueltas=%d\n", kart.c_str(), kart_tam, margen, sx, sz, vueltas);
        if (!escena.empty()) fprintf(f, "# escena 3D con el terreno y los objetos (se edita en los modos Terreno y Escena)\nescena=%s\n", escena.c_str());
        fputs("# efecto=superficie(0 asfalto,1 tierra,2 hielo,3 barro) agarre velocidad aceleracion   (multiplicadores)\n", f);
        for (int i = 0; i < N_SUP; i++) fprintf(f, "efecto=%d %.3f %.3f %.3f\n", i, sup_ef[i].agarre, sup_ef[i].vel, sup_ef[i].acel);
        fputs("# zona=capa(0..3, la que se pinta en el terreno) activa(0/1) agarre velocidad aceleracion nombre\n", f);
        for (int i = 0; i < 4; i++) fprintf(f, "zona=%d %d %.3f %.3f %.3f %s\n", i, zona_on[i], zona_ef[i].agarre, zona_ef[i].vel, zona_ef[i].acel, zona_nom[i].empty() ? "zona" : zona_nom[i].c_str());
        fputs("# textura=parte metros_por_repeticion ruta (- = la de serie)\n", f);
        for (int k = 0; k < MT_N; k++) fprintf(f, "textura=%s %.2f %s\n", MT_NOMBRE[k], esc[k], tex[k].empty() ? "-" : tex[k].c_str());
        fputs("# camino=nombre cerrado(0/1) principal(0/1)\n"
              "# nodo=x z altura ancho peralte tangenteX tangenteZ | tramo que sale de este nodo: tipo(0 recta,1 curva,2 salto) superficie(0 asfalto,1 tierra,2 hielo) muroIzq muroDer fuerzaSalto suave(0/1) | sigueTerreno(0/1) metrosSobreElTerreno\n", f);
        for (auto& c : caminos) {
            std::string nm = c.nombre.empty() ? "camino" : c.nombre; for (auto& ch : nm) if (isspace((unsigned char)ch)) ch = '_';
            fprintf(f, "camino=%s %d %d\n", nm.c_str(), c.cerrado ? 1 : 0, c.principal ? 1 : 0);
            for (size_t i = 0; i < c.nodos.size(); i++) {
                const Nodo& n = c.nodos[i]; const Tramo& t = c.tramos[i];
                fprintf(f, "nodo=%.3f %.3f %.3f %.2f %.2f %.3f %.3f %d %d %d %d %.1f %d %d %.2f\n", n.x, n.z, n.y, n.ancho, n.peralte, n.tx, n.tz,
                        t.tipo, t.sup, t.muro_izq, t.muro_der, t.fuerza, t.suave, n.sigue, n.dy);
            }
        }
        fputs("# rampa=x z ancho largo alto fuerza\n", f);
        for (auto& r : rampas) fprintf(f, "rampa=%.3f %.3f %.2f %.2f %.2f %.2f\n", r.x, r.z, r.ancho, r.largo, r.alto, r.fuerza);
        fputs("# turbo=x z largo ancho   (placa sobre la carretera)\n", f);
        for (auto& t : turbos) fprintf(f, "turbo=%.3f %.3f %.2f %.2f\n", t.x, t.z, t.largo, t.ancho);
        fputs("# lago=centro_x centro_z ancho largo profundidad profundo(0 no, 1 reaparece)\n", f);
        for (auto& l : lagos) fprintf(f, "lago=%.3f %.3f %.2f %.2f %.2f %d\n", l.cx, l.cz, l.w, l.h, l.prof, l.mortal);
        fputs("# ambiente=modo(0 como la escena, 1 dia, 2 noche, 3 ciclo) hora_de_salida duracion_del_dia_en_segundos faros_del_kart(0/1)\n", f);
        fprintf(f, "ambiente=%d %.2f %.1f %d\n", amb_modo, amb_hora, amb_dur, faros);
        fputs("# luz=x z altura r g b intensidad radio solo_de_noche(0/1) poste(0/1)\n", f);
        for (auto& l : luces) fprintf(f, "luz=%.2f %.2f %.2f %.3f %.3f %.3f %.2f %.1f %d %d\n", l.x, l.z, l.y, l.r, l.g, l.b, l.inten, l.radio, l.noche, l.poste);
        fputs("# cruce=x z tipo(0 puente, 1 tunel)   (donde dos carreteras de distinta altura se cruzan)\n", f);
        for (auto& cr : cruces) fprintf(f, "cruce=%.2f %.2f %d\n", cr.x, cr.z, cr.tipo);
        fputs("# objeto=x z altura_extra giro alto colision(0 no, 1 caja, 2 malla) modelo\n", f);
        for (auto& o : objetos) fprintf(f, "objeto=%.3f %.3f %.2f %.1f %.2f %d %s\n", o.x, o.z, o.y, o.yaw, o.alto, o.colision, o.modelo.c_str());
        fclose(f); return true;
    }

    // ------------------------------------------------------------- muestras
    int sig(int i) const {       // muestra siguiente en su camino (-1 si es el final de uno abierto)
        const Strip& s = strips[m[i].camino]; int k = i - s.ini;
        return k + 1 < s.n ? i + 1 : (s.cerrado ? s.ini : -1);
    }
    int ant(int i) const {
        const Strip& s = strips[m[i].camino]; int k = i - s.ini;
        return k > 0 ? i - 1 : (s.cerrado ? s.ini + s.n - 1 : -1);
    }
    // la muestra de carretera mas cercana (no cuenta los saltos); solo = camino concreto o -1 para todos
    int muestra_cercana(float x, float z, int solo = -1) const {
        int best = -1; float bd = 1e30f;
        for (size_t i = 0; i < m.size(); i++) {
            if (m[i].hueco || (solo >= 0 && m[i].camino != solo)) continue;
            float dx = m[i].c.x - x, dz = m[i].c.z - z, d = dx * dx + dz * dz;
            if (d < bd) { bd = d; best = (int)i; }
        }
        if (best < 0) best = 0;
        return best;
    }
    static float yaw_de(V3 t) { return std::atan2(t.x, t.z) * 180.0f / KPI; }   // adelante = (sin yaw, cos yaw)

    // altura del suelo en (x,z): la carretera si esta encima, si no el cesped (0)
    float suelo_en(float x, float z) const {
        if (m.empty()) return 0;
        const Muestra& s = m[muestra_cercana(x, z)];
        float dx = s.c.x - x, dz = s.c.z - z;
        return (dx * dx + dz * dz <= (s.ancho * 0.5f + 0.5f) * (s.ancho * 0.5f + 0.5f)) ? s.c.y : 0.0f;
    }

    // tangente de un nodo: la suya, o la automatica (Catmull-Rom) segun sus vecinos
    static void tangente(const Camino& c, int i, float& tx, float& tz) {
        const Nodo& a = c.nodos[i];
        if (a.tx != 0 || a.tz != 0) { tx = a.tx; tz = a.tz; return; }
        int n = (int)c.nodos.size(), ip = i - 1, in = i + 1;
        if (c.cerrado) { ip = (i - 1 + n) % n; in = (i + 1) % n; }
        bool hp = ip >= 0, hn = in < n;
        const Nodo& p = c.nodos[hp ? ip : i]; const Nodo& q = c.nodos[hn ? in : i];
        float k = (hp && hn) ? 0.5f : 1.0f;
        tx = (q.x - p.x) * k; tz = (q.z - p.z) * k;
    }

    void calcular() {
        m.clear(); strips.clear(); tramps.clear(); turbo_z.clear(); cps.clear(); alt.clear(); vuelta = 0; ps = -1;
        const V3 UP = v3(0, 1, 0);
        struct Raw { float x, y, z, b, w; int mi, md, sup, tipo, tramo; float fuerza; int sg; float dy; };
        for (size_t ci = 0; ci < caminos.size(); ci++) {
            const Camino& c = caminos[ci];
            Strip st; st.ini = (int)m.size(); st.cerrado = c.cerrado && c.nodos.size() >= 3;
            std::vector<Raw> raw;
            int n = (int)c.nodos.size(), ntr = c.ntr();
            for (int k = 0; k < ntr; k++) {
                const Nodo &A = c.nodos[k], &B = c.nodos[(k + 1) % n]; const Tramo& T = c.tramos[k];
                float tax, taz, tbx, tbz; tangente(c, k, tax, taz); tangente(c, (k + 1) % n, tbx, tbz);
                float chord = std::hypot(B.x - A.x, B.z - A.z);
                bool curva = T.tipo == T_CURVA;
                float L = chord * (curva ? 1.2f : 1.0f);
                int ns = std::max(2, (int)std::ceil(L / (T.tipo == T_SALTO ? 4.0f : 2.0f)));
                int last = (!st.cerrado && k == ntr - 1) ? ns : ns - 1;      // un camino abierto acaba en su ultimo nodo
                for (int q = 0; q <= last; q++) {
                    float t = (float)q / ns, u = T.suave ? t * t * (3 - 2 * t) : t;
                    Raw r; r.sg = 0; r.dy = 0;
                    if (curva) {
                        float h00 = 2 * t * t * t - 3 * t * t + 1, h10 = t * t * t - 2 * t * t + t, h01 = -2 * t * t * t + 3 * t * t, h11 = t * t * t - t * t;
                        r.x = h00 * A.x + h10 * tax + h01 * B.x + h11 * tbx;
                        r.z = h00 * A.z + h10 * taz + h01 * B.z + h11 * tbz;
                    } else { r.x = A.x + (B.x - A.x) * t; r.z = A.z + (B.z - A.z) * t; }
                    r.y = std::max(0.0f, A.y + (B.y - A.y) * u);
                    if (terreno_h && A.sigue && B.sigue) { r.sg = 1; r.dy = A.dy + (B.dy - A.dy) * u; }
                    r.b = A.peralte + (B.peralte - A.peralte) * u;
                    r.w = std::max(4.0f, A.ancho + (B.ancho - A.ancho) * u);
                    r.mi = T.muro_izq; r.md = T.muro_der; r.sup = std::clamp(T.sup, 0, N_SUP - 1); r.tipo = T.tipo; r.tramo = k;
                    r.fuerza = T.tipo == T_SALTO ? T.fuerza : 0;
                    if (q == ns) { r.tipo = T_CURVA; r.fuerza = 0; }          // la muestra final de un camino abierto no es un salto
                    raw.push_back(r);
                }
            }
            int N = (int)raw.size(); st.n = N;
            if (N < 2) { st.n = 0; strips.push_back(st); continue; }
            // la carretera pegada al terreno: la mas alta de las tres alturas del ancho (centro y bordes), suavizada a lo largo
            if (terreno_h) {
                std::vector<float> hm(N, 0.0f);
                for (int i = 0; i < N; i++) {
                    if (!raw[i].sg) continue;
                    int ip = st.cerrado ? (i - 1 + N) % N : std::max(0, i - 1), in = st.cerrado ? (i + 1) % N : std::min(N - 1, i + 1);
                    float tx = raw[in].x - raw[ip].x, tz = raw[in].z - raw[ip].z, l = std::hypot(tx, tz); if (l < 1e-4f) l = 1;
                    float lx = tz / l, lz = -tx / l, w = raw[i].w * 0.5f;
                    float fx_ = tx / l, fz_ = tz / l, mx = -1e30f;                // el relieve de debajo de TODA la huella de la carretera: que no asome por el asfalto
                    for (int la = -2; la <= 2; la++) for (int lo = -1; lo <= 1; lo++)
                        mx = std::max(mx, terreno_h(raw[i].x + lx * w * 0.5f * la + fx_ * 1.25f * lo, raw[i].z + lz * w * 0.5f * la + fz_ * 1.25f * lo));
                    hm[i] = mx;
                }
                for (int i = 0; i < N; i++) {
                    if (!raw[i].sg) continue;
                    float acc = 0; int cnt = 0;
                    for (int q = -3; q <= 3; q++) {
                        int j = st.cerrado ? ((i + q) % N + N) % N : i + q; if (j < 0 || j >= N || !raw[j].sg) continue;
                        acc += hm[j]; cnt++;
                    }
                    raw[i].y = std::max(0.0f, std::max(acc / std::max(1, cnt), hm[i]) + raw[i].dy);
                }
                // pendiente maxima: la carretera nunca sube ni baja mas de 'pend_max' (si no, el kart no puede con ella);
                // solo se SUBE lo bajo (los valles se rellenan y la carretera queda a la altura de las cumbres), asi nunca queda enterrada
                auto ds = [&](int i, int j) { return std::max(0.5f, std::hypot(raw[j].x - raw[i].x, raw[j].z - raw[i].z)); };
                for (int it = 0; it < 3; it++) {
                    for (int q = 0; q < (st.cerrado ? 2 * N : N - 1); q++) {          // hacia delante
                        int i = q % N, j = (i + 1) % N; if (!st.cerrado && j == 0) break;
                        if (raw[i].sg && raw[j].sg) raw[j].y = std::max(raw[j].y, raw[i].y - pend_max * ds(i, j));
                    }
                    for (int q = 0; q < (st.cerrado ? 2 * N : N - 1); q++) {          // hacia atras
                        int i = ((N - 1 - q) % N + N) % N, j = (i - 1 + N) % N; if (!st.cerrado && i == 0) break;
                        if (raw[i].sg && raw[j].sg) raw[j].y = std::max(raw[j].y, raw[i].y - pend_max * ds(i, j));
                    }
                }
            }
            for (int i = 0; i < N; i++) {
                int ip = i - 1, in = i + 1;
                if (st.cerrado) { ip = (i - 1 + N) % N; in = (i + 1) % N; } else { ip = std::max(0, ip); in = std::min(N - 1, in); }
                const Raw &p = raw[ip], &q = raw[in], &r = raw[i];
                V3 t = norm(v3(q.x - p.x, q.y - p.y, q.z - p.z));
                V3 izq = norm(v3(t.z, 0, -t.x));                       // +X con rumbo +Z = izquierda del piloto
                float b = r.b * KPI / 180.0f, w = r.w * 0.5f;
                Muestra s;
                s.c = v3(r.x, r.y + std::fabs(std::sin(b)) * w, r.z);   // el borde bajo no se hunde
                s.t = t; s.izq = izq; s.ancho = r.w; s.muro_izq = r.mi; s.muro_der = r.md; s.sup = r.sup;
                s.hueco = r.tipo == T_SALTO ? 1 : 0; s.fuerza = r.fuerza; s.tipo = r.tipo; s.tramo = r.tramo;
                s.camino = (int)ci; s.yb = r.y; s.peralte = r.b;
                // peralte + = sube el borde DERECHO (curvas a la izquierda)
                s.der_borde = s.c - izq * (std::cos(b) * w) + UP * (std::sin(b) * w);
                s.izq_borde = s.c + izq * (std::cos(b) * w) - UP * (std::sin(b) * w);
                s.dist = i == 0 ? 0 : m.back().dist + len(s.c - m.back().c);
                m.push_back(s);
            }
            st.largo = st.cerrado ? m.back().dist + len(m[st.ini].c - m.back().c) : m.back().dist;
            strips.push_back(st);
        }
        // camino principal: el marcado (cerrado) o el primer cerrado
        for (size_t ci = 0; ci < caminos.size(); ci++) if (caminos[ci].principal && strips[ci].n >= 3 && strips[ci].cerrado) { ps = (int)ci; break; }
        if (ps < 0) for (size_t ci = 0; ci < caminos.size(); ci++) if (strips[ci].n >= 3 && strips[ci].cerrado) { ps = (int)ci; break; }
        if (m.empty()) { if (!pista_glb.empty()) { salida_y = sy_manual; salida_yaw = syaw_manual; gx0 = gz0 = -3000.0f; gx1 = gz1 = 3000.0f; } return; }   // sin trazado: limites muy amplios, para que no cuente como 'fuera del mapa'
        vuelta = ps >= 0 ? strips[ps].largo : 0;

        // cesped / limites
        gx0 = gz0 = 1e30f; gx1 = gz1 = -1e30f;
        for (auto& s : m) for (V3 e : {s.izq_borde, s.der_borde}) {
            gx0 = std::min(gx0, e.x); gx1 = std::max(gx1, e.x); gz0 = std::min(gz0, e.z); gz1 = std::max(gz1, e.z);
        }
        gx0 -= margen; gx1 += margen; gz0 -= margen; gz1 += margen;

        // salida: la muestra mas cercana del camino principal
        if (ps >= 0) {
            float bd = 1e30f; salida_i = strips[ps].ini;
            for (int i = strips[ps].ini; i < strips[ps].ini + strips[ps].n; i++) {
                if (m[i].hueco) continue;
                float dx = m[i].c.x - sx, dz = m[i].c.z - sz, d = dx * dx + dz * dz; if (d < bd) { bd = d; salida_i = i; }
            }
        } else salida_i = 0;
        salida_y = m[salida_i].c.y + 1.0f;
        salida_yaw = yaw_de(m[salida_i].t);

        // trampolines en la cresta de cada rampa
        for (auto& rp : rampas) {
            int si = muestra_cercana(rp.x, rp.z);
            V3 f = norm(v3(m[si].t.x, 0, m[si].t.z)), l = v3(f.z, 0, -f.x);
            float base = m[si].c.y;
            V3 o = v3(rp.x, base, rp.z) - f * (rp.largo * 0.5f);
            float s0 = rp.largo * 0.45f, s1 = rp.largo * 0.68f, h = rp.ancho * 0.5f;
            Tramp tr; tr.x0 = tr.z0 = 1e30f; tr.x1 = tr.z1 = -1e30f;
            for (float s : {s0, s1}) for (float lat : {-h, h}) {
                V3 p = o + f * s + l * lat;
                tr.x0 = std::min(tr.x0, p.x); tr.x1 = std::max(tr.x1, p.x);
                tr.z0 = std::min(tr.z0, p.z); tr.z1 = std::max(tr.z1, p.z);
            }
            tr.y0 = base + rp.alto * 0.6f; tr.y1 = base + rp.alto + 1.6f; tr.fuerza = rp.fuerza;
            tramps.push_back(tr);
        }
        // ---- cruces: dos carreteras que se superponen con mas de 4 m de diferencia de altura ----
        detectar_cruces();

        // lanzadores de los saltos: los ultimos 8 m de carretera antes del corte
        for (int i = 0; i < (int)m.size(); i++) {
            int a = ant(i);
            if (!m[i].hueco || a < 0 || m[a].hueco) continue;
            Tramp tr; tr.x0 = tr.z0 = 1e30f; tr.x1 = tr.z1 = -1e30f;
            float y = m[i].c.y;
            for (int q = i, g = 0; q >= 0 && g < 60 && !(q != i && m[i].dist - m[q].dist > 8.0f); q = ant(q), g++) {
                if (m[q].hueco && q != i) break;
                for (V3 e : {m[q].izq_borde, m[q].der_borde}) {
                    tr.x0 = std::min(tr.x0, e.x); tr.x1 = std::max(tr.x1, e.x);
                    tr.z0 = std::min(tr.z0, e.z); tr.z1 = std::max(tr.z1, e.z);
                }
            }
            tr.y0 = y - 2.5f; tr.y1 = y + 3.0f; tr.fuerza = m[i].fuerza;
            tramps.push_back(tr);
        }
        // placas de turbo: zona sobre los tramos de carretera que cubren
        for (auto& t : turbos) {
            int si = muestra_cercana(t.x, t.z);
            int h = std::max(1, (int)(t.largo * 0.5f / 2.0f));
            Tramp z; z.x0 = z.z0 = 1e30f; z.x1 = z.z1 = -1e30f; z.y0 = 1e30f; z.y1 = -1e30f; z.fuerza = 0;
            auto abarca = [&](int q) {
                for (V3 e : {m[q].izq_borde, m[q].der_borde}) {
                    z.x0 = std::min(z.x0, e.x); z.x1 = std::max(z.x1, e.x); z.z0 = std::min(z.z0, e.z); z.z1 = std::max(z.z1, e.z);
                }
                z.y0 = std::min(z.y0, m[q].c.y - 2.5f); z.y1 = std::max(z.y1, m[q].c.y + 3.0f);
            };
            abarca(si);
            for (int q = si, g = 0; g < h; g++) { q = sig(q); if (q < 0 || m[q].hueco) break; abarca(q); }
            for (int q = si, g = 0; g < h; g++) { q = ant(q); if (q < 0 || m[q].hueco) break; abarca(q); }
            turbo_z.push_back(z);
        }
        // checkpoints del camino principal, desde la linea de meta, cada 'paso' metros
        if (ps >= 0) {
            const Strip& S = strips[ps];
            meta_i = avanzar(salida_i, 10.0f);
            paso = std::max(6.0f, vuelta / 380.0f);
            float acc = paso;           // el primero cae justo en la meta
            for (int q = 0; q < S.n && (int)cps.size() < 399; q++) {
                int i = S.ini + ((meta_i - S.ini + q) % S.n), pi = S.ini + ((meta_i - S.ini + q - 1 + S.n) % S.n);
                if (q > 0) acc += len(m[i].c - m[pi].c);
                if (acc >= paso) {
                    acc = 0;
                    const Muestra& s = m[i];
                    Cp c; c.x = s.c.x; c.y = s.c.y; c.z = s.c.z; c.yaw = yaw_de(s.t); c.hw = s.ancho * 0.5f; c.sup = s.hueco ? -1 : s.sup;
                    cps.push_back(c);
                }
            }
        } else meta_i = 0;
        // los demas caminos (atajos): un punto cada ~6 m para reconocer la carretera
        rutas.clear(); avisos_ruta.clear();
        for (size_t ci = 0; ci < caminos.size(); ci++) {
            if ((int)ci == ps || strips[ci].n < 2) continue;
            float acc = 6.0f; size_t ini_alt = alt.size();
            for (int i = strips[ci].ini; i < strips[ci].ini + strips[ci].n && (int)alt.size() < 399; i++) {
                if (i > strips[ci].ini) acc += len(m[i].c - m[i - 1].c);
                if (acc < 6.0f) continue;
                acc = 0; const Muestra& s = m[i];
                Cp c; c.x = s.c.x; c.y = s.c.y; c.z = s.c.z; c.yaw = yaw_de(s.t); c.hw = s.ancho * 0.5f; c.sup = s.hueco ? -1 : s.sup;
                alt.push_back(c);
            }
            // ¿enlaza con el principal? Un camino ABIERTO cuyo primer y ultimo punto caen cerca del principal es una ruta alternativa
            int na = (int)(alt.size() - ini_alt);
            if (na >= 2 && ps >= 0 && !cps.empty() && !caminos[ci].cerrado) {
                auto cercano = [&](const Cp& p, float& d2) { int b = 0; d2 = 1e30f; for (int k = 0; k < (int)cps.size(); k++) { float dx = cps[k].x - p.x, dz = cps[k].z - p.z, dy = cps[k].y - p.y, d = dx*dx + dz*dz + dy*dy * 4.0f; if (d < d2) { d2 = d; b = k; } } return b; };
                float d0, d1; int e = cercano(alt[ini_alt], d0), x = cercano(alt[ini_alt + na - 1], d1);
                int N = (int)cps.size(), fw = ((x - e) % N + N) % N;
                if (std::sqrt(d0) > 30.0f || std::sqrt(d1) > 30.0f) avisos_ruta.push_back("el camino '" + caminos[ci].nombre + "' no empieza o no acaba cerca del principal: no es una ruta");
                else if (fw < 3 && N - fw < 3) avisos_ruta.push_back("el camino '" + caminos[ci].nombre + "' empieza y acaba en el mismo sitio del principal");
                else {
                    if (fw > N / 2) {                                    // va al reves: se da la vuelta (el principal tambien se recorre hacia delante)
                        std::reverse(alt.begin() + ini_alt, alt.end());
                        for (size_t q = ini_alt; q < alt.size(); q++) alt[q].yaw = std::fmod(alt[q].yaw + 180.0f, 360.0f);
                        std::swap(e, x);
                    }
                    Ruta rt; rt.ini = (int)ini_alt; rt.n = na; rt.e = e; rt.x = x;
                    if ((int)rutas.size() < 7) rutas.push_back(rt);
                }
            }
        }
        // objetos: suelo debajo
        for (auto& o : objetos) o.base = suelo_en(o.x, o.z);
    }

    // puntos para nivelar el terreno bajo la carretera: posicion, altura del borde mas bajo y radio de influencia
    struct Corte { float x, z, y, r; float cy, ry, ly, rx, rz, lx, lz; };
    std::vector<Corte> cortes() const {
        std::vector<Corte> v;
        for (auto& s : m) {
            if (s.hueco) continue;
            float bajo = std::min(s.der_borde.y, s.izq_borde.y) - 0.3f;      // el terreno queda siempre por debajo de la carretera
            v.push_back({s.c.x, s.c.z, bajo, s.ancho * 0.5f + 6.0f + 0.6f * s.c.y,
                         s.c.y, s.der_borde.y, s.izq_borde.y, s.der_borde.x, s.der_borde.z, s.izq_borde.x, s.izq_borde.z});
        }
        return v;
    }

    void detectar_cruces() {
        int N = (int)m.size();
        cruces_det.clear(); avisos_cruce.clear(); m_cdet.assign(N, -1);
        std::vector<char> marc(N, 0); std::vector<int> socio(N, -1); std::vector<float> sdy(N, 0.0f);
        bool poca = false; float px = 0, pz = 0;
        for (int i = 0; i < N; i++) {
            if (m[i].hueco) continue;
            for (int j = 0; j < N; j += 2) {
                if (i == j || m[j].hueco) continue;
                float dy = m[i].c.y - m[j].c.y; if (dy < 1.5f) continue;            // i = la de arriba
                float dx = m[i].c.x - m[j].c.x, dz = m[i].c.z - m[j].c.z, rr = (m[i].ancho + m[j].ancho) * 0.5f;
                if (dx * dx + dz * dz > rr * rr) continue;
                if (m[i].camino == m[j].camino) {                                    // la propia curva no es un cruce
                    const Strip& S = strips[m[i].camino]; float dd = std::fabs(m[i].dist - m[j].dist);
                    if (S.cerrado) dd = std::min(dd, S.largo - dd);
                    if (dd < 60.0f) continue;
                }
                if (dy < 4.0f) { poca = true; px = m[i].c.x; pz = m[i].c.z; continue; }
                marc[i] = 1; if (dy > sdy[i]) { sdy[i] = dy; socio[i] = j; }
            }
        }
        if (poca) { char b[160]; snprintf(b, sizeof b, "dos carreteras se cruzan cerca de (%.0f, %.0f) con menos de 4 m de altura libre: sube una al menos 5 m para que haya puente o tunel", px, pz); avisos_cruce.push_back(b); }
        // tramos seguidos de muestras marcadas = un cruce (se unen los huecos de hasta 3 muestras)
        for (size_t ci = 0; ci < strips.size(); ci++) {
            const Strip& S = strips[ci]; int q = 0;
            while (q < S.n) {
                if (!marc[S.ini + q]) { q++; continue; }
                int a = q, b = q;                                           // se extiende mientras haya otra marcada a menos de 4 muestras
                for (;;) {
                    int nx = -1;
                    for (int t = b + 1; t <= std::min(S.n - 1, b + 4); t++) if (marc[S.ini + t]) { nx = t; break; }
                    if (nx < 0) break;
                    b = nx;
                }
                CruceDet cd; float sx_ = 0, sz_ = 0; int cnt = 0; std::vector<char> usado(N, 0);
                int lo = std::max(0, a - 2), hi = std::min(S.n - 1, b + 2);         // un margen a cada lado
                for (int t = lo; t <= hi; t++) {
                    int i = S.ini + t; cd.sup.push_back(i); sx_ += m[i].c.x; sz_ += m[i].c.z; cnt++;
                    if (marc[i]) { cd.dy = std::max(cd.dy, sdy[i]); int j = socio[i]; if (j >= 0 && !usado[j]) { usado[j] = 1; cd.inf.push_back(j); } }
                }
                // la carretera de abajo: sus muestras solapadas y dos vecinas a cada lado
                std::vector<int> ext = cd.inf;
                for (int j : cd.inf) { int u = j, v = j; for (int g = 0; g < 3; g++) { u = ant(u); v = sig(v); if (u >= 0 && !usado[u]) { usado[u] = 1; ext.push_back(u); } if (v >= 0 && !usado[v]) { usado[v] = 1; ext.push_back(v); } } }
                cd.inf = ext;
                cd.x = sx_ / cnt; cd.z = sz_ / cnt; cd.tipo = 0;
                float bd = 20.0f * 20.0f;                                             // la eleccion guardada mas cercana
                for (auto& sv : cruces) { float dx = sv.x - cd.x, dz = sv.z - cd.z; if (dx * dx + dz * dz < bd) { bd = dx * dx + dz * dz; cd.tipo = sv.tipo; } }
                int id = (int)cruces_det.size();
                for (int i : cd.sup) m_cdet[i] = id;
                cruces_det.push_back(cd);
                q = hi + 1;
            }
        }
        cruces.clear();
        for (auto& cd : cruces_det) { Cruce cr; cr.x = cd.x; cr.z = cd.z; cr.tipo = cd.tipo; cruces.push_back(cr); }
    }
    // la carretera de abajo del cruce 'cd' pasa por (x, z)?  (la boca de un tunel)
    bool en_huella(int cd, V3 p) const {
        for (int q : cruces_det[cd].inf) { float dx = p.x - m[q].c.x, dz = p.z - m[q].c.z, r = m[q].ancho * 0.5f + 1.5f; if (dx * dx + dz * dz <= r * r) return true; }
        return false;
    }

    // indice de la muestra que esta d metros por delante de i (dentro de su camino)
    int avanzar(int i, float d) const {
        float acc = 0; int k = i;
        for (int q = 0; q < (int)m.size() && acc < d; q++) { int nx = sig(k); if (nx < 0) break; acc += len(m[nx].c - m[k].c); k = nx; }
        return k;
    }

    // peralte segun lo cerrada que es cada curva (curvas a la izquierda: positivo)
    void peralte_auto(int ci, float k = 18.0f, float maxp = 25.0f) {
        if (ci < 0 || ci >= (int)caminos.size()) return;
        Camino& c = caminos[ci]; int n = (int)c.nodos.size(); if (n < 3) return;
        for (int i = 0; i < n; i++) {
            if (!c.cerrado && (i == 0 || i == n - 1)) { c.nodos[i].peralte = 0; continue; }
            const Nodo &a = c.nodos[(i - 1 + n) % n], &b = c.nodos[i], &d = c.nodos[(i + 1) % n];
            V3 d0 = v3(b.x - a.x, 0, b.z - a.z), d1 = v3(d.x - b.x, 0, d.z - b.z);
            float l0 = len(d0), l1 = len(d1); if (l0 < 1e-3f || l1 < 1e-3f) continue;
            float dyaw = yaw_de(d1) - yaw_de(d0);
            while (dyaw > 180) dyaw -= 360; while (dyaw < -180) dyaw += 360;
            float p = std::clamp(dyaw / (0.5f * (l0 + l1)) * k, -maxp, maxp);
            c.nodos[i].peralte = std::fabs(p) < 2.0f ? 0.0f : p;
        }
    }

    // suaviza el peralte: cada punto pasa a la media con sus vecinos
    void peralte_suavizar(int ci) {
        if (ci < 0 || ci >= (int)caminos.size()) return;
        Camino& c = caminos[ci]; int n = (int)c.nodos.size(); if (n < 3) return;
        std::vector<float> p(n);
        for (int i = 0; i < n; i++) {
            int a = c.cerrado ? (i - 1 + n) % n : std::max(0, i - 1), d = c.cerrado ? (i + 1) % n : std::min(n - 1, i + 1);
            p[i] = 0.25f * c.nodos[a].peralte + 0.5f * c.nodos[i].peralte + 0.25f * c.nodos[d].peralte;
        }
        for (int i = 0; i < n; i++) c.nodos[i].peralte = p[i];
    }

    // avisos de diseno (no impiden guardar)
    std::string avisos() const {
        std::string a;
        if (!escena.empty() && !lagos.empty()) a += "con una escena 3D vinculada los lagos de este circuito se ignoran: pon el agua con las herramientas de terreno\n";
        for (auto& v : avisos_cruce) a += v + "\n";
        if (ps < 0) a += "no hay un camino principal cerrado (con 3 nodos o mas): no habra vueltas ni checkpoints\n";
        for (size_t li = 0; li < lagos.size(); li++) {
            const Lago& l = lagos[li];
            for (int i = 0; i < (int)m.size(); i += 3)
                if (std::fabs(m[i].c.x - l.cx) < l.w * 0.5f + m[i].ancho * 0.5f && std::fabs(m[i].c.z - l.cz) < l.h * 0.5f + m[i].ancho * 0.5f) {
                    a += "el lago " + std::to_string(li) + " toca la carretera (la cuenca se excava bajo ella)\n"; break;
                }
        }
        return a;
    }
    // alcance aproximado (m) de un salto: fuerza F, a 45 m/s, desnivel dy (final - inicial)
    static float alcance(float F, float dy) {
        float disc = F * F - 76.0f * dy; if (disc < 0) return -1;
        return 45.0f * (F + std::sqrt(disc)) / 38.0f;
    }

    // altura del terreno por los lagos (<= 0: la cuenca, con orillas suaves)
    float lago_h(float x, float z) const {
        float h = 0;
        for (auto& l : lagos) {
            float dx = std::min(x - (l.cx - l.w * 0.5f), (l.cx + l.w * 0.5f) - x);
            float dz = std::min(z - (l.cz - l.h * 0.5f), (l.cz + l.h * 0.5f) - z);
            float mm = std::min(dx, dz); if (mm <= 0) continue;
            float t = std::min(1.0f, mm / std::min(6.0f, std::min(l.w, l.h) * 0.4f)); t = t * t * (3 - 2 * t);
            h = std::min(h, -l.prof * t);
        }
        return h;
    }

    // ------------------------------------------------------------- malla + .glb
    struct Malla {
        std::string nombre, uri;       // uri de la textura, relativa al .glb
        std::vector<float> pos, nrm, uv; std::vector<uint32_t> idx;
        uint32_t vert(V3 p, V3 n, float u, float v) {
            pos.insert(pos.end(), {p.x, p.y, p.z}); nrm.insert(nrm.end(), {n.x, n.y, n.z}); uv.insert(uv.end(), {u, v});
            return (uint32_t)(pos.size() / 3 - 1);
        }
        void tri(uint32_t a, uint32_t b, uint32_t c, V3 quiero) {   // orienta la cara hacia 'quiero'
            V3 pa = v3(pos[a * 3], pos[a * 3 + 1], pos[a * 3 + 2]), pb = v3(pos[b * 3], pos[b * 3 + 1], pos[b * 3 + 2]),
               pc = v3(pos[c * 3], pos[c * 3 + 1], pos[c * 3 + 2]);
            if (dot(cross(pb - pa, pc - pa), quiero) < 0) std::swap(b, c);
            idx.insert(idx.end(), {a, b, c});
        }
        void quad(V3 p0, V3 p1, V3 p2, V3 p3, float u0, float v0, float u1, float v1, V3 n) {
            n = norm(n);
            uint32_t i0 = vert(p0, n, u0, v0), i1 = vert(p1, n, u0, v1), i2 = vert(p2, n, u1, v1), i3 = vert(p3, n, u1, v0);
            tri(i0, i1, i2, n); tri(i0, i2, i3, n);
        }
    };

    std::vector<Malla> construir() const {
        std::vector<Malla> M(MT_N);
        for (int k = 0; k < MT_N; k++) { M[k].nombre = MT_NOMBRE[k]; M[k].uri = std::string("../Textures/carreras/") + MT_NOMBRE[k] + ".png"; }
        Malla &CE = M[MT_CESPED], &MU = M[MT_MURO], &BA = M[MT_BASE], &ME = M[MT_META], &RA = M[MT_RAMPA], &FO = M[MT_FONDO];
        const float eC = esc[MT_CESPED], eM = esc[MT_MURO], eB = esc[MT_BASE], eR = esc[MT_RAMPA], eT = esc[MT_TURBO], eF = esc[MT_FONDO];
        const V3 UP = v3(0, 1, 0);
        const float G = -0.05f, WH = 1.2f, WT = 0.6f;
        int N = (int)m.size(); if (N < 2) return M;
        // carretera, laterales y muros, camino a camino
        for (int i = 0; i < N; i++) {
            int j = sig(i); if (j < 0) continue;                  // final de un camino abierto
            const Muestra &a = m[i], &b = m[j];
            if (a.hueco) continue;                                // en un salto no hay carretera
            const Strip& S = strips[a.camino];
            Malla& AS = M[MT_ASFALTO + std::clamp(a.sup, 0, N_SUP - 1)];
            const float eAs = esc[MT_ASFALTO + std::clamp(a.sup, 0, N_SUP - 1)];
            float d0 = a.dist, d1 = (j == S.ini) ? S.largo : b.dist;
            V3 nup = cross(b.c - a.c, a.izq_borde - a.der_borde);
            if (nup.y < 0) nup = nup * -1.0f;
            // asfalto: u = 0 en el borde derecho, 1 en el izquierdo
            AS.quad(a.der_borde, b.der_borde, b.izq_borde, a.izq_borde, 0, d0 / eAs, 1, d1 / eAs, nup);
            struct Borde { V3 e0, e1, fuera; int muro; } bordes[2] = {
                {a.izq_borde, b.izq_borde, a.izq, a.muro_izq}, {a.der_borde, b.der_borde, a.izq * -1.0f, a.muro_der}};
            int cdi = (i < (int)m_cdet.size() && m_cdet[i] >= 0) ? m_cdet[i] : (j < (int)m_cdet.size() ? m_cdet[j] : -1);
            const float TL = 1.0f;                                  // grueso de la losa de un cruce
            // losa por DEBAJO de toda carretera elevada: si no, desde abajo (por la boca de un tunel, o entre pilares) se veria
            // el reverso del asfalto, con sus rayas, en vez de una superficie solida
            bool elevada = std::max(std::max(a.der_borde.y, a.izq_borde.y), std::max(b.der_borde.y, b.izq_borde.y)) > 0.08f;
            if (cdi >= 0 || elevada) {
                V3 dn = v3(0, -TL, 0);
                BA.quad(a.der_borde + dn, b.der_borde + dn, b.izq_borde + dn, a.izq_borde + dn, 0, d0 / eB, 1, d1 / eB, v3(0, -1, 0));
            }
            for (auto& bd : bordes) {
                bool lateral = std::max(bd.e0.y, bd.e1.y) > 0.08f;
                if (cdi >= 0) {
                    BA.quad(bd.e0, bd.e1, bd.e1 + v3(0, -TL, 0), bd.e0 + v3(0, -TL, 0), 0, d0 / eB, TL / eB, d1 / eB, bd.fuera);   // canto de la losa
                    if (cruces_det[cdi].tipo == 0) lateral = false;                                                         // puente: abierto por debajo
                    else if (en_huella(cdi, (bd.e0 + bd.e1) * 0.5f)) lateral = false;                                        // tunel: solo la boca
                }
                if (lateral) {                                                      // la pared se ve por las dos caras (tambien desde dentro del hueco)
                    float hl = (std::max(bd.e0.y, bd.e1.y) - G) / eB;                // u = altura, v = largo: la textura no se estira
                    float g0 = G, g1 = G;                                          // con terreno, la pared baja solo hasta un poco por debajo de el
                    if (terreno_h) { g0 = std::min(bd.e0.y, std::max(G, terreno_h(bd.e0.x, bd.e0.z) - 1.5f)); g1 = std::min(bd.e1.y, std::max(G, terreno_h(bd.e1.x, bd.e1.z) - 1.5f)); }
                    BA.quad(bd.e0, bd.e1, v3(bd.e1.x, g1, bd.e1.z), v3(bd.e0.x, g0, bd.e0.z), 0, d0 / eB, hl, d1 / eB, bd.fuera);
                    BA.quad(bd.e0, bd.e1, v3(bd.e1.x, g1, bd.e1.z), v3(bd.e0.x, g0, bd.e0.z), 0, d0 / eB, hl, d1 / eB, bd.fuera * -1.0f);
                }
                if (!bd.muro) continue;
                V3 H = UP * WH, O0 = bd.e0 + bd.fuera * WT, O1 = bd.e1 + bd.fuera * WT;
                float w0 = d0 / eM, w1 = d1 / eM;
                MU.quad(bd.e0, bd.e1, bd.e1 + H, bd.e0 + H, 0, w0, 1, w1, bd.fuera * -1.0f);
                MU.quad(bd.e0 + H, bd.e1 + H, O1 + H, O0 + H, 0, w0, 1, w1, UP);
                MU.quad(O0, O1, O1 + H, O0 + H, 0, w0, 1, w1, bd.fuera);
            }
        }
        // puentes: dos pilares a los lados de la carretera de abajo; tuneles: la boca forrada con muros hasta el techo
        for (size_t ci = 0; ci < cruces_det.size(); ci++) {
            const CruceDet& cd = cruces_det[ci];
            if (cd.sup.empty() || cd.inf.empty()) continue;
            int iu = cd.sup[0]; float bdm = 1e30f;
            for (int u : cd.sup) { float dx = m[u].c.x - cd.x, dz = m[u].c.z - cd.z, d = dx * dx + dz * dz; if (d < bdm) { bdm = d; iu = u; } }
            int jl = cd.inf[0]; bdm = 1e30f;
            for (int q : cd.inf) { float dx = m[q].c.x - cd.x, dz = m[q].c.z - cd.z, d = dx * dx + dz * dz; if (d < bdm) { bdm = d; jl = q; } }
            const float TL = 1.0f;
            if (cd.tipo == 0) {
                float top = std::min(m[iu].der_borde.y, m[iu].izq_borde.y) - TL;
                for (int lado = -1; lado <= 1; lado += 2) {
                    V3 c0 = m[jl].c + m[jl].izq * (lado * (m[jl].ancho * 0.5f + 2.4f)); c0.y = G;
                    V3 f = norm(v3(m[jl].t.x, 0, m[jl].t.z)), l = v3(f.z, 0, -f.x); const float h = 1.1f;
                    V3 p[4] = {c0 - f * h - l * h, c0 + f * h - l * h, c0 + f * h + l * h, c0 - f * h + l * h};
                    for (int k = 0; k < 4; k++) {
                        V3 a = p[k], b = p[(k + 1) % 4], fuera = norm((a + b) * 0.5f - c0);
                        BA.quad(a, b, v3(b.x, top, b.z), v3(a.x, top, a.z), 0, 0, top / eB, 2.2f / eB, fuera);
                    }
                }
            } else {
                // tunel: dos muros MACIZOS a los lados de la carretera de abajo. Los dos llegan hasta la losa (misma altura)
                // y se recortan al ancho de la carretera de arriba, para que no sobresalgan del terraplen.
                const float TW = 1.0f;                                                     // grueso de los muros
                std::vector<int> ord = cd.inf; std::sort(ord.begin(), ord.end());
                auto sup_cerca = [&](V3 p) { int bb = cd.sup[0]; float b2 = 1e30f;
                    for (int u : cd.sup) { float dx = m[u].c.x - p.x, dz = m[u].c.z - p.z, d = dx * dx + dz * dz; if (d < b2) { b2 = d; bb = u; } } return bb; };
                auto lat_de = [&](V3 p, int u) { return (p.x - m[u].c.x) * m[u].izq.x + (p.z - m[u].c.z) * m[u].izq.z; };   // distancia lateral al eje de la de arriba
                auto recorta = [](float g0, float g1, float& t0, float& t1) {                // intervalo de t donde la recta g(t) >= 0
                    if (g0 < 0 && g1 < 0) { t0 = 1; t1 = 0; return; }
                    if (g0 >= 0 && g1 >= 0) return;
                    float rr = g0 / (g0 - g1);
                    if (g0 < 0) t0 = std::max(t0, rr); else t1 = std::min(t1, rr);
                };
                for (size_t k = 0; k + 1 < ord.size(); k++) {
                    int a = ord[k], b = ord[k + 1]; if (sig(a) != b || m[a].hueco) continue;
                    int ua = sup_cerca(m[a].c), ub = sup_cerca(m[b].c);
                    float hwa = m[ua].ancho * 0.5f, hwb = m[ub].ancho * 0.5f;
                    float techoA = std::min(m[ua].der_borde.y, m[ua].izq_borde.y) - TL;      // el techo es el mismo para los dos muros
                    float techoB = std::min(m[ub].der_borde.y, m[ub].izq_borde.y) - TL;
                    bool prev_ok = k > 0 && !m[ord[k - 1]].hueco && sig(ord[k - 1]) == a;
                    bool next_ok = k + 2 < ord.size() && sig(b) == ord[k + 2];
                    float seg = len(m[b].c - m[a].c);
                    for (int lado = 0; lado < 2; lado++) {
                        V3 e0 = lado ? m[a].der_borde : m[a].izq_borde, e1 = lado ? m[b].der_borde : m[b].izq_borde;
                        float la = lat_de(e0, ua), lb = lat_de(e1, ub), t0 = 0, t1 = 1;
                        recorta(hwa - la, hwb - lb, t0, t1); recorta(hwa + la, hwb + lb, t0, t1);        // |lateral| <= semiancho de la de arriba
                        if (t1 - t0 < 1e-3f) continue;
                        V3 fuera = lado ? m[a].izq * -1.0f : m[a].izq;                                  // alejandose de la carretera de abajo
                        // la cara de dentro se adelanta 6 cm hacia la carretera: asi tapa el muro del borde (que esta en el mismo plano) y no parpadea
                        V3 P0 = e0 + (e1 - e0) * t0 - fuera * 0.06f, P1 = e0 + (e1 - e0) * t1 - fuera * 0.06f;
                        float h0 = techoA + (techoB - techoA) * t0, h1 = techoA + (techoB - techoA) * t1;
                        if (h0 - P0.y < 2.0f || h1 - P1.y < 2.0f) continue;                              // no cabe un kart
                        V3 dir = norm(P1 - P0), o = fuera * (TW + 0.06f);                                // la cara de fuera queda donde estaba
                        V3 B0 = v3(P0.x, P0.y - 0.4f, P0.z), B1 = v3(P1.x, P1.y - 0.4f, P1.z), T0 = v3(P0.x, h0, P0.z), T1 = v3(P1.x, h1, P1.z);
                        float d0 = (m[a].dist + seg * t0) / eB, d1 = (m[a].dist + seg * t1) / eB, hh = (h0 - B0.y) / eB;
                        BA.quad(B0, B1, T1, T0, 0, d0, hh, d1, fuera * -1.0f);                           // cara de dentro (la que ve el kart)
                        BA.quad(B0 + o, B1 + o, T1 + o, T0 + o, 0, d0, hh, d1, fuera);                  // cara de fuera
                        BA.quad(T0, T1, T1 + o, T0 + o, 0, d0, 1, d1, v3(0, 1, 0));                     // arriba (bajo la losa)
                        if (t0 > 1e-3f || !prev_ok) BA.quad(B0, B0 + o, T0 + o, T0, 0, 0, 1, hh, dir * -1.0f);   // tapa del principio
                        if (t1 < 1.0f - 1e-3f || !next_ok) BA.quad(B1, B1 + o, T1 + o, T1, 0, 0, 1, hh, dir);    // tapa del final
                    }
                }
            }
        }
        // tapas donde la carretera se acaba: antes y despues de un salto y en los extremos de un camino abierto
        for (int i = 0; i < N; i++) {
            if (m[i].hueco && i != 0) { /* el salto empieza en el nodo: la carretera acaba en la muestra i */ }
            int a = ant(i), j = sig(i);
            bool fin_i = m[i].hueco && a >= 0 && !m[a].hueco;                 // aqui acaba la carretera (empieza un salto)
            bool ini_i = !m[i].hueco && (a < 0 || m[a].hueco);                // aqui empieza (despues de un salto o de un camino abierto)
            bool fin_abierto = !m[i].hueco && j < 0;                          // ultimo punto de un camino abierto
            if (!fin_i && !ini_i && !fin_abierto) continue;
            const Muestra& s = m[i];
            if (std::max(s.der_borde.y, s.izq_borde.y) <= 0.08f) continue;
            V3 dir = (fin_i || fin_abierto) ? s.t : s.t * -1.0f;
            BA.quad(s.der_borde, s.izq_borde, v3(s.izq_borde.x, G, s.izq_borde.z), v3(s.der_borde.x, G, s.der_borde.z), 0, 0, 1, 1, dir);
        }
        // placas de turbo: tiras pegadas a la carretera
        for (auto& t : turbos) {
            int si = muestra_cercana(t.x, t.z);
            int h = std::max(1, (int)(t.largo * 0.5f / 2.0f));
            std::vector<int> idx;                                          // las muestras de la placa, en orden
            { std::vector<int> atras; int q = si; for (int g = 0; g < h; g++) { q = ant(q); if (q < 0 || m[q].hueco) break; atras.push_back(q); }
              idx.assign(atras.rbegin(), atras.rend()); idx.push_back(si);
              q = si; for (int g = 0; g < h; g++) { q = sig(q); if (q < 0 || m[q].hueco) break; idx.push_back(q); } }
            for (size_t k = 0; k + 1 < idx.size(); k++) {
                const Muestra &a = m[idx[k]], &b = m[idx[k + 1]];
                float fa0 = 0.5f - t.ancho / (2 * a.ancho), fa1 = 0.5f + t.ancho / (2 * a.ancho);
                float fb0 = 0.5f - t.ancho / (2 * b.ancho), fb1 = 0.5f + t.ancho / (2 * b.ancho);
                auto pt = [&](const Muestra& s, float fr) { return s.der_borde + (s.izq_borde - s.der_borde) * fr + UP * 0.04f; };
                float v0 = (m[idx[k]].dist - m[idx[0]].dist) / eT, v1 = v0 + len(b.c - a.c) / eT;
                V3 nup = cross(b.c - a.c, a.izq_borde - a.der_borde); if (nup.y < 0) nup = nup * -1.0f;
                M[MT_TURBO].quad(pt(a, fa0), pt(b, fb0), pt(b, fb1), pt(a, fa1), 0, v0, 1, v1, nup);
            }
        }
        // cesped (con las cuencas de los lagos: una malla con alturas). Con una escena 3D vinculada
        // el suelo es su terreno: no se pone cesped (taparia el relieve con la colision)
        if (!escena.empty()) {
        } else if (lagos.empty()) {
            CE.quad(v3(gx0, G, gz0), v3(gx1, G, gz0), v3(gx1, G, gz1), v3(gx0, G, gz1), gx0 / eC, gz0 / eC, gx1 / eC, gz1 / eC, UP);
        } else {
            float area = (gx1 - gx0) * (gz1 - gz0), cell = std::max(4.0f, std::sqrt(area / 50000.0f));
            int nx = std::max(1, (int)std::ceil((gx1 - gx0) / cell)), nz = std::max(1, (int)std::ceil((gz1 - gz0) / cell));
            float cx = (gx1 - gx0) / nx, cz = (gz1 - gz0) / nz;
            auto alt_ = [&](float x, float z) { return G + lago_h(x, z); };
            auto nrm = [&](float x, float z) {
                const float e = 0.5f;
                return norm(v3(-(lago_h(x + e, z) - lago_h(x - e, z)) / (2 * e), 1.0f, -(lago_h(x, z + e) - lago_h(x, z - e)) / (2 * e)));
            };
            for (int iz = 0; iz < nz; iz++) for (int ix = 0; ix < nx; ix++) {
                float xa = gx0 + ix * cx, xb = xa + cx, za = gz0 + iz * cz, zb = za + cz;
                float hs[4] = {alt_(xa, za), alt_(xb, za), alt_(xb, zb), alt_(xa, zb)};
                bool pozo = hs[0] < -0.5f && hs[1] < -0.5f && hs[2] < -0.5f && hs[3] < -0.5f;
                Malla& T = pozo ? FO : CE; float es = pozo ? eF : eC;
                float px[4] = {xa, xb, xb, xa}, pz[4] = {za, za, zb, zb};
                uint32_t id[4];
                for (int c4 = 0; c4 < 4; c4++) id[c4] = T.vert(v3(px[c4], hs[c4], pz[c4]), nrm(px[c4], pz[c4]), px[c4] / es, pz[c4] / es);
                T.tri(id[0], id[1], id[2], UP); T.tri(id[0], id[2], id[3], UP);
            }
        }
        // meta: 10 m por delante de la salida (en el camino principal)
        if (ps >= 0) {
            const Muestra& s = m[meta_i];
            V3 f = norm(v3(s.t.x, 0, s.t.z)), l = v3(f.z, 0, -f.x), c = s.c + UP * 0.02f;
            float h = s.ancho * 0.5f;
            ME.quad(c - f - l * h, c + f - l * h, c + f + l * h, c - f + l * h, 0, 0, 1, 1, UP);
        }
        // rampas: sube en 0.55 del largo, meseta hasta 0.64, baja hasta el final
        for (auto& rp : rampas) {
            int si = muestra_cercana(rp.x, rp.z);
            V3 f = norm(v3(m[si].t.x, 0, m[si].t.z)), l = v3(f.z, 0, -f.x);
            float base = m[si].c.y, h = rp.ancho * 0.5f;
            V3 o = v3(rp.x, base, rp.z) - f * (rp.largo * 0.5f);
            const float perfil[4][2] = {{0.0f, 0.01f}, {0.55f, 1.0f}, {0.64f, 1.0f}, {1.0f, 0.01f}};
            V3 Lp[4], Rp[4]; float S[4];
            for (int k = 0; k < 4; k++) {
                float s = perfil[k][0] * rp.largo, y = perfil[k][1] <= 0.02f ? 0.01f : rp.alto;
                V3 c = o + f * s; c.y = base + y;
                Lp[k] = c + l * h; Rp[k] = c - l * h; S[k] = s;
            }
            for (int k = 0; k < 3; k++) {
                RA.quad(Rp[k], Rp[k + 1], Lp[k + 1], Lp[k], 0, S[k] / eR, 1, S[k + 1] / eR, UP);
                BA.quad(Lp[k], Lp[k + 1], v3(Lp[k + 1].x, base, Lp[k + 1].z), v3(Lp[k].x, base, Lp[k].z), S[k] / eB, 0, S[k + 1] / eB, 1, l);
                BA.quad(Rp[k], Rp[k + 1], v3(Rp[k + 1].x, base, Rp[k + 1].z), v3(Rp[k].x, base, Rp[k].z), S[k] / eB, 0, S[k + 1] / eB, 1, l * -1.0f);
            }
        }
        return M;
    }

    // Escribe el .glb (texturas como ficheros externos, por su uri).
    static bool escribir_glb(const std::vector<Malla>& M, const std::string& ruta, std::string& err) {
        std::vector<uint8_t> bin;
        auto alinear = [&]() { while (bin.size() % 4) bin.push_back(0); };
        auto pegar = [&](const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; bin.insert(bin.end(), b, b + n); };
        std::string views, accs, imgs, texs, mats, prims;
        int nv = 0, na = 0, nm = 0;
        auto coma = [](std::string& s) { if (!s.empty()) s += ","; };
        for (auto& m : M) {
            if (m.idx.empty()) continue;
            size_t nverts = m.pos.size() / 3;
            float mn[3] = {1e30f, 1e30f, 1e30f}, mx[3] = {-1e30f, -1e30f, -1e30f};
            for (size_t i = 0; i < nverts; i++) for (int k = 0; k < 3; k++) { mn[k] = std::min(mn[k], m.pos[i * 3 + k]); mx[k] = std::max(mx[k], m.pos[i * 3 + k]); }
            int acc[4];
            struct { const void* p; size_t n; int target; } partes[4] = {
                {m.pos.data(), m.pos.size() * 4, 34962}, {m.nrm.data(), m.nrm.size() * 4, 34962},
                {m.uv.data(), m.uv.size() * 4, 34962}, {m.idx.data(), m.idx.size() * 4, 34963}};
            for (int k = 0; k < 4; k++) {
                alinear(); size_t off = bin.size(); pegar(partes[k].p, partes[k].n);
                char b[200]; snprintf(b, sizeof b, "{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu,\"target\":%d}", off, partes[k].n, partes[k].target);
                coma(views); views += b;
                char a[400];
                if (k == 0) snprintf(a, sizeof a, "{\"bufferView\":%d,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC3\",\"min\":[%g,%g,%g],\"max\":[%g,%g,%g]}", nv, nverts, mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]);
                else if (k == 1) snprintf(a, sizeof a, "{\"bufferView\":%d,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC3\"}", nv, nverts);
                else if (k == 2) snprintf(a, sizeof a, "{\"bufferView\":%d,\"componentType\":5126,\"count\":%zu,\"type\":\"VEC2\"}", nv, nverts);
                else snprintf(a, sizeof a, "{\"bufferView\":%d,\"componentType\":5125,\"count\":%zu,\"type\":\"SCALAR\"}", nv, m.idx.size());
                coma(accs); accs += a; acc[k] = na++; nv++;
            }
            coma(imgs); imgs += "{\"uri\":\"" + m.uri + "\"}";
            coma(texs); texs += "{\"sampler\":0,\"source\":" + std::to_string(nm) + "}";
            coma(mats); mats += "{\"name\":\"" + m.nombre + "\",\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":" + std::to_string(nm) +
                                "},\"metallicFactor\":0,\"roughnessFactor\":0.9}}";
            char p[300]; snprintf(p, sizeof p, "{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":%d}", acc[0], acc[1], acc[2], acc[3], nm);
            coma(prims); prims += p; nm++;
        }
        alinear();
        std::string js = "{\"asset\":{\"version\":\"2.0\",\"generator\":\"editor3d modo Carreras\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],"
                         "\"nodes\":[{\"name\":\"pista\",\"mesh\":0}],\"meshes\":[{\"name\":\"pista\",\"primitives\":[" + prims + "]}],"
                         "\"materials\":[" + mats + "],\"textures\":[" + texs + "],\"images\":[" + imgs + "],"
                         "\"samplers\":[{\"magFilter\":9729,\"minFilter\":9987,\"wrapS\":10497,\"wrapT\":10497}],"
                         "\"buffers\":[{\"byteLength\":" + std::to_string(bin.size()) + "}],"
                         "\"bufferViews\":[" + views + "],\"accessors\":[" + accs + "]}";
        while (js.size() % 4) js += ' ';
        FILE* f = fopen(ruta.c_str(), "wb");
        if (!f) { err = "no se puede escribir " + ruta; return false; }
        uint32_t h[3] = {0x46546C67u, 2u, (uint32_t)(12 + 8 + js.size() + 8 + bin.size())};
        uint32_t cj[2] = {(uint32_t)js.size(), 0x4E4F534Au}, cb[2] = {(uint32_t)bin.size(), 0x004E4942u};
        fwrite(h, 4, 3, f); fwrite(cj, 4, 2, f); fwrite(js.data(), 1, js.size(), f);
        fwrite(cb, 4, 2, f); fwrite(bin.data(), 1, bin.size(), f);
        fclose(f);
        return true;
    }

    // .kart -> Assets/Models/<nombre>_pista.glb (+ texturas). rel = ruta relativa al proyecto.
    // aviso: lo que no impide guardar pero conviene saber (texturas que faltan...).
    bool exportar(const std::string& proyecto, const std::string& nombre, const std::string& tex_src,
                  std::string& rel, std::string& err, std::string* aviso = nullptr) {
        // si la escena vinculada no existe se usa cesped plano (y se restaura el vinculo al salir)
        struct Resto { std::string& e; std::string v; ~Resto() { e = v; } } resto{escena, escena};
        if (!escena.empty() && !fs::exists(fs::path(proyecto) / escena)) escena.clear();
        calcular();
        const bool propia_pista = !pista_glb.empty();                // pista propia: su .glb se usa tal cual; solo se generan turbos, meta y rampas encima
        ex_vis.clear(); ex_col.clear(); ex_pcol.clear();
        if (propia_pista && !fs::exists(fs::path(proyecto) / pista_glb)) { err = "no existe la pista " + pista_glb; return false; }
        if (!propia_pista && m.empty()) { err = "el circuito necesita al menos un camino con 2 nodos"; return false; }
        std::error_code ec;
        fs::create_directories(proyecto + "/Assets/Models", ec);
        fs::create_directories(proyecto + "/Assets/Textures/carreras", ec);
        std::vector<Malla> M = construir();
        for (int k = 0; k < MT_N; k++) {
            bool propia = !tex[k].empty() && fs::exists(proyecto + "/" + tex[k]);
            if (!tex[k].empty() && !propia && aviso)
                *aviso += std::string("no existe la textura ") + tex[k] + " (" + MT_ETIQ[k] + "): uso la de serie\n";
            if (propia) {
                /* El cargador glTF del motor NO decodifica %20 ni acentos en las rutas
                   de las imagenes: una textura con espacios saldria en blanco. Se copia
                   con un nombre seguro junto a las de serie y el .glb apunta a la copia. */
                fs::path src = fs::path(proyecto) / tex[k];
                std::string seguro = std::string("usr_") + MT_NOMBRE[k] + "_" + src.filename().string();
                for (auto& ch : seguro) if (!isalnum((unsigned char)ch) && ch != '.' && ch != '_' && ch != '-') ch = '_';
                fs::path dst = fs::path(proyecto) / "Assets/Textures/carreras" / seguro;
                std::error_code e2;
                bool igual = fs::exists(dst, e2) && fs::file_size(dst, e2) == fs::file_size(src, e2);
                if (!igual) fs::copy_file(src, dst, fs::copy_options::overwrite_existing, e2);
                if (e2) { err = "no se pudo copiar la textura " + tex[k]; return false; }
                M[k].uri = "../Textures/carreras/" + seguro;
                continue;
            }
            std::string dst = proyecto + "/Assets/Textures/carreras/" + MT_NOMBRE[k] + ".png";
            if (!fs::exists(dst)) {
                fs::copy_file(tex_src + "/" + MT_NOMBRE[k] + ".png", dst, ec);
                if (ec) { err = "no encuentro la textura " + tex_src + "/" + MT_NOMBRE[k] + ".png"; return false; }
            }
        }
        for (auto& o : objetos)
            if (aviso && !fs::exists(proyecto + "/" + o.modelo)) *aviso += "no existe el modelo " + o.modelo + "\n";
        if (aviso) *aviso += avisos();
        if (propia_pista) {
            rel = pista_glb;
            if ((!sincol.empty() || col_alto > 0 || col_sin_techos) && hook_col) {   // la colision sin las piezas quitadas ni las caras que estorban
                std::string rc = "Assets/Models/" + nombre + "_col.glb", e3;
                if (hook_col(proyecto, pista_glb, sincol, col_sin_techos ? -0.3f : -2.0f, col_alto > 0 ? col_alto / std::max(0.001f, pista_esc) : 0.0f, rc, e3)) ex_pcol = rc; else if (aviso) *aviso += "colision: " + e3 + "\n";
            }
            if (m.empty()) return true;
            std::vector<Malla> V = M, C = M; bool hv = false, hc = false;
            for (int k = 0; k < MT_N; k++) {
                if (k != MT_TURBO && k != MT_META) V[k].idx.clear(); else if (!V[k].idx.empty()) hv = true;
                if (k != MT_RAMPA) C[k].idx.clear(); else if (!C[k].idx.empty()) hc = true;
            }
            if (hv) { std::string r2 = "Assets/Models/" + nombre + "_extras.glb"; if (!escribir_glb(V, proyecto + "/" + r2, err)) return false; ex_vis = r2; }
            if (hc) { std::string r2 = "Assets/Models/" + nombre + "_rampas.glb"; if (!escribir_glb(C, proyecto + "/" + r2, err)) return false; ex_col = r2; }
            return true;
        }
        rel = "Assets/Models/" + nombre + "_pista.glb";
        return escribir_glb(M, proyecto + "/" + rel, err);
    }
};

// --------------------------------------------------------------------------- miniaturas
struct Miniaturas {
    std::map<std::string, H2Img> cache;
    int cargas = 0;
    void frame() {
        cargas = 0;
        if (cache.size() > 350) { for (auto& kv : cache) if (kv.second.tex) h2_free_image(&kv.second); cache.clear(); }
    }
    // nullptr = aun sin cargar (se carga a razon de 3 por fotograma)
    H2Img* get(const std::string& ruta) {
        auto it = cache.find(ruta);
        if (it != cache.end()) return it->second.tex ? &it->second : nullptr;
        if (cargas >= 3) return nullptr;
        cargas++;
        H2Img im = {0, 0, 0};
        h2_load_thumb(ruta.c_str(), 96, &im);
        auto ins = cache.emplace(ruta, im).first;
        return ins->second.tex ? &ins->second : nullptr;
    }
};

// explorador de carpetas del proyecto (Assets/...)
struct Expl {
    std::string dir = "Assets";
    char filtro[64] = "";
    std::string listado;                 // carpeta de la que son dirs/files
    std::vector<std::string> dirs, files;
};

// --------------------------------------------------------------------------- editor
struct EditorCarreras {
    std::string proyecto, scenes_dir, tex_src;   // los pone main.cpp
    std::function<void(const std::string&)> on_play;      // nombre del circuito: main.cpp genera y ejecuta
    std::function<void(const std::string&)> on_created;   // se acaba de crear un circuito nuevo
    std::function<std::string(int)> asset_ruta;           // indice del panel Assets -> "Assets/..." (arrastrar)
    std::function<std::vector<std::string>()> lista_escenas;   // escenas .scene del proyecto (sin la ruta)
    std::function<std::string(const std::string&)> crear_escena;  // crea una escena vacia; devuelve "Scenes/x.scene" o ""
    std::function<void()> editar_3d;                      // abre la escena vinculada en el modo Terreno
    std::function<void()> adaptar_terreno;                // nivela el terreno bajo la carretera
    bool prev_sucia = true;                               // la vista previa 3D hay que rehacerla
    std::string nombre;                                    // circuito abierto (sin .kart)
    Circuito c;
    bool inited = false, sucio = false, cargado = false;
    std::string msg;

    // ventana y vista cenital
    bool  maximizada = true;
    float vx = 70, vz = 0, zoom = 2.0f;
    int herr = 0;                 // 0 mover, 1 punto, 2 salida, 3 rampa, 4 objeto, 5 turbo, 6 agua
    int sel_tipo = 0, sel = -1;   // 1 nodo, 2 rampa, 3 salida, 4 objeto, 5 turbo, 6 lago, 7 tramo (nodo/tramo: del camino cam_sel)
    int  cam_sel = 0;             // camino activo
    int  tipo_nuevo = T_CURVA;    // tipo del siguiente tramo al dibujar
    bool pen = false; int pen_c = 0, pen_n = 0;     // arrastrando la tangente del nodo recien puesto
    int  asa = 0;                 // arrastrando la asa de un nodo: +1 la de salida, -1 la de entrada
    // goma: arrastrar sobre el mapa borra lo que toca (filtrable) / deshacer y rehacer
    bool  gom_car = true, gom_obj = true, gom_rt = true, gom_lago = true;
    float goma_px = 18.0f;
    bool  trazo_snap = false, drag_snap = false, panel_activo = false;
    std::vector<Circuito> deshacer, rehacer;
    Circuito base_panel;          // el estado estable de antes de tocar un control del panel
    void push_undo() { deshacer.push_back(c); if (deshacer.size() > 60) deshacer.erase(deshacer.begin()); rehacer.clear(); }
    void tras_cambio_historial() {
        c.calcular(); sel = -1; sel_tipo = 0; girando = false; pen = false; asa = 0;
        if (cam_sel >= (int)c.caminos.size()) cam_sel = std::max(0, (int)c.caminos.size() - 1);
        sucio = true; prev_sucia = true;
    }
    void do_undo() { if (deshacer.empty()) return; rehacer.push_back(c); c = deshacer.back(); deshacer.pop_back(); tras_cambio_historial(); msg = "Deshecho"; }
    void do_redo() { if (rehacer.empty()) return; deshacer.push_back(c); c = rehacer.back(); rehacer.pop_back(); tras_cambio_historial(); msg = "Rehecho"; }
    bool girando = false;         // arrastrando el asa de giro de un objeto
    int slot_tex = MT_ASFALTO;    // parte del circuito a la que se asigna la textura
    std::string obj_modelo;       // modelo elegido en el explorador para colocar

    // explorador
    std::vector<std::string> modelos, texturas;   // todos (para la busqueda)
    Expl ex_tex, ex_mod, ex_kart;
    Miniaturas mini;
    int tab_forzada = -1;         // solo pruebas

    std::string ruta() const { return scenes_dir + "/" + nombre + ".kart"; }

    static std::string minus(std::string t) { std::transform(t.begin(), t.end(), t.begin(), ::tolower); return t; }
    static bool es_img(const std::string& e) { return e == ".png" || e == ".jpg" || e == ".jpeg"; }
    static bool es_mod(const std::string& e) { return e == ".glb" || e == ".gltf" || e == ".fbx"; }

    void escanear() {
        modelos.clear(); texturas.clear();
        std::error_code ec;
        fs::recursive_directory_iterator it(proyecto + "/Assets", fs::directory_options::skip_permission_denied, ec);
        if (ec) return;
        for (; it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) break;
            std::error_code e1;
            if (!it->is_regular_file(e1)) continue;
            std::string ext = minus(it->path().extension().string());
            std::string rel = fs::relative(it->path(), proyecto, e1).generic_string();
            if (e1) continue;
            if (es_mod(ext)) { if (rel.find("_pista.glb") == std::string::npos) modelos.push_back(rel); }
            else if (es_img(ext)) texturas.push_back(rel);
        }
        std::sort(modelos.begin(), modelos.end());
        std::sort(texturas.begin(), texturas.end());
        ex_tex.listado.clear(); ex_mod.listado.clear(); ex_kart.listado.clear();
    }

    void init(const std::string& proj) {
        proyecto = proj; scenes_dir = proj + "/Scenes";
        inited = true; cargado = false; sel = -1; sel_tipo = 0;
        if (nombre.empty()) {   // el primer .kart del proyecto
            std::error_code ec;
            for (auto& e : fs::directory_iterator(scenes_dir, ec))
                if (e.path().extension() == ".kart") { nombre = e.path().stem().string(); break; }
        }
        if (!nombre.empty() && c.load(ruta())) { cargado = true; c.calcular(); centrar(); }
        escanear();
        // ganchos de prueba (sin GUI): EDITOR_KR_SEL=<tipo>:<indice> elige algo; EDITOR_KR_TAB=<0..3> la pestana
        if (const char* e = getenv("EDITOR_KR_SEL")) { int t = 0, i = 0; if (sscanf(e, "%d:%d", &t, &i) == 2) { sel_tipo = t; sel = i; } }
        if (const char* e = getenv("EDITOR_KR_TAB")) tab_forzada = atoi(e);
        if (const char* e = getenv("EDITOR_KR_HERR")) herr = atoi(e);
    }
    void centrar() {
        if (c.m.empty()) return;
        vx = (c.gx0 + c.gx1) * 0.5f; vz = (c.gz0 + c.gz1) * 0.5f;
    }
    bool nuevo(const std::string& n) {
        std::error_code ec; fs::create_directories(scenes_dir, ec);
        nombre = n; c = Circuito(); c.ejemplo(); c.calcular();
        cargado = true; sucio = true;
        bool ok = guardar();
        if (ok && on_created) on_created(nombre);
        centrar();
        return ok;
    }
    bool guardar() {
        if (!cargado) return false;
        if (!c.save(ruta())) { msg = "no se pudo guardar " + ruta(); return false; }
        std::string rel, err, aviso;
        if (!c.exportar(proyecto, nombre, tex_src, rel, err, &aviso)) { msg = "ERROR: " + err; return false; }
        sucio = false;
        char b[260]; snprintf(b, sizeof b, "guardado: vuelta de %.0f m, %d checkpoints, %d cruces -> %s", c.vuelta, (int)c.cps.size(), (int)c.cruces_det.size(), rel.c_str());
        msg = b;
        if (!aviso.empty()) msg += "\nAVISO: " + aviso;
        return true;
    }
    void tocar() { sucio = true; prev_sucia = true; c.calcular(); }

    // ------------------------------------------------------------- explorador
    // Devuelve la ruta (relativa al proyecto) en la que se ha hecho clic, o "".
    // Los archivos se pueden arrastrar (payload = su ruta) al mapa o a una casilla.
    std::string explorar(Expl& e, bool imagenes, const char* payload, const std::string& actual, float alto) {
        std::string elegido;
        // ---- migas de pan: Assets / Models / props ----
        {
            std::string acum; size_t p = 0;
            while (p <= e.dir.size()) {
                size_t q = e.dir.find('/', p); if (q == std::string::npos) q = e.dir.size();
                std::string parte = e.dir.substr(p, q - p);
                acum += (acum.empty() ? "" : "/") + parte;
                ImGui::PushID((int)p);
                if (p > 0) ImGui::SameLine(0, 2);
                if (ImGui::SmallButton(parte.c_str())) { e.dir = acum; e.filtro[0] = 0; ImGui::PopID(); break; }
                ImGui::PopID();
                p = q + 1;
            }
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filtro", "buscar en todo el proyecto...", e.filtro, sizeof e.filtro);
        bool buscando = e.filtro[0] != 0;
        // ---- contenido ----
        if (!buscando && e.listado != e.dir) {
            e.listado = e.dir; e.dirs.clear(); e.files.clear();
            std::error_code ec;
            for (auto& it : fs::directory_iterator(proyecto + "/" + e.dir, ec)) {
                std::string nm = it.path().filename().string();
                if (nm.empty() || nm[0] == '.') continue;
                std::error_code e1;
                if (it.is_directory(e1)) e.dirs.push_back(nm);
                else {
                    std::string ext = minus(it.path().extension().string());
                    if (imagenes ? es_img(ext) : es_mod(ext)) { if (nm.find("_pista.glb") == std::string::npos) e.files.push_back(nm); }
                }
            }
            std::sort(e.dirs.begin(), e.dirs.end()); std::sort(e.files.begin(), e.files.end());
        }
        std::vector<std::string> res;      // rutas relativas al proyecto
        if (buscando) {
            std::string f = minus(e.filtro);
            for (auto& r : (imagenes ? texturas : modelos)) {
                if (minus(r).find(f) != std::string::npos) res.push_back(r);
                if (res.size() >= 300) break;
            }
        } else {
            for (auto& f : e.files) res.push_back(e.dir + "/" + f);
        }
        ImGui::BeginChild(("##expl" + std::string(payload)).c_str(), ImVec2(0, alto), true);
        if (!buscando) {
            if (e.dir != "Assets" && ImGui::Selectable("  [..]  carpeta superior")) {
                size_t q = e.dir.rfind('/'); e.dir = q == std::string::npos ? "Assets" : e.dir.substr(0, q);
            }
            std::string entrar;
            for (auto& d : e.dirs) if (ImGui::Selectable(("  [+]  " + d).c_str())) entrar = d;
            if (!entrar.empty()) { e.dir += "/" + entrar; ImGui::EndChild(); return ""; }
        } else {
            ImGui::TextDisabled("%d resultados%s", (int)res.size(), res.size() >= 300 ? " (los primeros)" : "");
        }
        const float cell = 76.0f, sp = ImGui::GetStyle().ItemSpacing.x;
        int cols = std::max(1, (int)((ImGui::GetContentRegionAvail().x + sp) / (cell + sp))), n = 0;
        for (auto& r : res) {
            ImGui::PushID(r.c_str());
            std::string nm = fs::path(r).filename().string();
            bool on = (r == actual);
            if (imagenes) {
                if (n % cols != 0) ImGui::SameLine();
                ImVec2 p = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable("##i", on, 0, ImVec2(cell, cell + 16))) elegido = r;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", r.c_str());
                drag_fuente(payload, r, nm);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                if (H2Img* im = mini.get(proyecto + "/" + r))
                    dl->AddImage((ImTextureID)(intptr_t)im->tex, ImVec2(p.x + 2, p.y + 2), ImVec2(p.x + cell - 2, p.y + cell - 2));
                else dl->AddRectFilled(ImVec2(p.x + 2, p.y + 2), ImVec2(p.x + cell - 2, p.y + cell - 2), IM_COL32(60, 60, 66, 255));
                std::string corto = nm.size() > 11 ? nm.substr(0, 10) + "~" : nm;
                dl->AddText(ImVec2(p.x + 3, p.y + cell), IM_COL32(210, 210, 215, 255), corto.c_str());
            } else {
                std::string et = nm; if (buscando) et = r.substr(7);        // sin "Assets/"
                if (ImGui::Selectable(et.c_str(), on)) elegido = r;
                if (ImGui::IsItemHovered() && buscando) ImGui::SetTooltip("%s", r.c_str());
                drag_fuente(payload, r, nm);
            }
            ImGui::PopID();
            n++;
        }
        if (res.empty() && (buscando || e.dirs.empty())) ImGui::TextDisabled("%s", buscando ? "Nada con ese nombre." : "Carpeta vacia.");
        ImGui::EndChild();
        return elegido;
    }
    void drag_fuente(const char* payload, const std::string& ruta, const std::string& nombre_corto) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload(payload, ruta.c_str(), ruta.size() + 1);
            ImGui::Text("%s", nombre_corto.c_str());
            ImGui::EndDragDropSource();
        }
    }

    // ------------------------------------------------------------- dibujo
    void draw() {
        mini.frame();
        ImGuiWindowFlags fl = 0;
        if (maximizada) {
            // pantalla completa: todo el area de trabajo, sin tapar el menu ni la barra de modos
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->WorkPos);
            ImGui::SetNextWindowSize(vp->WorkSize);
            fl = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
        } else {
            ImGui::SetNextWindowSize(ImVec2(1100, 700), ImGuiCond_FirstUseEver);
        }
        if (!ImGui::Begin("Circuito de karts", nullptr, fl)) { ImGui::End(); return; }
        {   // Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y (no mientras se escribe en un campo de texto)
            ImGuiIO& io = ImGui::GetIO();
            if (io.KeyCtrl && !io.WantTextInput && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                if (ImGui::IsKeyPressed(ImGuiKey_Z)) { if (io.KeyShift) do_redo(); else do_undo(); }
                else if (ImGui::IsKeyPressed(ImGuiKey_Y)) do_redo();
            }
        }
        ImGui::BeginChild("kr_panel", ImVec2(390, 0), true);
        panel();
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("kr_lienzo", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        lienzo();
        ImGui::EndChild();
        ImGui::End();
    }

    void panel() {
        static char nom[64] = "circuito1";
        if (ImGui::SmallButton(maximizada ? "Ventana" : "Pantalla completa")) maximizada = !maximizada;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(maximizada ? "Hacerla ventana (para arrastrar desde el panel Assets)" : "Ocupar toda la pantalla del editor");
        ImGui::SameLine();
        if (cargado) ImGui::Text("%s.kart%s", nombre.c_str(), sucio ? "  (sin guardar)" : "");
        if (cargado) {
            int nn = 0; for (auto& cm : c.caminos) nn += (int)cm.nodos.size();
            ImGui::Text("Vuelta %.0f m - %d caminos, %d nodos - %d rampas - %d objetos", c.vuelta, (int)c.caminos.size(), nn, (int)c.rampas.size(), (int)c.objetos.size());
            if (ImGui::Button("Guardar", ImVec2(180, 0))) guardar();
            ImGui::SameLine();
            if (ImGui::Button("Probar", ImVec2(180, 0)) && guardar() && on_play) on_play(nombre);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Guarda, genera el juego empezando por este circuito y lo lanza");
        } else {
            ImGui::TextWrapped("No hay ningun circuito abierto. Crea uno nuevo (sale un ovalo de ejemplo para retocar).");
        }
        if (cargado) {
            ImGui::BeginDisabled(deshacer.empty()); if (ImGui::SmallButton("Deshacer")) do_undo(); ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(rehacer.empty());  if (ImGui::SmallButton("Rehacer")) do_redo();  ImGui::EndDisabled();
            ImGui::SameLine(); ImGui::TextDisabled("(Ctrl+Z / Ctrl+Y)");
        }
        ImGui::SetNextItemWidth(220);
        ImGui::InputText("##nom", nom, sizeof nom); ImGui::SameLine();
        if (ImGui::Button("Nuevo")) {
            std::string n = nom; for (auto& ch : n) if (!isalnum((unsigned char)ch)) ch = '_';
            if (!n.empty()) { if (cargado && sucio) guardar(); nuevo(n); }
        }
        if (!msg.empty()) ImGui::TextWrapped("%s", msg.c_str());
        if (!cargado) return;

        bool cambio = false;
        if (!ImGui::IsAnyItemActive() && !panel_activo) base_panel = c;       // estado estable para deshacer lo que se toque ahora
        if (ImGui::BeginTabBar("kr_tabs")) {
            auto fl = [&](int k) { return tab_forzada == k ? ImGuiTabItemFlags_SetSelected : 0; };
            if (ImGui::BeginTabItem("Trazado", nullptr, fl(0)))  { cambio |= tab_trazado();  ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Texturas", nullptr, fl(1))) { cambio |= tab_texturas(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Objetos", nullptr, fl(2)))  { cambio |= tab_objetos();  ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Ambiente", nullptr, fl(4))) { cambio |= tab_ambiente(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Carrera", nullptr, fl(3)))  { cambio |= tab_carrera();  ImGui::EndTabItem(); }
            tab_forzada = -1;
            ImGui::EndTabBar();
        }
        if (cambio && !panel_activo) { deshacer.push_back(base_panel); if (deshacer.size() > 60) deshacer.erase(deshacer.begin()); rehacer.clear(); panel_activo = true; }
        if (!ImGui::IsAnyItemActive() && !cambio) panel_activo = false;
        if (cambio) tocar();
    }

    // ---- pestana Trazado ----
    // parte el tramo k de un camino en dos, con un nodo nuevo en la muestra 'mi'
    void dividir(int ci, int k, int mi) {
        Camino& cm = c.caminos[ci]; const Muestra& s = c.m[mi];
        Nodo n; n.x = s.c.x; n.z = s.c.z; n.y = s.yb; n.ancho = s.ancho; n.peralte = s.peralte;
        Tramo t = cm.tramos[k];
        cm.nodos.insert(cm.nodos.begin() + k + 1, n); cm.tramos.insert(cm.tramos.begin() + k + 1, t);
        cam_sel = ci; sel_tipo = 1; sel = k + 1; tocar();
    }
    // quita un tramo: un camino cerrado se abre ahi; uno abierto se parte en dos
    void eliminar_tramo(int ci, int k) {
        if (ci < 0 || ci >= (int)c.caminos.size()) return;
        Camino cm = c.caminos[ci]; int n = (int)cm.nodos.size();
        if (k < 0 || k >= cm.ntr()) return;
        if (cm.cerrado) {
            Camino r = cm; r.cerrado = false;
            for (int i = 0; i < n; i++) { r.nodos[i] = cm.nodos[(k + 1 + i) % n]; r.tramos[i] = cm.tramos[(k + 1 + i) % n]; }
            c.caminos[ci] = r;
        } else {
            Camino a = cm, b = cm;
            a.nodos.assign(cm.nodos.begin(), cm.nodos.begin() + k + 1);  a.tramos.assign(cm.tramos.begin(), cm.tramos.begin() + k + 1);
            b.nodos.assign(cm.nodos.begin() + k + 1, cm.nodos.end());    b.tramos.assign(cm.tramos.begin() + k + 1, cm.tramos.end());
            b.principal = false; b.nombre = cm.nombre + "b";
            std::vector<Camino> nuevos;
            if (a.nodos.size() >= 2) nuevos.push_back(a);
            if (b.nodos.size() >= 2) nuevos.push_back(b);
            if (cm.principal && !nuevos.empty()) nuevos[0].principal = true;
            c.caminos.erase(c.caminos.begin() + ci); c.caminos.insert(c.caminos.begin() + ci, nuevos.begin(), nuevos.end());
        }
        cam_sel = std::clamp(cam_sel, 0, std::max(0, (int)c.caminos.size() - 1)); sel = -1; sel_tipo = 0; tocar();
    }
    void snap_trazo() { if (!trazo_snap) { push_undo(); trazo_snap = true; } }
    // la goma: borra lo que haya bajo (wx, wz) en un radio R (m), segun los filtros. true = borro algo
    bool borrar_en(float wx, float wz, float R) {
        auto d2 = [&](float x, float z) { float dx = x - wx, dz = z - wz; return dx * dx + dz * dz; };
        if (gom_obj) for (int i = (int)c.objetos.size() - 1; i >= 0; i--)
            if (d2(c.objetos[i].x, c.objetos[i].z) <= R * R + 4.0f) { snap_trazo(); c.objetos.erase(c.objetos.begin() + i); tocar(); sel = -1; sel_tipo = 0; return true; }
        if (gom_obj) for (int i = (int)c.luces.size() - 1; i >= 0; i--)
            if (d2(c.luces[i].x, c.luces[i].z) <= R * R + 4.0f) { snap_trazo(); c.luces.erase(c.luces.begin() + i); tocar(); sel = -1; sel_tipo = 0; return true; }
        if (gom_rt) {
            for (int i = (int)c.turbos.size() - 1; i >= 0; i--) { float rr = R + c.turbos[i].largo * 0.3f; if (d2(c.turbos[i].x, c.turbos[i].z) <= rr * rr) { snap_trazo(); c.turbos.erase(c.turbos.begin() + i); tocar(); sel = -1; sel_tipo = 0; return true; } }
            for (int i = (int)c.rampas.size() - 1; i >= 0; i--) { float rr = R + c.rampas[i].largo * 0.3f; if (d2(c.rampas[i].x, c.rampas[i].z) <= rr * rr) { snap_trazo(); c.rampas.erase(c.rampas.begin() + i); tocar(); sel = -1; sel_tipo = 0; return true; } }
        }
        if (gom_lago) for (int i = (int)c.lagos.size() - 1; i >= 0; i--) {
            const Lago& l = c.lagos[i];
            if (std::fabs(wx - l.cx) <= l.w * 0.5f + R && std::fabs(wz - l.cz) <= l.h * 0.5f + R) { snap_trazo(); c.lagos.erase(c.lagos.begin() + i); tocar(); sel = -1; sel_tipo = 0; return true; }
        }
        if (gom_car) {
            for (int ci = 0; ci < (int)c.caminos.size(); ci++)                       // un nodo (une los tramos de sus lados)
                for (int k = 0; k < (int)c.caminos[ci].nodos.size(); k++)
                    if (d2(c.caminos[ci].nodos[k].x, c.caminos[ci].nodos[k].z) <= R * R) { snap_trazo(); borrar_nodo(ci, k); return true; }
            int best = -1; float bd = 1e30f;                                          // un tramo
            for (int i = 0; i < (int)c.m.size(); i++) { float d = d2(c.m[i].c.x, c.m[i].c.z); if (d <= R * R && d < bd) { bd = d; best = i; } }
            if (best >= 0) { snap_trazo(); eliminar_tramo(c.m[best].camino, c.m[best].tramo); return true; }
        }
        return false;
    }
    void borrar_nodo(int ci, int k) {
        Camino& cm = c.caminos[ci];
        if (cm.nodos.size() <= 2) { bool era = cm.principal; c.caminos.erase(c.caminos.begin() + ci); if (era && !c.caminos.empty()) c.caminos[0].principal = true; cam_sel = 0; }
        else { cm.nodos.erase(cm.nodos.begin() + k); cm.tramos.erase(cm.tramos.begin() + k); if (cm.cerrado && cm.nodos.size() < 3) cm.cerrado = false; }
        sel = -1; sel_tipo = 0; tocar();
    }
    bool tab_trazado() {
        bool cambio = false;
        if (cam_sel >= (int)c.caminos.size()) cam_sel = std::max(0, (int)c.caminos.size() - 1);
        ImGui::TextDisabled("HERRAMIENTA");
        struct { int id; const char* n; } hs[] = {{0, "Mover / elegir"}, {1, "Dibujar camino"}, {2, "Poner la salida"}, {3, "Poner una rampa"}, {5, "Placa de turbo"}, {6, "Poner agua"}, {8, "Poner luz"}, {7, "Borrar (goma)"}};
        for (int i = 0; i < 8; i++) { if (i % 2) ImGui::SameLine(200); ImGui::RadioButton(hs[i].n, &herr, hs[i].id); }
        if (herr == 7) {
            ImGui::TextWrapped("Arrastra sobre el mapa: borra lo que toque. Un nodo se quita y sus tramos se unen; un tramo se quita y el camino se abre o se parte.");
            ImGui::SliderFloat("Tamano de la goma", &goma_px, 6.0f, 80.0f, "%.0f px");
            ImGui::TextDisabled("Que borra:");
            ImGui::Checkbox("Carretera", &gom_car); ImGui::SameLine(); ImGui::Checkbox("Objetos y luces", &gom_obj);
            ImGui::Checkbox("Rampas y turbos", &gom_rt); ImGui::SameLine(); ImGui::Checkbox("Lagos", &gom_lago);
        }
        if (ImGui::Button("Borrar todo...")) ImGui::OpenPopup("Borrar del circuito");
        if (ImGui::BeginPopupModal("Borrar del circuito", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Que quieres borrar?  (se puede deshacer con Ctrl+Z)");
            bool hecho = false;
            if (ImGui::Button("La carretera (todos los caminos)", ImVec2(300, 0))) { push_undo(); c.caminos.clear(); hecho = true; }
            if (ImGui::Button("Objetos, luces, rampas, turbos y lagos", ImVec2(300, 0))) { push_undo(); c.objetos.clear(); c.rampas.clear(); c.turbos.clear(); c.lagos.clear(); c.luces.clear(); hecho = true; }
            if (ImGui::Button("Todo", ImVec2(300, 0))) { push_undo(); c.caminos.clear(); c.objetos.clear(); c.rampas.clear(); c.turbos.clear(); c.lagos.clear(); c.luces.clear(); hecho = true; }
            if (ImGui::Button("Cancelar", ImVec2(300, 0))) ImGui::CloseCurrentPopup();
            if (hecho) { sel = -1; sel_tipo = 0; cam_sel = 0; herr = 0; ImGui::CloseCurrentPopup(); cambio = true; panel_activo = true; }
            ImGui::EndPopup();
        }
        if (herr == 1) {
            ImGui::TextWrapped("Clic = nodo nuevo (arrastra para curvar). Clic en el primer nodo cierra el camino. Intro o clic derecho: terminar.");
            ImGui::TextDisabled("Tipo del siguiente tramo:");
            ImGui::RadioButton("Recta##n", &tipo_nuevo, T_RECTO); ImGui::SameLine();
            ImGui::RadioButton("Curva##n", &tipo_nuevo, T_CURVA); ImGui::SameLine();
            ImGui::RadioButton("Salto##n", &tipo_nuevo, T_SALTO);
        } else if (herr == 0) {
            ImGui::TextDisabled("Clic en un nodo o en un tramo para editarlo. Doble clic\nen un tramo: nuevo nodo. Rueda: zoom. Boton derecho: mover.");
        }
        if (ImGui::Button("Centrar la vista")) centrar();
        ImGui::Separator();

        // ---- caminos ----
        ImGui::TextDisabled("CAMINOS");
        for (size_t i = 0; i < c.caminos.size(); i++) {
            const Camino& cm = c.caminos[i];
            char et[160]; snprintf(et, sizeof et, "%d  %s%s%s   (%d nodos)", (int)i, cm.nombre.c_str(), cm.principal ? "  [principal]" : "", cm.cerrado ? "" : "  [abierto]", (int)cm.nodos.size());
            if (ImGui::Selectable(et, cam_sel == (int)i)) { cam_sel = (int)i; if (sel_tipo == 1 || sel_tipo == 7) { sel_tipo = 0; sel = -1; } }
        }
        if (ImGui::Button("Nuevo camino")) {
            Camino cm; cm.cerrado = false; cm.nombre = "camino" + std::to_string(c.caminos.size());
            cm.principal = c.caminos.empty();
            c.caminos.push_back(cm); cam_sel = (int)c.caminos.size() - 1; herr = 1; sel_tipo = 0; sel = -1;
        }
        if (!c.cruces_det.empty() || !c.avisos_cruce.empty()) {
            ImGui::TextDisabled("CRUCES (carreteras que pasan una por encima de otra)");
            for (size_t i = 0; i < c.cruces_det.size(); i++) {
                char et[120]; snprintf(et, sizeof et, "Cruce %d  (%.0f, %.0f)  %.1f m libres  -  %s", (int)i, c.cruces_det[i].x, c.cruces_det[i].z, c.cruces_det[i].dy, c.cruces_det[i].tipo ? "tunel" : "puente");
                if (ImGui::Selectable(et, sel_tipo == 8 && sel == (int)i)) { sel_tipo = 8; sel = (int)i; }
            }
            for (auto& a : c.avisos_cruce) ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "%s", a.c_str());
        }
        if (!c.caminos.empty()) {
            Camino& cm = c.caminos[cam_sel];
            ImGui::SameLine();
            if (ImGui::Button("Borrar camino")) {
                bool era = cm.principal; c.caminos.erase(c.caminos.begin() + cam_sel);
                if (era && !c.caminos.empty()) c.caminos[0].principal = true;
                cam_sel = 0; sel_tipo = 0; sel = -1; return true;
            }
            bool pr = cm.principal;
            if (ImGui::Checkbox("Principal (lleva las vueltas)", &pr) && pr) { for (auto& o : c.caminos) o.principal = false; cm.principal = true; cambio = true; }
            ImGui::SameLine();
            bool ce = cm.cerrado;
            if (ImGui::Checkbox("Cerrado", &ce) && (!ce || cm.nodos.size() >= 3)) { cm.cerrado = ce; cambio = true; }
            if (ImGui::Button("Peralte automatico")) { c.peralte_auto(cam_sel); cambio = true; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Inclina cada curva de este camino segun lo\ncerrada que es (las rectas quedan planas)");
        }
        ImGui::Separator();

        // ---- lo elegido ----
        bool hay_cam = !c.caminos.empty() && cam_sel < (int)c.caminos.size();
        if (sel_tipo == 1 && hay_cam && sel >= 0 && sel < (int)c.caminos[cam_sel].nodos.size()) {
            Camino& cm = c.caminos[cam_sel]; Nodo& nd = cm.nodos[sel];
            ImGui::TextDisabled("NODO %d  (camino %d)", sel, cam_sel);
            cambio |= ImGui::DragFloat("X", &nd.x, 0.5f, -2000, 2000, "%.1f");
            cambio |= ImGui::DragFloat("Z", &nd.z, 0.5f, -2000, 2000, "%.1f");
            cambio |= ImGui::SliderFloat("Altura", &nd.y, 0.0f, 80.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Ancho", &nd.ancho, 4.0f, 40.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Peralte", &nd.peralte, -45.0f, 45.0f, "%.0f grados");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Positivo: la curva gira a la izquierda (el borde\nderecho sube). Negativo: a la derecha.");
            if (nd.tx != 0 || nd.tz != 0) { if (ImGui::SmallButton("Curva automatica")) { nd.tx = nd.tz = 0; cambio = true; } }
            else ImGui::TextDisabled("Curva automatica (arrastra su asa para cambiarla)");
            if (ImGui::Button("Editar el tramo que sale")) sel_tipo = 7;
            ImGui::SameLine();
            if (ImGui::Button("Borrar nodo")) { borrar_nodo(cam_sel, sel); return true; }
        } else if (sel_tipo == 7 && hay_cam && sel >= 0 && sel < c.caminos[cam_sel].ntr()) {
            Camino& cm = c.caminos[cam_sel]; int n = (int)cm.nodos.size();
            Nodo &A = cm.nodos[sel], &B = cm.nodos[(sel + 1) % n]; Tramo& T = cm.tramos[sel];
            ImGui::TextDisabled("TRAMO %d  (del nodo %d al %d, camino %d)", sel, sel, (sel + 1) % n, cam_sel);
            cambio |= ImGui::Combo("Tipo", &T.tipo, TIPO_NOMBRE, 3);
            if (T.tipo == T_SALTO) {
                cambio |= ImGui::SliderFloat("Fuerza del salto", &T.fuerza, 8.0f, 45.0f, "%.0f");
                float largo = std::hypot(B.x - A.x, B.z - A.z), al = Circuito::alcance(T.fuerza, B.y - A.y);
                bool ok = al >= largo;
                ImGui::TextColored(ok ? ImVec4(0.5f, 1, 0.5f, 1) : ImVec4(1, 0.5f, 0.4f, 1),
                                   "Hueco %.0f m, alcance a 160 km/h: %s%.0f m%s", largo, al < 0 ? "<" : "", al < 0 ? 0 : al, ok ? "" : "  (no llega)");
                ImGui::TextDisabled("Sin carretera entre los dos nodos; el kart sale\nlanzado al final del tramo anterior.");
            }
            ImGui::Separator();
            ImGui::TextDisabled("Valores al principio y al final del tramo");
            cambio |= ImGui::SliderFloat("Altura inicio", &A.y, 0.0f, 80.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Altura fin", &B.y, 0.0f, 80.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Peralte inicio", &A.peralte, -45.0f, 45.0f, "%.0f gr");
            cambio |= ImGui::SliderFloat("Peralte fin", &B.peralte, -45.0f, 45.0f, "%.0f gr");
            cambio |= ImGui::SliderFloat("Ancho inicio", &A.ancho, 4.0f, 40.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Ancho fin", &B.ancho, 4.0f, 40.0f, "%.1f m");
            bool su = T.suave; if (ImGui::Checkbox("Perfil suave (si no, lineal)", &su)) { T.suave = su; cambio = true; }
            ImGui::Separator();
            if (T.tipo != T_SALTO) cambio |= ImGui::Combo("Superficie", &T.sup, SUP_NOMBRE, N_SUP);
            bool mi = T.muro_izq, md = T.muro_der;
            if (ImGui::Checkbox("Muro izquierdo", &mi)) { T.muro_izq = mi; cambio = true; }
            ImGui::SameLine();
            if (ImGui::Checkbox("Muro derecho", &md)) { T.muro_der = md; cambio = true; }
            if (ImGui::Button("Dividir por la mitad") && sel < (int)c.m.size()) {
                int mid = -1, cnt = 0, first = -1;                            // la muestra central del tramo
                for (int i = 0; i < (int)c.m.size(); i++) if (c.m[i].camino == cam_sel && c.m[i].tramo == sel) { if (first < 0) first = i; cnt++; }
                mid = first + cnt / 2;
                if (first >= 0) { dividir(cam_sel, sel, mid); return true; }
            }
        } else if (sel_tipo == 9 && sel >= 0 && sel < (int)c.luces.size()) {
            Luz& l = c.luces[sel];
            ImGui::TextDisabled("LUZ %d", sel);
            cambio |= ImGui::SliderFloat("Altura##lz", &l.y, 0.5f, 25.0f, "%.1f m");
            float col[3] = {l.r, l.g, l.b};
            if (ImGui::ColorEdit3("Color", col)) { l.r = col[0]; l.g = col[1]; l.b = col[2]; cambio = true; }
            cambio |= ImGui::SliderFloat("Intensidad", &l.inten, 0.2f, 10.0f, "%.1f");
            cambio |= ImGui::SliderFloat("Alcance", &l.radio, 5.0f, 90.0f, "%.0f m");
            bool nc = l.noche, po = l.poste;
            if (ImGui::Checkbox("Solo de noche", &nc)) { l.noche = nc; cambio = true; }
            ImGui::SameLine();
            if (ImGui::Checkbox("Con poste", &po)) { l.poste = po; cambio = true; }
            if (ImGui::Button("Duplicar##lz")) { Luz d = l; d.x += 12; c.luces.push_back(d); sel = (int)c.luces.size() - 1; cambio = true; }
            ImGui::SameLine();
            if (ImGui::Button("Borrar##lz")) { c.luces.erase(c.luces.begin() + sel); sel = -1; sel_tipo = 0; cambio = true; }
        } else if (sel_tipo == 8 && sel >= 0 && sel < (int)c.cruces.size()) {
            ImGui::TextDisabled("CRUCE %d  (%.0f, %.0f)", sel, c.cruces[sel].x, c.cruces[sel].z);
            if (sel < (int)c.cruces_det.size()) ImGui::Text("Altura libre: %.1f m", c.cruces_det[sel].dy);
            int tp = c.cruces[sel].tipo;
            if (ImGui::Combo("Como cruza", &tp, CRUCE_NOMBRE, 2)) { c.cruces[sel].tipo = tp; cambio = true; }
            ImGui::TextWrapped("Puente: la carretera de arriba queda abierta por debajo, con una losa gruesa y pilares. Tunel: sus laterales siguen cerrados y la carretera de abajo entra por una boca forrada con muros.");
        } else if (sel_tipo == 2 && sel >= 0 && sel < (int)c.rampas.size()) {
            Rampa& r = c.rampas[sel];
            ImGui::TextDisabled("RAMPA %d", sel);
            cambio |= ImGui::SliderFloat("Ancho##r", &r.ancho, 3.0f, 30.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Largo", &r.largo, 8.0f, 60.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Alto", &r.alto, 0.5f, 10.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Fuerza", &r.fuerza, 5.0f, 45.0f, "%.0f");
            ImGui::TextDisabled("Sigue el rumbo de la carretera. El trampolin\nesta en la cresta.");
        } else if (sel_tipo == 3) {
            ImGui::TextDisabled("SALIDA");
            ImGui::Text("(%.1f, %.1f)  rumbo %.0f grados", c.sx, c.sz, c.salida_yaw);
            ImGui::TextDisabled("El rumbo y la altura salen de la carretera.\nLa linea de meta esta 10 m por delante.");
        } else if (sel_tipo == 5 && sel >= 0 && sel < (int)c.turbos.size()) {
            Turbo& t = c.turbos[sel];
            ImGui::TextDisabled("PLACA DE TURBO %d", sel);
            cambio |= ImGui::SliderFloat("Largo##t", &t.largo, 4.0f, 40.0f, "%.1f m");
            cambio |= ImGui::SliderFloat("Ancho##t", &t.ancho, 2.0f, 14.0f, "%.1f m");
        } else if (sel_tipo == 6 && sel >= 0 && sel < (int)c.lagos.size()) {
            Lago& l = c.lagos[sel];
            ImGui::TextDisabled("LAGO %d", sel);
            cambio |= ImGui::SliderFloat("Ancho##l", &l.w, 10.0f, 300.0f, "%.0f m");
            cambio |= ImGui::SliderFloat("Largo##l", &l.h, 10.0f, 300.0f, "%.0f m");
            cambio |= ImGui::SliderFloat("Profundidad", &l.prof, 1.0f, 12.0f, "%.1f m");
            bool mo = l.mortal; if (ImGui::Checkbox("Profundo: el kart reaparece", &mo)) { l.mortal = mo; cambio = true; }
            ImGui::TextDisabled("Si no, se puede cruzar a poca velocidad.");
        } else {
            ImGui::TextDisabled("Elige un nodo o un tramo en el mapa (o en el perfil\nde abajo) para ajustar su altura, peralte, ancho...");
        }
        return cambio;
    }

    // ---- pestana Texturas ----
    bool tab_texturas() {
        bool cambio = false;
        ImGui::TextWrapped("Arrastra una imagen sobre lo que quieras cambiar en el mapa (carretera, cesped, muros, rampas...) o sobre una casilla de abajo.");
        ImGui::Separator();
        const float cell = 80.0f, sp = ImGui::GetStyle().ItemSpacing.x;
        int cols = std::max(1, (int)((ImGui::GetContentRegionAvail().x + sp) / (cell + sp)));
        for (int k = 0; k < MT_N; k++) {
            if (k % cols != 0) ImGui::SameLine();
            ImGui::PushID(k);
            ImVec2 p = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable("##slot", slot_tex == k, 0, ImVec2(cell, cell + 16))) slot_tex = k;
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("KR_TEX")) { c.tex[k] = (const char*)pl->Data; slot_tex = k; cambio = true; }
                ImGui::EndDragDropTarget();
            }
            std::string ruta = c.tex[k].empty() ? tex_src + "/" + MT_NOMBRE[k] + ".png" : proyecto + "/" + c.tex[k];
            ImDrawList* dl = ImGui::GetWindowDrawList();
            if (H2Img* im = mini.get(ruta)) dl->AddImage((ImTextureID)(intptr_t)im->tex, ImVec2(p.x + 3, p.y + 3), ImVec2(p.x + cell - 3, p.y + cell - 3));
            dl->AddText(ImVec2(p.x + 4, p.y + cell), IM_COL32(230, 230, 235, 255), MT_ETIQ[k]);
            ImGui::PopID();
        }
        ImGui::Separator();
        ImGui::TextDisabled("%s", MT_ETIQ[slot_tex]);
        if (slot_tex != MT_META) cambio |= ImGui::SliderFloat("Se repite cada", &c.esc[slot_tex], 0.5f, 60.0f, "%.1f m");
        if (!c.tex[slot_tex].empty()) {
            ImGui::TextDisabled("%s", c.tex[slot_tex].c_str());
            if (ImGui::SmallButton("Volver a la de serie")) { c.tex[slot_tex].clear(); cambio = true; }
        }
        ImGui::TextDisabled("EXPLORADOR (clic = asignar a '%s')", MT_ETIQ[slot_tex]);
        std::string r = explorar(ex_tex, true, "KR_TEX", c.tex[slot_tex], 0);
        if (!r.empty()) { c.tex[slot_tex] = r; cambio = true; }
        return cambio;
    }

    // ---- pestana Objetos ----
    bool tab_objetos() {
        bool cambio = false;
        ImGui::TextWrapped("Arrastra un modelo al mapa, o haz clic en uno y luego clic en el mapa (tantos como quieras).");
        if (herr == 4) {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1, 1), "COLOCANDO: %s", fs::path(obj_modelo).filename().string().c_str());
            ImGui::SameLine(); if (ImGui::SmallButton("Parar")) herr = 0;
        }
        std::string r = explorar(ex_mod, false, "KR_MOD", herr == 4 ? obj_modelo : "", 230);
        if (!r.empty()) { obj_modelo = r; herr = 4; }
        ImGui::Separator();
        ImGui::TextDisabled("COLOCADOS (%d)", (int)c.objetos.size());
        ImGui::BeginChild("kr_obj_lista", ImVec2(0, 90), true);
        for (size_t i = 0; i < c.objetos.size(); i++) {
            ImGui::PushID((int)i);
            std::string et = std::to_string(i) + "  " + fs::path(c.objetos[i].modelo).filename().string();
            if (ImGui::Selectable(et.c_str(), sel_tipo == 4 && sel == (int)i)) { sel_tipo = 4; sel = (int)i; }
            ImGui::PopID();
        }
        ImGui::EndChild();
        if (sel_tipo == 4 && sel >= 0 && sel < (int)c.objetos.size()) {
            Objeto& o = c.objetos[sel];
            ImGui::TextDisabled("OBJETO %d   (Q / E giran 15 grados; tambien el asa del mapa)", sel);
            cambio |= ImGui::SliderFloat("Giro", &o.yaw, -180.0f, 180.0f, "%.0f grados");
            cambio |= ImGui::SliderFloat("Tamano", &o.alto, 0.0f, 40.0f, o.alto <= 0 ? "original" : "%.1f m de alto");
            cambio |= ImGui::SliderFloat("Altura extra", &o.y, 0.0f, 30.0f, "%.1f m");
            const char* cols[] = {"Sin colision", "Caja", "Malla exacta"};
            cambio |= ImGui::Combo("Colision", &o.colision, cols, 3);
            if (o.colision == 2 && std::fabs(o.yaw) > 0.5f)
                ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "Con giro la malla exacta no gira: se usara una caja.");
            if (ImGui::Button("Duplicar")) { Objeto d = o; d.x += 4; d.z += 4; c.objetos.push_back(d); sel = (int)c.objetos.size() - 1; cambio = true; }
            ImGui::SameLine();
            if (ImGui::Button("Borrar")) { c.objetos.erase(c.objetos.begin() + sel); sel = -1; sel_tipo = 0; cambio = true; }
        }
        return cambio;
    }

    // ---- pestana Ambiente: hora del dia y luces ----
    bool tab_ambiente() {
        bool cambio = false;
        ImGui::TextDisabled("MOMENTO DEL DIA");
        for (int i = 0; i < 4; i++) {
            bool off = i == 0 && c.escena.empty();
            ImGui::BeginDisabled(off);
            if (ImGui::RadioButton(AMB_NOMBRE[i], c.amb_modo == i)) { c.amb_modo = i; cambio = true; }
            ImGui::EndDisabled();
            if (off && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Hace falta una escena vinculada (pestana Carrera)");
        }
        if (c.amb_modo == 0) ImGui::TextWrapped("El sol, el cielo y la hora los lleva la escena vinculada (panel Entorno del modo Escena).");
        if (c.amb_modo == 3) {
            cambio |= ImGui::SliderFloat("Hora de salida", &c.amb_hora, 0.0f, 24.0f, "%.1f h");
            cambio |= ImGui::SliderFloat("Dura un dia", &c.amb_dur, 30.0f, 1800.0f, "%.0f s");
            int hh = (int)c.amb_hora, mm = (int)((c.amb_hora - hh) * 60);
            ImGui::TextDisabled("Sale a las %02d:%02d; un dia entero dura %.1f min.", hh, mm, c.amb_dur / 60.0f);
        } else if (c.amb_modo == 1) ImGui::TextDisabled("Pleno dia (hacia las 10:00), sin cambios.");
        else if (c.amb_modo == 2) ImGui::TextDisabled("Noche cerrada: solo luz de luna, farolas y los faros del kart.");
        bool fa = c.faros; if (ImGui::Checkbox("Faros del kart (se encienden de noche)", &fa)) { c.faros = fa; cambio = true; }
        ImGui::Separator();
        ImGui::TextDisabled("LUCES (%d)", (int)c.luces.size());
        if (ImGui::Button("Poner luz en el mapa")) { herr = 8; }
        if (herr == 8) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.5f, 0.8f, 1, 1), "clic en el mapa"); ImGui::SameLine(); if (ImGui::SmallButton("Parar")) herr = 0; }
        if (c.luces.size() > 24) ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "El motor admite 32 luces en total: pasarse de 24 no se vera.");
        ImGui::BeginChild("kr_luces", ImVec2(0, 120), true);
        for (size_t i = 0; i < c.luces.size(); i++) {
            ImGui::PushID((int)i);
            char et[96]; snprintf(et, sizeof et, "%d  (%.0f, %.0f)  %s", (int)i, c.luces[i].x, c.luces[i].z, c.luces[i].noche ? "de noche" : "siempre");
            if (ImGui::Selectable(et, sel_tipo == 9 && sel == (int)i)) { sel_tipo = 9; sel = (int)i; }
            ImGui::PopID();
        }
        ImGui::EndChild();
        if (sel_tipo == 9 && sel >= 0 && sel < (int)c.luces.size()) {
            Luz& l = c.luces[sel];
            ImGui::TextDisabled("LUZ %d", sel);
            cambio |= ImGui::SliderFloat("Altura", &l.y, 0.5f, 25.0f, "%.1f m");
            float col[3] = {l.r, l.g, l.b};
            if (ImGui::ColorEdit3("Color", col)) { l.r = col[0]; l.g = col[1]; l.b = col[2]; cambio = true; }
            cambio |= ImGui::SliderFloat("Intensidad", &l.inten, 0.2f, 10.0f, "%.1f");
            cambio |= ImGui::SliderFloat("Alcance", &l.radio, 5.0f, 90.0f, "%.0f m");
            bool nc = l.noche, po = l.poste;
            if (ImGui::Checkbox("Solo de noche", &nc)) { l.noche = nc; cambio = true; }
            ImGui::SameLine();
            if (ImGui::Checkbox("Con poste", &po)) { l.poste = po; cambio = true; }
            if (ImGui::Button("Duplicar")) { Luz d = l; d.x += 12; c.luces.push_back(d); sel = (int)c.luces.size() - 1; cambio = true; }
            ImGui::SameLine();
            if (ImGui::Button("Borrar")) { c.luces.erase(c.luces.begin() + sel); sel = -1; sel_tipo = 0; cambio = true; }
        }
        return cambio;
    }

    // ---- pestana Carrera ----
    bool tab_carrera() {
        bool cambio = false;
        ImGui::TextDisabled("TERRENO Y OBJETOS EN 3D");
        {
            std::vector<std::string> esc = lista_escenas ? lista_escenas() : std::vector<std::string>();
            std::string actual = c.escena.empty() ? "(ninguna: cesped plano)" : fs::path(c.escena).filename().string();
            if (ImGui::BeginCombo("Escena", actual.c_str())) {
                if (ImGui::Selectable("(ninguna: cesped plano)", c.escena.empty())) { c.escena.clear(); cambio = true; }
                for (auto& e : esc) if (ImGui::Selectable(e.c_str(), c.escena == "Scenes/" + e)) { c.escena = "Scenes/" + e; cambio = true; }
                ImGui::EndCombo();
            }
            if (c.escena.empty()) {
                ImGui::TextWrapped("Vincula una escena para esculpir el terreno, pintarlo, poner vegetacion, agua y objetos con las herramientas 3D de siempre, con la carretera a la vista.");
                if (crear_escena && ImGui::Button("Crear una escena nueva para este circuito")) {
                    std::string r = crear_escena(nombre + "_mundo");
                    if (!r.empty()) { c.escena = r; cambio = true; msg = "escena creada: " + r; }
                }
            } else {
                if (editar_3d && ImGui::Button("Editar terreno y objetos en 3D")) { guardar(); editar_3d(); }
                if (adaptar_terreno && ImGui::Button("Adaptar el terreno a la carretera")) { guardar(); adaptar_terreno(); guardar(); }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sube o baja el terreno bajo cada tramo de carretera\n(terraplen o desmonte) para que no se meta por\nencima. Los saltos no se tocan.");
                ImGui::TextDisabled("En el visor 3D veras la carretera. No pongas un jugador\nen esa escena: el kart es el jugador.");
            }
        }
        ImGui::Separator();
        ImGui::TextDisabled("CARRERA");
        cambio |= ImGui::SliderInt("Vueltas", &c.vueltas, 1, 9);
        cambio |= ImGui::SliderFloat("Cesped alrededor", &c.margen, 10.0f, 250.0f, "%.0f m");
        ImGui::Separator();
        ImGui::TextDisabled("KART");
        static char kbuf[256];
        snprintf(kbuf, sizeof kbuf, "%s", c.kart.c_str());
        if (ImGui::InputText("Modelo", kbuf, sizeof kbuf)) { c.kart = kbuf; cambio = true; }
        if (!fs::exists(proyecto + "/" + c.kart)) ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "No existe: saldra un cubo");
        cambio |= ImGui::SliderFloat("Tamano", &c.kart_tam, 0.5f, 3.0f, "%.2f m");
        ImGui::TextDisabled("EXPLORADOR (clic = usar como kart)");
        std::string r = explorar(ex_kart, false, "KR_KART", c.kart, 0);
        if (!r.empty()) { c.kart = r; cambio = true; }
        return cambio;
    }

    // ------------------------------------------------------------- mapa
    // mundo <-> pantalla (cenital: +Z arriba, +X a la IZQUIERDA)
    ImVec2 a_pant(float x, float z, ImVec2 cen) const { return ImVec2(cen.x - (x - vx) * zoom, cen.y - (z - vz) * zoom); }
    void a_mundo(ImVec2 p, ImVec2 cen, float& x, float& z) const { x = vx - (p.x - cen.x) / zoom; z = vz - (p.y - cen.y) / zoom; }

    void poner_objeto(const std::string& modelo, float x, float z) {
        if (modelo.empty()) return;
        Objeto o; o.modelo = modelo; o.x = x; o.z = z;
        c.objetos.push_back(o); sel_tipo = 4; sel = (int)c.objetos.size() - 1; tocar();
    }
    static bool dentro_rect(float lx, float lz, float f_x, float f_z, float qx, float qz, float largo, float ancho, float cx, float cz) {
        float dx = qx - cx, dz = qz - cz;
        float lon = dx * f_x + dz * f_z, lat = dx * f_z - dz * f_x;      // izquierda = (f_z, -f_x)
        (void)lx; (void)lz;
        return std::fabs(lon) <= largo * 0.5f && std::fabs(lat) <= ancho * 0.5f;
    }
    // que parte del circuito hay en (x, z): la textura que se asignaria al soltar ahi
    int parte_en(float x, float z) const {
        for (auto& r : c.rampas) {
            int si = c.muestra_cercana(r.x, r.z); V3 f = norm(v3(c.m[si].t.x, 0, c.m[si].t.z));
            if (dentro_rect(0, 0, f.x, f.z, x, z, r.largo, r.ancho, r.x, r.z)) return MT_RAMPA;
        }
        for (auto& t : c.turbos) {
            int si = c.muestra_cercana(t.x, t.z); V3 f = norm(v3(c.m[si].t.x, 0, c.m[si].t.z));
            if (dentro_rect(0, 0, f.x, f.z, x, z, t.largo, t.ancho, t.x, t.z)) return MT_TURBO;
        }
        for (auto& l : c.lagos) if (std::fabs(x - l.cx) <= l.w * 0.5f && std::fabs(z - l.cz) <= l.h * 0.5f) return MT_FONDO;
        if (!c.m.empty()) {
            const Muestra& s = c.m[c.muestra_cercana(x, z)];
            V3 d = v3(x - s.c.x, 0, z - s.c.z);
            float lat = dot(d, v3(s.izq.x, 0, s.izq.z)), lon = dot(d, norm(v3(s.t.x, 0, s.t.z))), hw = s.ancho * 0.5f;
            if (!s.hueco && std::fabs(lon) <= 3.0f) {
                if (std::fabs(lat) <= hw + 1.5f && std::fabs(lat) >= hw - 1.0f && (lat > 0 ? s.muro_izq : s.muro_der)) return MT_MURO;
                if (std::fabs(lat) <= hw) return MT_ASFALTO + std::clamp(s.sup, 0, N_SUP - 1);
            }
        }
        return MT_CESPED;
    }

    // perfil de altura del camino activo (abajo del mapa): clic = elegir el nodo mas cercano
    void perfil(ImDrawList* dl, ImVec2 a, ImVec2 sz, bool hov) {
        dl->AddRectFilled(a, ImVec2(a.x + sz.x, a.y + sz.y), IM_COL32(22, 24, 28, 255));
        if (cam_sel < 0 || cam_sel >= (int)c.strips.size() || c.strips[cam_sel].n < 2) {
            dl->AddText(ImVec2(a.x + 10, a.y + 8), IM_COL32(150, 150, 155, 255), "Perfil de altura: dibuja un camino con 2 nodos o mas");
            return;
        }
        const Strip& S = c.strips[cam_sel];
        float ymin = 1e30f, ymax = -1e30f;
        for (int i = S.ini; i < S.ini + S.n; i++) { ymin = std::min(ymin, c.m[i].c.y); ymax = std::max(ymax, c.m[i].c.y); }
        ymin = std::min(ymin, 0.0f); if (ymax - ymin < 6.0f) ymax = ymin + 6.0f;
        const float ml = 44, mt = 14, mb = 18;
        float gw = sz.x - ml - 10, gh = sz.y - mt - mb, L = std::max(1.0f, S.largo);
        auto X = [&](float d) { return a.x + ml + d / L * gw; };
        auto Y = [&](float y) { return a.y + mt + (1.0f - (y - ymin) / (ymax - ymin)) * gh; };
        dl->AddLine(ImVec2(a.x + ml, Y(0)), ImVec2(a.x + ml + gw, Y(0)), IM_COL32(90, 130, 90, 255));
        char t[48]; snprintf(t, sizeof t, "%.0f m", ymax); dl->AddText(ImVec2(a.x + 4, a.y + mt - 6), IM_COL32(170, 170, 175, 255), t);
        snprintf(t, sizeof t, "%.0f m", ymin); dl->AddText(ImVec2(a.x + 4, a.y + sz.y - mb - 8), IM_COL32(170, 170, 175, 255), t);
        snprintf(t, sizeof t, "Perfil de altura - camino %d - %.0f m", cam_sel, S.largo); dl->AddText(ImVec2(a.x + ml, a.y + 1), IM_COL32(200, 200, 205, 255), t);
        for (int i = S.ini; i < S.ini + S.n; i++) {
            int j = c.sig(i); if (j < 0) continue;
            float d1 = (j == S.ini) ? S.largo : c.m[j].dist;
            ImVec2 p1(X(c.m[i].dist), Y(c.m[i].c.y)), p2(X(d1), Y(c.m[j].c.y));
            bool selt = sel_tipo == 7 && c.m[i].tramo == sel;
            if (c.m[i].hueco) { if ((i / 2) % 2 == 0) dl->AddLine(p1, p2, IM_COL32(255, 90, 70, 255), 2.0f); }
            else dl->AddLine(p1, p2, selt ? IM_COL32(255, 220, 90, 255) : IM_COL32(140, 185, 255, 255), selt ? 3.0f : 2.0f);
        }
        // nodos
        const Camino& cm = c.caminos[cam_sel];
        int near_k = -1; float near_d = 1e30f; ImVec2 mp = ImGui::GetIO().MousePos;
        for (int k = 0; k < (int)cm.nodos.size(); k++) {
            int mi = -1; for (int i = S.ini; i < S.ini + S.n; i++) if (c.m[i].tramo == k) { mi = i; break; }
            if (mi < 0) { if (k == (int)cm.nodos.size() - 1 && !cm.cerrado) mi = S.ini + S.n - 1; else continue; }
            ImVec2 p(X(c.m[mi].dist), Y(c.m[mi].c.y));
            bool on = sel_tipo == 1 && sel == k;
            dl->AddLine(ImVec2(p.x, a.y + mt), ImVec2(p.x, a.y + sz.y - mb), IM_COL32(70, 72, 80, 255));
            dl->AddCircleFilled(p, on ? 6.0f : 4.5f, on ? IM_COL32(255, 200, 60, 255) : IM_COL32(250, 250, 250, 255));
            snprintf(t, sizeof t, "%d", k); dl->AddText(ImVec2(p.x - 3, a.y + sz.y - mb + 2), IM_COL32(190, 190, 195, 255), t);
            float d = std::fabs(mp.x - p.x); if (d < near_d) { near_d = d; near_k = k; }
        }
        if (hov && near_k >= 0 && near_d < 14 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { sel_tipo = 1; sel = near_k; }
    }

    void lienzo() {
        ImVec2 p0 = ImGui::GetCursorScreenPos(), sz = ImGui::GetContentRegionAvail();
        if (sz.x < 50 || sz.y < 50) return;
        const float PH = 130.0f;                                   // alto del perfil de altura
        ImVec2 szm(sz.x, std::max(60.0f, sz.y - PH));
        ImGui::InvisibleButton("kr_canvas", szm, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
        ImVec2 cen(p0.x + szm.x * 0.5f, p0.y + szm.y * 0.5f);
        ImGuiIO& io = ImGui::GetIO();
        float mxw, mzw; a_mundo(io.MousePos, cen, mxw, mzw);
        // ---- soltar cosas arrastradas sobre el mapa ----
        if (cargado && !c.m.empty()) {
            const ImGuiPayload* dp = ImGui::GetDragDropPayload();
            if (hov && dp && dp->IsDataType("KR_TEX")) ImGui::SetTooltip("Soltar: %s", MT_ETIQ[parte_en(mxw, mzw)]);
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("KR_TEX")) {
                    int parte = parte_en(mxw, mzw);
                    c.tex[parte] = (const char*)pl->Data; slot_tex = parte; tocar();
                    msg = std::string("Textura asignada a: ") + MT_ETIQ[parte];
                }
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("KR_MOD")) poner_objeto((const char*)pl->Data, mxw, mzw);
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("ASSET_IDX")) { if (asset_ruta) poner_objeto(asset_ruta(*(const int*)pl->Data), mxw, mzw); }
                ImGui::EndDragDropTarget();
            }
        }
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(p0, ImVec2(p0.x + szm.x, p0.y + szm.y), true);
        dl->AddRectFilled(p0, ImVec2(p0.x + szm.x, p0.y + szm.y), IM_COL32(28, 30, 34, 255));
        if (!cargado) { dl->PopClipRect(); return; }

        // vista: zoom y desplazamiento
        if (hov && io.MouseWheel != 0) zoom = std::clamp(zoom * std::pow(1.15f, io.MouseWheel), 0.2f, 20.0f);
        if (act && (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))) {
            vx += io.MouseDelta.x / zoom; vz += io.MouseDelta.y / zoom;
        }

        if (!c.m.empty()) {
            // cesped, rejilla cada 20 m
            dl->AddRectFilled(a_pant(c.gx0, c.gz0, cen), a_pant(c.gx1, c.gz1, cen), IM_COL32(48, 82, 44, 255));
            for (float gx = std::ceil(c.gx0 / 20) * 20; gx <= c.gx1; gx += 20)
                dl->AddLine(a_pant(gx, c.gz0, cen), a_pant(gx, c.gz1, cen), IM_COL32(60, 96, 56, 255));
            for (float gz = std::ceil(c.gz0 / 20) * 20; gz <= c.gz1; gz += 20)
                dl->AddLine(a_pant(c.gx0, gz, cen), a_pant(c.gx1, gz, cen), IM_COL32(60, 96, 56, 255));
        }
        // lagos
        for (size_t i = 0; i < c.lagos.size(); i++) {
            const Lago& l = c.lagos[i]; bool on = sel_tipo == 6 && sel == (int)i;
            ImVec2 a = a_pant(l.cx + l.w * 0.5f, l.cz + l.h * 0.5f, cen), b = a_pant(l.cx - l.w * 0.5f, l.cz - l.h * 0.5f, cen);
            dl->AddRectFilled(a, b, IM_COL32(50, 110, 190, 200));
            dl->AddRect(a, b, on ? IM_COL32(255, 230, 120, 255) : IM_COL32(120, 180, 255, 255), 0, 0, on ? 3.0f : 1.5f);
            dl->AddText(ImVec2(std::min(a.x, b.x) + 6, std::min(a.y, b.y) + 4), IM_COL32(220, 235, 255, 255), l.mortal ? "AGUA PROFUNDA" : "AGUA");
        }

        // carreteras: color segun superficie y altura; muros; saltos
        int N = (int)c.m.size();
        for (int i = 0; i < N; i++) {
            int j = c.sig(i); if (j < 0) continue;
            const Muestra &a = c.m[i], &b = c.m[j];
            ImVec2 q[4] = {a_pant(a.der_borde.x, a.der_borde.z, cen), a_pant(b.der_borde.x, b.der_borde.z, cen),
                           a_pant(b.izq_borde.x, b.izq_borde.z, cen), a_pant(a.izq_borde.x, a.izq_borde.z, cen)};
            if (a.hueco) {                                          // salto: linea central discontinua
                if ((i / 2) % 2 == 0) dl->AddLine(a_pant(a.c.x, a.c.z, cen), a_pant(b.c.x, b.c.z, cen), IM_COL32(255, 90, 70, 255), 3.0f);
                continue;
            }
            int g = (int)std::min(210.0f, 90 + a.c.y * 3.0f);
            bool otro = a.camino != cam_sel;
            ImU32 col = a.sup == 1 ? IM_COL32(g + 25, g, g - 35, 255) : a.sup == 2 ? IM_COL32(g - 10, g + 15, g + 45, 255) : a.sup == 3 ? IM_COL32(g - 30, g - 50, g - 70, 255) : IM_COL32(g, g, g + 6, 255);
            if (otro) col = (col & 0x00FFFFFF) | (0xC0u << 24);
            dl->AddQuadFilled(q[0], q[1], q[2], q[3], col);
            if (a.muro_der) dl->AddLine(q[0], q[1], IM_COL32(70, 130, 230, 255), 3.0f);
            if (a.muro_izq) dl->AddLine(q[3], q[2], IM_COL32(70, 130, 230, 255), 3.0f);
            if (a.tipo == T_RECTO) dl->AddLine(a_pant(a.c.x, a.c.z, cen), a_pant(b.c.x, b.c.z, cen), IM_COL32(255, 255, 255, 60), 1.0f);
        }
        // el tramo elegido, resaltado
        if (sel_tipo == 7) for (int i = 0; i < N; i++) {
            if (c.m[i].camino != cam_sel || c.m[i].tramo != sel) continue;
            int j = c.sig(i); if (j < 0) continue;
            dl->AddLine(a_pant(c.m[i].c.x, c.m[i].c.z, cen), a_pant(c.m[j].c.x, c.m[j].c.z, cen), IM_COL32(255, 220, 90, 255), 4.0f);
        }
        // flechas de sentido
        for (int i = 0; i < N; i += 30) {
            const Muestra& s = c.m[i];
            ImVec2 a = a_pant(s.c.x, s.c.z, cen), b = a_pant(s.c.x + s.t.x * 6, s.c.z + s.t.z * 6, cen);
            dl->AddLine(a, b, IM_COL32(230, 230, 230, 120), 2.0f);
            dl->AddCircleFilled(b, 3.0f, IM_COL32(230, 230, 230, 160));
        }
        // rampas y trampolines
        for (size_t i = 0; i < c.rampas.size(); i++) {
            const Rampa& r = c.rampas[i];
            if (c.m.empty()) break;
            int si = c.muestra_cercana(r.x, r.z);
            V3 f = norm(v3(c.m[si].t.x, 0, c.m[si].t.z)), l = v3(f.z, 0, -f.x), o = v3(r.x, 0, r.z);
            V3 k[4] = {o - f * (r.largo / 2) - l * (r.ancho / 2), o + f * (r.largo / 2) - l * (r.ancho / 2),
                       o + f * (r.largo / 2) + l * (r.ancho / 2), o - f * (r.largo / 2) + l * (r.ancho / 2)};
            ImU32 col = (sel_tipo == 2 && sel == (int)i) ? IM_COL32(255, 230, 80, 255) : IM_COL32(235, 190, 30, 230);
            dl->AddQuadFilled(a_pant(k[0].x, k[0].z, cen), a_pant(k[1].x, k[1].z, cen), a_pant(k[2].x, k[2].z, cen), a_pant(k[3].x, k[3].z, cen), col);
        }
        // placas de turbo
        for (size_t i = 0; i < c.turbos.size(); i++) {
            const Turbo& t = c.turbos[i];
            if (c.m.empty()) break;
            int si = c.muestra_cercana(t.x, t.z);
            V3 f = norm(v3(c.m[si].t.x, 0, c.m[si].t.z)), l = v3(f.z, 0, -f.x), o = v3(t.x, 0, t.z);
            V3 k[4] = {o - f * (t.largo / 2) - l * (t.ancho / 2), o + f * (t.largo / 2) - l * (t.ancho / 2),
                       o + f * (t.largo / 2) + l * (t.ancho / 2), o - f * (t.largo / 2) + l * (t.ancho / 2)};
            ImU32 col = (sel_tipo == 5 && sel == (int)i) ? IM_COL32(120, 255, 255, 255) : IM_COL32(30, 200, 230, 235);
            dl->AddQuadFilled(a_pant(k[0].x, k[0].z, cen), a_pant(k[1].x, k[1].z, cen), a_pant(k[2].x, k[2].z, cen), a_pant(k[3].x, k[3].z, cen), col);
        }
        for (auto& t : c.tramps)
            dl->AddRect(a_pant(t.x0, t.z0, cen), a_pant(t.x1, t.z1, cen), IM_COL32(255, 80, 60, 255), 0, 0, 2.0f);
        // objetos: un rombo, el nombre y el asa de giro del elegido
        for (size_t i = 0; i < c.objetos.size(); i++) {
            const Objeto& o = c.objetos[i];
            bool on = sel_tipo == 4 && sel == (int)i;
            ImVec2 s = a_pant(o.x, o.z, cen);
            float r = on ? 8.0f : 6.0f, yr = o.yaw * KPI / 180.0f;
            ImU32 col = on ? IM_COL32(120, 220, 255, 255) : IM_COL32(170, 120, 230, 255);
            dl->AddQuadFilled(ImVec2(s.x, s.y - r), ImVec2(s.x + r, s.y), ImVec2(s.x, s.y + r), ImVec2(s.x - r, s.y), col);
            ImVec2 tip = ImVec2(s.x - std::sin(yr) * 34, s.y - std::cos(yr) * 34);      // +X a la izquierda, +Z arriba
            dl->AddLine(s, tip, col, 2.0f);
            if (on) dl->AddCircleFilled(tip, 6.0f, IM_COL32(255, 220, 90, 255));
            dl->AddText(ImVec2(s.x + 9, s.y - 6), IM_COL32(220, 210, 240, 220), fs::path(o.modelo).stem().string().c_str());
        }
        // luces: un punto amarillo y, la elegida, su alcance
        for (size_t i = 0; i < c.luces.size(); i++) {
            const Luz& l = c.luces[i]; bool on = sel_tipo == 9 && sel == (int)i;
            ImVec2 s = a_pant(l.x, l.z, cen);
            ImU32 col = IM_COL32((int)(l.r * 255), (int)(l.g * 255), (int)(l.b * 255), 255);
            if (on) dl->AddCircle(s, l.radio * zoom, IM_COL32((int)(l.r * 255), (int)(l.g * 255), (int)(l.b * 255), 120), 0, 1.5f);
            dl->AddCircleFilled(s, on ? 7.0f : 5.0f, col);
            dl->AddCircle(s, on ? 7.0f : 5.0f, IM_COL32(30, 30, 30, 255), 0, 1.5f);
            dl->AddText(ImVec2(s.x + 8, s.y - 6), IM_COL32(255, 240, 180, 220), l.noche ? "luz (noche)" : "luz");
        }
        // cruces: una cruz (naranja = puente, morada = tunel)
        for (size_t i = 0; i < c.cruces_det.size(); i++) {
            const CruceDet& cd = c.cruces_det[i]; bool on = sel_tipo == 8 && sel == (int)i;
            ImVec2 s = a_pant(cd.x, cd.z, cen); float r = on ? 12.0f : 9.0f;
            ImU32 col = cd.tipo ? IM_COL32(190, 120, 255, 255) : IM_COL32(255, 160, 50, 255);
            dl->AddCircle(s, r, col, 0, on ? 3.0f : 2.0f);
            dl->AddLine(ImVec2(s.x - r * 0.6f, s.y - r * 0.6f), ImVec2(s.x + r * 0.6f, s.y + r * 0.6f), col, 2.0f);
            dl->AddLine(ImVec2(s.x - r * 0.6f, s.y + r * 0.6f), ImVec2(s.x + r * 0.6f, s.y - r * 0.6f), col, 2.0f);
            dl->AddText(ImVec2(s.x + r + 3, s.y - 7), col, cd.tipo ? "tunel" : "puente");
        }
        // salida
        if (!c.m.empty()) {
            ImVec2 s = a_pant(c.sx, c.sz, cen);
            float yr = c.salida_yaw * KPI / 180.0f;
            ImVec2 tip = a_pant(c.sx + std::sin(yr) * 8, c.sz + std::cos(yr) * 8, cen);
            ImU32 col = sel_tipo == 3 ? IM_COL32(255, 255, 255, 255) : IM_COL32(240, 60, 60, 255);
            dl->AddCircleFilled(s, 6, col); dl->AddLine(s, tip, col, 3.0f);
            dl->AddText(ImVec2(s.x + 8, s.y - 16), col, "SALIDA");
        }
        // nodos y la asa de curva del elegido
        for (size_t ci = 0; ci < c.caminos.size(); ci++) {
            const Camino& cm = c.caminos[ci];
            for (size_t i = 0; i < cm.nodos.size(); i++) {
                const Nodo& nd = cm.nodos[i];
                ImVec2 s = a_pant(nd.x, nd.z, cen);
                bool on = sel_tipo == 1 && cam_sel == (int)ci && sel == (int)i;
                bool act_c = (int)ci == cam_sel;
                ImU32 rel = on ? IM_COL32(255, 200, 60, 255) : (cm.principal ? IM_COL32(250, 250, 250, 255) : IM_COL32(120, 230, 255, 255));
                if (on) {
                    float tx, tz; Circuito::tangente(cm, (int)i, tx, tz);
                    ImVec2 h1 = a_pant(nd.x + tx / 3, nd.z + tz / 3, cen), h2 = a_pant(nd.x - tx / 3, nd.z - tz / 3, cen);
                    dl->AddLine(h2, h1, IM_COL32(255, 200, 60, 200), 1.5f);
                    dl->AddRectFilled(ImVec2(h1.x - 4, h1.y - 4), ImVec2(h1.x + 4, h1.y + 4), IM_COL32(255, 200, 60, 255));
                    dl->AddRectFilled(ImVec2(h2.x - 3, h2.y - 3), ImVec2(h2.x + 3, h2.y + 3), IM_COL32(255, 200, 60, 150));
                }
                dl->AddCircleFilled(s, on ? 8.0f : 6.0f, rel);
                dl->AddCircle(s, on ? 8.0f : 6.0f, IM_COL32(20, 20, 20, 255), 0, 1.5f);
                if (act_c) {
                    char t[48]; snprintf(t, sizeof t, "%d  %.0fm", (int)i, nd.y);
                    dl->AddText(ImVec2(s.x + 8, s.y + 2), IM_COL32(230, 230, 230, 200), t);
                }
            }
        }
        if (hov && herr == 7) {                                     // la goma: un circulo del tamano del trazo
            dl->AddCircle(io.MousePos, goma_px, IM_COL32(255, 90, 90, 255), 0, 2.0f);
            dl->AddCircleFilled(io.MousePos, goma_px, IM_COL32(255, 90, 90, 40));
        }
        dl->PopClipRect();

        // ---- interaccion ----
        auto cerca = [&](float x, float z, float r) {
            ImVec2 s = a_pant(x, z, cen); float dx = s.x - io.MousePos.x, dy = s.y - io.MousePos.y; return dx * dx + dy * dy <= r * r;
        };
        // asa de giro del objeto elegido
        if (sel_tipo == 4 && sel >= 0 && sel < (int)c.objetos.size()) {
            Objeto& o = c.objetos[sel];
            ImVec2 s = a_pant(o.x, o.z, cen); float yr = o.yaw * KPI / 180.0f;
            ImVec2 tip = ImVec2(s.x - std::sin(yr) * 34, s.y - std::cos(yr) * 34);
            float dx = io.MousePos.x - tip.x, dy = io.MousePos.y - tip.y;
            if (hov && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && dx * dx + dy * dy < 100.0f) { push_undo(); girando = true; }
            if (girando) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) { o.yaw = std::atan2(mxw - o.x, mzw - o.z) * 180.0f / KPI; tocar(); }
                else girando = false;
            }
            if (hov && !girando) {
                if (ImGui::IsKeyPressed(ImGuiKey_Q)) { push_undo(); o.yaw += 15; tocar(); }
                if (ImGui::IsKeyPressed(ImGuiKey_E)) { push_undo(); o.yaw -= 15; tocar(); }
                while (o.yaw > 180) o.yaw -= 360; while (o.yaw < -180) o.yaw += 360;
            }
        }
        // pluma: la tangente del nodo recien puesto sigue al raton mientras se arrastra
        if (pen) {
            if (pen_c < (int)c.caminos.size() && pen_n < (int)c.caminos[pen_c].nodos.size()) {
                Nodo& nd = c.caminos[pen_c].nodos[pen_n];
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    ImVec2 s = a_pant(nd.x, nd.z, cen); float dx = io.MousePos.x - s.x, dy = io.MousePos.y - s.y;
                    if (dx * dx + dy * dy > 36.0f) { nd.tx = 3 * (mxw - nd.x); nd.tz = 3 * (mzw - nd.z); tocar(); }
                } else pen = false;
            } else pen = false;
        }
        // asa de curva de un nodo
        if (asa != 0) {
            if (sel_tipo == 1 && cam_sel < (int)c.caminos.size() && sel < (int)c.caminos[cam_sel].nodos.size() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                Nodo& nd = c.caminos[cam_sel].nodos[sel];
                nd.tx = 3.0f * asa * (mxw - nd.x); nd.tz = 3.0f * asa * (mzw - nd.z); tocar();
            } else asa = 0;
        }
        bool libre = !girando && !pen && asa == 0;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { drag_snap = false; trazo_snap = false; }
        if (hov && libre && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (herr != 0 && herr != 7) push_undo();                  // cada clic que crea algo se puede deshacer
            if (herr == 0) {
                bool hecho = false;
                // 1) asa del nodo elegido
                if (sel_tipo == 1 && cam_sel < (int)c.caminos.size() && sel < (int)c.caminos[cam_sel].nodos.size()) {
                    const Camino& cm = c.caminos[cam_sel]; const Nodo& nd = cm.nodos[sel];
                    float tx, tz; Circuito::tangente(cm, sel, tx, tz);
                    if (cerca(nd.x + tx / 3, nd.z + tz / 3, 9)) { push_undo(); asa = 1; hecho = true; }
                    else if (cerca(nd.x - tx / 3, nd.z - tz / 3, 9)) { push_undo(); asa = -1; hecho = true; }
                }
                // 2) un nodo (los del camino activo primero)
                for (int pasada = 0; pasada < 2 && !hecho; pasada++)
                    for (size_t ci = 0; ci < c.caminos.size() && !hecho; ci++) {
                        if ((pasada == 0) != ((int)ci == cam_sel)) continue;
                        for (size_t i = 0; i < c.caminos[ci].nodos.size(); i++)
                            if (cerca(c.caminos[ci].nodos[i].x, c.caminos[ci].nodos[i].z, 10)) { cam_sel = (int)ci; sel = (int)i; sel_tipo = 1; hecho = true; break; }
                    }
                if (!hecho) { sel = -1; sel_tipo = 0; elegir(io.MousePos, cen, mxw, mzw); }
            }
            else if (herr == 1) {                                     // pluma
                if (cam_sel < 0 || cam_sel >= (int)c.caminos.size() || c.caminos[cam_sel].cerrado) {
                    Camino cm; cm.cerrado = false; cm.nombre = "camino" + std::to_string(c.caminos.size()); cm.principal = c.caminos.empty();
                    c.caminos.push_back(cm); cam_sel = (int)c.caminos.size() - 1;
                }
                Camino& cm = c.caminos[cam_sel];
                if (cm.nodos.size() >= 3 && cerca(cm.nodos[0].x, cm.nodos[0].z, 12)) {   // cerrar el camino
                    cm.cerrado = true; sel_tipo = 0; sel = -1; tocar();
                } else {
                    Nodo nd; nd.x = mxw; nd.z = mzw;
                    if (!cm.nodos.empty()) { nd.y = cm.nodos.back().y; nd.ancho = cm.nodos.back().ancho; }
                    if (!cm.nodos.empty()) cm.tramos[cm.nodos.size() - 1].tipo = tipo_nuevo;
                    Tramo t; t.tipo = tipo_nuevo; if (cm.nodos.size() >= 1) { t.sup = cm.tramos.back().sup; t.muro_izq = cm.tramos.back().muro_izq; t.muro_der = cm.tramos.back().muro_der; }
                    cm.nodos.push_back(nd); cm.tramos.push_back(t);
                    pen = true; pen_c = cam_sel; pen_n = (int)cm.nodos.size() - 1;
                    sel_tipo = 1; sel = pen_n; tocar();
                }
            }
            else if (herr == 2) { c.sx = mxw; c.sz = mzw; sel_tipo = 3; sel = 0; tocar(); }
            else if (herr == 3 && !c.m.empty()) {
                int si = c.muestra_cercana(mxw, mzw);
                Rampa r; r.x = c.m[si].c.x; r.z = c.m[si].c.z;
                c.rampas.push_back(r); sel_tipo = 2; sel = (int)c.rampas.size() - 1; tocar();
            }
            else if (herr == 4) poner_objeto(obj_modelo, mxw, mzw);
            else if (herr == 5 && !c.m.empty()) {
                int si = c.muestra_cercana(mxw, mzw);
                Turbo t; t.x = c.m[si].c.x; t.z = c.m[si].c.z;
                c.turbos.push_back(t); sel_tipo = 5; sel = (int)c.turbos.size() - 1; tocar();
            }
            else if (herr == 8) { Luz l; l.x = mxw; l.z = mzw; c.luces.push_back(l); sel_tipo = 9; sel = (int)c.luces.size() - 1; tocar(); }
            else if (herr == 6) {
                Lago l; l.cx = mxw; l.cz = mzw;
                c.lagos.push_back(l); sel_tipo = 6; sel = (int)c.lagos.size() - 1; tocar();
            }
        }
        // doble clic en un tramo: nodo nuevo ahi
        if (hov && herr == 0 && libre && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && sel_tipo == 7 && !c.m.empty()) {
            int mi = -1; float bd = 1e30f;
            for (int i = 0; i < (int)c.m.size(); i++) {
                if (c.m[i].camino != cam_sel || c.m[i].tramo != sel) continue;
                float dx = c.m[i].c.x - mxw, dz = c.m[i].c.z - mzw, d = dx * dx + dz * dz; if (d < bd) { bd = d; mi = i; }
            }
            if (mi >= 0) { push_undo(); dividir(cam_sel, sel, mi); }
        }
        // la goma: mientras se arrastra, borra lo que toca (un solo "deshacer" por trazo)
        if (herr == 7 && hov && libre && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float Rm = goma_px / zoom;
            for (int g = 0; g < 12 && borrar_en(mxw, mzw, Rm); g++) {}
        }
        // terminar la pluma
        if (herr == 1 && hov && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsKeyPressed(ImGuiKey_Enter))) { herr = 0; sel_tipo = 0; sel = -1; }
        if (herr == 0 && libre && act && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && sel >= 0) {
            if (!drag_snap) { push_undo(); drag_snap = true; }          // mover algo: un "deshacer" por arrastre
            if (sel_tipo == 1 && cam_sel < (int)c.caminos.size() && sel < (int)c.caminos[cam_sel].nodos.size()) { c.caminos[cam_sel].nodos[sel].x = mxw; c.caminos[cam_sel].nodos[sel].z = mzw; tocar(); }
            if (sel_tipo == 2 && sel < (int)c.rampas.size()) {
                int si = c.muestra_cercana(mxw, mzw);   // las rampas van pegadas a la carretera
                c.rampas[sel].x = c.m[si].c.x; c.rampas[sel].z = c.m[si].c.z; tocar();
            }
            if (sel_tipo == 3) { c.sx = mxw; c.sz = mzw; tocar(); }
            if (sel_tipo == 4 && sel < (int)c.objetos.size()) { c.objetos[sel].x = mxw; c.objetos[sel].z = mzw; tocar(); }
            if (sel_tipo == 9 && sel < (int)c.luces.size()) { c.luces[sel].x = mxw; c.luces[sel].z = mzw; tocar(); }
            if (sel_tipo == 5 && sel < (int)c.turbos.size()) {
                int si = c.muestra_cercana(mxw, mzw);
                c.turbos[sel].x = c.m[si].c.x; c.turbos[sel].z = c.m[si].c.z; tocar();
            }
            if (sel_tipo == 6 && sel < (int)c.lagos.size()) { c.lagos[sel].cx = mxw; c.lagos[sel].cz = mzw; tocar(); }
        }
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete) && sel >= 0) {
            push_undo();
            if (sel_tipo == 1) borrar_nodo(cam_sel, sel);
            else if (sel_tipo == 7) eliminar_tramo(cam_sel, sel);
            else if (sel_tipo == 2 && sel < (int)c.rampas.size()) { c.rampas.erase(c.rampas.begin() + sel); sel = -1; sel_tipo = 0; tocar(); }
            else if (sel_tipo == 4 && sel < (int)c.objetos.size()) { c.objetos.erase(c.objetos.begin() + sel); sel = -1; sel_tipo = 0; tocar(); }
            else if (sel_tipo == 9 && sel < (int)c.luces.size()) { c.luces.erase(c.luces.begin() + sel); sel = -1; sel_tipo = 0; tocar(); }
            else if (sel_tipo == 5 && sel < (int)c.turbos.size()) { c.turbos.erase(c.turbos.begin() + sel); sel = -1; sel_tipo = 0; tocar(); }
            else if (sel_tipo == 6 && sel < (int)c.lagos.size()) { c.lagos.erase(c.lagos.begin() + sel); sel = -1; sel_tipo = 0; tocar(); }
        }
        if (hov) {
            char t[64]; snprintf(t, sizeof t, "x %.1f  z %.1f", mxw, mzw);
            ImGui::GetWindowDrawList()->AddText(ImVec2(p0.x + 8, p0.y + szm.y - 20), IM_COL32(220, 220, 220, 200), t);
        }
        // ---- perfil de altura ----
        ImVec2 pa(p0.x, p0.y + szm.y + 4), ps_(sz.x, PH - 4);
        dl->PushClipRect(pa, ImVec2(pa.x + ps_.x, pa.y + ps_.y), true);
        ImVec2 mp = io.MousePos;
        perfil(dl, pa, ps_, mp.x >= pa.x && mp.x <= pa.x + ps_.x && mp.y >= pa.y && mp.y <= pa.y + ps_.y && ImGui::IsWindowHovered());
        dl->PopClipRect();
    }

    // elige lo que haya bajo el raton (nodos y asas ya se miraron): objetos, salida, turbos, rampas, tramos y lagos
    void elegir(ImVec2 m, ImVec2 cen, float mxw, float mzw) {
        auto cerca = [&](float x, float z, float r) {
            ImVec2 s = a_pant(x, z, cen); float dx = s.x - m.x, dy = s.y - m.y; return dx * dx + dy * dy <= r * r;
        };
        for (size_t i = 0; i < c.objetos.size(); i++) if (cerca(c.objetos[i].x, c.objetos[i].z, 10)) { sel = (int)i; sel_tipo = 4; return; }
        if (!c.m.empty() && cerca(c.sx, c.sz, 12)) { sel = 0; sel_tipo = 3; return; }
        for (size_t i = 0; i < c.luces.size(); i++) if (cerca(c.luces[i].x, c.luces[i].z, 10)) { sel = (int)i; sel_tipo = 9; return; }
        for (size_t i = 0; i < c.cruces_det.size(); i++) if (cerca(c.cruces_det[i].x, c.cruces_det[i].z, 13)) { sel = (int)i; sel_tipo = 8; return; }
        for (size_t i = 0; i < c.turbos.size(); i++) if (cerca(c.turbos[i].x, c.turbos[i].z, std::max(10.0f, c.turbos[i].largo * 0.5f * zoom))) { sel = (int)i; sel_tipo = 5; return; }
        for (size_t i = 0; i < c.rampas.size(); i++) {
            float r = std::max(10.0f, c.rampas[i].largo * 0.5f * zoom);
            if (cerca(c.rampas[i].x, c.rampas[i].z, r)) { sel = (int)i; sel_tipo = 2; return; }
        }
        // un tramo: la muestra mas cercana dentro del ancho de la carretera (o 8 px)
        int best = -1; float bd = 1e30f;
        for (int i = 0; i < (int)c.m.size(); i++) {
            float dx = c.m[i].c.x - mxw, dz = c.m[i].c.z - mzw, d = dx * dx + dz * dz;
            float lim = std::max(c.m[i].ancho * 0.5f, 8.0f / zoom); if (d <= lim * lim && d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { cam_sel = c.m[best].camino; sel = c.m[best].tramo; sel_tipo = 7; return; }
        for (size_t i = 0; i < c.lagos.size(); i++) {
            const Lago& l = c.lagos[i];
            if (std::fabs(mxw - l.cx) <= l.w * 0.5f && std::fabs(mzw - l.cz) <= l.h * 0.5f) { sel = (int)i; sel_tipo = 6; return; }
        }
    }
};

} // namespace kr
