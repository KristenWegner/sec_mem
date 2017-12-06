#define SEC_OP_HRDRND64 (0x8022U) // Function: Tests for rdrand support: sec_g64_f.
#define SEC_OP_RDRAND64 (0x8EE5U) // Function: Read rand via rdrand: sec_g64_f.
#define SEC_OP_SHRGML64 (0xF99EU) // Function: Multiply using Schrage's method: sec_sch_t;
#define SEC_OP_FS20SS64 (0x5D78U) // Size: Fishman-20 64 RNG state size: uint64_t.
#define SEC_OP_FS20SD64 (0x1440U) // Function: Fishman-20 64 RNG seed: sec_srs_f.
#define SEC_OP_FS20RG64 (0xE5D5U) // Function: Fishman-20 64 RNG generate: sec_r64_f.
#define SEC_OP_KN02SS64 (0xE29DU) // Size: Knuth 2002 64 with Random Bit Shift RNG state size: uint64_t.
#define SEC_OP_KN02SD64 (0xCAE4U) // Function: Knuth 2002 64 with Random Bit Shift RNG seed: sec_srs_f.
#define SEC_OP_KN02RG64 (0xCC09U) // Function: Knuth 2002 64 with Random Bit Shift RNG generate: sec_r64_f.
#define SEC_OP_LECUSS32 (0x8E1BU) // Size: L'Ecuyer 32 RNG state size: uint64_t.
#define SEC_OP_LECUSR32 (0xBF8AU) // Function: L'Ecuyer 32 RND seed: sec_srs_f.
#define SEC_OP_LECURG32 (0x9C34U) // Function: L'Ecuyer 32 RNG generate: sec_r32_f.
#define SEC_OP_GFSRSS32 (0xA0DCU) // Size: GFSR4 32 RNG state size: uint64_t.
#define SEC_OP_GFSRSR32 (0x39FBU) // Function: GFSR4 32 RND seed: sec_srs_f.
#define SEC_OP_GFSRRG32 (0x56F3U) // Function: GFSR4 32 RNG generate: sec_r32_f.
#define SEC_OP_SPMXSS64 (0x216FU) // Size: Split Mix 64 RNG state size: uint64_t.
#define SEC_OP_SPMXSR64 (0x0990U) // Function: Split Mix 64 RNG seed: sec_srs_f.
#define SEC_OP_SPMXRG64 (0xA530U) // Function: Split Mix 64 RNG generate: sec_r64_f.
#define SEC_OP_XSHISS64 (0xA077U) // Size: Xoroshiro128+ 64 RNG state size: uint64_t.
#define SEC_OP_XSHISR64 (0xA834U) // Function: Xoroshiro128+ 64 RNG seed: sec_srs_f.
#define SEC_OP_XSHIRG64 (0x7EF8U) // Function: Xoroshiro128+ 64 RNG generate: sec_r64_f.
#define SEC_OP_XSFSSS64 (0x230DU) // Size: XorShift1024* 64 RNG state size: uint64_t.
#define SEC_OP_XSFSSR64 (0x7C38U) // Function: XorShift1024* 64 RNG seed: sec_srs_f.
#define SEC_OP_XSFSRG64 (0xF5E3U) // Function: XorShift1024* 64 RNG generate: sec_r64_f.
#define SEC_OP_MERSSS64 (0x3D40U) // Size: Mersenne Twister 19937 64 RNG state size: uint64_t.
#define SEC_OP_MERSSR64 (0x7A99U) // Function: Mersenne Twister 19937 64 RNG seed: sec_srs_f.
#define SEC_OP_MERSRG64 (0xDBDDU) // Function: Mersenne Twister 19937 64 RNG generate: sec_r64_f.
#define SEC_OP_RD48SS32 (0x024DU) // Size: Rand 48 32 RND state size: uint64_t.
#define SEC_OP_RANDSR48 (0x1D4DU) // Function: Rand 48 32 RND seed: sec_srs_f.
#define SEC_OP_RANDRG48 (0x3778U) // Function: Rand 48 32 RND generate: sec_r32_f.
#define SEC_OP_COMPHTAB (0x0DE8U) // Size: Hash table size needed by COMPRESS: uint64_t.
#define SEC_OP_COMPRESS (0xE5F6U) // Function: Compress data: sec_cpr_f. Needs pointer to buffer of size COMPHTAB.
#define SEC_OP_DECOMPRS (0x9BB5U) // Function: Decompress data: sec_dcp_f.
#define SEC_OP_CRC32TAB (0x4A9AU) // Data: CRC 32-Bit LUT Used by CRC32FUN.
#define SEC_OP_CRC32FUN (0x0B46U) // Function: CRC 32: sec_c32_f. Needs pointer to LUT referenced by CRC32TAB.
#define SEC_OP_CRC64TAB (0xA278U) // Data: CRC 64-Bit LUT Used by CRC64FUN.
#define SEC_OP_CRC64FUN (0x4B45U) // Function: CRC 64: sec_c64_f. Needs pointer to LUT referenced by CRC64TAB.
#define SEC_OP_MEMCPYFN (0x58E1U) // Function: Same as memcpy: sec_cpy_f.
#define SEC_OP_MEMMEMFN (0xBEF1U) // Function: Same as memmem: sec_mem_f.
#define SEC_OP_MEMXORRG (0x6E4FU) // Function: Memory XOR given RNG function and state: sec_mxr_f.
#define SEC_OP_HCITYB64 (0x84C9U) // Function: City Hash 64-Bit, up to 8 bytes: sec_h64_f.
#define SEC_OP_HMURMU32 (0x1EF2U) // Function: Murmur Hash 32-Bit: sec_h32_f.
#define SEC_OP_HMURMU64 (0x855BU) // Function: Murmur Hash 64-Bit: sec_h64_f.
