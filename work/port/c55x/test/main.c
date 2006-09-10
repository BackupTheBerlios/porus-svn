
/* port/c55x/test/main.c -- main module for PORUS test on MMB0 
   (TMS320VC5507) */

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

#include "mmb0.h"
#include "usb.h"
#include "clk.h"
#include <stdlib.h>
#include <c55.h>
#include <mbx.h>

// commands
#define CMD_WVAR	0x01
#define CMD_RVAR	0x02
#define CMD_BLKI	0x03
#define CMD_BLKO	0x04
#define CMD_STAT	0x05
#define CMD_RCRC	0x06
#define CMD_TMOI	0x07
#define CMD_TMOO	0x08

// stat codes
#define STAT_IDLE	0x00
#define STAT_BORX	0x01
#define STAT_BOCC	0x02
#define STAT_BICC	0x03
#define STAT_BITX	0x04
#define STAT_TIMO	0x05
#define STAT_EROR	0x06

typedef struct msg_t {
	u16 msg;
	u32 arg1;
} msg_t;

//static MBX_Handle test_mbx;

static usb_data_t usbtxbuf[32];

static volatile struct {
	unsigned int test_stat:3,
		initted:1,
		err:1,
		timeout:1;
	int state;
	u16 var;
	u32 crc;
	usb_data_t *lastbuf;
} flags;

void delay(u32 c);

/*
static void longalign(u32 *adr)
{
	if (*adr&3) {
		*adr&=~((unsigned long)(3));
		*adr+=4;
	}
}
*/

static void *sys_malloc(size_t len)
{
	return MEM_alloc(0,len,0);
}

static void sys_free(void *mem)
{
	MEM_free(0,mem,0);
}

static u32 usbU32(usb_data_t *buf)
{
	u32 d;
	
	d=(u32)(*buf)<<16;
	d|=buf[1];
	return d;
}

static void usbPutU32(usb_data_t *buf, u32 arg)
{
	buf[0]=(arg>>16)&0xffff;
	buf[1]=(arg&0xffff);
}

static u16 usbU16(usb_data_t *buf)
{
	return (u16)(*buf);
}

static void usbPutU16(usb_data_t *buf, u16 arg)
{
	*(u16 *)buf=arg;
}

static u8 usbU8(usb_data_t *buf)
{
	return (u8)(*buf>>8);
}

static void usbPutU8(usb_data_t *buf, u8 arg)
{
	*(u16 *)buf=(arg<<8)&0xff00;
}

/* ------ Test thread */

// random gens from *DOCTOR* George Marsaglia
#define znew   (z=36969*(z&65535)+(z>>16))
#define wnew   (w=18000*(w&65535)+(w>>16))
#define MWC    ((znew<<16)+wnew )

/* Generates len packed bytes in buf */
void genrnd(void *buf, u16 len)
{
	static u32 z=362436069, w=521288629;

	/* The 55x cannot access bytes directly so we have to generate these packed; len gives the number of bytes */
	while (len&(~1)) {
		*(u16 *)buf=(u16)(MWC>>8);
		buf=(void *)(((unsigned long)buf)+1); // the c55 sucks, yes it does
		len-=2;
	}
}

// from crc32.c
unsigned long crc32(unsigned long crc, unsigned int *buf, unsigned int len);

static void test_blki(u16 len)
{
	usb_data_t *buf;
	usb_endpoint_t *ep=usb_get_ep(usb_get_config(),17);
	int err;

	usb_cancel(ep);
	flags.test_stat=STAT_BICC;
	buf=sys_malloc(usb_mem_len(len));
	if (!buf) {
		flags.err=1;
		return;
	}
	genrnd(buf,len);
	flags.crc=crc32(0,(unsigned int *)buf,len);
	flags.test_stat=STAT_BITX;
	err=usb_move_wait(ep,buf,len);
	if (err==-4)
		flags.timeout=1;
	else if (err)
		flags.err=1;
	sys_free(buf);
	flags.test_stat=STAT_IDLE;
}

