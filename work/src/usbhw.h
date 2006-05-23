
#ifndef GUARD_usbhw_h
#define GUARD_usbhw_h

/* :wrap=soft: */
#include "usbpriv.h"
//#include <portconf.h>

/*! \ingroup grp_hw
@{
*/

#if 0
//! Packet I/O request structure
/*! This structure encapsulates a request to transmit or receive a USB packet.  It identifies the endpoint, data length, and certain other parameters, and contains the actual data for the packet.  The structure is set up by the PORUS core and passed to usbhw_rx and usbhw_tx for processing.

Note that ports may implement a queueing scheme, where several requests can be active on a single endpoint at once.

When this structure is used for transmission (on an IN endpoint), the following fields are filled in:

 - \c data points to the data to be transmitted
 - \c reqlen is the number of bytes to be transmitted
 - \c ep identifies the endpoint to transmit on
 - \c done must be set to zero
 - \c timeout should be set as needed (not used yet)

Following transmission, the \c actlen field should contain the number of bytes that were actually transmitted, and \c done should be set to 1, indicating that the request was completed.  If \c actlen is zero, something went wrong; usually this means that a timeout occurred.

When this structure is used for reception (on an OUT endpoint), the following fields are filled in:

  - \c data points to an area in memory large enough to contain the received data
  - \c reqlen is the number of bytes requested
  - \c ep identifies the endpoint to receive from
  - \c done must be set to zero
  - \c timeout should be set as needed (not used yet)

Following reception, the \c actlen field should contain the number of bytes that were actually recieved, and \c done should be set to 1, indicating that the request was completed.  If \c actlen is zero, something went wrong; usually this means that a timeout occurred.

\sa usbhw_tx, usbhw_rx
*/
typedef struct usb_packet_req_t {
	unsigned int done:1, // 0 = request waiting, 1 = request complete
		timeout:1; // 0 = no timeout, 1 = timeout occured
	usb_endpoint_t *ep; // ptr to endpoint
	usb_data_t *data; // ptr to buffer
	u16 reqlen; // requested length in bytes
	u16 actlen; // actual length in bytes
} usb_packet_req_t;

//! Transmit a single USB packet
/*! Submits the packet request \p pkt for transmission.

On blocking systems, this function copies the data in the request packet 
to a holding buffer, where it becomes ready for an IN.  This function 
does not wait for an IN; it returns when the copy is complete.

On nonblocking systems using DMA, this function sets up and activates the DMA transfer to the holding buffer, and returns immediately.  

On some systems, two or more requests can be active at once.  If no 
requests can be accepted, -1 is returned.

This function should never block waiting for a request slot to become available, even on blocking systems.  The core decides whether and how to wait for a free slot.

When a request is complete, the port must update the endpoint's \c actlen field by adding to it the number of bytes successfully transmitted; then usb_evt_cpdone() must be called.

\c pkt is the packet request structure.  When this function is called, the following fields in \c pkt are filled in:

  - \c data points to an area in memory large enough to contain the received data
  - \c reqlen is the number of bytes requested
  - \c ep identifies the endpoint to receive from
  - \c done is set to zero

Following reception, the \c actlen field should contain the number of bytes that were actually recieved, and \c done should be set to 1, indicating that the request was completed.  If something goes wrong, such as a timeout, \c actlen should be set to zero.

This function's return code indicates whether the request could be accepted; it does \e not indicate whether the request completed successfully, even on blocking systems.  For example, if a timeout occurs, this function should still return zero.

\param[in] pkt Pointer to a packet request.
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted

\sa usb_evt_cpdone()
*/
int usbhw_tx(usb_packet_req_t *pkt);
#endif

