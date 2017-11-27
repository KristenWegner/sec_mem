#include<stdio.h>
#include<stdlib.h>

#define W 32
#define R 1391
#define P 15
#define MASKU (0xffffffffU>>(W-P))
#define MASKL (~MASKU)

/* To obtain the WELL44497b, uncomment the following line */
/* #define TEMPERING                                      */
#define TEMPERB 0x93dd1400U
#define TEMPERC 0xfa118000U

#define M1 23
#define M2 481
#define M3 229

#define MAT0POS(t,v) (v^(v>>t))
#define MAT0NEG(t,v) (v^(v<<(-(t))))
#define MAT1(v) v
#define MAT2(a,v) ((v & 1U)?((v>>1)^a):(v>>1))
#define MAT3POS(t,v) (v>>t)
#define MAT3NEG(t,v) (v<<(-(t)))
#define MAT4POS(t,b,v) (v ^ ((v>>  t ) & b))
#define MAT4NEG(t,b,v) (v ^ ((v<<(-(t))) & b))
#define MAT5(r,a,ds,dt,v) ((v & dt)?((((v<<r)^(v>>(W-r)))&ds)^a):(((v<<r)^(v>>(W-r)))&ds))
#define MAT7(v) 0

#define V0            STATE[state_i]
#define VM1Over       STATE[state_i+M1-R]
#define VM1           STATE[state_i+M1]
#define VM2Over       STATE[state_i+M2-R]
#define VM2           STATE[state_i+M2]
#define VM3Over       STATE[state_i+M3-R]
#define VM3           STATE[state_i+M3]
#define Vrm1          STATE[state_i-1]
#define Vrm1Under     STATE[state_i+R-1]
#define Vrm2          STATE[state_i-2]
#define Vrm2Under     STATE[state_i+R-2]

#define newV0         STATE[state_i-1]
#define newV0Under    STATE[state_i-1+R]
#define newV1         STATE[state_i]
#define newVRm1       STATE[state_i-2]
#define newVRm1Under  STATE[state_i-2+R]
#define FACT 2.32830643653869628906e-10

static unsigned int STATE[R];
static unsigned int y, z0,z1,z2;
static unsigned int state_i=0;

void InitWELLRNG44497av2(unsigned int *init ){
   int j;
   state_i=0;
   
   for(j=0;j<R;j++)
      STATE[j]=init[j];
}

double WELLRNG44497av2(void){
  
  if(state_i==0){
    z0 = (Vrm1Under & MASKL) | (Vrm2Under & MASKU);
    z1 = MAT0NEG(-24,V0) ^ MAT0POS(30,VM1);
    z2 = MAT0NEG(-10,VM2) ^ MAT3NEG(-26,VM3);
    newV1  = z1 ^ z2;
    newV0Under = MAT1(z0) ^ MAT0POS(20,z1) ^ MAT5(9,0xb729fcecU,0xfbffffffU,0x00020000U,z2) ^ MAT1(newV1);
  } else if(state_i==1){
    z0 = (Vrm1 & MASKL) | (Vrm2Under & MASKU);
    z1 = MAT0NEG(-24,V0) ^ MAT0POS(30,VM1);
    z2 = MAT0NEG(-10,VM2) ^ MAT3NEG(-26,VM3);
    newV1 = z1 ^ z2;
    newV0 =  MAT1(z0) ^ MAT0POS(20,z1) ^ MAT5(9,0xb729fcecU,0xfbffffffU,0x00020000U,z2) ^ MAT1(newV1);
  } else if(state_i+M1>=R){
    z0 = (Vrm1 & MASKL) | (Vrm2 & MASKU);
    z1 = MAT0NEG(-24,V0) ^ MAT0POS(30,VM1Over);
    z2 = MAT0NEG(-10,VM2Over) ^ MAT3NEG(-26,VM3Over);
    newV1 = z1 ^ z2;
    newV0 = MAT1(z0) ^ MAT0POS(20,z1) ^ MAT5(9,0xb729fcecU,0xfbffffffU,0x00020000U,z2) ^ MAT1(newV1);
  } else if (state_i+M3>=R){
    z0 = (Vrm1 & MASKL) | (Vrm2 & MASKU);
    z1 = MAT0NEG(-24,V0) ^ MAT0POS(30,VM1);
    z2 = MAT0NEG(-10,VM2Over) ^ MAT3NEG(-26,VM3Over);
    newV1 = z1 ^ z2;
    newV0 = MAT1(z0) ^ MAT0POS(20,z1) ^ MAT5(9,0xb729fcecU,0xfbffffffU,0x00020000U,z2) ^ MAT1(newV1);
  } else if (state_i+M2>=R){
    z0 = (Vrm1 & MASKL) | (Vrm2 & MASKU);
    z1 = MAT0NEG(-24,V0) ^ MAT0POS(30,VM1);
    z2 = MAT0NEG(-10,VM2Over) ^ MAT3NEG(-26,VM3);
    newV1 = z1 ^ z2;
    newV0 = MAT1(z0) ^ MAT0POS(20,z1) ^ MAT5(9,0xb729fcecU,0xfbffffffU,0x00020000U,z2) ^ MAT1(newV1);
  } else {
    z0 = (Vrm1 & MASKL) | (Vrm2 & MASKU);
    z1 = MAT0NEG(-24,V0) ^ MAT0POS(30,VM1);
    z2 = MAT0NEG(-10,VM2) ^ MAT3NEG(-26,VM3);
    newV1 = z1 ^ z2;
    newV0 = MAT1(z0) ^ MAT0POS(20,z1) ^ MAT5(9,0xb729fcecU,0xfbffffffU,0x00020000U,z2) ^ MAT1(newV1);
  }
  state_i--;
  if(state_i==-1)
    state_i=R-1;
#ifdef TEMPERING
   y = STATE[state_i] ^ ((STATE[state_i] << 7) & TEMPERB);
   y =              y ^ ((             y << 15) & TEMPERC);
   return ((double) y * FACT);
#else
   return ((double) STATE[state_i] * FACT);
#endif
}



