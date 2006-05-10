
#ifndef GUARD_usbtypes_h
#define GUARD_usbtypes_h

// :mode=c:wrap=soft:

#include "types.h"

/*!
\defgroup grp_states USB states
\ingroup grp_public

These macros define codes for each state that the stack can be in.  
With one exception (USB_STATE_WILL_ADDRESS), they correspond to states 
described in the USB standard.

@{
*/

//! Attached state
/*!
This state is mentioned in the standard, but we currently do not 
use it; we go straight to POWERED, since without power we can't function
at all, so the ATTACHED state cannot occur.
*/
#define USB_STATE_ATTACHED 0

//! Powered-on state
/*!
This is the state we enter when we attach to the bus.  usb_connect() 
enters this state.
*/
#define USB_STATE_POWERED 1

//! Default state
/*!
We enter this state after a bus reset.
*/
#define USB_STATE_DEFAULT 2

//! Addressed state
/*!
The stack moves to this state when an address setup sequence is completed.
Note that the stack moves through USB_STATE_WILL_ADDRESS on its way to  
this state.

\sa USB_STATE_WILL_ADDRESS
*/
#define USB_STATE_ADDRESS 3

//! Configured state
/*!
The stack moves to this state after receiving a configuration set packet.
*/
#define USB_STATE_CONFIGURED 4

//! Suspended state
/*!
The stack moves to this state when it detects a bus suspend.  It moves 
back to the previous state after the suspend ends.
*/
#define USB_STATE_SUSPENDED 5

//! Will-address state
/*!
This is an internal state used for address-setting.  When the stack 
receives a set address request, it goes into this state instead of 
USB_STATE_ADDRESS, since the stack cannot actually change the device 
address until the status phase is complete.  When the status phase 
completes in USB_STATE_WILL_ADDRESS, the stack changes the device address 
and moves to USB_STATE_ADDRESS.

This state is not part of the USB standard.
*/
#define USB_STATE_WILL_ADDRESS 6

//! Detached state
/*!
This state is not named in the USB standard, but the 
standard does describe a state in which the device is not attached to 
the bus.

This state is assigned by usb_init() and usb_disconnect().
*/
#define USB_STATE_DETACHED 7
/*!
@}
*/

/*!
\defgroup grp_endpoint_types Endpoint types
\ingroup grp_public
@{
*/
#define USB_EPTYPE_CONTROL 0
#define USB_EPTYPE_BULK 1
#define USB_EPTYPE_INTERRUPT 2
#define USB_EPTYPE_ISOCHRONOUS 3
//!@}

/*!
\defgroup grp_epstat Endpoint states
\ingroup grp_public

These define the various states in which an endpoint can be.

@{
*/

//! Unused endpoint
/*! The endpoint is not defined or used. */
#define USB_EPSTAT_UNUSED 0

//! Endpoint is idle
/*! The endpoint is idle, is not stalled, and no transfers are taking place. */
#define USB_EPSTAT_IDLE 1

//! Transfer in progress
/*! A data transfer is in progress. */
#define USB_EPSTAT_XFER 2

//! Stalled
/*! The endpoint is stalled.

This status is never set on an endpoint; it is used with the done callback as a notification. */
#define USB_EPSTAT_STALL 3

//! Timed out
/*! Endpoint timed out.

This status is never set on an endpoint; it is used with the done callback as a notification. */
#define USB_EPSTAT_TIMEOUT 4

//! Inactive
/*! Endpoint is not activated.  Endpoints begin in this state; they are 
activated when the USB hardware turns on, and then they enter IDLE. */
#define USB_EPSTAT_INACTIVE 5

//!@}

/*!
\defgroup grp_ctl_phases Control transaction phases
\ingroup grp_public_control

See usb_setup_t#phase for information about these.
@{
*/
#define USB_CTL_PHASE_SETUP 0
#define USB_CTL_PHASE_OUT 1
#define USB_CTL_PHASE_IN 2
#define USB_CTL_PHASE_STATUS 3
//!@}

/*! \defgroup grp_control_states Control machine states
\ingroup grp_public_control

These states are used by PORUS' control endpoint 
handling system.  usb_ctl_state() reports this.
*/
//!@{
/*! Either no transactions are in progress, or all 
data has been copied to the hardware and the host is 
in the process of retrieving it.  In this state, 
PORUS is not waiting 
for a transaction to be finished.
*/
#define USB_CTL_STATE_IDLE 0
/*! Waiting for write data.

A write SETUP has been 
received, and it specified data.  PORUS is waiting 
to receive the data.

If the SETUP does not say that any data will be transmitted (i.e., \p len 
is zero), this state is not entered.
*/
#define USB_CTL_STATE_WWD 1
/*! Received write data.

All data from the write transaction has been 
received.  PORUS is waiting for you to finish the transaction 
using usb_ctl_write_end().
*/
#define USB_CTL_STATE_RWD 2
/*! Received read setup.

A read SETUP has been received.  PORUS is wating for you to 
finish the transaction by returning data with usb_ctl_read_end().
*/
#define USB_CTL_STATE_RRS 3
/*! Sending read data.

usb_ctl_read_end() has been called, and data is being 
copied up.  PORUS is not waiting for you to do anything.
*/
#define USB_CTL_STATE_SRD 4
//!@}

