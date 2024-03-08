#include "../mite/mite.h"
#include "../smap.h"
#include "impl.h"

#include <time.h>

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

TEST_CASE(hash_test)
{
	const size_t seed = _smap_hash_seed();
	const char* const s = "xxxxxx";

	TEST(_smap_calc_hash(s, 3, seed) == _smap_calc_hash(s + 3, 3, seed));
}

static
size_t count_used_slots(smap* const map)
{
	if(!map->slots)
		return 0;

	size_t count = 0;
	const smap_slot* const end = map->slots + map->cap;

	for(const smap_slot* p = map->slots; p < end; ++p)
		if(p->entry)
			++count;

	return count;
}

static size_t release_count = 0;

static
void count_releases(void* UNUSED(p)) { ++release_count; }

static
size_t clear_map(smap* const map)
{
	release_count = 0;
	smap_clear(map, count_releases);

	return release_count;
}

TEST_CASE(basic_test)
{
	smap map = smap_empty;

	// check empty
	TEST(map.count == 0);
	TEST(map.cap == 0);
	TEST(map.seed == 0);
	TEST(map.slots == NULL);

	// try a key
	const void* key = "xxx";
	const size_t len = sizeof("xxx") - 1;

	TEST(smap_get(&map, key, len) == NULL);

	// add a key
	void** p = smap_add(&map, key, len);

	TEST(p != NULL);
	TEST(map.count == 1);
	TEST(map.cap == BASE_CAP);
	TEST(map.seed != 0);
	TEST(map.slots != NULL);

	*p = (void*)1;

	p = smap_get(&map, key, len);

	TEST(p != NULL);
	TEST(*p == (void*)1);
	TEST(count_used_slots(&map) == 1);

	// change the value
	*p = (void*)42;

	TEST(*smap_get(&map, key, len) == (void*)42);

	// delete the key
	TEST(smap_del(&map, key, len) == (void*)42);
	TEST(smap_size(&map) == 0);
	TEST(map.cap == BASE_CAP);
	TEST(map.slots != NULL);

	// add N keys
	char buff[128];

#define key_len(n) snprintf(buff, sizeof(buff), "key number %zu", (n))

	const size_t N = 1000;

	for(size_t i = 1; i <= N; ++i)
	{
		p = smap_add(&map, buff, key_len(i));

		TEST(p != NULL);
		TEST(*p == NULL);

		*p = (void*)i;
	}

	TEST(smap_size(&map) == N);
	TEST(count_used_slots(&map) == N);

	// get and validate N keys, in reverse order
	for(size_t i = N; i > 0; --i)
	{
		p = smap_get(&map, buff, key_len(i));

		TEST(p != NULL);
		TEST(*p == (void*)i);
	}

	// delete first N/2 keys
	for(size_t i = 1; i <= N / 2; ++i)
		TEST(smap_del(&map, buff, key_len(i)) == (void*)i);

	TEST(smap_size(&map) == N / 2);
	TEST(count_used_slots(&map) == N / 2);

	// see if the remaining keys are still there
	for(size_t i = N / 2 + 1; i <= N; ++i)
	{
		p = smap_get(&map, buff, key_len(i));

		TESTF(p != NULL, "missing key: %zu", i);
		TEST(*p == (void*)i);
	}

	// clean-up
	smap_clear(&map, NULL);

	// check empty
	TEST(map.count == 0);
	TEST(map.cap == 0);
	TEST(map.seed == 0);
	TEST(map.slots == NULL);

#undef make_key
}

static
int delete_odd_key(void* param, const void* UNUSED(key), size_t UNUSED(len), void** pval)
{
	const int val = (int)(intptr_t)*pval;

	if((val & 1) != 0)
	{
		*(size_t*)param -= 1;
		return -1;
	}

	return 0;
}

static
int check_even_key(void* param, const void* key, size_t UNUSED(len), void** pval)
{
	const int val = (int)(intptr_t)*pval;

	if((val & 1) != 0)
	{
		printf("error: unexpected value %d for key %zu\n", val, *(const size_t*)key);
		return 1;
	}

	*(size_t*)param += 1;
	return 0;
}

TEST_CASE(random_test)
{
	srand(time(NULL));

	smap map = smap_empty;

	size_t N = 1000000;

	// generate and add N random keys
	for(size_t i = 0; i < N; ++i)
		*smap_add(&map, &i, sizeof(i)) = (void*)(intptr_t)rand();

	TEST(map.count == N);

	// delete odd keys
	TEST(smap_scan(&map, delete_odd_key, &N) == 0);
	TEST(map.count == N);

	// check remaining keys
	size_t n = 0;

	TEST(smap_scan(&map, check_even_key, &n) == 0);
	TEST(n == N);

	// clean-up with check
	TEST(clear_map(&map) == N);
}

