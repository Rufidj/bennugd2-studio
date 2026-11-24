# Documentacion de misc: cifrado, geometria 2D, memoria, senales, regex, ordenar, ejecutar programas,
# teclado/progreso del sistema, arrays de cadenas y listas. Comprobado contra libmod_misc/m_*.c.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo=""):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n] = e

# ---- Cifrado ----
CR = " Cifra bloques de 8 bytes (DES o 3DES, modo ECB: sin encadenar bloques). El tamano de los datos debe ser multiplo de 8."
D("CRYPT_NEW","Crea un cifrador con un metodo (CRYPT_NONE, CRYPT_DES o CRYPT_3DES) y una clave: un puntero a 8 bytes para DES o a 24 bytes para 3DES. Se usa con CRYPT_ENCRYPT y CRYPT_DECRYPT y se libera con CRYPT_DEL.",["metodo","&clave"],"manejador del cifrador, o 0 si la clave no es valida")
D("CRYPT_DEL","Libera un cifrador creado con CRYPT_NEW.",["cifrador"])
D("CRYPT_ENCRYPT(PPPI)","Cifra n bloques de 8 bytes de un buffer a otro con un cifrador ya creado." + CR,["cifrador","&origen","&destino","bloques"],"0 si va bien, -1 si hay error",
  "c = crypt_new(CRYPT_DES, &clave);\ncrypt_encrypt(c, &texto, &cifrado, 4);\ncrypt_del(c);")
D("CRYPT_DECRYPT(PPPI)","Descifra n bloques de 8 bytes de un buffer a otro con un cifrador ya creado." + CR,["cifrador","&origen","&destino","bloques"],"0 si va bien, -1 si hay error")
D("CRYPT_ENCRYPT(IPPPI)","Cifra sin crear antes un cifrador: indicas directamente el metodo y la clave." + CR,["metodo","&clave","&origen","&destino","bloques"],"0 si va bien, -1 si hay error")
D("CRYPT_DECRYPT(IPPPI)","Descifra sin crear antes un cifrador: indicas directamente el metodo y la clave." + CR,["metodo","&clave","&origen","&destino","bloques"],"0 si va bien, -1 si hay error")

# ---- Geometria 2D ----
D("INTERSECT","Calcula donde se cortan dos SEGMENTOS: (x1, y1)-(x2, y2) y (x3, y3)-(x4, y4). Si se cortan escribe el punto en las dos variables double por referencia.",["x1","y1","x2","y2","x3","y3","x4","y4","&x","&y"],"1 si se cortan, 0 si no",
  "double ix, iy;\nif (intersect(0, 0, 10, 10, 0, 10, 10, 0, &ix, &iy))\n    say(\"se cruzan en \" + ix + \",\" + iy);\nend")
D("INTERSECT_LINE_CIRCLE","Calcula donde corta un SEGMENTO (x1, y1)-(x2, y2) a una circunferencia de centro (cx, cy) y radio r. Escribe hasta dos puntos en las variables double por referencia.",["x1","y1","x2","y2","cx","cy","r","&xa","&ya","&xb","&yb"],"numero de puntos de corte (0, 1 o 2); solo se escriben los que existen")
D("INTERSECT_CIRCLE","Calcula donde se cortan dos circunferencias: centro (cx1, cy1) y (cx2, cy2) con radios r1 y r2. Escribe hasta dos puntos en las variables double por referencia.",["cx1","cy1","cx2","cy2","r1","r2","&xa","&ya","&xb","&yb"],"numero de puntos de corte (0, 1 o 2); solo se escriben los que existen")
D("ORTHO","Proyeccion ortogonal de un punto sobre un segmento: el punto del segmento (x1, y1)-(x2, y2) mas cercano a (px, py). Lo escribe en dos variables double.",["x1","y1","x2","y2","px","py","&x","&y"],"distancia (double) del punto al segmento")
D("PROJECT","Recta paralela a un segmento que pasa por un punto: devuelve el segmento (x1, y1)-(x2, y2) desplazado en perpendicular hasta (px, py). Escribe sus dos extremos en variables double.",["x1","y1","x2","y2","px","py","&xa","&ya","&xb","&yb"],"distancia (double) entre las dos rectas paralelas")

