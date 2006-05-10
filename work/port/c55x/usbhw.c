
#include "usbhw.h"
#include <std.h>
#include <c55.h>
#include <usb_5509.h>
#include <mem.h>
#include <sem.h>
#include <clk.h>
#include <stdlib.h>

//#include "libmmb0/ui.h"

static u16 USBHW_DMA_LOGSIZE;
volatile usbhw_dmalog_t *usbhw_dmalog;
static int usbhw_dmalog_index=0;

void usbhw_dmalog_write(u8 id, u8 epn)
{
	usbhw_dmalog_t l;

	if (usbhw_dmalog_index>=USBHW_DMA_LOGSIZE)
		return;
	l.ep=epn;
	if (epn>16) epn-=8;
	l.nakx=(USBOCTX(epn)&USBOCTX_NAK)?1:0;
	l.naky=(USBOCTY(epn)&USBOCTY_NAK)?1:0;
	l.toggle=(USBICNF(epn)&USBICNF_TOGGLE)?1:0;
	l.dmago=(USBODCTL(epn)&USBODCTL_GO)?1:0;
	l.dmastp=(USBODCTL(epn)&USBODCTL_STP)?1:0;
	l.dmaovf=(USBODCTL(epn)&USBODCTL_OVF)?1:0;
	l.dmacount=USBODCT(epn);
	l.id=id;
	usbhw_dmalog[usbhw_dmalog_index++]=l;
}

#if 0
#define pkt_from_pkt(P) ((usb_packet_req_t *)((P)->ep->data->hwdata))

typedef struct usb_packet_req_t {
	unsigned int done:1, // 0 = request waiting, 1 = request complete
		timeout:1; // 0 = no timeout, 1 = timeout occured
	usb_endpoint_t *ep; // ptr to endpoint
	usb_data_t *data; // ptr to buffer
	u16 reqlen; // requested length in bytes
	u16 actlen; // actual length in bytes
} usb_packet_req_t;
#endif

struct c55x_params params;

static void *sys_malloc(u32 len)
{
	return MEM_alloc(0,len,0);
}

static void sys_free(void *mem)
{
	MEM_free(0,mem,0);
}

/* timeouts -------------------- */

static void update_check_time(usb_endpoint_t *ep)
{
	ep->data->check_time=CLK_getltime();
}

static u32 ticks_to_ms(u32 ticks)
{
	return params.us_per_prd_tick*ticks/1000;
}

static u32 ms_to_ticks(u32 ms)
{
	return (ms*1000)/params.us_per_prd_tick;
}

//static u8 lastled;

void usbhw_check_timeouts(void)
{
	usb_endpoint_t *ep;
	u32 curtime=CLK_getltime();

	ep=usb_get_first_ep(usb_get_config());
	while (ep) {
		if (ep->data->stat==USB_EPSTAT_XFER) {
			if (ticks_to_ms(curtime-ep->data->check_time)>ep->data->timeout)
				usb_evt_timeout(ep);
		}
		ep=ep->next;
	}
#if 0
	++lastled;
	if (lastled>99) lastled=0;
	if (!(lastled%10))
		led_showdig(lastled/10);
#endif
}

/* alarm clock ----------------- */

usb_alarm_t *usbhw_mkalarm(void)
{
	volatile usb_alarm_t *alm;

	alm=(usb_alarm_t *)SEM_create(0,NULL);
	return (usb_alarm_t *)alm;
}

void usbhw_rmalarm(usb_alarm_t *alarm)
{
	SEM_post((SEM_Handle)alarm);
	SEM_delete((SEM_Handle)alarm);
}

int usbhw_sleep(usb_alarm_t *alarm, int timeout_ms)
{
	if (!timeout_ms) {
		SEM_pend((SEM_Handle)alarm,SYS_FOREVER);
	} else {
		if (!SEM_pend((SEM_Handle)alarm,ms_to_ticks(timeout_ms)))
			return -1;
	}
	return 0;
}

void usbhw_wake(usb_alarm_t *alarm)
{
	SEM_post((SEM_Handle)alarm);
}

/* ------------------------------- */

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
	USBIEPIE|=(1<<(epn-8));
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

// ---------------------------------------------------------------------

static void showtoggle(void)
{
	if (USBICNF(9)&USBICNF_TOGGLE)
		led_showdig(1);
	else
		led_showdig(0);
}

