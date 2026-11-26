; operadores.inc
; Logica de evaluacion de la calculadora.
; Soporta: 1 operador binario (+ - X /), negativos (-1-3, -1*-2, 5+-3),
;          punto decimal (2 digitos max), % postfix.
; Escalado: enteros con 2 decimales fijos (×100).

.data

strError     db 'Error',0
bufferCalc   rb 128
resultStr    db 40 dup(0)
parseResult  dd 0
numLeft      dd 0
numSigno     dd 0
theOp        db 0

.code

; =========================================================
; Punto de entrada desde ProcesarComando (al presionar '=')
; =========================================================
proc ResolverOperacion
    push ebx
    push esi
    push edi

    invoke GetWindowText,[hEditOp],bufferCalc,127
    cmp eax,0
    je .error

    ; Solo "-" no es valido
    cmp eax,1
    jne .eval
    cmp byte [bufferCalc],'-'
    je .error

.eval:
    call EvaluarExpresion
    jc .error

    mov eax,[parseResult]
    lea edi,[resultStr]
    call Int32ToStr

    invoke SetWindowText,[hEditRes],resultStr
    jmp .fin

.error:
    invoke SetWindowText,[hEditRes],strError

.fin:
    pop edi
    pop esi
    pop ebx
    ret
endp

; =========================================================
; Evalua [bufferCalc]. Resultado en [parseResult].
; CF=1 si error.
; =========================================================
proc EvaluarExpresion
    push esi
    push ebx
    push ecx
    push edx

    lea esi,[bufferCalc]

    ; --- Parsear numero izquierdo ---
    call ParseNumero
    jc .err
    mov eax,[parseResult]
    mov [numLeft],eax

    ; --- Operador o fin? ---
    mov al,[esi]
    cmp al,0
    je .solo_numero

    cmp al,'+'
    je .op_ok
    cmp al,'-'
    je .op_ok
    cmp al,'X'
    je .op_ok
    cmp al,'x'
    je .op_ok
    cmp al,'*'
    je .op_ok
    cmp al,'/'
    je .op_ok
    jmp .err

.op_ok:
    mov [theOp],al
    inc esi

    ; --- Parsear numero derecho ---
    call ParseNumero
    jc .err

    ; Debe terminar aca
    cmp byte [esi],0
    jne .err

    ; --- Computar ---
    mov eax,[numLeft]
    mov ebx,[parseResult]
    mov cl,[theOp]

    cmp cl,'+'
    je .op_add
    cmp cl,'-'
    je .op_sub
    cmp cl,'X'
    je .op_mul
    cmp cl,'x'
    je .op_mul
    cmp cl,'*'
    je .op_mul
    cmp cl,'/'
    je .op_div
    jmp .err

.op_add:
    add eax,ebx
    jo .err
    jmp .done

.op_sub:
    sub eax,ebx
    jo .err
    jmp .done

.op_mul:
    ; eax * ebx (ambos ×100) → dividir por 100
    imul ebx        ; edx:eax = producto
    mov ebx,100
    idiv ebx        ; eax = cociente (×100)
    jmp .done

.op_div:
    cmp ebx,0
    je .err
    ; (eax × 100) / ebx
    mov ecx,100
    imul ecx        ; edx:eax = eax*100
    idiv ebx        ; eax = cociente (×100)
    jmp .done

.done:
    mov [parseResult],eax
    clc
    jmp .fin

.solo_numero:
    mov eax,[numLeft]
    mov [parseResult],eax
    clc
    jmp .fin

.err:
    stc

.fin:
    pop edx
    pop ecx
    pop ebx
    pop esi
    ret
endp

; =========================================================
; ParseNumero
; Entrada: esi -> inicio del numero
; Salida : [parseResult] = int32 escalado ×100
;          esi apunta al caracter siguiente (despues del numero y %)
;          CF=1 si error
; =========================================================
proc ParseNumero
    push ebx
    push ecx
    push edx
    push eax

    xor ebx,ebx
    mov dword [numSigno],0

    ; Signo
    cmp byte [esi],'-'
    jne .no_signo
    mov dword [numSigno],1
    inc esi
.no_signo:

    ; Debe haber digito
    mov al,[esi]
    cmp al,'0'
    jb .err
    cmp al,'9'
    ja .err

.read_int:
    mov al,[esi]
    cmp al,'0'
    jb .int_done
    cmp al,'9'
    ja .int_done
    imul ebx,ebx,10
    jo .err
    sub al,'0'
    movzx edx,al
    add ebx,edx
    jo .err
    inc esi
    jmp .read_int

.int_done:
    cmp byte [esi],'.'
    jne .no_frac
    inc esi

    ; Debe haber digito despues del punto
    mov al,[esi]
    cmp al,'0'
    jb .err
    cmp al,'9'
    ja .err

    xor ecx,ecx        ; contador fracional
.read_frac:
    mov al,[esi]
    cmp al,'0'
    jb .frac_done
    cmp al,'9'
    ja .frac_done
    cmp ecx,2
    jae .frac_skip
    imul ebx,ebx,10
    jo .err
    sub al,'0'
    movzx edx,al
    add ebx,edx
    jo .err
    inc ecx
.frac_skip:
    inc esi
    jmp .read_frac
.frac_done:
    cmp ecx,2
    je .scaled
    cmp ecx,1
    jne .err
    imul ebx,ebx,10
    jo .err
    jmp .scaled

.no_frac:
    imul ebx,ebx,100
    jo .err

.scaled:
    ; Porcentaje
    cmp byte [esi],'%'
    jne .no_pct
    mov eax,ebx
    cdq
    mov ecx,100
    idiv ecx
    mov ebx,eax
    inc esi
.no_pct:

    ; Signo
    cmp dword [numSigno],1
    jne .positivo
    neg ebx
.positivo:

    mov [parseResult],ebx
    clc
    jmp .fin

.err:
    stc

.fin:
    pop eax
    pop edx
    pop ecx
    pop ebx
    ret
endp

; =========================================================
; Int32ToStr: eax (escalado ×100) -> string en edi
; Formato: entero, con decimales solo si hay
; =========================================================
proc Int32ToStr
    push eax
    push ebx
    push ecx
    push edx
    push esi

    test eax,eax
    jns .positivo
    mov byte [edi],'-'
    inc edi
    neg eax
.positivo:

    xor edx,edx
    mov ecx,100
    div ecx            ; eax = entero, edx = frac (0-99)
    push edx

    call ImprimirEntero

    pop eax            ; eax = frac

    test eax,eax
    jz .sin_frac

    mov byte [edi],'.'
    inc edi

    xor edx,edx
    mov ecx,10
    div ecx            ; eax = decena, edx = unidad
    test edx,edx
    jnz .dos_digitos

    add al,'0'
    mov [edi],al
    inc edi
    jmp .fin_frac

.dos_digitos:
    add al,'0'
    mov [edi],al
    inc edi
    add dl,'0'
    mov [edi],dl
    inc edi

.fin_frac:
.sin_frac:
    mov byte [edi],0

    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret
endp

; =========================================================
; ImprimirEntero: eax -> decimal en edi
; =========================================================
proc ImprimirEntero
    push eax
    push ebx
    push ecx
    push edx
    push esi

    mov esi,esp
    xor ecx,ecx
    mov ebx,10

.div_loop:
    xor edx,edx
    div ebx
    add dl,'0'
    push edx
    inc ecx
    test eax,eax
    jnz .div_loop

.pop_loop:
    pop eax
    mov [edi],al
    inc edi
    dec ecx
    jnz .pop_loop

    mov esp,esi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret
    
endp