# ---- Memoria ----
MS = " Escribe el mismo valor n veces seguidas en memoria desde el puntero, con el ancho indicado."
D("MEM_SETW","Rellena memoria con valores de 16 bits (word)." + MS,["&destino","valor","n"],"1")
D("MEM_SETD","Rellena memoria con valores de 32 bits (dword)." + MS,["&destino","valor","n"],"1")
D("MEM_SETI","Rellena memoria con valores de 64 bits (int)." + MS,["&destino","valor","n"],"1")
D("MEMSETW","Igual que MEM_SETW (nombre alternativo).",["&destino","valor","n"],"1")
D("MEMSETD","Igual que MEM_SETD (nombre alternativo).",["&destino","valor","n"],"1")
D("MEMSETI","Igual que MEM_SETI (nombre alternativo).",["&destino","valor","n"],"1")
D("MEMORY_TOTAL","Memoria fisica total del sistema, en bytes (el calculo depende de la plataforma).",[],"bytes")

# ---- Procesos ----
D("SIGNAL_ACTION(II)","Cambia como reacciona ESTE proceso a una senal: la senal (S_KILL, S_WAKEUP, S_SLEEP o S_FREEZE) y la accion (S_IGN = ignorarla, S_DFL = comportamiento normal).",["senal","accion"],"","signal_action(S_KILL, S_IGN);   // este proceso no muere con signal(0, S_KILL)")
D("SIGNAL_ACTION(III)","Igual para otro proceso, para todos los de un tipo (pasando el tipo de proceso) o para todos (ALL_PROCESS).",["proceso","senal","accion"])
D("PAUSE","Pausa TODOS los procesos excepto el que la llama. Reanuda con RESUME. Es lo habitual para un menu de pausa.",[],"0","pause();\n// ... menu de pausa ...\nresume();")
D("RESUME","Reanuda todos los procesos pausados con PAUSE.",[],"0")
D("PAUSE(I)","No disponible: la version con un proceso concreto esta desactivada en esta version del motor.",["proceso"])
D("RESUME(I)","No disponible: la version con un proceso concreto esta desactivada en esta version del motor.",["proceso"])

# ---- Texto y datos ----
D("REGEX","Busca una expresion regular (sintaxis POSIX extendida, sin distinguir mayusculas) en una cadena. Si hay coincidencia devuelve su posicion y rellena REGEX_REG[0] con el texto que coincide y REGEX_REG[1..15] con los grupos entre parentesis.",["patron","texto"],"posicion de la coincidencia, o -1 si no hay",
  "if (regex(\"([0-9]+)-([0-9]+)\", \"rango 10-20\") != -1)\n    say(regex_reg[1] + \" a \" + regex_reg[2]);\nend")
D("REGEX_REPLACE","Sustituye en un texto cada coincidencia de una expresion regular por otro texto (en el reemplazo se aceptan \\0 a \\9 para insertar los grupos). OJO al orden: patron, REEMPLAZO y luego el texto. REGEX_REG queda rellenado con la primera coincidencia.",["patron","reemplazo","texto"],"el texto resultante",
  "say(regex_replace(\"[0-9]+\", \"#\", \"a1b22c333\"));   // a#b#c#")
D("ISORT","Ordena un array de estructuras en memoria (quicksort) por una clave: puntero al array, tamano de cada elemento en bytes, numero de elementos, desplazamiento de la clave dentro del elemento, tamano de la clave en bytes y tipo de la clave (0 = entero, 1 = decimal).",["&array","tam_elemento","n","desplazamiento","tam_clave","tipo"],"1",
  "isort(&jugadores, sizeof(jugadores[0]), 10, 0, 8, 0);")
D("EXEC","Ejecuta un programa externo: modo (_P_WAIT espera a que termine y devuelve su codigo de salida; _P_NOWAIT devuelve su identificador de proceso del sistema), nombre del programa, numero de argumentos y un array de cadenas con los argumentos. No esta disponible en Switch, PS3 ni PS5 (devuelve -1).",["modo","programa","n_args","&argumentos"],"codigo de salida o identificador, -1 si falla")