void usbhw_cancel(usb_endpoint_t *ep)
{
	int epn=ep->id;

	if (epn>15) epn-=8;
	//((usb_packet_req_t *)(ep->data->hwdata))->done=1;
	if (USBIDCTL(epn)&USBIDCTL_GO) {
		USBIDCTL(epn)|=USBIDCTL_STP;
		USBIDCTL(epn)&=~USBIDCTL_RLD;
	}
	// if only one buffer has NAK off, then probably TOGGLE is wrong
	// and needs to be inverted manually
	//showtoggle();
#if 0
	if (ep->type!=USB_EPTYPE_ISOCHRONOUS) {
		if ((USBICTX(epn)&USBICTX_NAK)^(USBICTY(epn)&USBICTY_NAK)) {
			USBICNF(epn)^=USBICNF_TOGGLE;
		}
	}
	showtoggle();
#endif
	//USBICTX(epn)|=USBICTX_NAK;
	//USBICTY(epn)|=USBICTY_NAK;
}

// works for out and in both
static void dmaGo(int epn, u32 data, u16 len)
{
	if (epn==17) usbhw_dmalog_write(11,epn);
	if (epn>15) epn-=8;
	//usb_dma_set_ptr(epn,data);
	USBODADL(epn)=data&0xffff;
	USBODADH(epn)=(data>>16)&0xff;
	/* this should not even work, but in fact it's required */
	USBODCT(epn)=0;
	USBODSIZ(epn)=len;
	//USBOCTX(epn)=USBOCTX_NAK|len;
	//USBOCTY(epn)=USBOCTY_NAK|len;
	USBODCTL(epn)=USBODCTL_GO|USBODCTL_OVF|USBODCTL_END|USBODCTL_SHT;
	if (epn==9) usbhw_dmalog_write(12,epn+8);
	//showtoggle();
}

#define RXTX {\
	usb_packet_req_t *pkt=(usb_packet_req_t *)(ep->data->hwdata);\
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
{
/*	usb_packet_req_t *pkt=(usb_packet_req_t *)(ep->data->hwdata);

	if (!pkt->done) return -1;
	pkt->ep=ep;
	pkt->data=data;
	pkt->reqlen=len;
	pkt->actlen=0;
	pkt->done=0; */
	update_check_time(ep);
	dmaGo(ep->id,(u32)(data)<<1,len);
	return 0;
}

int usbhw_rx(usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
/*	usb_packet_req_t *pkt=(usb_packet_req_t *)(ep->data->hwdata);

	if (!pkt->done) return -1;
	pkt->ep=ep;
	pkt->data=data;
	pkt->reqlen=len;
	pkt->actlen=0;
	pkt->done=0; */
	update_check_time(ep);
	dmaGo(ep->id,(u32)(data)<<1,len);
	return 0;
}

#undef RXTX

static void isrDMA(int epn, int rld)
{
	usb_endpoint_t *ep;
	//volatile u16 x,y;
	//usb_packet_req_t *pkt;

	ep=usb_get_ep(usb_get_config(),epn);
	if (!ep) return;
	//usbhw_dmalog_write(USBHW_DMALOG_ISRDMA,epn);
	// we get an interrupt even on timeout, so we need to check this
	// can't use timed_out because sometimes it gets cleared before 
	// we get the interrupt
	if (ep->data->stat!=USB_EPSTAT_XFER) return;
	update_check_time(ep);
	//pkt=(usb_packet_req_t *)(ep->data->hwdata);
	//pkt->done=1;
	//txpkt->actlen=txpkt->reqlen;
	if (epn>15) epn-=8;
	ep->data->actlen+=USBODCT(epn);
	//x=USBISIZ(epn);
	//y=USBICTY(epn);
	usb_evt_cpdone(ep);
	showtoggle();
}

// all we do here is update the timeout
static void isrEP(int epn)
{
	usb_endpoint_t *ep;

	if (epn>8) epn+=8;
	ep=usb_get_ep(usb_get_config(),epn);
	if (!ep) return;
	update_check_time(ep);
}

