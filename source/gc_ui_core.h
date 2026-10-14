#ifndef GC_UI_CORE_H
#define GC_UI_CORE_H
#include <stdint.h>
#define UI_UP 1u
#define UI_DOWN 2u
#define UI_LEFT 4u
#define UI_RIGHT 8u
typedef struct { unsigned held; uint64_t next; } UiRepeat;
unsigned ui_repeat(UiRepeat *state,unsigned held,uint64_t milliseconds);
int ui_viewport(int count,int selected,int *scroll,int visible);
int ui_grid_move(int selected,int count,int columns,unsigned direction);
uint32_t ui_utf8_next(const char **cursor);
#endif
