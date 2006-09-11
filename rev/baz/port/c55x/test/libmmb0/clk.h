
#ifndef GUARD_clk_h
#define GUARD_clk_h

#define CLK_SRC_REF		0
#define CLK_SRC_PLL1		2
#define CLK_SRC_PLL1_180	3
#define CLK_SRC_PLL2		4
#define CLK_SRC_PLL2_180	5
#define CLK_SRC_PLL3		6
#define CLK_SRC_PLL3_180	7

#define CLK_PIN_A		0
#define CLK_PIN_B		1
#define CLK_PIN_C		2
#define CLK_PIN_D		3

void clk_init(void);

void clk_enable_pll(int pll);
void clk_disable_pll(int pll);

// q<2 turns off pll; pll is 1, 2, or 3
void clk_set_pll(int pll, u16 p, u8 q);

float clk_set_freq(int pll, float freq, u8 *post);

// div = 0 to turn off output; div <= 127
// use CLK_SRC_* for pllsrc
// use CLK_PIN_* for output
void clk_set_out(int output, int pllsrc, u8 div);

float clk_set_outfreq(int output, int pll, float freq);

#endif
