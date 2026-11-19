; Modifica una cadena de hasta 127 caracteres. EAX=1 si cambio.
; Sin estado decimal oculto: borrar y AC siempre permiten reintroducirlo.
proc AgregarEntrada uses ebx esi edi, texto,caracter
    mov esi,[texto]
    xor ebx,ebx
.longitud:
    cmp byte [esi+ebx],0
    je .clasificar
    inc ebx
    cmp ebx,128
    jb .longitud
    jmp .rechazar
.clasificar:
    mov edx,[caracter]
    cmp dl,'0'
    jb .no_digito
    cmp dl,'9'
    jbe .digito
.no_digito:
    cmp dl,'.'
    je .decimal
    cmp dl,'%'
    je .porcentaje
    cmp dl,'+'
    je .operador
    cmp dl,'-'
    je .operador
    cmp dl,'X'
    je .operador
    cmp dl,'/'
    je .operador
    jmp .rechazar
.digito:
    test ebx,ebx
    jz .anexar
    cmp byte [esi+ebx-1],'%'
    je .rechazar
    ; Evitar 0002; conservar ceros despues del separador.
    cmp byte [esi+ebx-1],'0'
    jne .anexar
    cmp ebx,1
    je .reemplazar
    mov al,[esi+ebx-2]
    cmp al,'+'
    je .reemplazar
    cmp al,'-'
    je .reemplazar
    cmp al,'X'
    je .reemplazar
    cmp al,'/'
    je .reemplazar
    jmp .anexar
.decimal:
    ; Un solo punto decimal por numero.
    xor ecx,ecx
    xor edi,edi
.scan_decimal:
    cmp ecx,ebx
    je .fin_scan
    mov al,[esi+ecx]
    cmp al,'.'
    je .separador
    cmp al,'+'
    je .nuevo_numero
    cmp al,'-'
    je .nuevo_numero
    cmp al,'X'
    je .nuevo_numero
    cmp al,'/'
    jne .siguiente
.nuevo_numero:
    xor edi,edi
    jmp .siguiente
.separador:
    cmp al,dl
    jne .rechazar
    mov edi,1
.siguiente:
    inc ecx
    jmp .scan_decimal
.fin_scan:
    test edi,edi
    jnz .rechazar
    test ebx,ebx
    jz .cero_decimal
    mov al,[esi+ebx-1]
    cmp al,'%'
    je .rechazar
    cmp al,'0'
    jb .cero_decimal
    cmp al,'9'
    jbe .anexar
.cero_decimal:
    cmp ebx,125
    ja .rechazar
    mov byte [esi+ebx],'0'
    inc ebx
    jmp .anexar
.porcentaje:
    test ebx,ebx
    jz .rechazar
    mov al,[esi+ebx-1]
    cmp al,'0'
    jb .rechazar
    cmp al,'9'
    ja .rechazar
    jmp .anexar
.operador:
    test ebx,ebx
    jnz .ultimo
    cmp dl,'-'
    je .anexar
    jmp .rechazar
.ultimo:
    mov al,[esi+ebx-1]
    cmp al,'.'
    je .rechazar
    cmp al,'+'
    je .cambiar_operador
    cmp al,'-'
    je .cambiar_operador
    cmp al,'X'
    je .cambiar_operador
    cmp al,'/'
    jne .anexar
.cambiar_operador:
    cmp ebx,1
    je .rechazar
.reemplazar:
    mov [esi+ebx-1],dl
    jmp .aceptar
.anexar:
    cmp ebx,127
    jae .rechazar
    mov [esi+ebx],dl
    mov byte [esi+ebx+1],0
.aceptar:
    mov eax,1
    ret
.rechazar:
    xor eax,eax
    ret
endp
