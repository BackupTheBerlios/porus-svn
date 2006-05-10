
#ifndef GUARD_usbpriv_h
#define GUARD_usbpriv_h

// :wrap=soft:

#include "usb.h"

//! Called when a packet copy is complete
/*! This function is called by the hardware layer when the data for a received packet request has been copied into user memory, or when user memory has been copied to USB.  It may be called as a result of a copy completing, or as a result of a DMA interrupt.

May be called under interrupt, especially on DMA systems.
*/
void usb_evt_cpdone(usb_endpoint_t *ep);

//! Called to signal a timeout on an endpoint
/*! Sets the timeout status on the given endpoint and cancels any pending transactions.

\param[in] ep Endpoint
*/
void usb_evt_timeout(usb_endpoint_t *ep);

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

//! Called to change an endpoint's state
/*! Call this when an endpoint's status changes.  This calls the endpoint callbacks if necessary and updates the endpoint data structure.

\p stat is not checked, so be careful to pass only valid state constants.

\sa grp_epstat
*/
void usb_set_epstat(usb_endpoint_t *ep, int stat);

void usb_set_state(int state);
int usb_get_state(void);
void usb_set_address(u8 adr);
int usb_set_config(int cfn);
int usb_get_config(void);

int usb_ctl_state(void);

void usb_ctl_init(void);

//@}

#endif
