#!/usr/bin/env python3
"""Genera la documentacion web: docs/web/index.html (un solo fichero, sin servidor).

Une tres cosas:
  - las guias de tools/guias/*.md (con ejemplos que verificar_ejemplos.py compila),
  - la referencia de la API (gen_api.leer() + tools/api_docs.json),
  - un buscador y la navegacion, todo en el cliente.
"""
import glob, html, json, os, re, sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import gen_api  # noqa: E402

SALIDA = os.path.join(AQUI, "..", "docs", "web", "index.html")

# categoria -> guia que la explica con ejemplo
GUIA_DE = {
    "scene": "01-primeros-pasos", "camera": "01-primeros-pasos", "light": "01-primeros-pasos",
    "entity": "02-procesos-3d", "char": "04-personaje", "collider": "04-personaje",
    "rigidbody": "05-fisica", "model": "06-modelos", "load": "06-modelos",
    "sprite": "07-sprites", "vehicle": "08-vehiculo", "road": "10-carreras-por-codigo",
}


# ---------------------------------------------------------------- markdown minimo
def inline(s):
    s = html.escape(s, quote=False)
    s = re.sub(r"`([^`]+)`", lambda m: "<code>%s</code>" % m.group(1), s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"\[([^\]]+)\]\((#[^)]+)\)", r'<a href="\2">\1</a>', s)
    s = re.sub(r"\[([^\]]+)\]\((https?://[^)]+)\)", r'<a href="\2" rel="noopener" target="_blank">\1</a>', s)
    return s


def md(texto):
    out, lineas, i = [], texto.split("\n"), 0
    titulo = ""
    while i < len(lineas):
        l = lineas[i]
        m = re.match(r"```(\w*)\s*(\w*)", l)
        if m:
            lang, marca = m.group(1), m.group(2)
            cod = []
            i += 1
            while i < len(lineas) and not lineas[i].startswith("```"):
                cod.append(lineas[i]); i += 1
            i += 1
            cls = "lang-%s" % (lang or "txt")
            ins = '<span class="verif" title="Este ejemplo se compila con bgdc al generar la documentacion">compila</span>' if marca == "compila" else ""
            out.append('<div class="bloque">%s<button class="copiar" type="button">Copiar</button><pre><code class="%s">%s</code></pre></div>'
                       % (ins, cls, html.escape("\n".join(cod))))
            continue
        m = re.match(r"(#{1,3}) (.*)", l)
        if m:
            n = len(m.group(1))
            if n == 1 and not titulo:
                titulo = m.group(2)
            ident = re.sub(r"[^a-z0-9]+", "-", m.group(2).lower()).strip("-")
            out.append("<h%d id=\"%s\">%s</h%d>" % (n, ident, inline(m.group(2)), n)); i += 1; continue
        if l.startswith("|"):
            filas = []
            while i < len(lineas) and lineas[i].startswith("|"):
                filas.append([c.strip() for c in lineas[i].strip().strip("|").split("|")]); i += 1
            cab, cuerpo = filas[0], [f for f in filas[2:]]
            out.append("<div class=\"tabla\"><table><thead><tr>%s</tr></thead><tbody>%s</tbody></table></div>" % (
                "".join("<th>%s</th>" % inline(c) for c in cab),
                "".join("<tr>%s</tr>" % "".join("<td>%s</td>" % inline(c) for c in f) for f in cuerpo)))
            continue
        if l.startswith("- "):
            items = []
            while i < len(lineas) and lineas[i].startswith("- "):
                items.append(inline(lineas[i][2:])); i += 1
            out.append("<ul>%s</ul>" % "".join("<li>%s</li>" % x for x in items)); continue
        if l.startswith("> "):
            q = []
            while i < len(lineas) and lineas[i].startswith("> "):
                q.append(lineas[i][2:]); i += 1
            out.append("<blockquote>%s</blockquote>" % inline(" ".join(q))); continue
        if l.strip() == "":
            i += 1; continue
        p = []
        while i < len(lineas) and lineas[i].strip() and not re.match(r"(```|#{1,3} |\||- |> )", lineas[i]):
            p.append(lineas[i]); i += 1
        out.append("<p>%s</p>" % inline(" ".join(p)))
    return titulo, "\n".join(out)


