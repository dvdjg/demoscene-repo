/*
 * Tiles16 — MagicLand-style 16×16 tile map, 5 bitplanes, copper fine scroll + dirty tiles.
 *
 * **Map encoding (`Load`):** Each cell is shifted left by 2 and OR’d with `3` so the low
 * two bits are **dirty flags** (one per hidden screen buffer). `TriggerRefresh` ORs `3`
 * on visible cells to request a redraw. At runtime `UpdateTiles` uses `current = active + 1`
 * (values `1` or `2`) so `tile & current` selects work queued for the buffer about to be
 * shown; after the blit, `map[-1] ^= current` clears those bits (no full-map refresh).
 *
 * **Interleaved blits (`BM_INTERLEAVED`):** Tile graphics are contiguous in CHIP; the
 * destination is interleaved (`BitmapSetPointers` in libgfx). One A→D copy with
 * `bltsize = ((TILEH * DEPTH) << 6) | 1` stamps all five planes in a single blitter pass
 * (height counts plane “slices” — fewer separate `bltsize` writes than planar mode).
 * `bltdmod` skips from the end of one 16×16 column to the start of the next within the
 * wide bitmap; the vertical step `BPLMOD + 15 * WIDTH * DEPTH / 8` advances to the next
 * row of tiles (see HRM blitter modulo for interleaved playfields).
 *
 * **`WAITBLT`:** Busy-waits on DMACONR bit 6 (BBUSY). Required before touching blitter
 * registers or starting the next tile — the 68000 does not stall on blitter completion.
 *
 * **Display:** `SetupDisplayWindow` is one tile smaller than the bitmap so the map can
 * scroll; `CopInsSet32` adds `x << 1` to BPLxPT (word-aligned coarse scroll) and
 * `bplcon1` supplies 4+4 pixel nibble fine scroll. Double copper + double screen: patch
 * active list, `CopListRun`, VBlank, `active ^= 1`.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <gfx.h>
#include <system/memory.h>

#define TILEW 16
#define TILEH 16

#define WIDTH (320 + TILEW)
#define HEIGHT (256 + TILEH)
#define DEPTH 5
#define BPLMOD (WIDTH * (DEPTH - 1) / 8)

#define VTILES (HEIGHT / TILEH)
#define HTILES (WIDTH / TILEW)
#define TILESIZE (TILEW * TILEH * DEPTH / 8)

static CopInsPairT *bplptr[2];
static BitmapT *screen[2];
static CopInsT *bplcon1[2];
static CopListT *cp[2];
static void **tileptrs;
static short active;

#include "data/MagicLand-map.c"
#include "data/MagicLand-tiles.c"
#define tilecount MagicLand_ntiles
#define tilemap_width MagicLand_map_width
#define tilemap_height MagicLand_map_height
#define tilemap ((short *)MagicLand_map)

static void Load(void) {
  tileptrs = MemAlloc(sizeof(void *) * tilecount, MEMF_PUBLIC);
  {
    short n = tilecount;
    void *base = tiles.planes[0];
    void **ptrs = tileptrs;
    while (--n >= 0) {
      *ptrs++ = base;
      base += TILESIZE;
    }
  }

  {
    int i;
    for (i = 0; i < tilemap_width * tilemap_height; i++) {
      /* Pack tile index in high bits; mark both dirty bits so first frames fill the map. */
      tilemap[i] <<= 2;
      tilemap[i] |= 3;
    }
  }
}

static CopListT *MakeCopperList(int i) {
  CopListT *cp = NewCopList(100);
  bplptr[i] = CopSetupBitplanes(cp, screen[i], DEPTH);
  bplcon1[i] = CopMove16(cp, bplcon1, 0);
  return CopListFinish(cp);
}

