
#ifndef GUARD_usb_h
#define GUARD_usb_h

// usbconfig.h is a generated file
#include "usbconfig.h"

//! Initialisation function
void usb_init(void);

void usb_set_sof_cb(usb_cb_sof cb);
void usb_set_presof_cb(usb_cb_sof cb);

int usb_stall(u8 epnum);
int usb_unstall(u8 epnum);
int usb_is_stalled(u8 epnum);

int usb_get_state(void);
int usb_get_config(void);

int usb_tx(u8 epnum, usb_buffer_t data);

void usb_dev_reset(void);
void usb_attach(void);
void usb_detach(void);
int usb_is_attached(void);
void usb_remote_wakeup(void);

int usb_set_out_cb(int cfg, int epn, usb_cb_out cb);
int usb_set_in_cb(int cfg, int epn, usb_cb_in cb);

//void usb_sched(void);


#endif
