/*
 * Ring buffer of EventT between ISR and main/task context.
 *
 * Purpose: keyboard/mouse/serial push events; consumers PopEvent. PushEventISR
 * raises IPL briefly to keep the queue consistent without nested interrupt races.
 */
#include <debug.h>
#include <system/cpu.h>
#include <system/event.h>
#include <system/task.h>

#define QUEUELEN 32

/* queue/head/tail/used — fixed-size power-of-two ring buffer state.
 * head: read index, tail: write index, used: number of queued events. */
static EventT queue[QUEUELEN];
static u_short head, tail, used;

/* _PushEvent — internal enqueue primitive; caller must ensure atomicity.
 * Drops events when full to keep ISR non-blocking and deterministic. */
static void _PushEvent(EventT *event) {
  if (used < QUEUELEN) {
    queue[tail] = *event;
    tail = (tail + 1) & (QUEUELEN - 1);
    used++;
  } else {
    Log("[Event] Queue full, dropping event!\n");
  }
}

void PushEventISR(EventT *event) {
  /* Raise IPL to interrupt mask level while touching queue shared with tasks. */
  u_short ipl = SetIPL(SR_IM);
  _PushEvent(event);
  (void)SetIPL(ipl);
}

/* PushEvent — task-context enqueue with global interrupt disable/enable guard. */
void PushEvent(EventT *event) {
  IntrDisable();
  _PushEvent(event);
  IntrEnable();
}

bool PopEvent(EventT *event) {
  bool present = false;

  IntrDisable();

  if (used > 0) {
    /* FIFO order: consume from head and wrap with power-of-two mask. */
    present = true;
    *event = queue[head];
    head = (head + 1) & (QUEUELEN - 1);
    used--;
  }

  IntrEnable();

  return present;
}
