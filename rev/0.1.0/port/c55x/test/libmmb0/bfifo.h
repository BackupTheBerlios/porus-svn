
#ifndef GUARD_bfifo_h
#define GUARD_bfifo_h

#include "types.h"
#include <stddef.h>

/*!
bfifo implements a non-atomic block-reserved byte or word-oriented
circular buffer.

- Write operations are not atomic.  You reserve a region of the FIFO to
write to.
- Read operations are not atomic.  You reserve a region of the FIFO to read
from.
- This implementation can interrupt blocks at page boundaries.

*/

typedef u16 bfifo_data_t;
typedef size_t bfifo_size_t;

typedef struct bfifo_t {
	u32 adr, blocksize, size, len;
	u32 wpos, rpos;
	unsigned int writing:1,
		reading:1;
} bfifo_t;

u32 bfifo_wstart(bfifo_t *f);
void bfifo_wend(bfifo_t *f);
void bfifo_wcancel(bfifo_t *f);
u32 bfifo_rstart(bfifo_t *f);
void bfifo_rend(bfifo_t *f);
void bfifo_rcancel(bfifo_t *f);

void bfifo_init(bfifo_t *f, u32 adr, u32 size, u32 blocksize);
void bfifo_clear(bfifo_t *f);
int bfifo_isfull(bfifo_t *f);
int bfifo_isempty(bfifo_t *f);
int bfifo_ravail(bfifo_t *f);
int bfifo_wavail(bfifo_t *f);
int bfifo_isreading(bfifo_t *f);
int bfifo_iswriting(bfifo_t *f);
u32 bfifo_readloc(bfifo_t *f);
u32 bfifo_writeloc(bfifo_t *f);

#endif
