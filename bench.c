#include <assert.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "rld0.h"
#include "b2b.h"

double rb3_cputime(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_utime.tv_sec + r.ru_stime.tv_sec + 1e-6 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}

uint64_t splitmix64(uint64_t *x)
{
	uint64_t z = ((*x) += 0x9e3779b97f4a7c15ULL);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
	return z ^ (z >> 31);
}

int main(int argc, char *argv[])
{
	uint64_t x, tot, checksum, ok[RLD_MAX_ASIZE];
	int32_t n = 10000000, i;
	double t;
	rld_t *e = 0;
	b2b_t *b = 0;

	if (argc == 1) {
		fprintf(stderr, "Usage: rankbench <in.fmd>|<in.b2b>\n");
		return 1;
	}
	e = rld_restore(argv[1]);
	if (e == 0) b = b2b_restore(argv[1]);
	assert(e || b);

	if (e) tot = e->cnt[e->asize];
	if (b) tot = b->acc[b->asize];

	t = rb3_cputime();
	for (i = 0, x = 11, checksum = 0; i < n; ++i) {
		uint64_t k = splitmix64(&x) % tot;
		if (e) rld_rank1a(e, k, ok);
		if (b) b2b_rank1a(b, k, (int64_t*)ok);
		checksum ^= ok[2];
	}
	printf("checksum: %lx\n", (unsigned long)checksum);
	printf("time: %f\n", rb3_cputime() - t);

	if (e) rld_destroy(e);
	if (b) b2b_destroy(b);
	return 0;
}
