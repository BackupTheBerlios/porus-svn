
#include "ui.h"
#include "mmb0.h"

static u8 current_led;
static u8 lastswitches;
task_argfn_t mmb0_key_handler;
static LgUns swatime, swbtime;

/* charset format: pgfedcba */
static const unsigned char segtbl[]={
	// punctuation, 0x20-0x2f
     //' ' !   "    # $ % & '    (    )    *    +    , - .    /
	0,0x82,0x22,0,0,0,0,0x20,0x39,0xf,0x49,0x46,0,0x40,0x80,0,
	// numbers: 0x30-0x39
	//0  1    2    3    4    5    6    7    8    9
	0x3f,0x6,0x5b,0x4f,0x66,0x6d,0x7d,0x7,0x7f,0x6f,
	// extra stuff, 0x3a-0x40
     // : ; < = > ?    @@
	0,0,0,0x48,0,0xd3,0,
	// letters, 0x41-0x59
	//a  b    c    d    e    f    g    h
	0x5f,0x7c,0x58,0x5e,0x7b,0x71,0x3d,0x74,
	//i  j    k    l    m    n    o    p
	0x4,0x1e,0x78,0x38,0x2b,0x54,0x5c,0x73,
	//q  r    s    t    u    v    w    x
	0x67,0x50,0x6c,0x70,0x1c,0x1d,0x6a,0x52,
	//y  z
	0x6e,0x53,
};

void led_set(u8 d)
{
	if (d==current_led) return;
	i2c_w8(I2C_BUS_MB,I2C_ADR_LED,~d);
	current_led=d;
}

u8 led(void)
{
	return current_led;
}

void led_dp(int i)
{
	if (i) {
		led_set(current_led|0x80);
	} else {
		led_set(current_led&0x7F);
	}
}

static unsigned char chrtosegs(char c)
{
	if (c>='a'&&c<='z')
		c-=0x20;
	if (c>=' '&&c<='Z')
		return segtbl[c-' '];
	if (c=='_')
		return 8;
	if (c=='|')
		return 6;
	if (c=='`')
		return 0x20;
	return 0;
}

void led_showchar(char c)
{
	led_set((current_led&0x80)|(chrtosegs(c)&0x7F));
}

void led_showdig(int d)
{
	d&=15;
	if (d>9) led_showchar((d-10)+'a');
	else led_showchar(d+'0');
}

u8 sw_read(void)
{
	return i2c_r8(I2C_BUS_MB,I2C_ADR_GPIO)&(PIN_SWA|PIN_SWB);
}

// switch interrupt
void isr_SWINT(Arg a)
{
	LgUns t;

	u8 keys;
	u8 changed;
	
	if (!mmb0_key_handler) return;
	keys=sw_read();
	changed=keys^lastswitches;
	if (!keys)
		led_dp(0);
	else if (keys==3)
		led_dp(1);
	if (changed&1) {
		t=PRD_getticks();
		if ((t-swatime)>50) {
			swatime=t;
			//task_post_argfn(mmb0_key_handler,0,(keys&1)?1:-1);
		}
	}
	if (changed&2) {
		t=PRD_getticks();
		if ((t-swbtime)>50) {
			swbtime=t;
			//task_post_argfn(mmb0_key_handler,0,(keys&2)?2:-2);
		}
	}
}

void ui_init(void)
{
	//lastswitches=sw_read();
	led_set(0x80);
	mmb0_key_handler=0;
	//IRQ_enable(IRQ_EVT_INT2);
}
