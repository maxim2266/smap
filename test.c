/*
BSD 3-Clause License

Copyright (c) 2021, Maxim Konakov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "smap.h"
#include "mite/mite.h"

#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

TEST_CASE(smap_key_test)
{
	TEST(smap_lit("zzz").len == 3);
	TEST(strcmp(smap_lit("zzz").ptr, "zzz") == 0);

	const char str[] = "zzz\0z";

	TEST(smap_str(str).len == 3);
	TEST(strcmp(smap_str(str).ptr, "zzz") == 0);

	smap_key k = smap_bytes(str + 1, 2);

	TEST(k.len == 2);
	TEST(strcmp(k.ptr, "zz") == 0);
}

// defined in smap64.c
uint32_t _smap_calc_mask(const uint32_t cap);
uint32_t _smap_calc_hash(const smap_key key);

static
void bench_hash(char* const buff, const size_t N, const size_t n)
{
	uint64_t sum = 0;
	const clock_t start = clock();

	for(const char* s = buff; s < buff + N; s += n)
		sum += _smap_calc_hash(smap_bytes(s, n));

	const double d = (double)(clock() - start) / CLOCKS_PER_SEC;
	const size_t count = N / n;

	printf("  %zu keys of %zu bytes in %fs (%.2f ns/hash), symmetry = %.4f\n",
		   count, n, d, d / count * 1e9, (sum / count) / (double)0x80000000u);
}

TEST_CASE(hash_bench)
{
	srand(getpid() ^ time(NULL));

	const size_t N = 128 * 1024 * 1024;
	char* const buff = malloc(N);

	for(char* p = buff; p < buff + N; ++p)
		*p = 'a' + rand() % 26;

	for(size_t n = 8; n <= 64 * 1024; n *= 2)
		bench_hash(buff, N, n);

	free((void*)buff);
}

static size_t release_count = 0;

static
void count_releases(void* UNUSED(p)) { ++release_count; }

static
int count_keys(void* param, const smap_key UNUSED(key), void** pval)
{
	++(*(size_t*)param);

	return *pval == NULL;
}

TEST_CASE(basic_test)
{
	smap map;

	TEST(smap_init(&map, 0) == 0);

	*smap_add(&map, smap_lit("aaa")) = (void*)1;
	*smap_add(&map, smap_lit("bbb")) = (void*)2;
	*smap_add(&map, smap_lit("ccc")) = (void*)3;
	TEST(map.count == 3);
	TEST(map.mask == 0xf);

	TEST(*smap_get(&map, smap_lit("aaa")) == (void*)1);
	TEST(*smap_get(&map, smap_lit("bbb")) == (void*)2);
	TEST(*smap_get(&map, smap_lit("ccc")) == (void*)3);

	TEST(!smap_get(&map, smap_lit("ddd")));
	TEST(!smap_del(&map, smap_lit("ddd")));

	TEST(smap_del(&map, smap_lit("aaa")) == (void*)1);
	TEST(!smap_get(&map, smap_lit("aaa")));
	TEST(map.count == 2);

	*smap_add(&map, smap_lit("")) = (void*)42;
	TEST(*smap_get(&map, smap_lit("")) == (void*)42);

	const int k = 123;

	*smap_add(&map, smap_bin(k)) = (void*)17;
	TEST(*smap_get(&map, smap_bin(k)) == (void*)17);

	size_t key_count = 0;

	TEST(smap_scan(&map, count_keys, &key_count) == 0);
	TEST(key_count == 4);

	smap_release(&map, count_releases);
	TEST(release_count == 4);
}

static
char* make_fixed_len_keys(const size_t n, const size_t len)
{
// 	TEST(len >= 10);

	static
	const char letters[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	_Static_assert((sizeof(letters) - 1) == 64, "invalid number of letters");

	const uint16_t mask = 64 - 1;

	uint16_t r = rand();
	char* const buff = malloc(n * len);

	for(size_t i = 0; i < n; ++i)
	{
		char* const base = buff + i * len;

		for(char* p = base + snprintf(base, len, "%zu_", i); p < base + len; ++p)
		{
			r = ((7 & (uintptr_t)p) == 7) ? rand() : (r >> 1);
			*p = letters[r & mask];
		}
	}

	return buff;
}

static
void do_random_test(const size_t n, const size_t len)
{
	release_count = 0;

	char* const keys = make_fixed_len_keys(n, len);

	smap map;

	TEST(smap_init(&map, 0) == 0);

	// add n keys
	for(size_t i = 0; i < n; ++i)
		*smap_add(&map, smap_bytes(keys + i * len, len)) = (void*)(i + 1);

	TEST(map.count == n);
	TEST(map.mask == _smap_calc_mask(n));

	// check n keys
	for(size_t i = 0; i < n; ++i)
		TEST(*smap_get(&map, smap_bytes(keys + i * len, len)) == (void*)(i + 1));

	// delete odd keys
	for(size_t i = 1; i < n; i += 2)
		TEST(smap_del(&map, smap_bytes(keys + i * len, len)) == (void*)(i + 1));

	// check map
	TEST(map.count == n / 2);

	for(size_t i = 0; i < n; ++i)
	{
		void** const p = smap_get(&map, smap_bytes(keys + i * len, len));

		if(i & 1)
			TEST(!p);
		else
			TEST(*p == (void*)(i + 1));
	}

	smap_release(&map, count_releases);
	TEST(release_count == n / 2);

	free(keys);
}

TEST_CASE(random_test)
{
	srand(time(NULL) ^ getpid());

	do_random_test((64 * 1024 - 1) - (64 * 1024 - 1) / 4, 64);
}

TEST_CASE(big_random_test)
{
#ifdef __OPTIMIZE__
	do_random_test(1000000, 20);
#else
	do_random_test(500000, 20);
#endif
}

static
int inc_value(void* param, const smap_key key, void** pval)
{
	if(key.len != 3 || !strchr("abc", *key.ptr))
		return 42;

	*(size_t*)pval += (size_t)param;
	return 0;
}

static
int delete_key(void* param, const smap_key key, void** UNUSED(pval))
{
	return (key.len == 3 && memcmp(key.ptr, param, key.len) == 0) ? -1 : 0;
}

TEST_CASE(scan_test)
{
	smap map;

	TEST(smap_init(&map, 0) == 0);

	*smap_add(&map, smap_lit("aaa")) = (void*)1;
	*smap_add(&map, smap_lit("bbb")) = (void*)2;
	*smap_add(&map, smap_lit("ccc")) = (void*)3;

	TEST(map.count == 3);
	TEST(map.mask == 0xf);

	TEST(smap_scan(&map, inc_value, (void*)1) == 0);

	TEST(*smap_get(&map, smap_lit("aaa")) == (void*)2);
	TEST(*smap_get(&map, smap_lit("bbb")) == (void*)3);
	TEST(*smap_get(&map, smap_lit("ccc")) == (void*)4);

	TEST(smap_scan(&map, delete_key, "bbb") == 0);

	TEST(*smap_get(&map, smap_lit("aaa")) == (void*)2);
	TEST(!smap_get(&map, smap_lit("bbb")));
	TEST(*smap_get(&map, smap_lit("ccc")) == (void*)4);

	TEST(map.count == 2);

	release_count = 0;
	smap_release(&map, count_releases);

	TEST(release_count == 2);
}

TEST_CASE(compact_test)
{
	smap map;

	TEST(smap_init(&map, 100) == 0);

	*smap_add(&map, smap_lit("aaa")) = (void*)1;
	*smap_add(&map, smap_lit("bbb")) = (void*)2;
	*smap_add(&map, smap_lit("ccc")) = (void*)3;

	TEST(smap_compact(&map) == 0);
	TEST(map.mask == 0xf);
	TEST(map.count == 3);

	TEST(*smap_get(&map, smap_lit("aaa")) == (void*)1);
	TEST(*smap_get(&map, smap_lit("bbb")) == (void*)2);
	TEST(*smap_get(&map, smap_lit("ccc")) == (void*)3);

	release_count = 0;

	smap_release(&map, count_releases);
	TEST(release_count == 3);
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
	printf("  %zu keys of %zu bytes (load factor %.2f, memory %.2f Mb)\n",
		   n, len, (double)n / _smap_calc_mask(n),
#if __SIZEOF_SIZE_T__ == 8
		   (((_smap_calc_mask(n) + 1) * sizeof(uintptr_t) + (16 + len) * n)) / (double)(1024 * 1024));
#else
		   (((_smap_calc_mask(n) + 1) * 8 + (8 + len) * n)) / (double)(1024 * 1024));
#endif

	char* const keys = make_fixed_len_keys(n, len);

	smap map;

	TEST(smap_init(&map, n) == 0);

	// set
	clock_t ts = clock();

	for(size_t i = 0; i < n; ++i)
		*smap_add(&map, smap_bytes(keys + i * len, len)) = (void*)(i + 1);

	print_bench_results("set", ts, n);

	TEST(map.count == n);
	TEST(map.mask == _smap_calc_mask(n));

	// get
	ts = clock();

	for(size_t i = 0; i < n; ++i)
		TEST(*smap_get(&map, smap_bytes(keys + i * len, len)) == (void*)(i + 1));

	print_bench_results("get", ts, n);

	// del
	ts = clock();

	for(size_t i = 0; i < n; ++i)
		TEST(smap_del(&map, smap_bytes(keys + i * len, len)) == (void*)(i + 1));

	print_bench_results("del", ts, n);

	TEST(map.count == 0);

	// all done
	release_count = 0;

	smap_release(&map, count_releases);
	free(keys);

	TEST(release_count == 0);
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
	run_bench(1000000, 1000);

#ifdef __OPTIMIZE__
	run_bench(10000000, 10);
	run_bench(10000000, 32);
	run_bench(10000000, 100);
#endif
}
