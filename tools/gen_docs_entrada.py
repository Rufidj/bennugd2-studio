# Documentacion del modulo de entrada (libmod_input): teclado y mando. Verificado contra libmod_input.c y libbginput.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e

# ---- teclado ----
D("KEY","Indica si una tecla esta pulsada AHORA. Se le pasa una constante de tecla (_A, _SPACE, _ESC, _UP, _LEFT...). Una tecla pulsada y soltada dentro del mismo fotograma tambien cuenta como pulsada, para que el juego no se pierda pulsaciones rapidas.",["tecla"],"1 si esta pulsada, 0 si no",
  "IF (key(_SPACE)) saltar(); END\nIF (key(_LEFT)) x -= 4; END")
D("KEY_DOWN","Indica si una tecla se ACABA de pulsar: vale 1 solo en el fotograma en que pasa de suelta a pulsada. Util para acciones de un solo disparo (saltar, disparar, abrir un menu).",["tecla"],"1 solo en el fotograma de la pulsacion",
  "IF (key_down(_SPACE)) disparar(); END   // un disparo por pulsacion, aunque mantengas la tecla")
D("KEYDOWN","Alias de KEY_DOWN.",["tecla"],"1 solo en el fotograma de la pulsacion")
D("KEY_UP","Indica si una tecla se ACABA de soltar: vale 1 solo en el fotograma en que pasa de pulsada a suelta.",["tecla"],"1 solo en el fotograma de soltarla")
D("KEYUP","Alias de KEY_UP.",["tecla"],"1 solo en el fotograma de soltarla")

# ---- seleccion y estado de los mandos ----
D("JOY_NUMJOYSTICKS","Numero de mandos conectados.",ret="cantidad de mandos")
D("JOY_NUMBER","Alias antiguo de JOY_NUMJOYSTICKS.",ret="cantidad de mandos")
D("NUMBER_JOY","Alias antiguo de JOY_NUMJOYSTICKS.",ret="cantidad de mandos")
D("JOY_SELECT","Elige el mando con el que trabajan las funciones JOY_* que no llevan numero de mando. BennuGD2 arranca SIN ningun mando elegido: hasta que se elige, JOY_GETBUTTON y JOY_GETAXIS devuelven siempre 0. Normalmente se hace una vez al empezar.",["mando"],"el mando elegido",
  "IF (joy_numjoysticks() > 0) joy_select(0); END")
D("SELECT_JOY","Alias antiguo de JOY_SELECT.",["mando"])
D("JOY_NAME","Nombre del mando. Sin argumento, el del mando elegido; con un numero, el de ese mando.",["mando"],"el nombre",sig="I")
D("JOY_NAME","Nombre del mando elegido.",[],"el nombre",sig="")
D("JOY_IS_ATTACHED","Indica si un mando esta conectado. Sin argumento, el elegido.",["mando"],"1 si esta conectado",sig="I")
D("JOY_IS_ATTACHED","Indica si el mando elegido esta conectado.",[],"1 si esta conectado",sig="")
for n,desc in [("JOY_NUMBUTTONS","Numero de botones"),("JOY_BUTTONS","Numero de botones (nombre antiguo)"),
               ("JOY_NUMAXES","Numero de ejes"),("JOY_AXES","Numero de ejes (nombre antiguo)"),
               ("JOY_NUMHATS","Numero de cruces (sombreros / POV hats)"),("JOY_NUMBALLS","Numero de bolas (trackballs)")]:
    D(n,desc+" del mando indicado.",["mando"],"cantidad",sig="I")
    D(n,desc+" del mando elegido.",[],"cantidad",sig="")

# ---- lectura ----
D("JOY_GETBUTTON","Estado de un boton del mando indicado. Los botones se piden con las constantes JOY_BUTTON_A, _B, _X, _Y, _START, _BACK, _LEFTSHOULDER, _RIGHTSHOULDER, _DPAD_UP/DOWN/LEFT/RIGHT, _LEFTSTICK, _RIGHTSTICK, _GUIDE y _TOUCHPAD.",["mando","boton"],"1 si esta pulsado",
  "IF (joy_getbutton(0, JOY_BUTTON_A)) saltar(); END")
