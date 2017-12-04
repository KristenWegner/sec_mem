; rdr.asm - RDRAND Support

_TEXT SEGMENT

	PUBLIC HRDRND64
    PUBLIC RDRAND64

; Tests for the presence of the RDRAND instruction.
; uint64_t HRDRND64(void)
HRDRND64 PROC
    push    rbx
    mov     eax, 1
    cpuid
    bt      ecx, 30
    jnc     FAIL
DONE:
    mov     rax, 1
    pop     rbx
    ret
FAIL:
    sub     rax, rax
    jmp     short DONE
HRDRND64 ENDP

; Calls the RDRAND instruction, and returns the resultant 64-bit value.
; uint64_t RDRAND64()
RDRAND64 PROC
REDO:
    rdrand  rax
    ; db    048h, 0Fh, 0C7h, 0F0h
    jnc     REDO
    ret
RDRAND64 ENDP

_TEXT ENDS

END
