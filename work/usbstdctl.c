
#include "usb.h"

#define USB_DESC_DEVICE 1
#define USB_DESC_CONFIGURATION 2
#define USB_DESC_STRING 3
#define USB_DESC_INTERFACE 4
#define USB_DESC_ENDPOINT 5
#define USB_DESC_DEVICE_QUALIFIER 6
#define USB_DESC_OTHER_SPEED_CONFIGURATION 7
#define USB_DESC_INTERFACE_POWER 8

#define FEATURE_ENDPOINT_HALT 0
#define FEATURE_DEVICE_REMOTE_WAKEUP 1
#define FEATURE_TEST_MODE 2

#define USB_REQ_GET_STATUS 0
#define USB_REQ_CLEAR_FEATURE 1
#define USB_REQ_SET_FEATURE 3
#define USB_REQ_SET_ADDRESS 5
#define USB_REQ_GET_DESCRIPTOR 6
#define USB_REQ_SET_DESCRIPTOR 7
#define USB_REQ_GET_CONFIGURATION 8
#define USB_REQ_SET_CONFIGURATION 9
#define USB_REQ_GET_INTERFACE 10
#define USB_REQ_SET_INTERFACE 11
#define USB_REQ_SYNCH_FRAME 12

void usb_set_state_unsuspend(void);
void usb_set_state(int state);
void usb_set_address(u8 adr);
int usb_set_config(int cfn);

static usb_cb_ctl classCtlCallback, vendorCtlCallback;
static usb_cb_ctl vendorCtlWriteCallback, vendorCtlReadCallback;

usb_data_t txbuf[4];

static void reply_u8(usb_setup_t *setup, u8 data)
{
	txbuf[0]=data<<8;
	setup->data=txbuf;
	setup->datalen=1;
}

static void reply_u16(usb_setup_t *setup, u16 data)
{
	txbuf[0]=(data&0xff)<<8;
	txbuf[0]|=(data>>8)&0xff;
	setup->data=txbuf;
	setup->datalen=2;
}

static int usb_ctl_std_get_configuration(usb_setup_t *setup)
{
	if (!setup->dataDir) return -1;
	if (setup->recipient!=USB_RCPT_DEV) return -1;
	if (setup->len!=1) return -1;
	if (usb_get_state()==USB_STATE_ADDRESS) {
		reply_u8(setup,0);
	} else if (usb_get_state()==USB_STATE_CONFIGURED) {
		reply_u8(setup,usb_get_config());
	} else
		return -1;
	return 0;
}

static int usb_ctl_std_set_configuration(usb_setup_t *setup)
{
	if (setup->dataDir) return -1;
	if (setup->len) return -1;
	if (setup->index) return -1;
	return usb_set_config(setup->value);
}

static int usb_ctl_std_get_status(usb_setup_t *setup)
{
	u16 data;
	int ret, epn;

	if (usb_get_state()<=USB_STATE_DEFAULT) return -1;
	if (!setup->dataDir) return -1;
	if (setup->len!=2) return -1;

	switch(setup->recipient) {
	case USB_RCPT_DEV:
		//### FIX ME!!
		//data=flags.enableRemoteWakeup?0x200:0;
		//data|=flags.selfPowered?0x100:0;
		data=0x100;
		reply_u16(setup,data);
		break;
	case USB_RCPT_IFACE:
		if (usb_get_state()!=USB_STATE_CONFIGURED) return -1;
		if (!usb_have_iface(1,setup->index)) return -1;
		reply_u16(setup,0);
		break;
	case USB_RCPT_EP:
		if (usb_get_state()==USB_STATE_ADDRESS)
			if (setup->index!=0) return -1;
		epn=setup->index;
		if (epn&0x80) epn=(epn&15)+8;
		ret=usb_is_stalled(epn);
		if (ret<0) return -1;
		reply_u16(setup,ret==1?0x100:0);
		break;
	default:
		return -1;
	}
	return 0;
}

static int usb_ctl_std_clear_feature(usb_setup_t *setup)
{
	int epn;

	if (setup->dataDir) return -1;
	
	switch(setup->value) {
	case FEATURE_ENDPOINT_HALT:
		if (setup->recipient!=USB_RCPT_EP) return -1;
		epn=setup->index;
		if (epn&0x80) epn=(epn&15)+8;
		if (usb_unstall(epn))
			return -1;
		break;
	case FEATURE_DEVICE_REMOTE_WAKEUP:
		if (!(usb_config_features(1)&2)) return -1;
		if (setup->recipient!=USB_RCPT_DEV) return -1;
		//usb_enable_remote_wakeup(1);
		break;
	default:
		return -1;
	}
	return 0;
}

