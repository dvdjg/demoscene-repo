/*
 * Amiga mouse via JOY0DAT quadrature counters.
 *
 * Purpose: hardware increments/decrements per mickey movement; we integrate into
 * screen coordinates and clamp to a window. Buttons come from POTGO/cia paths
 * depending on the specific wiring.
 *
 * Why JOY0DAT: OCS reads relative motion without CPU polling a serial stream —
 * the counters are updated by the hardware between reads.
 *
 * HRM (JOY0DAT, gameport): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <custom.h>
#include <debug.h>
#include <types.h>
#include <system/cia.h>
#include <system/event.h>
#include <system/interrupt.h>
#include <system/mouse.h>

typedef struct {
  MouseEventT event;

  /* Last sampled hardware counters from JOY0DAT (x low byte, y high byte). */
  char xctr, yctr;
  /* Current latched button bitset. */
  u_char button;

  /* Movement clamp window in screen coordinates. */
  short left;
  short right;
  short top;
  short bottom;
} MouseDataT;

static MouseDataT MouseData;

/* GetMouseMove — read relative deltas from JOY0DAT quadrature counters, clamp,
 * and update event absolute/relative fields. Returns true when movement occurred. */
static bool GetMouseMove(MouseDataT *mouse) {
  u_short joy0dat = custom->joy0dat;
  char xctr, xrel, yctr, yrel;

  xctr = joy0dat;
  xrel = xctr - mouse->xctr;

  if (xrel) {
    short x = mouse->event.x + xrel;

    if (x < mouse->left)
      x = mouse->left;
    if (x > mouse->right)
      x = mouse->right;

    mouse->event.x = x;
    mouse->event.xrel = xrel;
    mouse->xctr = xctr;
  }

  yctr = joy0dat >> 8;
  yrel = yctr - mouse->yctr;

  if (yrel) {
    short y = mouse->event.y + yrel;

    if (y < mouse->top)
      y = mouse->top;
    if (y > mouse->bottom)
      y = mouse->bottom;

    mouse->event.y = y;
    mouse->event.yrel = yrel;
    mouse->yctr = yctr;
  }

  return xrel || yrel;
}

static inline u_char ReadButtonState(void) {
  u_char state = 0;

	if (!(ciaa->ciapra & CIAF_GAMEPORT0))
    state |= LMB_PRESSED;
  if (!(custom->potinp & DATLY))
    state |= RMB_PRESSED;

  return state;
}

static bool GetMouseButton(MouseDataT *mouse) {
  u_char button = ReadButtonState();
  u_char change = (mouse->button ^ button) & (LMB_PRESSED | RMB_PRESSED);

  if (!change)
    return false;

  mouse->button = button;

  if (change & LMB_PRESSED)
    mouse->event.button |= (button & LMB_PRESSED) ? LMB_PRESSED : LMB_RELEASED;

  if (change & RMB_PRESSED)
    mouse->event.button |= (button & RMB_PRESSED) ? RMB_PRESSED : RMB_RELEASED;

  return true;
}

static void MouseIntHandler(void *data) {
  MouseDataT *mouse = (MouseDataT *)data;

  mouse->event.button = 0;

  /* Register mouse position change first. */
  if (GetMouseMove(mouse))
    PushEventISR((EventT *)&mouse->event);

  /* After that a change in mouse button state. */
  if (GetMouseButton(mouse))
    PushEventISR((EventT *)&mouse->event);
}

INTSERVER(MouseServer, -5, (IntFuncT)MouseIntHandler, (void *)&MouseData);

/* MouseInit — initialize bounds/counters and hook VBlank polling server.
 * VBlank cadence is enough for UI input and keeps implementation simple. */
void MouseInit(Box2D *win) {
  Log("[Mouse] Initialize driver!\n");

  /* Settings from MouseData structure. */
  MouseData.left = win->minX;
  MouseData.right = win->maxX;
  MouseData.top = win->minY;
  MouseData.bottom = win->maxY;
  MouseData.xctr = custom->joy0dat & 0xff;
  MouseData.yctr = custom->joy0dat >> 8;
  MouseData.button = ReadButtonState();
  MouseData.event.x = win->minX;
  MouseData.event.y = win->minY;
  MouseData.event.type = EV_MOUSE;

  AddIntServer(INTB_VERTB, MouseServer);
}

void MouseKill(void) {
  RemIntServer(INTB_VERTB, MouseServer);
}
