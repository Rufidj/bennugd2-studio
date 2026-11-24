# Documentacion de libmod_sound. Es una envoltura de SDL_mixer: bucles, canales, rangos y retornos verificados en libbgsound.c.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e

# Conceptos: MUSICA = una pista (una a la vez); SONIDO = un efecto corto (varios a la vez, cada uno en un CANAL).
# Bucles: 0 = una vez, n = se repite n veces mas, -1 = sin fin. Canal -1 = el primer canal libre.
# Volumen: 0 a 128.
D("SOUNDSYS_INIT","Inicia el sistema de sonido. No hace falta llamarla: se inicia sola al cargar el primer sonido o musica. Antes de iniciarlo se pueden fijar sound.freq, sound.mode y sound.channels.",ret="0 si va bien, -1 si falla")
D("SOUNDSYS_QUIT","Cierra el sistema de sonido y libera sus recursos.")

# ---- musica ----
D("MUSIC_LOAD","Carga una musica (MP3, OGG, WAV, MOD...) desde un fichero. Devuelve un identificador para el resto de funciones MUSIC_*; 0 si no se pudo cargar. Inicia el sonido si hacia falta.",["fichero"],"identificador de la musica, 0 si falla",
  "m = music_load(\"Assets/Music/tema.ogg\");\nmusic_play(m);   // sin fin",sig="S")
D("MUSIC_LOAD","Carga una musica EN SEGUNDO PLANO, sin parar el juego: el identificador se guarda en la variable que se pasa por puntero cuando termina.",["fichero","&identificador"],"",sig="SP")
D("MUSIC_UNLOAD","Libera una musica cargada.",["musica"],sig="I")
D("MUSIC_UNLOAD","Libera una musica cargada con carga en segundo plano, y deja su variable a 0.",["&musica"],sig="P")
D("MUSIC_PLAY","Reproduce una musica repitiendola 'bucles' veces (0 = una vez, -1 = sin fin). Solo suena una musica a la vez: empezar otra corta la anterior.",["musica","bucles"],"0 si va bien, -1 si hay error",sig="II")
D("MUSIC_PLAY","Reproduce una musica en bucle sin fin.",["musica"],"0 si va bien, -1 si hay error",sig="I")
D("MUSIC_STOP","Para la musica que suena.")
D("MUSIC_PAUSE","Pausa la musica.")
D("MUSIC_RESUME","Reanuda una musica pausada.")
D("MUSIC_REWIND","Vuelve al principio de la musica.")
D("MUSIC_FADE_IN","Empieza una musica subiendo el volumen poco a poco durante 'ms' milisegundos.",["musica","bucles","ms"],"0 si va bien, -1 si hay error",sig="III")
D("MUSIC_FADE_IN","Igual que la anterior, empezando en un punto de la musica ('posicion' en segundos).",["musica","bucles","ms","posicion"],"0 si va bien, -1 si hay error",sig="IIID")
D("MUSIC_FADE_OFF","Baja el volumen de la musica poco a poco durante 'ms' milisegundos y la para.",["ms"])
D("MUSIC_IS_PLAYING","Indica si hay una musica sonando.",ret="1 si suena")
D("MUSIC_IS_PAUSED","Indica si la musica esta en pausa.",ret="1 si esta en pausa")
D("MUSIC_IS_FADING","Indica si la musica esta subiendo o bajando de volumen.",ret="distinto de 0 si esta en un fundido")
D("MUSIC_SET_VOLUME","Volumen de la musica, de 0 a 128 (los valores fuera de rango se recortan).",["volumen"],ret="0 si va bien, -1 si el sonido no esta iniciado",ejemplo="music_set_volume(64);   // la mitad")
D("MUSIC_GET_VOLUME","Volumen de una musica (necesita SDL_mixer 2.6 o mas reciente; con una anterior devuelve -1).",["musica"],"volumen")
D("MUSIC_SET_PLAYBACK_POSITION","Salta a un punto de la musica, en segundos.",["segundos"])
D("MUSIC_SET_TIME_POSITION","Alias de MUSIC_SET_PLAYBACK_POSITION.",["segundos"])
D("MUSIC_GET_PLAYBACK_POSITION","Posicion actual de la musica, en segundos.",["musica"],"segundos")
D("MUSIC_GET_TIME_POSITION","Alias de MUSIC_GET_PLAYBACK_POSITION.",["musica"],"segundos")
D("MUSIC_GET_DURATION","Duracion de una musica, en segundos.",["musica"],"segundos",sig="I")
D("MUSIC_GET_DURATION","Duracion de la musica que suena, en segundos.",[],"segundos",sig="")
for n,t in [("START","comienzo"),("END","final"),("LEN","longitud")]:
    D("MUSIC_GET_LOOP_"+n,"Segundo en que esta el %s del bucle propio de la musica, si el fichero lo define."%t if n!="LEN" else "Duracion del bucle propio de la musica, en segundos, si el fichero lo define.",["musica"],"segundos",sig="I")
    D("MUSIC_GET_LOOP_"+n,"Lo mismo para la musica que suena.",[],"segundos",sig="")
