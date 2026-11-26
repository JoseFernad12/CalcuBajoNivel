; Evaluate at most one binary operator, with independently signed operands.
; EAX: 0 = incomplete, 1 = result, 2 = error. Output is always terminated.
; Preserve the caller's complete x87 environment, including on error paths.
proc EvaluarExpresion uses ebx esi edi, texto,salida
    locals
        estadoFPU rb 108
        valorA dt ?
        valorB dt ?
        operacion dd ?
    endl
    lea eax,[estadoFPU]
    fnsave [eax]
    mov edi,[salida]
    mov byte [edi],0
    mov esi,[texto]
    xor ecx,ecx
.longitud:
    cmp byte [esi+ecx],0
    je .primero
    inc ecx
    cmp ecx,MAX_ENTRADA
    jbe .longitud
    jmp .rango
.primero:
    stdcall LeerNumero,esi
    cmp eax,1
    jne .estado_numero
    fstp tword [valorA]
    movzx ebx,byte [edx]
    test ebx,ebx
    jnz .operador
    ; A plain first operand is not yet an a op b expression.
    test ecx,ecx
    jz .incompleta
    fld tword [valorA]
    jmp .formato
.operador:
    cmp bl,'+'
    je .segundo
    cmp bl,'-'
    je .segundo
    cmp bl,'X'
    je .segundo
    cmp bl,'/'
    jne .sintaxis
.segundo:
    mov [operacion],ebx
    inc edx
    stdcall LeerNumero,edx
    cmp eax,1
    jne .estado_numero
    fstp tword [valorB]
    cmp byte [edx],0
    jne .sintaxis
    cmp [operacion],'/'
    jne .calcular
    fld tword [valorB]
    ftst
    fnstsw ax
    sahf
    fstp st0
    je .division_cero
.calcular:
    fld tword [valorA]
    fld tword [valorB]
    cmp [operacion],'+'
    je .sumar
    cmp [operacion],'-'
    je .restar
    cmp [operacion],'X'
    je .multiplicar
    fdivp st1,st0
    jmp .formato
.sumar:
    faddp st1,st0
    jmp .formato
.restar:
    fsubp st1,st0
    jmp .formato
.multiplicar:
    fmulp st1,st0
.formato:
    stdcall FormatearResultado,[salida]
    jmp .fin
.estado_numero:
    test eax,eax
    jz .incompleta
.sintaxis:
    mov esi,ErrorExpresion
    jmp .error
.division_cero:
    mov esi,ErrorDivision
    jmp .error
.rango:
    mov esi,ErrorRango
.error:
    mov edi,[salida]
.copiar_error:
    lodsb
    stosb
    test al,al
    jnz .copiar_error
    mov eax,2
    jmp .fin
.incompleta:
    xor eax,eax
.fin:
    lea edx,[estadoFPU]
    frstor [edx]
    ret
endp

; Parse [-]digits[.digits][%]. ST0 contains the value on success.
; EDX points to the delimiter; ECX indicates a percent suffix.
proc LeerNumero uses ebx esi edi, texto
    locals
        digito dd ?
        escala dd ?
        negativo dd ?
        decimal dd ?
        ultimoDigito dd ?
    endl
    mov esi,[texto]
    mov [escala],1
    mov [negativo],0
    mov [decimal],0
    mov [ultimoDigito],0
    xor edi,edi
    xor ecx,ecx
    fldz
    cmp byte [esi],'-'
    jne .leer
    mov [negativo],1
    inc esi
.leer:
    movzx eax,byte [esi]
    cmp al,'0'
    jb .no_digito
    cmp al,'9'
    ja .no_digito
    sub eax,'0'
    mov [digito],eax
    fimul dword [CalcDiez]
    fiadd dword [digito]
    inc edi
    mov [ultimoDigito],1
    cmp [decimal],0
    je .siguiente
    imul eax,[escala],10
    mov [escala],eax
.siguiente:
    inc esi
    jmp .leer
.no_digito:
    cmp al,'.'
    jne .terminar
    cmp [decimal],0
    jne .invalido
    test edi,edi
    jz .invalido
    mov [decimal],1
    mov [ultimoDigito],0
    jmp .siguiente
.terminar:
    cmp [ultimoDigito],0
    jne .valor
    test al,al
    jnz .invalido
    xor eax,eax
    ret
.valor:
    fidiv dword [escala]
    cmp [negativo],0
    je .porcentaje
    fchs
.porcentaje:
    cmp byte [esi],'%'
    jne .delimitador
    fidiv dword [CalcCien]
    mov ecx,1
    inc esi
.delimitador:
    mov al,[esi]
    test al,al
    jz .valido
    cmp al,'+'
    je .valido
    cmp al,'-'
    je .valido
    cmp al,'X'
    je .valido
    cmp al,'/'
    jne .invalido
.valido:
    mov edx,esi
    mov eax,1
    ret
.invalido:
    mov eax,2
    ret
endp

; Round magnitude to cents, halves away from zero, then trim trailing zeros.
; All arithmetic above is x87; integer division here only prints decimal digits.
proc FormatearResultado uses ebx esi edi, salida
    locals
        centavos dq ?
        parteAlta dd ?
        fraccion dd ?
        negativo dd ?
        temporal rb 24
    endl
    mov [negativo],0
    ftst
    fnstsw ax
    sahf
    jp .rango
    jae .magnitud
    mov [negativo],1
.magnitud:
    fabs
    fcom qword [CalcLimite]
    fnstsw ax
    sahf
    jae .rango
    fimul dword [CalcCien]
    fld st0
    fmul qword [CalcRoundRel]
    fadd qword [CalcRoundAbs]
    faddp st1,st0
    fadd qword [CalcMedio]
    fldcw [CalcTruncar]
    fistp qword [centavos]
    mov eax,dword [centavos]
    mov edx,dword [centavos+4]
    mov ecx,eax
    or ecx,edx
    jnz .dividir_cien
    mov [negativo],0
.dividir_cien:
    mov ebx,100
    mov ecx,eax
    mov eax,edx
    xor edx,edx
    div ebx
    mov [parteAlta],eax
    mov eax,ecx
    div ebx
    mov [fraccion],edx
    mov edx,[parteAlta]
    lea esi,[temporal]
    xor edi,edi
    mov ebx,10
.digitos:
    mov ecx,eax
    mov eax,edx
    xor edx,edx
    div ebx
    mov [parteAlta],eax
    mov eax,ecx
    div ebx
    add dl,'0'
    mov [esi+edi],dl
    inc edi
    mov edx,[parteAlta]
    mov ecx,eax
    or ecx,edx
    jnz .digitos
    mov ecx,edi
    mov edi,[salida]
    cmp [negativo],0
    je .entero
    mov byte [edi],'-'
    inc edi
.entero:
    dec ecx
    mov al,[esi+ecx]
    stosb
    test ecx,ecx
    jnz .entero
    mov eax,[fraccion]
    test eax,eax
    jz .terminar
    mov byte [edi],'.'
    inc edi
    xor edx,edx
    div ebx
    add al,'0'
    stosb
    test edx,edx
    jz .terminar
    mov al,dl
    add al,'0'
    stosb
.terminar:
    mov byte [edi],0
    sub edi,[salida]
    cmp edi,MAX_ENTRADA
    ja .rango
    mov eax,1
    ret
.rango:
    mov esi,ErrorRango
    mov edi,[salida]
.copiar:
    lodsb
    stosb
    test al,al
    jnz .copiar
    mov eax,2
    ret
endp
