/*
 * Music playback / audio module interface (ahx.h).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __AHX_H__
#define __AHX_H__

#include "types.h"

#define AHX_SAMPLE_LEN 320

typedef struct AhxVoiceTemp {
  char Track;
  char Transpose;
  char NextTrack;
  char NextTranspose;
  char ADSRVolume;
  char pad0[91];
  void *AudioPointer;
  short AudioPeriod;
  short AudioVolume;
  char pad1[128];
} AhxVoiceTempT; /* sizeof(AhxVoiceTempT) == 232 */

typedef struct AhxInfo {
  char ExternalTiming;
  char MainVolume;
  char Subsongs;
  char SongEnd;
  char Playing;
  char pad1[3];
  int FrameCount;
  char pad2[2];
  AhxVoiceTempT VoiceTemp[4];
  char pad3[156];
  short Row;
  short Pos;
} AhxInfoT;

struct AhxPlayer {
  AhxInfoT *Public;   // pointer to ahx's public (fast) memory block
  void *Chip;         // pointer to ahx's explicit chip memory block
  u_int PublicSize;   // size of public memory (intern use only!)
  u_int ChipSize;     // size of chip memory (intern use only!)
  void *Module;       // pointer to ahxModule after InitModule
  u_int IsCIA;        // byte flag (using ANY (intern/own) cia?)
  u_int Tempo;        // word to cia tempo (normally NOT needed to xs)
};

int AhxInitCIA(void (*ciaInt)(void));
void AhxKillCIA(void);

#define AHX_LOAD_WAVES_FILE 0
#define AHX_EXPLICIT_WAVES_PRECALCING 1
#define AHX_FILTERS 0
#define AHX_NO_FILTERS 1

int AhxInitPlayer(int waves __ASM_REG_PARM("d0"), int filters __ASM_REG_PARM("d1"));

int AhxInitModule(void *module __ASM_REG_PARM("a0"));
int AhxInitSubSong(int subsong __ASM_REG_PARM("d0"), int waitPlay __ASM_REG_PARM("d1"));
void AhxInterrupt(void);
void AhxStopSong(void);
void AhxKillPlayer(void);
void AhxNextPattern(void);
void AhxPrevPattern(void);

extern volatile struct AhxPlayer Ahx;

#endif