# etiquetas
for n,t in [("TYPE","tipo de formato"),("TITLE","titulo"),("TITLE_TAG","etiqueta de titulo"),("ARTIST_TAG","etiqueta de artista"),("ALBUM_TAG","etiqueta de album"),("COPYRIGHT_TAG","etiqueta de copyright")]:
    D("MUSIC_GET_"+n,"Devuelve el %s de una musica, segun lo que traiga el fichero."%t,["musica"],"el valor",sig="I")
    D("MUSIC_GET_"+n,"Devuelve el %s de la musica que suena."%t,[],"el valor",sig="")
D("GET_NUM_MUSIC_DECODERS","Numero de decodificadores de musica disponibles (formatos que se pueden leer).",ret="cantidad")
D("GET_MUSIC_DECODER","Nombre de un decodificador de musica, por su indice.",["indice"],"el nombre")
D("HAS_MUSIC_DECODER","Indica si hay un decodificador con ese nombre.",["nombre"],"1 si existe")

# ---- sonidos (efectos) ----
D("SOUND_LOAD","Carga un efecto de sonido (WAV, OGG...) desde un fichero. Devuelve un identificador para SOUND_PLAY; 0 si no se pudo cargar.",["fichero"],"identificador del sonido, 0 si falla",
  "s = sound_load(\"Assets/Sounds/salto.wav\");\nsound_play(s);",sig="S")
