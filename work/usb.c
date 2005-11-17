
#include "usbhw.h"

static volatile struct {
	unsigned int suspended:1,
		state:3;
	u8 address;
	u8 config;
} flags;

static usb_cb_sof sofCB, preSOFCB;
static usb_cb_state stateChangeCallback;

void usb_cancel(u8 epn)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return;
	usbhw_cancel(ep);
	usb_set_epstat(USB_EPSTAT_IDLE);
	//ep->data->xferInProgress=0;
}

int usb_move(u8 epn, usb_data_t *data, u32 len)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return -1;
	if (len&&!data) return -2;

	if (ep->data->epstat!=USB_EPSTAT_IDLE) return -3;
	usb_set_epstat(ep,USB_EPSTAT_XFER);
	//ep->data->xferInProgress=1;
	ep->data->buf=data;
	ep->data->reqlen=len;
	ep->data->actlen=0;
	//usbhw_int_dis_txdone(epn);
	usb_evt_cpdone(ep,0);
	//usbhw_int_en_txdone(epn);
	return 0;
}

#if 0
int usb_tx(u8 epn, usb_data_t *data, u32 len)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);

	if (!ep) return -1;
	if (len&&!data) return -2;

	if (ep->data->epstat!=USB_EPSTAT_IDLE) return -3;
	usb_set_epstat(ep,USB_EPSTAT_XFER);
	//ep->data->xferInProgress=1;
	ep->data->buf=data;
	ep->data->reqlen=len;
	ep->data->actlen=0;
	usbhw_int_dis_txdone(epn);
	usb_evt_cpdone(ep,0);
	usbhw_int_en_txdone(epn);
	return 0;
}
#endif

#if 0
void usb_evt_cpdone(usb_endpoint_t *ep, u16 actlen)
{
	usb_packet_req_t pkt;
	u32 l;

	//if (!ep->data->xferInProgress) return;
	if (ep->data->epstat!=USB_EPSTAT_XFER) return;
	ep->data->actlen+=actlen;
	if (ep->data->actlen>=ep->data->reqlen) {
		usb_set_epstat(USB_EPSTAT_IDLE);
		//ep->data->xferInProgress=0;
		if (actlen<64) {
			return;
		} else {
			l=pkt.reqlen=0;
		}
	} else {
		l=ep->data->reqlen-ep->data->actlen;
		if (l>ep->packetSize)
			l=ep->packetSize;
		pkt.reqlen=l;
		pkt.data=ep->data->buf+usb_mem_len(ep->data->actlen);
	}
	pkt.ep=ep;
	if (ep->id&16)
		usbhw_tx(&pkt);
	else
		usbhw_rx(&pkt);
	//ep->data->actlen+=l;
}
#endif

void usb_evt_cpdone(usb_endpoint_t *ep)
{
	usb_packet_req_t pkt;
	u32 l;

	//if (!ep->data->xferInProgress) return;
	if (ep->data->epstat!=USB_EPSTAT_XFER) return;
	ep->data->actlen+=actlen;
	if (ep->data->actlen>=ep->data->reqlen) {
		usb_set_epstat(USB_EPSTAT_IDLE);
		return;
	}
	l=ep->data->reqlen-ep->data->actlen;
	if (l>ep->packetSize)
		l=ep->packetSize;
	pkt.reqlen=l;
	pkt.data=ep->data->buf+usb_mem_len(ep->data->actlen);
	pkt.ep=ep;
	if (ep->id&16)
		usbhw_tx(&pkt);
	else
		usbhw_rx(&pkt);
	//ep->data->actlen+=l;
}

void usb_set_sof_cb(usb_cb_sof cb)
{
	if (!cb) {
		usbhw_int_dis_sof();
		sofCB=cb;
	} else {
		sofCB=cb;
		usbhw_int_en_sof();
	}
}

void usb_set_presof_cb(usb_cb_sof cb)
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
	usb_set_epstat(USB_EPSTAT_STALLED);
	return 0;
}

int usb_unstall(u8 epn)
{
	if (epn>31) return -1;
	if (!(epn&15)) return -1;
	usbhw_unstall(epn);
	usb_set_epstat(USB_EPSTAT_IDLE);
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

	flags.suspended=0;
	usb_set_state(USB_STATE_DEFAULT);
	usbhw_reset();
	// sof & presof interrupts only set if we have callbacks
	if (sofCB) usbhw_int_en_sof();
	if (preSOFCB) usbhw_int_en_presof();
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

void usb_set_epstat(usb_endpoint_t *ep, int stat)
{
	if (!ep) return;

	if (ep->data->stat==USB_EPSTAT_XFER&&stat==USB_EPSTAT_IDLE) {
		if (ep->data->done_cb) ep->data->done_cb(ep->id);
	ep->data->stat=stat;
	if (ep->data->epstat_cb) ep->data->epstat_cb(ep->id,stat);
}

int usb_get_epstat(int epn)
{
	usb_endpoint_t *ep;
	
	if (!(epn&15)) return -1;
	ep=usb_get_ep(flags.config,epn);

	if (!ep) return USB_EPSTAT_UNUSED;
	return ep->data->epstat;
}

void usb_set_epstat_cb(int epn, usb_cb_epstat cb)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);
	
	if (!ep) return;
	ep->data->epstat_cb=cb;
}

void usb_set_epstat_cb(int epn, usb_cb_done cb)
{
	usb_endpoint_t *ep=usb_get_ep(flags.config,epn);
	
	if (!ep) return;
	ep->data->done_cb=cb;
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
	if (epn<15) return;
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
			//ep->data->xferInProgress=0;
			usb_set_epstat(ep,USB_EPSTAT_IDLE);
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
