/*
 * carretera.h - la herramienta CARRETERA de editor3d.
 *
 * Una carretera es SOLO informacion de por donde se puede circular: una lista de nodos (posicion, ancho, peralte)
 * unidos por curvas suaves, y la superficie de cada tramo. No se dibuja en el juego ni modifica el suelo: el aspecto es
 * el del terreno pintado o el modelo (.glb) que el usuario haya puesto, y las reglas (salida, meta, vueltas, rivales...)
 * las programa el usuario con las funciones g3d_road_*.
 *
 * Vive DENTRO de la escena (lineas ROAD / ROADNODE del fichero .scene) y se emite en el juego como codigo g3d_road_*.
 *
 * calcular() es el mismo algoritmo que libmod_3d_road.c (curvas Hermite con tangentes Catmull-Rom, seguimiento del
 * terreno con suavizado y pendiente maxima): el visor del editor tiene que mostrar la carretera que ve el juego. Si se
 * cambia uno, hay que cambiar el otro.
 *
 * Convenciones (las del motor): Y arriba, metros; el rumbo 0 mira a +Z; mirando a +Z, +X queda a la IZQUIERDA. El peralte
 * positivo sube el borde derecho (curvas a la izquierda).
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace car {

// --------------------------------------------------------------------------- vectores
struct V3 { float x = 0, y = 0, z = 0; };
static inline V3 v3(float x, float y, float z) { V3 v; v.x = x; v.y = y; v.z = z; return v; }
static inline V3 operator+(V3 a, V3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V3 operator-(V3 a, V3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V3 operator*(V3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float len(V3 a) { return std::sqrt(dot(a, a)); }
static inline V3 norm(V3 a) { float l = len(a); return l > 1e-9f ? a * (1.0f / l) : a; }
static const float KPI = 3.14159265f;

// --------------------------------------------------------------------------- superficies (etiquetas del tramo)
static const int N_SUP = 4;
static const char* SUP_NOMBRE[N_SUP] = { "Asfalto", "Tierra", "Hielo", "Barro" };

// --------------------------------------------------------------------------- datos
// Un CAMINO es una lista de NODOS unidos por TRAMOS. El tramo i va del nodo i al i+1 (el ultimo vuelve al primero si el
// camino es cerrado). Ancho, altura y peralte viven en los nodos y se interpolan a lo largo de cada tramo.
struct Nodo {
    float x = 0, z = 0, y = 0, ancho = 14, peralte = 0;
    float tx = 0, tz = 0;            // tangente propia; 0,0 = automatica
    int   sigue = 1;                 // 1 = la altura sigue al terreno (terreno + dy); 0 = altura fija (y), p. ej. sobre un modelo
    float dy = 0.3f;                 // metros sobre el terreno cuando sigue
};
struct Tramo { int sup = 0; };       // superficie del tramo que SALE de este nodo
struct Camino {
    std::string nombre = "camino";
    std::vector<Nodo> nodos;
    std::vector<Tramo> tramos;       // siempre tantos como nodos (en un camino abierto el ultimo no se usa)
    bool cerrado = false, principal = false;
    int  ntr() const { int n = (int)nodos.size(); return cerrado ? (n >= 3 ? n : 0) : std::max(0, n - 1); }
    void sincronizar() { tramos.resize(nodos.size()); }
};
struct Muestra {
    V3 c, izq_borde, der_borde, t, izq;   // centro, bordes, tangente, lateral (+X = izquierda con rumbo 0)
    float dist = 0, ancho = 14, peralte = 0;
    int sup = 0, camino = 0;
};
struct Strip { int ini = 0, n = 0; bool cerrado = false; float largo = 0; };   // las muestras de un camino

struct Carreteras {
    std::vector<Camino> caminos;
    std::vector<Muestra> m;
    std::vector<Strip> strips;
    std::function<float(float, float)> terreno_h;     // altura del terreno en (x, z); sin el, las carreteras no siguen al relieve
    float pend_max = 0.40f;                            // pendiente maxima de la carretera que sigue al terreno
    float largo = 0;                                   // longitud del camino principal

    bool vacia() const { for (auto& c : caminos) if (!c.nodos.empty()) return false; return true; }
    void limpiar() { caminos.clear(); m.clear(); strips.clear(); largo = 0; }

    int sig(int i) const {       // muestra siguiente en su camino (-1 si es el final de uno abierto)
        const Strip& s = strips[m[i].camino]; int k = i - s.ini;
        return k + 1 < s.n ? i + 1 : (s.cerrado ? s.ini : -1);
    }
    int ant(int i) const {
        const Strip& s = strips[m[i].camino]; int k = i - s.ini;
        return k > 0 ? i - 1 : (s.cerrado ? s.ini + s.n - 1 : -1);
    }
    // la muestra de carretera mas cercana; solo = camino concreto o -1 para todos
    int muestra_cercana(float x, float z, int solo = -1) const {
        int best = -1; float bd = 1e30f;
        for (size_t i = 0; i < m.size(); i++) {
            if (solo >= 0 && m[i].camino != solo) continue;
            float dx = m[i].c.x - x, dz = m[i].c.z - z, d = dx * dx + dz * dz;
            if (d < bd) { bd = d; best = (int)i; }
        }
        return best < 0 ? 0 : best;
    }
    static float yaw_de(V3 t) { return std::atan2(t.x, t.z) * 180.0f / KPI; }   // adelante = (sin yaw, cos yaw)

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

    // Calcula la linea central y los bordes de cada camino. Mismo algoritmo que libmod_3d_road.c.
    void calcular() {
        m.clear(); strips.clear(); largo = 0;
        const V3 UP = v3(0, 1, 0);
        struct Raw { float x, y, z, b, w; int sup; int sg; float dy; };
        for (size_t ci = 0; ci < caminos.size(); ci++) {
            Camino& c = caminos[ci]; c.sincronizar();
            Strip st; st.ini = (int)m.size(); st.cerrado = c.cerrado && c.nodos.size() >= 3;
            std::vector<Raw> raw;
            int n = (int)c.nodos.size(), ntr = c.ntr();
            for (int k = 0; k < ntr; k++) {
                const Nodo &A = c.nodos[k], &B = c.nodos[(k + 1) % n]; const Tramo& T = c.tramos[k];
                float tax, taz, tbx, tbz; tangente(c, k, tax, taz); tangente(c, (k + 1) % n, tbx, tbz);
                float chord = std::hypot(B.x - A.x, B.z - A.z);
                int ns = std::max(2, (int)std::ceil(chord * 1.2f / 2.0f));
                int last = (!st.cerrado && k == ntr - 1) ? ns : ns - 1;      // un camino abierto acaba en su ultimo nodo
                for (int q = 0; q <= last; q++) {
                    float t = (float)q / ns, u = t * t * (3 - 2 * t);
                    float h00 = 2 * t * t * t - 3 * t * t + 1, h10 = t * t * t - 2 * t * t + t, h01 = -2 * t * t * t + 3 * t * t, h11 = t * t * t - t * t;
                    Raw r;
                    r.x = h00 * A.x + h10 * tax + h01 * B.x + h11 * tbx;
                    r.z = h00 * A.z + h10 * taz + h01 * B.z + h11 * tbz;
                    r.y = A.y + (B.y - A.y) * u;
                    r.sg = (terreno_h && A.sigue && B.sigue) ? 1 : 0; r.dy = A.dy + (B.dy - A.dy) * u;
                    r.b = A.peralte + (B.peralte - A.peralte) * u;
                    r.w = std::max(4.0f, A.ancho + (B.ancho - A.ancho) * u);
                    r.sup = std::clamp(T.sup, 0, N_SUP - 1);
                    raw.push_back(r);
                }
            }
            int N = (int)raw.size(); st.n = N;
            if (N < 2) { st.n = 0; strips.push_back(st); continue; }
            // la carretera pegada al terreno: la mas alta de la huella entera, suavizada a lo largo
            if (terreno_h) {
                std::vector<float> hm(N, 0.0f);
                for (int i = 0; i < N; i++) {
                    if (!raw[i].sg) continue;
                    int ip = st.cerrado ? (i - 1 + N) % N : std::max(0, i - 1), in = st.cerrado ? (i + 1) % N : std::min(N - 1, i + 1);
                    float tx = raw[in].x - raw[ip].x, tz = raw[in].z - raw[ip].z, l = std::hypot(tx, tz); if (l < 1e-4f) l = 1;
                    float lx = tz / l, lz = -tx / l, w = raw[i].w * 0.5f;
                    float fx_ = tx / l, fz_ = tz / l, mx = -1e30f;
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
                    raw[i].y = std::max(cnt ? acc / cnt : hm[i], hm[i]) + raw[i].dy;
                }
                auto ds = [&](int i, int j) { return std::max(0.5f, std::hypot(raw[j].x - raw[i].x, raw[j].z - raw[i].z)); };
                for (int it = 0; it < 3; it++) {            // solo se SUBE lo bajo: la carretera nunca queda enterrada
                    for (int q = 0; q < (st.cerrado ? 2 * N : N - 1); q++) {
                        int i = q % N, j = (i + 1) % N; if (!st.cerrado && j == 0) break;
                        if (raw[i].sg && raw[j].sg) raw[j].y = std::max(raw[j].y, raw[i].y - pend_max * ds(i, j));
                    }
                    for (int q = 0; q < (st.cerrado ? 2 * N : N - 1); q++) {
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
                V3 izq = norm(v3(t.z, 0, -t.x));                       // +X con rumbo +Z = izquierda
                float b = r.b * KPI / 180.0f, w = r.w * 0.5f;
                Muestra s;
                s.c = v3(r.x, r.y + std::fabs(std::sin(b)) * w, r.z);   // el borde bajo no se hunde
                s.t = t; s.izq = izq; s.ancho = r.w; s.sup = r.sup; s.camino = (int)ci; s.peralte = r.b;
                s.der_borde = s.c - izq * (std::cos(b) * w) + UP * (std::sin(b) * w);
                s.izq_borde = s.c + izq * (std::cos(b) * w) - UP * (std::sin(b) * w);
                s.dist = i == 0 ? 0 : m.back().dist + len(s.c - m.back().c);
                m.push_back(s);
            }
            st.largo = st.cerrado ? m.back().dist + len(m[st.ini].c - m.back().c) : m.back().dist;
            strips.push_back(st);
        }
        for (size_t ci = 0; ci < caminos.size(); ci++) if (strips[ci].n >= 2) { largo = strips[ci].largo; break; }
    }

    // inclina cada curva hacia dentro segun lo cerrada que sea
    void peralte_auto(int ci, float k = 18.0f, float maxp = 25.0f) {
        if (ci < 0 || ci >= (int)caminos.size()) return;
        Camino& c = caminos[ci]; int n = (int)c.nodos.size(); if (n < 3) return;
        for (int i = 0; i < n; i++) {
            if (!c.cerrado && (i == 0 || i == n - 1)) { c.nodos[i].peralte = 0; continue; }
            const Nodo &a = c.nodos[(i - 1 + n) % n], &b = c.nodos[i], &d = c.nodos[(i + 1) % n];
            V3 d0 = v3(b.x - a.x, 0, b.z - a.z), d1 = v3(d.x - b.x, 0, d.z - b.z);
            float l0 = len(d0), l1 = len(d1); if (l0 < 1e-3f || l1 < 1e-3f) continue;
            float dyaw = yaw_de(d1) - yaw_de(d0);
            while (dyaw > 180) dyaw -= 360;
            while (dyaw < -180) dyaw += 360;
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

    // ---- en el fichero de escena: ROAD <cerrada> y debajo sus ROADNODE (mismo formato que lee el motor) ----
    void escribir(FILE* f) const {
        for (auto& c : caminos) {
            if (c.nodos.size() < 2) continue;
            fprintf(f, "ROAD %d\n", c.cerrado ? 1 : 0);
            for (size_t i = 0; i < c.nodos.size(); i++) {
                const Nodo& n = c.nodos[i];
                fprintf(f, "ROADNODE %.3f %.3f %.3f %.3f %.3f %d %.3f %d %.3f %.3f\n", n.x, n.z, n.y, n.ancho, n.peralte, n.sigue ? 1 : 0, n.dy,
                        i < c.tramos.size() ? c.tramos[i].sup : 0, n.tx, n.tz);
            }
        }
    }
    // Una linea del fichero de escena; devuelve true si era de la carretera.
    bool leer_linea(const char* line) {
        if (!strncmp(line, "ROADNODE", 8)) {
            if (caminos.empty()) return true;
            Nodo n; int sg = 1, sp = 0;
            float tx = 0, tz = 0;
            int k = sscanf(line, "ROADNODE %f %f %f %f %f %d %f %d %f %f", &n.x, &n.z, &n.y, &n.ancho, &n.peralte, &sg, &n.dy, &sp, &tx, &tz);
            if (k >= 8) {
                n.sigue = sg != 0; if (k >= 10) { n.tx = tx; n.tz = tz; }
                Tramo t; t.sup = std::clamp(sp, 0, N_SUP - 1);
                caminos.back().nodos.push_back(n); caminos.back().tramos.push_back(t);
            }
            return true;
        }
        if (!strncmp(line, "ROAD", 4)) {
            int cer = 1; sscanf(line, "ROAD %d", &cer);
            Camino c; c.cerrado = cer != 0; c.principal = caminos.empty(); c.nombre = "camino" + std::to_string(caminos.size());
            caminos.push_back(c);
            return true;
        }
        return false;
    }
};

// --------------------------------------------------------------------------- estado del editor
struct EditorCarretera {
    Carreteras c;
    bool sucio = false;                       // hay cambios sin guardar en la escena
    std::vector<std::vector<Camino>> deshacer, rehacer;

    void tocar() { sucio = true; }
    void push_undo() { deshacer.push_back(c.caminos); if (deshacer.size() > 60) deshacer.erase(deshacer.begin()); rehacer.clear(); }
    void tras_historial() { c.calcular(); sucio = true; }
    bool do_undo() { if (deshacer.empty()) return false; rehacer.push_back(c.caminos); c.caminos = deshacer.back(); deshacer.pop_back(); tras_historial(); return true; }
    bool do_redo() { if (rehacer.empty()) return false; deshacer.push_back(c.caminos); c.caminos = rehacer.back(); rehacer.pop_back(); tras_historial(); return true; }
    void vaciar() { c.limpiar(); deshacer.clear(); rehacer.clear(); sucio = false; }
};

} // namespace car
