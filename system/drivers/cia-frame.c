/*
 * CIA-A TOD as a frame counter (with IPL guard).
 *
 * Purpose: when VSYNC is wired to CIA-A TOD clock, reads yield a stable frame
 * number for animation. SetIPL(SR_IM) prevents interrupts from interleaving TOD
 * byte reads (otherwise you could see torn values).
 */
#include <system/cia.h>
#include <system/task.h>
#include <system/cpu.h>

/* All TOD registers latch on a read of MSB event and remain latched
 * until after a read of LSB event. */

/* ReadFrameCounter — atomic 24-bit TOD read from CIA-A.
 * IPL is raised to mask interrupts so the hi/mid/low byte sequence cannot
 * be interleaved by ISR code that might also touch TOD state. */
u_int ReadFrameCounter(void) {
  u_int res = 0;
  u_short ipl = SetIPL(SR_IM);
  res |= ciaa->ciatodhi;
  res <<= 8;
  res |= ciaa->ciatodmid;
  res <<= 8;
  res |= ciaa->ciatodlow;
  (void)SetIPL(ipl);
  return res;
}

/* TOD is automatically stopped whenever a write to the register occurs. The
 * clock will not start again until after a write to the LSB event register. */

/* SetFrameCounter — preload CIA-A TOD with a frame index (24-bit). */
void SetFrameCounter(u_int frame) {
  u_short ipl = SetIPL(SR_IM);
  ciaa->ciatodhi = frame >> 16;
  ciaa->ciatodmid = frame >> 8;
  ciaa->ciatodlow = frame;
  (void)SetIPL(ipl);
}
