
#include "usb.h"
#include "usb_5509.h"

static usb_cb_sof sofCB, preSOFCB;
static usb_cb_state stateChangeCallback;
static int usbState, usbSuspended;
static u8 usbAddress;
static u8 usbConfig; // config number

static usb_setup_t usbSetup;
static usb_data_t ctlRxBuf[usb_mem_len(USB_CTL_PACKET_SIZE)];
	// no IN buf, we copy data from the user

void usb_set_state_unsuspend(void);
void usb_set_state(int state);
void usb_set_address(u8 adr);
int usb_ctl(usb_setup_t *setup);

void usb_lock(void)
{
	IRQ_disable(IRQ_EVT_USB);
}

void usb_unlock(void)
{
	IRQ_enable(IRQ_EVT_USB);
}

static void usb_ctl_stall(void)
{
	USBICNF0|=USBICNF0_STALL;
	USBOCNF0|=USBOCNF0_STALL;
}

static void isrOUT0(void)
{
	int i;

	/*### FIXME need to check for bad (non-zero) OUT packet here at end of 
	  setup txn */
	// stall if there wasn't supposed to be a data phase, or if we 
	//   have no callback
	if (!usbSetup.len) {
		usb_ctl_stall();
		return;
	}

	// copy out the data
	usbSetup.datalen=USBOCT0&USBOCT0_COUNT;
	for (i=0;i<usbSetup.datalen;++i) {
		if (i&1)
			ctlRxBuf[i>>1]|=USBBUFOUT0(i);
		else
			ctlRxBuf[i>>1]=USBBUFOUT0(i)<<8;
	}
	USBOCT0&=~USBOCT0_NAK;
	if (usbSetup.datact) usbSetup.phase=USB_CTL_PHASE_OUT;
	usbSetup.datact+=usbSetup.datalen;
	if (usbSetup.datact>usbSetup.len) { // too much data
		usb_ctl_stall();
		return;
	}
	if (usbSetup.datact<USB_CTL_PACKET_SIZE) { // short packet, we're done
		USBICT0=0;
	}
	usbSetup.dataofs=0;
	usbSetup.data=ctlRxBuf;
	// go get the next ptr
	if (usb_ctl(&usbSetup)) usb_ctl_stall();
}

static void isrIN0(void)
{
	int i,l;

	if (!usbSetup.len||!usbSetup.data) {
		usb_ctl_stall();
		return;
	}

	l=usbSetup.datalen-usbSetup.dataofs;
	if (l>USB_CTL_PACKET_SIZE) l=USB_CTL_PACKET_SIZE;
	if (l>(usbSetup.len-usbSetup.datact)) l=usbSetup.len-usbSetup.datact;
	if (l<0) l=0;
	for (i=0;i<l;++i) {
		if ((i+usbSetup.dataofs)&1)
			USBBUFIN0(i)=usbSetup.data[(i+usbSetup.dataofs)>>1];
		else
			USBBUFIN0(i)=usbSetup.data[(i+usbSetup.dataofs)>>1]>>8;
	}
	USBICT0=l;
	usbSetup.datact+=l;
	if (!l||usbSetup.datact>=usbSetup.len) {
		USBOCT0=0; // we're done, now expect a handshake
		return;
	}
	if (usbSetup.datalen==USB_CTL_PACKET_SIZE) {
		usbSetup.phase=USB_CTL_PHASE_IN;
		if (usb_ctl(&usbSetup)) usb_ctl_stall();
	} else {
		usbSetup.dataofs+=l;
	}
}

