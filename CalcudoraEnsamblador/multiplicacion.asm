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
MAX_DEC EQU 4      ; mismo limite que usa division.asm

; int MultiplicacionDec(ll a, ll b, ll* res, ull decA, ull decB, ull* decRes)
;   RCX = a, RDX = b, R8 = res, R9 = decA
;   [rsp+40] = decB, [rsp+48] = decRes
;   EAX: 0 = ok, 1 = desbordamiento, 2 = puntero nulo, 4 = decimales fuera de rango
; Ejemplo: 1.5 * 2.25 -> a=15, decA=1, b=225, decB=2 -> res=3375, decRes=3 (3.375)
MultiplicacionDec PROC
    mov  r10, [rsp+40]        ; r10 = decB
    mov  r11, [rsp+48]        ; r11 = puntero decRes
    test r11, r11
    jz   nuloD
    cmp  r9, MAX_DEC
    ja   rangoD
    cmp  r10, MAX_DEC
    ja   rangoD
    add  r9, r10              ; decimales del resultado = decA + decB
    mov  [r11], r9
    jmp  Multiplicacion       ; RCX, RDX y R8 siguen intactos: reutiliza el ciclo
nuloD:
    mov  eax, 2
    ret
rangoD:
    mov  eax, 4
    ret
MultiplicacionDec ENDP
END