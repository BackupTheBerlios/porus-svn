
/* usb.c -- PORUS core logic */

/* PORUS
   Portable USB Stack

   (c) 2004-2006 Texas Instruments Inc.
*/

/* This file is part of PORUS.  You can redistribute and/or modify
   it under the terms of the Common Public License as published by
   IBM Corporation; either version 1.0 of the License, or
   (at your option) any later version.

   PORUS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   Common Public License for more details.

   You should have received a copy of the Common Public License
   along with PORUS.  It may also be available at the following URL:
   
      http://www.opensource.org/licenses/cpl1.0.txt
   
   If you cannot obtain a copy of the License, please contact the 
   Data Acquisition Products Applications Department at Texas 
   Instruments Inc.
*/

#include "usbhw.h"

static volatile struct {
	unsigned int suspended:1,
		state:3;
	u8 address;
	u8 config;
} flags;

static usb_cb_sof sofCB, preSOFCB;
static usb_cb_state stateChangeCallback;

void usb_cancel(usb_endpoint_t *ep)
{
	if (!ep) return;
	usb_set_epstat(ep,USB_EPSTAT_CANCELLING);
	usbhw_cancel(ep);
}

#if 0
int usb_move(usb_endpoint_t *ep, usb_data_t *data, u32 len, usb_cb_done cb, void *ptr)
{
	if (!ep) return -1;
	if (len&&!data) return -2;

	//usbhw_dmalog_write(USBHW_DMALOG_MOVE,ep->id);
	if (ep->data->stat!=USB_EPSTAT_IDLE) return -3;
	usb_set_epstat(ep,USB_EPSTAT_XFER);
	//ep->data->xferInProgress=1;
	ep->data->buf=data;
	ep->data->reqlen=len;
	ep->data->actlen=0;
	ep->data->done_cb=cb;
	ep->data->cb_ptr=ptr;
	//usbhw_int_dis_txdone(epn);
	usb_evt_cpdone(ep);
	//usbhw_int_en_txdone(epn);
	return 0;
}

static void wait_callback(void *ptr, int stat)
{
	usbhw_wake((usb_alarm_t *)ptr);
}

int usb_move_wait(usb_endpoint_t *ep, usb_data_t *data, u32 len)
{
	usb_alarm_t *alm;
	int err;

	//usbhw_dmalog_write(USBHW_DMALOG_MOVE_WAIT,ep->id);
	alm=usbhw_mkalarm();
	//usb_move(ep,data,len,wait_callback,alm);
	if (err=usb_move(ep,data,len,wait_callback,alm)) {
		usbhw_rmalarm(alm);
		return err;
	}
	//while (usb_get_epstat(ep)!=USB_EPSTAT_IDLE);
	while(usb_get_epstat(ep)==USB_EPSTAT_XFER)
		usbhw_sleep(alm,300);
	usbhw_rmalarm(alm);
	if (usb_check_timeout(ep)) {
		usb_clear_timeout(ep);
		return -4;
	}
	return 0;
}
#endif

#if 0
void usb_evt_cpdone(usb_endpoint_t *ep)
{
	usb_data_t *data;
	u32 l;

	if (ep->data->stat!=USB_EPSTAT_XFER) return;
	if (ep->data->timed_out) return;
	if (ep->data->actlen>=ep->data->reqlen) {
		usb_set_epstat(ep,USB_EPSTAT_IDLE);
		if (ep->data->done_cb)
			ep->data->done_cb(ep->data->cb_ptr,USB_EPSTAT_IDLE);
		ep->data->done_cb=0;
		return;
	}
	l=ep->data->reqlen-ep->data->actlen;
	data=ep->data->buf+usb_mem_len(ep->data->actlen);
	if (ep->id&16)
		usbhw_tx(ep,data,l);
	else
		usbhw_rx(ep,data,l);
}

void usb_evt_cpdone(usb_endpoint_t *ep)
{
	usb_data_t *data;
	u32 l;

	//usbhw_dmalog_write(USBHW_DMALOG_CPDONE,ep->id);
	if (ep->data->stat!=USB_EPSTAT_XFER) return;
	if (ep->data->timed_out) return;
	if (ep->data->actlen) {
		usb_set_epstat(ep,USB_EPSTAT_IDLE);
		if (ep->data->done_cb)
			ep->data->done_cb(ep->data->cb_ptr,USB_EPSTAT_IDLE);
		ep->data->done_cb=0;
		return;
	}
	l=ep->data->reqlen-ep->data->actlen;
	data=ep->data->buf+usb_mem_len(ep->data->actlen);
	if (ep->id&16)
		usbhw_tx(ep,data,l);
	else
		usbhw_rx(ep,data,l);
}
#endif

void usb_evt_done(usb_endpoint_t *ep, usb_data_t *data, u16 len, u8 evt)
{
	//usb_set_epstat(ep,USB_EPSTAT_IDLE);
	if (ep->data->evt_cb) ep->data->evt_cb(ep,data,len,evt);
}

int usb_rx_chain(usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
	return usbhw_rx_chain(ep,data,len);
}

int usb_rx(usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
	return usbhw_rx(ep,data,len);
}

