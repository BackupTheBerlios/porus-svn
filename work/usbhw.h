
#ifndef GUARD_usbhw_h
#define GUARD_usbhw_h

#include "usbpriv.h"

//! Transmit a single USB packet
/*! Submits the packet request \p pkt for transmission.  When it is 
complete, usb_evt_txdone() is called.

On blocking systems, this function copies the data in the request packet 
to a holding buffer, where it becomes ready for an IN.  This function 
does not wait for an IN; it returns when the copy is complete.

On nonblocking systems, this function sets up and activates the DMA 
transfer to the holding buffer, and returns immediately.  

On some systems, two or more requests can be active at once.  If no 
requests can be accepted, -1 is returned.  This function does not block 
waiting for a request slot to become available, even on blocking 
systems.

\param[in] pkt Pointer to a packet request.
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted
*/
int usbhw_tx(usb_packet_req_t *pkt);

//! Receive a single USB packet
/*! Waits for an OUT transaction.  When it occurs, the request packet 
is filled in and returns.

This function makes the endpoint hardware ready for 
an OUT transaction.  When the OUT transaction occurs, an ISR copies the 
data to the request packet, possibly using DMA.  When the data has been 
copied, usb_evt_rxdone() is called.

On some systems, two or more requests can be active at once.  If no 
requests can be accepted, -1 is returned.  This function does not block 
waiting for a request slot to become available, even on blocking 
systems.

\param[in] pkt Pointer to a packet request.
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted
*/
int usbhw_rx(usb_packet_req_t *pkt);

//! Cancel any pending transaction
void usbhw_cancel(usb_endpoint_t *ep);

//! Time in milliseconds
/*! Returns a time in milliseconds.

The time need not start at any particular value; the only requirement 
is that relative times are somewhat accurate.

An accuracy of at least 10 milliseconds is preferred.
*/
#ifdef USBHW_HAVE_TIMEOUT
u32 usbhw_time(void);
#endif

//! Set up USB hardware
/*! Performs any necessary initialisation on USB hardware.  Called at 
system initialisation time.

\p parms optionally points to device-specific parameters needed for setup.
What it points to must be described in the port's documentation.  It may 
be ignored.

When the USB hardware is initialised, it should be detached from the bus 
if possible.  If not, the initial state will be USB_STATE_POWERED.  The 
USBHW_HAVE_DETACH define controls this.

Control endpoints should be set up and ready for data.  No other endpoints 
should be made active.

This function is not called in response to a bus reset; usbhw_reset() is 
called for that.

\param[in] parms Platform-specific parameters
\retval 0 Successful
\retval -1 Error
*/
int usbhw_init(void *parms);

//! Respond to a bus reset
/*! Do anything needed on the hardware in response to a bus reset.  This 
function is always called in response to a USB bus reset.
*/
void usbhw_reset(void);

//! Get SETUP transaction data
/*! Copies the data for the most recent SETUP packet from the hardware to 
the given usb_setup_t structure.

\param s Setup structure to copy data to
\retval 0 Successful
\retval -1 Could not copy
*/
int usbhw_get_setup(usb_setup_t *s);

//! Get control write data
/*! Copies the data for the most recently received OUT packet from the 
control endpoint hardware to the given buffer.  

At entry, \p len must point to the number of bytes that \p d can hold.  
If the amount of received data is greater than \p len, -1 is returned.
Otherwise, at exit, the 
number of bytes actually copied is put in \p len.

This is usually called in response to usb_evt_ctl_rx().

This blocks until the copy is complete.

\retval -1 More data received than expected
*/
int usbhw_get_ctl_write_data(u8 *len, usb_data_t *d);

//! Handshake a write transaction
/*! Causes an ACK handshake to appear for a write transaction. */
void usbhw_ctl_write_handshake(void);

//! Copy up control read data
/*! Copies the given data to the control endpoint for transmission.  
Blocks until the copy is complete.  When the host actually gets the data, 
with an IN, usb_evt_ctl_tx() is called.
*/
void usbhw_put_ctl_read_data(u8 len, usb_data_t *d);

//! Handshake a read transaction
/*! Causes an ACK handshake to appear for a read transaction. */
void usbhw_ctl_read_handshake(void);

//! Stall endpoint
/*! Stall the given endpoint.  Does not apply to control endpoints; use 
usbhw_ctl_stall() on those. */
void usbhw_stall(int epn);

//! Unstall endpoint
/*! Unstall the given endpoint.  Does not apply to control endpoints; use 
usbhw_ctl_stall() on those. */
void usbhw_unstall(int epn);

//! Endpoint stall status
/*! Returns 1 if the endpoint is stalled, 0 if not. */
int usbhw_is_stalled(int epn);

//! Control stall
/*! Set the USB hardware for a control stall. */
void usbhw_ctl_stall(void);

//! Control stall status
/*! Returns 1 if the control endpoint is stalled, 0 if not. */
int usbhw_ctl_is_stalled(void);

//! Do hardware setup for the given endpoint, and activate it
/*! Activates the hardware for the endpoint.  Makes it ready to 
operate.  Returns -1 if this can't be done.
*/
int usbhw_activate_ep(usb_endpoint_t *ep);

//! Deactivate the given endpoint
/*! Deactivates the hardware for the endpoint.
*/
void usbhw_deactivate_ep(usb_endpoint_t *ep);

//! Set the node address in hardware
void usbhw_set_address(u8 adr);

void usbhw_int_en(void);
void usbhw_int_en_sof(void);
void usbhw_int_en_presof(void);
void usbhw_int_en_txdone(int epn);
void usbhw_int_en_rxdone(int epn);
void usbhw_int_en_setup(void);
void usbhw_int_en_ctlin(void);
void usbhw_int_en_ctlout(void);
void usbhw_int_dis(void);
void usbhw_int_dis_sof(void);
void usbhw_int_dis_presof(void);
void usbhw_int_dis_txdone(int epn);
void usbhw_int_dis_rxdone(int epn);
void usbhw_int_dis_setup(void);
void usbhw_int_dis_ctlin(void);
void usbhw_int_dis_ctlout(void);

#endif
