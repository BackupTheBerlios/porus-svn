
#include "mmb0.h"
#include <std.h>
#include <clk.h>

static volatile u8 current_gpio, current_ctl;

static volatile pin_state_t iostate, iostate2;

static void pin_update_builtin_io(void)
{
	u8 d;
	
	d=iostate.cntl?PIN_CNTL:0;
	d|=iostate.fsxb?PIN_FSXB:0;
	d|=iostate.romcs?PIN_ROMCS:0;
	IODATA=d;
	d=PIN_ROMCS;
	d|=iostate.cntldir?PIN_CNTL:0;
	d|=iostate.fsxbdir?PIN_FSXB:0;
	IODIR=d;
}

static void pin_update_mcbsp_io(void)
{
	u8 pcr;
	
	pcr=PCR_XIOEN|PCR_RIOEN;
	pcr|=iostate.gpio3?PCR_CLKXP:0;
	pcr|=iostate.gpio3dir?PCR_CLKXM:0;
	pcr|=iostate.gpio0?PCR_FSXP:0;
	pcr|=iostate.gpio0dir?PCR_FSXM:0;
	pcr|=iostate.dx0?PCR_DXSTAT:0;
	pcr|=iostate.gpio1?PCR_CLKRP:0;
	pcr|=iostate.gpio1dir?PCR_CLKRM:0;
	pcr|=iostate.gpio2?PCR_FSRP:0;
	pcr|=iostate.gpio2dir?PCR_FSRM:0;
	PCR(0)=pcr;
}

static void init_mcbsp_io(void)
{
	SPCR1(0)=0;
	SPCR2(0)=0;
	iostate.cntldir=0;
	iostate.fsxbdir=0;
	iostate.romcs=1;
	iostate.gpio0dir=1;
	iostate.gpio0=1;
	iostate.gpio1dir=1;
	iostate.gpio1=1;
	iostate.gpio2dir=0;
	iostate.gpio3dir=0;
	pin_update_mcbsp_io();
}

static void pin_update_mcbsp2_io(void)
{
	u8 pcr;
	
	pcr=PCR_XIOEN|PCR_RIOEN;
	pcr|=iostate2.gpio3?PCR_CLKXP:0;
	pcr|=iostate2.gpio3dir?PCR_CLKXM:0;
	pcr|=iostate2.gpio0?PCR_FSXP:0;
	pcr|=iostate2.gpio0dir?PCR_FSXM:0;
	pcr|=iostate2.dx0?PCR_DXSTAT:0;
	pcr|=iostate2.gpio1?PCR_CLKRP:0;
	pcr|=iostate2.gpio1dir?PCR_CLKRM:0;
	pcr|=iostate2.gpio2?PCR_FSRP:0;
	pcr|=iostate2.gpio2dir?PCR_FSRM:0;
	PCR(2)=pcr;
}

static void init_mcbsp2_io(void)
{
	SPCR1(2)=0;
	SPCR2(2)=0;
	iostate2.cntldir=1;
	iostate2.cntl=1;
	iostate2.fsxbdir=1;
	iostate2.fsxb=1;
	iostate2.romcs=1;
	iostate2.gpio0dir=1;
	iostate2.gpio0=1;
	iostate2.gpio1dir=1;
	iostate2.gpio1=1;
	iostate2.gpio2dir=1;
	iostate2.gpio3dir=1;
	pin_update_mcbsp2_io();
}

