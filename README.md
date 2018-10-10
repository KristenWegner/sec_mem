# Secure Memory Manager

This is an experiment in providing a secure memory manager, currently in the *very* early stages.

Utility functions are encrypted and dynamically loaded at random memory locations, decrypted as needed, and then reencrypted when not in use. Data blocks are strictly size-checked, encrypted when not in use, and CRC'd and/or compressed if desired. Secrets can be automatically relocated according to an interrupt.

The ultimate aim of this package is to provide a foundation for read-once, write-only, write-once, and other kinds of transient secrets and to provide an environment and API that will make it difficult for malicious or intrusive code, with real-time access to in-memory sensitive information, to actually get hold of something actionable. It is essentially a shell game with memory blocks and program code.

Perhaps this will also, in the future, integrate some manner of dynamic code polymorphism using primitive blocks that can be rearranged as needed, in order to better obfuscate algorithms, although we may end up getting a little too close to looking like a virus.

Last Update: 2018-10-10
License: This is protected under the MIT License and is Copyright (c) 2018 by Kristen Wegner
