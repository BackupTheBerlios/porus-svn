
#include "usb.h"
#include "usb_5509.h"

static volatile struct {
	unsigned int suspended:1,
		state:3;
	u8 address;
	u8 config;
} flags;

static usb_cb_sof sofCB, preSOFCB;
static usb_cb_state stateChangeCallback;

int usb_ctl(usb_setup_t *setup);

void usb_cancel(u8 epn)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return;
	usbhw_cancel(epn);
	ep->data->xferInProgress=0;
}

int usb_tx(u8 epn, usb_data_t *data, u32 len)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);
	usb_packet_req_t pkt;

	if (!ep) return -1;
	if (len&&!data) return -2;

	if (ep->data->xferInProgress) return;
	ep->data->xferInProgress=1;
	ep->data->buf=data;
	ep->data->reqlen=len;
	ep->data->actlen=0;
	usbhw_int_dis_txdone(epn);
	usb_evt_txdone(ep);
	usbhw_int_en_txdone(epn);
	return 0;
}

void usb_evt_txdone(usb_endpoint_t *ep)
{
	usb_packet_req_t pkt;
	u32 l;

	if (!ep->data->xferInProgress) return;
	if (ep->data->actlen>=ep->data->reqlen) {
		ep->data->xferInProgress=0;
		return;
	}
	pkt.ep=ep;
	l=ep->data->reqlen-ep->data->actlen;
	if (l>ep->packetSize)
		l=ep->packetSize;
	pkt.data=ep->data->buf+usb_mem_len(l);
	usbhw_tx(&pkt);
	ep->data->actlen+=l;
}

// ### FIXME: make receive work

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

void usb_set_address(u8 adr)
{
	if (adr==flags.address) return;
#ifdef USB_DEFERRED_ADDRESS
	usb_set_state(USB_STATE_WILL_ADDRESS);
	flags.address=adr;
#else
	usbhw_set_address(adr);
	usb_set_state(USB_STATE_ADDRESS);
#endif
}

int usb_stall(u8 epn)
{
	if (epn>31) return -1;
	if (!(epn&15)) return -1;
	usbhw_stall(epn);
	return 0;
}

int usb_unstall(u8 epn)
{
	if (epn>31) return -1;
	if (!(epn&15) return -1;
	usbhw_unstall(epn);
	return 0;
}

int usb_is_stalled(u8 epn)
{
	u8 cnf;

	if (epn>31) return -1;
	if (!(epn&15)) return -1;
	return usbhw_is_stalled(epn);
}

int usb_set_config(int cfn)
{
	if (usb_get_state()!=USB_STATE_ADDRESS&&usb_get_state()!=USB_STATE_CONFIGURED)
		return -1;
	if (!cfn) {
		flags.config=0;
		usb_set_state(USB_STATE_ADDRESS);
	} else {
		if (!usb_have_config(cfn)) return -1;
		flags.config=cfn;
		if (setupEndpointRegs(cfn)) return -1;
		usb_set_state(USB_STATE_CONFIGURED);
	}
	return 0;
}

int usb_get_config(void)
{
	return flags.config;
}

int usb_get_state(void)
{
	if (flags.suspended) return USB_STATE_SUSPENDED;
	else return flags.state;
}

void usb_set_state(int state)
{
	if (!flags.suspended&&(state==flags.state)) return;
	if (state==USB_STATE_SUSPENDED) {
		if (flags.suspended) return;
		flags.suspended=1;
	} else {
		flags.suspended=0;
		flags.state=state;
	}
	if (stateChangeCallback) stateChangeCallback(state);
}

void usb_evt_reset(void)
{
	usb_set_state(USB_STATE_DEFAULT);
	// sof & presof interrupts only set if we have callbacks
	if (sofCB) USBIE|=USBIE_SOF;
	if (preSOFCB) USBIE|=USBIE_PSOF;

	/* note: since we use FRSTE=1 we know the USB module is already (mostly) 
	at power-up values, so we don't need to reset addresses etc etc */
}

void usb_evt_suspend(void)
{
	usb_set_state(USB_STATE_SUSPENDED);
}

void usb_evt_resume(void)
{
	if (!flags.suspended) return;
	flags.suspended=0;
	if (stateChangeCallback) stateChangeCallback(flags.state);
}

void usb_set_state_cb(usb_cb_state cb)
{
	stateChangeCallback=cb;
}

/* codes: -1: no such OUT ep */
int usb_set_out_cb(int cfg, int epn, usb_cb_out cb)
{
	usb_endpoint_t *ep=usb_get_ep(cfg,epn);
	usb_cb_out oldcb;

	if (!ep) return -1;
	if (epn>7) return -1;
	if (flags.state!=USB_STATE_CONFIGURED) {
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
	if (flags.state!=USB_STATE_CONFIGURED) {
		ep->data->cb.i=cb;
	} if (!cb) {
		USBIEPIE&=~(1<<(epn&7));
		ep->data->cb.o=0;
	} else {
		ep->data->cb.o=0; // ### FIXME: this is wrong!
		USBIEPIE|=1<<(epn&7);
	}
	return 0;
}

void usb_set_txdone_cb(int epn, usb_cb_done cb)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return;
	if (epn<9) return;
	ep->data->doneCallback=cb;
}

int usb_is_attached(void)
{
#ifndef USBHW_HAVE_ATTACH
	return 1;
#else
	return usb_get_state()!=USB_STATE_DETACHED;
#endif
}

void usb_attach(void)
{
#ifdef USBHW_HAVE_ATTACH
	if (usb_is_attached()) return;
	usbhw_attach();
	usb_set_state(USB_STATE_POWERED);
#endif
}

void usb_detach(void)
{
#ifdef USBHW_HAVE_ATTACH
	if (!usb_is_attached()) return;
	usb_set_state(USB_STATE_DETACHED);
#endif
}

void usb_init(void *param)
{
	int i;
	usb_endpoint_t *ep;

	flags.state=USB_STATE_DETACHED;
	flags.config=0;
	flags.suspended=0;
	flags.address=0;
	ctlflags.state=USB_CTL_STATE_IDLE;
	sofCB=preSOFCB=0;
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
	usbhw_init(param);
}
