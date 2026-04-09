/*
 * engine-demo — minimal effect using amiga-engine Amiga C bridge (copper + libgfx).
 */
#include <effect.h>

#include <amiga_engine/ae_amiga.h>

static void Init(void) { AeEngine_Init(); }

static void Kill(void) { AeEngine_Shutdown(); }

static void Render(void) { AeEngine_RenderFrame(); }

EFFECT(EngineDemo, NULL, NULL, Init, Kill, Render, NULL);
