; Constantes de validacion
MAX_DIGITOS     = 11
MAX_CARACTERES  = 30

proc ProcesarComando uses ebx esi edi, hwnd,wparam,lparam

    mov eax,[wparam]
    and eax,0FFFFh

    ; SALIR
    cmp eax,IDC_BTN_SALIR
    je .salir

    ; AC
    cmp eax,IDC_BTN_AC
    je .limpiar

    ; BORRAR
    cmp eax,IDC_BTN_BACK
    je .borrar

    ; '=' reservado para la logica
    cmp eax,IDC_BTN_IGUAL
    je .fin

    ; ignorar mensajes que no son de botones de entrada (1102..1142)
    cmp eax,IDC_BTN_PORC
    jb .fin
    cmp eax,IDC_BTN_PUNTO
    ja .fin

    ; =====================
    ; Boton que agrega texto
    ; =====================

    invoke GetWindowText,[hEditOp],buffer,127
    mov ebx,eax

    invoke GetWindowText,[lparam],btnText,15
    mov edx,eax

    ; --- Regla 1: limite total de caracteres ---
    mov eax,ebx
    add eax,edx
    cmp eax,MAX_CARACTERES
    ja .fin

    ; --- Regla 2: limite de 11 digitos totales ---
    call ContarDigitosBuffer
    mov ecx,eax
    call ContarDigitosBoton
    add ecx,eax
    cmp ecx,MAX_DIGITOS
    ja .fin

    ; --- Regla 3: punto, solo uno por numero y solo despues de digito ---
    call BotonEsPunto
    jnz .no_es_punto
    cmp ebx,0
    je .fin
    lea esi,[buffer]
    add esi,ebx
    dec esi
    mov al,[esi]
    cmp al,'0'
    jb .fin
    cmp al,'9'
    ja .fin
    call PuntoYaUsadoEnNumeroActual
    jz .fin
    jmp .agregar
.no_es_punto:

    ; --- Regla 4: operadores (+, -, X, /) ---
    call BotonEsOperador
    jnz .no_operador

    ; el boton es operador
    cmp ebx,0
    je .operador_inicio

    lea esi,[buffer]
    add esi,ebx
    dec esi
    mov al,[esi]

    call BotonEsMenos
    jz .es_menos

    ; ===== Boton es + X / =====
    ; ultimo debe ser digito o '%'
    cmp al,'0'
    jb .check_pct
    cmp al,'9'
    jbe .ok_bin
.check_pct:
    cmp al,'%'
    je .ok_bin
    jmp .fin
.ok_bin:
    call ContarOperadoresBinarios
    cmp eax,1
    jae .fin
    jmp .agregar

.es_menos:
    ; Boton '-'. al = ultimo caracter del buffer.
    cmp al,'-'
    je .fin                         ; no dos '-' seguidos
    cmp al,'.'
    je .fin                         ; '-' despues de punto incompleto
    cmp al,'%'
    je .minus_bin                   ; '%' antes de '-' -> binario
    call CaracterEsOperador
    jz .agregar                     ; ultimo + X / -> unario
.minus_bin:
    ; ultimo digito (o %) -> binario
    call ContarOperadoresBinarios
    cmp eax,1
    jae .fin
    jmp .agregar

.operador_inicio:
    ; buffer vacio: solo '-' permitido (negativo inicial)
    call BotonEsMenos
    jz .agregar
    jmp .fin

.no_operador:
    ; --- Regla 5: porcentaje ---
    call BotonEsPorcentaje
    jnz .no_porcentaje
    ; es '%'
    cmp ebx,0
    je .fin                         ; no empezar con %
    lea esi,[buffer]
    add esi,ebx
    dec esi
    mov al,[esi]
    cmp al,'0'
    jb .fin                         ; ultimo debe ser digito
    cmp al,'9'
    ja .fin
    call ContarPorcentajes
    cmp eax,1
    jae .fin                        ; solo un % por expresion
    jmp .agregar
