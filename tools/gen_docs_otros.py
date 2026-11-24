# Documentacion de libmod_net, libmod_ads, libmod_iap y libmod_debug.
import json, os
R = os.path.join(os.path.dirname(__file__), "api_docs.json")
d = json.load(open(R, encoding="utf-8"))
def D(n, desc, params=(), ret="", ejemplo="", sig=None):
    e = {"desc": desc}
    if params: e["params"] = list(params)
    if ret: e["ret"] = ret
    if ejemplo: e["ejemplo"] = ejemplo
    d[n if sig is None else "%s(%s)" % (n, sig)] = e

# ---- red (sockets TCP y UDP) ----
D("NET_OPEN","Abre una conexion de red y devuelve un manejador (0 si falla). El modo es 0 para SERVIDOR (espera conexiones y escucha en un puerto) o 1 para CLIENTE (se conecta a una direccion). El protocolo es 0 para UDP o 1 para TCP. Admite direcciones IPv4 e IPv6.",["modo","protocolo","direccion","puerto"],"manejador de la conexion, 0 si falla",
  "srv = net_open(0, 1, \"\", 5000);                // servidor TCP en el puerto 5000\ncli = net_open(1, 1, \"192.168.1.20\", 5000);   // cliente TCP")
D("NET_WAIT","Espera a que haya actividad en un grupo de conexiones (una lista creada con LIST_CREATE) hasta 'espera' milisegundos (-1 = sin limite) y deja en otra lista las que tienen novedades.",["conexiones","espera","eventos"],"cuantas conexiones tienen novedades, o -1 si hay error")
D("NET_SEND","Envia datos por una conexion: un bloque de memoria y su longitud en bytes.",["conexion","datos","bytes"],"bytes enviados, o -1 si hay error")
D("NET_RECV","Recibe datos de una conexion en un bloque de memoria de como mucho 'bytes' bytes.",["conexion","datos","bytes"],"bytes recibidos, o -1 si hay error")
D("NET_CLOSE","Cierra una conexion.",["conexion"])
D("NET_GETAVAILABLEBYTES","Bytes que hay disponibles para leer en una conexion.",["conexion"],"bytes")
D("NET_GETRECEIVEBUFFERSIZE","Tamano del buffer de recepcion de una conexion.",["conexion"],"bytes")
D("NET_GETREMOTEADDR","Direccion IP del otro extremo de una conexion.",["conexion"],"la direccion")
D("NET_IS_NEW_CONNECTION","Indica, para una conexion que ha salido de NET_WAIT, si es un cliente nuevo que se ha conectado a un servidor.",["conexion"],"1 si es una conexion nueva")
D("NET_IS_MESSAGE_INCOMING","Indica, para una conexion que ha salido de NET_WAIT, si trae un mensaje para leer.",["conexion"],"1 si hay un mensaje")

# ---- anuncios (AdMob; solo Android: en el resto de sistemas las funciones existen pero no hacen nada y devuelven 0) ----
ANDROID = " Solo funciona en Android (AdMob); en el resto de sistemas no hace nada y devuelve 0, asi el mismo programa compila en todas partes."
D("ADS_INITIALIZE","Inicia los anuncios. Hay que llamarla antes que las demas ADS_*."+ANDROID,ret="1 si va bien")
D("ADS_LOAD_INTERSTITIAL","Pide un anuncio a pantalla completa (intersticial) con el identificador de unidad de AdMob. Conviene hacerlo con tiempo (al empezar el nivel) porque tarda en cargar."+ANDROID,["id_unidad"])
D("ADS_IS_INTERSTITIAL_LOADED","Indica si el intersticial ya esta cargado y listo para mostrarse."+ANDROID,ret="1 si esta listo")
D("ADS_SHOW_INTERSTITIAL","Muestra el anuncio intersticial cargado."+ANDROID)
D("ADS_LOAD_REWARDED","Pide un anuncio con recompensa (el usuario lo ve a cambio de algo) con el identificador de unidad de AdMob."+ANDROID,["id_unidad"])
D("ADS_IS_REWARDED_LOADED","Indica si el anuncio con recompensa esta cargado."+ANDROID,ret="1 si esta listo")
D("ADS_SHOW_REWARDED","Muestra el anuncio con recompensa. El resultado se consulta con ADS_POLL_EVENT."+ANDROID)
D("ADS_SHOW_BANNER","Muestra un banner fijo con el identificador de unidad de AdMob, arriba (ADS_BANNER_POS_TOP) o abajo (ADS_BANNER_POS_BOTTOM) de la pantalla."+ANDROID,["id_unidad","posicion"])
D("ADS_HIDE_BANNER","Oculta el banner."+ANDROID)
D("ADS_POLL_EVENT","Devuelve el codigo de un evento de anuncios ocurrido (por ejemplo ADS_EVENT_CONSENT_FORM_DISMISSED, que vale 8, al cerrarse el formulario de consentimiento), o 0 si no hay ninguno."+ANDROID,ret="codigo del evento, 0 si no hay")
D("ADS_CHECK_CONSENT","Comprueba si hace falta pedir al usuario el consentimiento de privacidad (obligatorio en la Union Europea)."+ANDROID)
D("ADS_SHOW_CONSENT_FORM","Muestra el formulario de consentimiento de privacidad. Al cerrarse lo avisa el evento ADS_EVENT_CONSENT_FORM_DISMISSED."+ANDROID)
D("ADS_SET_CHILD_DIRECTED","Marca la aplicacion como dirigida a ninos (1) o no (0), para que los anuncios cumplan la normativa de proteccion de menores."+ANDROID,["dirigida_a_ninos"])

# ---- compras dentro de la aplicacion (solo Android) ----
IAP = " Solo funciona en Android (Google Play Billing); en el resto de sistemas no hace nada."
D("IAP_INITIALIZE","Inicia el sistema de compras dentro de la aplicacion. Hay que llamarla antes que las demas IAP_*."+IAP,ret="1 si va bien")
D("IAP_PURCHASE","Inicia la compra de un producto por su identificador (SKU). El resultado llega despues como un evento de IAP_POLL_EVENT."+IAP,["sku"])
D("IAP_CONSUME","Consume un producto ya comprado (para poder comprarlo otra vez, como las monedas)."+IAP,["sku"])
D("IAP_GET_PRICE","Precio de un producto, con la moneda y el formato de la tienda."+IAP,["sku"],"el precio como texto")
D("IAP_POLL_EVENT","Devuelve el codigo de un evento de compras ocurrido, o 0 si no hay ninguno."+IAP,ret="codigo del evento, 0 si no hay")
D("IAP_GET_LAST_PURCHASE_SKU","Identificador (SKU) de la ultima compra realizada."+IAP,ret="el SKU")
D("IAP_GET_LAST_PURCHASE_TOKEN","Token de la ultima compra realizada, para validarla en un servidor."+IAP,ret="el token")

# ---- depuracion ----
D("TRACE","Activa (1) o desactiva (0) el modo de seguimiento del depurador integrado.",["activar"])
json.dump(d, open(R, "w", encoding="utf-8"), ensure_ascii=False, indent=1, sort_keys=True)
print(len(d), "entradas")
