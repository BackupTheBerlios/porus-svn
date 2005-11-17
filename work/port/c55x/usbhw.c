
#include "usbhw.h"
#include <bios.h>
#include <c55.h>
#include <usb_5509.h>
#include <mem.h>
#include <pgactlcfg.h>
#include <gbl.h>

#define pkt_from_pkt(P) ((usb_packet_req_t *)((P)->ep->data->hwdata))

typedef struct usb_packet_req_t {
	unsigned int done:1, // 0 = request waiting, 1 = request complete
		timeout:1; // 0 = no timeout, 1 = timeout occured
	usb_endpoint_t *ep; // ptr to endpoint
	usb_data_t *data; // ptr to buffer
	u16 reqlen; // requested length in bytes
	u16 actlen; // actual length in bytes
} usb_packet_req_t;

void usbhw_int_dis(void)
{
	C55_disableIER0(C55_IEN08);
}

void usbhw_int_en(void)
{
	C55_enableIER0(C55_IEN08);
}

void usbhw_int_en_sof(void)
{
	USBIE|=USBIE_SOF;
}

void usbhw_int_en_presof(void)
{
	USBIE|=USBIE_PSOF;
}

void usbhw_int_en_txdone(int epn)
{
	if (epn<16||epn>23) return;
	USBIEPIE|=(1<<(epn-16));
}

void usbhw_int_en_rxdone(int epn)
{
	if (epn<0||epn>7) return;
	USBOEPIE|=(1<<epn);
}

void usbhw_int_en_setup(void)
{
	USBIE|=USBIE_SETUP;
}

void usbhw_int_en_ctlin(void)
{
	USBICNF0|=USBICNF0_INTE;
	USBIEPIE|=1;
}

void usbhw_int_en_ctlout(void)
{
	USBOCNF0|=USBOCNF0_INTE;
	USBOEPIE|=1;
}

void usbhw_int_dis_sof(void)
{
	USBIE&=~USBIE_SOF;
}

void usbhw_int_dis_presof(void)
{
	USBIE&=~USBIE_PSOF;
}

void usbhw_int_dis_txdone(int epn)
{
	if (epn<16||epn>23) return;
	USBIEPIE&=~(1<<(epn-16));
}

void usbhw_int_dis_rxdone(int epn)
{
	if (epn<0||epn>7) return;
	USBOEPIE&=~(1<<epn);
}

void usbhw_int_dis_setup(void)
{
	USBIE&=~USBIE_SETUP;
}

void usbhw_int_dis_ctlin(void)
{
	USBICNF0&=~USBICNF0_INTE;
	USBIEPIE&=~1;
}

void usbhw_int_dis_ctlout(void)
{
	USBOCNF0&=~USBOCNF0_INTE;
	USBOEPIE&=~1;
}

int usbhw_get_setup(usb_setup_t *s)
{
	u8 b;

	USBCTL|=USBCTL_SETUP;
	// set both NAK bits now, in case of early STATUS
	USBOCT0=USBOCT0_NAK;
	USBICT0=USBICT0_NAK;

	b=USBBUFSETUP(0);
	s->dataDir=b&0x80?1:0;
	s->type=(b>>5)&3;
	s->recipient=b&31;
	s->request=USBBUFSETUP(1);
	s->value=USBBUFSETUP(3)<<8|USBBUFSETUP(2);
	s->index=USBBUFSETUP(5)<<8|USBBUFSETUP(4);
	s->len=USBBUFSETUP(7)<<8|USBBUFSETUP(6);
	USBIF=USBIF_SETUP|USBIF_STPOW;
	if (s->dataDir)
		USBCTL|=USBCTL_DIR;
	else {
		if (s->len) {
			USBCTL&=~USBCTL_DIR;
			USBOCT0=0;
		} else {
			//USBCTL|=USBCTL_DIR;
		}
	}
	return 0;
}

void usbhw_put_ctl_read_data(u8 len, usb_data_t *d)
{
	int i;
	
	for (i=0;i<len;++i) {
		if (i&1)
			USBBUFIN0(i)=d[i>>1];
		else
			USBBUFIN0(i)=d[i>>1]>>8;
	}
	USBICT0=len;
}

