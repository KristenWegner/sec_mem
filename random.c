// random.c


#include "config.h"


static const uint64_t _k[80] =
{
	UINT64_C(0x428A2F98D728AE22),  UINT64_C(0x7137449123EF65CD),
	UINT64_C(0xB5C0FBCFEC4D3B2F),  UINT64_C(0xE9B5DBA58189DBBC),
	UINT64_C(0x3956C25BF348B538),  UINT64_C(0x59F111F1B605D019),
	UINT64_C(0x923F82A4AF194F9B),  UINT64_C(0xAB1C5ED5DA6D8118),
	UINT64_C(0xD807AA98A3030242),  UINT64_C(0x12835B0145706FBE),
	UINT64_C(0x243185BE4EE4B28C),  UINT64_C(0x550C7DC3D5FFB4E2),
	UINT64_C(0x72BE5D74F27B896F),  UINT64_C(0x80DEB1FE3B1696B1),
	UINT64_C(0x9BDC06A725C71235),  UINT64_C(0xC19BF174CF692694),
	UINT64_C(0xE49B69C19EF14AD2),  UINT64_C(0xEFBE4786384F25E3),
	UINT64_C(0x0FC19DC68B8CD5B5),  UINT64_C(0x240CA1CC77AC9C65),
	UINT64_C(0x2DE92C6F592B0275),  UINT64_C(0x4A7484AA6EA6E483),
	UINT64_C(0x5CB0A9DCBD41FBD4),  UINT64_C(0x76F988DA831153B5),
	UINT64_C(0x983E5152EE66DFAB),  UINT64_C(0xA831C66D2DB43210),
	UINT64_C(0xB00327C898FB213F),  UINT64_C(0xBF597FC7BEEF0EE4),
	UINT64_C(0xC6E00BF33DA88FC2),  UINT64_C(0xD5A79147930AA725),
	UINT64_C(0x06CA6351E003826F),  UINT64_C(0x142929670A0E6E70),
	UINT64_C(0x27B70A8546D22FFC),  UINT64_C(0x2E1B21385C26C926),
	UINT64_C(0x4D2C6DFC5AC42AED),  UINT64_C(0x53380D139D95B3DF),
	UINT64_C(0x650A73548BAF63DE),  UINT64_C(0x766A0ABB3C77B2A8),
	UINT64_C(0x81C2C92E47EDAEE6),  UINT64_C(0x92722C851482353B),
	UINT64_C(0xA2BFE8A14CF10364),  UINT64_C(0xA81A664BBC423001),
	UINT64_C(0xC24B8B70D0F89791),  UINT64_C(0xC76C51A30654BE30),
	UINT64_C(0xD192E819D6EF5218),  UINT64_C(0xD69906245565A910),
	UINT64_C(0xF40E35855771202A),  UINT64_C(0x106AA07032BBD1B8),
	UINT64_C(0x19A4C116B8D2D0C8),  UINT64_C(0x1E376C085141AB53),
	UINT64_C(0x2748774CDF8EEB99),  UINT64_C(0x34B0BCB5E19B48A8),
	UINT64_C(0x391C0CB3C5C95A63),  UINT64_C(0x4ED8AA4AE3418ACB),
	UINT64_C(0x5B9CCA4F7763E373),  UINT64_C(0x682E6FF3D6B2B8A3),
	UINT64_C(0x748F82EE5DEFB2FC),  UINT64_C(0x78A5636F43172F60),
	UINT64_C(0x84C87814A1F0AB72),  UINT64_C(0x8CC702081A6439EC),
	UINT64_C(0x90BEFFFA23631E28),  UINT64_C(0xA4506CEBDE82BDE9),
	UINT64_C(0xBEF9A3F7B2C67915),  UINT64_C(0xC67178F2E372532B),
	UINT64_C(0xCA273ECEEA26619C),  UINT64_C(0xD186B8C721C0C207),
	UINT64_C(0xEADA7DD6CDE0EB1E),  UINT64_C(0xF57D4F7FEE6ED178),
	UINT64_C(0x06F067AA72176FBA),  UINT64_C(0x0A637DC5A2C898A6),
	UINT64_C(0x113F9804BEF90DAE),  UINT64_C(0x1B710B35131C471B),
	UINT64_C(0x28DB77F523047D84),  UINT64_C(0x32CAAB7B40C72493),
	UINT64_C(0x3C9EBE0A15C9BEBC),  UINT64_C(0x431D67C49C100D4C),
	UINT64_C(0x4CC5D4BECB3E42B6),  UINT64_C(0x597F299CFC657E2A),
	UINT64_C(0x5FCB6FAB3AD6FAEC),  UINT64_C(0x6C44198C4A475817)
};


const uint64_t* sec_random_get_k(void)
{
	return &_k[0];
}
