
#ifndef GUARD_usb_h
#define GUARD_usb_h

// :wrap=soft:

#include "usbconfig.h"
#include "portconf.h"

extern usb_setup_t usb_setup;

//! Initialisation function
/*! Initialises PORUS and the hardware.  You must call this before calling 
any of the other functions in PORUS.

\p param points to parameters specific to the hardware port, if there 
are any.  See the port's documentation for details.

\ingroup grp_public_support
*/
void usb_init(void *param);

//@}

//! Set start-of-frame callback
/*! Sets a callback for Start Of Frame.  The callback will be called 
under interrupt whenever a SOF token is received.  The callback takes 
no arguments and returns nothing.

This is by default not set.  If this is not set, SOF interrupts are
ignored.

To unset the callback, pass 0 for \p cb .

\ingroup grp_public_io
*/
void usb_set_sof_cb(usb_cb_sof cb);

//! Set pre-SOF callback
/*! Sets a callback for pre-SOF, if supported by the hardware.

Some USB peripherals offer a pre-SOF interrupt.  This is timed to 
occur at a fixed time before the next SOF interrupt occurs.  If the 
hardware supports it, you can set this time using usb_set_presof_time().  
See the port documentation for details.

The callback takes no arguments and returns nothing.

To unset the callback, pass 0 for \p cb .

\ingroup grp_public_io
*/
void usb_set_presof_cb(usb_cb_sof cb);

//! Set pre-SOF time
/*! Sets the amount of time between pre-SOF and SOF, on hardware that 
supports it.  \p time is in microseconds.

The port documentation should say whether this is supported.

\ingroup grp_public_io
*/
void usb_set_presof_time(u16 time);

//! Get node's USB state
/*! Returns the current USB state, e.g. attached, powered, configured, 
etc.

\ingroup grp_public_support
*/
int usb_get_state(void);

//! Get node's configuration number
/*! Returns the current configuration number, or 0 if the node is not 
configured.

\ingroup grp_public_support
*/
int usb_get_config(void);

//! Attach the node
/*! Causes the USB hardware to attach to the bus, on systems that 
support this.  On systems without this capability, nothing happens.
Usually this causes the pullup to be activated.

If the state is not \c USB_STATE_DETACHED, this is ignored.

Following this call, state becomes \c USB_STATE_ATTACHED. 

\ingroup grp_public_support
*/
void usb_attach(void);

//! Detach the node
/*! Causes the USB hardware to detach from the bus, on systems that 
support this.  On systems without this capability, nothing happens.  
Usually this causes the pullup to be turned off.

If the state is \c USB_STATE_ATTACHED, this is ignored.

If any transactions are in progress when this call is made, the result 
is unknown.  Do not call this while a transaction is in progress.

Following this call, state becomes \c USB_STATE_DETACHED.

\ingroup grp_public_support
*/
void usb_detach(void);

//! Whether node is attached
/*! Returns non-zero if the node is attached.

\retval 0 Node is not attached
\retval 1 Node is attached

\ingroup grp_public_support
*/
int usb_is_attached(void);

//! Signal the host to wake up
/*! Initiate remote wakeup on the host.

\ingroup grp_public_support
*/
void usb_remote_wakeup(void);

//int usb_set_out_cb(int cfg, int epn, usb_cb_out cb);
//int usb_set_in_cb(int cfg, int epn, usb_cb_in cb);

//! Set USB state change notification callback
/*! Sets the USB state change notification callback.  The callback is called whenever the USB state changes, and after the state change has actually occurred.

\sa usb_cb_state, grp_states

\ingroup grp_public_support
*/
void usb_set_state_cb(usb_cb_state cb);

//! Get endpoint status
/*! Returns the status of the given endpoint.  See grp_epstat for a list of possible states.

This cannot be used for control endpoints.  If it is, -1 is returned.

\param[in] ep Endpoint
\retval >=0 Endpoint status
\retval -1 Invalid endpoint

\sa grp_epstat
*/
//int usb_get_epstat(usb_endpoint_t *ep);
#define usb_get_epstat(EP) ((EP)?((EP)->data->stat):(-1))

int usb_set_evt_cb(usb_endpoint_t *ep, usb_evt_cb cb);

//! Stall an endpoint
/*! Stalls the given endpoint.  Does nothing if the endpoint is stalled 
already.

This cannot be used for the control endpoint.  To stall a control 
endpoint, call usb_ctl_read_end() or usb_ctl_write_end() with a status 
of 1.

\param[in] ep Endpoint
\retval 0 Success
\retval -1 Invalid endpoint

\ingroup grp_public_io
*/
int usb_stall(usb_endpoint_t *ep);

//! Unstall an endpoint
/*! Unstalls the given endpoint.  Does nothing if the endpoint is not 
stalled.

This cannot be used for the control endpoint.  Control endpoints are 
unstalled when a SETUP arrives.

\param[in] ep Endpoint
\retval 0 Success
\retval -1 Invalid endpoint

\ingroup grp_public_io
*/
int usb_unstall(usb_endpoint_t *ep);

