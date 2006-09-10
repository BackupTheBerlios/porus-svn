// :wrap=soft:

/* port/c55x/portconf.h -- configuration header for C55x port */

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

#ifndef GUARD_portconf_h
#define GUARD_portconf_h

//#define USBHW_HAVE_BLOCKING

/*! Define this if the port supports timeouts */
#define USBHW_HAVE_TIMEOUT

/*! Define this if the hardware can attach / deattach from the bus */
#define USBHW_HAVE_ATTACH

/*! Defined if the hardware supports auto-chaining */
#define USBHW_AUTO_CHAIN

//! C55X parameters
/*! This structure is used to initialise the C55X port. */
struct c55x_params {
	//! CLKIN frequency in kHz
	/*! The C5509 USB peripheral has its own PLL which is driven from the CLKIN pin (or crystal oscillator).  This parameter is the frequency on CLKIN in kHz.  It is used to calculate the multiplier for the PLL, which must generate a 48 MHz clock. */
	u32 clkin_khz;

	//! Microseconds per PRD tick
	/*! This parameter gives the number of microseconds for each PRD timer tick.  In GConf, it is the value marked "Microseconds/Int" in the property dialog for the CLK module. */
	u32 us_per_prd_tick;
};

typedef struct c55x_params c55x_params;

/*! \defgroup port_c55x

This port works with the USB peripheral on the Texas Instruments TMS320VC5509A and TMS320VC5507 DSPs, and is designed for use with DSP/BIOS.

This version has been built with DSP/BIOS 4.9.

This port does not use CSL.

\section usb_init() parameters

The struct c55x_params must be passed to usb_init().  It contains two fields: clkin_khz, which is used to set up the USB PLL, and us_per_prd_tick, which is used by the timeout mechanism.  See c55x_params for more information.

\section DSP/BIOS setup

This port may use MEM_alloc and MEM_free.  Segment 0 is used.

The DSP/BIOS configuration must include the following:

- HWI 8 must point to _usbhw_isr.

- A PRD object must be created to execute the function _usbhw_check_timeouts() (not a standard PORUS function).  The more often it runs, the more accurate timeouts will be, but the more processor time will be occupied.  50-100ms may be a good value to start with.

\section Implementation notes

This port is DMA driven and non-blocking.  It uses the undocumented ability of the USB peripheral to automatically produce chained packets on a single DMA transaction.  usb_evt_cpdone is therefore called only twice, since it need not participate in packet chaining.

\subsection Buffer count in buffer

The USB peripheral in the C5509 has a curious and unfortunate property.  When the DMA copies from the USB hardware to DSP memory, it inserts a word containing the actual transfer length at the beginning of the buffer.  The buffer is therefore two bytes (one word) longer than requested.

There is unfortunately no way to disable this "feature", save by copying the data manually from the USB peripheral, which, on the C5509, is a very inefficient operation.  Code which must run on the C5509 must therefore take this extra word into account when allocating buffers and processing data.

\subsection Timeouts

Timeouts are implemented using a watchdog function called by the PRD module, which the user must configure (see above).  This function checks for pending transactions on all endpoints which have timed out.  If it finds any, it cancels the transaction and sets the endpoint's timeout flag.

Timeout depends on the check_time field.  The watchdog function compares the current time to check_time, and if it is greater than the endpoint's timeout value, it times out the endpoint.

check_time is updated by the static function update_check_time(), which calls PRD_getticks.  check_time is therefore in units of ticks.  The watchdog function converts the timeout_ms field to ticks each time it is run (using ticks_to_ms()).

check_time is set at the following points:

- In the i/o functions
- At the DMA interrupt
- At IN and OUT interrupts

*/

void usbhw_pack55(u8 *src, u16 *dest, u16 len);
void usbhw_unpack55(u16 *src, u8 *dest, u16 len);

//#define USBHW_DMALOG

#ifdef USBHW_DMALOG

#define USBHW_DMALOG_ISRDMA 0
#define USBHW_DMALOG_DMAGO 1
#define USBHW_DMALOG_CANCEL 2
#define USBHW_DMALOG_MOVE 3
#define USBHW_DMALOG_MOVE_WAIT 4
#define USBHW_DMALOG_CPDONE 5
#define USBHW_DMALOG_TIMEOUT 6

typedef struct usbhw_dmalog_t {
	unsigned int nakx:1,
		naky:1,
		toggle:1,
		dmago:1,
		dmastp:1,
		dmaovf:1;
	u16 dmacount;
	u16 id;
	u16 ep;
} usbhw_dmalog_t;

//#define USBHW_DMA_LOGSIZE 64

extern volatile usbhw_dmalog_t *usbhw_dmalog;
void usbhw_dmalog_write(u8 id, u8 epn);

#else

#define usbhw_dmalog_write(x,y)

#endif

#endif