int usbhw_get_ctl_write_data(u8 *len, usb_data_t *d, int last)
{
	int i;

	if (!*len) {
		USBOCT0=0;
		return 0;
	}
	if (*len>USBOCT0&USBOCT0_COUNT) {
		return -1;
	}
	*len=USBOCT0&USBOCT0_COUNT;
	for (i=0;i<*len;++i) {
		if (i&1)
			d[i>>1]|=USBBUFOUT0(i);
		else
			d[i>>1]=USBBUFOUT0(i)<<8;
	}
	if (last) {
		USBCTL|=USBCTL_DIR;
	} else {
		USBOCT0&=~USBOCT0_NAK;
	}
	return 0;
}

void usbhw_ctl_write_handshake(void)
{
	USBOCT0=USBOCT0_NAK;
	USBICT0=0;
	USBCTL|=USBCTL_DIR;
}

void usbhw_ctl_read_handshake(void)
{
	USBICT0=USBICT0_NAK;
	USBOCT0=0;
	USBCTL&=~USBCTL_DIR;
}

#if 0
static void epReload(int epn, usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
	//ep->data->reload=buf;
	usb_dma_set_rld_ptr(epn,data);
	USBODRSZ(epn)=len;
	USBODCTL(epn)|=USBODCTL_RLD;
}

static void isrOUT(int epn)
{
	usb_endpoint_t *ep;
	usb_buffer_t buf;
	u16 dctl;
	
	ep=usb_get_ep(usb_get_config(),epn);
	if (!ep) return;
	if (epn>15) epn-=8;
	dctl=USBODCTL(epn);
	if (!(dctl&USBODCTL_GO)) {
		// no transfers, so start one
		//buf=ep->data->cb.o(epn,0);
		//if (!buf) return; // oops, no buffer, oh well
		//epGo(epn,ep,buf);
	}
}
#endif

void usbhw_cancel(usb_endpoint_t *ep)
{
	int epn=ep->id;

	if (epn>15) epn-=8;
	((usb_packet_req_t *)(ep->data->hwdata))->done=1;
	if (USBIDCTL(epn)&USBIDCTL_GO) {
		USBIDCTL(epn)|=USBIDCTL_STP;
		USBIDCTL(epn)&=~USBIDCTL_RLD;
	}
	USBICTX(epn)|=USBICTX_NAK;
	USBICTY(epn)|=USBICTY_NAK;
}

// works for out and in both
static void dmaGo(int epn, u32 data, u16 len)
{
	if (epn>15) epn-=8;
	//usb_dma_set_ptr(epn,data);
	USBODADL(epn)=data&0xffff;
	USBODADH(epn)=(data>>16)&0xff;
	/* this should not even work, but in fact it's required */
	USBODCT(epn)=0;
	USBODSIZ(epn)=len;
	USBODCTL(epn)=USBODCTL_GO|USBODCTL_OVF|USBODCTL_END;
}

#define RXTX {\
	usb_packet_req_t *pkt=(usb_packet_req_t *)(pkt->ep->data->hwdata);\
\
	if (!pkt->done) return -1;\
	pkt->ep=ep;\
	pkt->data=data;\
	pkt->reqlen=len;\
	pkt->actlen=0;\
	pkt->done=0;\
	dmaGo(ep->id,(u32)(data)<<1,len);\
	return 0;\
}

int usbhw_tx(usb_endpoint_t *ep, usb_data_t *data, u16 len)
RXTX

int usbhw_rx(usb_endpoint_t *ep, usb_data_t *data, u16 len)
RXTX

#undef RXTX

static void isrDMA(int epn, int rld)
{
	usb_endpoint_t *ep;
	usb_packet_req_t *pkt;

	ep=usb_get_ep(usb_get_config(),epn);
	if (!ep) return;
	pkt=(usb_packet_req_t *)(ep->data->hwdata);
	pkt->done=1;
	//txpkt->actlen=txpkt->reqlen;
	if (epn>15) epn-=8;
	ep->actlen+=USBODCT(epn);
	usb_evt_cpdone(ep);
}

