# Documentacion de libmod_video (reproduccion de video con FFmpeg). Verificado en libmod_video.c y su README.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e

D("VIDEO_OPEN","Abre un video o un audio (MP4/H.264, WebM/VP9, MKV, AVI, OGV, MP3... lo que FFmpeg sepa decodificar; tambien direcciones de internet) y devuelve su manejador (0 si falla). Para verlo hace falta VIDEO_GRAPH.",["fichero"],"manejador del video",
  "v = video_open(\"intro.mp4\");\ng = video_graph(v);\nvideo_play(v, 0);\ngraph = g; x = 640; y = 360;   // un proceso que muestra el video\nWHILE (!video_is_finished(v)) FRAME; END\nvideo_close(v);")
D("VIDEO_OPEN_H","Abre un video enviando cabeceras HTTP extra (\"Cabecera: valor\\r\\n\"): hace falta con servidores que piden cuenta (Authorization, Cookie...).",["direccion","cabeceras"],"manejador del video")
D("VIDEO_GRAPH","Grafico que muestra siempre el fotograma actual del video. Se usa como cualquier grafico (la variable graph de un proceso). Al cerrar el video, el grafico se libera unos 30 fotogramas despues.",["video"],"codigo del grafico")
D("VIDEO_PLAY","Empieza a reproducir el video; con repetir = 1 lo hace en bucle, con 0 una sola vez.",["video","repetir"],ejemplo="video_play(v, 1);   // en bucle")
D("VIDEO_PAUSE","Pausa (1) o reanuda (0) el video.",["video","pausa"])
D("VIDEO_STOP","Para el video.",["video"])
D("VIDEO_SEEK","Salta a un punto del video, en milisegundos.",["video","ms"])
D("VIDEO_CLOSE","Cierra un video y libera sus recursos.",["video"])
D("VIDEO_WIDTH","Ancho del video en pixeles.",["video"],"ancho")
D("VIDEO_HEIGHT","Alto del video en pixeles.",["video"],"alto")
D("VIDEO_DURATION","Duracion del video en milisegundos.",["video"],"milisegundos")
D("VIDEO_POSITION","Posicion actual del video en milisegundos. Es el reloj de la reproduccion: sigue avanzando aunque la decodificacion vaya con retraso.",["video"],"milisegundos")
D("VIDEO_IS_PLAYING","Indica si el video se esta reproduciendo.",["video"],"1 si se reproduce")
D("VIDEO_IS_FINISHED","Indica si el video ha llegado al final.",["video"],"1 si ha terminado")
D("VIDEO_VOLUME","Volumen del video, de 0 a 128. El audio sale por el mismo dispositivo que la musica de BennuGD, y mientras un video tiene el audio sustituye a MUSIC_PLAY (solo un video a la vez lo tiene).",["video","volumen"])
D("VIDEO_IS_AUDIO_ONLY","Indica si el fichero no tiene imagen (es solo musica).",["video"],"1 si es solo audio")
D("VIDEO_SPEED","Velocidad de reproduccion en porcentaje, de 25 a 300 (100 = normal). Mantiene la posicion; el sonido se reinicia desde ella.",["video","porcentaje"])
D("VIDEO_GET_SPEED","Velocidad de reproduccion actual en porcentaje.",["video"],"porcentaje")
D("VIDEO_FRAMES","Fotogramas realmente mostrados hasta ahora (los que llegan tarde se descartan para no perder el reloj). Es el numero con el que comparar rendimientos.",["video"],"fotogramas")

# ---- subtitulos ----
D("VIDEO_SUBTITLE_TRACKS","Cuantas pistas de subtitulos de texto hay (las del fichero y las cargadas aparte). Se ignoran las que son imagenes (PGS, DVB).",["video"],"cantidad")
D("VIDEO_SUBTITLE_LANG","Nombre y idioma de una pista de subtitulos (empezando en 0).",["video","pista"],"el nombre, por ejemplo \"spa - Castellano\"")
D("VIDEO_SUBTITLE_SELECT","Elige la pista de subtitulos a mostrar; -1 los quita (es lo que hay por defecto).",["video","pista"])
D("VIDEO_SUBTITLE_LOAD","Carga un fichero de subtitulos externo (.srt, .ass, .ssa, .vtt...) como una pista mas, que queda elegida.",["video","fichero"])
D("VIDEO_SUBTITLE_TEXT","Texto del subtitulo que toca ahora (vacio si no hay ninguno). El modulo NO lo dibuja: se escribe con WRITE y la fuente que quieras.",["video"],"el texto",
  "write_var(0, 640, 680, ALIGN_BOTTOM, texto);   // texto = video_subtitle_text(v) cada frame")
D("VIDEO_SUB_DELAY","Retrasa los subtitulos tantos milisegundos (negativo: los adelanta).",["video","ms"])

# ---- audio y sincronia ----
D("VIDEO_AUDIO_TRACKS","Cuantas pistas de audio tiene el video.",["video"],"cantidad")
D("VIDEO_AUDIO_LANG","Nombre y idioma de una pista de audio (empezando en 0).",["video","pista"],"el nombre")
D("VIDEO_AUDIO_SELECT","Cambia a otra pista de audio (empezando en 0). Puede oirse un pequeno corte.",["video","pista"])
D("VIDEO_AV_OFFSET","Muestra la imagen tantos milisegundos mas tarde (negativo: mas pronto), para corregir un sonido desincronizado.",["video","ms"])
D("VIDEO_SPECTRUM","Nivel, de 0 a 255, de una de las 32 bandas de frecuencia (de 40 Hz a 16 kHz, repartidas en escala logaritmica) de lo que suena ahora; 0 si esta en pausa. Sirve para hacer un ecualizador.",["video","banda"],"nivel de 0 a 255")
D("VIDEO_WAVE","Muestra de la onda que suena ahora, con signo (-128 a 127), para un osciloscopio. El indice va de 0 a 255.",["video","indice"],"valor de -128 a 127")
D("VIDEO_CHAPTERS","Cuantos capitulos tiene el video.",["video"],"cantidad")
D("VIDEO_CHAPTER_START","Donde empieza un capitulo (empezando en 0).",["video","capitulo"],"posicion")
D("VIDEO_CHAPTER_TITLE","Titulo de un capitulo (empezando en 0).",["video","capitulo"],"el titulo")

# ---- decodificacion (ajustes avanzados) ----
D("VIDEO_USE_GPU","Conversion del color en la GPU para los videos que se abran despues (1 por defecto). Con 0 se hace por software. Solo vale para video yuv420p de 8 bits; con otros formatos, o si el motor grafico no puede, se usa el software solo.",["activar"],"el valor fijado")
D("VIDEO_GPU","Indica si un video se esta convirtiendo en la GPU.",["video"],"1 si usa la GPU")
D("VIDEO_USE_HW","Decodificador de video de la consola (PS5) para los videos que se abran despues (1 por defecto). Si no puede con un video, o falla, se pasa solo a libavcodec desde la posicion actual.",["activar"],"el valor fijado")
D("VIDEO_HW","Indica que decodificador usa un video: 1 el de la consola, 2 el simulado (en PC con LIBMOD_VIDEO_FAKE_HW=1), 0 software.",["video"],"0, 1 o 2")
D("VIDEO_USE_PLANE","Ajuste interno de decodificacion para los videos que se abran despues.",["activar"],"el valor fijado")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
