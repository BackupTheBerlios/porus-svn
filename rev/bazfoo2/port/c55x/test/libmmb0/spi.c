
#include "spi.h"

static MCBSP_Handle mcbsp;

u8 spi_shift8(u8 d)
{
	while (!MCBSP_xrdy(mcbsp));
	MCBSP_write16(mcbsp,d);
	while (!MCBSP_rrdy(mcbsp));
	return (u8)(MCBSP_read16(mcbsp)&0xff);
}

void spi_flush(void)
{
	while (MCBSP_xempty(mcbsp));
}

s32 spi_rs24(void)
{
	s32 data;
	
	data=spi_r8()<<16;
	data|=spi_r8()<<8;
	data|=spi_r8();
	if (data&0x00800000) data|=0xff000000;
	return data;
}

void spi_init(void)
{
	mcbsp=MCBSP_open(MCBSP_PORT1,MCBSP_OPEN_RESET);
	MCBSP_config(mcbsp,&MCBSP_slowspi);
	//MCBSP_start?
}
