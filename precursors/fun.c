// fun.c - Generates random functions.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <float.h>
#include <fenv.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/timeb.h>
#include <sys/stat.h>


#include "../config.h"
#include "../compatibility/gettimeofday.h"
#include "../bits.h"
#include "../sm.h"


#if defined(SM_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#include <time.h>
#include <process.h>
#include <windows.h>
#define getpid _getpid
#define time _time64
#elif defined(SM_OS_LINUX)
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif


extern uint64_t next_rand();
extern FILE* open_file(const char* path);
extern char* trim(char* s);
extern int replace(char* s, const char* m, const char* r);
extern int evaluate_size(char* entity_size, uint64_t* result);


static const char* alphas = "abcdefghijklmnopqrstuvwxyz";


static const uint32_t powers[] = { 2, 4, 8, 16, 32, 64, 128, 256, 512 };


#define T_UINT08	0
#define T_UINT16	1
#define T_UINT32	2
#define T_UINT64	3
#define T_SINT08	4
#define T_SINT16	5
#define T_SINT32	6
#define T_SINT64	7
#define T_SINGLE	8
#define T_UINT		9
#define T_SINT		10
#define T_ANY		11


#define V_SCALAR 0
#define V_VECTOR 1


#define N "\n"
#define T "\t"


static uint64_t next_rand_in(uint64_t a, uint64_t b)
{
	if (a == b) return a;

	uint64_t r = (b - a);
	uint64_t v = next_rand();

	if (v == 0) return a;

	v %= r;
	v += a;

	return v;
}


typedef struct
{
	char name[32]; // Variable name.
	uint8_t type; // Type code.
	uint8_t arity; // Scalar or vector.
	uint16_t count; // Count of elements, if vector.
	uint8_t is_used; // Has been used.
	uint8_t modified; // Was modified.
	uint8_t assigned_to; // Last assigned to index.
}
var_t;


const char* get_type(uint8_t t)
{
	switch (t)
	{
	case T_UINT08: return "uint8_t";
	case T_UINT16: return "uint16_t";
	case T_UINT32: return "uint32_t";
	case T_UINT64: return "uint64_t";
	case T_SINT08: return "int8_t";
	case T_SINT16: return "int16_t";
	case T_SINT32: return "int32_t";
	case T_SINT64: return "int64_t";
	case T_SINGLE: return "float";
	default:
		return "uint64_t";
	}
}

const char* get_const(uint8_t t)
{
	switch (t)
	{
	case T_UINT08: return "UINT8_C";
	case T_UINT16: return "UINT16_C";
	case T_UINT32: return "UINT32_C";
	case T_UINT64: return "UINT64_C";
	case T_SINT08: return "INT8_C";
	case T_SINT16: return "INT16_C";
	case T_SINT32: return "INT32_C";
	case T_SINT64: return "INT64_C";
	case T_SINGLE: return "(float)";
	default:
		return "UINT64_C";
	}
}

uint8_t get_bits(uint8_t t)
{
	switch (t)
	{
	case T_UINT08: return 8;
	case T_UINT16: return 16;
	case T_UINT32: return 32;
	case T_UINT64: return 64;
	case T_SINT08: return 8;
	case T_SINT16: return 16;
	case T_SINT32: return 32;
	case T_SINT64: return 64;
	case T_SINGLE: return 32;
	default:
		return 64;
	}
}


typedef struct
{
	uint8_t operands; // Count of operands needed.
	uint8_t operand_types[16]; // Types of operands needed.
	uint8_t result; // Result variable index.
	char code[4096]; // The code expression.
}
stmt_t;


// Macros

#define M_VARNM "var" // Argument macro prefix.
#define M_TEMPN "tmp" // A temporary variable name.
#define M_TYPEV "typ" // The type of the same-index variable.
#define M_CONST "cst" // Constant macro for type of the same-index variable.

// Constants

#define M_RAH08 "rh08" // Random unsigned hex integers.
#define M_RAH16 "rh16"
#define M_RAH32 "rh32"
#define M_RAH64 "rh64" // [0..UINT64_MAX]

#define M_RAP08 "rp08" // Random hex bit patterns.
#define M_RAP16 "rp16"
#define M_RAP32 "rp32"
#define M_RAP64 "rp64"
char* bit_patterns[] =
{
	"1111111111111111",
	"0101010101010101",
	"1010101010101010",
	"2222222222222222",
	"0202020202020202",
	"2020202020202020",
	"3333333333333333",
	"0303030303030303",
	"3030303030303030",
	"4444444444444444",
	"0404040404040404",
	"4040404040404040",
	"5555555555555555",
	"0505050505050505",
	"5050505050505050",
	"6666666666666666",
	"0606060606060606",
	"6060606060606060",
	"7777777777777777",
	"0707070707070707",
	"7070707070707070",
	"8888888888888888",
	"0808080808080808",
	"8080808080808080",
	"9999999999999999",
	"0909090909090909",
	"9090909090909090",
	"AAAAAAAAAAAAAAAA",
	"0A0A0A0A0A0A0A0A",
	"A0A0A0A0A0A0A0A0",
	"BBBBBBBBBBBBBBBB",
	"0B0B0B0B0B0B0B0B",
	"B0B0B0B0B0B0B0B0",
	"CCCCCCCCCCCCCCCC",
	"0C0C0C0C0C0C0C0C",
	"C0C0C0C0C0C0C0C0",
	"DDDDDDDDDDDDDDDD",
	"0D0D0D0D0D0D0D0D",
	"D0D0D0D0D0D0D0D0",
	"EEEEEEEEEEEEEEEE",
	"0E0E0E0E0E0E0E0E",
	"E0E0E0E0E0E0E0E0",
	"FFFFFFFFFFFFFFFF"
	"0F0F0F0F0F0F0F0F",
	"F0F0F0F0F0F0F0F0",
	"7F7F7F7F7F7F7F7F",
	"F7F7F7F7F7F7F7F7"
};


#define M_RAS08 "rs08" // Random signed decimal integers.
#define M_RAS16 "rs16"
#define M_RAS32 "rs32"
#define M_RAS64 "rs64" // [INT64_MIN..INT64_MAX]

#define M_RAD08 "rd08" // Random unsigned decimal integers.
#define M_RAD16 "rd16"
#define M_RAD32 "rd32"
#define M_RAD64 "rd64" // [0..INT64_MAX]

#define M_RAB08 "bc08" // Random bit counts.
#define M_RAB16 "bc16"
#define M_RAB32 "bc32"
#define M_RAB64 "bc64" // [0..64]

#define M_RSH08 "bs08" // Random bit shifts.
#define M_RSH16 "bs16"
#define M_RSH32 "bs32"
#define M_RSH64 "bs64" // [1..63]

#define M_RFL01 "fl01" // Random float in [FLT_EPSILON..1.0)
#define M_RFLTA "flts" // Random signed float in (FLT_MIN..FLT_MAX)
#define M_RFLTP "fltp" // Random positive float in (0..FLT_MAX).
#define M_RFLTC "fltc" // Standard floats, constants and common values.
float rfltcs[] =
{
	0.1F,
	0.2F,
	0.3F,
	0.4F,
	0.6F,
	0.7F,
	0.8F,
	0.9F,
	1.0F,
	2.0F,
	3.0F,
	4.0F,
	5.0F,
	6.0F,
	7.0F,
	8.0F,
	9.0F,
	0.5F,
	0.25F,
	0.1666666667F,
	0.1428571428F,
	0.125F,
	0.1F,
	0.0625F,
	0.03125F,
	0.015625F,
	0.0078125F,
	0.00390625F,
	0.001953125F,
	0.0009765625F,
	0.1111111111F,
	0.2222222222F,
	0.3333333333F,
	0.4444444444F,
	0.5555555555F,
	0.6666666666F,
	0.7777777777F,
	0.8888888888F,
	0.9999999999F,
	3.1415926536F,
	2.7182818285F,
	1.4426950408F,
	0.4342944819F,
	0.6931471806F,
	2.3025850929F,
	1.5707963268F,
	0.7853981634F,
	0.3183098862F,
	0.6366197724F,
	1.1283791671F,
	1.4142135624F,
	0.7071067812F,
	1.6180339887F,
	0.5772156649F,
	2.6854520010F,
	1.2824271291F,
	0.1234567891F,
	1.7320508076F,
	2.2360679775F,
	0.2614972128F,
	0.2801694990F,
	0.3036630029F,
	0.3532363718F,
	0.5671432904F,
	0.6243299885F,
	0.6434105462F,
	0.6601618158F,
	0.6627434194F,
	0.9159655942F,
	1.0986858055F,
	1.2020569032F,
	1.3035772690F,
	1.9021605831F,
	2.5029078751F,
	2.5849817596F,
	3.3598856662F,
	4.6692016091F,
	1.13198824F,
	0.70258F
};


// Ops.

#define M_BOPAR "aop" // Binary arithmetic add, sub or mul ops.
const char* bopars[] = { "+", "-", "*" };

#define M_BOPBT "bop" // Binary bit ops.
const char* bopbts[] = { "&", "|", "^", "<<", ">>" };

#define M_BOPDV "dop" // Binary arithmetic div or mod ops.
const char* bopdvs[] = { "/", "%" };

#define M_UONOT "not" // Unary not.
const char* uonots[] = { "~", "" };

#define M_UOSGN "sgn" // Unary sign +/-.
const char* uosgns[] = { "+", "-", "" };

#define M_AOPAR "aao" // Assignment arithmetic assign, add, sub or mul ops.
const char* aopars[] = { "+=", "-=", "*=", "=" };

