#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "b2b.h"

#define B2B_CALLOC(type, cnt) ((type*)calloc((cnt), sizeof(type)))

static inline int32_t b2b_ceil_log2(uint64_t v)
{
	int32_t i;
	for (i = 1; i < 64 && 1ULL<<i < v; ++i) {}
	return i;
}

void b2b_itr_init(b2b_itr_t *itr)
{
	itr->c = -1, itr->l = 0, itr->n_b = 0;
}

b2b_t *b2b_init(int32_t asize, int32_t bbit)
{
	b2b_t *b;
	assert(asize <= B2B_MAX_ASIZE);
	b = B2B_CALLOC(b2b_t, 1);
	b->asize = asize;
	b->bbit = bbit > 0 && bbit < 16? bbit : B2B_DEFAULT_BBIT;
	b->acc = B2B_CALLOC(int64_t, asize + 1);
	b->x.abit = b2b_ceil_log2(asize);
	b->x.amask = (1<<b->x.abit) - 1;
	b->x.cbit = B2B_CALLOC(int32_t, asize);
	return b;
}

void b2b_destroy(b2b_t *b)
{
	free(b->x.cbit); free(b->acc); free(b->b); free(b->c); free(b);
}

void b2b_alloc_blocks(b2b_t *b, int64_t n_b)
{
	b->b = (uint16_t*)malloc(n_b * 2);
}

static inline void b2b_enc_int(uint64_t *a, uint64_t *x, uint64_t v, int32_t bit)
{
	uint64_t i = *x>>6;
	int32_t off = *x&0x3f;
	a[i] |= v << off;
	if (64 - off <= bit) // split between a[i] and a[i+1]
		a[i + 1] = v >> (64 - off);
	*x += bit;
}

static inline uint64_t b2b_dec_int(const uint64_t *a, uint64_t *x, int32_t bit)
{
	uint64_t v, i = *x>>6;
	int32_t off = *x&0x3f;
	v = a[i] >> off;
	if (64 - off <= bit) // split between a[i] and a[i+1]
		v |= a[i + 1] << (64 - off);
	*x += bit;
	return v & ((1ULL << bit) - 1);
}

static void b2b_cal_derived(b2b_t *b) // assuming $n_b and $acc[] are populated
{
	int c;
	int64_t n_c;
	b->x.obit = b2b_ceil_log2(b->n_b);
	b->x.rbit = b->x.obit;
	for (c = 0; c < b->asize; ++c) {
		b->x.cbit[c] = b2b_ceil_log2(b->acc[c+1] - b->acc[c]);
		b->x.rbit += b->x.cbit[c];
	}
	n_c = (b->acc[b->asize] + (1<<b->bbit) - 1) >> b->bbit;
	b->x.n_c = (n_c * b->x.rbit + 63) / 64;
}

void b2b_index(b2b_t *b)
{
	int64_t i, j, cnt[B2B_MAX_ASIZE];
	uint64_t p = 0, n;
	int a;

	// precompute counts
	memset(cnt, 0, 8 * b->asize);
	for (i = 0; i < b->n_b; ++i) {
		uint16_t x = b->b[i] >> b->x.abit;
		int c = b->b[i] & b->x.amask;
		cnt[c] += x;
	}
	for (a = 0, b->acc[0] = 0; a < b->asize; ++a)
		b->acc[a + 1] = b->acc[a] + cnt[a];
	b2b_cal_derived(b);

	// popular b->c
	b->c = B2B_CALLOC(uint64_t, b->x.n_c);
	memset(cnt, 0, 8 * b->asize);
	p = 0;
	b2b_enc_int(b->c, &p, 0, b->x.obit);
	for (a = 0; a < b->asize; ++a)
		b2b_enc_int(b->c, &p, 0, b->x.cbit[a]);
	for (i = 0, j = 1, n = 0; i < b->n_b; ++i) {
		uint16_t x = b->b[i] >> b->x.abit;
		int a, c = b->b[i] & b->x.amask;
		if (n >> b->bbit != (n + x) >> b->bbit) {
			for (; j <= (n + x) >> b->bbit; ++j) {
				b2b_enc_int(b->c, &p, i, b->x.obit);
				for (a = 0; a < b->asize; ++a)
					b2b_enc_int(b->c, &p, cnt[a], b->x.cbit[a]);
			}
		}
		n += x, cnt[c] += x;
	}
}

void b2b_enc_finish(b2b_t *b, b2b_itr_t *itr)
{
	b->n_b = itr->n_b;
	b2b_enc(b, itr, -1, 0);
	b2b_index(b);
}

int b2b_dump(const char *fn, const b2b_t *b)
{
	FILE *fp;
	fp = strcmp(fn, "-")? fopen(fn, "wb") : stdout;
	if (fp == 0) return -1;
	fwrite("B2B\1", 1, 4, fp); // write magic
	fwrite(&b->asize, 4, 1, fp);
	fwrite(&b->bbit, 4, 1, fp);
	fwrite(b->acc, 8, b->asize + 1, fp);
	fwrite(&b->n_b, 8, 1, fp);
	fwrite(b->b, 2, b->n_b, fp);
	fwrite(b->c, 8, b->x.n_c, fp);
	fclose(fp);
	return 0;
}

static b2b_t *b2b_restore_header(const char *fn, FILE **_fp)
{
	FILE *fp;
	b2b_t *b;
	char magic[4];
	int32_t asize, bbit;

	if (strcmp(fn, "-") == 0) *_fp = fp = stdin;
	else if ((*_fp = fp = fopen(fn, "rb")) == 0) return 0;
	fread(magic, 1, 4, fp);
	if (strncmp(magic, "B2B\1", 4)) return 0;
	fread(&asize, 4, 1, fp);
	fread(&bbit, 4, 1, fp);
	assert(asize <= B2B_MAX_ASIZE);
	b = b2b_init(asize, bbit);
	b->acc = B2B_CALLOC(int64_t, asize + 1);
	fread(b->acc, 8, asize + 1, fp);
	fread(&b->n_b, 8, 1, fp);
	b2b_cal_derived(b);
	return b;
}

b2b_t *b2b_restore(const char *fn)
{
	FILE *fp;
	b2b_t *b;
	b = b2b_restore_header(fn, &fp);
	if (b == 0) return 0;
	b2b_alloc_blocks(b, b->n_b);
	fread(b->b, 2, b->n_b, fp);
	b->c = B2B_CALLOC(uint64_t, b->x.n_c);
	fread(b->c, 8, b->x.n_c, fp);
	return b;
}

int b2b_rank1a(const b2b_t *b, int64_t k, int64_t *ok)
{
	int c;
	uint64_t p = (k >> b->bbit) * b->x.rbit;
	int64_t n;
	const uint16_t *q;
	if (k >= b->acc[b->asize]) {
		for (c = 0; c < b->asize; ++c)
			ok[c] = b->acc[c + 1] - b->acc[c];
		return -1;
	}
	q = b->b + b2b_dec_int(b->c, &p, b->x.obit);
	__builtin_prefetch(q);
	for (c = 0, n = 0; c < b->asize; ++c) {
		ok[c] = b2b_dec_int(b->c, &p, b->x.cbit[c]);
		n += ok[c];
	}
	assert(n <= k);
	while (n < k) {
		int l = *q >> b->x.abit, c = *q & b->x.amask;
		if (n + l > k) {
			ok[c] += k - n;
			break;
		}
		++q, ok[c] += l, n += l;
	}
	return *q & b->x.amask;
}
