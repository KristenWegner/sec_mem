// transcode.c

#include "config.h"
#include "compatibility/gettimeofday.h"
#include "bits.h"


// Quick 64-bit randomize using Split Mix 64.
inline static uint64_t sm_qrand(register uint64_t *restrict v)
{
	*v += UINT64_C(0x9E3779B97F4A7C15);
	register uint64_t z = *v;
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}


////////////////////////////////////////////////////////////////////////////////
// Do a single XOR pass on the given data using XorShift1024* 64-bit values
// generated using the given seed (key).
////////////////////////////////////////////////////////////////////////////////
void* sm_xor_pass(void *restrict data, register size_t bytes, uint64_t key)
{
	uint64_t v[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	register uint8_t i, p = 0;
	register uint64_t z;

	// Seed the state vector.

	for (i = 0; i < 16; ++i)
	{
		z = (key += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		v[i] = z ^ (z >> 31);
	}

	// Process the data buffer in-place.

	register uint8_t* d = data;

	while (bytes-- > 0)
	{
		const uint64_t s0 = v[p];
		uint64_t s1 = sm_shuffle_64(v[p = (p + 1) & 0x0F]);
		s1 ^= s1 << 31;
		v[p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
		*d = *d ^ sm_fold_64_to_8(v[p] * UINT64_C(0x9E3779B97F4A7C13));
		d++;
	}

	// Scramble the temporary state vector.

	for (i = 0; i < 16; ++i)
		v[i] ^= (v[i] - i & 1) ? sm_rotl_64(sm_qrand(&v[i]), i) : sm_rotr_64(sm_qrand(&v[15 - i]), i);

	return data;
}


////////////////////////////////////////////////////////////////////////////////
// Cross XOR using XorShift1024* 64-bit, decoding into dst using key1 (the 
// first seed), while byte by byte recoding src again using key2 (the second 
// seed). Returns dst.
////////////////////////////////////////////////////////////////////////////////
void* sm_xor_cross(void *restrict dst, void *restrict src, register size_t bytes, uint64_t key1, uint64_t key2)
{
	const uint64_t ca = UINT64_C(0x9E3779B97F4A7C15);
	const uint64_t cb = UINT64_C(0xBF58476D1CE4E5B9);
	const uint64_t cc = UINT64_C(0x94D049BB133111EB);
	const uint64_t cd = UINT64_C(0x9E3779B97F4A7C13);

	register uint64_t z1, z2;
	register uint8_t i, p1 = 0, p2 = 0;

	uint64_t v1[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint64_t v2[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	
	// Seed the state vectors.

	for (i = 0; i < 16; ++i)
	{
		z1 = (key1 += ca);
		z2 = (key2 += ca);

		z1 = (z1 ^ (z1 >> 30)) * cb;
		z2 = (z2 ^ (z2 >> 30)) * cb;

		z1 = (z1 ^ (z1 >> 27)) * cc;
		z2 = (z2 ^ (z2 >> 27)) * cc;

		v1[i] = z1 ^ (z1 >> 31);
		v2[i] = z2 ^ (z2 >> 31);
	}

	// Process the buffers.

	register uint8_t* s = src;
	register uint8_t* d = dst;

	while (bytes-- > 0)
	{
		// Generate two new random values using modified XorShift1024*.

		const uint64_t s0 = v1[p1];
		const uint64_t t0 = v2[p2];

		uint64_t s1 = sm_shuffle_64(v1[p1 = (p1 + 1) & 0x0F]);
		uint64_t t1 = sm_shuffle_64(v2[p2 = (p2 + 1) & 0x0F]);

		s1 ^= s1 << 31;
		t1 ^= t1 << 31;

		v1[p1] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);
		v2[p2] = t1 ^ t0 ^ (t1 >> 11) ^ (t0 >> 30);

		// Decode/encode.

		*d = *s ^ sm_fold_64_to_8(v1[p1] * cd); // Decode XOR src into dst.
		*s = *d ^ sm_fold_64_to_8(v2[p2] * cd); // Recode XOR dst into src.

		// Move next.

		d++;
		s++;
	}

	// Scramble the temporary state vectors.

	for (i = 0; i < 16; ++i)
	{
		v1[i] ^= ((v2[i] - i) & 1) ? sm_rotl_64(sm_qrand(&v2[i]), i) : sm_rotr_64(sm_qrand(&v2[15 - i]), i);
		v2[i] ^= ((v2[1] + i) & 1) ? sm_rotr_64(sm_qrand(&v1[15 - i]), i) : sm_rotl_64(sm_qrand(&v1[i]), i);
	}

	return dst;
}


////////////////////////////////////////////////////////////////////////////////
// 
// In-place transcode ptr of len bytes using sequence with state seeded by seed.
//
// Parameters:
//
//     - encode: If true then encode, else decode.
//     - ptr: Pointer to data to encode.
//     - len: Count of bytes to ptr.
//     - key: The random key (seed).
//     - state: Random state buffer.
//     - size: State size in bytes.
//     - seed: Random seed function.
//     - random: Random generate function.
//     - memory: Memory context.
//
// Returns a pointer to the start of the transcoded buffer.
// 
////////////////////////////////////////////////////////////////////////////////
exported void* callconv sm_transcode(uint8_t encode, void *restrict data, register size_t bytes, uint64_t key, void* state, size_t size, void(*seed)(void*, uint64_t), uint64_t(*random)(void*))
{
	register uint8_t* sp = (uint8_t*)state; // Zero the random state.
	register size_t sn = size;
	while (sn-- > 0) *sp++ = 0;

	seed(state, key); // Seed the state.

	register uint8_t* pd = (uint8_t*)data; // Pointer to the data.
	register union { uint64_t q; uint8_t b[sizeof(uint64_t)]; } un; // Union for getting bytes from the random value.

	while (bytes-- > UINT64_C(0))
	{
		un.q = random(state); // Get the next random number. 

		register uint8_t ix = (un.b[0] == 0) ? un.b[0] : un.b[0] % sizeof(uint64_t); // Compute index to byte to use for XOR.
		uint8_t sb = (uint8_t)sm_unzip_64(un.q) % (sizeof(uint64_t) * CHAR_BIT); // Get an index between 0 and 63.
		uint8_t ro = (un.b[sizeof(uint64_t) / 2] % 8); // How many bits to rotate by.
		uint64_t rm = sm_yellow_64(un.q); // Get a mask for determining the directionality of the bit rotation. 
		register uint8_t bb = *pd; // Get the current byte.

		if (encode) // Encode.
		{
			// Do the XOR.

			bb ^= un.b[ix]; // XOR using the byte at offset ix.

			// Rotate in the determined direction, and count of bits.

			if (sm_is_bit_set_64(rm, sb)) // If the bit at ix is set:
				bb = sm_rotl_8(bb, ro); // Rotate left by ro.
			else bb = sm_rotr_8(bb, ro); // Rotate right by ro.
		}
		else // Decode.
		{
			// Rotate in the opposite of the above determined direction, and count of bits.

			if (sm_is_bit_set_64(rm, sb)) // If the bit at sb is not set:
				bb = sm_rotr_8(bb, ro); // Rotate right by ro.
			else bb = sm_rotl_8(bb, ro); // Rotate left by ro.

			// Do the XOR.

			bb ^= un.b[ix]; // XOR using the byte at offset ix.
		}

		*pd++ = bb; // Set the byte and increment.
	}

	key ^= sm_yellow_64(~key) ^ sm_unzip_64(sm_qrand(&key)); // Scramble the key.

	sp = (uint8_t*)state;
	sn = size;

	while (sn-- > 0) // Scramble the random state before returning.
		*sp++ = (uint8_t)sm_qrand(&key);

	return data;
}

