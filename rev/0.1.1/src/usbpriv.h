// :wrap=soft:

/* priv.h -- private definitions */

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

#ifndef GUARD_usbpriv_h
#define GUARD_usbpriv_h

#include "usb.h"

//! Called when a packet copy is complete
/*! This function is called by the hardware layer when the data for a received packet request has been copied into user memory, or when user memory has been copied to USB.  It may be called as a result of a copy completing, or as a result of a DMA interrupt.

May be called under interrupt, especially on DMA systems.
*/
void usb_evt_cpdone(usb_endpoint_t *ep);

//! Report an endpoint event
/*! This function must be called by the port when one of the following occurs:

- An endpoint has been enumerated and configured, and is ready for requests
- A transmission or reception request has completed, and the endpoint is ready for requests
- An endpoint cancellation has completed, and the endpoint is ready for requests
- An endpoint timeout has occurred, pending requests have been cancelled, and the endpoint is ready for requests
- An endpoint has been deconfigured, is now inactive, and is no longer ready for requests

The request in question is identified by the \p data pointer.  The number of bytes actually transmitted or received, and therefore written to or read from \p data, is passed in \p len.  \p evt identifies the reason for the call, and is either USB_EVT_DONE, USB_EVT_CANCELLED, or USB_EVT_TIMEOUT.

This function may be called under interrupt.

The port must allow new requests to be submitted during this callback.

\param ep Endpoint in question
\param data Data buffer for the request
\param len Number of bytes successfully transmitted or received
\param evt Event code
*/
void usb_evt_done(usb_endpoint_t *ep, usb_data_t *data, u16 len, u8 evt);

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
#define usb_set_epstat(EP,STAT) if (EP) (EP)->data->stat=STAT;
//void usb_set_epstat(usb_endpoint_t *ep, int stat);

void usb_set_state(int state);
int usb_get_state(void);
void usb_set_address(u8 adr);
int usb_set_config(int cfn);
int usb_get_config(void);

int usb_ctl_state(void);

void usb_ctl_init(void);

//@}

#endif
