
#include "usbhw.h"

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

//void usb_set_state_unsuspend(void);
//void usb_set_state(int state);
//void usb_set_address(u8 adr);
//int usb_set_config(int cfn);

usb_setup_t usb_setup;

static usb_data_t txbuf[4];

static volatile struct {
	int state;
	int ct; // number of bytes received / transmitted
	// these are only used for read txns:
	int ofs; // offset into tx data
	usb_data_t *txdata; // pointer to transmit data
	int txlen; // number of bytes to transmit
} ctlflags;

static void reply_u8(u8 data)
{
	txbuf[0]=data<<8;
	usb_ctl_read_end(0,1,txbuf);
}

static void reply_u16(u16 data)
{
	txbuf[0]=(data&0xff)<<8;
	txbuf[0]|=(data>>8)&0xff;
	usb_ctl_read_end(0,2,txbuf);
}

static int usb_ctl_std_get_configuration(void)
{
	if (usb_setup.recipient!=USB_RCPT_DEV) return -1;
	if (usb_setup.len!=1) return -1;
	if (usb_get_state()==USB_STATE_ADDRESS) {
		reply_u8(0);
	} else if (usb_get_state()==USB_STATE_CONFIGURED) {
		reply_u8(usb_get_config());
	} else
		return -1;
	return 0;
}

static int usb_ctl_std_set_configuration(void)
{
	if (usb_setup.len) return -1;
	if (usb_setup.index) return -1;
	return usb_set_config(usb_setup.value);
}

static int usb_ctl_std_get_status(void)
{
	u16 data;
	int ret, epn;

	if (usb_get_state()<=USB_STATE_DEFAULT) return -1;
	if (usb_setup.len!=2) return -1;

	switch(usb_setup.recipient) {
	case USB_RCPT_DEV:
		data=0x100; // ### TODO: support remote wakeup / self powered etc.
		reply_u16(data);
		break;
	case USB_RCPT_IFACE:
		if (usb_get_state()!=USB_STATE_CONFIGURED) return -1;
		if (!usb_have_iface(1,usb_setup.index)) return -1;
		reply_u16(0);
		break;
	case USB_RCPT_EP:
		if (usb_get_state()==USB_STATE_ADDRESS)
			if (usb_setup.index!=0) return -1;
		epn=usb_setup.index;
		if (epn&0x80) epn=(epn&15)+8;
		ret=usb_is_stalled(epn);
		if (ret<0) return -1;
		reply_u16(ret==1?0x100:0);
		break;
	default:
		return -1;
	}
	return 0;
}

static int usb_ctl_std_clear_feature(void)
{
	int epn;

	switch(usb_setup.value) {
	case FEATURE_ENDPOINT_HALT:
		if (usb_setup.recipient!=USB_RCPT_EP) return -1;
		epn=usb_setup.index;
		if (epn&0x80) epn=(epn&15)+8;
		if (usb_unstall(epn))
			return -1;
		break;
	case FEATURE_DEVICE_REMOTE_WAKEUP:
		if (!(usb_config_features(1)&2)) return -1;
		if (usb_setup.recipient!=USB_RCPT_DEV) return -1;
		//usb_enable_remote_wakeup(1);
		break;
	default:
		return -1;
	}
	return 0;
}

static int usb_ctl_std_set_feature(void)
{
	int epn;

	switch(usb_setup.value) {
	case FEATURE_ENDPOINT_HALT:
		if (usb_setup.recipient!=USB_RCPT_EP) return -1;
		epn=usb_setup.index;
		if (epn&0x80) epn=(epn&15)+8;
		if (usb_stall(epn))
			return -1;
		break;
	case FEATURE_DEVICE_REMOTE_WAKEUP:
		if (!(usb_config_features(1)&2)) return -1;
		if (usb_setup.recipient!=USB_RCPT_DEV) return -1;
		//usb_enable_remote_wakeup(0);
		break;
	default:
		return -1;
	}
	return 0;
}

static int usb_ctl_std_set_address(void)
{
	if (usb_setup.index) return -1;
	if (usb_setup.len) return -1;
	if (usb_setup.value>127) return -1;
	if (usb_get_state()==USB_STATE_DEFAULT) {
		if (!usb_setup.value) return 0;
		usb_set_address(usb_setup.value);
	} else if (usb_get_state()==USB_STATE_ADDRESS) {
		usb_set_address(usb_setup.value);
	} else
		return -1;
	return 0;
}

static int usb_ctl_std_get_interface(void)
{
	if (!usb_setup.dataDir) return -1;
	if (usb_setup.value) return -1;
	if (usb_setup.recipient!=USB_RCPT_IFACE) return -1;
	if (usb_setup.len!=1) return -1;
	if (usb_get_state()!=USB_STATE_CONFIGURED) return -1;
	reply_u8(1); // ### NOTE: change this for alt setting support
	return 0;
}

static int usb_ctl_std_get_descriptor(void)
{
	int desclen;
	usb_data_t *buf;

	if (!usb_setup.dataDir) return -1;
	if (usb_setup.recipient!=USB_RCPT_DEV) return -1;
	switch((usb_setup.value>>8)&0xff) {
	case USB_DESC_DEVICE:
		if (usb_setup.index) return -1;
		usb_get_device_desc(&buf,&desclen);
		break;
	case USB_DESC_CONFIGURATION:
		if (usb_setup.index) return -1;
		if (usb_get_config_desc(usb_setup.value&0xff,&buf,&desclen)) return -1;
		break;
	case USB_DESC_STRING:
		if (usb_get_string_desc(usb_setup.value&0xff,usb_setup.index,&buf,&desclen)) return -1;
		break;
	default:
		return -1;
	}
	usb_ctl_read_end(0,desclen,buf);
	return 0;
}

