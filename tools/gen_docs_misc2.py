# Documentacion de libmod_misc, segunda tanda: memoria, ficheros y directorios, ordenacion, comprobaciones de caracteres y matematicas menores.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e

# ---- matematicas menores ----
D("GET_DISTX","Desplazamiento horizontal de avanzar una distancia en un angulo: distancia * cos(angulo). Angulo en milesimas de grado.",["angulo","distancia"],"desplazamiento en X")
D("GET_DISTY","Desplazamiento vertical de avanzar una distancia en un angulo: -distancia * sin(angulo) (la Y de pantalla crece hacia abajo). Angulo en milesimas de grado.",["angulo","distancia"],"desplazamiento en Y",
  "x += get_distx(angle, 5);\ny += get_disty(angle, 5);   // lo mismo que advance(5)")
D("DECIMAL","Parte decimal de un numero conservando el signo: decimal(-2.75) = -0.75.",["valor"],"la parte decimal")
D("MODULUS","Resto de dividir a entre b, siempre positivo para un b positivo: modulus(-1, 5) = 4 (el operador % da -1).",["a","b"],"el resto")
D("MMOD","Alias de MODULUS.",["a","b"],"el resto")
D("RANGECHK","Compara un valor con un rango: -1 si esta por debajo, 1 si esta por encima y 0 si esta dentro (limites incluidos; el orden de los limites da igual).",["valor","limite_a","limite_b"],"-1, 0 o 1",sig="III")
D("RANGECHK","Lo mismo con decimales.",["valor","limite_a","limite_b"],"-1, 0 o 1",sig="DDD")
D("NORMALIZE","Convierte un valor entero del rango [bajo, alto] a un numero entre 0 y 1.",["valor","bajo","alto"],"la fraccion",sig="III")
D("ISNAN","Indica si un decimal no es un numero (NaN).",["valor"],"1 si no es un numero")
D("ISINF","Indica si un decimal es infinito.",["valor"],"1 si es infinito")
D("FINITE","Indica si un decimal es un numero normal (ni infinito ni NaN).",["valor"],"1 si es finito")

# ---- caracteres ----
for n,t in [("ISALPHA","es una letra"),("ISNUM","es una cifra (0-9)"),("ISALNUM","es una letra o una cifra"),("ISSPACE","es un espacio, tabulador o salto de linea"),
            ("ISXNUM","es una cifra hexadecimal (0-9, A-F)"),("ISWORDCHAR","puede formar parte de un identificador (letra, cifra o _)"),("ISWORDFIRST","puede empezar un identificador (letra o _)")]:
    D(n,"Indica si un caracter (su codigo) "+t+".",["caracter"],"1 si lo es")
D("TOUPPER","Pasa un caracter (su codigo) a mayuscula.",["caracter"],"el codigo en mayuscula")
D("TOLOWER","Pasa un caracter (su codigo) a minuscula.",["caracter"],"el codigo en minuscula")
D("STRTOK","Parte una cadena en trozos separados por caracteres delimitadores: la primera llamada con la cadena y los delimitadores devuelve el primer trozo.",["texto","delimitadores"],"el trozo")

# ---- memoria (para punteros; no hace falta en juegos normales) ----
D("ALLOC","Reserva un bloque de memoria de 'bytes' bytes y devuelve su puntero (0 si no hay memoria). Hay que liberarlo con FREE.",["bytes"],"puntero al bloque")
D("CALLOC","Reserva memoria para 'cantidad' elementos de 'tamano' bytes cada uno, puesta a cero.",["cantidad","tamano"],"puntero al bloque")
D("REALLOC","Cambia el tamano de un bloque reservado, conservando su contenido.",["puntero","bytes"],"el puntero (puede cambiar)")
D("FREE","Libera un bloque reservado con ALLOC, CALLOC o REALLOC.",["puntero"])
D("MEMSET","Rellena 'bytes' bytes de memoria con el valor de un byte.",["puntero","valor","bytes"])
D("MEMCOPY","Copia 'bytes' bytes de memoria de un sitio a otro (no deben solaparse; para eso, MEMMOVE).",["destino","origen","bytes"])
D("MEMMOVE","Copia 'bytes' bytes de memoria de un sitio a otro, aunque se solapen.",["destino","origen","bytes"])
D("MEMCMP","Compara dos bloques de memoria byte a byte: 0 si son iguales.",["a","b","bytes"],"0 si son iguales")
D("MEMORY_FREE","Memoria fisica libre, en bytes (solo informativo; puede ser aproximado).",ret="bytes libres")
D("MEM_AVAILABLE","Alias de MEMORY_FREE.",ret="bytes libres")
for a,b in [("MEM_ALLOC","ALLOC"),("MEM_CALLOC","CALLOC"),("MEM_REALLOC","REALLOC"),("MEM_FREE","FREE"),("MEM_SET","MEMSET"),("MEM_COPY","MEMCOPY"),("MEM_MOVE","MEMMOVE"),("MEM_CMP","MEMCMP"),("MEM_TOTAL","MEMORY_TOTAL")]:
    d[a] = {"desc": "Alias de %s." % b}

