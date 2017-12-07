#define SEC_OP_HRDRND64 (0x73CAU) // Function: Tests for rdrand support: sec_g64_f.
#define SEC_OP_RDRAND64 (0x50C7U) // Function: Read rand via rdrand: sec_g64_f.
#define SEC_OP_SHRGML64 (0x5282U) // Function: Multiply using Schrage's method: sec_sch_t;
#define SEC_OP_FS20SS64 (0x6852U) // Size: Fishman-20 64 RNG state size: uint64_t.
#define SEC_OP_FS20SD64 (0xCF73U) // Function: Fishman-20 64 RNG seed: sec_srs_f.
#define SEC_OP_FS20RG64 (0xD564U) // Function: Fishman-20 64 RNG generate: sec_r64_f.
#define SEC_OP_KN02SS64 (0xAF89U) // Size: Knuth 2002 64 with Random Bit Shift RNG state size: uint64_t.
#define SEC_OP_KN02SD64 (0xDEDFU) // Function: Knuth 2002 64 with Random Bit Shift RNG seed: sec_srs_f.
#define SEC_OP_KN02RG64 (0x24ABU) // Function: Knuth 2002 64 with Random Bit Shift RNG generate: sec_r64_f.
#define SEC_OP_LECUSS32 (0x2D90U) // Size: L'Ecuyer 32 RNG state size: uint64_t.
#define SEC_OP_LECUSR32 (0x46BDU) // Function: L'Ecuyer 32 RND seed: sec_srs_f.
#define SEC_OP_LECURG32 (0x36C6U) // Function: L'Ecuyer 32 RNG generate: sec_r32_f.
#define SEC_OP_GFSRSS32 (0x3AD8U) // Size: GFSR4 32 RNG state size: uint64_t.
#define SEC_OP_GFSRSR32 (0xF01DU) // Function: GFSR4 32 RND seed: sec_srs_f.
#define SEC_OP_GFSRRG32 (0x01B5U) // Function: GFSR4 32 RNG generate: sec_r32_f.
#define SEC_OP_SPMXSS64 (0x787EU) // Size: Split Mix 64 RNG state size: uint64_t.
#define SEC_OP_SPMXSR64 (0xCA73U) // Function: Split Mix 64 RNG seed: sec_srs_f.
#define SEC_OP_SPMXRG64 (0x62B7U) // Function: Split Mix 64 RNG generate: sec_r64_f.
#define SEC_OP_XSHISS64 (0x452AU) // Size: Xoroshiro128+ 64 RNG state size: uint64_t.
#define SEC_OP_XSHISR64 (0x49C2U) // Function: Xoroshiro128+ 64 RNG seed: sec_srs_f.
#define SEC_OP_XSHIRG64 (0xEADFU) // Function: Xoroshiro128+ 64 RNG generate: sec_r64_f.
#define SEC_OP_XSFSSS64 (0xDD9FU) // Size: XorShift1024* 64 RNG state size: uint64_t.
#define SEC_OP_XSFSSR64 (0x699FU) // Function: XorShift1024* 64 RNG seed: sec_srs_f.
#define SEC_OP_XSFSRG64 (0xA72CU) // Function: XorShift1024* 64 RNG generate: sec_r64_f.
#define SEC_OP_MERSSS64 (0x3213U) // Size: Mersenne Twister 19937 64 RNG state size: uint64_t.
#define SEC_OP_MERSSR64 (0x0214U) // Function: Mersenne Twister 19937 64 RNG seed: sec_srs_f.
#define SEC_OP_MERSRG64 (0xDDFAU) // Function: Mersenne Twister 19937 64 RNG generate: sec_r64_f.
#define SEC_OP_RD48SS32 (0xD696U) // Size: Rand 48 32 RND state size: uint64_t.
#define SEC_OP_RANDSR48 (0x59ADU) // Function: Rand 48 32 RND seed: sec_srs_f.
#define SEC_OP_RANDRG48 (0x7542U) // Function: Rand 48 32 RND generate: sec_r32_f.
#define SEC_OP_COMPHTAB (0x9355U) // Size: Hash table size needed by COMPRESS: uint64_t.
#define SEC_OP_COMPRESS (0x3D77U) // Function: Compress data: sec_cpr_f. Needs pointer to buffer of size COMPHTAB.
#define SEC_OP_DECOMPRS (0x2E25U) // Function: Decompress data: sec_dcp_f.
#define SEC_OP_CRC32TAB (0xC999U) // Data: CRC 32-Bit LUT Used by CRC32FUN.
#define SEC_OP_CRC32FUN (0x2D7BU) // Function: CRC 32: sec_c32_f. Needs pointer to LUT referenced by CRC32TAB.
#define SEC_OP_CRC64TAB (0xA2C1U) // Data: CRC 64-Bit LUT Used by CRC64FUN.
#define SEC_OP_CRC64FUN (0x4615U) // Function: CRC 64: sec_c64_f. Needs pointer to LUT referenced by CRC64TAB.
#define SEC_OP_MEMXORRG (0x51E9U) // Function: Memory XOR given RNG function and state: sec_mxr_f.
#define SEC_OP_HCITYB64 (0xB618U) // Function: City Hash 64-Bit, up to 8 bytes: sec_h64_f.
#define SEC_OP_HMURMU32 (0xFD56U) // Function: Murmur Hash 32-Bit: sec_h32_f.
#define SEC_OP_HMURMU64 (0x7A11U) // Function: Murmur Hash 64-Bit: sec_h64_f.