static void usb_ctl_std_read(void)
{
	int err;

	switch(usb_setup.request) {
	case USB_REQ_GET_STATUS:
		err=usb_ctl_std_get_status();
		break;
	case USB_REQ_GET_DESCRIPTOR:
		err=usb_ctl_std_get_descriptor();
		break;
	case USB_REQ_GET_CONFIGURATION:
		err=usb_ctl_std_get_configuration();
		break;
	case USB_REQ_GET_INTERFACE:
		err=usb_ctl_std_get_interface();
		break;
	//case USB_REQ_SYNCH_FRAME:
	default:
		err=-1;
		break;
	}
	if (err)
		usb_ctl_read_end(1,0,0);
}

static void usb_ctl_std_write(void)
{
	int err;

	switch(usb_setup.request) {
	case USB_REQ_CLEAR_FEATURE:
		err=usb_ctl_std_clear_feature();
		break;
	case USB_REQ_SET_FEATURE:
		err=usb_ctl_std_set_feature();
		break;
	case USB_REQ_SET_ADDRESS:
		err=usb_ctl_std_set_address();
		break;
	case USB_REQ_SET_CONFIGURATION:
		err=usb_ctl_std_set_configuration();
		break;
	//case USB_REQ_SET_INTERFACE:
	//case USB_REQ_SET_DESCRIPTOR:
	default:
		err=-1;
		break;
	}
	if (err)
		usb_ctl_write_end(1);
	else
		usb_ctl_write_end(0);
}

void usb_ctl_stall(void)
{
	ctlflags.state=USB_CTL_STATE_IDLE;
	usbhw_ctl_stall();
}

int usb_ctl_std(void)
{
	if (ctlflags.state!=USB_CTL_STATE_RRS&&ctlflags.state!=USB_CTL_STATE_RWD) {
		usb_ctl_stall();
		return -1;
	}
	if (usb_setup.type!=USB_CTL_TYPE_STD)
		return 0;
	else {
		if (usb_setup.dataDir)
			usb_ctl_std_read();
		else
			usb_ctl_std_write();
		return 1;
	}
}

void usb_evt_ctl_rx(void)
{
	u8 l;

	if (ctlflags.state!=USB_CTL_STATE_WWD) {
		usb_ctl_stall();
		return;
	}
	l=usb_setup.len-ctlflags.ct;
	if (l>USB_CTL_PACKET_SIZE) l=USB_CTL_PACKET_SIZE;
	if (usbhw_get_ctl_write_data(&l,usb_ctl_write_data+usb_mem_len(ctlflags.ct))) {
		usb_ctl_stall();
		return;
	}
	ctlflags.ct+=l;
	if (ctlflags.ct>=usb_setup.len) {
		ctlflags.state=USB_CTL_STATE_RWD;
		usb_ctl();
	}
}

void usb_evt_ctl_tx(void)
{
	int l;

	if (ctlflags.state!=USB_CTL_STATE_SRD) {
		usb_ctl_stall();
		return;
	}
	l=ctlflags.txlen-ctlflags.ct;
	if (l>USB_CTL_PACKET_SIZE) l=USB_CTL_PACKET_SIZE;
	if (l<0) l=0;
	if (l) {
		usbhw_put_ctl_read_data(l,ctlflags.txdata+usb_mem_len(ctlflags.ct+ctlflags.ofs));
		ctlflags.ct+=l;
	}
	else if (!l||(ctlflags.ct>=usb_setup.len)) {
		usbhw_ctl_read_handshake();
		ctlflags.state=USB_CTL_STATE_IDLE;
	}
}

void usb_ctl_read_end(int stat, int len, usb_data_t *data)
{
	if (ctlflags.state!=USB_CTL_STATE_RRS||!len||!data||stat) {
		usb_ctl_stall();
		return;
	}
	ctlflags.state=USB_CTL_STATE_SRD;
	ctlflags.ct=0;
	ctlflags.ofs=0;
	ctlflags.txdata=data;
	if (len>usb_setup.len) len=usb_setup.len;
	ctlflags.txlen=len;
	usb_evt_ctl_tx();
}

void usb_ctl_write_end(int stat)
{
	if (ctlflags.state!=USB_CTL_STATE_RWD||stat) {
		usb_ctl_stall();
		return;
	}
	usbhw_ctl_write_handshake();
	ctlflags.state=USB_CTL_STATE_IDLE;
}

void usb_evt_setup(void)
{
	if (ctlflags.state!=USB_CTL_STATE_IDLE) {
		usb_ctl_stall();
		return;
	}
	if (usbhw_get_setup(&usb_setup)) {
		usb_ctl_stall();
		return;
	}

	ctlflags.ct=0;

	if (usb_setup.dataDir) { // read txn
		ctlflags.state=USB_CTL_STATE_RRS;
		usb_ctl();
	} else {		// it's a write txn
		if (usb_setup.len>USB_CTL_WRITE_BUF_SIZE) {
			usb_ctl_stall();
			return;
		}
		if (usb_setup.len) {	// we'll get data
			ctlflags.state=USB_CTL_STATE_WWD;
			// no dispatch now, wait for OUTs
		} else {		// not expecting data
			ctlflags.state=USB_CTL_STATE_RWD;
			usb_ctl();	// dispatch now
		}
	}
}

void usb_ctl_init(void)
{
	ctlflags.state=USB_CTL_STATE_IDLE;
}