interrupt void usbhw_isr(void)
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
	else if (src<0x2E) { // endpoint interrupt
		if (src&1) { // spurious
			//usb_unlock();
			return;
		}
		isrEP((src-0x10)>>1);
	} else if (src>=0x32&&src<0x4f) { // dma
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
				isrDMA(src+8,0);
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

/* Returns the buffer base adr assigned to the given endpoint, 
relative to the base address of the USB module.  The address is 
a byte address.  The first usable address is 0x80 since the 
data buffers start there (the DMA registers are in 0-0x80).

The beginning of the X buffer is returned.  Y is the same size 
and starts immediately after the X buffer.
*/
static u16 usbhw_get_ep_buf_ofs(u8 epn)
{
	volatile u16 b;

	if (epn>=16)
		return ((u16)(USBIBAX(epn-8)))<<4;
	else
		b=((u16)(USBOBAX(epn)))<<4;
	return b;
}

/* Sets the buffer base addresses and sizes for the X and Y buffers for 
the given endpoint.  The address is a byte address and is relative 
to the base address of the USB module.  The first usable address 
is therefore 0x80.

Packet size is taken from ep->packetSize.  If the endpoint is isochronous, 
SIZH is written; otherwise only SIZ is written.
*/
static void set_ep_hwbuf(usb_endpoint_t *ep, u16 ofs)
{
	int epn=ep->id;

	if (epn>=16) epn-=8;
	USBIBAX(epn)=(u8)(ofs>>4);
	USBIBAY(epn)=(u8)((ofs+64)>>4);
	USBISIZ(epn)=ep->packetSize&0x7f;
	if (ep->type==USB_EPTYPE_ISOCHRONOUS) {
		if (epn<8)
			USBOCNF(epn)=(USBOCNF(epn)&0xf8)|((ep->packetSize>>7)&7);
		else
			USBOCNF(epn)=(ep->packetSize>>7)&7;
	}
}

static int alloc_ep_buffers(int conf)
{
	u16 ofs;
	usb_endpoint_t *ep;

	ofs=0x80;
	ep=usb_get_first_ep(conf);
	while (ep) {
		if (ofs+ep->packetSize*2>=0xe80)
			return -1;
		set_ep_hwbuf(ep,ofs);
		ofs+=ep->packetSize*2;
		ep=ep->next;
	}
	return 0;
}

static int activate_ep(usb_endpoint_t *ep)
{
	int epn=ep->id;
	//usb_packet_req_t *pkt;

	if (epn>15) epn-=8;
	USBICNF(epn)=0;
	/*pkt=(usb_packet_req_t *)sys_malloc(sizeof(usb_packet_req_t));
	if (!pkt) return -1;
	ep->data->hwdata=(void *)pkt;
	pkt->done=1;*/
	if (ep->type==USB_EPTYPE_ISOCHRONOUS) {
		USBICNF(epn)|=USBICNF_ISO;
	} else {
		USBICNF(epn)|=USBICNF_DBUF;
	}
	if (epn>8) {
		USBICTX(epn)=USBICTX_NAK;
		USBICTY(epn)=USBICTY_NAK;
	} else {
		USBOCTX(epn)=0;
		USBOCTY(epn)=0;
	}
	USBIDCTL(epn)=0;
	USBIDADH(epn)=0;
	USBIDADL(epn)=0;
	USBIDSIZ(epn)=0;
	USBICNF(epn)|=USBICNF_UBME;
	if (epn>7) {
		USBIDIE|=1<<(epn-8);
	} else {
		USBODIE|=1<<epn;
	}
	return 0;
}

int usbhw_activate_eps(int cnf)
{
	usb_endpoint_t *ep;

	ep=usb_get_first_ep(cnf);
	while (ep) {
		if (activate_ep(ep))
			return -1;
		ep=ep->next;
	}
	if (alloc_ep_buffers(cnf))
		return -1;
	return 0;
}

void usbhw_deactivate_eps(int cnf)
{
	usb_endpoint_t *ep;

	ep=usb_get_first_ep(cnf);
	while (ep) {
		if (ep->id>16)
			USBICNF(ep->id-8)=0;
		else
			USBOCNF(ep->id)=0;
		if (ep->data->hwdata) {
			sys_free(ep->data->hwdata);
			ep->data->hwdata=0;
		}
		ep=ep->next;
	}
}

//### TODO: support APLL on 5507 / 5509A
static void usbhw_init_pll()
{
	int mult=48/(params.clkin_khz/1000);

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
	if (!param) return -1;
	params=*(struct c55x_params *)param;
	usbhw_init_pll();

	USBHW_DMA_LOGSIZE=2048;
	usbhw_dmalog=malloc(sizeof(usbhw_dmalog_t)*USBHW_DMA_LOGSIZE);

	return 0;
}
