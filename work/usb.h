
#ifndef GUARD_usb_h
#define GUARD_usb_h

#include "usbconfig.h"

extern usb_setup_t usb_setup;

//! Initialisation function
void usb_init(void *param);

void usb_set_sof_cb(usb_cb cb);
void usb_set_presof_cb(usb_cb cb);

int usb_stall(u8 epnum);
int usb_unstall(u8 epnum);
int usb_is_stalled(u8 epnum);

int usb_get_state(void);
int usb_get_config(void);

//! Perform a bulk transmission
/*! Transmits data on a bulk endpoint.

In nonblocking systems, this initiates the transaction, 
which proceeds in the background, typically using DMA.  
When the transaction is complete, a callback is called, 
set with usb_set_epstat_cb().  The status can be checked using 
usb_epstat().

In blocking systems, this performs the transaction.  
Status can be checked from a different thread using 
usb_epstat(), and when 
the transaction is complete, the \c epstat callback is called.

This call cannot be used on the control endpoint.

\p len can be zero.  This causes a zero-length packet to be transmitted.

If this call is made incorrectly, nothing happens.  Possible errors are:

- Attempt to call this for a control endpoint
- Endpoint does not exist
- data is null

If this call is made on a nonexistent endpoint, nothing happens.

\param[in] epn Endpoint number.  Must be 1-7.
\param[in] data Pointer to data buffer to transmit.
\param[in] len Number of bytes to transmit.
\return Status code.
\retval 0 No error
\retval -1 Invalid endpoint
\retval -2 Null data pointer
\retval -3 Endpoint is busy
*/
int usb_tx(u8 epn, usb_data_t *data, u32 len);

//! Perform a bulk reception
/*! Receives data from a bulk endpoint.

On nonblocking systems, this causes the stack to receive len bytes 
of data to the 
given buffer from the given endpoint, once the host sends it.

On blocking systems, this waits for the host to send data, 
copies it to the buffer, then returns.

When the transfer is complete, or when the timeout has passed, the epstat 
callback is called.

This call cannot be used on the control endpoint.
*/
int usb_rx(u8 epn, usb_data_t *data, u32 len);

//! Immediately stop the endpoint from sending data
/*! Cancels a transaction in progress.

This may not happen immediately.  In particular, if the host is in the 
middle of moving a packet, this function does not cancel it.  However, at 
the next possible opportunity the endpoint will NAK further IN or OUT 
requests from the host.
*/
void usb_bulk_cancel(u8 epn);

void usb_dev_reset(void);

//! Attach the node
/*! Causes the USB hardware to attach to the bus, on systems that 
support this.  On systems without this capability, nothing happens.
Usually this causes the pullup to be activated.

If the state is not \c USB_STATE_DETACHED, this is ignored.

Following this call, state becomes \c USB_STATE_ATTACHED. 
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
*/
void usb_detach(void);

//! Whether node is attached
/*! Returns non-zero if the node is attached.
\retval 0 Node is not attached
\retval 1 Node is attached
*/
int usb_is_attached(void);

//! Signal the host to wake up
/*! Initiate remote wakeup on the host. */
void usb_remote_wakeup(void);

//int usb_set_out_cb(int cfg, int epn, usb_cb_out cb);
//int usb_set_in_cb(int cfg, int epn, usb_cb_in cb);
void usb_set_state_cb(usb_cb_int cb);
//void usb_set_epstat_cb(int epn, usb_cb_done cb);
//int usb_get_epstat(int epn);

//! Set endpoint's timeout
/*! Set the endpoint's timeout.

For receive endpoints, the timeout starts over when usb_bulk_rx() is called
and when OUT packets are received, 
and expires when the timeout passes without any OUT packets from the host 
being received within an expected time.

For transmit endpoints, the timeout works the same, except that IN packets 
are expected.

A value of zero disables the timeout.

Not all systems support timeout.  On those that do,
the accuracy of the timeout depends on 
the accuracy of the usbhw_time() function.

This call does not apply to the control endpoint.

If this call is made for an invalid endpoint, nothing is done.

\param epn Endpoint number.  Must be 1-7 or 9-15.
\param timeout Timeout in milliseconds.
*/
void usb_set_ep_timeout(int epn, u16 timeout);

/*! \fn void usb_ctl(void)
\brief Callback for control transactions

PORUS calls this function when a new SETUP packet arrives.  
At the time of the call, data for OUT transactions has been copied to 
\c usb_ctl_write_data .

This call is generally made at interrupt time.  You can either 
handle the packet right away or defer the handling to a task.

If a new SETUP arrives before the old one has ended, PORUS stalls it 
and ignores calls to usb_ctl_write_end() or usb_ctl_read_end() .
*/

//! Check and handle standard control transactions
/*! Checks the current \p usb_setup .  If it is a standard 
transaction, this handles it and returns 1.  If it is not a 
standard transaction, returns 0.

Unless you mean to handle standard transactions yourself, you should 
call this before processing a setup packet.
*/
int usb_ctl_std(void);

//! Conclude a control read
/*! Ends a control read, either with a stall or by returning data.

If \p stat is 1, the control endpoint is stalled.  Otherwise, data is 
returned using \p len and \p data .

\param[in] stat Status.  0 to ACK, 1 to stall.
\param[in] len Length of returned data in bytes.  Ignored if 
\p stall is 1.
\param[in] data Pointer to data to transmit.  Ignored if 
\p stall is 1.
*/
void usb_ctl_read_end(int stat, int len, usb_data_t *data);

//! Conclude a control write
/*! Ends a control write, either with a stall or with an ACK 
handshake.  If \p stat is 1, the control endpoint is stalled; 
if 0, an ACK is returned.

\param[in] stat Status.  0 to ACK, 1 to stall.
*/
void usb_ctl_write_end(int stat);

//! Get currently active configuration number
/*! Returns the number of the currently active configuration, 
or -1 if no configuration is active. */
int usb_get_config(void);

#endif
