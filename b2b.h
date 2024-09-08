#ifndef RB3_B2B_H
#define RB3_B2B_H

#include <stdint.h>

#define B2B_DEFAULT_BBIT 12
#define B2B_MAX_ASIZE    31

typedef struct {
	int32_t n, m;
	uint16_t *b;
} b2b_sblock_t;

typedef struct {
	int32_t asize; // alphabet size; for DNA, it is six: $/A/C/G/T/N
	int32_t bbit;  // block size is 2**$bbit
	int64_t n_b;   // size of $b
	uint16_t *b;
	uint64_t *c;   // bit-packed counts
	int64_t *acc;  // accumulative count of size $asize+1
	struct { // derived
		int32_t abit;  // ceil(log2($asize))
		int32_t amask; // (1<<$abit) - 1
		int32_t obit;  // ceil(log2($n_b))
		int32_t rbit;  // sum of cbit and obit
		int64_t n_c;   // size of $c, derived from $acc
		int32_t *cbit; // cbit[c]: #bits needed to store count for symbol
	} x;
} b2b_t;

typedef struct {
	int32_t c;
	int64_t l, n_b;
} b2b_itr_t;

b2b_t *b2b_init(int32_t asize, int32_t bbit);
void b2b_destroy(b2b_t *b);

void b2b_itr_init(b2b_itr_t *itr);
void b2b_alloc_blocks(b2b_t *b, int64_t n);
void b2b_enc_finish(b2b_t *b, b2b_itr_t *itr);

int b2b_dump(const char *fn, const b2b_t *b);
b2b_t *b2b_restore(const char *fn);

int b2b_rank1a(const b2b_t *b, int64_t k, int64_t *ok);

static inline void b2b_enc(b2b_t *b, b2b_itr_t *itr, int32_t c, int64_t l) // finish with c=-1
{
	if (c != itr->c) {
		int32_t step = (1LL << (16 - b->x.abit)) - 1;
		int64_t k = itr->l;
		while (k > 0) {
			uint16_t x = k >= step? step : k;
			if (b->b)
				b->b[itr->n_b++] = x << b->x.abit | itr->c;
			else
				itr->n_b++;
			k -= x;
		}
		itr->c = c, itr->l = 0;
	}
	itr->l += l;
}

#endif
