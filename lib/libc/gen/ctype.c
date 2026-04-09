/*
 * Minimal libc/runtime piece for bare-metal (gen/ctype.c).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include "ctype.h"

const char _ctype_[128] = {
  ['\t'] = _SPACE,
  ['\n'] = _SPACE,
  ['\v'] = _SPACE,
  ['\f'] = _SPACE,
  ['\r'] = _SPACE,
  [' '] = _SPACE,
  ['0'...'9'] = _DIGIT | _XDIGIT,
  ['A'...'F'] = _ALPHA | _XDIGIT, 
  ['G'...'Z'] = _ALPHA,
  ['a'...'f'] = _ALPHA | _XDIGIT,
  ['g'...'z'] = _ALPHA
};
