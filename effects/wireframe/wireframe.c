/*
 * Wireframe — 3D mesh edges with Blitter line mode + planar interleaved buffers.
 *
 * Pipeline:
 * - Load `pilka` as an `Object3D` (vertices, faces, edges, precomputed face normals).
 * - Each frame: world transform, back-face test, mark visible edges, perspective
 *   project vertices to screen space, then draw each visible edge with the **Blitter
 *   in line mode** (BC0F_LINE_OR, Bresenham setup in BLTxCON/BLTSIZE — same family as
 *   lib `BlitterLine`, but inlined here with `_WaitBlitter` + custom register ptr).
 * - **BLITHOG** gives the blitter bus priority so line drawing keeps up with rotation.
 *
 * Visibility:
 * - `UpdateFaceVisibilityFast`: dot(face normal, vector from camera to first vertex).
 *   Sign tells front/back; result is stored in the **high byte of the normal pointer
 *   slot** (`*(char *)fn`) — a compact flag channel in the mesh data.
 * - `UpdateEdgeVisibility`: for front faces, mark vertex and edge nodes so
 *   `TransformVertices` only projects what matters.
 * - Optional `SetFaceVisibility` (right mouse): clears all face flags so **no** edges
 *   draw — quick way to blank the object for debugging.
 *
 * TransformVertices uses a factored matrix multiply (MULVERTEX macros) related to
 * `(m00+y)*(m01+x)+…` — see inline algebra. NOTE: the code adjusts `M->z` in place
 * (comment in source); do not reuse this matrix elsewhere without resetting camera.
 *
 * Display: four bitplane pointers are **rotated** in the copper each frame (`active`
 * cycles 0..DEPTH) so motion blur / temporal accumulation spreads edges across planes
 * (classic “interleaved wire” look).
 *
 * HRM (Blitter line mode, minterms): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <strings.h>
#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "3d.h"
#include "fx.h"

#define WIDTH  256
#define HEIGHT 256
#define DEPTH 4

static Object3D *cube;
static CopListT *cp;
/* DEPTH+1 planes allocated; copper lists four for the wireframe stack. */
static BitmapT *screen;
/* Which plane receives new lines this frame (0..DEPTH, then wraps). */
static u_short active = 0;
static CopInsPairT *bplptr;

#include "data/wireframe-pal.c"
#include "data/pilka.c"

/* 256² @ 4 bitplanes + extra plane row; copper list only wires first four planes. */
static void Init(void) {
  cube = NewObject3D(&pilka);
  cube->translate.z = fx4i(-250);

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1, BM_CLEAR);

  SetupPlayfield(MODE_LORES, DEPTH, X(32), Y(0), WIDTH, HEIGHT);
  LoadColors(wireframe_colors, 0);

  cp = NewCopList(80);
  bplptr = CopSetupBitplanes(cp, screen, DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);
  EnableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DeleteCopList(cp);
  DeleteBitmap(screen);
  DeleteObject3D(cube);
}

/* Clear all face flag bytes — used with RMB to suppress drawing. */
static void SetFaceVisibility(Object3D *object) {
  void *_objdat = object->objdat;
  short *group = object->faceGroups;
  short f;

  do {
    while ((f = *group++))
      FACE(f)->flags = 0;
  } while (*group);
}

/*
 * Back-face culling via dot(normal, camera-to-vertex). Stores 0 / -1 in the byte
 * following the three normal components (see FACE layout in 3d.h / mesh tools).
 */
static void UpdateFaceVisibilityFast(Object3D *object) {
  short cx = object->camera.x;
  short cy = object->camera.y;
  short cz = object->camera.z;

  void *_objdat = object->objdat;
  short *group = object->faceGroups;
  short f;

  do {
    while ((f = *group++)) {
      short px, py, pz;
      int v;

      {
        short i = FACE(f)->indices[0].vertex;
        short *p = (short *)POINT(i);
        px = cx - *p++;
        py = cy - *p++;
        pz = cz - *p++;
      }

      {
        short *fn = FACE(f)->normal;
        int x = *fn++ * px;
        int y = *fn++ * py;
        int z = *fn++ * pz;
        v = x + y + z;

        *(char *)fn = v >= 0 ? 0 : -1;
      }
    }
  } while (*group);
}

/* Walk front faces; mark vertices/edges so TransformVertices projects drawn edges only. */
static void UpdateEdgeVisibility(Object3D *object) {
  register short s __ASM_REG_PARM("d2") = 1;

  void *_objdat = object->objdat;
  short *group = object->faceGroups;
  short f;

  do {
    while ((f = *group++)) {
      if (FACE(f)->flags >= 0) {
        register short *index __ASM_REG_PARM("a3") = (short *)(FACE(f)->indices);
        short vertices = FACE(f)->count - 3;
        short i;

        /* Face has at least (and usually) three vertices / edges. */
        i = *index++; NODE3D(i)->flags = s;
        i = *index++; EDGE(i)->flags = s;

        i = *index++; NODE3D(i)->flags = s;
        i = *index++; EDGE(i)->flags = s;

        do {
          i = *index++; NODE3D(i)->flags = s;
          i = *index++; EDGE(i)->flags = s;
        } while (--vertices != -1);
      }
    }
  } while (*group);
}

