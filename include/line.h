/*
 * CPU-drawn lines and edges on bitplanes (Bresenham-style in asm).
 *
 * CpuLineSetup/CpuEdgeSetup — bind destination bitmap + plane for following calls.
 * CpuLine/CpuEdge — coordinates in d0–d3 (GCC asm("d0")…) to match M68k register
 * convention in lib/libgfx/CpuLine.c; avoids spilling args across calls in inner loops.
 * C equivalent: same coordinates as plain ints; no register forcing — compiler may use stack.
 * For filled areas or many lines, blitter line mode is often faster (see blitter.h).
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#ifndef __LINE_H__
#define __LINE_H__

#include "gfx.h"

void CpuLineSetup(const BitmapT *bitmap, u_short plane);
void CpuLine(short x1 asm("d0"), short y1 asm("d1"), short x2 asm("d2"), short y2 asm("d3"));
void CpuEdgeSetup(const BitmapT *bitmap, u_short plane);
void CpuEdge(short x1 asm("d0"), short y1 asm("d1"), short x2 asm("d2"), short y2 asm("d3"));

#endif
