
#ifndef GUARD_usbtypes_h
#define GUARD_usbtypes_h

#include "types.h"

//! USB states
/*!
These macros define codes for each state that the stack can be in.  
With one exception (USB_STATE_WILL_ADDRESS), they correspond to states 
described in the USB standard.
*/
//@{

/*!
This state is mentioned in the standard, but we currently do not 
use it; we go straight to POWERED, since without power we can't function
at all, so the ATTACHED state cannot occur.
*/
#define USB_STATE_ATTACHED 0

/*!
This is the state we enter when we attach to the bus.  usb_connect() 
enters this state.
*/
#define USB_STATE_POWERED 1

/*!
We enter this state after a bus reset.
*/
#define USB_STATE_DEFAULT 2

/*!
The stack moves to this state when an address setup sequence is completed.
Note that the stack moves through USB_STATE_WILL_ADDRESS on its way to  
this state.

\sa USB_STATE_WILL_ADDRESS
*/
#define USB_STATE_ADDRESS 3

/*!
The stack moves to this state after receiving a configuration set packet.
*/
#define USB_STATE_CONFIGURED 4

/*!
The stack moves to this state when it detects a bus suspend.  It moves 
back to the previous state after the suspend ends.
*/
#define USB_STATE_SUSPENDED 5

/*!
This is an internal state used for address-setting.  When the stack 
receives a set address request, it goes into this state instead of 
USB_STATE_ADDRESS, since the stack cannot actually change the device 
address until the status phase is complete.  When the status phase 
completes in USB_STATE_WILL_ADDRESS, the stack changes the device address 
and moves to USB_STATE_ADDRESS.

This state is not part of the USB standard documentation.
*/
#define USB_STATE_WILL_ADDRESS 6

/*!
This state is not named in the USB standard, but the 
standard does describe a state in which the device is not attached to 
the bus.

This state is assigned by usb_init() and usb_disconnect().
*/
#define USB_STATE_DETACHED 7
//@}

//! Endpoint types
//@{
#define USB_EPTYPE_CONTROL 0
#define USB_EPTYPE_BULK 1
#define USB_EPTYPE_INTERRUPT 2
#define USB_EPTYPE_ISOCHRONOUS 3
//@}

#define USB_DEVEV_BUS_EVENT 0
#define USB_DEVEV_STATE_CHANGE 1

//! Control transaction phases
/*! See usb_setup_t#phase for information about these. */
//@{
#define USB_CTL_PHASE_SETUP 0
#define USB_CTL_PHASE_OUT 1
#define USB_CTL_PHASE_IN 2
#define USB_CTL_PHASE_STATUS 3
//@}

#define USB_RCPT_DEV 0
#define USB_RCPT_IFACE 1
#define USB_RCPT_EP 2

#define USB_CTL_TYPE_STD 0
#define USB_CTL_TYPE_CLASS 1
#define USB_CTL_TYPE_VENDOR 2

//! USB data packet
typedef usb_data_t *usb_buffer_t;

//! USB setup packet
/*! usb_setup_t holds all of the fields in a USB setup packet, 
together with a 
few fields used by the stack for data phases.  It is passed to control 
callbacks.

The fields in this structure carry all the information needed to deduce 
a transaction's progress at any time, but the way to do this may not be 
obvious.  In a setup write transaction, assuming the host is behaving properly, 
you can find out the following:

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

There are two ways to run a split data read transaction.  The first way 
is quite easy: simply point #data at the beginning of your buffer, set 
#datalen to the total amount of data you want to return, and 
let the stack do the rest.  If #datalen is longer than \c USB_CTL_PACKET_SIZE,
the stack will automatically split your packet by "walking" it.
If the host has requested \e less data than you have to send, don't worry: 
the stack takes care of this case for you too.

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
		recipient:5,
		//! Transaction phase
		/*! This is provided as an aid to control callbacks.  It 
		identifies to the callback what part of the phase it's 
		being called in.  It can take on one of three values:

		\arg \c USB_CTL_PHASE_SETUP The call is being made as a 
		result of a setup packet, and possibly a data OUT packet 
		as well (but only one).  When you get this, you should 
		process and decode the setup packet fields.

		\arg \c USB_CTL_PHASE_OUT Extra data, part of a long 
		write transaction.  You only get this when there is more 
		than one OUT packet as part of a data phase, and it starts 
		on the second OUT; on the first OUT, USB_CTL_PHASE_SETUP 
		is used.

		\arg \c USB_CTL_PHASE_IN An IN is processed.  
		You should update the data fields, if necessary.  You don't 
		always get this; see usb_setup_t for more information.

		This field is neither in the setup packet nor in the 
		standard.
		*/
		phase:2;
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
	//! Data sequence number
	/*! Number of bytes transmitted or received in the data phase of 
	this transaction.
	
	\warning Treat this field as read-only; altering it will disrupt the 
	transaction state machine.

	This field is neither in the setup packet nor in the standard.
	*/
	u16 datact;
	//! Current packet length
	/*! This is the number of bytes of data currently stored in #data.  
	<b>It is not the same as #len.</b>

	For read transactions, if you set this field to zero, the stack 
	will return a zero-length packet; this ends the transaction.  See 
	usb_setup_t for information on why you would want to do this during 
	a read.

	The stack does not alter this field during a read transaction, 
	except that for clamping it to \c USB_CTL_PACKET_SIZE.
	It \e does alter it during a write transaction.

	If #data is in packed-byte format, this 
	field will not reflect its true length; use usb_mem_len() to 
	calculate the actual data length.  For portability, you should 
	always use usb_mem_len().

	Note that if a transaction uses multiple data packets, this 
	number will always be shorter than #len.  See usb_setup_t for 
	details.

	This field is neither in the setup packet nor in the standard.
	*/
	u16 datalen;
	//! Data offset
	/*! Offset into #data in bytes.  This is used only for setup 
	reads; for setup writes, this field is always zero, and you 
	can ignore it.

	For read transactions, you can use this field to provide a byte-valued offset 
	into #data, even on systems where the buffer is in packed-byte 
	format.  You should almost always use #dataofs instead of 
	pointing #data to different places in a buffer; this helps keep 
	your code portable.
	*/
	u16 dataofs;
	//! Data pointer
	/*! Points to data received, or to data to be transmitted.
	
	During a write transaction with a data phase, this points to an 
	internal buffer containing the received data.  It may 
	be zero if there is no data phase, but do not rely on this.
	
	For a read transaction, you should point this to a buffer containing 
	the data to be transmitted.  If you set it to zero, the stack will 
	NAK the next IN token.  The stack will not alter this pointer 
	during a read transaction; it trusts you to set it appropriately.

	This field is neither in the setup packet nor in the standard.
	*/
	usb_data_t *data;
} usb_setup_t;

