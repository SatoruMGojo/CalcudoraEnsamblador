.code
multiplicacion PROC
    xor rax, rax         ; resultado = 0
ciclo:
    test rdx, rdx        ; ¿quedan bits en el multiplicador?
    jz fin
    test rdx, 1          ; ¿el bit menos significativo es 1?
    jz no_suma
    add rax, rcx         ; resultado += multiplicando
no_suma:
    shl rcx, 1           ; multiplicando *= 2
    shr rdx, 1           ; pasa al siguiente bit del multiplicador
    jmp ciclo
fin:
    ret
multiplicacion ENDP
END
