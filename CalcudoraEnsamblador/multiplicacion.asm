.code
PUBLIC multiplicacion          ; hace visible el símbolo para el linker (C++)

multiplicacion PROC
    xor rax, rax               ; rax = 0 -> acumulador del resultado

ciclo:
    test rdx, rdx              ; ¿rdx == 0? (AND sin guardar; solo ajusta flags)
    jz ok                      ; si no quedan bits en el multiplicador, terminamos bien

    test rdx, 1                ; revisa el bit menos significativo (bit 0) de rdx
    jz no_suma                 ; si el bit es 0, no se suma nada en esta vuelta

    add rax, rcx               ; bit = 1 -> resultado += multiplicando
    jc overflow                ; CF=1: la suma no cupo en 64 bits -> overflow

no_suma:
    shl rcx, 1                 ; multiplicando *= 2; el bit que sale va a CF
    jnc sigue                  ; CF=0: no se perdió nada, seguimos normal

    cmp rdx, 1                 ; se perdió un bit: ¿quedan más bits en el multiplicador?
    ja overflow                ; rdx > 1: sí quedan, y usarían un rcx truncado -> overflow
                               ; (si rdx == 1 era el último bit, la pérdida no importa)

sigue:
    shr rdx, 1                 ; multiplicador /= 2 (baja al siguiente bit)
    jmp ciclo                  ; repite

ok:
    mov [r8], rax              ; *resultado = rax (escribe por el puntero)
    mov eax, 1                 ; return true
    ret

overflow:
    xor eax, eax               ; return false (no se toca *resultado)
    ret
multiplicacion ENDP
END
