/*
 * UVMapRGB — HAM chunky UV texture + JIT inner loop + IRQ-driven C2P (uvmap variant).
 *
 * Compared to uvmap.c: 12-bit RGB textures. PixmapScramble packs RGB444 into
 * planar HAM-friendly bit patterns via redtab/greentab/bluetab nibble shuffles.
 * Per-pixel work is a JIT stream of 68000 opcodes from MakeUVMapRenderCode
 * (move.w from offset table, move.w from texture, rts). UVMapRenderCrop / Restore
 * patch bra.w skips so only an 80×64 window runs — a viewport into the larger
 * uvmap without executing dead pixels.
 *
 * Display: ChunkyToPlanar runs in Blitter IRQ (multi-pass shuffle); case 12
 * reorders BPL pointers for HAM base planes. MakeCopperList does line quadrupling
 * (BPLxMOD) and alternates BPLCON1 for vertical scale / shimmer.
 *
 * MODE_HAM; on AGA, two extra bitplanes plus fixed bpldat / plane fill — see Init.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <system/interrupt.h>
#include <system/memory.h>

/* Logical render window (cropped from full uvmap); display is 4× taller via copper. */
#define WIDTH 80
#define HEIGHT 64
#define DEPTH 4

static BitmapT *screen[2];
static u_short active = 0;
/* Scrambled RGB444→planar-friendly words; second 32K copy enables cheap scroll (see memcpy). */
static u_short *texture;
/* CHIP chunky buffers (double-buffered); fed by JIT, consumed by C2P. */
static u_short *chunky[2];
static CopListT *cp;
static CopInsPairT *bplptr;

#include "data/texture-rgb.c"
#include "data/uvmap-rgb.c"

#define UVMapRenderSize (uvmap_width * uvmap_height * 8 + 2)
typedef void (*UVMapRenderT)(u_short *chunky asm("a0"),
                             u_short *offsets asm("a1"),
                             u_short *texture asm("a2"));
/* MEMF_PUBLIC buffer holding generated 68000 (not a normal C function pointer target). */
static UVMapRenderT UVMapRender;
/* Per-row byte pairs into texture / offset table — animated in ControlOffsets. */
static u_short UVMapOffsets[image_height];

#define UVMapRenderBackupSize (HEIGHT * 4)
/* Words saved while UVMapRenderCrop overwrites inner rows with bra.w. */
static u_short *UVMapRenderBackup;

/* Emit one texture sample per uvmap texel: load u offset, index texture, write chunky. */
static void MakeUVMapRenderCode(void) {
  u_short *code = (void *)UVMapRender;
  u_char *data = (void *)uvmap;
  short n = uvmap_width * uvmap_height;

  while (--n >= 0) {
    /* both are already multiplied by 2 */
    u_short u = *data++;
    u_short v = *data++;

    /* 3029 00uu | move.w $00uu(a1),d0 */
    *code++ = 0x3029;
 
    /* We're going to index 16-bit values. */
    *code++ = u;

    /* 30f2 00vv | move.w $vv(a2,d0.w),(a0)+ */
    *code++ = 0x30f2;

    /* Same as with `u`, though for 64..127 the value will be sign extended
     * hence will wrap around. */
    *code++ = v;
  }

  *code++ = 0x4e75; /* rts */
}

/* Replace inner rows with bra.w skips; save overwritten words to `backup` for restore. */
static void* UVMapRenderCrop(void *code, void *backup, short x, short y) {
  u_short *data = code;
  u_short *save = backup;
  short i;

  /* move to selected location */
  data += (y * uvmap_width + x) * 4;
  
  /* save the pointer to rendering code */
  code = data;

  /* move to the first instruction that should be skipped */
  data += WIDTH * 4;

  for (i = 0; i < HEIGHT - 1; i++) {
    /* 6000 xxxx | bra.w xxxx */
    *save++ = *data;
    *data++ = 0x6000;
    *save++ = *data;
    *data++ = (uvmap_width - WIDTH) * 8 - 2;
    /* move to the next row */
    data += (uvmap_width - 1) * 4 + 2;
  }

  *save++ = *data;
  *data++ = 0x4e75; /* rts */
  *save++ = *data;
  *data++ = 0x4e75; /* rts */

  return code;
}

static void UVMapRenderRestore(void *code, void *backup) {
  u_short *data = code;
  u_short *save = backup;
  short i;

  /* move to the first instruction that should be restored */
  data += WIDTH * 4;

  for (i = 0; i < HEIGHT; i++) {
    *data++ = *save++;
    *data++ = *save++;
    /* move to the next row */
    data += (uvmap_width - 1) * 4 + 2;
  }
}

static u_short bluetab[16] = {
  0x0000, 0x0003, 0x0030, 0x0033, 0x0300, 0x0303, 0x0330, 0x0333,
  0x3000, 0x3003, 0x3030, 0x3033, 0x3300, 0x3303, 0x3330, 0x3333,
};

static u_short greentab[16] = {
  0x0000, 0x0004, 0x0040, 0x0044, 0x0400, 0x0404, 0x0440, 0x0444,
  0x4000, 0x4004, 0x4040, 0x4044, 0x4400, 0x4404, 0x4440, 0x4444,
};