//! Find out whether endpoint is stalled
/*! Returns 1 if the given endpoint is stalled, 0 otherwise.

This cannot be used for control endpoints.

\param[in] ep Endpoint

\ingroup grp_public_io
*/
int usb_is_stalled(usb_endpoint_t *ep);

#if 0
//! Perform data I/O
/*! Transmits or receives data.  On OUT endpoints, this call performs reception from the host; on IN endpoints, it performs transmission to the host.

For transmission, PORUS will attempt to send all bytes requested, breaking up the buffer into shorter packets as necessary.  When all bytes have been sent, the endpoint status is changed and the callback is called (see below).  If the last packet is equal to the maximum packet size, PORUS sends an additional zero-byte packet, as recommended in the USB specification.

For reception, \p len is a maximum only; PORUS does not wait for \p len bytes to be received before returning.  PORUS instead waits for the first complete OUT or chain of OUT packets, ending in a short or zero-length packet.  If the host sends more than \p len bytes, PORUS begins returning NAK when more than \p len bytes have been received, and ends the transaction with an OVERFLOW endpoint status.  PORUS never stores more than \p len bytes in the buffer, i.e., it never writes past the end of the given buffer.

In nonblocking systems, usb_move() initiates the transaction, which proceeds in the background, perhaps using DMA.  When the transaction is complete, a callback is called, set with usb_set_epstat_cb().  The status can be checked using usb_epstat().  PORUS does not support multiple transactions; if a transaction is ongoing, usb_move() returns -3.

In blocking systems, usb_move() performs the transaction before returning, copying data to the USB hardware as necessary.  Status can be checked from a different thread using usb_epstat(). When the transaction is complete, the endpoint status and I/O done callbacks are called, if set.

This call cannot be used on the control endpoints.  The control endpoints use a separate system for data transfer.

\p len can be zero.  For transmission, this causes a zero-length packet to be transmitted; for reception, nothing happens.

\note If a packet is received on an OUT endpoint, and no usb_move() has been issued, the next usb_move() will receive that packet.  Use usb_cancel() to cancel unreceived packets on an OUT endpoint.

If this call is made incorrectly, nothing happens, and an error code is returned.  Possible errors are:

- Attempt to call this for a control endpoint
- Endpoint does not exist
- \p data is null
- Endpoint is busy

\param[in] ep Endpoint
\param[in] data Pointer to data buffer to transmit
\param[in] len Number of bytes to transmit
\param[in] cb Callback; if 0, no callback is used
\param[in] ptr Pointer to pass to callback

\return Status code
\retval 0 No error
\retval -1 Invalid endpoint
\retval -2 Null data pointer
\retval -3 Endpoint is busy

\sa usb_epstat(), usb_evt_cb
\ingroup grp_public_io
*/
int usb_move(usb_endpoint_t *ep, usb_data_t *data, 
	u32 len, usb_cb_done cb, void *ptr);

//! Move data and wait for completion
/*! Same as usb_move(), but blocks until the transfer is complete, and no callback is called.

\param[in] ep Endpoint
\param[in] data Data buffer
\param[in] len Number of bytes to move
\return Status code
\retval 0 Success
\retval -1 Invalid endpoint
\retval -2 Null data pointer
\retval -3 Endpoint is busy

\sa usb_set_ep_timeout(), usb_move()
*/
int usb_move_wait(usb_endpoint_t *ep, usb_data_t *data, u32 len);
#endif

