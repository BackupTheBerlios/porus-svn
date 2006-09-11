
#include "bfifo.h"
#include <stdlib.h>

/*
static void adjustLen(bfifo_t *f, bfifo_data_t *adr, bfifo_size_t *len, int pagebreak)
{
	u32 ptr=(u32)adr;
	u32 size=(u32)(*len);
	u32 nextpage=(u32)(f->pagesize);

	if (pagebreak) {
		// adjust for page
		nextpage=((~(nextpage-1))&ptr)+f->pagesize;
		if (size>(nextpage-ptr))
			size=nextpage-ptr;
	}

	// adjust for fifo end
	nextpage=(u32)((u32)(f->adr)+f->size);
	if (size>(nextpage-ptr))
		size=nextpage-ptr;

	//if (size>(u32)(f->len)) size=(u32)(f->len);
	// adjust for adr limit
	if (hadr>adr) {
		nextpage=(u32)(hadr);
		if (size>(nextpage-ptr)) 
			size=nextpage-ptr;
	}

	*len=(bfifo_size_t)size;	
}
*/

int bfifo_isfull(bfifo_t *f)
{
	return f->len>=f->size;
}

int bfifo_isempty(bfifo_t *f)
{
	return (f->len==0);
}

int bfifo_ravail(bfifo_t *f)
{
	if (!f) return 0;
	return f->len;
}

int bfifo_wavail(bfifo_t *f)
{
	if (!f) return 0;
	return f->size-f->len;
}

u32 bfifo_wstart(bfifo_t *f)
{
	if (!f) return 0;
	bfifo_wend(f);
	if (f->len>=f->size) return 0;
	f->writing=1;
	return bfifo_writeloc(f);
}

void bfifo_wend(bfifo_t *f)
{
	if (!f) return;
	if (!f->writing) return;
	++f->wpos;
	if (f->wpos>=f->size) f->wpos=0;
	++f->len;
	f->writing=0;
}

void bfifo_wcancel(bfifo_t *f)
{
	if (!f) return;
	f->writing=0;
}

u32 bfifo_rstart(bfifo_t *f)
{
	if (!f) return 0;
	bfifo_rend(f);
	if (!f->len) return 0;
	f->reading=1;
	--f->len;
	return bfifo_readloc(f);
}

void bfifo_rend(bfifo_t *f)
{
	if (!f) return;
	if (!f->reading) return;
	++f->rpos;
	if (f->rpos>=f->size) f->rpos=0;
	f->reading=0;
}

void bfifo_rcancel(bfifo_t *f)
{
	if (!f) return;
	if (!f->reading) return;
	f->reading=0;
	++f->len;
}

void bfifo_clear(bfifo_t *f)
{
	if (!f) return;
	f->wpos=0;
	f->rpos=0;
	f->writing=0;
	f->reading=0;
	f->len=0;
}

int bfifo_isreading(bfifo_t *f)
{
	if (!f) return 0;
	return (f->reading);
}

int bfifo_iswriting(bfifo_t *f)
{
	if (!f) return 0;
	return (f->writing);
}

u32 bfifo_readloc(bfifo_t *f)
{
	if (!f) return 0;
	return f->adr+(f->rpos*f->blocksize);
}

u32 bfifo_writeloc(bfifo_t *f)
{
	if (!f) return 0;
	return f->adr+(f->wpos*f->blocksize);
}

void bfifo_init(bfifo_t *f, u32 adr, u32 size, u32 blocksize)
{
	if (!f) return;
	f->adr=adr;
	f->size=size;
	f->blocksize=blocksize;
	bfifo_clear(f);
}
