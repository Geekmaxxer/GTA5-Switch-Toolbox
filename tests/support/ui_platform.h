#ifndef UI_PLATFORM_H
#define UI_PLATFORM_H
#include <stdint.h>
#define __attribute__(x)
typedef struct {int unused;} PadState;
typedef struct {int unused;} Framebuffer;
typedef struct {uint64_t uid[2];} AccountUid;
typedef struct {int unused;} FsFileSystem;
typedef struct {int x,y;} HidAnalogStickState;
typedef struct {int count;struct {int x,y;} touches[16];} HidTouchScreenState;
enum {
    HidNpadButton_A=1,HidNpadButton_B=2,HidNpadButton_X=4,HidNpadButton_Y=8,
    HidNpadButton_L=64,HidNpadButton_R=128,HidNpadButton_ZL=256,HidNpadButton_ZR=512,
    HidNpadButton_Plus=1024,HidNpadButton_Minus=2048,HidNpadButton_Left=4096,
    HidNpadButton_Up=8192,HidNpadButton_Right=16384,HidNpadButton_Down=32768
};
static uint64_t fixture_tick=192000000ULL;
static uint64_t svcGetSystemTick(void) {return fixture_tick;}
#endif