# ---- Teclado y progreso del sistema ----
KB = " Es un dialogo del sistema: solo existe de verdad en PS5 y Android; en PC devuelve -100 (el programa debe usar su propio teclado)."
D("SYSTEM_KEYBOARD_OPEN","Abre el teclado en pantalla del sistema: titulo, texto inicial, maximo de caracteres (hasta 2048) y banderas (1 = contrasena; 2 = tipo de teclado alternativo, pensado para numeros)." + KB,["titulo","texto_inicial","maximo","banderas"],"0 si se abrio, -2 si ya hay uno abierto, -3 si no pudo iniciarse, -100 si no hay teclado del sistema")
D("SYSTEM_KEYBOARD_STATUS","Estado del teclado del sistema: 0 = cerrado, 1 = abierto (el usuario escribe), 2 = terminado. Llamala cada frame hasta que sea 2." + KB,[],"estado")
D("SYSTEM_KEYBOARD_OK","Una vez terminado, indica si el usuario acepto (1) o cancelo (0).",[],"1 o 0")
D("SYSTEM_KEYBOARD_TEXT","Texto escrito por el usuario (vacio si cancelo o aun no ha terminado).",[],"texto")
D("SYSTEM_KEYBOARD_CLOSE","Cierra el teclado del sistema (lo aborta si seguia abierto).")
PG = " Es una barra del sistema: en PS5 no esta disponible aun en las consolas probadas y en PC solo simula (si existe la variable de entorno MP_FAKE_PROGRESS escribe en la consola); el programa debe dibujar su propio progreso."
D("SYSTEM_PROGRESS_OPEN","Abre una barra de progreso del sistema con un titulo; banderas = 1 anade boton de cancelar." + PG,["titulo","banderas"],"0 si se abrio, -2 si ya hay una, -6 si no esta disponible, -100 en PC")
D("SYSTEM_PROGRESS_SET","Fija el porcentaje de la barra de progreso (0 a 100).",["porcentaje"],"resultado, o -1 si no hay barra abierta")
D("SYSTEM_PROGRESS_MESSAGE","Cambia el mensaje que muestra la barra de progreso.",["mensaje"],"resultado, o -1 si no hay barra abierta")
D("SYSTEM_PROGRESS_UPDATE","Estado de la barra de progreso (3 = el usuario pulso cancelar).",[],"estado")
D("SYSTEM_PROGRESS_CLOSE","Cierra la barra de progreso.")

# ---- Arrays de cadenas ----
D("STRING_NEW_ARRAY()","Crea un array de cadenas vacio (de tipo STRING *). Se amplia con STRING_RESIZE_ARRAY.",[],"puntero al array")
D("STRING_NEW_ARRAY(I)","Crea un array dinamico de n cadenas (todas vacias).",["n"],"puntero al array")
D("STRING_RESIZE_ARRAY","Cambia el tamano de un array de cadenas. Hay que pasar la DIRECCION de la variable puntero (con &) porque puede moverse; las cadenas sobrantes se liberan.",["&array","nuevo_n"],"nuevo tamano")
D("STRING_DEL_ARRAY","Libera un array de cadenas y todas sus cadenas. Se pasa la direccion de la variable puntero (con &), que queda a 0.",["&array"],"1")
D("STRING_SIZE_ARRAY","Numero de elementos de un array de cadenas (se pasa el puntero tal cual, sin &).",["array"],"numero de elementos")
D("STRING_NEWA()","Igual que STRING_NEW_ARRAY() (nombre corto).",[],"puntero al array")
D("STRING_NEWA(I)","Igual que STRING_NEW_ARRAY(n) (nombre corto).",["n"],"puntero al array")
D("STRING_RESIZEA","Igual que STRING_RESIZE_ARRAY (nombre corto).",["&array","nuevo_n"],"nuevo tamano")
D("STRING_DELA","Igual que STRING_DEL_ARRAY (nombre corto).",["&array"],"1")
D("STRING_SIZEA","Igual que STRING_SIZE_ARRAY (nombre corto).",["array"],"numero de elementos")
D("STRING_DUMP","Escribe en la consola todas las cadenas que tiene el programa en memoria. Util para detectar fugas.",[],"1")

# ---- Listas ----
LS = " La lista es una lista enlazada de punteros (valores sueltos de 64 bits): los nuevos elementos se anaden AL PRINCIPIO."
D("LIST_CREATE()","Crea una lista vacia." + LS,[],"manejador de la lista","lista = list_create();\nlist_insertitem(lista, &miobjeto);\nlist_free(lista);")
D("LIST_CREATE(I)","Crea una lista indicando cuantos nodos se guardan como reserva para reutilizarlos (-1 = el valor por defecto)." + LS,["reserva"],"manejador de la lista")
D("LIST_FREE","Libera la lista (no libera lo que apuntan sus elementos).",["lista"])
D("LIST_EMPTY","Quita todos los elementos de la lista, pero conserva la lista.",["lista"])
D("LIST_INSERTITEM","Anade un elemento (un puntero o un valor) al principio de la lista.",["lista","elemento"])
D("LIST_REMOVEITEM","Quita de la lista el elemento con ese valor (el primero que coincida).",["lista","elemento"])
D("LIST_WALK","Recorre la lista elemento a elemento. Pasa una variable de contexto por referencia puesta a 0 al empezar; cada llamada devuelve el siguiente elemento y avanza el contexto, y devuelve 0 al llegar al final.",["lista","&contexto"],"el siguiente elemento, o 0 al acabar",
  "ctx = 0;\nwhile ((item = list_walk(lista, &ctx)) != 0)\n    say(item);\nend")
D("LIST_SIZE","Numero de elementos de la lista.",["lista"],"numero de elementos")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
