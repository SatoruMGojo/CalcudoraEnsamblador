.code
suma PROC
    mov rax, rcx      ; 1er parámetro (RCX)
    add rax, rdx      ; 2do parámetro (RDX)
    ret               ; resultado en RAX
suma ENDP
END