static void isrSetup(void)
{
	u8 b;

	USBCTL|=USBCTL_SETUP;
	b=USBBUFSETUP(0);
	usbSetup.dataDir=b&0x80?1:0;
	usbSetup.type=(b>>5)&3;
	usbSetup.recipient=b&31;
	usbSetup.request=USBBUFSETUP(1);
	usbSetup.value=USBBUFSETUP(3)<<8|USBBUFSETUP(2);
	usbSetup.index=USBBUFSETUP(5)<<8|USBBUFSETUP(4);
	usbSetup.len=USBBUFSETUP(7)<<8|USBBUFSETUP(6);
	USBIF=USBIF_SETUP|USBIF_STPOW;
	usbSetup.datact=0;
	usbSetup.datalen=0;
	usbSetup.data=0;
	usbSetup.dataofs=0;
	usbSetup.phase=USB_CTL_PHASE_SETUP;

	// set both NAK bits in case of early STATUS
	USBOCT0=USBOCT0_NAK;
	USBICT0=USBICT0_NAK;

	if (usbSetup.dataDir) { 		// we have a read txn
		USBCTL|=USBCTL_DIR;		// there will be an IN pkt
		usbSetup.datalen=usbSetup.len;	// set up datalen, clamp to max packet size
		if (usbSetup.datalen>USB_CTL_PACKET_SIZE)
			usbSetup.datalen=USB_CTL_PACKET_SIZE;
		if (usb_ctl(&usbSetup)) {
			usb_ctl_stall();
			return;
		}
		isrIN0(); // fake IN interrupt, this sets up the endpoint as needed
	} else {				// it's a write txn
		USBCTL&=~USBCTL_DIR;		// and we are expecting an OUT packet
		if (usbSetup.len) {
			usbSetup.data=ctlRxBuf;
			USBOCT0=0; // now allow OUTs
			// no dispatch, wait for OUT
		} else {		// not expecting data
			/* we do this first so we can go to address state if nec.
			   also we should keep naking until we have something ready ... */
			if (usb_ctl(&usbSetup)) usb_ctl_stall(); // dispatch now
			USBICT0=0;	// expecting an IN handshake
		}
	}
}

static void usb_reset(void)
{
	usb_set_state(USB_STATE_DEFAULT);
	// set up interrupts & control endpoints
	USBOEPIE|=1;
	USBIEPIE|=1;
	// now we can respond to everything ..
	USBIE=USBIE_RSTR|USBIE_SUSR|USBIE_RESR|USBIE_SETUP|USBIE_STPOW;
	USBOCNF0|=USBOCNF0_UBME|USBOCNF0_INTE;
	USBICNF0|=USBICNF0_UBME|USBICNF0_INTE;
	// sof & presof interrupts only set if we have callbacks
	if (sofCB) USBIE|=USBIE_SOF;
	if (preSOFCB) USBIE|=USBIE_PSOF;

	/* note: since we use FRSTE=1 we know the USB module is already (mostly) 
	at power-up values, so we don't need to reset addresses etc etc */
}

static void usb_suspend(void)
{
	usb_set_state(USB_STATE_SUSPENDED);
}

static void usb_resume(void)
{
	usb_set_state_unsuspend();
}

#if 0
int usb_is_buf_idle(int epn, usb_data_t *buf)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

	if (!ep) return -1;
	if (epn>8) {
		if (ep->data->go!=buf) return 1;
		//if (usb_buf_data(buf)!=usb_dma_ptr(epn)) return 1;
	} else {
		if (buf!=usb_dma_ptr(epn)) return 1;
	}
	return (!(USBDMA(epn,USBODCTL)&USBODCTL_GO));
}
#endif

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

// FIXME: make isrOUT work again
static void isrOUT(int epn)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);
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

static void isrIN(int epn)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

	if (!ep) return;
	//if (!inCallback) return;
	//if (ep->data->goBusy||ep->data->reloadBusy) return;

	// looks like we're all done, signal it
	//inCallback(epn,0,0);
}

void usb_tx_cancel(u8 epn)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

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

void usb_bulk_tx_cancel(u8 epn)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);
	usb_endpoint_data_t *epd;

	if (!ep) return;
	if (epn<9) return;
	if (!ep->data->bulkInProgress) return;

	usb_tx_cancel(epn);
	epd=ep->data;
	epd->buf=0;
	epd->buflen=0;
}