static u_short redtab[16] = {
  0x0000, 0x0008, 0x0080, 0x0088, 0x0800, 0x0808, 0x0880, 0x0888,
  0x8000, 0x8008, 0x8080, 0x8088, 0x8800, 0x8808, 0x8880, 0x8888,
};

/* Pack 12-bit RGB pixmap into 16-bit words where each nibble lane matches HAM plane layout. */
static void PixmapScramble(const PixmapT *image, u_short *texture) {
  u_char *in = image->pixels;
  u_short *out = texture;
  short n = 128 * 128;

  while (--n >= 0) {
    short ri = *in++;
    short gi = *in++;
    short bi = gi;

    /* [-- -- -- -- 11 10  9  8  7  6  5  4  3  2  1  0] */
    /* [-- -- -- -- r0 r1 r2 r3 g0 g1 g2 g3 b0 b1 b2 b3] */
    /* [11  7  3  3 10  6  2  2  9  5  1  1  8  4  0  0] */
    /* [r0 g0 b0 b0 r1 g1 b1 b1 r2 g2 b2 b2 r3 g3 b3 b3] */

    gi >>= 4;
    bi &= 15;

    *out++ = getword(redtab, ri) + getword(greentab, gi) + getword(bluetab, bi);
  }

  /* Extra half for cheap texture motion. */
  memcpy((void *)texture + 32768, texture, 32768);
}

static void Load(void) {
  texture = MemAlloc(65536, MEMF_PUBLIC);
  PixmapScramble(&image, texture);
}

static void UnLoad(void) {
  MemFree(texture);
}

/*
 * C2P IRQ state. phase 256 until Render sets 0 (avoids running switch before first frame).
 * chunky: temporary work + source for blits; bpl: destination screen planes.
 */
static struct {
  short phase;
  void **bpl;
  void *chunky;
} c2p = { 256, NULL, NULL };

#define BPLSIZE ((WIDTH * 4) * HEIGHT / 8) /* 2560 bytes */
#define BLTSIZE ((WIDTH * 4) * HEIGHT / 2) /* 10240 bytes */

/*
 * Blitter IRQ: one case per interrupt. Even/odd phase pairs often repeat bltsize with
 * the same minterms (second pass / reverse direction) — see cases 0–1, 2–3, etc.
 * Case 12 rewires copper BPL pointers for HAM plane order after shuffle completes.
 */
static void ChunkyToPlanar(void) {
  void *src = c2p.chunky;
  void *dst = c2p.chunky + BLTSIZE;
  void **bpl = c2p.bpl;

  switch (c2p.phase) {
    case 0:
      /* Initialize chunky to planar. */
      custom->bltamod = 4;
      custom->bltbmod = 4;
      custom->bltdmod = 4;
      custom->bltcdat = 0x00FF;
      custom->bltafwm = -1;
      custom->bltalwm = -1;

      /* Swap 8x4, pass 1. */
      custom->bltapt = src + 4;
      custom->bltbpt = src;
      custom->bltdpt = dst;

      /* ((a >> 8) & 0x00FF) | (b & ~0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ANBC | ABNC | NABNC) | ASHIFT(8);
      custom->bltcon1 = 0;
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 1:
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 2:
      /* Swap 8x4, pass 2. */
      custom->bltapt = src + BLTSIZE - 6;
      custom->bltbpt = src + BLTSIZE - 2;
      custom->bltdpt = dst + BLTSIZE - 2;

      /* ((a << 8) & ~0x00FF) | (b & 0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABNC | ANBNC | ABC | NABC) | ASHIFT(8);
      custom->bltcon1 = BLITREVERSE;
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 3:
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 4:
      custom->bltamod = 6;
      custom->bltbmod = 6;
      custom->bltdmod = 0;
      custom->bltcdat = 0x0F0F;

      custom->bltapt = dst + 2;
      custom->bltbpt = dst;
      custom->bltdpt = bpl[0];

      /* ((a >> 4) & 0x0F0F) | (b & ~0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ANBC | ABNC | NABNC) | ASHIFT(4);
      custom->bltcon1 = 0;
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 5:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 6:
      custom->bltapt = dst + 6;
      custom->bltbpt = dst + 4;
      custom->bltdpt = bpl[2];
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 7:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 8:
      custom->bltapt = dst + BLTSIZE - 8;
      custom->bltbpt = dst + BLTSIZE - 6;
      custom->bltdpt = bpl[1] + BPLSIZE - 2;

      /* ((a << 8) & ~0x0F0F) | (b & 0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABNC | ANBNC | ABC | NABC) | ASHIFT(4);
      custom->bltcon1 = BLITREVERSE;
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 9:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 10:
      custom->bltapt = dst + BLTSIZE - 4;
      custom->bltbpt = dst + BLTSIZE - 2;
      custom->bltdpt = bpl[3] + BPLSIZE - 2;
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 11:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 12:
      /* HAM: expose shuffled planes in order expected by playfield (see MODE_HAM setup). */
      CopInsSet32(&bplptr[0], bpl[3]);
      CopInsSet32(&bplptr[1], bpl[2]);
      CopInsSet32(&bplptr[2], bpl[1]);
      CopInsSet32(&bplptr[3], bpl[0]);
      break;

    default:
      break;
  }

  c2p.phase++;

  ClearIRQ(INTF_BLIT);
}