D("SOUND_LOAD","Carga un efecto de sonido EN SEGUNDO PLANO: el identificador se guarda en la variable que se pasa por puntero cuando termina.",["fichero","&identificador"],"",sig="SP")
D("SOUND_UNLOAD","Libera un sonido cargado.",["sonido"],sig="I")
D("SOUND_UNLOAD","Libera un sonido cargado con carga en segundo plano y deja su variable a 0.",["&sonido"],sig="P")
D("SOUND_PLAY","Reproduce un sonido en el primer canal libre, repitiendolo 'bucles' veces (0 = una vez, -1 = sin fin). Se pueden oir varios a la vez, cada uno en su canal.",["sonido","bucles"],"el canal usado, o -1 si no hay ninguno libre",sig="II")
D("SOUND_PLAY","Reproduce un sonido una vez en el primer canal libre.",["sonido"],"el canal usado, o -1 si no hay ninguno libre",sig="I")
D("SOUND_PLAY","Reproduce un sonido en un canal concreto (-1 = el primero libre).",["sonido","bucles","canal"],"el canal usado, o -1",sig="III")
D("SOUND_PLAY","Reproduce un sonido en un canal y lo corta a los 'ms' milisegundos aunque no haya acabado.",["sonido","bucles","canal","ms"],"el canal usado, o -1",sig="IIII")
D("SOUND_STOP","Para el sonido de un canal (-1 = todos).",["canal"])
D("SOUND_PAUSE","Pausa el sonido de un canal (-1 = todos).",["canal"])
D("SOUND_RESUME","Reanuda el sonido de un canal pausado (-1 = todos).",["canal"])
D("SOUND_FADE_IN","Reproduce un sonido en un canal subiendo el volumen durante 'ms' milisegundos.",["sonido","bucles","ms","canal"],"el canal usado, o -1",sig="IIII")
D("SOUND_FADE_IN","Igual, y ademas lo corta a los 'tiempo' milisegundos.",["sonido","bucles","ms","canal","tiempo"],"el canal usado, o -1",sig="IIIII")
D("SOUND_FADE_OFF","Baja el volumen del sonido de un canal durante 'ms' milisegundos y lo para.",["canal","ms"])
D("SOUND_IS_PLAYING","Indica si suena algo en un canal (-1 = cuenta los canales que suenan).",["canal"],"1 si suena (o cuantos, con -1)")
D("SOUND_IS_PAUSED","Indica si el sonido de un canal esta en pausa.",["canal"],"1 si esta en pausa")
D("SOUND_IS_FADING","Indica si un canal esta en un fundido.",["canal"],"distinto de 0 si esta en un fundido")
D("SOUND_SET_VOLUME","Volumen propio de un sonido, de 0 a 128. Se aplica cada vez que suena.",["sonido","volumen"],sig="II")
D("SOUND_SET_VOLUME","Volumen de TODOS los canales, de 0 a 128.",["volumen"],sig="I")
D("CHANNEL_SET_VOLUME","Volumen de un canal, de 0 a 128.",["canal","volumen"],sig="II")
D("CHANNEL_SET_VOLUME","Volumen de todos los canales, de 0 a 128.",["volumen"],sig="I")
D("SET_MASTER_VOLUME","Volumen general de todo el sonido, de 0 a 128.",["volumen"],ret="0 si va bien, -1 si el sonido no esta iniciado")
D("CHANNEL_SET_EXPIRE","Hace que un canal se pare solo pasados 'ms' milisegundos (-1 = sin limite).",["canal","ms"],sig="II")
D("CHANNEL_SET_EXPIRE","Hace que todos los canales se paren solos pasados 'ms' milisegundos.",["ms"],sig="I")
D("RESERVE_CHANNELS","Reserva los 'cantidad' primeros canales para que no se usen como libres (para sonidos que tu eliges el canal). Devuelve cuantos se reservaron.",["cantidad"],"canales reservados")
# espacial: solo valen mientras el canal suena
ESP=" Solo funciona mientras el canal esta sonando: si no suena nada en ese canal devuelve -1 y no hace nada."
D("CHANNEL_SET_PANNING","Reparto izquierda/derecha de un canal: volumen de cada lado, de 0 a 255 (255 y 255 = centrado, 255 y 0 = solo izquierda)."+ESP,["canal","izquierda","derecha"],"0 si va bien, -1 si no")
D("SET_PANNING","Alias de CHANNEL_SET_PANNING.",["canal","izquierda","derecha"],"0 si va bien, -1 si no")
D("CHANNEL_SET_DISTANCE","Distancia de un canal, de 0 (cerca, a todo volumen) a 255 (lejos, casi inaudible)."+ESP,["canal","distancia"],"0 si va bien, -1 si no")
D("SET_DISTANCE","Alias de CHANNEL_SET_DISTANCE.",["canal","distancia"],"0 si va bien, -1 si no")
D("SOUND_SET_LOCATION","Sonido posicional: angulo (0 a 360, 0 = delante, 90 = derecha) y distancia (0 a 255) de un canal respecto al oyente."+ESP,["canal","angulo","distancia"],"0 si va bien, -1 si no")
D("SOUND_SET_SPATIAL_POSITION","Alias de SOUND_SET_LOCATION.",["canal","angulo","distancia"],"0 si va bien, -1 si no")
D("SET_POSITION","Alias de SOUND_SET_LOCATION.",["canal","angulo","distancia"],"0 si va bien, -1 si no")
D("REVERSE_STEREO","Invierte (1) o no (0) los canales izquierdo y derecho de un canal."+ESP,["canal","invertir"],"0 si va bien, -1 si no")
# grupos
D("SOUND_GROUP_CHANNEL","Mete un canal en un grupo (una etiqueta numerica), para tratar varios canales a la vez.",["canal","grupo"])
D("SOUND_GROUP_CHANNELS","Mete un rango de canales en un grupo.",["desde","hasta","grupo"],"cuantos canales se agruparon")
D("SOUND_GROUP_AVAILABLE","Primer canal libre de un grupo, o -1 si no hay.",["grupo"],"canal o -1")
D("SOUND_GROUP_COUNT","Cuantos canales tiene un grupo.",["grupo"],"cantidad")
D("SOUND_GROUP_OLDEST","El canal del grupo que lleva mas tiempo sonando.",["grupo"],"canal o -1")
D("SOUND_GROUP_NEWER","El canal del grupo que empezo a sonar hace menos.",["grupo"],"canal o -1")
D("SOUND_GROUP_HALT","Para todos los sonidos de un grupo.",["grupo"])
D("SOUND_GROUP_FADE_OUT","Baja el volumen de todos los sonidos de un grupo durante 'ms' milisegundos y los para.",["grupo","ms"])
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
