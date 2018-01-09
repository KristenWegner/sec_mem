// hsh_decl.h - Auto-Generated (Mon Jan 8 14:49:00 2018): Declarations for the 'hsh' module. Include in your module header file.

#define CITY_HASH (0xE5F3U) // Protected Function: CityHash, 64-bit hash function (sm_hsh64_f) by Pike and Alakuijala of Google. Note: This uses, at maximum, 8 bytes of input.
#define MURMUR_3_HASH (0x82F0U) // Bootstrap/Integral Function: Murmur 3, 64-bit hash function (sm_hsh64_f) by Austin Appleby.
#define MURMUR_3_32_HASH (0x91F7U) // Protected Function: Murmur 3, 32-bit hash function (sm_hsh32_f) by Austin Appleby.
#define FNV1A_HASH (0x4BAAU) // Bootstrap/Integral Function: Fowler/Noll/Vo-0 FNV-1A 64-bit hash function (sm_hsh64_f).
#define SHA_3_STATE (0x0B39U) // Size: SHA-3 (FIPS PUB 202) 512-bit un-keyed cryptographic hash state size.
#define SHA_3_RESULT (0xEF68U) // Size: SHA-3 (FIPS PUB 202) 512-bit un-keyed cryptographic hash result size.
#define SHA_3_KEYED (0xF7F3U) // Size: SHA-3 (FIPS PUB 202) 512-bit un-keyed cryptographic hash keyed flag.
#define SHA_3_HASH (0xFF18U) // Protected Function: SHA-3 (FIPS PUB 202) 512-bit un-keyed cryptographic hash function (sm_chsh_f) by the Keccak Team.
#define SIP_STATE (0x3A3EU) // Size: SipHash 24 64-bit cryptographic keyed hash state size.
#define SIP_RESULT (0x6A3FU) // Size: SipHash 24 64-bit cryptographic keyed hash result size.
#define SIP_KEYED (0xEAB6U) // Size: SipHash 24 64-bit cryptographic keyed hash keyed flag.
#define SIP_HASH (0x458AU) // Protected Function: SipHash 24 64-bit cryptographic keyed hash function (sm_chsh_f) by Aumasson and Bernstein.
#define HIGHWAY_STATE (0x3974U) // Size: Highway 64-bit cryptographic keyed hash state size.
#define HIGHWAY_RESULT (0x0909U) // Size: Highway 64-bit cryptographic keyed hash result size.
#define HIGHWAY_KEYED (0xFFA5U) // Size: Highway 64-bit cryptographic keyed hash keyed flag.
#define HIGHWAY_HASH (0x04D0U) // Protected Function: Highway 64-bit cryptographic keyed hash function (sm_chsh_f) by Aumasson and Bernstein.