# ---- ficheros y directorios ----
D("FILE","Lee un fichero de texto entero y devuelve su contenido en una cadena (vacia si no existe).",["fichero"],"el contenido",ejemplo="datos = file(\"nivel.txt\");")
D("FREAD","Lee 'bytes' bytes de un fichero en un bloque de memoria.",["puntero","bytes","fichero"],"bytes leidos",sig="PII")
D("FREAD","Lee del fichero una variable (o estructura) escrita con FWRITE.",["fichero","variable"],"bytes leidos",sig="IV++")
D("FWRITE","Escribe 'bytes' bytes de un bloque de memoria en un fichero.",["puntero","bytes","fichero"],"bytes escritos",sig="PII")
D("FWRITE","Escribe en el fichero una variable (o estructura).",["fichero","variable"],"bytes escritos",sig="IV++")
D("FSEEK","Mueve la posicion de lectura o escritura de un fichero. 'origen' es el punto de referencia (principio, posicion actual o final).",["fichero","desplazamiento","origen"],"la posicion nueva")
D("FFLUSH","Fuerza a escribir en disco lo que quede pendiente de un fichero abierto.",["fichero"])
D("CD","Carpeta de trabajo actual.",ret="la ruta",sig="")
D("CD","Cambia la carpeta de trabajo.",["carpeta"],"0 si va bien",sig="S")
D("CHDIR","Cambia la carpeta de trabajo.",["carpeta"],"0 si va bien")
D("MKDIR","Crea una carpeta.",["carpeta"],"0 si va bien")
D("RMDIR","Borra una carpeta vacia.",["carpeta"],"0 si va bien")
D("RM","Borra un fichero.",["fichero"],"0 si va bien")
D("GLOB","Busca ficheros con comodines ('*' y '?'): la primera llamada con la ruta devuelve el primero que coincide y cada llamada siguiente devuelve el siguiente, hasta que no quedan y devuelve una cadena vacia.",["ruta_con_comodines"],"nombre del fichero, vacio al terminar",
  "f = glob(\"Assets/*.png\");\nWHILE (f != \"\")\n    say(f);\n    f = glob(\"Assets/*.png\");\nEND")
D("DIROPEN","Abre una carpeta para leer su contenido y devuelve un manejador.",["carpeta"],"manejador")
D("DIRREAD","Lee la entrada siguiente de una carpeta abierta (vacia al terminar).",["manejador"],"nombre")
D("DIRCLOSE","Cierra una carpeta abierta con DIROPEN.",["manejador"])
D("GETENV","Valor de una variable de entorno del sistema (cadena vacia si no existe).",["nombre"],"el valor")
D("GET_PREF_LANGUAGE","Idioma preferido del usuario segun el sistema.",ret="el codigo del idioma")

# ---- ordenacion ----
D("SORT","Ordena un array de numeros o de estructuras (en estas, usando su primera variable como clave).",["array"],sig="V++")
D("SORT","Igual, indicando cuantos elementos ordenar.",["array","cantidad"],sig="V++I")
D("KSORT","Ordena un array de estructuras usando como clave la variable que se indica.",["array","clave"],sig="V++V++")
D("KSORT","Igual, indicando cuantos elementos ordenar.",["array","clave","cantidad"],sig="V++V++I")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
