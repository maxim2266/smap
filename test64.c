#include "smap.h"
#include "mite/mite.h"

#if __SIZEOF_SIZE_T__ != 8
#error "This is 64 bit code."
#endif

// wget https://raw.githubusercontent.com/rurban/smhasher/master/wyhash.h
/*
#include "wyhash.h"

static
uint64_t wy_hash(const smap_key key)
{
	return wyhash(key.ptr, (uint64_t)key.len, 123u, _wyp);
}

TEST_CASE(print_hashes)
{
	const size_t N = 65;

	char* const buff = calloc(N + 1, 1);
	char* end = buff;

	for(*end++ = 'a'; end <= buff + N; *end++ = 'a')
		printf("{ smap_lit(\"%s\"), (uint64_t)0x%zx },\n",
			   buff, wy_hash(smap_bytes(buff, end - buff)));

	free((void*)buff);
}
*/

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// defined in smap64.c
uint64_t _smap_calc_hash_s(const smap_key key, uint64_t seed);

TEST_CASE(hash_test)
{
	typedef struct
	{
		smap_key key;
		uint64_t  hash;
	} data_item;

	static const data_item data[] =
	{
		{ smap_lit("a"), (uint64_t)0x61204c7a5b048390 },
		{ smap_lit("aa"), (uint64_t)0xed3608a995b2f8a },
		{ smap_lit("aaa"), (uint64_t)0x36c693dbdf6bcb8c },
		{ smap_lit("aaaa"), (uint64_t)0x5a93884ecaaed237 },
		{ smap_lit("aaaaa"), (uint64_t)0x8da9ca986bf077f7 },
		{ smap_lit("aaaaaa"), (uint64_t)0xc0a674e9085a18b6 },
		{ smap_lit("aaaaaaa"), (uint64_t)0x7bbcb53aa6adba71 },
		{ smap_lit("aaaaaaaa"), (uint64_t)0xd768e5ff5aa2353f },
		{ smap_lit("aaaaaaaaa"), (uint64_t)0xa652648fbf5d6fc },
		{ smap_lit("aaaaaaaaaa"), (uint64_t)0xbd636099985f7bbe },
		{ smap_lit("aaaaaaaaaaa"), (uint64_t)0xf079a2eb36a11d61 },
		{ smap_lit("aaaaaaaaaaaa"), (uint64_t)0x8341feb1c059c83a },
		{ smap_lit("aaaaaaaaaaaaa"), (uint64_t)0x365e3f037ea36df9 },
		{ smap_lit("aaaaaaaaaaaaaa"), (uint64_t)0x6954795c1ff50eb8 },
		{ smap_lit("aaaaaaaaaaaaaaa"), (uint64_t)0x9c52bbadbc58907e },
		{ smap_lit("aaaaaaaaaaaaaaaa"), (uint64_t)0x7f1eea62505d2b25 },
		{ smap_lit("aaaaaaaaaaaaaaaaa"), (uint64_t)0x9602c1a318476ae7 },
		{ smap_lit("aaaaaaaaaaaaaaaaaa"), (uint64_t)0x10060badf8283894 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaa"), (uint64_t)0x9a1a5dd6583d0644 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xf430576c38a7466f },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x7e3399969888143e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xf837e383789ce3d2 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x820b358dd861b182 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x3deccfedb9dd2ab9 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xc7e0101619a1f86b },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x41e45a00f9b2c618 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xcbe7ac0d598795c8 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x241da7c33801d5fb },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xae11e9cd9812a3ad },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x281533f679e7715e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xb3e885e0d9c85f0e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xecdc99b8a5eb2c4c },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xa053bc0856cf8e7b },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x7c1cff47755f8300 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x8de3e8291ef94a9 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x4695763689fe4735 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x1156b175a48e58dc },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x2d17f0b0c31e6c67 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xf9d1338e1fae610d },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x73a484d45ebfdfeb },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xe65c6137d4fd0b2 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xda27016e99dfe446 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xf6e040adb46ff9ff },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x249f79c1ac7fa84e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xff58bb1ccb0fbd16 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x8b19fa5be79fb6bd },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xa7db4599022fca44 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xd1ae8eff453f203d },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x58ae8a01ad0e0d0e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xd6da0b48291c8297 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x2c478870952b17e1 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xf96ff0ff32adc21e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x769b7127beb85760 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xcc080e6e3946accd },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x45b58c96a555225b },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x8f850385018f430c },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x53280cd8d9dd862 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x82be01f409a82ddb },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x182b9e3cf5b6a34d },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xa5f306bb11396d4a },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x236087e39dc7e2d4 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xb8ec042a19d27821 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0x3619855285e0cd97 },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xdf096a2664bc573e },
		{ smap_lit("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), (uint64_t)0xaad6340d0d064c72 },
	};

	for(unsigned i = 0; i < sizeof(data)/sizeof(data[0]); ++i)
	{
		const uint64_t h = _smap_calc_hash_s(data[i].key, 123 ^ 0xa0761d6478bd642full);

		TEST(h == data[i].hash);
	}
}

#else	// #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#warning "Do not have data for hash test on a big-endian CPU."
#endif
