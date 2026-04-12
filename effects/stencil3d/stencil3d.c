/*
 * Stencil3D — 3D mesh + stencil buffer + dual-playfield copper + blitter fill/lines.
 *
 * Display: MODE_DUALPF merges two playfields — PF1 = 3D object (screen[]), PF2 =
 * background bitmap. Copper points bplpt[0..4] at object planes + background planes
 * and per-scanline color[] from background_cols (gradient bars). Extra single-plane
 * buffer is aliased as screen[*]->planes[DEPTH]: used only as a scratch stencil
 * (not DMA-displayed), shared by both buffers to save CHIP RAM.
 *
 * DrawObject (back-to-front via SortFaces): for each visible face, blitter line mode
 * draws polygon edges into the stencil plane, XOR fill closes regions, then geometry
 * is merged into bitplane 2 with A_OR_B / A_AND_NOT_B from the stencil, and pattern
 * tiles (pattern_1 / pattern_2, three shades) are masked in with a minterm that
 * uses mask + pattern + destination.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "3d.h"
#include "fx.h"

#define WIDTH 256
#define HEIGHT 256
#define DEPTH 3

static __code Object3D *object;
static __code CopListT *cp;
/* Patched each frame: PF1 BPL0 + PF2 BPL0/1 + PF1 BPL2 (see MakeCopperList). */
static __code CopInsPairT *bplptr;
static __code BitmapT *screen[2];
/* Single-plane stencil/mask workspace; also screen[i]->planes[3]. */
static __code BitmapT *buffer;
static __code int active = 0;

#include "data/background-data.c"
#include "data/background-pal.c"
#include "data/pattern-1-1.c"
#include "data/pattern-1-2.c"
#include "data/pattern-1-3.c"
#include "data/pattern-2-1.c"
#include "data/pattern-2-2.c"
#include "data/pattern-2-3.c"

#include "data/kurak-head.c"
#include "data/cock-anim.c"

/*
 * Dual-PF copper: object on low bplpt slots, background on high; per-line colours
 * from background_cols_pixels (HAM-like bars without HAM mode).
 */
static CopListT *MakeCopperList(void) {
  CopListT *cp =
    NewCopList(100 + background_height * (background_cols_width + 3));

  /* Bitplane modulo for both playfields (flat fetch). */
  CopMove16(cp, bpl1mod, 0);
  CopMove16(cp, bpl2mod, 0);

  CopWait(cp, Y(-1), HP(0));

  bplptr = CopMove32(cp, bplpt[0], screen[1]->planes[0]);
  CopMove32(cp, bplpt[1], background.planes[0]);
  CopMove32(cp, bplpt[2], screen[1]->planes[1]);
  CopMove32(cp, bplpt[3], background.planes[1]);
  CopMove32(cp, bplpt[4], screen[1]->planes[2]);

  {
    u_short *data = background_cols_pixels;
    short i;

    for (i = 0; i < background_height; i++) {
      CopWaitSafe(cp, Y(i), HP(0));
      CopMove16(cp, color[0], *data++);
      CopMove16(cp, color[9], *data++);
      CopMove16(cp, color[10], *data++);
      CopMove16(cp, color[11], *data++);
    }
  }

  return CopListFinish(cp);
}

static void Load(void) {
  object = NewObject3D(&kurak);
  object->translate.z = fx4i(-256); /* base distance; Render adds cock path offset */
}

static void UnLoad(void) {
  DeleteObject3D(object);
}

static void Init(void) {
  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, 0);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, 0);
  buffer = NewBitmap(WIDTH, HEIGHT, 1, 0);

  /* Stencil plane: same CHIP buffer wired into both screen[] (not a 4th AllocPlanes). */
  screen[0]->planes[DEPTH] = buffer->planes[0];
  screen[1]->planes[DEPTH] = buffer->planes[0];

  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);
  BitmapClear(screen[0]);
  BitmapClear(screen[1]);
  BitmapClear(buffer);
  WaitBlitter();

  SetupDisplayWindow(MODE_LORES, X(32), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(32), WIDTH);
  /* 3 object planes + 2 background planes (see bplpt wiring in MakeCopperList). */
  SetupMode(MODE_DUALPF, DEPTH + background_depth);
  LoadColors(pattern_1_colors, 0);
  LoadColors(pattern_2_colors, 4);

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  BlitterStop();
  CopperStop();
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
  DeleteBitmap(buffer);
  DeleteCopList(cp);
}

