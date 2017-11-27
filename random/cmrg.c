
#include <stdlib.h>

/*
2147483646 //Max
0 // Min
*/

static inline unsigned long int cmrg_get (void *vstate);
static double cmrg_get_double (void *vstate);
static void cmrg_set (void *state, unsigned long int s);

static const long int m1 = 2147483647, m2 = 2145483479;

static const long int a2 = 63308, qa2 = 33921, ra2 = 12979;
static const long int a3 = -183326, qa3 = 11714, ra3 = 2883;
static const long int b1 = 86098, qb1 = 24919, rb1 = 7417;
static const long int b3 = -539608, qb3 = 3976, rb3 = 2071;

typedef struct
  {
    long int x1, x2, x3;        /* first component */
    long int y1, y2, y3;        /* second component */
  }
cmrg_state_t;

static inline unsigned long int cmrg_get (void *vstate)
{
  cmrg_state_t *state = (cmrg_state_t *) vstate;

  /* Component 1 */

  {
    long int h3 = state->x3 / qa3;
    long int p3 = -a3 * (state->x3 - h3 * qa3) - h3 * ra3;

    long int h2 = state->x2 / qa2;
    long int p2 = a2 * (state->x2 - h2 * qa2) - h2 * ra2;

    if (p3 < 0)
      p3 += m1;
    if (p2 < 0)
      p2 += m1;

    state->x3 = state->x2;
    state->x2 = state->x1;
    state->x1 = p2 - p3;
    if (state->x1 < 0)
      state->x1 += m1;
  }

  /* Component 2 */

  {
    long int h3 = state->y3 / qb3;
    long int p3 = -b3 * (state->y3 - h3 * qb3) - h3 * rb3;

    long int h1 = state->y1 / qb1;
    long int p1 = b1 * (state->y1 - h1 * qb1) - h1 * rb1;

    if (p3 < 0)
      p3 += m2;
    if (p1 < 0)
      p1 += m2;

    state->y3 = state->y2;
    state->y2 = state->y1;
    state->y1 = p1 - p3;
    if (state->y1 < 0)
      state->y1 += m2;
  }
  
  if (state->x1 < state->y1)
    return (state->x1 - state->y1 + m1);
  else
    return (state->x1 - state->y1);
}

static double cmrg_get_double (void *vstate)
{
  return cmrg_get (vstate) / 2147483647.0 ;
}


static voidcmrg_set (void *vstate, unsigned long int s)
{
  cmrg_state_t *state = (cmrg_state_t *) vstate;

  if (s == 0) s = 1;

#define LCG(n) ((69069 * n) & 0xffffffffUL)

  s = LCG (s);
  state->x1 = s % m1;
  s = LCG (s);
  state->x2 = s % m1;
  s = LCG (s);
  state->x3 = s % m1;

  s = LCG (s);
  state->y1 = s % m2;
  s = LCG (s);
  state->y2 = s % m2;
  s = LCG (s);
  state->y3 = s % m2;

  /* "warm it up" */
  cmrg_get (state);
  cmrg_get (state);
  cmrg_get (state);
  cmrg_get (state);
  cmrg_get (state);
  cmrg_get (state);
  cmrg_get (state);
}