.no_porcentaje:

    ; es digito. Si el ultimo es '%', bloquear (5%5 invalido).
    cmp ebx,0
    je .agregar
    lea esi,[buffer]
    add esi,ebx
    dec esi
    mov al,[esi]
    cmp al,'%'
    je .fin
    jmp .agregar

.agregar:
    mov eax,ebx
    add eax,edx
    cmp eax,127
    ja .fin

    lea edi,[buffer]
    add edi,ebx
    lea esi,[btnText]
    mov ecx,edx

.copiar:
    cmp ecx,0
    je .terminar
    mov al,[esi]
    mov [edi],al
    inc esi
    inc edi
    dec ecx
    jmp .copiar

.terminar:
    mov byte [edi],0
    invoke SetWindowText,[hEditOp],buffer
    jmp .fin

.limpiar:
    invoke SetWindowText,[hEditOp],Vacio
    invoke SetWindowText,[hEditRes],Vacio
    jmp .fin

.borrar:
    invoke GetWindowText,[hEditOp],buffer,127
    cmp eax,0
    je .fin
    lea edi,[buffer]
    add edi,eax
    dec edi
    mov byte [edi],0
    invoke SetWindowText,[hEditOp],buffer
    jmp .fin

.salir:
    invoke SendMessage,[hwnd],WM_CLOSE,0,0

.fin:
    ret

endp


; ===================== HELPERS =====================

proc ContarDigitosBuffer
    push esi
    push ecx
    lea esi,[buffer]
    xor eax,eax
.ciclo:
    mov cl,[esi]
    cmp cl,0
    je .fin
    cmp cl,'0'
    jb .sig
    cmp cl,'9'
    ja .sig
    inc eax
.sig:
    inc esi
    jmp .ciclo
.fin:
    pop ecx
    pop esi
    ret
endp

proc ContarDigitosBoton
    push esi
    push ecx
    push edx
    lea esi,[btnText]
    mov ecx,edx
    xor eax,eax
.ciclo:
    cmp ecx,0
    je .fin
    mov dl,[esi]
    cmp dl,'0'
    jb .sig
    cmp dl,'9'
    ja .sig
    inc eax
.sig:
    inc esi
    dec ecx
    jmp .ciclo
.fin:
    pop edx
    pop ecx
    pop esi
    ret
endp

; ZF=1 si el primer caracter de btnText es '.'
proc BotonEsPunto
    push eax
    push esi
    lea esi,[btnText]
    mov al,[esi]
    cmp al,'.'
    pop esi
    pop eax
    ret
endp

; ZF=1 si el primer caracter de btnText es '-'
proc BotonEsMenos
    push eax
    push esi
    lea esi,[btnText]
    mov al,[esi]
    cmp al,'-'
    pop esi
    pop eax
    ret
endp

; ZF=1 si el primer caracter de btnText es '+' '-' 'X' 'x' '*' '/'
proc BotonEsOperador
    push eax
    push esi
    lea esi,[btnText]
    mov al,[esi]
    cmp al,'+'
    je .si
    cmp al,'-'
    je .si
    cmp al,'X'
    je .si
    cmp al,'x'
    je .si
    cmp al,'*'
    je .si
    cmp al,'/'
    je .si
    pop esi
    pop eax
    ret
.si:
    pop esi
    pop eax
    ret
endp

; ZF=1 si el primer caracter de btnText es '%'
proc BotonEsPorcentaje
    push eax
    push esi
    lea esi,[btnText]
    mov al,[esi]
    cmp al,'%'
    pop esi
    pop eax
    ret
endp

; ZF=1 si AL es '+' '-' 'X' 'x' '*' '/'
proc CaracterEsOperador
    cmp al,'+'
    je .si
    cmp al,'-'
    je .si
    cmp al,'X'
    je .si
    cmp al,'x'
    je .si
    cmp al,'*'
    je .si
    cmp al,'/'
    je .si
    ret