D("JOY_GETBUTTON","Estado de un boton del mando elegido con JOY_SELECT.",["boton"],"1 si esta pulsado",sig="I")
D("GET_JOY_BUTTON","Alias antiguo de JOY_GETBUTTON.",["mando","boton"],"1 si esta pulsado")
D("GET_JOY_BUTTON","Alias antiguo de JOY_GETBUTTON para el mando elegido.",["boton"],"1 si esta pulsado",sig="I")
D("JOY_GETAXIS","Posicion de un eje del mando indicado. Los ejes se piden con JOY_AXIS_LEFTX, JOY_AXIS_LEFTY, JOY_AXIS_RIGHTX, JOY_AXIS_RIGHTY (sticks) y JOY_AXIS_TRIGGERLEFT, JOY_AXIS_TRIGGERRIGHT (gatillos). El valor va de -32768 a 32767 y es 0 en reposo; para tener -1.0 .. 1.0, dividelo entre 32767.0. Los sticks nunca vuelven a 0 exacto: ignora un margen pequeno (zona muerta).",["mando","eje"],"posicion del eje",
  "ax = joy_getaxis(0, JOY_AXIS_LEFTX) / 32767.0;\nIF (abs(ax) < 0.15) ax = 0.0; END   // zona muerta")
D("JOY_GETAXIS","Posicion de un eje del mando elegido con JOY_SELECT (de -32768 a 32767).",["eje"],"posicion del eje",sig="I")
D("JOY_GETPOSITION","Alias antiguo de JOY_GETAXIS.",["mando","eje"],"posicion del eje")
D("JOY_GETPOSITION","Alias antiguo de JOY_GETAXIS para el mando elegido.",["eje"],"posicion del eje",sig="I")
D("GET_JOY_POSITION","Alias antiguo de JOY_GETAXIS.",["mando","eje"],"posicion del eje")
D("GET_JOY_POSITION","Alias antiguo de JOY_GETAXIS para el mando elegido.",["eje"],"posicion del eje",sig="I")
D("JOY_GETHAT","Estado de una cruz (POV hat) del mando indicado: JOY_HAT_CENTERED, _UP, _DOWN, _LEFT, _RIGHT, _LEFTUP, _LEFTDOWN, _RIGHTUP o _RIGHTDOWN.",["mando","cruz"],"direccion de la cruz")
D("JOY_GETHAT","Estado de una cruz del mando elegido.",["cruz"],"direccion de la cruz",sig="I")
D("JOY_GETBALL","Lee cuanto se ha movido una bola (trackball) del mando indicado, en dos variables que se pasan por puntero.",["mando","bola","&dx","&dy"],"",sig="IIPP")
D("JOY_GETBALL","Lee cuanto se ha movido una bola del mando elegido, en dos variables que se pasan por puntero.",["bola","&dx","&dy"],"",sig="IPP")
D("JOY_GETACCEL","Lee el acelerometro del mando indicado en tres variables que se pasan por puntero.",["mando","&x","&y","&z"],"",sig="IPPP")
D("JOY_GETACCEL","Lee el acelerometro del mando elegido en tres variables que se pasan por puntero.",["&x","&y","&z"],"",sig="PPP")

# ---- consulta y ajustes avanzados ----
D("JOY_QUERY","Informacion especifica de un mando: se pide con un 'elemento' y, segun cual sea, un argumento extra.",["mando","elemento","arg"],"el valor pedido",sig="III")
D("JOY_QUERY","Informacion especifica de un mando: se pide con un 'elemento'.",["mando","elemento"],"el valor pedido",sig="II")
D("JOY_QUERY","Informacion general de los mandos: se pide con un 'elemento'.",["elemento"],"el valor pedido",sig="I")
D("JOY_SET","Cambia un ajuste de un mando: se indica el 'elemento' y hasta dos argumentos.",["mando","elemento","arg1","arg2"],"",sig="IIII")
D("JOY_SET","Cambia un ajuste de un mando: se indica el 'elemento' y hasta tres argumentos.",["mando","elemento","arg1","arg2","arg3"],"",sig="IIIII")
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
