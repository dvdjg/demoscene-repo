/*
 * Common typedefs/macros used across the demoscene framework.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include <types.h>

/*
 * --- GCC statement-expression macros (safe min/max/abs/swap) ---
 * Technique: `({ ... })` runs in its own scope so arguments are evaluated once
 * (unlike naive `#define max(a,b) ((a)<(b)?(a):(b))` which can double-evaluate).
 * C equivalent of abs: `((x)<0?-(x):(x))` with a temp for side-effect safety.
 */
#define abs(x)                                                                 \
  ({                                                                           \
    typeof(x) _x = (x);                                                        \
    (_x < 0) ? -_x : _x;                                                       \
  })

#define min(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a < _b ? _a : _b;                                                         \
  })

#define max(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a > _b ? _a : _b;                                                         \
  })

#define swap(a, b)                                                             \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(a) _b = (b);                                                        \
    (a) = _b;                                                                  \
    (b) = _a;                                                                  \
  })

/* Round integer x up/down to next multiple of y (y>0). Pure C; used for alignment. */
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y) (((x) / (y)) * (y))

/* Element count of a static array. C equivalent: same division; wrong if arg is pointer. */
#define nitems(x) (sizeof((x)) / sizeof((x)[0]))

/* Counts macro arguments (up to 10). Classic preprocessor trick; no C equivalent. */
#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/* Unrolled short for-loop for demos; expands to a for loop with a short index. */
#define ITER(_VAR, _BEGIN, _END, _EXPR) { \
  short _VAR; \
  for (_VAR = _BEGIN; _VAR <= _END; _VAR++) { \
    _EXPR; \
  } \
}

/*
 * getword / getlong — read word/long from table at scaled index.
 * Why asm: M68000 `move.w (An,Dn.w)` gives one instruction indexed fetch; GCC
 * often emits extra shifts for `((short*)tab)[idx]`. C equivalent:
 *   `*(short*)((char*)tab + idx*2)` (getword) or *4 for long.
 * assumes abs(idx) < 32768 (getword) / 16384 (getlong) so scaled index fits.
 */
static inline short getword(const void *tab, short idx) {
  short res;
  idx += idx;
  asm("movew (%2,%1:w),%0"
      : "=r" (res)
      : "d" (idx), "a" (tab));
  return res;
}

/* assumes that abs(idx) < 16384 */
static inline int getlong(const void *tab, short idx) {
  int res;
  idx += idx;
  /* Compiler barrier between half-index adjustments so idx is not merged wrongly. */
  asm("" ::: "memory");
  idx += idx;
  asm("movel (%2,%1:w),%0"
      : "=r" (res)
      : "d" (idx), "a" (tab));
  return res;
}

/* Pointer from table of longs (e.g. stab symbol table). */
#define getptr(tab, idx) ((void *)getlong(tab, idx))

static inline short absw(short a) {
  if (a < 0)
    return -a;
  return a;
}

/*
 * swap16 — swap upper/lower 16 bits of a 32-bit register (one `swap` opcode).
 * C equivalent: `((a & 0xffff) << 16) | (a >> 16)`; asm is one cycle on 68000.
 */
static inline u_int swap16(u_int a) {
  asm("swap %0": "+d" (a));
  return a;
}

/* Byte-swap 16-bit value. Pure C. */
static inline u_short swap8(u_short a) {
  return (a << 8) | (a >> 8);
}

/*
 * div16, mod16, udiv16, umod16, mul16 — 32/16 division and 16×16→32 multiply.
 * Asm uses native M68000 divs/divu/muls; avoids libgcc and matches cycle expectations.
 * C equivalent: cast and use / % *; mul16 C equivalent: `(int)a * (int)b`.
 */
static inline short div16(int a, short b) {
  short r;
  asm("divs %2,%0"
      : "=d" (r)
      : "0" (a), "dm" (b));
  return r;
}

/* Remainder after division: divs puts quotient in low word, remainder in high word; swap extracts it. */
static inline short mod16(int a, short b) {
  short r;
  asm("divs %2,%0\n\t"
      "swap %0"
      : "=d" (r)
      : "0" (a), "dm" (b));
  return r;
}

static inline u_short udiv16(u_int a, u_short b) {
  u_short r;
  asm("divu %2,%0"
      : "=d" (r)
      : "0" (a), "dm" (b));
  return r;
}

