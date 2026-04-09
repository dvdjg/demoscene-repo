/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <gfx.h>

/*
 * ClipArea — clip sprite/blit rectangle (pos + area w/h) to axis-aligned Box2D.
 *
 * Returns false if no intersection. Otherwise shrinks area and moves pos to
 * visible portion (area->x/y are relative offsets adjusted when clipping left/top).
 * Pure CPU; used before programming blitter with smaller rectangles.
 */
bool ClipArea(const Box2D *space, Point2D *pos, Area2D *area) {
  short minX = space->minX;
  short minY = space->minY;
  short maxX = space->maxX;
  short maxY = space->maxY;
  short posX = pos->x;
  short posY = pos->y;

  if ((posX + area->w <= minX) || (posX > maxX))
    return false;
  if ((posY + area->h <= minY) || (posY > maxY))
    return false;

  if (posX < minX) {
    area->x += minX - posX;
    area->w -= minX - posX;
    pos->x = posX = minX;
  }

  if (posY < minY) {
    area->y += minY - posY;
    area->h -= minY - posY;
    pos->y = posY = minY;
  }

  if (posX + area->w > maxX)
    area->w = maxX - posX + 1;

  if (posY + area->h > maxY)
    area->h = maxY - posY + 1;

  return true;
}
