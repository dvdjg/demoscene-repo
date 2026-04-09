/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <bitmap.h>

/*
 * InitSharedBitmap — fill BitmapT metadata and alias plane storage to donor->planes[0].
 * Used when one CHIP buffer holds multiple logical bitmaps (e.g. double buffer views).
 * Geometry (width/height/depth) may differ from donor; stride comes from new dimensions.
 */
void InitSharedBitmap(BitmapT *bitmap, u_short width, u_short height,
                      u_short depth, BitmapT *donor)
{
  u_short bytesPerRow = ((width + 15) & ~15) / 8;

  bitmap->width = width;
  bitmap->height = height;
  bitmap->depth = depth;
  bitmap->bytesPerRow = bytesPerRow;
  bitmap->bplSize = bytesPerRow * height;
  bitmap->flags = donor->flags;

  BitmapSetPointers(bitmap, donor->planes[0]);
}
