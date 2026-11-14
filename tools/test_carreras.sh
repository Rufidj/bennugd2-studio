#!/bin/bash
# Prueba de regresion del modo Carreras, sin ventanas del editor: genera un circuito de ejemplo, le pone rivales distintos
# y corre una carrera completa con el bot como jugador. Falla si no termina la carrera, si hay errores en el log o si algun
# rival no avanza. Uso: tools/test_carreras.sh   (desde modules/editor3d, con el editor y bgdi ya compilados)
set -u
AQUI="$(cd "$(dirname "$0")/.." && pwd)"
ED="$AQUI/bin/editor3d"
BIN="${BGD_BIN:-$AQUI/../../build/linux-gnu/bin}"
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
fallo=0; ok() { echo "  ok:    $1"; }; mal() { echo "  FALLO: $1"; fallo=1; }

[ -x "$ED" ] || { echo "no existe $ED (compila el editor)"; exit 2; }
[ -x "$BIN/bgdi" ] || { echo "no existe $BIN/bgdi (BGD_BIN=carpeta de bgdi)"; exit 2; }

echo "1. generar el circuito de ejemplo"
mkdir -p "$T/Scenes" "$T/Scripts" "$T/Assets"
( cd "$AQUI/bin" && EDITOR_PROYECTO="$T" EDITOR_MODO=6 EDITOR_GENERAR=1 timeout 300 ./editor3d > "$T/gen.log" 2>&1 )
grep -q 'EDITOR_GENERAR: compilado' "$T/gen.log" && ok "el juego se genera y compila" || { mal "no compila"; tail -5 "$T/gen.log"; exit 1; }

echo "2. rivales distintos"
K="$(ls "$T"/Scenes/*.kart | head -1)"
sed -i '/^piloto=/d;/^rivales=/d;/^jugador=/d' "$K"
sed -i '0,/^kart=/s//piloto=0.98 0.8 0.7 1.08 1.05 1.0 1.0 1.4 0 Bruto|\npiloto=0.92 0.4 0.4 1.0 1.0 1.05 1.1 1.0 0 Ana|\npiloto=0.86 0.2 0.8 0.92 1.1 1.1 0.9 0.8 0 Lento|\njugador=1 1 1 1 1 |Tu\nkart=/' "$K"
( cd "$AQUI/bin" && EDITOR_PROYECTO="$T" EDITOR_MODO=6 EDITOR_GENERAR=1 timeout 300 ./editor3d > "$T/gen2.log" 2>&1 )
grep -q 'kr_n_ia = 3' "$T/main.prg" && ok "3 pilotos generados" || mal "no salen los 3 pilotos"

echo "3. carrera completa con el bot"
( cd "$T" && rm -f carreras.log && PATH="$BIN:$PATH" LD_LIBRARY_PATH="$BIN" timeout 240 "$BIN/bgdi" main.dcb bot > "$T/run.log" 2>&1 )
L="$T/carreras.log"
grep -q 'carrera terminada' "$L" && ok "el bot termina la carrera" || mal "la carrera no termina"
grep -q 'ERR ' "$L" && mal "hay errores en el log: $(grep 'ERR ' "$L" | head -2)" || ok "sin errores en el log"
for i in 1 2 3; do
  av=$(grep "kart $i:" "$L" | tail -1 | sed 's/.*avance=//')
  [ -n "$av" ] && awk "BEGIN{exit !($av > 50)}" && ok "el rival $i avanza ($av)" || mal "el rival $i no avanza (avance=${av:-?})"
done
echo
[ $fallo = 0 ] && echo "TODO BIEN" || echo "HAY FALLOS"
exit $fallo