TEST_CASE(capacity_test)
{
	smap map = smap_empty;

	// resize empty map
	TEST(smap_resize(&map, BASE_CAP - BASE_CAP / 4 - 1) == 0);
	TEST(map.slots);
	TEST(map.cap == BASE_CAP);
	TEST(map.count == 0);

	// fill to capacity
	const size_t N = smap_cap(&map);

	for(size_t i = 0; i < N; ++i)
		*smap_add(&map, &i, sizeof(i)) = (void*)i;

	TEST(map.count == N);
	TEST(map.cap == BASE_CAP);

	// add one more
	*smap_add(&map, &N, sizeof(N)) = (void*)N;

	TEST(map.count == N + 1);
	TEST(map.cap == 2 * BASE_CAP);

	// delete all except one
	for(size_t i = 0; i < N; ++i)
		TEST(smap_del(&map, &i, sizeof(i)) == (void*)i);

	// downsize to the number of keys
	TEST(smap_resize(&map, smap_size(&map)) == 0);
	TEST(map.cap == BASE_CAP);
	TEST(map.count == 1);
	TEST(*smap_get(&map, &N, sizeof(N)) == (void*)N);

	smap_clear(&map, NULL);
}

static
void do_bench_hash(char* const buff, const size_t N, const size_t n)
{
	const size_t seed = _smap_hash_seed();
	double sum = 0;

#if __SIZEOF_SIZE_T__ == 8
	const double K = 0x8000000000000000u;
#elif __SIZEOF_SIZE_T__ == 4
	const double K = 0x80000000u;
#endif

	const clock_t start = clock();

	for(const char* s = buff; s < buff + N; s += n)
		sum += _smap_calc_hash(s, n, seed);

	const double d = (double)(clock() - start) / CLOCKS_PER_SEC;
	const size_t count = N / n;

	printf("  %zu keys of %zu bytes in %fs (%.2f ns/hash), symmetry = %.3f\n",
		   count, n, d, d / count * 1e9, sum / count / K);
}

TEST_CASE(hash_bench)
{
	const size_t N = 128 * 1024 * 1024;
	char* const buff = malloc(N);

	for(char* p = buff; p < buff + N; ++p)
		*p = 'a' + rand() % 26;

	for(size_t n = 8; n <= 64 * 1024; n *= 2)
		do_bench_hash(buff, N, n);

	free((void*)buff);
}

static
char* make_fixed_len_keys(const size_t n, const size_t len)
{
	static
	const char letters[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// 	_Static_assert((sizeof(letters) - 1) == 64, "invalid number of letters");

	const uint16_t mask = 64 - 1;

	uint16_t r = rand();
	char* const buff = malloc(n * len);

	for(size_t i = 0; i < n; ++i)
	{
		char* const base = buff + i * len;

		for(char* p = base + snprintf(base, len, "%zu_", i); p < base + len; ++p)
		{
			r = ((7 & (uintptr_t)p) == 7) ? rand() : (r >> 1);	// reuse random value
			*p = letters[r & mask];
		}
	}

	return buff;
}

static
void print_bench_results(const char* const op, const clock_t start, const size_t n)
{
	const double d = ((double)(clock() - start)) / CLOCKS_PER_SEC;

	printf("    %s: %fs (%zu ns/op, %zu op/s)\n",
		   op, d, (size_t)(d / n * 1e9), (size_t)(n / d));
}

static
void run_bench(const size_t n, const size_t len)
{
	smap map = smap_empty;

	TEST(smap_resize(&map, n) == 0);

	// print out stats
	printf("  %zu keys of %zu bytes (load factor %.2f, memory %.2f MB)\n",
		   n, len, (double)n / map.cap,
		   (map.cap * sizeof(smap_slot) + (sizeof(smap_entry) + len) * n)/(double)(1024 * 1024));

	char* const keys = make_fixed_len_keys(n, len);

	// add
	clock_t ts = clock();

	for(size_t i = 0; i < n; ++i)
		*smap_add(&map, keys + i * len, len) = (void*)(i + 1);

	print_bench_results("add", ts, n);

	TEST(map.count == n);

	// get
	ts = clock();

	for(size_t i = 0; i < n; ++i)
		TEST(*smap_get(&map, keys + i * len, len) == (void*)(i + 1));

	print_bench_results("get", ts, n);

	// del
	ts = clock();

	for(size_t i = 0; i < n; ++i)
		TEST(smap_del(&map, keys + i * len, len) == (void*)(i + 1));

	print_bench_results("del", ts, n);

	TEST(map.count == 0);

	// all done
	TEST(clear_map(&map) == 0);
}

TEST_CASE(smap_bench)
{
	run_bench(1000, 10);
	run_bench(1000, 32);
	run_bench(1000, 100);
	run_bench(1000, 316);
	run_bench(1000, 1000);
	run_bench(1000, 3162);
	run_bench(1000, 10000);
	run_bench(1000, 31623);
	run_bench(1000, 100000);
	run_bench(1000, 316228);
	run_bench(1000, 1000000);

	run_bench(10000, 10);
	run_bench(10000, 32);
	run_bench(10000, 100);
	run_bench(10000, 316);
	run_bench(10000, 1000);
	run_bench(10000, 3162);
	run_bench(10000, 10000);

	run_bench(100000, 10);
	run_bench(100000, 32);
	run_bench(100000, 100);
	run_bench(100000, 316);
	run_bench(100000, 1000);
	run_bench(100000, 3162);

	run_bench(1000000, 10);
	run_bench(1000000, 32);
	run_bench(1000000, 100);
	run_bench(1000000, 316);

#if __SIZEOF_SIZE_T__ == 8
	run_bench(1000000, 1000);

	run_bench(10000000, 10);
	run_bench(10000000, 32);
#endif
}
