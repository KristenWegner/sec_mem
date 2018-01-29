/* 
   This is a maximally equidistributed Mersenne Twister MEMT19937-II by Shin Harase (2014/11/10).


   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include <stdio.h>

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfU  /* constant vector a */
#define UPPER_MASK 0x80000000U /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffU /* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UPPER_MASK) | ((v) & LOWER_MASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1U ? MATRIX_A : 0U))

/* tempering parameters */
#define LAG1 151
#define LAG2 36
#define LAG1over 473
#define LAG2over 588
#define TEMPERING_SHIFT_1(z)  (z << 8)
#define TEMPERING_SHIFT_2(z)  (z << 14)
#define MASK1 0xb219beabU
#define MASK2 0x56bde52aU

static unsigned int mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/*plot type*/
static unsigned int case_1(void);
static unsigned int case_2(void);
static unsigned int case_3(void);
static unsigned int case_4(void);
static unsigned int case_5(void);
/* generates a random number on [0,0xffffffff]-interval */
       unsigned int (*genrand_int32)(void);

static unsigned int z;

/* do not use this function directly */
/* initializes mt[N] with a seed */
void init_genrand(unsigned int s)
{
    mt[0]= s & 0xffffffffU;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253U * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffU;
        /* for >32 bit machines */
    }
	mti = 0;
	genrand_int32 = case_1;
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned int init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218U);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffU; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941U))
          - i; /* non linear */
        mt[i] &= 0xffffffffU; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000U; /* MSB is 1; assuring non-zero initial array */
	mti = 0;
	genrand_int32 = case_1;
}

unsigned int case_1(void){
			mt[mti] = mt[mti+M] ^ TWIST(mt[mti],mt[mti+1]);
			z  = mt[mti] ^ (mt[mti + LAG1] & MASK1);
			z ^= TEMPERING_SHIFT_1(z);
			z ^= TEMPERING_SHIFT_2(z);
			z ^= (mt[mti + LAG2] & MASK2);
			mti++;
			if(mti == N-M) genrand_int32 = case_2;
			return z;
			}

static unsigned int case_2(void){
			mt[mti] = mt[mti+(M-N)] ^ TWIST(mt[mti],mt[mti+1]);
			z  = mt[mti] ^ (mt[mti + LAG1] & MASK1);
			z ^= TEMPERING_SHIFT_1(z);
			z ^= TEMPERING_SHIFT_2(z);
			z ^= (mt[mti + LAG2] & MASK2);
			mti++;
			if(mti == LAG1over) genrand_int32 = case_3;
			return z;
		}

static unsigned int case_3(void){
			mt[mti] = mt[mti+(M-N)] ^ TWIST(mt[mti],mt[mti+1]);
			z  = mt[mti] ^ (mt[mti - LAG1over] & MASK1);
			z ^= TEMPERING_SHIFT_1(z);
			z ^= TEMPERING_SHIFT_2(z);
			z ^= (mt[mti + LAG2] & MASK2);
			mti++;
			if(mti == LAG2over) genrand_int32 = case_4;
			return z;
		}
		
static unsigned int case_4(void){
			mt[mti] = mt[mti+(M-N)] ^ TWIST(mt[mti],mt[mti+1]);
			z  = mt[mti] ^ (mt[mti - LAG1over] & MASK1);
			z ^= TEMPERING_SHIFT_1(z);
			z ^= TEMPERING_SHIFT_2(z);
			z ^= (mt[mti - LAG2over] & MASK2);
			mti++;
			if(mti==N-1)genrand_int32 = case_5;
			return z;
		}

static unsigned int case_5(void){
			mt[N-1] = mt[M-1] ^ TWIST(mt[N-1],mt[0]);
			z  = mt[mti] ^ (mt[mti - LAG1over] & MASK1);
			z ^= TEMPERING_SHIFT_1(z);
			z ^= TEMPERING_SHIFT_2(z);
			z ^= (mt[mti - LAG2over] & MASK2);
			mti=0;
			genrand_int32 = case_1;
			return z;
			}

/* generates a random number on [0,0x7fffffff]-interval */
unsigned int genrand_int31(void)
{
    return (unsigned int)(genrand_int32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double genrand_real1(void)
{
    return genrand_int32()*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double genrand_real2(void)
{
    return genrand_int32()*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double genrand_real3(void)
{
    return (((double)genrand_int32()) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(void) 
{ 
    unsigned int a=genrand_int32()>>5, b=genrand_int32()>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */

int main(void)
{
    int i;
    unsigned int init[4]={0x123U, 0x234U, 0x345U, 0x456U}, length=4;
    init_by_array(init, length);
    printf("1000 outputs of genrand_int32()\n");
    for (i=0; i<1000; i++) {
      printf("%10u ", genrand_int32());
      if (i%5==4) printf("\n");
    }
    printf("\n1000 outputs of genrand_real2()\n");
    for (i=0; i<1000; i++) {
      printf("%10.8f ", genrand_real2());
      if (i%5==4) printf("\n");
    }
    return 0;
}