//! Receive callback
/*!
\param[in] ep Endpoint number.  Useful when multiple endpoints call the same callback.
\param[in] buf Pointer to the packet that was just received, or 0.
\return Pointer to a packet structure which is ready to receive data, or 0.
*/
typedef usb_buffer_t (*usb_cb_out)(int ep, usb_buffer_t buf);

//! Transmit callback
/*!
\param[in] ep Endpoint number.  Useful when multiple endpoints call the same callback.
\param[in] buf Pointer to the packet that was just transmitted, or 0.
\return Pointer to a new packet to transmit, or 0.
*/
typedef usb_buffer_t (*usb_cb_in)(int ep, usb_buffer_t buf);

//! SOF callback
/*! Called when a SOF (start-of-frame) token is received.  This is called at 
interrupt time.
*/
typedef void (*usb_cb_sof)(void);

//! Control endpoint callback
/*!
\param[in] s Setup packet.
\retval 0 Success.
\retval -1 Error.  Returning this causes a protocol stall on the control 
endpoints.
*/
typedef int (*usb_cb_ctl)(usb_setup_t *s);

typedef void (*usb_cb_state)(int state);

typedef struct usb_endpoint_data_t {
	union {
		usb_cb_out o;
		usb_cb_in i;
	} cb;
	unsigned int goBusy:1,
		reloadBusy:1;
	usb_data_t *go;
	usb_buffer_t reload;
} usb_endpoint_data_t;

typedef struct usb_endpoint_t {
	unsigned int id:4, // endpoint number, 0-15
		type:2; // endpoint type
	unsigned short packetSize; // in bytes
	usb_endpoint_data_t *data;
	int in_timeout; // in frames
} usb_endpoint_t;

/*! \defgroup usbdata USB data manipulation
*/
/*!@{*/
//! Total space occupied by a USB buffer
/*! This macro evaluates to the total amount of space occupied by a 
USB buffer, including the length byte(s).  The length is a size_t, 
and can be passed to malloc.

\param len Number of USB bytes to make space for.  The actual size of the 
declared structure, in units of size_t, may be quite different.
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
declared structure, in units of size_t, may be quite different.  To find 
this size, use usb_buf_sizeof().
*/
#define USB_BUF_STATIC(name,len) usb_data_t name[usb_mem_len(len)+USB_BUF_LEN_SIZE]

/*! \fn size_t usb_mem_len(int l)
\brief Calculate words needed to contain USB bytes

This function calculates the number of machine words needed to contain 
\a l packed bytes.

On machines with octet-addressable memory, this returns \a l.  On machines 
with word and long-word-addressable memory, this calculates the number of 
complete words needed to contain \a l bytes after packing.

This function is typically implemented as a macro, and is generated by 
usbgen for the target machine.  It is defined in usbconfig.h.
*/

/*! \def USB_BUF_LEN_SIZE
\brief Size of a buffer length

This is the number of words which a buffer length occupies.
It is mainly used internally.

This is generated by 
usbgen for the target machine, and is defined in usbconfig.h.
*/

/*! \def USB_CTL_PACKET_SIZE
\brief Maximum length of a control packet

Maximum length of a control packet, in bytes.

This is generated by 
usbgen for the target machine, and is defined in usbconfig.h.
*/

/*! \fn unsigned short usb_buf_len(usb_buffer_t buf)
\brief Number of bytes stored in a USB buffer

Given a buffer \a buf, returns the number of bytes currently stored in it.  
(This is \e not the maximum or allocated length of the buffer.)

\note \a len is always in bytes, regardless of memory width.

This function is typically implemented as a macro, and is generated by 
usbgen for the target machine.  It is defined in usbconfig.h.

\sa usb_buf_set_len()
*/

/*! \fn void usb_buf_set_len(usb_buffer_t buf, unsigned short len)
\brief Set the length field of a USB buffer

Given a buffer \a buf, sets its length field to \a len.  The length field 
represents the number of bytes stored in the field.

\note \a len is always in bytes, regardless of memory width.

\param len Length field in bytes.

This function is typically implemented as a macro, and is generated by 
usbgen for the target machine.  It is defined in usbconfig.h.

\sa usb_buf_len()
*/

/*! \fn usb_data_t *usb_buf_data(usb_buffer_t buf)
\brief Retrieve data pointer for a USB buffer

Returns a pointer to the beginning of the data field of a USB buffer.

The word format of a USB buffer is system dependent; the data type is 
usb_data_t.
*/
/*!@}*/

#endif