int usb_tx_chain(usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
	return usbhw_tx_chain(ep,data,len);
}

int usb_tx(usb_endpoint_t *ep, usb_data_t *data, u16 len)
{
	return usbhw_tx(ep,data,len);
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

int usb_stall(usb_endpoint_t *ep)
{
	if (!ep) return -1;
	usbhw_stall(ep->id);
	usb_set_epstat(ep,USB_EPSTAT_STALLED);
	return 0;
}

int usb_unstall(usb_endpoint_t *ep)
{
	if (!ep) return -1;
	if (usb_get_epstat(ep)==USB_EPSTAT_XFER)
		usb_cancel(ep);
	usbhw_unstall(ep->id);
	usb_set_epstat(ep,USB_EPSTAT_IDLE);
	return 0;
}

int usb_is_stalled(usb_endpoint_t *ep)
{
	if (!ep) return -1;
	return usbhw_is_stalled(ep->id);
}

static void sendepevt(int e, int stat)
{
	usb_endpoint_t *ep;

	ep=usb_get_first_ep(usb_get_config());
	while(ep) {
		if (stat>=0) usb_set_epstat(ep,stat);
		if (ep->data->evt_cb) ep->data->evt_cb(ep,0,0,e);
		ep=ep->next;
	}
}

static int activate_endpoints(int config)
{
	usb_endpoint_t *ep;

	if (usbhw_activate_eps(config)) {
		usbhw_deactivate_eps(config);
		return -1;
	}
	ep=usb_get_first_ep(config);
	while(ep) {
		if (usb_get_epstat(ep)==USB_EPSTAT_INACTIVE) {
			usb_set_epstat(ep,USB_EPSTAT_IDLE);
			if (ep->data->evt_cb) ep->data->evt_cb(ep,0,0,USB_EVT_CONFIGURED);
		}
		ep=ep->next;
	}
	return 0;
}

static void deactivate_endpoints(void)
{
	int config;
	usb_endpoint_t *ep;

	config=usb_get_config();
	usbhw_deactivate_eps(config);
	ep=usb_get_first_ep(config);
	while(ep) {
		usb_set_epstat(ep,USB_EPSTAT_INACTIVE);
		if (ep->data->evt_cb) ep->data->evt_cb(ep,0,0,USB_EVT_DECONFIGURED);
		ep=ep->next;
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
		if (usb_get_state()==USB_STATE_CONFIGURED) {
			if (flags.config==cfn)
				return 0;
		}
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
	sendepevt(USB_EVT_SUSPENDED,-1);
}
                                               
void usb_evt_resume(void)
{
	if (!flags.suspended) return;
	flags.suspended=0;
	if (stateChangeCallback) stateChangeCallback(flags.state);
	sendepevt(USB_EVT_RESUMED,-1);
}

void usb_set_state_cb(usb_cb_state cb)
{
	stateChangeCallback=cb;
}

#if 0
void usb_set_epstat(usb_endpoint_t *ep, int stat)
{
	if (!ep) return;

	//if (ep->data->stat==USB_EPSTAT_XFER&&stat==USB_EPSTAT_IDLE) {
	//	if (ep->data->done_cb) ep->data->done_cb(ep->id);
	ep->data->stat=stat;
	//if (ep->data->epstat_cb) ep->data->epstat_cb(ep,stat);
}

int usb_get_epstat(usb_endpoint_t *ep)
{
	if (!ep) return USB_EPSTAT_UNUSED;
	return ep->data->stat;
}
#endif

int usb_set_evt_cb(usb_endpoint_t *ep, usb_evt_cb cb)
{
	if (!ep) return -1;
	ep->data->evt_cb=cb;
	return 0;
}

u16 usb_get_ep_timeout(usb_endpoint_t *ep)
{
	if (!ep) return 0;
	return ep->data->timeout;
}

void usb_set_ep_timeout(usb_endpoint_t *ep, u16 ms)
{
	if (!ep) return;
	ep->data->timeout=ms;
}

void usb_evt_timeout(usb_endpoint_t *ep)
{
	//usbhw_dmalog_write(USBHW_DMALOG_TIMEOUT,ep->id);
	if (usb_get_epstat(ep)!=USB_EPSTAT_XFER) return;
	usb_set_epstat(ep,USB_EPSTAT_TIMING_OUT);
	usbhw_cancel(ep);
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
	usb_endpoint_t *ep;

	flags.state=USB_STATE_DETACHED;
	flags.config=0;
	flags.suspended=0;
	flags.address=0;
	usb_ctl_init();
	sofCB=preSOFCB=0;
	stateChangeCallback=0;

	// ### TODO: need to do this for all configurations
	ep=usb_get_first_ep(1);
	while (ep) {
		if (!ep) continue;
		ep->data->stat=USB_EPSTAT_INACTIVE;
		ep->data->timed_out=0;
		ep->data->timeout=3000;
		//ep->data->epstat_cb=0;
		ep->data->evt_cb=0;
		//ep->data->cp_ptr=0;
		ep->data->buf=0;
		ep->data->reqlen=0;
		ep->data->actlen=0;
		ep->data->hwdata=0;
		ep=ep->next;
	}
	usbhw_init(param);
	usbhw_int_en();
}