.si:
    ret
endp

; ZF=1 si el numero actual (desde el ultimo operador hasta el final del buffer)
; ya contiene un punto. Usa ebx = longitud del buffer.
proc PuntoYaUsadoEnNumeroActual
    push esi
    push ecx
    push eax
    mov ecx,ebx
    cmp ecx,0
    je .no_punto
    lea esi,[buffer]
    add esi,ecx
    dec esi
.loop:
    mov al,[esi]
    cmp al,'.'
    je .encontrado_punto
    cmp al,'+'
    je .fin_numero
    cmp al,'-'
    je .fin_numero
    cmp al,'X'
    je .fin_numero
    cmp al,'x'
    je .fin_numero
    cmp al,'*'
    je .fin_numero
    cmp al,'/'
    je .fin_numero
    cmp al,'%'
    je .fin_numero
    dec esi
    dec ecx
    jnz .loop
    jmp .no_punto
.encontrado_punto:
    pop eax
    pop ecx
    pop esi
    xor al,al
    ret
.fin_numero:
.no_punto:
    pop eax
    pop ecx
    pop esi
    or  al,1
    ret
endp

; Cuenta operadores BINARIOS en [buffer] (largo en ebx).
; '-' cuenta como binario solo si el anterior es digito, '.' o '%'.
; '%' NO cuenta como binario.
proc ContarOperadoresBinarios
    push esi
    push ecx
    push edx
    push edi
    lea esi,[buffer]
    mov ecx,ebx
    xor eax,eax
    xor edi,edi
.loop:
    test ecx,ecx
    jz .fin
    mov dl,[esi]
    cmp dl,'0'
    jb .maybe_point
    cmp dl,'9'
    jbe .is_operand
.maybe_point:
    cmp dl,'.'
    je .is_operand
    cmp dl,'%'
    je .is_operand
    cmp dl,'+'
    je .is_bin
    cmp dl,'X'
    je .is_bin
    cmp dl,'x'
    je .is_bin
    cmp dl,'*'
    je .is_bin
    cmp dl,'/'
    je .is_bin
    cmp dl,'-'
    je .check_minus
    xor edi,edi
    jmp .next
.is_operand:
    mov edi,1
    jmp .next
.is_bin:
    inc eax
    xor edi,edi
    jmp .next
.check_minus:
    test edi,edi
    jz .minus_unary
    inc eax
.minus_unary:
    xor edi,edi
    jmp .next
.next:
    inc esi
    dec ecx
    jmp .loop
.fin:
    pop edi
    pop edx
    pop ecx
    pop esi
    ret
endp

; Cuenta '%' en [buffer] (largo en ebx). Salida: eax
proc ContarPorcentajes
    push esi
    push ecx
    push edx
    lea esi,[buffer]
    mov ecx,ebx
    xor eax,eax
.loop:
    test ecx,ecx
    jz .fin
    mov dl,[esi]
    cmp dl,'%'
    jne .next
    inc eax
.next:
    inc esi
    dec ecx
    jmp .loop
.fin:
    pop edx
    pop ecx
    pop esi
    ret
endp


; WndProc del edit: bloquea TODO el teclado (entrada solo por botones)
proc EditSinTeclado hwnd,wmsg,wparam,lparam
    cmp [wmsg],WM_CHAR
    je .bloquear
    cmp [wmsg],WM_KEYDOWN
    je .bloquear
    cmp [wmsg],WM_KEYUP
    je .bloquear
    cmp [wmsg],WM_PASTE
    je .bloquear
    cmp [wmsg],WM_CUT
    je .bloquear
    cmp [wmsg],WM_CLEAR
    je .bloquear
    invoke CallWindowProc,[hEditOpOldProc],[hwnd],[wmsg],[wparam],[lparam]
    ret
.bloquear:
    xor eax,eax
    ret
endp