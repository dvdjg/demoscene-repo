/*
 * Amiga-side C bridge for the experimental engine layer (links with libgfx).
 * C++20 core is validated on host; this file is the on-target integration point
 * until m68k-amigaos-g++ is available in the toolchain.
 */
#include <copper.h>
#include <custom.h>
#include <effect.h>
#include <playfield.h>

#include <amiga_engine/ae_amiga.h>

static CopListT *ae_cop;

void AeEngine_Init(void) {
  ae_cop = NewCopList(64);
  if (!ae_cop)
    return;
  CopListReset(ae_cop);
  CopSetupMode(ae_cop, MODE_LORES, 1);
  CopSetupDisplayWindow(ae_cop, MODE_LORES, HP(0x81), VP(0x2c), 320, 256);
  CopSetColor(ae_cop, 0, 0x0340);
  CopListFinish(ae_cop);
  CopListActivate(ae_cop);
}

void AeEngine_Shutdown(void) {
  CopperStop();
  if (ae_cop) {
    DeleteCopList(ae_cop);
    ae_cop = NULL;
  }
}

void AeEngine_RenderFrame(void) {
  static u_short c;
  if (!ae_cop)
    return;
  custom->color[0] = c++;
  TaskWaitVBlank();
}