//! Make an endpoint ready for reception
/*! Makes an endpoint ready for reception.  Packets are accepted and copied to \p data until at least \p len bytes are received.  When \p len or more bytes have been received, possibly in multiple packets, the endpoint's status is updated (usb_get_epstat()), and the endpoint's event callback, if any, is called (usb_evt_cb).  If \p len bytes are not received in time, or if reception is interrupted for too long, a timeout occurs.

The event callback is called if a timeout or cancellation occurs; see usb_evt_cb for details.

If the endpoint is busy, i.e. an unfinished reception request is in progress, this function returns -1.  Some ports may support multiple requests.

This function cannot be used for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to buffer for incoming data
\param[in] len Maximum number of bytes to receive
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint

\sa usb_evt_cb, usb_get_epstat()
*/
int usb_rx(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Make an endpoint ready for chained reception
/*! Makes an endpoint ready for chained reception.  Packets are accepted and copied to \p data until either \p len bytes are received or a short packet is received.  After a short packet is received, the endpoint's status is updated (usb_get_epstat()), and the endpoint's event callback, if any, is called (usb_evt_cb).

The event callback is also called if a timeout or cancellation occurs; see usb_evt_cb for details.

If the endpoint is busy, i.e. an unfinished reception request is in progress, this function returns -1.  Some ports may support multiple requests.

This function cannot be used for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to buffer for incoming data
\param[in] len Maximum number of bytes to receive
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint

\sa usb_evt_cb, usb_get_epstat()
*/
int usb_rx_chain(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Request a transmission
/*! Causes the hardware to prepare to transmit the \p len bytes at \p data packet by packet.  A short packet is not appended.  The packet size is the endpoint's maximum.

Since all data transfer is controlled by the host, there is no guarantee that the data will go out immediately.  This function initiates a request, and does not wait for the data to be transmitted.  When all of the data has been transmitted, the endpoint's status (usb_get_epstat()) is updated and the endpoint's event callback is called, if any (usb_set_evt_cb(), usb_evt_cb).

The callback is also called if a timeout or cancellation occurs; see usb_evt_cb for details.

If the transmission request cannot be accepted, -1 is returned.

This function cannot be used for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to beginning of data to be transmitted
\param[in] len Length of data; clipped to maximum packet length
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint

\sa usb_evt_cb, usb_get_epstat()
*/
int usbhw_tx(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Request a chained transmission
/*! Causes the hardware to prepare to transmit the \p len bytes at \p data packet by packet, ending with a short packet.  The packet size is the endpoint's maximum.

Since all data transfer is controlled by the host, there is no guarantee that the data will go out immediately.  This function initiates a request, and does not wait for the data to be transmitted.  When all of the data has been transmitted, the endpoint's status (usb_ep_stat()) is updated and the endpoint's callback is called, if any (usb_set_evt_cb(), usb_evt_cb).

The callback is also called if a timeout or cancellation occurs; see usb_evt_cb for details.

If the transmission request cannot be accepted, -1 is returned.

This function cannot be used for the control endpoint.

\param[in] ep Endpoint for transmission
\param[in] data Pointer to beginning of data to be transmitted
\param[in] len Length of data; clipped to maximum packet length
\return Status code
\retval 0 Success
\retval -1 Request could not be accepted
\retval -2 Invalid endpoint

\sa usb_evt_cb, usb_get_epstat()
*/
int usbhw_tx_chain(usb_endpoint_t *ep, usb_data_t *data, u16 len);

//! Cancel transfers
/*! Cancels any transfer in progress on the endpoint.

This may not take effect immediately.  USB packet transfer is usually hardware-controlled and cannot be cancelled.  However, the endpoint will NAK further IN or OUT requests from the host.

This function causes the endpoint callback to be called with status of USB_EPSTAT_CANCELLED.  It does not change the endpoint's status.

\param[in] ep Endpoint

\ingroup grp_public_io
*/
void usb_cancel(usb_endpoint_t *ep);

//! Callback for control transactions
/*! This function is a required callback, and must be supplied by the user.

It is called when a new SETUP packet arrives.  At the time of the call, data for OUT transactions has been copied to the global variable \c usb_ctl_write_data .

This call is generally made at interrupt time.  It should be handled as quickly as possible, or the handling should somehow be deferred.

If a new SETUP arrives before the old one has ended, PORUS stalls it 
and ignores calls to usb_ctl_write_end() or usb_ctl_read_end() .

\ingroup grp_public_control
*/
void usb_ctl(void);

//! Check and handle standard control transactions
/*! Checks the current \p usb_setup .  If it is a standard 
transaction, this handles it and returns 1.  If it is not a 
standard transaction, returns 0.

Unless you will handle standard transactions yourself, you should 
call this before processing a setup packet.

\ingroup grp_public_control
*/
int usb_ctl_std(void);

//! Conclude a control read
/*! Ends a control read. The data in \p data is returned to the host along with an ACK.

\param[in] len Length of returned data in bytes.
\param[in] data Pointer to data to transmit.

\ingroup grp_public_control
*/
void usb_ctl_read_end(int len, usb_data_t *data);

//! Conclude a control write
/*! Ends a control write.  An ACK is returned to the host.

\ingroup grp_public_control
*/
void usb_ctl_write_end(void);

void usb_ctl_stall(void);

//! Set endpoint timeout
/*! Sets the timeout for the given endpoint in milliseconds.

The timeout is, roughly speaking, the maximum amount of quiet time that may elapse for an endpoint with an active transfer request.  The default is three seconds.

The accuracy of the timeout mechanism depends on the port.  See the port notes for details.

Zero disables timeouts; the endpoint will wait forever.

\param[in] ep Endpoint
\param[in] ms Timeout in milliseconds, or 0 to disable
*/
void usb_set_ep_timeout(usb_endpoint_t *ep, u16 ms);

//! Get endpoint timeout
/*! Returns the timeout set for the given endpoint, in milliseconds.  Zero indicates that no timeout is active.

\param[in] ep Endpoint

\sa usb_set_ep_timeout()
*/
u16 usb_get_ep_timeout(usb_endpoint_t *ep);

#endif
