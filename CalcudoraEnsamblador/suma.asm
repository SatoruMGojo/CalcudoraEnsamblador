.code
; int Suma(long long a, long long b, long long* res)
;   RCX = a, RDX = b, R8 = res
;   EAX: 0 = ok, 1 = desbordamiento, 2 = puntero nulo
Suma PROC
    test r8, r8
    jz   nulo
    mov  rax, rcx
    add  rax, rdx
    jo   desborde          ; overflow con signo
    mov  [r8], rax
    xor  eax, eax
    ret
desborde:
    mov  eax, 1
    ret
nulo:
    mov  eax, 2
    ret
Suma ENDP
END