#define MULVERTEX1(D, E)                                                       \
  {                                                                            \
    short t0 = (*v++) + y;                                                     \
    short t1 = (*v++) + x;                                                     \
    int t2 = (*v++) * z;                                                       \
    v++;                                                                       \
    D = ((t0 * t1 + t2 - xy) >> 4) + E;                                        \
  }

#define MULVERTEX2(D)                                                          \
  {                                                                            \
    short t0 = (*v++) + y;                                                     \
    short t1 = (*v++) + x;                                                     \
    int t2 = (*v++) * z;                                                       \
    short t3 = (*v++);                                                         \
    D = normfx(t0 * t1 + t2 - xy) + t3;                                        \
  }

/* Transforms only nodes with flags set (see mesh); writes 12.4 screen + zp. */
static void TransformVertices(Object3D *object) {
  Matrix3D *M = &object->objectToWorld;
  void *_objdat = object->objdat;
  short *group = object->vertexGroups;

  int m0 = (M->x << 8) - ((M->m00 * M->m01) >> 4);
  int m1 = (M->y << 8) - ((M->m10 * M->m11) >> 4);

  /* WARNING! This modifies camera matrix! */
  M->z -= normfx(M->m20 * M->m21);

  /*
   * A = m00 * m01
   * B = m10 * m11
   * C = m20 * m21
   * yx = y * x
   *
   * (m00 + y) * (m01 + x) + m02 * z - yx + (mx - A)
   * (m10 + y) * (m11 + x) + m12 * z - yx + (my - B)
   * (m20 + y) * (m21 + x) + m22 * z - yx + (mz - C)
   */

  do {
    short i;

    while ((i = *group++)) {
      if (NODE3D(i)->flags) {
        short *pt = (short *)NODE3D(i);
        short *v = (short *)M;
        short x, y, z, zp;
        int xy, xp, yp;

        /* clear flags */
        *pt++ = 0;

        x = *pt++;
        y = *pt++;
        z = *pt++;
        xy = x * y;

        MULVERTEX1(xp, m0);
        MULVERTEX1(yp, m1);
        MULVERTEX2(zp);

        *pt++ = div16(xp, zp) + WIDTH / 2;  /* div(xp * 256, zp) */
        *pt++ = div16(yp, zp) + HEIGHT / 2; /* div(yp * 256, zp) */
        *pt++ = zp;
      }
    }
  } while (*group);
}

/* [material variant][shade 0..2] → pattern bitplane arrays for texturing faces. */
static __code void **patterns[2][3] = {
  {
    (void **)pattern_2_3.planes,
    (void **)pattern_2_2.planes,
    (void **)pattern_2_1.planes,
  },
  {
    (void **)pattern_1_3.planes,
    (void **)pattern_1_2.planes,
    (void **)pattern_1_1.planes,
  },
};

/* Buckets face lighting flags (0..15) into pattern row index 0, 1, or 2. */
static __code short pattern_shade[16] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 2, 2, 2, 2
};

/*
 * Rasterize sorted faces: stencil plane = polygon from line draw + XOR fill;
 * bitplane 2 gets object pixels masked by stencil; pattern blits add detail.
 * planes[DEPTH] is the shared single-bitplane buffer.
 */
