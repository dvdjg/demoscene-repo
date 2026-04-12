#!/usr/bin/env bash
# Ejecuta una herramienta Go del repo (binario Linux x86-64 en tools/<name>/) bajo WSL.
# Uso desde MSYS/Windows: se compila una vez <name>-linux con go y se reenvían los argumentos.
# Rutas de ficheros se convierten con wslpath para el cwd y paths en línea de órdenes.

set -euo pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
TOOL=${1:?nombre de subdirectorio bajo tools/ (p. ej. png2c)}
shift
TOOLDIR="$ROOT/$TOOL"
BIN="$TOOLDIR/${TOOL}-linux"

if [[ ! -d "$TOOLDIR" ]]; then
	echo "wsl-go-tool: no existe $TOOLDIR" >&2
	exit 1
fi

WSLDIR=$(wsl -e wslpath -u "$TOOLDIR")
WSLBIN=$(wsl -e wslpath -u "$BIN")
WCWD=$(wsl -e wslpath -u "$(pwd)")

if [[ ! -f "$BIN" ]]; then
	echo "[wsl-go-tool] compilando $TOOL en WSL (go build)…" >&2
	wsl -e bash -c "cd $(printf %q "$WSLDIR") && go build -buildvcs=false -o ${TOOL}-linux ."
fi

# ¿Parece una ruta de fichero y debe traducirse a /mnt/c/... ?
is_probably_path() {
	local a=$1
	[[ "$a" == *,* ]] && return 1
	[[ "$a" == -* ]] && return 1
	[[ "$a" == +* ]] && return 1
	[[ -f "$a" || -d "$a" ]] && return 0
	[[ "$a" =~ ^-?[0-9]+\.[0-9]+$ ]] && return 1
	[[ "$a" == */* ]] && return 0
	[[ "$a" =~ ^[a-zA-Z]:/ ]] && return 0
	[[ "$a" =~ ^/[^/] ]] && return 0
	[[ "$a" == *.png || "$a" == *.obj || "$a" == *.svg || "$a" == *.tmx || "$a" == *.mod ]] && return 0
	[[ "$a" == *.c || "$a" == *.h || "$a" == *.json || "$a" == *.psfu || "$a" == *.2d ]] && return 0
	return 1
}

args=()
for arg in "$@"; do
	if is_probably_path "$arg"; then
		args+=("$(wsl -e wslpath -u "$(realpath -m "$arg")")")
	else
		args+=("$arg")
	fi
done

quoted=$(printf ' %q' "${args[@]}")
exec wsl -e bash -c "cd $(printf %q "$WCWD") && exec $(printf %q "$WSLBIN")$quoted"
