
#include "mmb0.h"
#include "clk.h"
#include "i2c.h"

#define REFCLK 12e6

typedef struct clk_pll_config_t {
	u8 q;
	u16 p;
	unsigned int lf:3, po:1, en:1;
} clk_pll_config_t;

typedef struct clk_config_t {
	u8 clkdiv[6]; // a0 a1 b0 b1 c d
	u8 fs[4]; // a b c d
	unsigned int acadj_cx:2,
		acadj_abde:2,
		pdnen:1,
		xbufoe:1,
		div_clke:2,
		dcadj_x:2,
		dcadj_de:2,
		dcadj_c:2,
		dcadj_ab:2;
	unsigned int osc_cap:6,
		osc_drv:2;
	clk_pll_config_t pll1[8];
	clk_pll_config_t pll2, pll3;
} clk_config_t;

clk_config_t conf;

#define bits(src,mask,lsb,elsb) (((src&mask)>>lsb)<<elsb)

#define mkpll0(pll) (pll->q&0xff)
#define mkpll1(pll) (pll->p&0xff)
#define mkpll2(pll) ((pll->en<<6)|(pll->lf<<3)|(pll->po<<2)|((pll->p>>8)&3))

#define mk08(c) (((c->fs[0]&1)<<7)|(c->clkdiv[0]&0x7f))
#define mk09(c) (((c->fs[0]&1)<<7)|(c->clkdiv[1]&0x7f))
#define mk0A(c) (((c->fs[1]&1)<<7)|(c->clkdiv[2]&0x7f))
#define mk0B(c) (((c->fs[1]&1)<<7)|(c->clkdiv[3]&0x7f))
#define mk0C(c) (((c->fs[2]&1)<<7)|(c->clkdiv[4]&0x7f))
#define mk0D(c) (((c->fs[3]&1)<<7)|(c->clkdiv[5]&0x7f))
#define mk0E(c) (bits(c->fs[3],6,1,6)|bits(c->fs[2],6,1,4)|bits(c->fs[1],6,1,2)|bits(c->fs[0],6,1,0))
#define mk0F(c) ((c->acadj_cx<<6)|(c->acadj_abde<<4)|(c->pdnen<<3)|(c->xbufoe<<2)|(c->div_clke&3))
#define mk10(c) ((c->dcadj_x<<6)|(c->dcadj_de<<4)|(c->dcadj_c<<2)|(c->dcadj_ab))
#define mk17(c) ((c->osc_cap<<2)|(c->osc_drv))

static u8 mkPLLByte(clk_pll_config_t *pll, u8 ofs)
{
	switch (ofs) {
	case 0:
		return mkpll0(pll);
	case 1:
		return mkpll1(pll);
	case 2:
		return mkpll2(pll);
	default:
		break;
	}
	return 0;
}

static u8 mkRegByte(clk_config_t *conf, u8 adr)
{
	if ((adr>=0x11)&&(adr<=0x13))
		return mkPLLByte(&(conf->pll2),adr-0x11);
	if ((adr>=0x14)&&(adr<=0x16))
		return mkPLLByte(&(conf->pll3),adr-0x14);
	if ((adr>=0x40)&&(adr<=0x57))
		return mkPLLByte(&(conf->pll1[(adr-0x40)/3]),(adr-0x40)%3);
	switch (adr) {
	case 0x08:
		return mk08(conf);
	case 0x09:
		return mk09(conf);
	case 0x0A:
		return mk0A(conf);
	case 0x0B:
		return mk0B(conf);
	case 0x0C:
		return mk0C(conf);
	case 0x0D:
		return mk0D(conf);
	case 0x0E:
		return mk0E(conf);
	case 0x0F:
		return mk0F(conf);
	case 0x10:
		return mk10(conf);
	case 0x17:
		return mk17(conf);
	default:
		break;
	}
	return 0;
}

static void wrChip(u8 firstAdr, u8 endAdr)
{
	u8 buf[17];
	int i;

	if (endAdr==firstAdr) return;
	if ((endAdr-firstAdr)>16)
		endAdr=firstAdr+16;
	for (i=firstAdr;i<endAdr;++i)
		buf[i-firstAdr+1]=mkRegByte(&conf,i);
	buf[0]=firstAdr;
	i2c_wr(I2C_BUS_MB,I2C_ADR_CLK,buf,endAdr-firstAdr+1);
}

void clk_set_enable_pll(int pll, int on)
{
	if (pll==1) {
		conf.pll1[0].en=on;
		wrChip(0x42,0x43);
	} else if (pll==2) {
		conf.pll2.en=on;
		wrChip(0x13,0x14);
	} else {
		conf.pll3.en=on;
		wrChip(0x16,0x17);
	}
}

void clk_enable_pll(int pll)
{
	clk_set_enable_pll(pll,1);
}

void clk_disable_pll(int pll)
{
	clk_set_enable_pll(pll,0);
}

