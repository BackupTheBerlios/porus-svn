
#include "usbhw.h"

static volatile struct {
	unsigned int suspended:1,
		state:3;
	u8 address;
	u8 config;
} flags;

static usb_cb sofCB, preSOFCB;
static usb_cb_int stateChangeCallback;

void usb_cancel(u8 epn)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return;
	usbhw_cancel(ep);
	ep->data->xferInProgress=0;
}

int usb_tx(u8 epn, usb_data_t *data, u32 len)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return -1;
	if (len&&!data) return -2;

	if (ep->data->xferInProgress) return -3;
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

void usb_set_sof_cb(usb_cb cb)
{
	if (!cb) {
		usbhw_int_dis_sof();
		sofCB=cb;
	} else {
		sofCB=cb;
		usbhw_int_en_sof();
	}
}

void usb_set_presof_cb(usb_cb cb)
{
	if (!cb) {
		usbhw_int_dis_presof();
		preSOFCB=cb;
	} else {
		preSOFCB=cb;
		usbhw_int_en_presof();
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
	if (!(epn&15)) return -1;
	usbhw_unstall(epn);
	return 0;
}

int usb_is_stalled(u8 epn)
{
	if (epn>31) return -1;
	if (!(epn&15)) return -1;
	return usbhw_is_stalled(epn);
}

static int activate_endpoints(int config)
{
	int i;
	usb_endpoint_t *ep;

	for (i=1;i<32;++i) {
		ep=usb_get_ep(config,i);
		if (!ep) continue;
		if (usbhw_activate_ep(ep))
			return -1;
	}
	return 0;
}

static void deactivate_endpoints(void)
{
	int i, config;
	usb_endpoint_t *ep;

	config=usb_get_config();
	for (i=1;i<32;++i) {
		ep=usb_get_ep(config,i);
		if (!ep) continue;
		usbhw_deactivate_ep(ep);
	}
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
		if (activate_endpoints(cfn)) return -1;
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

void usb_evt_sof(void)
{
	if (sofCB) sofCB();
}

void usb_evt_presof(void)
{
	if (preSOFCB) preSOFCB();
}

void usb_evt_reset(void)
{
	deactivate_endpoints();

	usb_set_state(USB_STATE_DEFAULT);
	usbhw_reset();
	// sof & presof interrupts only set if we have callbacks
	if (sofCB) usbhw_int_en_sof();
	if (preSOFCB) usbhw_int_en_presof();

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

void usb_set_state_cb(usb_cb_int cb)
{
	stateChangeCallback=cb;
}


#if 0
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
	//ep->data->doneCallback=cb;
}
#endif

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
	usb_ctl_init();
	sofCB=preSOFCB=0;
	stateChangeCallback=0;

	// ### TODO: need to do this for all configurations
	for (i=0;i<31;++i) {
		ep=usb_get_ep(1,i);
		if (!ep) continue;
		//ep->data->go=0;
		//ep->data->reload=0;
		if (i>15) {
			//ep->data->cb.i=usb_bulk_in;
			ep->data->xferInProgress=0;
			//ep->data->stop=0;
			ep->data->buf=0;
			//ep->data->buflen=0;
			//ep->data->count=0;
			//ep->data->doneCallback=0;
			//ep->data->lastlen=0;
		}
	}
	usbhw_init(param);
	usbhw_int_en();
}
