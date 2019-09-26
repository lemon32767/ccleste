#include <allegro5/allegro.h>
#include "celeste.h"

void InitEmu(void);
void DeinitEmu(void);
void InputEvKeyDown(int k);
void InputEvKeyUp(int k);
void EmuUpdate();
void EmuDraw();

Celeste_P8_val HandleP8Call(CELESTE_P8_CALLBACK_TYPE t, ...);