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

#if __SIZEOF_SIZE_T__ != 4
#error "This is 32 bit code."
#endif

#include "smap.h"
#include "mite/mite.h"

// wget https://raw.githubusercontent.com/rurban/smhasher/master/wyhash32.h
/*
#include "wyhash32.h"

static
uint32_t wy_hash(const smap_key key)
{
	return wyhash32(key.ptr, (uint64_t)key.len, 123u);
}

TEST_CASE(print_hashes)
{
	const size_t N = 20;

	char* const buff = calloc(N + 1, 1);
	char* end = buff;

	for(*end++ = 'a'; end <= buff + N; *end++ = 'a')
		printf("{ smap_lit(\"%s\"), 0x%xu },\n",
			   buff, wy_hash(smap_bytes(buff, end - buff)));

	free((void*)buff);
}
*/

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// defined in smap32.c
uint32_t _smap_calc_hash_s(const smap_key s, const uint32_t seed);

TEST_CASE(hash_test)
{
	typedef struct
	{
		smap_key key;
		uint32_t  hash;
	} data_item;

	static const data_item data[] =
	{
		{ smap_lit("a"), 0x486ee3b5u },
		{ smap_lit("aa"), 0x8ed7592bu },
		{ smap_lit("aaa"), 0x1030e630u },
		{ smap_lit("aaaa"), 0xe260fae1u },
		{ smap_lit("aaaaa"), 0xf0c6647u },
		{ smap_lit("aaaaaa"), 0x442bb04bu },
		{ smap_lit("aaaaaaa"), 0xcc2b09f9u },
		{ smap_lit("aaaaaaaa"), 0xa5000a28u },
		{ smap_lit("aaaaaaaaa"), 0x8298032fu },
		{ smap_lit("aaaaaaaaaa"), 0x3babb0adu },
		{ smap_lit("aaaaaaaaaaa"), 0x672bd77bu },
		{ smap_lit("aaaaaaaaaaaa"), 0xf9e4993fu },
		{ smap_lit("aaaaaaaaaaaaa"), 0xa1f89732u },
		{ smap_lit("aaaaaaaaaaaaaa"), 0xd294d7b8u },
		{ smap_lit("aaaaaaaaaaaaaaa"), 0x6bdd5d62u },
		{ smap_lit("aaaaaaaaaaaaaaaa"), 0x9b0bbc66u },
		{ smap_lit("aaaaaaaaaaaaaaaaa"), 0x601a7060u },
		{ smap_lit("aaaaaaaaaaaaaaaaaa"), 0x6f71e653u },
		{ smap_lit("aaaaaaaaaaaaaaaaaaa"), 0x5ad1a38fu },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaa"), 0x523ee9a9u }
	};

	for(unsigned i = 0; i < sizeof(data)/sizeof(data[0]); ++i)
	{
		const uint32_t h = _smap_calc_hash_s(data[i].key, 123u);

		TEST(h == data[i].hash);
	}
}

#else
#warning "Cannot run hash test on a big-endian CPU."
#endif
