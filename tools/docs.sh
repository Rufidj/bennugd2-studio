#!/bin/sh
# Regenera la documentacion de la API: catalogo del editor (autocompletado), API.md y la web.
# Orden: descripciones -> catalogo -> verificar que los ejemplos compilan -> web.
set -e
cd "$(dirname "$0")"
python3 gen_docs_nucleo.py
python3 gen_docs_objetos.py
python3 gen_docs_carretera.py
python3 gen_docs_jolt.py      # corrige/amplia lo anterior: va el ultimo
python3 gen_api.py
python3 verificar_ejemplos.py
python3 gen_web.py
echo "Listo. Recompila el editor para que el autocompletado vea el catalogo nuevo."
