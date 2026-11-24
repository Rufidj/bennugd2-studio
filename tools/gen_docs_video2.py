# Documentacion de los addons de red de libmod_video: HTTP, descarga de video, torrents, subida desde el navegador,
# DLNA, FTP y WebDAV. Comprobado contra libmod_video.c, torrent.h, upload.h y dlna.h.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- HTTP ----
HT = " Solo admite direcciones http:// y https://. Espera como maximo 40 s en total y lee hasta 8 MB de respuesta."
D("HTTP_FETCH","Hace una peticion HTTP y ESPERA a la respuesta (el programa se queda parado mientras tanto: para no bloquear usa HTTP_FETCH_BEGIN). La respuesta se guarda en memoria y se lee con HTTP_LENGTH, HTTP_READ o HTTP_SAVE." + HT + " cabeceras son lineas \"Nombre: valor\" separadas por salto de linea (o cadena vacia); metodo puede ser \"GET\", \"POST\"... (vacio = GET); si das cuerpo y no pones metodo, se usa POST.",
  ["url","cabeceras","metodo","cuerpo"],"codigo HTTP (200 = bien, 400, 401, 403, 404, 500) o 0 si no hubo conexion (HTTP_ERROR dice por que)",
  "if (http_fetch(\"https://ejemplo.com/datos.json\", \"\", \"GET\", \"\") == 200)\n    n = http_length();\n    say(\"recibidos \" + n + \" bytes\");\nelse\n    say(\"error: \" + http_error());\nend")
D("HTTP_FETCH_BEGIN","Igual que HTTP_FETCH pero en segundo plano: vuelve enseguida y el programa sigue funcionando. Luego se llama a HTTP_FETCH_POLL cada frame; HTTP_PROGRESS y HTTP_TOTAL permiten dibujar una barra. Solo una peticion a la vez." + HT,
  ["url","cabeceras","metodo","cuerpo"],"0 si arranco, -1 si ya habia una en curso, -2 sin memoria, -3 si no pudo crear el hilo")
D("HTTP_FETCH_POLL","Consulta la peticion lanzada con HTTP_FETCH_BEGIN.",[],"-1 mientras sigue, el codigo HTTP cuando termina (el mismo que HTTP_FETCH), -3 si no se habia lanzado ninguna")
D("HTTP_PROGRESS","Bytes recibidos hasta ahora de la peticion en curso.",[],"bytes")
D("HTTP_TOTAL","Tamano total que anuncio el servidor para la respuesta en curso, o 0 si no lo dijo.",[],"bytes")
D("HTTP_LENGTH","Tamano en bytes de la ultima respuesta recibida.",[],"bytes")
D("HTTP_READ","Copia hasta max bytes de la ultima respuesta en un buffer propio (por ejemplo un array de bytes).",["&buffer","max"],"bytes copiados")
D("HTTP_SAVE","Guarda la ultima respuesta en un archivo del disco.",["ruta"],"1 si se guardo, 0 si no")
D("HTTP_WRITE","Escribe un texto en un archivo del disco (sirve para guardar datos sin pasar por una peticion).",["ruta","texto"],"1 si se escribio, 0 si no")
D("HTTP_ERROR","Texto con el motivo del ultimo fallo de red (vacio si no hubo).",[],"texto")
D("HTTP_TRACE","Para depurar cuelgues: con el texto \"PATH:ruta\" fija un archivo de registro; cualquier otro texto se anade como una linea a ese archivo y se fuerza a disco.",["texto"],"1")
D("WEBDAV_PARSE","Interpreta la respuesta de la ultima peticion (un PROPFIND de WebDAV hecho con HTTP_FETCH) como una lista de carpetas y archivos. Se pasa la ruta de la carpeta pedida para no incluirla a ella misma. Devuelve lineas separadas por tabuladores: carpetas \"C<TAB>href<TAB>nombre<TAB>0\" y archivos \"I<TAB>href<TAB>nombre<TAB>bytes<TAB>0<TAB>v\".",["ruta_carpeta"],"texto con una linea por entrada")

