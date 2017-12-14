// cmp.c - Basic compression.


#include "../config.h"


inline static uint8_t sm_compress_size(const void* src, uint64_t slen, uint64_t* dlen)
{
	uint8_t const *ip = (const uint8_t*)src;
	uint8_t const *const ie = ip + slen;
	uint64_t tl = 0;

	while (ip < ie)
	{
		uint64_t ct = *ip++;

		if (ct < (UINT64_C(1) << 5))
		{
			ct++;

			if (ip + ct > ie)
				return 2;

			tl += ct;
			ip += ct;
		}
		else
		{
			uint64_t ln = (ct >> 5);

			if (ln == 7)
				ln += *ip++;

			ln += 2;

			if (ip >= ie)
				return 2;

			ip++;

			tl += ln;
		}
	}

	*dlen = tl;

	return 0;
}


exported uint8_t callconv sec_compress(const void *const src, const uint64_t slen, void *dst, uint64_t *const dlen, void* htab)
{
	const uint8_t** hs;
	uint64_t hv;
	const uint8_t *rf;
	const uint8_t *ip = (const uint8_t*)src;
	const uint8_t *const ie = ip + slen;
	uint8_t *op = (uint8_t*)dst;
	const uint8_t *const oe = (dlen == NULL) ? NULL : op + *dlen;
	int64_t lt;
	uint64_t of;
	uint8_t** ht = htab;

	if (dlen == 0) return 3;
	if (ht == 0) return 3;

	if (src == 0)
	{
		if (slen != 0) return 3;
		*dlen = UINT64_C(0);
		return 0;
	}

	if (dst == NULL)
	{
		if (dlen != 0) return 3;
		return sm_compress_size(src, slen, dlen);
	}

	register uint8_t* p = (uint8_t*)ht; // Zero the htab.
	register size_t n = sizeof(uint8_t*) * UINT64_C(0x10000);
	while (n-- > UINT64_C(0)) *p++ = 0;

	lt = 0;
	op++;

	hv = ((ip[0] << 8) | ip[1]);

	while (ip + 2 < ie)
	{
		hv = ((hv << 8) | ip[2]);
		hs = ht + (((hv >> (3 * 8 - 0x10)) - hv) & UINT64_C(0xFFFF));
		rf = *hs;
		*hs = ip;

		if (rf < ip && (of = ip - rf - 1) < 0x2000 && ip + 4 < ie &&  rf > (uint8_t*)src &&  rf[0] == ip[0] &&
			rf[1] == ip[1] && rf[2] == ip[2])
		{
			uint64_t ln = UINT64_C(3);
			const uint64_t xl = ie - ip - 2 > 0x108 ? UINT64_C(0x108) : ie - ip - 2;

			if (op - !lt + 3 + 1 >= oe)
				return 1;

			op[-lt - INT64_C(1)] = lt - 1;
			op -= !lt;

			while (ln < xl && rf[ln] == ip[ln])
				ln++;

			ln -= UINT64_C(2);

			if (ln < 7)
			{
				*op++ = (of >> 8) + (ln << 5);
				*op++ = of;
			}
			else
			{
				*op++ = (uint8_t)((of >> 8) + UINT64_C(0xE0));
				*op++ = (uint8_t)(ln - UINT64_C(7));
				*op++ = (uint8_t)of;
			}

			lt = INT64_C(0), op++;

			ip += ln + UINT64_C(1);

			if (ip + 3 >= ie)
			{
				ip++;
				break;
			}

			hv = (uint64_t)((ip[0] << 8) | ip[1]);
			hv = ((hv << 8) | (uint64_t)ip[2]);
			ht[(((hv >> (3 * 8 - 0x10)) - hv) & UINT64_C(0xFFFF))] = ip;
			ip++;
		}
		else
		{
			if (op >= oe)
				return 1;

			lt++;
			*op++ = *ip++;

			if (lt == INT64_C(0x20))
			{
				op[-lt - INT64_C(1)] = (uint8_t)(lt - INT64_C(1));
				lt = INT64_C(0), op++;
			}
		}
	}

	if (op + 3 > oe) return 1;

	while (ip < ie)
	{
		lt++;
		*op++ = *ip++;

		if (lt == INT64_C(0x20))
		{
			op[-lt - 1] = (uint8_t)(lt - INT64_C(1));
			lt = INT64_C(0), op++;
		}
	}

	op[-lt - INT64_C(1)] = (uint8_t)(lt - INT64_C(1));
	op -= !lt;

	*dlen = op - (uint8_t*)dst;

	return 0;
}


exported uint8_t callconv sec_decompress(const void* src, uint64_t slen, void* dst, uint64_t* dlen)
{
	uint8_t const *ip = (const uint8_t*)src;
	uint8_t const *const ie = ip + slen;
	uint8_t *op = (uint8_t*)dst;
	uint8_t const *const oe = (dlen == UINT64_C(0)) ? 0 : op + *dlen;
	uint64_t rl = UINT64_C(0);
	uint8_t rc;

	if (dlen == NULL)
		return 3;

	if (src == NULL)
	{
		if (slen != UINT64_C(0))
			return 3;

		*dlen = UINT64_C(0);

		return 0;
	}

	if (dst == NULL)
	{
		if (dlen != NULL)
			return 3;

		return sm_compress_size(src, slen, dlen);
	}

	do
	{
		uint64_t ct = *ip++;

		if (ct < UINT64_C(0x20))
		{
			ct++;

			if (op + ct > oe)
			{
				--ip;
				goto LOC_GUESS;
			}

			if (ip + ct > ie)
				return 2;

			do *op++ = *ip++;
			while (--ct);
		}
		else
		{
			uint64_t ln = (ct >> 5);
			uint8_t *rf = op - ((ct & UINT64_C(0x1F)) << 8) - 1;

			if (ln == UINT64_C(7)) ln += *ip++;

			ln += 2;

			if (op + ln > oe)
			{
				ip -= (ln >= UINT64_C(9)) ? 2 : 1;
				goto LOC_GUESS;
			}

			if (ip >= ie)
				return 2;

			rf -= *ip++;

			if (rf < (uint8_t*)dst)
				return 2;

			do *op++ = *rf++;
			while (--ln);
		}
	}
	while (ip < ie);

	*dlen = op - (uint8_t*)dst;

	return 0;

LOC_GUESS:

	rc = sm_compress_size(ip, slen - (ip - (uint8_t*)src), &rl);

	if (rc >= 0)
		*dlen = rl + (op - (uint8_t*)dst);

	return rc;
}