/* HAM playfield: tall copper list with per-line modulo to repeat each logical line 4×. */
static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(1200);
  short i;

  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH + (IsAGA() ? 2 : 0));
  CopLoadColor(cp, 0, 15, 0);
  for (i = 0; i < HEIGHT * 4; i++) {
    CopWaitSafe(cp, Y(i), HP(0));
    /* Line quadrupling: negative modulo holds fetch until fourth subline advances. */
    CopMove16(cp, bpl1mod, ((i & 3) != 3) ? -40 : 0);
    CopMove16(cp, bpl2mod, ((i & 3) != 3) ? -40 : 0);
    /* Alternating shift by one for bitplane data. */
    CopMove16(cp, bplcon1, (i & 1) ? 0x0022 : 0x0000);
  }
  return CopListFinish(cp);
}

static void Init(void) {
  screen[0] = NewBitmap(WIDTH * 4, HEIGHT, DEPTH + (IsAGA() ? 2 : 0), 0);
  screen[1] = NewBitmap(WIDTH * 4, HEIGHT, DEPTH + (IsAGA() ? 2 : 0), 0);

  chunky[0] = MemAlloc((WIDTH * 4) * HEIGHT, MEMF_CHIP);
  chunky[1] = MemAlloc((WIDTH * 4) * HEIGHT, MEMF_CHIP);

  UVMapRender = MemAlloc(UVMapRenderSize, MEMF_PUBLIC);
  MakeUVMapRenderCode();

  UVMapRenderBackup = MemAlloc(UVMapRenderBackupSize, MEMF_PUBLIC);

  EnableDMA(DMAF_BLITTER);

  BitmapClear(screen[0]);
  BitmapClear(screen[1]);

  SetupPlayfield(MODE_HAM, IsAGA() ? 6 : 7, X(0), Y(0), WIDTH * 4 + 2, HEIGHT * 4);

  if (IsAGA()) {
    /* Extra planes: static nibble patterns for HAM hold / modify behaviour on AGA. */
    memset(screen[0]->planes[4], 0x77, WIDTH * 4 * HEIGHT / 8);
    memset(screen[1]->planes[4], 0x77, WIDTH * 4 * HEIGHT / 8);
    memset(screen[0]->planes[5], 0xcc, WIDTH * 4 * HEIGHT / 8);
    memset(screen[1]->planes[5], 0xcc, WIDTH * 4 * HEIGHT / 8);
  } else {
    /* OCS/ECS: same rgbb idea via bpldat during missing initial fetch. */
    custom->bpldat[4] = 0x7777; /* rgbb: 0111 */
    custom->bpldat[5] = 0xcccc; /* rgbb: 1100 */
  }

  cp = MakeCopperList();
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);

  SetIntVector(INTB_BLIT, (IntHandlerT)ChunkyToPlanar, NULL);
  EnableINT(INTF_BLIT);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER);

  DisableINT(INTF_BLIT);
  ResetIntVector(INTB_BLIT);

  DeleteCopList(cp);

  MemFree(UVMapRenderBackup);
  MemFree(UVMapRender);

  MemFree(chunky[0]);
  MemFree(chunky[1]);

  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(UVMapRGB);

/* Wobble the UV offset table rows so the texture "breathes" (sine on row/column). */
static void ControlOffsets(void) {
  u_char *off = (u_char *)UVMapOffsets;
  short i;

  for (i = 0; i < image_height; i++) {
    short j;

    j = SIN(frameCount * 128 - i * 64) >> 10;
    // j = -4;
    *off++ = (i + j + 4) & 127;

    j = SIN(frameCount * 128 + i * 128) >> 10;
    // j = -4;
    *off++ = (j + 4) * 2;
  }
}

static void Render(void) {
  ProfilerStart(UVMapRGB);
  ControlOffsets();
  {
    /* Moving 80×64 window over full uvmap; texture pointer scrolls in 64K table. */
    short x = normfx(SIN(frameCount * 8) * WIDTH / 2) + WIDTH / 2;
    short y = normfx(COS(frameCount * 8) * HEIGHT / 2) + HEIGHT / 2;
    UVMapRenderT code = UVMapRenderCrop(UVMapRender, UVMapRenderBackup, x, y);
    (*code)(chunky[active], UVMapOffsets, &texture[(frameCount * 2) & 16383]);
    UVMapRenderRestore(code, UVMapRenderBackup);
  }
  ProfilerStop(UVMapRGB);

  /* Kick C2P chain for this chunky[] buffer; IRQs advance c2p.phase until idle. */
  c2p.phase = 0;
  c2p.chunky = chunky[active];
  c2p.bpl = screen[active]->planes;
  ChunkyToPlanar();
  active ^= 1;
}

EFFECT(UVMapRGB, Load, UnLoad, Init, Kill, Render, NULL);
