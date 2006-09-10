
#ifndef GUARD_tms320vc5507_h
#define GUARD_tms320vc5507_h

#define IOTYPE 	ioport unsigned short *
#define IOTYPE_SIGNED ioport short *
#define IOREG(a)	*(IOTYPE)(a)
#define IOREG_SIGNED(a) *(IOTYPE_SIGNED)(a)

#include "5509a_emif.h"

#define I2C_BASE 	0x3C00
#include "5509a_i2c.h"

#define MCBSP_BASE	0x2800
#include "5509a_mcbsp.h"

#define DMA_BASE	0xC00
#include "5509a_dma.h"

#define CLKMD		IOREG(0x1C00)
#define ICR		IOREG(0x0001)
#define ISTR		IOREG(0x0002)
#define SYSR		IOREG(0x07FD)
#define IODIR		IOREG(0x3400)
#define IODATA	IOREG(0x3401)

#define DMACCR_REVT1	5

#endif
