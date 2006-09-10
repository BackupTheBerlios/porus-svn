
#ifndef GUARD_flash_h
#define GUARD_flash_h

#include "types.h"

u8 flash_rdsr(void);
void flash_wren(void);
void flash_wrdi(void);
void flash_wrsr(u8 sr);
u8 flash_res(void);
void flash_dp(void);
void flash_be(void);
void flash_se(int sec);
void flash_pp(u32 adr, u16 *data, u16 len);

//! Erase entire flash
void flash_erase(void);

//! Read from flash
void flash_read(u32 adr, u16 *data, u32 len);

//! Test flash
int flash_test(void);

void flash_init(void);

#endif