// len==0 means the transfer is done. return 0 in this case.
// 0  = ok
// -1 = no more data
// -2 = invalid endpoint
// -3 = bad parameters
static int usb_bulk_in(int epn, u32 *buf, u16 *len)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

	if (!ep) return -2;
	if (epn<9) return -2;

	if (!len) {
		ep->data->bulkInProgress=0;
		return 0;
	}
	if (!buf) return -3;
	if (ep->data->count>=ep->data->buflen) {
		*buf=0;
		ep->data->buf=0;
		return -1;
	}
	*buf=ep->data->buf+(ep->data->count);
	*len=ep->data->buflen;
	if (*len>(ep->data->buflen-ep->data->count))
		*len=ep->data->buflen-ep->data->count;
	ep->data->count+=*len;
	return 0;
}

/*
static void usb_tx_start(u8 epn, usb_endpoint_t *ep, u16 len)
{
	if (ep->data->rldBuf) {
		ep->data->buf=ep->data->rldBuf;
		ep->data->buflen=ep->data->rldBuflen;
		ep->data->rldBuf=0;
		ep->data->rldBuflen=0;
		ep->data->count=0;
		ep->data->bulkInProgress=1;
		epGo(epn,ep,ep->data->buf,len);
	} else {
		ep->data->buf=0;
		ep->data->bulkInProgress=0;
	}
}
*/

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
	}
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
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

	if (!ep) return;

	if (!ep->data->stop)
		nextDMAIN(epn,ep,0);
}

//! Point the endpoint to a new memory block
/*!
*/
void usb_bulk_tx(u8 epn, u32 data, u32 len)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

	if (!ep) return;
	if (epn<9) return;
	
	if (ep->data->bulkInProgress)
		usb_tx_cancel(epn);
	ep->data->buf=data;
	ep->data->buflen=len;
	ep->data->count=0;
	nextDMAIN(epn,ep,0);
	//if (ep->data->lastlen>=ep->packetSize)
	//	nextDMAIN(epn,ep,1);
}

// FIXME: make OUT DMA work
/*
static void isrDMAOUTRLD(int epn)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);
	usb_buffer_t buf;

	if (!ep) return;

	buf=ep->data->cb.o(epn,ep->data->go); // a go did complete ...
	ep->data->go=ep->data->reload;
	if (!buf) return;
	epReload(epn,ep,buf);
}


static void isrDMAOUT(u8 epn, int rld)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);
	usb_buffer_t buf;

	// TODO: merge these
	if (rld) {
		//isrDMAOUTRLD(epn);
		return;
	}
	if (!ep) return;

	if (!ep->data->cb.o) return;
	buf=ep->data->cb.o(epn,ep->data->go);
	if (!buf) return;
	epGo(epn,ep,buf);
}
*/