# ---- Descarga de video ----
DV = " Una sola descarga a la vez. Se hace en segundo plano; consulta VIDEO_DOWNLOAD_POLL cada frame."
D("VIDEO_DOWNLOAD_BEGIN","Descarga un video (lista HLS, archivo o direccion) a un solo .mp4 SIN recodificar: copia la mejor imagen y el mejor audio tal como estan." + DV,["origen","cabeceras","ruta_salida"],"0 si arranco, -1 si ya hay una descarga, -2 sin memoria, -3 si no pudo crear el hilo")
D("VIDEO_DOWNLOAD_AUDIO_BEGIN","Descarga solo el audio. La extension del archivo final se ajusta al codec (.m4a, .mp3, .opus...); VIDEO_DOWNLOAD_PATH dice cual salio." + DV,["origen","cabeceras","ruta_salida"],"0 si arranco, -1 si ya hay una descarga, -2 sin memoria, -3 si no pudo crear el hilo")
D("VIDEO_DOWNLOAD_CONVERT_BEGIN","Descarga solo el audio y lo recodifica al formato pedido: \"mp3\", \"opus\", \"flac\", \"wav\" o \"aac\"." + DV,["origen","cabeceras","ruta_salida","formato"],"0 si arranco, -1 si ya hay una descarga, -2 sin memoria, -3 si no pudo crear el hilo")
D("VIDEO_DOWNLOAD_POLL","Estado de la descarga.",[],"-1 en curso, 0 terminada bien, -2 cancelada, -10 fallo (VIDEO_DOWNLOAD_ERROR dice por que), -3 si no habia ninguna")
D("VIDEO_DOWNLOAD_PERCENT","Porcentaje descargado, de 0 a 100.",[],"porcentaje")
D("VIDEO_DOWNLOAD_CANCEL","Cancela la descarga en curso.")
D("VIDEO_DOWNLOAD_PATH","Ruta del archivo que realmente se esta escribiendo (en modo audio la extension depende del codec).",[],"ruta")
D("VIDEO_DOWNLOAD_ERROR","Motivo del fallo de la ultima descarga.",[],"texto")

# ---- Torrents ----
TOR = " Los archivos de un torrent se sirven en una direccion local (127.0.0.1) con soporte de rangos, asi que VIDEO_OPEN los reproduce como cualquier otro stream mientras se descargan (primero las piezas que se van a ver)."
D("TORRENT_CACHE","Carpeta donde se guardan los datos descargados (se crea al iniciar el primer torrent). Llamala antes del primer TORRENT_ADD.",["carpeta"],"0")
D("TORRENT_ADD","Empieza un torrent a partir de un enlace magnet:, la ruta de un .torrent o la direccion http(s) de uno. Se descarga en segundo plano." + TOR,["origen"],"manejador (1 o mas), -1 si no quedan huecos, -2 si el origen no vale",
  "t = torrent_add(\"magnet:?xt=urn:btih:...\");\nwhile (torrent_state(t) == 0) frame; end   // buscando la lista de archivos\nf = torrent_main_file(t);")
D("TORRENT_REMOVE","Detiene un torrent; con borrar = 1 borra tambien lo descargado del disco.",["torrent","borrar"])
D("TORRENT_STATE","Estado del torrent.",["torrent"],"0 = buscando la lista de archivos o leyendo el .torrent, 1 = listo, 2 = fallo (TORRENT_ERROR)")
D("TORRENT_ERROR","Motivo del fallo de un torrent.",["torrent"],"texto")
D("TORRENT_NAME","Nombre del torrent.",["torrent"],"texto")
D("TORRENT_FILES","Numero de archivos del torrent.",["torrent"],"numero de archivos")
D("TORRENT_FILE_NAME","Nombre del archivo i (desde 0) del torrent.",["torrent","i"],"texto")
D("TORRENT_FILE_SIZE","Tamano en bytes del archivo i del torrent.",["torrent","i"],"bytes")
D("TORRENT_MAIN","Indice del archivo de video mas grande del torrent: el que se querra reproducir.",["torrent"],"indice, o -1 si no hay ninguno")
D("TORRENT_SELECT","Elige el archivo que se descarga primero (los demas no se descargan).",["torrent","i"],"0 si va bien, -1 si el torrent aun no esta listo o el indice no existe")
D("TORRENT_URL","Direccion local desde la que se sirve el archivo i, lista para VIDEO_OPEN.",["torrent","i"],"direccion http://127.0.0.1:...")
D("TORRENT_PEERS","Numero de pares conectados.",["torrent"],"pares")
D("TORRENT_RATE","Velocidad de descarga en bytes por segundo.",["torrent"],"bytes por segundo")
D("TORRENT_PERCENT","Porcentaje (0 a 100) del archivo elegido que ya esta en disco.",["torrent"],"porcentaje")
D("TORRENT_BUFFERED","Bytes ya listos justo despues de la posicion que se esta reproduciendo.",["torrent"],"bytes")
D("TORRENT_READY","Cuanto de lo que necesita el reproductor para abrir el archivo esta ya descargado (0 a 100). Espera a 100 antes de llamar a VIDEO_OPEN.",["torrent"],"porcentaje")
D("TORRENT_DOWNLOADED","Total de bytes recibidos del torrent.",["torrent"],"bytes")
D("TORRENT_OPTIONS","Ajustes que se fijan ANTES de TORRENT_ADD: trackers publicos extra (1 = anadirlos a todos los torrents, por defecto activo) y DHT (1 = buscar pares tambien en la red DHT sin trackers, solo preguntando; por defecto activo).",["trackers_extra","dht"])
D("TORRENT_AHEAD","Cuantos megas por delante de la posicion de reproduccion se descargan (de 64 a 8192; 192 por defecto). Subelo si necesitas un buffer grande.",["megas"])
D("TORRENT_UPLOAD_LIMIT","Bytes por segundo que el cliente puede enviar a otros. 0 = no subir nada (valor por defecto).",["bytes_por_segundo"])

