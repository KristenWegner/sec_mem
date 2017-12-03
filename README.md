# sec_mem

An experiment in providing a secure memory manager. Currently in the early stages.

Utility functions are encrypted and dynamically loaded at random memory locations and decrypted as needed.

Data blocks are strictly size-checked, encrypted when not in use, and CRC'd and/or compressed if desired.

Essentially this is a shell game with memory blocks and program code.

The intention is to provide an environment that will make it difficult for malicious code to access sensitive information.


