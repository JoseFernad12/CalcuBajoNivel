; Input capacity is MAX_ENTRADA characters plus the null terminator.
; Return EAX=1 when accepted; rejected input leaves the buffer unchanged.
; Sin estado decimal oculto: borrar y AC siempre permiten reintroducirlo.
proc AgregarEntrada uses ebx esi edi, texto,caracter
    locals
        indiceOperador dd ?
    endl
    mov esi,[texto]
    xor ebx,ebx
.longitud:
    cmp byte [esi+ebx],0
    je .clasificar
    inc ebx
    cmp ebx,MAX_ENTRADA+1
    jb .longitud
    jmp .rechazar
.clasificar:
    ; Skip the initial minus sign and find the single binary operator.
    mov [indiceOperador],-1
    mov ecx,1
.buscar_operador:
    cmp ecx,ebx
    jae .caracter
    mov al,[esi+ecx]
    cmp al,'+'
    je .encontrado
    cmp al,'-'
    je .encontrado
    cmp al,'X'
    je .encontrado
    cmp al,'/'
    je .encontrado
    inc ecx
    jmp .buscar_operador
.encontrado:
    mov [indiceOperador],ecx
.caracter:
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
    ; A decimal point must leave room for at least one following digit.
    cmp ebx,MAX_ENTRADA-1
    jae .rechazar
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
    ; Insert "0." atomically and reserve space for its fractional digit.
    cmp ebx,MAX_ENTRADA-3
    ja .rechazar
    mov byte [esi+ebx],'0'
    inc ebx
    jmp .anexar
.porcentaje:
    ; The last available position is reserved for a digit.
    cmp ebx,MAX_ENTRADA-1
    jae .rechazar
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
    cmp byte [esi+ebx-1],'.'
    je .rechazar
    mov ecx,[indiceOperador]
    cmp ecx,-1
    jne .operador_existente
    ; An initial sign alone is not a complete first operand.
    cmp ebx,1
    jne .anexar_operador
    cmp byte [esi],'-'
    je .rechazar
    jmp .anexar_operador
.operador_existente:
    inc ecx
    cmp ecx,ebx
    jne .signo_pendiente
    ; Minus after a binary operator starts a negative second operand.
    cmp dl,'-'
    je .anexar_operador
    jmp .reemplazar
.signo_pendiente:
    inc ecx
    cmp ecx,ebx
    jne .rechazar
    cmp byte [esi+ebx-1],'-'
    jne .rechazar
    cmp dl,'-'
    je .rechazar
    mov ecx,[indiceOperador]
    mov [esi+ecx],dl
    mov byte [esi+ecx+1],0
    jmp .aceptar
.reemplazar:
    mov [esi+ebx-1],dl
    jmp .aceptar
.anexar_operador:
    ; Replacing an operator is allowed; appending one needs two slots.
    cmp ebx,MAX_ENTRADA-1
    jae .rechazar
.anexar:
    cmp ebx,MAX_ENTRADA
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
