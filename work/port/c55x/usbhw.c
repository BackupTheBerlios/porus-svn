
#include "usbhw.h"

static usb_packet_req_t txpkt, rxpkt;

void usbhw_lock(void)
{
	IRQ_disable(IRQ_EVT_USB);
}

void usbhw_unlock(void)
{
	IRQ_enable(IRQ_EVT_USB);
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
		USBCTL&=~USBCTL_DIR;
		USBOCT0=0;
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

int usbhw_get_ctl_write_data(u8 *len, usb_data_t *d)
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
	USBOCT0&=~USBOCT0_NAK;
}

void usbhw_ctl_write_handshake(void)
{
	USBICT0=0;
}

void usbhw_ctl_read_handshake(void)
{
	USBOCT0=0;
}

static void epGo(int epn, usb_endpoint_t *ep, u32 data, u16 len)
{
	//ep->data->go=data;
	usb_dma_set_ptr(epn,data);
	/* this should not even work, but in fact it's required */
	USBDMA(epn,USBODCT)=0;
	USBDMA(epn,USBODSIZ)=len;
	USBDMA(epn,USBODCTL)=USBODCTL_GO|USBODCTL_OVF|USBODCTL_END;
}

static void epReload(int epn, usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
	//ep->data->reload=buf;
	usb_dma_set_rld_ptr(epn,data);
	USBDMA(epn,USBODRSZ)=len;
	USBDMA(epn,USBODCTL)|=USBODCTL_RLD;
}

// ### FIXME: make isrOUT work again
static void isrOUT(int epn)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);
	usb_buffer_t buf;
	u16 dctl;
	
	if (!ep) return;

	dctl=USBDMA(epn,USBODCTL);
	if (!(dctl&USBODCTL_GO)) {
		// no transfers, so start one
		//buf=ep->data->cb.o(epn,0);
		//if (!buf) return; // oops, no buffer, oh well
		//epGo(epn,ep,buf);
	}
}

// ### FIXME: why is isrIN empty here?
static void isrIN(int epn)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return;
	//if (!inCallback) return;
	//if (ep->data->goBusy||ep->data->reloadBusy) return;

	// looks like we're all done, signal it
	//inCallback(epn,0,0);
}

void usb_tx_cancel(u8 epn)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (epn<9) return;
	if (!ep) return;
	ep->data->stop=1;
	if (USBDMA(epn,USBIDCTL)&USBIDCTL_GO) {
		USBDMA(epn,USBIDCTL)|=USBIDCTL_STP;
		USBDMA(epn,USBIDCTL)&=~USBIDCTL_RLD;
	}
	USBEPDEF(epn,USBICTX)|=USBICTX_NAK;
	USBEPDEF(epn,USBICTY)|=USBICTY_NAK;
}

static void nextDMAIN(u8 epn, usb_endpoint_t *ep, int rld)
{
	u32 data;
	u16 len;

	if (!ep->data->cb.i) return;
	len=ep->packetSize;
	if (ep->data->cb.i(epn,&data,&len)) {
		ep->data->buf=0;
		ep->data->bulkInProgress=0;
		if (ep->data->doneCallback)
			ep->data->doneCallback(epn);
		if (ep->data->lastlen>=ep->packetSize) {
			data=0;
			len=0;
		} else
			return;
	} else
		ep->data->bulkInProgress=1;
	ep->data->lastlen=len;
	/*
	if (rld)
		epReload(epn,ep,data,len);
	else
		epGo(epn,ep,data,len);
	*/
	epGo(epn,ep,data,len);
}

static void isrDMAIN(u8 epn, int rld)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return;

	if (!ep->data->stop)
		nextDMAIN(epn,ep,0);
}

int usbhw_tx(usb_packet_req_t *pkt)
{
	if (!txpkt.done) return -1;
	txpkt.ep=pkt.ep;
	txpkt.data=pkt.data;
	txpkt.reqlen=pkt.reqlen;
	txpkt.actlen=0;
	
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
	} else if (src<0x2E) { // endpoint interrupt
		if (src&1) { // spurious
			//usb_unlock();
			return;
		}
		src=(src-0x10)>>1; // src is now the endpoint number
		if (src&8) isrIN(src);
		else isrOUT(src);
	} else if (src<0x4f) { // dma
		if (src<0x32) {
			//usb_unlock();
			return; // spurious
		}
		src-=0x30;
		if (src&1) { // go
			src>>=1;
			// FIXME: make OUT DMA work again
			if (src<8) { ; //isrDMAOUT(src,0);
			}
			else isrDMAIN(src,0);
		} else {
			src>>=1;
			if (src<8) { ; //isrDMAOUT(src,1);
			}
			else isrDMAIN(src,1);
		}
	}
	//usb_unlock();
}

int usbhw_stall(int epn)
{
	if (USBICNF(epn)&USBICNF_ISO)
		return 0;
	else
		USBICNF(epn)|=USBICNF_STALL;
}

int usbhw_unstall(int epn)
{
	if (USBEPDEF(epnum,USBICNF)&USBICNF_ISO)
		return 0;
	else
		USBEPDEF(epnum,USBICNF)&=~USBICNF_STALL;
}

int usbhw_is_stalled(int epn)
{
	if (USBICNF(epn)&USBICNF_ISO)
		return 0;
	else
		return USBICNF(epn)&USBICNF_STALL;
}

int usbhw_ctl_stall(void)
{
	USBOCNF0|=USBOCNF0_STALL;
	USBICNF0|=USBICNF0_STALL;
}

int usbhw_ctl_unstall(void)
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
	if (epn>=8)
		return (((u16)(USBOBAX(epn))<<4)-0x80);
	else
		return (((u16)(USBIBAX(epn))<<4)-0x80);
}

static void usbhw_set_ep_buf_ofs(u8 epn, u16 ofs)
{
	if (epn>=8)
		USBOBAX(epn)=(u8)((ofs-0x80)>>4);
	else
		USBIBAX(epn)=(u8)((ofs-0x80)>>4);
}

// returns -1 if it runs out of space
static int usbhw_alloc_ep_buf(int config, usb_endpoint_t *ep)
{
	int i;
	usb_endpoint_t *oldep;
	u16 ofs,b;

	ofs=0;
	oldep=-1;
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
	usbhw_set_ep_buf_ofs(ofs);
	return 0;
}

int usbhw_activate_ep(usb_endpoint_t *ep)
{
	int epn=ep->id,config;
	u8 cnf=0;

	if (usbhw_alloc_ep_buf(config,ep)) {
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
	USBIEPIE|=1<<epn;
	USBIDIE|=1<<epn;
	return 0;
}

int usbhw_deactivate_ep(usb_endpoint_t *ep)
{
	USBICNF(ep->id)=0;
}

//### TODO: support APLL on 5507 / 5509A
static void usbhw_init_pll(unsigned int fin)
{
	int mult=48/fin;
	
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
	USBIE=USBIE_RSTR|USBIE_SUSR|USBIE_RESR|USBIE_SETUP|USBIE_STPOW;
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
	IRQ_enable(IRQ_EVT_USB);
	USBCTL|=USBCTL_FEN;
	USBIE=USBIE_RSTR|USBIE_SUSR|USBIE_RESR;
	USBCTL|=USBCTL_CONN;
}

void usbhw_detach(void)
{
	// reset the module completely and keep it there
	// (this also disconnects us)
	USBIDLECTL&=~USBIDLECTL_USBRST;
	IRQ_disable(IRQ_EVT_USB);
}

int usbhw_init(void *param)
{
	txpkt.done=1;
	rxpkt.done=1;
}