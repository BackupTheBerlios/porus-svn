
#include "mmb0.h"
#include "flash.h"
#include "spibb.h"

#define INS_WREN 6
#define INS_WRDI 4
#define INS_RDSR 5
#define INS_WRSR 1
#define INS_READ 3
#define INS_FAST_READ 11
#define INS_PP 2
#define INS_SE 0xD8
#define INS_BE 0xC7
#define INS_DP 0xB9
#define INS_RES 0xAB

static spibb_t spic;

static void istart(u8 ins)
{
	pin_set_ROMCS(0);
	spibb_shift8(&spic,ins);
}

static void istop(void)
{
	pin_set_ROMCS(1);
}

u8 flash_rdsr(void)
{
	u8 s;

	istart(INS_RDSR);
	s=spibb_shift8(&spic,0);
	istop();
	
	return s;
}

void flash_wren(void)
{
	istart(INS_WREN);
	istop();
}

void flash_wrdi(void)
{
	istart(INS_WRDI);
	istop();
}

void flash_wrsr(u8 sr)
{
	istart(INS_WRSR);
	spibb_shift8(&spic,sr);
	istop();
}

void flash_read(u32 adr, u16 *data, u32 len)
{
	u32 i;

	istart(INS_FAST_READ);
	spibb_wu24(&spic,adr);
	spibb_shift8(&spic,0);
	for (i=0;i<len;++i) {
		if (i&1) {
			data[i>>1]|=spibb_shift8(&spic,0);
		} else {
			data[i>>1]=spibb_shift8(&spic,0)<<8;
		}
	}
	istop();
}

u8 flash_res(void)
{
	u8 a;

	istart(INS_RES);
	spibb_shift8(&spic,0);
	spibb_shift8(&spic,0);
	spibb_shift8(&spic,0);
	a=spibb_shift8(&spic,0);
	istop();
	return a;
}

void flash_dp(void)
{
	istart(INS_DP);
	istop();
}

void flash_be(void)
{
	istart(INS_BE);
	istop();
}

void flash_se(int sec)
{
	u32 adr=(u32)sec<<15;
	
	istart(INS_SE);
	spibb_wu24(&spic,adr);
	istop();
}

void flash_pp(u32 adr, u16 *data, u16 len)
{
	u16 i;

	istart(INS_PP);
	spibb_wu24(&spic,adr);
	for (i=0;i<len;++i) {
		if (i&1)
			spibb_shift8(&spic,data[i>>1]&0xff);
		else
			spibb_shift8(&spic,(data[i>>1]>>8)&0xff);
	}
	istop();
}

int flash_test(void)
{
	if (flash_res()!=0x10) return -1;
	else return 0;
}

void flash_init(void)
{
	pin_set_ROMCS(1);
	spibb_open(&spic,0);
	spibb_set_phase(&spic,0,1,0);
}