//! Request a chained transmission
/*! Causes the hardware to prepare to transmit the \p len bytes at \p data packet by packet, ending with a short packet.  The packet size is the endpoint's maximum.

Since all data transfer is controlled by the host, there is no guarantee that the data will go out immediately; this function is therefore a request, and must not wait for the data to actually be transmitted.  When all of the data has been transmitted, the port must call usb_evt_done().

If the transmission request cannot be accepted, -1 is returned.

The port must set the endpoint status to USB_EPSTAT_XFER when it prepares the endpoint.  When the reception has finished, the port must set the endpoint status to USB_EPSTAT_IDLE, if no other transactions are pending.

If a timeout occurs, the port must call usb_evt_done() with the proper status.

usb_evt_done() may be called under interrupt.

This function is never called for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to beginning of data to be transmitted
\param[in] len Length of data; clipped to maximum packet length
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint

\sa usb_evt_done()
*/
int usbhw_tx_chain(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Request a transmission
/*! Causes the hardware to prepare to transmit the \p len bytes at \p data packet by packet.  A short packet is not appended.  The packet size is the endpoint's maximum.

Since reception is controlled by the host, there is no guarantee that the data will go out immediately; this function is therefore a request, and must not wait for the data to actually be transmitted.  When all of the data has been transmitted, the port must call usb_evt_done().

If the transmission request cannot be accepted, -1 is returned.

The port must set the endpoint status to USB_EPSTAT_XFER when it prepares the endpoint.  When the reception has finished, the port must set the endpoint status to USB_EPSTAT_IDLE, if no other transactions are pending.

If a timeout occurs, the port must call usb_evt_done() with the proper status.

usb_evt_done() may be called under interrupt.

This function is never called for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to beginning of data to be transmitted
\param[in] len Length of data; clipped to maximum packet length
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint

\sa usb_evt_done()
*/
int usbhw_tx(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Make an endpoint ready for chained reception
/*! Makes an endpoint ready for chained reception.  Packets are accepted and copied to \p data until either \p len bytes are received or a short packet is received.  The port then calls usb_evt_done().

The port must set the endpoint status to USB_EPSTAT_XFER when it prepares the endpoint.  When the reception has finished, the port must set the endpoint status to USB_EPSTAT_IDLE, if no other transactions are pending.

If a timeout occurs, the port calls usb_evt_done().

usb_evt_done() may be called under interrupt.

If the endpoint is busy, i.e. an unfinished reception request is in progress, this function returns -1.  Some ports may support multiple requests.

Hardware should configure the endpoint so that it returns NAKs when a receive request is not pending.

This function is never called for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to buffer for incoming data
\param[in] len Maximum number of bytes to receive
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint
*/
int usbhw_rx_chain(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Make an endpoint ready for reception
/*! Makes an endpoint ready for reception.  Packets are accepted and copied to \p data until at least \p len bytes are received.  When \p len or more bytes have been received, possibly in multiple packets, the port must call usb_evt_done().  If \p len bytes are not received in time, or if reception is interrupted for too long, a timeout occurs.

The port must set the endpoint status to USB_EPSTAT_XFER when it prepares the endpoint.  When the reception has finished, the port must set the endpoint status to USB_EPSTAT_IDLE, if no other transactions are pending.

If a timeout occurs, the port must call usb_evt_done().

usb_evt_done() may be called under interrupt.

If the endpoint is busy, i.e. a reception request or chained reception request is in progress, this function returns -1.  Some ports may support multiple requests.

Hardware should configure the endpoint so that it returns NAKs when a receive request is not pending.

This function is never called for the control endpoint.

*/
int usbhw_rx(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Cancel any pending transaction
/*! Attempts to cancel all requests occurring or pending on the given endpoint.

It is usually not possible to stop a packet transmission.  However, it should be possible to stop a DMA operation, or to stop the hardware between packets.

This function is called either because the user explicitly requested a cancellation, or because a timeout occurred.  These may be distinguished by the endpoint's status at the time of the call.  If the user requests a cancellation, the endpoint will have status USB_EPSTAT_CANCELLING; if a timeout occurs, the endpoint will have status USB_EPSTAT_TIMEOUT.

When all requests have been successfully cancelled, and the endpoint is ready for a new request, usb_evt_done() must be called with the proper event: USB_EVT_TIMEOUT or USB_EVT_CANCELLED.  Note that this may be done in a DMA interrupt service routine, and not by this function.

\param ep Endpoint to cancel on
*/
void usbhw_cancel(usb_endpoint_t *ep);

//! Time in milliseconds
/*! Returns a time in milliseconds.

The time need not start at any particular value; the only requirement 
is that relative times are somewhat accurate.

An accuracy of at least 10 milliseconds is preferred.

The times are typically used for timeout calculations.
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
Otherwise, at exit, the number of bytes actually copied is put in \p len.

\p last is a flag used to tell the hardware whether this is the last 
data packet expected.  If \p last is 1, the hardware will expect a status 
handshake; if \p last is 0, the hardware will expect more data.  Some 
hardware will stall automatically if an IN is received when an OUT is 
expected.

This function is called in response to usb_evt_ctl_rx(), so is done under 
interrupt. It blocks until the copy is complete.

\param len Pointer to length parameter
\param d Pointer to buffer
\param last End flag
\retval -1 More data received than expected
*/
int usbhw_get_ctl_write_data(u8 *len, usb_data_t *d, int last);

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
/*! Returns 1 if the endpoint is stalled, 0 if not.  Applies to both IN and OUT endpoints, but does not apply to control endpoints; use usbhw_ctl_is_stalled() for those.

\param epn Endpoint number; range is 1-31, except for 16.
*/
int usbhw_is_stalled(int epn);

//! Stall the control endpoint
/*! Stalls the control endpoint. */
void usbhw_ctl_stall(void);

//! Control stall status
/*! Returns whether the control endpoint is stalled.

\retval 1 Control endpoint is stalled
\retval 0 Control endpoint is not stalled
*/
int usbhw_ctl_is_stalled(void);

//! Activate endpoints in the given configuration
/*! Activates the endpoints in the given configuration, preparing the hardware for each endpoint as necessary.

The routine should iterate through all possible endpoints, and activate those which usb_get_ep() reports as existing.

If any of the endpoints cannot be activated, this routine should return -1.  If this happens, it is not necessary to deactivate the activated endpoints, if any; the core will call usbhw_deactivate_eps() if needed.

\param[in] cnf Configuration to activate
\return 0 on success, or -1 on error

\sa usbhw_deactivate_eps()
*/
int usbhw_activate_eps(int cnf);

//! Deactivate all endpoints
/*! Deactivates the endpoints in the given configuration (or all endpoints); i.e., shuts down the hardware for the endpoints so that they will not respond to transactions.

If the endpoints are already inactive, this routine should do nothing.  If only some of the endpoints are active, this routine should deactivate them.

This routine should also free any private memory allocated on hwdata for the endpoints.

\param[in] cnf Configuration to deactivate
*/
void usbhw_deactivate_eps(int cnf);

//! Set the node address in hardware
/*! Called by the standard control layer when it receives a SET_ADDRESS request.  This should set the node's hardware address.  This should be done immediately, so that the next received packet is checked against the given address.

If this address is the same as the current address, nothing needs to be done.

\param adr Hardware address to use
*/
void usbhw_set_address(u8 adr);

//! Enable USB hardware interrupts
/*! Enables USB hardware interrupts, or enables the main USB interrupt, if such exists.

This function must re-enable the set of interrupts which were enabled before usbhw_int_dis() is called.  If usbhw_int_dis() has not been called, it should enable no interrupts except the main USB interrupt, if there is one; i.e., the default state is no interrupts enabled.

If the hardware does not have a main interrupt, it is the port's responsibility to keep a record of what interrupts are enabled.

This function may be used for locks etc., and should if possible be a fast operation.
*/
void usbhw_int_en(void);

//! Enable SOF interrupt
/*! Enables SOF (start-of-frame) packet interrupt, if there is one.  Does nothing if the hardware has no SOF interrupt.
*/
void usbhw_int_en_sof(void);

//! Enable pre-SOF interrupt
/*! Enables the pre-SOF interrupt, if there is one.  Does nothing if the hardware lacks a pre-SOF mechanism.

Pre-SOF is typically generated by a timer triggered by the previous SOF packet.  It provides a SOF interrupt ahead of time, so that software can prepare data for the next SOF.
*/
void usbhw_int_en_presof(void);

//! Enable TXDONE interrupt
/*! Enables the TXDONE interrupt, if any, for the given IN endpoint.  Does nothing if there is no such interrupt.

The TXDONE interrupt is triggered when a packet is moved out of the device.

If the endpoint number is out of range, nothing is done.

\param epn Endpoint number, 17-31
*/
void usbhw_int_en_txdone(int epn);

//! Enable RXDONE interrupt
/*! Enables the RXDONE interrupt, if any, for the given OUT endpoint.  Does nothing if there is no such interrupt.

The RXDONE interrupt is triggered when a packet is moved into the device.

If the endpoint number is out of range, nothing is done.

\param epn Endpoint number, 1-16
*/
void usbhw_int_en_rxdone(int epn);

//! Enable SETUP interrupt
/*! Enables the SETUP interrupt, if any.  Does nothing if there is no such interrupt.

The SETUP interrupt is triggered when a SETUP packet is received.
*/
void usbhw_int_en_setup(void);

//! Enable CTLIN interrupt
/*! Enables the CTLIN interrupt, if any.  Does nothing if there is no such interrupt.

The CTLIN interrupt is triggered when a control packet is transmitted.
*/
void usbhw_int_en_ctlin(void);

//! Enable CTLOUT interrupt
/*! Enables the CTLOUT interrupt, if any.  Does nothing if there is no such interrupt.

The CTLOUT interrupt is triggered when a control packet is received.
*/
void usbhw_int_en_ctlout(void);

//! Disable all USB hardware interrupts
/*! Disables all USB hardware interrupts, or disables the main hardware interrupt, if such exists.

This function must remember which interrupts were enabled, so that usbhw_int_en() can restore them.
*/
void usbhw_int_dis(void);

//! Disable SOF interrupt
/*! Disables SOF (start-of-frame) packet interrupt, if there is one.  Does nothing if the hardware has no SOF interrupt.
*/
void usbhw_int_dis_sof(void);

//! Disable pre-SOF interrupt
/*! Disables the pre-SOF interrupt, if there is one.  Does nothing if the hardware lacks a pre-SOF mechanism.

Pre-SOF is typically generated by a timer triggered by the previous SOF packet.  It provides a SOF interrupt ahead of time, so that software can prepare data for the next SOF.
*/
void usbhw_int_dis_presof(void);

//! Disable TXDONE interrupt
/*! Disables the TXDONE interrupt, if any, for the given IN endpoint.  Does nothing if there is no such interrupt.

The TXDONE interrupt is triggered when a packet is moved out of the device.

If the endpoint number is out of range, nothing is done.

\param epn Endpoint number, 17-31
*/
void usbhw_int_dis_txdone(int epn);

//! Disable TXDONE interrupt
/*! Disables the TXDONE interrupt, if any, for the given OUT endpoint.  Does nothing if there is no such interrupt.

The RXDONE interrupt is triggered when a packet is moved into the device.

If the endpoint number is out of range, nothing is done.

\param epn Endpoint number, 1-16
*/
void usbhw_int_dis_rxdone(int epn);

//! Disable SETUP interrupt
/*! Disables the SETUP interrupt, if any.  Does nothing if there is no such interrupt.

The SETUP interrupt is triggered when a SETUP packet is received.
*/
void usbhw_int_dis_setup(void);

//! Disable CTLIN interrupt
/*! Disables the CTLIN interrupt, if any.  Does nothing if there is no such interrupt.

The CTLIN interrupt is triggered when a control packet is transmitted.
*/
void usbhw_int_dis_ctlin(void);

//! Disable CTLOUT interrupt
/*! Disables the CTLOUT interrupt, if any.  Does nothing if there is no such interrupt.

The CTLOUT interrupt is triggered when a control packet is received.
*/
void usbhw_int_dis_ctlout(void);

//@}

usb_alarm_t *usbhw_mkalarm(void);
int usbhw_sleep(usb_alarm_t *alarm, int timeout_ms);
void usbhw_wake(usb_alarm_t *alarm);

#endif