void pin_set_GPIO0(int d)
{
	iostate.gpio0=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO1(int d)
{
	iostate.gpio1=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO2(int d)
{
	iostate.gpio2=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO3(int d)
{
	iostate.gpio3=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO0_dir(int d)
{
	iostate.gpio0dir=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO1_dir(int d)
{
	iostate.gpio1dir=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO2_dir(int d)
{
	iostate.gpio2dir=d;
	pin_update_mcbsp_io();
}

void pin_set_GPIO3_dir(int d)
{
	iostate.gpio3dir=d;
	pin_update_mcbsp_io();
}

void pin_set_CNTL(int d)
{
	iostate.cntl=d;
	pin_update_builtin_io();
}

void pin_set_FSXB(int d)
{
	iostate.fsxb=d;
	pin_update_builtin_io();
}

void pin_set_ROMCS(int d)
{
	iostate.romcs=d;
	pin_update_builtin_io();
}

void pin_set_CNTL_dir(int d)
{
	iostate.cntldir=d;
	pin_update_builtin_io();
}

void pin_set_FSXB_dir(int d)
{
	iostate.fsxbdir=d;
	pin_update_builtin_io();
}

void pin_set_gpio(u8 data)
{
	u8 d=data;

	d=d|PIN_SWA|PIN_SWB;
	if (!(d&PIN_GPIO4DIR)) {
		d=d|PIN_GPIO4;
	}
	if (!(d&PIN_GPIO5DIR)) {
		d=d|PIN_GPIO5;
	}
	i2c_lock();
	current_gpio=d;
	i2c_w8(I2C_BUS_MB,I2C_ADR_GPIO,d);
	i2c_unlock();
}

void pin_frob_gpio(u8 mask, u8 data)
{
	pin_set_gpio((mask&data)|((~mask)&current_gpio));
}

void pin_set_ctl(u8 d)
{
	i2c_lock();
	current_ctl=d;
	i2c_w8(I2C_BUS_MB,I2C_ADR_CTL,~d);
	i2c_unlock();
}

void pin_frob_ctl(u8 mask, u8 data)
{
	pin_set_ctl((mask&data)|((~mask)&current_ctl));
}

// freq in Hz
static void mmb0_set_cpuclk(u32 freq)
{
	int i;
	u32 m;
	volatile u16 clkmd;

	for (i=4;i>0;--i) {
		m=(freq/(12000L*1000L))*i;
		if (m<32) break;
	}
	if (m>=32) m=31;
	clkmd=0x2010|(m<<7)|((i-1)<<5);
	CLKMD=clkmd;
	//GBL_setFrequency(12000*(m/i));
	//CLK_reconfig();
}

static void emif_init(void)
{
	IOREG(0x6c00)=1;
	EMIF_EGCR=0x220;
	EMIF_CE01=0x3FFF;
	EMIF_CE02=0x5FFF;
	EMIF_CE03=0;
	EMIF_CE11=0x3FFF;
	EMIF_CE12=0x5FFF;
	EMIF_CE13=0;
	EMIF_CE21=0x3FFF;
	EMIF_CE22=0x5FFF;
	EMIF_CE23=0;
	EMIF_CE31=0x3FFF;
	EMIF_CE32=0x5FFF;
	EMIF_CE33=0;
	EMIF_SDC1=0x3522;
	EMIF_SDC2=0x0242;
	EMIF_SDC3=0x0003;
	EMIF_SDPER=0x0100;
	EMIF_EMIRST=0;
	EMIF_INIT=0;
}

void mmb0_preinit(void)
{
	emif_init();
}

void mmb0_init(u32 cpufreq)
{
	// system register; see '5507 PDS
	IOREG(0x7FD)=6; // divide clock by 12
	mmb0_set_cpuclk(cpufreq);
	i2c_init();
	pin_set_gpio(PIN_DCEN);
	pin_set_ctl(0);
	mmb0_activate_card();
	init_mcbsp_io();
	init_mcbsp2_io();
	ui_init();
	//clk_init();
	//dc_init();
}

#ifdef DEFAULT_INT
// daughtercard INT pin
void isr_INT(Arg a)
{
}
#endif

#ifdef DEFAULT_EINT
// external interrupt (pin on ext i2c connector)
void isr_EINT(Arg a)
{
}
#endif

// power interrupt
void isr_PWRINT(Arg a)
{
}
