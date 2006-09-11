
#ifndef GUARD_mmb0_h
#define GUARD_mmb0_h

#include <chip/tms320vc5507.h>
#include "config.h"
#include "types.h"
//#include "task.h"

typedef struct pin_state_t {
	unsigned int cntl:1,	// gpio6
		cntldir:1,
		fsxb:1,		// gpio7
		fsxbdir:1,
		romcs:1,	// gpio4
		dx0:1,		// for flash
		gpio0:1,	// fsx0
		gpio0dir:1,
		gpio1:1,	// clkr0
		gpio1dir:1,
		gpio2:1,	// fsr0
		gpio2dir:1,
		gpio3:1,	// clkx0
		gpio3dir:1;
} pin_state_t;

#define I2C_ADR_MUX	0xE2
#define I2C_ADR_GPIO	0x40
#define I2C_ADR_LED	0x42
#define I2C_ADR_CTL	0x44
#define I2C_ADR_CLK	(0x69<<1)

// bus numbers for the I2C mux
#define I2C_BUS_DC 0
#define I2C_BUS_MB 1
#define I2C_BUS_EXT 2

// U1 -- address I2C_ADR_GPIO
#define PIN_SWA	(1<<0)
#define PIN_SWB	(1<<1)
#define PIN_GPIO4	(1<<2)
#define PIN_GPIO5	(1<<3)
	// dir bits: 1 = out
#define PIN_GPIO4DIR	(1<<4)
#define PIN_GPIO5DIR	(1<<5)
#define PIN_DCEN	(1<<7)

// U8 -- address I2C_ADR_CTL
#define PIN_FSXON	(1<<0)
#define PIN_FSXBON	(1<<1)
#define PIN_CLKXON	(1<<2)
#define PIN_CLKCON	(1<<3)
#define PIN_CLKRON	(1<<4)
#define PIN_CLKAON	(1<<5)
#define PIN_TOUT0ON	(1<<6)
#define PIN_CLKBON	(1<<7)

// on the built-in gpio port
#define PIN_ROMCS (1<<4)
#define PIN_CNTL (1<<6)
#define PIN_FSXB (1<<7)

#define CTL_FSX_FSX		PIN_FSXON
#define CTL_FSX_FSXB		PIN_FSXBON
#define CTL_CLKX_CLKX		PIN_CLKXON
#define CTL_CLKX_CLKC		PIN_CLKCON
#define CTL_CLKR_CLKR		PIN_CLKRON
#define CTL_CLKR_CLKA		PIN_CLKAON
#define CTL_TOUT_TOUT0	PIN_TOUT0ON
#define CTL_TOUT_CLKB		PIN_CLKBON

// gpio control etc.
void pin_set_gpio(u8 d);
void pin_frob_gpio(u8 mask, u8 data);
// void pin_set_gpio_bit(u8 bit, u8 on);
#define pin_set_gpio_bit(mask,on) pin_frob_gpio(mask,on?0xff:0)
// int pin_gpio(void);
#define pin_gpio() i2c_r8(I2C_BUS_MB,I2C_ADR_GPIO)

#define mmb0_activate_card() pin_set_gpio_bit(PIN_DCEN,0)
#define mmb0_deactivate_card() pin_set_gpio_bit(PIN_DCEN,1)

void pin_set_ctl(u8 d);
void pin_frob_ctl(u8 mask, u8 data);
// void pin_set_ctl_bit(u8 bit, u8 on);
#define pin_set_ctl_bit(mask,on) pin_frob_ctl(mask,on?0xff:0)

void pin_set_GPIO0(int);
void pin_set_GPIO1(int);
void pin_set_GPIO2(int);
void pin_set_GPIO3(int);
//void pin_set_GPIO4(int);
#define pin_set_GPIO4(d) pin_set_gpio_bit(PIN_GPIO4,d)
//void pin_set_GPIO5(int);
#define pin_set_GPIO5(d) pin_set_gpio_bit(PIN_GPIO5,d)
void pin_set_CNTL(int);
void pin_set_FSXB(int);
void pin_set_ROMCS(int d);
void pin_set_GPIO0_dir(int);
void pin_set_GPIO1_dir(int);
void pin_set_GPIO2_dir(int);
void pin_set_GPIO3_dir(int);
//void pin_set_GPIO4_dir(int);
#define pin_set_GPIO4_dir(d) pin_set_gpio_bit(PIN_GPIO4DIR,d)
//void pin_set_GPIO5DIR(int);
#define pin_set_GPIO5_dir(d) pin_set_gpio_bit(PIN_GPIO5DIR,d)
void pin_set_CNTL_dir(int);
void pin_set_FSXB_dir(int);
//int pin_GPIO0();
#define pin_GPIO0() (PCR(0)&PCR_FSXP)
//int pin_GPIO1();
#define pin_GPIO1() (PCR(0)&PCR_CLKRP)
//int pin_GPIO2();
#define pin_GPIO2() (PCR(0)&PCR_FSRP)
//int pin_GPIO3();
#define pin_GPIO3() (PCR(0)&PCR_CLKXP)
//int pin_GPIO4();
#define pin_GPIO4() (pin_gpio()&PIN_GPIO4)
//int pin_GPIO5();
#define pin_GPIO5() (pin_gpio()&PIN_GPIO5)
//int pin_CNTL();
#define pin_CNTL() (IODATA&PIN_CNTL)
//int pin_FSXB();
#define pin_FSXB() (IODATA&PIN_FSXB)

void mmb0_init(u32 cpufreq);

#endif
