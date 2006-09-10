
#include "spibb.h"
#include "mmb0.h"

#define CLKHI(C) PCR(C->port)|=PCR_CLKXP
#define CLKLO(C) PCR(C->port)&=~PCR_CLKXP
#define DHI(C) PCR(C->port)|=PCR_DXSTAT
#define DLO(C) PCR(C->port)&=~PCR_DXSTAT
#define CSHI(C) PCR(C->port)|=PCR_FSXP
#define CSLO(C) PCR(C->port)&=~PCR_FSXP
#define DR(C) (PCR(C->port)&PCR_DRSTAT)

static void delay(u32 d)
{
	volatile u32 i;
	
	for (i=0;i<d;++i);
}

void spibb_cshi(spibb_t *c)
{
	CSHI(c);
}

void spibb_cslo(spibb_t *c)
{
	CSLO(c);
}

u8 spibb_shift8(spibb_t *c, u8 out)
{
	u8 in;
	u8 mask;

	mask=c->lsbfirst?1:0x80;
	in=0;
	while (mask&0xff) {
		if (mask&out)
			DHI(c);
		else
			DLO(c);
		if (c->slfall) {
			CLKHI(c);
			delay(c->delay1);
			CLKLO(c);
		} else {
			CLKLO(c);
			delay(c->delay1);
			CLKHI(c);
		}
		if (DR(c))
			in|=mask;
		if (c->lsbfirst)
			mask<<=1;
		else
			mask>>=1;
		if (mask&0xff)
			delay(c->delay2);
	}
	if (c->slfall&&c->idlehi) {
		delay(c->delay2);
		CLKHI(c);
	} else if (!(c->slfall||c->idlehi)) {
		delay(c->delay2);
		CLKLO(c);
	}
	return in;
}

s32 spibb_rs24(spibb_t *c)
{
	s32 data;
	
	data=((s32)spibb_shift8(c,0))<<16;
	data|=((s32)spibb_shift8(c,0))<<8;
	data|=spibb_shift8(c,0);
	if (data&0x00800000) data|=0xff000000;
	return data;
}

void spibb_wu24(spibb_t *c, u32 d)
{
	spibb_shift8(c,(d>>16)&0xff);
	spibb_shift8(c,(d>>8 )&0xff);
	spibb_shift8(c,(d    )&0xff);
}

void spibb_set_phase(spibb_t *c, int slfall, int idlehi, int lsbfirst)
{
	c->slfall=slfall;
	c->idlehi=idlehi;
	c->lsbfirst=lsbfirst;
}

void spibb_set_delay(spibb_t *c, int ph1, int ph2)
{
	c->delay1=ph1;
	c->delay2=ph2;
}

int spibb_open(spibb_t *c, int port)
{
	if (!c) return -1;
	c->port=port;

	SPCR1(port)=0;
	SPCR2(port)=0;
	PCR(port)=PCR_XIOEN|PCR_RIOEN|PCR_CLKXM|PCR_FSXM;
	c->delay1=8;
	c->delay2=8;
	c->idlehi=1;
	c->slfall=0;
	c->lsbfirst=0;
	return 0;
}
