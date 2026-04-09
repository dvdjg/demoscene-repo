/*
 * Serial port as FileT: IRQ-driven RX/TX queues.
 *
 * Purpose: wraps the UART (INTEN TBE/RBF) with small ring buffers so tasks can
 * read/write without losing bytes at moderate baud rates. Uses multitasking
 * notify when data arrives.
 *
 * HRM (serial hardware): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <custom.h>
#include <debug.h>
#include <stdio.h>
#include <strings.h>
#include <system/file.h>
#include <system/interrupt.h>
#include <system/memory.h>
#include <system/mutex.h>
#include <system/task.h>
#include <system/serial.h>

#define CLOCK 3546895
#define QUEUELEN 80

/* CharQueueT — byte FIFO shared between task context and UART ISRs. */
typedef struct {
  u_short head, tail;
  volatile u_short used;
  u_char data[QUEUELEN];
} CharQueueT;

struct File {
  FileOpsT *ops;
  u_int flags;
  /* sendq: bytes waiting for TBE interrupt to drain. */
  CharQueueT sendq[1];
  /* recvq: bytes captured by RBF interrupt until readers consume them. */
  CharQueueT recvq[1];
};

/* _PushChar — low-level enqueue without locking (caller must provide atomicity). */
static void _PushChar(CharQueueT *queue, u_char data) {
  if (queue->used < QUEUELEN) {
    queue->data[queue->tail] = data;
    queue->tail = (queue->tail + 1) & (QUEUELEN - 1);
    queue->used++;
  } else {
    Log("[Serial] Queue full, dropping character!\n");
  }
}

/* _PopChar — low-level dequeue without locking, returns -1 if empty. */
static int _PopChar(CharQueueT *queue) {
  int result = -1;
  if (queue->used > 0) {
    result = queue->data[queue->head];
    queue->head = (queue->head + 1) & (QUEUELEN - 1);
    queue->used--;
  }
  return result;
}

/* PushChar — enqueue for TX, or write directly to SERDAT when transmitter is idle.
 * Blocking mode waits for queue space via TaskWait(INTF_TBE). */
static void PushChar(CharQueueT *queue, u_char data, u_int flags) {
  /* If send queue and serdat register are empty,
   * then push out first character directly. */
  IntrDisable();
  if (queue->used == 0 && custom->serdatr & SERDATF_TBE) {
    custom->serdat = data | 0x100;
  } else {
    while (queue->used == QUEUELEN && !(flags & O_NONBLOCK))
      TaskWait(INTF_TBE);
    _PushChar(queue, data);
  }
  IntrEnable();
}

/* PopChar — dequeue one RX byte; blocking mode waits for INTF_RBF notification. */
static int PopChar(CharQueueT *queue, u_int flags) {
  int result;
  IntrDisable();
  for (;;) {
    result = _PopChar(queue);
    if (result >= 0 || flags & O_NONBLOCK)
      break;
    TaskWait(INTF_RBF);
  }
  IntrEnable();
  return result;
}

/* SendIntHandler — TBE ISR: feed next queued byte to UART TX register. */
static void SendIntHandler(FileT *f) {
  CharQueueT *sendq = f->sendq;
  int data;
  ClearIRQ(INTF_TBE);
  data = _PopChar(sendq);
  if (data >= 0) {
    custom->serdat = data | 0x100;
    if (!(f->flags & O_NONBLOCK))
      TaskNotifyISR(INTF_TBE);
  }
}

/* RecvIntHandler — RBF ISR: move received byte from SERDATR into recv queue. */
static void RecvIntHandler(FileT *f) {
  CharQueueT *recvq = f->recvq;
  u_short code = custom->serdatr;
  ClearIRQ(INTF_RBF);
  if (code & SERDATF_RBF) {
    _PushChar(recvq, code);
    if (!(f->flags & O_NONBLOCK))
      TaskNotifyISR(INTF_RBF);
  }
}

static int SerialRead(FileT *f, void *buf, u_int nbyte);
static int SerialWrite(FileT *f, const void *buf, u_int nbyte);
static void SerialClose(FileT *f);

static FileOpsT SerialOps = {
  .read = SerialRead,
  .write = SerialWrite,
  .seek = NoSeek,
  .close = SerialClose
};

static MUTEX(SerialMtx);

/* OpenSerial — singleton open: config baud divider, install IRQ handlers, enable INTs. */
FileT *OpenSerial(u_int baud, u_int flags) {
  static FileT *f = NULL;

  MutexLock(&SerialMtx);

  if (f == NULL) {
    f = MemAlloc(sizeof(FileT), MEMF_PUBLIC|MEMF_CLEAR);
    f->ops = &SerialOps;
    f->flags = flags;

    custom->serper = CLOCK / baud - 1;

    SetIntVector(INTB_TBE, (IntHandlerT)SendIntHandler, f);
    SetIntVector(INTB_RBF, (IntHandlerT)RecvIntHandler, f);

    ClearIRQ(INTF_TBE | INTF_RBF);
    EnableINT(INTF_TBE | INTF_RBF);
  }

  MutexUnlock(&SerialMtx);

  return f;
}

static void SerialClose(FileT *f) {
  DisableINT(INTF_TBE | INTF_RBF);
  ClearIRQ(INTF_TBE | INTF_RBF);

  ResetIntVector(INTB_RBF);
  ResetIntVector(INTB_TBE);

  MemFree(f);
}

/* SerialWrite — push buffer to TX queue; emits CR after LF for terminal compatibility. */
static int SerialWrite(FileT *f, const void *_buf, u_int nbyte) {
  const u_char *buf = _buf;
  u_int i;

  for (i = 0; i < nbyte; i++) {
    u_char data = *buf++;
    PushChar(f->sendq, data, f->flags);
    if (data == '\n')
      PushChar(f->sendq, '\r', f->flags);
  }

  return nbyte;
}

/* SerialRead — read up to nbyte bytes, stopping early on newline. */
static int SerialRead(FileT *f, void *_buf, u_int nbyte) {
  u_char *buf = _buf;
  u_int i = 0;

  while (i < nbyte) {
    buf[i] = PopChar(f->recvq, f->flags);
    if (buf[i++] == '\n')
      break;
  }

  return i;
}
