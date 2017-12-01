// embedded.h - Embedded function access.

#include "config.h"

#ifndef INCLUDE_EMBEDDED_H
#define INCLUDE_EMBEDDED_H 1

// Opcodes for embedded/obfuscated functions.

#define SEC_OP_HRDRND (0x4300U) // Tests for rdrand support: uint64_t (*)();
#define SEC_OP_RDRAND (0x9ADFU) // Read rand via rdrand: uint64_t (*)();
#define SEC_OP_SHRGML (0xF661U) // Multiply using Schrage's method: uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t);
#define SEC_OP_FS20SD (0x1888U) // Fishman-20 RNG seed: void (*)(void*, uint64_t); State is 8-bytes.
#define SEC_OP_FS20RG (0xBE56U) // Fishman-20 RNG rand: uint64_t (*)(void*); State is 8-bytes.
#define SEC_OP_KN02SD (0x046BU) // Knuth 2002 RNG seed: void (*)(void*, uint64_t); State is 0x22AC-bytes.
#define SEC_OP_KN02RG (0x7BC6U) // Knuth 2002 RNG rand: uint64_t (*)(void*); State is 0x22AC-bytes.

// Gets the specified operation, or zero.
uint64_t sec_get_op(uint16_t);

#endif // INCLUDE_EMBEDDED_H

