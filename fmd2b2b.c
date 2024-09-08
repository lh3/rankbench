#include <assert.h>
#include <stdlib.h>
#include "rld0.h"
#include "b2b.h"

b2b_t *rb3_enc_fmd2b2b(rld_t *e, int bbit, int is_free)
{
	b2b_t *b;
	rlditr_t itr;
	b2b_itr_t bi;
	int64_t l;
	int c;

	b = b2b_init(e->asize, bbit);
	b2b_itr_init(&bi);
	rld_itr_init(e, &itr, 0);
	while ((l = rld_dec(e, &itr, &c, 0)) > 0)
		b2b_enc(b, &bi, c, l);
	b2b_enc(b, &bi, -1, 0);
	b2b_alloc_blocks(b, bi.n_b);

	b2b_itr_init(&bi);
	rld_itr_init(e, &itr, 0);
	while ((l = rld_dec(e, &itr, &c, is_free)) > 0)
		b2b_enc(b, &bi, c, l);
	b2b_enc_finish(b, &bi);
	if (is_free) rld_destroy(e);
	return b;
}

int main(int argc, char *argv[])
{
	int32_t bbit = 12;
	rld_t *e;
	b2b_t *b;
	if (argc < 3) {
		fprintf(stderr, "Usage: fmd2b2b <in.fmd> <out.b2b> [bbit=%d]\n", bbit);
		return 1;
	}
	if (argc >= 4) bbit = atoi(argv[3]);
	e = rld_restore(argv[1]);
	assert(e);
	b = rb3_enc_fmd2b2b(e, bbit, 1);
	b2b_dump(argv[2], b);
	b2b_destroy(b);
	return 0;
}
