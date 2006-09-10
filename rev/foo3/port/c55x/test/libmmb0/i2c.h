
#ifndef GUARD_i2c_h
#define GUARD_i2c_h

#include "types.h"

void i2c_w8(u8 bus, u8 adr, u8 b);
void i2c_wr(u8 bus, u8 adr, u8 *b, int len);
u8 i2c_r8(u8 bus, u8 adr);
void i2c_frob(u8 bus, u8 adr, u8 data, u8 mask);
void i2c_set_bus(u8 bus);
void i2c_init(void);

#endif
