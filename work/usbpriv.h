
#ifndef GUARD_usbpriv_h
#define GUARD_usbpriv_h

#include "usb.h"

typedef struct usb_packet_req_t {
	unsigned int done:1, // 0 = request waiting, 1 = request complete
		timeout:1; // 0 = no timeout, 1 = timeout occured
	usb_endpoint_t *ep; // ptr to endpoint
	usb_data_t *data; // ptr to buffer
	u16 reqlen; // requested length in bytes
	u16 actlen; // actual length in bytes
} usb_packet_req_t;

//! Called when data copy is done
void usb_evt_txdone(usb_endpoint_t *ep);
//! Called when OUT is done
void usb_evt_rxdone(usb_endpoint_t *ep);
//! Called if IN is not received in time
void usb_evt_txtimeout(usb_endpoint_t *ep);
//! Called if OUT is not received in time
void usb_evt_rxtimeout(usb_endpoint_t *ep);

//! Called in response to a bus reset
void usb_evt_reset(void);
//! Called for a SOF
void usb_evt_sof(void);
//! Called for a pre-SOF
void usb_evt_presof(void);
//! Called for Suspend
void usb_evt_suspend(void);
//! Called for Resume
void usb_evt_resume(void);

//! Called in response to a SETUP
void usb_evt_setup(void);
//! Called when a control OUT finishes
void usb_evt_ctl_rxdone(void);
//! Called when a control IN finishes
void usb_evt_ctl_txdone(void);

void usb_set_state(int state);
int usb_get_state(void);
void usb_set_address(u8 adr);
int usb_set_config(int cfn);
int usb_get_config(void);

int usb_ctl_state(void);

void usb_ctl_init(void);

#endif
