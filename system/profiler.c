/*
 * Simple raster-line profiler (measures work in scanlines).
 *
 * Purpose: ReadLineCounter() delta approximates how many display lines a code
 * block consumed — useful to compare effects or inner loops on real hardware
 * where cycle-exact profiling is overkill.
 *
 * Why lines, not microseconds: on OCS, the display timing is line-based; line
 * counts map intuitively to "how much of the frame budget" you spent.
 */
#include <debug.h>
#include <common.h>
#include <effect.h>
#include <system/cia.h>

void _ProfilerStart(ProfileT *prof) {
  prof->lines = ReadLineCounter();
  if (prof->reportFrame == 0)
    prof->reportFrame = frameCount;
}

void _ProfilerStop(ProfileT *prof) {
  u_short lines = ReadLineCounter() - prof->lines;

  if (lines > 32767)
    lines = -lines;

  if (lines < prof->min)
    prof->min = lines;
  if (lines > prof->max)
    prof->max = lines;

  prof->total += lines;
  prof->count++;

  /* Report every second! */
  if (prof->reportFrame + 50 < frameCount) {
    Log("%s took %d-%d-%d (min-avg-max) raster lines.\n",
        prof->name, prof->min, div16(prof->total, prof->count), prof->max);
    prof->reportFrame += 50;
  }
}