void usbhw_isr(void)
{
	u8 src=USBINTSRC;
	
	if (!src) return;
	if (src>=0x4f) return;
	//usb_lock();
	//HWI_enable();
	if (src<0x12) { // bus interrupts
		switch(src) {
		case 2: // OUT0
			usb_evt_ctl_rx();
			break;
		case 4: // IN0
			usb_evt_ctl_tx();
			break;
		case 6: // bus reset
			usb_evt_reset();
			break;
		case 8: // bus suspend
			usb_evt_suspend();
			break;
		case 0xA: // bus resume
			usb_evt_resume();
			break;
		case 0xC: // setup
			usb_evt_setup();
			break;
		case 0xE: // setup overwrite
			//isrSetup();
			return;
		case 0x10: // SOF
			usb_evt_sof();
			break;
		case 0x11: // pre-SOF
			usb_evt_presof();
			break;
		default: // spurious / unknown ..
			//usb_unlock();
			return;
		}
	} 
#if 0
	else if (src<0x2E) { // endpoint interrupt
		if (src&1) { // spurious
			//usb_unlock();
			return;
		}
		src=(src-0x10)>>1; // src is now the endpoint number
		if (!(src&8)) isrOUT(src); // no tx interrupt
	} else 
#endif
	if (src>=0x32&&src<0x4f) { // dma
		//if (src<0x32) {
			//usb_unlock();
		//	return; // spurious
		//}
		src-=0x30;
		if (src&1) { // go
			src>>=1;
			if (src<8)
				isrDMA(src,0);
			else
				isrDMA(src+16,0);
		}
#if 0
		else { // reload
			src>>=1;
			if (src<8) { ; //isrDMAOUT(src,1);
			}
			else isrDMA(src+8,1);
		}
#endif
	}
	//usb_unlock();
}

void usbhw_stall(int epn)
{
	if (epn>15) epn-=8;
	if (!(USBICNF(epn)&USBICNF_ISO))
		USBICNF(epn)|=USBICNF_STALL;
}

void usbhw_unstall(int epn)
{
	if (epn>15) epn-=8;
	if (!(USBICNF(epn)&USBICNF_ISO))
		USBICNF(epn)&=~USBICNF_STALL;
}

int usbhw_is_stalled(int epn)
{
	if (epn>15) epn-=8;
	if (USBICNF(epn)&USBICNF_ISO)
		return 0;
	else
		return USBICNF(epn)&USBICNF_STALL;
}

void usbhw_ctl_stall(void)
{
	USBOCNF0|=USBOCNF0_STALL;
	USBICNF0|=USBICNF0_STALL;
}

void usbhw_ctl_unstall(void)
{
	USBOCNF0&=~USBOCNF0_STALL;
	USBICNF0&=~USBICNF0_STALL;
}

int usbhw_ctl_is_stalled(void)
{
	return (USBOCNF0&USBOCNF0_STALL)||(USBICNF0&USBICNF0_STALL);
}

void usbhw_set_address(u8 adr)
{
	USBADDR=adr;
}

// returns the buffer base adr assigned to the given endpoint
// the beginning of the X buffer is returned.  Y is assumed to 
// be the same size and starts after the X buffer.
static u16 usbhw_get_ep_buf_ofs(u8 epn)
{
	if (epn>=16)
		return (((u16)(USBIBAX(epn-8))<<4)-0x80);
	else
		return (((u16)(USBOBAX(epn))<<4)-0x80);
}

static void usbhw_set_ep_buf_ofs(u8 epn, u16 ofs)
{
	if (epn>=16)
		USBIBAX(epn-8)=(u8)((ofs-0x80)>>4);
	else
		USBOBAX(epn)=(u8)((ofs-0x80)>>4);
}

