#!/usr/bin/env bash
# Ejecuta un comando con el entorno demoscene activo.
#
# Por defecto: intenta activate-barto (extensión BartmanAbyss, m68k-amiga-elf).
# Si falla (p. ej. Linux sin extensión), usa activate (demoscene-toolchain .deb).
#
# Para forzar solo el toolchain Linux: crea .force-linux-demoscene-toolchain en la raíz.
set -eo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if [ -f "${ROOT}/.force-linux-demoscene-toolchain" ]; then
  # shellcheck source=/dev/null
  source "${ROOT}/activate"
else
  # shellcheck source=/dev/null
  if source "${ROOT}/activate-barto" 2>/dev/null; then
    :
  else
    # shellcheck source=/dev/null
    source "${ROOT}/activate"
  fi
fi
exec "$@"
