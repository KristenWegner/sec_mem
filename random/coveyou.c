
#include <stdlib.h>

#define MM 0xffffffffUL // 2 ^ 32 - 1.

/*
MM - 1 // Max
2 // Min
*/

static inline unsigned long int ran_get (void *vstate);
static double ran_get_double (void *vstate);
static void ran_set (void *state, unsigned long int s);

typedef struct
{
  unsigned long int x;
}
ran_state_t;

static inline unsigned long int ran_get (void *vstate)
{
  ran_state_t *state = (ran_state_t *) vstate;

  state->x = (state->x * (state->x + 1)) & MM;

  return state->x;
}

static double ran_get_double (void *vstate)
{
  ran_state_t *state = (ran_state_t *) vstate;

  return ran_get (state) / 4294967296.0;
}

static void ran_set (void *vstate, unsigned long int s)
{
  ran_state_t *state = (ran_state_t *) vstate;

  unsigned long int diff = ((s % 4UL) - 2UL) % MM;

  if (diff)
    state->x = (s - diff) & MM;
  else
    state->x = s & MM;

  return;
}