static int usb_ctl_std_set_feature(usb_setup_t *setup)
{
	int epn;

	if (setup->dataDir) return -1;

	switch(setup->value) {
	case FEATURE_ENDPOINT_HALT:
		if (setup->recipient!=USB_RCPT_EP) return -1;
		epn=setup->index;
		if (epn&0x80) epn=(epn&15)+8;
		if (usb_stall(epn))
			return -1;
		break;
	case FEATURE_DEVICE_REMOTE_WAKEUP:
		if (!(usb_config_features(1)&2)) return -1;
		if (setup->recipient!=USB_RCPT_DEV) return -1;
		//usb_enable_remote_wakeup(0);
		break;
	default:
		return -1;
	}
	return 0;
}

static int usb_ctl_std_set_address(usb_setup_t *setup)
{
	if (setup->dataDir) return -1;
	if (setup->index) return -1;
	if (setup->len) return -1;
	if (setup->value>127) return -1;
	if (usb_get_state()==USB_STATE_DEFAULT) {
		if (!setup->value) return 0;
		usb_set_address(setup->value);
	} else if (usb_get_state()==USB_STATE_ADDRESS) {
		usb_set_address(setup->value);
	} else
		return -1;
	return 0;
}

static int usb_ctl_std_get_interface(usb_setup_t *setup)
{
	if (!setup->dataDir) return -1;
	if (setup->value) return -1;
	if (setup->recipient!=USB_RCPT_IFACE) return -1;
	if (setup->len!=1) return -1;
	if (usb_get_state()!=USB_STATE_CONFIGURED) return -1;
	reply_u8(setup,1); //### change this for alt setting support
	return 0;
}

static int usb_ctl_std_get_descriptor(usb_setup_t *setup)
{
	int desclen;
	usb_data_t *buf;

	if (!setup->dataDir) return -1;
	if (setup->recipient!=USB_RCPT_DEV) return -1;
	switch((setup->value>>8)&0xff) {
	case USB_DESC_DEVICE:
		if (setup->index) return -1;
		usb_get_device_desc(&buf,&desclen);
		break;
	case USB_DESC_CONFIGURATION:
		if (setup->index) return -1;
		if (usb_get_config_desc(setup->value&0xff,&buf,&desclen)) return -1;
		break;
	case USB_DESC_STRING:
		if (usb_get_string_desc(setup->value&0xff,setup->index,&buf,&desclen)) return -1;
		break;
	default:
		return -1;
	}
	setup->data=buf;
	setup->datalen=desclen;
	return 0;
}

static int usb_ctl_std(usb_setup_t *setup)
{
	int err;

	if (setup->phase!=USB_CTL_PHASE_SETUP) return -1;

	switch(setup->request) {
	case USB_REQ_GET_STATUS:
		err=usb_ctl_std_get_status(setup);
		break;
	case USB_REQ_CLEAR_FEATURE:
		err=usb_ctl_std_clear_feature(setup);
		break;
	case USB_REQ_SET_FEATURE:
		err=usb_ctl_std_set_feature(setup);
		break;
	case USB_REQ_SET_ADDRESS:
		err=usb_ctl_std_set_address(setup);
		break;
	case USB_REQ_GET_DESCRIPTOR:
		err=usb_ctl_std_get_descriptor(setup);
		break;
	case USB_REQ_GET_CONFIGURATION:
		err=usb_ctl_std_get_configuration(setup);
		break;
	case USB_REQ_SET_CONFIGURATION:
		err=usb_ctl_std_set_configuration(setup);
		break;
	case USB_REQ_GET_INTERFACE:
		err=usb_ctl_std_get_interface(setup);
		break;
	//case USB_REQ_SET_INTERFACE:
	//case USB_REQ_SYNCH_FRAME:
	//case USB_REQ_SET_DESCRIPTOR:
	default:
		err=-1;
		break;
	}
	return err;
}

void usb_set_ctl_class_cb(usb_cb_ctl cb)
{
	classCtlCallback=cb;
}

void usb_set_ctl_vendor_cb(usb_cb_ctl cb)
{
	vendorCtlCallback=cb;
}

void usb_set_ctl_vendor_write_cb(usb_cb_ctl cb)
{
	vendorCtlWriteCallback=cb;
}

void usb_set_ctl_vendor_read_cb(usb_cb_ctl cb)
{
	vendorCtlReadCallback=cb;
}

int usb_ctl(usb_setup_t *setup)
{
	switch(setup->type) {
	case USB_CTL_TYPE_STD:
		return usb_ctl_std(setup);
	case USB_CTL_TYPE_CLASS:
		if (classCtlCallback)
			return classCtlCallback(setup);
		break;
	case USB_CTL_TYPE_VENDOR:
		if (setup->dataDir&&vendorCtlReadCallback)
			return vendorCtlReadCallback(setup);
		else if (vendorCtlWriteCallback)
			return vendorCtlWriteCallback(setup);
		else if (vendorCtlCallback)
			return vendorCtlCallback(setup);
		break;
	default:
		break;
	}
	return -1;
}
