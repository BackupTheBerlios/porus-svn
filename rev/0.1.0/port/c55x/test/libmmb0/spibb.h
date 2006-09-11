
#ifndef GUARD_spibb_h
#define GUARD_spibb_h

#include "types.h"

typedef struct spibb_t {
	unsigned int port:3,
		lsbfirst:1,
		idlehi:1,
		slfall:1;
	u16 delay1, delay2;
} spibb_t;

u8 spibb_shift8(spibb_t *c, u8 out);
void spibb_cshi(spibb_t *c);
void spibb_cslo(spibb_t *c);
s32 spibb_rs24(spibb_t *c);
void spibb_set_phase(spibb_t *c, int slfall, int idlehi, int lsbfirst);
void spibb_set_delay(spibb_t *c, int ph1, int ph2);
int spibb_open(spibb_t *c, int port);
void spibb_wu24(spibb_t *c, u32 d);

#endif