void usb_isr(void)
{
	u8 src=USBINTSRC;
	
	if (!src) return;
	if (src>=0x4f) return;
	usb_lock();
	HWI_enable();
	if (src<0x12) { // bus interrupts
		switch(src) {
		case 2: // OUT0
			isrOUT0();
			break;
		case 4: // IN0
			isrIN0();
			break;
		case 6: // bus reset
			usb_reset();
			break;
		case 8: // bus suspend
			usb_suspend();
			break;
		case 0xA: // bus resume
			usb_resume();
			break;
		case 0xC: // setup
			isrSetup();
			break;
		case 0xE: // setup overwrite
			isrSetup();
			break;
		case 0x10: // SOF
			sofCB();
			break;
		case 0x11: // pre-SOF
			preSOFCB();
			break;
		default: // spurious / unknown ..
			usb_unlock();
			return;
		}
	} else if (src<0x2E) { // endpoint interrupt
		if (src&1) { // spurious
			usb_unlock();
			return;
		}
		src=(src-0x10)>>1; // src is now the endpoint number
		if (src&8) isrIN(src);
		else isrOUT(src);
	} else if (src<0x4f) { // dma
		if (src<0x32) {
			usb_unlock();
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
	usb_unlock();
}

void usb_set_sof_cb(usb_cb_sof cb)
{
	if (!cb) {
		USBIE&=~USBIE_SOF;
		sofCB=cb;
	} else {
		sofCB=cb;
		USBIE|=USBIE_SOF;
	}
}

void usb_set_presof_cb(usb_cb_sof cb)
{
	if (!cb) {
		USBIE&=~USBIE_PSOF;
		preSOFCB=cb;
	} else {
		preSOFCB=cb;
		USBIE|=USBIE_PSOF;
	}
}

static int activateOutEp(int epn, usb_endpoint_t *ep)
{
	USBDMA(epn,USBODCTL)=0;
	USBDMA(epn,USBODADH)=0;
	USBDMA(epn,USBODADL)=0;
	USBDMA(epn,USBODSIZ)=0;
	USBEPDEF(epn,USBOCTX)=0;
	USBEPDEF(epn,USBOCTY)=0;
	USBEPDEF(epn,USBOCNF)|=USBOCNF_UBME;
	USBOEPIE|=1<<epn;
	USBODIE|=1<<epn;
	return 0;
}

static int activateInEp(int epn, usb_endpoint_t *ep)
{
	USBDMA(epn,USBIDCTL)=0;
	USBDMA(epn,USBIDADH)=0;
	USBDMA(epn,USBIDADL)=0;
	USBDMA(epn,USBIDSIZ)=0;
	USBEPDEF(epn,USBICTX)=0x80;
	USBEPDEF(epn,USBICTY)=0x80;
	USBEPDEF(epn,USBICNF)|=USBICNF_UBME;
	USBIEPIE|=1<<(epn&7);
	USBIDIE|=1<<(epn&7);
	return 0;
}

static int setupInEpRegs(int epn, usb_endpoint_t *ep, int base_adr)
{
	int cnf=0;

	USBEPDEF(epn,USBIBAX)=base_adr>>4;
	base_adr+=ep->packetSize;
	if (base_adr>=3584) return -1;
	USBEPDEF(epn,USBISIZ)=ep->packetSize&0x7f;
	USBEPDEF(epn,USBIBAY)=base_adr>>4;
	base_adr+=ep->packetSize;
	if (base_adr>=3584) return -1;
	USBEPDEF(epn,USBICTX)=USBICTX_NAK;
	USBEPDEF(epn,USBICTY)=USBICTY_NAK;
	if (ep->type==USB_EPTYPE_ISOCHRONOUS) {
		cnf|=USBICNF_ISO;
		USBEPDEF(epn,USBISIZH)=(ep->packetSize>>7)&7;
	} else {
		cnf|=USBICNF_DBUF;
	}
	USBEPDEF(epn,USBICNF)=cnf;

	return base_adr;
}

static int setupOutEpRegs(int epn, usb_endpoint_t *ep, int base_adr)
{
	int cnf=0;

	USBEPDEF(epn,USBOBAX)=base_adr>>4;
	base_adr+=ep->packetSize;
	if (base_adr>=3584) return -1;
	USBEPDEF(epn,USBOSIZ)=ep->packetSize&0x7f;
	USBEPDEF(epn,USBOBAY)=base_adr>>4;
	base_adr+=ep->packetSize;
	if (base_adr>=3584) return -1;
	USBEPDEF(epn,USBOCTX)=USBOCTX_NAK;
	USBEPDEF(epn,USBOCTY)=USBOCTY_NAK;
	if (ep->type==USB_EPTYPE_ISOCHRONOUS) {
		cnf|=USBOCNF_ISO;
		cnf|=(ep->packetSize>>7)&7;
	} else {
		cnf|=USBOCNF_DBUF;
	}
	USBEPDEF(epn,USBOCNF)=cnf;

	return base_adr;
}

static int setupEndpointRegs(int cfn)
{
	int i;
	usb_endpoint_t *ep;
	int next_buffer_adr=0;

	for (i=1;i<16;++i) {
		if (i==8) continue;
		ep=usb_get_ep(cfn,i);
		if (!ep) continue;
		if (i<8) {
			next_buffer_adr=setupOutEpRegs(i,ep,next_buffer_adr);
			if (ep->data->cb.o) activateOutEp(i,ep);
		} else {
			next_buffer_adr=setupInEpRegs(i,ep,next_buffer_adr);
			activateInEp(i,ep);
		}
		if (next_buffer_adr<0) return -1;
	}
	return 0;
}

int usb_set_config(int cfn)
{
	if (usb_get_state()!=USB_STATE_ADDRESS&&usb_get_state()!=USB_STATE_CONFIGURED)
		return -1;
	if (!cfn) {
		usbConfig=0;
		usb_set_state(USB_STATE_ADDRESS);
	} else {
		if (!usb_have_config(cfn)) return -1;
		usbConfig=cfn;
		if (setupEndpointRegs(cfn)) return -1;
		usb_set_state(USB_STATE_CONFIGURED);
	}
	return 0;
}

int usb_get_config(void)
{
	return usbConfig;
}

int usb_stall(u8 epnum)
{
	if (epnum>15) return -1;
	if (usb_is_stalled(epnum)) return 0;
	if (epnum==0)
		USBOCNF0|=USBOCNF0_STALL;
	else if (epnum==8)
		USBICNF0|=USBICNF0_STALL;
	else {
		if (USBEPDEF(epnum,USBICNF)&USBICNF_ISO)
			return 0;
		else
			USBEPDEF(epnum,USBICNF)|=USBICNF_STALL;
	}
	return 0;
}

int usb_unstall(u8 epnum)
{
	if (epnum>15) return -1;
	if (!usb_is_stalled(epnum)) return 0;
	if (epnum==0)
		USBOCNF0&=~USBOCNF0_STALL;
	else if (epnum==8)
		USBICNF0&=~USBICNF0_STALL;
	else {
		if (USBEPDEF(epnum,USBICNF)&USBICNF_ISO)
			return 0;
		else
			USBEPDEF(epnum,USBICNF)&=~USBICNF_STALL;
	}
	return 0;
}

int usb_is_stalled(u8 epnum)
{
	u8 cnf;

	if (epnum>15) return -1;
	if (epnum==0)
		return USBOCNF0&USBOCNF0_STALL;
	else if (epnum==8)
		return USBICNF0&USBICNF0_STALL;
	else {
		cnf=USBEPDEF(epnum,USBICNF);
		if (cnf&USBICNF_ISO) return 0;
		else return cnf&USBICNF_STALL;
	}
}

int usb_get_state(void)
{
	if (usbSuspended) return USB_STATE_SUSPENDED;
	else return usbState;
}

void usb_set_state(int state)
{
	if (!usbSuspended&&(state==usbState)) return;
	if (state==USB_STATE_SUSPENDED) {
		if (usbSuspended) return;
		usbSuspended=1;
	} else {
		usbSuspended=0;
		usbState=state;
	}
	if (stateChangeCallback) stateChangeCallback(state);
}

void usb_set_state_unsuspend(void)
{
	if (usbSuspended) return;
	usbSuspended=0;
}

void usb_set_state_cb(usb_cb_state cb)
{
	stateChangeCallback=cb;
}

void usb_set_address(u8 adr)
{
	if (adr==usbAddress) return;
#ifdef USB_DEFERRED_ADDRESS
	usb_set_state(USB_STATE_WILL_ADDRESS);
	usbAddress=adr;
#else
	USBADDR=adr;
	usb_set_state(USB_STATE_ADDRESS);
#endif
}

/* codes: -1: no such OUT ep */
int usb_set_out_cb(int cfg, int epn, usb_cb_out cb)
{
	usb_endpoint_t *ep=usb_get_ep(cfg,epn);
	usb_cb_out oldcb;

	if (!ep) return -1;
	if (epn>7) return -1;
	if (usbState!=USB_STATE_CONFIGURED) {
		ep->data->cb.o=cb;
	} else if (!cb) {
		USBOEPIE&=~(1<<epn);
		ep->data->cb.o=0;
	} else {
		oldcb=ep->data->cb.o;
		ep->data->cb.o=cb;
		if (!oldcb) activateOutEp(epn,ep);
	}
	return 0;
}

/* codes: -1: no such IN ep */
int usb_set_in_cb(int cfg, int epn, usb_cb_in cb)
{
	usb_endpoint_t *ep=usb_get_ep(cfg,epn);
	
	if (!ep) return -1;
	if (epn<9) return -1;
	if (usbState!=USB_STATE_CONFIGURED) {
		ep->data->cb.i=cb;
	} if (!cb) {
		USBIEPIE&=~(1<<(epn&7));
		ep->data->cb.o=0;
	} else {
		ep->data->cb.o=0; // FIXME: this is wrong!
		USBIEPIE|=1<<(epn&7);
	}
	return 0;
}

void usb_set_txdone_cb(int epn, usb_cb_done cb)
{
	usb_endpoint_t *ep=usb_get_ep(usbConfig,epn);

	if (!ep) return;
	if (epn<9) return;
	ep->data->doneCallback=cb;
}

void usb_dev_reset(void)
{
	//int i;
	//usb_endpoint_t *ep;

	usb_set_state(USB_STATE_DEFAULT);
}

void usb_attach(void)
{
	if (usb_is_attached()) return;
	USBIDLECTL=USBIDLECTL_USBRST; // un-reset the USB module
	// in the POWERED state, we must respond only to 
	// reset, suspend, and resume
	IRQ_enable(IRQ_EVT_USB);
	USBCTL|=USBCTL_FEN;
	USBIE=USBIE_RSTR|USBIE_SUSR|USBIE_RESR;
	usb_set_state(USB_STATE_POWERED);
	USBCTL|=USBCTL_CONN;
}

void usb_detach(void)
{
	if (!usb_is_attached()) return;
	// reset the module completely and keep it there
	// (this also disconnects us)
	USBIDLECTL&=~USBIDLECTL_USBRST;
	usb_set_state(USB_STATE_DETACHED);
	IRQ_disable(IRQ_EVT_USB);
}

int usb_is_attached(void)
{
	return usb_get_state()!=USB_STATE_DETACHED;
}

//### TODO: support APLL on 5507 / 5509A
static void usb_init_pll(unsigned int fin)
{
	int mult=48/fin;
	
	if (mult>31) mult=31;
	USBPLL=0x2012|(mult<<7); //### FIXME: can't do fractional mults ..
	while (!(USBPLL&1)); // wait for lock
}

void usb_init(void)
{
	int i;
	usb_endpoint_t *ep;

	usb_init_pll(12); //### FIXME: usb input freq is hard-coded
	usbState=USB_STATE_DETACHED;
	usbConfig=0;
	usbSuspended=0;
	usbAddress=0;
	sofCB=preSOFCB=0;
	usb_set_ctl_class_cb(0);
	usb_set_ctl_vendor_cb(0);
	usb_set_ctl_vendor_write_cb(0);
	usb_set_ctl_vendor_read_cb(0);
	stateChangeCallback=0;

	// ### TODO: need to do this for all configurations
	for (i=0;i<16;++i) {
		ep=usb_get_ep(1,i);
		if (!ep) continue;
		ep->data->go=0;
		ep->data->reload=0;
		if (i>8) {
			ep->data->cb.i=usb_bulk_in;
			ep->data->bulkInProgress=0;
			ep->data->stop=0;
			ep->data->buf=0;
			ep->data->buflen=0;
			ep->data->count=0;
			ep->data->doneCallback=0;
			ep->data->lastlen=0;
		}
	}
}
