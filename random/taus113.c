
#include <stdlib.h>

/*
0xFFFFFFFF // Max
0 //Min
*/

#define LCG(n) ((69069UL * n) & 0xFFFFFFFFUL)
#define MASK 0xFFFFFFFFUL

static inline unsigned long int taus113_get (void *vstate);
static double taus113_get_double (void *vstate);
static void taus113_set (void *state, unsigned long int s);

typedef struct
{
  unsigned long int z1, z2, z3, z4;
}
taus113_state_t;

static inline unsigned long taus113_get (void *vstate)
{
  taus113_state_t *state = (taus113_state_t *) vstate;
  unsigned long b1, b2, b3, b4;
  b1 = ((((state->z1 << 6UL) & MASK) ^ state->z1) >> 13UL);
  state->z1 = ((((state->z1 & 4294967294UL) << 18UL) & MASK) ^ b1);
  b2 = ((((state->z2 << 2UL) & MASK) ^ state->z2) >> 27UL);
  state->z2 = ((((state->z2 & 4294967288UL) << 2UL) & MASK) ^ b2);
  b3 = ((((state->z3 << 13UL) & MASK) ^ state->z3) >> 21UL);
  state->z3 = ((((state->z3 & 4294967280UL) << 7UL) & MASK) ^ b3);
  b4 = ((((state->z4 << 3UL) & MASK) ^ state->z4) >> 12UL);
  state->z4 = ((((state->z4 & 4294967168UL) << 13UL) & MASK) ^ b4);
  return (state->z1 ^ state->z2 ^ state->z3 ^ state->z4);

}

static double taus113_get_double (void *vstate)
{
  return taus113_get (vstate) / 4294967296.0;
}

static void taus113_set (void *vstate, unsigned long int s)
{
  taus113_state_t *state = (taus113_state_t *) vstate;
  if (!s) s = 1UL;
  state->z1 = LCG (s);
  if (state->z1 < 2UL) state->z1 += 2UL;
  state->z2 = LCG (state->z1);
  if (state->z2 < 8UL) state->z2 += 8UL;
  state->z3 = LCG (state->z2);
  if (state->z3 < 16UL) state->z3 += 16UL;
  state->z4 = LCG (state->z3);
  if (state->z4 < 128UL) state->z4 += 128UL;
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);
  taus113_get (state);

  return;
}

