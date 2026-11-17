.code
; int Division(ull dividendo, ull divisor, ull* cociente, ull* residuo)
Division PROC
    test r8, r8             ; puntero cociente nulo?
    jz   nulo
    test r9, r9             ; puntero residuo nulo?
    jz   nulo
    test rdx, rdx           ; divisor == 0 ?
    jz   cero

    mov  rax, rcx
    mov  r10, rdx
    xor  edx, edx
    div  r10
    mov  [r8], rax
    mov  [r9], rdx
    xor  eax, eax
    ret
cero:
    mov  eax, 1
    ret
nulo:
    mov  eax, 2
    ret
Division ENDP
END