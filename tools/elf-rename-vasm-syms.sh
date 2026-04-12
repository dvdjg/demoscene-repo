#!/usr/bin/env bash
# Renombra símbolos globales «_Nombre» → «Nombre» en un .o ELF generado por vasm.
# GCC m68k-amiga-elf usa nombres C sin barra baja inicial; xdef en sintaxis Amiga
# exporta con prefijo _. Solo toca símbolos definidos (no UND), globales, un solo '_'.

set -euo pipefail
OBJCOPY=${1:?missing arg: objcopy}
OBJDUMP=${2:?missing arg: objdump}
OBJ=${3:?missing arg: object file}

tmp=$(mktemp "${TMPDIR:-/tmp}/vasmelf.XXXXXX.o")
cp -f "$OBJ" "$tmp"

args=()
while read -r old new; do
	[[ -z "$old" || -z "$new" ]] && continue
	args+=(--redefine-sym "${old}=${new}")
done < <("$OBJDUMP" -t "$tmp" | awk '$2 == "g" && $NF ~ /^_[^_]/ { old = $NF; new = substr(old, 2); print old " " new }')

if ((${#args[@]} > 0)); then
	"$OBJCOPY" "${args[@]}" "$tmp" "$OBJ"
fi
rm -f "$tmp"
