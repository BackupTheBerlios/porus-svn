
#include "i2c.h"
#include "mmb0.h"

#ifndef I2C_MODCLOCK
#define I2C_MODCLOCK 8000000UL
#endif

#ifndef I2C_DEFAULT_DATARATE
#define I2C_DEFAULT_DATARATE 400000
#endif

#define POSTDELAY 100000

static u8 current_bus;

#if 0
static void postdelay(void)
{
	volatile unsigned long i=POSTDELAY;
	
	while(--i);
}
#endif

void i2c_lock(void)
{
	//IRQ_enable(IRQ_EVT_INT2);
}

void i2c_unlock(void)
{
	//IRQ_disable(IRQ_EVT_INT2);
}

#if 0
static void sendstop(void)
{
	//if (!(I2CSTR&I2CSTR_BB)) return;
	I2CMDR|=I2CMDR_STP;
	while (I2CMDR&I2CMDR_STP);
}
#endif

static int wr(u8 adr, u8 *b, int len, int stop)
{
	u8 stat,mdr;

	I2CCNT=len;
	I2CSAR=adr;
	I2CDXR=*b++;
	while (I2CSTR&I2CSTR_BB);
	I2CMDR=0x2E20;
	for(;;) {
		stat=I2CSTR;
		mdr=I2CMDR;
		if (stat&I2CSTR_NACK) break;
		//if (stat&I2CSTR_ARDY) break;
		if (!(mdr&I2CMDR_STP)) break;
		if (stat&I2CSTR_XRDY)
			I2CDXR=*b++;
	}
//	for(;;) {
//		I2CMDR=0x0E20;
//		if (I2CMDR&I2CMDR_STP) break;
//	}
//	stat=I2CSTR;
	if (stat&I2CSTR_NACK) return -1;
	return 0;
}

static int rd(u8 adr, u8 *b, int len)
{
	u8 stat,mdr;

	I2CCNT=len;
	I2CSAR=adr;
	while (I2CSTR&I2CSTR_BB);
	I2CMDR=0x2C20;
	for(;;) {
		stat=I2CSTR;
		mdr=I2CMDR;
		if (stat&I2CSTR_NACK) break;
		//if (stat&I2CSTR_ARDY) break;
		if (!(mdr&I2CMDR_STP)) break;
		if (stat&I2CSTR_RRDY)
			*b++=I2CDRR;
	}
//	for(;;) {
//		I2CMDR=0x2C20;
//		if (I2CMDR&I2CMDR_STP) break;
//	}
	*b++=I2CDRR;
	if (stat&I2CSTR_NACK) return -1;
	return 0;

}

#if 0
static int wr(u8 adr, u8 *b, int len, int stop)
{
	while (I2CSTR&I2CSTR_BB);
	I2CSTR=I2CSTR_XRDY|I2CSTR_RRDY|I2CSTR_ARDY|I2CSTR_NACK|I2CSTR_AL;
	I2CSAR=adr;
	I2CDXR=*b++;
#if 1
	I2CMDR=I2CMDR_MST|I2CMDR_TRX|I2CMDR_IRS|I2CMDR_FREE|I2CMDR_RM;
	I2CMDR|=I2CMDR_STT;
	//while (I2CSTR&I2CSTR_XSMT); // wait for address to go out
	//if (I2CSTR&I2CSTR_NACK) return -1; // oops, nak of adr
	while (len) {
		if (len<2) I2CMDR|=I2CMDR_STP;
		while (!(I2CSTR&I2CSTR_XRDY)) {
			if (len>1&&I2CSTR&I2CSTR_NACK) {
				I2CSTR=I2CSTR_NACK;
				sendstop();
				I2CMDR=0;
				I2CMDR=I2CMDR_IRS;
				return -2; // nack during transmission
			}
		}
		--len;
		if (!len) break;
		I2CDXR=*b++;
	}
	//while (!(I2CSTR&I2CSTR_XSMT));
	if (stop) sendstop();
#else
	I2CCNT=len;
	I2CMDR=I2CMDR_MST|I2CMDR_TRX|I2CMDR_IRS|I2CMDR_FREE;
	I2CMDR=I2CMDR_STT|I2CMDR_STP;
	while (I2CSTR&I2CSTR_BB) {
		if (I2CSTR&I2CSTR_XRDY)
			I2CDXR=*b++;
	}
#endif

	postdelay();
	return 0;
}

static int rd(u8 adr, u8 *b, int len)
{
	if (!len) return 0;
	I2CSAR=adr;
	I2CCNT=len;
	I2CSTR=I2CSTR_RRDY|I2CSTR_ARDY|I2CSTR_NACK|I2CSTR_AL;
	I2CMDR=I2CMDR_MST|I2CMDR_IRS|I2CMDR_FREE;
	I2CMDR|=I2CMDR_STT;
	//if (I2CSTR&I2CSTR_NACK) return -1; // oops, nak of adr
	while (len) {
		while (!(I2CSTR&I2CSTR_RRDY)) {
			if (I2CSTR&I2CSTR_NACK) {
				I2CSTR=I2CSTR_NACK;
				sendstop();
				I2CMDR=0;
				I2CMDR=I2CMDR_IRS;
				return -2; // early nack
			}
		}
		*b++=I2CDRR;
		--len;
	}
	sendstop();
	postdelay();
	return 0;
}
#endif

void i2c_set_bus(u8 bus)
{
	u8 d;

	if (bus==current_bus) return;
	d=4|(bus&3);
	i2c_lock();
	while (wr(I2C_ADR_MUX>>1,&d,1,1));
	i2c_unlock();
	current_bus=bus;
}

void i2c_wr(u8 bus, u8 adr, u8 *b, int len)
{
	i2c_lock();
	i2c_set_bus(bus);
	wr(adr>>1,b,len,1);
	i2c_unlock();
}

void i2c_w8(u8 bus, u8 adr, u8 b)
{
	u8 d=b;

	i2c_lock();
	i2c_set_bus(bus);
	wr(adr>>1,&d,1,1);
	i2c_unlock();
}

u8 i2c_r8(u8 bus, u8 adr)
{
	u8 d;

	i2c_lock();
	i2c_set_bus(bus);
	rd(adr>>1,&d,1);
	i2c_unlock();
	return d;
}

void i2c_frob(u8 bus, u8 adr, u8 data, u8 mask)
{
	i2c_w8(bus,adr,(i2c_r8(bus,adr)&~mask)|(data&mask));
}

void i2c_reset(u32 clkrate, u16 selfadr)
{
	u16 psc,d,div;

	I2CMDR=0; // resets module by clearing IRS
	I2CMDR2=0;
	I2CIER=0; // we poll everything at this time
	psc=(CPUCLOCK/I2C_MODCLOCK)-1;
	if (psc==0) d=7;
	else if (psc==1) d=6;
	else d=5;
	div=((CPUCLOCK/psc)/clkrate)-d;
	I2CPSC=psc;
	div/=2;
	I2CCLKL=div;
	I2CCLKH=div;
	I2COAR=selfadr>>1;
	I2CMDR=I2CMDR_IRS;
	I2CSTR=I2CSTR_BB;
}

void i2c_init(void)
{
	current_bus=0xff;
	i2c_reset(I2C_DEFAULT_DATARATE,0);
	i2c_set_bus(I2C_BUS_MB);
}
