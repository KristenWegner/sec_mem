; rdr.asm - RDRAND Support


_TEXT SEGMENT

	PUBLIC sm_have_rdrand
    PUBLIC sm_rdrand


; Tests for the presence of the RDRAND instruction.
; uint64_t sm_have_rdrand(void)
sm_have_rdrand PROC
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
sm_have_rdrand ENDP


; Calls the RDRAND instruction, and returns the resultant 64-bit value.
; uint64_t sm_rdrand()
sm_rdrand PROC
REDO:
    rdrand  rax
    ; db    048h, 0Fh, 0C7h, 0F0h
    jnc     REDO
    ret
sm_rdrand ENDP

_TEXT ENDS

END