static void test_blko(u16 len)
{
	usb_data_t *buf;
	int err;
	usb_endpoint_t *ep=usb_get_ep(usb_get_config(),1);

	usb_cancel(ep);
	flags.test_stat=STAT_BORX;
	buf=sys_malloc(usb_mem_len(len)+2);
	if (!buf) {
		flags.test_stat=STAT_IDLE;
		flags.err=1;
		return;
	}
	err=usb_move_wait(ep,buf,len);
	if (err==-4)
		flags.timeout=1;
	else if (err)
		flags.err=1;
	if (err) {
		flags.test_stat=STAT_IDLE;
		sys_free(buf);
		return;
	}
	flags.test_stat=STAT_BOCC;
	flags.crc=crc32(0,(unsigned int *)buf+1,len);
	sys_free(buf);
	flags.test_stat=STAT_IDLE;
}

static void test_tmoi(u16 len, char c)
{
	u16 fill;
	int i;
	usb_endpoint_t *ep=usb_get_ep(usb_get_config(),17);
	usb_data_t *buf;

	usb_cancel(ep);
	flags.test_stat=STAT_BITX;
	buf=sys_malloc(usb_mem_len(len));
	if (!buf) {
		flags.err=1;
		return;
	}
	fill=c|c<<8;
	for (i=0;i<usb_mem_len(len);++i)
		buf[i]=fill;
	if (usb_move_wait(ep,buf,len))
		flags.timeout=1;
	flags.test_stat=STAT_IDLE;
}

void test_thread(void)
{
	msg_t m;

	for(;;) {
		MBX_pend(&test_mbx,&m,SYS_FOREVER);
		switch(m.msg) {
		case CMD_BLKI:
			test_blki(m.arg1);
			break;
		case CMD_BLKO:
			test_blko(m.arg1);
			break;
		case CMD_TMOI:
			test_tmoi(m.arg1&0xffff,(m.arg1>>16)&0xff);
			break;
		default:
			break;
		}
	}
}

static void msg_post(MBX_Handle mbx, u16 msg, u32 arg1)
{
	msg_t m;

	m.msg=msg;
	m.arg1=arg1;
	MBX_post(mbx,&m,0);
}

static void start_blki(u16 len)
{
	msg_post(&test_mbx,CMD_BLKI,len);
}

static void start_blko(u16 len)
{
	msg_post(&test_mbx,CMD_BLKO,len);
}

static void start_tmoi(u16 len, char c)
{
	msg_post(&test_mbx,CMD_TMOI,(u32)len|((u32)c<<16));
}

static void ctl_write(void)
{
	int err;

	err=0;
	switch(usb_setup.request) {
	case CMD_WVAR:
		flags.var=usb_setup.value;
		break;
	case CMD_BLKI:
		start_blki(usb_setup.value);
		break;
	case CMD_BLKO:
		start_blko(usb_setup.value);
		break;
	case CMD_TMOI:
		start_tmoi(usb_setup.value,usbU8(usb_ctl_write_data));
		break;
	default:
		err=-1;
	}
	if (err)
		usb_ctl_stall();
	else
		usb_ctl_write_end();
}

static u8 get_stat(void)
{
	if (flags.err) {
		flags.err=0;
		return STAT_EROR;
	}
	if (flags.timeout) {
		flags.timeout=0;
		return STAT_TIMO;
	}
	return flags.test_stat;
}

static void ctl_read(void)
{
	switch(usb_setup.request) {
	case CMD_RVAR:
		usbPutU16(usbtxbuf,flags.var);
		usb_ctl_read_end(2,usbtxbuf);
		break;
	case CMD_STAT:
		usbPutU8(usbtxbuf,get_stat());
		usb_ctl_read_end(1,usbtxbuf);
		break;
	case CMD_RCRC:
		usbPutU32(usbtxbuf,flags.crc);
		usb_ctl_read_end(4,usbtxbuf);
		break;
	default:
		usb_ctl_stall();
		break;
	}
}

void handle_ctl(void)
{
	int err;
	
	err=usb_ctl_std();
	if (!err) {
		if (usb_setup.dataDir)
			ctl_read();
		else
			ctl_write();
	}
}

void usb_ctl(void)
{
	SWI_post(&swi_ctl);
}

void main(void)
{
	struct c55x_params p;

	p.clkin_khz=12000;
	p.us_per_prd_tick=1000;
	mmb0_init(144000000);
	flash_init();
	usb_init(&p);
	flags.test_stat=STAT_IDLE;
	flags.lastbuf=0;
	flags.timeout=0;
	flags.err=0;
	led_showchar('i');
	//usb_set_state_cb(state);
	usb_attach();
}