# ---------------------------------------------------------------- datos
def datos():
    docs = json.load(open(os.path.join(AQUI, "api_docs.json"), encoding="utf-8"))
    guias = []
    for f in sorted(glob.glob(os.path.join(AQUI, "guias", "*.md"))):
        t, h = md(open(f, encoding="utf-8").read())
        guias.append({"id": os.path.basename(f)[:-3], "t": t, "h": h})
    api = []
    for f in gen_api.leer():
        d = docs.get(f["nombre"], {})
        api.append({
            "n": f["nombre"].lower(), "m": f["modulo"], "c": f["cat"],
            "p": f["params"], "pn": d.get("params", []), "r": f["ret"],
            "d": d.get("desc", ""), "rd": d.get("ret", ""), "e": d.get("ejemplo", ""),
            "g": GUIA_DE.get(f["cat"], "") if f["modulo"] == "3d" else "",
        })
    return {"guias": guias, "api": api, "doc": sum(1 for a in api if a["d"])}


PLANTILLA = r'''<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>BennuGD2 3D - Documentacion</title>
<style>
:root{--bg:#fbfbfa;--panel:#f2f2ef;--tx:#1d1f24;--mut:#666b75;--bd:#dcdcd6;--ac:#1f5fbf;--acbg:#e6eefb;--cod:#f4f4f1;--ok:#1b7a43;--kw:#a02a8a;--st:#8a5a00;--cm:#6b7280;--fn:#1f5fbf;--nu:#0b7a75}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--bg:#15171b;--panel:#1b1e23;--tx:#e6e7ea;--mut:#9aa0ab;--bd:#2c3038;--ac:#7db0ff;--acbg:#1f2c44;--cod:#1f2227;--ok:#5cd08a;--kw:#d78be0;--st:#e3b062;--cm:#7c8594;--fn:#7db0ff;--nu:#6fd3cc}}
:root[data-theme="dark"]{--bg:#15171b;--panel:#1b1e23;--tx:#e6e7ea;--mut:#9aa0ab;--bd:#2c3038;--ac:#7db0ff;--acbg:#1f2c44;--cod:#1f2227;--ok:#5cd08a;--kw:#d78be0;--st:#e3b062;--cm:#7c8594;--fn:#7db0ff;--nu:#6fd3cc}
*{box-sizing:border-box}html{scroll-behavior:smooth}
body{margin:0;background:var(--bg);color:var(--tx);font:16px/1.6 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
a{color:var(--ac)}code,pre{font-family:ui-monospace,"JetBrains Mono",Menlo,Consolas,monospace}
header{position:sticky;top:0;z-index:5;display:flex;gap:12px;align-items:center;padding:10px 16px;background:var(--bg);border-bottom:1px solid var(--bd)}
header h1{font-size:16px;margin:0;white-space:nowrap}
#q{flex:1;min-width:0;padding:8px 12px;border:1px solid var(--bd);border-radius:8px;background:var(--panel);color:var(--tx);font:inherit}
button{font:inherit;color:var(--tx);background:var(--panel);border:1px solid var(--bd);border-radius:8px;padding:6px 10px;cursor:pointer}
#menu{display:none}
.layout{display:grid;grid-template-columns:290px minmax(0,1fr)}
nav{position:sticky;top:57px;align-self:start;height:calc(100vh - 57px);overflow:auto;padding:14px 12px 40px 16px;border-right:1px solid var(--bd);background:var(--panel);font-size:14px}
nav h2{font-size:12px;text-transform:uppercase;letter-spacing:.06em;color:var(--mut);margin:18px 0 6px}
nav a{display:block;padding:3px 8px;border-radius:6px;color:var(--tx);text-decoration:none}
nav a:hover{background:var(--acbg)}nav a.act{background:var(--acbg);color:var(--ac);font-weight:600}
nav details{margin:2px 0}nav summary{cursor:pointer;padding:3px 8px;border-radius:6px}nav summary:hover{background:var(--acbg)}
nav details a{padding-left:22px}
nav .n{color:var(--mut);font-size:12px}
main{padding:28px clamp(16px,4vw,56px) 80px;max-width:960px;min-width:0}
main h1{font-size:30px;line-height:1.2;margin:0 0 16px}main h2{font-size:22px;margin:34px 0 10px;padding-top:6px;border-top:1px solid var(--bd)}main h3{font-size:17px;margin:24px 0 6px}
.bloque{position:relative;margin:14px 0}
.bloque pre{margin:0;padding:14px 16px;background:var(--cod);border:1px solid var(--bd);border-radius:10px;overflow:auto;font-size:13.5px;line-height:1.5}
.copiar{position:absolute;top:8px;right:8px;font-size:12px;padding:3px 8px;opacity:.0;transition:opacity .15s}.bloque:hover .copiar,.copiar:focus{opacity:1}
.verif{position:absolute;top:-9px;left:12px;font-size:11px;padding:0 8px;border-radius:99px;background:var(--bg);border:1px solid var(--ok);color:var(--ok)}
.verif:before{content:"\2713  "}
p code,li code,td code,h2 code,h3 code{background:var(--cod);border:1px solid var(--bd);border-radius:5px;padding:0 5px;font-size:.9em}
.tabla{overflow:auto;margin:12px 0}table{border-collapse:collapse;width:100%;font-size:14.5px}th,td{padding:7px 12px;border:1px solid var(--bd);text-align:left;vertical-align:top}th{background:var(--panel)}
blockquote{margin:14px 0;padding:8px 16px;border-left:3px solid var(--ac);background:var(--acbg);border-radius:0 8px 8px 0}
.firma{font-size:18px;padding:12px 16px;background:var(--cod);border:1px solid var(--bd);border-radius:10px;overflow:auto;margin:8px 0 18px}
.etq{display:inline-block;font-size:12px;padding:1px 9px;border-radius:99px;background:var(--acbg);color:var(--ac);margin-right:6px}
.aviso{color:var(--mut);font-style:italic}
.lista a{display:block;padding:8px 12px;border:1px solid var(--bd);border-radius:8px;margin:6px 0;text-decoration:none;color:var(--tx)}.lista a:hover{background:var(--acbg)}
.lista .d{color:var(--mut);font-size:14px}
.k{color:var(--kw)}.s{color:var(--st)}.c{color:var(--cm);font-style:italic}.f{color:var(--fn)}.u{color:var(--nu)}
#res{display:none;position:absolute;left:16px;right:16px;top:52px;max-height:70vh;overflow:auto;background:var(--bg);border:1px solid var(--bd);border-radius:10px;box-shadow:0 12px 40px rgba(0,0,0,.25)}
#res a{display:block;padding:8px 14px;text-decoration:none;color:var(--tx);border-bottom:1px solid var(--bd)}#res a:hover,#res a.sel{background:var(--acbg)}
#res .d{color:var(--mut);font-size:13px}
@media (max-width:820px){.layout{grid-template-columns:1fr}nav{position:fixed;left:0;top:57px;width:290px;z-index:4;transform:translateX(-100%);transition:transform .2s}nav.abierto{transform:none}#menu{display:inline-block}}
</style></head><body>
<header><button id="menu" type="button" aria-label="Menu">&#9776;</button><h1>BennuGD2 3D</h1>
<input id="q" type="search" placeholder="Buscar funcion o tema (pulsa /)" autocomplete="off" spellcheck="false">
<button id="tema" type="button" title="Cambiar tema">Tema</button><div id="res"></div></header>
<div class="layout"><nav id="nav"></nav><main id="main"></main></div>
<script>
const D=__DATOS__;
const MOD={"3d":"Motor 3D","gfx":"Graficos 2D","input":"Entrada","sound":"Sonido","video":"Video","misc":"Misc","net":"Red","ads":"Anuncios","iap":"Compras","debug":"Depuracion"};
const $=s=>document.querySelector(s),esc=s=>s.replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const byName={};D.api.forEach(a=>byName[a.n]=a);
const TIPO={I:"int",D:"double",F:"float",S:"string",P:"pointer",W:"word",B:"byte",V:"..."};
function firma(a){const ps=a.p.map((t,i)=>{const n=a.pn[i]||"";return '<span class="u">'+esc(t)+'</span>'+(n?' '+esc(n):'')}).join(", ");return '<span class="f">'+a.n+'</span>('+ps+')'+(a.r?' &rarr; <span class="u">'+a.r+'</span>':'')}
// ---- resaltado de codigo BennuGD2
const KW=new Set("program import global local private public const declare begin end process function if else elseif then while repeat until for from to step loop switch case default return break continue frame clone and or not xor mod int float double string byte word dword pointer struct type".split(" "));
function pinta(t){return t.replace(/(\/\/[^\n]*|\/\*[\s\S]*?\*\/)|("(?:[^"\\\n]|\\.)*")|\b([A-Za-z_][A-Za-z0-9_]*)\b|\b(\d+(?:\.\d+)?)\b/g,(m,c,s,w,n)=>{
 if(c)return '<span class="c">'+esc(c)+'</span>';if(s)return '<span class="s">'+esc(s)+'</span>';
 if(n)return '<span class="u">'+n+'</span>';
 const l=w.toLowerCase();if(KW.has(l))return '<span class="k">'+w+'</span>';
 if(byName[l])return '<a class="f" style="text-decoration:none" href="#api/'+l+'">'+w+'</a>';return w;})}
function codigo(raiz){raiz.querySelectorAll("code.lang-bennugd").forEach(c=>{if(!c.dataset.p){c.dataset.p=1;c.innerHTML=pinta(c.textContent)}});
 raiz.querySelectorAll(".copiar").forEach(b=>b.onclick=()=>{const t=b.parentElement.querySelector("code").textContent;try{navigator.clipboard.writeText(t);b.textContent="Copiado";setTimeout(()=>b.textContent="Copiar",1200)}catch(e){}})}
// ---- navegacion
function nav(){let h='<h2>Guias</h2>'+D.guias.map(g=>'<a data-r="guia/'+g.id+'" href="#guia/'+g.id+'">'+esc(g.t)+'</a>').join("");
 h+='<h2>Referencia ('+D.api.length+' funciones)</h2>';const mods={};D.api.forEach(a=>{(mods[a.m]=mods[a.m]||{})[a.c]=(mods[a.m][a.c]||0)+1});
 Object.keys(MOD).filter(m=>mods[m]).forEach(m=>{const cats=Object.keys(mods[m]).sort();const tot=cats.reduce((s,c)=>s+mods[m][c],0);
  h+='<details data-m="'+m+'"><summary>'+MOD[m]+' <span class="n">'+tot+'</span></summary>'+cats.map(c=>'<a data-r="cat/'+m+'/'+c+'" href="#cat/'+m+'/'+c+'">'+(c=="general"?"general":c)+' <span class="n">'+mods[m][c]+'</span></a>').join("")+'</details>'});
 $("#nav").innerHTML=h}
function marca(r){document.querySelectorAll("nav a").forEach(a=>a.classList.toggle("act",a.dataset.r===r));const m=r.split("/");
 if(m[0]=="cat"||m[0]=="api"){const mod=m[0]=="cat"?m[1]:(byName[m[1]]||{}).m;const d=document.querySelector('nav details[data-m="'+mod+'"]');if(d)d.open=true}}
function lista(items){return '<div class="lista">'+items.map(a=>'<a href="#api/'+a.n+'"><code>'+firma(a)+'</code>'+(a.d?'<div class="d">'+esc(a.d.length>150?a.d.slice(0,147)+"...":a.d)+'</div>':'<div class="d aviso">Sin documentar todavia</div>')+'</a>').join("")+'</div>'}
function pagina(){const r=location.hash.slice(1)||"guia/"+D.guias[0].id;const p=r.split("/");let h="",t="";
 if(p[0]=="guia"){const g=D.guias.find(x=>x.id==p[1])||D.guias[0];t=g.t;h=g.h;const i=D.guias.indexOf(g);
  h+='<hr style="border:0;border-top:1px solid var(--bd);margin:40px 0 16px">'+(i>0?'<a href="#guia/'+D.guias[i-1].id+'">&larr; '+esc(D.guias[i-1].t)+'</a>':'')+(i<D.guias.length-1?'<a style="float:right" href="#guia/'+D.guias[i+1].id+'">'+esc(D.guias[i+1].t)+' &rarr;</a>':'')}
 else if(p[0]=="cat"){const it=D.api.filter(a=>a.m==p[1]&&a.c==p[2]);t=(MOD[p[1]]||p[1])+" / "+p[2];const g=it[0]&&it[0].g&&D.guias.find(x=>x.id==it[0].g);
  h='<h1>'+esc(t)+'</h1>'+(g?'<p>Guia con ejemplo: <a href="#guia/'+g.id+'">'+esc(g.t)+'</a></p>':'')+lista(it)}
 else if(p[0]=="api"){const a=byName[p[1]];if(!a){h='<h1>No existe</h1><p>No hay ninguna funcion llamada <code>'+esc(p[1])+'</code>.</p>'}else{t=a.n;
  const g=a.g&&D.guias.find(x=>x.id==a.g);
  h='<p><span class="etq">'+(MOD[a.m]||a.m)+'</span><span class="etq">'+a.c+'</span></p><h1>'+a.n+'</h1><div class="firma">'+firma(a)+'</div>';
  h+=a.d?'<p>'+esc(a.d)+'</p>':'<p class="aviso">Esta funcion aun no esta documentada: solo se conoce su firma (los tipos de sus parametros).</p>';
  if(a.p.length&&a.pn.length){h+='<h3>Parametros</h3><div class="tabla"><table><thead><tr><th>Nombre</th><th>Tipo</th></tr></thead><tbody>'+a.p.map((t,i)=>'<tr><td><code>'+esc(a.pn[i]||"?")+'</code></td><td>'+esc(t)+'</td></tr>').join("")+'</tbody></table></div>'}
  if(a.rd||a.r)h+='<h3>Devuelve</h3><p>'+(a.rd?esc(a.rd):'<code>'+a.r+'</code>')+'</p>';
  if(a.e)h+='<h3>Ejemplo</h3><div class="bloque"><button class="copiar" type="button">Copiar</button><pre><code class="lang-bennugd">'+esc(a.e)+'</code></pre></div>';
  if(g)h+='<h3>Mas ejemplos</h3><p>Programa completo con esta funcion: <a href="#guia/'+g.id+'">'+esc(g.t)+'</a>.</p>';
  const rel=D.api.filter(x=>x.m==a.m&&x.c==a.c&&x.n!=a.n).slice(0,12);if(rel.length)h+='<h3>Mismo grupo</h3>'+lista(rel)}}
 $("#main").innerHTML=h;document.title=(t?t+" - ":"")+"BennuGD2 3D";codigo($("#main"));marca(r);window.scrollTo(0,0);$("#nav").classList.remove("abierto")}
// ---- buscador
let sel=0;function busca(){const q=$("#q").value.trim().toLowerCase(),R=$("#res");if(!q){R.style.display="none";return}
 const ts=q.split(/\s+/);const pun=a=>{let s=0;const txt=(a.n+" "+a.d+" "+a.c).toLowerCase();for(const t of ts){if(a.n==t)s+=100;else if(a.n.startsWith(t))s+=40;else if(a.n.includes(t))s+=20;else if(txt.includes(t))s+=5;else return 0}return s+(a.d?2:0)};
 const gs=D.guias.filter(g=>ts.every(t=>(g.t+g.h).toLowerCase().includes(t))).slice(0,4);
 const as=D.api.map(a=>[pun(a),a]).filter(x=>x[0]).sort((x,y)=>y[0]-x[0]).slice(0,40).map(x=>x[1]);sel=0;
 R.innerHTML=gs.map(g=>'<a href="#guia/'+g.id+'"><b>Guia:</b> '+esc(g.t)+'</a>').join("")+as.map(a=>'<a href="#api/'+a.n+'"><code>'+a.n+'</code> <span class="d">'+esc((a.d||"").slice(0,90))+'</span></a>').join("")||'<a>Sin resultados</a>';R.style.display="block"}
$("#q").addEventListener("input",busca);
$("#q").addEventListener("keydown",e=>{const l=[...document.querySelectorAll("#res a[href]")];if(e.key=="Escape"){$("#res").style.display="none";$("#q").blur()}
 else if(e.key=="ArrowDown"||e.key=="ArrowUp"){e.preventDefault();sel=(sel+(e.key=="ArrowDown"?1:-1)+l.length)%l.length;l.forEach((a,i)=>a.classList.toggle("sel",i==sel));l[sel]&&l[sel].scrollIntoView({block:"nearest"})}
 else if(e.key=="Enter"&&l[sel]){location.hash=l[sel].getAttribute("href");$("#res").style.display="none";$("#q").blur()}});
document.addEventListener("click",e=>{if(!e.target.closest("#res")&&e.target.id!="q")$("#res").style.display="none";if(e.target.closest("#res a"))$("#res").style.display="none"});
document.addEventListener("keydown",e=>{if(e.key=="/"&&document.activeElement.tagName!="INPUT"){e.preventDefault();$("#q").focus()}});
$("#menu").onclick=()=>$("#nav").classList.toggle("abierto");
$("#tema").onclick=()=>{const r=document.documentElement;const osOscuro=matchMedia("(prefers-color-scheme: dark)").matches;const actual=r.dataset.theme||(osOscuro?"dark":"light");const n=actual=="dark"?"light":"dark";r.dataset.theme=n;try{localStorage.setItem("tema",n)}catch(e){}};
try{const t=localStorage.getItem("tema");if(t)document.documentElement.dataset.theme=t}catch(e){}
addEventListener("hashchange",pagina);nav();pagina();
</script></body></html>
'''


def main():
    d = datos()
    os.makedirs(os.path.dirname(SALIDA), exist_ok=True)
    # '</' dentro de un <script> cerraria la etiqueta: se escapa
    js = json.dumps(d, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    open(SALIDA, "w", encoding="utf-8").write(PLANTILLA.replace("__DATOS__", js))
    print("%s: %d guias, %d funciones (%d documentadas), %d KB" % (
        os.path.relpath(SALIDA), len(d["guias"]), len(d["api"]), d["doc"], os.path.getsize(SALIDA) // 1024))


if __name__ == "__main__":
    main()