static void Init(void) {
  /* Vertical slack in the bitmap so coarse horizontal scroll does not run out of map. */
  short extra = tilemap_width * TILEW / WIDTH;

  Log("Allocate %d extra lines!\n", extra);

  /* Interleaved: one blit per tile stamps all planes (see file header). */
  screen[0] = NewBitmap(WIDTH, HEIGHT + extra, DEPTH,
                        BM_CLEAR | BM_INTERLEAVED);
  screen[1] = NewBitmap(WIDTH, HEIGHT + extra, DEPTH,
                        BM_CLEAR | BM_INTERLEAVED);

  SetupMode(MODE_LORES, DEPTH);
  SetupDisplayWindow(MODE_LORES, X(0), Y(0), WIDTH - 16, HEIGHT - 16);
  SetupBitplaneFetch(MODE_LORES, X(-16), WIDTH);
  LoadColors(tiles_colors, 0);

  cp[0] = MakeCopperList(0);
  cp[1] = MakeCopperList(1);

  CopListActivate(cp[1]);

  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);

  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);

  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

void TriggerRefresh(short x, short y, short w __unused, short h __unused)
{
  short *map = tilemap;
  int tilemod = tilemap_width - HTILES;

  map += x + (short)y * (short)tilemap_width;

  {
    short j = VTILES - 1;
    do {
      short i = HTILES - 1;

      do {
        *map++ |= 3;
      } while (--i >= 0);

      map += tilemod;
    } while (--j >= 0);
  }
}

/*
 * Poll Blitter Busy in DMACONR until clear (HRM: BBUSY bit 6). `custom` points at
 * $DFF000; DMACONR is at offset +2 from the base of `struct Custom`.
 */
#define WAITBLT()                               \
  asm("1: btst #6,%0@(2)\n" /* dmaconr */       \
      "   bnes 1b"                              \
      :: "a" (custom));

/*
 * Stamp dirty 16×16 tiles for the visible HTILES×VTILES window. `custom` in a6 keeps
 * the hot blitter register block address in a fixed address register (shorter encodings
 * than reloading from global each time on some assemblers).
 */
static void UpdateTiles(BitmapT *screen, short x, short y,
                        CustomPtrT custom_ asm("a6"))
{
  short *map = tilemap;
  void *ptrs = tileptrs;
  void *dst = screen->planes[0] + (x << 1);
  short size = ((TILEH * DEPTH) << 6) | 1;
  int tilemod = tilemap_width - HTILES;
  short current = active + 1;

  map += x + (short)y * (short)tilemap_width;

  WAITBLT();

  custom_->bltafwm = -1;
  custom_->bltalwm = -1;
  custom_->bltamod = 0;
  custom_->bltdmod = (WIDTH - TILEW) / 8;
  custom_->bltcon0 = (SRCA | DEST | A_TO_D);
  custom_->bltcon1 = 0;

  {
    short j = VTILES - 1;

    do {
      short i = HTILES - 1;

      do {
        short tile = *map++;

        /* `tile & ~3` is the tile index; low bits are dirty flags only. */
        if (tile & current) {
          void *src = *(void **)(ptrs + (tile & ~3));

          WAITBLT();
          custom_->bltapt = src;
          custom_->bltdpt = dst;
          custom_->bltsize = size;

          map[-1] ^= current;
        }

        dst += 2;
      } while (--i >= 0);

      map += tilemod;
      dst += BPLMOD + 15 * WIDTH * DEPTH / 8;
    } while (--j >= 0);
  }
}

PROFILE(Tiles16);

static void Render(void) {
  ProfilerStart(Tiles16);
  {
    short t = frameCount;
    short tile = t >> 4;
    short pixel = 15 - (t & 15);

    short x = tile % (tilemap_width - HTILES);
    short y = 35;

    UpdateTiles(screen[active], x, y, custom);

    {
      short i;
      CopInsPairT *_bplptr = bplptr[active];
      void **_planes = screen[active]->planes;
      int offset = x << 1;

      for (i = 0; i < DEPTH; i++)
        CopInsSet32(&_bplptr[i], _planes[i] + offset);
    }
    CopInsSet16(bplcon1[active], pixel | (pixel << 4));
    CopListRun(cp[active]);
  }
  ProfilerStop(Tiles16);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Tiles16, Load, NULL, Init, Kill, Render, NULL);
