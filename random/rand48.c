
#include <math.h>
#include <stdlib.h>

/*
0xFFFFFFFF // Max
0 // Min
*/

static inline void rand48_advance (void *vstate);
static unsigned long int rand48_get (void *vstate);
static double rand48_get_double (void *vstate);
static void rand48_set (void *state, unsigned long int s);

static const unsigned short int a0 = 0xE66D ;
static const unsigned short int a1 = 0xDEEC ;
static const unsigned short int a2 = 0x0005 ;

static const unsigned short int c0 = 0x000B ;

typedef struct
  {
    unsigned short int x0, x1, x2;
  }
rand48_state_t;

static inline void rand48_advance (void *vstate)
{
  rand48_state_t *state = (rand48_state_t *) vstate;
  const unsigned long int x0 = (unsigned long int) state->x0;
  const unsigned long int x1 = (unsigned long int) state->x1;
  const unsigned long int x2 = (unsigned long int) state->x2;
  unsigned long int a;
  
  a = a0 * x0 + c0;
  state->x0 = (a & 0xFFFF);
  a >>= 16;
  a += a0 * x1 + a1 * x0 ; 
  state->x1 = (a & 0xFFFF);
  a >>= 16;
  a += a0 * x2 + a1 * x1 + a2 * x0;
  state->x2 = (a & 0xFFFF);
}

static unsigned long int rand48_get (void *vstate)
{
  unsigned long int x1, x2;
  rand48_state_t *state = (rand48_state_t *) vstate;
  rand48_advance (state) ;
  x2 = (unsigned long int) state->x2;
  x1 = (unsigned long int) state->x1;
  return (x2 << 16) + x1;
}

static double rand48_get_double (void * vstate)
{
  rand48_state_t *state = (rand48_state_t *) vstate;
  rand48_advance (state);  
  return (ldexp((double) state->x2, -16) + ldexp((double) state->x1, -32) + ldexp((double) state->x0, -48)) ;
}

static void rand48_set (void *vstate, unsigned long int s)
{
  rand48_state_t *state = (rand48_state_t *) vstate;
  if (s == 0)
    {
      state->x0 = 0x330E;
      state->x1 = 0xABCD;
      state->x2 = 0x1234;
    }
  else 
    {
      state->x0 = 0x330E;
      state->x1 = s & 0xFFFF;
      state->x2 = (s >> 16) & 0xFFFF;
    }
}