# ---- Recibir archivos desde otro equipo ----
D("UPLOAD_START","Arranca un pequeno servidor web para recibir archivos desde otro equipo: abriendo su direccion en un navegador sale una pagina donde se sueltan los archivos. Se protege con un PIN que hay que escribir; sin el no se escribe nada. Nunca sobrescribe (un nombre repetido pasa a \"video (2).mp4\"), solo escribe en la carpeta dada y reduce cada nombre a un nombre base seguro.",
  ["carpeta","puerto","pin"],"el puerto que quedo escuchando (prueba el dado y los 19 siguientes), o un numero negativo si falla",
  "p = upload_start(\"Videos\", 8080, \"1234\");\nsay(\"abre http://\" + upload_ip() + \":\" + p);")
D("UPLOAD_STOP","Para el servidor de recepcion de archivos.")
D("UPLOAD_FILES","Archivos recibidos desde que arranco el servidor.",[],"numero de archivos")
D("UPLOAD_BYTES","Bytes recibidos desde que arranco el servidor.",[],"bytes")
D("UPLOAD_PERCENT","Porcentaje (0 a 100) del archivo que se esta recibiendo ahora, o -1 si no hay ninguno.",[],"porcentaje")
D("UPLOAD_CURRENT","Nombre del archivo que se esta recibiendo (vacio si ninguno).",[],"texto")
D("UPLOAD_URL","Direccion (http, https o magnet) que el usuario envio desde la pagina para reproducirla. Al leerla se borra, asi que devuelve vacio hasta que llegue otra.",[],"direccion, o vacio")
D("UPLOAD_IP","Direccion de este equipo en la red local (por ejemplo \"192.168.1.50\"), vacia si no se encuentra.",[],"texto")
D("UPLOAD_ERROR","Motivo del ultimo fallo del servidor de recepcion.",[],"texto")

# ---- DLNA / UPnP ----
DL = " Solo lee lo que ofrece el servidor; nunca escribe en el."
D("DLNA_SCAN","Busca servidores multimedia UPnP/DLNA de la red local (unos 3 s, en segundo plano)." + DL,[],"0 si arranco, -1 si ya estaba buscando")
D("DLNA_SCAN_STATE","Estado de la busqueda.",[],"0 = nunca iniciada, 1 = en curso, 2 = terminada")
D("DLNA_COUNT","Numero de servidores encontrados.",[],"numero de servidores")
D("DLNA_NAME","Nombre del servidor i (desde 0).",["i"],"texto")
D("DLNA_ADD","Anade un servidor a mano por la direccion de su descripcion (http://ip:puerto/desc.xml), para redes donde la busqueda automatica no llega. DLNA_SCAN_STATE vale 1 mientras trabaja y 2 al acabar.",["url"],"0 si arranco, -1 si la direccion no vale o esta ocupado")
D("DLNA_FIND","Indice de un servidor ya anadido con DLNA_ADD, a partir de su direccion.",["url"],"indice, o -1 si no esta")
D("DLNA_BROWSE","Pide al servidor i el contenido de una carpeta (el id \"0\" es la raiz): cuantos elementos y desde cual. Se hace en segundo plano; mira DLNA_BROWSE_STATE.",["servidor","id_carpeta","desde","cuantos"],"0 si arranco, -1 si esta ocupado o el servidor no vale")
D("DLNA_BROWSE_STATE","Estado de la consulta de carpeta.",[],"0 = nunca iniciada, 1 = en curso, 2 = terminada, 3 = fallo (DLNA_ERROR)")
D("DLNA_RESULT","Contenido de la carpeta, una linea por elemento separado por tabuladores: carpetas \"C<TAB>id<TAB>titulo<TAB>hijos\" y archivos \"I<TAB>url<TAB>titulo<TAB>bytes<TAB>segundos<TAB>tipo\" (tipo v = video, a = audio, i = imagen). La url de un archivo se reproduce con VIDEO_OPEN.",[],"texto con una linea por elemento")
D("DLNA_TOTAL","Cuantos elementos tiene en total la carpeta consultada (para paginar).",[],"numero de elementos")
D("DLNA_ERROR","Motivo del ultimo fallo de DLNA.",[],"texto")

# ---- FTP ----
D("FTP_LIST_BEGIN","Lista en segundo plano el contenido de una carpeta FTP (la direccion debe empezar por ftp://, con usuario y clave en la direccion si hacen falta).",["url"],"0 si arranco, -1 si ya hay una consulta, -2 si la direccion no vale, -3 si no pudo crear el hilo")
D("FTP_LIST_POLL","Estado de la consulta FTP.",[],"-1 en curso, 0 terminada bien, 1 fallo (FTP_LIST_ERROR)")
D("FTP_LIST_RESULT","Contenido de la carpeta FTP, una linea por entrada separada por tabuladores: carpetas \"C<TAB>nombre<TAB>nombre<TAB>0\" y archivos \"I<TAB>nombre<TAB>nombre<TAB>bytes<TAB>0<TAB>v\". Si el servidor no da el tipo, una entrada sin punto se toma por carpeta.",[],"texto con una linea por entrada")
D("FTP_LIST_ERROR","Motivo del ultimo fallo de la consulta FTP.",[],"texto")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
