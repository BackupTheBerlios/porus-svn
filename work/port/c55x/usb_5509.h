
#ifndef GUARD_usb_5509_h
#define GUARD_usb_5509_h

#define USB_BASE 		0x5800

#define IOTYPE 		ioport unsigned short *
#define IOREG(a)		(*(IOTYPE)(a))
#define USBREG(ofs)		IOREG(USB_BASE+ofs)

#define USBPLL			IOREG(0x1E00)

#define USBIDLECTL		IOREG(0x7000)
#define USBIDLECTL_USBRST	(1<<2)
#define USBIDLECTL_IDLESTAT	(1<<1)
#define USBIDLECTL_IDLEEN	(1<<0)

/* Endpoint numbers (for 'ep'): IN is 1-7, OUT is 9-15 */
#define USBDMA(ep,reg)	USBREG(((ep)*8+(reg)))
#define USBBUF(reg)		USBREG((0x80+(reg)))
#define USBBUFOUT0(reg)	USBREG((0xE80+(reg)))
#define USBBUFIN0(reg)	USBREG((0xEC0+(reg)))
#define USBBUFSETUP(reg)	USBREG((0xF00+(reg)))
#define USBEPDEF(ep,reg)	USBREG((0xF00+(ep)*8+(reg)))

#define USBICNF0		USBREG(0xF80)
#define USBICNF0_UBME		(1<<7)
#define USBICNF0_TOGGLE	(1<<5)
#define USBICNF0_STALL	(1<<3)
#define USBICNF0_INTE		(1<<2)

#define USBICT0		USBREG(0xF81)
#define USBICT0_NAK		(1<<7)
#define USBICT0_COUNT		(0x7F)

#define USBOCNF0		USBREG(0xF82)
#define USBOCNF0_UBME		(1<<7)
#define USBOCNF0_TOGGLE	(1<<5)
#define USBOCNF0_STALL	(1<<3)
#define USBOCNF0_INTE		(1<<2)

#define USBOCT0		USBREG(0xF83)
#define USBOCT0_NAK		(1<<7)
#define USBOCT0_COUNT		(0x7F)

#define USBGCTL		USBREG(0xF91)
#define USBGCTL_SOFTRST	(1)

#define USBINTSRC		USBREG(0xF92)
#define USBIEPIF		USBREG(0xF93)
#define USBOEPIF		USBREG(0xF94)
#define USBIDRIF		USBREG(0xF95)
#define USBODRIF		USBREG(0xF96)
#define USBIDGIF		USBREG(0xF97)
#define USBODGIF		USBREG(0xF98)
#define USBIDIE		USBREG(0xF99)
#define USBODIE		USBREG(0xF9A)
#define USBIEPIE		USBREG(0xF9B)
#define USBOEPIE		USBREG(0xF9C)
#define USBFNUML		USBREG(0xFF8)
#define USBFNUMH		USBREG(0xFF9)
#define USBPSOFTMR		USBREG(0xFFA)

#define USBCTL			USBREG(0xFFC)
#define USBCTL_CONN		(1<<7)
#define USBCTL_FEN		(1<<6)
#define USBCTL_RWUP		(1<<5)
#define USBCTL_FRSTE		(1<<4)
#define USBCTL_SETUP		(1<<1)
#define USBCTL_DIR		(1<<0)

#define USBIE			USBREG(0xFFD)
#define USBIE_RSTR		(1<<7)
#define USBIE_SUSR		(1<<6)
#define USBIE_RESR		(1<<5)
#define USBIE_SOF		(1<<4)
#define USBIE_PSOF		(1<<3)
#define USBIE_SETUP		(1<<2)
#define USBIE_STPOW		(1<<0)

#define USBIF			USBREG(0xFFE)
#define USBIF_RSTR		(1<<7)
#define USBIF_SUSR		(1<<6)
#define USBIF_RESR		(1<<5)
#define USBIF_SOF		(1<<4)
#define USBIF_PSOF		(1<<3)
#define USBIF_SETUP		(1<<2)
#define USBIF_STPOW		(1<<0)

