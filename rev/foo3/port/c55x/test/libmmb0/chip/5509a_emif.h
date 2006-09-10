
#ifndef GUARD_emif_5509a_h
#define GUARD_emif_5509a_h

#define IOTYPE 		ioport unsigned short *
#define IOREG(a)		*(IOTYPE)(a)

#define EMIF_EGCR		IOREG(0x0800)
#define EMIF_EMIRST		IOREG(0x0801)     
#define EMIF_EMIBE		IOREG(0x0802)
#define EMIF_CE01		IOREG(0x0803)
#define EMIF_CE02		IOREG(0x0804)
#define EMIF_CE03		IOREG(0x0805)
#define EMIF_CE11		IOREG(0x0806)
#define EMIF_CE12		IOREG(0x0807)
#define EMIF_CE13		IOREG(0x0808)
#define EMIF_CE21		IOREG(0x0809)
#define EMIF_CE22		IOREG(0x080A)
#define EMIF_CE23		IOREG(0x080B)
#define EMIF_CE31		IOREG(0x080C)
#define EMIF_CE32		IOREG(0x080D)
#define EMIF_CE33		IOREG(0x080E)
#define EMIF_SDC1       	IOREG(0x080F)
#define EMIF_SDPER      	IOREG(0x0810)
#define EMIF_SDCNT      	IOREG(0x0811)
#define EMIF_INIT       	IOREG(0x0812)
#define EMIF_SDC2       	IOREG(0x0813)
#define EMIF_SDC3       	IOREG(0x0814)

#endif
