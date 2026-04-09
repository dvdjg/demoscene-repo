/*
 * Custom chip register helpers (DMA, mouse buttons, AGA detect, mirrors).
 *
 * English tutorial: `custom` is the volatile view of the OCS/ECS chipset registers.
 * DMA and interrupt enables are write-only with SET/CLR bits — helpers here keep
 * the idiom safe. See HRM register map for addresses.
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#ifndef __CUSTOM_H__
#define __CUSTOM_H__

#include <custom_regdef.h>

typedef volatile struct Custom *const CustomPtrT;

extern struct Custom volatile _custom;

#define custom (&_custom)

/* Macros below take or'ed DMAF_* flags. */
static inline void EnableDMA(uint16_t x) { custom->dmacon = DMAF_SETCLR | x; }
static inline void DisableDMA(uint16_t x) { custom->dmacon = x; }

static inline bool RightMouseButton(void) {
  return !(custom->potinp & DATLY);
}

static inline short IsAGA(void) {
  return ((custom->vposr >> 8) & CHIPID_MASK) == CHIPID_ALICE_V3;
}

/* Following variables mirror corresponding register values, since the
 * registers are write-only. */
extern u_short __bplcon3;
extern u_short __fmode;

/* Change bits selected by `mask` to values in `val`. Returns value that may be
 * either written directly to register or to copper MOVE instruction. */

static inline u_short bplcon3(u_short val, u_short mask) {
  return __bplcon3 = (val & mask) | (__bplcon3 & ~mask);
}

static inline u_short fmode(u_short val, u_short mask) {
  return __fmode = (val & mask) | (__fmode & ~mask);
}

#endif /* !__CUSTOM_H__ */
