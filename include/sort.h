/*
 * Header: sort.h — see includes and call sites in the repo.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __SORT_H__
#define __SORT_H__

#include "common.h"

typedef struct {
  short key, index;
} SortItemT;

void SortItemArray(SortItemT *table, short size);

#endif
