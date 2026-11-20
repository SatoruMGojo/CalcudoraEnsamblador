MAX_DEC EQU 4                    ; límite estándar de decimales (operandos y resultado)

.data
pow10 QWORD 1, 10, 100, 1000, 10000   ; 10^0 ... 10^MAX_DEC

.code
; int DivisionDec(ll dividendo, ll divisor, ll* cociente, ll* residuo,
;                 char* digitos, ull n, ull decDividendo, ull decDivisor,
;                 int* negativo)
; negativo: se escribe 1 si el resultado es negativo, 0 si no
; retorna: 0 = OK, 1 = división entre cero, 2 = puntero nulo,
;          3 = desbordamiento (al escalar o el resultado no cabe con signo),
;          4 = decimales o n fuera de rango (> MAX_DEC)
DivisionDec PROC
    push rsi                     ; rsi = 1 si el dividendo es negativo
    push rdi                     ; rdi = 1 si el cociente es negativo
    ; Con los 2 push los parámetros de pila se desplazan 16 bytes:
    ; digitos [rsp+56], n [rsp+64], decDividendo [rsp+72],
    ; decDivisor [rsp+80], negativo [rsp+88]

    test r8, r8                  ; puntero cociente nulo?
    jz   nulo
    test r9, r9                  ; puntero residuo nulo?
    jz   nulo
    cmp  qword ptr [rsp+56], 0   ; buffer de dígitos nulo?
    je   nulo
    cmp  qword ptr [rsp+88], 0   ; puntero negativo nulo?
    je   nulo
    test rdx, rdx                ; divisor == 0 ?
    jz   cero

    ; --- guardar signos y pasar a valor absoluto ---
    xor  esi, esi                ; dividendo positivo
    xor  edi, edi                ; cociente positivo
    test rcx, rcx
    jns  dvdoPos
    neg  rcx                     ; |dividendo|
    mov  esi, 1                  ; dividendo negativo
    mov  edi, 1                  ; cociente negativo (por ahora)
dvdoPos:
    test rdx, rdx
    jns  dvsrPos
    neg  rdx                     ; |divisor|
    xor  edi, 1                  ; invierte el signo del cociente
dvsrPos:

    mov  r10, rdx                ; r10 = divisor (libera rdx para mul)
    mov  rax, [rsp+72]           ; rax = decDividendo
    mov  r11, [rsp+80]           ; r11 = decDivisor

    ; --- validar rango (todo <= MAX_DEC) ---
    cmp  rax, MAX_DEC
    ja   rango
    cmp  r11, MAX_DEC
    ja   rango
    cmp  qword ptr [rsp+64], MAX_DEC   ; n <= MAX_DEC
    ja   rango

    ; --- igualar escalas ---
    cmp  rax, r11
    je   listo
    ja   escDivisor

    ; decDividendo < decDivisor: dividendo *= 10^(r11 - rax)
    sub  r11, rax
    lea  rdx, pow10
    mov  r11, [rdx + r11*8]      ; r11 = multiplicador
    mov  rax, rcx
    mul  r11                     ; rdx:rax = dividendo * multiplicador
    test rdx, rdx                ; rdx != 0 -> no cupo en 64 bits
    jnz  desborde
    mov  rcx, rax
    jmp  listo

escDivisor:                      ; decDividendo > decDivisor: divisor *= 10^(rax - r11)
    sub  rax, r11
    lea  rdx, pow10
    mov  rax, [rdx + rax*8]      ; rax = multiplicador
    mul  r10                     ; rdx:rax = divisor * multiplicador
    test rdx, rdx
    jnz  desborde
    mov  r10, rax

listo:
    ; el divisor escalado debe caber en 63 bits para que el residuo
    ; también quepa como número con signo
    test r10, r10
    js   desborde

    ; --- parte entera (con valores absolutos) ---
    mov  rax, rcx
    xor  edx, edx
    div  r10                     ; rax = cociente, rdx = residuo
    test rax, rax                ; ¿el cociente no cabe con signo (bit 63)?
    js   desborde

    test edi, edi                ; aplicar signo al cociente
    jz   cocPos
    neg  rax
cocPos:
    mov  [r8], rax               ; neg no modifica rdx (residuo intacto)

    ; --- parte decimal (siempre del valor absoluto) ---
    mov  r11, [rsp+56]           ; buffer
    mov  rcx, [rsp+64]           ; n dígitos (0..MAX_DEC)
    test rcx, rcx
    jz   fin
digito:
    mov  rax, rdx
    mov  edx, 10
    mul  rdx                     ; rdx:rax = residuo * 10
    div  r10                     ; rax = dígito, rdx = nuevo residuo
    add  al, '0'
    mov  [r11], al
    inc  r11
    dec  rcx
    jnz  digito

fin:
    test esi, esi                ; residuo con el signo del dividendo
    jz   resPos
    neg  rdx
resPos:
    mov  [r9], rdx
    mov  rcx, [rsp+88]
    mov  dword ptr [rcx], edi    ; indicador de resultado negativo
    xor  eax, eax
    jmp  salir

cero:
    mov  eax, 1
    jmp  salir
nulo:
    mov  eax, 2
    jmp  salir
desborde:
    mov  eax, 3
    jmp  salir
rango:
    mov  eax, 4
salir:
    pop  rdi
    pop  rsi
    ret
DivisionDec ENDP
END