static void DrawObject(Object3D *object, void **planes,
                       CustomPtrT custom_ __ASM_REG_PARM("a6")) {
  register SortItemT *item __ASM_REG_PARM("a3") = object->visibleFace;
  void *_objdat = object->objdat;

  custom_->bltafwm = -1;
  custom_->bltalwm = -1;

  for (; item->index >= 0; item++) {
    short ii = item->index;

    {
      register short *index __ASM_REG_PARM("a4") = (short *)&FACE(ii)->count;
      short m = (*index++) - 1;

      do {
        /* Draw edge. */
        short bltcon0, bltcon1, bltsize, bltbmod, bltamod;
        int bltapt, bltcpt;
        short x0, y0, x1, y1;
        short dmin, dmax, derr;

        /* skip vertex index */
        index++;

        {
          short i = *index++; /* edge index */
          short *edge = &EDGE(i)->point[0];
          short *vertex;

          vertex = &VERTEX(*edge++)->x;
          x0 = *vertex++;
          y0 = *vertex++;

          vertex = &VERTEX(*edge++)->x;
          x1 = *vertex++;
          y1 = *vertex++;
        }

        /* Horizontal edges do not contribute to XOR fill closure. */
        if (y0 == y1)
          continue;

        if (y0 > y1) {
          swapr(x0, x1);
          swapr(y0, y1);
        }

        dmax = x1 - x0;
        if (dmax < 0)
          dmax = -dmax;

        dmin = y1 - y0;
        if (dmax >= dmin) {
          if (x0 >= x1)
            bltcon1 = AUL | SUD | LINEMODE | ONEDOT;
          else
            bltcon1 = SUD | LINEMODE | ONEDOT;
        } else {
          if (x0 >= x1)
            bltcon1 = SUL | LINEMODE | ONEDOT;
          else
            bltcon1 = LINEMODE | ONEDOT;
          swapr(dmax, dmin);
        }

        bltcpt = (int)planes[DEPTH] + (short)(((y0 << 5) + (x0 >> 3)) & ~1);

        bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_EOR;
        bltcon1 |= rorw(x0 & 15, 4);

        dmin <<= 1;
        derr = dmin - dmax;
        if (derr < 0)
          bltcon1 |= SIGNFLAG;

        bltamod = derr - dmax;
        bltbmod = dmin;
        bltsize = (dmax << 6) + 66;
        bltapt = derr;

        _WaitBlitter(custom_);

        custom_->bltbdat = 0xffff;
        custom_->bltadat = 0x8000;
        custom_->bltcon0 = bltcon0;
        custom_->bltcon1 = bltcon1;
        custom_->bltcpt = (void *)bltcpt;
        custom_->bltapt = (void *)bltapt;
        custom_->bltdpt = planes[DEPTH];
        custom_->bltcmod = WIDTH / 8;
        custom_->bltbmod = bltbmod;
        custom_->bltamod = bltamod;
        custom_->bltdmod = WIDTH / 8;
        custom_->bltsize = bltsize;
      } while (--m != -1);
    }

    {
      short bltstart, bltend;
      u_short bltmod, bltsize;

      {
        short minX, minY, maxX, maxY;

        register short *index __ASM_REG_PARM("a4") = (short *)&FACE(ii)->count;
        short m = (*index++) - 2;
        short *vertex;

        vertex = &VERTEX(*index++)->x;
        index++; /* skip edge index */

        minX = maxX = *vertex++;
        minY = maxY = *vertex++;

        /* Calculate area. */
        do {
          short x, y;

          vertex = &VERTEX(*index++)->x;
          index++; /* skip edge index */

          x = *vertex++;
          y = *vertex++;

          if (x < minX)
            minX = x;
          else if (x > maxX)
            maxX = x;
          if (y < minY)
            minY = y;
          else if (y > maxY)
            maxY = y;
        } while (--m != -1);

        /* Align to word boundary. */
        minX = (minX & ~15) >> 3;
        /* to avoid case where a line is on right edge */
        maxX = ((maxX + 16) & ~15) >> 3;

        {
          short w = maxX - minX;
          short h = maxY - minY + 1;

          bltstart = minX + minY * (WIDTH / 8);
          bltend = maxX + maxY * (WIDTH / 8) - 2;
          bltsize = (h << 6) | (w >> 1);
          bltmod = (WIDTH / 8) - w;
        }
      }

      /* XOR fill enclosed region in stencil plane (reverse blit from bottom-right). */
      {
        void *dst = planes[DEPTH] + bltend;

        _WaitBlitter(custom_);

        custom_->bltcon0 = (SRCA | DEST) | A_TO_D;
        custom_->bltcon1 = BLITREVERSE | FILL_XOR;
        custom_->bltapt = dst;
        custom_->bltdpt = dst;
        custom_->bltamod = bltmod;
        custom_->bltbmod = bltmod;
        custom_->bltcmod = bltmod;
        custom_->bltdmod = bltmod;
        custom_->bltsize = bltsize;
      }

      /* Merge stencil into object colour plane: add or subtract from plane 2. */
      {
        void *dst = planes[2] + bltstart;
        void *mask = planes[DEPTH] + bltstart;

        u_short bltcon0;

        if (FACE(ii)->material & 1) {
          bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
        } else {
          bltcon0 = (SRCA | SRCB | DEST) | A_AND_NOT_B;
        }

        _WaitBlitter(custom_);

        custom_->bltcon0 = bltcon0;
        custom_->bltcon1 = 0;
        custom_->bltapt = dst;
        custom_->bltbpt = mask;
        custom_->bltdpt = dst;
        custom_->bltsize = bltsize;
      }

#if 0
      Assert(FACE(ii)->flags <= 16);
      Assert(FACE(ii)->flags >= 0);
#endif

      {
        void **srcbpl;
        void **dstbpl = planes;
        void *mask = planes[DEPTH] + bltstart;
        short shade = pattern_shade[(short)FACE(ii)->flags];
        short pat = FACE(ii)->material & 1;
        short i;

        srcbpl = patterns[pat][shade];

        /* A=mask B=pattern C=dst D=dst — classic masked pattern OR into PF1 planes. */
        for (i = 0; i < pattern_1_1_depth; i++) {
          void *src = *srcbpl++;
          void *dst = *dstbpl++ + bltstart;
          u_short bltcon0 =
            (SRCA | SRCB | SRCC | DEST) | (ABC | ABNC | NABC | NANBC);

          _WaitBlitter(custom_);

          custom_->bltcon0 = bltcon0;
          custom_->bltcon1 = 0;
          custom_->bltbmod = bltmod - WIDTH / 16;
          custom_->bltapt = mask;
          custom_->bltbpt = src;
          custom_->bltcpt = dst;
          custom_->bltdpt = dst;
          custom_->bltsize = bltsize;
        }
      }

      /* Zero stencil rectangle for next face. */
      {
        void *data = planes[DEPTH] + bltstart;

        _WaitBlitter(custom_);

        custom_->bltcon0 = (DEST | A_TO_D);
        custom_->bltadat = 0;
        custom_->bltdpt = data;
        custom_->bltsize = bltsize;
      }
    }
  }
}

