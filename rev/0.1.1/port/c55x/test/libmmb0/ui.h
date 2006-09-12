
#ifndef GUARD_ui_h
#define GUARD_ui_h

#include "types.h"
#include "task.h"

u8 led(void);
void led_set(u8 d);
void led_dp(int i);
void led_showchar(char c);
void led_showdig(int d);
u8 sw_read(void);

#endif