#define M_AOPBT "abo" // Assignment bit ops.
const char* aopbts[] = { "&=", "|=", "^=", "<<=", ">>=" };

#define M_AOPDV "adv" // Assignment arithmetic div or mod ops.
const char* aopdvs[] = { "/=", "%=" };

#define M_COMPR "cmp" // Comparison operators.
const char* comprs[] = { ">", "<", "<=", ">=", "==", "!=" };


stmt_t statements[] =
{
	{ 1, { T_UINT64 }, 0, "$var0 $aao0 ($var0 >> 1) & $cst0($rp640)" },
	{ 1, { T_UINT32 }, 0, "$var0 $aao0 ($var0 >> 1) & $cst0($rp320)" },
	{ 1, { T_UINT16 }, 0, "$var0 $aao0 ($var0 >> 1) & $cst0($rp160)" },
	{ 1, { T_UINT08 }, 0, "$var0 $aao0 ($var0 >> 1) & $cst0($rp080)" },

	{ 1, { T_UINT64 }, 0, "$var0 $aao0 (($var0 >> $bs640) $bop1 $cst0($rp640)) $aop0 ($var0 $bop0 $cst0($rp640))" },
	{ 1, { T_UINT32 }, 0, "$var0 $aao0 (($var0 >> $bs320) $bop1 $cst0($rp320)) $aop0 ($var0 $bop0 $cst0($rp320))" },
	{ 1, { T_UINT16 }, 0, "$var0 $aao0 (($var0 >> $bs160) $bop1 $cst0($rp160)) $aop0 ($var0 $bop0 $cst0($rp160))" },
	{ 1, { T_UINT08 }, 0, "$var0 $aao0 (($var0 >> $bs080) $bop1 $cst0($rp080)) $aop0 ($var0 $bop0 $cst0($rp080))" },

	{ 1, { T_UINT64 }, 0, "$var0 $aao0 (($var0 >> $bs640) $bop1 $cst0($rp640)) $aop0 ($var0 $bop0 $cst0($rh640))" },
	{ 1, { T_UINT32 }, 0, "$var0 $aao0 (($var0 >> $bs320) $bop1 $cst0($rp320)) $aop0 ($var0 $bop0 $cst0($rh320))" },
	{ 1, { T_UINT16 }, 0, "$var0 $aao0 (($var0 >> $bs160) $bop1 $cst0($rp160)) $aop0 ($var0 $bop0 $cst0($rh160))" },
	{ 1, { T_UINT08 }, 0, "$var0 $aao0 (($var0 >> $bs080) $bop1 $cst0($rp080)) $aop0 ($var0 $bop0 $cst0($rh080))" },

	{ 1, { T_UINT64 }, 0, "$var0 $abo0 (($var0 >> $bs640) $aop0 $var0) $bop0 $cst0($rp640)" },
	{ 1, { T_UINT32 }, 0, "$var0 $abo0 (($var0 >> $bs320) $aop0 $var0) $bop0 $cst0($rp320)" },
	{ 1, { T_UINT16 }, 0, "$var0 $abo0 (($var0 >> $bs160) $aop0 $var0) $bop0 $cst0($rp160)" },
	{ 1, { T_UINT08 }, 0, "$var0 $abo0 (($var0 >> $bs080) $aop0 $var0) $bop0 $cst0($rp080)" },

	{ 1, { T_UINT64 }, 0, "$var0 $abo0 $cst0($rp640)" },
	{ 1, { T_UINT32 }, 0, "$var0 $abo0 $cst0($rp320)" },
	{ 1, { T_UINT16 }, 0, "$var0 $abo0 $cst0($rp160)" },
	{ 1, { T_UINT08 }, 0, "$var0 $abo0 $cst0($rp080)" },

	{ 1, { T_UINT64 }, 0, "$var0 $aao0 $cst0($rh640)" },
	{ 1, { T_UINT32 }, 0, "$var0 $aao0 $cst0($rh320)" },
	{ 1, { T_UINT16 }, 0, "$var0 $aao0 $cst0($rh160)" },
	{ 1, { T_UINT08 }, 0, "$var0 $aao0 $cst0($rh080)" },

	{ 3, { T_UINT64, T_UINT64, T_UINT64 }, 2, "$var0 = $cst0(64) >> 1; $var1 = ~$cst0(0) << $var0; do { $var2 ^= (($var2 & $var1) >> $var0); $var0 >>= 1; $var1 ^= ($var1 >> $var0); } while ($var0)" },
	{ 3, { T_UINT32, T_UINT32, T_UINT32 }, 2, "$var0 = $cst0(32) >> 1; $var1 = ~$cst0(0) << $var0; do { $var2 ^= (($var2 & $var1) >> $var0); $var0 >>= 1; $var1 ^= ($var1 >> $var0); } while ($var0)" },
	{ 3, { T_UINT16, T_UINT16, T_UINT16 }, 2, "$var0 = $cst0(16) >> 1; $var1 = ~$cst0(0) << $var0; do { $var2 ^= (($var2 & $var1) >> $var0); $var0 >>= 1; $var1 ^= ($var1 >> $var0); } while ($var0)" },
	{ 3, { T_UINT08, T_UINT08, T_UINT08 }, 2, "$var0 = $cst0(8) >> 1; $var1 = ~$cst0(0) << $var0; do { $var2 ^= (($var2 & $var1) >> $var0); $var0 >>= 1; $var1 ^= ($var1 >> $var0); } while ($var0)" },

	{ 2, { T_UINT64, T_UINT64 }, 0, "$var0 $abo0 ((~$var1) << $bs080)" },
	{ 2, { T_UINT64, T_UINT32 }, 0, "$var0 $abo0 ((~($typ0)$var1) << $bs160)" },
	{ 2, { T_UINT64, T_UINT16 }, 0, "$var0 $abo0 ((~($typ0)$var1) << $bs320)" },
	{ 2, { T_UINT64, T_UINT08 }, 0, "$var0 $abo0 ((~($typ0)$var1) << $bs640)" },

	{ 2, { T_UINT64, T_UINT64 }, 0, "$var0 $aao0 ((~$var1) << $bs080)" },
	{ 2, { T_UINT64, T_UINT32 }, 0, "$var0 $aao0 ((~($typ0)$var1) << $bs160)" },
	{ 2, { T_UINT64, T_UINT16 }, 0, "$var0 $aao0 ((~($typ0)$var1) << $bs320)" },
	{ 2, { T_UINT64, T_UINT08 }, 0, "$var0 $aao0 ((~($typ0)$var1) << $bs640)" },

	{ 3, { T_UINT64, T_UINT64, T_UINT64 }, 0, "$var1 = (CHAR_BIT * sizeof($var0) - 1); $var2 &= $var1; $var0 = ($var0 >> $var2) | ($var0 << ((-$var2) & $var1))" },
	{ 3, { T_UINT32, T_UINT32, T_UINT32 }, 0, "$var1 = (CHAR_BIT * sizeof($var0) - 1); $var2 &= $var1; $var0 = ($var0 >> $var2) | ($var0 << ((-$var2) & $var1))" },
	{ 3, { T_UINT16, T_UINT16, T_UINT16 }, 0, "$var1 = (CHAR_BIT * sizeof($var0) - 1); $var2 &= $var1; $var0 = ($var0 >> $var2) | ($var0 << ((-$var2) & $var1))" },
	{ 3, { T_UINT08, T_UINT08, T_UINT08 }, 0, "$var1 = (CHAR_BIT * sizeof($var0) - 1); $var2 &= $var1; $var0 = ($var0 >> $var2) | ($var0 << ((-$var2) & $var1))" },

	{ 2, { T_UINT32, T_UINT32 }, 0, "$var0 $aao0 ((((uint64_t)$var0) * ((uint64_t)$var1)) >> 32)" },
	{ 2, { T_UINT16, T_UINT16 }, 0, "$var0 $aao0 ((((uint32_t)$var0) * ((uint32_t)$var1)) >> 16)" },
	{ 2, { T_UINT08, T_UINT08 }, 0, "$var0 $aao0 ((((uint16_t)$var0) * ((uint16_t)$var1)) >> 8)" },

	{ 1, { T_UINT64 }, 0xFF, "result $aao0 $var0" },
	{ 1, { T_UINT32 }, 0xFF, "result $aao0 (uint64_t)$var0" },
	{ 1, { T_UINT16 }, 0xFF, "result $aao0 (uint64_t)$var0" },
	{ 1, { T_UINT08 }, 0xFF, "result $aao0 (uint64_t)$var0" },

	{ 1, { T_SINT64 }, 0xFF, "result $aao0 $var0" },
	{ 1, { T_SINT32 }, 0xFF, "result $aao0 $var0" },
	{ 1, { T_SINT16 }, 0xFF, "result $aao0 $var0" },
	{ 1, { T_SINT08 }, 0xFF, "result $aao0 $var0" },
	
	{ 1, { T_SINT64 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_SINT32 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_SINT16 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_SINT08 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_UINT64 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_UINT32 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_UINT16 }, 0xFF, "result $aao0 $sgn0$var0" },
	{ 1, { T_UINT08 }, 0xFF, "result $aao0 $sgn0$var0" },

	{ 1, { T_UINT64 }, 0xFF, "result $abo0 $not0$var0" },
	{ 1, { T_UINT32 }, 0xFF, "result $abo0 $not0((uint64_t)$var0)" },
	{ 1, { T_UINT16 }, 0xFF, "result $abo0 $not0((uint64_t)$var0)" },
	{ 1, { T_UINT08 }, 0xFF, "result $abo0 $not0((uint64_t)$var0)" },

	{ 2, { T_UINT64, T_UINT08 }, 1, "register union { $typ0 v; $typ1 b[sizeof($typ0)]; } $tmp0 = { .v = $var0 }; $var1 $abo0 ($typ1)($tmp0.b[7] ^ $tmp0.b[0] ^ $tmp0.b[6] ^ $tmp0.b[1] ^ $tmp0.b[5] ^ $tmp0.b[2] ^ $tmp0.b[4] ^ $tmp0.b[3])" },
	{ 2, { T_UINT32, T_UINT08 }, 1, "register union { $typ0 v; $typ1 b[sizeof($typ0)]; } $tmp0 = { .v = $var0 }; $var1 $abo0 ($typ1)($tmp0.b[3] ^ $tmp0.b[0] ^ $tmp0.b[2] ^ $tmp0.b[1])" },
	{ 2, { T_UINT16, T_UINT08 }, 1, "register union { $typ0 v; $typ1 b[sizeof($typ0)]; } $tmp0 = { .v = $var0 }; $var1 $abo0 ($typ1)($tmp0.b[1] ^ $tmp0.b[0])" },
	{ 2, { T_SINT64, T_UINT08 }, 1, "register union { $typ0 v; $typ1 b[sizeof($typ0)]; } $tmp0 = { .v = $var0 }; $var1 $abo0 ($typ1)($tmp0.b[7] ^ $tmp0.b[0] ^ $tmp0.b[6] ^ $tmp0.b[1] ^ $tmp0.b[5] ^ $tmp0.b[2] ^ $tmp0.b[4] ^ $tmp0.b[3])" },
	{ 2, { T_SINT32, T_UINT08 }, 1, "register union { $typ0 v; $typ1 b[sizeof($typ0)]; } $tmp0 = { .v = $var0 }; $var1 $abo0 ($typ1)($tmp0.b[3] ^ $tmp0.b[0] ^ $tmp0.b[2] ^ $tmp0.b[1])" },
	{ 2, { T_SINT16, T_UINT08 }, 1, "register union { $typ0 v; $typ1 b[sizeof($typ0)]; } $tmp0 = { .v = $var0 }; $var1 $abo0 ($typ1)($tmp0.b[1] ^ $tmp0.b[0])" },

	{ 2, { T_UINT08, T_UINT64 }, 1, "register union { $typ1 v; $typ0 b[sizeof($typ1)]; } $tmp0 = { .b = { $var0, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080 } }; $var1 $abo0 $tmp0.v" },
	{ 2, { T_UINT08, T_UINT32 }, 1, "register union { $typ1 v; $typ0 b[sizeof($typ1)]; } $tmp0 = { .b = { $var0, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080 } }; $var1 $abo0 $tmp0.v" },
	{ 2, { T_UINT08, T_UINT16 }, 1, "register union { $typ1 v; $typ0 b[sizeof($typ1)]; } $tmp0 = { .b = { $var0, $var0 << $bs080 } }; $var1 $abo0 $tmp0.v" },
	{ 2, { T_UINT08, T_SINT64 }, 1, "register union { $typ1 v; $typ0 b[sizeof($typ1)]; } $tmp0 = { .b = { $var0, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080 } }; $var1 $abo0 $tmp0.v" },
	{ 2, { T_UINT08, T_SINT32 }, 1, "register union { $typ1 v; $typ0 b[sizeof($typ1)]; } $tmp0 = { .b = { $var0, $var0 << $bs080, $var0 >> $bs080, $var0 << $bs080 } }; $var1 $abo0 $tmp0.v" },
	{ 2, { T_UINT08, T_SINT16 }, 1, "register union { $typ1 v; $typ0 b[sizeof($typ1)]; } $tmp0 = { .b = { $var0, $var0 << $bs080 } }; $var1 $abo0 $tmp0.v" },

	{ 1, { T_UINT64 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs640); $tmp0 -= ($tmp0 >> $bs640) & $cst0($rp640); $tmp0 = (($tmp0 >> $bs641) & $cst0($rp641)) + ($tmp0 & $cst0($rp641)); $tmp0 = (($tmp0 >> $bs642) + $tmp0) & $cst0($rp642); $tmp0 *= $cst0($rp643); $var0 = ($var0 & $cst0($bs640)) + ($tmp0 >> $bs643) / $cst0(2)" },
	{ 1, { T_UINT32 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs320); $tmp0 -= ($tmp0 >> $bs320) & $cst0($rp320); $tmp0 = (($tmp0 >> $bs321) & $cst0($rp321)) + ($tmp0 & $cst0($rp321)); $tmp0 = (($tmp0 >> $bs322) + $tmp0) & $cst0($rp322); $tmp0 *= $cst0($rp323); $var0 = ($var0 & $cst0($bs320)) + ($tmp0 >> $bs323) / $cst0(2)" },
	{ 1, { T_UINT16 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs160); $tmp0 -= ($tmp0 >> $bs160) & $cst0($rp160); $tmp0 = (($tmp0 >> $bs161) & $cst0($rp161)) + ($tmp0 & $cst0($rp161)); $tmp0 = (($tmp0 >> $bs162) + $tmp0) & $cst0($rp162); $tmp0 *= $cst0($rp163); $var0 = ($var0 & $cst0($bs160)) + ($tmp0 >> $bs163) / $cst0(2)" },
	{ 1, { T_UINT08 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs080); $tmp0 -= ($tmp0 >> $bs080) & $cst0($rp080); $tmp0 = (($tmp0 >> $bs081) & $cst0($rp081)) + ($tmp0 & $cst0($rp081)); $tmp0 = (($tmp0 >> $bs082) + $tmp0) & $cst0($rp082); $tmp0 *= $cst0($rp083); $var0 = ($var0 & $cst0($bs080)) + ($tmp0 >> $bs083) / $cst0(2)" },
	{ 1, { T_SINT64 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs640); $tmp0 -= ($tmp0 >> $bs640) & $cst0($rp640); $tmp0 = (($tmp0 >> $bs641) & $cst0($rp641)) + ($tmp0 & $cst0($rp641)); $tmp0 = (($tmp0 >> $bs642) + $tmp0) & $cst0($rp642); $tmp0 *= $cst0($rp643); $var0 = ($var0 & $cst0($bs640)) + ($tmp0 >> $bs643) / $cst0(2)" },
	{ 1, { T_SINT32 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs320); $tmp0 -= ($tmp0 >> $bs320) & $cst0($rp320); $tmp0 = (($tmp0 >> $bs321) & $cst0($rp321)) + ($tmp0 & $cst0($rp321)); $tmp0 = (($tmp0 >> $bs322) + $tmp0) & $cst0($rp322); $tmp0 *= $cst0($rp323); $var0 = ($var0 & $cst0($bs320)) + ($tmp0 >> $bs323) / $cst0(2)" },
	{ 1, { T_SINT16 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs160); $tmp0 -= ($tmp0 >> $bs160) & $cst0($rp160); $tmp0 = (($tmp0 >> $bs161) & $cst0($rp161)) + ($tmp0 & $cst0($rp161)); $tmp0 = (($tmp0 >> $bs162) + $tmp0) & $cst0($rp162); $tmp0 *= $cst0($rp163); $var0 = ($var0 & $cst0($bs160)) + ($tmp0 >> $bs163) / $cst0(2)" },
	{ 1, { T_SINT08 }, 0, "register $typ0 $tmp0 = $var0 ^ ($var0 >> $bs080); $tmp0 -= ($tmp0 >> $bs080) & $cst0($rp080); $tmp0 = (($tmp0 >> $bs081) & $cst0($rp081)) + ($tmp0 & $cst0($rp081)); $tmp0 = (($tmp0 >> $bs082) + $tmp0) & $cst0($rp082); $tmp0 *= $cst0($rp083); $var0 = ($var0 & $cst0($bs080)) + ($tmp0 >> $bs083) / $cst0(2)" },

	{ 2, { T_UINT08, T_UINT08 }, 1, "$var0 ^= $var1, $var1 ^= $var0, $var0 ^= $var1" },
	{ 2, { T_UINT16, T_UINT16 }, 1, "$var0 ^= $var1, $var1 ^= $var0, $var0 ^= $var1" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "$var0 ^= $var1, $var1 ^= $var0, $var0 ^= $var1" },
	{ 2, { T_UINT64, T_UINT64 }, 1, "$var0 ^= $var1, $var1 ^= $var0, $var0 ^= $var1" },

	{ 1, { T_UINT08 }, 0, "$var0 $abo0 (($var0 * UINT64_C(0x0202020202) & UINT64_C(0x010884422010)) % 1023U)" },
	{ 2, { T_UINT08, T_UINT08 }, 1, "$var1 $abo0 (($var0 * UINT64_C(0x0202020202) & UINT64_C(0x010884422010)) % 1023U)" },

	{ 1, { T_UINT08 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0++" },
	{ 1, { T_UINT16 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0++" },
	{ 1, { T_UINT32 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16, $var0++" },
	{ 1, { T_UINT64 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16, $var0 |= $var0 >> 32, $var0++" },
	{ 1, { T_SINT08 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0++" },
	{ 1, { T_SINT16 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0++" },
	{ 1, { T_SINT32 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16, $var0++" },
	{ 1, { T_SINT64 }, 0, "$var0--, $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16, $var0 |= $var0 >> 32, $var0++" },

	{ 3, { T_UINT08, T_UINT08, T_UINT16 }, 2, "$var2 $abo0 (($var0 * UINT64_C(0x0101010101010101) & UINT64_C(0x8040201008040201)) * UINT64_C(0x0102040810204081) >> 49) & UINT64_C(0x5555) | (($var1 * UINT64_C(0x0101010101010101) & UINT64_C(0x8040201008040201)) * UINT64_C(0x0102040810204081) >> 48) & UINT64_C(0xAAAA)" },

	{ 2, { T_UINT08, T_UINT08 }, 1, "$var1 $abo0 (~(((($var0 & UINT16_C(0x7F7F)) + UINT16_C(0x7F7F)) | $var0) | UINT16_C(0x7F7F)))" },
	{ 2, { T_UINT16, T_UINT16 }, 1, "$var1 $abo0 (~(((($var0 & UINT32_C(0x7F7F7F7F)) + UINT32_C(0x7F7F7F7F)) | $var0) | UINT32_C(0x7F7F7F7F)))" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "$var1 $abo0 (~(((($var0 & UINT64_C(0x7F7F7F7F7F7F7F7F)) + UINT64_C(0x7F7F7F7F7F7F7F7F)) | $var0) | UINT64_C(0x7F7F7F7F7F7F7F7F)))" },

	{ 3, { T_UINT16, T_UINT08, T_UINT08 }, 2, "$var2 $abo0 ((($var0 ^ (~UINT32_C(0) / UINT32_C(0xFF) * $var1)) - UINT32_C(0x01010101)) & ~($var0 ^ (~UINT32_C(0) / UINT32_C(0xFF) * $var1)) & UINT32_C(0x80808080))" },
	{ 3, { T_UINT32, T_UINT16, T_UINT16 }, 2, "$var2 $abo0 ((($var0 ^ (~UINT64_C(0) / UINT64_C(0xFFFF) * $var1)) - UINT64_C(0x0101010101010101)) & ~($var0 ^ (~UINT64_C(0) / UINT64_C(0xFFFF) * $var1)) & UINT64_C(0x8080808080808080))" },

	{ 3, { T_UINT16, T_UINT08, T_UINT08 }, 2, "$var2 = (((($var0 & ~UINT32_C(0) / 0xFF * 0x7F) + ~UINT32_C(0) / 0xFF * (0x7F - $var1) | $var0) & ~UINT32_C(0) / 0xFF * 0x80) / 0x80 % 0xFF)" },

	{ 2, { T_UINT08, T_UINT08 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_UINT16, T_UINT16 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_UINT64, T_UINT64 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_SINT08, T_SINT08 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_SINT16, T_SINT16 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_SINT32, T_SINT32 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },
	{ 2, { T_SINT64, T_SINT64 }, 1, "$typ0 $tmp0 = (($var0 | ($var0 - $cst0(1))) + $cst0(1)); $var1 = ($typ1)($tmp0 | (((($tmp0 & -$tmp0) / ($var0 & -$var0)) >> 1) - $cst0(1)))" },

	{ 2, { T_UINT08, T_UINT08 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_UINT16, T_UINT16 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_UINT32, T_UINT32 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_UINT64, T_UINT64 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_SINT08, T_SINT08 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_SINT16, T_SINT16 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_SINT32, T_SINT32 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },
	{ 2, { T_SINT64, T_SINT64 }, 0, "$var0 $abo0 ($var0 ^ (($var0 ^ $var1) & -($var0 < $var1)))" },

	{ 1, { T_UINT16 }, 0, "$var0 = ((($var0 & $cst0(0xAAAA)) >> 1) | (($var0 & $cst0(0x5555)) << 1)), $var0 = ((($var0 & $cst0(0xCCCC)) >> 1) | (($var0 & $cst0(0x3333)) << 1)), $var0 = ((($var0 & $cst0(0xF0F0)) >> 2) | (($var0 & $cst0(0x0F0F)) << 2)), $var0 = ((($var0 & $cst0(0xFF00)) >> 4) | (($var0 & $cst0(0x00FF)) << 4)), $var0 = (($var0 >> 8) | ($var0 << 8))" },
	{ 1, { T_SINT16 }, 0, "$var0 = ((($var0 & $cst0(0xAAAA)) >> 1) | (($var0 & $cst0(0x5555)) << 1)), $var0 = ((($var0 & $cst0(0xCCCC)) >> 1) | (($var0 & $cst0(0x3333)) << 1)), $var0 = ((($var0 & $cst0(0xF0F0)) >> 2) | (($var0 & $cst0(0x0F0F)) << 2)), $var0 = ((($var0 & $cst0(0xFF00)) >> 4) | (($var0 & $cst0(0x00FF)) << 4)), $var0 = (($var0 >> 8) | ($var0 << 8))" },
	{ 1, { T_UINT32 }, 0, "$var0 = ((($var0 & $cst0(0xAAAAAAAA)) >> 1) | (($var0 & $cst0(0x55555555)) << 1)), $var0 = ((($var0 & $cst0(0xCCCCCCCC)) >> 2) | (($var0 & $cst0(0x33333333)) << 2)), $var0 = ((($var0 & $cst0(0xF0F0F0F0)) >> 4) | (($var0 & $cst0(0x0F0F0F0F)) << 4)), $var0 = ((($var0 & $cst0(0xFF00FF00)) >> 8) | (($var0 & $cst0(0x00FF00FF)) << 8)), $var0 = (($var0 >> 16) | ($var0 << 16))" },
	{ 1, { T_SINT32 }, 0, "$var0 = ((($var0 & $cst0(0xAAAAAAAA)) >> 1) | (($var0 & $cst0(0x55555555)) << 1)), $var0 = ((($var0 & $cst0(0xCCCCCCCC)) >> 2) | (($var0 & $cst0(0x33333333)) << 2)), $var0 = ((($var0 & $cst0(0xF0F0F0F0)) >> 4) | (($var0 & $cst0(0x0F0F0F0F)) << 4)), $var0 = ((($var0 & $cst0(0xFF00FF00)) >> 8) | (($var0 & $cst0(0x00FF00FF)) << 8)), $var0 = (($var0 >> 16) | ($var0 << 16))" },

	{ 2, { T_UINT32, T_UINT32 }, 1, "$var0 = ((($var0 & $cst0(0xAAAAAAAA)) >> 1) | (($var0 & $cst0(0x55555555)) << 1)), $var0 = ((($var0 & $cst0(0xCCCCCCCC)) >> 2) | (($var0 & $cst0(0x33333333)) << 2)), $var0 = ((($var0 & $cst0(0xF0F0F0F0)) >> 4) | (($var0 & $cst0(0x0F0F0F0F)) << 4)), $var0 = ((($var0 & $cst0(0xFF00FF00)) >> 8) | (($var0 & $cst0(0x00FF00FF)) << 8)), $var1 = (($var0 >> 16) | ($var0 << 16))" },
	{ 2, { T_SINT32, T_SINT32 }, 1, "$var0 = ((($var0 & $cst0(0xAAAAAAAA)) >> 1) | (($var0 & $cst0(0x55555555)) << 1)), $var0 = ((($var0 & $cst0(0xCCCCCCCC)) >> 2) | (($var0 & $cst0(0x33333333)) << 2)), $var0 = ((($var0 & $cst0(0xF0F0F0F0)) >> 4) | (($var0 & $cst0(0x0F0F0F0F)) << 4)), $var0 = ((($var0 & $cst0(0xFF00FF00)) >> 8) | (($var0 & $cst0(0x00FF00FF)) << 8)), $var1 = (($var0 >> 16) | ($var0 << 16))" },

	{ 2, { T_UINT32, T_UINT16 }, 1, "$var0 ^= ($var0 >> 16), $var0 ^= ($var0 >> 8), $var0 ^= ($var0 >> 4), $var0 ^= ($var0 >> 2), $var1 = $var0, $var1 ^= ($var0 >> 1)" },
	{ 2, { T_SINT32, T_SINT16 }, 1, "$var0 ^= ($var0 >> 16), $var0 ^= ($var0 >> 8), $var0 ^= ($var0 >> 4), $var0 ^= ($var0 >> 2), $var1 = $var0, $var1 ^= ($var0 >> 1)" },

	{ 1, { T_UINT08 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x55)), $var0 = ((($var0 >> 1) & $cst0(0x33)) + ($var0 & $cst0(0x33))), $var0 = ((($var0 >> 1) + $var0) & $cst0(0x0F)), $var0 += ($var0 >> 2), $var0 += ($var0 >> 4), $var0 = ($var0 & $cst0(0x3F))" },
	{ 1, { T_SINT08 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x55)), $var0 = ((($var0 >> 1) & $cst0(0x33)) + ($var0 & $cst0(0x33))), $var0 = ((($var0 >> 1) + $var0) & $cst0(0x0F)), $var0 += ($var0 >> 2), $var0 += ($var0 >> 4), $var0 = ($var0 & $cst0(0x3F))" },
	{ 1, { T_UINT16 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x5555)), $var0 = ((($var0 >> 1) & $cst0(0x3333)) + ($var0 & $cst0(0x3333))), $var0 = ((($var0 >> 2) + $var0) & $cst0(0x0F0F)), $var0 += ($var0 >> 4), $var0 += ($var0 >> 8), $var0 = ($var0 & $cst0(0x003F))" },
	{ 1, { T_SINT16 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x5555)), $var0 = ((($var0 >> 1) & $cst0(0x3333)) + ($var0 & $cst0(0x3333))), $var0 = ((($var0 >> 2) + $var0) & $cst0(0x0F0F)), $var0 += ($var0 >> 4), $var0 += ($var0 >> 8), $var0 = ($var0 & $cst0(0x003F))" },
	{ 1, { T_UINT32 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x55555555)), $var0 = ((($var0 >> 2) & $cst0(0x33333333)) + ($var0 & $cst0(0x33333333))), $var0 = ((($var0 >> 4) + $var0) & $cst0(0x0F0F0F0F)), $var0 += ($var0 >> 8), $var0 += ($var0 >> 16), $var0 = ($var0 & $cst0(0x0000003F))" },
	{ 1, { T_SINT32 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x55555555)), $var0 = ((($var0 >> 2) & $cst0(0x33333333)) + ($var0 & $cst0(0x33333333))), $var0 = ((($var0 >> 4) + $var0) & $cst0(0x0F0F0F0F)), $var0 += ($var0 >> 8), $var0 += ($var0 >> 16), $var0 = ($var0 & $cst0(0x0000003F))" },

	{ 2, { T_UINT32, T_UINT32 }, 1, "$var0 -= (($var0 >> 1) & $cst0(0x55555555)), $var0 = ((($var0 >> 2) & $cst0(0x33333333)) + ($var0 & $cst0(0x33333333))), $var0 = ((($var0 >> 4) + $var0) & $cst0(0x0F0F0F0F)), $var0 += ($var0 >> 8), $var0 += ($var0 >> 16), $var1 = ($var0 & $cst0(0x0000003F))" },
	{ 1, { T_UINT32 }, 0, "$var0 -= (($var0 >> 1) & $cst0(0x55555555)), $var0 = ((($var0 >> 2) & $cst0(0x33333333)) + ($var0 & $cst0(0x33333333))), $var0 = ((($var0 >> 4) + $var0) & $cst0(0x0F0F0F0F)), $var0 += ($var0 >> 8), $var0 += ($var0 >> 16), $var0 = ($var0 & $cst0(0x0000003F))" },

	{ 2, { T_SINT08, T_SINT08 }, 1, "$var1 $abo0 ($var0 + ($var0 >> sizeof($typ0) * CHAR_BIT - 1)) ^ ($var0 >> sizeof($typ0) * CHAR_BIT - 1)" },
	{ 2, { T_SINT16, T_SINT16 }, 1, "$var1 $abo0 ($var0 + ($var0 >> sizeof($typ0) * CHAR_BIT - 1)) ^ ($var0 >> sizeof($typ0) * CHAR_BIT - 1)" },
	{ 2, { T_SINT32, T_SINT32 }, 1, "$var1 $abo0 ($var0 + ($var0 >> sizeof($typ0) * CHAR_BIT - 1)) ^ ($var0 >> sizeof($typ0) * CHAR_BIT - 1)" },
	{ 2, { T_SINT64, T_SINT64 }, 1, "$var1 $abo0 ($var0 + ($var0 >> sizeof($typ0) * CHAR_BIT - 1)) ^ ($var0 >> sizeof($typ0) * CHAR_BIT - 1)" },

	{ 2, { T_SINT08, T_SINT08, T_UINT08 }, 1, "$var0 = $var0 & (($cst2(1) << $var2) - 1), $var1 $abo0 ($var0 ^ ($cst2(1) << ($var2 - 1))) - ($cst2(1) << ($var2 - 1))" },
	{ 2, { T_SINT16, T_SINT16, T_UINT16 }, 1, "$var0 = $var0 & (($cst2(1) << $var2) - 1), $var1 $abo0 ($var0 ^ ($cst2(1) << ($var2 - 1))) - ($cst2(1) << ($var2 - 1))" },
	{ 2, { T_SINT32, T_SINT32, T_UINT32 }, 1, "$var0 = $var0 & (($cst2(1) << $var2) - 1), $var1 $abo0 ($var0 ^ ($cst2(1) << ($var2 - 1))) - ($cst2(1) << ($var2 - 1))" },
	{ 2, { T_SINT64, T_SINT64, T_UINT64 }, 1, "$var0 = $var0 & (($cst2(1) << $var2) - 1), $var1 $abo0 ($var0 ^ ($cst2(1) << ($var2 - 1))) - ($cst2(1) << ($var2 - 1))" },

	{ 4, { T_UINT08, T_UINT08, T_UINT08, T_UINT08 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_UINT16, T_UINT16, T_UINT16, T_UINT16 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_UINT32, T_UINT32, T_UINT32, T_UINT32 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_UINT64, T_UINT64, T_UINT64, T_UINT64 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_SINT08, T_SINT08, T_SINT08, T_SINT08 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_SINT16, T_SINT16, T_SINT16, T_SINT16 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_SINT32, T_SINT32, T_SINT32, T_SINT32 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },
	{ 4, { T_SINT64, T_SINT64, T_SINT64, T_SINT64 }, 3, "$var3 $abo0 ($var0 ^ (($var0 ^ $var1) & $var2))" },

	{ 2, { T_UINT08, T_UINT08 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_UINT16, T_UINT16 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_UINT64, T_UINT64 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_SINT08, T_SINT08 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_SINT16, T_SINT16 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_SINT32, T_SINT32 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },
	{ 2, { T_SINT64, T_SINT64 }, 1, "for ($var1 = $cst0(0); $var0; ++$var1) $var0 &= $var0 - $cst0(1)" },

	{ 1, { T_UINT32 }, 0, "$var0 ^= $var0 >> 16, $var0 ^= $var0 >> 8, $var0 ^= $var0 >> 4, $var0 &= $cst0(0xF), $var0 = ($cst0(0x6996) >> $var0) & $cst0(1)" },

	{ 2, { T_UINT08, T_UINT08 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_UINT16, T_UINT16 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_UINT32, T_UINT32 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_UINT64, T_UINT64 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_SINT08, T_SINT08 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_SINT16, T_SINT16 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_SINT32, T_SINT32 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },
	{ 2, { T_SINT64, T_SINT64 }, 0, "$var0 = $var0 & (((($typ0)1) << $var1) - (($typ0)1))" },

	{ 2, { T_UINT32, T_UINT08 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_UINT16 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_UINT64 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_SINT08 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_SINT16 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_SINT32 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },
	{ 2, { T_UINT32, T_SINT64 }, 1, "$typ1 $tmp0[] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 }; $var0 |= $var0 >> 1, $var0 |= $var0 >> 2, $var0 |= $var0 >> 4, $var0 |= $var0 >> 8, $var0 |= $var0 >> 16; $var1 = $tmp0[($typ0)($var0 * $cst0(0x07C4ACDD)) >> 27]" },

	{ 2, { T_UINT16, T_UINT16 }, 1, "$var1 = $cst0(16); $var0 &= -((int16_t)$var0); if ($var0) $var1--; if ($var0 & $cst0(0x00FF)) $var1 -= $cst0(16); if ($var0 & $cst0(0x00FF)) $var1 -= $cst0(8); if ($var0 & $cst0(0x0F0F)) $var1 -= $cst0(4); if ($var0 & $cst0(0x3333)) $var1 -= $cst0(4); if ($var0 & $cst0(0x5555)) $var1 -= $cst0(1)" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "$var1 = $cst0(32); $var0 &= -((int32_t)$var0); if ($var0) $var1--; if ($var0 & $cst0(0x0000FFFF)) $var1 -= $cst0(16); if ($var0 & $cst0(0x00FF00FF)) $var1 -= $cst0(8); if ($var0 & $cst0(0x0F0F0F0F)) $var1 -= $cst0(4); if ($var0 & $cst0(0x33333333)) $var1 -= $cst0(2); if ($var0 & $cst0(0x55555555)) $var1 -= $cst0(1)" },
	
	{ 2, { T_UINT32, T_UINT08 }, 1, "$var1 = ($var0 >= $cst0(1000000000)) ? 9 : ($var0 >= $cst0(100000000)) ? 8 : ($var0 >= $cst0(10000000)) ? 7 : ($var0 >= $cst0(1000000)) ? 6 : ($var0 >= $cst0(100000)) ? 5 : ($var0 >= $cst0(10000)) ? 4 : ($var0 >= $cst0(1000)) ? 3 : ($var0 >= $cst0(100)) ? 2 : ($var0 >= $cst0(10)) ? 1 : 0" },
	{ 2, { T_UINT32, T_UINT16 }, 1, "$var1 = ($var0 >= $cst0(1000000000)) ? 9 : ($var0 >= $cst0(100000000)) ? 8 : ($var0 >= $cst0(10000000)) ? 7 : ($var0 >= $cst0(1000000)) ? 6 : ($var0 >= $cst0(100000)) ? 5 : ($var0 >= $cst0(10000)) ? 4 : ($var0 >= $cst0(1000)) ? 3 : ($var0 >= $cst0(100)) ? 2 : ($var0 >= $cst0(10)) ? 1 : 0" },
	{ 2, { T_UINT32, T_UINT32 }, 1, "$var1 = ($var0 >= $cst0(1000000000)) ? 9 : ($var0 >= $cst0(100000000)) ? 8 : ($var0 >= $cst0(10000000)) ? 7 : ($var0 >= $cst0(1000000)) ? 6 : ($var0 >= $cst0(100000)) ? 5 : ($var0 >= $cst0(10000)) ? 4 : ($var0 >= $cst0(1000)) ? 3 : ($var0 >= $cst0(100)) ? 2 : ($var0 >= $cst0(10)) ? 1 : 0" },
	{ 2, { T_UINT32, T_UINT64 }, 1, "$var1 = ($var0 >= $cst0(1000000000)) ? 9 : ($var0 >= $cst0(100000000)) ? 8 : ($var0 >= $cst0(10000000)) ? 7 : ($var0 >= $cst0(1000000)) ? 6 : ($var0 >= $cst0(100000)) ? 5 : ($var0 >= $cst0(10000)) ? 4 : ($var0 >= $cst0(1000)) ? 3 : ($var0 >= $cst0(100)) ? 2 : ($var0 >= $cst0(10)) ? 1 : 0" },

	{ 2, { T_SINGLE, T_UINT08 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs080)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_UINT16 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs160)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_UINT32 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs320)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_UINT64 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs640)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_SINT08 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs080)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_SINT16 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs160)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_SINT32 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs320)) - 1.0F)) / $fltc0) + $var0)" },
	{ 2, { T_SINGLE, T_SINT64 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + 1.0F) / $var0) * ((($typ0)($var1 << $bs640)) - 1.0F)) / $fltc0) + $var0)" },

	{ 2, { T_SINGLE, T_UINT08 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rd080)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_UINT16 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rd160)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_UINT32 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rd320)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_UINT64 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rd640)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_SINT08 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rs080)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_SINT16 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rs160)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_SINT32 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rs320)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },
	{ 2, { T_SINGLE, T_SINT64 }, 1, "$var1 $aao0 ($typ1)((((((($typ0)$var1) + $fltc0) / $var0) $aop1 ((($typ0)($var1 $aop0 $rs640)) $aop3 1.0F)) / $fltc0) $aop2 $var0)" },

	{ 2, { T_SINGLE, T_UINT08 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc080" },
	{ 2, { T_SINGLE, T_UINT16 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc160" },
	{ 2, { T_SINGLE, T_UINT32 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc320" },
	{ 2, { T_SINGLE, T_UINT64 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc640" },
	{ 2, { T_SINGLE, T_SINT08 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc080" },
	{ 2, { T_SINGLE, T_SINT16 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc160" },
	{ 2, { T_SINGLE, T_SINT32 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc320" },
	{ 2, { T_SINGLE, T_SINT64 }, 1, "$var1 $aao0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) / $var0) $aop1 $bc640" },

	{ 2, { T_SINGLE, T_UINT08 }, 1, "$var1 $abo0 $bp080 $bop0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_UINT16 }, 1, "$var1 $abo0 $bp160 $bop0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_UINT32 }, 1, "$var1 $abo0 $bp320 $bop0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_UINT64 }, 1, "$var1 $abo0 $bp640 $bop0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },

	{ 2, { T_SINGLE, T_SINT08 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_SINT16 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_SINT32 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_SINT64 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },

	{ 2, { T_SINT08, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },
	{ 2, { T_SINT16, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },
	{ 2, { T_SINT32, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },
	{ 2, { T_SINT64, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },

	{ 2, { T_SINT08, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },
	{ 2, { T_SINT16, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },
	{ 2, { T_SINT32, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },
	{ 2, { T_SINT64, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },

	{ 2, { T_SINGLE, T_UINT08 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_UINT16 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_UINT32 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },
	{ 2, { T_SINGLE, T_UINT64 }, 1, "$var1 $abo0 ($typ1)(((($typ0)$var1) $aop0 $fltc0) $aop1 $var0)" },

	{ 2, { T_UINT08, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },
	{ 2, { T_UINT16, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },
	{ 2, { T_UINT32, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },
	{ 2, { T_UINT64, T_SINGLE }, 1, "$var1 $aao0 ($typ1)($var0 $aop0 $fltc0)" },

	{ 2, { T_UINT08, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },
	{ 2, { T_UINT16, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },
	{ 2, { T_UINT32, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },
	{ 2, { T_UINT64, T_SINGLE }, 1, "$var1 $aao0 $fl010 + (((($typ1)$var0) $aop1 $var1) $aop0 $fltc0)" },

	{ 1, { T_SINGLE }, 0, "$var0 $aao0 ($var0 $aop0 $fltc0)" },
	{ 1, { T_SINGLE }, 0, "$var0 $aao0 ($var0 $aop0 $fl010)" },
	{ 1, { T_SINGLE }, 0, "$var0 $aao0 ($var0 $aop0 $fltp0)" },

	{ 1, { T_SINGLE }, 0, "$var0 = ($var0 $cmp0 $fltc0) ? ($var0 $aop0 $fltc0) : $fltc1 $aop0 $sgn0$var0" },
	{ 1, { T_SINGLE }, 0, "$var0 = ($var0 $cmp0 $fl010) ? ($var0 $aop0 $fl010) : $fltc1 $aop0 $sgn0$var0" },
	{ 1, { T_SINGLE }, 0, "$var0 = ($var0 $cmp0 $fltp0) ? ($var0 $aop0 $fltp0) : $fltc1 $aop0 $sgn0$var0" },

	{ 1, { T_SINGLE }, 0, "$var0 = ($var0 $cmp0 ($var0 $aop0 $fltc0)) ? $sgn1($var0 $aop0 $fltc0) : $fltc1 $aop0 $sgn0$var0" },
	{ 1, { T_SINGLE }, 0, "$var0 = ($var0 $cmp0 ($var0 $aop0 $fl010)) ? $sgn1($var0 $aop0 $fl010) : $fltc1 $aop0 $sgn0$var0" },
	{ 1, { T_SINGLE }, 0, "$var0 = ($var0 $cmp0 ($var0 $aop0 $fltp0)) ? $sgn1($var0 $aop0 $fltp0) : $fltc1 $aop0 $sgn0$var0" },

	{ 1, { T_SINGLE }, 0, "$var0 $aao0 $var0" },
	{ 1, { T_SINGLE }, 0, "$var0 $aao0 $fltc0" },
	{ 1, { T_SINGLE }, 0, "$var0 $aao0 $fl010" },
	{ 1, { T_SINGLE }, 0, "$var0 $aao0 $fltp0" },

	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 $var1" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $fl010)" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $fltc0)" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $fltp0)" },

	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $var0)" },

	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $var0) $aop1 $fl010" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $var0) $aop1 $fltc0" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $var0) $aop1 $fltp0" },

	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $fl010)" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $fltc0)" },
	{ 2, { T_SINGLE, T_SINGLE }, 0, "$var0 $aao0 ($var1 $aop0 $fltp0)" },
};


const char* get_macro(const char* type, uint8_t index)
{
	static char buffer[64];
	buffer[0] = 0;
	sprintf(buffer, "$%s%d", type, (int)index);
	return buffer;
}


#define N_HEX 0 // Hex
#define N_PAT 1 // Hex pattern.
#define N_SGN 2 // Signed decimal.
#define N_UNS 3 // Unsigned decimal.
#define N_BIT 4 // Bit count.
#define N_SHF 5 // Bit shift.
#define N_FL1 6 // Float 0..1
#define N_FLS 7 // Float signed.
#define N_FLU 8 // Float unsigned.
#define N_FLC 9 // Float common/constant.
#define N_PRI 10 // Prime number.

uint8_t is_prime(uint64_t v) 
{
	uint16_t i;
	if (v == 0 || v == 1) return 0;
	for (i = 2; i*i <= v; ++i)
		if (v % i == 0) return 0;
	return 1;
}


uint64_t next_prime_in(uint64_t a, uint64_t b)
{
	uint64_t n = next_rand_in(a, b);
	while(!is_prime(n)) n = next_rand_in(a, b);
	return n;
}


const char* make_number(uint8_t type, uint8_t bits)
{
	static char buffer[512] = { 0 };

	char temp[512] = { 0 };

	buffer[0] = 0;

	switch (type)
	{
	case N_HEX:
	{
		sprintf(buffer, "0x%016" PRIX64, next_rand());
		bits /= 4;
		if (bits < 16) buffer[bits + 2] = '\0';
		return buffer;
	}
	case N_PAT:
	{
		const char* pat = bit_patterns[next_rand_in(0, 45)];
		sprintf(buffer, "0x%s", pat);
		bits /= 4;
		if (bits < 16) buffer[bits + 2] = '\0';
		return buffer;
	}
	case N_SGN:
	{
		uint64_t m = ((uint64_t)pow(1.9999999998, (double)bits)) / 2;
		int64_t n = (int64_t)next_rand_in(1, m);
		if (next_rand_in(0, 1) == 1) n = -n;
		sprintf(buffer, "%" PRId64, n);
		return buffer;
	}
	case N_UNS:
	{
		uint64_t m = ((uint64_t)pow(1.9999999998, (double)bits));
		uint64_t n = next_rand_in(1, m);
		sprintf(buffer, "%" PRId64, n);
		return buffer;
	}
	case N_BIT:
	{
		uint64_t n = next_rand_in(0, (uint64_t)bits);
		sprintf(buffer, "%" PRId64, n);
		return buffer;
	}
	case N_SHF:
	{
		uint64_t n = next_rand_in(1, (uint64_t)(bits - 1));
		sprintf(buffer, "%" PRId64, n);
		return buffer;
	}
	case N_PRI:
	{
		uint64_t n = 0;
		if (bits == 8) n = next_prime_in(3, (uint64_t)255);
		else if (bits == 16) n = next_prime_in(3, (uint64_t)999);
		else n = next_prime_in(3, (uint64_t)9999);
		sprintf(buffer, "%" PRId64, n);
		return buffer;
	}
	case N_FL1:
	{
		double n = (double)next_rand_in(2, 999999999);
		n = 1.0 / n;
		float f = (float)n;
		temp[0] = '%';
		sprintf(&temp[1], "1.%dfF", (int)next_rand_in(1, 8));
		sprintf(buffer, temp, f);
		return buffer;
	}
	case N_FLS:
	{
		double n1 = (double)next_rand_in(1, 999999999);
		double n2 = (double)next_rand_in(2, 999999998);
		if (next_rand_in(0, 1) == 1) n1 = -n1;
		n1 = n1 / n2;
		float f = (float)n1;
		temp[0] = '%';
		sprintf(&temp[1], "%d.%dfF", (int)next_rand_in(1, 4), (int)next_rand_in(1, 8));
		sprintf(buffer, temp, f);
		return buffer;
	}
	case N_FLU:
	{
		double n1 = (double)next_rand_in(1, 999999999);
		double n2 = (double)next_rand_in(2, 999999998);
		n1 = n1 / n2;
		float f = (float)fabs(n1);
		temp[0] = '%';
		sprintf(&temp[1], "%d.%dfF", (int)next_rand_in(1, 4), (int)next_rand_in(1, 8));
		sprintf(buffer, temp, f);
		return buffer;
	}
	case N_FLC:
	{
		float f = rfltcs[next_rand_in(0, 78)];
		sprintf(buffer, "%fF", f);
		return buffer;
	}
	default: return "1";
	}
}


// Blindly expand all possibilities in the given statement.
char* expand(char* buffer, const stmt_t* stmt, var_t** vars, uint8_t vcnt, uint8_t looping)
{
	uint8_t i;
	char tempn[64] = { 0 };
	char arryname[1024] = { 0 };

	buffer[0] = 0;
	strcpy(buffer, stmt->code);

	for (i = 0; i < vcnt; ++i)
	{
		if (vars[i]->arity == V_VECTOR && i == looping)
		{
			sprintf(arryname, "%s[i]", vars[i]->name);
			replace(buffer, get_macro(M_VARNM, i), arryname);
		}
		else if (vars[i]->arity == V_VECTOR && looping == 0xFF)
		{
			sprintf(arryname, "%s[%d]", vars[i]->name, (int)next_rand_in(0, vars[i]->count - 1));
			replace(buffer, get_macro(M_VARNM, i), arryname);
		}
		else replace(buffer, get_macro(M_VARNM, i), vars[i]->name);

		replace(buffer, get_macro(M_TYPEV, i), get_type(vars[i]->type));
		replace(buffer, get_macro(M_CONST, i), get_const(vars[i]->type));
	}

	for (i = 0; i < 16; ++i)
	{
		uint64_t a = i + (i * 2);
		uint64_t b = a + 32;

		sprintf(tempn, "t%c%c%d", alphas[next_rand_in(0, 25)], alphas[next_rand_in(0, 25)], (int)next_rand_in(a, b));
		replace(buffer, get_macro(M_TEMPN, i), tempn);

		replace(buffer, get_macro(M_RAH08, i), make_number(N_HEX, 8));
		replace(buffer, get_macro(M_RAH16, i), make_number(N_HEX, 16));
		replace(buffer, get_macro(M_RAH32, i), make_number(N_HEX, 32));
		replace(buffer, get_macro(M_RAH64, i), make_number(N_HEX, 64));
		replace(buffer, get_macro(M_RAP08, i), make_number(N_PAT, 8));
		replace(buffer, get_macro(M_RAP16, i), make_number(N_PAT, 16));
		replace(buffer, get_macro(M_RAP32, i), make_number(N_PAT, 32));
		replace(buffer, get_macro(M_RAP64, i), make_number(N_PAT, 64));
		replace(buffer, get_macro(M_RAS08, i), make_number(N_SGN, 8));
		replace(buffer, get_macro(M_RAS16, i), make_number(N_SGN, 16));
		replace(buffer, get_macro(M_RAS32, i), make_number(N_SGN, 32));
		replace(buffer, get_macro(M_RAS64, i), make_number(N_SGN, 64));
		replace(buffer, get_macro(M_RAD08, i), make_number(N_UNS, 8));
		replace(buffer, get_macro(M_RAD16, i), make_number(N_UNS, 16));
		replace(buffer, get_macro(M_RAD32, i), make_number(N_UNS, 32));
		replace(buffer, get_macro(M_RAD64, i), make_number(N_UNS, 64));
		replace(buffer, get_macro(M_RAB08, i), make_number(N_BIT, 8));
		replace(buffer, get_macro(M_RAB16, i), make_number(N_BIT, 16));
		replace(buffer, get_macro(M_RAB32, i), make_number(N_BIT, 32));
		replace(buffer, get_macro(M_RAB64, i), make_number(N_BIT, 64));
		replace(buffer, get_macro(M_RSH08, i), make_number(N_SHF, 8));
		replace(buffer, get_macro(M_RSH16, i), make_number(N_SHF, 16));
		replace(buffer, get_macro(M_RSH32, i), make_number(N_SHF, 32));
		replace(buffer, get_macro(M_RSH64, i), make_number(N_SHF, 64));
		replace(buffer, get_macro(M_RFL01, i), make_number(N_FL1, 32));
		replace(buffer, get_macro(M_RFLTA, i), make_number(N_FLS, 32));
		replace(buffer, get_macro(M_RFLTP, i), make_number(N_FLU, 32));
		replace(buffer, get_macro(M_RFLTC, i), make_number(N_FLC, 32));

		replace(buffer, get_macro(M_BOPAR, i), bopars[next_rand_in(0, 2)]);
		replace(buffer, get_macro(M_BOPBT, i), bopbts[next_rand_in(0, 4)]);
		replace(buffer, get_macro(M_BOPDV, i), bopdvs[next_rand_in(0, 1)]);
		replace(buffer, get_macro(M_UONOT, i), uonots[next_rand_in(0, 1)]);
		replace(buffer, get_macro(M_UOSGN, i), uosgns[next_rand_in(0, 2)]);
		replace(buffer, get_macro(M_AOPAR, i), aopars[next_rand_in(0, 3)]);
		replace(buffer, get_macro(M_AOPBT, i), aopbts[next_rand_in(0, 4)]);
		replace(buffer, get_macro(M_AOPDV, i), aopdvs[next_rand_in(0, 1)]);
		replace(buffer, get_macro(M_COMPR, i), comprs[next_rand_in(0, 5)]);
	}

	for (i = 0; i < vcnt; ++i)
	{
		vars[i]->modified = 0;
		vars[i]->assigned_to = stmt->result;
		vars[i]->is_used++;
	}

	if (stmt->result != 0xFF)
	{
		vars[stmt->result]->modified = 1;
		vars[stmt->result]->assigned_to = stmt->result;
	}
	else if (stmt->result == 0xFF)
	{
		vars[vcnt - 1]->modified = 1;
		vars[vcnt - 1]->assigned_to = (vcnt - 1);
	}

	return buffer;
}


void make_variable_name(char* buffer, uint64_t i)
{
	sprintf(buffer, "v%c%c%d", alphas[next_rand_in(0, 26)], alphas[next_rand_in(0, 26)], (int)(i + 1));
}


#define I_RAND 0 // Random init.
#define I_ZERO 1 // Zero init.
#define I_INPT 2 // Input init.

const char* make_init_value(var_t* var, uint8_t type, uint8_t bits)
{
	static char buffer[1024] = { '0', 0 };

	if (type == I_RAND)
	{
		switch (var->type)
		{
		case T_UINT08: case T_UINT16: case T_UINT32: case T_UINT64:
		{
			if (!next_rand_in(0, 1)) return make_number(N_PAT, bits);
			else if (!next_rand_in(0, 2)) return make_number(N_HEX, bits);
			else if (!next_rand_in(0, 2)) return make_number(N_UNS, bits);
			else if (!next_rand_in(0, 1)) return make_number(N_BIT, bits);
			else if (!next_rand_in(0, 1)) return make_number(N_SHF, bits);
			else if (!next_rand_in(0, 1)) return make_number(N_PRI, bits);
			else return make_number(N_HEX, bits);
		}
		case T_SINT08: case T_SINT16: case T_SINT32: case T_SINT64:
		{
			if (!next_rand_in(0, 1)) return make_number(N_SGN, bits);
			else if (!next_rand_in(0, 2)) return make_number(N_BIT, bits);
			else if (!next_rand_in(0, 2)) return make_number(N_SHF, bits);
			else if (!next_rand_in(0, 1)) return make_number(N_UNS, bits > 8 ? bits / 2 : bits);
			else if (!next_rand_in(0, 1)) return make_number(N_PRI, bits);
			else return make_number(N_SGN, bits > 8 ? bits / 2 : bits);
		}
		case T_SINGLE:
		{
			if (!next_rand_in(0, 1)) return make_number(N_FLC, bits);
			else if (!next_rand_in(0, 1)) return make_number(N_FL1, bits);
			else if (!next_rand_in(0, 1)) return make_number(N_FLU, bits);
			else if (!next_rand_in(0, 2)) return make_number(N_FLS, bits);
			else return make_number(N_FLC, bits);
		}
		}
	}
	else if (type == I_ZERO)
	{
		switch (var->type)
		{
		case T_UINT08: case T_UINT16: case T_UINT32: case T_UINT64:
		case T_SINT08: case T_SINT16: case T_SINT32: case T_SINT64:
			sprintf(buffer, "%s(0)", get_const(var->type));
			return buffer;
		case T_SINGLE: return "0.0F";
		default: return "0";
		}
	}
	else
	{
		sprintf(buffer, "((%s)value)", get_type(var->type));
		return buffer;
	}

	return buffer;
}


const char* make_init(var_t* var, uint8_t type)
{
	static char buffer[16384] = { 0 };
	char temp[1024] = { 0 };
	uint16_t i;
	uint8_t bits = get_bits(var->type);
	const char* number = NULL;

	if (var->arity == V_VECTOR)
	{
		buffer[0] = 0;
		sprintf(buffer, "%s %s[%d] = { ", get_type(var->type), var->name, (int)var->count);
		for (i = 0; i < var->count; ++i)
		{
			strcat(buffer, make_init_value(var, type, bits));
			if (i < (var->count - 1)) strcat(buffer, ",");
			strcat(buffer, " ");
			type = (uint8_t)next_rand_in(0, 2);
		}
		strcat(buffer, "}");
	}
	else
	{
		buffer[0] = 0;
		sprintf(buffer, "%s %s = %s", get_type(var->type), var->name, make_init_value(var, type, bits));
	}

	return buffer;
}


uint8_t select_vars_for_stmt(const stmt_t* stmt, var_t* vars, uint64_t nvars, var_t** selvars, uint64_t* nselvars, uint8_t wantvect, uint8_t* vectat)
{
	uint8_t tries = 0;
	uint8_t i, j, currvar = 0;
	var_t* varptr = NULL;
	uint8_t havevect = 0;

	*nselvars = 0;
	*vectat = 0xFF;

	for (i = 0; i < stmt->operands; ++i)
	{
		uint8_t type = stmt->operand_types[i];
		uint8_t varsels = 0;

		tries = 0;

	LOC_TRY_AGAIN:

		for (j = (uint8_t)next_rand_in(0, nvars - 1); vars[j].type != type; j = (uint8_t)next_rand_in(0, nvars - 1))
		{
			varsels++;
			if (varsels == 0xFF) return 0;
		}

		if (vars[j].type != type)
		{
			tries++;
			if (tries < 128) goto LOC_TRY_AGAIN;
			else return 0;
		}

		if (vars[j].arity == V_VECTOR)
		{
			havevect = 1;
			*vectat = currvar;
		}

		varptr = &vars[j];
		selvars[currvar] = varptr;
		currvar++;
		
		if (currvar == stmt->operands)
			break;
	}

	*nselvars = stmt->operands;

	return 1;
}


uint8_t make_next_statement(FILE* file, var_t* tvars, uint64_t nvars, uint8_t* tabs, uint8_t looping)
{
	char buffer[32768] = { 0 };
	var_t* tmpv[32] = { NULL };
	uint64_t vcnt = 0;
	uint8_t i, vat;

	const stmt_t* stmt = &statements[next_rand_in(0, (sizeof(statements) / sizeof(stmt_t)) - 1)];

	while (!select_vars_for_stmt(stmt, tvars, nvars, tmpv, &vcnt, 0, &vat))
		stmt = &statements[next_rand_in(0, 258)];

	if (select_vars_for_stmt(stmt, tvars, nvars, tmpv, &vcnt, 0, &vat))
	{
		for (i = 0; i < *tabs; ++i) fprintf(file, T);
		expand(buffer, stmt, tmpv, (uint8_t)vcnt, looping);
		if (strchr(buffer, '$')) return 0;
		fprintf(file, "%s;" N, buffer);
		return 1;
	}

	return 0;
}


uint8_t make_next_loop_statement(FILE* file, const stmt_t* stmt, var_t** tmpv, uint64_t vcnt, uint8_t* tabs, uint8_t vectat)
{
	char buffer[32768] = { 0 };
	uint8_t i;
	for (i = 0; i < *tabs; ++i) fprintf(file, T);
	expand(buffer, stmt, tmpv, (uint8_t)vcnt, vectat);
	if (strchr(buffer, '$')) return 0;
	fprintf(file, "%s;" N, buffer);
	return 1;
}


uint8_t are_all_vars_used(var_t* tvars, uint64_t vcnt)
{
	uint64_t i;
	for (i = 0; i < vcnt; ++i)
		if (tvars[i].is_used < 2)
			return 0;
	return 1;
}

const stmt_t* get_next_statement_for_looping(var_t* tvars, uint64_t nvars, var_t** tmpv, uint64_t* vcnt, uint8_t* vectat)
{
	const stmt_t* stmt = &statements[next_rand_in(0, (sizeof(statements) / sizeof(stmt_t)) - 1)];
	*vectat = 0xFF;

	while (!select_vars_for_stmt(stmt, tvars, nvars, tmpv, vcnt, 1, vectat))
		stmt = &statements[next_rand_in(0, (sizeof(statements) / sizeof(stmt_t)) - 1)];

	if (select_vars_for_stmt(stmt, tvars, nvars, tmpv, vcnt, 1, vectat))
		return stmt;

	*vectat = 0xFF;
	return NULL;
}


void generate_mutator_function(const char* name)
{
	uint64_t i, j, k, n;
	char file_name[512];
	char buffer[32768] = { 0 };

	sprintf(file_name, "%s.c", name);

	FILE* file = open_file(file_name);

	if (!file)
	{
		printf("mkc: Error: Failed to open function file \"%s\" for output.\n", file_name);
		return;
	}

	uint64_t nvars = next_rand_in(8, 16) + 1;
	uint64_t steps = next_rand_in(nvars, nvars * 2);
	uint64_t subxs = next_rand_in(2, 8);

	var_t tvars[64];

	memset(tvars, 0, sizeof(tvars));

	uint8_t has_vectors = 0;

	strcpy(tvars[0].name, "value");
	tvars[0].type = T_UINT64;
	tvars[0].arity = V_SCALAR;
	tvars[0].modified = 1;
	tvars[0].is_used = 0;

	for (i = 1; i < nvars; ++i)
	{
		make_variable_name(tvars[i].name, i);

		if (next_rand_in(0, 7) == 0)
		{
			tvars[i].arity = V_VECTOR;
			if (next_rand_in(0, 3) < 3)
				tvars[i].count = (uint16_t)powers[next_rand_in(0, 6)];
			else tvars[i].count = (uint16_t)next_rand_in(2, 64);
			has_vectors++;
		}
		else
		{
			tvars[i].arity = V_SCALAR;
			tvars[i].count = 1;
		}

		if (!next_rand_in(0, 7))
			tvars[i].type = T_SINGLE; 
		else if (!next_rand_in(0, 3)) 
			tvars[i].type = (uint8_t)next_rand_in(4, 7);
		else tvars[i].type = (uint8_t)next_rand_in(0, 3);

		tvars[i].modified = 1;
		tvars[i].is_used = 0;
	}

	strcpy(tvars[i].name, "result");
	tvars[i].type = T_UINT64;
	tvars[i].arity = V_SCALAR;
	tvars[i].modified = 0;
	tvars[i].is_used = 0;
	nvars++;

	fprintf(file, "// %s - Randomly-generated mutator function \"%s\"." N N, file_name, name);

	fprintf(file, "#include <stdint.h>" N);
	fprintf(file, "#include <stdlib.h>" N);
	fprintf(file, "#include <inttypes.h>" N);
	fprintf(file, "#include <limits.h>" N N);
	fprintf(file, "#include \"../config.h\"" N N);

	fprintf(file, "exported uint64_t callconv %s(uint64_t value)" N, name);
	fprintf(file, "{" N);
	if (has_vectors)
		fprintf(file, T	"uint32_t i = 0;" N);
	fprintf(file, T		"uint64_t result = value;" N N);

	for (i = 1; i < nvars - 1; ++i) // Make inits.
	{
		if (next_rand_in(0, 3))
			fprintf(file, T	"%s;" N, make_init(&tvars[i], I_RAND));
		else if(next_rand_in(0, 1))
			fprintf(file, T	"%s;" N, make_init(&tvars[i], I_INPT));
		else if (next_rand_in(0, 1))
			fprintf(file, T "%s;" N, make_init(&tvars[i], I_ZERO));
		else fprintf(file, T "%s;" N, make_init(&tvars[i], I_RAND));
	}
	fprintf(file, T N);

	uint8_t tabs = 1;
	uint8_t loops = 0;
	 
	for (i = 0; !are_all_vars_used(tvars, nvars) && i < steps; ++i)
	{
		if (has_vectors && next_rand_in(0, 2) && has_vectors > loops)
		{
			var_t* tmpv[32] = { NULL };
			uint64_t vcnt = 0;
			uint8_t vectat = 0xFF;
			const stmt_t* stmt = NULL;
			
		LOC_TRY_AGAIN:

			stmt = get_next_statement_for_looping(tvars, nvars, tmpv, &vcnt, &vectat);

			if (stmt && vectat != 0xFF)
			{
				if (steps > 0) fprintf(file, T N);
				fprintf(file, T		"for (i = 0; i < %d; ++i)" N, tmpv[vectat]->count);
				fprintf(file, T		"{" N);
				tabs++;

				if (!make_next_loop_statement(file, stmt, tmpv, vcnt, &tabs, vectat))
					goto LOC_TRY_AGAIN;

				uint64_t vectsteps = next_rand_in(3, 8);

				for (j = 0; j < vectsteps; ++j)
				{
					uint8_t currvect = 0xFF;
					stmt = get_next_statement_for_looping(tvars, nvars, tmpv, &vcnt, &currvect);
					if (stmt && vectat == currvect)
					{
						if (!make_next_loop_statement(file, stmt, tmpv, vcnt, &tabs, vectat))
							if (steps > 0) steps--;
					}
					else { if (j > 0) j--; }
				}

				tabs--;
				fprintf(file, T		"}" N N);
				loops++;
			}
			else { if (steps > 0) steps--; }
		}
		else
		{
			if (!make_next_statement(file, tvars, nvars, &tabs, 0xFF))
				if (steps > 0) steps--;
		}
	}

	fprintf(file, T N);
	fprintf(file, T		"return result;" N);
	fprintf(file, "}" N N);

	fflush(file);
	fclose(file);

}

