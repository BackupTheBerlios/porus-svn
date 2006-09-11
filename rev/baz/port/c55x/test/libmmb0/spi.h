
#ifndef GUARD_spi_h
#define GUARD_spi_h

#include "types.h"

u8 spi_shift8(u8 d);

inline extern void spi_w8(u8 d)
{
	while (!(IFG1&UTXIFG0));
	U0TXBUF=d;
}

inline extern void spi_flush(void)
{
	while (!(U0TCTL&TXEPT));
}

inline extern u8 spi_shift8(u8 out)
{
	while (!(IFG1&UTXIFG0));
	U0TXBUF=out;
	spi_flush();
	return U0RXBUF;
}

inline extern u8 spi_r8(void)
{
	return spi_shift8(0xff);
}

void spi_init(void);
s32 spi_rs24(void);

#endif