#define USBADDR		USBREG(0xFFF)

/* reg field offsets */

// for USBDMA
//#define USBIDCTL		0
#define USBIDCTL(ep)		USBDMA(ep,0)
#define USBIDCTL_PM		(1<<8)
#define USBIDCTL_EM		(1<<7)
#define USBIDCTL_SHT		(1<<6)
#define USBIDCTL_CAT		(1<<5)
#define USBIDCTL_END		(1<<4)
#define USBIDCTL_OVF		(1<<3)
#define USBIDCTL_RLD		(1<<2)
#define USBIDCTL_STP		(1<<1)
#define USBIDCTL_GO		(1<<0)

#define USBIDSIZ(ep)		USBDMA(ep,1)
#define USBIDADL(ep)		USBDMA(ep,2)
#define USBIDADH(ep)		USBDMA(ep,3)
#define USBIDCT(ep)		USBDMA(ep,4)
#define USBIDRSZ(ep)		USBDMA(ep,5)
#define USBIDRAL(ep)		USBDMA(ep,6)
#define USBIDRAH(ep)		USBDMA(ep,7)

#define USBODCTL(ep)		USBDMA(ep,0)
#define USBODCTL_PM		(1<<8)
#define USBODCTL_EM		(1<<7)
#define USBODCTL_SHT		(1<<6)
#define USBODCTL_CAT		(1<<5)
#define USBODCTL_END		(1<<4)
#define USBODCTL_OVF		(1<<3)
#define USBODCTL_RLD		(1<<2)
#define USBODCTL_STP		(1<<1)
#define USBODCTL_GO		(1<<0)

#define USBODSIZ(ep)		USBDMA(ep,1)
#define USBODADL(ep)		USBDMA(ep,2)
#define USBODADH(ep)		USBDMA(ep,3)
#define USBODCT(ep)	USBDMA(ep,4)
#define USBODRSZ(ep)		USBDMA(ep,5)
#define USBODRAL(ep)		USBDMA(ep,6)
#define USBODRAH(ep)		USBDMA(ep,7)

// for USBEPDEF
#define USBICNF(ep)		USBEPDEF(ep,0)
#define USBICNF_UBME		(1<<7)
#define USBICNF_ISO		(1<<6)
#define USBICNF_TOGGLE	(1<<5)
#define USBICNF_DBUF		(1<<4)
#define USBICNF_STALL		(1<<3)

#define USBIBAX(ep)		USBEPDEF(ep,1)

#define USBICTX(ep)		USBEPDEF(ep,2)
#define USBICTX_NAK		(1<<7)

#define USBISIZH(ep)		USBEPDEF(ep,3)
#define USBISIZ(ep)		USBEPDEF(ep,4)
#define USBIBAY(ep)		USBEPDEF(ep,5)

#define USBICTY(ep)		USBEPDEF(ep,6)
#define USBICTY_NAK		(1<<7)

#define USBOCNF(ep)		USBEPDEF(ep,0)
#define USBOCNF_UBME		(1<<7)
#define USBOCNF_ISO		(1<<6)
#define USBOCNF_TOGGLE	(1<<5)
#define USBOCNF_DBUF		(1<<4)
#define USBOCNF_STALL		(1<<3)

#define USBOBAX(ep)		USBEPDEF(ep,1)

#define USBOCTX(ep)		USBEPDEF(ep,2)
#define USBOCTX_NAK		(1<<7)

#define USBOCTXH(ep)		USBEPDEF(ep,3)
#define USBOSIZ(ep)		USBEPDEF(ep,4)
#define USBOBAY(ep)		USBEPDEF(ep,5)

#define USBOCTY(ep)		USBEPDEF(ep,6)
#define USBOCTY_NAK		(1<<7)

#define USBOCTYH(ep)		USBEPDEF(ep,7)

#endif