// returns -1 if it runs out of space
static int usbhw_alloc_ep_buf(int config, usb_endpoint_t *ep)
{
	int i;
	usb_endpoint_t *oldep;
	u16 ofs,b;

	ofs=0;
	oldep=0;
	for (i=1;i<16;++i) {
		if (i==8) continue;
		b=usbhw_get_ep_buf_ofs(i);
		if (b>ofs) {
			oldep=usb_get_ep(config,i);
			if (oldep)
				ofs=b;
			else
				oldep=0;
		}
	}
	if (oldep)
		ofs+=oldep->packetSize*2;
	if (ofs+(ep->packetSize*2)>3584)
		return -1;
	usbhw_set_ep_buf_ofs(ep->id,ofs);
	return 0;
}

extern Int auxheap;

int usbhw_activate_ep(usb_endpoint_t *ep)
{
	int epn=ep->id;
	u8 cnf=0;
	usb_packet_req_t *pkt;

	if (epn>15) epn-=8;
	pkt=(usb_packet_req_t *)MEM_alloc(auxheap,sizeof(usb_packet_req_t),0);
	if (!pkt) {
		USBICNF(epn)=0;
		return -1;
	}
	ep->data->hwdata=(void *)pkt;
	pkt->done=1;
	if (usbhw_alloc_ep_buf(usb_get_config(),ep)) {
		USBICNF(epn)=0;
		return -1;
	}
	USBISIZ(epn)=ep->packetSize&0x7f;
	if (ep->type==USB_EPTYPE_ISOCHRONOUS) {
		cnf|=USBICNF_ISO;
		cnf|=(ep->packetSize>>7)&7;
	} else {
		cnf|=USBICNF_DBUF;
	}
	USBICNF(epn)=cnf;
	USBICTX(epn)=USBICTX_NAK;
	USBICTY(epn)=USBICTY_NAK;
	USBIDCTL(epn)=0;
	USBIDADH(epn)=0;
	USBIDADL(epn)=0;
	USBIDSIZ(epn)=0;
	USBICTX(epn)=0;
	USBICTY(epn)=0;
	USBICNF(epn)|=USBICNF_UBME;
	if (epn>7) {
		//USBIEPIE|=1<<(epn-16);
		USBIDIE|=1<<(epn-8);
	} else {
		//USBOEPIE|=1<<epn;
		USBODIE|=1<<epn;
	}
	return 0;
}

void usbhw_deactivate_ep(usb_endpoint_t *ep)
{
	USBICNF(ep->id)=0;
	if (ep->data->hwdata) free(ep->data->hwdata);
}

//### TODO: support APLL on 5507 / 5509A
static void usbhw_init_pll(void)
{
	int mult=48/(GBL_getClkIn()/1000);

	if (mult>31) mult=31;
	USBPLL=0x2012|(mult<<7); //### FIXME: can't do fractional mults ..
	while (!(USBPLL&1)); // wait for lock
}

void usbhw_reset(void)
{
	// set up interrupts & control endpoints
	USBOEPIE|=1;
	USBIEPIE|=1;
	// now we can respond to everything ..
	USBIE=USBIE_RSTR|USBIE_SUSR|USBIE_RESR|USBIE_SETUP;
	USBOCNF0|=USBOCNF0_UBME|USBOCNF0_INTE;
	USBICNF0|=USBICNF0_UBME|USBICNF0_INTE;

	/* note: since we use FRSTE=1 we know the USB module is already (mostly) 
	at power-up values, so we don't need to reset addresses etc etc */
}

void usbhw_attach(void)
{
	USBIDLECTL=USBIDLECTL_USBRST; // un-reset the USB module
	// in the POWERED state, we must respond only to 
	// reset, suspend, and resume
	usbhw_int_en();
	USBCTL|=USBCTL_FEN;
	USBIE=USBIE_RSTR|USBIE_SUSR|USBIE_RESR|USBIE_SETUP;
	USBCTL|=USBCTL_CONN;
}

void usbhw_detach(void)
{
	// reset the module completely and keep it there
	// (this also disconnects us)
	USBIDLECTL&=~USBIDLECTL_USBRST;
	C55_disableIER0(C55_IEN08);
}

int usbhw_init(void *param)
{
	usbhw_init_pll();
	return 0;
}
