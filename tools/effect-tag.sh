#!/usr/bin/env sh
# Primer argumento: ruta absoluta al .c o .cpp del efecto. Escribe el identificador
# de EFFECT(Nombre, ...) en stdout (sin espacios). Salida vacía si no hay coincidencia.
set -eu
test "$#" -ge 1 || exit 0
grep -E '^[[:space:]]*EFFECT\(' "$1" | head -1 | sed 's/.*EFFECT(//' | sed 's/,.*//' | tr -d ' \t\r'