#define MULVERTEX1(D, E) {              \
  short t0 = (*v++) + y;                \
  short t1 = (*v++) + x;                \
  int t2 = (*v++) * z;                  \
  v++;                                  \
  D = ((t0 * t1 + t2 - xy) >> 4) + E;   \
}

#define MULVERTEX2(D) {                 \
  short t0 = (*v++) + y;                \
  short t1 = (*v++) + x;                \
  int t2 = (*v++) * z;                  \
  short t3 = (*v++);                    \
  D = normfx(t0 * t1 + t2 - xy) + t3;   \
}

/* Perspective divide writes screen x/y + inv-z into vertex slots for DrawObject. */
static void TransformVertices(Object3D *object) {
  Matrix3D *M = &object->objectToWorld;
  void *_objdat = object->objdat;
  short *group = object->vertexGroups;

  int m0 = (M->x - normfx(M->m00 * M->m01)) << 8;
  int m1 = (M->y - normfx(M->m10 * M->m11)) << 8;
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

/*
 * Draw visible edges into one bitplane `bplpt` using Blitter line mode.
 * `custom_` pinned to a6 for fast `_WaitBlitter` macros between line kicks.
 */
static void DrawObject(Object3D *object, void *bplpt,
                       CustomPtrT custom_ __ASM_REG_PARM("a6"))
{
  void *_objdat = object->objdat;
  short *group = object->edgeGroups;

  _WaitBlitter(custom_);
  custom_->bltafwm = -1;
  custom_->bltalwm = -1;
  custom_->bltadat = 0x8000;
  custom_->bltbdat = 0xffff; /* Line texture pattern. */
  custom_->bltcmod = WIDTH / 8;
  custom_->bltdmod = WIDTH / 8;

  do {
    short i;

    while ((i = *group++)) {
      short *edge = (short *)EDGE(i);

      short x0, y0, x1, y1;
      void *data;

      if (*edge == 0)
        continue;

      /* clear visibility */
      *edge++ = 0;

      {
        short i;

        i = *edge++;
        x0 = VERTEX(i)->x;
        y0 = VERTEX(i)->y;

        i = *edge++;
        x1 = VERTEX(i)->x;
        y1 = VERTEX(i)->y;
      }

      if (y0 > y1) {
        swapr(x0, x1);
        swapr(y0, y1);
      }

      {
        short dmax = x1 - x0;
        short dmin = y1 - y0;
        short derr;
        u_short bltcon1 = LINEMODE;

        if (dmax < 0)
          dmax = -dmax;

        if (dmax >= dmin) {
          if (x0 >= x1)
            bltcon1 |= (AUL | SUD);
          else
            bltcon1 |= SUD;
        } else {
          if (x0 >= x1)
            bltcon1 |= SUL;
          swapr(dmax, dmin);
        }

        {
          short y0_ = y0 << 5;
          short x0_ = x0 >> 3;
          short start = (y0_ + x0_) & ~1;
          data = bplpt + start;
        }

        dmin <<= 1;
        derr = dmin - dmax;
        if (derr < 0)
          bltcon1 |= SIGNFLAG;
        bltcon1 |= rorw(x0 & 15, 4);

        {
          u_short bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_OR;
          u_short bltamod = derr - dmax;
          u_short bltbmod = dmin;
          u_short bltsize = (dmax << 6) + 66;
          void *bltapt = (void *)(int)derr;

          _WaitBlitter(custom_);
          custom_->bltcon0 = bltcon0;
          custom_->bltcon1 = bltcon1;
          custom_->bltamod = bltamod;
          custom_->bltbmod = bltbmod;
          custom_->bltapt = bltapt;
          custom_->bltcpt = data;
          custom_->bltdpt = data;
          custom_->bltsize = bltsize;
        }
      }
    }
  } while (*group);
}

PROFILE(Transform);
PROFILE(Draw);

static void Render(void) {
  BlitterClear(screen, active);

  ProfilerStart(Transform);
  {
    cube->rotate.x = cube->rotate.y = cube->rotate.z = frameCount * 8;

    UpdateObjectTransformation(cube);
    if (RightMouseButton())
      SetFaceVisibility(cube);
    else
      UpdateFaceVisibilityFast(cube);
    UpdateEdgeVisibility(cube);
    TransformVertices(cube);
  }
  ProfilerStop(Transform);

  ProfilerStart(Draw);
  {
    DrawObject(cube, screen->planes[active], custom);
  }
  ProfilerStop(Draw);

  {
    /*
     * Rotate which physical plane buffer the copper points at per bitplane slot —
     * spreads new lines across planes each frame (persistence / multilayer wire).
     */
    void **planes = screen->planes;
    short n = DEPTH;
    short i = active;

    while (--n >= 0) {
      CopInsSet32(&bplptr[n], planes[i]);
      if (i == 0)
        i = DEPTH + 1;
      i--;
    }
  }

  TaskWaitVBlank();

  active++;
  if (active > DEPTH)
    active = 0;
}

EFFECT(Wireframe, NULL, NULL, Init, Kill, Render, NULL);
