_TEXT SEGMENT


    PUBLIC sec_rdrand_step_32
    PUBLIC sec_rdrand_retry_32
    PUBLIC sec_rdrand_step_64
    PUBLIC sec_rdrand_retry_64
	PUBLIC sec_hrdrnd


; uint64_t sec_hrdrnd(void)
sec_hrdrnd PROC
        push rbx
        mov eax, 1
        cpuid
        bt ecx, 30
        jnc failed
done:
        mov rax, 1
        pop rbx
        ret
failed:
        sub rax, rax
        jmp short done
sec_hrdrnd ENDP


; int32_t sec_rdrand_step_32(uint32_t*)
sec_rdrand_step_32 PROC
    xor     eax, eax
    rdrand  edx
    ; db    0fh, 0c7h, 0f2h
    setc    al
    mov     [rcx], edx
    ret
sec_rdrand_step_32 ENDP


; uint32_t sec_rdrand_retry_32()
sec_rdrand_retry_32 PROC
again:
    rdrand  eax
    ; db    0fh, 0c7h, 0f0h
    jnc     again
    ret
sec_rdrand_retry_32 ENDP


; int32_t sec_rdrand_step_64(uint64_t*)
sec_rdrand_step_64 PROC
    xor     eax, eax
    rdrand  rdx
    ; db    048h, 0fh, 0c7h, 0f2h
    setc    al
    mov     [rcx], edx
    ret
sec_rdrand_step_64 ENDP


; uint64_t sec_rdrand_retry_64()
sec_rdrand_retry_64 PROC
again:
    rdrand  rax
    ; db    048h, 0fh, 0c7h, 0f0h
    jnc     again
    ret
sec_rdrand_retry_64 ENDP


_TEXT ENDS


END