/*!
\defgroup grp_control_recipient Control packet recipient types
\ingroup grp_public_control

Recipient types for control packets.  These are the same as those listed in the USB specification.
@{
*/
//! Recipient is the device
#define USB_RCPT_DEV 0
//! Recipient is the interface
#define USB_RCPT_IFACE 1
//! Recipient is the endpoint
#define USB_RCPT_EP 2
//!@}

/*!
\defgroup grp_control_packet_type Control packet types
\ingroup grp_public_control

SETUP (control) packet type.  These are the same as those listed in the USB specification.
@{
*/
//! Standard packet
#define USB_CTL_TYPE_STD 0
//! Class packet
#define USB_CTL_TYPE_CLASS 1
//! Vendor-specific packet
#define USB_CTL_TYPE_VENDOR 2
//!@}

//! USB data packet
typedef usb_data_t *usb_buffer_t;

extern usb_data_t usb_ctl_write_data[];

//! USB setup packet
/*! usb_setup_t holds all of the fields in a USB setup packet, together with a few fields used by the stack for data phases.  It is passed to control callbacks.

The fields in this structure carry all the information needed to deduce a transaction's progress at any time.  In a setup write transaction, assuming the host is behaving properly, you can find out the following:

- If there is no data phase, #len is zero.
- If there is a data phase, #len is nonzero.
- If the data phase is split, #len is nonzero and greater than #datalen.
- You have reached the last packet in the data phase when 
  <tt>bytect + USB_CTL_PACKET_SIZE</tt> is greater than #len.

For a setup read transaction:

- If there is no data phase, #len is zero.
- If there is a data phase, #len is nonzero, and you must supply data.
- If #len is greater than <tt>USB_CTL_PACKET_SIZE</tt>, you must split 
  the transaction.

There are two ways to run a split data read transaction.  The first method is to point #data at the beginning of your buffer and set #datalen to the total amount of data you want to return.  If #datalen is longer than \c USB_CTL_PACKET_SIZE, the stack will automatically split the packet by "walking" it.  If the host has requested \e less data than you have to send, only that amount of data will be sent.

The second method requires a little more work, but is useful when you need
more control, or if you are generating data on the fly or reading it from a
special peripheral (to mention two examples). The trick is this: if #datalen 
is equal to \c USB_CTL_PACKET_SIZE, \e and #len is 
longer than #datalen, the stack will send #datalen bytes, and it will then 
call you, asking for more data.  It will keep calling you until it has sent 
#len bytes or until you set #datalen to something less than USB_CTL_PACKET_SIZE, 
which causes the stack to send a short packet, which ends the transaction.

Before the stack calls you, if fills out the setup structure with values it thinks are
appropriate.  You are free to alter (some of) these values at
that time, and the stack will honour your alterations (as long as they make
sense)!  The values you can alter are #data, #dataofs, and #datalen. You should 
\e never alter any of the standard USB fields, and you should <em>never, 
ever alter the #bytect field;</em> doing the former will probably get you into
serious trouble, and doing the latter will definitely get you into serious 
trouble.

The #dataofs field may seem a bit of a waste, but it's needed during read 
transactions for systems 
which use byte packing to store USB packet data, where it can save you a 
lot of work.  If portability is a 
concern, you should always favour it over merely altering #data.  
See the documentation for #dataofs for details.

\ingroup grp_public_control
*/
typedef struct usb_setup_t {
	unsigned int
		//! Data direction
		/*!
		\arg \c 0 Host transmits, device receives (setup write).
		\arg \c 1 Host receives, device transmits (setup read).
		*/
		dataDir:1,
		type:2,
		recipient:5;
	//! USB bRequest field
	/*! Same as bRequest in the standard. */
	u8 request;
	//! USB wValue field
	/*! Same as wValue in the standard. */
	u16 value;
	//! USB wIndex field
	/*! Same as wIndex in the standard. */
	u16 index;
	//! USB wLength field
	/*! This is the number of bytes requested or the number of bytes 
	to be transmitted.  It comes from the host, and corresponds to the 
	wLength field in the standard.  This number is not altered during 
	a transaction.
	*/
	u16 len;
} usb_setup_t;

typedef struct usb_endpoint_t usb_endpoint_t;
typedef struct usb_alarm_t usb_alarm_t;

//! Transaction complete callback
/*! Optionally called when a transaction completes, when passed to usb_move().

\p ptr is a pointer chosen by the user, which can be used (for example) to identify the transaction.  \p stat reports the status of the transfer and can be one of the following:

 - \a 0 Transfer completed, no problems
 - \a -1 Timeout occured; transfer did not complete

This may be called at interrupt time, so it should execute quickly.

\param[in] ptr User pointer
\param[in] stat Status code

\sa usb_move()
\ingroup grp_public_io
*/
typedef void (*usb_cb_done)(void *ptr, int stat);

