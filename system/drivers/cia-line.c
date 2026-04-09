/*
 * CIA-B Time-of-Day (TOD) as a raster line counter.
 *
 * Purpose: CIAB TOD can be clocked from the vertical sync (VSYNC) to count
 * display lines — useful for profiling and sync. See original comments for
 * latch ordering rules (MSB read latches LSB).
 */
#include <system/cia.h>

/* All TOD registers latch on a read of MSB event and remain latched until
 * after a read of LSB event. */
/* ReadLineCounter — read latched 24-bit TOD value used as raster/line counter. */
u_int ReadLineCounter(void) {
  int res = 0;
  res |= ciab->ciatodhi;
  res <<= 8;
  res |= ciab->ciatodmid;
  res <<= 8;
  res |= ciab->ciatodlow;
  return res;
}

/* TOD is automatically stopped whenever a write to the register occurs. The
 * clock will not start again until after a write to the LSB event register. */
/* WriteLineCounter — preload TOD counter (write order high->mid->low). */
void WriteLineCounter(u_int line) {
  ciab->ciatodhi = line >> 16;
  ciab->ciatodmid = line >> 8;
  ciab->ciatodlow = line;
}
