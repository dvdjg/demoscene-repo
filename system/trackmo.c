/*
 * Trackmo disk bootstrap: open floppy and initialize the custom filesystem layer.
 *
 * Purpose: when running from a custom disk trackloader (not AmigaDOS), BootDev
 * selects which drive; we open it via the floppy driver and InitFileSys so
 * subsequent effect loads can read packed data from disk.
 *
 * RKM / physical drivers are abstracted here — see system/drivers/filesys.c.
 */
#include <linkerset.h>
#include <debug.h>
#include <system/boot.h>
#include <system/filesys.h>
#include <system/file.h>
#include <system/floppy.h>
#include <system/memfile.h>

void InitTrackmo(void) {
  FileT *dev = NULL;
  if (BootDev != 0xff) {
    dev = FloppyOpen(BootDev);
  } else {
    Panic("[Trackmo] Not configured to run from: %d!", BootDev);
  }
  InitFileSys(dev);
}

void CheckTrackmo(void) {
  CheckFileSys();
}

void KillTrackmo(void) {
  KillFileSys();
}

ADD2INIT(InitTrackmo, 0);
ADD2EXIT(KillTrackmo, 0);