/* One blit clears all planes starting at plane[0] (height × depth as vertical size). */
static void BitmapClearFast(BitmapT *dst) {
  u_short height = (short)dst->height * (short)dst->depth;
  u_short bltsize = (height << 6) | (dst->bytesPerRow >> 1);
  void *bltpt = dst->planes[0];

  WaitBlitter();

  custom->bltcon0 = DEST | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltadat = 0;
  custom->bltdmod = 0;
  custom->bltdpt = bltpt;
  custom->bltsize = bltsize;
}

PROFILE(Transform);
PROFILE(Draw);

static void Render(void) {
  BitmapClearFast(screen[active]);

  /* Path keyframe drives transform on top of kurak mesh base. */
  {
    short *frame = cock_anim[frameCount % cock_anim_frames];
    object->translate.x = *frame++;
    object->translate.y = *frame++;
    object->translate.z = *frame++;
    object->translate.z += fx4i(-256);
    object->rotate.x = *frame++;
    object->rotate.y = *frame++;
    object->rotate.z = *frame++;
    object->scale.x = *frame++;
    object->scale.y = *frame++;
    object->scale.z = *frame++;
  }

  ProfilerStart(Transform);
  {
    UpdateObjectTransformation(object);
    UpdateFaceVisibility(object);
    UpdateVertexVisibility(object);
    TransformVertices(object);
    SortFaces(object);
  }
  ProfilerStop(Transform); // Average: 163

  ProfilerStart(Draw);
  DrawObject(object, screen[active]->planes, custom);
  ProfilerStop(Draw); // Average: 671

  /* Point copper at the buffer we just drew (PF1); PF2 stays background. */
  CopInsSet32(&bplptr[0], screen[active]->planes[0]);
  CopInsSet32(&bplptr[2], screen[active]->planes[1]);
  CopInsSet32(&bplptr[4], screen[active]->planes[2]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Stencil3D, Load, UnLoad, Init, Kill, Render, NULL);