//! SOF callback
/*! Called when a SOF (start-of-frame) token is received.  This is called at 
interrupt time.

\ingroup grp_public_io
*/
typedef void (*usb_cb_sof)(void);

//! USB state change callback
/*! USB state change notification callback.  Called whenever the USB state changes, and after the state change has actually occurred.

This is set with usb_set_state_cb().

\sa usb_set_state_cb(), grp_states
\ingroup grp_public_support
*/
typedef void (*usb_cb_state)(int state);

//! Total space occupied by a USB buffer
/*! This macro evaluates to the total amount of space occupied by a 
USB buffer, including the length byte(s).  The length is a size_t, 
and can be passed to malloc.

\param len Number of USB bytes to make space for.  The actual size of the 
declared structure, in units of size_t, may be different.

\ingroup grp_public_io
*/
#define usb_buf_sizeof(len) (sizeof(usb_data_t[usb_mem_len(len)+USB_BUF_LEN_SIZE]))

//! Static USB data buffer declarator
/*! This macro provides a convenient way to declare a static USB 
packet buffer of a fixed size.

The buffer is not initialised in any way.  This means that the length 
will be incorrect; you should generally call usb_buf_set_len() on it 
before using it.

\param name Buffer name, which becomes the name of the buffer structure.
\param len Number of USB bytes to allocate for.  The actual size of the 
declared structure, in units of size_t, may be different.  To find 
the actual size, use usb_buf_sizeof().

\ingroup grp_public_io
*/
#define USB_BUF_STATIC(name,len) usb_data_t name[usb_mem_len(len)+USB_BUF_LEN_SIZE]

//! Writable endpoint structure
/*! This structure is pointed to by usb_endpoint_t, and is stored in RAM.  It contains primarily status information.

As each endpoint can sustain only one complete transaction at a time, the information for it is kept here.  \a buf, \a reqlen, and \a actlen keep the necessary information.  Note that this is for the current \e transaction, and not for a single \e packet.  A transaction may be made of several packets.

The \c hwdata field is useful for ports that need to store additional information for endpoints.  In particular, \c hwdata is often used for storing the active packet request, or a pointer to a queue of packet requests.

There is no dedicated packet request field, since some ports use a queue of packet requests, and some require only one.

Users of the external PORUS API never see this structure; it is used only by the lower-level layers.

\ingroup grp_private
*/
struct usb_endpoint_data_t {
	//unsigned int xferInProgress:1;
	//! Endpoint status
	unsigned int stat:3,
		//! Timeout status
		timed_out:1;
	//! Time of last transaction; units port-dependent
	u32 check_time;
	//! Timeout in milliseconds; 0 = no timeout
	u16 timeout;
	//! Status callback
	//usb_cb_epstat epstat_cb;
	//! Transaction complete callback
	usb_cb_done done_cb;
	//! Callback pointer
	void *cb_ptr;
	//! Pointer to data buffer
	usb_data_t *buf;
	//! Buffer length in bytes
	u32 reqlen,
	//! Number of bytes having completed transmission or reception
		actlen;
	//! Generic pointer for port use
	void *hwdata;
};

typedef struct usb_endpoint_data_t usb_endpoint_data_t;

//! Endpoint structure
/*! Contains status and configuration information for one endpoint.  It has a constant part and a writable part, usb_endpoint_data_t.  The constant part is generated by the PORUS configuration program usbgen; the writable part is used in several places, and includes a variable which the port can use.

usb_endpoint_t is always declared constant in the file generated by the PORUS configurator, so that it can be stored in ROM.  One structure is declared for each endpoint listed in the configuration file (no structures are declared for the control endpoints).  For each usb_endpoint_t structure, a companion usb_endpoint_data_t is declared, which is writable and contains status information.

Users of the external PORUS API never see this structure; it is used only by the lower-level layers.

\ingroup grp_private
*/
struct usb_endpoint_t {
	unsigned int
		//! Endpoint number
		/*! Endpoint number.  Range is 1-31, excepting 16, which is a control endpoint.
		
		The endpoint's direction is indicated by this field: if the high bit (4) is set, the endpoint is IN; otherwise the endpoint is OUT.
		*/
		id:5, 
		//! Endpoint type
		/*! Endpoint type.  One of the following:
		\arg \c USB_EPTYPE_CONTROL Control endpoint (not used)
		\arg \c USB_EPTYPE_BULK Bulk endpoint
		\arg \c USB_EPTYPE_INTERRUPT Interrupt endpoint
		\arg \c USB_EPTYPE_ISOCHRONOUS Isochronous endpoint
		*/
		type:2; // endpoint type
	//! Maximum packet size
	/*! The maximum allowable packet size for this endpoint, in bytes. 
	
	This is important for endpoint configuration, as this must usually be passed to the hardware.
	*/
	unsigned short packetSize; // in bytes
	//! Pointer to writable section
	/*! Points to a writable usb_endpoint_data_t structure in RAM.
	*/
	usb_endpoint_data_t *data;
	int in_timeout; // in frames
	//! Next endpoint structure, or 0
	usb_endpoint_t *next;
};

#endif