/* NOTE: possible issue: umod16 uses divs (signed); intended for unsigned paths — verify caller ranges. */
static inline u_short umod16(u_int a, u_short b) {
  u_short r;
  asm("divs %2,%0\n\t"
      "swap %0"
      : "=d" (r)
      : "0" (a), "dm" (b));
  return r;
}

static inline int mul16(short a, short b) {
  int r;
  asm("muls %2,%0"
      : "=d" (r)
      : "0" (a), "dm" (b));
  return r;
}

/*
 * divmod16 — quotient and remainder in one divs (faster than two separate calls).
 * _n: int dividend, _d: short divisor, outputs _q quotient, _r remainder (words).
 */
#define divmod16(_n, _d, _q, _r)                                               \
  asm("divs %3,%0\n\t"                                                         \
      "move.w %0,%1\n\t"                                                       \
      "swap %0"                                                                \
      : "=d" (_r), "=d" (_q)                                                   \
      : "0" (_n), "d" (_d));

/*
 * bclr/bset/bchg — test bit in memory and clear/set/toggle (68000 bit ops).
 * C equivalent: `*ptr = (*ptr & ~(1<<bit))` etc., but not atomic; these match
 * one instruction for CIA/custom registers. `dI` constraint: immediate bit 0–7.
 */
static inline void bclr(u_char *ptr, char bit) {
  asm("bclr %1,%0" :: "m" (*ptr), "dI" (bit));
}

static inline void bset(u_char *ptr, char bit) {
  asm("bset %1,%0" :: "m" (*ptr), "dI" (bit));
}

static inline void bchg(u_char *ptr, char bit) {
  asm("bchg %1,%0" :: "m" (*ptr), "dI" (bit));
}

/* Rotate word/long — maps to ror.w / ror.l; C equivalent: shifts with wrap. */
static inline short rorw(short a, short b) {
  short r;
  asm("ror.w %2,%0"
      : "=d" (r)
      : "0" (a), "dI" (b));
  return r;
}

static inline int rorl(int a, short b) {
  int r;
  asm("ror.l %2,%0"
      : "=d" (r)
      : "0" (a), "dI" (b));
  return r;
}

static inline short rolw(short a, short b) {
  short r;
  asm("rol.w %2,%0"
      : "=d" (r)
      : "0" (a), "dI" (b));
  return r;
}

static inline int roll(int a, short b) {
  int r;
  asm("rol.l %2,%0"
      : "=d" (r)
      : "0" (a), "dI" (b));
  return r;
}

/* Exchange two registers in one instruction (no temp). C needs a third variable. */
#define swapr(a, b) \
  asm ("exg %0,%1" : "+r" (a), "+r" (b))

/* Store byte/word/longword `d` under `p` with postincrement. */
/*
 * stbi/stwi/stli / stbd/stwd/stld — store with address postincrement or predecrement.
 * Technique: GCC `(%0)+` addressing mode; one instruction per store. C equivalent:
 * `*(p++) = d` but may not emit postincrement addressing for all targets.
 */
#define stbi(p, d)                \
  asm volatile("move.b %2,(%0)+"  \
      : "=a" (p)                  \
      : "0" (p), "dmi" ((u_char)d))

#define stwi(p, d)                \
  asm volatile("move.w %2,(%0)+"  \
      : "=a" (p)                  \
      : "0" (p), "dmi" ((u_short)d))

#define stli(p, d)                \
  asm volatile("move.l %2,(%0)+"  \
      : "=a" (p)                  \
      : "0" (p), "dmi" ((u_long)d))

/* Store byte/word/longword `d` under `p` with predecrement. */
#define stbd(p, d)                \
  asm volatile("move.b %2,-(%0)"  \
      : "=a" (p)                  \
      : "0" (p), "dmi" ((u_char)d))

#define stwd(p, d)                \
  asm volatile("move.w %2,-(%0)"  \
      : "=a" (p)                  \
      : "0" (p), "dmi" ((u_short)d))

#define stld(p, d)                \
  asm volatile("move.l %2,-(%0)"  \
      : "=a" (p)                  \
      : "0" (p), "dmi" ((u_long)d))

/* Current stack pointer; C has no standard way to read SP on M68k. */
static inline void *GetSP(void) {
  void *sp;
  asm("movel sp,%0" : "=r" (sp));
  return sp;
}

#endif