void clk_init(void)
{
	int i;
	
	for (i=0;i<6;++i)
		conf.clkdiv[i]=0;
	for (i=0;i<4;++i)
		conf.fs[i]=0;
	conf.acadj_cx=1;
	conf.acadj_abde=1;
	conf.xbufoe=0;
	conf.div_clke=0;
	conf.dcadj_x=1;
	conf.dcadj_de=1;
	conf.dcadj_c=1;
	conf.dcadj_ab=1;
	conf.osc_cap=0;
	conf.osc_drv=0;
	conf.pll2.en=0;
	conf.pll3.en=0;
	for (i=0;i<8;++i)
		conf.pll1[i].en=0;
	wrChip(0x8,0x18);
	wrChip(0x40,0x50);
	wrChip(0x50,0x58);
}

static void setPLL(clk_pll_config_t *pll, u16 p, u8 q)
{
	if (q<2) {
		pll->en=0;
		return;
	}
	pll->po=(p-6)&1;
	pll->p=((u16)(p-6)>>1);
	pll->q=(u8)(q-2);
	if (p<231) pll->lf=0;
	else if (p<626) pll->lf=1;
	else if (p<834) pll->lf=2;
	else if (p<1043) pll->lf=3;
	else pll->lf=4;
}

void clk_set_pll(int pll, u16 p, u8 q)
{
	switch(pll) {
	case 1:
		setPLL(&(conf.pll1[0]),p,q);
		wrChip(0x40,0x43);
		break;
	case 2:
		setPLL(&(conf.pll2),p,q);
		wrChip(0x11,0x14);
		break;
	case 3:
		setPLL(&(conf.pll3),p,q);
		wrChip(0x14,0x17);
		break;
	default:
		break;
	}
}

void clk_set_out(int output, int pllsrc, u8 div)
{
	if (output>3) return;
	if (pllsrc>7) return;
	conf.fs[output]=pllsrc<<1;
	switch(output) {
	case CLK_PIN_A:
		conf.clkdiv[0]=div;
		wrChip(8,9);
		break;
	case CLK_PIN_B:
		conf.clkdiv[2]=div;
		wrChip(10,11);
		break;
	case CLK_PIN_C:
		conf.clkdiv[4]=div;
		wrChip(12,13);
		break;
	case CLK_PIN_D:
		conf.clkdiv[5]=div;
		wrChip(13,14);
		break;
	}
	wrChip(14,15);
}

#define QLIMIT 40

static void farey(float rat, u16 *p, u16 *q, u16 qlimit)
{
	u16 mediant0, mediant1, lower0, lower1, upper0, upper1;
	
	/* This is an implementation of Farey's method for finding
	simplified rationals from decimal numbers.  The implementation
	is due to S. D. Daniels and was translated from his Python
	source.  It does not handle negative input.
	
	rat is an input rational number.  The routine calculates 
	p and q such that rat ~= p / q as closely as possible, 
	within the width limits of p and q.
	
	QLIMIT defines an additional maximum for q.
	*/
	lower0=0;
	lower1=1;
	upper0=1;
	upper1=0;
	for(;;) {
		mediant0=lower0+upper0;
		mediant1=lower1+upper1;
		if (rat*mediant1>mediant0) {
			if (qlimit<mediant1) {
				*p=upper0;
				*q=upper1;
				break;
			} else {
				lower0=mediant0;
				lower1=mediant1;
			}
		} else if (rat*mediant1==mediant0) {
			if (qlimit>=mediant1) {
				*p=mediant0;
				*q=mediant1;
				break;
			} else if (lower1<upper1) {
				*p=lower0;
				*q=lower1;
				break;
			} else {
				*p=upper0;
				*q=upper1;
				break;
			}
		} else {
			if (qlimit<mediant1) {
				*p=lower0;
				*q=lower1;
				break;
			} else {
				upper0=mediant0;
				upper1=mediant1;
			}
		}
	}
}

float clk_calc_div(float in, float out, u16 *p, u16 *q, u8 *post)
{
	float rat, fpll;

	*post=1;
	for(;;) {
		rat=((out)*(float)(*post))/in;
		//printf("rat=%f ",rat);
		farey(rat,p,q,257);
		//printf("p=%d q=%d ",(int)*p,(int)*q);
		if ((*q)<2) {
			(*p)*=2;
			(*q)=2;
		}
		fpll=in*((float)(*p)/(float)(*q));
		if (fpll+in>400e6) break;
		++(*post);
	}
	for (;;) {
		//printf("adjust: p=%d q=%d post=%d\n",(int)*p,(int)*q,(int)*post);
		if (*p<1600) break;
		*p/=2;
		*q/=2;
		//*post*=2;
		if (*q<2||*p<16) {
			*p=0;
			*q=0;
			break;
		}
	}
	return (in*((float)(*p)/(float)(*q)))/(float)(*post);
}

float clk_set_freq(int pll, float freq, u8 *post)
{
	u16 p, q;
	float out;

	out=clk_calc_div(REFCLK,freq,&p,&q,post);
	//printf("out=%f p=%d q=%d post=%d\n",out,(int)p,(int)q,(int)post);
	if (p&&q) {
		clk_set_pll(pll,p,q);
		return out;
	} else {
		return 0;
	}
}

float clk_set_outfreq(int output, int pll, float freq)
{
	u8 post;
	float act;

	clk_set_out(output,pll,0);
	clk_disable_pll(pll);
	act=clk_set_freq(pll,freq,&post);
	clk_enable_pll(pll);
	clk_set_out(output,pll,post);
	return act;
}
