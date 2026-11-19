.code
; int Multiplicacion(long long a, long long b, long long* res)
;   RCX = a, RDX = b, R8 = res
;   EAX: 0 = ok, 1 = desbordamiento, 2 = puntero nulo
Multiplicacion PROC
    test r8, r8
    jz   nulo

    xor  r9d, r9d            ; r9 = 1 si el resultado es negativo
    test rcx, rcx
    jns  a_pos
    neg  rcx
    xor  r9b, 1
a_pos:
    test rdx, rdx
    jns  b_pos
    neg  rdx
    xor  r9b, 1
b_pos:
    xor  rax, rax            ; resultado = 0
ciclo:
    test rdx, rdx            ; ¿quedan bits en el multiplicador?
    jz   fin
    test dl, 1               ; ¿el bit menos significativo es 1?
    jz   no_suma
    add  rax, rcx            ; resultado += multiplicando
    jc   desborde            ; acarreo = no cabe en 64 bits
no_suma:
    shr  rdx, 1              ; siguiente bit del multiplicador
    jz   fin                 ; ya no quedan bits, no hace falta desplazar rcx
    shl  rcx, 1              ; multiplicando *= 2
    jc   desborde
    jmp  ciclo
fin:
    test r9b, r9b
    jz   positivo
    mov  r10, 8000000000000000h
    cmp  rax, r10            ; el negativo maximo es -2^63
    ja   desborde
    neg  rax
    jmp  guardar
positivo:
    test rax, rax
    js   desborde            ; positivo no puede pasar de 2^63 - 1
guardar:
    mov  [r8], rax
    xor  eax, eax
    ret
desborde:
    mov  eax, 1
    ret
nulo:
    mov  eax, 2
    ret
Multiplicacion ENDP
END